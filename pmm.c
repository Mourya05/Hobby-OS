#include "pmm.h"

// Exposed by the linker script linker.ld
extern uint32_t _kernel_end;

// Place the bitmap directly after the kernel
static uint32_t* pmm_bitmap = (uint32_t*)&_kernel_end;

static uint32_t pmm_max_frames = 0;
static uint32_t pmm_used_frames = 0;

void pmm_set_bit(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    pmm_bitmap[index] |= (1 << bit);
}

void pmm_clear_bit(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    pmm_bitmap[index] &= ~(1 << bit);
}

bool pmm_test_bit(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    return (pmm_bitmap[index] & (1 << bit)) != 0;
}

void init_pmm(uint32_t start_addr, uint32_t size) {
    // 1. Calculate how many frames we have in total
    pmm_max_frames = size / PMM_FRAME_SIZE;
    
    // 2. Calculate the size of the bitmap itself
    //    Each frame requires 1 bit. Divide by 32 to get number of uint32_t blocks.
    uint32_t bitmap_size_dwords = pmm_max_frames / 32;
    if (pmm_max_frames % 32 != 0) {
        bitmap_size_dwords++;
    }

    // 3. Mark everything as free (0)
    for (uint32_t i = 0; i < bitmap_size_dwords; i++) {
        pmm_bitmap[i] = 0; 
    }
    
    // 4. We must protect critical memory regions from being allocated!
    //    - The first 1 MB (0x0 to 0x100000) which contains the BIOS Data Area, VGA text buffer, etc.
    //    - The Kernel code and data (0x100000 to _kernel_end)
    //    - The PMM Bitmap itself (_kernel_end to _kernel_end + bitmap size)
    
    uint32_t bitmap_size_bytes = bitmap_size_dwords * 4;
    uint32_t reserved_end_addr = (uint32_t)&_kernel_end + bitmap_size_bytes;
    
    // Align up to the next page boundary
    reserved_end_addr = (reserved_end_addr + PMM_FRAME_SIZE - 1) & ~(PMM_FRAME_SIZE - 1);
    
    uint32_t reserved_frames = reserved_end_addr / PMM_FRAME_SIZE;
    
    // Set bits to 1 (used) for all frames up to the end of our reserved area
    for (uint32_t frame = 0; frame < reserved_frames; frame++) {
        pmm_set_bit(frame);
    }
    
    pmm_used_frames = reserved_frames;
    
    // We suppress unused warnings for start_addr as we assume 0x0 base for simplicity
    (void)start_addr; 
}

uint32_t pmm_alloc_block(void) {
    // Search the bitmap for the first free 0
    for (uint32_t i = 0; i < pmm_max_frames / 32; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) { // If block isn't entirely full of 1s
            for (uint32_t bit = 0; bit < 32; bit++) {
                if (!(pmm_bitmap[i] & (1 << bit))) { // We found a 0 bit!
                    uint32_t frame = i * 32 + bit;
                    pmm_set_bit(frame);              // Mark it as used
                    pmm_used_frames++;
                    return frame * PMM_FRAME_SIZE;   // Return the physical memory address
                }
            }
        }
    }
    return 0; // Return 0 if Out of Memory
}
