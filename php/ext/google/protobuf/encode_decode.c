// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
//
#include <php.h>
#include <Zend/zend_exceptions.h>

#include "protobuf.h"
#include "utf8.h"

/* stringsink *****************************************************************/

static void *stringsink_start(void *_sink, const void *hd, size_t size_hint) {
  stringsink *sink = _sink;
  sink->len = 0;
  return sink;
}

size_t stringsink_string(void *_sink, const void *hd, const char *ptr,
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

void stringsink_uninit(stringsink *sink) { free(sink->ptr); }

void stringsink_uninit_opaque(void *sink) { stringsink_uninit(sink); }

/* stackenv *****************************************************************/

// Stack-allocated context during an encode/decode operation. Contains the upb
// environment and its stack-based allocator, an initial buffer for allocations
// to avoid malloc() when possible, and a template for PHP exception messages
// if any error occurs.
#define STACK_ENV_STACKBYTES 4096
typedef struct {
  upb_arena *arena;
  upb_status status;
  const char *php_error_template;
  char allocbuf[STACK_ENV_STACKBYTES];
} stackenv;


static void stackenv_init(stackenv* se, const char* errmsg);
static void stackenv_uninit(stackenv* se);

static void stackenv_init(stackenv* se, const char* errmsg) {
  se->php_error_template = errmsg;
  se->arena = upb_arena_new();
  upb_status_clear(&se->status);
}

static void stackenv_uninit(stackenv* se) {
  upb_arena_free(se->arena);

  if (!upb_ok(&se->status)) {
    // TODO(teboring): have a way to verify that this is actually a parse error,
    // instead of just throwing "parse error" unconditionally.
    TSRMLS_FETCH();
    zend_throw_exception_ex(NULL, 0 TSRMLS_CC, se->php_error_template,
                            upb_status_errmsg(&se->status));
  }
}

// -----------------------------------------------------------------------------
// Parsing.
// -----------------------------------------------------------------------------

// TODO(teboring): This shoud be a bit in upb_msgdef
static bool is_wrapper_msg(const upb_msgdef *msg) {
  return !strcmp(upb_filedef_name(upb_msgdef_file(msg)),
                 "google/protobuf/wrappers.proto");
}

#define DEREF(msg, ofs, type) *(type*)(((uint8_t *)msg) + ofs)

// Creates a handlerdata that simply contains the offset for this field.
static const void* newhandlerdata(upb_handlers* h, uint32_t ofs) {
  size_t* hd_ofs = (size_t*)malloc(sizeof(size_t));
  *hd_ofs = ofs;
  upb_handlers_addcleanup(h, hd_ofs, free);
  return hd_ofs;
}

typedef struct {
  void* closure;
  stringsink sink;
} stringfields_parseframe_t;

typedef size_t (*encodeunknown_handlerfunc)(void* _sink, const void* hd,
                                            const char* ptr, size_t len,
                                            const upb_bufhandle* handle);

typedef struct {
  encodeunknown_handlerfunc handler;
} unknownfields_handlerdata_t;

// Creates a handlerdata for unknown fields.
static const void *newunknownfieldshandlerdata(upb_handlers* h) {
  unknownfields_handlerdata_t* hd =
      (unknownfields_handlerdata_t*)malloc(sizeof(unknownfields_handlerdata_t));
  hd->handler = stringsink_string;
  upb_handlers_addcleanup(h, hd, free);
  return hd;
}

typedef struct {
  size_t ofs;
  const upb_msgdef *md;
} submsg_handlerdata_t;

// Creates a handlerdata that contains offset and submessage type information.
static const void *newsubmsghandlerdata(upb_handlers* h, uint32_t ofs,
                                        const upb_fielddef* f) {
  submsg_handlerdata_t* hd =
      (submsg_handlerdata_t*)malloc(sizeof(submsg_handlerdata_t));
  hd->ofs = ofs;
  hd->md = upb_fielddef_msgsubdef(f);
  upb_handlers_addcleanup(h, hd, free);
  return hd;
}

typedef struct {
  size_t ofs;              // union data slot
  size_t case_ofs;         // oneof_case field
  int property_ofs;        // properties table cache
  uint32_t oneof_case_num; // oneof-case number to place in oneof_case field
  const upb_msgdef *md;    // msgdef, for oneof submessage handler
  const upb_msgdef *parent_md;  // msgdef, for parent submessage
} oneof_handlerdata_t;

static const void *newoneofhandlerdata(upb_handlers *h,
                                       uint32_t ofs,
                                       uint32_t case_ofs,
                                       int property_ofs,
                                       const upb_msgdef *m,
                                       const upb_fielddef *f) {
  oneof_handlerdata_t* hd =
      (oneof_handlerdata_t*)malloc(sizeof(oneof_handlerdata_t));
  hd->ofs = ofs;
  hd->case_ofs = case_ofs;
  hd->property_ofs = property_ofs;
  hd->parent_md = m;
  // We reuse the field tag number as a oneof union discriminant tag. Note that
  // we don't expose these numbers to the user, so the only requirement is that
  // we have some unique ID for each union case/possibility. The field tag
  // numbers are already present and are easy to use so there's no reason to
  // create a separate ID space. In addition, using the field tag number here
  // lets us easily look up the field in the oneof accessor.
  hd->oneof_case_num = upb_fielddef_number(f);
  if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE) {
    hd->md = upb_fielddef_msgsubdef(f);
  } else {
    hd->md = NULL;
  }
  upb_handlers_addcleanup(h, hd, free);
  return hd;
}

// A handler that starts a repeated field.  Gets the Repeated*Field instance for
// this field (such an instance always exists even in an empty message).
static void *startseq_handler(void* closure, const void* hd) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  return CACHED_PTR_TO_ZVAL_PTR(DEREF(message_data(msg), *ofs, CACHED_VALUE*));
}

// Handlers that append primitive values to a repeated field.
#define DEFINE_APPEND_HANDLER(type, ctype)                          \
  static bool append##type##_handler(void* closure, const void* hd, \
                                     ctype val) {                   \
    zval* array = (zval*)closure;                                   \
    TSRMLS_FETCH();                                                 \
    RepeatedField* intern = UNBOX(RepeatedField, array);            \
    repeated_field_push_native(intern, &val);                       \
    return true;                                                    \
  }

DEFINE_APPEND_HANDLER(bool,   bool)
DEFINE_APPEND_HANDLER(int32,  int32_t)
DEFINE_APPEND_HANDLER(uint32, uint32_t)
DEFINE_APPEND_HANDLER(float,  float)
DEFINE_APPEND_HANDLER(int64,  int64_t)
DEFINE_APPEND_HANDLER(uint64, uint64_t)
DEFINE_APPEND_HANDLER(double, double)

// Appends a string or 'bytes' string to a repeated field.
static void* appendstr_handler(void *closure,
                               const void *hd,
                               size_t size_hint) {
  UPB_UNUSED(hd);

  stringfields_parseframe_t* frame =
      (stringfields_parseframe_t*)malloc(sizeof(stringfields_parseframe_t));
  frame->closure = closure;
  stringsink_init(&frame->sink);

  return frame;
}

static bool appendstr_end_handler(void *closure, const void *hd) {
  stringfields_parseframe_t* frame = closure;

  zval* array = (zval*)frame->closure;
  TSRMLS_FETCH();
  RepeatedField* intern = UNBOX(RepeatedField, array);

#if PHP_MAJOR_VERSION < 7
  zval* str;
  MAKE_STD_ZVAL(str);
  PHP_PROTO_ZVAL_STRINGL(str, frame->sink.ptr, frame->sink.len, 1);
  repeated_field_push_native(intern, &str);
#else
  zend_string* str = zend_string_init(frame->sink.ptr, frame->sink.len, 1);
  repeated_field_push_native(intern, &str);
#endif

  stringsink_uninit(&frame->sink);
  free(frame);

  return true;
}

