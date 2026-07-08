#include "platform.h"
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#include "platform/context.h"
#include "queue.h"
#include "alloc.h"
#include "gc.h"

/* expected to be initialized in gc_init */
gc_state_t gc_state;

/* arenas to keep track of. Should be initialized by arena_create */
arena_t* arenas[MAX_ARENAS];
int arenas_count;

arena_t* global_arena;

/* all roots to be scanned */
safe_gcqueue_t* roots;

/* forward declarations */
static void gc_mark_mem_region(char *start, char *end);

/**
 * @brief Create the global arena for garbage collection.
 * @return Pointer to the created global arena.
 */
arena_t* gc_create_global_arena(void) {
    arena_t* ar = arena_create();
    global_arena = ar;
    return ar;
}

/**
 * @brief Register a task as a GC root.
 * @param t Task to register as root.
 */
void gc_register_root(task_t* t) {
    assert(roots != NULL);
    assert(t != NULL);
    safe_gcq_push(roots, t);
}

/**
 * @brief Unregister a task as a GC root.
 * @param t Task to unregister as root.
 */
void gc_unregister_root(task_t* t) {
    assert(roots != NULL);
    assert(t != NULL);
    safe_gcq_remove(roots, t);
}

/**
 * @brief Create a new arena for garbage collection.
 * @return Pointer to the created arena.
 */
arena_t* gc_create_arena(void) {
    if(arenas_count + 1 > MAX_ARENAS) {
        perror("failed to create new arena: arena count reached max\n");
        return NULL;
    }

    arena_t* ar = arena_create();

    /* register arena */
    arenas[arenas_count++] = ar;
    return ar;
}

static void mark_chunk_recursive(inuse_chunk_t* ch) {
    if(ch->prev_size & __GC_MARK_FLAG_MASK) return;

    char* payload_start = (char*)ch + HEADER_SIZE;
    char* payload_end = payload_start + (ch->size & (size_t)__CHUNK_SIZE_MASK);
    gc_mark_mem_region(payload_start, payload_end);
}

static inuse_chunk_t* find_chunk_in_heap(arena_t* ar __attribute__((unused)), alloced_heap_t* heap, char* ptr){
    char* scan = heap->start;

    while (scan < heap->end) {
        inuse_chunk_t *chunk = (inuse_chunk_t*)scan;
        size_t payload_size = chunk->size & (size_t)__CHUNK_SIZE_MASK;

        if( !(chunk->size & __CURR_IN_USE_FLAG_MASK) ) {
            scan += HEADER_SIZE + payload_size;
            continue;
        }

        char* payload_start = scan + HEADER_SIZE;
        char* payload_end   = payload_start + payload_size;

        if (ptr >= payload_start && ptr < payload_end)
            return chunk;

        scan += HEADER_SIZE + payload_size;
    }
    return NULL;
}


static inline int is_pointer_aligned(uintptr_t v) {
    return (v & GC_ALIGN_MASK) == 0;
}

/* Try to mark a single candidate pointer value.
 * Returns 1 if it marked something, 0 otherwise.
 */
static int try_mark_pointer(uintptr_t val) {
    if (!is_pointer_aligned(val)) return 0;

    char *p = (char*)val;

    for (int j = 0; j < arenas_count; ++j) {
        arena_t *ar = arenas[j];
        for (int i = 0; i < ar->alloced_heap_count; ++i) {
            char *hs = ar->alloced_heaps[i].start;
            char *he = ar->alloced_heaps[i].end;

            if ((char*)p < hs || (char*)p >= he) continue;

            inuse_chunk_t *ch = find_chunk_in_heap(ar, &ar->alloced_heaps[i], p);
            if (!ch) return 0;

            ch->prev_size |= __GC_MARK_FLAG_MASK;
            mark_chunk_recursive(ch);
            return 1;
        }
    }
    return 0;
}

/* Scan a memory region (stack or other) - read words safely and test each as pointer */
static void gc_mark_mem_region(char *start, char *end) {
    // printf("gc_mark_mem_region - [%p ... %p]\n", (char*)start, (void*)end);
    
    // read as uintptr_t; use memcpy for safe unaligned reads on strict platforms
    for (char *p = start; p + (ptrdiff_t)sizeof(uintptr_t) <= end; p += sizeof(uintptr_t)) {
        uintptr_t val;
        memcpy(&val, p, sizeof(val));   // safe on all architectures

        if (val == 0) continue;
        try_mark_pointer(val);
    }
}

