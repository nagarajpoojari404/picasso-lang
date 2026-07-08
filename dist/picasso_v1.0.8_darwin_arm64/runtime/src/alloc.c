#include "platform.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <stddef.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <limits.h>

#include "alloc.h"

/* utils */
static inline size_t align16(size_t size) {
    return (size + 15) & ~(size_t)15;
}

static inline size_t align_page(size_t size) {
    static size_t page_size = 0;
    if (!page_size) page_size = (size_t)sysconf(_SC_PAGESIZE);
    return (size + page_size - 1) & ~(page_size - 1);
}

static void* alloc_memory(size_t size) {
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(p == MAP_FAILED) return NULL;
    return p; 
}


static inline bool is_prev_inuse(free_chunk_t* fc) {
    return fc->size & __PREV_IN_USE_FLAG_MASK;
}

static inline void set_prev_inuse(free_chunk_t* fc) {
    fc->size |= __PREV_IN_USE_FLAG_MASK;
}

static inline void unset_prev_inuse(free_chunk_t* fc) {
    fc->size &= ~(size_t)__PREV_IN_USE_FLAG_MASK;
}

static inline bool is_curr_inuse(free_chunk_t* fc) {
    return fc->size & __CURR_IN_USE_FLAG_MASK;
}

static inline void set_curr_inuse(free_chunk_t* fc) {
    fc->size |= __CURR_IN_USE_FLAG_MASK;
}

static inline void unset_curr_inuse(free_chunk_t* fc) {
    fc->size &= ~(size_t)__CURR_IN_USE_FLAG_MASK;
}

__attribute__((unused)) static inline bool is_mmap_alloced(free_chunk_t* fc) {
    return fc->size & __MMAP_ALLOCATED_FLAG_MASK;
}

__attribute__((unused)) static inline void set_mmap_flag(free_chunk_t* fc) {
    fc->size |= __MMAP_ALLOCATED_FLAG_MASK;
}

static inline void unset_mmap_flag(free_chunk_t* fc) {
    fc->size &= ~(size_t)__MMAP_ALLOCATED_FLAG_MASK;
}

static inline ssize_t get_size(free_chunk_t* fc) {
    return (ssize_t)(fc->size & (size_t)__CHUNK_SIZE_MASK);
}

static inline ssize_t get_prev_size(free_chunk_t* fc) {
    return (ssize_t)(fc->prev_size & (size_t)__CHUNK_SIZE_MASK);
}

static inline ssize_t get_size_flags(free_chunk_t* fc) {
    return fc->size & __SIZE_BITS;
}

static inline ssize_t get_prev_size_flags(free_chunk_t* fc) {
    return fc->prev_size & __SIZE_BITS;
}

static inline void set_size(free_chunk_t* fc, ssize_t size, ssize_t flags) {
    fc->size = (size_t)size | (size_t)flags;
}

static inline void set_prev_size(free_chunk_t* fc, ssize_t size, ssize_t flags) {
    assert(size != 0);
    fc->prev_size = (size_t)size | (size_t)flags;
}

static inline free_chunk_t* next_chunk(free_chunk_t* fc) {
    return (free_chunk_t*)(
        (char*)fc + HEADER_SIZE + get_size(fc)
    );
}

static inline free_chunk_t* prev_chunk(free_chunk_t* fc) {
    /* unsafe */
    return (free_chunk_t*)(
        (char*)fc - HEADER_SIZE - get_prev_size(fc)
    );
}

// static void __dump__chunk(free_chunk_t *c) {
//     printf("CHUNK chunk @ %p\n", (void*)c);
//     printf("  prev_size     = %zu\n", c->prev_size);
//     printf("  size          = %zu\n", get_size(c));
//     printf("  size(raw)          = %zu\n", c->size);
//     printf("  flags         = [inuse=%d mmapped=%d prev_inuse=%d]\n",
//              is_curr_inuse(c),
//              is_mmap_alloced(c),
//              is_prev_inuse(c));
//     printf("  fd            = %p\n", (void*)c->fd);
//     printf("  bk            = %p\n", (void*)c->bk);
//     printf("  next_sizeptr  = %p\n", (void*)c->next_sizeptr);
//     printf("  prev_sizeptr  = %p\n", (void*)c->prev_sizeptr);
// }


