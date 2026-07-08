#ifndef STR_H
#define STR_H
#include "platform.h"
#include <stdint.h>
#include "array.h"

/* format modifiers */
#define PERCENT       '%'
#define STRING        's' 
#define SIGNED_INT    'd' 
#define UNSIGNED_INT  'u' 
#define LONG          'l' 
#define POINTER       'p' 
#define FLOAT         'f' 


typedef struct {
    char* data;
    size_t size; 
} __public__string_t;


/**
 * @brief Allocate memory in heap for given string
 * @param fmt Format string
 * @param size Number of bytes
 * @return Pointer to the formatted string
 */
__public__string_t* __public__strings_alloc_from_raw(const char* fmt, size_t size);

/**
 * @brief Get pointer to byte stream
 * @param fmt Format string
 * @return array of byte stream
 */
__public__array_t* __public__strings_get_bytes(__public__string_t* fmt);

/**
 * @brief Allocate memory in heap for given string
 * @param fmt Format string
 * @param size Number of bytes
 * @return Pointer to the formatted string
 */
__public__string_t* __public__strings_alloc_from_arr(__public__array_t* fmt);


/**
 * @brief Format a string with variable arguments
 * @param fmt Format string
 * @param ... Variable arguments to format
 * @return Pointer to the formatted string
 */
__public__string_t* __public__strings_format(__public__string_t* fmt, ...) ;

/**
 * @brief Get the length of a string
 * @param str String to measure
 * @return Length of the string
 */
int __public__strings_length(__public__string_t* str);

/**
 * @brief Get the ith byte of a string
 * @note this returns ith byte not ith unicode character or grapheme 
 * @param str String to measure
 * @param i ith byte
 * @return Length of the string
 */
uint8_t __public__strings_get(__public__string_t* str, int i);

/**
 * @brief Compare two strings
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @return 0 if equal, negative if str1 < str2, positive if str1 > str2
 */
int __public__strings_compare(__public__string_t* str1, __public__string_t* str2);

/* utils */

/**
 * @brief Append charcater to given heap buffer
 * @param buf buffer
 * @param cap size to scale buffer
 * @param len current size of buffer
 * @param src charcter/string to be appended
 * @param n size of src
 */
void buf_append(char **buf, size_t *cap, size_t *len, const char *src, size_t n);

/**
 * @brief Format given unsigned int to string
 * @param v u64
 * @param tmp buffer to hold output formated string
 * @return length of formated string
 */
size_t u64_to_dec(uint64_t v, char tmp[32]);

/**
 * @brief Format given signed int to string
 * @param v int64
 * @param tmp buffer to hold output formated string
 * @return length of formated string
 */
size_t i64_to_dec(int64_t v, char tmp[32]);

/**
 * @brief Format given pointer to hex string
 * @param p pointer
 * @param tmp buffer to hold output formated string
 * @return length of formated string
 */
size_t ptr_to_hex(const void *p, char tmp[32]);


/**
 * @brief Format given float to string
 * @param v double/float
 * @param tmp buffer to hold output formated string
 * @return length of formated string
 */
size_t f64_to_dec(double v, char tmp[64]);

/**
 * @brief append character to given string
 * @param str string
 * @param str characted to be appended
 */
void __public__strings_append(__public__string_t* str, int8_t ch);


/**
 * @brief append string to given string
 * @param str1 string1
 * @param str2 string2
 */
void __public__strings_join(__public__string_t* str1, __public__string_t* str2);


__public__string_t* __public__strings_substring(__public__string_t* s, int64_t start, int64_t end);
#endif