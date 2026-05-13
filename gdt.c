#include "gdt.h"

struct gdt_entry_struct gdt_entries[6];
struct gdt_ptr_struct   gdt_ptr;
tss_entry_t             tss_entry;

// External assembly function to tell the CPU where the GDT is
extern void gdt_flush(uint32_t);

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry);

    // 0xE9 = Access Byte for 32-bit TSS
    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    // Initialize the TSS to 0 to avoid garbage values
    uint8_t *tss_ptr = (uint8_t *)&tss_entry;
    for(uint32_t i = 0; i < sizeof(tss_entry); i++) {
        tss_ptr[i] = 0;
    }

    tss_entry.ss0  = ss0;  // Kernel Stack Segment
    tss_entry.esp0 = esp0; // Kernel Stack Pointer (we'll set this dynamically later)

    // User mode segments (OR'd with 3 for Ring 3 RPL)
    // 0x08 (Kernel Code) -> 0x0B
    // 0x10 (Kernel Data) -> 0x13
    tss_entry.cs   = 0x0b;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
}

void init_gdt() {
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data
    
    // Allocate a simple kernel stack for when interrupts fire in user-mode
    static uint8_t kernel_stack[4096];
    write_tss(5, 0x10, (uint32_t)&kernel_stack[4096]); // TSS

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
}