static inline void set_fdbk_to(free_chunk_t* fc, free_chunk_t* v) {
    fc->fd = fc->bk = v;
}

static inline void set_nextprev_sizeptr_to(free_chunk_t* fc, free_chunk_t* v) {
    fc->next_sizeptr = fc->prev_sizeptr = v;
}

static free_chunk_t* alloc_chunk(size_t size) {
    size_t total_size = align16(size + HEADER_SIZE);
    free_chunk_t* fc = (free_chunk_t*)alloc_memory(total_size);
    assert(fc != NULL);
    
    /* init fields with default values */
    set_size(fc, (ssize_t)size, __MMAP_ALLOCATED_FLAG_MASK);
    set_fdbk_to(fc, NULL);
    
    return fc;
}

static void unlink_chunk_from_fdlist(free_chunk_t* p) {
    if(!p->bk || !p->fd) return;
    
    p->fd->bk = p->bk;
    p->bk->fd = p->fd;
    p->fd = p->bk = NULL;
}

static void unlink_chunk_from_sortedlist(free_chunk_t* p) {
    if(!p->prev_sizeptr || !p->next_sizeptr) return;

    p->prev_sizeptr->next_sizeptr = p->next_sizeptr;
    p->next_sizeptr->prev_sizeptr = p->prev_sizeptr;
    p->next_sizeptr = p->prev_sizeptr = NULL;
}

static void insert_into_fdlist(free_chunk_t* head, free_chunk_t* p) {
    p->fd = head->fd;
    p->bk = head;
    head->fd->bk = p;
    head->fd = p;
}

static inline int get_smallbin_index(size_t size) {
    return (int)((size >> 4) - 1);
}

static inline int floor_log2(size_t x) {
    return (int)(sizeof(size_t) * CHAR_BIT - 1 - (unsigned long)__builtin_clzl(x));
}

static int get_largebin_index(size_t size) {
    /* large bins start at 512 bytes */
    if (size < 512)
        return -1;

    /* 0–31: 64-byte steps (512 B – 64 KB) */
    if (size <= 64 * 1024) {
        int idx = (int)((size - 512) >> 6);   // /64
        if (idx > 31) idx = 31;
        return idx;
    }

    /*
     * 32–63: logarithmic bins
     * Each bin represents one power-of-two size class
     */
    int lg = floor_log2(size);

    /*
     * 64 KB = 2^16 → bin 32
     * Max size_t (~2^63) → bin 63
     */
    int idx = 32 + (lg - 16);

    if (idx < 32) idx = 32;
    if (idx > 63) idx = 63;

    return idx;
}


static void grow_heap(arena_t* ar) {
    /* exponential doubling till 64MB then increase by constant 64MB */
    size_t next_heap_size;

    if (HEAP_BASE_SIZE << ar->heap_expo_growth_iters <= HEAP_EXPONENTIAL_GROWTH_LIMIT) {
        next_heap_size = (size_t)HEAP_BASE_SIZE << ar->heap_expo_growth_iters;
        ar->heap_expo_growth_iters++;
    } else {
        next_heap_size = (size_t)HEAP_EXPONENTIAL_GROWTH_LIMIT + (size_t)HEAP_CONSTANT_GROWTH * (size_t)ar->heap_constant_growth_iters;
        ar->heap_constant_growth_iters++;
    }

    if(next_heap_size > HEAP_MAX_SIZE) {
        perror("heap overflow \n");
    }

    Debug("[%p][=========== heap growing %zu========]\n", (void*)ar, next_heap_size);

    free_chunk_t* new_block = alloc_chunk(next_heap_size + HEAP_BOUNDARY_SIZE);
    if(!new_block) {
        perror("failed to allocate heap \n");
    }
    /* override few fields */
    unset_mmap_flag(new_block);
    set_size(new_block, (ssize_t)next_heap_size, __PREV_IN_USE_FLAG_MASK);
    
    ar->top_chunk = new_block;

    /* setup boundary */
    free_chunk_t* boundary = (free_chunk_t*)((char*)new_block + HEADER_SIZE + next_heap_size);
    set_size(boundary, 0, __CURR_IN_USE_FLAG_MASK | __PREV_IN_USE_FLAG_MASK);
    set_fdbk_to(boundary, NULL);

    /* register alloced heap */
    ar->alloced_heaps[ar->alloced_heap_count++] = (alloced_heap_t){
        .start = (char*)new_block, 
        .end = (char*)new_block + next_heap_size + HEADER_SIZE 
    };
}


