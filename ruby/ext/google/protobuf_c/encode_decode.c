// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "protobuf.h"

// This function is equivalent to rb_str_cat(), but unlike the real
// rb_str_cat(), it doesn't leak memory in some versions of Ruby.
// For more information, see:
//   https://bugs.ruby-lang.org/issues/11328
VALUE noleak_rb_str_cat(VALUE rb_str, const char *str, long len) {
  char *p;
  size_t oldlen = RSTRING_LEN(rb_str);
  rb_str_modify_expand(rb_str, len);
  p = RSTRING_PTR(rb_str);
  memcpy(p + oldlen, str, len);
  rb_str_set_len(rb_str, oldlen + len);
  return rb_str;
}

// The code below also comes from upb's prototype Ruby binding, developed by
// haberman@.

/* stringsink *****************************************************************/

static void *stringsink_start(void *_sink, const void *hd, size_t size_hint) {
  stringsink *sink = _sink;
  sink->len = 0;
  return sink;
}

static size_t stringsink_string(void *_sink, const void *hd, const char *ptr,
                                size_t len, const upb_bufhandle *handle) {
  stringsink *sink = _sink;
  size_t new_size = sink->size;

  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

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
  sink->len = 0;
}

void stringsink_uninit(stringsink *sink) {
  free(sink->ptr);
}

// -----------------------------------------------------------------------------
// Parsing.
// -----------------------------------------------------------------------------

#define DEREF(msg, ofs, type) *(type*)(((uint8_t *)msg) + ofs)

typedef struct {
  size_t ofs;
  int32_t hasbit;
} field_handlerdata_t;

// Creates a handlerdata that contains the offset and the hasbit for the field
static const void* newhandlerdata(upb_handlers* h, uint32_t ofs, int32_t hasbit) {
  field_handlerdata_t *hd = ALLOC(field_handlerdata_t);
  hd->ofs = ofs;
  hd->hasbit = hasbit;
  upb_handlers_addcleanup(h, hd, xfree);
  return hd;
}

typedef struct {
  size_t ofs;
  int32_t hasbit;
  VALUE subklass;
} submsg_handlerdata_t;

// Creates a handlerdata that contains offset and submessage type information.
static const void *newsubmsghandlerdata(upb_handlers* h,
                                        uint32_t ofs,
                                        int32_t hasbit,
                                        VALUE subklass) {
  submsg_handlerdata_t *hd = ALLOC(submsg_handlerdata_t);
  hd->ofs = ofs;
  hd->hasbit = hasbit;
  hd->subklass = subklass;
  upb_handlers_addcleanup(h, hd, xfree);
  return hd;
}

typedef struct {
  size_t ofs;              // union data slot
  size_t case_ofs;         // oneof_case field
  uint32_t oneof_case_num; // oneof-case number to place in oneof_case field
  VALUE subklass;
} oneof_handlerdata_t;

static const void *newoneofhandlerdata(upb_handlers *h,
                                       uint32_t ofs,
                                       uint32_t case_ofs,
                                       const upb_fielddef *f,
                                       const Descriptor* desc) {
  oneof_handlerdata_t *hd = ALLOC(oneof_handlerdata_t);
  hd->ofs = ofs;
  hd->case_ofs = case_ofs;
  // We reuse the field tag number as a oneof union discriminant tag. Note that
  // we don't expose these numbers to the user, so the only requirement is that
  // we have some unique ID for each union case/possibility. The field tag
  // numbers are already present and are easy to use so there's no reason to
  // create a separate ID space. In addition, using the field tag number here
  // lets us easily look up the field in the oneof accessor.
  hd->oneof_case_num = upb_fielddef_number(f);
  if (is_value_field(f)) {
    hd->oneof_case_num |= ONEOF_CASE_MASK;
  }
  hd->subklass = field_type_class(desc->layout, f);
  upb_handlers_addcleanup(h, hd, xfree);
  return hd;
}

// A handler that starts a repeated field.  Gets the Repeated*Field instance for
// this field (such an instance always exists even in an empty message).
static void *startseq_handler(void* closure, const void* hd) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  return (void*)DEREF(msg, *ofs, VALUE);
}

// Handlers that append primitive values to a repeated field.
#define DEFINE_APPEND_HANDLER(type, ctype)                 \
  static bool append##type##_handler(void *closure, const void *hd, \
                                     ctype val) {                   \
    VALUE ary = (VALUE)closure;                                     \
    RepeatedField_push_native(ary, &val);                           \
    return true;                                                    \
  }

DEFINE_APPEND_HANDLER(bool,   bool)
DEFINE_APPEND_HANDLER(int32,  int32_t)
DEFINE_APPEND_HANDLER(uint32, uint32_t)
DEFINE_APPEND_HANDLER(float,  float)
DEFINE_APPEND_HANDLER(int64,  int64_t)
DEFINE_APPEND_HANDLER(uint64, uint64_t)
DEFINE_APPEND_HANDLER(double, double)

// Appends a string to a repeated field.
static void* appendstr_handler(void *closure,
                               const void *hd,
                               size_t size_hint) {
  VALUE ary = (VALUE)closure;
  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyStringUtf8Encoding);
  RepeatedField_push_native(ary, &str);
  return (void*)str;
}

static void set_hasbit(void *closure, int32_t hasbit) {
  if (hasbit > 0) {
    uint8_t* storage = closure;
    storage[hasbit/8] |= 1 << (hasbit % 8);
  }
}

// Appends a 'bytes' string to a repeated field.
static void* appendbytes_handler(void *closure,
                                 const void *hd,
                                 size_t size_hint) {
  VALUE ary = (VALUE)closure;
  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyString8bitEncoding);
  RepeatedField_push_native(ary, &str);
  return (void*)str;
}

// Sets a non-repeated string field in a message.
static void* str_handler(void *closure,
                         const void *hd,
                         size_t size_hint) {
  MessageHeader* msg = closure;
  const field_handlerdata_t *fieldhandler = hd;

  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyStringUtf8Encoding);
  DEREF(msg, fieldhandler->ofs, VALUE) = str;
  set_hasbit(closure, fieldhandler->hasbit);
  return (void*)str;
}

// Sets a non-repeated 'bytes' field in a message.
static void* bytes_handler(void *closure,
                           const void *hd,
                           size_t size_hint) {
  MessageHeader* msg = closure;
  const field_handlerdata_t *fieldhandler = hd;

  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyString8bitEncoding);
  DEREF(msg, fieldhandler->ofs, VALUE) = str;
  set_hasbit(closure, fieldhandler->hasbit);
  return (void*)str;
}

