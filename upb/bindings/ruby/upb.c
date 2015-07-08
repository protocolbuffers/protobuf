/*
** upb (prototype) extension for Ruby.
*/

#include "ruby/ruby.h"
#include "ruby/vm.h"

#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/encoder.h"
#include "upb/pb/glue.h"
#include "upb/shim/shim.h"
#include "upb/symtab.h"

// References to global state.
//
// Ruby does not have multi-VM support and it is common practice to store
// references to classes and other per-VM state in global variables.
static VALUE cSymbolTable;
static VALUE cMessageDef;
static VALUE cMessage;
static VALUE message_map;
static upb_inttable objcache;
static bool objcache_initialized = false;

struct rupb_Message;
struct rupb_MessageDef;
typedef struct rupb_Message rupb_Message;
typedef struct rupb_MessageDef rupb_MessageDef;

#define DEREF_RAW(ptr, ofs, type) *(type*)((char*)ptr + ofs)
#define DEREF(msg, ofs, type) *(type*)(&msg->data[ofs])

void rupb_checkstatus(upb_status *s) {
  if (!upb_ok(s)) {
    rb_raise(rb_eRuntimeError, "%s", upb_status_errmsg(s));
  }
}

static rupb_MessageDef *msgdef_get(VALUE self);
static rupb_Message *msg_get(VALUE self);
static const rupb_MessageDef *get_rbmsgdef(const upb_msgdef *md);
static const upb_handlers *new_fill_handlers(const rupb_MessageDef *rmd,
                                             const void *owner);
static void putmsg(rupb_Message *msg, const rupb_MessageDef *rmd,
                   upb_sink *sink);
static VALUE msgdef_getwrapper(const upb_msgdef *md);
static VALUE new_message_class(VALUE message_def);
static VALUE get_message_class(VALUE klass, VALUE message);
static VALUE msg_new(VALUE msgdef);

/* Ruby VALUE <-> C primitive conversions *************************************/

// Ruby VALUE -> C.
// TODO(haberman): add type/range/precision checks.
static float    value_to_float(VALUE val)  { return NUM2DBL(val);  }
static double   value_to_double(VALUE val) { return NUM2DBL(val);  }
static bool     value_to_bool(VALUE val)   { return RTEST(val);    }
static int32_t  value_to_int32(VALUE val)  { return NUM2INT(val);  }
static uint32_t value_to_uint32(VALUE val) { return NUM2LONG(val); }
static int64_t  value_to_int64(VALUE val)  { return NUM2LONG(val); }
static uint64_t value_to_uint64(VALUE val) { return NUM2ULL(val);  }

// C -> Ruby VALUE
static VALUE float_to_value(float val)     { return rb_float_new(val);    }
static VALUE double_to_value(double val)   { return rb_float_new(val);    }
static VALUE bool_to_value(bool val)       { return val ? Qtrue : Qfalse; }
static VALUE int32_to_value(int32_t val)   { return INT2NUM(val);         }
static VALUE uint32_to_value(uint32_t val) { return LONG2NUM(val);        }
static VALUE int64_to_value(int64_t val)   { return LONG2NUM(val);        }
static VALUE uint64_to_value(uint64_t val) { return ULL2NUM(val);         }


/* stringsink *****************************************************************/

// This should probably be factored into a common upb component.

typedef struct {
  upb_byteshandler handler;
  upb_bytessink sink;
  char *ptr;
  size_t len, size;
} stringsink;

static void *stringsink_start(void *_sink, const void *hd, size_t size_hint) {
  stringsink *sink = _sink;
  sink->len = 0;
  return sink;
}

static size_t stringsink_string(void *_sink, const void *hd, const char *ptr,
                                size_t len, const upb_bufhandle *handle) {
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  stringsink *sink = _sink;
  size_t new_size = sink->size;

  while (sink->len + len > new_size) {
    new_size *= 2;
  }

  if (new_size != sink->size) {
    sink->ptr = realloc(sink->ptr, new_size);
    sink->size = new_size;
  }

  memcpy(sink->ptr + sink->len, ptr, len);
  sink->len += len;

  return len;
}

void stringsink_init(stringsink *sink) {
  upb_byteshandler_init(&sink->handler);
  upb_byteshandler_setstartstr(&sink->handler, stringsink_start, NULL);
  upb_byteshandler_setstring(&sink->handler, stringsink_string, NULL);

  upb_bytessink_reset(&sink->sink, &sink->handler, sink);

  sink->size = 32;
  sink->ptr = malloc(sink->size);
}

void stringsink_uninit(stringsink *sink) {
  free(sink->ptr);
}


/* object cache ***************************************************************/

