/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2013 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Code to compile a upb::MessageDef into bytecode for decoding that message.
 * Bytecode definition is in decoder.int.h.
 */

#include <stdarg.h>
#include "upb/pb/decoder.int.h"
#include "upb/pb/varint.int.h"
#include "upb/bytestream.h"

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif

#define MAXLABEL 5
#define EMPTYLABEL -1

/* upb_pbdecodermethod ********************************************************/

static upb_pbdecodermethod *newmethod(const upb_msgdef *msg,
                                      const upb_handlers *dest_handlers) {
  upb_pbdecodermethod *ret = malloc(sizeof(upb_pbdecodermethod));
  ret->msg = msg;
  ret->dest_handlers = dest_handlers;
  ret->native_code = false;  // If we JIT, it will update this later.
  upb_inttable_init(&ret->dispatch, UPB_CTYPE_UINT64);

  if (ret->dest_handlers) {
    upb_handlers_ref(ret->dest_handlers, ret);
  }
  return ret;
}

static void freemethod(upb_pbdecodermethod *method) {
  if (method->dest_handlers) {
    upb_handlers_unref(method->dest_handlers, method);
  }

  upb_inttable_uninit(&method->dispatch);
  free(method);
}


/* upb_pbdecoderplan **********************************************************/

upb_pbdecoderplan *newplan() {
  upb_pbdecoderplan *p = malloc(sizeof(*p));
  upb_inttable_init(&p->methods, UPB_CTYPE_PTR);
  p->code = NULL;
  p->code_end = NULL;
  return p;
}

void freeplan(void *_p) {
  upb_pbdecoderplan *p = _p;

  upb_inttable_iter i;
  upb_inttable_begin(&i, &p->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    freemethod(method);
  }
  upb_inttable_uninit(&p->methods);
  free(p->code);
#ifdef UPB_USE_JIT_X64
  upb_pbdecoder_freejit(p);
#endif
  free(p);
}

void set_bytecode_handlers(upb_pbdecoderplan *p, upb_handlers *h) {
  upb_handlers_setstartstr(h, UPB_BYTESTREAM_BYTES, upb_pbdecoder_start, p,
                           NULL);
  upb_handlers_setstring(h, UPB_BYTESTREAM_BYTES, upb_pbdecoder_decode, p,
                         freeplan);
  upb_handlers_setendstr(h, UPB_BYTESTREAM_BYTES, upb_pbdecoder_end, p, NULL);
}

static const upb_pbdecoderplan *getdecoderplan(const upb_handlers *h) {
  if (upb_handlers_frametype(h) != &upb_pbdecoder_frametype)
    return NULL;
  upb_selector_t sel;
  if (!upb_handlers_getselector(UPB_BYTESTREAM_BYTES, UPB_HANDLER_STARTSTR,
                                &sel)) {
    return NULL;
  }
  return upb_handlers_gethandlerdata(h, sel);
}


/* compiler *******************************************************************/

// Data used only at compilation time.
typedef struct {
  upb_pbdecoderplan *plan;

  uint32_t *pc;
  int fwd_labels[MAXLABEL];
  int back_labels[MAXLABEL];
} compiler;

static compiler *newcompiler(upb_pbdecoderplan *plan) {
  compiler *ret = malloc(sizeof(compiler));
  ret->plan = plan;
  for (int i = 0; i < MAXLABEL; i++) {
    ret->fwd_labels[i] = EMPTYLABEL;
    ret->back_labels[i] = EMPTYLABEL;
  }
  return ret;
}

static void freecompiler(compiler *c) {
  free(c);
}

const size_t ptr_words = sizeof(void*) / sizeof(uint32_t);

// How many words an instruction is.
static int instruction_len(uint32_t instr) {
  switch (getop(instr)) {
    case OP_SETDISPATCH: return 1 + ptr_words;
    case OP_TAGN: return 3;
    case OP_SETBIGGROUPNUM: return 2;
    default: return 1;
  }
}

bool op_has_longofs(int32_t instruction) {
  switch (getop(instruction)) {
    case OP_CALL:
    case OP_BRANCH:
    case OP_CHECKDELIM:
      return true;
    // The "tag" instructions only have 8 bytes available for the jump target,
    // but that is ok because these opcodes only require short jumps.
    case OP_TAG1:
    case OP_TAG2:
    case OP_TAGN:
      return false;
    default:
      assert(false);
      return false;
  }
}

