; Switches video mode to 80x50 text mode (VGA). Useful
; for typing large amounts of text in the console.
;
; Compile by means of wasm:
;   wasm set80x50.asm
;   wcl set80x50.obj
;   exe2bin set80x50.exe
;
; (C) 2020 Alexey Voskov
; License: MIT (X11) License
.model tiny
.code

org 100h

start:
	; Check if VGA adapter is present
	mov ax, 01A00h
	int 10h
	cmp al, 1Ah
	jne novga
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
	mov dx, offset info_msg
	int 21h
	; Return to DOS
	ret

novga:
	; Print error message
	mov ah, 9
	mov dx, offset novga_msg
	int 21h
	; Return to DOS
	ret

info_msg  db '80x50 text video mode setter$'
novga_msg db '80x50 text video mode setter: no VGA detected$'

end start