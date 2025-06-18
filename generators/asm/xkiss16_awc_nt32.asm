;
; xkiss16_awc_nt32.asm  An implementation of XKISS16/AWC: a combined 16-bit
; generator written in 8086-friendly manner. It is written in 80386 assembly
; language for wasm and Windows NT. The CDECL calling convention is used for
; all functions.
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; XKISS16/AWC state uses the next layout:
;
;    typedef struct {
;        uint16_t weyl; ///< Discrete Weyl sequence state.
;        uint16_t s[2]; ///< xoroshiro32+ state.
;        uint16_t awc_x0; ///< AWC state, \f$ x_{n-1}) \f$.
;        uint16_t awc_x1; ///< AWC state, \f$ x_{n-2}) \f$.
;        uint16_t awc_c; ///< AWC state, carry.
;    } Xkiss16AwcState;
;
;
; (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
; alvoskov@gmail.com
; 
; This software is licensed under the MIT license.
;
.386
.model flat
include consts.inc

.code

;
; void *create(const GeneratorInfo *gi, const CallerAPI *intf)
; Creates and seeds the PRNG
;
create proc
    push ebp
    push esi
    mov  ebp, [esp + 16] ; Get pointer to the CallerAPI structure
    push dword ptr 12    ; Allocate 12-byte struct
    call [ebp + 12]      ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax        ; Save address of the PRNG state
    call [ebp + get_seed32_ind] ; Call intf->get_seed32 function
    mov  word ptr [esi], 0 ; Weyl sequence
    mov  [esi + 2], ax
    rol  eax, 16
    mov  [esi + 4], ax
    mov  eax, esi        ; Return the address
    pop  esi
    pop  ebp
    ret
create endp

;
; void free(void *state, const GeneratorInfo *gi, const CallerAPI *intf)
; Free the generator state.
;
free proc
    mov  eax, [esp + 4]   ; Get generator state
    push ebp
    mov  ebp, [esp + 16]  ; Get pointer to the CallerAPI structure
    push eax              ; Call intf->free function
    call [ebp + free_ind]
    add  esp, 4
    pop  ebp
    ret
free endp


get_bits16 proc
    ; AWC (add with carry) part
    xor ax, ax
    mov bx, [ebp + 6]  ; obj->awc_x0
    mov dx, [ebp + 8]  ; obj->awc_x1
    mov di, [ebp + 10] ; obj->awc_c
    mov cx, bx         ; t = obj->awc_x0 + obj->awc_x1 + obj->awc_c
    add cx, dx         ;   + obj->awc_x1
    adc ax, 0          ;     ...
    add cx, di         ;   + obj->awc_c
    adc ax, 0          ;     ...
    mov [ebp + 6], cx  ; obj->awc_x0 = t & 0xFFFF
    mov [ebp + 8], bx  ; obj->awc_x1 = obj->awc_x0
    mov [ebp + 10], ax ; obj->c = t >> 16
    mov ax, cx         ; out += obj->awc_x0
    ; Discrete Weyl sequence part
    mov dx, [ebp]
    add dx, 9E39h
    mov [ebp], dx
    add ax, dx ; out += obj->weyl
    ; xoroshiro32+ part
    mov bx, [ebp + 2] ; s0 = obj->s[0]
    mov dx, [ebp + 4] ; s1 = obj->s[1]
    xor dx, bx        ; s1 ^= s0    
    mov cl, 13
    rol bx, 13        ; rotl16(s0, 13)
    xor bx, dx        ; rotl16(s0, 13) ^ s1
    mov di, dx        ; (s1 << 5)
    mov cl, 5
    shl di, cl
    xor bx, di        ; rotl16(s0, 13) ^ s1 ^ (s1 << 5)
    mov cl, 10
    rol dx, cl        ; rotl16(s1, 10)
    mov [ebp + 2], bx
    mov [ebp + 4], dx
    add ax, bx        ; out += obj->s[0]
    add ax, dx        ; out += obj->s[1]
    ret
get_bits16 endp


; uint64_t get_bits(void *state)
; Generate one 32-bit unsigned integer.
;
; The used algorithm consists of xoroshiro32+, AWC (add with carry)
; and discrete Weyl sequence parts.
;
get_bits proc
    ; Save the state
    push ebp
    push edi
    push esi
    push ebx
    mov  ebp, [esp + 20]  ; Get pointer to the PRNG state
    ; PRNG calls
    xor esi, esi
    call get_bits16
    mov si, ax
    shl esi, 16
    call get_bits16
    mov si, ax
    ; Output function
    mov  eax, esi
    xor  edx, edx
    pop  ebx
    pop  esi
    pop  edi
    pop  ebp
    ret
get_bits endp


;
; int run_self_test(const CallerAPI *intf)
; Runs an internal self-test.
;
run_self_test proc
    push ebp
    push ebx
    mov  ebp, [esp + 12] ; Pointer to CallerAPI struct
    ; Generate reference value
    mov  ecx, 10000
loop_gen_ref:
    push ecx
    push offset prng_test_obj
    call get_bits
    add  esp, 4
    pop  ecx
    loop loop_gen_ref
    ; Compare the result
    xor  ebx, ebx
    cmp  eax, out_ref
    sete bl
    ; Print the result
    push dword ptr out_ref
    push eax
    push offset printf_fmt
    call [ebp + printf_ind]
    add  esp, 12    
    mov  eax, ebx ; Comparison result
    pop  ebp
    pop  ebx
    ret
run_self_test endp


;
; int gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
; Returns the information about the generator.
;
gen_getinfo proc export
    mov eax, [esp + 4]
    mov dword ptr [eax],      offset prng_name
    mov dword ptr [eax + 4],  offset prng_descr
    mov dword ptr [eax + 8],  32
    mov dword ptr [eax + 12], create
    mov dword ptr [eax + 16], free
    mov dword ptr [eax + 20], get_bits
    mov dword ptr [eax + 24], run_self_test ; self_test
    mov dword ptr [eax + 28], 0 ; get_sum
    mov dword ptr [eax + 32], 0 ; parent
    mov eax, 1 ; success
    ret
gen_getinfo endp

; Data section. We need it because PRNG state for an internal self-test
; should be mutable.
.data
    prng_name  db 'XKISS16_AWC', 0
    prng_descr db 'XKISS16_AWC implementation for 80386', 0
    printf_fmt db 'Output: %X, reference: %X', 13, 10, 0
    prng_test_obj dw 1234, 8765, 4321, 3, 2, 1
    out_ref dd 0BC84B06Eh

end
