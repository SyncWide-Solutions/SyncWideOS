org 0x7C00
bits 16

%define ENDL 0x0D, 0x0A

%include "src/kernel/dhcp.asm"  ; Include the DHCP handler

start:
    jmp main

; Renamed puts to puts_main to avoid conflict
puts_main:
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
    
clear_screen:
    ; clear the screen
    mov ax, 0x0003      ; function to set video mode (text mode)
    int 0x10
    ret

main:
    call clear_screen    ; clear the screen

    call dhcp_handler     ; call the network handler

    ; setup data segments
    mov ax, 0           ; can't set ds/es directly
    mov ds, ax
    mov es, ax
    
    ; setup stack
    mov ss, ax
    mov sp, 0x7C00      ; stack grows downwards from where we are loaded in memory

    ; print hello world message
    mov si, msg_line
    call puts_main       ; Call the renamed puts function

    mov si, msg_info
    call puts_main       ; Call the renamed puts function

    mov si, msg_line
    call puts_main       ; Call the renamed puts function

    mov si, msg_empty
    call puts_main       ; Call the renamed puts function

    hlt

.halt:
    jmp .halt

msg_line: db '--------------------------------------', ENDL, 0
msg_info: db '| SyncWide OS | https://os.syncwi.de |', ENDL, 0
msg_empty: db '', ENDL, 0

times 510-($-$) db 0
dw 0AA55h