static size_t stringdata_handler(void* closure, const void* hd,
                                 const char* str, size_t len,
                                 const upb_bufhandle* handle) {
  VALUE rb_str = (VALUE)closure;
  noleak_rb_str_cat(rb_str, str, len);
  return len;
}

static bool stringdata_end_handler(void* closure, const void* hd) {
  VALUE rb_str = (VALUE)closure;
  rb_obj_freeze(rb_str);
  return true;
}

static bool appendstring_end_handler(void* closure, const void* hd) {
  VALUE rb_str = (VALUE)closure;
  rb_obj_freeze(rb_str);
  return true;
}

// Appends a submessage to a repeated field (a regular Ruby array for now).
static void *appendsubmsg_handler(void *closure, const void *hd) {
  VALUE ary = (VALUE)closure;
  const submsg_handlerdata_t *submsgdata = hd;
  MessageHeader* submsg;

  VALUE submsg_rb = rb_class_new_instance(0, NULL, submsgdata->subklass);
  RepeatedField_push(ary, submsg_rb);

  TypedData_Get_Struct(submsg_rb, MessageHeader, &Message_type, submsg);
  return submsg;
}

// Sets a non-repeated submessage field in a message.
static void *submsg_handler(void *closure, const void *hd) {
  MessageHeader* msg = closure;
  const submsg_handlerdata_t* submsgdata = hd;
  VALUE submsg_rb;
  MessageHeader* submsg;

  if (DEREF(msg, submsgdata->ofs, VALUE) == Qnil) {
    DEREF(msg, submsgdata->ofs, VALUE) =
        rb_class_new_instance(0, NULL, submsgdata->subklass);
  }

  set_hasbit(closure, submsgdata->hasbit);

  submsg_rb = DEREF(msg, submsgdata->ofs, VALUE);
  TypedData_Get_Struct(submsg_rb, MessageHeader, &Message_type, submsg);

  return submsg;
}

// Handler data for startmap/endmap handlers.
typedef struct {
  size_t ofs;
  upb_fieldtype_t key_field_type;
  upb_fieldtype_t value_field_type;
  VALUE subklass;
} map_handlerdata_t;

// Temporary frame for map parsing: at the beginning of a map entry message, a
// submsg handler allocates a frame to hold (i) a reference to the Map object
// into which this message will be inserted and (ii) storage slots to
// temporarily hold the key and value for this map entry until the end of the
// submessage. When the submessage ends, another handler is called to insert the
// value into the map.
typedef struct {
  VALUE map;
  const map_handlerdata_t* handlerdata;
  char key_storage[NATIVE_SLOT_MAX_SIZE];
  char value_storage[NATIVE_SLOT_MAX_SIZE];
} map_parse_frame_t;

static void MapParseFrame_mark(void* _self) {
  map_parse_frame_t* frame = _self;

  // This shouldn't strictly be necessary since this should be rooted by the
  // message itself, but it can't hurt.
  rb_gc_mark(frame->map);

  native_slot_mark(frame->handlerdata->key_field_type, &frame->key_storage);
  native_slot_mark(frame->handlerdata->value_field_type, &frame->value_storage);
}

void MapParseFrame_free(void* self) {
  xfree(self);
}

rb_data_type_t MapParseFrame_type = {
  "MapParseFrame",
  { MapParseFrame_mark, MapParseFrame_free, NULL },
};

// Handler to begin a map entry: allocates a temporary frame. This is the
// 'startsubmsg' handler on the msgdef that contains the map field.
static void *startmap_handler(void *closure, const void *hd) {
  MessageHeader* msg = closure;
  const map_handlerdata_t* mapdata = hd;
  map_parse_frame_t* frame = ALLOC(map_parse_frame_t);
  VALUE map_rb = DEREF(msg, mapdata->ofs, VALUE);

  frame->handlerdata = mapdata;
  frame->map = map_rb;
  native_slot_init(mapdata->key_field_type, &frame->key_storage);
  native_slot_init(mapdata->value_field_type, &frame->value_storage);

  Map_set_frame(map_rb,
                TypedData_Wrap_Struct(rb_cObject, &MapParseFrame_type, frame));

  return frame;
}

static bool endmap_handler(void *closure, const void *hd) {
  MessageHeader* msg = closure;
  const map_handlerdata_t* mapdata = hd;
  VALUE map_rb = DEREF(msg, mapdata->ofs, VALUE);
  Map_set_frame(map_rb, Qnil);
  return true;
}

// Handler to end a map entry: inserts the value defined during the message into
// the map. This is the 'endmsg' handler on the map entry msgdef.
static bool endmapentry_handler(void* closure, const void* hd, upb_status* s) {
  map_parse_frame_t* frame = closure;
  const map_handlerdata_t* mapdata = hd;

  VALUE key = native_slot_get(
      mapdata->key_field_type, Qnil,
      &frame->key_storage);

  VALUE value = native_slot_get(
      mapdata->value_field_type, mapdata->subklass,
      &frame->value_storage);

  Map_index_set(frame->map, key, value);

  return true;
}

// Allocates a new map_handlerdata_t given the map entry message definition. If
// the offset of the field within the parent message is also given, that is
// added to the handler data as well. Note that this is called *twice* per map
// field: once in the parent message handler setup when setting the startsubmsg
// handler and once in the map entry message handler setup when setting the
// key/value and endmsg handlers. The reason is that there is no easy way to
// pass the handlerdata down to the sub-message handler setup.
static map_handlerdata_t* new_map_handlerdata(
    size_t ofs,
    const upb_msgdef* mapentry_def,
    const Descriptor* desc) {
  const upb_fielddef* key_field;
  const upb_fielddef* value_field;
  map_handlerdata_t* hd = ALLOC(map_handlerdata_t);
  hd->ofs = ofs;
  key_field = upb_msgdef_itof(mapentry_def, MAP_KEY_FIELD);
  assert(key_field != NULL);
  hd->key_field_type = upb_fielddef_type(key_field);
  value_field = upb_msgdef_itof(mapentry_def, MAP_VALUE_FIELD);
  assert(value_field != NULL);
  hd->value_field_type = upb_fielddef_type(value_field);
  hd->subklass = field_type_class(desc->layout, value_field);

  return hd;
}

// Handlers that set primitive values in oneofs.
#define DEFINE_ONEOF_HANDLER(type, ctype)                           \
  static bool oneof##type##_handler(void *closure, const void *hd,  \
                                     ctype val) {                   \
    const oneof_handlerdata_t *oneofdata = hd;                      \
    DEREF(closure, oneofdata->case_ofs, uint32_t) =                 \
        oneofdata->oneof_case_num;                                  \
    DEREF(closure, oneofdata->ofs, ctype) = val;                    \
    return true;                                                    \
  }

