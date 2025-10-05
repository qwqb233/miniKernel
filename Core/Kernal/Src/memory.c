/*
 * @Author: qwqb233 qwqb.zhang@gmail.com
 * @Date: 2025-10-05 13:32:37
 * @LastEditors: qwqb233 qwqb.zhang@gmail.com
 * @LastEditTime: 2025-10-05 17:26:17
 * @FilePath: \asm_test\Core\Kernal\Src\memory.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "sys/memory.h"

// TODO: 动态内存分配，管理一页内存

extern uint32_t _sbss[],_ebss[],_estack[];

// 创建页表，管理一页内存
typedef struct page_list{
    uint32_t start_addr;  // 内存起始地址
    uint32_t remain;    // 剩余内存大小
    uint32_t* page_ptr;  // 页指针
    struct page_list* next;  // 下一个页表
}page_list_t;

page_list_t* page_list_head = NULL;

void init_page_list(void)
{
    // TODO: 初始化页表
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
        // TODO: 改为置错误位
        PANIC("Out of memory");                      // 触发内存不足错误
    }
    ker_memset((void*)now_page_ptr, 0, PAGE_SIZE);   // 将分配的内存页清零
    return (void*)now_page_ptr;                      // 返回分配的内存页起始地址
}

void* ker_malloc(uint32_t size)
{

}