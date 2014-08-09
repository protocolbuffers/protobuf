
#include "ruby.h"
#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb/shim/shim.h"
#include "upb/symtab.h"

static VALUE cMessageDef;
static VALUE cMessage;

// Wrapper around a upb_msgdef.
typedef struct {
  // The msgdef for this message, and a DecoderMethod to parse protobufs and
  // fill a message.
  //
  // We own refs on both of these.
  const upb_msgdef *md;
  const upb_pbdecodermethod *fill_method;

  size_t size;
  uint32_t *field_offsets;
} rb_msgdef;

// Ruby message object.
// This will be sized according to what fields are actually present.
typedef struct {
  union u {
    VALUE rbmsgdef;
    char data[1];
  } data;
} rb_msg;

#define DEREF(msg, ofs, type) *(type*)(&msg->data.data[ofs])

static void symtab_free(void *md) {
  upb_symtab_unref(md, UPB_UNTRACKED_REF);
}

void rupb_checkstatus(upb_status *s) {
  if (!upb_ok(s)) {
    rb_raise(rb_eRuntimeError, "%s", upb_status_errmsg(s));
  }
}

/* handlers *******************************************************************/

// These are handlers for populating a Ruby protobuf message when parsing.

static size_t strhandler(void *closure, const void *hd, const char *str,
                         size_t len, const upb_bufhandle *handle) {
  rb_msg *msg = closure;
  const size_t *ofs = hd;
  DEREF(msg, *ofs, VALUE) = rb_str_new(str, len);
  return len;
}

static const void *newhandlerdata(upb_handlers *h, uint32_t ofs) {
  size_t *hd_ofs = ALLOC(size_t);
  *hd_ofs = ofs;
  upb_handlers_addcleanup(h, hd_ofs, free);
  return hd_ofs;
}

static void add_handlers_for_message(const void *closure, upb_handlers *h) {
  // XXX: Doesn't support submessages properly yet.
  const rb_msgdef *rmd = closure;
  upb_msg_iter i;
  for (upb_msg_begin(&i, rmd->md); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);

    if (upb_fielddef_isseq(f)) {
      rb_raise(rb_eRuntimeError, "Doesn't support repeated fields yet.");
    }

    size_t ofs = rmd->field_offsets[upb_fielddef_index(f)];

    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_BOOL:
      case UPB_TYPE_INT32:
      case UPB_TYPE_UINT32:
      case UPB_TYPE_ENUM:
      case UPB_TYPE_FLOAT:
      case UPB_TYPE_INT64:
      case UPB_TYPE_UINT64:
      case UPB_TYPE_DOUBLE:
        upb_shim_set(h, f, ofs, -1);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES: {
        upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
        upb_handlerattr_sethandlerdata(&attr, newhandlerdata(h, ofs));
        // XXX: does't currently handle split buffers.
        upb_handlers_setstring(h, f, strhandler, &attr);
        upb_handlerattr_uninit(&attr);
        break;
      }
      case UPB_TYPE_MESSAGE:
        rb_raise(rb_eRuntimeError, "Doesn't support submessages yet.");
        break;
    }
  }
}

// Creates upb handlers for populating a message.
static const upb_handlers *new_fill_handlers(const rb_msgdef *rmd,
                                             const void *owner) {
  return upb_handlers_newfrozen(rmd->md, owner, add_handlers_for_message, rmd);
}

// General alignment rules are that each type needs to be stored at an address
// that is a multiple of its size.
static size_t align_up(size_t val, size_t align) {
  return val % align == 0 ? val : val + align - (val % align);
}

// Byte size to store each upb type.
static size_t rupb_sizeof(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_BOOL:
      return 1;
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_ENUM:
    case UPB_TYPE_FLOAT:
      return 4;
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_DOUBLE:
      return 8;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      return sizeof(VALUE);
  }
  assert(false);
}

/* msg ************************************************************************/

static void msg_free(void *msg) {
  free(msg);
}

// Invoked by the Ruby GC whenever it is doing a mark-and-sweep.
static void msg_mark(void *p) {
  rb_msg *msg = p;
  rb_msgdef *rmd;
  Data_Get_Struct(msg->data.rbmsgdef, rb_msgdef, rmd);

  // Mark the msgdef to keep it alive.
  rb_gc_mark(msg->data.rbmsgdef);

  // We need to mark all references to other Ruby values: strings, arrays, and
  // submessages that we point to.  Only strings are implemented so far.
  upb_msg_iter i;
  for (upb_msg_begin(&i, rmd->md); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    if (upb_fielddef_isstring(f)) {
      size_t ofs = rmd->field_offsets[upb_fielddef_index(f)];
      rb_gc_mark(DEREF(msg, ofs, VALUE));
    }
  }
}