DEFINE_ONEOF_HANDLER(bool,   bool)
DEFINE_ONEOF_HANDLER(int32,  int32_t)
DEFINE_ONEOF_HANDLER(uint32, uint32_t)
DEFINE_ONEOF_HANDLER(float,  float)
DEFINE_ONEOF_HANDLER(int64,  int64_t)
DEFINE_ONEOF_HANDLER(uint64, uint64_t)
DEFINE_ONEOF_HANDLER(double, double)

#undef DEFINE_ONEOF_HANDLER

// Handlers for strings in a oneof.
static void *oneofstr_handler(void *closure,
                              const void *hd,
                              size_t size_hint) {
  MessageHeader* msg = closure;
  const oneof_handlerdata_t *oneofdata = hd;
  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyStringUtf8Encoding);
  DEREF(msg, oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;
  DEREF(msg, oneofdata->ofs, VALUE) = str;
  return (void*)str;
}

static void *oneofbytes_handler(void *closure,
                                const void *hd,
                                size_t size_hint) {
  MessageHeader* msg = closure;
  const oneof_handlerdata_t *oneofdata = hd;
  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyString8bitEncoding);
  DEREF(msg, oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;
  DEREF(msg, oneofdata->ofs, VALUE) = str;
  return (void*)str;
}

static bool oneofstring_end_handler(void* closure, const void* hd) {
  VALUE rb_str = rb_str_new2("");
  rb_obj_freeze(rb_str);
  return true;
}

// Handler for a submessage field in a oneof.
static void *oneofsubmsg_handler(void *closure,
                                 const void *hd) {
  MessageHeader* msg = closure;
  const oneof_handlerdata_t *oneofdata = hd;
  uint32_t oldcase = DEREF(msg, oneofdata->case_ofs, uint32_t);

  VALUE submsg_rb;
  MessageHeader* submsg;

  if (oldcase != oneofdata->oneof_case_num ||
      DEREF(msg, oneofdata->ofs, VALUE) == Qnil) {
    DEREF(msg, oneofdata->ofs, VALUE) =
        rb_class_new_instance(0, NULL, oneofdata->subklass);
  }
  // Set the oneof case *after* allocating the new class instance -- otherwise,
  // if the Ruby GC is invoked as part of a call into the VM, it might invoke
  // our mark routines, and our mark routines might see the case value
  // indicating a VALUE is present and expect a valid VALUE. See comment in
  // layout_set() for more detail: basically, the change to the value and the
  // case must be atomic w.r.t. the Ruby VM.
  DEREF(msg, oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;

  submsg_rb = DEREF(msg, oneofdata->ofs, VALUE);
  TypedData_Get_Struct(submsg_rb, MessageHeader, &Message_type, submsg);
  return submsg;
}

// Set up handlers for a repeated field.
static void add_handlers_for_repeated_field(upb_handlers *h,
                                            const Descriptor* desc,
                                            const upb_fielddef *f,
                                            size_t offset) {
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;
  attr.handler_data = newhandlerdata(h, offset, -1);
  upb_handlers_setstartseq(h, f, startseq_handler, &attr);

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
    case UPB_TYPE_BYTES: {
      bool is_bytes = upb_fielddef_type(f) == UPB_TYPE_BYTES;
      upb_handlers_setstartstr(h, f, is_bytes ?
                               appendbytes_handler : appendstr_handler,
                               NULL);
      upb_handlers_setstring(h, f, stringdata_handler, NULL);
      upb_handlers_setendstr(h, f, appendstring_end_handler, NULL);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      VALUE subklass = field_type_class(desc->layout, f);
      upb_handlerattr attr = UPB_HANDLERATTR_INIT;
      attr.handler_data = newsubmsghandlerdata(h, 0, -1, subklass);
      upb_handlers_setstartsubmsg(h, f, appendsubmsg_handler, &attr);
      break;
    }
  }
}

// Set up handlers for a singular field.
static void add_handlers_for_singular_field(const Descriptor* desc,
                                            upb_handlers* h,
                                            const upb_fielddef* f,
                                            size_t offset, size_t hasbit_off) {
  // The offset we pass to UPB points to the start of the Message,
  // rather than the start of where our data is stored.
  int32_t hasbit = -1;
  if (hasbit_off != MESSAGE_FIELD_NO_HASBIT) {
    hasbit = hasbit_off + sizeof(MessageHeader) * 8;
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
      upb_msg_setscalarhandler(h, f, offset, hasbit);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      bool is_bytes = upb_fielddef_type(f) == UPB_TYPE_BYTES;
      upb_handlerattr attr = UPB_HANDLERATTR_INIT;
      attr.handler_data = newhandlerdata(h, offset, hasbit);
      upb_handlers_setstartstr(h, f,
                               is_bytes ? bytes_handler : str_handler,
                               &attr);
      upb_handlers_setstring(h, f, stringdata_handler, &attr);
      upb_handlers_setendstr(h, f, stringdata_end_handler, &attr);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      upb_handlerattr attr = UPB_HANDLERATTR_INIT;
      attr.handler_data = newsubmsghandlerdata(
          h, offset, hasbit, field_type_class(desc->layout, f));
      upb_handlers_setstartsubmsg(h, f, submsg_handler, &attr);
      break;
    }
  }
}

// Adds handlers to a map field.
static void add_handlers_for_mapfield(upb_handlers* h,
                                      const upb_fielddef* fielddef,
                                      size_t offset,
                                      const Descriptor* desc) {
  const upb_msgdef* map_msgdef = upb_fielddef_msgsubdef(fielddef);
  map_handlerdata_t* hd = new_map_handlerdata(offset, map_msgdef, desc);
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;

  upb_handlers_addcleanup(h, hd, xfree);
  attr.handler_data = hd;
  upb_handlers_setstartsubmsg(h, fielddef, startmap_handler, &attr);
  upb_handlers_setendsubmsg(h, fielddef, endmap_handler, &attr);
}

// Adds handlers to a map-entry msgdef.
static void add_handlers_for_mapentry(const upb_msgdef* msgdef, upb_handlers* h,
                                      const Descriptor* desc) {
  const upb_fielddef* key_field = map_entry_key(msgdef);
  const upb_fielddef* value_field = map_entry_value(msgdef);
  map_handlerdata_t* hd = new_map_handlerdata(0, msgdef, desc);
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;

  upb_handlers_addcleanup(h, hd, xfree);
  attr.handler_data = hd;
  upb_handlers_setendmsg(h, endmapentry_handler, &attr);

  add_handlers_for_singular_field(
      desc, h, key_field,
      offsetof(map_parse_frame_t, key_storage),
      MESSAGE_FIELD_NO_HASBIT);
  add_handlers_for_singular_field(
      desc, h, value_field,
      offsetof(map_parse_frame_t, value_storage),
      MESSAGE_FIELD_NO_HASBIT);
}

