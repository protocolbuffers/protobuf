/*
** protobuf decoder bytecode compiler
**
** Code to compile a upb::Handlers into bytecode for decoding a protobuf
** according to that specific schema and destination handlers.
**
** Bytecode definition is in decoder.int.h.
*/

#include <stdarg.h>
#include "upb/pb/decoder.int.h"
#include "upb/pb/varint.int.h"

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif

#include "upb/port_def.inc"

#define MAXLABEL 5
#define EMPTYLABEL -1

/* upb_pbdecodermethod ********************************************************/

static void freemethod(upb_pbdecodermethod *method) {
  upb_inttable_uninit(&method->dispatch);
  upb_gfree(method);
}

static upb_pbdecodermethod *newmethod(const upb_handlers *dest_handlers,
                                      mgroup *group) {
  upb_pbdecodermethod *ret = upb_gmalloc(sizeof(*ret));
  upb_byteshandler_init(&ret->input_handler_);

  ret->group = group;
  ret->dest_handlers_ = dest_handlers;
  upb_inttable_init(&ret->dispatch, UPB_CTYPE_UINT64);

  return ret;
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


/* mgroup *********************************************************************/

static void freegroup(mgroup *g) {
  upb_inttable_iter i;

  upb_inttable_begin(&i, &g->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    freemethod(upb_value_getptr(upb_inttable_iter_value(&i)));
  }

  upb_inttable_uninit(&g->methods);
  upb_gfree(g->bytecode);
  upb_gfree(g);
}

mgroup *newgroup(void) {
  mgroup *g = upb_gmalloc(sizeof(*g));
  upb_inttable_init(&g->methods, UPB_CTYPE_PTR);
  g->bytecode = NULL;
  g->bytecode_end = NULL;
  return g;
}


/* bytecode compiler **********************************************************/

/* Data used only at compilation time. */
typedef struct {
  mgroup *group;

  uint32_t *pc;
  int fwd_labels[MAXLABEL];
  int back_labels[MAXLABEL];

  /* For fields marked "lazy", parse them lazily or eagerly? */
  bool lazy;
} compiler;

static compiler *newcompiler(mgroup *group, bool lazy) {
  compiler *ret = upb_gmalloc(sizeof(*ret));
  int i;

  ret->group = group;
  ret->lazy = lazy;
  for (i = 0; i < MAXLABEL; i++) {
    ret->fwd_labels[i] = EMPTYLABEL;
    ret->back_labels[i] = EMPTYLABEL;
  }
  return ret;
}

static void freecompiler(compiler *c) {
  upb_gfree(c);
}

const size_t ptr_words = sizeof(void*) / sizeof(uint32_t);

/* How many words an instruction is. */
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
    /* The "tag" instructions only have 8 bytes available for the jump target,
     * but that is ok because these opcodes only require short jumps. */
    case OP_TAG1:
    case OP_TAG2:
    case OP_TAGN:
      return false;
    default:
      UPB_ASSERT(false);
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
    *instruction = getop(*instruction) | (uint32_t)ofs << 8;
  } else {
    *instruction = (*instruction & ~0xff00) | ((ofs & 0xff) << 8);
  }
  UPB_ASSERT(getofs(*instruction) == ofs);  /* Would fail in cases of overflow. */
}

static uint32_t pcofs(compiler *c) {
  return (uint32_t)(c->pc - c->group->bytecode);
}

/* Defines a local label at the current PC location.  All previous forward
 * references are updated to point to this location.  The location is noted
 * for any future backward references. */
static void label(compiler *c, unsigned int label) {
  int val;
  uint32_t *codep;

  UPB_ASSERT(label < MAXLABEL);
  val = c->fwd_labels[label];
  codep = (val == EMPTYLABEL) ? NULL : c->group->bytecode + val;
  while (codep) {
    int ofs = getofs(*codep);
    setofs(codep, (int32_t)(c->pc - codep - instruction_len(*codep)));
    codep = ofs ? codep + ofs : NULL;
  }
  c->fwd_labels[label] = EMPTYLABEL;
  c->back_labels[label] = pcofs(c);
}

