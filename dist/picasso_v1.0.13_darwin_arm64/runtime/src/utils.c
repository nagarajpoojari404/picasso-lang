#include "utils.h"
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

int _printf(const char *fmt, ...){
    char buf[4096];
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len < 0)
        return len;

    if (len > sizeof(buf))
        len = sizeof(buf);

    return write(1, buf, len);
}