// Set up handlers for a oneof field.
static void add_handlers_for_oneof_field(upb_handlers *h,
                                         const upb_fielddef *f,
                                         size_t offset,
                                         size_t oneof_case_offset,
                                         const Descriptor* desc) {
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;
  attr.handler_data =
      newoneofhandlerdata(h, offset, oneof_case_offset, f, desc);

  switch (upb_fielddef_type(f)) {

#define SET_HANDLER(utype, ltype)                                 \
  case utype:                                                     \
    upb_handlers_set##ltype(h, f, oneof##ltype##_handler, &attr); \
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
    case UPB_TYPE_BYTES: {
      bool is_bytes = upb_fielddef_type(f) == UPB_TYPE_BYTES;
      upb_handlers_setstartstr(h, f, is_bytes ?
                               oneofbytes_handler : oneofstr_handler,
                               &attr);
      upb_handlers_setstring(h, f, stringdata_handler, NULL);
      upb_handlers_setendstr(h, f, oneofstring_end_handler, &attr);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      upb_handlers_setstartsubmsg(h, f, oneofsubmsg_handler, &attr);
      break;
    }
  }
}

static bool unknown_field_handler(void* closure, const void* hd,
                                  const char* buf, size_t size) {
  MessageHeader* msg = (MessageHeader*)closure;
  UPB_UNUSED(hd);

  if (msg->unknown_fields == NULL) {
    msg->unknown_fields = malloc(sizeof(stringsink));
    stringsink_init(msg->unknown_fields);
  }

  stringsink_string(msg->unknown_fields, NULL, buf, size, NULL);

  return true;
}

void add_handlers_for_message(const void *closure, upb_handlers *h) {
  const VALUE descriptor_pool = (VALUE)closure;
  const upb_msgdef* msgdef = upb_handlers_msgdef(h);
  Descriptor* desc =
      ruby_to_Descriptor(get_msgdef_obj(descriptor_pool, msgdef));
  upb_msg_field_iter i;
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;

  // Ensure layout exists. We may be invoked to create handlers for a given
  // message if we are included as a submsg of another message type before our
  // class is actually built, so to work around this, we just create the layout
  // (and handlers, in the class-building function) on-demand.
  if (desc->layout == NULL) {
    create_layout(desc);
  }

  // If this is a mapentry message type, set up a special set of handlers and
  // bail out of the normal (user-defined) message type handling.
  if (upb_msgdef_mapentry(msgdef)) {
    add_handlers_for_mapentry(msgdef, h, desc);
    return;
  }

  upb_handlers_setunknown(h, unknown_field_handler, &attr);

  for (upb_msg_field_begin(&i, desc->msgdef);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    const upb_oneofdef *oneof = upb_fielddef_containingoneof(f);
    size_t offset = desc->layout->fields[upb_fielddef_index(f)].offset +
        sizeof(MessageHeader);

    if (oneof) {
      size_t oneof_case_offset =
          desc->layout->oneofs[upb_oneofdef_index(oneof)].case_offset +
          sizeof(MessageHeader);
      add_handlers_for_oneof_field(h, f, offset, oneof_case_offset, desc);
    } else if (is_map_field(f)) {
      add_handlers_for_mapfield(h, f, offset, desc);
    } else if (upb_fielddef_isseq(f)) {
      add_handlers_for_repeated_field(h, desc, f, offset);
    } else {
      add_handlers_for_singular_field(
          desc, h, f, offset,
          desc->layout->fields[upb_fielddef_index(f)].hasbit);
    }
  }
}

// Constructs the handlers for filling a message's data into an in-memory
// object.
const upb_handlers* get_fill_handlers(Descriptor* desc) {
  DescriptorPool* pool = ruby_to_DescriptorPool(desc->descriptor_pool);
  return upb_handlercache_get(pool->fill_handler_cache, desc->msgdef);
}

static const upb_pbdecodermethod *msgdef_decodermethod(Descriptor* desc) {
  DescriptorPool* pool = ruby_to_DescriptorPool(desc->descriptor_pool);
  return upb_pbcodecache_get(pool->fill_method_cache, desc->msgdef);
}

static const upb_json_parsermethod *msgdef_jsonparsermethod(Descriptor* desc) {
  DescriptorPool* pool = ruby_to_DescriptorPool(desc->descriptor_pool);
  return upb_json_codecache_get(pool->json_fill_method_cache, desc->msgdef);
}

static const upb_handlers* msgdef_pb_serialize_handlers(Descriptor* desc) {
  DescriptorPool* pool = ruby_to_DescriptorPool(desc->descriptor_pool);
  return upb_handlercache_get(pool->pb_serialize_handler_cache, desc->msgdef);
}

static const upb_handlers* msgdef_json_serialize_handlers(
    Descriptor* desc, bool preserve_proto_fieldnames) {
  DescriptorPool* pool = ruby_to_DescriptorPool(desc->descriptor_pool);
  if (preserve_proto_fieldnames) {
    return upb_handlercache_get(pool->json_serialize_handler_preserve_cache,
                                desc->msgdef);
  } else {
    return upb_handlercache_get(pool->json_serialize_handler_cache,
                                desc->msgdef);
  }
}


// Stack-allocated context during an encode/decode operation. Contains the upb
// environment and its stack-based allocator, an initial buffer for allocations
// to avoid malloc() when possible, and a template for Ruby exception messages
// if any error occurs.
#define STACK_ENV_STACKBYTES 4096
typedef struct {
  upb_arena *arena;
  upb_status status;
  const char* ruby_error_template;
  char allocbuf[STACK_ENV_STACKBYTES];
} stackenv;

static void stackenv_init(stackenv* se, const char* errmsg);
static void stackenv_uninit(stackenv* se);

static void stackenv_init(stackenv* se, const char* errmsg) {
  se->ruby_error_template = errmsg;
  se->arena =
      upb_arena_init(se->allocbuf, sizeof(se->allocbuf), &upb_alloc_global);
  upb_status_clear(&se->status);
}

static void stackenv_uninit(stackenv* se) {
  upb_arena_free(se->arena);

  if (!upb_ok(&se->status)) {
    // TODO(haberman): have a way to verify that this is actually a parse error,
    // instead of just throwing "parse error" unconditionally.
    VALUE errmsg = rb_str_new2(upb_status_errmsg(&se->status));
    rb_raise(cParseError, se->ruby_error_template, errmsg);
  }
}

