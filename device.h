#pragma once

#include <pthread.h>

#include <triton.h>
#include <vulkan/vulkan.h>
#include <vulkan_d3dddi.h>

#define VK_INSTANCE_FUNCTION_LIST \
    X(GetPhysicalDeviceMemoryProperties2) \
    X(DestroyInstance)

#define VK_DEVICE_FUNCTION_LIST \
    X(AllocateMemory) \
    X(FreeMemory) \
    X(CreateImage) \
    X(DestroyImage) \
    X(BindImageMemory2) \
    X(GetImageMemoryRequirements2) \
    X(GetImageDrmFormatModifierPropertiesEXT) \
    X(GetSemaphoreWin32HandleKHR) \
    X(GetMemoryWin32HandleKHR)

typedef struct {
    TRITON_DEVICE base;
    DXGI_DDI_BASE_CALLBACKS *dxgi_callbacks;
    HMODULE vulkan;
    HMODULE icd;
    VkD3DDDICallbacksMESA callbacks;
    IDXGIAdapter *adapter;
    // ID3D11Device is stored in base

    struct {
        D3DKMT_HANDLE queue;
        D3DKMT_HANDLE sync_object;
        void *fence_value;
    } paging;

    struct {
        HANDLE context;
        VkSemaphore semaphore;
        D3DKMT_HANDLE fence;
    } present;

    VkInstance vk_inst;
    VkPhysicalDevice vk_phys;
    VkDevice vk;
#define X(name) PFN_vk##name vk_##name;
    VK_INSTANCE_FUNCTION_LIST
    VK_DEVICE_FUNCTION_LIST
#undef X
} VIRTIO_WDDM_Device;

SIZE_T APIENTRY virtio_wddm_calc_device_size(D3D10DDI_HADAPTER hAdapter, const D3D10DDIARG_CALCPRIVATEDEVICESIZE *pArgs);
HRESULT APIENTRY virtio_wddm_create_device(D3D10DDI_HADAPTER hAdapter, D3D10DDIARG_CREATEDEVICE *pArgs);

extern const char *vk_result_to_str(VkResult result);
