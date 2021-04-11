/**
 * \file
 *
 * \brief Syscalls for SAM0 (GCC).
 *
 * Copyright (C) 2012-2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sam.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef errno
extern uint32_t errno;
extern uint32_t __HeapBase;
extern uint32_t __HeapLimit;

extern caddr_t _sbrk(int incr);
extern int link(char *old, char *n);
extern int _close(int file);
extern int _fstat(int file, struct stat *st);
extern int _isatty(int file);
extern int _lseek(int file, int ptr, int dir);
extern void _exit(int status);
extern void _kill(int pid, int sig);
extern int _getpid(void);

uint32_t getHeapUsed(void);
uint32_t getHeapSize(void);
int32_t getLastMalloc(void);

uint32_t heapUsed=0; 
int32_t lastMalloc=0;
uint32_t getHeapUsed(void)
{
    return heapUsed;
}

uint32_t getHeapSize(void)
{
    return (uint32_t)&__HeapLimit-(uint32_t)&__HeapBase;
}

int32_t getLastMalloc(void)
{
    return lastMalloc;

}
extern caddr_t _sbrk(int incr)
{
    static unsigned char *heap = NULL;
    unsigned char *prev_heap;

    if (heap == NULL) {
	heap = (unsigned char *)&__HeapBase;
    }
    prev_heap = heap;

    heap += incr;

    lastMalloc = incr;
    heapUsed = (uint32_t) heap - (uint32_t) ((unsigned char *) &__HeapBase);

//#if defined DEBUG
//    __BKPT(3);  //if you hit this something allocated memory, which maybe you do not want... 
//#endif
//    for (;;) { }

    return (caddr_t) prev_heap;
}

extern int link(__attribute__((unused)) char *old,__attribute__((unused)) char *n)
{
    return -1;
}

extern int _close(__attribute__((unused)) int file)
{
    return -1;
}

extern int _fstat(__attribute__((unused)) int file, struct stat *st)
{
    st->st_mode = S_IFCHR;

    return 0;
}

extern int _isatty(__attribute__((unused)) int file)
{
    return 1;
}

extern int _lseek(__attribute__((unused)) int file,__attribute__((unused)) int ptr,__attribute__((unused)) int dir)
{
    return 0;
}

extern void _exit(int status)
{
    printf("Exiting with status %d.\n", status);

    for (;;);
}

extern void _kill(__attribute__((unused)) int pid, __attribute__((unused)) int sig)
{
    return;
}

extern int _getpid(void)
{
    return -1;
}



#ifdef __cplusplus
}
#endif
