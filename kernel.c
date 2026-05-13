/*
 * =============================================================================
 * kernel.c - Freestanding C Kernel Entry Point
 * =============================================================================
 * This file is compiled WITHOUT the C standard library (-ffreestanding
 * -nostdlib). That means NO printf, NO malloc, NO string.h — nothing from
 * the host OS. Every operation must talk directly to hardware.
 *
 * VGA Text Mode (80x25):
 *   The BIOS/GRUB leaves the display in 80-column × 25-row VGA text mode.
 *   Video memory is mapped starting at physical address 0xB8000.
 *   Each character on screen occupies TWO consecutive bytes:
 *
 *       Byte 0 (low):  ASCII character code
 *       Byte 1 (high): Attribute byte  [bits 7-4 = background, 3-0 = foreground]
 *
 *   Common VGA colour codes (foreground/background):
 *       0x0 = Black   0x1 = Blue    0x2 = Green   0x3 = Cyan
 *       0x4 = Red     0x5 = Magenta 0x6 = Brown   0x7 = Light Grey
 *       0x8 = Dark Grey ...         0xF = White
 *
 *   Attribute 0x0F = White text on Black background  (our choice)
 *   Attribute 0x02 = Green text on Black background
 * =============================================================================
 */
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "multiboot.h"
#include "pmm.h"
#include "multiboot.h"
#include "pmm.h"
#include "paging.h"
#include "task.h"
#include "syscall.h"

#include <stdint.h>

/* ── VGA Constants ────────────────────────────────────────────────────────── */
#define VGA_WIDTH       80          /* Columns per row                        */
#define VGA_HEIGHT      25          /* Rows on screen                         */
#define VGA_MEMORY      0xB8000     /* Physical base address of video RAM     */

/* Colour attribute byte: high nibble = background (Black=0), 
                          low nibble  = foreground (Light Cyan=0xB)          */
#define VGA_COLOR_DEFAULT   0x0B    /* Light Cyan on Black — stands out nicely*/
#define VGA_COLOR_BRIGHT    0x0F    /* Bright White on Black                  */
#define VGA_COLOR_SUCCESS   0x0A    /* Bright Green on Black                  */

/* ── Pointer to the VGA framebuffer ──────────────────────────────────────── */
/*
 * 'volatile' is CRITICAL here.
 * Without it the compiler may optimise away writes to this pointer because
 * from C's perspective nobody reads them back. The 'volatile' keyword tells
 * the compiler: "side-effects matter — do NOT eliminate these writes."
 */
static volatile uint16_t *const vga_buffer =
    (volatile uint16_t *)VGA_MEMORY;

/* ── Internal cursor state ────────────────────────────────────────────────── */
static uint32_t cursor_row = 0;
static uint32_t cursor_col = 0;

/* =============================================================================
 * vga_make_entry()
 * Build a 16-bit VGA entry from a character and an attribute byte.
 *   Bit layout:  [15..8] = attribute   [7..0] = ASCII code
 * ============================================================================= */
static inline uint16_t vga_make_entry(char c, uint8_t attr)
{
    return (uint16_t)((uint16_t)attr << 8) | (uint8_t)c;
}

/* =============================================================================
 * vga_clear_screen()
 * Fill every cell of the 80×25 grid with a space character (black on black),
 * effectively blanking the display. Called once at kernel startup.
 * ============================================================================= */
static void vga_clear_screen(void)
{
    uint32_t i;
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        /* Write space character with default attribute to every cell */
        vga_buffer[i] = vga_make_entry(' ', VGA_COLOR_DEFAULT);
    }
    /* Reset software cursor to top-left */
    cursor_row = 0;
    cursor_col = 0;
}

/* =============================================================================
 * vga_put_char()
 * Write a single character to the VGA buffer at the current cursor position,
 * handling newlines and advancing the cursor.
 * ============================================================================= */
