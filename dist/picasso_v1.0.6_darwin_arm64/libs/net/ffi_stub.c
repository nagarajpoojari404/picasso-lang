#include "netio.h"

/* Force references so Clang emits declarations */
void* __ffi_force[] = {
    (void*)__public__net_accept,
    (void*)__public__net_read,
    (void*)__public__net_write,
    (void*)__public__net_listen,
    (void*)__public__net_dial,
};