static void insert_into_fastbin(arena_t* ar, free_chunk_t* fc) {
    size_t size = (size_t)get_size(fc);
    int idx = (int)((size >> 4) - 1);
    if (idx >= 0 && idx < FASTBINS_COUNT) {
        Debug("Releasing chunk size: %zu to fastbin  ar->fastbins[idx] = %p \n", size,  (void*)ar->fastbins[idx]);
        fc->fd = ar->fastbins[idx];
        ar->fastbins[idx] = fc;

        
        /* weird way of preventing fastbin coalesce. */
        /* neither i set current chunk free, nor do i tell next chunk that it is free */
        /* unset_curr_inuse(fc); */
        /* @experimental: gc_sweep depends on curr_inuse flag, so it is mandatory to unset to avoid sweep */
        unset_curr_inuse(fc);
        /* unset_prev_inuse(next_chunk(fc)); */
        return;
    }
}

static void insert_into_smallbin(arena_t* ar, free_chunk_t* fc) {
    size_t sz = (size_t)get_size(fc);
    int idx = get_smallbin_index(sz);
    assert(idx >= 0 && idx < SMALLBINS_COUNT);

    /* insertion to smallbin comes from only unsortedbin, these fields are taken care by */
    /* insert_into_unsortedbin, no need to do again. */
    /* unset_curr_inuse(fc); */
    /* unset_prev_inuse(next_chunk(fc)); */

    insert_into_fdlist(ar->smallbins[idx], fc);
}

static void insert_into_largebin(arena_t* ar, free_chunk_t* fc) {
    size_t size = (size_t)get_size(fc);
    int idx = get_largebin_index(size);

    
    assert(idx >= 0 && idx < LARGEBINS_COUNT);
    free_chunk_t* head = ar->largebins[idx];
    assert(ar->largebins[idx] != NULL);
    
    free_chunk_t* ceil_chunk;
    free_chunk_t* floor_chunk;
    
    ceil_chunk = head->next_sizeptr; 

    while (ceil_chunk != head && (size_t)get_size(ceil_chunk) < size) {
        ceil_chunk = ceil_chunk->next_sizeptr;
    }
    
    floor_chunk = ceil_chunk->prev_sizeptr;
    
    /* insertion to smallbin comes from only unsortedbin, these fields are taken care by */
    /* by default these fields are NULL, this makes easy to distinguish largebin chunks with rest.*/
    fc->next_sizeptr = ceil_chunk;
    fc->prev_sizeptr = floor_chunk;
    
    
    ceil_chunk->prev_sizeptr = fc;
    floor_chunk->next_sizeptr = fc;

    /* insertion to smallbin comes from only unsortedbin, these fields are taken care by */
    /* insert_into_unsortedbin, no need to do again. */
    /* unset_curr_inuse(fc); */
    /* unset_prev_inuse(next_chunk(fc)); */
    
    insert_into_fdlist(head, fc);
}

static void insert_into_unsortedbin(arena_t* ar, free_chunk_t* fc) {
    assert(ar->unsortedbin != NULL);
    free_chunk_t* head = ar->unsortedbin;

    unset_curr_inuse(fc);
    unset_prev_inuse(next_chunk(fc));

    free_chunk_t* next_fc = next_chunk(fc);
    set_prev_size(next_fc, get_size(fc), get_prev_size_flags(next_fc));
    
    /* set next_sizeptr & prev_sizeptr to NULL */
    /* by default set it to NULL, only when it goes to largebin it becomes a */
    /* valid pointer. This comes handy to distinguish whether a chunk is in largebin */
    /* or not while doing coalescing */
    if(get_size(fc) > 16)
    set_nextprev_sizeptr_to(fc, NULL);
    
    insert_into_fdlist(head, fc);
}

