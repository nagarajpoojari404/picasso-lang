#include "str.h"

/* Force references so Clang emits declarations */
void* __ffi_force[] = {
    (void*)__public__strings_format,
    (void*)__public__strings_length,
    (void*)__public__strings_alloc_from_arr,
    (void*)__public__strings_alloc_from_raw,
    (void*)__public__strings_get_bytes,
    (void*)__public__strings_get,
    (void*)__public__strings_compare,
    (void*)__public__strings_append,
    (void*)__public__strings_join,
    (void*)__public__strings_substring,
};
