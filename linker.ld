/*
 * =============================================================================
 * linker.ld - GNU LD Linker Script for the x86 Hobby OS Kernel
 * =============================================================================
 * PURPOSE
 *   A linker script controls HOW the linker combines object files and WHERE it
 *   places each section in the final binary. Without one, ld would use its
 *   built-in defaults which are designed for the host OS — completely wrong
 *   for a bare-metal kernel.
 *
 * KEY DECISIONS
 *   1. Load address: 0x00100000 (1 MiB)
 *      The Multiboot Specification requires the kernel to be loaded above 1 MiB
 *      so it does not conflict with the BIOS data area, real-mode IVT, VGA
 *      buffers, or video ROM that live in the first 640 KB.
 *
 *   2. Entry point: _start
 *      Must match the label exported from boot.asm. GRUB reads the ELF header
 *      to find this address and jumps to it.
 *
 *   3. Section order: .multiboot → .text → .rodata → .data → .bss
 *      The Multiboot header section is placed FIRST so it falls within the
 *      first 8 KB of the file (required by the spec).
 * =============================================================================
 */

/* ── 1. Entry Point ──────────────────────────────────────────────────────── */
/* Tell the ELF linker which symbol is the program entry point.              */
/* GRUB reads the ELF e_entry field and jumps to this address.               */
ENTRY(_start)

/* ── 2. Memory Layout ────────────────────────────────────────────────────── */
SECTIONS
{
    /*
     * '.' is the "location counter" — it tracks the current virtual address.
     * Setting it to 0x100000 means all subsequent sections are placed starting
     * at the 1 MiB mark in virtual (and physical) address space.
     */
    . = 1M;         /* 1M = 0x00100000 — standard Multiboot load address */

    /* ---------------------------------------------------------------------- */
    /* .multiboot — Must be within the first 8 KB of the output file          */
    /* ---------------------------------------------------------------------- */
    /*
     * GRUB performs a brute-force scan of the first 8 KB searching for the
     * Multiboot magic (0x1BADB002). Placing this section first guarantees
     * the header is within the scan window.
     */
    .multiboot :
    {
        KEEP(*(.multiboot))     /* KEEP() prevents --gc-sections from removing it */
    }

    /* ---------------------------------------------------------------------- */
    /* .text — Executable Code                                                 */
    /* ---------------------------------------------------------------------- */
    /*
     * All code sections from all object files are merged here.
     * The ALIGN(4096) ensures the section starts on a 4 KB (page) boundary,
     * which is required later when we implement paging (each page is 4 KB).
     */
    .text ALIGN(4096) :
    {
        *(.text)                /* All .text sections from all object files    */
        *(.text.*)              /* LTO/function-section split variants         */
    }

    /* ---------------------------------------------------------------------- */
    /* .rodata — Read-Only Data (string literals, const globals)               */
    /* ---------------------------------------------------------------------- */
    .rodata ALIGN(4096) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* ---------------------------------------------------------------------- */
    /* .data — Initialised Read-Write Data                                     */
    /* ---------------------------------------------------------------------- */
    /*
     * Global and static variables that have a non-zero initial value.
     * These values are stored in the file and copied to RAM at load time.
     */
    .data ALIGN(4096) :
    {
        *(.data)
        *(.data.*)
    }

    /* ---------------------------------------------------------------------- */
    /* .bss — Uninitialised Data (Zero-Initialised by convention)              */
    /* ---------------------------------------------------------------------- */
    /*
     * Global/static variables without explicit initialisers, and our 16 KB
     * kernel stack (declared with 'resb' in boot.asm). The OS loader (GRUB)
     * is expected to zero this region. Because .bss contents are all zeros,
     * the section occupies NO space in the file on disk — only in memory.
     *
     * COMMON: un-initialised C globals that the compiler may emit as "common"
     * symbols (before they are placed into a final section) are collected here.
     */
    .bss ALIGN(4096) :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /*
     * Optional: export the end-of-kernel address as a symbol.
     * Useful later for a simple bump allocator: the first free page starts here.
     * Usage in C:  extern uint32_t _kernel_end;
     *              uint32_t first_free_page = (uint32_t)&_kernel_end;
     */
    _kernel_end = .;
}
