#define INITGUID
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winddk_compat.h>
#include <virtio_wddm_uapi.h>
#include <virgl_hw.h>

#undef ERROR
#include "adapter.h"
#include "device.h"
#include "dxvk.h"
#include "resource.h"

size_t APIENTRY virtio_wddm_calc_resource_size(D3D10DDI_HDEVICE hDevice, const D3D11DDIARG_CREATERESOURCE *pArgs) {
    return sizeof(VIRTIO_WDDM_Resource);
}

size_t APIENTRY virtio_wddm_calc_opened_resource_size(D3D10DDI_HDEVICE hDevice, const D3D10DDIARG_OPENRESOURCE *pArgs) {
    return sizeof(VIRTIO_WDDM_Resource);
}

static inline const char *resource_type_name(D3D10DDIRESOURCE_TYPE type) {
    switch (type) {
        case D3D10DDIRESOURCE_BUFFER:      return "BUFFER";
        case D3D10DDIRESOURCE_TEXTURE1D:   return "TEXTURE1D";
        case D3D10DDIRESOURCE_TEXTURE2D:   return "TEXTURE2D";
        case D3D10DDIRESOURCE_TEXTURE3D:   return "TEXTURE3D";
        case D3D10DDIRESOURCE_TEXTURECUBE: return "CUBE";
        case D3D11DDIRESOURCE_BUFFEREX:    return "BUFFEREX";
        default:
            ERROR("%s: unsupported resource type: %u", __FUNCTION__, type);
            return "unknown";
    }
}

/*
typedef struct {
    char c_str[256];
} SmallString;

static inline SmallString bind_flags_to_string(unsigned bind) {
    static const struct {
        uint32_t value;
        const char *name;
    } flag_table[] = {
        { D3D10_DDI_BIND_VERTEX_BUFFER,    "VERTEX_BUFFER"    },
        { D3D10_DDI_BIND_INDEX_BUFFER,     "INDEX_BUFFER"     },
        { D3D10_DDI_BIND_CONSTANT_BUFFER,  "CONSTANT_BUFFER"  },
        { D3D10_DDI_BIND_SHADER_RESOURCE,  "SHADER_RESOURCE"  },
        { D3D10_DDI_BIND_STREAM_OUTPUT,    "STREAM_OUTPUT"    },
        { D3D10_DDI_BIND_RENDER_TARGET,    "RENDER_TARGET"    },
        { D3D10_DDI_BIND_DEPTH_STENCIL,    "DEPTH_STENCIL"    },
        { D3D10_DDI_BIND_PRESENT,          "PRESENT"          },
        { D3D11_DDI_BIND_UNORDERED_ACCESS, "UNORDERED_ACCESS" },
        { D3D11_DDI_BIND_DECODER,          "DECODER"          },
        { D3D11_DDI_BIND_VIDEO_ENCODER,    "VIDEO_ENCODER"    },
        { D3D11_DDI_BIND_CAPTURE,          "CAPTURE"          }
    };

    SmallString str = {};

    // TODO
}
*/

static inline VkFormat dxgi_to_vk_format(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:    return VK_FORMAT_R32G32B32A32_SFLOAT;
        case DXGI_FORMAT_R32G32B32A32_UINT:     return VK_FORMAT_R32G32B32A32_UINT;
        case DXGI_FORMAT_R32G32B32A32_SINT:     return VK_FORMAT_R32G32B32A32_SINT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:    return VK_FORMAT_R32G32B32_SFLOAT;
        case DXGI_FORMAT_R32G32B32_FLOAT:       return VK_FORMAT_R32G32B32_SFLOAT;
        case DXGI_FORMAT_R32G32B32_UINT:        return VK_FORMAT_R32G32B32_UINT;
        case DXGI_FORMAT_R32G32B32_SINT:        return VK_FORMAT_R32G32B32_SINT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:    return VK_FORMAT_R16G16B16A16_SFLOAT;
        case DXGI_FORMAT_R16G16B16A16_UNORM:    return VK_FORMAT_R16G16B16A16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_UINT:     return VK_FORMAT_R16G16B16A16_UINT;
        case DXGI_FORMAT_R16G16B16A16_SNORM:    return VK_FORMAT_R16G16B16A16_SNORM;
        case DXGI_FORMAT_R16G16B16A16_SINT:     return VK_FORMAT_R16G16B16A16_SINT;
        case DXGI_FORMAT_R32G32_TYPELESS:       return VK_FORMAT_R32G32_SFLOAT;
        case DXGI_FORMAT_R32G32_FLOAT:          return VK_FORMAT_R32G32_SFLOAT;
        case DXGI_FORMAT_R32G32_UINT:           return VK_FORMAT_R32G32_UINT;
        case DXGI_FORMAT_R32G32_SINT:           return VK_FORMAT_R32G32_SINT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case DXGI_FORMAT_R10G10B10A2_UNORM:     return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case DXGI_FORMAT_R10G10B10A2_UINT:      return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        case DXGI_FORMAT_R11G11B10_FLOAT:       return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case DXGI_FORMAT_R8G8_TYPELESS:         return VK_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R8G8_UNORM:            return VK_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R8G8_UINT:             return VK_FORMAT_R8G8_UINT;
        case DXGI_FORMAT_R8G8_SNORM:            return VK_FORMAT_R8G8_SNORM;
        case DXGI_FORMAT_R8G8_SINT:             return VK_FORMAT_R8G8_SINT;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return VK_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM:        return VK_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return VK_FORMAT_R8G8B8A8_SRGB;
        case DXGI_FORMAT_R8G8B8A8_UINT:         return VK_FORMAT_R8G8B8A8_UINT;
        case DXGI_FORMAT_R8G8B8A8_SNORM:        return VK_FORMAT_R8G8B8A8_SNORM;
        case DXGI_FORMAT_R8G8B8A8_SINT:         return VK_FORMAT_R8G8B8A8_SINT;
        case DXGI_FORMAT_R16G16_TYPELESS:       return VK_FORMAT_R16G16_SFLOAT;
        case DXGI_FORMAT_R16G16_FLOAT:          return VK_FORMAT_R16G16_SFLOAT;
        case DXGI_FORMAT_R16G16_UNORM:          return VK_FORMAT_R16G16_UNORM;
        case DXGI_FORMAT_R16G16_UINT:           return VK_FORMAT_R16G16_UINT;
        case DXGI_FORMAT_R16G16_SNORM:          return VK_FORMAT_R16G16_SNORM;
        case DXGI_FORMAT_R16G16_SINT:           return VK_FORMAT_R16G16_SINT;
        case DXGI_FORMAT_R32_TYPELESS:          return VK_FORMAT_R32_UINT;
        case DXGI_FORMAT_D32_FLOAT:             return VK_FORMAT_D32_SFLOAT;
        case DXGI_FORMAT_R32_FLOAT:             return VK_FORMAT_R32_SFLOAT;
        case DXGI_FORMAT_R32_UINT:              return VK_FORMAT_R32_UINT;
        case DXGI_FORMAT_R32_SINT:              return VK_FORMAT_R32_SINT;
        case DXGI_FORMAT_R16_TYPELESS:          return VK_FORMAT_R16_UINT;
        case DXGI_FORMAT_R16_FLOAT:             return VK_FORMAT_R16_SFLOAT;
        case DXGI_FORMAT_D16_UNORM:             return VK_FORMAT_D16_UNORM;
        case DXGI_FORMAT_R16_UNORM:             return VK_FORMAT_R16_UNORM;
        case DXGI_FORMAT_R16_UINT:              return VK_FORMAT_R16_UINT;
        case DXGI_FORMAT_R16_SNORM:             return VK_FORMAT_R16_SNORM;
        case DXGI_FORMAT_R16_SINT:              return VK_FORMAT_R16_SINT;
        case DXGI_FORMAT_R8_TYPELESS:           return VK_FORMAT_R8_UNORM;
        case DXGI_FORMAT_R8_UNORM:              return VK_FORMAT_R8_UNORM;
        case DXGI_FORMAT_R8_UINT:               return VK_FORMAT_R8_UINT;
        case DXGI_FORMAT_R8_SNORM:              return VK_FORMAT_R8_SNORM;
        case DXGI_FORMAT_R8_SINT:               return VK_FORMAT_R8_SINT;
        case DXGI_FORMAT_A8_UNORM:              return VK_FORMAT_A8_UNORM;
        case DXGI_FORMAT_B5G6R5_UNORM:          return VK_FORMAT_R5G6B5_UNORM_PACK16;
        case DXGI_FORMAT_B5G5R5A1_UNORM:        return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
        case DXGI_FORMAT_B8G8R8A8_UNORM:        return VK_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM:        return VK_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return VK_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return VK_FORMAT_B8G8R8A8_SRGB;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:     return VK_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return VK_FORMAT_B8G8R8A8_SRGB;
        case DXGI_FORMAT_AYUV:                  return VK_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_NV12:                  return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
        case DXGI_FORMAT_420_OPAQUE:            return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
        case DXGI_FORMAT_YUY2:                  return VK_FORMAT_G8B8G8R8_422_UNORM;
        case DXGI_FORMAT_B4G4R4A4_UNORM:        return VK_FORMAT_A4R4G4B4_UNORM_PACK16;
    }

    return VK_FORMAT_UNDEFINED;
}

