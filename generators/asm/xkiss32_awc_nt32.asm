;
; xkiss16_awc_nt32.asm  An implementation of XKISS32/AWC: a combined 32-bit
; generator written in 80386 assembly language for wasm and Windows NT.
; The CDECL calling convention is used for all functions.
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; XKISS32/AWC state uses the next layout:
;
;    typedef struct {
;        uint32_t s[2];
;        uint32_t awc_x0;
;        uint32_t awc_x1;
;        uint32_t awc_c;
;    } Xkiss32AwcState;
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
    push dword ptr 20    ; Allocate 20-byte struct
    call [ebp + 12]      ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax        ; Save address of the PRNG state
    call [ebp + get_seed32_ind] ; Call intf->get_seed32 function
    mov  [esi], eax                       ; LFSR
    mov  dword ptr [esi + 4], 0DEADBEEFh   ;
    mov  dword ptr [esi + 8], 3  ; AWC
    mov  dword ptr [esi + 12], 2 ; AWC
    mov  dword ptr [esi + 16], 1 ; AWC
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
    push edi
    push esi
    push ebx
    mov  ebp, [esp + 20]  ; Get pointer to the PRNG state
    ; PRNG calls
    ; -- AWC part
    mov ebx, [ebp + 8]  ; obj->awc_x0
    mov edx, [ebp + 12] ; obj->awc_x1
    mov edi, [ebp + 16] ; obj->awc_c
    mov ecx, ebx        ; t = obj->awc_x0
    add ecx, edx        ;   + obj->awc_x1
    add ecx, edi        ;   + obj->awc_c
    mov [ebp + 12], ebx ; obj->awc_x1 = obj->awc_x0
    mov eax, ecx        ; obj->c = t >> 26
    shr eax, 26         ;
    mov [ebp + 16], eax ;
    mov eax, ecx        ; obj->awc_x0 = t & 0x3FFFFFF
    and eax, 3FFFFFFh   ;
    mov [ebp + 8],  eax ;
    shl eax, 6          ; out = ((obj->awc_x0 << 6) ^ obj->awc_x1);
    xor eax, ebx
    ; -- xorshift part
    mov ebx, [ebp]     ; s0 = obj->s[0]
    mov edx, [ebp + 4] ; s1 = obj->s[1]
    xor edx, ebx       ; s1 ^= s0    
    rol ebx, 26        ; rotl32(s0, 26)
    xor ebx, edx       ; rotl32(s0, 26) ^ s1
    mov edi, edx       ; (s1 << 9)
    shl edi, 9
    xor ebx, edi       ; rotl16(s0, 26) ^ s1 ^ (s1 << 5)
    rol edx, 13        ; rotl16(s1, 13)
    mov [ebp],     ebx ; obj->s[0] = s0
    mov [ebp + 4], edx ; obj->s[1] = s1
    add eax, ebx       ; out += obj->s[0]
    add eax, edx       ; out += obj->s[1]
    ; Output function
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
    prng_name  db 'XKISS32-AWC', 0
    prng_descr db 'XKISS32-AWC implementation for 80386', 0
    printf_fmt db 'Output: %X, reference: %X', 13, 10, 0
    prng_test_obj dd 8765, 4321, 3, 2, 1
    out_ref dd 59AA1AB5h

end
