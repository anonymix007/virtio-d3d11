#define INITGUID
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winddk_compat.h>
#include <d3d9.h>
#include <virtio_wddm_uapi.h>

#include <stdatomic.h>
#include <stdio.h>

#undef ERROR
#include "adapter.h"
#include "device.h"
#include "dxgi.h"
#include "resource.h"

/*
Presentable formats:
DXGI_FORMAT_B5G6R5_UNORM => VK_FORMAT_B5G6R5_UNORM_PACK16 => PIPE_FORMAT_R5G6B5_UNORM
DXGI_FORMAT_B5G5R5A1_UNORM => VK_FORMAT_B5G5R5A1_UNORM_PACK16 => PIPE_FORMAT_A1R5G5B5_UNORM
DXGI_FORMAT_B8G8R8A8_UNORM => VK_FORMAT_B8G8R8A8_UNORM => PIPE_FORMAT_B8G8R8A8_UNORM
DXGI_FORMAT_B8G8R8X8_UNORM => VK_FORMAT_B8G8R8A8_UNORM => PIPE_FORMAT_B8G8R8A8_UNORM
DXGI_FORMAT_R16G16B16A16_FLOAT => VK_FORMAT_R16G16B16A16_SFLOAT => PIPE_FORMAT_R16G16B16A16_FLOAT
DXGI_FORMAT_R10G10B10A2_UNORM => VK_FORMAT_A2B10G10R10_UNORM_PACK32 => PIPE_FORMAT_R10G10B10A2_UNORM
DXGI_FORMAT_R8G8B8A8_UNORM => VK_FORMAT_R8G8B8A8_UNORM => PIPE_FORMAT_R8G8B8A8_UNORM
DXGI_FORMAT_R8G8B8A8_UNORM_SRGB => VK_FORMAT_R8G8B8A8_SRGB => PIPE_FORMAT_R8G8B8A8_SRGB
*/