static void vga_put_char(char c, uint8_t attr)
{
    if (c == '\n') {
        /* Newline: move to the first column of the next row */
        cursor_col = 0;
        cursor_row++;
        return;
    }

    if (c == '\r') {
        /* Carriage return: move to beginning of current row */
        cursor_col = 0;
        return;
    }

    /* Calculate the linear index into the flat VGA buffer array */
    uint32_t index = cursor_row * VGA_WIDTH + cursor_col;

    /* Write the character+attribute entry into video RAM */
    vga_buffer[index] = vga_make_entry(c, attr);

    /* Advance the cursor; wrap to next line if we hit the right edge */
    cursor_col++;
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }

    /* TODO (future): implement scrolling when cursor_row >= VGA_HEIGHT */
}

/* =============================================================================
 * vga_print()
 * Print a null-terminated ASCII string to the VGA buffer.
 * We cannot use strlen() or any libc function, so we walk the pointer manually.
 * ============================================================================= */
static void vga_print(const char *str, uint8_t attr)
{
    /* Iterate until the null terminator '\0' */
    while (*str != '\0') {
        vga_put_char(*str, attr);
        str++;
    }
}

/* =============================================================================
 * vga_print_at()
 * Move the cursor to (row, col) and then print a string.
 * Useful for placing labels at specific positions on the screen.
 * ============================================================================= */
void vga_print_at(uint32_t row, uint32_t col, const char *str, uint8_t attr)
{
    cursor_row = row;
    cursor_col = col;
    vga_print(str, attr);
}

/* =============================================================================
 * draw_border()
 * Draw a simple ASCII border around the screen using box-drawing characters.
 * This demonstrates direct framebuffer manipulation beyond simple string output.
 * ============================================================================= */
static void draw_border(void)
{
    uint32_t r, c;
    uint8_t border_attr = 0x09; /* Bright Blue on Black */

    /* Top and bottom rows */
    for (c = 0; c < VGA_WIDTH; c++) {
        vga_buffer[0 * VGA_WIDTH + c]                    = vga_make_entry('=', border_attr);
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + c]     = vga_make_entry('=', border_attr);
    }

    /* Left and right columns (skip corners already drawn) */
    for (r = 1; r < VGA_HEIGHT - 1; r++) {
        vga_buffer[r * VGA_WIDTH + 0]              = vga_make_entry('|', border_attr);
        vga_buffer[r * VGA_WIDTH + VGA_WIDTH - 1]  = vga_make_entry('|', border_attr);
    }
}

/* =============================================================================
 * Helper function to convert an integer to a string (base 10)
 * ============================================================================= */
