DEFAULT REL  ; Default to RIP-relative addressing instead of absolute.

extern _upb_decode_varint_fast64

SECTION .data

; Our dispatch table; used to jump to the right handler, keyed on the field's
; type.
dispatch_table:
  dq _upb_fastdecode.cant_fast_path  ; field not in table (type == 0).  (check_4).
  dq _upb_fastdecode.fixed64  ; double
  dq _upb_fastdecode.fixed32  ; float
  dq _upb_fastdecode.varint   ; int64
  dq _upb_fastdecode.varint   ; uint64
  dq _upb_fastdecode.varint   ; int32
  dq _upb_fastdecode.fixed64  ; fixed64
  dq _upb_fastdecode.fixed32  ; fixed32
  dq _upb_fastdecode.varint   ; bool
  dq _upb_fastdecode.cant_fast_path  ; string (TODO)
  dq _upb_fastdecode.cant_fast_path  ; group (check_6)
  dq _upb_fastdecode.cant_fast_path  ; message
  dq _upb_fastdecode.cant_fast_path  ; bytes (TODO)
  dq _upb_fastdecode.varint   ; uint32
  dq _upb_fastdecode.varint   ; enum
  dq _upb_fastdecode.fixed32  ; sfixed32
  dq _upb_fastdecode.fixed64  ; sfixed64
  dq _upb_fastdecode.varint_sint32 ; sint32
  dq _upb_fastdecode.varint_sint64 ; sint64

  GLOBAL _upb_decode_fast

SECTION .text
; Register allocation.
%define BUF rbx       ; const char *p, current buf position.
%define END rbp       ; const char *end, where the buf ends (either submsg end or buf end)
%define BUF_ADDR r12  ; upb_decoder *d.
%define FIELDDEF r13  ; upb_fielddef *f, needs to be preserved across varint decoding call.
%define CALLBACK r14
%define CLOSURE r15

; Stack layout: *tableptr, uint32_t maxfield_times_8
%define STACK_SPACE 24      ; this value + 8 must be a multiple of 16.
%define TABLE_SPILL [rsp]   ; our lookup table, indexed by field number.
%define MAXFIELD_TIMES_8_SPILL [rsp+8]