// Handlers that append primitive values to a repeated field.
#define DEFINE_SINGULAR_HANDLER(type, ctype)                \
  static bool type##_handler(void* closure, const void* hd, \
                                     ctype val) {           \
    MessageHeader* msg = (MessageHeader*)closure;           \
    const size_t *ofs = hd;                                 \
    DEREF(message_data(msg), *ofs, ctype) = val;            \
    return true;                                            \
  }

DEFINE_SINGULAR_HANDLER(bool,   bool)
DEFINE_SINGULAR_HANDLER(int32,  int32_t)
DEFINE_SINGULAR_HANDLER(uint32, uint32_t)
DEFINE_SINGULAR_HANDLER(float,  float)
DEFINE_SINGULAR_HANDLER(int64,  int64_t)
DEFINE_SINGULAR_HANDLER(uint64, uint64_t)
DEFINE_SINGULAR_HANDLER(double, double)

#undef DEFINE_SINGULAR_HANDLER

#if PHP_MAJOR_VERSION < 7
static void *empty_php_string(zval** value_ptr) {
  SEPARATE_ZVAL_IF_NOT_REF(value_ptr);
  if (Z_TYPE_PP(value_ptr) == IS_STRING &&
      !IS_INTERNED(Z_STRVAL_PP(value_ptr))) {
    FREE(Z_STRVAL_PP(value_ptr));
  }
  ZVAL_EMPTY_STRING(*value_ptr);
  return (void*)(*value_ptr);
}
#else
static void *empty_php_string(zval* value_ptr) {
  if (Z_TYPE_P(value_ptr) == IS_STRING) {
    zend_string_release(Z_STR_P(value_ptr));
  }
  ZVAL_EMPTY_STRING(value_ptr);
  return value_ptr;
}
#endif
#if PHP_MAJOR_VERSION < 7
static void *empty_php_string2(zval** value_ptr) {
  SEPARATE_ZVAL_IF_NOT_REF(value_ptr);
  if (Z_TYPE_PP(value_ptr) == IS_STRING &&
      !IS_INTERNED(Z_STRVAL_PP(value_ptr))) {
    FREE(Z_STRVAL_PP(value_ptr));
  }
  ZVAL_EMPTY_STRING(*value_ptr);
  return (void*)(*value_ptr);
}
static void new_php_string(zval** value_ptr, const char* str, size_t len) {
  SEPARATE_ZVAL_IF_NOT_REF(value_ptr);
  if (Z_TYPE_PP(value_ptr) == IS_STRING &&
      !IS_INTERNED(Z_STRVAL_PP(value_ptr))) {
    FREE(Z_STRVAL_PP(value_ptr));
  }
  ZVAL_EMPTY_STRING(*value_ptr);
  ZVAL_STRINGL(*value_ptr, str, len, 1);
}
#else
static void *empty_php_string2(zval* value_ptr) {
  if (Z_TYPE_P(value_ptr) == IS_STRING) {
    zend_string_release(Z_STR_P(value_ptr));
  }
  ZVAL_EMPTY_STRING(value_ptr);
  return value_ptr;
}
static void new_php_string(zval* value_ptr, const char* str, size_t len) {
  if (Z_TYPE_P(value_ptr) == IS_STRING) {
    zend_string_release(Z_STR_P(value_ptr));
  }
  ZVAL_NEW_STR(value_ptr, zend_string_init(str, len, 0));
}
#endif

// Sets a non-repeated string/bytes field in a message.
static void* str_handler(void *closure,
                         const void *hd,
                         size_t size_hint) {
  UPB_UNUSED(hd);

  stringfields_parseframe_t* frame =
      (stringfields_parseframe_t*)malloc(sizeof(stringfields_parseframe_t));
  frame->closure = closure;
  stringsink_init(&frame->sink);

  return frame;
}

static bool str_end_handler(void *closure, const void *hd) {
  stringfields_parseframe_t* frame = closure;
  const size_t *ofs = hd;
  MessageHeader* msg = (MessageHeader*)frame->closure;

  new_php_string(DEREF(message_data(msg), *ofs, CACHED_VALUE*),
                 frame->sink.ptr, frame->sink.len);

  stringsink_uninit(&frame->sink);
  free(frame);

  return true;
}

static size_t stringdata_handler(void* closure, const void* hd,
                                 const char* str, size_t len,
                                 const upb_bufhandle* handle) {
  stringfields_parseframe_t* frame = closure;
  return stringsink_string(&frame->sink, hd, str, len, handle);
}

// Appends a submessage to a repeated field.
static void *appendsubmsg_handler(void *closure, const void *hd) {
  zval* array = (zval*)closure;
  TSRMLS_FETCH();
  RepeatedField* intern = UNBOX(RepeatedField, array);

  const submsg_handlerdata_t *submsgdata = hd;
  Descriptor* subdesc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj((void*)submsgdata->md));
  zend_class_entry* subklass = subdesc->klass;
  MessageHeader* submsg;

#if PHP_MAJOR_VERSION < 7
  zval* val = NULL;
  MAKE_STD_ZVAL(val);
  ZVAL_OBJ(val, subklass->create_object(subklass TSRMLS_CC));
  repeated_field_push_native(intern, &val);
  submsg = UNBOX(MessageHeader, val);
#else
  zend_object* obj = subklass->create_object(subklass TSRMLS_CC);
  repeated_field_push_native(intern, &obj);
  submsg = (MessageHeader*)((char*)obj - XtOffsetOf(MessageHeader, std));
#endif
  custom_data_init(subklass, submsg PHP_PROTO_TSRMLS_CC);

  return submsg;
}

// Sets a non-repeated submessage field in a message.
static void *submsg_handler(void *closure, const void *hd) {
  MessageHeader* msg = closure;
  const submsg_handlerdata_t* submsgdata = hd;
  TSRMLS_FETCH();
  Descriptor* subdesc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj((void*)submsgdata->md));
  zend_class_entry* subklass = subdesc->klass;
  zval* submsg_php;
  MessageHeader* submsg;

  if (Z_TYPE_P(CACHED_PTR_TO_ZVAL_PTR(DEREF(message_data(msg), submsgdata->ofs,
                                            CACHED_VALUE*))) == IS_NULL) {
#if PHP_MAJOR_VERSION < 7
    zval val;
    ZVAL_OBJ(&val, subklass->create_object(subklass TSRMLS_CC));
    MessageHeader* intern = UNBOX(MessageHeader, &val);
    custom_data_init(subklass, intern PHP_PROTO_TSRMLS_CC);
    REPLACE_ZVAL_VALUE(DEREF(message_data(msg), submsgdata->ofs, zval**),
                       &val, 1);
    zval_dtor(&val);
#else
    zend_object* obj = subklass->create_object(subklass TSRMLS_CC);
    ZVAL_OBJ(DEREF(message_data(msg), submsgdata->ofs, zval*), obj);
    MessageHeader* intern = UNBOX_HASHTABLE_VALUE(MessageHeader, obj);
    custom_data_init(subklass, intern PHP_PROTO_TSRMLS_CC);
#endif
  }

  submsg_php = CACHED_PTR_TO_ZVAL_PTR(
      DEREF(message_data(msg), submsgdata->ofs, CACHED_VALUE*));

  submsg = UNBOX(MessageHeader, submsg_php);
  return submsg;
}

// Handler data for startmap/endmap handlers.
typedef struct {
  size_t ofs;
  upb_fieldtype_t key_field_type;
  upb_fieldtype_t value_field_type;
} map_handlerdata_t;

// Temporary frame for map parsing: at the beginning of a map entry message, a
// submsg handler allocates a frame to hold (i) a reference to the Map object
// into which this message will be inserted and (ii) storage slots to
// temporarily hold the key and value for this map entry until the end of the
// submessage. When the submessage ends, another handler is called to insert the
// value into the map.
typedef struct {
  char key_storage[NATIVE_SLOT_MAX_SIZE];
  char value_storage[NATIVE_SLOT_MAX_SIZE];
} map_parse_frame_data_t;

PHP_PROTO_WRAP_OBJECT_START(map_parse_frame_t)
  map_parse_frame_data_t* data;  // Place needs to be consistent with
                                 // MessageHeader.
  zval* map;
  // In php7, we cannot allocate zval dynamically. So we need to add zval here
  // to help decoding.
  zval key_zval;
  zval value_zval;
