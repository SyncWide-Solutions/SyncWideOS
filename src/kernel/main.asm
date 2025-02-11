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

clear_screen:
    ; clear the screen
    mov ax, 0x0003      ; function to set video mode (text mode)
    int 0x10
    ret

; Sound routine - Add this after your existing procedures
play_sound:
    ; save registers
    push ax
    push bx
    push cx
    
    mov al, 182         ; Prepare the speaker for the note
    out 43h, al         
    mov ax, 2280        ; Frequency number (higher = lower sound)
    out 42h, al         ; Output low byte
    mov al, ah          ; Output high byte
    out 42h, al 
    in al, 61h         ; Turn on note (get value from port 61h)
    or al, 00000011b   ; Set bits 1 and 0
    out 61h, al        ; Send new value
    
    ; Add delay for sound duration
    mov cx, 0xFFFF
    
.delay:
    loop .delay
    
    ; Turn off speaker
    in al, 61h         ; Turn off note (get value from port 61h)
    and al, 11111100b  ; Reset bits 1 and 0
    out 61h, al        ; Send new value
    
    ; restore registers
    pop cx
    pop bx
    pop ax
    ret

main:

    call clear_screen

    call play_sound

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