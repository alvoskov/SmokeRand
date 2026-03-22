;
; tf0duper32_nt32.asm  An implementation of tf0duper32 PRNG: a combined
; generator inspired by SuperDuper and designed by A.L. Voskov. It uses
; TF0 (Klimov-Shamir T-function) instead of LCG and xorrot instead of
; xorshift. It is written in 80386 assembly language for wasm and Windows NT.
; The CDECL calling convention is used for all functions.
;
;    obj->tf += obj->tf * obj->tf | 0x4005;
;    obj->xs ^= obj->xs << 1;
;    obj->xs ^= rotl32(obj->xs, 9) ^ rotl32(obj->xs, 27);
;    const uint32_t s = rotl32(obj->tf, 5) + obj->xs;
;    return s ^ (s >> 16);
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; The tf0duper32 generator uses the 8-byte state with the next layout:
; [tf; xs] where tf and xs are 32-bit words.
;
; (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
; alvoskov@gmail.com
; 
; This software is licensed under the MIT license.
;
.386
.model flat
include consts.inc

; Reference value for an internal self-test
out_ref equ 657F5782h

.code

;
; void *create(const GeneratorInfo *gi, const CallerAPI *intf)
; Creates and seeds the PRNG
;
create proc
    push ebp
    push esi
    mov  ebp, [esp + 16]           ; Get pointer to the CallerAPI structure
    push dword ptr 8               ; Allocate 8-byte struct
    call [ebp + malloc_ind]        ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax                  ; Save address of the PRNG state
    ; LCG + xorshift initialization
    call [ebp + get_seed64_ind]    ; Call intf->get_seed64 function
    mov  [esi], eax                ; x = seed (tf0, lower 32 bits)
    or   edx, 1                    ; To prevent zeroland in LFSR
    mov  [esi + 4], edx            ; y = seed (xorrot, lower 32 bits)
    ;
    mov  eax, esi                  ; Return the address
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

;
;
get_bits proc
    push ebp
    mov  ebp, [esp + 8]  ; Get pointer to the PRNG state
    ; xorrot (LFSR) part
    mov  ecx, [ebp + 4]
    mov  edx, ecx ; xs ^= xs << 1
    shl  edx, 1
    xor  ecx, edx
    mov  eax, ecx ; xs ^= rotl32(xs, 9) ^ rotl32(xs, 27)
    mov  edx, ecx
    rol  eax, 9
    ror  edx, 5
    xor  ecx, eax
    xor  ecx, edx
    mov  [ebp + 4], ecx
    ; TF0 part: tf += tf * tf | 0x4005;
    mov  edx, [ebp]
    mov  eax, edx
    imul eax, eax
    or   eax, 4005h
    add  eax, edx
    mov  [ebp], eax
    ; Output
    rol  eax, 5   ; u = rotl32(tf, 5) + xs
    add  eax, ecx
    mov  edx, eax ; u ^= u >> 16
    shr  edx, 16
    xor  eax, edx
    xor  edx, edx ; Output is edx:eax, but upper 32 bits are 0
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
    mov  ecx, 9999999
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
    xor  edx, edx ; Comparison result
    mov  eax, ebx ; Comparison result
    pop  ebx
    pop  ebp
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
    mov dword ptr [eax + 24], run_self_test
    mov dword ptr [eax + 28], 0 ; get_sum
    mov dword ptr [eax + 32], 0 ; parent
    mov eax, 1 ; Success
    ret
gen_getinfo endp

; Data section. We need it because PRNG state for an internal self-test
; should be mutable.
.data
    prng_name  db 'tf0duper32:asm', 0
    prng_descr db 'tf0duper32 implementation for 80386', 0
    prng_test_obj dd 123456789, 987654321
    printf_fmt db 'Output: %X, reference: %X', 13, 10, 0

end