static free_chunk_t* fastbin_search(arena_t* ar, size_t requested_size) {
    int idx = (int)((requested_size >> 4) - 1); // Assuming min chunk size 32, or payload of 16

    if (idx >= 0 && idx < FASTBINS_COUNT && ar->fastbins[idx] != NULL) {
        free_chunk_t* victim = ar->fastbins[idx];

        /* victim & victim->fd must be valid */
        ar->fastbins[idx] = victim->fd; // Pop head
        victim->fd = NULL;

        set_curr_inuse(victim);
        /* don't want to merge fastbin chunks, so don't update prev_size/inuse fields of next chunk */
        /* set_prev_inuse(next_chunk(victim)); */
        return victim;
    }
    return NULL;
}

static free_chunk_t* smallbin_search(arena_t* ar, ssize_t requested_size) {
    int idx = get_smallbin_index((size_t)requested_size);

    if (idx >= 0 && idx < SMALLBINS_COUNT ) {
        free_chunk_t* head = ar->smallbins[idx];
        
        // If head->fd == head, it's empty
        if (head->fd != head) {
            free_chunk_t* victim = head->fd; // FIFO: take the first one
            unlink_chunk_from_fdlist(victim);
            
            set_curr_inuse(victim);
            set_prev_inuse(next_chunk(victim));
            return victim;
        }
    }
    return NULL;
}

static free_chunk_t* largebin_search(arena_t* ar, size_t requested_size) {
    int idx = get_largebin_index(requested_size);
    if(idx < 0 || idx >= LARGEBINS_COUNT) return NULL;
    
    for(int i=idx; i< LARGEBINS_COUNT; i++) {
        assert(ar->largebins[i] != NULL);

        free_chunk_t* head = ar->largebins[i];
        if(head == head->next_sizeptr) continue;

        free_chunk_t* curr = head->next_sizeptr;
        free_chunk_t* next = NULL;

        while( curr != head ) {
            next = curr->next_sizeptr;

            size_t curr_size = (size_t)get_size(curr);
            
            if(curr_size >= requested_size) {
                unlink_chunk_from_fdlist(curr);
                unlink_chunk_from_sortedlist(curr);

                size_t remainder_chunk_size = curr_size - requested_size;

                
                if(remainder_chunk_size >= MIN_PAYLOAD_SIZE + HEADER_SIZE) {
                    size_t remainder_payload_size = remainder_chunk_size - HEADER_SIZE;

                    free_chunk_t* remainder = (free_chunk_t*)((char*)curr + HEADER_SIZE + requested_size);


                    set_size(remainder, (ssize_t)remainder_payload_size, __PREV_IN_USE_FLAG_MASK);
                    set_prev_size(next_chunk(remainder), (ssize_t)remainder_payload_size, 0);
                    /* unset_prev_inuse(next_chunk(remainder)) */
                    insert_into_unsortedbin(ar, remainder);

                    set_size(curr, (ssize_t)requested_size, get_size_flags(curr));
                    set_curr_inuse(curr);

                }else {

                    set_curr_inuse(curr);
                    set_prev_inuse(next_chunk(curr));
                }

                return curr;
            }
            curr = next;
        }
    }

    return NULL;
}

static free_chunk_t* unsortedbin_search(arena_t* ar, size_t requested_size) {
    free_chunk_t* head = ar->unsortedbin;
    free_chunk_t* curr = head->fd;
    free_chunk_t* next = NULL;

    while( curr != head ) {
        next = curr->fd;

        unlink_chunk_from_fdlist(curr);
        size_t curr_size = (size_t)get_size(curr);

        if(curr_size >= requested_size) {
            size_t remainder_chunk_size = curr_size - requested_size;

            
            if(remainder_chunk_size >= MIN_PAYLOAD_SIZE + HEADER_SIZE) {
                size_t remainder_payload_size = remainder_chunk_size - HEADER_SIZE;

                free_chunk_t* remainder = (free_chunk_t*)((char*)curr + HEADER_SIZE + requested_size);


                set_size(remainder, (ssize_t)remainder_payload_size, __PREV_IN_USE_FLAG_MASK);
                set_prev_size(next_chunk(remainder), (ssize_t)remainder_payload_size, 0);
                /* unset_prev_inuse(next_chunk(remainder)) */
                insert_into_unsortedbin(ar, remainder);

                set_size(curr, (ssize_t)requested_size, get_size_flags(curr));
                set_curr_inuse(curr);

            }else {

                set_curr_inuse(curr);
                set_prev_inuse(next_chunk(curr));
            }

            return curr;
        }else {
            if (curr_size < 16 * SMALLBINS_COUNT) {
                insert_into_smallbin(ar, curr);
            }else {
                insert_into_largebin(ar, curr);
            }
        }

        curr = next;
    }

    return NULL;
}

