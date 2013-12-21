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

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif

#define MAXLABEL 5
#define EMPTYLABEL -1

static const void *methodkey(const upb_msgdef *md, const upb_handlers *h) {
  const void *ret = h ? (const void*)h : (const void*)md;
  assert(ret);
  return ret;
}


/* mgroup *********************************************************************/

static void freegroup(upb_refcounted *r) {
  mgroup *g = (mgroup*)r;
  upb_inttable_uninit(&g->methods);
#ifdef UPB_USE_JIT_X64
  upb_pbdecoder_freejit(g);
#endif
  free(g->bytecode);
  free(g);
}

static void visitgroup(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const mgroup *g = (const mgroup*)r;
  upb_inttable_iter i;
  upb_inttable_begin(&i, &g->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    visit(r, UPB_UPCAST(method), closure);
  }
}

mgroup *newgroup(const void *owner) {
  mgroup *g = malloc(sizeof(*g));
  static const struct upb_refcounted_vtbl vtbl = {visitgroup, freegroup};
  upb_refcounted_init(UPB_UPCAST(g), &vtbl, owner);
  upb_inttable_init(&g->methods, UPB_CTYPE_PTR);
  g->bytecode = NULL;
  g->bytecode_end = NULL;
  return g;
}


/* upb_pbdecodermethod ********************************************************/

static void freemethod(upb_refcounted *r) {
  upb_pbdecodermethod *method = (upb_pbdecodermethod*)r;
  upb_byteshandler_uninit(&method->input_handler_);

  if (method->dest_handlers_) {
    upb_handlers_unref(method->dest_handlers_, method);
  }

  upb_inttable_uninit(&method->dispatch);
  free(method);
}

static void visitmethod(const upb_refcounted *r, upb_refcounted_visit *visit,
                        void *closure) {
  const upb_pbdecodermethod *m = (const upb_pbdecodermethod*)r;
  visit(r, m->group, closure);
}

static upb_pbdecodermethod *newmethod(const upb_msgdef *msg,
                                      const upb_handlers *dest_handlers,
                                      mgroup *group,
                                      const void *key) {
  static const struct upb_refcounted_vtbl vtbl = {visitmethod, freemethod};
  upb_pbdecodermethod *ret = malloc(sizeof(*ret));
  upb_refcounted_init(UPB_UPCAST(ret), &vtbl, &ret);
  upb_byteshandler_init(&ret->input_handler_);

  // The method references the group and vice-versa, in a circular reference.
  upb_ref2(ret, group);
  upb_ref2(group, ret);
  upb_inttable_insertptr(&group->methods, key, upb_value_ptr(ret));  // Owns ref
  upb_refcounted_unref(UPB_UPCAST(ret), &ret);

  ret->group = UPB_UPCAST(group);
  ret->schema_ = msg;
  ret->dest_handlers_ = dest_handlers;
  ret->is_native_ = false;  // If we JIT, it will update this later.
  upb_inttable_init(&ret->dispatch, UPB_CTYPE_UINT64);

  if (ret->dest_handlers_) {
    upb_handlers_ref(ret->dest_handlers_, ret);
  }
  return ret;
}

void upb_pbdecodermethod_ref(const upb_pbdecodermethod *m, const void *owner) {
  upb_refcounted_ref(UPB_UPCAST(m), owner);
}

void upb_pbdecodermethod_unref(const upb_pbdecodermethod *m,
                               const void *owner) {
  upb_refcounted_unref(UPB_UPCAST(m), owner);
}

void upb_pbdecodermethod_donateref(const upb_pbdecodermethod *m,
                                   const void *from, const void *to) {
  upb_refcounted_donateref(UPB_UPCAST(m), from, to);
}

void upb_pbdecodermethod_checkref(const upb_pbdecodermethod *m,
                                  const void *owner) {
  upb_refcounted_checkref(UPB_UPCAST(m), owner);
}

const upb_msgdef *upb_pbdecodermethod_schema(const upb_pbdecodermethod *m) {
  return m->schema_;
}

const upb_handlers *upb_pbdecodermethod_desthandlers(
    const upb_pbdecodermethod *m) {
  return m->dest_handlers_;
}