// The object cache is a singleton mapping of void* -> Ruby Object.
// It caches Ruby objects that wrap C objects.
//
// When we are wrapping C objects it is desirable to give them identity
// semantics.  In other words, if you reach the same C object via two different
// paths, it is desirable (and sometimes even required) that you get the same
// wrapper object both times.  If we instead just created a new wrapper object
// every time you ask for one, we could end up with unexpected results like:
//
//   f1 = msgdef.field("request_id")
//   f2 = msgdef.field("request_id")
//
//   # equal? tests identity equality.  Returns false without a cache.
//   f1.equal?(f2)
//
// We do not register the cache with Ruby's GC, so being in this map will not
// keep the object alive.  This is the desired behavior, because it lets objects
// be freed if they have no references from Ruby.  We do require, though, that
// objects remove themselves from the map when they are freed.  In this respect
// the cache operates like a weak map where the values are weak.

typedef VALUE createfunc(const void *obj);

// Call to initialize the cache.  Should be done once on process startup.
static void objcache_init() {
  upb_inttable_init(&objcache, UPB_CTYPE_UINT64);
  objcache_initialized = true;
}

// Call to uninitialize the cache.  Should be done once on process shutdown.
static void objcache_uninit(ruby_vm_t *vm) {
  assert(objcache_initialized);
  assert(upb_inttable_count(&objcache) == 0);

  objcache_initialized = false;
  upb_inttable_uninit(&objcache);
}

// Looks up the given object in the cache.  If the corresponding Ruby wrapper
// object is found, returns it, otherwise creates the wrapper and returns that.
static VALUE objcache_getorcreate(const void *obj, createfunc *func) {
  assert(objcache_initialized);

  upb_value v;
  if (!upb_inttable_lookupptr(&objcache, obj, &v)) {
    v = upb_value_uint64(func(obj));
    upb_inttable_insertptr(&objcache, obj, v);
  }
  return upb_value_getuint64(v);
}

// Removes the given object from the cache.  Should only be called by the code
// that is freeing the wrapper object.
static void objcache_remove(const void *obj) {
  assert(objcache_initialized);

  bool removed = upb_inttable_removeptr(&objcache, obj, NULL);
  UPB_ASSERT_VAR(removed, removed);
}

/* message layout *************************************************************/

// We layout Ruby messages using a raw block of C memory.  We assign offsets for
// each member so that instances are laid out like a C struct instead of as
// instance variables.  This saves both memory and CPU.

typedef struct {
  // The size of the block of memory we should allocate for instances.
  size_t size;

  // Prototype to memcpy() onto new message instances.  Size is "size" above.
  void *prototype;

  // An offset for each member, indexed by upb_fielddef_index(f).
  uint32_t *field_offsets;
} rb_msglayout;

// Returns true for fields where the field value we store is a Ruby VALUE (ie. a
// direct pointer to another Ruby object) instead of storing the value directly
// in the message.
static bool is_ruby_value(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) {
    // Repeated fields are pointers to arrays.
    return true;
  }

  if (upb_fielddef_issubmsg(f)) {
    // Submessage fields are pointers to submessages.
    return true;
  }

  if (upb_fielddef_isstring(f)) {
    // String fields are pointers to string objects.
    return true;
  }

  return false;
}

// General alignment rules are that each type needs to be stored at an address
// that is a multiple of its size.
static size_t align_up(size_t val, size_t align) {
  return val % align == 0 ? val : val + align - (val % align);
}

// Byte size to store each upb type.
static size_t rupb_sizeof(const upb_fielddef *f) {
  if (is_ruby_value(f)) {
    return sizeof(VALUE);
  }

  switch (upb_fielddef_type(f)) {
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
    default:
      break;
  }
  assert(false);
  return 0;
}