PHP_PROTO_WRAP_OBJECT_END
typedef struct map_parse_frame_t map_parse_frame_t;

static void map_slot_init(void* memory, upb_fieldtype_t type, zval* cache) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
#if PHP_MAJOR_VERSION < 7
      // Store zval** in memory in order to be consistent with the layout of
      // singular fields.
      zval** holder = ALLOC(zval*);
      *(zval***)memory = holder;
      zval* tmp;
      MAKE_STD_ZVAL(tmp);
      PHP_PROTO_ZVAL_STRINGL(tmp, "", 0, 1);
      *holder = tmp;
#else
      *(zval**)memory = cache;
      PHP_PROTO_ZVAL_STRINGL(*(zval**)memory, "", 0, 1);
#endif
      break;
    }
    case UPB_TYPE_MESSAGE: {
#if PHP_MAJOR_VERSION < 7
      zval** holder = ALLOC(zval*);
      zval* tmp;
      MAKE_STD_ZVAL(tmp);
      ZVAL_NULL(tmp);
      *holder = tmp;
      *(zval***)memory = holder;
#else
      *(zval**)memory = cache;
      ZVAL_NULL(*(zval**)memory);
#endif
      break;
    }
    default:
      native_slot_init(type, memory, NULL);
  }
}

static void map_slot_uninit(void* memory, upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_MESSAGE:
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
#if PHP_MAJOR_VERSION < 7
      zval** holder = *(zval***)memory;
      zval_ptr_dtor(holder);
      FREE(holder);
#else
      php_proto_zval_ptr_dtor(*(zval**)memory);
#endif
      break;
    }
    default:
      break;
  }
}

static void map_slot_key(upb_fieldtype_t type, const void* from,
                         const char** keyval,
                         size_t* length) {
  if (type == UPB_TYPE_STRING) {
#if PHP_MAJOR_VERSION < 7
    zval* key_php = **(zval***)from;
#else
    zval* key_php = *(zval**)from;
#endif
    *keyval = Z_STRVAL_P(key_php);
    *length = Z_STRLEN_P(key_php);
  } else {
    *keyval = from;
    *length = native_slot_size(type);
  }
}

static void map_slot_value(upb_fieldtype_t type, const void* from,
                           upb_value* v) {
  size_t len;
  void* to = upb_value_memory(v);
#ifndef NDEBUG
  v->ctype = UPB_CTYPE_UINT64;
#endif

  memset(to, 0, native_slot_size(type));

  switch (type) {
#if PHP_MAJOR_VERSION < 7
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE: {
      *(zval**)to = **(zval***)from;
      Z_ADDREF_PP((zval**)to);
      break;
    }
#else
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      *(zend_string**)to = Z_STR_P(*(zval**)from);
      zend_string_addref(*(zend_string**)to);
      break;
    case UPB_TYPE_MESSAGE:
      *(zend_object**)to = Z_OBJ_P(*(zval**)from);
      GC_ADDREF(*(zend_object**)to);
      break;
#endif
    default:
      len = native_slot_size(type);
      memcpy(to, from, len);
  }
}

// Handler to begin a map entry: allocates a temporary frame. This is the
// 'startsubmsg' handler on the msgdef that contains the map field.
static void *startmapentry_handler(void *closure, const void *hd) {
  MessageHeader* msg = closure;
  const map_handlerdata_t* mapdata = hd;
  zval* map = CACHED_PTR_TO_ZVAL_PTR(
      DEREF(message_data(msg), mapdata->ofs, CACHED_VALUE*));

  map_parse_frame_t* frame = ALLOC(map_parse_frame_t);
  frame->data = ALLOC(map_parse_frame_data_t);
  frame->map = map;

  map_slot_init(&frame->data->key_storage, mapdata->key_field_type,
                &frame->key_zval);
  map_slot_init(&frame->data->value_storage, mapdata->value_field_type,
                &frame->value_zval);

  return frame;
}

// Handler to end a map entry: inserts the value defined during the message into
// the map. This is the 'endmsg' handler on the map entry msgdef.
static bool endmap_handler(void* closure, const void* hd, upb_status* s) {
  map_parse_frame_t* frame = closure;
  const map_handlerdata_t* mapdata = hd;

  TSRMLS_FETCH();
  Map *map = UNBOX(Map, frame->map);

  const char* keyval = NULL;
  upb_value v;
  size_t length;

  map_slot_key(map->key_type, &frame->data->key_storage, &keyval, &length);
  map_slot_value(map->value_type, &frame->data->value_storage, &v);

  map_index_set(map, keyval, length, v);

  map_slot_uninit(&frame->data->key_storage, mapdata->key_field_type);
  map_slot_uninit(&frame->data->value_storage, mapdata->value_field_type);
  FREE(frame->data);
  FREE(frame);

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
    Descriptor* desc) {
  const upb_fielddef* key_field;
  const upb_fielddef* value_field;
  // TODO(teboring): Use emalloc and efree.
  map_handlerdata_t* hd =
      (map_handlerdata_t*)malloc(sizeof(map_handlerdata_t));

  hd->ofs = ofs;
  key_field = upb_msgdef_itof(mapentry_def, MAP_KEY_FIELD);
  assert(key_field != NULL);
  hd->key_field_type = upb_fielddef_type(key_field);
  value_field = upb_msgdef_itof(mapentry_def, MAP_VALUE_FIELD);
  assert(value_field != NULL);
  hd->value_field_type = upb_fielddef_type(value_field);

  return hd;
}

// Handlers that set primitive values in oneofs.
#define DEFINE_ONEOF_HANDLER(type, ctype)                          \
  static bool oneof##type##_handler(void* closure, const void* hd, \
                                    ctype val) {                   \
    const oneof_handlerdata_t* oneofdata = hd;                     \
    MessageHeader* msg = (MessageHeader*)closure;                  \
    DEREF(message_data(closure), oneofdata->case_ofs, uint32_t) =  \
        oneofdata->oneof_case_num;                                 \
    DEREF(message_data(closure), oneofdata->ofs, ctype) = val;     \
    return true;                                                   \
  }

DEFINE_ONEOF_HANDLER(bool,   bool)
DEFINE_ONEOF_HANDLER(int32,  int32_t)
DEFINE_ONEOF_HANDLER(uint32, uint32_t)
DEFINE_ONEOF_HANDLER(float,  float)
DEFINE_ONEOF_HANDLER(int64,  int64_t)
DEFINE_ONEOF_HANDLER(uint64, uint64_t)
DEFINE_ONEOF_HANDLER(double, double)

#undef DEFINE_ONEOF_HANDLER

static void oneof_cleanup(MessageHeader* msg,
                          const oneof_handlerdata_t* oneofdata) {
  uint32_t old_case_num =
      DEREF(message_data(msg), oneofdata->case_ofs, uint32_t);
  if (old_case_num == 0) {
    return;
  }

  const upb_fielddef* old_field =
      upb_msgdef_itof(oneofdata->parent_md, old_case_num);
  bool need_clean = false;

  switch (upb_fielddef_type(old_field)) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      need_clean = true;
      break;
    case UPB_TYPE_MESSAGE:
      if (oneofdata->oneof_case_num != old_case_num) {
        need_clean = true;
      }
      break;
    default:
      break;
  }

  if (need_clean) {
#if PHP_MAJOR_VERSION < 7
    SEPARATE_ZVAL_IF_NOT_REF(
        DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*));
    php_proto_zval_ptr_dtor(
        *DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*));
    MAKE_STD_ZVAL(*DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*));
    ZVAL_NULL(*DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*));
#endif
  }
}