static VALUE msg_new(VALUE msgdef) {
  const rb_msgdef *rmd;
  Data_Get_Struct(msgdef, rb_msgdef, rmd);

  rb_msg *msg = (rb_msg*)ALLOC_N(char, rmd->size);
  memset(msg, 0, rmd->size);
  msg->data.rbmsgdef = msgdef;

  VALUE ret = Data_Wrap_Struct(cMessage, msg_mark, msg_free, msg);
  return ret;
}

static const upb_fielddef *lookup_field(rb_msg *msg, const char *field,
                                        size_t *ofs) {
  const rb_msgdef *rmd;
  Data_Get_Struct(msg->data.rbmsgdef, rb_msgdef, rmd);
  const upb_fielddef *f = upb_msgdef_ntof(rmd->md, field);
  if (!f) {
    rb_raise(rb_eArgError, "No such field: %s", field);
  }
  *ofs = rmd->field_offsets[upb_fielddef_index(f)];
  return f;
}

static VALUE msg_setter(rb_msg *msg, VALUE field, VALUE val) {
  size_t ofs;
  char *fieldp = RSTRING_PTR(field);
  size_t field_last = RSTRING_LEN(field) - 1;

  // fieldp is a string like "id=".  But we want to look up "id".
  // We take the liberty of temporarily setting the "=" to NULL.
  assert(fieldp[field_last] == '=');
  fieldp[field_last] = '\0';
  const upb_fielddef *f = lookup_field(msg, fieldp, &ofs);
  fieldp[field_last] = '=';

  // Possibly introduce stricter type checking.
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_FLOAT:   DEREF(msg, ofs, float) = NUM2DBL(val);
    case UPB_TYPE_DOUBLE:  DEREF(msg, ofs, double) = NUM2DBL(val);
    case UPB_TYPE_BOOL:    DEREF(msg, ofs, bool) = RTEST(val);
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:   DEREF(msg, ofs, VALUE) = val;
    case UPB_TYPE_MESSAGE: return Qnil;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:   DEREF(msg, ofs, int32_t) = NUM2INT(val);
    case UPB_TYPE_UINT32:  DEREF(msg, ofs, uint32_t) = NUM2LONG(val);
    case UPB_TYPE_INT64:   DEREF(msg, ofs, int64_t) = NUM2LONG(val);
    case UPB_TYPE_UINT64:  DEREF(msg, ofs, uint64_t) = NUM2ULL(val);
  }

  return val;
}

static VALUE msg_getter(rb_msg *msg, VALUE field) {
  size_t ofs;
  const upb_fielddef *f = lookup_field(msg, RSTRING_PTR(field), &ofs);

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_FLOAT:   return rb_float_new(DEREF(msg, ofs, float));
    case UPB_TYPE_DOUBLE:  return rb_float_new(DEREF(msg, ofs, double));
    case UPB_TYPE_BOOL:    return DEREF(msg, ofs, bool) ? Qtrue : Qfalse;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:   return DEREF(msg, ofs, VALUE);
    case UPB_TYPE_MESSAGE: return Qnil;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:   return INT2NUM(DEREF(msg, ofs, int32_t));
    case UPB_TYPE_UINT32:  return LONG2NUM(DEREF(msg, ofs, uint32_t));
    case UPB_TYPE_INT64:   return LONG2NUM(DEREF(msg, ofs, int64_t));
    case UPB_TYPE_UINT64:  return ULL2NUM(DEREF(msg, ofs, uint64_t));
  }

  rb_bug("Unexpected type");
}

static VALUE msg_accessor(int argc, VALUE *argv, VALUE obj) {
  rb_msg *msg;
  Data_Get_Struct(obj, rb_msg, msg);

  // method_missing protocol: (method [, arg1, arg2, ...])
  assert(argc >= 1 && SYMBOL_P(argv[0]));
  VALUE method = rb_id2str(SYM2ID(argv[0]));
  const char *method_str = RSTRING_PTR(method);
  size_t method_len = RSTRING_LEN(method);

  if (method_str[method_len - 1] == '=') {
    // Call was:
    //   foo.bar = x
    //
    // Ruby should guarantee that we have exactly one more argument (x)
    assert(argc == 2);
    return msg_setter(msg, method, argv[1]);
  } else {
    // Call was:
    //   foo.bar
    //
    // ...but may have had arguments. We want to disallow arguments.
    if (argc > 1) {
      rb_raise(rb_eArgError, "Accessor %s takes no arguments", method_str);
    }
    return msg_getter(msg, method);
  }
}

/* msgdef *********************************************************************/

static void msgdef_free(void *_rmd) {
  rb_msgdef *rmd = _rmd;
  upb_msgdef_unref(rmd->md, &rmd->md);
  if (rmd->fill_method) {
    upb_pbdecodermethod_unref(rmd->fill_method, &rmd->fill_method);
  }
  free(rmd->field_offsets);
}

