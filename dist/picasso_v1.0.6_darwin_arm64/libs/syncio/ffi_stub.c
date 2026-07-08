#include "diskio.h"

/* Force references so Clang emits declarations */
void* __ffi_force[] = {
    (void*)__public__syncio_scan,
    (void*)__public__syncio_printf,
    (void*)__public__syncio_fread,
    (void*)__public__syncio_fwrite,
    (void*)__public__syncio_get_stdout,
    (void*)__public__syncio_fopen,
    (void*)__public__syncio_fclose,
    (void*)__public__syncio_fflush,
    (void*)__public__syncio_fseek,
};