// Handlers for string/bytes in a oneof.
static void *oneofbytes_handler(void *closure,
                                const void *hd,
                                size_t size_hint) {
  MessageHeader* msg = closure;
  const oneof_handlerdata_t *oneofdata = hd;

  oneof_cleanup(msg, oneofdata);

  DEREF(message_data(msg), oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;
  DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*) =
      OBJ_PROP(&msg->std, oneofdata->property_ofs);

   return empty_php_string(DEREF(
       message_data(msg), oneofdata->ofs, CACHED_VALUE*));
}
static bool oneofstr_end_handler(void *closure, const void *hd) {
  stringfields_parseframe_t* frame = closure;
  MessageHeader* msg = (MessageHeader*)frame->closure;
  const oneof_handlerdata_t *oneofdata = hd;

  oneof_cleanup(msg, oneofdata);

  DEREF(message_data(msg), oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;
  DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*) =
      OBJ_PROP(&msg->std, oneofdata->property_ofs);

  new_php_string(DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*),
                 frame->sink.ptr, frame->sink.len);

  stringsink_uninit(&frame->sink);
  free(frame);

  return true;
}

static void *oneofstr_handler(void *closure,
                              const void *hd,
                              size_t size_hint) {
  UPB_UNUSED(hd);

  stringfields_parseframe_t* frame =
      (stringfields_parseframe_t*)malloc(sizeof(stringfields_parseframe_t));
  frame->closure = closure;
  stringsink_init(&frame->sink);

  return frame;
}

// Handler for a submessage field in a oneof.
static void* oneofsubmsg_handler(void* closure, const void* hd) {
  MessageHeader* msg = closure;
  const oneof_handlerdata_t *oneofdata = hd;
  uint32_t oldcase = DEREF(message_data(msg), oneofdata->case_ofs, uint32_t);
  TSRMLS_FETCH();
  Descriptor* subdesc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj((void*)oneofdata->md));
  zend_class_entry* subklass = subdesc->klass;
  zval* submsg_php;
  MessageHeader* submsg;

  if (oldcase != oneofdata->oneof_case_num) {
    oneof_cleanup(msg, oneofdata);

    // Create new message.
    DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*) =
        OBJ_PROP(&msg->std, oneofdata->property_ofs);
#if PHP_MAJOR_VERSION < 7
    zval val;
    ZVAL_OBJ(&val, subklass->create_object(subklass TSRMLS_CC));
    REPLACE_ZVAL_VALUE(DEREF(message_data(msg), oneofdata->ofs, zval**),
                       &val, 1);
    zval_dtor(&val);
#else
    zend_object* obj = subklass->create_object(subklass TSRMLS_CC);
    ZVAL_OBJ(DEREF(message_data(msg), oneofdata->ofs, zval*), obj);
#endif
  }

  DEREF(message_data(msg), oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;

  submsg_php = CACHED_PTR_TO_ZVAL_PTR(
      DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*));
  submsg = UNBOX(MessageHeader, submsg_php);
  custom_data_init(subklass, submsg PHP_PROTO_TSRMLS_CC);
  return submsg;
}

// Set up handlers for a repeated field.
static void add_handlers_for_repeated_field(upb_handlers *h,
                                            const upb_fielddef *f,
                                            size_t offset) {
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;
  attr.handler_data = newhandlerdata(h, offset);
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
      upb_handlers_setstartstr(h, f, appendstr_handler, NULL);
      upb_handlers_setstring(h, f, stringdata_handler, NULL);
      upb_handlers_setendstr(h, f, appendstr_end_handler, &attr);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      upb_handlerattr attr = UPB_HANDLERATTR_INIT;
      attr.handler_data = newsubmsghandlerdata(h, 0, f);
      upb_handlers_setstartsubmsg(h, f, appendsubmsg_handler, &attr);
      break;
    }
  }
}

// Set up handlers for a singular field.
static void add_handlers_for_singular_field(upb_handlers *h,
                                            const upb_fielddef *f,
                                            size_t offset) {
  switch (upb_fielddef_type(f)) {
#define SET_HANDLER(utype, ltype)                          \
  case utype: {                                            \
    upb_handlerattr attr = UPB_HANDLERATTR_INIT;           \
    attr.handler_data = newhandlerdata(h, offset);         \
    upb_handlers_set##ltype(h, f, ltype##_handler, &attr); \
    break;                                                 \
  }

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
      upb_handlerattr attr = UPB_HANDLERATTR_INIT;
      attr.handler_data = newhandlerdata(h, offset);
      upb_handlers_setstartstr(h, f, str_handler, &attr);
      upb_handlers_setstring(h, f, stringdata_handler, &attr);
      upb_handlers_setendstr(h, f, str_end_handler, &attr);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      upb_handlerattr attr = UPB_HANDLERATTR_INIT;
      attr.handler_data = newsubmsghandlerdata(h, offset, f);
      upb_handlers_setstartsubmsg(h, f, submsg_handler, &attr);
      break;
    }
  }
}

// Adds handlers to a map field.
static void add_handlers_for_mapfield(upb_handlers* h,
                                      const upb_fielddef* fielddef,
                                      size_t offset,
                                      Descriptor* desc) {
  const upb_msgdef* map_msgdef = upb_fielddef_msgsubdef(fielddef);
  map_handlerdata_t* hd = new_map_handlerdata(offset, map_msgdef, desc);
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;

  upb_handlers_addcleanup(h, hd, free);
  attr.handler_data = hd;
  upb_handlers_setstartsubmsg(h, fielddef, startmapentry_handler, &attr);
}

// Adds handlers to a map-entry msgdef.
static void add_handlers_for_mapentry(const upb_msgdef* msgdef, upb_handlers* h,
                                      Descriptor* desc) {
  const upb_fielddef* key_field = map_entry_key(msgdef);
  const upb_fielddef* value_field = map_entry_value(msgdef);
  map_handlerdata_t* hd = new_map_handlerdata(0, msgdef, desc);
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;

  upb_handlers_addcleanup(h, hd, free);
  attr.handler_data = hd;
  upb_handlers_setendmsg(h, endmap_handler, &attr);

  add_handlers_for_singular_field(h, key_field,
                                  offsetof(map_parse_frame_data_t,
                                           key_storage));
  add_handlers_for_singular_field(h, value_field,
                                  offsetof(map_parse_frame_data_t,
                                           value_storage));
}

// Set up handlers for a oneof field.
static void add_handlers_for_oneof_field(upb_handlers *h,
                                         const upb_msgdef *m,
                                         const upb_fielddef *f,
                                         size_t offset,
                                         size_t oneof_case_offset,
                                         int property_cache_offset) {

  upb_handlerattr attr = UPB_HANDLERATTR_INIT;
  attr.handler_data = newoneofhandlerdata(h, offset, oneof_case_offset,
                                          property_cache_offset, m, f);

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
      upb_handlers_setstartstr(h, f, oneofstr_handler, &attr);
      upb_handlers_setstring(h, f, stringdata_handler, NULL);
      upb_handlers_setendstr(h, f, oneofstr_end_handler, &attr);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      upb_handlers_setstartsubmsg(h, f, oneofsubmsg_handler, &attr);
      break;
    }
  }
}

static bool add_unknown_handler(void* closure, const void* hd, const char* buf,
                         size_t size) {
  encodeunknown_handlerfunc handler =
      ((unknownfields_handlerdata_t*)hd)->handler;

  MessageHeader* msg = (MessageHeader*)closure;
  stringsink* unknown = DEREF(message_data(msg), 0, stringsink*);
  if (unknown == NULL) {
    DEREF(message_data(msg), 0, stringsink*) = ALLOC(stringsink);
    unknown = DEREF(message_data(msg), 0, stringsink*);
    stringsink_init(unknown);
  }

  handler(unknown, NULL, buf, size, NULL);

  return true;
}