#define FORMAT_FAMILY_CASE(...) {                    \
        static VkFormat formats[] = { __VA_ARGS__ }; \
        *vk_formats = formats;                       \
        *vk_count = ARRAY_SIZE(formats);             \
        return true;                                 \
    }


static inline bool dxgi_to_vk_format_family(DXGI_FORMAT format, const VkFormat **vk_formats, uint32_t *vk_count) {
    switch (format) {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SFLOAT);
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32_SFLOAT);
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SFLOAT);
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_A2B10G10R10_UINT_PACK32);
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_SINT);
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            FORMAT_FAMILY_CASE(VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB);
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            FORMAT_FAMILY_CASE(VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB);
        case DXGI_FORMAT_R16G16_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SFLOAT);
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            FORMAT_FAMILY_CASE(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB);
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            FORMAT_FAMILY_CASE(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB);
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB);
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            FORMAT_FAMILY_CASE(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB);
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB);
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            FORMAT_FAMILY_CASE(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB);
        case DXGI_FORMAT_R8_TYPELESS:
            FORMAT_FAMILY_CASE(VK_FORMAT_R8_UNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8_SNORM);
        case DXGI_FORMAT_A8_UNORM:
            FORMAT_FAMILY_CASE(VK_FORMAT_R8_UNORM, VK_FORMAT_A8_UNORM);
        case DXGI_FORMAT_AYUV:
            FORMAT_FAMILY_CASE(VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM);
        case DXGI_FORMAT_NV12:
            FORMAT_FAMILY_CASE(VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8_UINT);
        case DXGI_FORMAT_P010:
            FORMAT_FAMILY_CASE(VK_FORMAT_R16_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16_UINT, VK_FORMAT_R16G16_UINT);
        case DXGI_FORMAT_P016:
            FORMAT_FAMILY_CASE(VK_FORMAT_R16_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16_UINT, VK_FORMAT_R16G16_UINT);
        case DXGI_FORMAT_420_OPAQUE:
            FORMAT_FAMILY_CASE(VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8_UINT);
        case DXGI_FORMAT_YUY2:
            FORMAT_FAMILY_CASE(VK_FORMAT_G8B8G8R8_422_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UINT);
        default:
            return false;
    }
}

static inline VkSampleCountFlagBits dxgi_to_vk_sample_count(unsigned count) {
    switch (count) {
        case 1:  return VK_SAMPLE_COUNT_1_BIT;
        case 2:  return VK_SAMPLE_COUNT_2_BIT;
        case 4:  return VK_SAMPLE_COUNT_4_BIT;
        case 8:  return VK_SAMPLE_COUNT_8_BIT;
        case 16: return VK_SAMPLE_COUNT_16_BIT;
    }

    return 0;
}

static ssize_t vulkan_find_mem_type(VIRTIO_WDDM_Device *device, VkMemoryPropertyFlags flags, uint32_t req_bits) {
    VkPhysicalDeviceMemoryProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
    };
    device->vk_GetPhysicalDeviceMemoryProperties2(device->vk_phys, &props);
    for (unsigned i = 0u; i < props.memoryProperties.memoryTypeCount; ++i) {
        if (req_bits & (1 << i)) {
            if ((props.memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
                return i;
            }
        }
    }

    return -1;
}

