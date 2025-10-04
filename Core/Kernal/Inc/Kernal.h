#pragma once

// 内核恐慌处理
// ##__VA_ARGS__ 宏，当不传入参数时，##__VA_ARGS__ 会被忽略，保证编译不出错
#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        ker_printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)

void boot(void);
void kernel_main(void);
uint32_t get_free_RAM(uint32_t page_ptr);

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

#define WRITE_REG(reg_name, val) \
    __asm__ __volatile__("mov " #reg_name ", %0" :: "r"(val) : "memory");
#define READ_REG(reg_name) \
    ({                     \
    uint32_t __val;       \
    __asm__ __volatile__("mov %0, " #reg_name : "=r"(__val) : : "memory"); \
    __val;                 \
    })