void add_handlers_for_message(const void* closure, upb_handlers* h) {
  const upb_msgdef* msgdef = upb_handlers_msgdef(h);
  TSRMLS_FETCH();
  Descriptor* desc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj((void*)msgdef));
  upb_msg_field_iter i;

  // If this is a mapentry message type, set up a special set of handlers and
  // bail out of the normal (user-defined) message type handling.
  if (upb_msgdef_mapentry(msgdef)) {
    add_handlers_for_mapentry(msgdef, h, desc);
    return;
  }

  // Ensure layout exists. We may be invoked to create handlers for a given
  // message if we are included as a submsg of another message type before our
  // class is actually built, so to work around this, we just create the layout
  // (and handlers, in the class-building function) on-demand.
  if (desc->layout == NULL) {
    desc->layout = create_layout(desc->msgdef);
  }

  upb_handlerattr attr = UPB_HANDLERATTR_INIT;
  attr.handler_data = newunknownfieldshandlerdata(h);
  upb_handlers_setunknown(h, add_unknown_handler, &attr);

  for (upb_msg_field_begin(&i, desc->msgdef);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    size_t offset = desc->layout->fields[upb_fielddef_index(f)].offset;

    if (upb_fielddef_containingoneof(f)) {
      size_t oneof_case_offset =
          desc->layout->fields[upb_fielddef_index(f)].case_offset;
      int property_cache_index =
          desc->layout->fields[upb_fielddef_index(f)].cache_index;
      add_handlers_for_oneof_field(h, desc->msgdef, f, offset,
                                   oneof_case_offset, property_cache_index);
    } else if (is_map_field(f)) {
      add_handlers_for_mapfield(h, f, offset, desc);
    } else if (upb_fielddef_isseq(f)) {
      add_handlers_for_repeated_field(h, f, offset);
    } else {
      add_handlers_for_singular_field(h, f, offset);
    }
  }
}

// Constructs the handlers for filling a message's data into an in-memory
// object.
const upb_handlers* get_fill_handlers(Descriptor* desc) {
  return upb_handlercache_get(desc->pool->fill_handler_cache, desc->msgdef);
}

static const upb_pbdecodermethod *msgdef_decodermethod(Descriptor* desc) {
  return upb_pbcodecache_get(desc->pool->fill_method_cache, desc->msgdef);
}

static const upb_json_parsermethod *msgdef_jsonparsermethod(Descriptor* desc) {
  return upb_json_codecache_get(desc->pool->json_fill_method_cache, desc->msgdef);
}

// -----------------------------------------------------------------------------
// Serializing.
// -----------------------------------------------------------------------------

static void putmsg(zval* msg, const Descriptor* desc, upb_sink sink,
                   int depth, bool is_json TSRMLS_DC);
static void putrawmsg(MessageHeader* msg, const Descriptor* desc,
                      upb_sink sink, int depth, bool is_json,
                      bool open_msg TSRMLS_DC);
static void putjsonany(MessageHeader* msg, const Descriptor* desc,
                       upb_sink sink, int depth TSRMLS_DC);
static void putjsonlistvalue(
    MessageHeader* msg, const Descriptor* desc,
    upb_sink sink, int depth TSRMLS_DC);
static void putjsonstruct(
    MessageHeader* msg, const Descriptor* desc,
    upb_sink sink, int depth TSRMLS_DC);

static void putstr(zval* str, const upb_fielddef* f, upb_sink sink,
                   bool force_default);

static void putrawstr(const char* str, int len, const upb_fielddef* f,
                      upb_sink sink, bool force_default);

static void putsubmsg(zval* submsg, const upb_fielddef* f, upb_sink sink,
                      int depth, bool is_json TSRMLS_DC);
static void putrawsubmsg(MessageHeader* submsg, const upb_fielddef* f,
                         upb_sink sink, int depth, bool is_json TSRMLS_DC);

static void putarray(zval* array, const upb_fielddef* f, upb_sink sink,
                     int depth, bool is_json TSRMLS_DC);
static void putmap(zval* map, const upb_fielddef* f, upb_sink sink,
                   int depth, bool is_json TSRMLS_DC);

static upb_selector_t getsel(const upb_fielddef* f, upb_handlertype_t type) {
  upb_selector_t ret;
  bool ok = upb_handlers_getselector(f, type, &ret);
  UPB_ASSERT(ok);
  return ret;
}

static void put_optional_value(const void* memory, int len,
                               const upb_fielddef* f,
                               int depth, upb_sink sink,
                               bool is_json TSRMLS_DC) {
  assert(upb_fielddef_label(f) == UPB_LABEL_OPTIONAL);

  switch (upb_fielddef_type(f)) {
#define T(upbtypeconst, upbtype, ctype, default_value)                         \
  case upbtypeconst: {                                                         \
    ctype value = DEREF(memory, 0, ctype);                                     \
    if (is_json || value != default_value) {                                   \
      upb_selector_t sel = getsel(f, upb_handlers_getprimitivehandlertype(f)); \
      upb_sink_put##upbtype(sink, sel, value);                                 \
    }                                                                          \
  } break;

    T(UPB_TYPE_FLOAT, float, float, 0.0)
    T(UPB_TYPE_DOUBLE, double, double, 0.0)
    T(UPB_TYPE_BOOL, bool, uint8_t, 0)
    T(UPB_TYPE_ENUM, int32, int32_t, 0)
    T(UPB_TYPE_INT32, int32, int32_t, 0)
    T(UPB_TYPE_UINT32, uint32, uint32_t, 0)
    T(UPB_TYPE_INT64, int64, int64_t, 0)
    T(UPB_TYPE_UINT64, uint64, uint64_t, 0)

#undef T
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      putrawstr(memory, len, f, sink, is_json);
      break;
    case UPB_TYPE_MESSAGE: {
#if PHP_MAJOR_VERSION < 7
      MessageHeader *submsg = UNBOX(MessageHeader, *(zval**)memory);
#else
      MessageHeader *submsg =
          (MessageHeader*)((char*)(*(zend_object**)memory) -
              XtOffsetOf(MessageHeader, std));
#endif
      putrawsubmsg(submsg, f, sink, depth, is_json TSRMLS_CC);
      break;
    }
    default:
      assert(false);
  }
}

// Only string/bytes fields are stored as zval.
static const char* raw_value(void* memory, const upb_fielddef* f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
#if PHP_MAJOR_VERSION < 7
      return Z_STRVAL_PP((zval**)memory);
#else
      return ZSTR_VAL(*(zend_string**)memory);
#endif
      break;
    default:
      return memory;
  }
}

static int raw_value_len(void* memory, int len, const upb_fielddef* f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
#if PHP_MAJOR_VERSION < 7
      return Z_STRLEN_PP((zval**)memory);
#else
      return ZSTR_LEN(*(zend_string**)memory);
#endif
    default:
      return len;
  }
}

static void putmap(zval* map, const upb_fielddef* f, upb_sink sink,
                   int depth, bool is_json TSRMLS_DC) {
  upb_sink subsink;
  const upb_fielddef* key_field;
  const upb_fielddef* value_field;
  MapIter it;
  int len, size;

  assert(map != NULL);
  Map* intern = UNBOX(Map, map);
  size = upb_strtable_count(&intern->table);
  if (size == 0) return;

  upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);

  assert(upb_fielddef_type(f) == UPB_TYPE_MESSAGE);
  key_field = map_field_key(f);
  value_field = map_field_value(f);

  for (map_begin(map, &it TSRMLS_CC); !map_done(&it); map_next(&it)) {
    upb_status status;

    upb_sink entry_sink;
    upb_sink_startsubmsg(subsink, getsel(f, UPB_HANDLER_STARTSUBMSG),
                         &entry_sink);
    upb_sink_startmsg(entry_sink);

    // Serialize key.
    const char *key = map_iter_key(&it, &len);
    put_optional_value(key, len, key_field, depth + 1,
                       entry_sink, is_json TSRMLS_CC);

    // Serialize value.
    upb_value value = map_iter_value(&it, &len);
    put_optional_value(raw_value(upb_value_memory(&value), value_field),
                       raw_value_len(upb_value_memory(&value), len, value_field),
                       value_field, depth + 1, entry_sink, is_json TSRMLS_CC);

    upb_sink_endmsg(entry_sink, &status);
    upb_sink_endsubmsg(subsink, getsel(f, UPB_HANDLER_ENDSUBMSG));
  }

  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static void putmsg(zval* msg_php, const Descriptor* desc, upb_sink sink,
                   int depth, bool is_json TSRMLS_DC) {
  MessageHeader* msg = UNBOX(MessageHeader, msg_php);
  putrawmsg(msg, desc, sink, depth, is_json, true TSRMLS_CC);
}