static HRESULT create_shared_vk_image(VIRTIO_WDDM_Device *device, D3D11_TEXTURE2D_DESC1 *desc, VkImageTiling tiling, void *image_next, void *alloc_next, VkDeviceMemory *memory, VkImage *image, D3DKMT_HANDLE *alloc) {
    VkExternalMemoryImageCreateInfo external_mem_info = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .pNext = image_next,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
    };

    VkImageFormatListCreateInfo format_list_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO,
        .pNext = &external_mem_info,
    };

    VkImageCreateFlags flags = VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;

    if (dxgi_to_vk_format_family(desc->Format, &format_list_info.pViewFormats, &format_list_info.viewFormatCount)) {
        flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = &format_list_info,
        .flags = flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = dxgi_to_vk_format(desc->Format),
        .extent = { desc->Width, desc->Height, 1 },
        .mipLevels = desc->MipLevels,
        .arrayLayers = desc->ArraySize,
        .samples = dxgi_to_vk_sample_count(desc->SampleDesc.Count),
        .tiling = tiling,
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (image_create_info.format == VK_FORMAT_UNDEFINED) {
        ERROR("%s: Unsupported DXGI format: %u", __FUNCTION__, desc->Format);
        return E_OUTOFMEMORY;
    }

    if (image_create_info.samples == 0) {
        ERROR("%s: Unsupported sample count: %u", __FUNCTION__, desc->SampleDesc.Count);
        return E_OUTOFMEMORY;
    }

    VkResult res = device->vk_CreateImage(device->vk, &image_create_info, NULL, image);
    if (res != VK_SUCCESS) {
        ERROR("%s: Failed to create Vulkan image: %s", __FUNCTION__, vk_result_to_str(res));
        return E_OUTOFMEMORY;
    }

    VkImageMemoryRequirementsInfo2 mem_req_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
        .image = *image,
    };
    VkMemoryRequirements2 mem_req = {
       .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
    };
    device->vk_GetImageMemoryRequirements2(device->vk, &mem_req_info, &mem_req);
    ssize_t mem = vulkan_find_mem_type(device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_req.memoryRequirements.memoryTypeBits);
    if (mem < 0) {
        mem = vulkan_find_mem_type(device, 0, mem_req.memoryRequirements.memoryTypeBits);
    }
    if (mem < 0) {
        ERROR("%s: Failed to find Vulkan memory type", __FUNCTION__);
        return E_OUTOFMEMORY;
    }

    //VkImageDrmFormatModifierPropertiesEXT mod_props = {
    //    .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT,
    //};
    //res = device->vk_GetImageDrmFormatModifierPropertiesEXT(device->vk, *image, &mod_props);
    //if (res == VK_SUCCESS) {
    //    INFO("Image needs %zu bytes, modifier: 0x%016llx\n", mem_req.memoryRequirements.size, mod_props.drmFormatModifier);
    //} else {
    //    INFO("Image needs %zu bytes (failed to get modifier: %s)\n", mem_req.memoryRequirements.size, vk_result_to_str(res));
    //}

    VkMemoryDedicatedAllocateInfo dedicated_info = {
       .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
       .pNext = alloc_next,
       .image = *image,
    };
    VkExportMemoryAllocateInfo export_info = {
        .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
        .pNext = &dedicated_info,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
    };
    VkMemoryAllocateInfo memory_alloc_info = {
       .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
       .pNext = &export_info,
       .allocationSize = mem_req.memoryRequirements.size,
       .memoryTypeIndex = mem,
    };
    res = device->vk_AllocateMemory(device->vk, &memory_alloc_info, NULL, memory);
    if (res != VK_SUCCESS) {
        ERROR("%s: Failed to allocate Vulkan memory: %s", __FUNCTION__, vk_result_to_str(res));
        device->vk_DestroyImage(device->vk, *image, NULL);
        *image = VK_NULL_HANDLE;
        return E_OUTOFMEMORY;
    }

    VkBindImageMemoryInfo bind_info = {
        .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
        .image = *image,
        .memory = *memory,
        .memoryOffset = 0,
    };
    res = device->vk_BindImageMemory2(device->vk, 1, &bind_info);
    if (res != VK_SUCCESS) {
        ERROR("%s: Failed to bind Vulkan image to memory: %s", __FUNCTION__, vk_result_to_str(res));
        device->vk_DestroyImage(device->vk, *image, NULL);
        device->vk_FreeMemory(device->vk, *memory, NULL);
        *image = VK_NULL_HANDLE;
        *memory = VK_NULL_HANDLE;
        return E_OUTOFMEMORY;
    }

    if (alloc != NULL) {
        VkMemoryGetWin32HandleInfoKHR get_handle_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
            .memory = *memory,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
        };
        HANDLE h = NULL;
        res = device->vk_GetMemoryWin32HandleKHR(device->vk, &get_handle_info, &h);
        if (res != VK_SUCCESS) {
            ERROR("%s: Failed to export handle from Vulkan memory: %s", __FUNCTION__, vk_result_to_str(res));
            device->vk_DestroyImage(device->vk, *image, NULL);
            device->vk_FreeMemory(device->vk, *memory, NULL);
            *image = VK_NULL_HANDLE;
            *memory = VK_NULL_HANDLE;
            return E_OUTOFMEMORY;
        }

        *alloc = (D3DKMT_HANDLE) (intptr_t) h;
    }

    return S_OK;
}

static inline unsigned dxgi_to_virgl_format(DXGI_FORMAT format) {
    // FIXME: host support for formats here is quite limited, see gbm_to_vk_conversions in virglrenderer/src/vrend/vrend_venus_interop.c
    switch (format) {
        // case DXGI_FORMAT_R32G32B32A32_TYPELESS: return VIRGL_FORMAT_R32G32B32A32_UNORM;
        // case DXGI_FORMAT_R32G32B32A32_FLOAT:    return VIRGL_FORMAT_R32G32B32A32_FLOAT;
        // case DXGI_FORMAT_R32G32B32A32_UINT:     return VIRGL_FORMAT_R32G32B32A32_UINT;
        // case DXGI_FORMAT_R32G32B32A32_SINT:     return VIRGL_FORMAT_R32G32B32A32_SINT;

        // case DXGI_FORMAT_R32G32B32_TYPELESS:    return VIRGL_FORMAT_R32G32B32_UNORM;
        // case DXGI_FORMAT_R32G32B32_FLOAT:       return VIRGL_FORMAT_R32G32B32_FLOAT;
        // case DXGI_FORMAT_R32G32B32_UINT:        return VIRGL_FORMAT_R32G32B32_UINT;
        // case DXGI_FORMAT_R32G32B32_SINT:        return VIRGL_FORMAT_R32G32B32_SINT;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return VIRGL_FORMAT_R16G16B16A16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:    return VIRGL_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_UNORM:    return VIRGL_FORMAT_R16G16B16A16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_UINT:     return VIRGL_FORMAT_R16G16B16A16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_SNORM:    return VIRGL_FORMAT_R16G16B16A16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_SINT:     return VIRGL_FORMAT_R16G16B16A16_UNORM;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return VIRGL_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_R10G10B10A2_UNORM:     return VIRGL_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_R10G10B10A2_UINT:      return VIRGL_FORMAT_R10G10B10A2_UNORM;

        // case DXGI_FORMAT_R11G11B10_FLOAT:       return VIRGL_FORMAT_R11G11B10_FLOAT;

        case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return VIRGL_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM:        return VIRGL_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return VIRGL_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UINT:         return VIRGL_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_SNORM:        return VIRGL_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_SINT:         return VIRGL_FORMAT_R8G8B8A8_UNORM;

        case DXGI_FORMAT_B8G8R8A8_UNORM:        return VIRGL_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM:        return VIRGL_FORMAT_B8G8R8X8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return VIRGL_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return VIRGL_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:     return VIRGL_FORMAT_B8G8R8X8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return VIRGL_FORMAT_B8G8R8X8_UNORM;

        case DXGI_FORMAT_B4G4R4A4_UNORM:        return VIRGL_FORMAT_B4G4R4A4_UNORM;

        case DXGI_FORMAT_NV12:                  return VIRGL_FORMAT_Y8_U8V8_420_UNORM;
        case DXGI_FORMAT_420_OPAQUE:            return VIRGL_FORMAT_Y8_U8_V8_420_UNORM;
    }

    return VIRGL_FORMAT_NONE;
}

static inline DXGI_FORMAT virgl_to_dxgi_format(unsigned format) {
    switch (format) {
        case VIRGL_FORMAT_R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case VIRGL_FORMAT_R16G16B16A16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;

        case VIRGL_FORMAT_R10G10B10A2_UNORM:  return DXGI_FORMAT_R10G10B10A2_UNORM;

        case VIRGL_FORMAT_R8G8B8A8_UNORM:     return DXGI_FORMAT_R8G8B8A8_UNORM;
        case VIRGL_FORMAT_R8G8B8A8_SRGB:      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case VIRGL_FORMAT_B8G8R8A8_UNORM:     return DXGI_FORMAT_B8G8R8A8_UNORM;
        case VIRGL_FORMAT_B8G8R8A8_SRGB:      return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case VIRGL_FORMAT_B8G8R8X8_UNORM:     return DXGI_FORMAT_B8G8R8X8_UNORM;
        case VIRGL_FORMAT_B8G8R8X8_SRGB:      return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

        case VIRGL_FORMAT_B4G4R4A4_UNORM:     return DXGI_FORMAT_B4G4R4A4_UNORM;

        case VIRGL_FORMAT_R8_UNORM:           return DXGI_FORMAT_R8_UNORM;
        case VIRGL_FORMAT_A8_UNORM:           return DXGI_FORMAT_A8_UNORM;

        case VIRGL_FORMAT_Y8_U8V8_420_UNORM:  return DXGI_FORMAT_NV12;
        case VIRGL_FORMAT_Y8_U8_V8_420_UNORM: return DXGI_FORMAT_420_OPAQUE;
    }

    ERROR("%s: Unsupported VirGL format: %u", __FUNCTION__, format);
    return DXGI_FORMAT_UNKNOWN;
}

