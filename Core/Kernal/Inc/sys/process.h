#pragma once

#include "common.h"

// 创建PCB(静态)
#define PROCS_MAX 8       // 最大进程数量

typedef enum{
    PROC_UNUSED = 0,    // 未使用的进程控制结构
    PROC_BLOCKED,       // 被阻塞的进程
    PROC_RUNNABLE,      // 可运行的进程
    PROC_RUNNING,       // 正在运行的进程
}proc_state_t;

// 最大255个线程
typedef struct process {
    uint8_t pid;             // 进程 ID
    uint8_t state;           // 进程状态: PROC_UNUSED 或 PROC_RUNNABLE
    uint8_t first_in;
    vaddr_t sp;          // 栈指针，切换任务时记得将sp寄存器保存到这里
    uint32_t stack[64]; // 内核栈
}process_t;


process_t * create_process(uint32_t pc, process_t * procs);
uint8_t yield(process_t * procs, process_t ** current_process, process_t ** next_process);
