;
; xkiss16_sh_awc_nt32.asm  An implementation of XKISS32/SHORT/AWC: a combined
; 32-bit generator written in 80386 assembly language for wasm and Windows NT.
; The CDECL calling convention is used for all functions.
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; XKISS32/SHORT/AWC state uses the next layout:
;
;    typedef struct {
;        uint32_t xs
;        uint32_t awc_x0;
;        uint32_t awc_x1;
;        uint32_t awc_c;
;    } Xkiss32ShAwcState;
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
    push dword ptr 16    ; Allocate 20-byte struct
    call [ebp + 12]      ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax        ; Save address of the PRNG state
    call [ebp + get_seed64_ind] ; Call intf->get_seed32 function
    or   eax, 1
    mov  [esi], eax                       ; LFSR
    and  edx, 3FFFFFFh
    mov  dword ptr [esi + 4], edx  ; awc_x0
    mov  dword ptr [esi + 8], 2    ; awc_x1
    mov  dword ptr [esi + 12], 1   ; awc_c
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


; uint64_t get_bits(void *state)
; Generate one 32-bit unsigned integer.
;
; The used algorithm consists of xoroshiro32+, AWC (add with carry)
; and discrete Weyl sequence parts.
;
get_bits proc
    ; Save the state
    push ebp
    push ebx
    push esi
    mov  ebp, [esp + 16]  ; Get pointer to the PRNG state
    ; PRNG calls
    ; -- AWC part
    ; t = obj->awc_x0 + obj->awc_x1 + obj->awc_c
    ; obj->awc_x1 = obj->awc_x0
    mov ecx, [ebp + 4]  ; obj->awc_x0
    mov eax, ecx        ; Save as future awc_x1
    add ecx, [ebp + 8]  ; + obj->awc_x1
    mov [ebp + 8], eax  ; obj->awc_x1 = obj->awc_x0
    add ecx, [ebp + 12] ; + obj->awc_c
    ; obj->c = t >> 26
    mov edx, ecx
    shr edx, 26
    mov [ebp + 12], edx
    ; obj->awc_x0 = t & 0x3FFFFFF
    mov edx, ecx
    and edx, 3FFFFFFh
    mov [ebp + 4], edx
    ; Output function
    ; out = ((obj->awc_x0 << 6) ^ obj->awc_x1);
    shl edx, 6
    xor eax, edx
    ; -- xorshift part
    mov ebx, [ebp]     ; xs = obj->xs
    mov esi, ebx       ; xs ^= (xs << 13)
    shl esi, 13
    xor ebx, esi
    mov esi, ebx       ; xs ^= (xs >> 17)
    shr esi, 17
    xor ebx, esi
    mov esi, ebx       ; xs ^= (xs << 5)
    shl esi, 5
    xor ebx, esi
    mov [ebp], ebx     ; xs = obj->xs
    add eax, ebx       ; out += obj->s[0]
    ; Output function
    xor  edx, edx
    pop  esi
    pop  ebx
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
    prng_name  db 'XKISS32/SHORT/AWC', 0
    prng_descr db 'XKISS32/SHORT/AWC implementation for 80386', 0
    printf_fmt db 'Output: %X, reference: %X', 13, 10, 0
    prng_test_obj dd 8765, 3, 2, 1
    out_ref dd 9D5B6B8h

end
