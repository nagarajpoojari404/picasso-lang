#include "diskio.h"

/* Force references so Clang emits declarations */
void* __ffi_force[] = {
    #if defined(__linux__)
    (void*)__public__asyncio_scan,
    (void*)__public__asyncio_printf,
    (void*)__public__asyncio_fread,
    (void*)__public__asyncio_fwrite,
    #endif
};
