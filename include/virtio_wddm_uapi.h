#ifndef __VIRTIO_WDDM_UAPI_H__
#define __VIRTIO_WDDM_UAPI_H__

#include <stdint.h>
#include <d3dkmthk.h>

#define VIRTIO_WDDM_PCI_VENDOR_ID 6900

#define VIRTIO_WDDM_PCI_DEVICE_ID 26985

#define VIRTIO_WDDM_ADAPTER_INFO_TAG 5066067239879590230ull

#define VIRTIO_WDDM_ESCAPE_CAPSET_TAG 6003370060433016150ull

#define VIRTIO_WDDM_ESCAPE_RESOURCE_INFO_TAG 5066067248504063318ull

#define VIRTIO_WDDM_ESCAPE_RESOURCE_BUSY_TAG 6004778599252706646ull

#define VIRTIO_WDDM_ESCAPE_BLOB_MAP_TAG 4778391004737914198ull

#define VIRTIO_WDDM_ESCAPE_CONTEXT_INIT_TAG 5285066810588349782ull

#define VIRTIO_WDDM_ESCAPE_BLIT_INIT_TAG 6073469419966907734ull

#define VIRTIO_WDDM_ESCAPE_EXEC_BUF_TAG 4847377628439725398ull

#define VIRTIO_WDDM_CREATE_RESOURCE_TAG 5932161353946775894ull

#define VIRTIO_WDDM_ALLOCATE_3D_TAG 5496436857343066454ull

#define VIRTIO_WDDM_ALLOCATE_BLOB_TAG 4778121577295003990ull

#define VIRTIO_WDDM_SUBMIT_COMMAND_VIRTUAL_TAG 6216178336949162838ull

#define VIRTIO_WDDM_MAX_SUBMIT_COMMAND_VIRTUAL_SIZE 8192

enum VIRTIO_WDDM_CapsetId
#if __STDC_VERSION__ >= 202311L
  : uint32_t
#endif // __STDC_VERSION__ >= 202311L
 {
    VIRTIO_WDDM_CAPSET_ID_VIRGL = 1,
    VIRTIO_WDDM_CAPSET_ID_VIRGL2 = 2,
    VIRTIO_WDDM_CAPSET_ID_GFXSTREAM = 3,
    VIRTIO_WDDM_CAPSET_ID_VENUS = 4,
    VIRTIO_WDDM_CAPSET_ID_CROSS_DOMAIN = 5,
    VIRTIO_WDDM_CAPSET_ID_DRM = 6,
};
#if __STDC_VERSION__ >= 202311L
typedef enum VIRTIO_WDDM_CapsetId VIRTIO_WDDM_CapsetId;
#else
typedef uint32_t VIRTIO_WDDM_CapsetId;
#endif // __STDC_VERSION__ >= 202311L

enum VIRTIO_WDDM_CommandId
#if __STDC_VERSION__ >= 202311L
  : uint16_t
#endif // __STDC_VERSION__ >= 202311L
 {
    VIRTIO_WDDM_COMMAND_ID_NOP = 0,
    VIRTIO_WDDM_COMMAND_ID_SUBMIT = 1,
    VIRTIO_WDDM_COMMAND_ID_TRANSFER_TO_HOST = 2,
    VIRTIO_WDDM_COMMAND_ID_TRANSFER_FROM_HOST = 3,
    VIRTIO_WDDM_COMMAND_ID_FENCE = 4,
    VIRTIO_WDDM_COMMAND_ID_MAP_APERTURE = 61440,
    VIRTIO_WDDM_COMMAND_ID_UNMAP_APERTURE = 61441,
};
#if __STDC_VERSION__ >= 202311L
typedef enum VIRTIO_WDDM_CommandId VIRTIO_WDDM_CommandId;
#else
typedef uint16_t VIRTIO_WDDM_CommandId;
#endif // __STDC_VERSION__ >= 202311L

typedef uint64_t VIRTIO_WDDM_CapsetMask;
#define VIRTIO_WDDM_CAPSET_MASK_VIRGL (uint64_t)(1ull << 1)
#define VIRTIO_WDDM_CAPSET_MASK_VIRGL2 (uint64_t)(1ull << 2)
#define VIRTIO_WDDM_CAPSET_MASK_GFXSTREAM (uint64_t)(1ull << 3)
#define VIRTIO_WDDM_CAPSET_MASK_VENUS (uint64_t)(1ull << 4)
#define VIRTIO_WDDM_CAPSET_MASK_CROSS_DOMAIN (uint64_t)(1ull << 5)
#define VIRTIO_WDDM_CAPSET_MASK_DRM (uint64_t)(1ull << 6)

typedef struct __attribute__((packed)) {
    uint64_t tag;
    uint64_t luid;
    VIRTIO_WDDM_CapsetMask capset_mask;
    bool supports_3d;
    bool has_shmem;
} VIRTIO_WDDM_AdapterInfo;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    uint32_t target;
    uint32_t format;
    uint32_t bind;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t array_size;
    uint32_t last_level;
    uint32_t nr_samples;
    uint32_t flags;
    uint64_t size;
} VIRTIO_WDDM_Allocate3d;

