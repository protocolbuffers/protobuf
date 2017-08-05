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

#include "protobuf.h"
#include "utf8.h"

/* stringsink *****************************************************************/

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

/* stackenv *****************************************************************/

// Stack-allocated context during an encode/decode operation. Contains the upb
// environment and its stack-based allocator, an initial buffer for allocations
// to avoid malloc() when possible, and a template for PHP exception messages
// if any error occurs.
#define STACK_ENV_STACKBYTES 4096
typedef struct {
  upb_env env;
  const char *php_error_template;
  char allocbuf[STACK_ENV_STACKBYTES];
} stackenv;


static void stackenv_init(stackenv* se, const char* errmsg);
static void stackenv_uninit(stackenv* se);

// Callback invoked by upb if any error occurs during parsing or serialization.
static bool env_error_func(void* ud, const upb_status* status) {
    char err_msg[100] = "";
    stackenv* se = ud;
    // Free the env -- zend_error will longjmp up the stack past the
    // encode/decode function so it would not otherwise have been freed.
    stackenv_uninit(se);

    // TODO(teboring): have a way to verify that this is actually a parse error,
    // instead of just throwing "parse error" unconditionally.
    sprintf(err_msg, se->php_error_template, upb_status_errmsg(status));
    TSRMLS_FETCH();
    zend_throw_exception(NULL, err_msg, 0 TSRMLS_CC);
    // Never reached.
    return false;
}

static void stackenv_init(stackenv* se, const char* errmsg) {
  se->php_error_template = errmsg;
  upb_env_init2(&se->env, se->allocbuf, sizeof(se->allocbuf), NULL);
  upb_env_seterrorfunc(&se->env, env_error_func, se);
}

static void stackenv_uninit(stackenv* se) {
  upb_env_uninit(&se->env);
}

// -----------------------------------------------------------------------------
// Parsing.
// -----------------------------------------------------------------------------

#define DEREF(msg, ofs, type) *(type*)(((uint8_t *)msg) + ofs)

// Creates a handlerdata that simply contains the offset for this field.
static const void* newhandlerdata(upb_handlers* h, uint32_t ofs) {
  size_t* hd_ofs = (size_t*)malloc(sizeof(size_t));
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

// Appends a string to a repeated field.
static void* appendstr_handler(void *closure,
                               const void *hd,
                               size_t size_hint) {
  zval* array = (zval*)closure;
  TSRMLS_FETCH();
  RepeatedField* intern = UNBOX(RepeatedField, array);

#if PHP_MAJOR_VERSION < 7
  zval* str;
  MAKE_STD_ZVAL(str);
  PHP_PROTO_ZVAL_STRING(str, "", 1);
  repeated_field_push_native(intern, &str);
  return (void*)str;
#else
  zend_string* str = zend_string_init("", 0, 1);
  repeated_field_push_native(intern, &str);
  return intern;
#endif
}

// Appends a 'bytes' string to a repeated field.
static void* appendbytes_handler(void *closure,
                                 const void *hd,
                                 size_t size_hint) {
  zval* array = (zval*)closure;
  TSRMLS_FETCH();
  RepeatedField* intern = UNBOX(RepeatedField, array);

#if PHP_MAJOR_VERSION < 7
  zval* str;
  MAKE_STD_ZVAL(str);
  PHP_PROTO_ZVAL_STRING(str, "", 1);
  repeated_field_push_native(intern, &str);
  return (void*)str;
#else
  zend_string* str = zend_string_init("", 0, 1);
  repeated_field_push_native(intern, &str);
  return intern;
#endif
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

// Sets a non-repeated string field in a message.
static void* str_handler(void *closure,
                         const void *hd,
                         size_t size_hint) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  return empty_php_string(DEREF(message_data(msg), *ofs, CACHED_VALUE*));
}

// Sets a non-repeated 'bytes' field in a message.
static void* bytes_handler(void *closure,
                           const void *hd,
                           size_t size_hint) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  return empty_php_string(DEREF(message_data(msg), *ofs, CACHED_VALUE*));
}

static size_t stringdata_handler(void* closure, const void* hd,
                                 const char* str, size_t len,
                                 const upb_bufhandle* handle) {
  zval* php_str = (zval*)closure;
#if PHP_MAJOR_VERSION < 7
  // Oneof string/bytes fields may have NULL initial value, which doesn't need
  // to be freed.
  if (Z_TYPE_P(php_str) == IS_STRING && !IS_INTERNED(Z_STRVAL_P(php_str))) {
    FREE(Z_STRVAL_P(php_str));
  }
  ZVAL_STRINGL(php_str, str, len, 1);
#else
  if (Z_TYPE_P(php_str) == IS_STRING) {
    zend_string_release(Z_STR_P(php_str));
  }
  ZVAL_NEW_STR(php_str, zend_string_init(str, len, 0));
#endif
  return len;
}