static inline unsigned dxgi_format_bytes_per_pixel(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return 16;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:    return 16;
        case DXGI_FORMAT_R32G32B32A32_UINT:     return 16;
        case DXGI_FORMAT_R32G32B32A32_SINT:     return 16;

        case DXGI_FORMAT_R32G32B32_TYPELESS:    return 12;
        case DXGI_FORMAT_R32G32B32_FLOAT:       return 12;
        case DXGI_FORMAT_R32G32B32_UINT:        return 12;
        case DXGI_FORMAT_R32G32B32_SINT:        return 12;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return 8;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:    return 8;
        case DXGI_FORMAT_R16G16B16A16_UNORM:    return 8;
        case DXGI_FORMAT_R16G16B16A16_UINT:     return 8;
        case DXGI_FORMAT_R16G16B16A16_SNORM:    return 8;
        case DXGI_FORMAT_R16G16B16A16_SINT:     return 8;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return 4;
        case DXGI_FORMAT_R10G10B10A2_UNORM:     return 4;
        case DXGI_FORMAT_R10G10B10A2_UINT:      return 4;

        case DXGI_FORMAT_R11G11B10_FLOAT:       return 4;

        case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return 4;
        case DXGI_FORMAT_R8G8B8A8_UNORM:        return 4;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return 4;
        case DXGI_FORMAT_R8G8B8A8_UINT:         return 4;
        case DXGI_FORMAT_R8G8B8A8_SNORM:        return 4;
        case DXGI_FORMAT_R8G8B8A8_SINT:         return 4;

        case DXGI_FORMAT_B8G8R8A8_UNORM:        return 4;
        case DXGI_FORMAT_B8G8R8X8_UNORM:        return 4;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return 4;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return 4;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:     return 4;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return 4;

        case DXGI_FORMAT_B4G4R4A4_UNORM:        return 2;

        case DXGI_FORMAT_NV12:                  return 4;
        case DXGI_FORMAT_420_OPAQUE:            return 4;
    }

    return 0;
}

/*
static HRESULT open_resource(VIRTIO_WDDM_Device *device, D3DKMT_HANDLE alloc) {
    VIRTIO_WDDM_Escape escape_priv = {
        .res_atta = {
            .tag = VIRTIO_WDDM_ESCAPE_RESOURCE_ATTACH_TAG,
            .handle = alloc,
        },
    };
    D3DDDICB_ESCAPE escape = {
        .hDevice = device->base.hRTDevice.handle,
        .pPrivateDriverData = &escape_priv,
        .PrivateDriverDataSize = sizeof(escape_priv),
    };
    HRESULT hr = device->base.KTCallbacks.pfnEscapeCb(device->base.pAdapter->hRTAdapter.handle, &escape);
    if (FAILED(hr)) {
        ERROR("%s: Failed to attach resource: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }

    return S_OK;
}
*/