static free_chunk_t* carve_from_top_chunk(arena_t* ar, size_t requested_size) {
    if(!ar->top_chunk) grow_heap(ar);

    free_chunk_t* curr = ar->top_chunk;
    size_t curr_size = (size_t)get_size(curr);

    if(curr_size >= requested_size) {
        size_t remainder_chunk_size = curr_size - requested_size;

        if(remainder_chunk_size >= MIN_PAYLOAD_SIZE + HEADER_SIZE) {
            size_t remainder_payload_size = remainder_chunk_size - HEADER_SIZE;

            free_chunk_t* remainder = (free_chunk_t*)((char*)curr + HEADER_SIZE + requested_size);

            set_size(remainder, (ssize_t)remainder_payload_size, __PREV_IN_USE_FLAG_MASK);
            set_prev_size(next_chunk(remainder), (ssize_t)remainder_payload_size, 0);
            /* unset_prev_inuse(next_chunk(remainder)) */

            set_size(curr, (ssize_t)requested_size, get_size_flags(curr));
            set_curr_inuse(curr);

            ar->top_chunk = remainder;

        }else {

            set_curr_inuse(curr);
            set_prev_inuse(next_chunk(curr));

            ar->top_chunk = NULL;
        }

        return curr;
        
    }else {
        perror("not enough memory in heap");   
    }
    return NULL;
}

void* _allocate(arena_t* ar, size_t requested_size) {

    if(!requested_size) return NULL;

    size_t payload_size = align16(requested_size);
    if (payload_size < MIN_PAYLOAD_SIZE) payload_size = MIN_PAYLOAD_SIZE;

    free_chunk_t* victim = NULL;

    /* if size is too big, use mmap */
    if(!victim && payload_size >= MMAP_THRESHOLD) {
        victim = alloc_chunk(payload_size);
    }
    
    /* search in fastbin */
    if(!victim && payload_size <= 16 * FASTBINS_COUNT) {
        victim = fastbin_search(ar, payload_size);
    }

    /* search in smallbins */
    if(!victim && payload_size < 16 * SMALLBINS_COUNT) {
        victim = smallbin_search(ar, (ssize_t)payload_size);
    }

    if(!victim) {
        victim = unsortedbin_search(ar, payload_size);
    }

    if(!victim) {
        victim = largebin_search(ar, payload_size);
    }

    if(!victim) {
        victim = carve_from_top_chunk(ar, payload_size);
    }

    if(!victim) {
        insert_into_unsortedbin(ar, ar->top_chunk);
        ar->top_chunk = NULL;    
        victim = carve_from_top_chunk(ar, payload_size);
    }

    return (void*)((char*)victim + HEADER_SIZE);
}

/**
 * @brief Allocate memory from the arena
 * @param ar Pointer to the arena
 * @param requested_size Size of memory to allocate in bytes
 * @return Pointer to the allocated memory
 */
void* allocate(arena_t* ar, size_t requested_size) {
    assert(ar != NULL);
    pthread_mutex_lock(&ar->mu);
    void* p = _allocate(ar, requested_size);
    pthread_mutex_unlock(&ar->mu);
    
    return p;
}

static free_chunk_t* forward_coalesce(arena_t* ar, free_chunk_t* fc) {
    free_chunk_t* next_fc = next_chunk(fc);
    
    /* this also ensure no fastbin coalescing */
    if(is_curr_inuse(next_fc) || get_size(next_fc) <= 16 * FASTBINS_COUNT) return fc;



    if(next_fc == ar->top_chunk) {
        size_t updated_size = (size_t)get_size(fc) + HEADER_SIZE + (size_t)get_size(next_fc);
        set_size(fc, (ssize_t)updated_size,  get_size_flags(fc));
        set_prev_size(next_chunk(fc), (ssize_t)updated_size, 0);
        ar->top_chunk = fc;

        return NULL;
    }

    unlink_chunk_from_fdlist(next_fc);
    unlink_chunk_from_sortedlist(next_fc);

    size_t updated_size = (size_t)get_size(fc) + HEADER_SIZE + (size_t)get_size(next_fc);
    set_size(fc, (ssize_t)updated_size,  get_size_flags(fc));
    
    /* need to update prev_size of next to next chunk */
    next_fc = next_chunk(fc);
    set_prev_size(next_fc, (ssize_t)updated_size, get_prev_size_flags(next_fc));

    return fc;
}