/* Creates a reference to a numbered label; either a forward reference
 * (positive arg) or backward reference (negative arg).  For forward references
 * the value returned now is actually a "next" pointer into a linked list of all
 * instructions that use this label and will be patched later when the label is
 * defined with label().
 *
 * The returned value is the offset that should be written into the instruction.
 */
static int32_t labelref(compiler *c, int label) {
  UPB_ASSERT(label < MAXLABEL);
  if (label == LABEL_DISPATCH) {
    /* No resolving required. */
    return 0;
  } else if (label < 0) {
    /* Backward local label.  Relative to the next instruction. */
    uint32_t from = (uint32_t)((c->pc + 1) - c->group->bytecode);
    return c->back_labels[-label] - from;
  } else {
    /* Forward local label: prepend to (possibly-empty) linked list. */
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
    /* TODO(haberman): handle OOM. */
    g->bytecode = upb_grealloc(g->bytecode, oldsize * sizeof(uint32_t),
                                            newsize * sizeof(uint32_t));
    g->bytecode_end = g->bytecode + newsize;
    c->pc = g->bytecode + ofs;
  }
  *c->pc++ = v;
}

static void putop(compiler *c, int op, ...) {
  va_list ap;
  va_start(ap, op);

  switch (op) {
    case OP_SETDISPATCH: {
      uintptr_t ptr = (uintptr_t)va_arg(ap, void*);
      put32(c, OP_SETDISPATCH);
      put32(c, (uint32_t)ptr);
      if (sizeof(uintptr_t) > sizeof(uint32_t))
        put32(c, (uint64_t)ptr >> 32);
      break;
    }
    case OP_STARTMSG:
    case OP_ENDMSG:
    case OP_PUSHLENDELIM:
    case OP_POP:
    case OP_SETDELIM:
    case OP_HALT:
    case OP_RET:
    case OP_DISPATCH:
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
    case OP_ENDSEQ:
    case OP_STARTSUBMSG:
    case OP_ENDSUBMSG:
    case OP_STARTSTR:
    case OP_STRING:
    case OP_ENDSTR:
    case OP_PUSHTAGDELIM:
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
      uint32_t instruction = (uint32_t)(op | (tag << 16));
      UPB_ASSERT(tag <= 0xffff);
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
      put32(c, (uint32_t)tag);
      put32(c, tag >> 32);
      break;
    }
  }

  va_end(ap);
}

#if defined(UPB_DUMP_BYTECODE)

const char *upb_pbdecoder_getopname(unsigned int op) {
#define QUOTE(x) #x
#define EXPAND_AND_QUOTE(x) QUOTE(x)
#define OPNAME(x) OP_##x
#define OP(x) case OPNAME(x): return EXPAND_AND_QUOTE(OPNAME(x));
#define T(x) OP(PARSE_##x)
  /* Keep in sync with list in decoder.int.h. */
  switch ((opcode)op) {
    T(DOUBLE) T(FLOAT) T(INT64) T(UINT64) T(INT32) T(FIXED64) T(FIXED32)
    T(BOOL) T(UINT32) T(SFIXED32) T(SFIXED64) T(SINT32) T(SINT64)
    OP(STARTMSG) OP(ENDMSG) OP(STARTSEQ) OP(ENDSEQ) OP(STARTSUBMSG)
    OP(ENDSUBMSG) OP(STARTSTR) OP(STRING) OP(ENDSTR) OP(CALL) OP(RET)
    OP(PUSHLENDELIM) OP(PUSHTAGDELIM) OP(SETDELIM) OP(CHECKDELIM)
    OP(BRANCH) OP(TAG1) OP(TAG2) OP(TAGN) OP(SETDISPATCH) OP(POP)
    OP(SETBIGGROUPNUM) OP(DISPATCH) OP(HALT)
  }
  return "<unknown op>";
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
        fprintf(f, " %s", upb_msgdef_fullname(
                              upb_handlers_msgdef(method->dest_handlers_)));
        break;
      }
      case OP_DISPATCH:
      case OP_STARTMSG:
      case OP_ENDMSG:
      case OP_PUSHLENDELIM:
      case OP_POP:
      case OP_SETDELIM:
      case OP_HALT:
      case OP_RET:
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
      case OP_PUSHTAGDELIM:
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
  /* No tag should be greater than 5 bytes. */
  UPB_ASSERT(encoded_tag <= 0xffffffffff);
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
  UPB_ASSERT(ok);
  return selector;
}