static HRESULT query_resource_info(VIRTIO_WDDM_Device *device, D3DKMT_HANDLE alloc, uint32_t *id, VIRTIO_WDDM_AllocationInfo *info) {
    VIRTIO_WDDM_Escape escape_priv = {
        .res_info = {
            .tag = VIRTIO_WDDM_ESCAPE_RESOURCE_INFO_TAG,
            .handle = alloc,
            .info = {
                ._3d = {
                    .modifier = 0x00FFFFFFFFFFFFFF,
                },
            },
        },
    };
    D3DDDICB_ESCAPE escape = {
        .hDevice = device->base.hRTDevice.handle,
        .pPrivateDriverData = &escape_priv,
        .PrivateDriverDataSize = sizeof(escape_priv),
    };
    HRESULT hr = device->base.KTCallbacks.pfnEscapeCb(device->base.pAdapter->hRTAdapter.handle, &escape);
    if (FAILED(hr)) {
        ERROR("%s: Failed to get resource id: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }


    if (id != NULL) {
        *id = escape_priv.res_info.id;
    }
    if (info != NULL) {
        *info = escape_priv.res_info.info;
    }

    return S_OK;
}

static HRESULT create_shared_3d_resource(VIRTIO_WDDM_Device *device, D3D11_TEXTURE2D_DESC1 *desc, bool primary, HANDLE hRTResource, D3DKMT_HANDLE *hKMResource, D3DDDI_ALLOCATIONINFO2 *alloc, uint32_t *id, VIRTIO_WDDM_Allocate3dFull *info) {
    VIRTIO_WDDM_CreateResource res_priv = {
        .tag = VIRTIO_WDDM_CREATE_RESOURCE_TAG,
    };

    VIRTIO_WDDM_CreateAllocation alloc_priv = {
        ._3d = {
            .tag = VIRTIO_WDDM_ALLOCATE_3D_TAG,
            .target = 2,
            .format = dxgi_to_virgl_format(desc->Format),
            .bind = VIRGL_BIND_RENDER_TARGET | VIRGL_BIND_DISPLAY_TARGET | VIRGL_BIND_SAMPLER_VIEW | VIRGL_BIND_SCANOUT | VIRGL_BIND_SHARED,
            .width = desc->Width,
            .height = desc->Height,
            .depth = 1,
            .array_size = desc->ArraySize,
            .last_level = desc->MipLevels - 1,
            .nr_samples = desc->SampleDesc.Count,
            .flags = 0,
            .size = desc->Width * desc->Height * dxgi_format_bytes_per_pixel(desc->Format),
        },
    };

    if (alloc_priv._3d.format == VIRGL_FORMAT_NONE || alloc_priv._3d.size == 0) {
        ERROR("%s: Unsupported DXGI format: %u", __FUNCTION__, desc->Format);
        return E_OUTOFMEMORY;
    }

    D3DDDI_ALLOCATIONINFO2 alloc_info = {
        .pPrivateDriverData = &alloc_priv,
        .PrivateDriverDataSize = sizeof(alloc_priv),
        .Flags = {
            .Primary = primary,
        },
    };

    D3DDDICB_ALLOCATE allocate = {
        .pPrivateDriverData = &res_priv,
        .PrivateDriverDataSize = sizeof(res_priv),
        .hResource = hRTResource,
        .NumAllocations = 1,
        .pAllocationInfo2 = &alloc_info,
    };

    HRESULT hr = device->base.KTCallbacks.pfnAllocateCb(device->base.hRTDevice.handle, &allocate);
    if (FAILED(hr)) {
        ERROR("%s: Failed to allocate: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }

    *hKMResource = allocate.hKMResource;

    VIRTIO_WDDM_AllocationInfo res_info;
    hr = query_resource_info(device, alloc_info.hAllocation, id, &res_info);
    if (FAILED(hr)) {
        ERROR("%s: Failed to query resource info: 0x%08lx", __FUNCTION__, hr);
        return hr;
    }
    ASSERT(res_info.tag == VIRTIO_WDDM_ALLOCATE_3D_TAG);
    *info = res_info._3d;
    *alloc = alloc_info;

    //INFO("%s: resource id: %u, modifier: 0x%016llx, rt: %p, km: 0x%08x, alloc: 0x%08x, dev: %p / %p", __FUNCTION__, *id, info->modifier, hRTResource, *hKMResource, alloc_info.hAllocation, device->base.hRTDevice.handle, device->callbacks.hRTDevice);

    //hr = open_resource(device, alloc_info.hAllocation);
    //if (FAILED(hr)) {
    //    ERROR("%s: failed to open resource: hr=0x%08lx", __FUNCTION__, hr);
    //}

    // FIXME: nope /* Paging is done by ICD on import */

    D3DDDI_MAKERESIDENT make_resident = {
        .hPagingQueue = device->paging.queue,
        .NumAllocations = 1,
        .AllocationList = &alloc_info.hAllocation,
        .PriorityList = NULL,
        .Flags = {
            .CantTrimFurther = 1,
            .MustSucceed = 1,
        },
    };
    hr = device->base.KTCallbacks.pfnMakeResidentCb(device->base.hRTDevice.handle, &make_resident);

    if (hr == E_PENDING) {
        D3DDDICB_WAITFORSYNCHRONIZATIONOBJECTFROMCPU wait = {
            .ObjectCount = 1,
            .ObjectHandleArray = &device->paging.sync_object,
            .FenceValueArray = &make_resident.PagingFenceValue,
        };

        hr = device->base.KTCallbacks.pfnWaitForSynchronizationObjectFromCpuCb(device->base.hRTDevice.handle, &wait);
        if (FAILED(hr)) {
            ERROR("%s: Failed to wait for residency: 0x%08lx", __FUNCTION__, hr);
            return hr;
        }
    } else if (FAILED(hr)) {
        ERROR("%s: Failed to make resident (ms+ctf): 0x%08lx", __FUNCTION__, hr);
        return hr;
    }

    return S_OK;
}

void update_subresource(VIRTIO_WDDM_Device *device, ID3D11Resource *resource, D3D11_SUBRESOURCE_DATA *subresource_data, size_t count) {
    for (size_t i = 0; i < count; i++) {
        ID3D11DeviceContext_UpdateSubresource(
            device->base.pCtx1, resource, i, NULL, subresource_data[i].pSysMem,
            subresource_data[i].SysMemPitch, subresource_data[i].SysMemSlicePitch
        );
        ID3D11DeviceContext_Flush(device->base.pCtx1);
    }
}

void APIENTRY virtio_wddm_create_resource(D3D10DDI_HDEVICE hDevice, const D3D11DDIARG_CREATERESOURCE *pArgs, D3D10DDI_HRESOURCE hResource, D3D10DDI_HRTRESOURCE hRTResource) {
    VIRTIO_WDDM_Device *device = hDevice.pDrvPrivate;
    VIRTIO_WDDM_Resource *resource = hResource.pDrvPrivate;

    memset(resource, 0, sizeof(*resource));

    resource->base.hRTResource   = hRTResource;
    resource->base.Dimension     = pArgs->ResourceDimension;
    resource->base.Format        = pArgs->Format;
    resource->base.MipLevels     = pArgs->MipLevels;
    resource->base.ArraySize     = pArgs->ArraySize;
    resource->base.BindFlags     = pArgs->BindFlags;
    resource->base.MiscFlags     = pArgs->MiscFlags;
    resource->base.MapFlags      = pArgs->MapFlags;
    resource->base.Usage         = pArgs->Usage;
    resource->base.SampleDesc    = pArgs->SampleDesc;
    resource->base.ByteStride    = pArgs->ByteStride;

    bool bind_present = !!(pArgs->BindFlags & D3D10_DDI_BIND_PRESENT);
    /* Primary and presentable surfaces has to be allocated as 3d resources, but shared resources could be also blobs instead */
    bool is_primary = bind_present && pArgs->pPrimaryDesc != NULL;
    bool is_present = bind_present && !is_primary;
    bool is_shared = !!(pArgs->MiscFlags & D3D10_DDI_RESOURCE_MISC_SHARED);

    resource->base.IsPresentable = is_primary;
    resource->base.IsShared = is_shared;

    if (pArgs->pMipInfoList) {
        resource->base.Width  = pArgs->pMipInfoList[0].TexelWidth;
        resource->base.Height = pArgs->pMipInfoList[0].TexelHeight;
        resource->base.Depth  = pArgs->pMipInfoList[0].TexelDepth;
    }

    D3D11_USAGE usage;
    unsigned cpu_access, bind, misc;
    tritonTranslateUsage(pArgs, &usage, &cpu_access, &bind, &misc);

    if (bind_present) {
        if (usage != D3D11_USAGE_STAGING) {
            bind |= D3D11_BIND_SHADER_RESOURCE;
        }
        misc |= D3D11_RESOURCE_MISC_SHARED;
    }

    D3D11_SUBRESOURCE_DATA *subresource_data = tritonBuildInitData(pArgs);
    HRESULT hr = E_FAIL;

    bool allocate_3d = is_present || is_primary || (is_shared && dxgi_to_virgl_format(resource->base.Format) != VIRGL_FORMAT_NONE);

    INFO("%s: creating %s: %ux%u, format %u, bind %u (%u), misc %u (%u), map %u (%u), usage %u, primary desc %p, 3d %u => %p", __FUNCTION__, resource_type_name(pArgs->ResourceDimension), resource->base.Width, resource->base.Height, pArgs->Format, pArgs->BindFlags, bind, pArgs->MiscFlags, misc, pArgs->MapFlags, cpu_access, pArgs->Usage, pArgs->pPrimaryDesc, allocate_3d, resource);

    if (allocate_3d) {
        ASSERT(pArgs->ResourceDimension == D3D10DDIRESOURCE_TEXTURE2D);

        D3D11_TEXTURE2D_DESC1 desc = {
            .Width          = resource->base.Width,
            .Height         = resource->base.Height,
            .MipLevels      = resource->base.MipLevels,
            .ArraySize      = resource->base.ArraySize,
            .Format         = resource->base.Format,
            .SampleDesc     = resource->base.SampleDesc,
            .Usage          = usage,
            .BindFlags      = bind,
            .CPUAccessFlags = cpu_access,
            .MiscFlags      = misc,
            .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED,
        };

        uint32_t id;
        D3DDDI_ALLOCATIONINFO2 alloc;
        VIRTIO_WDDM_Allocate3dFull alloc_info;
        hr = create_shared_3d_resource(device, &desc, is_primary, hRTResource.handle, &resource->base.hKMResource, &alloc, &id, &alloc_info);
        if (FAILED(hr)) {
            ERROR("%s: failed to create shared 3d texture", __FUNCTION__);
            tritonSetError(&device->base, hr);
            return;
        }
        resource->base.hKMAllocation = alloc.hAllocation;

        //INFO("%s: type: %s, w %u, h %u shared %u, primary %u, id %u", __FUNCTION__, resource_type_name(pArgs->ResourceDimension), resource->base.Width, resource->base.Height, is_shared, is_primary, id);

        // TODO: we need to check that the modifier is supported, but in practice it probably is supported
        // because host uses Vulkan to query modifiers for shared 3d textures
        VkSubresourceLayout planes[4];
        for (uint32_t i = 0; i < alloc_info.num_planes; i++) {
            planes[i] = (VkSubresourceLayout) {
                .offset = alloc_info.offsets[i],
                //.size = alloc_info.sizes[i],
                .rowPitch = alloc_info.strides[i],
            };
        }

        VkImageDrmFormatModifierExplicitCreateInfoEXT modifier_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,
            .drmFormatModifier = alloc_info.modifier,
            .drmFormatModifierPlaneCount = alloc_info.num_planes,
            .pPlaneLayouts = planes,
        };

        VIRTIO_WDDM_ResourceInfo res_info = {
            .tag = VIRTIO_WDDM_ESCAPE_RESOURCE_INFO_TAG,
            .handle = alloc.hAllocation,
            .id = id,
            .info = {
                ._3d = alloc_info,
            },
        };
        D3DDDI_OPENALLOCATIONINFO2 open_alloc = {
            .hAllocation = alloc.hAllocation,
            .pPrivateDriverData = &alloc_info._3d,
            .PrivateDriverDataSize = sizeof(alloc_info._3d),
        };

        D3D10DDIARG_OPENRESOURCE open_resource = {
            .NumAllocations = 1,
            .pOpenAllocationInfo2 = &open_alloc,
            .hKMResource = {
                .handle = resource->base.hKMResource,
            },
        };
        VkD3DDDIOpenResource d3d_open = {
            .sType = VK_STRUCTURE_TYPE_D3DDDI_OPEN_RESOURCE,
            .hRTResource = hRTResource.handle,
            .pOpenResource = &open_resource,
            .pResourceInfo = &res_info,
        };
        VkImportMemoryWin32HandleInfoKHR handle_info = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
            .pNext = &d3d_open,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
            .handle = (HANDLE) (intptr_t) resource->base.hKMAllocation,
        };

        hr = create_shared_vk_image(device, &desc, VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, &modifier_info, &handle_info, &resource->memory, &resource->image, NULL);
        if (FAILED(hr)) {
            ERROR("%s: failed to import shared 3d texture to vulkan", __FUNCTION__);
            tritonSetError(&device->base, hr);
            return;
        }

        ID3D11Texture2D *tex = NULL;
        hr = DXVK_IDXGIVkInteropDevice1_CreateTexture2DFromVkImage(device->base.pDev1, &desc, resource->image, &tex);
        if (FAILED(hr)) {
            ERROR("%s: failed to import VkImage to DXVK: 0x%08lx", __FUNCTION__, hr);
            tritonSetError(&device->base, hr);
            return;
        }

        ASSERT(tex != NULL);
        resource->base.pResource = (ID3D11Resource *)tex;

        if (subresource_data) {
            update_subresource(device, resource->base.pResource, subresource_data, desc.MipLevels * desc.ArraySize);
            free(subresource_data);
        }

        return;
    } else if (is_shared) {
        ASSERT(pArgs->ResourceDimension == D3D10DDIRESOURCE_TEXTURE2D);

        D3D11_TEXTURE2D_DESC1 desc = {
            .Width          = resource->base.Width,
            .Height         = resource->base.Height,
            .MipLevels      = resource->base.MipLevels,
            .ArraySize      = resource->base.ArraySize,
            .Format         = resource->base.Format,
            .SampleDesc     = resource->base.SampleDesc,
            .Usage          = usage,
            .BindFlags      = bind,
            .CPUAccessFlags = cpu_access,
            .MiscFlags      = misc,
            .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED,
        };

        VkD3DDDICreateResource d3d_create = {
            .sType = VK_STRUCTURE_TYPE_D3DDDI_CREATE_RESOURCE,
            .hRTResource = hRTResource.handle,
            .pCreateResource11 = pArgs,
        };
        hr = create_shared_vk_image(device, &desc, VK_IMAGE_TILING_OPTIMAL, NULL, &d3d_create, &resource->memory, &resource->image, &resource->base.hKMAllocation);

        ID3D11Texture2D *tex = NULL;
        hr = DXVK_IDXGIVkInteropDevice1_CreateTexture2DFromVkImage(device->base.pDev1, &desc, resource->image, &tex);
        if (FAILED(hr)) {
            ERROR("%s: failed to import VkImage to DXVK: 0x%08lx", __FUNCTION__, hr);
            tritonSetError(&device->base, hr);
            return;
        }

        ASSERT(tex != NULL);
        resource->base.pResource = (ID3D11Resource *)tex;

        if (subresource_data) {
            update_subresource(device, resource->base.pResource, subresource_data, desc.MipLevels * desc.ArraySize);
            free(subresource_data);
        }

        return;
    }

    switch (pArgs->ResourceDimension) {
        case D3D10DDIRESOURCE_TEXTURE2D: {
            D3D11_TEXTURE2D_DESC desc = {
                .Width          = resource->base.Width,
                .Height         = resource->base.Height,
                .MipLevels      = resource->base.MipLevels,
                .ArraySize      = resource->base.ArraySize,
                .Format         = resource->base.Format,
                .SampleDesc     = resource->base.SampleDesc,
                .Usage          = usage,
                .BindFlags      = bind,
                .CPUAccessFlags = cpu_access,
                .MiscFlags      = misc,
            };

            ID3D11Texture2D *tex = NULL;
            hr = ID3D11Device1_CreateTexture2D(device->base.pDev1, &desc, subresource_data, &tex);
            if (FAILED(hr)) break;

            ASSERT(tex != NULL);
            resource->base.pResource = (ID3D11Resource *)tex;
            break;
        }
        case D3D10DDIRESOURCE_BUFFER:
        case D3D11DDIRESOURCE_BUFFEREX: {
            D3D11_BUFFER_DESC desc = {
                .ByteWidth           = resource->base.Width,
                .Usage               = usage,
                .BindFlags           = bind,
                .CPUAccessFlags      = cpu_access,
                .MiscFlags           = misc,
                .StructureByteStride = resource->base.ByteStride,
            };

            ID3D11Buffer *buf = NULL;
            hr = ID3D11Device1_CreateBuffer(device->base.pDev1, &desc, subresource_data, &buf);
            if (FAILED(hr)) break;

            ASSERT(buf != NULL);
            resource->base.pResource = (ID3D11Resource *)buf;
            break;
        }
        case D3D10DDIRESOURCE_TEXTURE1D: {
            D3D11_TEXTURE1D_DESC desc = {
                .Width          = resource->base.Width,
                .MipLevels      = resource->base.MipLevels,
                .ArraySize      = resource->base.ArraySize,
                .Format         = resource->base.Format,
                .Usage          = usage,
                .BindFlags      = bind,
                .CPUAccessFlags = cpu_access,
                .MiscFlags      = misc,
            };

            ID3D11Texture1D *tex = NULL;
            hr = ID3D11Device1_CreateTexture1D(device->base.pDev1, &desc, subresource_data, &tex);
            if (FAILED(hr)) break;

            ASSERT(tex != NULL);
            resource->base.pResource = (ID3D11Resource *)tex;
            break;
        }
        case D3D10DDIRESOURCE_TEXTURE3D: {
            D3D11_TEXTURE3D_DESC desc = {
                .Width          = resource->base.Width,
                .Height         = resource->base.Height,
                .Depth          = resource->base.Depth,
                .MipLevels      = resource->base.MipLevels,
                .Format         = resource->base.Format,
                .Usage          = usage,
                .BindFlags      = bind,
                .CPUAccessFlags = cpu_access,
                .MiscFlags      = misc,
            };

            ID3D11Texture3D *tex = NULL;
            hr = ID3D11Device1_CreateTexture3D(device->base.pDev1, &desc, subresource_data, &tex);
            if (FAILED(hr)) break;

            ASSERT(tex != NULL);
            resource->base.pResource = (ID3D11Resource *)tex;
            break;
        }
        case D3D10DDIRESOURCE_TEXTURECUBE: {
            D3D11_TEXTURE2D_DESC desc = {
                .Width          = resource->base.Width,
                .Height         = resource->base.Height,
                .MipLevels      = resource->base.MipLevels,
                .ArraySize      = resource->base.ArraySize ? resource->base.ArraySize : 6,
                .Format         = resource->base.Format,
                .SampleDesc     = resource->base.SampleDesc,
                .Usage          = usage,
                .BindFlags      = bind,
                .CPUAccessFlags = cpu_access,
                .MiscFlags      = misc | D3D11_RESOURCE_MISC_TEXTURECUBE,
            };

            ID3D11Texture2D *tex = NULL;
            hr = ID3D11Device1_CreateTexture2D(device->base.pDev1, &desc, subresource_data, &tex);
            if (FAILED(hr)) break;

            ASSERT(tex != NULL);
            resource->base.pResource = (ID3D11Resource *)tex;
            break;
        }
        default:
            ERROR("%s: unsupported resource dimension: %s", __FUNCTION__, resource_type_name(pArgs->ResourceDimension));
            hr = E_INVALIDARG;
            break;
    }

    if (FAILED(hr)) {
        ERROR("%s: failed dim=%d hr=0x%08lx", __FUNCTION__, pArgs->ResourceDimension, hr);
        tritonSetError(&device->base, hr);
        return;
    }

    if (subresource_data) {
        free(subresource_data);
    }
}

void APIENTRY virtio_wddm_open_resource(D3D10DDI_HDEVICE hDevice, const D3D10DDIARG_OPENRESOURCE *pArgs, D3D10DDI_HRESOURCE hResource, D3D10DDI_HRTRESOURCE hRTResource) {
    VIRTIO_WDDM_Device *device = hDevice.pDrvPrivate;
    VIRTIO_WDDM_Resource *resource = hResource.pDrvPrivate;
    ASSERT(pArgs->NumAllocations == 1);

    memset(resource, 0, sizeof(*resource));

    uint32_t id;
    VIRTIO_WDDM_AllocationInfo alloc_info;
    HRESULT hr = query_resource_info(device, pArgs->pOpenAllocationInfo2->hAllocation, &id, &alloc_info);
    if (FAILED(hr)) {
        ERROR("%s: failed to query resource info: hr=0x%08lx", __FUNCTION__, hr);
        tritonSetError(&device->base, hr);
        return;
    }

    resource->base.hRTResource = hRTResource;
    resource->base.Dimension   = D3D10DDIRESOURCE_TEXTURE2D;

    resource->base.BindFlags   = D3D10_DDI_BIND_RENDER_TARGET | D3D10_DDI_BIND_PRESENT | D3D10_DDI_BIND_SHADER_RESOURCE;
    resource->base.MiscFlags   = D3D10_DDI_RESOURCE_MISC_SHARED;

    resource->base.hKMResource = pArgs->hKMResource.handle;
    resource->base.hKMAllocation = pArgs->pOpenAllocationInfo2->hAllocation;

    VIRTIO_WDDM_ResourceInfo res_info = {
        .tag = VIRTIO_WDDM_ESCAPE_RESOURCE_INFO_TAG,
        .handle = resource->base.hKMAllocation,
        .id = id,
        .info = alloc_info,
    };

    if (alloc_info.tag == VIRTIO_WDDM_ALLOCATE_3D_TAG) {
        resource->base.Format    = virgl_to_dxgi_format(alloc_info._3d._3d.format);
        resource->base.Width     = alloc_info._3d._3d.width;
        resource->base.Height    = alloc_info._3d._3d.height;
        resource->base.Depth     = alloc_info._3d._3d.depth;
        resource->base.MipLevels = alloc_info._3d._3d.last_level + 1;
        resource->base.ArraySize = alloc_info._3d._3d.array_size;

        resource->base.SampleDesc  = (DXGI_SAMPLE_DESC) {
            .Count = alloc_info._3d._3d.nr_samples,
            .Quality = 0,
        };

        INFO("%s: opening 2d: %ux%u, format %u => %p", __FUNCTION__, resource->base.Width, resource->base.Height, resource->base.Format, resource);

        D3D11_TEXTURE2D_DESC1 desc = {
            .Width          = resource->base.Width,
            .Height         = resource->base.Height,
            .MipLevels      = resource->base.MipLevels,
            .ArraySize      = resource->base.ArraySize,
            .Format         = resource->base.Format,
            .SampleDesc     = resource->base.SampleDesc,
            .Usage          = D3D11_USAGE_DEFAULT,
            .BindFlags      = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
            .CPUAccessFlags = 0,
            .MiscFlags      = D3D11_RESOURCE_MISC_SHARED,
            .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED,
        };

        // TODO: we need to check that the modifier is supported, but in practice it probably is supported
        // because host uses Vulkan to query modifiers for shared 3d textures
        VkSubresourceLayout planes[4];
        for (uint32_t i = 0; i < alloc_info._3d.num_planes; i++) {
            planes[i] = (VkSubresourceLayout) {
                .offset = alloc_info._3d.offsets[i],
                .rowPitch = alloc_info._3d.strides[i],
            };
        }

        VkImageDrmFormatModifierExplicitCreateInfoEXT modifier_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,
            .drmFormatModifier = alloc_info._3d.modifier,
            .drmFormatModifierPlaneCount = alloc_info._3d.num_planes,
            .pPlaneLayouts = planes,
        };

        VkD3DDDIOpenResource d3d_open = {
            .sType = VK_STRUCTURE_TYPE_D3DDDI_OPEN_RESOURCE,
            .hRTResource = hRTResource.handle,
            .pOpenResource = pArgs,
            .pResourceInfo = &res_info,
        };
        VkImportMemoryWin32HandleInfoKHR handle_info = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
            .pNext = &d3d_open,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
            .handle = (HANDLE) (intptr_t) resource->base.hKMAllocation,
        };

        hr = create_shared_vk_image(device, &desc, VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, &modifier_info, &handle_info, &resource->memory, &resource->image, NULL);
        if (FAILED(hr)) {
            ERROR("%s: failed to import shared 3d texture to vulkan", __FUNCTION__);
            tritonSetError(&device->base, hr);
            return;
        }

        ID3D11Texture2D *tex = NULL;
        hr = DXVK_IDXGIVkInteropDevice1_CreateTexture2DFromVkImage(device->base.pDev1, &desc, resource->image, &tex);
        if (FAILED(hr)) {
            ERROR("%s: failed to import VkImage to DXVK: 0x%08lx", __FUNCTION__, hr);
            tritonSetError(&device->base, hr);
            return;
        }

        ASSERT(tex != NULL);
        resource->base.pResource = (ID3D11Resource *)tex;
    } else if (alloc_info.tag == VIRTIO_WDDM_ALLOCATE_BLOB_TAG) {
        ASSERT(alloc_info.blob.info_valid);
        ASSERT(alloc_info.blob.created);

        resource->base.Format    = virgl_to_dxgi_format(alloc_info.blob.info.format);
        resource->base.Width     = alloc_info.blob.info.width;
        resource->base.Height    = alloc_info.blob.info.height;
        resource->base.Depth     = 1;
        resource->base.MipLevels = 1;
        resource->base.ArraySize = 1;
        resource->base.SampleDesc = (DXGI_SAMPLE_DESC) {
            .Count = 1,
            .Quality = 0,
        };

        INFO("%s: opening 2d: %ux%u, format %u => %p", __FUNCTION__, resource->base.Width, resource->base.Height, resource->base.Format, resource);

        D3D11_TEXTURE2D_DESC1 desc = {
            .Width          = resource->base.Width,
            .Height         = resource->base.Height,
            .MipLevels      = resource->base.MipLevels,
            .ArraySize      = resource->base.ArraySize,
            .Format         = resource->base.Format,
            .SampleDesc     = resource->base.SampleDesc,
            .Usage          = D3D11_USAGE_DEFAULT,
            .BindFlags      = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
            .CPUAccessFlags = 0,
            .MiscFlags      = D3D11_RESOURCE_MISC_SHARED,
            .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED,
        };

        // TODO: we need to check that the modifier is supported, but in practice it probably is supported
        // because this buffer was allocated via Vulkan already
        VkSubresourceLayout planes[4];
        for (uint32_t i = 0; i < 4; i++) {
            planes[i] = (VkSubresourceLayout) {
                .offset = alloc_info.blob.info.offsets[i],
                .rowPitch = alloc_info.blob.info.strides[i],
            };
        }

        VkImageDrmFormatModifierExplicitCreateInfoEXT modifier_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,
            .drmFormatModifier = alloc_info.blob.info.modifier,
            .drmFormatModifierPlaneCount = 1, // FIXME
            .pPlaneLayouts = planes,
        };

        VkD3DDDIOpenResource d3d_open = {
            .sType = VK_STRUCTURE_TYPE_D3DDDI_OPEN_RESOURCE,
            .hRTResource = hRTResource.handle,
            .pOpenResource = pArgs,
            .pResourceInfo = &res_info,
        };
        VkImportMemoryWin32HandleInfoKHR handle_info = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
            .pNext = &d3d_open,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
            .handle = (HANDLE) (intptr_t) resource->base.hKMAllocation,
        };

        if (alloc_info.blob.info.modifier != 0x00FFFFFFFFFFFFFF) {
            hr = create_shared_vk_image(device, &desc, VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, &modifier_info, &handle_info, &resource->memory, &resource->image, NULL);
        } else {
            hr = create_shared_vk_image(device, &desc, VK_IMAGE_TILING_OPTIMAL, NULL, &handle_info, &resource->memory, &resource->image, NULL);
        }
        if (FAILED(hr)) {
            ERROR("%s: failed to import shared blob to vulkan", __FUNCTION__);
            tritonSetError(&device->base, hr);
            return;
        }

        ID3D11Texture2D *tex = NULL;
        hr = DXVK_IDXGIVkInteropDevice1_CreateTexture2DFromVkImage(device->base.pDev1, &desc, resource->image, &tex);
        if (FAILED(hr)) {
            ERROR("%s: failed to import VkImage to DXVK: 0x%08lx", __FUNCTION__, hr);
            tritonSetError(&device->base, hr);
            return;
        }

        ASSERT(tex != NULL);
        resource->base.pResource = (ID3D11Resource *)tex;
    } else {
        ERROR("%s: Invalid allocation type: 0x%016x", __FUNCTION__, alloc_info.tag);
        tritonSetError(&device->base, E_INVALIDARG);
    }
}