#if PHP_MAJOR_VERSION >= 7
static size_t zendstringdata_handler(void* closure, const void* hd,
                                     const char* str, size_t len,
                                     const upb_bufhandle* handle) {
  RepeatedField* intern = (RepeatedField*)closure;

  unsigned char memory[NATIVE_SLOT_MAX_SIZE];
  memset(memory, 0, NATIVE_SLOT_MAX_SIZE);
  *(zend_string**)memory = zend_string_init(str, len, 0);

  HashTable *ht = PHP_PROTO_HASH_OF(intern->array);
  int index = zend_hash_num_elements(ht) - 1;
  php_proto_zend_hash_index_update_mem(
      ht, index, memory, sizeof(zend_string*), NULL);

  return len;
}
#endif

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
    zval* val = NULL;
    MAKE_STD_ZVAL(val);
    ZVAL_OBJ(val, subklass->create_object(subklass TSRMLS_CC));
    MessageHeader* intern = UNBOX(MessageHeader, val);
    custom_data_init(subklass, intern PHP_PROTO_TSRMLS_CC);
    php_proto_zval_ptr_dtor(*DEREF(message_data(msg), submsgdata->ofs, zval**));
    *DEREF(message_data(msg), submsgdata->ofs, zval**) = val;
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

  // We know that we can hold this reference because the handlerdata has the
  // same lifetime as the upb_handlers struct, and the upb_handlers struct holds
  // a reference to the upb_msgdef, which in turn has references to its subdefs.
  const upb_def* value_field_subdef;
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
      zval* tmp;
      MAKE_STD_ZVAL(tmp);
      PHP_PROTO_ZVAL_STRINGL(tmp, "", 0, 1);
      *holder = tmp;
      *(zval***)memory = holder;
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
      php_proto_zval_ptr_dtor(*holder);
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
      ++GC_REFCOUNT(*(zend_object**)to);
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
  hd->value_field_subdef = upb_fielddef_subdef(value_field);

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