typedef uint32_t VIRTIO_WDDM_BlobMem;
#define VIRTIO_WDDM_BLOB_MEM_GUEST (uint32_t)(1u << 0)
#define VIRTIO_WDDM_BLOB_MEM_HOST3D (uint32_t)(1u << 1)
#define VIRTIO_WDDM_BLOB_MEM_HOST3D_GUEST (uint32_t)((1u << 0) | (1u << 1))

typedef uint32_t VIRTIO_WDDM_BlobFlag;
#define VIRTIO_WDDM_BLOB_FLAG_NONE (uint32_t)0
#define VIRTIO_WDDM_BLOB_FLAG_MAPPABLE (uint32_t)(1u << 0)
#define VIRTIO_WDDM_BLOB_FLAG_SHAREABLE (uint32_t)(1u << 1)
#define VIRTIO_WDDM_BLOB_FLAG_CROSS_DEVICE (uint32_t)(1u << 2)

typedef struct __attribute__((packed)) {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t bind;
} VIRTIO_WDDM_BlobInfo;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    uint64_t id;
    VIRTIO_WDDM_BlobMem mem;
    VIRTIO_WDDM_BlobFlag flags;
    uint64_t size;
    VIRTIO_WDDM_BlobInfo info;
} VIRTIO_WDDM_AllocateBlob;

typedef union __attribute__((packed)) {
    uint64_t tag;
    VIRTIO_WDDM_Allocate3d _3d;
    VIRTIO_WDDM_AllocateBlob blob;
} VIRTIO_WDDM_CreateAllocationInfo;

typedef struct __attribute__((packed)) {
    uint64_t offset;
    uint64_t size;
    uint64_t stride;
} VIRTIO_WDDM_PlaneLayout;

typedef struct __attribute__((packed)) {
    VIRTIO_WDDM_CreateAllocationInfo base;
    uint32_t res_id;
    uint32_t num_planes;
    uint64_t modifier;
    VIRTIO_WDDM_PlaneLayout planes[4];
} VIRTIO_WDDM_CreateAllocation;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    uint8_t cmd[0];
} VIRTIO_WDDM_CreateResource;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    VIRTIO_WDDM_CapsetId capset_id;
    uint32_t version;
    uint8_t capset[0];
} VIRTIO_WDDM_Capset;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    VIRTIO_WDDM_CapsetId capset_id;
    uint32_t num_rings;
    uint8_t debug_name[64];
} VIRTIO_WDDM_ContextInit;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    D3DKMT_HANDLE handle;
    uint32_t padding;
    VIRTIO_WDDM_CreateAllocation info;
} VIRTIO_WDDM_ResourceInfo;

typedef union {
    HANDLE handle;
    uint64_t wow64;
} VIRTIO_WDDM_HANDLE64;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    VIRTIO_WDDM_HANDLE64 event;
    D3DKMT_HANDLE handle;
    bool wait;
    bool is_busy;
} VIRTIO_WDDM_ResourceBusy;

typedef uint32_t VIRTIO_WDDM_BlobMapFlags;
#define VIRTIO_WDDM_BLOB_MAP_FLAGS_UNMAP (uint32_t)(1u << 0)

typedef union {
    uint8_t *ptr;
    uint64_t wow64;
} VIRTIO_WDDM_Pointer64;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    D3DKMT_HANDLE handle;
    VIRTIO_WDDM_BlobMapFlags flags;
    VIRTIO_WDDM_Pointer64 ptr;
} VIRTIO_WDDM_BlobMap;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    uint64_t fence_id;
    uint8_t cmd[0];
} VIRTIO_WDDM_ExecBuffer;

typedef union __attribute__((packed)) {
    uint64_t tag;
    VIRTIO_WDDM_Capset caps_req;
    VIRTIO_WDDM_ContextInit ctx_init;
    VIRTIO_WDDM_ResourceInfo res_info;
    VIRTIO_WDDM_ResourceBusy res_busy;
    VIRTIO_WDDM_BlobMap blob_map;
    VIRTIO_WDDM_ExecBuffer exec_buf;
} VIRTIO_WDDM_Escape;

typedef uint8_t VIRTIO_WDDM_CommandFlag;
#define VIRTIO_WDDM_COMMAND_FLAG_RING_IDX (uint8_t)(1 << 0)
#define VIRTIO_WDDM_COMMAND_FLAG_SHADOW_VIRGL (uint8_t)(1 << 1)

typedef struct __attribute__((packed)) {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} VIRTIO_WDDM_Box3D;

typedef struct __attribute__((packed)) {
    uint32_t res_id;
    uint32_t stride;
    uint64_t offset;
    uint32_t level;
    uint32_t layer_stride;
    VIRTIO_WDDM_Box3D box;
} VIRTIO_WDDM_CommandTransfer;

typedef union __attribute__((packed)) {
    uint8_t submit[0];
    VIRTIO_WDDM_CommandTransfer transfer[0];
} VIRTIO_WDDM_Commands;

typedef struct __attribute__((packed)) {
    VIRTIO_WDDM_CommandId id;
    VIRTIO_WDDM_CommandFlag flags;
    uint8_t ring;
    uint32_t size;
    VIRTIO_WDDM_Commands body[0];
} VIRTIO_WDDM_CommandHeader;

typedef struct __attribute__((packed)) {
    uint64_t tag;
    uint8_t cmd[0];
} VIRTIO_WDDM_SubmitCommand;

#endif  /* __VIRTIO_WDDM_UAPI_H__ */
