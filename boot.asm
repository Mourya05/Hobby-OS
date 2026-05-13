; =============================================================================
; boot.asm - Multiboot-Compliant Kernel Entry Point
; =============================================================================
; This file is the very first code that runs after GRUB hands control to us.
; It satisfies the Multiboot Specification so GRUB can identify and load our
; kernel, sets up a small stack (required before calling C code), and then
; jumps into our C kernel_main() function.
;
; Assemble with: nasm -f elf32 boot.asm -o boot.o
; =============================================================================

; -----------------------------------------------------------------------------
; Section 1: Multiboot Header
; -----------------------------------------------------------------------------
; GRUB scans the first 8 KB of the kernel image looking for a Multiboot header.
; The header MUST be 4-byte aligned and contain three magic values.
; We place it in its own section so the linker script can guarantee it appears
; right at the start of the binary (within the first 8 KB).

section .multiboot
align 4                         ; The spec mandates 4-byte alignment

    ; Magic number — GRUB recognises this exact value (0x1BADB002)
    dd 0x1BADB002

    ; Flags — bit 0: align modules on page boundaries
    ;         bit 1: provide a memory map  (we set neither here for simplicity)
    dd 0x00000000

    ; Checksum — magic + flags + checksum must sum to zero (mod 2^32)
    ; Using twos-complement negation: -(magic + flags)
    dd -(0x1BADB002 + 0x00000000)

; -----------------------------------------------------------------------------
; Section 2: Stack Definition (BSS)
; -----------------------------------------------------------------------------
; C code requires a valid stack before it can execute.
; We reserve 16 KB (16,384 bytes) of uninitialised space in .bss.
; The label 'stack_top' points to the HIGH address because x86 stacks
; grow DOWNWARD (pushing decrements ESP).

section .bss
align 16                        ; ABI requires 16-byte stack alignment

stack_bottom:
    resb 16384                  ; Reserve 16 KB = 16 * 1024 bytes

stack_top:                      ; This label is the initial stack pointer

; -----------------------------------------------------------------------------
; Section 3: Kernel Entry Point (_start)
; -----------------------------------------------------------------------------
; _start is declared global so the linker can find it as the ELF entry point
; (specified via ENTRY(_start) in linker.ld).
; GRUB jumps here in 32-bit Protected Mode with interrupts disabled.

section .text
global _start                   ; Export _start to the linker
extern kernel_main              ; Import kernel_main() from kernel.c

_start:
    ; ── 1. Initialise the Stack ──────────────────────────────────────────────
    ; Load the address of stack_top into ESP (the Stack Pointer register).
    ; From this moment any PUSH/POP or function call will work correctly.
    mov esp, stack_top

    ; ── 2. (Optional) Save Multiboot Info ────────────────────────────────────
    ; GRUB places the Multiboot magic value in EAX and a pointer to the
    ; Multiboot Information structure in EBX before jumping here.
    ; If you need them later, push them onto the stack now so kernel_main
    ; can receive them as arguments:  kernel_main(uint32_t magic, void *mbi)
    push ebx                    ; Multiboot Info struct pointer (arg 2)
    push eax                    ; Multiboot magic number        (arg 1)

    ; ── 3. Call the C Kernel ─────────────────────────────────────────────────
    ; 'call' pushes the return address and jumps to kernel_main.
    call kernel_main

    ; ── 4. Halt — kernel_main should never return ────────────────────────────
    ; If it somehow does, disable interrupts and halt the CPU indefinitely.
    ; The 'cli' + 'hlt' loop is the canonical "kernel panic" idiom.
.hang:
    cli                         ; Clear Interrupt Flag — disable interrupts
    hlt                         ; Halt the CPU until the next interrupt
    jmp .hang                   ; If an NMI wakes it, loop back and halt again
