#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "str.h"
#include "alloc.h"

extern __thread arena_t* __arena__;

/**
 * @brief Allocate memory in heap for given string
 * @param fmt Format string
 * @param size Number of bytes
 * @return Pointer to the allocated string
 */
__public__string_t* __public__strings_alloc_from_raw(const char* fmt, size_t size) { 
    __public__string_t* s = allocate(__arena__, sizeof(__public__string_t));
    s->data = allocate(__arena__, size + 1);
    s->data[size] = '\0'; // only to provide backward compatibility with c code
    memcpy(s->data, fmt, size);
    s->size = size;
    return s;
}

/**
 * @brief Get pointer to byte stream
 * @param fmt Format string
 * @return array of byte stream
 */
__public__array_t* __public__strings_get_bytes(__public__string_t* fmt) {
    __public__array_t* arr = (__public__array_t*)allocate(__arena__, fmt->size);
    arr->data = fmt->data;
        
    size_t count = (size_t)fmt->size;
    arr->shape = (int64_t*)(arr->data + (count * sizeof(char)));
    
    arr->length = (int64_t)count;
    arr->rank = 1;
    return arr;
}

/**
 * @brief Allocate memory in heap for given string
 * @param fmt Format string
 * @param size Number of bytes
 * @return Pointer to the allocated string
 */
__public__string_t* __public__strings_alloc_from_arr(__public__array_t* fmt) {
    __public__string_t* s = allocate(__arena__, sizeof(__public__string_t));
    s->data = allocate(__arena__, (size_t)fmt->length + 1);
    s->data[fmt->length] = '\0'; // only to provide backward compatibility with c code
    memcpy(s->data, fmt->data, (size_t)fmt->length);
    s->size = (size_t)fmt->length;

    return s;
}

/**
 * @brief Append charcater to given heap buffer
 * @param buf buffer
 * @param cap size to scale buffer
 * @param len current size of buffer
 * @param src charcter/string to be appended
 * @param n size of src
 */
void buf_append(char **buf, size_t *cap, size_t *len, const char *src, size_t n) {
    if (*len + n > *cap) {
        size_t newcap = (*cap == 0) ? 64 : *cap * 2;
        while (newcap < *len + n)
            newcap *= 2;

        char *nb = allocate(__arena__, newcap);
        if (*buf)
            memcpy(nb, *buf, *len);
        *buf = nb;
        *cap = newcap;
    }
    memcpy(*buf + *len, src, n);
    *len += n;
}

/**
 * @brief Format given unsigned int to string
 * @param v u64
 * @param tmp buffer to hold output formated string
 * @return length of formated string
 */
size_t u64_to_dec(uint64_t v, char tmp[32]) {
    size_t i = 0;
    do {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    } while (v);

    for (size_t j = 0; j < i / 2; j++) {
        char c = tmp[j];
        tmp[j] = tmp[i - 1 - j];
        tmp[i - 1 - j] = c;
    }
    return i;
}

/**
 * @brief Format given signed int to string
 * @param v int64
 * @param tmp buffer to hold output formated string
 * @return length of formated string
 */
size_t i64_to_dec(int64_t v, char tmp[32]) {
    size_t i = 0;
    uint64_t x;

    if (v < 0) {
        tmp[i++] = '-';
        x = (uint64_t)(-v);
    } else {
        x = (uint64_t)v;
    }

    size_t n = u64_to_dec(x, tmp + i);
    return i + n;
}

/**
 * @brief Format given pointer to hex string
 * @param p pointer
 * @param tmp buffer to hold output formated string
 * @return length of formated string
 */
size_t ptr_to_hex(const void *p, char tmp[32]) {
    uintptr_t v = (uintptr_t)p;
    static const char hex[] = "0123456789abcdef";

    tmp[0] = '0';
    tmp[1] = 'x';

    size_t i = 2;
    int started = 0;

    for (int shift = (int)(sizeof(uintptr_t) * 8 - 4); shift >= 0; shift -= 4) {
        char d = hex[(v >> shift) & 0xF];
        if (d != '0' || started || shift == 0) {
            started = 1;
            tmp[i++] = d;
        }
    }
    return i;
}

/**
 * @brief Format given float to string
 * @param v double/float
 * @param tmp buffer to hold output formated string
 * @return length of formated string
 */
size_t f64_to_dec(double v, char tmp[64]) {
    size_t i = 0;

    if (v < 0) {
        tmp[i++] = '-';
        v = -v;
    }

    /* integer part */
    uint64_t ip = (uint64_t)v;
    double frac = v - (double)ip;

    char ibuf[32];
    size_t ilen = u64_to_dec(ip, ibuf);
    memcpy(tmp + i, ibuf, ilen);
    i += ilen;

    tmp[i++] = '.';

    /* fractional part: fixed 6 digits */
    for (int k = 0; k < 6; k++) {
        frac *= 10.0;
        int d = (int)frac;
        tmp[i++] = (char)('0' + d);
        frac -= d;
    }

    return i;
}

/**
 * @brief Format a string with variable arguments
 * @param fmt Format string
 * @param ... Variable arguments to format
 * @return Pointer to the formatted string
 */