/*
 * call-seq:
 *     MessageClass.decode(data) => message
 *
 * Decodes the given data (as a string containing bytes in protocol buffers wire
 * format) under the interpretration given by this message class's definition
 * and returns a message object with the corresponding field values.
 */
VALUE Message_decode(VALUE klass, VALUE data) {
  VALUE descriptor = rb_ivar_get(klass, descriptor_instancevar_interned);
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  VALUE msgklass = Descriptor_msgclass(descriptor);
  VALUE msg_rb;
  MessageHeader* msg;

  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for binary protobuf data.");
  }

  msg_rb = rb_class_new_instance(0, NULL, msgklass);
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);

  {
    const upb_pbdecodermethod* method = msgdef_decodermethod(desc);
    const upb_handlers* h = upb_pbdecodermethod_desthandlers(method);
    stackenv se;
    upb_sink sink;
    upb_pbdecoder* decoder;
    stackenv_init(&se, "Error occurred during parsing: %" PRIsVALUE);

    upb_sink_reset(&sink, h, msg);
    decoder = upb_pbdecoder_create(se.arena, method, sink, &se.status);
    upb_bufsrc_putbuf(RSTRING_PTR(data), RSTRING_LEN(data),
                      upb_pbdecoder_input(decoder));

    stackenv_uninit(&se);
  }

  return msg_rb;
}

/*
 * call-seq:
 *     MessageClass.decode_json(data, options = {}) => message
 *
 * Decodes the given data (as a string containing bytes in protocol buffers wire
 * format) under the interpretration given by this message class's definition
 * and returns a message object with the corresponding field values.
 *
 *  @param options [Hash] options for the decoder
 *   ignore_unknown_fields: set true to ignore unknown fields (default is to
 *   raise an error)
 */
VALUE Message_decode_json(int argc, VALUE* argv, VALUE klass) {
  VALUE descriptor = rb_ivar_get(klass, descriptor_instancevar_interned);
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  VALUE msgklass = Descriptor_msgclass(descriptor);
  VALUE msg_rb;
  VALUE data = argv[0];
  VALUE ignore_unknown_fields = Qfalse;
  MessageHeader* msg;

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

  if (argc == 2) {
    VALUE hash_args = argv[1];
    if (TYPE(hash_args) != T_HASH) {
      rb_raise(rb_eArgError, "Expected hash arguments.");
    }

    ignore_unknown_fields = rb_hash_lookup2(
        hash_args, ID2SYM(rb_intern("ignore_unknown_fields")), Qfalse);
  }

  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for JSON data.");
  }

  // TODO(cfallin): Check and respect string encoding. If not UTF-8, we need to
  // convert, because string handlers pass data directly to message string
  // fields.

  msg_rb = rb_class_new_instance(0, NULL, msgklass);
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);

  {
    const upb_json_parsermethod* method = msgdef_jsonparsermethod(desc);
    stackenv se;
    upb_sink sink;
    upb_json_parser* parser;
    DescriptorPool* pool = ruby_to_DescriptorPool(generated_pool);
    stackenv_init(&se, "Error occurred during parsing: %" PRIsVALUE);

    upb_sink_reset(&sink, get_fill_handlers(desc), msg);
    parser = upb_json_parser_create(se.arena, method, pool->symtab, sink,
                                    &se.status, RTEST(ignore_unknown_fields));
    upb_bufsrc_putbuf(RSTRING_PTR(data), RSTRING_LEN(data),
                      upb_json_parser_input(parser));

    stackenv_uninit(&se);
  }

  return msg_rb;
}

// -----------------------------------------------------------------------------
// Serializing.
// -----------------------------------------------------------------------------

/* msgvisitor *****************************************************************/

static void putmsg(VALUE msg, const Descriptor* desc, upb_sink sink, int depth,
                   bool emit_defaults, bool is_json, bool open_msg);

static upb_selector_t getsel(const upb_fielddef *f, upb_handlertype_t type) {
  upb_selector_t ret;
  bool ok = upb_handlers_getselector(f, type, &ret);
  UPB_ASSERT(ok);
  return ret;
}

static void putstr(VALUE str, const upb_fielddef *f, upb_sink sink) {
  upb_sink subsink;

  if (str == Qnil) return;

  assert(BUILTIN_TYPE(str) == RUBY_T_STRING);

  // We should be guaranteed that the string has the correct encoding because
  // we ensured this at assignment time and then froze the string.
  if (upb_fielddef_type(f) == UPB_TYPE_STRING) {
    assert(rb_enc_from_index(ENCODING_GET(str)) == kRubyStringUtf8Encoding);
  } else {
    assert(rb_enc_from_index(ENCODING_GET(str)) == kRubyString8bitEncoding);
  }

  upb_sink_startstr(sink, getsel(f, UPB_HANDLER_STARTSTR), RSTRING_LEN(str),
                    &subsink);
  upb_sink_putstring(subsink, getsel(f, UPB_HANDLER_STRING), RSTRING_PTR(str),
                     RSTRING_LEN(str), NULL);
  upb_sink_endstr(sink, getsel(f, UPB_HANDLER_ENDSTR));
}

static void putsubmsg(VALUE submsg, const upb_fielddef *f, upb_sink sink,
                      int depth, bool emit_defaults, bool is_json) {
  upb_sink subsink;
  VALUE descriptor;
  Descriptor* subdesc;

  if (submsg == Qnil) return;

  descriptor = rb_ivar_get(submsg, descriptor_instancevar_interned);
  subdesc = ruby_to_Descriptor(descriptor);

  upb_sink_startsubmsg(sink, getsel(f, UPB_HANDLER_STARTSUBMSG), &subsink);
  putmsg(submsg, subdesc, subsink, depth + 1, emit_defaults, is_json, true);
  upb_sink_endsubmsg(sink, getsel(f, UPB_HANDLER_ENDSUBMSG));
}