static int32_t getofs(uint32_t instruction) {
  if (op_has_longofs(instruction)) {
    return (int32_t)instruction >> 8;
  } else {
    return (int8_t)(instruction >> 8);
  }
}

static void setofs(uint32_t *instruction, int32_t ofs) {
  if (op_has_longofs(*instruction)) {
    *instruction = getop(*instruction) | ofs << 8;
  } else {
    *instruction = (*instruction & ~0xff00) | ((ofs & 0xff) << 8);
  }
  assert(getofs(*instruction) == ofs);  // Would fail in cases of overflow.
}

static uint32_t pcofs(compiler *c) { return c->pc - c->plan->code; }

// Defines a local label at the current PC location.  All previous forward
// references are updated to point to this location.  The location is noted
// for any future backward references.
static void label(compiler *c, unsigned int label) {
  assert(label < MAXLABEL);
  int val = c->fwd_labels[label];
  uint32_t *codep = (val == EMPTYLABEL) ? NULL : c->plan->code + val;
  while (codep) {
    int ofs = getofs(*codep);
    setofs(codep, c->pc - codep - instruction_len(*codep));
    codep = ofs ? codep + ofs : NULL;
  }
  c->fwd_labels[label] = EMPTYLABEL;
  c->back_labels[label] = pcofs(c);
}

// Creates a reference to a numbered label; either a forward reference
// (positive arg) or backward reference (negative arg).  For forward references
// the value returned now is actually a "next" pointer into a linked list of all
// instructions that use this label and will be patched later when the label is
// defined with label().
//
// The returned value is the offset that should be written into the instruction.
static int32_t labelref(compiler *c, int label) {
  assert(label < MAXLABEL);
  if (label == LABEL_DISPATCH) {
    // No resolving required.
    return 0;
  } else if (label < 0) {
    // Backward local label.  Relative to the next instruction.
    uint32_t from = (c->pc + 1) - c->plan->code;
    return c->back_labels[-label] - from;
  } else {
    // Forward local label: prepend to (possibly-empty) linked list.
    int *lptr = &c->fwd_labels[label];
    int32_t ret = (*lptr == EMPTYLABEL) ? 0 : *lptr - pcofs(c);
    *lptr = pcofs(c);
    return ret;
  }
}

static void put32(compiler *c, uint32_t v) {
  if (c->pc == c->plan->code_end) {
    int ofs = pcofs(c);
    size_t oldsize = c->plan->code_end - c->plan->code;
    size_t newsize = UPB_MAX(oldsize * 2, 64);
    // TODO(haberman): handle OOM.
    c->plan->code = realloc(c->plan->code, newsize * sizeof(uint32_t));
    c->plan->code_end = c->plan->code + newsize;
    c->pc = c->plan->code + ofs;
  }
  *c->pc++ = v;
}

static void putop(compiler *c, opcode op, ...) {
  va_list ap;
  va_start(ap, op);

  switch (op) {
    case OP_SETDISPATCH: {
      uintptr_t ptr = (uintptr_t)va_arg(ap, void*);
      put32(c, OP_SETDISPATCH);
      put32(c, ptr);
      if (sizeof(uintptr_t) > sizeof(uint32_t))
        put32(c, (uint64_t)ptr >> 32);
      break;
    }
    case OP_STARTMSG:
    case OP_ENDMSG:
    case OP_PUSHTAGDELIM:
    case OP_PUSHLENDELIM:
    case OP_POP:
    case OP_SETDELIM:
    case OP_HALT:
      put32(c, op);
      break;
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
    case OP_STARTSEQ:
    case OP_SETGROUPNUM:
    case OP_ENDSEQ:
    case OP_STARTSUBMSG:
    case OP_ENDSUBMSG:
    case OP_STARTSTR:
    case OP_STRING:
    case OP_ENDSTR:
      put32(c, op | va_arg(ap, upb_selector_t) << 8);
      break;
    case OP_SETBIGGROUPNUM:
      put32(c, op);
      put32(c, va_arg(ap, int));
      break;
    case OP_CALL: {
      const upb_pbdecodermethod *method = va_arg(ap, upb_pbdecodermethod *);
      put32(c, op | (method->base.ofs - (pcofs(c) + 1)) << 8);
      break;
    }
    case OP_CHECKDELIM:
    case OP_BRANCH: {
      uint32_t instruction = op;
      int label = va_arg(ap, int);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      break;
    }
    case OP_TAG1:
    case OP_TAG2: {
      int label = va_arg(ap, int);
      uint64_t tag = va_arg(ap, uint64_t);
      uint32_t instruction = op | (tag << 16);
      assert(tag <= 0xffff);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      break;
    }
    case OP_TAGN: {
      int label = va_arg(ap, int);
      uint64_t tag = va_arg(ap, uint64_t);
      uint32_t instruction = op | (upb_value_size(tag) << 16);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      put32(c, tag);
      put32(c, tag >> 32);
      break;
    }
  }

  va_end(ap);
}