; Executing the fast path requires the following conditions:
; - check_1: there are >=12 bytes left (<=2 byte tag and <=10 byte varint).
; - check_2: the tag is <= 2 bytes.
; - check_3: the field number is <= the table size
;   (ie. it must be an array lookup, not a hash lookup).
; - check_4: the field is known (found in the table).
; - check_5: the wire type we read is correct for the field number,
;   ("packed" fields are not accepted, yet.  this could be handled
;    efficiently by doing an extra check on the "type check failed"
;    path that goes into a tight loop if the encoding was packed).
; - check_6: the field is not a group or a message (or string, TODO)
;   (this could be relaxed, but due to delegation it's a bit tricky).
; - if the value is a string, the entire string is available in
;   the buffer, and our cached string object can be recycled.


%macro decode_and_dispatch_ 0
align 16
.decode_and_dispatch:
  ; Load a few values we'll need in a sec.
  mov r8, TABLE_SPILL
  mov r9d, MAXFIELD_TIMES_8_SPILL

  mov rax, END
  sub rax, BUF
  cmp rax, 12
  jb _upb_fastdecode.cant_fast_path ; check_1 (<12 bytes left).

  ; Decode a 1 or 2-byte varint -> eax.
  mov cl, byte [BUF]
  lea rdi, [BUF+1]
  movzx rax, cl    ; Need all of rax since we're doing a 64-bit lea later.
  and eax, 0x7f
  test cl, cl
  jns .one_byte_tag ; Should be predictable if fields are in order.
  movzx ecx, byte [BUF+1]
  lea rdi, [BUF+2]
  mov edx, ecx
  and edx, 0x7f
  shl edx, 7
  or eax, edx
  test al, al
  js _upb_fastdecode.cant_fast_path ; check_2 (tag was >2 bytes).
.one_byte_tag:
  mov BUF, rdi

  ; Decode tag and dispatch.
  mov ecx, eax
  and eax, 0x3ff8 ; eax now contains field number * 8
  lea r11, [r8+rax*2]   ; *2 is really *16, since rax is already *8.
  and ecx, 0x7    ; ecx now contains wire type.
  cmp eax, r9d
  jae _upb_fastdecode.cant_fast_path  ; check_3 (field number > table size)
  mov FIELDDEF, [r11+8]     ; Lookup fielddef (upb_itof_ent.f)
  movzx rdx, BYTE [r11+1]   ; Lookup field type.
  mov rax, qword dispatch_table
  jmp [rax+rdx*8]
%endmacro

%macro decode_and_dispatch 0
  jmp .decode_and_dispatch
%endmacro

%macro call_callback 0
  ; Value arg must already be in rdx when macro is called.
  mov rdi, CLOSURE
  mov rsi, FIELDDEF
  mov rcx, 33      ; RAW; we could pass the correct type, or only do this in non-debug modes.
  call CALLBACK
  mov [BUF_ADDR], BUF
  cmp eax, 0
  jne .done    ; Caller requested BREAK or SKIPSUBMSG.
%endmacro

%macro check_type 1
  cmp ecx, %1
  jne _upb_fastdecode.cant_fast_path  ; check_5 (wire type check failed).
%endmacro

; extern upb_flow_t upb_fastdecode(const char **p, const char *end,
;                                  upb_value_handler_t value_cb, void *closure,
;                                  void *table, int table_size);
align 16
global _upb_fastdecode
_upb_fastdecode:
  ; We use all callee-save regs.
  push rbx
  push rbp
  push r12
  push r13
  push r14
  push r15
  sub rsp, STACK_SPACE

  ; Parse arguments into reg vals and stack.
  mov BUF_ADDR, rdi
  mov BUF, [rdi]
  mov END, rsi
  mov CALLBACK, rdx
  mov CLOSURE, rcx
  mov TABLE_SPILL, r8
  shl r9, 3
  mov MAXFIELD_TIMES_8_SPILL, r9

  decode_and_dispatch

align 16
.varint:
  call _upb_decode_varint_fast64  ; BUF is already in rdi.
  test rax, rax
  jz _upb_fastdecode.cant_fast_path  ; Varint was unterminated, slow path will handle error.
  mov BUF, rax
  call_callback      ; rdx already holds value.
  decode_and_dispatch_

align 16
.fixed32:
  mov edx, DWORD [BUF]  ; Might be unaligned, but that's ok.
  add BUF, 4
  call_callback
  decode_and_dispatch

align 16
.fixed64:
  mov rdx, QWORD [BUF]   ; Might be unaligned, but that's ok.
  add BUF, 8
  call_callback
  decode_and_dispatch

align 16
.varint_sint32:
  call _upb_decode_varint_fast64  ; BUF is already in rdi.
  test rax, rax
  jz _upb_fastdecode.cant_fast_path  ; Varint was unterminated, slow path will handle error.
  mov BUF, rax

  ; Perform 32-bit zig-zag decoding.
  mov ecx, edx
  shr edx, 1
  and ecx, 0x1
  neg ecx
  xor edx, ecx
  call_callback
  decode_and_dispatch

align 16
.varint_sint64:
  call _upb_decode_varint_fast64  ; BUF is already in rdi.
  test rax, rax
  jz _upb_fastdecode.cant_fast_path  ; Varint was unterminated, slow path will handle error.
  mov BUF, rax

  ; Perform 64-bit zig-zag decoding.
  mov rcx, rdx
  shr rdx, 1
  and ecx, 0x1
  neg rcx
  xor rdx, rcx
  call_callback
  decode_and_dispatch

.cant_fast_path:
  mov rax, 0   ; UPB_CONTINUE -- continue as before.
.done:
  ; If coming via done, preserve the user callback's return in rax.
  add rsp, STACK_SPACE
  pop r15
  pop r14
  pop r13
  pop r12
  pop rbp
  pop rbx
  ret
