/* Keyboard driver for SyncWideOS */
/* Handles keyboard input and provides an interface for the kernel */

.section .data
keyboard_buffer:
    .zero 16                  /* Circular buffer for keyboard input */
buffer_head:
    .byte 0                   /* Head index of the buffer */
buffer_tail:
    .byte 0                   /* Tail index of the buffer */
shift_pressed:
    .byte 0                   /* Flag to track if shift is pressed */

.section .text
.global keyboard_init
.global keyboard_get_key
.global keyboard_handler

/* Keyboard scan code to ASCII mapping (US layout) */
keymap:
    .byte 0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8   /* 0-14 (8 is backspace) */
    .byte 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 10     /* 15-28 (10 is enter/return) */
    .byte 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`'        /* 29-41 (39 is single quote) */
    .byte 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*'     /* 42-55 */
    .byte 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                   /* 56-72 */
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                        /* 73-88 */

/* Shifted keymap for uppercase and symbols */
keymap_shifted:
    .byte 0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8   /* 0-14 */
    .byte 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 10     /* 15-28 */
    .byte 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~'         /* 29-41 */
    .byte 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*'      /* 42-55 */
    .byte 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                   /* 56-72 */
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                        /* 73-88 */

/* Initialize the keyboard */
keyboard_init:
    pushl %ebp
    movl %esp, %ebp
    
    /* Set up the keyboard interrupt handler (IRQ1) */
    /* This would typically involve setting up an IDT entry */
    /* For simplicity, we'll assume the IDT is already set up */
    /* and we just need to enable the keyboard IRQ */
    
    /* Enable keyboard IRQ (IRQ1) */
    inb $0x21, %al       /* Read PIC1 mask */
    andb $0xFD, %al      /* Clear bit 1 (IRQ1) */
    outb %al, $0x21      /* Write back to PIC1 */
    
    /* Reset buffer indices */
    movb $0, buffer_head
    movb $0, buffer_tail
    movb $0, shift_pressed
    
    movl %ebp, %esp
    popl %ebp
    ret

/* Keyboard interrupt handler */
/* Called when a key is pressed or released */
keyboard_handler:
    pushl %ebp
    movl %esp, %ebp
    pusha               /* Save all registers */
    
    /* Read scan code from keyboard port (0x60) */
    inb $0x60, %al
    
    /* Check if this is a key release (bit 7 set) */
    testb $0x80, %al
    jnz .key_release
    
    /* Handle key press */
    /* Check for special keys first */
    cmpb $0x2A, %al        /* Left shift */
    je .shift_down
    cmpb $0x36, %al        /* Right shift */
    je .shift_down
    
    /* Regular key, convert to ASCII */
    movzbl %al, %ebx
    cmpl $88, %ebx         /* Check if scan code is within our keymap */
    jae .done
    
    /* Check if shift is pressed to determine which keymap to use */
    cmpb $0, shift_pressed
    je .use_normal_keymap
    
    /* Use shifted keymap */
    movb keymap_shifted(%ebx), %al
    jmp .add_to_buffer
    
.use_normal_keymap:
    movb keymap(%ebx), %al
    
.add_to_buffer:
    /* Only add to buffer if it's a printable character */
    testb %al, %al
    jz .done
    
    /* Add to circular buffer */
    movzxb buffer_head, %ebx
    movb %al, keyboard_buffer(%ebx)
    
    /* Update head pointer */
    incb %bl
    andb $0x0F, %bl       /* Keep within buffer size (16 bytes) */
    movb %bl, buffer_head
    
    jmp .done
    
.shift_down:
    movb $1, shift_pressed
    jmp .done
    
.key_release:
    /* Handle key release */
    andb $0x7F, %al        /* Clear the release bit */
    
    /* Check if it's a shift key */
    cmpb $0x2A, %al        /* Left shift */
    je .shift_up
    cmpb $0x36, %al        /* Right shift */
    je .shift_up
    jmp .done
    
.shift_up:
    movb $0, shift_pressed
    
.done:
    /* Send EOI to PIC */
    movb $0x20, %al
    outb %al, $0x20
    
    popa                /* Restore all registers */
    movl %ebp, %esp
    popl %ebp
    iret                /* Return from interrupt */

/* Get a key from the keyboard buffer */
/* Returns: AL = ASCII character or 0 if no key is available */
keyboard_get_key:
    pushl %ebp
    movl %esp, %ebp
    
    /* Check if buffer is empty */
    movb buffer_head, %al
    cmpb buffer_tail, %al
    je .no_key
    
    /* Get key from buffer */
    movzxb buffer_tail, %ebx
    movb keyboard_buffer(%ebx), %al
    
    /* Update tail pointer */
    incb %bl
    andb $0x0F, %bl       /* Keep within buffer size (16 bytes) */
    movb %bl, buffer_tail
    
    jmp .key_done
    
.no_key:
    xorb %al, %al          /* Return 0 if no key is available */
    
.key_done:
    movl %ebp, %esp
    popl %ebp
    ret
