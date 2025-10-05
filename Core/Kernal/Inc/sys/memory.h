#pragma once

#include "common.h" 

void* alloc_page(uint32_t n);
uint32_t get_free_RAM(uint32_t page_ptr);
