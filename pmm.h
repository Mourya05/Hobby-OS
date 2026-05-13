#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stdbool.h>

#define PMM_FRAME_SIZE 4096 // 4 KB frames

// Initialize the physical memory manager
void init_pmm(uint32_t start_addr, uint32_t size);

// Bitmap manipulation functions
void pmm_set_bit(uint32_t frame);
void pmm_clear_bit(uint32_t frame);
bool pmm_test_bit(uint32_t frame);

// Allocate a contiguous 4KB physical block
uint32_t pmm_alloc_block(void);

#endif
