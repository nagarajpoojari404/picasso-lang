#ifndef ARRAY_H
#define ARRAY_H

#include "platform.h"
#include <stdint.h> 

typedef struct {
    char* data;
    int64_t* shape; 
    int64_t length; 
    int64_t rank;  
    int64_t capacity;  
    int32_t elem_size;
} __public__array_t;

/**
 * @brief allocate block of memory for array with pre-allocated subarrays
 * @param elem_size size of each element (for leaf arrays)
 * @param rank number of dimensions
 * @param ... variable number of dimension sizes (int64_t)
 *
 * Example: __public__alloc_array(sizeof(int), 3, 2, 3, 4) creates a 2x3x4 jagged array
 */
__public__array_t* __public__alloc_array(int32_t elem_size, int32_t rank, ...);

/**
 * @brief extend the length by 1, allocate 2*capacity if capacity is not enough 
 * @param __public__array_t array to extend
 */
void __public__extend_array(__public__array_t* arr, int32_t unused);

/**
 * @brief Get a sub-array pointer from a jagged array
 * @param arr The parent array (must have rank > 1)
 * @param index The index to access
 * @return Pointer to the sub-array at the given index
 */
__public__array_t* __public__get_subarray(__public__array_t* arr, int64_t index);

/**
 * @brief Set a sub-array pointer in a jagged array
 * @param arr The parent array (must have rank > 1)
 * @param index The index to set
 * @param sub_arr The sub-array to store at the given index
 */
void __public__set_subarray(__public__array_t* arr, int64_t index, __public__array_t* sub_arr);

/**
 * @brief utility func to print array information
 * @param arr array struct instance
 */
void __public__debug_array_info(__public__array_t* arr);
#endif
