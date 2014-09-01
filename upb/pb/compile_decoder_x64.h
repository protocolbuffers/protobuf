/*
** This file has been pre-processed with DynASM.
** http://luajit.org/dynasm.html
** DynASM version 1.3.0, DynASM x64 version 1.3.0
** DO NOT EDIT! The original file is in "upb/pb/compile_decoder_x64.dasc".
*/

#if DASM_VERSION != 10300
#error "Version mismatch between DynASM and included encoding engine"
#endif

# 1 "upb/pb/compile_decoder_x64.dasc"
//|//
//|// upb - a minimalist implementation of protocol buffers.
//|//
//|// Copyright (c) 2011-2013 Google Inc.  See LICENSE for details.
//|// Author: Josh Haberman <jhaberman@gmail.com>
//|//
//|// JIT compiler for upb_pbdecoder on x86-64.  Generates machine code from the
//|// bytecode generated in compile_decoder.c.
//|
//|.arch x64
//|.actionlist upb_jit_actionlist
static const unsigned char upb_jit_actionlist[1914] = {
  249,255,248,10,248,1,85,65,87,65,86,65,85,65,84,83,72,131,252,236,8,72,137,
  252,243,73,137,252,255,255,72,184,237,237,252,255,208,255,133,192,15,137,
  244,247,73,137,167,233,72,137,216,77,139,183,233,73,139,159,233,77,139,167,
  233,77,139,174,233,73,139,174,233,73,43,175,233,73,3,175,233,73,139,151,233,
  72,133,210,15,133,244,248,252,255,208,73,139,135,233,73,199,135,233,0,0,0,
  0,248,1,72,131,196,8,91,65,92,65,93,65,94,65,95,93,195,248,2,73,141,183,233,
  72,41,212,72,137,231,255,195,255,248,11,73,141,191,233,72,137,230,73,139,
  151,233,72,41,226,73,137,151,233,137,195,255,137,216,73,139,167,233,72,131,
  196,8,91,65,92,65,93,65,94,65,95,93,195,255,248,12,73,57,159,233,15,132,244,
  247,73,137,159,233,248,1,77,137,183,233,73,137,159,233,77,137,167,233,73,
  137,175,233,73,43,175,233,73,3,175,233,73,137,174,233,77,137,174,233,76,137,
  252,255,255,252,233,244,11,255,248,13,248,1,77,137,174,233,73,137,159,233,
  255,76,57,227,15,132,244,253,255,76,137,225,72,41,217,72,131,252,249,1,15,
  130,244,253,255,15,182,19,132,210,15,137,244,254,248,7,232,244,14,248,8,72,
  131,195,1,72,137,252,233,72,41,217,72,41,209,15,130,244,15,73,137,142,233,
  73,129,198,239,72,137,221,72,1,213,77,59,183,233,15,132,244,249,65,199,134,
  233,0,0,0,0,72,133,201,15,132,244,248,77,139,167,233,72,57,252,235,15,135,
  244,248,76,57,229,15,135,244,248,255,73,137,252,236,248,2,195,248,3,73,139,
  159,233,76,137,252,255,255,72,190,237,237,255,190,237,255,49,252,246,255,
  232,244,12,252,233,244,1,255,248,16,72,131,252,236,16,72,137,188,253,36,233,
  248,1,72,199,4,36,0,0,0,0,76,137,252,255,72,137,230,73,137,159,233,77,137,
  183,233,73,137,159,233,77,137,167,233,73,137,175,233,73,43,175,233,73,3,175,
  233,73,137,174,233,77,137,174,233,252,255,148,253,36,233,77,139,183,233,73,
  139,159,233,77,139,167,233,77,139,174,233,73,139,174,233,73,43,175,233,73,
  3,175,233,255,133,192,15,137,244,248,72,139,20,36,252,242,15,16,4,36,72,131,
  196,16,195,248,2,232,244,11,252,233,244,1,255,248,17,76,137,252,255,137,214,
  15,182,209,77,137,183,233,73,137,159,233,77,137,167,233,73,137,175,233,73,
  43,175,233,73,3,175,233,73,137,174,233,77,137,174,233,255,77,139,183,233,
  73,139,159,233,77,139,167,233,77,139,174,233,73,139,174,233,73,43,175,233,
  73,3,175,233,129,252,248,239,15,133,244,247,195,248,1,129,252,248,239,15,
  132,244,247,232,244,11,248,1,49,192,195,255,248,18,248,19,72,191,237,237,
  232,244,16,72,131,252,235,4,73,137,159,233,195,255,248,20,248,21,72,191,237,
  237,232,244,16,72,131,252,235,8,73,137,159,233,195,255,248,22,248,23,255,
  76,57,227,15,132,244,247,255,76,137,225,72,41,217,72,131,252,249,16,15,130,
  244,247,255,252,243,15,111,3,102,15,215,192,252,247,208,15,188,192,60,10,
  15,131,244,24,72,1,195,195,248,1,72,141,139,233,72,137,216,76,57,225,73,15,
  71,204,248,2,72,131,192,1,72,57,200,15,132,244,24,252,246,0,128,15,133,244,
  2,248,3,72,137,195,195,255,248,25,72,131,252,236,16,248,1,72,57,252,235,15,
  133,244,248,72,131,196,16,49,192,195,248,2,76,137,252,255,72,137,230,77,137,
  183,233,73,137,159,233,77,137,167,233,73,137,175,233,73,43,175,233,73,3,175,
  233,73,137,174,233,77,137,174,233,255,77,139,183,233,73,139,159,233,77,139,
  167,233,77,139,174,233,73,139,174,233,73,43,175,233,73,3,175,233,131,252,
  248,0,15,141,244,249,139,20,36,72,131,196,16,195,248,3,232,244,11,252,233,
  244,1,255,248,14,248,26,255,76,57,227,15,132,244,24,255,76,137,225,72,41,
  217,72,131,252,249,10,15,130,244,24,255,72,137,223,255,72,133,192,15,132,
  244,24,72,137,195,72,131,252,235,1,73,137,159,233,195,255,248,24,72,191,237,
  237,232,244,16,72,131,252,235,1,73,137,159,233,195,255,248,27,72,131,252,
  236,8,72,137,52,36,248,1,76,137,252,255,77,137,183,233,73,137,159,233,77,
  137,167,233,73,137,175,233,73,43,175,233,73,3,175,233,73,137,174,233,77,137,
  174,233,73,137,159,233,255,77,139,183,233,73,139,159,233,77,139,167,233,77,
  139,174,233,73,139,174,233,73,43,175,233,73,3,175,233,131,252,248,0,15,141,
  244,248,72,131,196,8,195,248,2,232,244,11,72,139,52,36,72,57,252,235,15,133,
  244,1,184,237,72,131,196,8,195,255,248,28,81,82,72,131,252,236,16,72,137,
  252,247,72,137,214,72,137,226,255,72,131,196,16,90,89,132,192,15,132,244,
  248,72,139,68,36,224,195,248,2,72,49,192,72,252,247,208,195,255,248,1,255,
  76,57,227,15,133,244,249,255,76,137,225,72,41,217,72,129,252,249,239,15,131,
  244,249,255,248,2,255,232,244,14,255,232,244,26,255,232,244,19,255,232,244,
  21,255,252,233,244,250,255,248,3,255,139,19,255,72,139,19,255,252,243,15,
  16,3,255,252,242,15,16,3,255,15,182,19,132,210,15,136,244,2,255,248,4,255,
  137,208,209,252,234,131,224,1,252,247,216,49,194,255,72,137,208,72,209,252,
  234,72,131,224,1,72,252,247,216,72,49,194,255,72,133,210,15,149,210,255,73,
  137,149,233,255,65,137,149,233,255,252,242,65,15,17,133,233,255,252,243,65,
  15,17,133,233,255,65,136,149,233,255,65,128,141,233,235,255,76,137,252,239,
  255,132,192,15,133,244,251,232,244,12,252,233,244,1,248,5,255,72,129,195,
  239,255,232,244,22,255,232,244,23,255,232,244,18,255,232,244,20,255,252,246,
  3,128,15,133,244,2,255,249,248,1,255,76,57,227,15,132,244,252,255,76,137,
  225,72,41,217,72,131,252,249,2,15,130,244,252,255,15,182,19,132,210,15,137,
  244,253,15,182,139,233,132,201,15,136,244,252,193,225,7,131,226,127,9,202,
  72,131,195,2,252,233,244,254,248,6,232,244,25,133,192,15,133,244,254,195,
  248,7,72,131,195,1,248,8,137,209,193,252,234,3,128,225,7,255,248,2,129,252,
  250,239,255,15,131,244,253,255,15,131,244,251,255,72,184,237,237,72,139,4,
  208,255,72,139,4,213,237,255,248,3,56,200,255,15,133,244,252,255,15,133,244,
  251,255,72,193,232,16,72,141,21,244,250,249,248,4,72,1,208,195,248,5,232,
  244,17,133,192,15,132,244,1,72,141,5,244,255,195,255,248,6,56,204,15,133,
  244,5,72,129,194,239,255,252,233,244,28,255,248,7,255,232,244,28,252,233,
  244,3,255,76,57,227,15,133,244,247,255,76,137,225,72,41,217,72,129,252,249,
  239,15,131,244,247,255,232,244,27,129,252,248,239,15,132,244,249,129,252,
  248,239,15,132,245,252,233,244,251,255,128,59,235,255,102,129,59,238,255,
  102,129,59,238,15,133,244,248,128,187,233,235,248,2,255,129,59,239,255,129,
  59,239,15,133,244,249,128,187,233,235,255,15,132,244,250,248,3,255,232,245,
  72,133,192,15,132,245,252,255,224,255,252,233,245,255,248,4,72,129,195,239,
  248,5,255,248,1,76,137,252,239,255,132,192,15,133,244,248,232,244,12,252,
  233,244,1,248,2,255,144,255,248,9,255,73,139,151,233,255,249,249,72,131,252,
  236,8,255,72,137,252,234,72,41,218,255,72,133,192,15,133,244,248,232,244,
  12,252,233,244,1,248,2,255,73,137,197,255,72,57,252,235,15,132,244,250,248,
  1,76,57,227,15,133,244,248,232,244,12,252,233,244,1,248,2,255,72,137,218,
  76,137,225,72,41,217,77,139,135,233,255,72,1,195,255,76,57,227,15,132,244,
  249,232,244,29,248,3,255,76,137,227,255,72,57,252,235,15,133,244,1,248,4,
  255,77,137,174,233,73,199,134,233,0,0,0,0,73,129,198,239,77,59,183,233,15,
  132,244,15,65,199,134,233,237,255,232,244,13,255,73,129,252,238,239,77,139,
  174,233,255,77,139,167,233,73,3,174,233,73,59,175,233,15,130,244,247,76,57,
  229,15,135,244,247,73,137,252,236,248,1,255,72,57,221,15,132,245,255,232,
  245,255,248,9,72,131,196,8,195,255
};

