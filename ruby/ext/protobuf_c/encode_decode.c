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

// -----------------------------------------------------------------------------
// Parsing.
// -----------------------------------------------------------------------------

#define DEREF(msg, ofs, type) *(type*)(((uint8_t *)msg) + ofs)

// Creates a handlerdata that simply contains the offset for this field.
static const void* newhandlerdata(upb_handlers* h, uint32_t ofs) {
  size_t* hd_ofs = ALLOC(size_t);
  *hd_ofs = ofs;
  upb_handlers_addcleanup(h, hd_ofs, free);
  return hd_ofs;
}

typedef struct {
  size_t ofs;
  const upb_msgdef *md;
} submsg_handlerdata_t;

// Creates a handlerdata that contains offset and submessage type information.
static const void *newsubmsghandlerdata(upb_handlers* h, uint32_t ofs,
                                        const upb_fielddef* f) {
  submsg_handlerdata_t *hd = ALLOC(submsg_handlerdata_t);
  hd->ofs = ofs;
  hd->md = upb_fielddef_msgsubdef(f);
  upb_handlers_addcleanup(h, hd, free);
  return hd;
}

// A handler that starts a repeated field.  Gets the Repeated*Field instance for
// this field (such an instance always exists even in an empty message).
static void *startseq_handler(void* closure, const void* hd) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  return (void*)DEREF(Message_data(msg), *ofs, VALUE);
}

// Handlers that append primitive values to a repeated field (a regular Ruby
// array for now).
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

// Appends a string to a repeated field (a regular Ruby array for now).
static void* appendstr_handler(void *closure,
                               const void *hd,
                               size_t size_hint) {
  VALUE ary = (VALUE)closure;
  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyStringUtf8Encoding);
  RepeatedField_push(ary, str);
  return (void*)str;
}

// Appends a 'bytes' string to a repeated field (a regular Ruby array for now).
static void* appendbytes_handler(void *closure,
                                 const void *hd,
                                 size_t size_hint) {
  VALUE ary = (VALUE)closure;
  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyString8bitEncoding);
  RepeatedField_push(ary, str);
  return (void*)str;
}

// Sets a non-repeated string field in a message.
static void* str_handler(void *closure,
                         const void *hd,
                         size_t size_hint) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyStringUtf8Encoding);
  DEREF(Message_data(msg), *ofs, VALUE) = str;
  return (void*)str;
}

// Sets a non-repeated 'bytes' field in a message.
static void* bytes_handler(void *closure,
                           const void *hd,
                           size_t size_hint) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  VALUE str = rb_str_new2("");
  rb_enc_associate(str, kRubyString8bitEncoding);
  DEREF(Message_data(msg), *ofs, VALUE) = str;
  return (void*)str;
}

static size_t stringdata_handler(void* closure, const void* hd,
                                 const char* str, size_t len,
                                 const upb_bufhandle* handle) {
  VALUE rb_str = (VALUE)closure;
  rb_str_cat(rb_str, str, len);
  return len;
}

// Appends a submessage to a repeated field (a regular Ruby array for now).
static void *appendsubmsg_handler(void *closure, const void *hd) {
  VALUE ary = (VALUE)closure;
  const submsg_handlerdata_t *submsgdata = hd;
  VALUE subdesc =
      get_def_obj((void*)submsgdata->md);
  VALUE subklass = Descriptor_msgclass(subdesc);

  VALUE submsg_rb = rb_class_new_instance(0, NULL, subklass);
  RepeatedField_push(ary, submsg_rb);

  MessageHeader* submsg;
  TypedData_Get_Struct(submsg_rb, MessageHeader, &Message_type, submsg);
  return submsg;
}