static const upb_handlers* msgdef_json_serialize_handlers(
    Descriptor* desc, bool preserve_proto_fieldnames);

static void putjsonany(MessageHeader* msg, const Descriptor* desc,
                       upb_sink sink, int depth TSRMLS_DC) {
  upb_status status;
  const upb_fielddef* type_field = upb_msgdef_itof(desc->msgdef, UPB_ANY_TYPE);
  const upb_fielddef* value_field = upb_msgdef_itof(desc->msgdef, UPB_ANY_VALUE);

  uint32_t type_url_offset;
  zval* type_url_php_str;
  const upb_msgdef *payload_type = NULL;

  upb_sink_startmsg(sink);

  /* Handle type url */
  type_url_offset = desc->layout->fields[upb_fielddef_index(type_field)].offset;
  type_url_php_str = CACHED_PTR_TO_ZVAL_PTR(
      DEREF(message_data(msg), type_url_offset, CACHED_VALUE*));
  if (Z_STRLEN_P(type_url_php_str) > 0) {
    putstr(type_url_php_str, type_field, sink, false);
  }

  {
    const char* type_url_str = Z_STRVAL_P(type_url_php_str);
    size_t type_url_len = Z_STRLEN_P(type_url_php_str);
    if (type_url_len <= 20 ||
        strncmp(type_url_str, "type.googleapis.com/", 20) != 0) {
      zend_error(E_ERROR, "Invalid type url: %s", type_url_str);
    }

    /* Resolve type url */
    type_url_str += 20;
    type_url_len -= 20;

    payload_type = upb_symtab_lookupmsg2(
        generated_pool->symtab, type_url_str, type_url_len);
    if (payload_type == NULL) {
      zend_error(E_ERROR, "Unknown type: %s", type_url_str);
      return;
    }
  }

  {
    uint32_t value_offset;
    zval* value_php_str;
    const char* value_str;
    size_t value_len;

    value_offset = desc->layout->fields[upb_fielddef_index(value_field)].offset;
    value_php_str = CACHED_PTR_TO_ZVAL_PTR(
        DEREF(message_data(msg), value_offset, CACHED_VALUE*));
    value_str = Z_STRVAL_P(value_php_str);
    value_len = Z_STRLEN_P(value_php_str);

    if (value_len > 0) {
      Descriptor* payload_desc =
          UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj((void*)payload_type));
      zend_class_entry* payload_klass = payload_desc->klass;
      zval val;
      upb_sink subsink;
      bool is_wellknown;

      /* Create message of the payload type. */
      ZVAL_OBJ(&val, payload_klass->create_object(payload_klass TSRMLS_CC));
      MessageHeader* intern = UNBOX(MessageHeader, &val);
      custom_data_init(payload_klass, intern PHP_PROTO_TSRMLS_CC);

      merge_from_string(value_str, value_len, payload_desc, intern);

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
      putrawmsg(intern, payload_desc, subsink, depth, true,
                is_wellknown TSRMLS_CC);

      zval_dtor(&val);
    }
  }

  upb_sink_endmsg(sink, &status);
}

static void putjsonlistvalue(
    MessageHeader* msg, const Descriptor* desc,
    upb_sink sink, int depth TSRMLS_DC) {
  upb_status status;
  upb_sink subsink;
  const upb_fielddef* f = upb_msgdef_itof(desc->msgdef, 1);
  uint32_t offset = desc->layout->fields[upb_fielddef_index(f)].offset;
  zval* array;
  RepeatedField* intern;
  HashTable *ht;
  int size, i;

  upb_sink_startmsg(sink);

  array = CACHED_PTR_TO_ZVAL_PTR(
      DEREF(message_data(msg), offset, CACHED_VALUE*));
  intern = UNBOX(RepeatedField, array);
  ht = PHP_PROTO_HASH_OF(intern->array);
  size = zend_hash_num_elements(ht);

  if (size == 0) {
    upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);
    upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
  } else {
    putarray(array, f, sink, depth, true TSRMLS_CC);
  }

  upb_sink_endmsg(sink, &status);
}

static void putjsonstruct(
    MessageHeader* msg, const Descriptor* desc,
    upb_sink sink, int depth TSRMLS_DC) {
  upb_status status;
  upb_sink subsink;
  const upb_fielddef* f = upb_msgdef_itof(desc->msgdef, 1);
  uint32_t offset = desc->layout->fields[upb_fielddef_index(f)].offset;
  zval* map;
  Map* intern;
  int size;

  upb_sink_startmsg(sink);

  map = CACHED_PTR_TO_ZVAL_PTR(
      DEREF(message_data(msg), offset, CACHED_VALUE*));
  intern = UNBOX(Map, map);
  size = upb_strtable_count(&intern->table);

  if (size == 0) {
    upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);
    upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
  } else {
    putmap(map, f, sink, depth, true TSRMLS_CC);
  }

  upb_sink_endmsg(sink, &status);
}

static void putrawmsg(MessageHeader* msg, const Descriptor* desc,
                      upb_sink sink, int depth, bool is_json,
                      bool open_msg TSRMLS_DC) {
  upb_msg_field_iter i;
  upb_status status;

  if (is_json &&
      upb_msgdef_wellknowntype(desc->msgdef) == UPB_WELLKNOWN_ANY) {
    putjsonany(msg, desc, sink, depth TSRMLS_CC);
    return;
  }

  if (is_json &&
      upb_msgdef_wellknowntype(desc->msgdef) == UPB_WELLKNOWN_LISTVALUE) {
    putjsonlistvalue(msg, desc, sink, depth TSRMLS_CC);
    return;
  }

  if (is_json &&
      upb_msgdef_wellknowntype(desc->msgdef) == UPB_WELLKNOWN_STRUCT) {
    putjsonstruct(msg, desc, sink, depth TSRMLS_CC);
    return;
  }

  if (open_msg) {
    upb_sink_startmsg(sink);
  }

  // Protect against cycles (possible because users may freely reassign message
  // and repeated fields) by imposing a maximum recursion depth.
  if (depth > ENCODE_MAX_NESTING) {
    zend_error(E_ERROR,
             "Maximum recursion depth exceeded during encoding.");
  }

  for (upb_msg_field_begin(&i, desc->msgdef); !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef* f = upb_msg_iter_field(&i);
    uint32_t offset = desc->layout->fields[upb_fielddef_index(f)].offset;
    bool containing_oneof = false;

    if (upb_fielddef_containingoneof(f)) {
      uint32_t oneof_case_offset =
          desc->layout->fields[upb_fielddef_index(f)].case_offset;
      // For a oneof, check that this field is actually present -- skip all the
      // below if not.
      if (DEREF(message_data(msg), oneof_case_offset, uint32_t) !=
          upb_fielddef_number(f)) {
        continue;
      }
      // Otherwise, fall through to the appropriate singular-field handler
      // below.
      containing_oneof = true;
    }

    if (is_map_field(f)) {
      zval* map = CACHED_PTR_TO_ZVAL_PTR(
          DEREF(message_data(msg), offset, CACHED_VALUE*));
      if (map != NULL) {
        putmap(map, f, sink, depth, is_json TSRMLS_CC);
      }
    } else if (upb_fielddef_isseq(f)) {
      zval* array = CACHED_PTR_TO_ZVAL_PTR(
          DEREF(message_data(msg), offset, CACHED_VALUE*));
      if (array != NULL) {
        putarray(array, f, sink, depth, is_json TSRMLS_CC);
      }
    } else if (upb_fielddef_isstring(f)) {
      zval* str = CACHED_PTR_TO_ZVAL_PTR(
          DEREF(message_data(msg), offset, CACHED_VALUE*));
      if (containing_oneof || (is_json && is_wrapper_msg(desc->msgdef)) ||
          Z_STRLEN_P(str) > 0) {
        putstr(str, f, sink, is_json && is_wrapper_msg(desc->msgdef));
      }
    } else if (upb_fielddef_issubmsg(f)) {
      putsubmsg(CACHED_PTR_TO_ZVAL_PTR(
                    DEREF(message_data(msg), offset, CACHED_VALUE*)),
                f, sink, depth, is_json TSRMLS_CC);
    } else {
      upb_selector_t sel = getsel(f, upb_handlers_getprimitivehandlertype(f));

#define T(upbtypeconst, upbtype, ctype, default_value)     \
  case upbtypeconst: {                                     \
    ctype value = DEREF(message_data(msg), offset, ctype); \
    if (containing_oneof ||                                \
        (is_json && is_wrapper_msg(desc->msgdef)) ||       \
        value != default_value) {                          \
      upb_sink_put##upbtype(sink, sel, value);             \
    }                                                      \
  } break;

      switch (upb_fielddef_type(f)) {
        T(UPB_TYPE_FLOAT, float, float, 0.0)
        T(UPB_TYPE_DOUBLE, double, double, 0.0)
        T(UPB_TYPE_BOOL, bool, uint8_t, 0)
        case UPB_TYPE_ENUM:
        T(UPB_TYPE_INT32, int32, int32_t, 0)
        T(UPB_TYPE_UINT32, uint32, uint32_t, 0)
        T(UPB_TYPE_INT64, int64, int64_t, 0)
        T(UPB_TYPE_UINT64, uint64, uint64_t, 0)

        case UPB_TYPE_STRING:
        case UPB_TYPE_BYTES:
        case UPB_TYPE_MESSAGE:
          zend_error(E_ERROR, "Internal error.");
      }

#undef T
    }
  }

  stringsink* unknown = DEREF(message_data(msg), 0, stringsink*);
  if (unknown != NULL) {
    upb_sink_putunknown(sink, unknown->ptr, unknown->len);
  }

  if (open_msg) {
    upb_sink_endmsg(sink, &status);
  }
}

