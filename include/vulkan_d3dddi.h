/*
 * This file contains mesa-internal D3DDDI extension structs and enumerants.
 * These are not exposed to apps, but instead used by D3D UMD implementations
 * layered on top of Vulkan.
 */

#ifndef VK_INTERNAL_D3DDDI_EXT_H
#define VK_INTERNAL_D3DDDI_EXT_H

#include <vulkan/vulkan_core.h>
#include <stdbool.h>

#ifndef _D3DUMDDI_H_
typedef struct _D3DDDI_ADAPTERCALLBACKS D3DDDI_ADAPTERCALLBACKS;
typedef struct _D3DDDI_DEVICECALLBACKS D3DDDI_DEVICECALLBACKS;
#endif

#ifndef _DXGIDDI_H
typedef struct DXGI_DDI_BASE_CALLBACKS DXGI_DDI_BASE_CALLBACKS;
#endif

#ifndef _D3D10UMDDI_H
typedef struct D3D11DDI_CORELAYER_DEVICECALLBACKS D3D11DDI_CORELAYER_DEVICECALLBACKS;
typedef struct D3D10DDIARG_OPENRESOURCE D3D10DDIARG_OPENRESOURCE;
typedef struct D3D10DDIARG_CREATERESOURCE D3D10DDIARG_CREATERESOURCE;
typedef struct D3D11DDIARG_CREATERESOURCE D3D11DDIARG_CREATERESOURCE;
#endif

typedef struct {
    VkStructureType sType;
    void *pNext;

    LUID AdapterLuid;

    HANDLE                          hRTAdapter;
    HANDLE                          hRTDevice;
    const D3DDDI_ADAPTERCALLBACKS  *pAdapterCallbacks;
    const D3DDDI_DEVICECALLBACKS   *pKTCallbacks;
    const DXGI_DDI_BASE_CALLBACKS  *pDXGIBaseCallbacks;

    HANDLE                                    hRTCoreLayer;
    const D3D11DDI_CORELAYER_DEVICECALLBACKS* p11UMCallbacks;
} VkD3DDDICallbacksMESA;

typedef struct {
    VkStructureType sType;
    void *pNext;
    HANDLE hRTResource;
    union {
        const D3D10DDIARG_CREATERESOURCE *pCreateResource10;
        const D3D11DDIARG_CREATERESOURCE *pCreateResource11;
    };
    void *pExtraData;
    size_t ExtraDataSize;
} VkD3DDDICreateResourceMESA;

typedef struct {
    VkStructureType sType;
    void *pNext;
    HANDLE hRTResource;
    const D3D10DDIARG_OPENRESOURCE *pOpenResource;
    const void *pResourceInfo;
} VkD3DDDIOpenResourceMESA;

#define VK_STRUCTURE_TYPE_D3DDDI_CALLBACKS_MESA \
   (VkStructureType)4281808695u
#define VK_STRUCTURE_TYPE_D3DDDI_CREATE_RESOURCE_MESA \
   (VkStructureType)4281808696u
#define VK_STRUCTURE_TYPE_D3DDDI_OPEN_RESOURCE_MESA \
   (VkStructureType)4281808697u

#define VK_STRUCTURE_TYPE_D3DDDI_CALLBACKS_MESA_cast \
   VkD3DDDICallbacksMESA
#define VK_STRUCTURE_TYPE_D3DDDI_CREATE_RESOURCE_MESA_cast \
   VkD3DDDICreateResourceMESA
#define VK_STRUCTURE_TYPE_D3DDDI_OPEN_RESOURCE_MESA_cast \
   VkD3DDDIOpenResourceMESA

#endif /* VK_INTERNAL_D3DDDI_EXT_H */