# 12 "upb/pb/compile_decoder_x64.dasc"
//|.globals UPB_JIT_GLOBAL_
enum {
  UPB_JIT_GLOBAL_enterjit,
  UPB_JIT_GLOBAL_exitjit,
  UPB_JIT_GLOBAL_suspend,
  UPB_JIT_GLOBAL_pushlendelim,
  UPB_JIT_GLOBAL_decodev32_fallback,
  UPB_JIT_GLOBAL_err,
  UPB_JIT_GLOBAL_getvalue_slow,
  UPB_JIT_GLOBAL_parse_unknown,
  UPB_JIT_GLOBAL_skipf32_fallback,
  UPB_JIT_GLOBAL_decodef32_fallback,
  UPB_JIT_GLOBAL_skipf64_fallback,
  UPB_JIT_GLOBAL_decodef64_fallback,
  UPB_JIT_GLOBAL_skipv32_fallback,
  UPB_JIT_GLOBAL_skipv64_fallback,
  UPB_JIT_GLOBAL_decode_varint_slow,
  UPB_JIT_GLOBAL_decode_unknown_tag_fallback,
  UPB_JIT_GLOBAL_decodev64_fallback,
  UPB_JIT_GLOBAL_checktag_fallback,
  UPB_JIT_GLOBAL_hashlookup,
  UPB_JIT_GLOBAL_strret_fallback,
  UPB_JIT_GLOBAL__MAX
};
# 13 "upb/pb/compile_decoder_x64.dasc"
//|.globalnames upb_jit_globalnames
static const char *const upb_jit_globalnames[] = {
  "enterjit",
  "exitjit",
  "suspend",
  "pushlendelim",
  "decodev32_fallback",
  "err",
  "getvalue_slow",
  "parse_unknown",
  "skipf32_fallback",
  "decodef32_fallback",
  "skipf64_fallback",
  "decodef64_fallback",
  "skipv32_fallback",
  "skipv64_fallback",
  "decode_varint_slow",
  "decode_unknown_tag_fallback",
  "decodev64_fallback",
  "checktag_fallback",
  "hashlookup",
  "strret_fallback",
  (const char *)0
};
# 14 "upb/pb/compile_decoder_x64.dasc"
//|
//|// Calling conventions.  Note -- this will need to be changed for
//|// Windows, which uses a different calling convention!
//|.define ARG1_64,   rdi
//|.define ARG2_8,    r6b  // DynASM's equivalent to "sil" -- low byte of esi.
//|.define ARG2_32,   esi
//|.define ARG2_64,   rsi
//|.define ARG3_8,    dl
//|.define ARG3_32,   edx
//|.define ARG3_64,   rdx
//|.define ARG4_64,   rcx
//|.define ARG5_64,   r8
//|.define XMMARG1,   xmm0
//|
//|// Register allocation / type map.
//|// ALL of the code in this file uses these register allocations.
//|// When we "call" within this file, we do not use regular calling
//|// conventions, but of course when calling to user callbacks we must.
//|.define PTR,       rbx                       // DECODER->ptr      (unsynced)
//|.define DATAEND,   r12                       // DECODER->data_end (unsynced)
//|.define CLOSURE,   r13                       // FRAME->closure    (unsynced)
//|.type   FRAME,     upb_pbdecoder_frame, r14  // DECODER->top      (unsynced)
#define Dt1(_V) (int)(ptrdiff_t)&(((upb_pbdecoder_frame *)0)_V)
# 36 "upb/pb/compile_decoder_x64.dasc"
//|.type   DECODER,   upb_pbdecoder, r15        // DECODER           (immutable)
#define Dt2(_V) (int)(ptrdiff_t)&(((upb_pbdecoder *)0)_V)
# 37 "upb/pb/compile_decoder_x64.dasc"
//|.define DELIMEND,  rbp
//|
//| // Spills unsynced registers back to memory.
//|.macro commit_regs
//|  mov  DECODER->top, FRAME
//|  mov  DECODER->ptr, PTR
//|  mov  DECODER->data_end, DATAEND
//|  // We don't guarantee that delim_end is NULL when out of range like the
//|  // interpreter does.
//|  mov  DECODER->delim_end, DELIMEND
//|  sub  DELIMEND, DECODER->buf
//|  add  DELIMEND, DECODER->bufstart_ofs
//|  mov  FRAME->end_ofs, DELIMEND
//|  mov  FRAME->sink.closure, CLOSURE
//|.endmacro
//|
//| // Loads unsynced registers from memory back into registers.
//|.macro load_regs
//|  mov  FRAME, DECODER->top
//|  mov  PTR, DECODER->ptr
//|  mov  DATAEND, DECODER->data_end
//|  mov  CLOSURE, FRAME->sink.closure
//|  mov  DELIMEND, FRAME->end_ofs
//|  sub  DELIMEND, DECODER->bufstart_ofs
//|  add  DELIMEND, DECODER->buf
//|.endmacro
//|
//| // OPT: use "call rel32" where possible.
//|.macro callp, addr
//|| {
//|| //int64_t ofs = (int64_t)addr - (int64_t)upb_status_init;
//|| //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
//|    mov64  rax, (uintptr_t)addr
//|    call   rax
//|| //} else {
//|  //  call   &addr
//|| //}
//|| }
//|.endmacro
//|
//|.macro ld64, val
//|| {
//|| uintptr_t v = (uintptr_t)val;
//|| if (v > 0xffffffff) {
//|    mov64  ARG2_64, v
//|| } else if (v) {
//|    mov    ARG2_32, v
//|| } else {
//|    xor    ARG2_32, ARG2_32
//|| }
//|| }
//|.endmacro
//|
//|.macro load_handler_data, h, arg
//|  ld64   upb_handlers_gethandlerdata(h, arg)
//|.endmacro
//|
//|.macro chkeob, bytes, target
//|| if (bytes == 1) {
//|  cmp    PTR, DATAEND
//|  je     target
//|| } else {
//|  mov    rcx, DATAEND
//|  sub    rcx, PTR
//|  cmp    rcx, bytes
//|  jb     target
//|| }
//|.endmacro
//|
//|.macro chkneob, bytes, target
//|| if (bytes == 1) {
//|  cmp    PTR, DATAEND
//|  jne    target
//|| } else {
//|  mov    rcx, DATAEND
//|  sub    rcx, PTR
//|  cmp    rcx, bytes
//|  jae    target
//|| }
//|.endmacro

//|.macro sethas, reg, hasbit
//|| if (hasbit >= 0) {
//|    or   byte [reg + ((uint32_t)hasbit / 8)], (1 << ((uint32_t)hasbit % 8))
//|| }
//|.endmacro
//|
//| // Decodes 32-bit varint into rdx, inlining 1 byte.
//|.macro dv32
//|  chkeob  1, >7
//|  movzx   edx, byte [PTR]
//|  test    dl, dl
//|  jns     >8
//|7:
//|  call    ->decodev32_fallback
//|8:
//|  add     PTR, 1
//|.endmacro

#define DECODE_EOF -3

static upb_func *gethandler(const upb_handlers *h, upb_selector_t sel) {
  return h ? upb_handlers_gethandler(h, sel) : NULL;
}

// Defines an "assembly label" for the current code generation offset.
// This label exists *purely* for debugging purposes: it is emitted into
// the .so, and printed as part of JIT debugging output when UPB_JIT_LOAD_SO is
// defined.
//
// We would define this in the .c file except that it conditionally defines a
// pclabel.
static void asmlabel(jitcompiler *jc, const char *fmt, ...) {
#ifndef NDEBUG
  int ofs = jc->dynasm->section->ofs;
  assert(ofs != jc->lastlabelofs);
  jc->lastlabelofs = ofs;
#endif

#ifndef UPB_JIT_LOAD_SO
  UPB_UNUSED(jc);
  UPB_UNUSED(fmt);
#else
  va_list args;
  va_start(args, fmt);
  char *str = upb_vasprintf(fmt, args);
  va_end(args);

  int pclabel = alloc_pclabel(jc);
  // Normally we would prefer to allocate this inline with the codegen,
  // ie.
  //   |=>asmlabel(...)
  // But since we do this conditionally, only when UPB_JIT_LOAD_SO is defined,
  // we do it here instead.
  //|=>pclabel:
  dasm_put(Dst, 0, pclabel);
# 172 "upb/pb/compile_decoder_x64.dasc"
  upb_inttable_insert(&jc->asmlabels, pclabel, upb_value_ptr(str));
#endif
}

