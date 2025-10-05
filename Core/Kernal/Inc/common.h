/*
 * @Author: qwqb233 qwqb.zhang@gmail.com
 * @Date: 2025-10-03 11:20:58
 * @LastEditors: qwqb233 qwqb.zhang@gmail.com
 * @LastEditTime: 2025-10-03 11:21:10
 * @FilePath: \asm_test\Core\Kernal\Inc\common.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#ifndef __STM32F1xx_HAL_H

typedef int bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;
typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;

#define true  1
#define false 0
#define NULL  ((void *) 0)
#define align_up(value, align)   __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member)   __builtin_offsetof(type, member)
#define va_list  __builtin_va_list
#define va_start __builtin_va_start
#define va_end   __builtin_va_end
#define va_arg   __builtin_va_arg

#endif

// 内核恐慌处理
// ##__VA_ARGS__ 宏，当不传入参数时，##__VA_ARGS__ 会被忽略，保证编译不出错
#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        ker_printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)

#define PAGE_SIZE 4096

void *ker_memset(void *buf, char c, size_t n);
void *ker_memcpy(void *dst, const void *src, size_t n);
char *ker_strcpy(char *dst, const char *src);
int ker_strcmp(const char *s1, const char *s2);
void ker_printf(const char *fmt, ...);