/* Takes an existing, primary dispatch table entry and repacks it with a
 * different alternate wire type.  Called when we are inserting a secondary
 * dispatch table entry for an alternate wire type. */
static uint64_t repack(uint64_t dispatch, int new_wt2) {
  uint64_t ofs;
  uint8_t wt1;
  uint8_t old_wt2;
  upb_pbdecoder_unpackdispatch(dispatch, &ofs, &wt1, &old_wt2);
  UPB_ASSERT(old_wt2 == NO_WIRE_TYPE);  /* wt2 should not be set yet. */
  return upb_pbdecoder_packdispatch(ofs, wt1, new_wt2);
}

/* Marks the current bytecode position as the dispatch target for this message,
 * field, and wire type. */
static void dispatchtarget(compiler *c, upb_pbdecodermethod *method,
                           const upb_fielddef *f, int wire_type) {
  /* Offset is relative to msg base. */
  uint64_t ofs = pcofs(c) - method->code_base.ofs;
  uint32_t fn = upb_fielddef_number(f);
  upb_inttable *d = &method->dispatch;
  upb_value v;
  if (upb_inttable_remove(d, fn, &v)) {
    /* TODO: prioritize based on packed setting in .proto file. */
    uint64_t repacked = repack(upb_value_getuint64(v), wire_type);
    upb_inttable_insert(d, fn, upb_value_uint64(repacked));
    upb_inttable_insert(d, fn + UPB_MAX_FIELDNUMBER, upb_value_uint64(ofs));
  } else {
    uint64_t val = upb_pbdecoder_packdispatch(ofs, wire_type, NO_WIRE_TYPE);
    upb_inttable_insert(d, fn, upb_value_uint64(val));
  }
}

static void putpush(compiler *c, const upb_fielddef *f) {
  if (upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_MESSAGE) {
    putop(c, OP_PUSHLENDELIM);
  } else {
    uint32_t fn = upb_fielddef_number(f);
    if (fn >= 1 << 24) {
      putop(c, OP_PUSHTAGDELIM, 0);
      putop(c, OP_SETBIGGROUPNUM, fn);
    } else {
      putop(c, OP_PUSHTAGDELIM, fn);
    }
  }
}

static upb_pbdecodermethod *find_submethod(const compiler *c,
                                           const upb_pbdecodermethod *method,
                                           const upb_fielddef *f) {
  const upb_handlers *sub =
      upb_handlers_getsubhandlers(method->dest_handlers_, f);
  upb_value v;
  return upb_inttable_lookupptr(&c->group->methods, sub, &v)
             ? upb_value_getptr(v)
             : NULL;
}

static void putsel(compiler *c, opcode op, upb_selector_t sel,
                   const upb_handlers *h) {
  if (upb_handlers_gethandler(h, sel, NULL)) {
    putop(c, op, sel);
  }
}

/* Puts an opcode to call a callback, but only if a callback actually exists for
 * this field and handler type. */
static void maybeput(compiler *c, opcode op, const upb_handlers *h,
                     const upb_fielddef *f, upb_handlertype_t type) {
  putsel(c, op, getsel(f, type), h);
}

static bool haslazyhandlers(const upb_handlers *h, const upb_fielddef *f) {
  if (!upb_fielddef_lazy(f))
    return false;

  return upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_STARTSTR), NULL) ||
         upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_STRING), NULL) ||
         upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_ENDSTR), NULL);
}


