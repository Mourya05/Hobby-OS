#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

/* Minimal Multiboot Information structure to extract memory limits.
 * GRUB passes a pointer to this structure in the EBX register, 
 * which our boot.asm pushes onto the stack as the second argument to kernel_main.
 */
typedef struct multiboot_info
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    /* ... other fields omitted for brevity ... */
} __attribute__((packed)) multiboot_info_t;

#define MULTIBOOT_INFO_MEMORY 0x00000001

#endif
