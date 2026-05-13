[bits 32]

global switch_to_user_mode
switch_to_user_mode:
    ; Set up for IRET
    ; IRET expects the stack to look like:
    ;   [SS]
    ;   [ESP]
    ;   [EFLAGS]
    ;   [CS]
    ;   [EIP]

    cli                 ; Disable interrupts before modifying the stack

    ; 1. Segment Selectors
    mov ax, 0x23        ; 0x23 is our User Data Segment (0x20) OR'd with RPL 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 2. Push SS
    push 0x23

    ; 3. Push ESP (We use the current stack for now, but usually it'd be a new user stack)
    mov eax, esp
    push eax

    ; 4. Push EFLAGS
    pushf
    pop eax
    or eax, 0x200       ; Set the IF (Interrupt Enable) flag
    push eax

    ; 5. Push CS
    push 0x1B           ; 0x1B is our User Code Segment (0x18) OR'd with RPL 3

    ; 6. Push EIP (Return Address)
    ; We push a dummy value and use a neat trick to jump right back here but in User Mode
    push .user_mode_start
    
    ; 7. IRET (Interrupt Return)
    iret

.user_mode_start:
    ; We are now running in Ring 3 (User Mode)!
    ret