static HRESULT virtio_wddm_sync_with_present_context(VIRTIO_WDDM_Device *device) {
    uint64_t value = atomic_fetch_add_explicit((volatile uint64_t *) &device->base.presentFenceValue, 1, memory_order_acq_rel);

    HRESULT hr = ID3D11DeviceContext4_Signal(device->base.pCtx4, device->base.pPresentFence, value);
    if (FAILED(hr)) {
        ERROR("%s: Failed to signal present fence: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }

    ID3D11DeviceContext1_Flush(device->base.pCtx1);

#if 0
    HANDLE event = CreateEventA(NULL, true, false, NULL);
    hr = ID3D11Fence_SetEventOnCompletion(device->base.pPresentFence, value, event);
    if (FAILED(hr)) {
        ERROR("%s: Failed to set event on present fence completion: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }

    WaitForSingleObject(event, INFINITE);
#else
    // NOTE: the following requires DXVK to submit synchronously in ID3D11DeviceContext1_Flush
    // because pfnPresentCb waits for pfnSignalSychronizationObjectFromGpu to be submitted to
    // kernel with the internal dxgkrnl lock is taken
    D3DDDICB_WAITFORSYNCHRONIZATIONOBJECTFROMGPU wait = {
        .hContext = device->present.context,
        .ObjectCount = 1,
        .ObjectHandleArray = &device->present.fence,
        .MonitoredFenceValueArray = &value,
    };
    hr = device->base.KTCallbacks.pfnWaitForSynchronizationObjectFromGpuCb(device->base.hRTDevice.handle, &wait);
    if (FAILED(hr)) {
        ERROR("%s: Failed to wait from gpu: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }
#endif
    return S_OK;
}

HRESULT APIENTRY virtio_wddm_present(DXGI_DDI_ARG_PRESENT *pArgs) {
    TRACE();
    VIRTIO_WDDM_Device *device = (void *) pArgs->hDevice;
    VIRTIO_WDDM_Resource *src = (void *) pArgs->hSurfaceToPresent;
    HRESULT hr = S_OK;

    // Looks like Blt could be zero for primaries
    //ASSERT(pArgs->Flags.Blt);
    ASSERT(pArgs->hDstResource == 0);
    ASSERT(src->base.hKMAllocation != 0);

    if (false) {
        char name[256];
        snprintf(name, sizeof(name), "present-%llu.ppm", device->base.presentFenceValue);
        virtio_wddm_save_texture_to_ppm(device, src, name);
    }

    hr = virtio_wddm_sync_with_present_context(device);
    if (FAILED(hr)) {
        ERROR("%s: Failed to flush: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }

    DXGIDDICB_PRESENT present = {
        .hSrcAllocation = src->base.hKMAllocation,
        .hDstAllocation = 0,
        .pDXGIContext = pArgs->pDXGIContext,
        .hContext = device->present.context,
    };
    hr = device->dxgi_callbacks->pfnPresentCb(device->base.hRTDevice.handle, &present);
    if (FAILED(hr)) {
        ERROR("%s: Failed to present: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }

    return S_OK;
}

HRESULT APIENTRY virtio_wddm_get_gamma_caps(DXGI_DDI_ARG_GET_GAMMA_CONTROL_CAPS *pArgs) {
    TRACE();
    pArgs->pGammaCapabilities->ScaleAndOffsetSupported = false;
    pArgs->pGammaCapabilities->MinConvertedValue = 0.f;
    pArgs->pGammaCapabilities->MaxConvertedValue = 1.f;
    pArgs->pGammaCapabilities->NumGammaControlPoints = 17;

    for (size_t i = 0; i < pArgs->pGammaCapabilities->NumGammaControlPoints; i++) {
        pArgs->pGammaCapabilities->ControlPointPositions[i] = i / (float) (pArgs->pGammaCapabilities->NumGammaControlPoints - 1);
    }

    return S_OK;
}

HRESULT APIENTRY virtio_wddm_set_display_mode(DXGI_DDI_ARG_SETDISPLAYMODE *pArgs) {
    TRACE();
    VIRTIO_WDDM_Device *device = (void *) pArgs->hDevice;
    VIRTIO_WDDM_Resource *resource = (void *) pArgs->hResource;

    ASSERT(resource->base.hKMAllocation != 0);
    ASSERT(resource->base.IsPresentable);

    D3DDDICB_SETDISPLAYMODE set_mode = {
        .hPrimaryAllocation = resource->base.hKMAllocation,
        .PrivateDriverFormatAttribute = 0,
    };

    HRESULT hr = device->base.KTCallbacks.pfnSetDisplayModeCb(device->base.hRTDevice.handle, &set_mode);
    if (FAILED(hr)) {
        ERROR("%s: failed to set display mode: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }
    return S_OK;
}

HRESULT APIENTRY virtio_wddm_set_resource_priority(DXGI_DDI_ARG_SETRESOURCEPRIORITY *pArgs) {
    TRACE();
    return S_OK;
}

HRESULT APIENTRY virtio_wddm_query_resource_residency(DXGI_DDI_ARG_QUERYRESOURCERESIDENCY *pArgs) {
    TRACE();
    HRESULT hr = S_OK;
    VIRTIO_WDDM_Device *device = (void *) pArgs->hDevice;

    for (size_t i = 0; i < pArgs->Resources; i++) {
        VIRTIO_WDDM_Resource *resource = (void *) pArgs->pResources[i];

        if (resource->base.hKMAllocation == 0) {
            pArgs->pStatus[i] = DXGI_DDI_RESIDENCY_FULLY_RESIDENT;
            continue;
        }

        D3DDDI_RESIDENCYSTATUS status;
        {
            D3DDDICB_QUERYRESIDENCY query = {
                .NumAllocations = 1,
                .HandleList = &resource->base.hKMAllocation,
                .pResidencyStatus = &status,
            };
            HRESULT hr = device->base.KTCallbacks.pfnQueryResidencyCb(device->base.hRTDevice.handle, &query);
            if (FAILED(hr)) {
                ERROR("%s: query residency failed: 0x%08lx", __FUNCTION__, hr);
                return hr;
            }
        }

        switch (status) {
            case D3DDDI_RESIDENCYSTATUS_RESIDENTINGPUMEMORY:
                pArgs->pStatus[i] = DXGI_DDI_RESIDENCY_FULLY_RESIDENT;
                break;
            case D3DDDI_RESIDENCYSTATUS_RESIDENTINSHAREDMEMORY:
                pArgs->pStatus[i] = DXGI_DDI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY;
                hr = S_RESIDENT_IN_SHARED_MEMORY;
                break;
            case D3DDDI_RESIDENCYSTATUS_NOTRESIDENT:
                pArgs->pStatus[i] = DXGI_DDI_RESIDENCY_EVICTED_TO_DISK;
                hr = S_NOT_RESIDENT;
                break;
        }
    }

    return hr;
}

HRESULT APIENTRY virtio_wddm_rotate_resource_identities(DXGI_DDI_ARG_ROTATE_RESOURCE_IDENTITIES *pArgs) {
    TRACE();
    if (pArgs->Resources <= 1) {
        return S_OK;
    }

    VIRTIO_WDDM_Device *device = (void *) pArgs->hDevice;
    VIRTIO_WDDM_Resource first = *(VIRTIO_WDDM_Resource *) pArgs->pResources[0];

    for (size_t i = 0; i < pArgs->Resources - 1; i++) {
        VIRTIO_WDDM_Resource *curr = (void *) pArgs->pResources[i];
        VIRTIO_WDDM_Resource *next = (void *) pArgs->pResources[i + 1];
        D3D10DDI_HRTRESOURCE res = curr->base.hRTResource;
        TRITON_VIEWLINK *views = curr->base.pViewList;
        *curr = *next;
        curr->base.hRTResource = res;
        curr->base.pViewList = views;
    }

    VIRTIO_WDDM_Resource *last = (void *) pArgs->pResources[pArgs->Resources - 1];
    D3D10DDI_HRTRESOURCE res = last->base.hRTResource;
    TRITON_VIEWLINK *views = last->base.pViewList;
    *last = first;
    last->base.hRTResource = res;
    last->base.pViewList = views;

    for (size_t i = 0; i < pArgs->Resources; i++) {
        VIRTIO_WDDM_Resource *resource = (void *) pArgs->pResources[i];
        tritonResourceRecreateViews(&device->base, &resource->base);
    }

    device->base.pUMCallbacks->pfnStateVsSrvCb(device->base.hRTCoreLayer, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
    device->base.pUMCallbacks->pfnStateGsSrvCb(device->base.hRTCoreLayer, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
    device->base.pUMCallbacks->pfnStatePsSrvCb(device->base.hRTCoreLayer, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
    if (device->base.uIfVersion >= D3D11_0_DDI_INTERFACE_VERSION) {
        device->base.pUMCallbacks->pfnStateHsSrvCb(device->base.hRTCoreLayer, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        device->base.pUMCallbacks->pfnStateDsSrvCb(device->base.hRTCoreLayer, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        device->base.pUMCallbacks->pfnStateCsSrvCb(device->base.hRTCoreLayer, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        device->base.pUMCallbacks->pfnStateCsUavCb(device->base.hRTCoreLayer, 0, D3D11_PS_CS_UAV_REGISTER_COUNT);
    }
    device->base.pUMCallbacks->pfnStateOmRenderTargetsCb(device->base.hRTCoreLayer);

    return S_OK;
}

HRESULT APIENTRY virtio_wddm_blt(DXGI_DDI_ARG_BLT *pArgs) {
    TRACE();
    VIRTIO_WDDM_Resource *src = (void *) pArgs->hSrcResource;
    DXGI_DDI_ARG_BLT1 args = {
        .hDevice = pArgs->hDevice,
        .hDstResource = pArgs->hDstResource,
        .DstSubresource = pArgs->DstSubresource,
        .DstLeft = pArgs->DstLeft,
        .DstTop = pArgs->DstTop,
        .DstRight = pArgs->DstRight,
        .DstBottom = pArgs->DstBottom,
        .hSrcResource = pArgs->hSrcResource,
        .SrcSubresource = pArgs->SrcSubresource,
        .SrcLeft = 0,
        .SrcTop = 0,
        .SrcRight = src->base.Width,
        .SrcBottom = src->base.Height,
        .Flags = pArgs->Flags,
        .Rotate = pArgs->Rotate,
    };
    return virtio_wddm_blt1(&args);
}

HRESULT APIENTRY virtio_wddm_resolve_shared_resource(DXGI_DDI_ARG_RESOLVESHAREDRESOURCE *pArgs) {
    TRACE();

    VIRTIO_WDDM_Device *device = (void *) pArgs->hDevice;
    /* MSDN says we should flush, so let's flush */
    ID3D11DeviceContext1_Flush(device->base.pCtx1);

    return S_OK;
}

HRESULT APIENTRY virtio_wddm_blt1(DXGI_DDI_ARG_BLT1 *pArgs) {
    TRACE();

    VIRTIO_WDDM_Device *device = (void *) pArgs->hDevice;
    VIRTIO_WDDM_Resource *dst = (void *) pArgs->hDstResource;
    VIRTIO_WDDM_Resource *src = (void *) pArgs->hSrcResource;

    ASSERT(pArgs->Rotate == DXGI_DDI_MODE_ROTATION_IDENTITY);

    if (pArgs->Flags.Resolve) {
        ID3D11DeviceContext1_ResolveSubresource(
            device->base.pCtx1,
            dst->base.pResource, pArgs->DstSubresource,
            src->base.pResource, pArgs->SrcSubresource,
            dst->base.Format
        );
    }

    if (pArgs->Flags.Stretch || pArgs->Flags.Convert) {
        TODO("Stretch/shrink or convert format");
    } else {
        D3D11_BOX src_box = {
            .left = pArgs->SrcLeft,
            .top = pArgs->SrcTop,
            .front = 0,
            .right = pArgs->SrcRight,
            .bottom = pArgs->SrcBottom,
            .back = 1,
        };

        ID3D11DeviceContext1_CopySubresourceRegion(
            device->base.pCtx1,
            dst->base.pResource, pArgs->DstSubresource, pArgs->DstLeft, pArgs->DstTop, 0,
            src->base.pResource, pArgs->SrcSubresource, &src_box
        );
    }

    if (pArgs->Flags.Present) {
        HRESULT hr = virtio_wddm_sync_with_present_context(device);
        if (FAILED(hr)) {
            ERROR("%s: Failed to flush: 0x%08lx", __FUNCTION__, hr);
            return hr;
        }
    }

    return S_OK;
}

HRESULT APIENTRY virtio_wddm_offer_resources(DXGI_DDI_ARG_OFFERRESOURCES *pArgs) {
    TRACE();
    return S_OK;
}

HRESULT APIENTRY virtio_wddm_reclaim_resources(DXGI_DDI_ARG_RECLAIMRESOURCES *pArgs) {
    TRACE();
    for (size_t i = 0; i < pArgs->Resources; i++) {
        pArgs->pDiscarded[i] = false;
    }

    return S_OK;
}
