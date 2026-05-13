[bits 32]

global load_page_directory
load_page_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]    ; Get the pointer to the page directory passed as a parameter
    mov cr3, eax        ; Load it into the CR3 register
    mov esp, ebp
    pop ebp
    ret

global enable_paging
enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0        ; Read CR0
    or eax, 0x80000000  ; Set the Paging bit (bit 31)
    mov cr0, eax        ; Write back to CR0 to enable paging
    mov esp, ebp
    pop ebp
    ret
