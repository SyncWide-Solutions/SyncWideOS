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
    mov bl, 0x0F        ; set color to white
    int 0x10

    jmp .loop

.done:
    pop bx
    pop ax
    pop si    
    ret
    
clear_screen:
    mov ax, 0x0003      ; set video mode to 80x25 text mode
    int 0x10            ; call BIOS interrupt to set video mode

    mov ax, 0x0600      ; function to scroll up the entire screen
    mov bh, 0x07        ; attribute (light gray on black)
    mov cx, 0           ; upper left corner (row 0, column 0)
    mov dx, 0x184F      ; lower right corner (row 24, column 79)
    int 0x10            ; call BIOS interrupt to clear the screen

    ret

main:
    ; setup data segments
    mov ax, 0           ; can't set ds/es directly
    mov ds, ax
    mov es, ax
    
    ; setup stack
    mov ss, ax
    mov sp, 0x7C00      ; stack grows downwards from where we are loaded in memory

    ; clear the screen
    call clear_screen

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