// Should only be called when the associated handler is known to exist.
static bool alwaysok(const upb_handlers *h, upb_selector_t sel) {
  upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
  bool ok = upb_handlers_getattr(h, sel, &attr);
  UPB_ASSERT_VAR(ok, ok);
  bool ret = upb_handlerattr_alwaysok(&attr);
  upb_handlerattr_uninit(&attr);
  return ret;
}

// Emit static assembly routines; code that does not vary based on the message
// schema.  Since it's not input-dependent, we only need one single copy of it.
// For the moment we generate a single copy per generated handlers.  Eventually
// we should generate this code at compile time and link it into the binary so
// we have one copy total.  To do that we'll want to be sure that it is within
// 2GB of our JIT code, so that branches between the two are near (rel32).
//
// We'd put this assembly in a .s file directly, but DynASM's ability to
// calculate structure offsets automatically is too useful to pass up (it's way
// more convenient to write DECODER->sink than [rbx + 0x96], especially since
// the latter would have to be changed whenever the structure is updated).
static void emit_static_asm(jitcompiler *jc) {
  //| // Trampolines for entering/exiting the JIT.  These are a bit tricky to
  //| // support full resuming; when we suspend we copy the JIT's portion of
  //| // the call stack into the upb_pbdecoder and restore it when we resume.
  asmlabel(jc, "enterjit");
  //|->enterjit:
  //|1:
  //|  push  rbp
  //|  push  r15
  //|  push  r14
  //|  push  r13
  //|  push  r12
  //|  push  rbx
  //|
  //| // Align stack.
  //| // Since the JIT can call other functions (the JIT'ted code is not a leaf
  //| // function) we must respect alignment rules.  All x86-64 systems require
  //| // 16-byte stack alignment.
  //|  sub   rsp, 8
  //|
  //|  mov   rbx, ARG2_64  // Preserve JIT method.
  //|
  //|  mov   DECODER, rdi
  //|  callp upb_pbdecoder_resume  // Same args as us; reuse regs.
  dasm_put(Dst, 2);
   {
   //int64_t ofs = (int64_t)upb_pbdecoder_resume - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)upb_pbdecoder_resume), (unsigned int)(((uintptr_t)upb_pbdecoder_resume)>>32));
   //} else {
   //}
   }
# 221 "upb/pb/compile_decoder_x64.dasc"
  //|  test  eax, eax
  //|  jns   >1
  //|  mov   DECODER->saved_rsp, rsp
  //|  mov   rax, rbx
  //|  load_regs
  //|
  //|  // Test whether we have a saved stack to resume.
  //|  mov   ARG3_64, DECODER->call_len
  //|  test  ARG3_64, ARG3_64
  //|  jnz   >2
  //|
  //|  call  rax
  //|
  //|  mov   rax, DECODER->size_param
  //|  mov   qword DECODER->call_len, 0
  //|1:
  //|  add   rsp, 8  // Counter previous alignment.
  //|  pop   rbx
  //|  pop   r12
  //|  pop   r13
  //|  pop   r14
  //|  pop   r15
  //|  pop   rbp
  //|  ret
  //|
  //|2:
  //|  // Resume decoder.
  //|  lea   ARG2_64, DECODER->callstack
  //|  sub   rsp, ARG3_64
  //|  mov   ARG1_64, rsp
  //|  callp memcpy  // Restore stack.
  dasm_put(Dst, 38, Dt2(->saved_rsp), Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt1(->sink.closure), Dt1(->end_ofs), Dt2(->bufstart_ofs), Dt2(->buf), Dt2(->call_len), Dt2(->size_param), Dt2(->call_len), Dt2(->callstack));
   {
   //int64_t ofs = (int64_t)memcpy - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)memcpy), (unsigned int)(((uintptr_t)memcpy)>>32));
   //} else {
   //}
   }
# 252 "upb/pb/compile_decoder_x64.dasc"
  //|  ret  // Return to resumed function (not ->enterjit caller).
  //|
  //| // Other code can call this to suspend the JIT.
  //| // To the calling code, it will appear that the function returns when
  //| // the JIT resumes, and more buffer space will be available.
  //| // Args: eax=the value that decode() should return.
  dasm_put(Dst, 135);
# 258 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "exitjit");
  //|->exitjit:
  //|  // Save the stack into DECODER->callstack.
  //|  lea   ARG1_64, DECODER->callstack
  //|  mov   ARG2_64, rsp
  //|  mov   ARG3_64, DECODER->saved_rsp
  //|  sub   ARG3_64, rsp
  //|  mov   DECODER->call_len, ARG3_64  // Preserve len for next resume.
  //|  mov   ebx, eax  // Preserve return value across memcpy.
  //|  callp memcpy    // Copy stack into decoder.
  dasm_put(Dst, 137, Dt2(->callstack), Dt2(->saved_rsp), Dt2(->call_len));
   {
   //int64_t ofs = (int64_t)memcpy - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)memcpy), (unsigned int)(((uintptr_t)memcpy)>>32));
   //} else {
   //}
   }
# 268 "upb/pb/compile_decoder_x64.dasc"
  //|  mov   eax, ebx  // This will be our return value.
  //|
  //|  // Must NOT do this before the memcpy(), otherwise memcpy() will
  //|  // clobber the stack we are trying to save!
  //|  mov   rsp, DECODER->saved_rsp
  //|  add   rsp, 8  // Counter previous alignment.
  //|  pop   rbx
  //|  pop   r12
  //|  pop   r13
  //|  pop   r14
  //|  pop   r15
  //|  pop   rbp
  //|  ret
  //|
  //| // Like suspend() in the C decoder, except that the function appears
  //| // (from the caller's perspective) not to return until the decoder is
  //| // resumed.
  dasm_put(Dst, 160, Dt2(->saved_rsp));
# 285 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "suspend");
  //|->suspend:
  //|  cmp   DECODER->ptr, PTR
  //|  je    >1
  //|  mov   DECODER->checkpoint, PTR
  //|1:
  //|  commit_regs
  //|  mov   rdi, DECODER
  //|  callp upb_pbdecoder_suspend
  dasm_put(Dst, 182, Dt2(->ptr), Dt2(->checkpoint), Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt2(->delim_end), Dt2(->buf), Dt2(->bufstart_ofs), Dt1(->end_ofs), Dt1(->sink.closure));
   {
   //int64_t ofs = (int64_t)upb_pbdecoder_suspend - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)upb_pbdecoder_suspend), (unsigned int)(((uintptr_t)upb_pbdecoder_suspend)>>32));
   //} else {
   //}
   }
# 294 "upb/pb/compile_decoder_x64.dasc"
  //|  jmp   ->exitjit
  //|
  dasm_put(Dst, 235);
# 296 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "pushlendelim");
  //|->pushlendelim:
  //|1:
  //|  mov   FRAME->sink.closure, CLOSURE
  //|  mov   DECODER->checkpoint, PTR
  //|  dv32
  dasm_put(Dst, 240, Dt1(->sink.closure), Dt2(->checkpoint));
   if (1 == 1) {
  dasm_put(Dst, 253);
   } else {
  dasm_put(Dst, 261);
   }
# 302 "upb/pb/compile_decoder_x64.dasc"
  //|  mov   rcx, DELIMEND
  //|  sub   rcx, PTR
  //|  sub   rcx, rdx
  //|  jb    ->err  // Len is greater than enclosing message.
  //|  mov   FRAME->end_ofs, rcx
  //|  add   FRAME, sizeof(upb_pbdecoder_frame)
  //|  mov   DELIMEND, PTR
  //|  add   DELIMEND, rdx
  //|  cmp   FRAME, DECODER->limit
  //|  je    >3   // Stack overflow
  //|  mov   dword FRAME->groupnum, 0
  //|  test  rcx, rcx
  //|  jz    >2
  //|  mov   DATAEND, DECODER->end
  //|  cmp   PTR, DELIMEND
  //|  ja    >2
  //|  cmp   DELIMEND, DATAEND
  //|  ja    >2
  //|  mov   DATAEND, DELIMEND  // If DELIMEND >= PTR && DELIMEND < DATAEND
  dasm_put(Dst, 277, Dt1(->end_ofs), sizeof(upb_pbdecoder_frame), Dt2(->limit), Dt1(->groupnum), Dt2(->end));
# 321 "upb/pb/compile_decoder_x64.dasc"
  //|2:
  //|  ret
  //|3:
  //|  // Error -- call seterr.
  //|  mov   PTR, DECODER->checkpoint  // Rollback to before the delim len.
  //|  // Prepare seterr args.
  //|  mov   ARG1_64, DECODER
  //|  ld64  kPbDecoderStackOverflow
  dasm_put(Dst, 368, Dt2(->checkpoint));
   {
   uintptr_t v = (uintptr_t)kPbDecoderStackOverflow;
   if (v > 0xffffffff) {
  dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
   } else if (v) {
  dasm_put(Dst, 391, v);
   } else {
  dasm_put(Dst, 394);
   }
   }
# 329 "upb/pb/compile_decoder_x64.dasc"
  //|  callp upb_pbdecoder_seterr
   {
   //int64_t ofs = (int64_t)upb_pbdecoder_seterr - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)upb_pbdecoder_seterr), (unsigned int)(((uintptr_t)upb_pbdecoder_seterr)>>32));
   //} else {
   //}
   }
# 330 "upb/pb/compile_decoder_x64.dasc"
  //|  call  ->suspend
  //|  jmp   <1
  //|
  //| // For getting a value that spans a buffer seam.  Falls back to C.
  //| // Args: rdi=C decoding function (prototype: int f(upb_pbdecoder*, void*))
  dasm_put(Dst, 398);
# 335 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "getvalue_slow");
  //|->getvalue_slow:
  //|  sub   rsp, 16         // Stack is [8-byte value, 8-byte func pointer]
  //|  mov   [rsp + 8], rdi  // Need to preserve fptr across suspends.
  //|1:
  //|  mov   qword [rsp], 0   // For parsing routines that only parse 32 bits.
  //|  mov   ARG1_64, DECODER
  //|  mov   ARG2_64, rsp
  //|  mov   DECODER->checkpoint, PTR
  //|  commit_regs
  //|  call  aword [rsp + 8]
  //|  load_regs
  //|  test  eax, eax
  dasm_put(Dst, 406, 8, Dt2(->checkpoint), Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt2(->delim_end), Dt2(->buf), Dt2(->bufstart_ofs), Dt1(->end_ofs), Dt1(->sink.closure), 8, Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt1(->sink.closure), Dt1(->end_ofs), Dt2(->bufstart_ofs), Dt2(->buf));
# 348 "upb/pb/compile_decoder_x64.dasc"
  //|  jns   >2
  //|  // Success; return parsed data (in rdx AND xmm0).
  //|  mov   rdx, [rsp]
  //|  movsd xmm0, qword [rsp]
  //|  add   rsp, 16
  //|  ret
  //|2:
  //|  call  ->exitjit   // Return eax from decode function.
  //|  jmp   <1
  //|
  dasm_put(Dst, 507);
# 358 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "parse_unknown");
  //| // Args: edx=fieldnum, cl=wire type
  //|->parse_unknown:
  //|  // OPT: handle directly instead of kicking to C.
  //|  // Check for ENDGROUP.
  //|  mov     ARG1_64, DECODER
  //|  mov     ARG2_32, edx
  //|  movzx   ARG3_32, cl
  //|  commit_regs
  //|  callp   upb_pbdecoder_skipunknown
  dasm_put(Dst, 538, Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt2(->delim_end), Dt2(->buf), Dt2(->bufstart_ofs), Dt1(->end_ofs), Dt1(->sink.closure));
   {
   //int64_t ofs = (int64_t)upb_pbdecoder_skipunknown - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)upb_pbdecoder_skipunknown), (unsigned int)(((uintptr_t)upb_pbdecoder_skipunknown)>>32));
   //} else {
   //}
   }
# 368 "upb/pb/compile_decoder_x64.dasc"
  //|  load_regs
  //|  cmp     eax, DECODE_ENDGROUP
  //|  jne     >1
  //|  ret     // Return eax=DECODE_ENDGROUP, not zero
  //|1:
  //|  cmp     eax, DECODE_OK
  //|  je      >1
  //|  call    ->exitjit  // Return eax from decode function.
  //|1:
  //|  xor     eax, eax
  //|  ret
  //|
  //| // Fallback functions for parsing single values.  These are used when the
  //| // buffer doesn't contain enough remaining data for the fast path.  Each
  //| // primitive type (v32, v64, f32, f64) has two functions: decode & skip.
  //| // Decode functions return their value in rsi/esi.
  //| //
  //| // These functions leave PTR = value_end - fast_path_bytes, so that we can
  //| // re-join the fast path which will add fast_path_bytes after the callback
  //| // completes.  We also set DECODER->ptr to this value which is a signal to
  //| // ->suspend that DECODER->checkpoint is up to date.
  dasm_put(Dst, 582, Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt1(->sink.closure), Dt1(->end_ofs), Dt2(->bufstart_ofs), Dt2(->buf), DECODE_ENDGROUP, DECODE_OK);
# 389 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "skip_decode_f32_fallback");
  //|->skipf32_fallback:
  //|->decodef32_fallback:
  //|  mov64    rdi, (uintptr_t)upb_pbdecoder_decode_f32
  //|  call     ->getvalue_slow
  //|  sub      PTR, 4
  //|  mov      DECODER->ptr, PTR
  //|  ret
  //|
  dasm_put(Dst, 638, (unsigned int)((uintptr_t)upb_pbdecoder_decode_f32), (unsigned int)(((uintptr_t)upb_pbdecoder_decode_f32)>>32), Dt2(->ptr));
# 398 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "skip_decode_f64_fallback");
  //|->skipf64_fallback:
  //|->decodef64_fallback:
  //|  mov64    rdi, (uintptr_t)upb_pbdecoder_decode_f64
  //|  call     ->getvalue_slow
  //|  sub      PTR, 8
  //|  mov      DECODER->ptr, PTR
  //|  ret
  //|
  //| // Called for varint >= 1 byte.
  dasm_put(Dst, 660, (unsigned int)((uintptr_t)upb_pbdecoder_decode_f64), (unsigned int)(((uintptr_t)upb_pbdecoder_decode_f64)>>32), Dt2(->ptr));
# 408 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "skip_decode_v32_fallback");
  //|->skipv32_fallback:
  //|->skipv64_fallback:
  //|  chkeob   16, >1
  dasm_put(Dst, 682);
   if (16 == 1) {
  dasm_put(Dst, 687);
   } else {
  dasm_put(Dst, 695);
   }
# 412 "upb/pb/compile_decoder_x64.dasc"
  //|  // With at least 16 bytes left, we can do a branch-less SSE version.
  //|  movdqu   xmm0, [PTR]
  //|  pmovmskb eax, xmm0   // bits 0-15 are continuation bits, 16-31 are 0.
  //|  not      eax
  //|  bsf      eax, eax
  //|  cmp      al, 10
  //|  jae      ->decode_varint_slow  // Error (>10 byte varint).
  //|  add      PTR, rax    // bsf result is 0-based, so PTR=end-1, as desired.
  //|  ret
  //|
  //|1:
  //|  // With fewer than 16 bytes, we have to read byte by byte.
  //|  lea      rcx, [PTR + 10]
  //|  mov      rax, PTR    // Preserve PTR in case of fallback to slow path.
  //|  cmp      rcx, DATAEND
  //|  cmova    rcx, DATAEND    // rax = MIN(DATAEND, PTR + 10)
  //|2:
  //|  add      rax, 1
  //|  cmp      rax, rcx
  //|  je       ->decode_varint_slow
  //|  test     byte [rax], 0x80
  //|  jnz      <2
  //|3:
  //|  mov      PTR, rax  // PTR = varint_end - 1, as desired
  //|  ret
  //|
  //| // Returns tag in edx
  dasm_put(Dst, 711, 10);
# 439 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "decode_unknown_tag_fallback");
  //|->decode_unknown_tag_fallback:
  //|  sub   rsp, 16
  //|1:
  //|  cmp      PTR, DELIMEND
  //|  jne      >2
  //|  add      rsp, 16
  //|  xor      eax, eax
  //|  ret
  //|2:
  //|  // OPT: Have a medium-fast path before falling back to _slow.
  //|  mov   ARG1_64, DECODER
  //|  mov   ARG2_64, rsp
  //|  commit_regs
  //|  callp upb_pbdecoder_decode_varint_slow
  dasm_put(Dst, 780, Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt2(->delim_end), Dt2(->buf), Dt2(->bufstart_ofs), Dt1(->end_ofs), Dt1(->sink.closure));
   {
   //int64_t ofs = (int64_t)upb_pbdecoder_decode_varint_slow - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)upb_pbdecoder_decode_varint_slow), (unsigned int)(((uintptr_t)upb_pbdecoder_decode_varint_slow)>>32));
   //} else {
   //}
   }
# 454 "upb/pb/compile_decoder_x64.dasc"
  //|  load_regs
  //|  cmp   eax, 0
  //|  jge   >3
  //|  mov   edx, [rsp]   // Success; return parsed data.
  //|  add   rsp, 16
  //|  ret
  //|3:
  //|  call  ->exitjit   // Return eax from decode function.
  //|  jmp   <1
  //|
  //| // Called for varint >= 1 byte.
  dasm_put(Dst, 846, Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt1(->sink.closure), Dt1(->end_ofs), Dt2(->bufstart_ofs), Dt2(->buf));
# 465 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "decode_v32_v64_fallback");
  //|->decodev32_fallback:
  //|->decodev64_fallback:
  //|  chkeob   10, ->decode_varint_slow
  dasm_put(Dst, 900);
   if (10 == 1) {
  dasm_put(Dst, 905);
   } else {
  dasm_put(Dst, 913);
   }
# 469 "upb/pb/compile_decoder_x64.dasc"
  //|  // OPT: do something faster than just calling the C version.
  //|  mov      rdi, PTR
  //|  callp    upb_vdecode_fast
  dasm_put(Dst, 929);
   {
   //int64_t ofs = (int64_t)upb_vdecode_fast - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)upb_vdecode_fast), (unsigned int)(((uintptr_t)upb_vdecode_fast)>>32));
   //} else {
   //}
   }
# 472 "upb/pb/compile_decoder_x64.dasc"
  //|  test     rax, rax
  //|  je       ->decode_varint_slow  // Unterminated varint.
  //|  mov      PTR, rax
  //|  sub      PTR, 1
  //|  mov      DECODER->ptr, PTR
  //|  ret
  //|
  dasm_put(Dst, 933, Dt2(->ptr));
# 479 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "decode_varint_slow");
  //|->decode_varint_slow:
  //|  // Slow path: end of buffer or error (varint length >= 10).
  //|  mov64    rdi, (uintptr_t)upb_pbdecoder_decode_varint_slow
  //|  call     ->getvalue_slow
  //|  sub      PTR, 1
  //|  mov      DECODER->ptr, PTR
  //|  ret
  //|
  //| // Args: rsi=expected tag, return=rax (DECODE_{OK,MISMATCH})
  dasm_put(Dst, 954, (unsigned int)((uintptr_t)upb_pbdecoder_decode_varint_slow), (unsigned int)(((uintptr_t)upb_pbdecoder_decode_varint_slow)>>32), Dt2(->ptr));
# 489 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "checktag_fallback");
  //|->checktag_fallback:
  //|  sub      rsp, 8
  //|  mov      [rsp], rsi  // Preserve expected tag.
  //|1:
  //|  mov      ARG1_64, DECODER
  //|  commit_regs
  //|  mov      DECODER->checkpoint, PTR
  //|  callp    upb_pbdecoder_checktag_slow
  dasm_put(Dst, 974, Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt2(->delim_end), Dt2(->buf), Dt2(->bufstart_ofs), Dt1(->end_ofs), Dt1(->sink.closure), Dt2(->checkpoint));
   {
   //int64_t ofs = (int64_t)upb_pbdecoder_checktag_slow - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)upb_pbdecoder_checktag_slow), (unsigned int)(((uintptr_t)upb_pbdecoder_checktag_slow)>>32));
   //} else {
   //}
   }
# 498 "upb/pb/compile_decoder_x64.dasc"
  //|  load_regs
  //|  cmp      eax, 0
  //|  jge      >2
  //|  add      rsp, 8
  //|  ret
  //|2:
  //|  call     ->exitjit
  //|  mov      rsi, [rsp]
  //|  cmp      PTR, DELIMEND
  //|  jne      <1
  //|  mov      eax, DECODE_EOF
  //|  add      rsp, 8
  //|  ret
  //|
  //| // Args: rsi=upb_inttable, rdx=key, return=rax (-1 if not found).
  //| // Preserves: rcx, rdx
  //| // OPT: Could write this in assembly if it's a hotspot.
  dasm_put(Dst, 1028, Dt2(->top), Dt2(->ptr), Dt2(->data_end), Dt1(->sink.closure), Dt1(->end_ofs), Dt2(->bufstart_ofs), Dt2(->buf), DECODE_EOF);
# 515 "upb/pb/compile_decoder_x64.dasc"
  asmlabel(jc, "hashlookup");
  //|->hashlookup:
  //|  push   rcx
  //|  push   rdx
  //|  sub    rsp, 16
  //|  mov    rdi, rsi
  //|  mov    rsi, rdx
  //|  mov    rdx, rsp
  //|  callp  upb_inttable_lookup
  dasm_put(Dst, 1094);
   {
   //int64_t ofs = (int64_t)upb_inttable_lookup - (int64_t)upb_status_init;
   //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
  dasm_put(Dst, 30, (unsigned int)((uintptr_t)upb_inttable_lookup), (unsigned int)(((uintptr_t)upb_inttable_lookup)>>32));
   //} else {
   //}
   }
# 524 "upb/pb/compile_decoder_x64.dasc"
  //|  add    rsp, 16
  //|  pop    rdx
  //|  pop    rcx
  //|  test   al, al
  //|  jz     >2  // Unknown field.
  //|  mov    rax, [rsp-32]  // Value from table.
  //|  ret
  //|2:
  //|  xor    rax, rax
  //|  not    rax
  //|  ret
  dasm_put(Dst, 1114);
# 535 "upb/pb/compile_decoder_x64.dasc"
}