static free_chunk_t* backward_coalesce(arena_t* ar __attribute__((unused)), free_chunk_t* fc) {
    if(is_prev_inuse(fc)) return fc;

    free_chunk_t* prev_fc = prev_chunk(fc);
    if(get_size(prev_fc) <= 16) return fc;
    
    unlink_chunk_from_fdlist(prev_fc);
    unlink_chunk_from_sortedlist(prev_fc);
    
    size_t updated_size = (size_t)get_size(prev_fc) + HEADER_SIZE + (size_t)get_size(fc);
    set_size(prev_fc, (ssize_t)updated_size,  get_size_flags(prev_fc));

    /* need to update prev_size of next to next chunk */
    fc = next_chunk(prev_fc);
    set_prev_size(fc, (ssize_t)updated_size, get_prev_size_flags(fc));
    return prev_fc;
}

static free_chunk_t* coalesce(arena_t* ar, free_chunk_t* fc) {
    fc = backward_coalesce(ar, fc);
    fc = forward_coalesce(ar, fc);
    return fc;
}

void _release(arena_t* ar, void* ptr) {
    if(!ptr) return;
    
    free_chunk_t* fc = (free_chunk_t*)((char*)ptr - HEADER_SIZE);
    size_t size = (size_t)get_size(fc);
    
    assert(size != 0);

    if(size >= MMAP_THRESHOLD){
        munmap(fc, size + HEADER_SIZE);
        return;
    }
    
    if(size <= 16 * FASTBINS_COUNT) {
        insert_into_fastbin(ar, fc);
        return;
    }
    
    free_chunk_t* merged_chunk = coalesce(ar, fc);
    if(merged_chunk) insert_into_unsortedbin(ar, merged_chunk);
}

/**
 * @brief Release allocated memory back to the arena
 * @param ar Pointer to the arena
 * @param ptr Pointer to the memory to release
 */
void release(arena_t* ar, void* ptr) {
    assert(ar != NULL);

    pthread_mutex_lock(&ar->mu);
    _release(ar, ptr);
    pthread_mutex_unlock(&ar->mu);
}

/**
 * @brief Create a new arena allocator
 * @return Pointer to the newly created arena
 */
arena_t* arena_create(void) {
    size_t needed = sizeof(arena_t) + (SMALLBINS_COUNT * sizeof(free_chunk_t)) + 
                    (LARGEBINS_COUNT * sizeof(free_chunk_t)) + sizeof(free_chunk_t);
    
    void* block = alloc_memory(align_page(needed));
    assert(block != NULL);

    arena_t* ar = (arena_t*)block;
    char* curr = (char*)block + sizeof(arena_t);

    /* initialize mutext */
    pthread_mutex_init(&ar->mu, NULL);

    /* initialize fastbins */
    for (size_t i = 0; i < FASTBINS_COUNT; i++) {
        ar->fastbins[i] = NULL;
    }

    /* initialize smallbins */
    for(int i=0; i<SMALLBINS_COUNT; i++) {
        free_chunk_t* fc = (free_chunk_t*)curr;
        set_fdbk_to(fc, fc);

        fc->size = 0;
        ar->smallbins[i] = fc;
        curr += sizeof(free_chunk_t);
    }

    /* initialize largebins */
    for(int i=0; i<LARGEBINS_COUNT; i++) {
        free_chunk_t* fc = (free_chunk_t*)curr;
        set_fdbk_to(fc, fc);
        set_nextprev_sizeptr_to(fc, fc);

        fc->size = 0;
        ar->largebins[i] = fc;

        curr += sizeof(free_chunk_t);
    }

    /* allocate unsortedbin */
    free_chunk_t* ufc = (free_chunk_t*)curr;
    set_fdbk_to(ufc, ufc);
    ufc->size = 0;
    ar->unsortedbin = ufc;

    grow_heap(ar);

    return ar;
}