// Sets a non-repeated submessage field in a message.
static void *submsg_handler(void *closure, const void *hd) {
  MessageHeader* msg = closure;
  const submsg_handlerdata_t* submsgdata = hd;
  VALUE subdesc =
      get_def_obj((void*)submsgdata->md);
  VALUE subklass = Descriptor_msgclass(subdesc);

  if (DEREF(Message_data(msg), submsgdata->ofs, VALUE) == Qnil) {
    DEREF(Message_data(msg), submsgdata->ofs, VALUE) =
        rb_class_new_instance(0, NULL, subklass);
  }

  VALUE submsg_rb = DEREF(Message_data(msg), submsgdata->ofs, VALUE);
  MessageHeader* submsg;
  TypedData_Get_Struct(submsg_rb, MessageHeader, &Message_type, submsg);
  return submsg;
}

static void add_handlers_for_message(const void *closure, upb_handlers *h) {
  Descriptor* desc = ruby_to_Descriptor(
      get_def_obj((void*)upb_handlers_msgdef(h)));
  // Ensure layout exists. We may be invoked to create handlers for a given
  // message if we are included as a submsg of another message type before our
  // class is actually built, so to work around this, we just create the layout
  // (and handlers, in the class-building function) on-demand.
  if (desc->layout == NULL) {
    desc->layout = create_layout(desc->msgdef);
  }

  upb_msg_iter i;

  for (upb_msg_begin(&i, desc->msgdef);
       !upb_msg_done(&i);
       upb_msg_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    size_t offset = desc->layout->offsets[upb_fielddef_index(f)];

    if (upb_fielddef_isseq(f)) {
      upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
      upb_handlerattr_sethandlerdata(&attr, newhandlerdata(h, offset));
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
        case UPB_TYPE_BYTES: {
          bool is_bytes = upb_fielddef_type(f) == UPB_TYPE_BYTES;
          upb_handlers_setstartstr(h, f, is_bytes ?
                                   appendbytes_handler : appendstr_handler,
                                   NULL);
          upb_handlers_setstring(h, f, stringdata_handler, NULL);
        }
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
        upb_shim_set(h, f, offset + sizeof(MessageHeader), -1);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES: {
        bool is_bytes = upb_fielddef_type(f) == UPB_TYPE_BYTES;
        upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
        upb_handlerattr_sethandlerdata(&attr, newhandlerdata(h, offset));
        upb_handlers_setstartstr(h, f,
                                 is_bytes ? bytes_handler : str_handler,
                                 &attr);
        upb_handlers_setstring(h, f, stringdata_handler, &attr);
        upb_handlerattr_uninit(&attr);
        break;
      }
      case UPB_TYPE_MESSAGE: {
        upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
        upb_handlerattr_sethandlerdata(&attr, newsubmsghandlerdata(h, offset, f));
        upb_handlers_setstartsubmsg(h, f, submsg_handler, &attr);
        upb_handlerattr_uninit(&attr);
        break;
      }
    }
  }
}

// Creates upb handlers for populating a message.
static const upb_handlers *new_fill_handlers(Descriptor* desc,
                                             const void* owner) {
  // TODO(cfallin, haberman): once upb gets a caching/memoization layer for
  // handlers, reuse subdef handlers so that e.g. if we already parse
  // B-with-field-of-type-C, we don't have to rebuild the whole hierarchy to
  // parse A-with-field-of-type-B-with-field-of-type-C.
  return upb_handlers_newfrozen(desc->msgdef, owner,
                                add_handlers_for_message, NULL);
}

// Constructs the handlers for filling a message's data into an in-memory
// object.
const upb_handlers* get_fill_handlers(Descriptor* desc) {
  if (!desc->fill_handlers) {
    desc->fill_handlers =
        new_fill_handlers(desc, &desc->fill_handlers);
  }
  return desc->fill_handlers;
}

// Constructs the upb decoder method for parsing messages of this type.
// This is called from the message class creation code.
const upb_pbdecodermethod *new_fillmsg_decodermethod(Descriptor* desc,
                                                     const void* owner) {
  const upb_handlers* handlers = get_fill_handlers(desc);
  upb_pbdecodermethodopts opts;
  upb_pbdecodermethodopts_init(&opts, handlers);

  const upb_pbdecodermethod *ret = upb_pbdecodermethod_new(&opts, owner);
  return ret;
}

