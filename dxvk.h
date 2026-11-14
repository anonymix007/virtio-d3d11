#pragma once

#include <dxgi.h>
#include <d3d11_1.h>
#include <vulkan/vulkan.h>

typedef struct {
    PFN_vkGetInstanceProcAddr loader_proc;
    VkInstance instance;
    uint32_t extension_count;
    const char **extension_names;
} Vulkan_Instance_Info;

HRESULT DXVK_CreateDXGIFactory(Vulkan_Instance_Info *instance, REFIID riid, void **ppFactory);
VkSemaphore DXVK_ID3D11Fence_GetVkSemaphore(ID3D11Fence *fence);

HRESULT DXVK_ID3D11Device_QueryInterface_IDXGIVkInteropDevice1(ID3D11Device *device, IUnknown **dxvk_interop);
void DXVK_IDXGIVkInteropDevice1_GetVulkanHandles(IUnknown *dxvk_interop, VkInstance *pInstance, VkPhysicalDevice *pPhysDev, VkDevice *pDevice);
HRESULT DXVK_IDXGIVkInteropDevice1_CreateTexture2DFromVkImage(IUnknown *dxvk_interop, const D3D11_TEXTURE2D_DESC1* pDesc, VkImage vkImage, ID3D11Texture2D **ppTexture2D);
void DXVK_IDXGIVkInteropDevice1_FlushRenderingCommands(IUnknown *dxvk_interop);