#if defined(UPB_USE_JIT_X64) || defined(UPB_DUMP_BYTECODE)

const char *upb_pbdecoder_getopname(unsigned int op) {
#define OP(op) [OP_ ## op] = "OP_" #op
#define T(op) OP(PARSE_##op)
  static const char *names[] = {
    "<no opcode>",
    T(DOUBLE), T(FLOAT), T(INT64), T(UINT64), T(INT32), T(FIXED64), T(FIXED32),
    T(BOOL), T(UINT32), T(SFIXED32), T(SFIXED64), T(SINT32), T(SINT64),
    OP(STARTMSG), OP(ENDMSG), OP(STARTSEQ), OP(ENDSEQ), OP(STARTSUBMSG),
    OP(ENDSUBMSG), OP(STARTSTR), OP(STRING), OP(ENDSTR), OP(CALL),
    OP(PUSHLENDELIM), OP(PUSHTAGDELIM), OP(SETDELIM), OP(CHECKDELIM),
    OP(BRANCH), OP(TAG1), OP(TAG2), OP(TAGN), OP(SETDISPATCH), OP(POP),
    OP(SETGROUPNUM), OP(SETBIGGROUPNUM), OP(HALT),
  };
  return op > OP_HALT ? names[0] : names[op];
#undef OP
#undef T
}

#endif

#ifdef UPB_DUMP_BYTECODE

static void dumpbc(uint32_t *p, uint32_t *end, FILE *f) {

  uint32_t *begin = p;

  while (p < end) {
    fprintf(f, "%p  %8tx", p, p - begin);
    uint32_t instr = *p++;
    uint8_t op = getop(instr);
    fprintf(f, " %s", upb_pbdecoder_getopname(op));
    switch ((opcode)op) {
      case OP_SETDISPATCH: {
        const upb_inttable *dispatch;
        memcpy(&dispatch, p, sizeof(void*));
        p += ptr_words;
        const upb_pbdecodermethod *method =
            (void *)((char *)dispatch -
                     offsetof(upb_pbdecodermethod, dispatch));
        fprintf(f, " %s", upb_msgdef_fullname(method->msg));
        break;
      }
      case OP_STARTMSG:
      case OP_ENDMSG:
      case OP_PUSHLENDELIM:
      case OP_PUSHTAGDELIM:
      case OP_POP:
      case OP_SETDELIM:
      case OP_HALT:
        break;
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
      case OP_STARTSEQ:
      case OP_ENDSEQ:
      case OP_STARTSUBMSG:
      case OP_ENDSUBMSG:
      case OP_STARTSTR:
      case OP_STRING:
      case OP_ENDSTR:
      case OP_SETGROUPNUM:
        fprintf(f, " %d", instr >> 8);
        break;
      case OP_SETBIGGROUPNUM:
        fprintf(f, " %d", *p++);
        break;
      case OP_CHECKDELIM:
      case OP_CALL:
      case OP_BRANCH:
        fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        break;
      case OP_TAG1:
      case OP_TAG2: {
        fprintf(f, " tag:0x%x", instr >> 16);
        if (getofs(instr)) {
          fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        }
        break;
      }
      case OP_TAGN: {
        uint64_t tag = *p++;
        tag |= (uint64_t)*p++ << 32;
        fprintf(f, " tag:0x%llx", (long long)tag);
        fprintf(f, " n:%d", instr >> 16);
        if (getofs(instr)) {
          fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        }
        break;
      }
    }
    fputs("\n", f);
  }
}

#endif

static uint64_t get_encoded_tag(const upb_fielddef *f, int wire_type) {
  uint32_t tag = (upb_fielddef_number(f) << 3) | wire_type;
  uint64_t encoded_tag = upb_vencode32(tag);
  // No tag should be greater than 5 bytes.
  assert(encoded_tag <= 0xffffffffff);
  return encoded_tag;
}

