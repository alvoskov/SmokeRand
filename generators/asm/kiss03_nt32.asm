;
; kiss03_nt32.asm  An implementation of KISS03: a classical combined PRNG
; designed by G. Marsaglia. It is written in 80386 assembly language
; for wasm and Windows NT. The CDECL calling convention is used for all
; functions.
;
;    // LCG part
;    x = 69069U * x + 12345U;
;    // xorshift part
;    y ^= y << 13;
;    y ^= y >> 17;
;    y ^= y << 5;
;    // MWC part
;    const uint64_t t = 698769069ULL * z + c;
;    c = (uint32_t) (t >> 32);
;    z = (uint32_t) t;
;    // Combined output
;    return x + y + z;
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; The KISS03 generator uses the 16-byte state with the next layout:
; [x; y; z; c] where x, y and z and c are 32-bit words.
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
out_ref equ 8E41D4F8h

.code

;
; void *create(const GeneratorInfo *gi, const CallerAPI *intf)
; Creates and seeds the PRNG
;
create proc
    push ebp
    push esi
    mov  ebp, [esp + 16]           ; Get pointer to the CallerAPI structure
    push dword ptr 16              ; Allocate 16-byte struct
    call [ebp + malloc_ind]        ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax                  ; Save address of the PRNG state
    ; LCG + xorshift initialization
    call [ebp + get_seed64_ind]    ; Call intf->get_seed64 function
    mov  [esi], eax                ; x = seed (lcg, lower 32 bits)
    or   eax, 1                    ; To prevent zeroland in xorshift
    mov  [esi + 4], edx            ; y = seed (xorshift, lower 32 bits)
    ; MWC initialization
    call [ebp + get_seed64_ind]    ; Call intf->get_seed64 function
    mov  dword ptr [esi + 8],  eax ; z (mwc, lower part)
    and  eax, 0FFFFFh              ; To prevent zeroland in MWC
    or   eax, 100000h
    mov  dword ptr [esi + 12], edx ; c (mwc, upper part)
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
    ; LCG part: x = 69069*x + 12345
    imul eax, [ebp], 69069
    add  eax, 12345
    mov  [ebp], eax
    mov  ecx, eax
    ; xorshift part
    mov  eax, [ebp + 4]
    mov  edx, eax ; y ^= y << 13
    shl  edx, 13
    xor  eax, edx
    mov  edx, eax ; y ^= y >> 17
    shr  edx, 17
    xor  eax, edx
    mov  edx, eax ; y ^= y << 5
    shl  edx, 5
    xor  eax, edx
    mov  [ebp + 4], eax
    add  ecx, eax
    ; MWC part: (c,x) = 698769069*x + c
    mov  edx, 698769069
    mov  eax, [ebp + 8]  ; z
    mul  edx
    add  eax, [ebp + 12] ; c
    adc  edx, 0
    mov  [ebp + 8],  eax
    mov  [ebp + 12], edx
    add  eax, ecx
    ; Output function
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
    prng_name  db 'kiss03:asm', 0
    prng_descr db 'KISS03 implementation for 80386', 0
    prng_test_obj dd 123456789, 987654321, 43219876, 6543217
    printf_fmt db 'Output: %X, reference: %X', 13, 10, 0

end