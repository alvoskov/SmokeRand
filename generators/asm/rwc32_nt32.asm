;
; rwc32_nt32.asm  An implementation of RWC32: a simple multiply-with-carry
; generator (called also RWC or recursion with carry) It is written in 80386
; assembly language for wasm and Windows NT. The CDECL calling convention
; is used for all functions.
;
; u_n = 1111111464*(x_{n-3} + x_{n-2}) + c_{n-1}
; x_n = u_n mod 2^32
; c_n = u_n div 2^32
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; The RWC32 generator uses the 16-byte state with the next layout:
; [x; y; z; c] where x, y and z and c are 32-bit words.
;
; (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
; alvoskov@gmail.com
; 
; This software is licensed under the MIT license.
;
.386
.model flat
include consts.inc

; Reference value for an internal self-test
out_ref equ 1EC5DE26h

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
    mov  [esi], eax               ; x = seed (lower 32 bits)
    mov  [esi + 8], eax           ; y = seed (lower 32 bits)
    mov  dword ptr [esi +  4], 1  ; z = 1
    mov  dword ptr [esi + 12], 1  ; c = 1    
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
; const uint64_t new = 1111111464ULL*((uint64_t)obj->y + obj->z) + obj->c;
; obj->z = obj->y;
; obj->y = obj->x;
; obj->x = (uint32_t) new;
; obj->c = (uint32_t) (new >> 32);
; return obj->x;
;
get_bits proc
    push ebp
    push ebx
    mov  ebp, [esp + 12]  ; Get pointer to the PRNG state
    ; Load data and make z=y, y=z update
    mov  ecx, [ebp]       ; ecx = x
    mov  ebx, [ebp + 4]   ; ebx = y
    mov  edx, [ebp + 8]   ; edx = z
    mov  [ebp + 4], ecx   ; y = x
    mov  [ebp + 8], ebx   ; z = y
    ; Put y + z into ecx:ebx
    xor  ecx, ecx         ; Buffer for upper 32 bits
    add  ebx, edx         ; eax += z
    adc  ecx, 0
    ; 1111111464ULL*(y + z)
    mov  eax, ecx         ; ecx:ebx = a * ecx:ebx: upper 32 bits
    mov  edx, 1111111464
    mul  edx
    mov  ecx, eax         
    mov  eax, ebx         ; ecx:ebx = a * ecx:ebx : lower 32 bits
    mov  edx, 1111111464
    mul  edx
    add  ecx, edx
    ; Add carry
    add  eax, [ebp + 12]
    adc  ecx, 0
    ; Update x and carry
    mov  [ebp],      eax ; x = eax (lower 32 bits)
    mov  [ebp + 12], ecx ; c = ecx (higher 32 bits)
    ; Output function
    xor  edx, edx ; Output is edx:eax, but upper 32 bits are 0
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
    mov  ecx, 10000001
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
    prng_name  db 'rwc32:asm', 0
    prng_descr db 'RWC32 implementation for 80386', 0
    prng_test_obj dd 12345678, 87654321, 12345, 1
    printf_fmt db 'Output: %X, reference: %X', 13, 10, 0

end