void itoa(uint32_t num, char* str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    while (num != 0) {
        str[i++] = (num % 10) + '0';
        num /= 10;
    }
    str[i] = '\0';
    
    /* Reverse the string */
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/* =============================================================================
 * idt_handler()
 * The C handler called by the common stub in interrupt.asm
 * ============================================================================= */
void idt_handler(registers_t *regs)
{
    if (regs->int_no == 32) {
        /* IRQ 0 - Timer */
        tick_count++;
        /* Send End of Interrupt */
        pic_send_eoi(0);
    } 
    else if (regs->int_no == 0) {
        /* ISR 0 - Division by Zero */
        vga_print_at(15, 20, "EXCEPTION: Division by Zero Detected!", 0x0C);
        for(;;) {
            __asm__ volatile ("hlt");
        }
    }
    else if (regs->int_no == 14) {
        /* ISR 14 - Page Fault */
        uint32_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r" (cr2));
        
        vga_print_at(15, 20, "EXCEPTION: Page Fault (ISR 14) Detected!", 0x0C);
        
        /* Quick and dirty print of CR2 (Faulting Address) */
        vga_print_at(16, 20, "Faulting Address CR2: ", 0x0E);
        
        // Print it roughly
        char cr2_str[16];
        itoa(cr2, cr2_str);
        vga_print_at(16, 42, cr2_str, 0x0E);
        
        for(;;) {
            __asm__ volatile ("hlt");
        }
    }
}

/* =============================================================================
 * user_function()
 * A simple function to run in User Mode (Ring 3).
 * It uses inline assembly to trigger an interrupt (System Call).
 * ============================================================================= */
void user_function(void) {
    /* We are now in Ring 3. 
       We trigger our custom system call interrupt. 
       EAX = 1 (sys_print)
       EBX = pointer to string
    */
    const char *my_string = "Hello from Ring 3! Syscall Works!";
    
    __asm__ volatile (
        "mov $1, %%eax \n"
        "mov %0, %%ebx \n"
        "int $0x80     \n"
        : 
        : "r" (my_string)
        : "eax", "ebx", "memory"
    );
    
    /* We spin infinitely. Note: in User Mode, 'hlt' is a privileged instruction,
       so we just use an empty loop. */
    for(;;) {}
}


/* =============================================================================
 * kernel_main()
 * The C entry point called by boot.asm.
 *
 * Parameters:
 *   multiboot_magic - Should equal 0x2BADB002 if loaded by a Multiboot loader
 *   multiboot_info  - Pointer to the Multiboot Information structure
 *                     (we ignore it here but it's useful for memory maps later)
 * ============================================================================= */
void kernel_main(uint32_t multiboot_magic, void *multiboot_info)
{
    /* Suppress unused-parameter warnings for magic */
    (void)multiboot_magic;

    /* ── Step 0: Initialize Hardware ──────────────────────────────────────── */
    init_gdt();
    init_idt();
    pic_remap();
    init_timer(100); // 100 Hz
    __asm__ volatile ("sti");

    /* ── Step 1: Initialize Physical Memory Manager ───────────────────────── */
    multiboot_info_t* mbi = (multiboot_info_t*)multiboot_info;
    uint32_t mem_size_kb = 16384; /* Default to 16MB if not provided */
    if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
        /* mem_upper is memory above 1MB in KB. Total = mem_upper + 1024 KB */
        mem_size_kb = mbi->mem_upper + 1024;
    }
    
    init_pmm(0, mem_size_kb * 1024);

    /* ── Step 2: Initialize Paging ────────────────────────────────────────── */
    init_paging();

    /* ── Step 3: Clear the entire screen ──────────────────────────────────── */
    vga_clear_screen();

    /* ── Step 4: Draw a decorative border ─────────────────────────────────── */
    draw_border();

    /* ── Step 5: Print the main success message (centred on row 10) ───────── */
    /*  strlen("  Kernel Loaded Successfully  ") = 30 → start col = (80-30)/2 = 25  */
    vga_print_at(10, 22, "[ Kernel Loaded Successfully ]", VGA_COLOR_SUCCESS);
    vga_print_at(11, 29, "[ IDT Initialized ]", VGA_COLOR_SUCCESS);
    vga_print_at(12, 29, "[ Paging Enabled ]",  VGA_COLOR_SUCCESS);

    /* ── Step 6: Print OS identification info ─────────────────────────────── */
    vga_print_at(13, 27, "32-bit x86 Hobby OS  v0.1",    VGA_COLOR_BRIGHT);
    vga_print_at(14, 24, "Protected Mode | VGA Direct Write", 0x07);

    /* Print total physical memory available */
    char mem_str[16];
    itoa(mem_size_kb / 1024, mem_str);
    vga_print_at(15, 27, "Memory Available: ", 0x0E); // Yellow text
    vga_print_at(15, 45, mem_str, 0x0E);
    vga_print_at(15, 48, " MB", 0x0E);
    
    /* ── Step 7: Verify Paging (Virtual Write) ────────────────────────────── */
    uint32_t *ptr = (uint32_t *)0x300000; // 3 MB mark (well above our small kernel)
    *ptr = 0xDEADBEEF;                    // "Virtual Write" test
    if (*ptr == 0xDEADBEEF) {
        vga_print_at(17, 24, "Paging Test: Virtual Write OK!", 0x0A); // Green
    }

    /* ── Step 8: Print boot instructions at the bottom ────────────────────── */
    vga_print_at(22, 25, "Running User-Mode Task...", 0x08);

    /* ── Step 9: Switch to User Mode ──────────────────────────────────────── */
    switch_to_user_mode();
    user_function();

    /* ── Unreachable ──────────────────────────────────────────────────────── */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