static void jitprimitive(jitcompiler *jc, opcode op,
                         const upb_handlers *h, upb_selector_t sel) {
  typedef enum { V32, V64, F32, F64, X } valtype_t;
  static valtype_t types[] = {
    X, F64, F32, V64, V64, V32, F64, F32, V64, X, X, X, X, V32, V32, F32, F64,
    V32, V64 };
  static char fastpath_bytes[] = { 1, 1, 4, 8 };
  const valtype_t type = types[op];
  const int fastbytes = fastpath_bytes[type];
  upb_func *handler = gethandler(h, sel);

  if (handler) {
    //|1:
    //|  chkneob  fastbytes, >3
    dasm_put(Dst, 1143);
     if (fastbytes == 1) {
    dasm_put(Dst, 1146);
     } else {
    dasm_put(Dst, 1154, fastbytes);
     }
# 551 "upb/pb/compile_decoder_x64.dasc"
    //|2:
    dasm_put(Dst, 1170);
# 552 "upb/pb/compile_decoder_x64.dasc"
    switch (type) {
    case V32:
      //|  call   ->decodev32_fallback
      dasm_put(Dst, 1173);
# 555 "upb/pb/compile_decoder_x64.dasc"
      break;
    case V64:
      //|  call   ->decodev64_fallback
      dasm_put(Dst, 1177);
# 558 "upb/pb/compile_decoder_x64.dasc"
      break;
    case F32:
      //|  call   ->decodef32_fallback
      dasm_put(Dst, 1181);
# 561 "upb/pb/compile_decoder_x64.dasc"
      break;
    case F64:
      //|  call   ->decodef64_fallback
      dasm_put(Dst, 1185);
# 564 "upb/pb/compile_decoder_x64.dasc"
      break;
    case X: break;
    }
    //|  jmp    >4
    dasm_put(Dst, 1189);
# 568 "upb/pb/compile_decoder_x64.dasc"

    // Fast path decode; for when check_bytes bytes are available.
    //|3:
    dasm_put(Dst, 1194);
# 571 "upb/pb/compile_decoder_x64.dasc"
    switch (op) {
    case OP_PARSE_SFIXED32:
    case OP_PARSE_FIXED32:
      //|  mov    edx, dword [PTR]
      dasm_put(Dst, 1197);
# 575 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_PARSE_SFIXED64:
    case OP_PARSE_FIXED64:
      //|  mov    rdx, qword [PTR]
      dasm_put(Dst, 1200);
# 579 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_PARSE_FLOAT:
      //|  movss  xmm0, dword [PTR]
      dasm_put(Dst, 1204);
# 582 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_PARSE_DOUBLE:
      //|  movsd  xmm0, qword [PTR]
      dasm_put(Dst, 1210);
# 585 "upb/pb/compile_decoder_x64.dasc"
      break;
    default:
      // Inline one byte of varint decoding.
      //|  movzx  edx, byte [PTR]
      //|  test   dl, dl
      //|  js     <2   // Fallback to slow path for >1 byte varint.
      dasm_put(Dst, 1216);
# 591 "upb/pb/compile_decoder_x64.dasc"
      break;
    }

    // Second-stage decode; used for both fast and slow paths
    // (only needed for a few types).
    //|4:
    dasm_put(Dst, 1226);
# 597 "upb/pb/compile_decoder_x64.dasc"
    switch (op) {
    case OP_PARSE_SINT32:
      // 32-bit zig-zag decode.
      //|  mov    eax, edx
      //|  shr    edx, 1
      //|  and    eax, 1
      //|  neg    eax
      //|  xor    edx, eax
      dasm_put(Dst, 1229);
# 605 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_PARSE_SINT64:
      // 64-bit zig-zag decode.
      //|  mov    rax, rdx
      //|  shr    rdx, 1
      //|  and    rax, 1
      //|  neg    rax
      //|  xor    rdx, rax
      dasm_put(Dst, 1243);
# 613 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_PARSE_BOOL:
      //|  test   rdx, rdx
      //|  setne  dl
      dasm_put(Dst, 1262);
# 617 "upb/pb/compile_decoder_x64.dasc"
      break;
    default: break;
    }

    // Call callback (or specialize if we can).
    upb_fieldtype_t type;
    const upb_shim_data *data = upb_shim_getdata(h, sel, &type);
    if (data) {
      switch (type) {
        case UPB_TYPE_INT64:
        case UPB_TYPE_UINT64:
          //|  mov   [CLOSURE + data->offset], rdx
          dasm_put(Dst, 1269, data->offset);
# 629 "upb/pb/compile_decoder_x64.dasc"
          break;
        case UPB_TYPE_INT32:
        case UPB_TYPE_UINT32:
        case UPB_TYPE_ENUM:
          //|  mov   [CLOSURE + data->offset], edx
          dasm_put(Dst, 1274, data->offset);
# 634 "upb/pb/compile_decoder_x64.dasc"
          break;
        case UPB_TYPE_DOUBLE:
          //|  movsd  qword [CLOSURE + data->offset], XMMARG1
          dasm_put(Dst, 1279, data->offset);
# 637 "upb/pb/compile_decoder_x64.dasc"
          break;
        case UPB_TYPE_FLOAT:
          //|  movss  dword [CLOSURE + data->offset], XMMARG1
          dasm_put(Dst, 1287, data->offset);
# 640 "upb/pb/compile_decoder_x64.dasc"
          break;
        case UPB_TYPE_BOOL:
          //|  mov   [CLOSURE + data->offset], dl
          dasm_put(Dst, 1295, data->offset);
# 643 "upb/pb/compile_decoder_x64.dasc"
          break;
        case UPB_TYPE_STRING:
        case UPB_TYPE_BYTES:
        case UPB_TYPE_MESSAGE:
          assert(false); break;
      }
      //|  sethas CLOSURE, data->hasbit
       if (data->hasbit >= 0) {
      dasm_put(Dst, 1300, ((uint32_t)data->hasbit / 8), (1 << ((uint32_t)data->hasbit % 8)));
       }
# 650 "upb/pb/compile_decoder_x64.dasc"
    } else if (handler) {
      //|  mov    ARG1_64, CLOSURE
      //|  load_handler_data h, sel
      dasm_put(Dst, 1306);
       {
       uintptr_t v = (uintptr_t)upb_handlers_gethandlerdata(h, sel);
       if (v > 0xffffffff) {
      dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
       } else if (v) {
      dasm_put(Dst, 391, v);
       } else {
      dasm_put(Dst, 394);
       }
       }
# 653 "upb/pb/compile_decoder_x64.dasc"
      //|  callp  handler
       {
       //int64_t ofs = (int64_t)handler - (int64_t)upb_status_init;
       //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
      dasm_put(Dst, 30, (unsigned int)((uintptr_t)handler), (unsigned int)(((uintptr_t)handler)>>32));
       //} else {
       //}
       }
# 654 "upb/pb/compile_decoder_x64.dasc"
      if (!alwaysok(h, sel)) {
        //|  test   al, al
        //|  jnz    >5
        //|  call   ->suspend
        //|  jmp    <1
        //|5:
        dasm_put(Dst, 1311);
# 660 "upb/pb/compile_decoder_x64.dasc"
      }
    }

    // We do this last so that the checkpoint is not advanced past the user's
    // data until the callback has returned success.
    //|  add    PTR, fastbytes
    dasm_put(Dst, 1327, fastbytes);
# 666 "upb/pb/compile_decoder_x64.dasc"
  } else {
    // No handler registered for this value, just skip it.
    //|  chkneob  fastbytes, >3
     if (fastbytes == 1) {
    dasm_put(Dst, 1146);
     } else {
    dasm_put(Dst, 1154, fastbytes);
     }
# 669 "upb/pb/compile_decoder_x64.dasc"
    //|2:
    dasm_put(Dst, 1170);
# 670 "upb/pb/compile_decoder_x64.dasc"
    switch (type) {
    case V32:
      //|  call   ->skipv32_fallback
      dasm_put(Dst, 1332);
# 673 "upb/pb/compile_decoder_x64.dasc"
      break;
    case V64:
      //|  call   ->skipv64_fallback
      dasm_put(Dst, 1336);
# 676 "upb/pb/compile_decoder_x64.dasc"
      break;
    case F32:
      //|  call   ->skipf32_fallback
      dasm_put(Dst, 1340);
# 679 "upb/pb/compile_decoder_x64.dasc"
      break;
    case F64:
      //|  call   ->skipf64_fallback
      dasm_put(Dst, 1344);
# 682 "upb/pb/compile_decoder_x64.dasc"
      break;
    case X: break;
    }

    // Fast-path skip.
    //|3:
    dasm_put(Dst, 1194);
# 688 "upb/pb/compile_decoder_x64.dasc"
    if (type == V32 || type == V64) {
      //|  test   byte [PTR], 0x80
      //|  jnz    <2
      dasm_put(Dst, 1348);
# 691 "upb/pb/compile_decoder_x64.dasc"
    }
    //|  add    PTR, fastbytes
    dasm_put(Dst, 1327, fastbytes);
# 693 "upb/pb/compile_decoder_x64.dasc"
  }
}