/* bytecode compiler code generation ******************************************/

/* Symbolic names for our local labels. */
#define LABEL_LOOPSTART 1  /* Top of a repeated field loop. */
#define LABEL_LOOPBREAK 2  /* To jump out of a repeated loop */
#define LABEL_FIELD     3  /* Jump backward to find the most recent field. */
#define LABEL_ENDMSG    4  /* To reach the OP_ENDMSG instr for this msg. */

/* Generates bytecode to parse a single non-lazy message field. */
static void generate_msgfield(compiler *c, const upb_fielddef *f,
                              upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);
  const upb_pbdecodermethod *sub_m = find_submethod(c, method, f);
  int wire_type;

  if (!sub_m) {
    /* Don't emit any code for this field at all; it will be parsed as an
     * unknown field.
     *
     * TODO(haberman): we should change this to parse it as a string field
     * instead.  It will probably be faster, but more importantly, once we
     * start vending unknown fields, a field shouldn't be treated as unknown
     * just because it doesn't have subhandlers registered. */
    return;
  }

  label(c, LABEL_FIELD);

  wire_type =
      (upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_MESSAGE)
          ? UPB_WIRE_TYPE_DELIMITED
          : UPB_WIRE_TYPE_START_GROUP;

  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));
   label(c, LABEL_LOOPSTART);
    putpush(c, f);
    putop(c, OP_STARTSUBMSG, getsel(f, UPB_HANDLER_STARTSUBMSG));
    putop(c, OP_CALL, sub_m);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSUBMSG, h, f, UPB_HANDLER_ENDSUBMSG);
    if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
      putop(c, OP_SETDELIM);
    }
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, wire_type, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putpush(c, f);
    putop(c, OP_STARTSUBMSG, getsel(f, UPB_HANDLER_STARTSUBMSG));
    putop(c, OP_CALL, sub_m);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSUBMSG, h, f, UPB_HANDLER_ENDSUBMSG);
    if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
      putop(c, OP_SETDELIM);
    }
  }
}

/* Generates bytecode to parse a single string or lazy submessage field. */
static void generate_delimfield(compiler *c, const upb_fielddef *f,
                                upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);

  label(c, LABEL_FIELD);
  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));
   label(c, LABEL_LOOPSTART);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSTR, getsel(f, UPB_HANDLER_STARTSTR));
    /* Need to emit even if no handler to skip past the string. */
    putop(c, OP_STRING, getsel(f, UPB_HANDLER_STRING));
    maybeput(c, OP_ENDSTR, h, f, UPB_HANDLER_ENDSTR);
    putop(c, OP_POP);
    putop(c, OP_SETDELIM);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSTR, getsel(f, UPB_HANDLER_STARTSTR));
    putop(c, OP_STRING, getsel(f, UPB_HANDLER_STRING));
    maybeput(c, OP_ENDSTR, h, f, UPB_HANDLER_ENDSTR);
    putop(c, OP_POP);
    putop(c, OP_SETDELIM);
  }
}

/* Generates bytecode to parse a single primitive field. */
static void generate_primitivefield(compiler *c, const upb_fielddef *f,
                                    upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);
  upb_descriptortype_t descriptor_type = upb_fielddef_descriptortype(f);
  opcode parse_type;
  upb_selector_t sel;
  int wire_type;

  label(c, LABEL_FIELD);

  /* From a decoding perspective, ENUM is the same as INT32. */
  if (descriptor_type == UPB_DESCRIPTOR_TYPE_ENUM)
    descriptor_type = UPB_DESCRIPTOR_TYPE_INT32;

  parse_type = (opcode)descriptor_type;

  /* TODO(haberman): generate packed or non-packed first depending on "packed"
   * setting in the fielddef.  This will favor (in speed) whichever was
   * specified. */

  UPB_ASSERT((int)parse_type >= 0 && parse_type <= OP_MAX);
  sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  wire_type = upb_pb_native_wire_types[upb_fielddef_descriptortype(f)];
  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));  /* Packed */
   label(c, LABEL_LOOPSTART);
    putop(c, parse_type, sel);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   dispatchtarget(c, method, f, wire_type);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));  /* Non-packed */
   label(c, LABEL_LOOPSTART);
    putop(c, parse_type, sel);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, wire_type, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);  /* Packed and non-packed join. */
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
    putop(c, OP_SETDELIM);  /* Could remove for non-packed by dup ENDSEQ. */
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putop(c, parse_type, sel);
  }
}

