# =============================================================================
# Makefile — Build System for the x86 Hobby OS Kernel
# =============================================================================
# HOW TO USE
#   make          → Build mykernel.bin  (default target = 'all')
#   make run      → Build and boot in QEMU
#   make clean    → Delete all generated files
#
# TOOLCHAIN REQUIREMENTS
#   nasm              → Assembler (https://nasm.us) — usually in PATH
#   i686-elf-gcc      → Cross-compiler targeting 32-bit x86 ELF
#   i686-elf-ld       → Cross-linker (ships with the cross-compiler)
#   qemu-system-i386  → Emulator for testing (optional)
#
# WHY a CROSS-COMPILER?
#   Your host GCC targets linux-x86_64 (or similar). If you compiled with it,
#   GCC would add host-specific startup code (crt0.o, libc) and emit 64-bit
#   instructions. i686-elf-gcc targets a bare 32-bit ELF ABI with no OS
#   assumptions, which is exactly what we need.
#
#   Build guide: https://wiki.osdev.org/GCC_Cross-Compiler
# =============================================================================

# ── Toolchain ────────────────────────────────────────────────────────────────
ASM           := nasm
CC            := i686-elf-gcc
LD            := i686-elf-ld

# ── Output Files ─────────────────────────────────────────────────────────────
KERNEL_BIN    := mykernel.bin      # Final flat/ELF kernel binary
BOOT_OBJ      := boot.o gdt_flush.o interrupt.o task.o paging_asm.o
KERNEL_OBJ    := kernel.o gdt.o idt.o io.o pic.o timer.o pmm.o paging.o syscall.o

# ── Linker Script ─────────────────────────────────────────────────────────────
LINKER_SCRIPT := linker.ld

# ── Assembler Flags ───────────────────────────────────────────────────────────
# -f elf32    : Output 32-bit ELF object file (compatible with i686-elf-ld)
ASMFLAGS      := -f elf32

# ── C Compiler Flags ─────────────────────────────────────────────────────────
# -m32                : Generate 32-bit x86 machine code
# -ffreestanding      : Do NOT assume the standard library exists; do not
#                       include start files; implies -fno-builtin
# -nostdlib           : Do NOT link against any standard libraries (libc, libgcc)
#                       Note: with i686-elf-gcc, -ffreestanding already covers
#                       most of this, but -nostdlib is added for clarity.
# -O2                 : Standard optimisation level — safe for kernel code
# -Wall -Wextra       : Enable all useful warnings so bugs surface early
# -std=c99            : Use C99 standard (no VLAs etc. from later standards)
# -fno-stack-protector: Stack canaries require libc (libssp); disable them
# -fno-pic            : No position-independent code — kernel has a fixed address
CFLAGS        := -m32             \
                 -ffreestanding   \
                 -nostdlib        \
                 -O2              \
                 -Wall -Wextra    \
                 -std=c99         \
                 -fno-stack-protector \
                 -fno-pic

# ── Linker Flags ─────────────────────────────────────────────────────────────
# -T linker.ld        : Use our custom linker script
# -m elf_i386         : Produce a 32-bit ELF output
# -nostdlib           : Do not add any standard startup or library objects
LDFLAGS       := -T $(LINKER_SCRIPT) \
                 -m elf_i386         \
                 -nostdlib

# ── Phony Targets ─────────────────────────────────────────────────────────────
# Declare targets that are NOT files so make doesn't confuse them with filenames
.PHONY: all run clean help

# =============================================================================
# Default Target: all
# Builds the final kernel binary.
# =============================================================================
all: $(KERNEL_BIN)

# =============================================================================
# Linking: Combine object files into the final binary
# =============================================================================
# Order matters: boot.o MUST come first so the Multiboot header lands at the
# start of the binary (within the first 8 KB as required by the spec).
$(KERNEL_BIN): $(BOOT_OBJ) $(KERNEL_OBJ) $(LINKER_SCRIPT)
	@echo "[LD]  Linking  -> $@"
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(KERNEL_OBJ)
	@echo "[OK]  Built    -> $@  (size: $$(wc -c < $@) bytes)"

# =============================================================================
# Assemble: .asm → .o
# =============================================================================
boot.o: boot.asm
	@echo "[ASM] Assembling $< -> $@"
	$(ASM) $(ASMFLAGS) $< -o $@

gdt_flush.o: gdt_flush.asm
	@echo "[ASM] Assembling $< -> $@"
	$(ASM) $(ASMFLAGS) $< -o $@

interrupt.o: interrupt.asm
	@echo "[ASM] Assembling $< -> $@"
	$(ASM) $(ASMFLAGS) $< -o $@

task.o: task.asm
	@echo "[ASM] Assembling $< -> $@"
	$(ASM) $(ASMFLAGS) $< -o $@

paging_asm.o: paging_asm.asm
	@echo "[ASM] Assembling $< -> $@"
	$(ASM) $(ASMFLAGS) $< -o $@

# =============================================================================
# Compile: .c → .o
# =============================================================================
kernel.o: kernel.c
	@echo "[CC]  Compiling  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

gdt.o: gdt.c
	@echo "[CC]  Compiling  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

idt.o: idt.c
	@echo "[CC]  Compiling  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

io.o: io.c
	@echo "[CC]  Compiling  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

pic.o: pic.c
	@echo "[CC]  Compiling  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

timer.o: timer.c
	@echo "[CC]  Compiling  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

pmm.o: pmm.c
	@echo "[CC]  Compiling  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

paging.o: paging.c
	@echo "[CC]  Compiling  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# =============================================================================
# Run: Launch the kernel in QEMU (no display window needed — serial console)
# =============================================================================
# -kernel mykernel.bin : Load our binary directly (QEMU has built-in Multiboot)
# -nographic           : Disable graphical window; redirect I/O to terminal
#                        (Press Ctrl+A then X to quit)
# NOTE: VGA memory at 0xB8000 is still active even without -nographic.
#       Remove -nographic to open a proper VGA display window.
run: $(KERNEL_BIN)
	@echo "[QEMU] Booting $(KERNEL_BIN) ..."
	qemu-system-i386 -kernel $(KERNEL_BIN)

# =============================================================================
# Run with VGA window explicitly (alias for clarity)
# =============================================================================
run-vga: $(KERNEL_BIN)
	@echo "[QEMU] Booting with VGA display ..."
	qemu-system-i386 -kernel $(KERNEL_BIN) -vga std

# =============================================================================
# Debug: Launch QEMU and wait for GDB to attach on port 1234
# Requires ELF output (remove --oformat binary from LDFLAGS for debugging)
# =============================================================================
debug: $(KERNEL_BIN)
	@echo "[QEMU] Debug mode — waiting for GDB on localhost:1234"
	qemu-system-i386 -kernel $(KERNEL_BIN) -s -S

# =============================================================================
# Clean: Remove all generated files
# =============================================================================
clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -f $(BOOT_OBJ) $(KERNEL_OBJ) $(KERNEL_BIN)
	@echo "[DONE]"

# =============================================================================
# Help: Print usage information
# =============================================================================
help:
	@echo "============================================="
	@echo "  x86 Hobby OS Kernel — Makefile Targets"
	@echo "============================================="
	@echo "  make          Build mykernel.bin"
	@echo "  make run      Build and run in QEMU"
	@echo "  make run-vga  Run with explicit VGA window"
	@echo "  make debug    Run with GDB remote stub"
	@echo "  make clean    Delete all build artifacts"
	@echo "  make help     Show this message"
	@echo "============================================="
