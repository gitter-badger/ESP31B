/*
 Copyright (c) 2014 Arduino.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdlib.h>
#include <assert.h>
extern "C" {
#include "esp_common.h"

void __panic_func(const char* file, int line, const char* func) __attribute__((noreturn));
#define panic() __panic_func(__FILE__, __LINE__, __func__)
}

void abort() __attribute__((noreturn));

void abort(){
    do { *((int*)0) = 0; } while(true);
}

void __panic_func(const char* file, int line, const char* func) {
  os_printf("PANIC: file: %s, line: %d, func: %s\n");
  abort();
}

void *operator new(size_t size) {
    size = ((size + 3) & ~((size_t)0x3));
    return malloc(size);
}

void *operator new[](size_t size) {
    size = ((size + 3) & ~((size_t)0x3));
    return malloc(size);
}

void operator delete(void * ptr) {
    free(ptr);
}

void operator delete[](void * ptr) {
    free(ptr);
}

extern "C" void __cxa_pure_virtual(void) __attribute__ ((__noreturn__));
extern "C" void __cxa_deleted_virtual(void) __attribute__ ((__noreturn__));

void __cxa_pure_virtual(void) {
  panic();
}

void __cxa_deleted_virtual(void) {
  panic();
}

namespace std {
void __throw_bad_function_call() {
  panic();
}

void __throw_length_error(char const*) {
  panic();
}

void __throw_bad_alloc() {
  panic();
}

void __throw_logic_error(const char* str) {
  panic();
}
}

// TODO: rebuild windows toolchain to make this unnecessary:
void* __dso_handle;