static void jitdispatch(jitcompiler *jc,
                        const upb_pbdecodermethod *method) {
  // Lots of room for tweaking/optimization here.

  const upb_inttable *dispatch = &method->dispatch;
  bool has_hash_entries = (dispatch->t.count > 0);

  // Whether any of the fields for this message can have two wire types which
  // are both valid (packed & non-packed).
  //
  // OPT: populate this more precisely; not all messages with hash entries have
  // this characteristic.
  bool has_multi_wiretype = has_hash_entries;

  //|=>define_jmptarget(jc, &method->dispatch):
  //|1:
  dasm_put(Dst, 1357, define_jmptarget(jc, &method->dispatch));
# 712 "upb/pb/compile_decoder_x64.dasc"
  // Decode the field tag.
  //|  mov     aword DECODER->checkpoint, PTR
  //|  chkeob  2, >6
  dasm_put(Dst, 248, Dt2(->checkpoint));
   if (2 == 1) {
  dasm_put(Dst, 1361);
   } else {
  dasm_put(Dst, 1369);
   }
# 715 "upb/pb/compile_decoder_x64.dasc"
  //|  movzx   edx, byte [PTR]
  //|  test    dl, dl
  //|  jns     >7    // Jump if first byte has no continuation bit.
  //|  movzx   ecx, byte [PTR + 1]
  //|  test    cl, cl
  //|  js      >6    // Jump if second byte has continuation bit.
  //|  // Confirmed two-byte varint.
  //|  shl     ecx, 7
  //|  and     edx, 0x7f
  //|  or      edx, ecx
  //|  add     PTR, 2
  //|  jmp     >8
  //|6:
  //|  call    ->decode_unknown_tag_fallback
  //|  test    eax, eax  // Hit DELIMEND?
  //|  jnz     >8
  //|  ret
  //|7:
  //|  add     PTR, 1
  //|8:
  //|  mov     ecx, edx
  //|  shr     edx, 3
  //|  and     cl, 7
  dasm_put(Dst, 1385, 1);
# 738 "upb/pb/compile_decoder_x64.dasc"

  // See comment attached to upb_pbdecodermethod.dispatch for layout of the
  // dispatch table.
  //|2:
  //|  cmp     edx, dispatch->array_size
  dasm_put(Dst, 1450, dispatch->array_size);
# 743 "upb/pb/compile_decoder_x64.dasc"
  if (has_hash_entries) {
    //|  jae     >7
    dasm_put(Dst, 1457);
# 745 "upb/pb/compile_decoder_x64.dasc"
  } else {
    //|  jae     >5
    dasm_put(Dst, 1462);
# 747 "upb/pb/compile_decoder_x64.dasc"
  }
  //|  // OPT: Compact the lookup arr into 32-bit entries.
  if ((uintptr_t)dispatch->array > 0x7fffffff) {
    //|  mov64 rax, (uintptr_t)dispatch->array
    //|  mov   rax, qword [rax + rdx * 8]
    dasm_put(Dst, 1467, (unsigned int)((uintptr_t)dispatch->array), (unsigned int)(((uintptr_t)dispatch->array)>>32));
# 752 "upb/pb/compile_decoder_x64.dasc"
  } else {
    //|  mov   rax, qword [rdx * 8 + dispatch->array]
    dasm_put(Dst, 1476, dispatch->array);
# 754 "upb/pb/compile_decoder_x64.dasc"
  }
  //|3:
  //|  // We take advantage of the fact that non-present entries are stored
  //|  // as -1, which will result in wire types that will never match.
  //|  cmp  al, cl
  dasm_put(Dst, 1482);
# 759 "upb/pb/compile_decoder_x64.dasc"
  if (has_multi_wiretype) {
    //|  jne  >6
    dasm_put(Dst, 1487);
# 761 "upb/pb/compile_decoder_x64.dasc"
  } else {
    //|  jne  >5
    dasm_put(Dst, 1492);
# 763 "upb/pb/compile_decoder_x64.dasc"
  }
  //|  shr  rax, 16
  //|
  //|  // Load the machine code address from the table entry.
  //|  // The table entry is relative to the dispatch->array jmptarget
  //|  // (patchdispatch() took care of this) which is the same as
  //|  // local label "4".  The "lea" is really just trying to do
  //|  //    lea  rax, [>4 + rax]
  //|  //
  //|  // But we can't write that directly for some reason, so we use
  //|  // rdx as a temporary.
  //|  lea  rdx, [>4]
  //|=>define_jmptarget(jc, dispatch->array):
  //|4:
  //|  add  rax, rdx
  //|  ret
  //|
  //|5:
  //|  // Field isn't in our table.
  //|  call ->parse_unknown
  //|  test eax, eax  // ENDGROUP?
  //|  jz   <1
  //|  lea  rax, [>9]  // ENDGROUP; Load address of OP_ENDMSG.
  //|  ret
  dasm_put(Dst, 1497, define_jmptarget(jc, dispatch->array));
# 787 "upb/pb/compile_decoder_x64.dasc"

  if (has_multi_wiretype) {
    //|6:
    //|  // Primary wire type didn't match, check secondary wire type.
    //|  cmp  ah, cl
    //|  jne  <5
    //|  // Secondary wire type is a match, look up fn + UPB_MAX_FIELDNUMBER.
    //|  add   rdx, UPB_MAX_FIELDNUMBER
    //|  // This key will never be in the array part, so do a hash lookup.
    dasm_put(Dst, 1531, UPB_MAX_FIELDNUMBER);
# 796 "upb/pb/compile_decoder_x64.dasc"
    assert(has_hash_entries);
    //|  ld64  dispatch
     {
     uintptr_t v = (uintptr_t)dispatch;
     if (v > 0xffffffff) {
    dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
     } else if (v) {
    dasm_put(Dst, 391, v);
     } else {
    dasm_put(Dst, 394);
     }
     }
# 798 "upb/pb/compile_decoder_x64.dasc"
    //|  jmp   ->hashlookup  // Tail call.
    dasm_put(Dst, 1544);
# 799 "upb/pb/compile_decoder_x64.dasc"
  }

  if (has_hash_entries) {
    //|7:
    //|  // Hash table lookup.
    //|  ld64   dispatch
    dasm_put(Dst, 1549);
     {
     uintptr_t v = (uintptr_t)dispatch;
     if (v > 0xffffffff) {
    dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
     } else if (v) {
    dasm_put(Dst, 391, v);
     } else {
    dasm_put(Dst, 394);
     }
     }
# 805 "upb/pb/compile_decoder_x64.dasc"
    //|  call   ->hashlookup
    //|  jmp    <3
    dasm_put(Dst, 1552);
# 807 "upb/pb/compile_decoder_x64.dasc"
  }
}

