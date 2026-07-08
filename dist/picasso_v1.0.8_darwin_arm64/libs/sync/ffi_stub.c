#include "sync.h"

/* Force references so Clang emits declarations */
void* __ffi_force[] = {
    (void*)__public__sync_rwmutex_create,
    (void*)__public__sync_rwmutex_rlock,
    (void*)__public__sync_rwmutex_rwlock,
    (void*)__public__sync_rwmutex_runlock,
    (void*)__public__sync_rwmutex_rwunlock,
    (void*)__public__sync_mutex_create,
    (void*)__public__sync_mutex_lock,
    (void*)__public__sync_waitgroup_create,
    (void*)__public__sync_waitgroup_add,
    (void*)__public__sync_waitgroup_done,
    (void*)__public__sync_waitgroup_wait,
};
