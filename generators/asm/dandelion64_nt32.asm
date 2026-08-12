;
; dandelion64_nt32.asm  An implementation dandelion64 32-bit PRNG
; in 80386 assembly language for wasm and Windows NT. The CDECL calling
; convention is used for all functions.
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; The dandelion64 generator uses the 8-byte state with the next layout:
; [x; y] where si are 32-bit words.
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
out_ref equ 7D8760E8h

.code

;
; void *create(const GeneratorInfo *gi, const CallerAPI *intf)
; Creates and seeds the PRNG
;
create proc
    push ebp
    push esi
    mov  ebp, [esp + 16]          ; Get pointer to the CallerAPI structure
    push dword ptr 8              ; Allocate 8-byte struct
    call [ebp + malloc_ind]       ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax                 ; Save address of the PRNG state
    call [ebp + get_seed64_ind]   ; Call intf->get_seed64 function
    mov  [esi], eax               ; x1 = seed (lower 32 bits)
    mov  [esi + 8], eax           ; x2 = seed (lower 32 bits)
    mov  eax, esi                 ; Return the address
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
; uint64_t get_bits(void *state)
; Generate one 32-bit unsigned integer.
;
; The used algorithm:
;
;    const uint32_t x = obj->x, y = obj->y;
;    obj->x = y ^ (x << 10);
;    obj->y = x ^ (uint32_t) ((int32_t) y >> 7);
;    const uint64_t xsq = (uint64_t) x * (uint64_t) x;
;    const uint32_t hi = (uint32_t) (xsq >> 32), lo = (uint32_t) xsq;
;    return (lo + y) ^ hi;
;
get_bits proc
    push ebp
    push ebx
    mov  ebp, [esp + 12]  ; Get pointer to the PRNG state
    ; Make result: res[eax] = ((x*x) % 2**32) + y) ^ ((x*x) // 2**32)`
    mov  eax, [ebp]       ; eax = x
    mov  ecx, [ebp + 4]   ; ecx = y
    mul  eax              ; edx:eax = x*x
    add  eax, ecx
    xor  eax, edx
    ; Update the LFSR state
    mov  edx, [ebp]       ; edx = x
    mov  ebx, edx         ; obj->x = y ^ (x << 10)
    shl  ebx, 10
    xor  ebx, ecx
    mov  [ebp], ebx
    sar  ecx, 7           ; obj->y = x ^ sar(y, 7)
    xor  ecx, edx
    mov  [ebp + 4], ecx
    ; Output function
    xor  edx, edx
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
    mov  ecx, 10000000
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
    prng_name  db 'dandelion64:asm', 0
    prng_descr db 'dandelion64 implementation for 80386', 0
    prng_test_obj dd 1h, 0h
    printf_fmt db 'Output: %X, reference: %X', 13, 10, 0

end