static void jittag(jitcompiler *jc, uint64_t tag, int n, int ofs,
                   const upb_pbdecodermethod *method) {
  // Internally we parse unknown fields; if this runs us into DELIMEND we jump
  // to the corresponding DELIMEND target (either msg end or repeated field
  // end), which we find from the OP_CHECKDELIM which must have necessarily
  // preceded us.
  uint32_t last_instruction = *(jc->pc - 2);
  int last_arg = (int32_t)last_instruction >> 8;
  assert((last_instruction & 0xff) == OP_CHECKDELIM);
  uint32_t *delimend = (jc->pc - 1) + last_arg;
  const size_t ptr_words = sizeof(void*) / sizeof(uint32_t);

  if (getop(*(jc->pc - 1)) == OP_TAGN) {
    jc->pc += ptr_words;
  }

  //|  chkneob n, >1
   if (n == 1) {
  dasm_put(Dst, 1560);
   } else {
  dasm_put(Dst, 1568, n);
   }
# 827 "upb/pb/compile_decoder_x64.dasc"

  //|  // OPT: this is way too much fallback code to put here.
  //|  // Reduce and/or move to a separate section to make better icache usage.
  //|  ld64  tag
   {
   uintptr_t v = (uintptr_t)tag;
   if (v > 0xffffffff) {
  dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
   } else if (v) {
  dasm_put(Dst, 391, v);
   } else {
  dasm_put(Dst, 394);
   }
   }
# 831 "upb/pb/compile_decoder_x64.dasc"
  //|  call  ->checktag_fallback
  //|  cmp   eax, DECODE_MISMATCH
  //|  je    >3
  //|  cmp   eax, DECODE_EOF
  //|  je     =>jmptarget(jc, delimend)
  //|  jmp   >5
  dasm_put(Dst, 1584, DECODE_MISMATCH, DECODE_EOF, jmptarget(jc, delimend));
# 837 "upb/pb/compile_decoder_x64.dasc"

  //|1:
  dasm_put(Dst, 1143);
# 839 "upb/pb/compile_decoder_x64.dasc"
  switch (n) {
  case 1:
    //|  cmp  byte [PTR], tag
    dasm_put(Dst, 1607, tag);
# 842 "upb/pb/compile_decoder_x64.dasc"
    break;
  case 2:
    //|  cmp  word [PTR], tag
    dasm_put(Dst, 1611, tag);
# 845 "upb/pb/compile_decoder_x64.dasc"
    break;
  case 3:
    //|   // OPT: Slightly more efficient code, but depends on an extra byte.
    //|   // mov  eax, dword [PTR]
    //|   // shl  eax, 8
    //|   // cmp  eax, tag << 8
    //|   cmp  word [PTR], (tag & 0xffff)
    //|   jne  >2
    //|   cmp  byte [PTR + 2], (tag >> 16)
    //|2:
    dasm_put(Dst, 1616, (tag & 0xffff), 2, (tag >> 16));
# 855 "upb/pb/compile_decoder_x64.dasc"
    break;
  case 4:
    //|   cmp  dword [PTR], tag
    dasm_put(Dst, 1631, tag);
# 858 "upb/pb/compile_decoder_x64.dasc"
    break;
  case 5:
    //|   cmp  dword [PTR], (tag & 0xffffffff)
    //|   jne  >3
    //|   cmp  byte  [PTR + 4], (tag >> 32)
    dasm_put(Dst, 1635, (tag & 0xffffffff), 4, (tag >> 32));
# 863 "upb/pb/compile_decoder_x64.dasc"
  }
  //|  je    >4
  //|3:
  dasm_put(Dst, 1647);
# 866 "upb/pb/compile_decoder_x64.dasc"
  if (ofs == 0) {
    //|  call   =>jmptarget(jc, &method->dispatch)
    //|  test   rax, rax
    //|  jz     =>jmptarget(jc, delimend)
    //|  jmp    rax
    dasm_put(Dst, 1654, jmptarget(jc, &method->dispatch), jmptarget(jc, delimend));
# 871 "upb/pb/compile_decoder_x64.dasc"
  } else {
    //|  jmp    =>jmptarget(jc, jc->pc + ofs)
    dasm_put(Dst, 1666, jmptarget(jc, jc->pc + ofs));
# 873 "upb/pb/compile_decoder_x64.dasc"
  }
  //|4:
  //|  add    PTR, n
  //|5:
  dasm_put(Dst, 1670, n);
# 877 "upb/pb/compile_decoder_x64.dasc"
}

