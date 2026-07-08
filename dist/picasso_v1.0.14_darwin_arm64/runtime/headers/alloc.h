#ifndef ALLOC_H
#define ALLOC_H

#include "platform.h"
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#define DEBUG_MODE 0

#define Debug(fmt, ...) \
    if(DEBUG_MODE) \
        fprintf(stderr, "\n[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)


#define TEST_MODE 0

#define HEAP_BASE_SIZE (1024 * 1024) // 128KB
#define HEAP_MAX_SIZE (10737418240) // 10GB
#define HEAP_EXPONENTIAL_GROWTH_LIMIT (64 * 1024 * 1024) // 64MB
#define HEAP_CONSTANT_GROWTH  (64 * 1024 * 1024) // 64MB
/* 
    expo_iterations = log2(HEAP_EXPONENTIAL_GROWTH_LIMIT / HEAP_BASE_SIZE)
    linear_iterations = (HEAP_MAX_SIZE - HEAP_EXPONENTIAL_GROWTH_LIMI) / HEAP_CONSTANT_GROWTH
    MAX_HEAP_GROWTH_ITERATIONS = expo_iteratons + linear_iterations
*/
#define MAX_HEAP_GROWTH_ITERATIONS 159

#define MMAP_THRESHOLD 131072
#define ALIGNMENT 16 
#define HEADER_SIZE (sizeof(size_t) * 2) // prev_size + size
#define MIN_PAYLOAD_SIZE 32
#define MIN_PAYLOAD_SIZE_FOR_LARGEBIN 16 + (sizeof(size_t) * 2)
#define FASTBINS_COUNT 7
#define SMALLBINS_COUNT 32
#define LARGEBINS_COUNT 64
#define SMALLBIN_MAX_SIZE 512
#define LARGEBIN_MAX_INDEX 63
#define HEAP_BOUNDARY_SIZE (4 * sizeof(size_t))

#define __PREV_IN_USE_FLAG_MASK 0x1
#define __GC_MARK_FLAG_MASK 0x2 /* will be set in prev_size field */
#define __CURR_IN_USE_FLAG_MASK 0x4
#define __MMAP_ALLOCATED_FLAG_MASK 0x2
#define __SIZE_BITS 0x7
#define __CHUNK_SIZE_MASK (~__SIZE_BITS)

typedef struct free_chunk {
    size_t prev_size;
    size_t size; /* contains size + flags */
    struct free_chunk* fd;
    struct free_chunk* bk;

    /*  Only used for large bins, but defined for generic structure */
    struct free_chunk* next_sizeptr;
    struct free_chunk* prev_sizeptr;
} free_chunk_t;

typedef struct inuse_chunk {
    size_t prev_size;
    size_t size; /* contains size + flags */
} inuse_chunk_t;
    
typedef struct alloced_heap {
    char* start;
    char* end;
} alloced_heap_t;

typedef struct arena {   
    /* stores non-sentinal nodes in singly linked list */
    free_chunk_t* fastbins[FASTBINS_COUNT];  

    /* remaining heap memory */
    free_chunk_t* top_chunk;

    /* Head of the list*/
    free_chunk_t* unsortedbin;
    
    free_chunk_t* smallbins[SMALLBINS_COUNT]; // Sentinels
    free_chunk_t* largebins[LARGEBINS_COUNT]; // Sentinels
    
    unsigned int smallbinmap;
    unsigned int largebinmap;

    /* rwmutex to prevent two threads allocating on same arena */
    pthread_mutex_t mu;

    int heap_expo_growth_iters;
    int heap_constant_growth_iters;

    alloced_heap_t alloced_heaps[MAX_HEAP_GROWTH_ITERATIONS];
    int alloced_heap_count;

} arena_t;


/**
 * @brief Create a new arena allocator
 * @return Pointer to the newly created arena
 */
arena_t* arena_create(void);

/**
 * @brief Release allocated memory back to the arena
 * @param ar Pointer to the arena
 * @param ptr Pointer to the memory to release
 */
void release(arena_t* ar, void* ptr);

/**
 * @brief Allocate memory from the arena
 * @param ar Pointer to the arena
 * @param requested_size Size of memory to allocate in bytes
 * @return Pointer to the allocated memory
 */
void* allocate(arena_t* ar, size_t requested_size);

// utils
typedef struct arena_stats {
    size_t top_chunk_size;
    size_t total_allocated_so_far;
    size_t total_allocated_effective;

    int count_of_heap_growth;

} arena_stats_t;

#endif