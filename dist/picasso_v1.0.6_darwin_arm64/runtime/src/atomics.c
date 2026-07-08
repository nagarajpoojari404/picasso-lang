#include "platform.h"
#include "atomics.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>


/** @boolean: */

/**
 * @brief Atomically store a boolean value.
 * @param ptr Pointer to an atomic _Bool variable.
 * @param val Value to store.
 */
void __public__atomics_store_boolean(_Atomic _Bool *ptr, _Bool val) { 
    atomic_store(ptr, val); 
}
/**
 * @brief Atomically load a boolean value.
 * @param ptr Pointer to an atomic _Bool variable.
 * @return The loaded boolean value.
 */
_Bool __public__atomics_load_boolean(_Atomic _Bool *ptr) { 
    return atomic_load(ptr); 
}

/**
 * @brief Atomically compare & swap boolean value.
 * @param ptr Pointer to an atomic _Bool variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_boolean(_Atomic _Bool *ptr, _Bool expected, _Bool desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/**
 * @brief Atomically exchange boolean value.
 * @param ptr Pointer to an atomic _Bool variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
_Bool __public__atomics_exchange_boolean(_Atomic _Bool *ptr, _Bool val) {
    return atomic_exchange(ptr, val);
}

/** @uint8: */

/**
 * @brief Atomically store an unsigned 8-bit value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_uint8(_Atomic uint8_t *ptr, uint8_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load an unsigned 8-bit value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @return The loaded uint8_t value.
 */