static void putary(VALUE ary, const upb_fielddef* f, upb_sink sink, int depth,
                   bool emit_defaults, bool is_json) {
  upb_sink subsink;
  upb_fieldtype_t type = upb_fielddef_type(f);
  upb_selector_t sel = 0;
  int size;
  int i;

  if (ary == Qnil) return;
  if (!emit_defaults && NUM2INT(RepeatedField_length(ary)) == 0) return;

  size = NUM2INT(RepeatedField_length(ary));
  if (size == 0 && !emit_defaults) return;

  upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);

  if (upb_fielddef_isprimitive(f)) {
    sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  }

  for (i = 0; i < size; i++) {
    void* memory = RepeatedField_index_native(ary, i);
    switch (type) {
#define T(upbtypeconst, upbtype, ctype)                     \
  case upbtypeconst:                                        \
    upb_sink_put##upbtype(subsink, sel, *((ctype*)memory)); \
    break;

      T(UPB_TYPE_FLOAT,  float,  float)
      T(UPB_TYPE_DOUBLE, double, double)
      T(UPB_TYPE_BOOL,   bool,   int8_t)
      case UPB_TYPE_ENUM:
      T(UPB_TYPE_INT32,  int32,  int32_t)
      T(UPB_TYPE_UINT32, uint32, uint32_t)
      T(UPB_TYPE_INT64,  int64,  int64_t)
      T(UPB_TYPE_UINT64, uint64, uint64_t)

      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        putstr(*((VALUE *)memory), f, subsink);
        break;
      case UPB_TYPE_MESSAGE:
        putsubmsg(*((VALUE*)memory), f, subsink, depth, emit_defaults, is_json);
        break;

#undef T

    }
  }
  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static void put_ruby_value(VALUE value, const upb_fielddef* f, VALUE type_class,
                           int depth, upb_sink sink, bool emit_defaults,
                           bool is_json) {
  upb_selector_t sel = 0;

  if (depth > ENCODE_MAX_NESTING) {
    rb_raise(rb_eRuntimeError,
             "Maximum recursion depth exceeded during encoding.");
  }

  if (upb_fielddef_isprimitive(f)) {
    sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  }

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
      upb_sink_putint32(sink, sel, NUM2INT(value));
      break;
    case UPB_TYPE_INT64:
      upb_sink_putint64(sink, sel, NUM2LL(value));
      break;
    case UPB_TYPE_UINT32:
      upb_sink_putuint32(sink, sel, NUM2UINT(value));
      break;
    case UPB_TYPE_UINT64:
      upb_sink_putuint64(sink, sel, NUM2ULL(value));
      break;
    case UPB_TYPE_FLOAT:
      upb_sink_putfloat(sink, sel, NUM2DBL(value));
      break;
    case UPB_TYPE_DOUBLE:
      upb_sink_putdouble(sink, sel, NUM2DBL(value));
      break;
    case UPB_TYPE_ENUM: {
      if (TYPE(value) == T_SYMBOL) {
        value = rb_funcall(type_class, rb_intern("resolve"), 1, value);
      }
      upb_sink_putint32(sink, sel, NUM2INT(value));
      break;
    }
    case UPB_TYPE_BOOL:
      upb_sink_putbool(sink, sel, value == Qtrue);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      putstr(value, f, sink);
      break;
    case UPB_TYPE_MESSAGE:
      putsubmsg(value, f, sink, depth, emit_defaults, is_json);
  }
}

static void putmap(VALUE map, const upb_fielddef* f, upb_sink sink, int depth,
                   bool emit_defaults, bool is_json) {
  Map* self;
  upb_sink subsink;
  const upb_fielddef* key_field;
  const upb_fielddef* value_field;
  Map_iter it;

  if (map == Qnil) return;
  if (!emit_defaults && Map_length(map) == 0) return;

  self = ruby_to_Map(map);

  upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);

  assert(upb_fielddef_type(f) == UPB_TYPE_MESSAGE);
  key_field = map_field_key(f);
  value_field = map_field_value(f);

  for (Map_begin(map, &it); !Map_done(&it); Map_next(&it)) {
    VALUE key = Map_iter_key(&it);
    VALUE value = Map_iter_value(&it);
    upb_status status;

    upb_sink entry_sink;
    upb_sink_startsubmsg(subsink, getsel(f, UPB_HANDLER_STARTSUBMSG),
                         &entry_sink);
    upb_sink_startmsg(entry_sink);

    put_ruby_value(key, key_field, Qnil, depth + 1, entry_sink, emit_defaults,
                   is_json);
    put_ruby_value(value, value_field, self->value_type_class, depth + 1,
                   entry_sink, emit_defaults, is_json);

    upb_sink_endmsg(entry_sink, &status);
    upb_sink_endsubmsg(subsink, getsel(f, UPB_HANDLER_ENDSUBMSG));
  }

  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static const upb_handlers* msgdef_json_serialize_handlers(
    Descriptor* desc, bool preserve_proto_fieldnames);

static void putjsonany(VALUE msg_rb, const Descriptor* desc, upb_sink sink,
                       int depth, bool emit_defaults) {
  upb_status status;
  MessageHeader* msg = NULL;
  const upb_fielddef* type_field = upb_msgdef_itof(desc->msgdef, UPB_ANY_TYPE);
  const upb_fielddef* value_field = upb_msgdef_itof(desc->msgdef, UPB_ANY_VALUE);

  size_t type_url_offset;
  VALUE type_url_str_rb;
  const upb_msgdef *payload_type = NULL;

  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);

  upb_sink_startmsg(sink);

  /* Handle type url */
  type_url_offset = desc->layout->fields[upb_fielddef_index(type_field)].offset;
  type_url_str_rb = DEREF(Message_data(msg), type_url_offset, VALUE);
  if (RSTRING_LEN(type_url_str_rb) > 0) {
    putstr(type_url_str_rb, type_field, sink);
  }

  {
    const char* type_url_str = RSTRING_PTR(type_url_str_rb);
    size_t type_url_len = RSTRING_LEN(type_url_str_rb);
    DescriptorPool* pool = ruby_to_DescriptorPool(generated_pool);

    if (type_url_len <= 20 ||
        strncmp(type_url_str, "type.googleapis.com/", 20) != 0) {
      rb_raise(rb_eRuntimeError, "Invalid type url: %s", type_url_str);
      return;
    }

    /* Resolve type url */
    type_url_str += 20;
    type_url_len -= 20;

    payload_type = upb_symtab_lookupmsg2(
        pool->symtab, type_url_str, type_url_len);
    if (payload_type == NULL) {
      rb_raise(rb_eRuntimeError, "Unknown type: %s", type_url_str);
      return;
    }
  }

  {
    uint32_t value_offset;
    VALUE value_str_rb;
    size_t value_len;

    value_offset = desc->layout->fields[upb_fielddef_index(value_field)].offset;
    value_str_rb = DEREF(Message_data(msg), value_offset, VALUE);
    value_len = RSTRING_LEN(value_str_rb);

    if (value_len > 0) {
      VALUE payload_desc_rb = get_msgdef_obj(generated_pool, payload_type);
      Descriptor* payload_desc = ruby_to_Descriptor(payload_desc_rb);
      VALUE payload_class = Descriptor_msgclass(payload_desc_rb);
      upb_sink subsink;
      bool is_wellknown;

      VALUE payload_msg_rb = Message_decode(payload_class, value_str_rb);

      is_wellknown =
          upb_msgdef_wellknowntype(payload_desc->msgdef) !=
              UPB_WELLKNOWN_UNSPECIFIED;
      if (is_wellknown) {
        upb_sink_startstr(sink, getsel(value_field, UPB_HANDLER_STARTSTR), 0,
                          &subsink);
      }

      subsink.handlers =
          msgdef_json_serialize_handlers(payload_desc, true);
      subsink.closure = sink.closure;
      putmsg(payload_msg_rb, payload_desc, subsink, depth, emit_defaults, true,
             is_wellknown);
    }
  }

  upb_sink_endmsg(sink, &status);
}