__public__string_t* __public__strings_format(__public__string_t* fmt, ...) {
    if (!fmt || !fmt->data)
        return NULL;

    va_list ap;
    va_start(ap, fmt);

    char *out = NULL;
    size_t cap = 0, len = 0;

    for (size_t i = 0; i < (size_t)fmt->size; i++) {
        char c = fmt->data[i];

        if (c != PERCENT) {
            buf_append(&out, &cap, &len, &c, 1);
            continue;
        }

        if (++i >= (size_t)fmt->size)
            break;

        char spec = fmt->data[i];

        switch (spec) {
        case PERCENT:
            buf_append(&out, &cap, &len, "%", 1);
            break;

        case STRING: {
            __public__string_t *s = va_arg(ap, __public__string_t*);
            if (s && s->data && s->size)
                buf_append(&out, &cap, &len, s->data, (size_t)s->size);
            break;
        }

        case SIGNED_INT: {
            char tmp[32];
            size_t n = i64_to_dec((int64_t)va_arg(ap, int), tmp);
            buf_append(&out, &cap, &len, tmp, n);
            break;
        }

        case UNSIGNED_INT: {
            char tmp[32];
            size_t n = u64_to_dec((uint64_t)va_arg(ap, unsigned int), tmp);
            buf_append(&out, &cap, &len, tmp, n);
            break;
        }

        case LONG: {
            if (i + 1 < fmt->size && fmt->data[i + 1] == UNSIGNED_INT) {
                i++;
                char tmp[32];
                size_t n = u64_to_dec(
                    (uint64_t)va_arg(ap, unsigned long), tmp);
                buf_append(&out, &cap, &len, tmp, n);
            } else {
                char tmp[32];
                size_t n = i64_to_dec(
                    (int64_t)va_arg(ap, long), tmp);
                buf_append(&out, &cap, &len, tmp, n);
            }
            break;
        }

        case POINTER: {
            char tmp[32];
            size_t n = ptr_to_hex(va_arg(ap, void*), tmp);
            buf_append(&out, &cap, &len, tmp, n);
            break;
        }

        case FLOAT: {
            char tmp[64];
            size_t n = f64_to_dec(va_arg(ap, double), tmp);
            buf_append(&out, &cap, &len, tmp, n);
            break;
        }

        default:
            /* unknown specifier → emit literally */
            buf_append(&out, &cap, &len, "%", 1);
            buf_append(&out, &cap, &len, &spec, 1);
            break;
        }
    }

    va_end(ap);

    __public__string_t *res = allocate(__arena__, sizeof(*res));

    res->data = out;
    res->size = (size_t)len;
    return res;
}


/**
 * @brief Get the length of a string
 * @param str String to measure
 * @return Length of the string
 */
int __public__strings_length(__public__string_t* str) {
    return (int)str->size;
}

/**
 * @brief Get the ith byte of a string
 * @param str String to measure
 * @return Length of the string
 */
uint8_t __public__strings_get(__public__string_t* str, int i) {
    // unsafe, need to do bound check
    return (uint8_t)str->data[i];
}

/**
 * @brief Compare two strings
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @return 0 if equal, negative if str1 < str2, positive if str1 > str2
 */
int __public__strings_compare(__public__string_t* str1, __public__string_t* str2) {
    if (str1 == NULL || str2 == NULL) return (str1 == str2) ? 0 : (str1 ? 1 : -1);

    int64_t min = (str1->size < str2->size) ? (int64_t)str1->size : (int64_t)str2->size;

    for (int64_t i = 0; i < min; i++) {
        unsigned char c1 = (unsigned char)str1->data[i];
        unsigned char c2 = (unsigned char)str2->data[i];
        if (c1 != c2)
            return c1 - c2;
    }

    return (int)(str1->size - str2->size);
}

/**
 * @brief append character to given string
 * @param str string
 * @param ch characted to be appended
 */
void __public__strings_append(__public__string_t* str, int8_t ch) {
    int64_t size = (int64_t)(++str->size);
    char* data = (char*)allocate(__arena__, (size_t)size + 1);
    memcpy(data, str->data, (size_t)size * sizeof(char));
    data[size-1] = ch;
    data[size] = '\0'; // only to provide backward compatibility with c code

    release(__arena__, str->data);
    str->data = data;
}

/**
 * @brief append string to given string
 * @param str1 string1
 * @param str2 string2
 */
void __public__strings_join(__public__string_t* str1, __public__string_t* str2) {
    int64_t size = (int64_t)(str1->size + str2->size);
    char* data = (char*)allocate(__arena__, (size_t)size + 1);

    memcpy(data, str1->data, str1->size * sizeof(char));

    memcpy(data + str1->size, str2->data, str2->size * sizeof(char));
    
    data[size] = '\0'; // only to provide backward compatibility with c code

    release(__arena__, str1->data);
    str1->data = data;
    str1->size = (size_t)size;
}

__public__string_t* __public__strings_substring(__public__string_t* s, int64_t start, int64_t end) {
    if( start < 0 || end > (int64_t)s->size) {
        return NULL;
    }
    
    int64_t size = end - start;
    char* data = (char*)allocate(__arena__, (size_t)size + 1);
    memcpy(data, s->data + start, (size_t)size * sizeof(char));
    data[size] = '\0';

    __public__string_t* res = allocate(__arena__, sizeof(__public__string_t));
    res->data = data;
    res->size = (size_t)size;

    return res;
}
