#include "idt.h"

// Define the IDT array with 256 entries
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// External function from interrupt.asm to load the IDT
extern void idt_load(uint32_t);

// Provided by kernel.c
extern void vga_print_at(uint32_t row, uint32_t col, const char *str, uint8_t attr);

/*
 * Set an IDT gate
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    
    // We must uncomment the OR below when we get to using user-mode.
    // It sets the interrupt gate's privilege level to 3.
    idt_entries[num].flags   = flags /* | 0x60 */;
}

/*
 * Initialize the IDT
 */
void init_idt(void)
{
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // Ideally, we would memset idt_entries to 0 here if we had memset.

    // Set gate for ISR 0 (Division by Zero)
    // 0x08 is the Kernel Code Segment selector from the GDT.
    // 0x8E is the flag for a 32-bit Interrupt Gate.
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    
    // Set gate for ISR 14 (Page Fault)
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    
    // Set gate for IRQ 0 (Timer) at index 32
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);

    // Set gate for System Call (int 0x80) at index 128
    // 0xEE = Present, DPL=3, 32-bit Interrupt Gate
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    // Load the IDT
    idt_load((uint32_t)&idt_ptr);
}