/* Scan saved registers from ucontext (AArch64) */
static void gc_mark_registers(platform_ctx_t *ctx) {
    // X0-X30
    for (int i = 0; i < 31; ++i) {
        uintptr_t val = platform_ctx_get_reg(ctx, i);
        if (val == 0) continue;
        try_mark_pointer(val);
    }

    // SP, PC
    try_mark_pointer(platform_ctx_get_stack_pointer(ctx));
    try_mark_pointer(platform_ctx_get_pc(ctx));
}

static void gc_mark_task(platform_ctx_t* ctx) {
    // stack
    char *stack_bottom = (char*)platform_ctx_get_stack_base(ctx);             // lowest address
    char *stack_top    = stack_bottom + platform_ctx_get_stack_size(ctx);   // highest address

    char *sp = (char*)platform_ctx_get_stack_pointer(ctx);                       // current SP

    if (sp < stack_bottom || sp > stack_top) {
        perror(" wrong stack \n");
        return;
    }

    gc_mark_mem_region(stack_bottom, stack_top);

    // registers
    gc_mark_registers(ctx);
}

static void gc_mark(void) {
    wait_q_metadata_t* head = roots->head;
    wait_q_metadata_t* tm = roots->head;

    if(roots == NULL || head == NULL) return;

    do {
        task_t* t = tm->t;
        if(t->state != TASK_FINISHED) {
            // printf("[SCANNING] %p \n", t);
            gc_mark_task(&t->ctx);
        }

        tm = tm->fd;
    }while(tm != NULL && tm != head);
}


static void gc_sweep(void) {

    /*  
        go through all arenas, all heaps 
        if gc not marked, call release()
    */

    for(int i=0; i<arenas_count; i++) {
        arena_t* ar = arenas[i];

        for(int j=0; j<ar->alloced_heap_count; j++) {
            alloced_heap_t* heap = &(ar->alloced_heaps[j]);
            
            char* scan = heap->start;

            while (scan < heap->end) {

                inuse_chunk_t *chunk = (inuse_chunk_t*)scan;
                size_t payload_size = chunk->size & (size_t)__CHUNK_SIZE_MASK;

                if( chunk->prev_size & __GC_MARK_FLAG_MASK) {
                    chunk->prev_size &= ~(size_t)__GC_MARK_FLAG_MASK;
                } else if(chunk->size & __CURR_IN_USE_FLAG_MASK) {
                    release(ar, (char*)chunk + HEADER_SIZE);
                }

                scan += HEADER_SIZE + payload_size;
            }
        }
    }
}

static void gc_collect(void) {
    gc_stop_the_world();
    
    gc_mark();
    gc_sweep();
    
    gc_resume_world();
}

static void* gc_run(void* arg __attribute__((unused))) {
    while (1) {
        gc_collect();
        usleep(GC_TIMEPERIOD);
    }

    return NULL;
}

/**
 * @brief Initialize the garbage collector.
 */
void gc_init(void) {
    roots = (safe_gcqueue_t*)malloc(sizeof(safe_gcqueue_t));
    safe_gcq_init(roots, INT32_MAX);
}

/**
 * @brief Start the garbage collector.
 */
void gc_start(void) {
    assert(roots != NULL);

    atomic_store(&gc_state.world_stopped, 0);
    atomic_store(&gc_state.stopped_count, 0);
    atomic_store(&gc_state.total_threads, 0);
    pthread_mutex_init(&gc_state.lock, 0);
    pthread_cond_init(&gc_state.cv_mutators_stopped, 0);
    pthread_cond_init(&gc_state.cv_world_resumed, 0);
    pthread_mutex_init(&gc_state.add_lock, 0);
    
    pthread_t t;
    pthread_create(&t, NULL, gc_run, NULL);
}

/**
 * @brief Stop all mutator threads for garbage collection.
 */
void gc_stop_the_world(void) {
    pthread_mutex_lock(&gc_state.lock);
    atomic_store(&gc_state.world_stopped, 1);
    
    while (atomic_load(&gc_state.stopped_count) < atomic_load(&gc_state.total_threads)){
        pthread_cond_wait(&gc_state.cv_mutators_stopped, &gc_state.lock);
    }
    
    pthread_mutex_unlock(&gc_state.lock);
    pthread_mutex_lock(&gc_state.add_lock);
}

/**
 * @brief Resume all mutator threads after garbage collection.
 */
void gc_resume_world(void) {
    pthread_mutex_lock(&gc_state.lock);

    atomic_store(&gc_state.world_stopped, 0);

    atomic_store(&gc_state.stopped_count, 0);

    pthread_cond_broadcast(&gc_state.cv_world_resumed);

    pthread_mutex_unlock(&gc_state.lock);
    pthread_mutex_unlock(&gc_state.add_lock);
}
