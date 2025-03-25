/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2018 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdlib.h>

#include "wiring.h"

// Replacement functions that use EXTMEM instead of RAM2 for new/delete

// with thanks to KurtE and Beermat 
// https://forum.pjrc.com/index.php?threads/putting-objects-instantiated-with-new-into-extmem-instead-of-ram2.76731/

#define RESERVE_RAM2 65536        // keep 64k of RAM2 reserved
extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;

void * operator new(size_t size)
{
    if ((char *)&_heap_end - __brkval > RESERVE_RAM2+size) {
        void * new_obj = malloc(size);
        if (new_obj) return new_obj;
    }
    return extmem_malloc(size);
}

void * operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void * ptr)
{
    if ((uint32_t) ptr >= 0x70000000) {
        extmem_free(ptr);
    } else {
        free(ptr);
    }
}

void operator delete[](void * ptr)
{
    if ((uint32_t) ptr >= 0x70000000) {
        extmem_free(ptr);
    } else {
        free(ptr);
    }
}

void operator delete(void * ptr, size_t size)
{
    if ((uint32_t) ptr >= 0x70000000) {
        extmem_free(ptr);
    } else {
        free(ptr);
    }
}

void operator delete[](void * ptr, size_t size)
{
    if ((uint32_t) ptr >= 0x70000000) {
        extmem_free(ptr);
    } else {
        free(ptr);
    }
}