#pragma once

#include <triton.h>

typedef struct {
    TRITON_ADAPTER base;
    LUID luid;
    VIRTIO_WDDM_CapsetMask supported_capsets;
} VIRTIO_WDDM_Adapter;

typedef enum {
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
} __Log_Level;

#define TODO(...) do { virtio_wddm_log(__FILE__, __LINE__, LOG_LEVEL_FATAL, "TODO", __VA_ARGS__); abort(); } while (0)
#define UNREACHABLE(...) do { virtio_wddm_log(__FILE__, __LINE__, LOG_LEVEL_FATAL, "UNREACHABLE", __VA_ARGS__); abort(); } while (0)
#define ERROR(...) virtio_wddm_log(__FILE__, __LINE__, LOG_LEVEL_ERROR, "ERROR", __VA_ARGS__)
#define INFO(...) virtio_wddm_log(__FILE__, __LINE__, LOG_LEVEL_INFO,  "INFO", __VA_ARGS__)
#define ASSERT(expr) if (!(expr)) { UNREACHABLE("Assertion failed: %s", #expr); }
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

static inline void free_pointer(void *ptr) { free(*(void **)ptr); }

#define CLEANUP_FREE __attribute__((cleanup(free_pointer)))

void virtio_wddm_log(const char *file, int line, __Log_Level level, const char *label, const char *format, ...) __attribute__((format(printf, 5, 6)));

typedef struct {
    const char *func;
    const char *file;
    int line;
} __Scope_Trace;

static inline __Scope_Trace __trace_scope_begin(const char *file, int line, const char *func) {
    virtio_wddm_log(file, line, LOG_LEVEL_TRACE, "TRACE", "BEGIN: %s", func);
    return (__Scope_Trace) { func, file, line };
}

static inline void __trace_scope_end(void *p) {
    __Scope_Trace *trace = p;
    virtio_wddm_log(trace->file, trace->line, LOG_LEVEL_TRACE, "TRACE", "END: %s", trace->func);
}

#define TRACE() __Scope_Trace __scope_trace__ __attribute__((cleanup(__trace_scope_end), unused)) = __trace_scope_begin(__FILE__, __LINE__, __FUNCTION__)