static void *oneofstr_handler(void *closure,
                              const void *hd,
                              size_t size_hint) {
  // TODO(teboring): Add it back.
  // rb_enc_associate(str, kRubyString8bitEncoding);
  return oneofbytes_handler(closure, hd, size_hint);
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
    ZVAL_OBJ(CACHED_PTR_TO_ZVAL_PTR(
        DEREF(message_data(msg), oneofdata->ofs, CACHED_VALUE*)),
        subklass->create_object(subklass TSRMLS_CC));
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
#if PHP_MAJOR_VERSION < 7
      upb_handlers_setstring(h, f, stringdata_handler, NULL);
#else
      upb_handlers_setstring(h, f, zendstringdata_handler, NULL);
#endif
      break;
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

// Set up handlers for a singular field.
static void add_handlers_for_singular_field(upb_handlers *h,
                                            const upb_fielddef *f,
                                            size_t offset) {
  switch (upb_fielddef_type(f)) {

#define SET_HANDLER(utype, ltype)                                     \
  case utype: {                                                       \
    upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;               \
    upb_handlerattr_sethandlerdata(&attr, newhandlerdata(h, offset)); \
    upb_handlers_set##ltype(h, f, ltype##_handler, &attr);            \
    break;                                                            \
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

// Adds handlers to a map field.
static void add_handlers_for_mapfield(upb_handlers* h,
                                      const upb_fielddef* fielddef,
                                      size_t offset,
                                      Descriptor* desc) {
  const upb_msgdef* map_msgdef = upb_fielddef_msgsubdef(fielddef);
  map_handlerdata_t* hd = new_map_handlerdata(offset, map_msgdef, desc);
  upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;

  upb_handlers_addcleanup(h, hd, free);
  upb_handlerattr_sethandlerdata(&attr, hd);
  upb_handlers_setstartsubmsg(h, fielddef, startmapentry_handler, &attr);
  upb_handlerattr_uninit(&attr);
}

// Adds handlers to a map-entry msgdef.
static void add_handlers_for_mapentry(const upb_msgdef* msgdef, upb_handlers* h,
                                      Descriptor* desc) {
  const upb_fielddef* key_field = map_entry_key(msgdef);
  const upb_fielddef* value_field = map_entry_value(msgdef);
  map_handlerdata_t* hd = new_map_handlerdata(0, msgdef, desc);
  upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;

  upb_handlers_addcleanup(h, hd, free);
  upb_handlerattr_sethandlerdata(&attr, hd);
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

  upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
  upb_handlerattr_sethandlerdata(
      &attr, newoneofhandlerdata(h, offset, oneof_case_offset,
                                 property_cache_offset, m, f));

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
      break;
    }
    case UPB_TYPE_MESSAGE: {
      upb_handlers_setstartsubmsg(h, f, oneofsubmsg_handler, &attr);
      break;
    }
  }

  upb_handlerattr_uninit(&attr);
}

static void add_handlers_for_message(const void* closure,
                                     upb_handlers* h) {
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

const upb_pbdecodermethod *new_fillmsg_decodermethod(Descriptor* desc,
                                                     const void* owner) {
  const upb_handlers* handlers = get_fill_handlers(desc);
  upb_pbdecodermethodopts opts;
  upb_pbdecodermethodopts_init(&opts, handlers);

  return upb_pbdecodermethod_new(&opts, owner);
}

static const upb_pbdecodermethod *msgdef_decodermethod(Descriptor* desc) {
  if (desc->fill_method == NULL) {
    desc->fill_method = new_fillmsg_decodermethod(
        desc, &desc->fill_method);
  }
  return desc->fill_method;
}

static const upb_json_parsermethod *msgdef_jsonparsermethod(Descriptor* desc) {
  if (desc->json_fill_method == NULL) {
    desc->json_fill_method =
        upb_json_parsermethod_new(desc->msgdef, &desc->json_fill_method);
  }
  return desc->json_fill_method;
}

// -----------------------------------------------------------------------------
// Serializing.
// -----------------------------------------------------------------------------

static void putmsg(zval* msg, const Descriptor* desc, upb_sink* sink,
                   int depth TSRMLS_DC);
static void putrawmsg(MessageHeader* msg, const Descriptor* desc,
                      upb_sink* sink, int depth TSRMLS_DC);

static void putstr(zval* str, const upb_fielddef* f, upb_sink* sink);

static void putrawstr(const char* str, int len, const upb_fielddef* f,
                      upb_sink* sink);

static void putsubmsg(zval* submsg, const upb_fielddef* f, upb_sink* sink,
                      int depth TSRMLS_DC);
static void putrawsubmsg(MessageHeader* submsg, const upb_fielddef* f,
                         upb_sink* sink, int depth TSRMLS_DC);

static void putarray(zval* array, const upb_fielddef* f, upb_sink* sink,
                     int depth TSRMLS_DC);
static void putmap(zval* map, const upb_fielddef* f, upb_sink* sink,
                   int depth TSRMLS_DC);

static upb_selector_t getsel(const upb_fielddef* f, upb_handlertype_t type) {
  upb_selector_t ret;
  bool ok = upb_handlers_getselector(f, type, &ret);
  UPB_ASSERT(ok);
  return ret;
}

static void put_optional_value(const void* memory, int len, const upb_fielddef* f,
                               int depth, upb_sink* sink TSRMLS_DC) {
  assert(upb_fielddef_label(f) == UPB_LABEL_OPTIONAL);

  switch (upb_fielddef_type(f)) {
#define T(upbtypeconst, upbtype, ctype, default_value)                         \
  case upbtypeconst: {                                                         \
    ctype value = DEREF(memory, 0, ctype);                                     \
    if (value != default_value) {                                              \
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
      putrawstr(memory, len, f, sink);
      break;
    case UPB_TYPE_MESSAGE: {
#if PHP_MAJOR_VERSION < 7
      MessageHeader *submsg = UNBOX(MessageHeader, *(zval**)memory);
#else
      MessageHeader *submsg =
          (MessageHeader*)((char*)(*(zend_object**)memory) -
              XtOffsetOf(MessageHeader, std));
#endif
      putrawsubmsg(submsg, f, sink, depth TSRMLS_CC);
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

static void putmap(zval* map, const upb_fielddef* f, upb_sink* sink,
                   int depth TSRMLS_DC) {
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
    upb_sink_startsubmsg(&subsink, getsel(f, UPB_HANDLER_STARTSUBMSG),
                         &entry_sink);
    upb_sink_startmsg(&entry_sink);

    // Serialize key.
    const char *key = map_iter_key(&it, &len);
    put_optional_value(key, len, key_field, depth + 1, &entry_sink TSRMLS_CC);

    // Serialize value.
    upb_value value = map_iter_value(&it, &len);
    put_optional_value(raw_value(upb_value_memory(&value), value_field),
                       raw_value_len(upb_value_memory(&value), len, value_field),
                       value_field, depth + 1, &entry_sink TSRMLS_CC);

    upb_sink_endmsg(&entry_sink, &status);
    upb_sink_endsubmsg(&subsink, getsel(f, UPB_HANDLER_ENDSUBMSG));
  }

  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static void putmsg(zval* msg_php, const Descriptor* desc, upb_sink* sink,
                   int depth TSRMLS_DC) {
  MessageHeader* msg = UNBOX(MessageHeader, msg_php);
  putrawmsg(msg, desc, sink, depth TSRMLS_CC);
}

static void putrawmsg(MessageHeader* msg, const Descriptor* desc,
                      upb_sink* sink, int depth TSRMLS_DC) {
  upb_msg_field_iter i;
  upb_status status;

  upb_sink_startmsg(sink);

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
        putmap(map, f, sink, depth TSRMLS_CC);
      }
    } else if (upb_fielddef_isseq(f)) {
      zval* array = CACHED_PTR_TO_ZVAL_PTR(
          DEREF(message_data(msg), offset, CACHED_VALUE*));
      if (array != NULL) {
        putarray(array, f, sink, depth TSRMLS_CC);
      }
    } else if (upb_fielddef_isstring(f)) {
      zval* str = CACHED_PTR_TO_ZVAL_PTR(
          DEREF(message_data(msg), offset, CACHED_VALUE*));
      if (containing_oneof || Z_STRLEN_P(str) > 0) {
        putstr(str, f, sink);
      }
    } else if (upb_fielddef_issubmsg(f)) {
      putsubmsg(CACHED_PTR_TO_ZVAL_PTR(
                    DEREF(message_data(msg), offset, CACHED_VALUE*)),
                f, sink, depth TSRMLS_CC);
    } else {
      upb_selector_t sel = getsel(f, upb_handlers_getprimitivehandlertype(f));

#define T(upbtypeconst, upbtype, ctype, default_value)     \
  case upbtypeconst: {                                     \
    ctype value = DEREF(message_data(msg), offset, ctype); \
    if (containing_oneof || value != default_value) {      \
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

  upb_sink_endmsg(sink, &status);
}

static void putstr(zval* str, const upb_fielddef *f, upb_sink *sink) {
  upb_sink subsink;

  if (ZVAL_IS_NULL(str)) return;

  assert(Z_TYPE_P(str) == IS_STRING);

  upb_sink_startstr(sink, getsel(f, UPB_HANDLER_STARTSTR), Z_STRLEN_P(str),
                    &subsink);

  // For oneof string field, we may get here with string length is zero.
  if (Z_STRLEN_P(str) > 0) {
    // Ensure that the string has the correct encoding. We also check at
    // field-set time, but the user may have mutated the string object since
    // then.
    if (upb_fielddef_type(f) == UPB_TYPE_STRING &&
        !is_structurally_valid_utf8(Z_STRVAL_P(str), Z_STRLEN_P(str))) {
      zend_error(E_USER_ERROR, "Given string is not UTF8 encoded.");
      return;
    }
    upb_sink_putstring(&subsink, getsel(f, UPB_HANDLER_STRING), Z_STRVAL_P(str),
                       Z_STRLEN_P(str), NULL);
  }

  upb_sink_endstr(sink, getsel(f, UPB_HANDLER_ENDSTR));
}

static void putrawstr(const char* str, int len, const upb_fielddef* f,
                      upb_sink* sink) {
  upb_sink subsink;

  if (len == 0) return;

  // Ensure that the string has the correct encoding. We also check at field-set
  // time, but the user may have mutated the string object since then.
  if (upb_fielddef_type(f) == UPB_TYPE_STRING &&
      !is_structurally_valid_utf8(str, len)) {
    zend_error(E_USER_ERROR, "Given string is not UTF8 encoded.");
    return;
  }

  upb_sink_startstr(sink, getsel(f, UPB_HANDLER_STARTSTR), len, &subsink);
  upb_sink_putstring(&subsink, getsel(f, UPB_HANDLER_STRING), str, len, NULL);
  upb_sink_endstr(sink, getsel(f, UPB_HANDLER_ENDSTR));
}

static void putsubmsg(zval* submsg_php, const upb_fielddef* f, upb_sink* sink,
                      int depth TSRMLS_DC) {
  if (Z_TYPE_P(submsg_php) == IS_NULL) return;

  MessageHeader *submsg = UNBOX(MessageHeader, submsg_php);
  putrawsubmsg(submsg, f, sink, depth TSRMLS_CC);
}

static void putrawsubmsg(MessageHeader* submsg, const upb_fielddef* f,
                         upb_sink* sink, int depth TSRMLS_DC) {
  upb_sink subsink;

  Descriptor* subdesc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(upb_fielddef_msgsubdef(f)));

  upb_sink_startsubmsg(sink, getsel(f, UPB_HANDLER_STARTSUBMSG), &subsink);
  putrawmsg(submsg, subdesc, &subsink, depth + 1 TSRMLS_CC);
  upb_sink_endsubmsg(sink, getsel(f, UPB_HANDLER_ENDSUBMSG));
}

static void putarray(zval* array, const upb_fielddef* f, upb_sink* sink,
                     int depth TSRMLS_DC) {
  upb_sink subsink;
  upb_fieldtype_t type = upb_fielddef_type(f);
  upb_selector_t sel = 0;
  int size, i;

  assert(array != NULL);
  RepeatedField* intern = UNBOX(RepeatedField, array);
  HashTable *ht = PHP_PROTO_HASH_OF(intern->array);
  size = zend_hash_num_elements(ht);
  // size = zend_hash_num_elements(PHP_PROTO_HASH_OF(intern->array));
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
    upb_sink_put##upbtype(&subsink, sel, *((ctype*)memory)); \
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
        putrawstr(rawstr, len, f, &subsink);
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
        putrawsubmsg(submsg, f, &subsink, depth TSRMLS_CC);
        break;
      }

#undef T
    }
  }
  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static const upb_handlers* msgdef_pb_serialize_handlers(Descriptor* desc) {
  if (desc->pb_serialize_handlers == NULL) {
    desc->pb_serialize_handlers =
        upb_pb_encoder_newhandlers(desc->msgdef, &desc->pb_serialize_handlers);
  }
  return desc->pb_serialize_handlers;
}

static const upb_handlers* msgdef_json_serialize_handlers(
    Descriptor* desc, bool preserve_proto_fieldnames) {
  if (preserve_proto_fieldnames) {
    if (desc->json_serialize_handlers == NULL) {
      desc->json_serialize_handlers =
          upb_json_printer_newhandlers(
              desc->msgdef, true, &desc->json_serialize_handlers);
    }
    return desc->json_serialize_handlers;
  } else {
    if (desc->json_serialize_handlers_preserve == NULL) {
      desc->json_serialize_handlers_preserve =
          upb_json_printer_newhandlers(
              desc->msgdef, false, &desc->json_serialize_handlers_preserve);
    }
    return desc->json_serialize_handlers_preserve;
  }
}

// -----------------------------------------------------------------------------
// PHP encode/decode methods
// -----------------------------------------------------------------------------

PHP_METHOD(Message, serializeToString) {
  Descriptor* desc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_ce_obj(Z_OBJCE_P(getThis())));

  stringsink sink;
  stringsink_init(&sink);

  {
    const upb_handlers* serialize_handlers = msgdef_pb_serialize_handlers(desc);

    stackenv se;
    upb_pb_encoder* encoder;

    stackenv_init(&se, "Error occurred during encoding: %s");
    encoder = upb_pb_encoder_create(&se.env, serialize_handlers, &sink.sink);

    putmsg(getThis(), desc, upb_pb_encoder_input(encoder), 0 TSRMLS_CC);

    PHP_PROTO_RETVAL_STRINGL(sink.ptr, sink.len, 1);

    stackenv_uninit(&se);
    stringsink_uninit(&sink);
  }
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

  {
    const upb_pbdecodermethod* method = msgdef_decodermethod(desc);
    const upb_handlers* h = upb_pbdecodermethod_desthandlers(method);
    stackenv se;
    upb_sink sink;
    upb_pbdecoder* decoder;
    stackenv_init(&se, "Error occurred during parsing: %s");

    upb_sink_reset(&sink, h, msg);
    decoder = upb_pbdecoder_create(&se.env, method, &sink);
    upb_bufsrc_putbuf(data, data_len, upb_pbdecoder_input(decoder));

    stackenv_uninit(&se);
  }
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
    printer = upb_json_printer_create(&se.env, serialize_handlers, &sink.sink);

    putmsg(getThis(), desc, upb_json_printer_input(printer), 0 TSRMLS_CC);

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

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) ==
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
    parser = upb_json_parser_create(&se.env, method, &sink);
    upb_bufsrc_putbuf(data, data_len, upb_json_parser_input(parser));

    stackenv_uninit(&se);
  }
}
