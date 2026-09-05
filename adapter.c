#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef ERROR_GRAPHICS_DRIVER_MISMATCH
#define ERROR_GRAPHICS_DRIVER_MISMATCH _HRESULT_TYPEDEF_(0xC0262009L)
#endif

#include <winddk_compat.h>
#include <d3d10umddi.h>
#include <d3d11.h>
#include <d3dumddi.h>

#include <triton.h>
#include <virtio_wddm_uapi.h>

#undef ERROR
#include "adapter.h"
#include "device.h"

#define NO_DEBUG

#ifdef NO_DEBUG
extern void triton_log_raw(const char *line) {
}

void virtio_wddm_log(const char *file, int line, const char *label, const char *format, ...) {
}

extern int __cdecl DXVK_umd_log_output(const char *line) {
    return 0;
}
#else
extern void print_log_raw(const char *line) {
    OutputDebugStringA(line);

    static FILE *out = NULL;
    if (out == NULL) {
        out = fopen("C:\\dx11um_log.txt", "a+");
    }

    if (out != NULL) {
        fwrite(line, strlen(line), 1, out);
        fflush(out);
    }
}

void virtio_wddm_log(const char *file, int line, const char *label, const char *format, ...) {
#if 0
    fprintf(stderr, "%s:%d: %s: ", file, line, label);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
#else
    char buf[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    size_t len = strlen(buf);
    if (len == sizeof(buf) - 1) {
        buf[sizeof(buf)-2] = '\n';
    } else {
        buf[len] = '\n';
        buf[len+1] = '\0';
    }
    va_end(args);
    print_log_raw(buf);
#endif
}

extern int __cdecl DXVK_umd_log_output(const char *line) {
    print_log_raw(line);
    return 0;
}

extern void triton_log_raw(const char *line) {
    print_log_raw(line);
}
#endif

static HRESULT APIENTRY virtio_wddm_close_adapter(D3D10DDI_HADAPTER hAdapter) {
    TRACE();
    free(hAdapter.pDrvPrivate);
    return S_OK;
}

static void dump_capsets(char *buf, size_t len, VIRTIO_WDDM_CapsetMask mask) {
    static const struct {
        VIRTIO_WDDM_CapsetMask value;
        const char *name;
    } mask_table[] = {
        { VIRTIO_WDDM_CAPSET_MASK_VIRGL,        "virgl"        },
        { VIRTIO_WDDM_CAPSET_MASK_VIRGL2,       "virgl2"       },
        { VIRTIO_WDDM_CAPSET_MASK_GFXSTREAM,    "gfxstream"    },
        { VIRTIO_WDDM_CAPSET_MASK_VENUS,        "venus"        },
        { VIRTIO_WDDM_CAPSET_MASK_CROSS_DOMAIN, "cross_domain" },
        { VIRTIO_WDDM_CAPSET_MASK_DRM,          "drm"          },
    };

    buf[0] = 0;

    size_t pos = 0;
    bool first = true;

    for (size_t i = 0; i < ARRAY_SIZE(mask_table); i++) {
        if (!(mask & mask_table[i].value)) continue;
        if (!first) {
            int ret = snprintf(&buf[pos], len - pos, " | ");
            if (ret < 0 || (size_t)ret >= len - pos) {
                break;
            }
            pos += ret;
        }
        first = false;

        int ret = snprintf(&buf[pos], len - pos, "%s", mask_table[i].name);
        if (ret < 0 || (size_t)ret >= len - pos)
            break;
        pos += ret;
    }

    if (first) {
        snprintf(buf, len, "NONE");
    }
}

__attribute__((visibility("default"))) HRESULT APIENTRY OpenAdapter10_2(D3D10DDIARG_OPENADAPTER* pArgs) {
    TRACE();
    VIRTIO_WDDM_AdapterInfo adapter_info_priv = { 0 };
    D3DDDICB_QUERYADAPTERINFO query_adapter_info = {
        .pPrivateDriverData = &adapter_info_priv,
        .PrivateDriverDataSize = sizeof(adapter_info_priv),
    };

    HRESULT hr = pArgs->pAdapterCallbacks->pfnQueryAdapterInfoCb(pArgs->hRTAdapter.handle, &query_adapter_info);
    if (FAILED(hr)) {
        return hr;
    };
    if (adapter_info_priv.tag != VIRTIO_WDDM_ADAPTER_INFO_TAG || !adapter_info_priv.supports_3d || !adapter_info_priv.has_shmem) {
        ERROR("Invalid driver: tag %llu, 3d %u, shmem %u", adapter_info_priv.tag, adapter_info_priv.supports_3d, adapter_info_priv.has_shmem);
        return ERROR_GRAPHICS_DRIVER_MISMATCH;
    }

    char capsets[4096];
    dump_capsets(capsets, sizeof(capsets), adapter_info_priv.capset_mask);
    INFO("%s: Supported capsets: %s", __FUNCTION__, capsets);

    if (!(adapter_info_priv.capset_mask & (VIRTIO_WDDM_CAPSET_MASK_VENUS | VIRTIO_WDDM_CAPSET_MASK_DRM))) {
        INFO("%s: Adapter does not support nor Venus nor vDRM", __FUNCTION__);
        return ERROR_GRAPHICS_DRIVER_MISMATCH;
    }

    VIRTIO_WDDM_Adapter *adapter = calloc(sizeof(*adapter), 1);
    if (!adapter) return E_OUTOFMEMORY;

    static_assert(sizeof(adapter->luid) == sizeof(adapter_info_priv.luid));
    memcpy(&adapter->luid, &adapter_info_priv.luid, sizeof(adapter->luid));
    adapter->supported_capsets = adapter_info_priv.capset_mask;

    adapter->base.hRTAdapter = pArgs->hRTAdapter;
    adapter->base.uIfVersion = pArgs->Interface;
    adapter->base.uRtVersion = pArgs->Version;
    adapter->base.KTCallbacks = *pArgs->pAdapterCallbacks;

    pArgs->hAdapter.pDrvPrivate = adapter;
    pArgs->pAdapterFuncs_2->pfnCalcPrivateDeviceSize = virtio_wddm_calc_device_size;
    pArgs->pAdapterFuncs_2->pfnCreateDevice          = virtio_wddm_create_device;
    pArgs->pAdapterFuncs_2->pfnCloseAdapter          = virtio_wddm_close_adapter;
    pArgs->pAdapterFuncs_2->pfnGetSupportedVersions  = tritonGetSupportedVersions;
    // TODO: we could create a temporary DXVK device from LUID and query caps from it.
    // NOTE: this device MUST NOT be used for anything else.
    pArgs->pAdapterFuncs_2->pfnGetCaps               = tritonGetCaps;

    return S_OK;
}
