#ifndef CRYPTO_H
#define CRYPTO_H

#include "platform.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @todo: remove this crypto module
 * @brief returns hash of given data
 * @param data data, e.g, struct pointer, string
 * @param len length of datatype
 */
uint64_t hash(const void *data, size_t len);
#endif