static void putchecktag(compiler *c, const upb_fielddef *f,
                        int wire_type, int dest) {
  uint64_t tag = get_encoded_tag(f, wire_type);
  switch (upb_value_size(tag)) {
    case 1:
      putop(c, OP_TAG1, dest, tag);
      break;
    case 2:
      putop(c, OP_TAG2, dest, tag);
      break;
    default:
      putop(c, OP_TAGN, dest, tag);
      break;
  }
}

static upb_selector_t getsel(const upb_fielddef *f, upb_handlertype_t type) {
  upb_selector_t selector;
  bool ok = upb_handlers_getselector(f, type, &selector);
  UPB_ASSERT_VAR(ok, ok);
  return selector;
}

// Marks the current bytecode position as the dispatch target for this message,
// field, and wire type.
//
static void dispatchtarget(compiler *c, upb_pbdecodermethod *method,
                           const upb_fielddef *f, int wire_type) {
  // Offset is relative to msg base.
  uint64_t ofs = pcofs(c) - method->base.ofs;
  uint32_t fn = upb_fielddef_number(f);
  upb_inttable *d = &method->dispatch;
  upb_value v;
  if (upb_inttable_remove(d, fn, &v)) {
    // TODO: prioritize based on packed setting in .proto file.
    uint64_t oldval = upb_value_getuint64(v);
    assert(((oldval >> 8) & 0xff) == 0);  // wt2 should not be set yet.
    upb_inttable_insert(d, fn, upb_value_uint64(oldval | (wire_type << 8)));
    upb_inttable_insert(d, fn + UPB_MAX_FIELDNUMBER, upb_value_uint64(ofs));
  } else {
    upb_inttable_insert(d, fn, upb_value_uint64((ofs << 16) | wire_type));
  }
}

static void putpush(compiler *c, const upb_fielddef *f) {
  if (upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_MESSAGE) {
    putop(c, OP_PUSHLENDELIM);
  } else {
    uint32_t fn = upb_fielddef_number(f);
    putop(c, OP_PUSHTAGDELIM);
    if (fn >= 1 << 24) {
      putop(c, OP_SETBIGGROUPNUM, fn);
    } else {
      putop(c, OP_SETGROUPNUM, fn);
    }
  }
}

static upb_pbdecodermethod *find_submethod(const compiler *c,
                                           const upb_pbdecodermethod *method,
                                           const upb_fielddef *f) {
  const void *key = method->dest_handlers ?
      (const void*)upb_handlers_getsubhandlers(method->dest_handlers, f) :
      (const void*)upb_downcast_msgdef(upb_fielddef_subdef(f));
  upb_value v;
  bool ok = upb_inttable_lookupptr(&c->plan->methods, key, &v);
  UPB_ASSERT_VAR(ok, ok);
  return upb_value_getptr(v);
}

