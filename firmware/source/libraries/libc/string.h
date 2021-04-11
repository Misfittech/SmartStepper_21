/*
 * string.h
 *
 *  Created on: Jan 8, 2020
 *      Author: trampas
 */

#ifndef SRC_LIBRARIES_LIBC_STRING_H_
#define SRC_LIBRARIES_LIBC_STRING_H_


#include <stdarg.h>
#include <memory.h> //for size_t
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

size_t get_string_after(const char *haystack, const char *needle, char *output, size_t max_length);
int stricmp(char const *a, char const *b);
char* stristr( const char* str1, const char* str2 );
size_t trim_left(char *str,const char *tokens);
size_t trim_right(char *str,const char *tokens);
size_t lstrip(char *str);
size_t rstrip(char *str);
size_t strip(char *str); //removes white space on left and right


#ifdef __cplusplus
}
#endif


#endif /* SRC_LIBRARIES_LIBC_STRING_H_ */
