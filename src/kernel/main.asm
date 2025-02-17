; Just an old backup file of the kernel
org 0x7C00
bits 16


%define ENDL 0x0D, 0x0A


start:
    jmp main


;
; Prints a string to the screen
; Params:
;   - ds:si points to string
;
puts:
    ; save registers we will modify
    push si
    push ax
    push bx

.loop:
    lodsb               ; loads next character in al
    or al, al           ; verify if next character is null?
    jz .done

    mov ah, 0x0E        ; call bios interrupt
    mov bh, 0           ; set page number to 0
    int 0x10

    jmp .loop

.done:
    pop bx
    pop ax
    pop si    
    ret
    
; Clear screen function
clear_screen:
    push ax
    push bx
    push cx
    push dx

    mov ah, 0x00    ; Set video mode function
    mov al, 0x03    ; Text mode 80x25 16 colors
    int 0x10        ; BIOS video interrupt

    mov ah, 0x06    ; Scroll up function
    mov al, 0x00    ; Clear entire screen
    mov bh, 0x07    ; White on black attribute
    mov cx, 0x0000  ; Upper left corner (0,0)
    mov dx, 0x184F  ; Lower right corner (79,24)
    int 0x10        ; BIOS video interrupt

    mov ah, 0x02    ; Set cursor position
    mov bh, 0x00    ; Page 0
    mov dx, 0x0000  ; Row 0, Column 0
    int 0x10        ; BIOS video interrupt

    pop dx
    pop cx
    pop bx
    pop ax
    ret

main:

    call clear_screen

    ; setup data segments
    mov ax, 0           ; can't set ds/es directly
    mov ds, ax
    mov es, ax
    
    ; setup stack
    mov ss, ax
    mov sp, 0x7C00      ; stack grows downwards from where we are loaded in memory

    ; print hello world message
    mov si, msg_line
    call puts

    mov si, msg_info
    call puts

    mov si, msg_line
    call puts

    mov si, msg_empty
    call puts

    mov si, msg_version
    call puts
    
    hlt

.halt:
    jmp .halt



msg_line: db '--------------------------------------', ENDL, 0
msg_info: db '| SyncWide OS | https://os.syncwi.de |', ENDL, 0
msg_empty: db '', ENDL, 0
msg_version: db 'SyncWide OS version 0.1', ENDL, 0


times 510-($-$$) db 0
dw 0AA55h