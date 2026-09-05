#define INITGUID
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winddk_compat.h>
#include <virtio_wddm_uapi.h>

#include <drm_hw.h>

#undef ERROR
#include "adapter.h"
#include "device.h"
#include "dxgi.h"
#include "resource.h"

#include "dxvk.h"

SIZE_T APIENTRY virtio_wddm_calc_device_size(D3D10DDI_HADAPTER hAdapter, const D3D10DDIARG_CALCPRIVATEDEVICESIZE *pArgs) {
    return sizeof(VIRTIO_WDDM_Device);
}

static inline void free_d3d11_device(void *ptr) {
    ID3D11Device_Release(*(ID3D11Device **)ptr);
}

static inline void free_d3d11_device_context(void *ptr) {
    ID3D11DeviceContext_Release(*(ID3D11DeviceContext **)ptr);
}

extern const char *vk_result_to_str(VkResult result) {
#define CASE_VK_RESULT(name) case VK_##name: return #name;
    switch (result) {
        CASE_VK_RESULT(SUCCESS)
        CASE_VK_RESULT(NOT_READY)
        CASE_VK_RESULT(TIMEOUT)
        CASE_VK_RESULT(EVENT_SET)
        CASE_VK_RESULT(EVENT_RESET)
        CASE_VK_RESULT(INCOMPLETE)
        CASE_VK_RESULT(ERROR_OUT_OF_HOST_MEMORY)
        CASE_VK_RESULT(ERROR_OUT_OF_DEVICE_MEMORY)
        CASE_VK_RESULT(ERROR_INITIALIZATION_FAILED)
        CASE_VK_RESULT(ERROR_DEVICE_LOST)
        CASE_VK_RESULT(ERROR_MEMORY_MAP_FAILED)
        CASE_VK_RESULT(ERROR_LAYER_NOT_PRESENT)
        CASE_VK_RESULT(ERROR_EXTENSION_NOT_PRESENT)
        CASE_VK_RESULT(ERROR_FEATURE_NOT_PRESENT)
        CASE_VK_RESULT(ERROR_INCOMPATIBLE_DRIVER)
        CASE_VK_RESULT(ERROR_TOO_MANY_OBJECTS)
        CASE_VK_RESULT(ERROR_FORMAT_NOT_SUPPORTED)
        CASE_VK_RESULT(ERROR_FRAGMENTED_POOL)
        CASE_VK_RESULT(ERROR_UNKNOWN)
        CASE_VK_RESULT(ERROR_VALIDATION_FAILED)
        CASE_VK_RESULT(ERROR_OUT_OF_POOL_MEMORY)
        CASE_VK_RESULT(ERROR_INVALID_EXTERNAL_HANDLE)
        CASE_VK_RESULT(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
        CASE_VK_RESULT(ERROR_FRAGMENTATION)
        CASE_VK_RESULT(PIPELINE_COMPILE_REQUIRED)
        CASE_VK_RESULT(ERROR_NOT_PERMITTED)
        CASE_VK_RESULT(ERROR_SURFACE_LOST_KHR)
        CASE_VK_RESULT(ERROR_NATIVE_WINDOW_IN_USE_KHR)
        CASE_VK_RESULT(SUBOPTIMAL_KHR)
        CASE_VK_RESULT(ERROR_OUT_OF_DATE_KHR)
        CASE_VK_RESULT(ERROR_INCOMPATIBLE_DISPLAY_KHR)
        CASE_VK_RESULT(ERROR_INVALID_SHADER_NV)
        CASE_VK_RESULT(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
        CASE_VK_RESULT(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
        CASE_VK_RESULT(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
        CASE_VK_RESULT(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
        CASE_VK_RESULT(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
        CASE_VK_RESULT(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
        CASE_VK_RESULT(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
        CASE_VK_RESULT(ERROR_PRESENT_TIMING_QUEUE_FULL_EXT)
        CASE_VK_RESULT(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
        CASE_VK_RESULT(THREAD_IDLE_KHR)
        CASE_VK_RESULT(THREAD_DONE_KHR)
        CASE_VK_RESULT(OPERATION_DEFERRED_KHR)
        CASE_VK_RESULT(OPERATION_NOT_DEFERRED_KHR)
        CASE_VK_RESULT(ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)
        CASE_VK_RESULT(ERROR_COMPRESSION_EXHAUSTED_EXT)
        CASE_VK_RESULT(INCOMPATIBLE_SHADER_BINARY_EXT)
        CASE_VK_RESULT(PIPELINE_BINARY_MISSING_KHR)
        CASE_VK_RESULT(ERROR_NOT_ENOUGH_SPACE_KHR)
    }
#undef CASE_VK_RESULT
    ERROR("Unknown VkResult: %d", result);
    return "unknown";
}

static inline const char *d3d10_ddi_interface_version_name(unsigned interface_) {
#define CASE_INTERFACE_VERSION(name) case name##_DDI_INTERFACE_VERSION: return #name;
    switch (interface_) {
        CASE_INTERFACE_VERSION(D3D10_0)
        CASE_INTERFACE_VERSION(D3D10_1)
        CASE_INTERFACE_VERSION(D3D11_0)
        CASE_INTERFACE_VERSION(D3D10on9)
        CASE_INTERFACE_VERSION(D3D10_0_x)
        CASE_INTERFACE_VERSION(D3D10_0_x_vista)
        CASE_INTERFACE_VERSION(D3D10_1_x)
        CASE_INTERFACE_VERSION(D3D10_1_x_vista)
        CASE_INTERFACE_VERSION(D3D10_0_7)
        CASE_INTERFACE_VERSION(D3D10_1_7)
        CASE_INTERFACE_VERSION(D3D11_0_7)
        CASE_INTERFACE_VERSION(D3D11_0_vista)
        CASE_INTERFACE_VERSION(D3D11_1)
        CASE_INTERFACE_VERSION(D3DWDDM1_3)
        CASE_INTERFACE_VERSION(D3DWDDM2_0)
        CASE_INTERFACE_VERSION(D3DWDDM2_1)
        CASE_INTERFACE_VERSION(D3DWDDM2_2)
        CASE_INTERFACE_VERSION(D3DWDDM2_3)
        CASE_INTERFACE_VERSION(D3DWDDM2_4)
        CASE_INTERFACE_VERSION(D3DWDDM2_5)
        CASE_INTERFACE_VERSION(D3DWDDM2_6)
        CASE_INTERFACE_VERSION(D3DWDDM2_7)
        CASE_INTERFACE_VERSION(D3DWDDM2_8)
        CASE_INTERFACE_VERSION(D3DWDDM2_9)
        CASE_INTERFACE_VERSION(D3DWDDM3_0)
        CASE_INTERFACE_VERSION(D3DWDDM3_1)
        CASE_INTERFACE_VERSION(D3DWDDM3_2)
    }
#undef CASE_INTERFACE_VERSION
    ERROR("Unknown interface version: %x", interface_);
    return "unknown";
}

static const char *dxgi_ddi_interface_version_name(unsigned interface_, unsigned version) {
    if (IS_DXGI1_6_1_BASE_FUNCTIONS(interface_, version)) {
        return "DXGI1.6.1";
    } else if (IS_DXGI1_6_BASE_FUNCTIONS(interface_, version)) {
        return "DXGI1.6";
    } else if (IS_DXGI1_5_BASE_FUNCTIONS(interface_, version)) {
        return "DXGI1.5";
    } else if (IS_DXGI1_4_BASE_FUNCTIONS(interface_, version)) {
        return "DXGI1.4";
    } else if (IS_DXGI1_3_BASE_FUNCTIONS(interface_, version)) {
        return "DXGI1.3";
    } /* else if (IS_DXGI_MULTIPLANE_OVERLAY_FUNCTIONS(interface_, version)) {
        return "DXGI MULTIPLANE OVERLAY";
    } */ else if (IS_DXGI1_2_BASE_FUNCTIONS(interface_, version)) {
        return "DXGI1.2";
    } else if (IS_DXGI1_1_BASE_FUNCTIONS(interface_, version)) {
        return "DXGI1.1";
    } else {
        return "DXGI1.0";
    }
}

static void APIENTRY virtio_wddm_destroy_device(D3D10DDI_HDEVICE hDevice)
{
    TRACE();
    VIRTIO_WDDM_Device *device = hDevice.pDrvPrivate;

    INFO("%s: destroying device %p / %p", __FUNCTION__, device->callbacks.hRTDevice, device->base.hRTDevice.handle);

    // FIXME: ensure that DXVK never tries to submit any more commands

    if (device->present.context != NULL) {
        D3DDDICB_DESTROYCONTEXT context = {
            .hContext = device->present.context,
        };
        HRESULT hr = device->base.KTCallbacks.pfnDestroyContextCb(device->base.hRTDevice.handle, &context);
        if (FAILED(hr)) {
            ERROR("%s: Failed to destroy present context: 0x%08lx", __FUNCTION__, hr);
        }
    }

    if (device->paging.queue != 0) {
        D3DDDI_DESTROYPAGINGQUEUE paging_queue = {
            .hPagingQueue = device->paging.queue,
        };
        HRESULT hr = device->base.KTCallbacks.pfnDestroyPagingQueueCb(device->base.hRTDevice.handle, &paging_queue);
        if (FAILED(hr)) {
            ERROR("%s: Failed to destroy paging queue: 0x%08lx", __FUNCTION__, hr);
        }
    }

    ID3D11Fence_Release(device->base.pPresentFence);

    ID3D11DeviceContext1_Flush(device->base.pCtx1);

    ID3D11DeviceContext1_Release(device->base.pCtx1);
    ID3D11DeviceContext2_Release(device->base.pCtx2);
    ID3D11DeviceContext3_Release(device->base.pCtx3);
    ID3D11DeviceContext4_Release(device->base.pCtx4);

    ID3D11Device1_Release(device->base.pDev1);
    ID3D11Device2_Release(device->base.pDev2);
    ID3D11Device3_Release(device->base.pDev3);
    ID3D11Device5_Release(device->base.pDev5);

    IDXGIAdapter_Release(device->adapter);

    device->vk_DestroyInstance(device->vk_inst, NULL);

    FreeLibrary(device->vulkan);
    FreeLibrary(device->icd);

    memset(device, 0, sizeof(*device));
}

#define CLEANUP_D3D11_DEVICE __attribute__((cleanup(free_d3d11_device)))
#define CLEANUP_D3D11_CONTEXT __attribute__((cleanup(free_d3d11_device_context)))

static HRESULT create_present_context(VIRTIO_WDDM_Device *device) {
#if 1
    D3DDDICB_CREATECONTEXT context = {};
    HRESULT hr = device->base.KTCallbacks.pfnCreateContextCb(device->base.hRTDevice.handle, &context);
    if (FAILED(hr)) {
        return hr;
    }
#else
    D3DDDICB_CREATECONTEXTVIRTUAL context = {
        .NodeOrdinal = 2,
    };
    HRESULT hr = device->base.KTCallbacks.pfnCreateContextVirtualCb(device->base.hRTDevice.handle, &context);
    if (FAILED(hr)) {
        return hr;
    }
#endif

    device->present.context = context.hContext;
    return S_OK;
}

static HRESULT create_paging_queue(VIRTIO_WDDM_Device *device) {
    D3DDDICB_CREATEPAGINGQUEUE paging_queue = {
        .Priority = D3DDDI_PAGINGQUEUE_PRIORITY_NORMAL,
        .PhysicalAdapterIndex = 0,
    };

    HRESULT hr = device->base.KTCallbacks.pfnCreatePagingQueueCb(device->base.hRTDevice.handle, &paging_queue);
    if (FAILED(hr)) {
        return hr;
    }

    device->paging.queue = paging_queue.hPagingQueue;
    device->paging.sync_object = paging_queue.hSyncObject;
    device->paging.fence_value = paging_queue.FenceValueCPUVirtualAddress;

    return S_OK;
}

static const char *get_drm_context_type_name(uint32_t drm_context_type) {
    switch (drm_context_type) {
        case VIRTGPU_DRM_CONTEXT_MSM:      return "msm";
        case VIRTGPU_DRM_CONTEXT_AMDGPU:   return "amdgpu";
        case VIRTGPU_DRM_CONTEXT_I915:     return "i915";
        case VIRTGPU_DRM_CONTEXT_ASAHI:    return "asahi";
        case VIRTGPU_DRM_CONTEXT_PANFROST: return "panfrost";
        case VIRTGPU_DRM_CONTEXT_XE:       return "xe";
    }
    ERROR("%s: Unknown drm context type: %u", __FUNCTION__, drm_context_type);
    return NULL;
}

static const wchar_t *get_drm_context_type_icd_name(uint32_t drm_context_type) {
    switch (drm_context_type) {
        // TODO: port more drivers
        /*
        case VIRTGPU_DRM_CONTEXT_MSM:      return L"vulkan_freedreno.dll";
        case VIRTGPU_DRM_CONTEXT_AMDGPU:   return L"vulkan_radeon.dll";
        case VIRTGPU_DRM_CONTEXT_I915:     return L"vulkan_intel.dll";
        case VIRTGPU_DRM_CONTEXT_ASAHI:    return L"vulkan_asahi.dll";
        case VIRTGPU_DRM_CONTEXT_PANFROST: return L"vulkan_panfrost.dll";
        */
        case VIRTGPU_DRM_CONTEXT_XE:       return L"vulkan_intel.dll";
    }
    ERROR("%s: Unsupported drm context type: %u (%s)", __FUNCTION__, drm_context_type, get_drm_context_type_name(drm_context_type));
    return NULL;
}

static HRESULT get_drm_context_type(VIRTIO_WDDM_Device *device, uint32_t *drm_context_type) {
    struct {
        VIRTIO_WDDM_Capset capset;
        uint8_t caps[sizeof(struct virgl_renderer_capset_drm)];
    } escape_priv = {
        .capset = {
            .tag = VIRTIO_WDDM_ESCAPE_CAPSET_TAG,
            .capset_id = VIRTIO_WDDM_CAPSET_ID_DRM,
            .version = 0,
        },
    };
    D3DDDICB_ESCAPE escape = {
        .hDevice = device->base.hRTDevice.handle,
        .pPrivateDriverData = &escape_priv,
        .PrivateDriverDataSize = sizeof(escape_priv),
    };

    HRESULT hr = device->base.KTCallbacks.pfnEscapeCb(device->base.pAdapter->hRTAdapter.handle, &escape);
    if (FAILED(hr)) {
        ERROR("%s: Failed to query DRM capset info: %08lx", __FUNCTION__, hr);
        return hr;
    }

    struct virgl_renderer_capset_drm caps;
    memcpy(&caps, escape_priv.caps, sizeof(caps));

    *drm_context_type = caps.context_type;

    return S_OK;
}

static HMODULE load_icd(const wchar_t *icd_name) {
    wchar_t umd_path[MAX_PATH];
    HMODULE umd_module = NULL;

    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&load_icd, &umd_module);

    if (!umd_module) return NULL;

    GetModuleFileNameW(umd_module, umd_path, MAX_PATH);
    wchar_t *last_slash = wcsrchr(umd_path, L'\\');
    if (!last_slash) {
        return NULL;
    }

    size_t remaining_space = MAX_PATH - (last_slash - umd_path) - 1;

    if (wcscpy_s(last_slash + 1, remaining_space, icd_name) != 0) {
        return FALSE;
    }

    return LoadLibraryExW(umd_path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}

HRESULT APIENTRY virtio_wddm_create_device(D3D10DDI_HADAPTER hAdapter, D3D10DDIARG_CREATEDEVICE *pArgs) {
    TRACE();
    HRESULT hr = S_OK;
    VkResult res = VK_SUCCESS;

    VIRTIO_WDDM_Adapter *adapter = hAdapter.pDrvPrivate;
    VIRTIO_WDDM_Device *device = pArgs->hDrvDevice.pDrvPrivate;
    memset(device, 0, sizeof(*device));

    device->base.pAdapter = &adapter->base;
    device->base.hRTDevice = pArgs->hRTDevice;
    device->base.uIfVersion = pArgs->Interface;
    device->base.uRtVersion = pArgs->Version;
    device->base.KTCallbacks = *pArgs->pKTCallbacks;
    device->base.hRTCoreLayer = pArgs->hRTCoreLayer;
    device->base.pUMCallbacks = pArgs->p11UMCallbacks;
    device->base.FeatureLevel = D3D_FEATURE_LEVEL_11_0;

    device->dxgi_callbacks = pArgs->DXGIBaseDDI.pDXGIBaseCallbacks;

    device->vulkan = LoadLibraryA("vulkan-1.dll");

    if (adapter->supported_capsets & VIRTIO_WDDM_CAPSET_MASK_DRM) {
        uint32_t drm_context_type = 0;
        hr = get_drm_context_type(device, &drm_context_type);
        if (SUCCEEDED(hr)) {
            INFO("%s: Supported DRM context type: %s", __FUNCTION__, get_drm_context_type_name(drm_context_type));
            const wchar_t *vdrm_icd_name = get_drm_context_type_icd_name(drm_context_type);
            if (vdrm_icd_name != NULL) {
                INFO("%s: Using vDRM Vulkan ICD: %ws", __FUNCTION__, vdrm_icd_name);
                device->icd = load_icd(vdrm_icd_name);
            }
        }
    }

    if (device->icd == NULL) {
        device->icd = load_icd(L"vulkan_virtio.dll");
    }

    ASSERT(device->icd != NULL);

    device->callbacks = (VkD3DDDICallbacks) {
        .sType = VK_STRUCTURE_TYPE_D3DDDI_CALLBACKS,
        .AdapterLuid = adapter->luid,
        .hRTAdapter = adapter->base.hRTAdapter.handle,
        .hRTDevice = device->base.hRTDevice.handle,
        .pAdapterCallbacks = &adapter->base.KTCallbacks,
        .pKTCallbacks = &device->base.KTCallbacks,
        .pDXGIBaseCallbacks = pArgs->DXGIBaseDDI.pDXGIBaseCallbacks,
        .hRTCoreLayer = pArgs->hRTCoreLayer.handle,
        .p11UMCallbacks = pArgs->p11UMCallbacks,
    };

    VkDirectDriverLoadingInfoLUNARG driver_info = {
        .sType = VK_STRUCTURE_TYPE_DIRECT_DRIVER_LOADING_INFO_LUNARG,
        .pNext = NULL,
        .flags = 0,
        .pfnGetInstanceProcAddr = (PFN_vkGetInstanceProcAddrLUNARG) GetProcAddress(device->icd, "vk_icdGetInstanceProcAddr"),
    };
    ASSERT(driver_info.pfnGetInstanceProcAddr != NULL);

    VkDirectDriverLoadingListLUNARG loading_list = {
        .sType = VK_STRUCTURE_TYPE_DIRECT_DRIVER_LOADING_LIST_LUNARG,
        .pNext = &device->callbacks,
        .mode = VK_DIRECT_DRIVER_LOADING_MODE_EXCLUSIVE_LUNARG,
        .driverCount = 1,
        .pDrivers = &driver_info,
    };

    INFO("D3DDDI callbacks: %p, dev %p", &device->callbacks, device->callbacks.hRTDevice);

    PFN_vkGetInstanceProcAddr vk_GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) GetProcAddress(device->vulkan, "vkGetInstanceProcAddr");
    PFN_vkGetDeviceProcAddr vk_GetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) GetProcAddress(device->vulkan, "vkGetDeviceProcAddr");

#define LOAD_PROC(name) PFN_vk##name vk_##name = (PFN_vk##name) vk_GetInstanceProcAddr(NULL, "vk" #name)
    LOAD_PROC(CreateInstance);
    LOAD_PROC(EnumerateInstanceExtensionProperties);
#undef LOAD_PROC

    uint32_t extension_count = 0;
    res = vk_EnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
    if (res != VK_SUCCESS) {
        ERROR("Failed to enumerate instance extensions: %s", vk_result_to_str(res));
        return E_FAIL;
    }

    CLEANUP_FREE VkExtensionProperties *extension_props = calloc(sizeof(*extension_props), extension_count);
    CLEANUP_FREE const char **extension_names = calloc(sizeof(*extension_names), extension_count);
    res = vk_EnumerateInstanceExtensionProperties(NULL, &extension_count, extension_props);
    if (res != VK_SUCCESS) {
        ERROR("Failed to enumerate instance extensions: %s", vk_result_to_str(res));
        return E_FAIL;
    }

    bool have_LUNARG_direct_driver_loading = false;

    for (size_t i = 0; i < extension_count; i++) {
        extension_names[i] = extension_props[i].extensionName;
        if (strcmp(extension_props[i].extensionName, VK_LUNARG_DIRECT_DRIVER_LOADING_EXTENSION_NAME) == 0) {
            have_LUNARG_direct_driver_loading = true;
        }
    }

    if (!have_LUNARG_direct_driver_loading) {
        ERROR("Direct driver loading is not supported, cannot continue");
        return E_FAIL;
    }

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "D3D11",
        .applicationVersion = 0,
        .pEngineName = "VirtIO D3D11 UMD",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &loading_list,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        //.enabledLayerCount = 1,
        //.ppEnabledLayerNames = (const char *[]) { "VK_LAYER_KHRONOS_validation" },
        // TODO: do we need more extensions?
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = (const char *[]) { VK_LUNARG_DIRECT_DRIVER_LOADING_EXTENSION_NAME },
    };

    VkInstance instance;
    res = vk_CreateInstance(&info, NULL, &instance);
    if (res != VK_SUCCESS) {
        ERROR("Failed to create instance: %s", vk_result_to_str(res));
        DebugBreak();
        return E_FAIL;
    }

    Vulkan_Instance_Info instance_info = {
        .loader_proc = vk_GetInstanceProcAddr,
        .instance = instance,
        // TODO: do we need other extensions?
        .extension_count = 1,
        .extension_names = (const char *[]) { VK_LUNARG_DIRECT_DRIVER_LOADING_EXTENSION_NAME },
    };

    IDXGIFactory4 *factory = NULL;
    hr = DXVK_CreateDXGIFactory(&instance_info, &IID_IDXGIFactory4, (void **) &factory);
    if (FAILED(hr)) {
        ERROR("Failed to create DXVK DXGI factory: 0x%x", hr);
        return hr;
    }

    hr = IDXGIFactory4_EnumAdapterByLuid(factory, adapter->luid, &IID_IDXGIAdapter, (void **) &device->adapter);
    if (FAILED(hr)) {
        ERROR("Failed to enum DXVK DXGI adapter by LUID %lx-%lx: 0x%x", adapter->luid.HighPart, adapter->luid.LowPart, hr);
        return hr;
    }

    D3D_FEATURE_LEVEL feature_level;
    switch (D3D11DDI_EXTRACT_3DPIPELINELEVEL_FROM_FLAGS(pArgs->Flags)) {
        case D3D11DDI_3DPIPELINELEVEL_10_0:
            feature_level = D3D_FEATURE_LEVEL_10_0;
            break;
        case D3D11DDI_3DPIPELINELEVEL_10_1:
            feature_level = D3D_FEATURE_LEVEL_10_1;
            break;
        case D3D11DDI_3DPIPELINELEVEL_11_0:
            feature_level = D3D_FEATURE_LEVEL_11_0;
            break;
        case D3D11_1DDI_3DPIPELINELEVEL_11_1:
            feature_level = D3D_FEATURE_LEVEL_11_1;
            break;
        default:
            ERROR("%s: unknown pipeline level %u", __FUNCTION__, D3D11DDI_EXTRACT_3DPIPELINELEVEL_FROM_FLAGS(pArgs->Flags));
            return E_FAIL;
    }

    CLEANUP_D3D11_DEVICE ID3D11Device *d3d11_device = NULL;
    CLEANUP_D3D11_CONTEXT ID3D11DeviceContext *d3d11_context = NULL;
    hr = D3D11CreateDevice(device->adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
                           &feature_level, 1, D3D11_SDK_VERSION, &d3d11_device,
                           &device->base.FeatureLevel, &d3d11_context);
    if (FAILED(hr)) {
        ERROR("Failed to create DXVK D3D11 device: 0x%x", hr);
        return hr;
    }

    hr = ID3D11Device_QueryInterface(d3d11_device, &IID_ID3D11Device1, (void **) &device->base.pDev1);
    if (FAILED(hr)) {
        ERROR("Failed to query ID3D11Device1 interface for DXVK D3D11 device: 0x%x", hr);
        return hr;
    }

    hr = ID3D11Device_QueryInterface(d3d11_device, &IID_ID3D11Device2, (void **) &device->base.pDev2);
    if (FAILED(hr)) {
        ERROR("Failed to query ID3D11Device2 interface for DXVK D3D11 device: 0x%x", hr);
        return hr;
    }

    hr = ID3D11Device_QueryInterface(d3d11_device, &IID_ID3D11Device3, (void **) &device->base.pDev3);
    if (FAILED(hr)) {
        ERROR("Failed to query ID3D11Device3 interface for DXVK D3D11 device: 0x%x", hr);
        return hr;
    }

    hr = ID3D11Device_QueryInterface(d3d11_device, &IID_ID3D11Device5, (void **) &device->base.pDev5);
    if (FAILED(hr)) {
        ERROR("Failed to query ID3D11Device5 interface for DXVK D3D11 device: 0x%x", hr);
        return hr;
    }

    hr = ID3D11DeviceContext_QueryInterface(d3d11_context, &IID_ID3D11DeviceContext1, (void **) &device->base.pCtx1);
    if (FAILED(hr)) {
        ERROR("Failed to query ID3D11DeviceContext1 interface for DXVK D3D11 device context: 0x%x", hr);
        return hr;
    }

    hr = ID3D11DeviceContext_QueryInterface(d3d11_context, &IID_ID3D11DeviceContext2, (void **) &device->base.pCtx2);
    if (FAILED(hr)) {
        ERROR("Failed to query ID3D11DeviceContext2 interface for DXVK D3D11 device context: 0x%x", hr);
        return hr;
    }

    hr = ID3D11DeviceContext_QueryInterface(d3d11_context, &IID_ID3D11DeviceContext3, (void **) &device->base.pCtx3);
    if (FAILED(hr)) {
        ERROR("Failed to query ID3D11DeviceContext3 interface for DXVK D3D11 device context: 0x%x", hr);
        return hr;
    }

    hr = ID3D11DeviceContext_QueryInterface(d3d11_context, &IID_ID3D11DeviceContext4, (void **) &device->base.pCtx4);
    if (FAILED(hr)) {
        ERROR("Failed to query ID3D11DeviceContext4 interface for DXVK D3D11 device context: 0x%x", hr);
        return hr;
    }

    hr = DXVK_IDXGIVkInteropDevice1_GetVulkanHandles(device->base.pDev1, &device->vk_inst, &device->vk_phys, &device->vk);
    if (FAILED(hr)) {
        ERROR("Failed to GetVulkanHandles from DXVK D3D11 device: 0x%x", hr);
        return hr;
    }

    INFO("%s: inst %p, phys %p, dev %p", __FUNCTION__, device->vk_inst, device->vk_phys, device->vk);

#define X(name) device->vk_##name = (PFN_vk##name) vk_GetInstanceProcAddr(device->vk_inst, "vk" #name);
    VK_INSTANCE_FUNCTION_LIST
#undef X
#define X(name) device->vk_##name = (PFN_vk##name) vk_GetDeviceProcAddr(device->vk, "vk" #name);
    VK_DEVICE_FUNCTION_LIST
#undef X
#define X(name) if (device->vk_##name == NULL) { ERROR("Failed to load vk%s from ICD", #name); return E_FAIL; }
    VK_INSTANCE_FUNCTION_LIST
    VK_DEVICE_FUNCTION_LIST
#undef X

    hr = ID3D11Device5_CreateFence(device->base.pDev5, 0, D3D11_FENCE_FLAG_SHARED, &IID_ID3D11Fence, (void **) &device->base.pPresentFence);
    if (FAILED(hr)) {
        ERROR("Failed to create D3D11 fence: 0x%x", hr);
        return hr;
    }
    device->base.presentFenceValue = 1;

    device->present.semaphore = DXVK_ID3D11Fence_GetVkSemaphore(device->base.pPresentFence);
    ASSERT(device->present.semaphore != VK_NULL_HANDLE);

    HANDLE present_fence = NULL;
    VkSemaphoreGetWin32HandleInfoKHR get_handle_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
        .semaphore = device->present.semaphore,
        .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D11_FENCE_BIT,
    };
    res = device->vk_GetSemaphoreWin32HandleKHR(device->vk, &get_handle_info, &present_fence);
    if (res != VK_SUCCESS) {
        ERROR("Failed to create instance: %s", vk_result_to_str(res));
        return E_FAIL;
    }

    device->present.fence = (D3DKMT_HANDLE) (intptr_t) present_fence;
    //INFO("%s: Present monitored fence = %x", __FUNCTION__, device->present.fence);

    hr = create_present_context(device);
    if (FAILED(hr)) {
        ERROR("Failed to create present context: 0x%x", hr);
        return hr;
    }

    hr = create_paging_queue(device);
    if (FAILED(hr)) {
        ERROR("Failed to create paging queue: 0x%x", hr);
        return hr;
    }

    INFO("%s: D3D11 DDI version = %s", __FUNCTION__, d3d10_ddi_interface_version_name(pArgs->Interface));
    INFO("%s: DXGI DDI version = %s", __FUNCTION__, dxgi_ddi_interface_version_name(pArgs->Interface, pArgs->Version));

    switch (pArgs->Interface) {
        case D3D11_0_DDI_INTERFACE_VERSION:
            tritonFillD3D11DeviceFuncs(pArgs->p11DeviceFuncs);
            pArgs->p11DeviceFuncs->pfnDestroyDevice = virtio_wddm_destroy_device;
            break;
        case D3D11_1_DDI_INTERFACE_VERSION:
            tritonFillD3D11_1DeviceFuncs(pArgs->p11_1DeviceFuncs);
            pArgs->p11_1DeviceFuncs->pfnDestroyDevice = virtio_wddm_destroy_device;
            break;
        case D3DWDDM1_3_DDI_INTERFACE_VERSION:
            tritonFillWDDM1_3DeviceFuncs(pArgs->pWDDM1_3DeviceFuncs);
            pArgs->pWDDM1_3DeviceFuncs->pfnDestroyDevice = virtio_wddm_destroy_device;
            break;
        case D3DWDDM2_0_DDI_INTERFACE_VERSION:
            tritonFillWDDM2_0DeviceFuncs(pArgs->pWDDM2_0DeviceFuncs);
            pArgs->pWDDM2_0DeviceFuncs->pfnDestroyDevice = virtio_wddm_destroy_device;
            break;
        case D3DWDDM2_1_DDI_INTERFACE_VERSION:
            tritonFillWDDM2_1DeviceFuncs(pArgs->pWDDM2_1DeviceFuncs);
            pArgs->pWDDM2_1DeviceFuncs->pfnDestroyDevice = virtio_wddm_destroy_device;
            break;
        default:
            ERROR("%s: unsupported interface: %u", __FUNCTION__, pArgs->Interface);
            return E_FAIL;
    }

    pArgs->p11DeviceFuncs->pfnCalcPrivateResourceSize = virtio_wddm_calc_resource_size;
    pArgs->p11DeviceFuncs->pfnCalcPrivateOpenedResourceSize = virtio_wddm_calc_opened_resource_size;
    pArgs->p11DeviceFuncs->pfnCreateResource = virtio_wddm_create_resource;
    pArgs->p11DeviceFuncs->pfnOpenResource = virtio_wddm_open_resource;
    pArgs->p11DeviceFuncs->pfnDestroyResource = virtio_wddm_destroy_resource;

    if (IS_DXGI1_2_BASE_FUNCTIONS(pArgs->Interface, pArgs->Version)) {
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnPresent                  = virtio_wddm_present;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnGetGammaCaps             = virtio_wddm_get_gamma_caps;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnSetDisplayMode           = virtio_wddm_set_display_mode;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnSetResourcePriority      = virtio_wddm_set_resource_priority;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnQueryResourceResidency   = virtio_wddm_query_resource_residency;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnRotateResourceIdentities = virtio_wddm_rotate_resource_identities;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnBlt                      = virtio_wddm_blt;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnResolveSharedResource    = virtio_wddm_resolve_shared_resource;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnBlt1                     = virtio_wddm_blt1;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnOfferResources           = virtio_wddm_offer_resources;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions3->pfnReclaimResources         = virtio_wddm_reclaim_resources;
    } else if (IS_DXGI1_1_BASE_FUNCTIONS(pArgs->Interface, pArgs->Version)) {
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions2->pfnPresent                  = virtio_wddm_present;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions2->pfnGetGammaCaps             = virtio_wddm_get_gamma_caps;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions2->pfnSetDisplayMode           = virtio_wddm_set_display_mode;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions2->pfnSetResourcePriority      = virtio_wddm_set_resource_priority;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions2->pfnQueryResourceResidency   = virtio_wddm_query_resource_residency;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions2->pfnRotateResourceIdentities = virtio_wddm_rotate_resource_identities;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions2->pfnBlt                      = virtio_wddm_blt;
        pArgs->DXGIBaseDDI.pDXGIDDIBaseFunctions2->pfnResolveSharedResource    = virtio_wddm_resolve_shared_resource;
    }

    return S_OK;
    //return DXGI_STATUS_NO_REDIRECTION;
}
