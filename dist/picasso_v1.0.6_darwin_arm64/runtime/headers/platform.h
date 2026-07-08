#ifndef PLATFORM_H
#define PLATFORM_H

#if defined(__linux__)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <signal.h>
#include <ucontext.h>
#include <liburing.h>
#include <sys/epoll.h>
#include <ffi.h>


#elif defined(__APPLE__)
#define _DARWIN_C_SOURCE
#include <signal.h>
#include <ffi/ffi.h>
#else
#error Unsupported platform
#endif

#endif