static void putstr(zval* str, const upb_fielddef *f,
                   upb_sink sink, bool force_default) {
  upb_sink subsink;

  if (ZVAL_IS_NULL(str)) return;

  assert(Z_TYPE_P(str) == IS_STRING);

  upb_sink_startstr(sink, getsel(f, UPB_HANDLER_STARTSTR), Z_STRLEN_P(str),
                    &subsink);

  // For oneof string field, we may get here with string length is zero.
  if (Z_STRLEN_P(str) > 0 || force_default) {
    // Ensure that the string has the correct encoding. We also check at
    // field-set time, but the user may have mutated the string object since
    // then.
    if (upb_fielddef_type(f) == UPB_TYPE_STRING &&
        !is_structurally_valid_utf8(Z_STRVAL_P(str), Z_STRLEN_P(str))) {
      zend_error(E_USER_ERROR, "Given string is not UTF8 encoded.");
      return;
    }
    upb_sink_putstring(subsink, getsel(f, UPB_HANDLER_STRING), Z_STRVAL_P(str),
                       Z_STRLEN_P(str), NULL);
  }

  upb_sink_endstr(sink, getsel(f, UPB_HANDLER_ENDSTR));
}

static void putrawstr(const char* str, int len, const upb_fielddef* f,
                      upb_sink sink, bool force_default) {
  upb_sink subsink;

  if (len == 0 && !force_default) return;

  // Ensure that the string has the correct encoding. We also check at field-set
  // time, but the user may have mutated the string object since then.
  if (upb_fielddef_type(f) == UPB_TYPE_STRING &&
      !is_structurally_valid_utf8(str, len)) {
    zend_error(E_USER_ERROR, "Given string is not UTF8 encoded.");
    return;
  }

  upb_sink_startstr(sink, getsel(f, UPB_HANDLER_STARTSTR), len, &subsink);
  upb_sink_putstring(subsink, getsel(f, UPB_HANDLER_STRING), str, len, NULL);
  upb_sink_endstr(sink, getsel(f, UPB_HANDLER_ENDSTR));
}

static void putsubmsg(zval* submsg_php, const upb_fielddef* f, upb_sink sink,
                      int depth, bool is_json TSRMLS_DC) {
  if (Z_TYPE_P(submsg_php) == IS_NULL) return;

  MessageHeader *submsg = UNBOX(MessageHeader, submsg_php);
  putrawsubmsg(submsg, f, sink, depth, is_json TSRMLS_CC);
}

static void putrawsubmsg(MessageHeader* submsg, const upb_fielddef* f,
                         upb_sink sink, int depth, bool is_json TSRMLS_DC) {
  upb_sink subsink;

  Descriptor* subdesc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(upb_fielddef_msgsubdef(f)));

  upb_sink_startsubmsg(sink, getsel(f, UPB_HANDLER_STARTSUBMSG), &subsink);
  putrawmsg(submsg, subdesc, subsink, depth + 1, is_json, true TSRMLS_CC);
  upb_sink_endsubmsg(sink, getsel(f, UPB_HANDLER_ENDSUBMSG));
}

static void putarray(zval* array, const upb_fielddef* f, upb_sink sink,
                     int depth, bool is_json TSRMLS_DC) {
  upb_sink subsink;
  upb_fieldtype_t type = upb_fielddef_type(f);
  upb_selector_t sel = 0;
  int size, i;

  assert(array != NULL);
  RepeatedField* intern = UNBOX(RepeatedField, array);
  HashTable *ht = PHP_PROTO_HASH_OF(intern->array);
  size = zend_hash_num_elements(ht);
  if (size == 0) return;

  upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);

  if (upb_fielddef_isprimitive(f)) {
    sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  }

  for (i = 0; i < size; i++) {
    void* memory = repeated_field_index_native(intern, i TSRMLS_CC);
    switch (type) {
#define T(upbtypeconst, upbtype, ctype)                      \
  case upbtypeconst:                                         \
    upb_sink_put##upbtype(subsink, sel, *((ctype*)memory)); \
    break;

      T(UPB_TYPE_FLOAT, float, float)
      T(UPB_TYPE_DOUBLE, double, double)
      T(UPB_TYPE_BOOL, bool, int8_t)
      case UPB_TYPE_ENUM:
        T(UPB_TYPE_INT32, int32, int32_t)
        T(UPB_TYPE_UINT32, uint32, uint32_t)
        T(UPB_TYPE_INT64, int64, int64_t)
        T(UPB_TYPE_UINT64, uint64, uint64_t)

      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES: {
#if PHP_MAJOR_VERSION < 7
        const char* rawstr = Z_STRVAL_P(*(zval**)memory);
        int len = Z_STRLEN_P(*(zval**)memory);
#else
        const char* rawstr = ZSTR_VAL(*(zend_string**)memory);
        int len = ZSTR_LEN(*(zend_string**)memory);
#endif
        putrawstr(rawstr, len, f, subsink,
                  is_json && is_wrapper_msg(upb_fielddef_containingtype(f)));
        break;
      }
      case UPB_TYPE_MESSAGE: {
#if PHP_MAJOR_VERSION < 7
        MessageHeader *submsg = UNBOX(MessageHeader, *(zval**)memory);
#else
        MessageHeader *submsg =
            (MessageHeader*)((char*)(Z_OBJ_P((zval*)memory)) -
                XtOffsetOf(MessageHeader, std));
#endif
        putrawsubmsg(submsg, f, subsink, depth, is_json TSRMLS_CC);
        break;
      }

#undef T
    }
  }
  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static const upb_handlers* msgdef_pb_serialize_handlers(Descriptor* desc) {
  return upb_handlercache_get(desc->pool->pb_serialize_handler_cache,
                              desc->msgdef);
}

static const upb_handlers* msgdef_json_serialize_handlers(
    Descriptor* desc, bool preserve_proto_fieldnames) {
  if (preserve_proto_fieldnames) {
    return upb_handlercache_get(
        desc->pool->json_serialize_handler_preserve_cache, desc->msgdef);
  } else {
    return upb_handlercache_get(desc->pool->json_serialize_handler_cache,
                                desc->msgdef);
  }
}

// -----------------------------------------------------------------------------
// PHP encode/decode methods
// -----------------------------------------------------------------------------

