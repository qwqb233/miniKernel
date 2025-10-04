#include "asm_main.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <sys/cdefs.h>

void test(void)
{
    __ASM volatile
    (
        "mov r1, #0xff \n"
        "bx lr \n"
    );
}

unsigned int Task1_Stk[64] __attribute__((section(".bss"), aligned(8)));
unsigned int Task2_Stk[64] __attribute__((section(".bss"), aligned(8)));
unsigned int curr_task __attribute__((section(".bss")));
unsigned int Task1_StkTop __attribute__((section(".bss")));
unsigned int Task2_StkTop __attribute__((section(".bss")));

// void Task_body(uint32_t delay_time) __attribute__((naked));
__attribute__((target("thumb"), noinline)) 
void Task_body(void)
{
    __ASM volatile("mov r5, r0");
    // r0默认用于参数传递
    // __volatile uint32_t delay_time;
    // __ASM volatile (
    //     "str %0, [%1]"
    //     :
    //     : "r"(r0), "r"(&delay_time)
    //     : "memory"
    // );
    while(1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        HAL_Delay(1000);   
    }
    
}

void init_stack(void)
{
    // __ASM volatile
    // (
    //     "@ 使用keil时需要导入变量地址，否则无法找到变量地址 \n"

    //     "ldr r0, =Task1_StkTop \n"
    //     "ldr r1, =Task1_Stk + 64 * 4 \n"
    //     "str r1, [r0] \n"

    //     "ldr r0, =Task2_StkTop \n"
    //     "ldr r1, =Task2_Stk + 64 * 4 \n"
    //     "str r1, [r0] \n"

    //     "mov r0, #1 \n"
    //     "msr psp, r0 \n"
    //     "mov r0, #2 \n"
    //     "msr CONTROL, r0 \n"
    //     "isb \n"
    //     "cpsie i \n"
    //     "bx lr \n"
    // );
    // uint32_t* stack_top = (uint32_t*)Task1_StkTop;
    // stack_top--; // 预留PC
    // *stack_top = (uint32_t)Task_body; // 设置任务入口
    // stack_top--; // 预留LR
    // *stack_top = 0xFFFFFFFD; // 设置返回地址
    // stack_top--; // 预留PSR
    // *stack_top = 0x01000000; // 设置PSR
    // // 定位到r0位置，基于默认延时
    // for(int i = 0;i < 5;i++,stack_top--);
    // *stack_top = 1000; // 设置延时参数
    // // 其他位于.bss段初始化为0

    // stack_top = (uint32_t*)Task2_StkTop;
    // stack_top--; // 预留PC
    // *stack_top = (uint32_t)Task_body; // 设置任务入口
    // stack_top--; // 预留LR
    // *stack_top = 0xFFFFFFFD; // 设置返回地址
    // stack_top--; // 预留PSR
    // *stack_top = 0x01000000; // 设置PSR
    // // 定位到r0位置，基于默认延时
    // for(int i = 0;i < 5;i++,stack_top--);
    // *stack_top = 500; // 设置延时参数
    // // 其他位于.bss段初始化为0

    // --- 任务1栈初始化 ---
    // 获取栈的物理顶部地址
    uint32_t* task1_stack_ptr = &Task1_Stk[64]; // 注意：数组索引64是越界的，但指针指向的是末尾之后的位置，即栈顶

    // 在栈上构建异常栈帧
    // task1_stack_ptr--; // PSR
    // *task1_stack_ptr = 0x01000000;
    // task1_stack_ptr--; // PC
    // *task1_stack_ptr = (uint32_t)Task_body;
    // task1_stack_ptr--; // LR
    // *task1_stack_ptr = 0xFFFFFFFD;
    // task1_stack_ptr--; // R12
    // *task1_stack_ptr = 0x22;
    // task1_stack_ptr--; // R3
    // *task1_stack_ptr = 0;
    // task1_stack_ptr--; // R2
    // *task1_stack_ptr = 0;
    // task1_stack_ptr--; // R1
    // *task1_stack_ptr = 0;
    // task1_stack_ptr--; // R0
    // *task1_stack_ptr = 1000; // 任务1的参数

    task1_stack_ptr -= 8;
    
    // 构建异常栈帧（顺序与硬件压栈顺序一致）
    task1_stack_ptr[0] = 1000;                // R0: 任务参数
    task1_stack_ptr[1] = 0;                   // R1
    task1_stack_ptr[2] = 0;                   // R2  
    task1_stack_ptr[3] = 0;                   // R3
    task1_stack_ptr[4] = 0x22;                // R12
    task1_stack_ptr[5] = 0xFFFFFFFD;          // LR = EXC_RETURN (使用PSP，线程模式)
    task1_stack_ptr[6] = (uint32_t)Task_body | 1; // PC = 任务入口(Thumb状态)
    task1_stack_ptr[7] = 0x01000000;          // xPSR (Thumb状态)

    // 保存最终的栈帧指针，这是任务启动时PSP应该指向的位置
    Task1_StkTop = (unsigned int)task1_stack_ptr;
    // Task1_StkTop = (unsigned int)Task1_Stk + 64 * 4 - 1*4;

    // --- 任务2栈初始化 ---
    uint32_t* task2_stack_ptr = &Task2_Stk[64];

    task2_stack_ptr--; // PSR
    *task2_stack_ptr = 0x01000000;
    task2_stack_ptr--; // PC
    *task2_stack_ptr = (uint32_t)Task_body | 1;
    task2_stack_ptr--; // LR
    *task2_stack_ptr = 0xFFFFFFFD;
    task2_stack_ptr--; // R12
    *task2_stack_ptr = 0x22;
    task2_stack_ptr--; // R3
    *task2_stack_ptr = 0;
    task2_stack_ptr--; // R2
    *task2_stack_ptr = 0;
    task2_stack_ptr--; // R1
    *task2_stack_ptr = 0;
    task2_stack_ptr--; // R0
    *task2_stack_ptr = 500; // 任务2的参数

    Task2_StkTop = (unsigned int)task2_stack_ptr;
    // Task2_StkTop = (unsigned int)Task2_Stk + 64 * 4 - 1*4;
}

void start_test(void)
{
    init_stack();
    uint32_t temp = (uint32_t)((uint32_t*)Task1_StkTop)[6];
    __ASM volatile(
        "msr PSP, %0 \n"
        "mov r1, #2 \n"
        "msr CONTROL, r1 \n"
        "isb \n"
        "mov r0, #0xFFFFFFFD \n"
        "mov lr, %1 \n"
        :
        : "r"(Task1_StkTop), "r"(temp)
        : "r0","r1"
    );
    __ASM volatile("bx r0");
}

void my_main(void)
{
    test();
    // 栈初始化测试完成,PSP没有被设置
    // init_stack();
    start_test();
    while(1);
}

