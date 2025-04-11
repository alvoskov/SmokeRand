;
; mwc3232x_nt32.asm  An implementation of MWC64X: a combined generator made
; of two MWC (multiply-with-carry) generators with 64-bit state each.
; It is written in 80386 assembly language for wasm and Windows NT. The CDECL
; calling convention is used for all functions.
;
; It is significantly faster than the C implementation compiled by Open Watcom.
;
; The MWC3232X generator uses the 16-byte state with the next layout:
; [x1; c1; x2; c2] where x and c are 32-bit words.
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
    push dword ptr 16    ; Allocate 16-byte struct
    call [ebp + malloc_ind]      ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax        ; Save address of the PRNG state
    call [ebp + get_seed64_ind]       ; Call intf->get_seed64 function
    mov  [esi], eax      ; x1 = seed (lower 32 bits)
    mov  [esi + 8], eax           ; x2 = seed (lower 32 bits)
    mov  dword ptr [esi +  4], 1  ; c1 = 1
    mov  dword ptr [esi + 12], 1  ; c2 = 1    
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


;    Mwc3232xShared *obj = state;
;    uint32_t z_lo = obj->z & 0xFFFFFFFF, z_hi = obj->z >> 32;
;    uint32_t w_lo = obj->w & 0xFFFFFFFF, w_hi = obj->w >> 32;
;    obj->z = 4294441395 * z_lo + z_hi; // 2^32 - 525901  => FFF7F9B3
;    obj->w = 4294440669 * w_lo + w_hi; // 2^32 - 526627  => FFF7F6DD  
;    return ((obj->z << 32) | (obj->z >> 32)) ^ obj->w;

;
; uint64_t get_bits(void *state)
; Generate one 32-bit unsigned integer.
;
; The used algorithm for [HI,LO] 64-bit values:
;
; [c1_new, x1_new] = A1 * x1 + c1    ; A0 = 2^32 - 525901 = 0xFFF7F9B3
; [c2_new, x2_new] = A1 * x2 + c2    ; A1 = 2^32 - 526627 = 0xFFF7F6DD
; return [x2_new ^ c1_new, x1_new ^ c2_new]
;
get_bits proc
    mov ecx, [esp + 4]  ; Get pointer to the PRNG state
    ; Generator 1
    mov eax, [ecx]      ; Load obj->x and multiply it
    mov edx, 0FFF7F9B3h ; by 2^32 - 525901
    mul edx
    add eax, [ecx + 4]  ; Get obj->c and add it to A*x
    adc edx, 0          ; (carry to the upper 32 bits)
    mov [ecx], eax      ; Save obj->x
    mov [ecx + 4], edx  ; Save obj->c
    ; Generator 2
    mov eax, [ecx + 4]  ; Load obj->x and multiply it
    mov edx, 0FFF7F6DDh ; by 2^32 - 526627
    mul edx
    add eax, [ecx + 12] ; Get obj->c and add it to A*x
    adc edx, 0          ; (carry to the upper 32 bits)
    mov [ecx + 8], eax  ; Save obj->x
    mov [ecx + 12], edx ; Save obj->c
    ; Output function
    xor eax, [ecx + 4]  ; x2 ^ c1
    xor edx, [ecx]      ; c2 ^ x1
    ret
get_bits endp


;
; int gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
; Returns the information about the generator.
;
gen_getinfo proc export
    mov eax, [esp + 4]
    mov dword ptr [eax],      offset prng_name
    mov dword ptr [eax + 4],  offset prng_descr
    mov dword ptr [eax + 8],  64
    mov dword ptr [eax + 12], create
    mov dword ptr [eax + 16], free
    mov dword ptr [eax + 20], get_bits
    mov dword ptr [eax + 24], 0 ; self_test
    mov dword ptr [eax + 28], 0 ; get_sum
    mov dword ptr [eax + 32], 0 ; parent
    mov eax, 1 ; success
    ret
gen_getinfo endp

; Some information about the generator. We don't need a separate section
; in PE file for it.
prng_name  db 'MWC3232X:asm', 0
prng_descr db 'MWC3232X implementation for 80386', 0

end
