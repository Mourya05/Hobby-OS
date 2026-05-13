# Hobby-OS: A Custom 32-bit x86 Monolithic Kernel

A from-scratch Operating System kernel written in **C** and **x86 Assembly**. This project demonstrates core systems programming concepts including hardware abstraction, memory protection, and privilege isolation.

## 🚀 Key Features & Milestones

The kernel has evolved from a simple bootloader into a protected, paged environment capable of executing isolated user-mode tasks.

* **Bootloading:** Multiboot compliant, compatible with GRUB and other modern bootloaders.
* **Memory Management:**
    * **PMM:** Implementation of a Bitmap-based Physical Memory Manager.
    * **Paging:** 32-bit Two-level paging with Identity Mapping for the kernel and VGA buffer.
* **CPU Initialization:**
    * **GDT:** Global Descriptor Table for code/data segment management.
    * **IDT:** Interrupt Descriptor Table handling CPU exceptions (e.g., Division by Zero) and hardware IRQs.
* **Hardware Interfacing:**
    * **PIC Remapping:** Redirected 8259 PIC hardware interrupts to avoid conflicts with CPU exceptions.
    * **PIT Timer:** System heartbeat running at 100Hz via the Programmable Interval Timer.
    * **VGA Driver:** Direct-to-video-memory text rendering for system logs.
* **Security & Tasking:**
    * **Ring 3 Support:** Successfully transitioning the CPU from Kernel Mode (Ring 0) to User Mode.
    * **System Calls:** Implementation of the `int 0x80` interface to allow sandboxed tasks to communicate with the kernel.

## 📂 Project Structure

```text
├── boot.asm          # Kernel entry point & Multiboot header
├── gdt.c/h           # Segment descriptors & GDT loading
├── idt.c/h           # Interrupt gate management & ISR handling
├── pmm.c/h           # Physical memory bitmap allocator
├── paging.c/h        # Page directory and table setup
├── syscall.c/h       # User-mode system call interface
├── timer.c/h         # PIT timer initialization and tick handling
├── io.c/h            # Low-level port I/O (inb/outb)
├── kernel.c          # Main kernel initialization logic
├── linker.ld         # Memory layout definition
└── Makefile          # Build system for the i686-elf toolchain
```

## 🛠️ Toolchain
This project uses an i686-elf cross-compiler to ensure the binary is independent of the host operating system's libraries.

* **Compiler:** i686-elf-gcc
* **Assembler:** nasm
* **Linker:** i686-elf-ld
* **Emulator:** qemu-system-i386

## 🔨 Building and Running
**1. Clean previous builds**
```bash
make clean
```
**2. Compile the kernel**
```bash
make
```
**3. Run in QEMU**
```bash
qemu-system-i386 -kernel mykernel.bin -display curses
```
(To exit QEMU in curses mode: Press Alt + 2, type quit, and press Enter; or use Ctrl + C in the terminal)

## 🔬 Technical Implementation Details
**The Privilege Switch**
To enable user-mode execution, the kernel utilizes a Task State Segment (TSS). The TSS stores the kernel stack pointer (esp0), which the CPU uses to automatically switch from the User Stack back to a secure Kernel Stack whenever an interrupt or system call is triggered.

**Virtual Memory Transition**
Paging is initialized by identity-mapping the first 4MB of physical memory. This is critical for the "jump" into virtual mode, as it ensures that the instruction pointer (EIP) and the VGA buffer (0xB8000) remain valid immediately after the PG bit in the CR0 register is enabled.

**Author:** Mourya Birru

Developed as a deep-dive into Computer Architecture and OS Design.
