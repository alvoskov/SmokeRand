;
; mwc64x_nt32.asm  An implementation of MWC64X: a simple multiply-with-carry
; generator with a simple output scrambler. It is written in 80386 assembly
; language for wasm and Windows NT. The CDECL calling convention is used for
; all functions.
;
; It is significantly faster than C implementation compiled by Open Watcom.
;
; The MWC64X generator uses the 8-byte state with the next layout:
; [x; c] where x and c are 32-bit words.
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
    push dword ptr 8     ; Allocate 8-byte struct
    call [ebp + 12]      ; using the intf->malloc function
    add  esp, 4
    mov  esi, eax        ; Save address of the PRNG state
    call [ebp + get_seed32_ind] ; Call intf->get_seed32 function
    mov  [esi], eax      ; x = seed
    mov  dword ptr [esi + 4], 1    ; c = 1
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


;    const uint64_t A0 = 
;    MWC64XState *obj = state;
;    uint32_t c = obj->data >> 32;
;    uint32_t x = obj->data & 0xFFFFFFFF;
;    obj->data = A0 * x + c;
;    return x ^ c;

;
; uint64_t get_bits(void *state)
; Generate one 32-bit unsigned integer.
;
; The used algorithm:
; [c_new,x_new] = A0 * x + c ;A0 = 0xff676488 (2^32 - 10001272)
; return x ^ c
;
get_bits proc
    mov ecx, [esp + 4]  ; Get pointer to the PRNG state
    mov eax, [ecx]      ; Load obj->x and multiply it
    mov edx, 0ff676488h ; by 2^32 - 10001272
    mul edx
    add eax, [ecx + 4]  ; Get obj->c and add it to A*x
    adc edx, 0          ; (carry to the upper 32 bits)
    mov [ecx], eax      ; Save obj->x
    mov [ecx + 4], edx  ; Save obj->c
    xor eax, edx        ; return x ^ c
    xor edx, edx        ; Output is 64-bit, set upper 32 bits to 0
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
    mov dword ptr [eax + 8],  32
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
prng_name  db 'MWC64X', 0
prng_descr db 'MWC64X implementation for 80386', 0

end