// Adds bytecode for parsing the given message to the given decoderplan,
// while adding all dispatch targets to this message's dispatch table.
static void compile_method(compiler *c, upb_pbdecodermethod *method) {
  assert(method);

  // Symbolic names for our local labels.
  const int LABEL_LOOPSTART = 1;  // Top of a repeated field loop.
  const int LABEL_LOOPBREAK = 2;  // To jump out of a repeated loop
  const int LABEL_FIELD = 3;   // Jump backward to find the most recent field.
  const int LABEL_ENDMSG = 4;  // To reach the OP_ENDMSG instr for this msg.

  // Index is descriptor type.
  static const uint8_t native_wire_types[] = {
    UPB_WIRE_TYPE_END_GROUP,     // ENDGROUP
    UPB_WIRE_TYPE_64BIT,         // DOUBLE
    UPB_WIRE_TYPE_32BIT,         // FLOAT
    UPB_WIRE_TYPE_VARINT,        // INT64
    UPB_WIRE_TYPE_VARINT,        // UINT64
    UPB_WIRE_TYPE_VARINT,        // INT32
    UPB_WIRE_TYPE_64BIT,         // FIXED64
    UPB_WIRE_TYPE_32BIT,         // FIXED32
    UPB_WIRE_TYPE_VARINT,        // BOOL
    UPB_WIRE_TYPE_DELIMITED,     // STRING
    UPB_WIRE_TYPE_START_GROUP,   // GROUP
    UPB_WIRE_TYPE_DELIMITED,     // MESSAGE
    UPB_WIRE_TYPE_DELIMITED,     // BYTES
    UPB_WIRE_TYPE_VARINT,        // UINT32
    UPB_WIRE_TYPE_VARINT,        // ENUM
    UPB_WIRE_TYPE_32BIT,         // SFIXED32
    UPB_WIRE_TYPE_64BIT,         // SFIXED64
    UPB_WIRE_TYPE_VARINT,        // SINT32
    UPB_WIRE_TYPE_VARINT,        // SINT64
  };

  // Clear all entries in the dispatch table.
  upb_inttable_uninit(&method->dispatch);
  upb_inttable_init(&method->dispatch, UPB_CTYPE_UINT64);

 method->base.ofs = pcofs(c);
  putop(c, OP_SETDISPATCH, &method->dispatch);
  putop(c, OP_STARTMSG);
 label(c, LABEL_FIELD);
  upb_msg_iter i;
  for(upb_msg_begin(&i, method->msg); !upb_msg_done(&i); upb_msg_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    upb_descriptortype_t type = upb_fielddef_descriptortype(f);

    // From a decoding perspective, ENUM is the same as INT32.
    if (type == UPB_DESCRIPTOR_TYPE_ENUM)
      type = UPB_DESCRIPTOR_TYPE_INT32;

    label(c, LABEL_FIELD);

    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_MESSAGE: {
        const upb_pbdecodermethod *sub_m = find_submethod(c, method, f);
        int wire_type = (type == UPB_DESCRIPTOR_TYPE_MESSAGE) ?
            UPB_WIRE_TYPE_DELIMITED : UPB_WIRE_TYPE_START_GROUP;
        if (upb_fielddef_isseq(f)) {
          putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
          putchecktag(c, f, wire_type, LABEL_DISPATCH);
         dispatchtarget(c, method, f, wire_type);
          putop(c, OP_PUSHTAGDELIM);
          putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));
         label(c, LABEL_LOOPSTART);
          putpush(c, f);
          putop(c, OP_STARTSUBMSG, getsel(f, UPB_HANDLER_STARTSUBMSG));
          putop(c, OP_CALL, sub_m);
          putop(c, OP_POP);
          putop(c, OP_ENDSUBMSG, getsel(f, UPB_HANDLER_ENDSUBMSG));
          if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
            putop(c, OP_SETDELIM);
          }
          putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
          putchecktag(c, f, wire_type, LABEL_LOOPBREAK);
          putop(c, OP_BRANCH, -LABEL_LOOPSTART);
         label(c, LABEL_LOOPBREAK);
          putop(c, OP_POP);
          putop(c, OP_ENDSEQ, getsel(f, UPB_HANDLER_ENDSEQ));
        } else {
          putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
          putchecktag(c, f, wire_type, LABEL_DISPATCH);
         dispatchtarget(c, method, f, wire_type);
          putpush(c, f);
          putop(c, OP_STARTSUBMSG, getsel(f, UPB_HANDLER_STARTSUBMSG));
          putop(c, OP_CALL, sub_m);
          putop(c, OP_POP);
          putop(c, OP_ENDSUBMSG, getsel(f, UPB_HANDLER_ENDSUBMSG));
          if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
            putop(c, OP_SETDELIM);
          }
        }
        break;
      }
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        if (upb_fielddef_isseq(f)) {
          putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
          putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
         dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
          putop(c, OP_PUSHTAGDELIM);
          putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));
         label(c, LABEL_LOOPSTART);
          putop(c, OP_PUSHLENDELIM);
          putop(c, OP_STARTSTR, getsel(f, UPB_HANDLER_STARTSTR));
          putop(c, OP_STRING, getsel(f, UPB_HANDLER_STRING));
          putop(c, OP_POP);
          putop(c, OP_ENDSTR, getsel(f, UPB_HANDLER_ENDSTR));
          putop(c, OP_SETDELIM);
          putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
          putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_LOOPBREAK);
          putop(c, OP_BRANCH, -LABEL_LOOPSTART);
         label(c, LABEL_LOOPBREAK);
          putop(c, OP_POP);
          putop(c, OP_ENDSEQ, getsel(f, UPB_HANDLER_ENDSEQ));
        } else {
          putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
          putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
         dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
          putop(c, OP_PUSHLENDELIM);
          putop(c, OP_STARTSTR, getsel(f, UPB_HANDLER_STARTSTR));
          putop(c, OP_STRING, getsel(f, UPB_HANDLER_STRING));
          putop(c, OP_POP);
          putop(c, OP_ENDSTR, getsel(f, UPB_HANDLER_ENDSTR));
          putop(c, OP_SETDELIM);
        }
        break;
      default: {
        opcode parse_type = (opcode)type;
        assert((int)parse_type >= 0 && parse_type <= OP_MAX);
        upb_selector_t sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
        int wire_type = native_wire_types[upb_fielddef_descriptortype(f)];
        if (upb_fielddef_isseq(f)) {
          putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
          putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
         dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
          putop(c, OP_PUSHLENDELIM);
          putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));  // Packed
         label(c, LABEL_LOOPSTART);
          putop(c, parse_type, sel);
          putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
          putop(c, OP_BRANCH, -LABEL_LOOPSTART);
         dispatchtarget(c, method, f, wire_type);
          putop(c, OP_PUSHTAGDELIM);
          putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));  // Non-packed
         label(c, LABEL_LOOPSTART);
          putop(c, parse_type, sel);
          putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
          putchecktag(c, f, wire_type, LABEL_LOOPBREAK);
          putop(c, OP_BRANCH, -LABEL_LOOPSTART);
         label(c, LABEL_LOOPBREAK);
          putop(c, OP_POP);  // Packed and non-packed join.
          putop(c, OP_ENDSEQ, getsel(f, UPB_HANDLER_ENDSEQ));
          putop(c, OP_SETDELIM);  // Could remove for non-packed by dup ENDSEQ.
        } else {
          putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
          putchecktag(c, f, wire_type, LABEL_DISPATCH);
         dispatchtarget(c, method, f, wire_type);
          putop(c, parse_type, sel);
        }
      }
    }
  }
  // For now we just loop back to the last field of the message (or if none,
  // the DISPATCH opcode for the message.
  putop(c, OP_BRANCH, -LABEL_FIELD);
 label(c, LABEL_ENDMSG);
  putop(c, OP_ENDMSG);

  upb_inttable_compact(&method->dispatch);
}

