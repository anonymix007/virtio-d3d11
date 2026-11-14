#include <d3d11_fence.h>
#include <dxgi_interfaces.h>
#include <dxgi_factory.h>
#include <dxgi_adapter.h>

extern "C" {
#include "dxvk.h"

HRESULT DXVK_CreateDXGIFactory(Vulkan_Instance_Info *instance, REFIID riid, void **ppFactory) {
    try {
        dxvk::DxvkInstanceImportInfo args { };
        args.loaderProc = instance->loader_proc;
        args.instance = instance->instance;
        args.extensionCount = instance->extension_count;
        args.extensionNames = instance->extension_names;
        dxvk::Com<dxvk::DxgiFactory> factory = new dxvk::DxgiFactory(args, 0);
        HRESULT hr = factory->QueryInterface(riid, ppFactory);
        if (FAILED(hr)) return hr;
        return S_OK;
    } catch (const dxvk::DxvkError& e) {
        dxvk::Logger::err(e.message());
        return E_FAIL;
    }
}

VkSemaphore DXVK_ID3D11Fence_GetVkSemaphore(ID3D11Fence *fence) {
    dxvk::D3D11Fence *d3d11_fence = static_cast<dxvk::D3D11Fence *>(fence);
    if (!d3d11_fence) {
        return VK_NULL_HANDLE;
    }

    return d3d11_fence->GetFence()->handle();
}

HRESULT DXVK_ID3D11Device_QueryInterface_IDXGIVkInteropDevice1(ID3D11Device *device, IUnknown **dxvk_interop) {
    IDXGIVkInteropDevice1 *interop_device;
    HRESULT hr = device->QueryInterface(__uuidof(IDXGIVkInteropDevice1), (void **) &interop_device);
    if (FAILED(hr)) {
        return hr;
    }
    *dxvk_interop = static_cast<IUnknown *>(interop_device);
    return S_OK;
}

void DXVK_IDXGIVkInteropDevice1_GetVulkanHandles(IUnknown *dxvk_interop, VkInstance *pInstance, VkPhysicalDevice *pPhysDev, VkDevice *pDevice) {
    auto interop_device = static_cast<IDXGIVkInteropDevice1 *>(dxvk_interop);
    interop_device->GetVulkanHandles(pInstance, pPhysDev, pDevice);
}

HRESULT DXVK_IDXGIVkInteropDevice1_CreateTexture2DFromVkImage(IUnknown *dxvk_interop, const D3D11_TEXTURE2D_DESC1* pDesc, VkImage vkImage, ID3D11Texture2D **ppTexture2D) {
    auto interop_device = static_cast<IDXGIVkInteropDevice1 *>(dxvk_interop);
    return interop_device->CreateTexture2DFromVkImage(pDesc, vkImage, ppTexture2D);
}

void DXVK_IDXGIVkInteropDevice1_FlushRenderingCommands(IUnknown *dxvk_interop) {
    auto interop_device = static_cast<IDXGIVkInteropDevice1 *>(dxvk_interop);
    interop_device->FlushRenderingCommands();
}

}
