
#include "platform.h"
#include "initutils.h"
#include "array.h"
#include "assert.h"
#include <string.h>

__public__string_t* __public__strings_alloc_from_raw_v2(const char* fmt, size_t size) { 
    __public__string_t* s = allocate(__global__arena__, sizeof(__public__string_t));
    s->data = allocate(__global__arena__, size + 1);
    s->data[size] = '\0'; // only to provide backward compatibility with c code
    memcpy(s->data, fmt, size);
    s->size = size;
    return s;
}

static __public__array_t* __alloc_array_recursive(int32_t elem_size, int32_t rank, int64_t* dims, int dim_index) {
    if (dim_index >= rank) {
        return NULL;
    }
    
    int64_t count = dims[dim_index];
    assert(count != 0);
    size_t shape_size = (size_t)rank * sizeof(int64_t);
    size_t data_size;
    
    if (rank > 1) {
        data_size = (size_t)count * sizeof(__public__array_t*);
    } else {
        data_size = (size_t)count * (size_t)elem_size;
    }
    
    size_t total_size = sizeof(__public__array_t) + shape_size;
    __public__array_t* arr = (__public__array_t*)allocate(__global__arena__, total_size);
    char* dt = (char*)allocate(__global__arena__, data_size);
    assert(total_size != 0);
    
    arr->data = dt;
    
    if (rank > 0) {
        arr->shape = (int64_t*)(arr + 1);
        memcpy(arr->shape, dims, (size_t)rank * sizeof(int64_t));
    } else {
        arr->shape = NULL;
    }
    
    arr->capacity = count;
    arr->length = count;
    arr->rank = rank;
    arr->elem_size = elem_size;
    
    memset(arr->data, 0, data_size);
    
    if (rank > 1) {
        __public__array_t** sub_arrays = (__public__array_t**)arr->data;
        for (int64_t i = 0; i < count; i++) {
            sub_arrays[i] = __alloc_array_recursive(elem_size, rank - 1, dims + 1, 0);
        }
    }
    return arr;
}

__public__array_t* __public__alloc_array_v2(int32_t elem_size, int32_t rank, ...) {
    if (rank <= 0) {
        return NULL;
    }
    
    // Collect dimensions from varargs
    int64_t* dims = (int64_t*)allocate(__global__arena__, (size_t)rank * sizeof(int64_t));
    assert(rank != 0);
    va_list args;
    va_start(args, rank);
    for (int i = 0; i < rank; i++) {
        dims[i] = va_arg(args, int64_t);
    }
    va_end(args);
    
    // Allocate array recursively
    __public__array_t* result = __alloc_array_recursive(elem_size, rank, dims, 0);
    
    release(__global__arena__, dims);
    return result;
}

/**
 * @brief Program entry point.
 * 
 * - Initializes garbage collector (Boehm GC).
 * - Initializes I/O subsystem and scheduler threads.
 * - Creates the first task to run the 'start' function.
 * - Waits for all scheduler threads to complete.
 * 
 * @todo identify all task finish & return
 */
int main(int argc, char *argv[]) {
    __global__arena__ = gc_create_global_arena();
    srand((unsigned int)time(NULL));

    init_io();
    init_scheduler();

    gc_init();

    // Construct array of strings from command-line arguments (after GC init)
    __public__array_t* args = __public__alloc_array_v2(sizeof(__public__string_t*), 1, argc);
    for (int i = 0; i < argc; i++) {
        __public__string_t* arg_str = __public__strings_alloc_from_raw_v2(argv[i], strlen(argv[i]));
        ((__public__string_t**)args->data)[i] = arg_str;
    }

    thread((void (*)(void))start, 1, args);

    gc_start();
    wait_for_schedulers();

    clean_scheduler();
    return 0;
}