void APIENTRY virtio_wddm_destroy_resource(D3D10DDI_HDEVICE hDevice, D3D10DDI_HRESOURCE hResource) {

    VIRTIO_WDDM_Device *device = hDevice.pDrvPrivate;
    VIRTIO_WDDM_Resource *resource = hResource.pDrvPrivate;
    INFO("%s: resource=%p", __FUNCTION__, resource);

    if (resource->base.pResource) {
        ID3D11Resource_Release(resource->base.pResource);
    }

    if (resource->image != VK_NULL_HANDLE) {
        device->vk_DestroyImage(device->vk, resource->image, NULL);
    }

    if (resource->memory != VK_NULL_HANDLE) {
        device->vk_FreeMemory(device->vk, resource->memory, NULL);
    }

    if (resource->base.hKMAllocation != 0) {
        D3DDDICB_DEALLOCATE deallocate = {
           .hResource = resource->base.hRTResource.handle,
           .NumAllocations = 0,
           .HandleList = NULL,
        };
        HRESULT hr = device->base.KTCallbacks.pfnDeallocateCb(device->base.hRTDevice.handle, &deallocate);
        if (FAILED(hr)) {
            ERROR("%s: Failed to deallocate: 0x%08lx", __FUNCTION__, hr);
        }
    }

    memset(resource, 0, sizeof(*resource));
}