/* Adds bytecode for parsing the given message to the given decoderplan,
 * while adding all dispatch targets to this message's dispatch table. */
static void compile_method(compiler *c, upb_pbdecodermethod *method) {
  const upb_handlers *h;
  const upb_msgdef *md;
  uint32_t* start_pc;
  int i, n;
  upb_value val;

  UPB_ASSERT(method);

  /* Clear all entries in the dispatch table. */
  upb_inttable_uninit(&method->dispatch);
  upb_inttable_init(&method->dispatch, UPB_CTYPE_UINT64);

  h = upb_pbdecodermethod_desthandlers(method);
  md = upb_handlers_msgdef(h);

 method->code_base.ofs = pcofs(c);
  putop(c, OP_SETDISPATCH, &method->dispatch);
  putsel(c, OP_STARTMSG, UPB_STARTMSG_SELECTOR, h);
 label(c, LABEL_FIELD);
  start_pc = c->pc;
  n = upb_msgdef_fieldcount(md);
  for(i = 0; i < n; i++) {
    const upb_fielddef *f = upb_msgdef_field(md, i);
    upb_fieldtype_t type = upb_fielddef_type(f);

    if (type == UPB_TYPE_MESSAGE && !(haslazyhandlers(h, f) && c->lazy)) {
      generate_msgfield(c, f, method);
    } else if (type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES ||
               type == UPB_TYPE_MESSAGE) {
      generate_delimfield(c, f, method);
    } else {
      generate_primitivefield(c, f, method);
    }
  }

  /* If there were no fields, or if no handlers were defined, we need to
   * generate a non-empty loop body so that we can at least dispatch for unknown
   * fields and check for the end of the message. */
  if (c->pc == start_pc) {
    /* Check for end-of-message. */
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    /* Unconditionally dispatch. */
    putop(c, OP_DISPATCH, 0);
  }

  /* For now we just loop back to the last field of the message (or if none,
   * the DISPATCH opcode for the message). */
  putop(c, OP_BRANCH, -LABEL_FIELD);

  /* Insert both a label and a dispatch table entry for this end-of-msg. */
 label(c, LABEL_ENDMSG);
  val = upb_value_uint64(pcofs(c) - method->code_base.ofs);
  upb_inttable_insert(&method->dispatch, DISPATCH_ENDMSG, val);

  putsel(c, OP_ENDMSG, UPB_ENDMSG_SELECTOR, h);
  putop(c, OP_RET);

  upb_inttable_compact(&method->dispatch);
}

/* Populate "methods" with new upb_pbdecodermethod objects reachable from "h".
 * Returns the method for these handlers.
 *
 * Generates a new method for every destination handlers reachable from "h". */
static void find_methods(compiler *c, const upb_handlers *h) {
  upb_value v;
  int i, n;
  const upb_msgdef *md;
  upb_pbdecodermethod *method;

  if (upb_inttable_lookupptr(&c->group->methods, h, &v))
    return;

  method = newmethod(h, c->group);
  upb_inttable_insertptr(&c->group->methods, h, upb_value_ptr(method));

  /* Find submethods. */
  md = upb_handlers_msgdef(h);
  n = upb_msgdef_fieldcount(md);
  for (i = 0; i < n; i++) {
    const upb_fielddef *f = upb_msgdef_field(md, i);
    const upb_handlers *sub_h;
    if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE &&
        (sub_h = upb_handlers_getsubhandlers(h, f)) != NULL) {
      /* We only generate a decoder method for submessages with handlers.
       * Others will be parsed as unknown fields. */
      find_methods(c, sub_h);
    }
  }
}