// Compile the bytecode to x64.
static void jitbytecode(jitcompiler *jc) {
  upb_pbdecodermethod *method = NULL;
  const upb_handlers *h = NULL;
  for (jc->pc = jc->group->bytecode; jc->pc < jc->group->bytecode_end; ) {
    int32_t instr = *jc->pc;
    opcode op = instr & 0xff;
    uint32_t arg = instr >> 8;
    int32_t longofs = arg;

    if (op != OP_SETDISPATCH) {
      // Skipped for SETDISPATCH because it defines its own asmlabel for the
      // dispatch code it emits.
      asmlabel(jc, "0x%lx.%s", pcofs(jc), upb_pbdecoder_getopname(op));

      // Skipped for SETDISPATCH because it should point at the function
      // prologue, not the dispatch function that is emitted first.
      // TODO: optimize this to only define pclabels that are actually used.
      //|=>define_jmptarget(jc, jc->pc):
      dasm_put(Dst, 0, define_jmptarget(jc, jc->pc));
# 898 "upb/pb/compile_decoder_x64.dasc"
    }

    jc->pc++;

    switch (op) {
    case OP_STARTMSG: {
      upb_func *startmsg = gethandler(h, UPB_STARTMSG_SELECTOR);
      if (startmsg) {
        // bool startmsg(void *closure, const void *hd)
        //|1:
        //|  mov   ARG1_64, CLOSURE
        //|  load_handler_data h, UPB_STARTMSG_SELECTOR
        dasm_put(Dst, 1679);
         {
         uintptr_t v = (uintptr_t)upb_handlers_gethandlerdata(h, UPB_STARTMSG_SELECTOR);
         if (v > 0xffffffff) {
        dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
         } else if (v) {
        dasm_put(Dst, 391, v);
         } else {
        dasm_put(Dst, 394);
         }
         }
# 910 "upb/pb/compile_decoder_x64.dasc"
        //|  callp startmsg
         {
         //int64_t ofs = (int64_t)startmsg - (int64_t)upb_status_init;
         //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
        dasm_put(Dst, 30, (unsigned int)((uintptr_t)startmsg), (unsigned int)(((uintptr_t)startmsg)>>32));
         //} else {
         //}
         }
# 911 "upb/pb/compile_decoder_x64.dasc"
        if (!alwaysok(h, UPB_STARTMSG_SELECTOR)) {
          //|  test  al, al
          //|  jnz   >2
          //|  call  ->suspend
          //|  jmp   <1
          //|2:
          dasm_put(Dst, 1686);
# 917 "upb/pb/compile_decoder_x64.dasc"
        }
      } else {
        //| nop
        dasm_put(Dst, 1702);
# 920 "upb/pb/compile_decoder_x64.dasc"
      }
      break;
    }
    case OP_ENDMSG: {
      upb_func *endmsg = gethandler(h, UPB_ENDMSG_SELECTOR);
      //|9:
      dasm_put(Dst, 1704);
# 926 "upb/pb/compile_decoder_x64.dasc"
      if (endmsg) {
        // bool endmsg(void *closure, const void *hd, upb_status *status)
        //|  mov   ARG1_64, CLOSURE
        //|  load_handler_data h, UPB_ENDMSG_SELECTOR
        dasm_put(Dst, 1306);
         {
         uintptr_t v = (uintptr_t)upb_handlers_gethandlerdata(h, UPB_ENDMSG_SELECTOR);
         if (v > 0xffffffff) {
        dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
         } else if (v) {
        dasm_put(Dst, 391, v);
         } else {
        dasm_put(Dst, 394);
         }
         }
# 930 "upb/pb/compile_decoder_x64.dasc"
        //|  mov   ARG3_64, DECODER->status
        //|  callp endmsg
        dasm_put(Dst, 1707, Dt2(->status));
         {
         //int64_t ofs = (int64_t)endmsg - (int64_t)upb_status_init;
         //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
        dasm_put(Dst, 30, (unsigned int)((uintptr_t)endmsg), (unsigned int)(((uintptr_t)endmsg)>>32));
         //} else {
         //}
         }
# 932 "upb/pb/compile_decoder_x64.dasc"
      }
      break;
    }
    case OP_SETDISPATCH: {
      uint32_t *op_pc = jc->pc - 1;

      // Load info for new method.
      upb_inttable *dispatch;
      memcpy(&dispatch, jc->pc, sizeof(void*));
      jc->pc += sizeof(void*) / sizeof(uint32_t);
      // The OP_SETDISPATCH bytecode contains a pointer that is
      // &method->dispatch; we want to go backwards and recover method.
      method =
          (void*)((char*)dispatch - offsetof(upb_pbdecodermethod, dispatch));
      // May be NULL, in which case no handlers for this message will be found.
      // OPT: we should do better by completely skipping the message in this
      // case instead of parsing it field by field.  We should also do the skip
      // in the containing message's code.
      h = method->dest_handlers_;
      const char *msgname = upb_msgdef_fullname(upb_handlers_msgdef(h));

      // Emit dispatch code for new method.
      asmlabel(jc, "0x%lx.dispatch.%s", pcofs(jc), msgname);
      jitdispatch(jc, method);

      // Emit function prologue for new method.
      asmlabel(jc, "0x%lx.parse.%s", pcofs(jc), msgname);
      //|=>define_jmptarget(jc, op_pc):
      //|=>define_jmptarget(jc, method):
      //|  sub   rsp, 8
      dasm_put(Dst, 1712, define_jmptarget(jc, op_pc), define_jmptarget(jc, method));
# 962 "upb/pb/compile_decoder_x64.dasc"

      break;
    }
    case OP_PARSE_DOUBLE:
    case OP_PARSE_FLOAT:
    case OP_PARSE_INT64:
    case OP_PARSE_UINT64:
    case OP_PARSE_INT32:
    case OP_PARSE_FIXED64:
    case OP_PARSE_FIXED32:
    case OP_PARSE_BOOL:
    case OP_PARSE_UINT32:
    case OP_PARSE_SFIXED32:
    case OP_PARSE_SFIXED64:
    case OP_PARSE_SINT32:
    case OP_PARSE_SINT64:
      jitprimitive(jc, op, h, arg);
      break;
    case OP_STARTSEQ:
    case OP_STARTSUBMSG:
    case OP_STARTSTR: {
      upb_func *start = gethandler(h, arg);
      if (start) {
        // void *startseq(void *closure, const void *hd)
        // void *startsubmsg(void *closure, const void *hd)
        // void *startstr(void *closure, const void *hd, size_t size_hint)
        //|1:
        //|  mov   ARG1_64, CLOSURE
        //|  load_handler_data h, arg
        dasm_put(Dst, 1679);
         {
         uintptr_t v = (uintptr_t)upb_handlers_gethandlerdata(h, arg);
         if (v > 0xffffffff) {
        dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
         } else if (v) {
        dasm_put(Dst, 391, v);
         } else {
        dasm_put(Dst, 394);
         }
         }
# 991 "upb/pb/compile_decoder_x64.dasc"
        if (op == OP_STARTSTR) {
          //|  mov    ARG3_64, DELIMEND
          //|  sub    ARG3_64, PTR
          dasm_put(Dst, 1720);
# 994 "upb/pb/compile_decoder_x64.dasc"
        }
        //|  callp start
         {
         //int64_t ofs = (int64_t)start - (int64_t)upb_status_init;
         //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
        dasm_put(Dst, 30, (unsigned int)((uintptr_t)start), (unsigned int)(((uintptr_t)start)>>32));
         //} else {
         //}
         }
# 996 "upb/pb/compile_decoder_x64.dasc"
        if (!alwaysok(h, arg)) {
          //|  test  rax, rax
          //|  jnz   >2
          //|  call  ->suspend
          //|  jmp   <1
          //|2:
          dasm_put(Dst, 1728);
# 1002 "upb/pb/compile_decoder_x64.dasc"
        }
        //|  mov   CLOSURE, rax
        dasm_put(Dst, 1745);
# 1004 "upb/pb/compile_decoder_x64.dasc"
      } else {
        // TODO: nop is only required because of asmlabel().
        //|  nop
        dasm_put(Dst, 1702);
# 1007 "upb/pb/compile_decoder_x64.dasc"
      }
      break;
    }
    case OP_ENDSEQ:
    case OP_ENDSUBMSG:
    case OP_ENDSTR: {
      upb_func *end = gethandler(h, arg);
      if (end) {
        // bool endseq(void *closure, const void *hd)
        // bool endsubmsg(void *closure, const void *hd)
        // bool endstr(void *closure, const void *hd)
        //|1:
        //|  mov   ARG1_64, CLOSURE
        //|  load_handler_data h, arg
        dasm_put(Dst, 1679);
         {
         uintptr_t v = (uintptr_t)upb_handlers_gethandlerdata(h, arg);
         if (v > 0xffffffff) {
        dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
         } else if (v) {
        dasm_put(Dst, 391, v);
         } else {
        dasm_put(Dst, 394);
         }
         }
# 1021 "upb/pb/compile_decoder_x64.dasc"
        //|  callp end
         {
         //int64_t ofs = (int64_t)end - (int64_t)upb_status_init;
         //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
        dasm_put(Dst, 30, (unsigned int)((uintptr_t)end), (unsigned int)(((uintptr_t)end)>>32));
         //} else {
         //}
         }
# 1022 "upb/pb/compile_decoder_x64.dasc"
        if (!alwaysok(h, arg)) {
          //|  test  al, al
          //|  jnz   >2
          //|  call  ->suspend
          //|  jmp   <1
          //|2:
          dasm_put(Dst, 1686);
# 1028 "upb/pb/compile_decoder_x64.dasc"
        }
      } else {
        // TODO: nop is only required because of asmlabel().
        //|  nop
        dasm_put(Dst, 1702);
# 1032 "upb/pb/compile_decoder_x64.dasc"
      }
      break;
    }
    case OP_STRING: {
      upb_func *str = gethandler(h, arg);
      //|  cmp   PTR, DELIMEND
      //|  je    >4
      //|1:
      //|  cmp   PTR, DATAEND
      //|  jne   >2
      //|  call  ->suspend
      //|  jmp   <1
      //|2:
      dasm_put(Dst, 1749);
# 1045 "upb/pb/compile_decoder_x64.dasc"
      if (str) {
        // size_t str(void *closure, const void *hd, const char *str, size_t n)
        //|  mov   ARG1_64, CLOSURE
        //|  load_handler_data h, arg
        dasm_put(Dst, 1306);
         {
         uintptr_t v = (uintptr_t)upb_handlers_gethandlerdata(h, arg);
         if (v > 0xffffffff) {
        dasm_put(Dst, 386, (unsigned int)(v), (unsigned int)((v)>>32));
         } else if (v) {
        dasm_put(Dst, 391, v);
         } else {
        dasm_put(Dst, 394);
         }
         }
# 1049 "upb/pb/compile_decoder_x64.dasc"
        //|  mov   ARG3_64, PTR
        //|  mov   ARG4_64, DATAEND
        //|  sub   ARG4_64, PTR
        //|  mov   ARG5_64, qword DECODER->handle
        //|  callp str
        dasm_put(Dst, 1776, Dt2(->handle));
         {
         //int64_t ofs = (int64_t)str - (int64_t)upb_status_init;
         //if (ofs > (1 << 30) || ofs < -(1 << 30)) {
        dasm_put(Dst, 30, (unsigned int)((uintptr_t)str), (unsigned int)(((uintptr_t)str)>>32));
         //} else {
         //}
         }
# 1054 "upb/pb/compile_decoder_x64.dasc"
        //|  add   PTR, rax
        dasm_put(Dst, 1790);
# 1055 "upb/pb/compile_decoder_x64.dasc"
        if (!alwaysok(h, arg)) {
          //|  cmp   PTR, DATAEND
          //|  je    >3
          //|  call  ->strret_fallback
          //|3:
          dasm_put(Dst, 1794);
# 1060 "upb/pb/compile_decoder_x64.dasc"
        }
      } else {
        //|  mov   PTR, DATAEND
        dasm_put(Dst, 1807);
# 1063 "upb/pb/compile_decoder_x64.dasc"
      }
      //|  cmp   PTR, DELIMEND
      //|  jne   <1
      //|4:
      dasm_put(Dst, 1811);
# 1067 "upb/pb/compile_decoder_x64.dasc"
      break;
    }
    case OP_PUSHTAGDELIM:
      //|  mov   FRAME->sink.closure, CLOSURE
      //|  // This shouldn't need to be read, because tag-delimited fields
      //|  // shouldn't have an OP_SETDELIM after them.  But for the moment
      //|  // non-packed repeated fields do OP_SETDELIM so they can share more
      //|  // code with the packed code-path.  If this is changed later, this
      //|  // store can be removed.
      //|  mov   qword FRAME->end_ofs, 0
      //|  add   FRAME, sizeof(upb_pbdecoder_frame)
      //|  cmp   FRAME, DECODER->limit
      //|  je    ->err
      //|  mov   dword FRAME->groupnum, arg
      dasm_put(Dst, 1822, Dt1(->sink.closure), Dt1(->end_ofs), sizeof(upb_pbdecoder_frame), Dt2(->limit), Dt1(->groupnum), arg);
# 1081 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_PUSHLENDELIM:
      //|  call  ->pushlendelim
      dasm_put(Dst, 1852);
# 1084 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_POP:
      //|  sub   FRAME, sizeof(upb_pbdecoder_frame)
      //|  mov   CLOSURE, FRAME->sink.closure
      dasm_put(Dst, 1856, sizeof(upb_pbdecoder_frame), Dt1(->sink.closure));
# 1088 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_SETDELIM:
      // OPT: experiment with testing vs old offset to optimize away.
      //|  mov   DATAEND, DECODER->end
      //|  add   DELIMEND, FRAME->end_ofs
      //|  cmp   DELIMEND, DECODER->buf
      //|  jb    >1
      //|  cmp   DELIMEND, DATAEND
      //|  ja    >1   // OPT: try cmov.
      //|  mov   DATAEND, DELIMEND
      //|1:
      dasm_put(Dst, 1866, Dt2(->end), Dt1(->end_ofs), Dt2(->buf));
# 1099 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_SETBIGGROUPNUM:
      //|  mov   dword FRAME->groupnum, *jc->pc++
      dasm_put(Dst, 1846, Dt1(->groupnum), *jc->pc++);
# 1102 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_CHECKDELIM:
      //|  cmp  DELIMEND, PTR
      //|  je   =>jmptarget(jc, jc->pc + longofs)
      dasm_put(Dst, 1896, jmptarget(jc, jc->pc + longofs));
# 1106 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_CALL:
      //|  call =>jmptarget(jc, jc->pc + longofs)
      dasm_put(Dst, 1903, jmptarget(jc, jc->pc + longofs));
# 1109 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_BRANCH:
      //|  jmp  =>jmptarget(jc, jc->pc + longofs);
      dasm_put(Dst, 1666, jmptarget(jc, jc->pc + longofs));
# 1112 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_RET:
      //|9:
      //|  add  rsp, 8
      //|  ret
      dasm_put(Dst, 1906);
# 1117 "upb/pb/compile_decoder_x64.dasc"
      break;
    case OP_TAG1:
      jittag(jc, (arg >> 8) & 0xff, 1, (int8_t)arg, method);
      break;
    case OP_TAG2:
      jittag(jc, (arg >> 8) & 0xffff, 2, (int8_t)arg, method);
      break;
    case OP_TAGN: {
      uint64_t tag;
      memcpy(&tag, jc->pc, 8);
      jittag(jc, tag, arg >> 8, (int8_t)arg, method);
      break;
    }
    case OP_HALT:
      assert(false);
    }
  }

  asmlabel(jc, "eof");
  //|  nop
  dasm_put(Dst, 1702);
# 1137 "upb/pb/compile_decoder_x64.dasc"
}
