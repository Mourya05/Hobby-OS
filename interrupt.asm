[bits 32]

extern idt_handler

; idt_load function
global idt_load
idt_load:
    mov eax, [esp + 4]      ; Get the pointer to the IDT passed as a parameter
    lidt [eax]              ; Load the IDT
    sti                     ; Re-enable interrupts
    ret

; ISR_NOERRCODE macro
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        push 0              ; Push a dummy error code
        push %1             ; Push the interrupt number
        jmp isr_common_stub
%endmacro

; Generate the stub for ISR 0 (Division by Zero)
ISR_NOERRCODE 0

; ISR_ERRCODE macro for interrupts that DO push an error code (like Page Faults)
%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        push %1             ; Push the interrupt number (Error code is already pushed by CPU)
        jmp isr_common_stub
%endmacro

; Page Fault
ISR_ERRCODE 14

; -----------------------------------------------------------------------------
; System Call Stub (int 0x80)
; -----------------------------------------------------------------------------
extern syscall_dispatcher
global isr128
isr128:
    push 0                  ; Dummy error code
    push 128                ; Interrupt number
    
    pusha                   ; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
    
    mov ax, ds              ; Save the data segment descriptor
    push eax                ; Push it to the stack
    
    mov ax, 0x10            ; Load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp                ; Pass a pointer to the stack (registers_t *)
    call syscall_dispatcher ; Call the C syscall dispatcher
    add esp, 4              ; Clean up the pushed esp
    
    pop eax                 ; Reload original data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                    ; Restore registers (EAX might have a return value if we updated it on the stack, but here we just pop it)
    add esp, 8              ; Clean up error code and interrupt number
    iret                    ; Return to user mode

; IRQ macro (Hardware interrupts do not push error codes)
%macro IRQ 2
    global irq%1
    irq%1:
        cli                 ; Disable interrupts
        push 0              ; Dummy error code
        push %2             ; Interrupt number (mapped IDT index)
        jmp isr_common_stub
%endmacro

; Timer IRQ (IRQ0 mapped to IDT 32)
IRQ 0, 32

; Common ISR stub
isr_common_stub:
    pusha                   ; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
    
    mov ax, ds              ; Save the data segment descriptor
    push eax                ; Push it to the stack
    
    mov ax, 0x10            ; Load the kernel data segment descriptor (0x10 is offset in GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp                ; Pass a pointer to the stack (registers_t *) to the C handler
    call idt_handler        ; Call our C handler
    add esp, 4              ; Clean up the pushed esp
    
    pop eax                 ; Reload the original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                    ; Pops edi, esi, ebp, esp, ebx, edx, ecx, eax
    add esp, 8              ; Cleans up the pushed error code and ISR number
    iret                    ; Return from interrupt (pops CS, EIP, EFLAGS, SS, ESP)