static void putjsonlistvalue(
    VALUE msg_rb, const Descriptor* desc,
    upb_sink sink, int depth, bool emit_defaults) {
  upb_status status;
  upb_sink subsink;
  MessageHeader* msg = NULL;
  const upb_fielddef* f = upb_msgdef_itof(desc->msgdef, 1);
  uint32_t offset =
      desc->layout->fields[upb_fielddef_index(f)].offset +
      sizeof(MessageHeader);
  VALUE ary;

  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);

  upb_sink_startmsg(sink);

  ary = DEREF(msg, offset, VALUE);

  if (ary == Qnil || RepeatedField_size(ary) == 0) {
    upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);
    upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
  } else {
    putary(ary, f, sink, depth, emit_defaults, true);
  }

  upb_sink_endmsg(sink, &status);
}

static void putmsg(VALUE msg_rb, const Descriptor* desc,
                   upb_sink sink, int depth, bool emit_defaults,
                   bool is_json, bool open_msg) {
  MessageHeader* msg;
  upb_msg_field_iter i;
  upb_status status;

  if (is_json &&
      upb_msgdef_wellknowntype(desc->msgdef) == UPB_WELLKNOWN_ANY) {
    putjsonany(msg_rb, desc, sink, depth, emit_defaults);
    return;
  }

  if (is_json &&
      upb_msgdef_wellknowntype(desc->msgdef) == UPB_WELLKNOWN_LISTVALUE) {
    putjsonlistvalue(msg_rb, desc, sink, depth, emit_defaults);
    return;
  }

  if (open_msg) {
    upb_sink_startmsg(sink);
  }

  // Protect against cycles (possible because users may freely reassign message
  // and repeated fields) by imposing a maximum recursion depth.
  if (depth > ENCODE_MAX_NESTING) {
    rb_raise(rb_eRuntimeError,
             "Maximum recursion depth exceeded during encoding.");
  }

  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);

  if (desc != msg->descriptor) {
    rb_raise(rb_eArgError,
             "The type of given msg is '%s', expect '%s'.",
             upb_msgdef_fullname(msg->descriptor->msgdef),
             upb_msgdef_fullname(desc->msgdef));
  }

  for (upb_msg_field_begin(&i, desc->msgdef);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    const upb_oneofdef *oneof = upb_fielddef_containingoneof(f);
    bool is_matching_oneof = false;
    uint32_t offset =
        desc->layout->fields[upb_fielddef_index(f)].offset +
        sizeof(MessageHeader);

    if (oneof) {
      uint32_t oneof_case =
          slot_read_oneof_case(desc->layout, Message_data(msg), oneof);
      // For a oneof, check that this field is actually present -- skip all the
      // below if not.
      if (oneof_case != upb_fielddef_number(f)) {
        continue;
      }
      // Otherwise, fall through to the appropriate singular-field handler
      // below.
      is_matching_oneof = true;
    }

    if (is_map_field(f)) {
      VALUE map = DEREF(msg, offset, VALUE);
      if (map != Qnil || emit_defaults) {
        putmap(map, f, sink, depth, emit_defaults, is_json);
      }
    } else if (upb_fielddef_isseq(f)) {
      VALUE ary = DEREF(msg, offset, VALUE);
      if (ary != Qnil) {
        putary(ary, f, sink, depth, emit_defaults, is_json);
      }
    } else if (upb_fielddef_isstring(f)) {
      VALUE str = DEREF(msg, offset, VALUE);
      bool is_default = false;

      if (upb_msgdef_syntax(desc->msgdef) == UPB_SYNTAX_PROTO2) {
        is_default = layout_has(desc->layout, Message_data(msg), f) == Qfalse;
      } else if (upb_msgdef_syntax(desc->msgdef) == UPB_SYNTAX_PROTO3) {
        is_default = RSTRING_LEN(str) == 0;
      }

      if (is_matching_oneof || emit_defaults || !is_default) {
        putstr(str, f, sink);
      }
    } else if (upb_fielddef_issubmsg(f)) {
      putsubmsg(DEREF(msg, offset, VALUE), f, sink, depth,
                emit_defaults, is_json);
    } else {
      upb_selector_t sel = getsel(f, upb_handlers_getprimitivehandlertype(f));

#define T(upbtypeconst, upbtype, ctype, default_value)                       \
  case upbtypeconst: {                                                       \
    ctype value = DEREF(msg, offset, ctype);                                 \
    bool is_default = false;                                                 \
    if (upb_fielddef_haspresence(f)) {                                       \
      is_default = layout_has(desc->layout, Message_data(msg), f) == Qfalse; \
    } else if (upb_msgdef_syntax(desc->msgdef) == UPB_SYNTAX_PROTO3) {       \
      is_default = default_value == value;                                   \
    }                                                                        \
    if (is_matching_oneof || emit_defaults || !is_default) {                 \
      upb_sink_put##upbtype(sink, sel, value);                               \
    }                                                                        \
  } break;

      switch (upb_fielddef_type(f)) {
        T(UPB_TYPE_FLOAT,  float,  float, 0.0)
        T(UPB_TYPE_DOUBLE, double, double, 0.0)
        T(UPB_TYPE_BOOL,   bool,   uint8_t, 0)
        case UPB_TYPE_ENUM:
        T(UPB_TYPE_INT32,  int32,  int32_t, 0)
        T(UPB_TYPE_UINT32, uint32, uint32_t, 0)
        T(UPB_TYPE_INT64,  int64,  int64_t, 0)
        T(UPB_TYPE_UINT64, uint64, uint64_t, 0)

        case UPB_TYPE_STRING:
        case UPB_TYPE_BYTES:
        case UPB_TYPE_MESSAGE: rb_raise(rb_eRuntimeError, "Internal error.");
      }

#undef T

    }
  }

  {
    stringsink* unknown = msg->unknown_fields;
    if (unknown != NULL) {
      upb_sink_putunknown(sink, unknown->ptr, unknown->len);
    }
  }

  if (open_msg) {
    upb_sink_endmsg(sink, &status);
  }
}

