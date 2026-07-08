#ifndef SIGERR_H
#define SIGERR_H
#include "platform.h"
#include "str.h"

/**
 * @brief raises runtime error
 * 
 * @param msg message to be printed in error
 */
void __public__rterr_die(__public__string_t* fmt);

/**
 * @brief raises runtime error
 * 
 * @param msg message to be printed in error
 */
void __public__runtime_error(const char* msg);

/**
 * @brief registers error handlers for common signals.
 */
void init_error_handlers(void);

#endif