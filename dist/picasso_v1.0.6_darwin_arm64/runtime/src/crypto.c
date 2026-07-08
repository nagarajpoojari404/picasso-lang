#include "platform.h"
#include "crypto.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/**
 * @brief returns hash of given data
 * 
 * @param data data, e.g, struct pointer, string
 * @param len length of datatype
 */
uint64_t hash(const void *data, size_t len) {
    const unsigned char *bytes = (const unsigned char *)data;
    uint64_t hash = 1469598103934665603ULL;  // FNV offset basis
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint64_t)bytes[i];
        hash *= 1099511628211ULL;  // FNV prime
    }
    return hash;
}
