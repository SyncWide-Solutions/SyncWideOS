org 0x7C00
bits 16

%define ENDL 0x0D, 0x0A
%define NETWORK_PORT 0x300
%define DHCP_SERVER_PORT 67
%define DHCP_CLIENT_PORT 68

; DHCP Message Types
%define DHCP_DISCOVER 1
%define DHCP_OFFER 2
%define DHCP_REQUEST 3
%define DHCP_ACK 5

; Network interface structure
struc NetIntf
    .ethAddr resb 6     ; Ethernet address
    .ipAddr resb 4      ; IP address
endstruc

puts:
    ; save registers we will modify
    push si
    push ax
    push bx

; Global network interface
global network_interface
network_interface:
    istruc NetIntf
        at NetIntf.ethAddr, db 0x52, 0x54, 0x00, 0x12, 0x34, 0x56
        at NetIntf.ipAddr, db 0,0,0,0
    iend

; DHCP Discover Packet Template
dhcp_discover_packet:
    ; Implement basic DHCP discover packet structure
    db 0x01           ; Message type: BOOTREQUEST
    db 0x01           ; Hardware type: Ethernet
    db 0x06           ; Hardware address length
    db 0x00           ; Hops
    dd 0x1234         ; Transaction ID
    dw 0x0000         ; Seconds elapsed
    dw 0x8000         ; Broadcast flag
    times 4 db 0x00   ; Client IP address
    times 4 db 0x00   ; Your IP address
    times 4 db 0x00   ; Server IP address
    times 4 db 0x00   ; Gateway IP address
    times 6 db 0x52   ; Client MAC address
    times 10 db 0x00  ; Padding
    times 64 db 0x00  ; Server name
    times 128 db 0x00 ; Boot filename
    dd 0x63825363     ; Magic cookie
    db 53, 1, DHCP_DISCOVER  ; DHCP Message Type
    db 255            ; End of options

dhcp_discover_packet_end:

; DHCP Process Function
dhcp_process:
    call init_network

    ; Send DHCP discover packet
    mov si, dhcp_discover_packet
    mov cx, dhcp_discover_packet_end - dhcp_discover_packet
    mov dx, NETWORK_PORT

.send_packet:
    lodsb
    out dx, al
    loop .send_packet

    ; Wait and check for response
    mov cx, 5000
.wait_response:
    dec cx
    jz .timeout

    ; TODO: Implement actual packet reception
    ; Check for DHCP offer

    jmp .done

.timeout:
    mov si, msg_dhcp_timeout
    call puts
    jmp .done

.done:
    ret

; Network Initialization
init_network:
    mov dx, NETWORK_PORT
    mov al, 0x01
    out dx, al
    ret

; Messages
msg_dhcp_timeout: db 'DHCP Timeout', ENDL, 0
msg_dhcp_offer: db 'DHCP Offer Received', ENDL, 0

clear_screen:
    mov ax, 0x0003      ; set video mode to 80x25 text mode
    int 0x10            ; call BIOS interrupt to set video mode

    mov ax, 0x0600      ; function to scroll up the entire screen
    mov bh, 0x07        ; attribute (light gray on black)
    mov cx, 0           ; upper left corner (row 0, column 0)
    mov dx, 0x184F      ; lower right corner (row 24, column 79)
    int 0x10            ; call BIOS interrupt to clear the screen

    ret

; Main Routine
main:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    call clear_screen
    call dhcp_process

    mov si, success_msg   ; Success message for GitHub Actions
    call puts

    hlt                   ; or loop forever if needed

success_msg:
    db "OS BOOT SUCCESS", 0

.halt:
    jmp .halt

; Standard bootloader padding
times 510 - ($ - $$) db 0
dw 0xAA55