/* (Re-)compile bytecode for all messages in "msgs."
 * Overwrites any existing bytecode in "c". */
static void compile_methods(compiler *c) {
  upb_inttable_iter i;

  /* Start over at the beginning of the bytecode. */
  c->pc = c->group->bytecode;

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
    upb_byteshandler *h = &m->input_handler_;

    m->code_base.ptr = g->bytecode + m->code_base.ofs;

    upb_byteshandler_setstartstr(h, upb_pbdecoder_startbc, m->code_base.ptr);
    upb_byteshandler_setstring(h, upb_pbdecoder_decode, g);
    upb_byteshandler_setendstr(h, upb_pbdecoder_end, m);
  }
}


/* TODO(haberman): allow this to be constructed for an arbitrary set of dest
 * handlers and other mgroups (but verify we have a transitive closure). */
const mgroup *mgroup_new(const upb_handlers *dest, bool lazy) {
  mgroup *g;
  compiler *c;

  g = newgroup();
  c = newcompiler(g, lazy);
  find_methods(c, dest);

  /* We compile in two passes:
   * 1. all messages are assigned relative offsets from the beginning of the
   *    bytecode (saved in method->code_base).
   * 2. forwards OP_CALL instructions can be correctly linked since message
   *    offsets have been previously assigned.
   *
   * Could avoid the second pass by linking OP_CALL instructions somehow. */
  compile_methods(c);
  compile_methods(c);
  g->bytecode_end = c->pc;
  freecompiler(c);

#ifdef UPB_DUMP_BYTECODE
  {
    FILE *f = fopen("/tmp/upb-bytecode", "w");
    UPB_ASSERT(f);
    dumpbc(g->bytecode, g->bytecode_end, stderr);
    dumpbc(g->bytecode, g->bytecode_end, f);
    fclose(f);

    f = fopen("/tmp/upb-bytecode.bin", "wb");
    UPB_ASSERT(f);
    fwrite(g->bytecode, 1, g->bytecode_end - g->bytecode, f);
    fclose(f);
  }
#endif

  set_bytecode_handlers(g);
  return g;
}


/* upb_pbcodecache ************************************************************/

upb_pbcodecache *upb_pbcodecache_new(upb_handlercache *dest) {
  upb_pbcodecache *c = upb_gmalloc(sizeof(*c));

  if (!c) return NULL;

  c->dest = dest;
  c->lazy = false;

  c->arena = upb_arena_new();
  if (!upb_inttable_init(&c->groups, UPB_CTYPE_CONSTPTR)) return NULL;

  return c;
}

void upb_pbcodecache_free(upb_pbcodecache *c) {
  upb_inttable_iter i;

  upb_inttable_begin(&i, &c->groups);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_value val = upb_inttable_iter_value(&i);
    freegroup((void*)upb_value_getconstptr(val));
  }

  upb_inttable_uninit(&c->groups);
  upb_arena_free(c->arena);
  upb_gfree(c);
}

void upb_pbdecodermethodopts_setlazy(upb_pbcodecache *c, bool lazy) {
  UPB_ASSERT(upb_inttable_count(&c->groups) == 0);
  c->lazy = lazy;
}

const upb_pbdecodermethod *upb_pbcodecache_get(upb_pbcodecache *c,
                                               const upb_msgdef *md) {
  upb_value v;
  bool ok;
  const upb_handlers *h;
  const mgroup *g;

  h = upb_handlercache_get(c->dest, md);
  if (upb_inttable_lookupptr(&c->groups, md, &v)) {
    g = upb_value_getconstptr(v);
  } else {
    g = mgroup_new(h, c->lazy);
    ok = upb_inttable_insertptr(&c->groups, md, upb_value_constptr(g));
    UPB_ASSUME(ok);
  }

  ok = upb_inttable_lookupptr(&g->methods, h, &v);
  UPB_ASSUME(ok);
  return upb_value_getptr(v);
}