uint8_t __public__atomics_load_uint8(_Atomic uint8_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to a uint8_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
uint8_t __public__atomics_add_uint8(_Atomic uint8_t *ptr, uint8_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from a uint8_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
uint8_t __public__atomics_sub_uint8(_Atomic uint8_t *ptr, uint8_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND a uint8_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
uint8_t __public__atomics_and_uint8(_Atomic uint8_t *ptr, uint8_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR a uint8_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
uint8_t __public__atomics_or_uint8(_Atomic uint8_t *ptr, uint8_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR a uint8_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
uint8_t __public__atomics_xor_uint8(_Atomic uint8_t *ptr, uint8_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange uint8_t value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
uint8_t __public__atomics_exchange_uint8(_Atomic uint8_t *ptr, uint8_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap uint8_t value.
 * @param ptr Pointer to an atomic uint8_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_uint8(_Atomic uint8_t *ptr,uint8_t expected, uint8_t desired ) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @uint16: */

/**
 * @brief Atomically store an unsigned 16-bit value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_uint16(_Atomic uint16_t *ptr, uint16_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load an unsigned 16-bit value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @return The loaded uint16_t value.
 */
uint16_t __public__atomics_load_uint16(_Atomic uint16_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to a uint16_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
uint16_t __public__atomics_add_uint16(_Atomic uint16_t *ptr, uint16_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from a uint16_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
uint16_t __public__atomics_sub_uint16(_Atomic uint16_t *ptr, uint16_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND a uint16_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
uint16_t __public__atomics_and_uint16(_Atomic uint16_t *ptr, uint16_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR a uint16_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
uint16_t __public__atomics_or_uint16(_Atomic uint16_t *ptr, uint16_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR a uint16_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
uint16_t __public__atomics_xor_uint16(_Atomic uint16_t *ptr, uint16_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange uint16_t value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
uint16_t __public__atomics_exchange_uint16(_Atomic uint16_t *ptr, uint16_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap uint16_t value.
 * @param ptr Pointer to an atomic uint16_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_uint16(_Atomic uint16_t *ptr, uint16_t expected, uint16_t desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @uint32: */

/**
 * @brief Atomically store an unsigned 32-bit value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_uint32(_Atomic uint32_t *ptr, uint32_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load an unsigned 32-bit value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @return The loaded uint32_t value.
 */
uint32_t __public__atomics_load_uint32(_Atomic uint32_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to a uint32_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
uint32_t __public__atomics_add_uint32(_Atomic uint32_t *ptr, uint32_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from a uint32_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
uint32_t __public__atomics_sub_uint32(_Atomic uint32_t *ptr, uint32_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND a uint32_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
uint32_t __public__atomics_and_uint32(_Atomic uint32_t *ptr, uint32_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR a uint32_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
uint32_t __public__atomics_or_uint32(_Atomic uint32_t *ptr, uint32_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR a uint32_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
uint32_t __public__atomics_xor_uint32(_Atomic uint32_t *ptr, uint32_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange uint32_t value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
uint32_t __public__atomics_exchange_uint32(_Atomic uint32_t *ptr, uint32_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap uint32_t value.
 * @param ptr Pointer to an atomic uint32_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_uint32( _Atomic uint32_t *ptr, uint32_t expected, uint32_t desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @uint64: */

/**
 * @brief Atomically store an unsigned 64-bit value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_uint64(_Atomic uint64_t *ptr, uint64_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load an unsigned 64-bit value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @return The loaded uint64_t value.
 */
uint64_t __public__atomics_load_uint64(_Atomic uint64_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to a uint64_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
uint64_t __public__atomics_add_uint64(_Atomic uint64_t *ptr, uint64_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from a uint64_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
uint64_t __public__atomics_sub_uint64(_Atomic uint64_t *ptr, uint64_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND a uint64_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
uint64_t __public__atomics_and_uint64(_Atomic uint64_t *ptr, uint64_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR a uint64_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
uint64_t __public__atomics_or_uint64(_Atomic uint64_t *ptr, uint64_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR a uint64_t variable and return the previous value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
uint64_t __public__atomics_xor_uint64(_Atomic uint64_t *ptr, uint64_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange uint64_t value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
uint64_t __public__atomics_exchange_uint64(_Atomic uint64_t *ptr, uint64_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap uint64_t value.
 * @param ptr Pointer to an atomic uint64_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_uint64( _Atomic uint64_t *ptr, uint64_t expected, uint64_t desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @int8: */

/**
 * @brief Atomically store a signed 8-bit value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_int8(_Atomic int8_t *ptr, int8_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a signed 8-bit value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @return The loaded int8_t value.
 */
int8_t __public__atomics_load_int8(_Atomic int8_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to an int8_t variable and return the previous value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
int8_t __public__atomics_add_int8(_Atomic int8_t *ptr, int8_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from an int8_t variable and return the previous value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
int8_t __public__atomics_sub_int8(_Atomic int8_t *ptr, int8_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND an int8_t variable and return the previous value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
int8_t __public__atomics_and_int8(_Atomic int8_t *ptr, int8_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR an int8_t variable and return the previous value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
int8_t __public__atomics_or_int8(_Atomic int8_t *ptr, int8_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR an int8_t variable and return the previous value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
int8_t __public__atomics_xor_int8(_Atomic int8_t *ptr, int8_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange int8_t value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
int8_t __public__atomics_exchange_int8(_Atomic int8_t *ptr, int8_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap int8_t value.
 * @param ptr Pointer to an atomic int8_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_int8( _Atomic int8_t *ptr, int8_t expected, int8_t desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @int16: */

/**
 * @brief Atomically store a signed 16-bit value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_int16(_Atomic int16_t *ptr, int16_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a signed 16-bit value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @return The loaded int16_t value.
 */
int16_t __public__atomics_load_int16(_Atomic int16_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to an int16_t variable and return the previous value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
int16_t __public__atomics_add_int16(_Atomic int16_t *ptr, int16_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from an int16_t variable and return the previous value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
int16_t __public__atomics_sub_int16(_Atomic int16_t *ptr, int16_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND an int16_t variable and return the previous value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
int16_t __public__atomics_and_int16(_Atomic int16_t *ptr, int16_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR an int16_t variable and return the previous value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
int16_t __public__atomics_or_int16(_Atomic int16_t *ptr, int16_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR an int16_t variable and return the previous value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
int16_t __public__atomics_xor_int16(_Atomic int16_t *ptr, int16_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange int16_t value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
int16_t __public__atomics_exchange_int16(_Atomic int16_t *ptr, int16_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap int16_t value.
 * @param ptr Pointer to an atomic int16_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_int16( _Atomic int16_t *ptr, int16_t expected, int16_t desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @int32: */

/**
 * @brief Atomically store a signed 32-bit value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_int32(_Atomic int32_t *ptr, int32_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a signed 32-bit value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @return The loaded int32_t value.
 */
int32_t __public__atomics_load_int32(_Atomic int32_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to an int32_t variable and return the previous value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
int32_t __public__atomics_add_int32(_Atomic int32_t *ptr, int32_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from an int32_t variable and return the previous value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
int32_t __public__atomics_sub_int32(_Atomic int32_t *ptr, int32_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND an int32_t variable and return the previous value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
int32_t __public__atomics_and_int32(_Atomic int32_t *ptr, int32_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR an int32_t variable and return the previous value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
int32_t __public__atomics_or_int32(_Atomic int32_t *ptr, int32_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR an int32_t variable and return the previous value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
int32_t __public__atomics_xor_int32(_Atomic int32_t *ptr, int32_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange int32_t value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
int32_t __public__atomics_exchange_int32(_Atomic int32_t *ptr, int32_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap int32_t value.
 * @param ptr Pointer to an atomic int32_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_int32( _Atomic int32_t *ptr, int32_t expected, int32_t desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @int64: */

/**
 * @brief Atomically store a signed 64-bit value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_int64(_Atomic int64_t *ptr, int64_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a signed 64-bit value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @return The loaded int64_t value.
 */
int64_t __public__atomics_load_int64(_Atomic int64_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
int64_t __public__atomics_add_int64(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
int64_t __public__atomics_sub_int64(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
int64_t __public__atomics_and_int64(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
int64_t __public__atomics_or_int64(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
int64_t __public__atomics_xor_int64(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange int64_t value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
int64_t __public__atomics_exchange_int64(_Atomic int64_t *ptr, int64_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap int64_t value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_int64( _Atomic int64_t *ptr, int64_t expected, int64_t desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @int: */

/**
 * @brief Atomically store a signed 64-bit value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to store.
 */
void __public__atomics_store_int(_Atomic int64_t *ptr, int64_t val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a signed 64-bit value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @return The loaded int64_t value.
 */
int64_t __public__atomics_load_int(_Atomic int64_t *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically add to an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to add.
 * @return The value of *ptr before the addition.
 */
int64_t __public__atomics_add_int(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_add(ptr, val);
}

/**
 * @brief Atomically subtract from an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to subtract.
 * @return The value of *ptr before the subtraction.
 */
int64_t __public__atomics_sub_int(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_sub(ptr, val);
}

/**
 * @brief Atomically AND an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to AND with.
 * @return The value of *ptr before the operation.
 */
int64_t __public__atomics_and_int(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_and(ptr, val);
}

/**
 * @brief Atomically OR an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to OR with.
 * @return The value of *ptr before the operation.
 */
int64_t __public__atomics_or_int(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_or(ptr, val);
}

/**
 * @brief Atomically XOR an int64_t variable and return the previous value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to XOR with.
 * @return The value of *ptr before the operation.
 */
int64_t __public__atomics_xor_int(_Atomic int64_t *ptr, int64_t val) {
    return atomic_fetch_xor(ptr, val);
}

/**
 * @brief Atomically exchange int64_t value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
int64_t __public__atomics_exchange_int(_Atomic int64_t *ptr, int64_t val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap int64_t value.
 * @param ptr Pointer to an atomic int64_t variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_int( _Atomic int64_t *ptr, int64_t expected, int64_t desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @float16: */

/**
 * @brief Atomically store a 16-bit floating-point value.
 * @param ptr Pointer to an atomic _Float16 variable.
 * @param val Value to store.
 */
void __public__atomics_store_float16(_Atomic _Float16 *ptr, _Float16 val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a 16-bit floating-point value.
 * @param ptr Pointer to an atomic _Float16 variable.
 * @return The loaded _Float16 value.
 */
_Float16 __public__atomics_load_float16(_Atomic _Float16 *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically exchange a 16-bit floating-point value.
 * @param ptr Pointer to an atomic _Float16 variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
_Float16 __public__atomics_exchange_float16(_Atomic _Float16 *ptr, _Float16 val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap a 16-bit floating-point value.
 * @param ptr Pointer to an atomic _Float16 variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_float16( _Atomic _Float16 *ptr, _Float16 expected, _Float16 desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @float32: */

/**
 * @brief Atomically store a 32-bit floating-point value.
 * @param ptr Pointer to an atomic float variable.
 * @param val Value to store.
 */
void __public__atomics_store_float32(_Atomic float *ptr, float val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a 32-bit floating-point value.
 * @param ptr Pointer to an atomic float variable.
 * @return The loaded float value.
 */
float __public__atomics_load_float32(_Atomic float *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically exchange a 32-bit floating-point value.
 * @param ptr Pointer to an atomic float variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
float __public__atomics_exchange_float32(_Atomic float *ptr, float val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap a 32-bit floating-point value.
 * @param ptr Pointer to an atomic float variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_float32(_Atomic float *ptr,float expected,float desired) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}


/** @float64: */

/**
 * @brief Atomically store a 64-bit floating-point value.
 * @param ptr Pointer to an atomic double variable.
 * @param val Value to store.
 */
void __public__atomics_store_float64(_Atomic double *ptr, double val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a 64-bit floating-point value.
 * @param ptr Pointer to an atomic double variable.
 * @return The loaded double value.
 */
double __public__atomics_load_float64(_Atomic double *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically exchange a 64-bit floating-point value.
 * @param ptr Pointer to an atomic double variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
double __public__atomics_exchange_float64(_Atomic double *ptr, double val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap a 64-bit floating-point value.
 * @param ptr Pointer to an atomic double variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_float64( _Atomic double *ptr, double expected, double desired ) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}

/** @double: */

/**
 * @brief Atomically store a 64-bit floating-point value.
 * @param ptr Pointer to an atomic double variable.
 * @param val Value to store.
 */
void __public__atomics_store_double(_Atomic double *ptr, double val) {
    atomic_store(ptr, val);
}

/**
 * @brief Atomically load a 64-bit floating-point value.
 * @param ptr Pointer to an atomic double variable.
 * @return The loaded double value.
 */
double __public__atomics_load_double(_Atomic double *ptr) {
    return atomic_load(ptr);
}

/**
 * @brief Atomically exchange a 64-bit floating-point value.
 * @param ptr Pointer to an atomic double variable.
 * @param val Value to store.
 * @return The previous value stored in *ptr.
 */
double __public__atomics_exchange_double(_Atomic double *ptr, double val) {
    return atomic_exchange(ptr, val);
}

/**
 * @brief Atomically compare & swap a 64-bit floating-point value.
 * @param ptr Pointer to an atomic double variable.
 * @param expected Expected value.
 * @param desired Desired value to store if *ptr == expected.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_double( _Atomic double *ptr, double expected, double desired ) {
    return atomic_compare_exchange_strong(ptr, &expected, desired);
}


/** @pointer: */

/**
 * @brief Atomically store a pointer value.
 * @param ptr Pointer to an atomic pointer variable.
 * @param val Pointer value to store.
 */
void __public__atomics_store_ptr(_Atomic uintptr_t *ptr, void *val) {
    atomic_store(ptr, (uintptr_t)val);
}
/**
 * @brief Atomically load a pointer value.
 * @param ptr Pointer to an atomic pointer variable.
 * @return The loaded pointer value.
 */
void *__public__atomics_load_ptr(_Atomic uintptr_t *ptr) {
    return (void *)atomic_load(ptr);
}

/**
 * @brief Atomically compare & swap pointer value.
 * @param ptr Pointer to an atomic pointer variable.
 * @param expected Expected pointer value.
 * @param desired Desired pointer value.
 * @return True if the swap was successful, false otherwise.
 */
_Bool __public__atomics_cas_ptr( _Atomic uintptr_t *ptr, void *expected, void *desired) {
    uintptr_t exp = (uintptr_t)expected;
    return atomic_compare_exchange_strong( ptr, &exp, (uintptr_t)desired);
}

/**
 * @brief Atomically exchange pointer value.
 * @param ptr Pointer to an atomic pointer variable.
 * @param val Pointer value to store.
 * @return The previous pointer stored in *ptr.
 */
void *__public__atomics_exchange_ptr( _Atomic uintptr_t *ptr, void *val) {
    return (void *)atomic_exchange(ptr, (uintptr_t)val);
}