#include <stdio.h>

// FIXME: this is only correct for RGBA textures
void virtio_wddm_save_texture_to_ppm(VIRTIO_WDDM_Device *device, VIRTIO_WDDM_Resource *resource, const char *filename) {
    if (resource->base.Dimension != D3D10DDIRESOURCE_TEXTURE2D) {
        ERROR("%s: Only 2D textures are supported, not %d", __FUNCTION__, resource->base.Dimension);
        return;
    }

    D3D11_TEXTURE2D_DESC desc = {
        .Width          = resource->base.Width,
        .Height         = resource->base.Height,
        .MipLevels      = resource->base.MipLevels,
        .ArraySize      = resource->base.ArraySize,
        .Format         = resource->base.Format,
        .SampleDesc     = resource->base.SampleDesc,
        .Usage          = D3D11_USAGE_STAGING,
        .BindFlags      = 0,
        .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
        .MiscFlags      = 0,
    };

    ID3D11Texture2D *staging = NULL;
    HRESULT hr = ID3D11Device1_CreateTexture2D(device->base.pDev1, &desc, NULL, &staging);
    if (FAILED(hr)) {
        ERROR("%s: Failed to create staging texture: 0x%08lx", __FUNCTION__, hr);
        return;
    }
    ASSERT(staging != NULL);

    ID3D11DeviceContext1_CopyResource(device->base.pCtx1, (ID3D11Resource *) staging, resource->base.pResource);
    ID3D11DeviceContext1_Flush(device->base.pCtx1);

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = ID3D11DeviceContext1_Map(device->base.pCtx1, (ID3D11Resource *) staging, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)){
        ERROR("%s: Failed to map staging texture: 0x%08lx", __FUNCTION__, hr);
        return;
    }

    {
        FILE *ppm = fopen(filename, "wb");
        fprintf(ppm, "P6\n");
        fprintf(ppm, "%u %u\n", mapped.RowPitch / 4, desc.Height);
        fprintf(ppm, "255\n");
        for (size_t y = 0; y < desc.Height; y++) {
            for (size_t x = 0; x < desc.Width; x++) {
                union {
                    uint32_t u32;
                    struct {
                        uint8_t r;
                        uint8_t g;
                        uint8_t b;
                        uint8_t a;
                    } color;
                } pixel = {
                    .u32 = *(((uint32_t *) (((uint8_t *) mapped.pData) + y * mapped.RowPitch)) + x),
                };
                fwrite(&pixel.color.r, 1, 1, ppm);
                fwrite(&pixel.color.g, 1, 1, ppm);
                fwrite(&pixel.color.b, 1, 1, ppm);
            }
        }
        fclose(ppm);
    }

    INFO("%s: Saved texture %p (rt %p, km 0x%08x) to %s", __FUNCTION__, resource, resource->base.hRTResource, resource->base.hKMAllocation, filename);

    ID3D11DeviceContext1_Unmap(device->base.pCtx1, (ID3D11Resource *) staging, 0);
    ID3D11Texture2D_Release(staging);
}