void serialize_to_string(zval* val, zval* return_value TSRMLS_DC) {
  Descriptor* desc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_ce_obj(Z_OBJCE_P(val)));

  stringsink sink;
  stringsink_init(&sink);

  {
    const upb_handlers* serialize_handlers = msgdef_pb_serialize_handlers(desc);

    stackenv se;
    upb_pb_encoder* encoder;

    stackenv_init(&se, "Error occurred during encoding: %s");
    encoder = upb_pb_encoder_create(se.arena, serialize_handlers, sink.sink);

    putmsg(val, desc, upb_pb_encoder_input(encoder), 0, false TSRMLS_CC);

    PHP_PROTO_RETVAL_STRINGL(sink.ptr, sink.len, 1);

    stackenv_uninit(&se);
    stringsink_uninit(&sink);
  }
}

PHP_METHOD(Message, serializeToString) {
  serialize_to_string(getThis(), return_value TSRMLS_CC);
}

void merge_from_string(const char* data, int data_len, Descriptor* desc,
                       MessageHeader* msg) {
  const upb_pbdecodermethod* method = msgdef_decodermethod(desc);
  const upb_handlers* h = upb_pbdecodermethod_desthandlers(method);
  stackenv se;
  upb_sink sink;
  upb_pbdecoder* decoder;
  stackenv_init(&se, "Error occurred during parsing: %s");

  upb_sink_reset(&sink, h, msg);
  decoder = upb_pbdecoder_create(se.arena, method, sink, &se.status);
  upb_bufsrc_putbuf(data, data_len, upb_pbdecoder_input(decoder));

  stackenv_uninit(&se);
}

PHP_METHOD(Message, mergeFromString) {
  Descriptor* desc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_ce_obj(Z_OBJCE_P(getThis())));
  MessageHeader* msg = UNBOX(MessageHeader, getThis());

  char *data = NULL;
  PHP_PROTO_SIZE data_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) ==
      FAILURE) {
    return;
  }

  merge_from_string(data, data_len, desc, msg);
}

PHP_METHOD(Message, serializeToJsonString) {
  Descriptor* desc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_ce_obj(Z_OBJCE_P(getThis())));

  zend_bool preserve_proto_fieldnames = false;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b",
                            &preserve_proto_fieldnames) == FAILURE) {
    return;
  }

  stringsink sink;
  stringsink_init(&sink);

  {
    const upb_handlers* serialize_handlers =
        msgdef_json_serialize_handlers(desc, preserve_proto_fieldnames);
    upb_json_printer* printer;
    stackenv se;

    stackenv_init(&se, "Error occurred during encoding: %s");
    printer = upb_json_printer_create(se.arena, serialize_handlers, sink.sink);

    putmsg(getThis(), desc, upb_json_printer_input(printer), 0, true TSRMLS_CC);

    PHP_PROTO_RETVAL_STRINGL(sink.ptr, sink.len, 1);

    stackenv_uninit(&se);
    stringsink_uninit(&sink);
  }
}

PHP_METHOD(Message, mergeFromJsonString) {
  Descriptor* desc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_ce_obj(Z_OBJCE_P(getThis())));
  MessageHeader* msg = UNBOX(MessageHeader, getThis());

  char *data = NULL;
  PHP_PROTO_SIZE data_len;
  zend_bool ignore_json_unknown = false;

  if (zend_parse_parameters(
          ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &data, &data_len,
          &ignore_json_unknown) ==
      FAILURE) {
    return;
  }

  // TODO(teboring): Check and respect string encoding. If not UTF-8, we need to
  // convert, because string handlers pass data directly to message string
  // fields.

  // TODO(teboring): Clear message.

  {
    const upb_json_parsermethod* method = msgdef_jsonparsermethod(desc);
    stackenv se;
    upb_sink sink;
    upb_json_parser* parser;
    stackenv_init(&se, "Error occurred during parsing: %s");

    upb_sink_reset(&sink, get_fill_handlers(desc), msg);
    parser = upb_json_parser_create(se.arena, method, generated_pool->symtab,
                                    sink, &se.status, ignore_json_unknown);
    upb_bufsrc_putbuf(data, data_len, upb_json_parser_input(parser));

    stackenv_uninit(&se);
  }
}

// TODO(teboring): refactoring with putrawmsg
static void discard_unknown_fields(MessageHeader* msg) {
  upb_msg_field_iter it;

  stringsink* unknown = DEREF(message_data(msg), 0, stringsink*);
  if (unknown != NULL) {
    stringsink_uninit(unknown);
    FREE(unknown);
    DEREF(message_data(msg), 0, stringsink*) = NULL;
  }

  // Recursively discard unknown fields of submessages.
  Descriptor* desc = msg->descriptor;
  TSRMLS_FETCH();
  for (upb_msg_field_begin(&it, desc->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    upb_fielddef* f = upb_msg_iter_field(&it);
    uint32_t offset = desc->layout->fields[upb_fielddef_index(f)].offset;
    bool containing_oneof = false;

    if (upb_fielddef_containingoneof(f)) {
      uint32_t oneof_case_offset =
          desc->layout->fields[upb_fielddef_index(f)].case_offset;
      // For a oneof, check that this field is actually present -- skip all the
      // below if not.
      if (DEREF(message_data(msg), oneof_case_offset, uint32_t) !=
          upb_fielddef_number(f)) {
        continue;
      }
      // Otherwise, fall through to the appropriate singular-field handler
      // below.
      containing_oneof = true;
    }

    if (is_map_field(f)) {
      MapIter map_it;
      int len, size;
      const upb_fielddef* value_field;

      value_field = map_field_value(f);
      if (!upb_fielddef_issubmsg(value_field)) continue;

      zval* map_php = CACHED_PTR_TO_ZVAL_PTR(
          DEREF(message_data(msg), offset, CACHED_VALUE*));
      if (map_php == NULL) continue;

      Map* intern = UNBOX(Map, map_php);
      for (map_begin(map_php, &map_it TSRMLS_CC);
           !map_done(&map_it); map_next(&map_it)) {
        upb_value value = map_iter_value(&map_it, &len);
        const void* memory = raw_value(upb_value_memory(&value), value_field);
#if PHP_MAJOR_VERSION < 7
        MessageHeader *submsg = UNBOX(MessageHeader, *(zval**)memory);
#else
        MessageHeader *submsg =
            (MessageHeader*)((char*)(Z_OBJ_P((zval*)memory)) -
                XtOffsetOf(MessageHeader, std));
#endif
        discard_unknown_fields(submsg);
      }
    } else if (upb_fielddef_isseq(f)) {
      if (!upb_fielddef_issubmsg(f)) continue;

      zval* array_php = CACHED_PTR_TO_ZVAL_PTR(
          DEREF(message_data(msg), offset, CACHED_VALUE*));
      if (array_php == NULL) continue;

      int size, i;
      RepeatedField* intern = UNBOX(RepeatedField, array_php);
      HashTable *ht = PHP_PROTO_HASH_OF(intern->array);
      size = zend_hash_num_elements(ht);
      if (size == 0) continue;

      for (i = 0; i < size; i++) {
        void* memory = repeated_field_index_native(intern, i TSRMLS_CC);
#if PHP_MAJOR_VERSION < 7
        MessageHeader *submsg = UNBOX(MessageHeader, *(zval**)memory);
#else
        MessageHeader *submsg =
            (MessageHeader*)((char*)(Z_OBJ_P((zval*)memory)) -
                XtOffsetOf(MessageHeader, std));
#endif
        discard_unknown_fields(submsg);
      }
    } else if (upb_fielddef_issubmsg(f)) {
      zval* submsg_php = CACHED_PTR_TO_ZVAL_PTR(
          DEREF(message_data(msg), offset, CACHED_VALUE*));
      if (Z_TYPE_P(submsg_php) == IS_NULL) continue;
      MessageHeader* submsg = UNBOX(MessageHeader, submsg_php);
      discard_unknown_fields(submsg);
    }
  }
}

PHP_METHOD(Message, discardUnknownFields) {
  MessageHeader* msg = UNBOX(MessageHeader, getThis());
  discard_unknown_fields(msg);
}
