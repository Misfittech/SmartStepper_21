/*
 * vsnprintf.h
 *
 *  Created on: Aug 7, 2018
 *      Author: xps15
 */

#ifndef SRC_LIBRARIES_LIBC_VSNPRINTF_H_
#define SRC_LIBRARIES_LIBC_VSNPRINTF_H_

#include <stdarg.h>
#include <memory.h> //for size_t
#include <stdarg.h>


#ifdef __cplusplus
extern "C"
{
#endif

size_t _snprintf(char *ptrStr, size_t maxLen, const char *fmt, ...);
size_t _vsnprintf(char *ptrStr, size_t maxLen, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif


#endif /* SRC_LIBRARIES_LIBC_VSNPRINTF_H_ */