/*
 * call-seq:
 *     MessageClass.encode(msg) => bytes
 *
 * Encodes the given message object to its serialized form in protocol buffers
 * wire format.
 */
VALUE Message_encode(VALUE klass, VALUE msg_rb) {
  VALUE descriptor = rb_ivar_get(klass, descriptor_instancevar_interned);
  Descriptor* desc = ruby_to_Descriptor(descriptor);

  stringsink sink;
  stringsink_init(&sink);

  {
    const upb_handlers* serialize_handlers =
        msgdef_pb_serialize_handlers(desc);

    stackenv se;
    upb_pb_encoder* encoder;
    VALUE ret;

    stackenv_init(&se, "Error occurred during encoding: %" PRIsVALUE);
    encoder = upb_pb_encoder_create(se.arena, serialize_handlers, sink.sink);

    putmsg(msg_rb, desc, upb_pb_encoder_input(encoder), 0, false, false, true);

    ret = rb_str_new(sink.ptr, sink.len);

    stackenv_uninit(&se);
    stringsink_uninit(&sink);

    return ret;
  }
}

/*
 * call-seq:
 *     MessageClass.encode_json(msg, options = {}) => json_string
 *
 * Encodes the given message object into its serialized JSON representation.
 * @param options [Hash] options for the decoder
 *  preserve_proto_fieldnames: set true to use original fieldnames (default is to camelCase)
 *  emit_defaults: set true to emit 0/false values (default is to omit them)
 */
VALUE Message_encode_json(int argc, VALUE* argv, VALUE klass) {
  VALUE descriptor = rb_ivar_get(klass, descriptor_instancevar_interned);
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  VALUE msg_rb;
  VALUE preserve_proto_fieldnames = Qfalse;
  VALUE emit_defaults = Qfalse;
  stringsink sink;

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

  msg_rb = argv[0];

  if (argc == 2) {
    VALUE hash_args = argv[1];
    if (TYPE(hash_args) != T_HASH) {
      rb_raise(rb_eArgError, "Expected hash arguments.");
    }
    preserve_proto_fieldnames = rb_hash_lookup2(
        hash_args, ID2SYM(rb_intern("preserve_proto_fieldnames")), Qfalse);

    emit_defaults = rb_hash_lookup2(
        hash_args, ID2SYM(rb_intern("emit_defaults")), Qfalse);
  }

  stringsink_init(&sink);

  {
    const upb_handlers* serialize_handlers =
        msgdef_json_serialize_handlers(desc, RTEST(preserve_proto_fieldnames));
    upb_json_printer* printer;
    stackenv se;
    VALUE ret;

    stackenv_init(&se, "Error occurred during encoding: %" PRIsVALUE);
    printer = upb_json_printer_create(se.arena, serialize_handlers, sink.sink);

    putmsg(msg_rb, desc, upb_json_printer_input(printer), 0,
           RTEST(emit_defaults), true, true);

    ret = rb_enc_str_new(sink.ptr, sink.len, rb_utf8_encoding());

    stackenv_uninit(&se);
    stringsink_uninit(&sink);

    return ret;
  }
}

static void discard_unknown(VALUE msg_rb, const Descriptor* desc) {
  MessageHeader* msg;
  upb_msg_field_iter it;

  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);

  {
    stringsink* unknown = msg->unknown_fields;
    if (unknown != NULL) {
      stringsink_uninit(unknown);
      msg->unknown_fields = NULL;
    }
  }

  for (upb_msg_field_begin(&it, desc->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    upb_fielddef *f = upb_msg_iter_field(&it);
    const upb_oneofdef *oneof = upb_fielddef_containingoneof(f);
    uint32_t offset =
        desc->layout->fields[upb_fielddef_index(f)].offset +
        sizeof(MessageHeader);

    if (oneof) {
      uint32_t oneof_case =
          slot_read_oneof_case(desc->layout, Message_data(msg), oneof);
      // For a oneof, check that this field is actually present -- skip all the
      // below if not.
      if (oneof_case != upb_fielddef_number(f)) {
        continue;
      }
      // Otherwise, fall through to the appropriate singular-field handler
      // below.
    }

    if (!upb_fielddef_issubmsg(f)) {
      continue;
    }

    if (is_map_field(f)) {
      VALUE map;
      Map_iter map_it;

      if (!upb_fielddef_issubmsg(map_field_value(f))) continue;
      map = DEREF(msg, offset, VALUE);
      if (map == Qnil) continue;
      for (Map_begin(map, &map_it); !Map_done(&map_it); Map_next(&map_it)) {
        VALUE submsg = Map_iter_value(&map_it);
        VALUE descriptor = rb_ivar_get(submsg, descriptor_instancevar_interned);
        const Descriptor* subdesc = ruby_to_Descriptor(descriptor);
        discard_unknown(submsg, subdesc);
      }
    } else if (upb_fielddef_isseq(f)) {
      VALUE ary = DEREF(msg, offset, VALUE);
      int size;
      int i;

      if (ary == Qnil) continue;
      size = NUM2INT(RepeatedField_length(ary));
      for (i = 0; i < size; i++) {
        void* memory = RepeatedField_index_native(ary, i);
        VALUE submsg = *((VALUE *)memory);
        VALUE descriptor = rb_ivar_get(submsg, descriptor_instancevar_interned);
        const Descriptor* subdesc = ruby_to_Descriptor(descriptor);
        discard_unknown(submsg, subdesc);
      }
    } else {
      VALUE submsg = DEREF(msg, offset, VALUE);
      VALUE descriptor;
      const Descriptor* subdesc;

      if (submsg == Qnil) continue;
      descriptor = rb_ivar_get(submsg, descriptor_instancevar_interned);
      subdesc = ruby_to_Descriptor(descriptor);
      discard_unknown(submsg, subdesc);
    }
  }
}

/*
 * call-seq:
 *     Google::Protobuf.discard_unknown(msg)
 *
 * Discard unknown fields in the given message object and recursively discard
 * unknown fields in submessages.
 */
VALUE Google_Protobuf_discard_unknown(VALUE self, VALUE msg_rb) {
  VALUE klass = CLASS_OF(msg_rb);
  VALUE descriptor = rb_ivar_get(klass, descriptor_instancevar_interned);
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  if (klass == cRepeatedField || klass == cMap) {
    rb_raise(rb_eArgError, "Expected proto msg for discard unknown.");
  } else {
    discard_unknown(msg_rb, desc);
  }
  return Qnil;
}
