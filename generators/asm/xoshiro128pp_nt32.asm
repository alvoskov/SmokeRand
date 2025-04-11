;
; xoshiro128pp_nt32.asm  An implementation of xoshiro128+ 32-bit PRNG
; in 80386 assembly language for wasm and Windows NT. The CDECL calling
; convention is used for all functions.
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; The xoshiro128+ generator uses the 16-byte state with the next layout:
; [s0; s1; s2; s3] where si are 32-bit words.
;
; (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
; alvoskov@gmail.com
; 
; This software is licensed under the MIT license.
;
.386
.model flat
include consts.inc

; Reference value for an internal self-test
out_ref equ 1E354D68h

.code

;
; void *create(const GeneratorInfo *gi, const CallerAPI *intf)
; Creates and seeds the PRNG
;
create proc
    push ebp
    push esi
    mov  ebp, [esp + 16]          ; Get pointer to the CallerAPI structure
    push dword ptr 16             ; Allocate 16-byte struct
    call [ebp + malloc_ind]       ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax                 ; Save address of the PRNG state
    call [ebp + get_seed64_ind]   ; Call intf->get_seed64 function
    mov  [esi], eax               ; x1 = seed (lower 32 bits)
    mov  [esi + 8], eax           ; x2 = seed (lower 32 bits)
    mov  dword ptr [esi +  4], 1  ; c1 = 1
    mov  dword ptr [esi + 12], 1  ; c2 = 1    
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
; res = rotl32(s0 + s3, 7) + s0
; t = s1 << 9;
; s2 ^= s0
; s3 ^= s1
; s1 ^= s2
; s0 ^= s3
; s2 ^= t
; s3 = rotl32(s3, 11)
; return res
;
get_bits proc
    push ebp
    push edi
    push esi
    push ebx
    mov  ebp, [esp + 20]  ; Get pointer to the PRNG state
    ; Make result: res[eax] = rotl32(s0 + s3, 7) + s0
    mov  eax, [ebp]       ; eax = s0 (will be transformed to output)
    mov  edi, eax         ; edi = s0 (will be used further)
    mov  edx, [ebp + 12]  ; edx = s3
    mov  ebx, edx         ; working copy of s3
    add  ebx, eax         ; s0 + s3
    rol  ebx, 7           ; rol32 (s0 + s3, 7)
    add  eax, ebx         ; <-- the result in eax
    ; Update the LFSR state
    ; a) load into registers: edi,ebx,ecx,edx = s0,s1,s2,s3
    mov  ebx, [ebp + 4]   ; ebx = s1
    mov  esi, ebx         ; t(esi) = s1 << 9
    shl  esi, 9
    mov  ecx, [ebp + 8]   ; ecx = s2
    ; b) update state
    xor  ecx, edi         ; s2 ^= s0
    xor  edx, ebx         ; s3 ^= s1
    xor  ebx, ecx         ; s1 ^= s2
    xor  edi, edx         ; s0 ^= s3
    xor  ecx, esi         ; s2 ^= t
    rol  edx, 11          ; s3 = rotl32(s3, 11)
    ; c) Save to the memory
    mov  [ebp],      edi
    mov  [ebp + 4],  ebx
    mov  [ebp + 8],  ecx
    mov  [ebp + 12], edx
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
    mov dword ptr [eax + 24], run_self_test
    mov dword ptr [eax + 28], 0 ; get_sum
    mov dword ptr [eax + 32], 0 ; parent
    mov eax, 1 ; Success
    ret
gen_getinfo endp

; Data section. We need it because PRNG state for an internal self-test
; should be mutable.
.data
    prng_name  db 'xoshiro128++:asm', 0
    prng_descr db 'xoshiro128++ implementation for 80386', 0
    prng_test_obj dd 012345678h, 087654321h, 0DEADBEEFh, 0F00FC7C8h
    printf_fmt db 'Output: %X, reference: %X', 13, 10, 0

end
