/*
 * @Author: qwqb233 qwqb.zhang@gmail.com
 * @Date: 2025-10-03 11:01:28
 * @LastEditors: qwqb233 qwqb.zhang@gmail.com
 * @LastEditTime: 2025-10-04 19:29:41
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

// 创建PCB(静态)
#define PROCS_MAX 8       // 最大进程数量

#define PROC_UNUSED   0   // 未使用的进程控制结构
#define PROC_RUNNABLE 1   // 可运行的进程
#define PROC_RUNNING 2   // 正在运行的进程

// 最大255个线程
typedef struct process {
    uint8_t pid;             // 进程 ID
    uint8_t state;           // 进程状态: PROC_UNUSED 或 PROC_RUNNABLE
    uint8_t first_in;
    vaddr_t sp;          // 栈指针，切换任务时记得将sp寄存器保存到这里
    uint32_t stack[64]; // 内核栈
}process_t;

process_t procs[PROCS_MAX];

process_t * create_process(uint32_t pc)
{
    process_t *p = NULL;
    int i = 0;
    for(;i < PROCS_MAX; i++)
    {
        if(procs[i].state == PROC_UNUSED)
        {
            p = &procs[i];
            break;
        }
    }
    if(!p)
    {
        PANIC("No free process");
    }
    p->pid = i;
    p->state = PROC_RUNNABLE;
    p->first_in = 0;
    // 构建异常帧
    p->sp = (vaddr_t)(&(p->stack[64-9]));
    vaddr_t *stack_frame = (vaddr_t*)p->sp;
    stack_frame[0] = 0; // r0
    stack_frame[1] = 0; // r1
    stack_frame[2] = 0; // r2
    stack_frame[3] = 0; // r3
    stack_frame[4] = 0; // r12
    stack_frame[5] = 0; // lr (初始为0，异常返回时使用)
    stack_frame[6] = pc; // pc (程序入口)
    stack_frame[7] = 0x01000000; // xPSR, Thumb模式
    
    return p;
}

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

process_t * current_process = NULL;
process_t * next_process = NULL;

// 使用函数调度器会出现栈问题，可选解决方案为
// 1、修改PendSV_Handler逻辑，将调度交由pendsv异常处理
// 2、将中断触发拿到任务体里面，在任务体中触发中断
// 3、将调度器整体变成宏定义函数
static inline uint8_t yield(void)
{
    process_t * next = next_process;
    // 遍历线程池，寻找空闲线程
    for (int i = 0; i < PROCS_MAX; i++) {
        if(procs[i].state == PROC_RUNNABLE)
        {
            next = &procs[i];
            break;
        }
    }

    if(next == current_process)
    {
        return 0;
    }

    next_process = next;
    // 触发PendSV异常，切换上下文
    return 1;
}

void process_A(void)
{
    // 触发中断
    while(1) 
    {
        switch_contex();
    }
       
}

void process_B(void)
{
    while(1)
    {
        switch_contex();
    }
}

process_t *pA = NULL;
process_t *pB = NULL;

void kernel_main(void) {
    my_memset(_sbss, 0, (size_t) _ebss - (size_t) _sbss);
    usage_fault_init();
    init_stack_top();
    // 将sp指向用户栈顶
    // __asm__ volatile("mov sp, %0":: "r" (user_stack_top));
    // 将psp指向用户栈顶,并切换到用户栈模式
    // __asm__ volatile("msr psp, %0":: "r" (user_stack_top));
    // __asm__ volatile("msr control, %0":: "r" (0x00000002));
    pA = create_process((uint32_t)process_A);
    pB = create_process((uint32_t)process_B);
    // 先切换psp再启动任务
    __asm__ volatile("msr psp, %0":: "r" (procs[0].sp));
    __asm__ volatile("msr control, %0":: "r" (0x00000002));
    switch_contex();  // 启动调度器
    // 触发用户中断
    // int a = 1 / 0;

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

/* 触发 PendSV 异常（立即挂起，等待当前异常返回后执行） */


void UsageFault_Handler(void)
{
    // 进入函数后，硬件自动保存当前上下文，但单步执行时会向栈中写类似pc的值，可能是PSP和MSP的问题
    // 在中断中会使用MSP，试一下在外部切换为PSP看看栈中会不会出现奇怪的值.
    
    // 问题解决，在进入中断前也就是执行任务函数时使用PSP，中断时使用MSP
    
    // cortex内核在进入中断时会自动按如下顺序压入寄存器：xPSR → PC → LR → R12 → R3 → R2 → R1 → R0
    // 需要手动写入其他寄存器：R4-R11

    // 保存上下文流程
    // 1.保存psp到r0
    // 2.使用r0作为栈指针，保存寄存器，stmdb sp!, {r0-r12, lr}
    // 3.FUP寄存器，这里是f103芯片，没有fpu
    // 4.psp更新为r0
    // __asm__ volatile(
    //     "mrs r0, psp\n"
    //     "stmdb r0!,{r4-r11}\n"
    //     "msr psp, r0\n"
    // );
    // 还原上下文流程，不包含切换任务，切换任务需要修改psp
    // 1.保存psp到r0
    // 2.使用r0作为栈指针，还原寄存器，ldmia sp!, {r4-r11}
    // 3.FUP寄存器，这里是f103芯片，没有fpu
    // 4.psp更新为r0
    // 5.中断返回后会自动从psp中还原其他寄存器
    // 不用单独保存lr，中断异常返回会自动还原lr
    // __asm__ volatile(
    //     "mrs r0, psp\n"
    //     "ldmia r0!,{r4-r11}\n"
    //     "msr psp, r0\n"
    // );
    // 任务切换要修改psp，中断返回后会自动还原psp，需要PCB（process control block）
    // 任务切换流程
    // 1.保存当前上下文，将psp存到PCB的sp中
    // 测试，创建了两个线程
    
    
    
    __asm__ volatile(
        // 调用处理函数
        "bl handler_panic               \n"
        "1: b 1b                        \n"
    );
}

void PendSV_Handler(void)
{
    // 保存当前上下文，将psp存到PCB的sp中
    // 使用调度器切换任务，如果没有其他任务则直接返回
    if(yield() == 0) goto return_U;
    if(current_process != NULL)
    {
        __asm__ volatile(
            "mrs r0, psp\n"
            "stmdb r0!,{r4-r11}\n"
            "str r0, %[current]\n"
            : [current] "=m" (current_process->sp)
            :
            : "r0", "memory"
        );
    }
    
    // 切换任务，将psp指向下一个任务的sp
    if(next_process->first_in == 0)
        __asm__ volatile(
            "ldr r0, %[next]\n"
            "msr psp, r0\n"
            :
            : [next] "m" (next_process->sp)
            : "r0", "memory"
        );
    else
        __asm__ volatile(
            "ldr r0, %[next]\n"
            "ldmia r0!, {r4-r11}\n"
            "msr psp, r0\n"
            :
            : [next] "m" (next_process->sp)
            : "r0", "memory"
        );
    next_process->state = PROC_RUNNING;
    next_process->first_in = 1;
    if(current_process != NULL)current_process->state = PROC_RUNNABLE;
    current_process = next_process;

return_U:
    __asm__ volatile(
        "mov lr, #0xFFFFFFFD \n"
        "bx lr\n"
    );
}

