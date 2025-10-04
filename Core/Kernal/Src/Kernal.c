/*
 * @Author: qwqb233 qwqb.zhang@gmail.com
 * @Date: 2025-10-03 11:01:28
 * @LastEditors: qwqb233 qwqb.zhang@gmail.com
 * @LastEditTime: 2025-10-04 11:41:01
 * @FilePath: \asm_test\Core\Kernal\Src\Kernal.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "common.h" 
#include "Kernal.h"


extern uint32_t _sbss[],_ebss[],_estack[];

//创建用户栈
#define USER_STACK_SIZE 64
__attribute__((section(".bss"), aligned(8)))
paddr_t user_stack[USER_STACK_SIZE];
paddr_t user_stack_top;

void init_stack_top(void)
{
    user_stack_top = (paddr_t)(&(user_stack[64]));
}

void *my_memset(void *buf, char c, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    while (n--)
        *p++ = c;
    return buf;
}

void usage_fault_init(void)
{
    __asm__ __volatile__(
        // 设置SCB->SHCSR的USGFAULTENA位
        "ldr r0, =0xE000ED24   \n"
        "ldr r1, [r0]          \n"
        "orr r1, r1, #0x00040000 \n"
        "str r1, [r0]          \n"
        
        // 设置SCB->CCR的DIV_0_TRP位
        "ldr r0, =0xE000ED14   \n"
        "ldr r1, [r0]          \n"
        "orr r1, r1, #0x10     \n"
        "str r1, [r0]          \n"
        :
        :
        : "r0", "r1", "memory"
    );
}
void trigger_usage_fault(void)
{
    // 确保已经初始化
    // 触发除零，进入UsageFault
    int a = 1 / 0;
}

void kernel_main(void) {
    my_memset(_sbss, 0, (size_t) _ebss - (size_t) _sbss);
    usage_fault_init();
    init_stack_top();
    // 将sp指向用户栈顶
    // __asm__ volatile("mov sp, %0":: "r" (user_stack_top));
    // 将psp指向用户栈顶,并切换到用户栈模式
    __asm__ volatile("msr psp, %0":: "r" (user_stack_top));
    __asm__ volatile("msr control, %0":: "r" (0x00000002));
    // 触发用户中断
    int a = 1 / 0;

    for (;;);
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mov sp, %[stack_top]\n" // 设置栈指针
        "b kernel_main\n"       // 跳转到内核主函数
        :
        : [stack_top] "r" (_estack) // 将栈顶地址作为 %[stack_top] 传递
    );
}

void handler_panic(const char *msg) {
    PANIC("test");
}


uint32_t get_free_RAM(uint32_t page_ptr)
{
    // 获取RAM的终止地址
    uint32_t ram_end = (uint32_t)_estack;
    // 获取出去.bss段后的地址
    uint32_t ram_start = (uint32_t)page_ptr;
    // 计算剩余的RAM大小
    uint32_t free_ram = ram_end - ram_start;

    return free_ram;
}

/**
 * 分配指定数量的页内存
 * @param n 需要分配的页数
 * @return 分配的内存起始地址
 */
void* alloc_page(uint32_t n)
{
    // 从栈顶开始分配一页内存
    static uint32_t next_page_ptr = (uint32_t)_ebss;  // 静态变量，记录下一次分配的内存起始地址，初始化为_ebss地址
    uint32_t now_page_ptr = next_page_ptr;            // 保存当前分配的内存起始地址
    next_page_ptr += PAGE_SIZE * n;                   // 更新下一次分配的内存起始地址，增加n个页的大小

    // 检查是否有足够的可用内存
    uint32_t free_RAM = get_free_RAM(now_page_ptr);  // 获取当前地址的可用内存大小
    if (free_RAM < PAGE_SIZE) {                      // 如果可用内存小于一页大小
        PANIC("Out of memory");                      // 触发内存不足错误
    }
    ker_memset((void*)now_page_ptr, 0, PAGE_SIZE);   // 将分配的内存页清零
    return (void*)now_page_ptr;                      // 返回分配的内存页起始地址
}

void UsageFault_Handler(void)
{
    // 进入函数后，硬件自动保存当前上下文，但单步执行时会向栈中写类似pc的值，可能是PSP和MSP的问题
    // 在中断中会使用MSP，试一下在外部切换为PSP看看栈中会不会出现奇怪的值.
    
    // 问题解决，在进入中断前也就是执行任务函数时使用PSP，中断时使用MSP
    
    // 保存上下文流程
    // 1.保存psp到r0
    // 2.使用r0作为栈指针，保存寄存器，stmdb sp!, {r0-r12, lr}
    // 3.FUP寄存器，这里是f103芯片，没有fpu
    // 4.psp更新为r0
    __asm__ volatile(
        "mrs r0, psp\n"
        "stmdb r0!,{r4-r11}\n"
        "msr psp, r0\n"
    );
    __asm__ volatile(
        // 调用处理函数
        "bl handler_panic               \n"
        "1: b 1b                        \n"
    );
}
