#ifndef GGC_H
#define GGC_H

#include "platform.h"
/**
 * @brief Initialize the runtime and garbage collector.
 * 
 * Must be called once at program startup before any GC-managed allocations.
 * Sets up internal GC data structures and prepares the system for memory management.
 */
void runtime_init(void);

/**
 * @brief Allocate memory managed by the garbage collector.
 * 
 * The returned memory will be automatically scanned and reclaimed by the GC
 * when no longer reachable. Suitable for objects containing pointers.
 * 
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory (never NULL if GC initialized correctly).
 */
void *__public__alloc(long size);

/**
 * @brief Allocate memory that the GC should not scan.
 * 
 * Useful for raw buffers, strings, or data that contains no pointers.
 * The GC will still manage the memory’s lifetime but will not scan its contents.
 * 
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory.
 */
void *lang_alloc_atomic(long size);

/**
 * @brief Force a garbage collection cycle.
 * 
 * Primarily used for debugging or testing memory usage.
 * Scans all GC roots and reclaims unreachable memory immediately.
 */
void runtime_collect(void);

#endif