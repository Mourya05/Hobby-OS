#include "syscall.h"
#include <stdint.h>

/* Provided by kernel.c */
extern void vga_print_at(uint32_t row, uint32_t col, const char *str, uint8_t attr);

void syscall_dispatcher(registers_t *regs) {
    /* 
     * Security Check: We must validate pointers passed from User Mode!
     * If we don't, a malicious user program could pass a kernel memory address 
     * (e.g., 0x100000) in EBX. Since the kernel has Ring 0 privileges, the 
     * vga_print_at function would blindly read and print kernel secrets to 
     * the screen. In a real OS, we would verify that the address in EBX 
     * falls strictly within the user process's allocated memory space.
     */
    
    uint32_t syscall_num = regs->eax;
    
    if (syscall_num == 1) {
        /* sys_print: EBX contains pointer to a null-terminated string */
        const char *str = (const char *)regs->ebx;
        
        /* NOTE: Missing pointer validation here! (Security Hole) */
        
        vga_print_at(19, 20, str, 0x0B); // Print in Cyan
    } else {
        vga_print_at(19, 20, "Unknown System Call!", 0x0C); // Red error
    }
}
