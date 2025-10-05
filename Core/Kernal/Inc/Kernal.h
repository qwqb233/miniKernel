/*
 * @Author: qwqb233 qwqb.zhang@gmail.com
 * @Date: 2025-10-03 11:01:23
 * @LastEditors: qwqb233 qwqb.zhang@gmail.com
 * @LastEditTime: 2025-10-05 13:41:50
 * @FilePath: \asm_test\Core\Kernal\Inc\Kernal.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include "common.h" 


typedef struct process process_t;

#define switch_contex() \
    __asm__ volatile("ldr r0, =0xE000ED04\n\t" "mov r1, #0x10000000\n\t" "str r1, [r0]\n\t" "isb\n\t": : : "r0", "r1", "memory");

// 无FUP
typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
}reg_t;


typedef enum{
    SYSCALL_YIELD = 0,
    SYSCALL_MALLOC,
    SYSCALL_PANIC,
}sysCall_t;

void kernel_main(void);
void usage_fault_init(void);
void trigger_usage_fault(void);
void kernal_boot(void);
void svc_panic(void);
