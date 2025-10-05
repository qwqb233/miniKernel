/*
 * @Author: qwqb233 qwqb.zhang@gmail.com
 * @Date: 2025-10-03 11:01:28
 * @LastEditors: qwqb233 qwqb.zhang@gmail.com
 * @LastEditTime: 2025-10-05 14:12:07
 * @FilePath: \asm_test\Core\Kernal\Src\Kernal.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include "Kernal.h"
#include "sys/memory.h"
#include "sys/process.h"

extern uint32_t _sbss[],_ebss[],_estack[];

#define WRITE_REG(reg_name, val) \
    __asm__ __volatile__("mov " #reg_name ", %0" :: "r"(val) : "memory");

#define READ_REG(reg_name) \
    ({                     \
    uint32_t __val;       \
    __asm__ __volatile__("mov %0, " #reg_name : "=r"(__val) : : "memory"); \
    __val;                 \
    })

// 系统调用

__attribute__((naked))
void svc_panic(void)
{
    __asm__ volatile(
        "svc 2\n"
        "bx lr \n"
    );
}

void* svc_alloc_page(uint32_t size)
{
    __asm__ volatile(
        "mov r2, %0\n"
        "svc 1\n"
        "bx lr \n"
        :
        : "r"(size)
        : "r2"
    );
}

typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
}svc_args_t;

void sys_call(uint32_t _psp, sysCall_t syscall_num, uint32_t arg1, uint32_t arg2)
{ 
    uint32_t result = 0;
    svc_args_t *args = (svc_args_t*)_psp;
    switch(syscall_num)
    {
        case SYSCALL_MALLOC:
            result = (uint32_t)alloc_page(arg1);
            break;
        case SYSCALL_PANIC:
            PANIC("System call panic");
            break;
        default:
            PANIC("Unknown system call");
            break;
    }
    args->r0 = result;
}

// 系统调用中断
__attribute__((naked))
void SVC_Handler(void)
{
    __asm__ volatile(
        "tst lr, #4 \n"
        "ite eq \n"
        "mrseq r0, msp \n"
        "mrsne r0, psp \n"
        "ldr r1, [r0, #24]   \n"  // 获取PC
        "ldrb r1, [r1, #-2]  \n"  // 获取SVC编号
        "bl sys_call \n"
        "mov lr, #0xFFFFFFFD \n"
        "bx lr \n"
    );
}


// TODO: 中断处理


// 线程池
process_t procs[PROCS_MAX]; // 线程池
process_t * current_process = NULL;
process_t * next_process = NULL;

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

// TODO: 完成时间片轮询后，删除这里的主动让出CPU
void idle_task(void)
{
    for(;;)
    {
        switch_contex();
    }
}

void kernel_main(void) {
    ker_memset(_sbss, 0, (size_t) _ebss - (size_t) _sbss);
    usage_fault_init();
    // 先切换psp再启动任务
    // 创建一个空闲任务，防止用户不创建任务时，调度器找不到任务
    create_process((uint32_t)idle_task, procs);
    __asm__ volatile("msr psp, %0":: "r" (procs[0].sp));
    __asm__ volatile("msr control, %0":: "r" (0x00000002));
    switch_contex();  // 启动调度器

    for (;;);
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void kernal_boot(void) {
    __asm__ __volatile__(
        "mov sp, %[stack_top]\n" // 设置栈指针
        "b kernel_main\n"       // 跳转到内核主函数
        :
        : [stack_top] "r" (_estack) // 将栈顶地址作为 %[stack_top] 传递
    );
}

void handler_panic(const char *msg) {
    PANIC("test");
    (void)msg;
}


// 除零错误会进入这个中断
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

// TODO: systick触发PendSV异常

void PendSV_Handler(void)
{
    // 保存当前上下文，将psp存到PCB的sp中
    // 使用调度器切换任务，如果没有其他任务则直接返回
    if(yield(procs, &current_process, &next_process) == 0) goto return_U;
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

