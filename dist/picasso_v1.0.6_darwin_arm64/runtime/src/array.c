#include "platform.h"
#include "array.h"
#include "alloc.h"
#include "sigerr.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

extern __thread arena_t* __arena__;

/**
 * @brief Internal helper to allocate array with dimensions
 * @param elem_size size of each element (for leaf arrays)
 * @param rank number of dimensions
 * @param dims array of dimension sizes
 * @param dim_index current dimension being processed
 */
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
    __public__array_t* arr = (__public__array_t*)allocate(__arena__, total_size);
    char* dt = (char*)allocate(__arena__, data_size);
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

/**
 * @brief allocate block of memory for array with pre-allocated subarrays
 * @param elem_size size of each element (for leaf arrays)
 * @param rank number of dimensions
 * @param ... variable number of dimension sizes (int64_t)
 *
 * Example: __public__alloc_array(sizeof(int), 3, 2, 3, 4) creates a 2x3x4 array
 */
__public__array_t* __public__alloc_array(int32_t elem_size, int32_t rank, ...) {
    if (rank <= 0) {
        return NULL;
    }
    
    // Collect dimensions from varargs
    int64_t* dims = (int64_t*)allocate(__arena__, (size_t)rank * sizeof(int64_t));
    assert(rank != 0);
    va_list args;
    va_start(args, rank);
    for (int i = 0; i < rank; i++) {
        dims[i] = va_arg(args, int64_t);
    }
    va_end(args);
    
    // Allocate array recursively
    __public__array_t* result = __alloc_array_recursive(elem_size, rank, dims, 0);
    
    release(__arena__, dims);
    return result;
}

/**
 * @brief extend the length by 1, allocate 2*capacity if capacity is not enough 
 * @param __public__array_t array to extend
 */
void __public__extend_array(__public__array_t* arr, int32_t unused) {
    (void)unused;
    
    if (arr == NULL) {
        __public__runtime_error("===== array is NULL in extend_array");
    }

    arr->length++;
    if(arr->length <= arr->capacity) {
        return;
    }

    int64_t new_cap = arr->capacity > 0 ? arr->capacity * 2 : 1;
    arr->capacity = new_cap;
    char* data = (char*)allocate(__arena__, (size_t)new_cap * (size_t)arr->elem_size);
    assert(new_cap * arr->elem_size != 0);
    memcpy(data, arr->data, (arr->length - 1) * arr->elem_size);
    
    release(__arena__, arr->data);
    arr->data = data;
}

int64_t __public__len(__public__array_t* arr) {
    return arr->length;
}

/**
 * @brief Get a sub-array pointer from a jagged array
 * @param arr The parent array (must have rank > 1)
 * @param index The index to access
 * @return Pointer to the sub-array at the given index
 */
__public__array_t* __public__get_subarray(__public__array_t* arr, int64_t index) {
    if (arr == NULL || arr->rank <= 1) {
        return NULL;
    }
    
    if (index < 0 || index >= arr->length) {
        return NULL;
    }
    
    // Cast data to array of pointers
    __public__array_t** sub_arrays = (__public__array_t**)arr->data;
    return sub_arrays[index];
}

/**
 * @brief Set a sub-array pointer in a jagged array
 * @param arr The parent array (must have rank > 1)
 * @param index The index to set
 * @param sub_arr The sub-array to store at the given index
 */
void __public__set_subarray(__public__array_t* arr, int64_t index, __public__array_t* sub_arr) {
    if (arr == NULL || arr->rank <= 1) {
        return;
    }
    
    if (index < 0 || index >= arr->length) {
        return;
    }
    
    // Cast data to array of pointers
    __public__array_t** sub_arrays = (__public__array_t**)arr->data;
    sub_arrays[index] = sub_arr;
}

/** @deprecated */
void __public__debug_array_info(__public__array_t* arr) {
    if (arr == NULL) {
        printf(" == DEBUG INFO: __public__array_t pointer is NULL ==\n");
        return;
    }

    printf(" == DEBUG INFO: Final __public__array_t State ==\n");
    printf("     Base Address (a.Ptr): %p\n", (void*)arr);
    printf("     data (Offset 0):      %p\n", (void*)arr->data);
    printf("     shape (Offset 8):     %p\n", (void*)arr->shape);
    printf("     length (Offset 16):   %lld\n", arr->length);
    printf("     rank (Offset 24):     %lld\n", arr->rank);
    
    // Print shape elements if rank > 0
    if (arr->rank > 0 && arr->shape != NULL) {
        printf("     Shape Dims: [");
        for (int i = 0; i < arr->rank; i++) {
            printf("%lld", arr->shape[i]);
            if (i < arr->rank - 1) {
                printf(", ");
            }
        }
        printf("]\n");
    } else {
        printf("     Shape Dims: (None or NULL)\n");
    }
}
