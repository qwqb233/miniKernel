/*
 * @Author: qwqb233 qwqb.zhang@gmail.com
 * @Date: 2025-10-05 13:47:53
 * @LastEditors: qwqb233 qwqb.zhang@gmail.com
 * @LastEditTime: 2025-10-05 16:57:22
 * @FilePath: \asm_test\Core\Kernal\Src\process.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "sys/process.h"

process_t * create_process(uint32_t pc, process_t * procs)
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

// 使用函数调度器会出现栈问题，可选解决方案为
// 1、修改PendSV_Handler逻辑，将调度交由pendsv异常处理
// 2、将中断触发拿到任务体里面，在任务体中触发中断
// 3、将调度器整体变成宏定义函数
inline uint8_t yield(process_t * procs, process_t ** current_process, process_t ** next_process)
{
    process_t * next = *next_process;
    process_t * current = *current_process;
    // 遍历线程池，寻找空闲线程
    for (int i = 0; i < PROCS_MAX; i++) {
        if(procs[i].state == PROC_RUNNABLE)
        {
            next = &procs[i];
            break;
        }
    }

    if(next == current)
    {
        return 0;
    }

    *next_process = next;
    // 触发PendSV异常，切换上下文
    return 1;
}