static const upb_pbdecodermethod *msgdef_decodermethod(Descriptor* desc) {
  if (desc->fill_method == NULL) {
    desc->fill_method = new_fillmsg_decodermethod(
        desc, &desc->fill_method);
  }
  return desc->fill_method;
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
  VALUE descriptor = rb_iv_get(klass, kDescriptorInstanceVar);
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  VALUE msgklass = Descriptor_msgclass(descriptor);

  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for binary protobuf data.");
  }

  VALUE msg_rb = rb_class_new_instance(0, NULL, msgklass);
  MessageHeader* msg;
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);

  const upb_pbdecodermethod* method = msgdef_decodermethod(desc);
  const upb_handlers* h = upb_pbdecodermethod_desthandlers(method);
  upb_pbdecoder decoder;
  upb_sink sink;
  upb_status status = UPB_STATUS_INIT;

  upb_pbdecoder_init(&decoder, method, &status);
  upb_sink_reset(&sink, h, msg);
  upb_pbdecoder_resetoutput(&decoder, &sink);
  upb_bufsrc_putbuf(RSTRING_PTR(data), RSTRING_LEN(data),
                    upb_pbdecoder_input(&decoder));

  upb_pbdecoder_uninit(&decoder);
  if (!upb_ok(&status)) {
    rb_raise(rb_eRuntimeError, "Error occurred during parsing: %s.",
             upb_status_errmsg(&status));
  }

  return msg_rb;
}

/*
 * call-seq:
 *     MessageClass.decode_json(data) => message
 *
 * Decodes the given data (as a string containing bytes in protocol buffers wire
 * format) under the interpretration given by this message class's definition
 * and returns a message object with the corresponding field values.
 */
VALUE Message_decode_json(VALUE klass, VALUE data) {
  VALUE descriptor = rb_iv_get(klass, kDescriptorInstanceVar);
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  VALUE msgklass = Descriptor_msgclass(descriptor);

  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for JSON data.");
  }
  // TODO(cfallin): Check and respect string encoding. If not UTF-8, we need to
  // convert, because string handlers pass data directly to message string
  // fields.

  VALUE msg_rb = rb_class_new_instance(0, NULL, msgklass);
  MessageHeader* msg;
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);

  upb_status status = UPB_STATUS_INIT;
  upb_json_parser parser;
  upb_json_parser_init(&parser, &status);

  upb_sink sink;
  upb_sink_reset(&sink, get_fill_handlers(desc), msg);
  upb_json_parser_resetoutput(&parser, &sink);
  upb_bufsrc_putbuf(RSTRING_PTR(data), RSTRING_LEN(data),
                    upb_json_parser_input(&parser));

  upb_json_parser_uninit(&parser);
  if (!upb_ok(&status)) {
    rb_raise(rb_eRuntimeError, "Error occurred during parsing: %s.",
             upb_status_errmsg(&status));
  }

  return msg_rb;
}

// -----------------------------------------------------------------------------
// Serializing.
// -----------------------------------------------------------------------------
//
// The code below also comes from upb's prototype Ruby binding, developed by
// haberman@.

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
  sink->len = 0;
}

void stringsink_uninit(stringsink *sink) {
  free(sink->ptr);
}

/* msgvisitor *****************************************************************/

// TODO: If/when we support proto2 semantics in addition to the current proto3
// semantics, which means that we have true field presence, we will want to
// modify msgvisitor so that it emits all present fields rather than all
// non-default-value fields.
//
// Likewise, when implementing JSON serialization, we may need to have a
// 'verbose' mode that outputs all fields and a 'concise' mode that outputs only
// those with non-default values.

static void putmsg(VALUE msg, const Descriptor* desc,
                   upb_sink *sink, int depth);

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

  // Ensure that the string has the correct encoding. We also check at field-set
  // time, but the user may have mutated the string object since then.
  native_slot_validate_string_encoding(upb_fielddef_type(f), str);

  upb_sink_startstr(sink, getsel(f, UPB_HANDLER_STARTSTR), RSTRING_LEN(str),
                    &subsink);
  upb_sink_putstring(&subsink, getsel(f, UPB_HANDLER_STRING), RSTRING_PTR(str),
                     RSTRING_LEN(str), NULL);
  upb_sink_endstr(sink, getsel(f, UPB_HANDLER_ENDSTR));
}

