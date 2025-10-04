/*
 * @Author: qwqb233 qwqb.zhang@gmail.com
 * @Date: 2025-10-03 11:01:28
 * @LastEditors: qwqb233 qwqb.zhang@gmail.com
 * @LastEditTime: 2025-10-04 10:57:53
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
void trigger_usage_fault(void)
{
    // 使用单个指令设置两个寄存器
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
    
    // 触发除零，进入UsageFault
    int a = 10;
    int b = 0;
    int c = a / b;
}

void kernel_main(void) {
    my_memset(_sbss, 0, (size_t) _ebss - (size_t) _sbss);
    init_stack_top();
    // 触发用户中断
    trigger_usage_fault();

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

// TODO: 试试中断保存到用户栈先
paddr_t push_registers_custom_stack(paddr_t stack_ptr) {
    // stack_ptr 要是栈顶地址
    // 可能不合法，AI写的都是乐色，不如我灵机一动
    // 会丢掉r0中的数据
    paddr_t origin_ptr = 0;
    paddr_t update_ptr = 0;
    __asm__ volatile (
        "mov %[ori_stack], sp\n"
        "mov sp, %[stack_ptr]\n"
        "push {r0-r12, lr}\n"
        "mov %[update_stack], sp\n"
        "mov sp, %[ori_stack]\n"
        : [ori_stack] "=r" (origin_ptr),[update_stack] "=r" (update_ptr)
        : [stack_ptr] "r" (stack_ptr)
        : "r0", "r1", "memory"
    );
    return update_ptr;
}

paddr_t pop_registers_custom_stack(paddr_t stack_ptr) {
    paddr_t update_ptr = 0;
    __asm__ volatile(
        "mov sp, %[stack_ptr]\n"
        :
        : [stack_ptr] "r" (stack_ptr)
        : "r0", "r1", "r2",  "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "memory"
    );
    return update_ptr;
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
    // 推入用户栈并更新栈顶
    user_stack_top = push_registers_custom_stack(user_stack_top);
    // user_stack_top = pop_registers_custom_stack(user_stack_top);
    __asm__ __volatile__(
        // 调用处理函数
        "bl handler_panic               \n"
        "1: b 1b                        \n"
    );
}