// Populate "methods" with new upb_pbdecodermethod objects reachable from "md".
// "h" can be NULL, in which case the methods will not be statically bound to
// destination handlers.
//
// Returns the method for this msgdef/handlers.
//
// Note that there is a deep difference between keying the method table on
// upb_msgdef and keying it on upb_handlers.  Since upb_msgdef : upb_handlers
// can be 1:many, binding a handlers statically can result in *more* methods
// being generated than if the methods are dynamically-bound.
//
// On the other hand, if/when the optimization mentioned below is implemented,
// binding to a upb_handlers can result in *fewer* methods being generated if
// many of the submessages have no handlers bound to them.
static upb_pbdecodermethod *find_methods(compiler *c,
                                         const upb_msgdef *md,
                                         const upb_handlers *h) {
  const void *key = h ? (const void*)h : (const void*)md;
  upb_value v;
  if (upb_inttable_lookupptr(&c->plan->methods, key, &v))
    return upb_value_getptr(v);
  upb_pbdecodermethod *method = newmethod(md, h);
  // Takes ownership of method.
  upb_inttable_insertptr(&c->plan->methods, key, upb_value_ptr(method));

  upb_msg_iter i;
  for(upb_msg_begin(&i, md); !upb_msg_done(&i); upb_msg_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    if (upb_fielddef_type(f) != UPB_TYPE_MESSAGE)
      continue;
    const upb_handlers *sub_h = h ? upb_handlers_getsubhandlers(h, f) : NULL;

    if (h && !sub_h &&
        upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_MESSAGE) {
      // OPT: We could optimize away the sub-method, but would have to make sure
      // this field is compiled as a string instead of a submessage.
    }

    find_methods(c, upb_downcast_msgdef(upb_fielddef_subdef(f)), sub_h);
  }

  return method;
}

// (Re-)compile bytecode for all messages in "msgs", ensuring that the code
// for "md" is emitted first.  Overwrites any existing bytecode in "c".
static void compile_methods(compiler *c) {
  // Start over at the beginning of the bytecode.
  c->pc = c->plan->code;
  compile_method(c, c->plan->topmethod);

  upb_inttable_iter i;
  upb_inttable_begin(&i, &c->plan->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    if (method != c->plan->topmethod) {
      compile_method(c, method);
    }
  }
}