static void putsubmsg(VALUE submsg, const upb_fielddef *f, upb_sink *sink,
                      int depth) {
  if (submsg == Qnil) return;

  upb_sink subsink;
  VALUE descriptor = rb_iv_get(submsg, kDescriptorInstanceVar);
  Descriptor* subdesc = ruby_to_Descriptor(descriptor);

  upb_sink_startsubmsg(sink, getsel(f, UPB_HANDLER_STARTSUBMSG), &subsink);
  putmsg(submsg, subdesc, &subsink, depth + 1);
  upb_sink_endsubmsg(sink, getsel(f, UPB_HANDLER_ENDSUBMSG));
}

static void putary(VALUE ary, const upb_fielddef *f, upb_sink *sink,
                   int depth) {
  if (ary == Qnil) return;

  upb_sink subsink;

  upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);

  upb_fieldtype_t type = upb_fielddef_type(f);
  upb_selector_t sel = 0;
  if (upb_fielddef_isprimitive(f)) {
    sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  }

  int size = NUM2INT(RepeatedField_length(ary));
  for (int i = 0; i < size; i++) {
    void* memory = RepeatedField_index_native(ary, i);
    switch (type) {
#define T(upbtypeconst, upbtype, ctype)                         \
  case upbtypeconst:                                            \
    upb_sink_put##upbtype(&subsink, sel, *((ctype *)memory));   \
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
        putstr(*((VALUE *)memory), f, &subsink);
        break;
      case UPB_TYPE_MESSAGE:
        putsubmsg(*((VALUE *)memory), f, &subsink, depth);
        break;

#undef T

    }
  }
  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static void putmsg(VALUE msg_rb, const Descriptor* desc,
                   upb_sink *sink, int depth) {
  upb_sink_startmsg(sink);

  // Protect against cycles (possible because users may freely reassign message
  // and repeated fields) by imposing a maximum recursion depth.
  if (depth > UPB_SINK_MAX_NESTING) {
    rb_raise(rb_eRuntimeError,
             "Maximum recursion depth exceeded during encoding.");
  }

  MessageHeader* msg;
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);
  void* msg_data = Message_data(msg);

  upb_msg_iter i;
  for (upb_msg_begin(&i, desc->msgdef);
       !upb_msg_done(&i);
       upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    uint32_t offset = desc->layout->offsets[upb_fielddef_index(f)];

    if (upb_fielddef_isseq(f)) {
      VALUE ary = DEREF(msg_data, offset, VALUE);
      if (ary != Qnil) {
        putary(ary, f, sink, depth);
      }
    } else if (upb_fielddef_isstring(f)) {
      VALUE str = DEREF(msg_data, offset, VALUE);
      if (RSTRING_LEN(str) > 0) {
        putstr(str, f, sink);
      }
    } else if (upb_fielddef_issubmsg(f)) {
      putsubmsg(DEREF(msg_data, offset, VALUE), f, sink, depth);
    } else {
      upb_selector_t sel = getsel(f, upb_handlers_getprimitivehandlertype(f));

#define T(upbtypeconst, upbtype, ctype, default_value)                \
  case upbtypeconst: {                                                \
      ctype value = DEREF(msg_data, offset, ctype);                   \
      if (value != default_value) {                                   \
        upb_sink_put##upbtype(sink, sel, value);                      \
      }                                                               \
    }                                                                 \
    break;

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

  upb_status status;
  upb_sink_endmsg(sink, &status);
}

static const upb_handlers* msgdef_pb_serialize_handlers(Descriptor* desc) {
  if (desc->pb_serialize_handlers == NULL) {
    desc->pb_serialize_handlers =
        upb_pb_encoder_newhandlers(desc->msgdef, &desc->pb_serialize_handlers);
  }
  return desc->pb_serialize_handlers;
}

static const upb_handlers* msgdef_json_serialize_handlers(Descriptor* desc) {
  if (desc->json_serialize_handlers == NULL) {
    desc->json_serialize_handlers =
        upb_json_printer_newhandlers(
            desc->msgdef, &desc->json_serialize_handlers);
  }
  return desc->json_serialize_handlers;
}

/*
 * call-seq:
 *     MessageClass.encode(msg) => bytes
 *
 * Encodes the given message object to its serialized form in protocol buffers
 * wire format.
 */
VALUE Message_encode(VALUE klass, VALUE msg_rb) {
  VALUE descriptor = rb_iv_get(klass, kDescriptorInstanceVar);
  Descriptor* desc = ruby_to_Descriptor(descriptor);

  stringsink sink;
  stringsink_init(&sink);

  const upb_handlers* serialize_handlers =
      msgdef_pb_serialize_handlers(desc);

  upb_pb_encoder encoder;
  upb_pb_encoder_init(&encoder, serialize_handlers);
  upb_pb_encoder_resetoutput(&encoder, &sink.sink);

  putmsg(msg_rb, desc, upb_pb_encoder_input(&encoder), 0);

  VALUE ret = rb_str_new(sink.ptr, sink.len);

  upb_pb_encoder_uninit(&encoder);
  stringsink_uninit(&sink);

  return ret;
}

/*
 * call-seq:
 *     MessageClass.encode_json(msg) => json_string
 *
 * Encodes the given message object into its serialized JSON representation.
 */
VALUE Message_encode_json(VALUE klass, VALUE msg_rb) {
  VALUE descriptor = rb_iv_get(klass, kDescriptorInstanceVar);
  Descriptor* desc = ruby_to_Descriptor(descriptor);

  stringsink sink;
  stringsink_init(&sink);

  const upb_handlers* serialize_handlers =
      msgdef_json_serialize_handlers(desc);

  upb_json_printer printer;
  upb_json_printer_init(&printer, serialize_handlers);
  upb_json_printer_resetoutput(&printer, &sink.sink);

  putmsg(msg_rb, desc, upb_json_printer_input(&printer), 0);

  VALUE ret = rb_str_new(sink.ptr, sink.len);

  upb_json_printer_uninit(&printer);
  stringsink_uninit(&sink);

  return ret;
}

/*
 * call-seq:
 *     Google::Protobuf.encode(msg) => bytes
 *
 * Encodes the given message object to protocol buffers wire format. This is an
 * alternative to the #encode method on msg's class.
 */
VALUE Google_Protobuf_encode(VALUE self, VALUE msg_rb) {
  VALUE klass = CLASS_OF(msg_rb);
  return Message_encode(klass, msg_rb);
}

/*
 * call-seq:
 *     Google::Protobuf.encode_json(msg) => json_string
 *
 * Encodes the given message object to its JSON representation. This is an
 * alternative to the #encode_json method on msg's class.
 */
VALUE Google_Protobuf_encode_json(VALUE self, VALUE msg_rb) {
  VALUE klass = CLASS_OF(msg_rb);
  return Message_encode_json(klass, msg_rb);
}

/*
 * call-seq:
 *     Google::Protobuf.decode(class, bytes) => msg
 *
 * Decodes the given bytes as protocol buffers wire format under the
 * interpretation given by the given class's message definition. This is an
 * alternative to the #decode method on the given class.
 */
VALUE Google_Protobuf_decode(VALUE self, VALUE klass, VALUE msg_rb) {
  return Message_decode(klass, msg_rb);
}

/*
 * call-seq:
 *     Google::Protobuf.decode_json(class, json_string) => msg
 *
 * Decodes the given JSON string under the interpretation given by the given
 * class's message definition. This is an alternative to the #decode_json method
 * on the given class.
 */
VALUE Google_Protobuf_decode_json(VALUE self, VALUE klass, VALUE msg_rb) {
  return Message_decode_json(klass, msg_rb);
}