// Calculates offsets for each field.
//
// This lets us pack protos like structs instead of storing them like
// dictionaries.  This speeds up a parsing a lot and also saves memory
// (unless messages are very sparse).
static void assign_offsets(rb_msglayout *layout, const upb_msgdef *md) {
  layout->field_offsets = ALLOC_N(uint32_t, upb_msgdef_numfields(md));
  size_t ofs = 0;
  upb_msg_field_iter i;

  for (upb_msg_field_begin(&i, md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    size_t field_size = rupb_sizeof(f);

    // Align field properly.
    //
    // TODO(haberman): optimize layout?  For example we could sort fields
    // big-to-small.
    ofs = align_up(ofs, field_size);

    layout->field_offsets[upb_fielddef_index(f)] = ofs;
    ofs += field_size;
  }

  layout->size = ofs;
}

// Creates a prototype; a buffer we can memcpy() onto new instances to
// initialize them.
static void make_prototype(rb_msglayout *layout, const upb_msgdef *md) {
  void *prototype = ALLOC_N(char, layout->size);

  // Most members default to zero, so we'll start from that and then overwrite
  // more specific initialization.
  memset(prototype, 0, layout->size);

  upb_msg_field_iter i;
  for (upb_msg_field_begin(&i, md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    if (is_ruby_value(f)) {
      size_t ofs = layout->field_offsets[upb_fielddef_index(f)];
      // Default all Ruby pointers to nil.
      DEREF_RAW(prototype, ofs, VALUE) = Qnil;
    }
  }

  layout->prototype = prototype;
}


static void msglayout_init(rb_msglayout *layout, const upb_msgdef *m) {
  assign_offsets(layout, m);
  make_prototype(layout, m);
}

static void msglayout_uninit(rb_msglayout *layout) {
  free(layout->field_offsets);
  free(layout->prototype);
}


/* Upb::MessageDef ************************************************************/

// C representation for Upb::MessageDef.
//
// Contains a reference to the underlying upb_msgdef, as well as associated data
// like a reference to the corresponding Ruby class.
struct rupb_MessageDef {
  // We own refs on all of these.

  // The upb_msgdef we are wrapping.
  const upb_msgdef *md;

  // A DecoderMethod for parsing a protobuf into this type.
  const upb_pbdecodermethod *fill_method;

  // Handlers for serializing into a protobuf of this type.
  const upb_handlers *serialize_handlers;

  // The Ruby class for instances of this type.
  VALUE klass;

  // Layout for messages of this type.
  rb_msglayout layout;
};

// Called by the Ruby GC when a Upb::MessageDef is being freed.
static void msgdef_free(void *_rmd) {
  rupb_MessageDef *rmd = _rmd;
  objcache_remove(rmd->md);
  upb_msgdef_unref(rmd->md, &rmd->md);
  if (rmd->fill_method) {
    upb_pbdecodermethod_unref(rmd->fill_method, &rmd->fill_method);
  }
  if (rmd->serialize_handlers) {
    upb_handlers_unref(rmd->serialize_handlers, &rmd->serialize_handlers);
  }
  msglayout_uninit(&rmd->layout);
  free(rmd);
}

// Called by the Ruby GC during the "mark" phase to decide what is still alive.
// We call rb_gc_mark on all Ruby VALUE pointers we reference.
static void msgdef_mark(void *_rmd) {
  rupb_MessageDef *rmd = _rmd;
  rb_gc_mark(rmd->klass);

  // Mark all submessage types.
  upb_msg_field_iter i;
  for (upb_msg_field_begin(&i, rmd->md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    if (upb_fielddef_issubmsg(f)) {
      // If we were trying to be more aggressively lazy, the submessage might
      // not be created and we only mark ones that are.
      rb_gc_mark(msgdef_getwrapper(upb_fielddef_msgsubdef(f)));
    }
  }
}

static const rb_data_type_t msgdef_type = {"Upb::MessageDef",
                                           {msgdef_mark, msgdef_free, NULL}};

// TODO(haberman): do we need an alloc func?  We want to prohibit dup and
// probably subclassing too.

static rupb_MessageDef *msgdef_get(VALUE self) {
  rupb_MessageDef *msgdef;
  TypedData_Get_Struct(self, rupb_MessageDef, &msgdef_type, msgdef);
  return msgdef;
}

// Constructs the upb decoder method for parsing messages of this type.
const upb_pbdecodermethod *new_fillmsg_decodermethod(const rupb_MessageDef *rmd,
                                                     const void *owner) {
  const upb_handlers *fill_handlers = new_fill_handlers(rmd, &fill_handlers);
  upb_pbdecodermethodopts opts;
  upb_pbdecodermethodopts_init(&opts, fill_handlers);

  const upb_pbdecodermethod *ret = upb_pbdecodermethod_new(&opts, owner);
  upb_handlers_unref(fill_handlers, &fill_handlers);
  return ret;
}

// Constructs a new Ruby wrapper object around the given msgdef.
static VALUE make_msgdef(const void *_md) {
  const upb_msgdef *md = _md;
  rupb_MessageDef *rmd;
  VALUE ret =
      TypedData_Make_Struct(cMessageDef, rupb_MessageDef, &msgdef_type, rmd);

  upb_msgdef_ref(md, &rmd->md);

  rmd->md = md;
  rmd->fill_method = NULL;

  // OPT: most of these things could be built lazily, when they are first
  // needed.
  msglayout_init(&rmd->layout, md);

  rmd->fill_method = NULL;
  rmd->klass = new_message_class(ret);
  rmd->serialize_handlers =
      upb_pb_encoder_newhandlers(md, &rmd->serialize_handlers);

  return ret;
}

// Accessor to get a decoder method for this message type.
// Constructs the decoder method lazily.
static const upb_pbdecodermethod *msgdef_decodermethod(rupb_MessageDef *rmd) {
  if (!rmd->fill_method) {
    rmd->fill_method = new_fillmsg_decodermethod(rmd, &rmd->fill_method);
  }

  return rmd->fill_method;
}

static VALUE msgdef_getwrapper(const upb_msgdef *md) {
  return objcache_getorcreate(md, make_msgdef);
}

static const rupb_MessageDef *get_rbmsgdef(const upb_msgdef *md) {
  return msgdef_get(msgdef_getwrapper(md));
}


/* Upb::Message ***************************************************************/

// Code to implement the Upb::Message object.
//
// A unique Ruby class is generated for each message type, but all message types
// share Upb::Message as their base class.  Upb::Message contains all of the
// actual functionality; the only reason the derived class exists at all is
// for convenience.  It lets Ruby users do things like:
//
//   message = MyMessage.new
//   if message.kind_of?(MyMessage)
//
// ... and other similar things that Ruby users expect they can do.

// C representation of Upb::Message.
//
// Represents a message instance, laid out like a C struct in a type-specific
// layout.
//
// This will be sized according to what fields are actually present.
struct rupb_Message {
  VALUE rbmsgdef;
  char data[];
};

// Returns the size of a message instance.
size_t msg_size(const rupb_MessageDef *rmd) {
  return sizeof(rupb_Message) + rmd->layout.size;
}

static void msg_free(void *msg) {
  free(msg);
}

// Invoked by the Ruby GC whenever it is doing a mark-and-sweep.
static void msg_mark(void *p) {
  rupb_Message *msg = p;
  rupb_MessageDef *rmd = msgdef_get(msg->rbmsgdef);

  // Mark the msgdef to keep it alive.
  rb_gc_mark(msg->rbmsgdef);

  // We need to mark all references to other Ruby values: strings, arrays, and
  // submessages that we point to.
  upb_msg_field_iter i;
  for (upb_msg_field_begin(&i, rmd->md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    if (is_ruby_value(f)) {
      size_t ofs = rmd->layout.field_offsets[upb_fielddef_index(f)];
      rb_gc_mark(DEREF(msg, ofs, VALUE));
    }
  }
}

static const rb_data_type_t msg_type = {"Upb::Message",
                                        {msg_mark, msg_free, NULL}};

static rupb_Message *msg_get(VALUE self) {
  rupb_Message *msg;
  TypedData_Get_Struct(self, rupb_Message, &msg_type, msg);
  return msg;
}

// Instance variable name that we use to store a reference from the Ruby class
// for a message and its Upb::MessageDef.
//
// We avoid prefixing this by "@" to make it inaccessible by Ruby.
static const char *kMessageDefMemberName = "msgdef";

static VALUE msg_getmsgdef(VALUE klass) {
  VALUE msgdef = rb_iv_get(klass, kMessageDefMemberName);

  if (msgdef == Qnil) {
    // TODO(haberman): If we want to allow subclassing, we might want to walk up
    // the hierarchy looking for this member.
    rb_raise(rb_eRuntimeError,
             "Can't call on Upb::Message directly, only subclasses");
  }

  return msgdef;
}

// Called by the Ruby VM when it wants to create a new message instance.
static VALUE msg_alloc(VALUE klass) {
  VALUE msgdef = msg_getmsgdef(klass);
  const rupb_MessageDef *rmd = msgdef_get(msgdef);

  rupb_Message *msg = (rupb_Message*)ALLOC_N(char, msg_size(rmd));
  msg->rbmsgdef = msgdef;
  memcpy(&msg->data, rmd->layout.prototype, rmd->layout.size);

  VALUE ret = TypedData_Wrap_Struct(klass, &msg_type, msg);
  return ret;
}

// Creates a new Ruby class for the given Upb::MessageDef.  The new class
// derives from Upb::Message but also stores a reference to the Upb::MessageDef.
static VALUE new_message_class(VALUE message_def) {
  msgdef_get(message_def);  // Check type.
  VALUE klass = rb_class_new(cMessage);
  rb_iv_set(klass, kMessageDefMemberName, message_def);

  // This shouldn't be necessary because we should inherit the alloc func from
  // the base class of Message.  For some reason this is not working properly
  // and we are having to define it manually.
  rb_define_alloc_func(klass, msg_alloc);

  return klass;
}

// Call to create a new Message instance.
static VALUE msg_new(VALUE msgdef) {
  return rb_class_new_instance(0, NULL, get_message_class(Qnil, msgdef));
}

// Looks up the given field.  On success returns the upb_fielddef and stores the
// offset in *ofs.  Otherwise raises a Ruby exception.
static const upb_fielddef *lookup_field(rupb_Message *msg, const char *field,
                                        size_t len, size_t *ofs) {
  const rupb_MessageDef *rmd = msgdef_get(msg->rbmsgdef);
  const upb_fielddef *f = upb_msgdef_ntof(rmd->md, field, len);

  if (!f) {
    rb_raise(rb_eArgError, "Message %s does not contain field %s",
             upb_msgdef_fullname(rmd->md), field);
  }

  *ofs = rmd->layout.field_offsets[upb_fielddef_index(f)];
  return f;
}

// Sets the given field to the given value.
static void setprimitive(rupb_Message *m, size_t ofs, const upb_fielddef *f,
                         VALUE val) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_FLOAT:  DEREF(m, ofs, float) = value_to_float(val); break;
    case UPB_TYPE_DOUBLE: DEREF(m, ofs, double) = value_to_double(val); break;
    case UPB_TYPE_BOOL:   DEREF(m, ofs, bool) = value_to_bool(val); break;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:  DEREF(m, ofs, int32_t) = value_to_int32(val); break;
    case UPB_TYPE_UINT32: DEREF(m, ofs, uint32_t) = value_to_uint32(val); break;
    case UPB_TYPE_INT64:  DEREF(m, ofs, int64_t) = value_to_int64(val); break;
    case UPB_TYPE_UINT64: DEREF(m, ofs, uint64_t) = value_to_uint64(val); break;
    default: rb_bug("Unexpected type");
  }
}

// Returns the Ruby VALUE for the given field.
static VALUE getprimitive(rupb_Message *m, size_t ofs, const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_FLOAT:  return float_to_value(DEREF(m, ofs, float));
    case UPB_TYPE_DOUBLE: return double_to_value(DEREF(m, ofs, double));
    case UPB_TYPE_BOOL:   return bool_to_value(DEREF(m, ofs, bool));
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:  return int32_to_value(DEREF(m, ofs, int32_t));
    case UPB_TYPE_UINT32: return uint32_to_value(DEREF(m, ofs, uint32_t));
    case UPB_TYPE_INT64:  return int64_to_value(DEREF(m, ofs, int64_t));
    case UPB_TYPE_UINT64: return uint64_to_value(DEREF(m, ofs, uint64_t));
    default: rb_bug("Unexpected type");
  }
}

static VALUE msg_setter(rupb_Message *msg, VALUE field, VALUE val) {
  size_t ofs;

  // fieldp is a string like "id=".  But we want to look up "id".
  const upb_fielddef *f =
      lookup_field(msg, RSTRING_PTR(field), RSTRING_LEN(field) - 1, &ofs);

  // Possibly introduce stricter type checking.
  if (is_ruby_value(f)) {
    DEREF(msg, ofs, VALUE) = val;
  } else {
    setprimitive(msg, ofs, f, val);
  }

  return val;
}

static VALUE msg_getter(rupb_Message *msg, VALUE field) {
  size_t ofs;
  const upb_fielddef *f =
      lookup_field(msg, RSTRING_PTR(field), RSTRING_LEN(field), &ofs);

  if (is_ruby_value(f)) {
    return DEREF(msg, ofs, VALUE);
  } else {
    return getprimitive(msg, ofs, f);
  }
}

// This is the Message object's "method_missing" method, so it receives calls
// for any method whose name was not recognized.  We use it to implement getters
// and setters for every field
//
// call-seq:
//     message.field -> current value of "field"
//     message.field = new_value
static VALUE msg_accessor(int argc, VALUE *argv, VALUE obj) {
  rupb_Message *msg = msg_get(obj);

  // method_missing protocol: (method [, arg1, arg2, ...])
  assert(argc >= 1 && SYMBOL_P(argv[0]));
  // OPT(haberman): find a better way to get the method name.
  // This is allocating a new string each time, which should not be necessary.
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

// Called when Ruby wants to turn this value into a string.
// TODO(haberman): implement.
static VALUE msg_tostring(VALUE self) {
  return rb_str_new2("tostring!");
}

// call-seq:
//     MessageClass.parse(binary_protobuf) -> message instance
//
// Parses a binary protobuf according to this message class and returns a new
// message instance of this class type.
static VALUE msg_parse(VALUE klass, VALUE binary_protobuf) {
  Check_Type(binary_protobuf, T_STRING);
  rupb_MessageDef *rmd = msgdef_get(msg_getmsgdef(klass));

  VALUE msg = rb_class_new_instance(0, NULL, klass);
  rupb_Message *msgp = msg_get(msg);

  const upb_pbdecodermethod *method = msgdef_decodermethod(rmd);
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);
  upb_pbdecoder decoder;
  upb_sink sink;
  upb_status status = UPB_STATUS_INIT;

  upb_pbdecoder_init(&decoder, method, &status);
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

// call-seq:
//     Message.serialize(message instance) -> serialized string
//
// Serializes the given message instance to a string.
static VALUE msg_serialize(VALUE klass, VALUE message) {
  rupb_Message *msg = msg_get(message);
  const rupb_MessageDef *rmd = msgdef_get(msg->rbmsgdef);

  stringsink sink;
  stringsink_init(&sink);

  upb_pb_encoder encoder;
  upb_pb_encoder_init(&encoder, rmd->serialize_handlers);
  upb_pb_encoder_resetoutput(&encoder, &sink.sink);

  putmsg(msg, rmd, upb_pb_encoder_input(&encoder));

  VALUE ret = rb_str_new(sink.ptr, sink.len);

  upb_pb_encoder_uninit(&encoder);
  stringsink_uninit(&sink);

  return ret;
}


/* Upb::SymbolTable ***********************************************************/

// Ruby wrapper around a SymbolTable.  Allows loading of descriptors and turning
// them into MessageDef objects.

void symtab_free(void *s) {
  upb_symtab_unref(s, UPB_UNTRACKED_REF);
}

static const rb_data_type_t symtab_type = {"Upb::SymbolTable",
                                           {NULL, symtab_free, NULL}};

// Called by the Ruby VM to allocate a SymbolTable object.
static VALUE symtab_alloc(VALUE klass) {
  upb_symtab *symtab = upb_symtab_new(UPB_UNTRACKED_REF);
  VALUE ret = TypedData_Wrap_Struct(klass, &symtab_type, symtab);

  return ret;
}

static upb_symtab *symtab_get(VALUE self) {
  upb_symtab *symtab;
  TypedData_Get_Struct(self, upb_symtab, &symtab_type, symtab);
  return symtab;
}

// call-seq:
//     symtab.load_descriptor(descriptor)
//
// Parses a FileDescriptorSet from the given string and adds the defs to the
// SymbolTable.  Raises if there was an error.
static VALUE symtab_load_descriptor(VALUE self, VALUE descriptor) {
  upb_symtab *symtab = symtab_get(self);
  Check_Type(descriptor, T_STRING);

  upb_status status = UPB_STATUS_INIT;
  upb_load_descriptor_into_symtab(
      symtab, RSTRING_PTR(descriptor), RSTRING_LEN(descriptor), &status);

  if (!upb_ok(&status)) {
    rb_raise(rb_eRuntimeError,
             "Error loading descriptor: %s", upb_status_errmsg(&status));
  }

  return Qnil;
}

// call-seq:
//     symtab.lookup(name)
//
// Returns the def for this name, or nil if none.
// TODO(haberman): only support messages right now, not enums.
static VALUE symtab_lookup(VALUE self, VALUE name) {
  upb_symtab *symtab = symtab_get(self);
  Check_Type(name, T_STRING);

  const char *cname = RSTRING_PTR(name);
  const upb_msgdef *m = upb_symtab_lookupmsg(symtab, cname);

  if (!m) {
    rb_raise(rb_eRuntimeError, "Message name '%s' not found", cname);
  }

  return msgdef_getwrapper(m);
}


/* handlers *******************************************************************/

// These are handlers for populating a Ruby protobuf message (rupb_Message) when
// parsing.

// Creates a handlerdata that simply contains the offset for this field.
static const void *newhandlerdata(upb_handlers *h, uint32_t ofs) {
  size_t *hd_ofs = ALLOC(size_t);
  *hd_ofs = ofs;
  upb_handlers_addcleanup(h, hd_ofs, free);
  return hd_ofs;
}

typedef struct {
  size_t ofs;
  const upb_msgdef *md;
} submsg_handlerdata_t;

// Creates a handlerdata that contains offset and submessage type information.
static const void *newsubmsghandlerdata(upb_handlers *h, uint32_t ofs,
                                        const upb_fielddef *f) {
  submsg_handlerdata_t *hd = ALLOC(submsg_handlerdata_t);
  hd->ofs = ofs;
  hd->md = upb_fielddef_msgsubdef(f);
  upb_handlers_addcleanup(h, hd, free);
  return hd;
}

// A handler that starts a repeated field.  Gets or creates a Ruby array for the
// field.
static void *startseq_handler(void *closure, const void *hd) {
  rupb_Message *msg = closure;
  const size_t *ofs = hd;

  if (DEREF(msg, *ofs, VALUE) == Qnil) {
    DEREF(msg, *ofs, VALUE) = rb_ary_new();
  }

  return (void*)DEREF(msg, *ofs, VALUE);
}

// Handlers that append primitive values to a repeated field (a regular Ruby
// array for now).
#define DEFINE_APPEND_HANDLER(type, ctype)                 \
  static bool append##type##_handler(void *closure, const void *hd, \
                                     ctype val) {                   \
    VALUE ary = (VALUE)closure;                                     \
    rb_ary_push(ary, type##_to_value(val));                         \
    return true;                                                    \
  }

DEFINE_APPEND_HANDLER(bool,   bool)
DEFINE_APPEND_HANDLER(int32,  int32_t)
DEFINE_APPEND_HANDLER(uint32, uint32_t)
DEFINE_APPEND_HANDLER(float,  float)
DEFINE_APPEND_HANDLER(int64,  int64_t)
DEFINE_APPEND_HANDLER(uint64, uint64_t)
DEFINE_APPEND_HANDLER(double, double)

// Appends a string to a repeated field (a regular Ruby array for now).
static size_t appendstr_handler(void *closure, const void *hd, const char *str,
                                size_t len, const upb_bufhandle *handle) {
  VALUE ary = (VALUE)closure;
  rb_ary_push(ary, rb_str_new(str, len));
  return len;
}

// Sets a non-repeated string field in a message.
static size_t str_handler(void *closure, const void *hd, const char *str,
                          size_t len, const upb_bufhandle *handle) {
  rupb_Message *msg = closure;
  const size_t *ofs = hd;
  DEREF(msg, *ofs, VALUE) = rb_str_new(str, len);
  return len;
}

// Appends a submessage to a repeated field (a regular Ruby array for now).
static void *appendsubmsg_handler(void *closure, const void *hd) {
  VALUE ary = (VALUE)closure;
  const submsg_handlerdata_t *submsgdata = hd;
  VALUE submsg = msg_new(msgdef_getwrapper(submsgdata->md));
  rb_ary_push(ary, submsg);
  return msg_get(submsg);
}

// Sets a non-repeated submessage field in a message.
static void *submsg_handler(void *closure, const void *hd) {
  rupb_Message *msg = closure;
  const submsg_handlerdata_t *submsgdata = hd;

  if (DEREF(msg, submsgdata->ofs, VALUE) == Qnil) {
    DEREF(msg, submsgdata->ofs, VALUE) =
        msg_new(msgdef_getwrapper(submsgdata->md));
  }

  VALUE submsg = DEREF(msg, submsgdata->ofs, VALUE);
  return msg_get(submsg);
}

static void add_handlers_for_message(const void *closure, upb_handlers *h) {
  const rupb_MessageDef *rmd = get_rbmsgdef(upb_handlers_msgdef(h));
  upb_msg_field_iter i;

  for (upb_msg_field_begin(&i, rmd->md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    size_t ofs = rmd->layout.field_offsets[upb_fielddef_index(f)];

    if (upb_fielddef_isseq(f)) {
      upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
      upb_handlerattr_sethandlerdata(&attr, newhandlerdata(h, ofs));
      upb_handlers_setstartseq(h, f, startseq_handler, &attr);
      upb_handlerattr_uninit(&attr);

      switch (upb_fielddef_type(f)) {

#define SET_HANDLER(utype, ltype)                                 \
  case utype:                                                     \
    upb_handlers_set##ltype(h, f, append##ltype##_handler, NULL); \
    break;

        SET_HANDLER(UPB_TYPE_BOOL,   bool);
        SET_HANDLER(UPB_TYPE_INT32,  int32);
        SET_HANDLER(UPB_TYPE_UINT32, uint32);
        SET_HANDLER(UPB_TYPE_ENUM,   int32);
        SET_HANDLER(UPB_TYPE_FLOAT,  float);
        SET_HANDLER(UPB_TYPE_INT64,  int64);
        SET_HANDLER(UPB_TYPE_UINT64, uint64);
        SET_HANDLER(UPB_TYPE_DOUBLE, double);

#undef SET_HANDLER

        case UPB_TYPE_STRING:
        case UPB_TYPE_BYTES:
          // XXX: does't currently handle split buffers.
          upb_handlers_setstring(h, f, appendstr_handler, NULL);
          break;
        case UPB_TYPE_MESSAGE: {
          upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
          upb_handlerattr_sethandlerdata(&attr, newsubmsghandlerdata(h, 0, f));
          upb_handlers_setstartsubmsg(h, f, appendsubmsg_handler, &attr);
          upb_handlerattr_uninit(&attr);
          break;
        }
      }
    }

    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_BOOL:
      case UPB_TYPE_INT32:
      case UPB_TYPE_UINT32:
      case UPB_TYPE_ENUM:
      case UPB_TYPE_FLOAT:
      case UPB_TYPE_INT64:
      case UPB_TYPE_UINT64:
      case UPB_TYPE_DOUBLE:
        // The shim writes directly at the given offset (instead of using
        // DEREF()) so we need to add the msg overhead.
        upb_shim_set(h, f, ofs + sizeof(rupb_Message), -1);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES: {
        upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
        upb_handlerattr_sethandlerdata(&attr, newhandlerdata(h, ofs));
        // XXX: does't currently handle split buffers.
        upb_handlers_setstring(h, f, str_handler, &attr);
        upb_handlerattr_uninit(&attr);
        break;
      }
      case UPB_TYPE_MESSAGE: {
        upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
        upb_handlerattr_sethandlerdata(&attr, newsubmsghandlerdata(h, ofs, f));
        upb_handlers_setstartsubmsg(h, f, submsg_handler, &attr);
        upb_handlerattr_uninit(&attr);
        break;
      }
    }
  }
}

// Creates upb handlers for populating a message.
static const upb_handlers *new_fill_handlers(const rupb_MessageDef *rmd,
                                             const void *owner) {
  return upb_handlers_newfrozen(rmd->md, owner, add_handlers_for_message, NULL);
}


/* msgvisitor *****************************************************************/

// This is code to push the contents of a Ruby message (rupb_Message) to a upb
// sink.

static upb_selector_t getsel(const upb_fielddef *f, upb_handlertype_t type) {
  upb_selector_t ret;
  bool ok = upb_handlers_getselector(f, type, &ret);
  UPB_ASSERT_VAR(ok, ok);
  return ret;
}

static void putstr(VALUE str, const upb_fielddef *f, upb_sink *sink) {
  if (str == Qnil) return;

  assert(BUILTIN_TYPE(str) == RUBY_T_STRING);
  upb_sink subsink;

  upb_sink_startstr(sink, getsel(f, UPB_HANDLER_STARTSTR), RSTRING_LEN(str),
                    &subsink);
  upb_sink_putstring(&subsink, getsel(f, UPB_HANDLER_STRING), RSTRING_PTR(str),
                     RSTRING_LEN(str), NULL);
  upb_sink_endstr(sink, getsel(f, UPB_HANDLER_ENDSTR));
}

static void putsubmsg(VALUE submsg, const upb_fielddef *f, upb_sink *sink) {
  if (submsg == Qnil) return;

  upb_sink subsink;
  const rupb_MessageDef *sub_rmd = get_rbmsgdef(upb_fielddef_msgsubdef(f));

  upb_sink_startsubmsg(sink, getsel(f, UPB_HANDLER_STARTSUBMSG), &subsink);
  putmsg(msg_get(submsg), sub_rmd, &subsink);
  upb_sink_endsubmsg(sink, getsel(f, UPB_HANDLER_ENDSUBMSG));
}

static void putary(VALUE ary, const upb_fielddef *f, upb_sink *sink) {
  if (ary == Qnil) return;

  assert(BUILTIN_TYPE(ary) == RUBY_T_ARRAY);
  upb_sink subsink;

  upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);

  upb_fieldtype_t type = upb_fielddef_type(f);
  upb_selector_t sel = 0;
  if (upb_fielddef_isprimitive(f)) {
    sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  }

  int i;
  for (i = 0; i < RARRAY_LEN(ary); i++) {
    VALUE val = rb_ary_entry(ary, i);
    switch (type) {

#define T(upbtypeconst, upbtype, ctype)                         \
  case upbtypeconst:                                            \
    upb_sink_put##upbtype(&subsink, sel, value_to_##upbtype(val)); \
    break;

      T(UPB_TYPE_FLOAT,  float,  float)
      T(UPB_TYPE_DOUBLE, double, double)
      T(UPB_TYPE_BOOL,   bool,   bool)
      case UPB_TYPE_ENUM:
      T(UPB_TYPE_INT32,  int32,  int32_t)
      T(UPB_TYPE_UINT32, uint32, uint32_t)
      T(UPB_TYPE_INT64,  int64,  int64_t)
      T(UPB_TYPE_UINT64, uint64, uint64_t)

      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        putstr(val, f, &subsink);
        break;
      case UPB_TYPE_MESSAGE:
        putsubmsg(val, f, &subsink);
        break;

#undef T

    }
  }
  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static void putmsg(rupb_Message *msg, const rupb_MessageDef *rmd,
                   upb_sink *sink) {
  upb_sink_startmsg(sink);

  upb_msg_field_iter i;
  for (upb_msg_field_begin(&i, rmd->md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    uint32_t ofs = rmd->layout.field_offsets[upb_fielddef_index(f)];

    if (upb_fielddef_isseq(f)) {
      VALUE ary = DEREF(msg, ofs, VALUE);
      if (ary != Qnil) {
        putary(ary, f, sink);
      }
    } else if (upb_fielddef_isstring(f)) {
      putstr(DEREF(msg, ofs, VALUE), f, sink);
    } else if (upb_fielddef_issubmsg(f)) {
      putsubmsg(DEREF(msg, ofs, VALUE), f, sink);
    } else {
      upb_selector_t sel = getsel(f, upb_handlers_getprimitivehandlertype(f));

#define T(upbtypeconst, upbtype, ctype)                       \
  case upbtypeconst:                                          \
    upb_sink_put##upbtype(sink, sel, DEREF(msg, ofs, ctype)); \
    break;

      switch (upb_fielddef_type(f)) {
        T(UPB_TYPE_FLOAT,  float,  float)
        T(UPB_TYPE_DOUBLE, double, double)
        T(UPB_TYPE_BOOL,   bool,   bool)
        case UPB_TYPE_ENUM:
        T(UPB_TYPE_INT32,  int32,  int32_t)
        T(UPB_TYPE_UINT32, uint32, uint32_t)
        T(UPB_TYPE_INT64,  int64,  int64_t)
        T(UPB_TYPE_UINT64, uint64, uint64_t)

        case UPB_TYPE_STRING:
        case UPB_TYPE_BYTES:
        case UPB_TYPE_MESSAGE: rb_raise(rb_eRuntimeError, "Internal error.");
      }

#undef T

    }
  }

  upb_status status;
  upb_sink_endmsg(sink, &status);
}


/* top level ******************************************************************/

static VALUE get_message_class(VALUE klass, VALUE message) {
  rupb_MessageDef *rmd = msgdef_get(message);
  return rmd->klass;
}

void Init_upb() {
  VALUE upb = rb_define_module("Upb");
  rb_define_singleton_method(upb, "get_message_class", get_message_class, 1);
  rb_gc_register_address(&message_map);

  cSymbolTable = rb_define_class_under(upb, "SymbolTable", rb_cObject);
  rb_define_alloc_func(cSymbolTable, symtab_alloc);
  rb_define_method(cSymbolTable, "load_descriptor", symtab_load_descriptor, 1);
  rb_define_method(cSymbolTable, "lookup", symtab_lookup, 1);

  cMessageDef = rb_define_class_under(upb, "MessageDef", rb_cObject);

  cMessage = rb_define_class_under(upb, "Message", rb_cObject);
  rb_define_alloc_func(cMessage, msg_alloc);
  rb_define_method(cMessage, "method_missing", msg_accessor, -1);
  rb_define_method(cMessage, "to_s", msg_tostring, 0);
  rb_define_singleton_method(cMessage, "parse", msg_parse, 1);
  rb_define_singleton_method(cMessage, "serialize", msg_serialize, 1);

  objcache_init();

  // This causes atexit crashes for unknown reasons. :(
  // ruby_vm_at_exit(objcache_uninit);
}