const upb_byteshandler *upb_pbdecodermethod_inputhandler(
    const upb_pbdecodermethod *m) {
  return &m->input_handler_;
}

bool upb_pbdecodermethod_isnative(const upb_pbdecodermethod *m) {
  return m->is_native_;
}

const upb_pbdecodermethod *upb_pbdecodermethod_newfordesthandlers(
    const upb_handlers *dest, const void *owner) {
  upb_pbcodecache cache;
  upb_pbcodecache_init(&cache);
  const upb_pbdecodermethod *ret =
      upb_pbcodecache_getdecodermethodfordesthandlers(&cache, dest);
  upb_pbdecodermethod_ref(ret, owner);
  upb_pbcodecache_uninit(&cache);
  return ret;
}


/* compiler *******************************************************************/

// Data used only at compilation time.
typedef struct {
  mgroup *group;

  uint32_t *pc;
  int fwd_labels[MAXLABEL];
  int back_labels[MAXLABEL];
} compiler;

static compiler *newcompiler(mgroup *group) {
  compiler *ret = malloc(sizeof(*ret));
  ret->group = group;
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

static uint32_t pcofs(compiler *c) { return c->pc - c->group->bytecode; }

// Defines a local label at the current PC location.  All previous forward
// references are updated to point to this location.  The location is noted
// for any future backward references.
static void label(compiler *c, unsigned int label) {
  assert(label < MAXLABEL);
  int val = c->fwd_labels[label];
  uint32_t *codep = (val == EMPTYLABEL) ? NULL : c->group->bytecode + val;
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
    uint32_t from = (c->pc + 1) - c->group->bytecode;
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
  mgroup *g = c->group;
  if (c->pc == g->bytecode_end) {
    int ofs = pcofs(c);
    size_t oldsize = g->bytecode_end - g->bytecode;
    size_t newsize = UPB_MAX(oldsize * 2, 64);
    // TODO(haberman): handle OOM.
    g->bytecode = realloc(g->bytecode, newsize * sizeof(uint32_t));
    g->bytecode_end = g->bytecode + newsize;
    c->pc = g->bytecode + ofs;
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
      put32(c, op | (method->code_base.ofs - (pcofs(c) + 1)) << 8);
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
        fprintf(f, " %s", upb_msgdef_fullname(method->schema_));
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
  uint64_t ofs = pcofs(c) - method->code_base.ofs;
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
  const upb_handlers *sub = method->dest_handlers_ ?
      upb_handlers_getsubhandlers(method->dest_handlers_, f) : NULL;
  const void *key = methodkey(upb_downcast_msgdef(upb_fielddef_subdef(f)), sub);
  upb_value v;
  bool ok = upb_inttable_lookupptr(&c->group->methods, key, &v);
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

 method->code_base.ofs = pcofs(c);
  putop(c, OP_SETDISPATCH, &method->dispatch);
  putop(c, OP_STARTMSG);
 label(c, LABEL_FIELD);
  upb_msg_iter i;
  for(upb_msg_begin(&i, method->schema_); !upb_msg_done(&i); upb_msg_next(&i)) {
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
static void find_methods(compiler *c, const upb_msgdef *md,
                         const upb_handlers *h) {
  const void *key = methodkey(md, h);
  upb_value v;
  if (upb_inttable_lookupptr(&c->group->methods, key, &v))
    return;
  newmethod(md, h, c->group, key);

  // Find submethods.
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
}

// (Re-)compile bytecode for all messages in "msgs."
// Overwrites any existing bytecode in "c".
static void compile_methods(compiler *c) {
  // Start over at the beginning of the bytecode.
  c->pc = c->group->bytecode;

  upb_inttable_iter i;
  upb_inttable_begin(&i, &c->group->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    compile_method(c, method);
  }
}

static void set_bytecode_handlers(mgroup *g) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &g->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *m = upb_value_getptr(upb_inttable_iter_value(&i));

    m->code_base.ptr = g->bytecode + m->code_base.ofs;

    upb_byteshandler *h = &m->input_handler_;
    upb_byteshandler_setstartstr(h, upb_pbdecoder_startbc, m->code_base.ptr);
    upb_byteshandler_setstring(h, upb_pbdecoder_decode, g);
    upb_byteshandler_setendstr(h, upb_pbdecoder_end, m);
  }
}


/* JIT setup. ******************************************************************/

#ifdef UPB_USE_JIT_X64

static void sethandlers(mgroup *g, bool allowjit) {
  g->jit_code = NULL;
  if (allowjit) {
    // Compile byte-code into machine code, create handlers.
    upb_pbdecoder_jit(g);
  } else {
    set_bytecode_handlers(g);
  }
}

static bool bind_dynamic(bool allowjit) {
  // For the moment, JIT handlers always bind statically, but bytecode handlers
  // never do.
  return !allowjit;
}

#else  // UPB_USE_JIT_X64

static void sethandlers(mgroup *g, bool allowjit) {
  // No JIT compiled in; use bytecode handlers unconditionally.
  UPB_UNUSED(allowjit);
  set_bytecode_handlers(g);
}

static bool bind_dynamic(bool allowjit) {
  // Bytecode handlers never bind statically.
  UPB_UNUSED(allowjit);
  return true;
}

#endif  // UPB_USE_JIT_X64


// TODO(haberman): allow this to be constructed for an arbitrary set of dest
// handlers and other mgroups (but verify we have a transitive closure).
const mgroup *mgroup_new(const upb_handlers *dest, bool allowjit,
                         const void *owner) {
  UPB_UNUSED(allowjit);
  assert(upb_handlers_isfrozen(dest));
  const upb_msgdef *md = upb_handlers_msgdef(dest);

  mgroup *g = newgroup(owner);
  compiler *c = newcompiler(g);

  if (bind_dynamic(allowjit)) {
    // If binding dynamically, remove the reference against destination
    // handlers.
    dest = NULL;
  }

  find_methods(c, md, dest);

  // We compile in two passes:
  // 1. all messages are assigned relative offsets from the beginning of the
  //    bytecode (saved in method->code_base).
  // 2. forwards OP_CALL instructions can be correctly linked since message
  //    offsets have been previously assigned.
  //
  // Could avoid the second pass by linking OP_CALL instructions somehow.
  compile_methods(c);
  compile_methods(c);
  g->bytecode_end = c->pc;
  freecompiler(c);

#ifdef UPB_DUMP_BYTECODE
  FILE *f = fopen("/tmp/upb-bytecode", "wb");
  assert(f);
  dumpbc(g->bytecode, g->bytecode_end, stderr);
  dumpbc(g->bytecode, g->bytecode_end, f);
  fclose(f);
#endif

  sethandlers(g, allowjit);
  return g;
}


/* upb_pbcodecache ************************************************************/

void upb_pbcodecache_init(upb_pbcodecache *c) {
  upb_inttable_init(&c->groups, UPB_CTYPE_CONSTPTR);
  c->allow_jit_ = true;
}

void upb_pbcodecache_uninit(upb_pbcodecache *c) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &c->groups);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    const mgroup *group = upb_value_getconstptr(upb_inttable_iter_value(&i));
    upb_refcounted_unref(UPB_UPCAST(group), c);
  }
  upb_inttable_uninit(&c->groups);
}

bool upb_pbcodecache_allowjit(const upb_pbcodecache *c) {
  return c->allow_jit_;
}

bool upb_pbcodecache_setallowjit(upb_pbcodecache *c, bool allow) {
  if (upb_inttable_count(&c->groups) > 0)
    return false;
  c->allow_jit_ = allow;
  return true;
}

const upb_pbdecodermethod *upb_pbcodecache_getdecodermethodfordesthandlers(
    upb_pbcodecache *c, const upb_handlers *handlers) {
  // Right now we build a new DecoderMethod every time.
  // TODO(haberman): properly cache methods by their true key.
  const mgroup *g = mgroup_new(handlers, c->allow_jit_, c);
  upb_inttable_push(&c->groups, upb_value_constptr(g));

  const upb_msgdef *md = upb_handlers_msgdef(handlers);
  if (bind_dynamic(c->allow_jit_)) {
    handlers = NULL;
  }

  upb_value v;
  bool ok = upb_inttable_lookupptr(&g->methods, methodkey(md, handlers), &v);
  UPB_ASSERT_VAR(ok, ok);
  return upb_value_getptr(v);
}