/* JIT setup. ******************************************************************/

#ifdef UPB_USE_JIT_X64

static void sethandlers(upb_pbdecoderplan *p, upb_handlers *h, bool allowjit) {
  p->jit_code = NULL;

  if (allowjit) {
    upb_pbdecoder_jit(p);  // Compile byte-code into machine code.
    upb_handlers_setstartstr(h, UPB_BYTESTREAM_BYTES, upb_pbdecoder_start, p,
                             freeplan);
    upb_handlers_setstring(h, UPB_BYTESTREAM_BYTES, p->jit_code, NULL, NULL);
    upb_handlers_setendstr(h, UPB_BYTESTREAM_BYTES, upb_pbdecoder_end, p, NULL);
  } else {
    set_bytecode_handlers(p, h);
  }
}

static bool bind_dynamic(bool allowjit) {
  // For the moment, JIT handlers always bind statically, but bytecode handlers
  // never do.
  return !allowjit;
}

#else  // UPB_USE_JIT_X64

static void sethandlers(upb_pbdecoderplan *p, upb_handlers *h, bool allowjit) {
  // No JIT compiled in; use bytecode handlers unconditionally.
  UPB_UNUSED(allowjit);
  set_bytecode_handlers(p, h);
}

static bool bind_dynamic(bool allowjit) {
  // Bytecode handlers never bind statically.
  UPB_UNUSED(allowjit);
  return true;
}

#endif  // UPB_USE_JIT_X64


/* Public interface ***********************************************************/

bool upb_pbdecoder_isdecoder(const upb_handlers *h) {
  return getdecoderplan(h) != NULL;
}

bool upb_pbdecoderplan_hasjitcode(const upb_pbdecoderplan *p) {
#ifdef UPB_USE_JIT_X64
  return p->jit_code != NULL;
#else
  UPB_UNUSED(p);
  return false;
#endif
}

bool upb_pbdecoder_hasjitcode(const upb_handlers *h) {
  const upb_pbdecoderplan *p = getdecoderplan(h);
  if (!p) return false;
  return upb_pbdecoderplan_hasjitcode(p);
}

uint32_t *upb_pbdecoderplan_codebase(const upb_pbdecoderplan *p) {
  return p->code;
}

upb_string_handler *upb_pbdecoderplan_jitcode(const upb_pbdecoderplan *p) {
#ifdef UPB_USE_JIT_X64
  return p->jit_code;
#else
  UPB_UNUSED(p);
  assert(false);
  return NULL;
#endif
}

const upb_handlers *upb_pbdecoder_getdesthandlers(const upb_handlers *h) {
  const upb_pbdecoderplan *p = getdecoderplan(h);
  if (!p) return NULL;
  return p->topmethod->dest_handlers;
}

const upb_handlers *upb_pbdecoder_gethandlers(const upb_handlers *dest,
                                              bool allowjit,
                                              const void *owner) {
  UPB_UNUSED(allowjit);
  assert(upb_handlers_isfrozen(dest));
  const upb_msgdef *md = upb_handlers_msgdef(dest);

  upb_pbdecoderplan *p = newplan();
  compiler *c = newcompiler(p);

  if (bind_dynamic(allowjit)) {
    // If binding dynamically, remove the reference against destination
    // handlers.
    dest = NULL;
  }

  p->topmethod = find_methods(c, md, dest);

  // We compile in two passes:
  // 1. all messages are assigned relative offsets from the beginning of the
  //    bytecode (saved in method->base).
  // 2. forwards OP_CALL instructions can be correctly linked since message
  //    offsets have been previously assigned.
  //
  // Could avoid the second pass by linking OP_CALL instructions somehow.
  compile_methods(c);
  compile_methods(c);
  p->code_end = c->pc;

#ifdef UPB_DUMP_BYTECODE
  FILE *f = fopen("/tmp/upb-bytecode", "wb");
  assert(f);
  dumpbc(p->code, p->code_end, stderr);
  dumpbc(p->code, p->code_end, f);
  fclose(f);
#endif

  upb_handlers *h = upb_handlers_new(
      UPB_BYTESTREAM, &upb_pbdecoder_frametype, owner);
  sethandlers(p, h, allowjit);

  freecompiler(c);

  return h;
}
