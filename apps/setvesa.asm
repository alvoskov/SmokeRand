;
; Switches video mode to 80x50 text mode (VGA). Useful
; for typing large amounts of text in the console.
;
; Compile by means of wasm:
;   wasm setvesa.asm
;   wcl setvesa.obj
;   exe2bin set80x50.exe
;
; References:
;
;   1. https://www.frolov-lib.ru/books/bsp/v21/ch7_2.htm
;   2. 
;
; Useful VESA text modes:
;   80x60:  0108h      132x25: 0109h     132x43: 010Ah
;   132x50: 010Bh      132x60: 010Ch
;
; (C) 2025 Alexey Voskov
; License: MIT (X11) License
;
.model tiny
.code

org 100h

start:
    ; Check if VESA is present
    mov ax, 4f00h
    mov bx, ds
    mov es, bx
    mov di, offset vesa_buf
    int 10h
    cmp al, 4fh
    jne no_vesa
    cmp ah, 0
    jne no_vesa
    ; Get video modes
    mov bx, word ptr [vesa_buf + 10h]
    mov es, bx
    mov si, word ptr [vesa_buf + 0eh]
    ; Scan video modes
    xor bx, bx ; For video mode
vscan_begin:
    mov ax, es:[si]  ; Get video mode
    cmp ax, 0ffffh   ; Check if we at the end of the list
    je  vscan_end
    add si, 2        ; Increment loop counter
    cmp ax, 0108h    ; Lower boundary of text modes
    jb  vscan_begin
    cmp ax, 010Ch    ; Upper boundary of test modes
    ja  vscan_begin
    
    mov bx, ax
    jmp vscan_begin
vscan_end:

    ; Check if some VESA text mode was found
    cmp bx, 0
    je  no_textmode
	; Set VESA text mode from BX
    push bx
    mov ax, 4f02h
    int 10h
    pop bx
    ; Print short info
    mov ah, 9
    mov si, offset info_msg_array
    sub bx, 0108h
    shl bx, 1
    mov dx, word ptr [si + bx]
    int 21h
    ; Return to DOS
    ret

no_vesa:
    mov ah, 9
    mov dx, offset no_vesa_msg
    int 21h
    jmp vga_fallback

no_textmode:
    mov ah, 9
    mov dx, offset no_textmode_msg
    int 21h

; ----- No VESA was detected: trying VGA 80x50 mode -----
vga_fallback:
	; Check if VGA adapter is present
	mov ax, 01A00h
	int 10h
	cmp al, 1Ah
	jne no_vga
	; Set ordinary text mode
	mov ax, 3
	int 10h
	; Set 8x8 font from ROM
	mov ax, 1112h
	xor bl, bl
	int 10h
	; Enable cursor
	mov ax, 0100h
	mov cx, 0607h
	int 10h
	; Print short info
	mov ah, 9
	mov dx, offset info_msg_vga
	int 21h
	; Return to DOS
	ret

no_vga:
	; Print error message
	mov ah, 9
	mov dx, offset no_vga_msg
	int 21h
	; Return to DOS
	ret

; ----- Data region -----
info_msg        db 'VESA text mode was set successfully$'
info_msg_80x60  db '80x60 text mode was set$'
info_msg_132x25 db '132x25 text mode was set$'
info_msg_132x43 db '132x43 text mode was set$'
info_msg_132x50 db '132x50 text mode was set$'
info_msg_132x60 db '132x60 text mode was set$'
info_msg_vga    db '80x50 text mode was set$'
no_vesa_msg     db 'No VESA interface was found$'
no_vga_msg      db 'No VGA adapter was detected$'
no_textmode_msg db 'No VESA text modes were found$'
info_msg_array  dw offset info_msg_80x60
                dw offset info_msg_132x25
                dw offset info_msg_132x43
                dw offset info_msg_132x50
                dw offset info_msg_132x60
; ----- Buffer for VESA information -----
vesa_buf        db 512 dup (0)

end start