const upb_pbdecodermethod *new_fillmsg_decodermethod(const rb_msgdef *rmd,
                                                     const void *owner) {
  const upb_handlers *fill_handlers = new_fill_handlers(rmd, &fill_handlers);
  upb_pbdecodermethodopts opts;
  upb_pbdecodermethodopts_init(&opts, fill_handlers);

  const upb_pbdecodermethod *ret = upb_pbdecodermethod_new(&opts, owner);
  upb_handlers_unref(fill_handlers, &fill_handlers);
  return ret;
}

// Calculates offsets for each field.
//
// This lets us pack protos like structs instead of storing them like
// dictionaries.  This speeds up a parsing a lot and also saves memory
// (unless messages are very sparse).
static void assign_offsets(rb_msgdef *rmd) {
  size_t ofs = sizeof(rb_msg);  // Msg starts with predeclared members.
  upb_msg_iter i;
  for (upb_msg_begin(&i, rmd->md); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    size_t field_size = rupb_sizeof(upb_fielddef_type(f));
    ofs = align_up(ofs, field_size);  // Align field properly.
    rmd->field_offsets[upb_fielddef_index(f)] = ofs;
    ofs += field_size;
  }
  rmd->size = ofs;
}

// Constructs a new Ruby wrapper object around the given msgdef.
static VALUE make_msgdef(const upb_msgdef *md) {
  rb_msgdef *rmd;
  VALUE ret = Data_Make_Struct(cMessageDef, rb_msgdef, NULL, msgdef_free, rmd);

  upb_msgdef_ref(md, &rmd->md);

  rmd->md = md;
  rmd->field_offsets = ALLOC_N(uint32_t, upb_msgdef_numfields(md));
  rmd->fill_method = NULL;

  assign_offsets(rmd);

  rmd->fill_method = new_fillmsg_decodermethod(rmd, &rmd->fill_method);

  return ret;
}

// Loads a descriptor and constructs a MessageDef to the named message.
static VALUE msgdef_load(VALUE klass, VALUE descriptor, VALUE message_name) {
  upb_symtab *symtab = upb_symtab_new(UPB_UNTRACKED_REF);

  // Wrap the symtab in a Ruby object so it gets GC'd.
  // In a real wrapper we would wrap this object more fully (ie. expose its
  // methods to Ruby callers).
  Data_Wrap_Struct(rb_cObject, NULL, symtab_free, symtab);

  upb_status status = UPB_STATUS_INIT;
  upb_load_descriptor_into_symtab(
      symtab, RSTRING_PTR(descriptor), RSTRING_LEN(descriptor), &status);

  if (!upb_ok(&status)) {
    rb_raise(rb_eRuntimeError,
             "Error loading descriptor: %s", upb_status_errmsg(&status));
  }

  const char *name = RSTRING_PTR(message_name);
  const upb_msgdef *m = upb_symtab_lookupmsg(symtab, name);

  if (!m) {
    rb_raise(rb_eRuntimeError, "Message name '%s' not found", name);
  }

  return make_msgdef(m);
}

static VALUE msgdef_parse(VALUE self, VALUE binary_protobuf) {
  const rb_msgdef *rmd;
  Data_Get_Struct(self, rb_msgdef, rmd);

  VALUE msg = msg_new(self);
  rb_msg *msgp;
  Data_Get_Struct(msg, rb_msg, msgp);

  const upb_handlers *h = upb_pbdecodermethod_desthandlers(rmd->fill_method);
  upb_pbdecoder decoder;
  upb_sink sink;
  upb_status status = UPB_STATUS_INIT;

  upb_pbdecoder_init(&decoder, rmd->fill_method, &status);
  upb_sink_reset(&sink, h, msgp);
  upb_pbdecoder_resetoutput(&decoder, &sink);
  upb_bufsrc_putbuf(RSTRING_PTR(binary_protobuf),
                    RSTRING_LEN(binary_protobuf),
                    upb_pbdecoder_input(&decoder));
  // TODO(haberman): make uninit optional if custom allocator for parsing
  // returns GC-rooted memory.  That will make decoding longjmp-safe (required
  // if parsing triggers any VM errors like OOM or errors in user handlers).
  upb_pbdecoder_uninit(&decoder);
  rupb_checkstatus(&status);

  return msg;
}

void Init_upb() {
  VALUE upb = rb_define_module("Upb");

  cMessageDef = rb_define_class_under(upb, "MessageDef", rb_cObject);
  rb_define_singleton_method(cMessageDef, "load", msgdef_load, 2);
  rb_define_method(cMessageDef, "parse", msgdef_parse, 1);

  cMessage = rb_define_class_under(upb, "Message", rb_cObject);
  rb_define_method(cMessage, "method_missing", msg_accessor, -1);
}
