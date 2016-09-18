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
    stackenv* se = ud;
    // Free the env -- zend_error will longjmp up the stack past the
    // encode/decode function so it would not otherwise have been freed.
    stackenv_uninit(se);

    // TODO(teboring): have a way to verify that this is actually a parse error,
    // instead of just throwing "parse error" unconditionally.
    zend_error(E_ERROR, se->php_error_template, upb_status_errmsg(status));
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
} oneof_handlerdata_t;

static const void *newoneofhandlerdata(upb_handlers *h,
                                       uint32_t ofs,
                                       uint32_t case_ofs,
                                       int property_ofs,
                                       const upb_fielddef *f) {
  oneof_handlerdata_t* hd =
      (oneof_handlerdata_t*)malloc(sizeof(oneof_handlerdata_t));
  hd->ofs = ofs;
  hd->case_ofs = case_ofs;
  hd->property_ofs = property_ofs;
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
  return (void*)(*DEREF(msg, *ofs, zval**));
}

// Handlers that append primitive values to a repeated field.
#define DEFINE_APPEND_HANDLER(type, ctype)                             \
  static bool append##type##_handler(void* closure, const void* hd,    \
                                     ctype val) {                      \
    zval* array = (zval*)closure;                                      \
    RepeatedField* intern =                                            \
        (RepeatedField*)zend_object_store_get_object(array TSRMLS_CC); \
    repeated_field_push_native(intern, &val);                          \
    return true;                                                       \
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
  RepeatedField* intern =
      (RepeatedField*)zend_object_store_get_object(array TSRMLS_CC);

  zval* str;
  MAKE_STD_ZVAL(str);
  ZVAL_STRING(str, "", 1);

  repeated_field_push_native(intern, &str TSRMLS_CC);
  return (void*)str;
}

// Appends a 'bytes' string to a repeated field.
static void* appendbytes_handler(void *closure,
                                 const void *hd,
                                 size_t size_hint) {
  zval* array = (zval*)closure;
  RepeatedField* intern =
      (RepeatedField*)zend_object_store_get_object(array TSRMLS_CC);

  zval* str;
  MAKE_STD_ZVAL(str);
  ZVAL_STRING(str, "", 1);

  repeated_field_push_native(intern, &str TSRMLS_CC);
  return (void*)str;
}

static void *empty_php_string(zval** value_ptr) {
  SEPARATE_ZVAL_IF_NOT_REF(value_ptr);
  zval* str = *value_ptr;
  zval_dtor(str);
  ZVAL_STRINGL(str, "", 0, 1);
  return (void*)str;
}

// Sets a non-repeated string field in a message.
static void* str_handler(void *closure,
                         const void *hd,
                         size_t size_hint) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  return empty_php_string(DEREF(msg, *ofs, zval**));
}

// Sets a non-repeated 'bytes' field in a message.
static void* bytes_handler(void *closure,
                           const void *hd,
                           size_t size_hint) {
  MessageHeader* msg = closure;
  const size_t *ofs = hd;
  return empty_php_string(DEREF(msg, *ofs, zval**));
}

static size_t stringdata_handler(void* closure, const void* hd,
                                 const char* str, size_t len,
                                 const upb_bufhandle* handle) {
  zval* php_str = (zval*)closure;

  char* old_str = Z_STRVAL_P(php_str);
  size_t old_len = Z_STRLEN_P(php_str);
  assert(old_str != NULL);

  char* new_str = emalloc(old_len + len + 1);

  memcpy(new_str, old_str, old_len);
  memcpy(new_str + old_len, str, len);
  new_str[old_len + len] = 0;
  FREE(old_str);

  Z_STRVAL_P(php_str) = new_str;
  Z_STRLEN_P(php_str) = old_len + len;

  return len;
}

// Appends a submessage to a repeated field.
static void *appendsubmsg_handler(void *closure, const void *hd) {
  zval* array = (zval*)closure;
  RepeatedField* intern =
      (RepeatedField*)zend_object_store_get_object(array TSRMLS_CC);

  const submsg_handlerdata_t *submsgdata = hd;
  zval* subdesc_php = get_def_obj((void*)submsgdata->md);
  Descriptor* subdesc = zend_object_store_get_object(subdesc_php TSRMLS_CC);
  zend_class_entry* subklass = subdesc->klass;
  MessageHeader* submsg;

  zval* val = NULL;
  MAKE_STD_ZVAL(val);
  Z_TYPE_P(val) = IS_OBJECT;
  Z_OBJVAL_P(val) = subklass->create_object(subklass TSRMLS_CC);

  repeated_field_push_native(intern, &val TSRMLS_CC);

  submsg = zend_object_store_get_object(val TSRMLS_CC);
  return submsg;
}

// Sets a non-repeated submessage field in a message.
static void *submsg_handler(void *closure, const void *hd) {
  MessageHeader* msg = closure;
  const submsg_handlerdata_t* submsgdata = hd;
  zval* subdesc_php = get_def_obj((void*)submsgdata->md);
  Descriptor* subdesc = zend_object_store_get_object(subdesc_php TSRMLS_CC);
  zend_class_entry* subklass = subdesc->klass;
  zval* submsg_php;
  MessageHeader* submsg;

  if (Z_TYPE_P(*DEREF(msg, submsgdata->ofs, zval**)) == IS_NULL) {
    zval* val = NULL;
    MAKE_STD_ZVAL(val);
    Z_TYPE_P(val) = IS_OBJECT;
    Z_OBJVAL_P(val) = subklass->create_object(subklass TSRMLS_CC);

    zval_ptr_dtor(DEREF(msg, submsgdata->ofs, zval**));
    *DEREF(msg, submsgdata->ofs, zval**) = val;
  }

  submsg_php = *DEREF(msg, submsgdata->ofs, zval**);

  submsg = zend_object_store_get_object(submsg_php TSRMLS_CC);
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
  zval* map;
  char key_storage[NATIVE_SLOT_MAX_SIZE];
  char value_storage[NATIVE_SLOT_MAX_SIZE];
} map_parse_frame_t;

static void map_slot_init(void* memory, upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      // Store zval** in memory in order to be consistent with the layout of
      // singular fields.
      zval** holder = ALLOC(zval*);
      zval* tmp;
      MAKE_STD_ZVAL(tmp);
      ZVAL_STRINGL(tmp, "", 0, 1);
      *holder = tmp;
      *(zval***)memory = holder;
      break;
    }
    case UPB_TYPE_MESSAGE: {
      zval** holder = ALLOC(zval*);
      zval* tmp;
      MAKE_STD_ZVAL(tmp);
      ZVAL_NULL(tmp);
      *holder = tmp;
      *(zval***)memory = holder;
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
      zval** holder = *(zval***)memory;
      zval_ptr_dtor(holder);
      FREE(holder);
      break;
    }
    default:
      break;
  }
}

static void map_slot_key(upb_fieldtype_t type, const void* from, char** keyval,
                         size_t* length) {
  if (type == UPB_TYPE_STRING) {
    zval* key_php = **(zval***)from;
    *keyval = Z_STRVAL_P(key_php);
    *length = Z_STRLEN_P(key_php);
  } else {
    *keyval = from;
    *length = native_slot_size(type);
  }
}

static void map_slot_value(upb_fieldtype_t type, const void* from, upb_value* v) {
  size_t len;
  void* to = upb_value_memory(v);
#ifndef NDEBUG
  v->ctype = UPB_CTYPE_UINT64;
#endif

  memset(to, 0, native_slot_size(type));

  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE: {
      *(zval**)to = **(zval***)from;
      Z_ADDREF_PP((zval**)to);
      break;
    }
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
  zval* map = *DEREF(msg, mapdata->ofs, zval**);

  map_parse_frame_t* frame = ALLOC(map_parse_frame_t);
  frame->map = map;

  map_slot_init(&frame->key_storage, mapdata->key_field_type);
  map_slot_init(&frame->value_storage, mapdata->value_field_type);

  return frame;
}

// Handler to end a map entry: inserts the value defined during the message into
// the map. This is the 'endmsg' handler on the map entry msgdef.
static bool endmap_handler(void *closure, const void *hd, upb_status* s) {
  map_parse_frame_t* frame = closure;
  const map_handlerdata_t* mapdata = hd;

  Map *map = (Map *)zend_object_store_get_object(frame->map TSRMLS_CC);

  const char* keyval = NULL;
  upb_value v;
  size_t length;

  map_slot_key(map->key_type, &frame->key_storage, &keyval, &length);
  map_slot_value(map->value_type, &frame->value_storage, &v);

  map_index_set(map, keyval, length, v);

  map_slot_uninit(&frame->key_storage, mapdata->key_field_type);
  map_slot_uninit(&frame->value_storage, mapdata->value_field_type);
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

  DEREF(msg, oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;
  DEREF(msg, oneofdata->ofs, zval**) =
      &(msg->std.properties_table)[oneofdata->property_ofs];

  return empty_php_string(DEREF(msg, oneofdata->ofs, zval**));
}

static void *oneofbytes_handler(void *closure,
                                const void *hd,
                                size_t size_hint) {
  MessageHeader* msg = closure;
  const oneof_handlerdata_t *oneofdata = hd;

  DEREF(msg, oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;
  DEREF(msg, oneofdata->ofs, zval**) =
      &(msg->std.properties_table)[oneofdata->property_ofs];

  // TODO(teboring): Add it back.
  // rb_enc_associate(str, kRubyString8bitEncoding);

  SEPARATE_ZVAL_IF_NOT_REF(DEREF(msg, oneofdata->ofs, zval**));
  zval* str = *DEREF(msg, oneofdata->ofs, zval**);
  zval_dtor(str);
  ZVAL_STRINGL(str, "", 0, 1);
  return (void*)str;
}

// Handler for a submessage field in a oneof.
static void *oneofsubmsg_handler(void *closure,
                                 const void *hd) {
  MessageHeader* msg = closure;
  const oneof_handlerdata_t *oneofdata = hd;
  uint32_t oldcase = DEREF(msg, oneofdata->case_ofs, uint32_t);
  zval* subdesc_php = get_def_obj((void*)oneofdata->md);
  Descriptor* subdesc = zend_object_store_get_object(subdesc_php TSRMLS_CC);
  zend_class_entry* subklass = subdesc->klass;
  zval* submsg_php;
  MessageHeader* submsg;

  if (oldcase != oneofdata->oneof_case_num) {
    DEREF(msg, oneofdata->ofs, zval**) =
        &(msg->std.properties_table)[oneofdata->property_ofs];
  }

  if (Z_TYPE_P(*DEREF(msg, oneofdata->ofs, zval**)) == IS_NULL) {
    zval* val = NULL;
    MAKE_STD_ZVAL(val);
    Z_TYPE_P(val) = IS_OBJECT;
    Z_OBJVAL_P(val) = subklass->create_object(subklass TSRMLS_CC);

    zval_ptr_dtor(DEREF(msg, oneofdata->ofs, zval**));
    *DEREF(msg, oneofdata->ofs, zval**) = val;
  }

  DEREF(msg, oneofdata->case_ofs, uint32_t) =
      oneofdata->oneof_case_num;

  submsg_php = *DEREF(msg, oneofdata->ofs, zval**);
  submsg = zend_object_store_get_object(submsg_php TSRMLS_CC);
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
      upb_handlers_setstring(h, f, stringdata_handler, NULL);
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
    case UPB_TYPE_BOOL:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_ENUM:
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_DOUBLE:
      upb_shim_set(h, f, offset, -1);
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
                                  offsetof(map_parse_frame_t, key_storage));
  add_handlers_for_singular_field(h, value_field,
                                  offsetof(map_parse_frame_t, value_storage));
}

// Set up handlers for a oneof field.
static void add_handlers_for_oneof_field(upb_handlers *h,
                                         const upb_fielddef *f,
                                         size_t offset,
                                         size_t oneof_case_offset,
                                         int property_cache_offset) {

  upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
  upb_handlerattr_sethandlerdata(
      &attr, newoneofhandlerdata(h, offset, oneof_case_offset,
                                 property_cache_offset, f));

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

static void add_handlers_for_message(const void *closure, upb_handlers *h) {
  const upb_msgdef* msgdef = upb_handlers_msgdef(h);
  Descriptor* desc = (Descriptor*)zend_object_store_get_object(
      get_def_obj((void*)msgdef) TSRMLS_CC);
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
    size_t offset = desc->layout->fields[upb_fielddef_index(f)].offset +
        sizeof(MessageHeader);

    if (upb_fielddef_containingoneof(f)) {
      size_t oneof_case_offset =
          desc->layout->fields[upb_fielddef_index(f)].case_offset +
          sizeof(MessageHeader);
      int property_cache_index =
          desc->layout->fields[upb_fielddef_index(f)].cache_index;
      add_handlers_for_oneof_field(h, f, offset, oneof_case_offset,
                                   property_cache_index);
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

// -----------------------------------------------------------------------------
// Serializing.
// -----------------------------------------------------------------------------

static void putmsg(zval* msg, const Descriptor* desc, upb_sink* sink,
                   int depth);

static void putstr(zval* str, const upb_fielddef* f, upb_sink* sink);

static void putrawstr(const char* str, int len, const upb_fielddef* f,
                      upb_sink* sink);

static void putsubmsg(zval* submsg, const upb_fielddef* f, upb_sink* sink,
                      int depth);

static void putarray(zval* array, const upb_fielddef* f, upb_sink* sink,
                     int depth);
static void putmap(zval* map, const upb_fielddef* f, upb_sink* sink, int depth);

static upb_selector_t getsel(const upb_fielddef* f, upb_handlertype_t type) {
  upb_selector_t ret;
  bool ok = upb_handlers_getselector(f, type, &ret);
  UPB_ASSERT(ok);
  return ret;
}

static void put_optional_value(void* memory, int len, const upb_fielddef* f,
                               int depth, upb_sink* sink) {
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
      zval* submsg = *(zval**)memory;
      putsubmsg(submsg, f, sink, depth);
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
      return Z_STRVAL_PP((zval**)memory);
      break;
    default:
      return memory;
  }
}

static int raw_value_len(void* memory, int len, const upb_fielddef* f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      return Z_STRLEN_PP((zval**)memory);
      break;
    default:
      return len;
  }
}

static void putmap(zval* map, const upb_fielddef* f, upb_sink* sink,
                   int depth) {
  Map* self;
  upb_sink subsink;
  const upb_fielddef* key_field;
  const upb_fielddef* value_field;
  MapIter it;
  int len;

  if (map == NULL) return;
  self = UNBOX(Map, map);

  upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);

  assert(upb_fielddef_type(f) == UPB_TYPE_MESSAGE);
  key_field = map_field_key(f);
  value_field = map_field_value(f);

  for (map_begin(map, &it); !map_done(&it); map_next(&it)) {
    upb_status status;

    upb_sink entry_sink;
    upb_sink_startsubmsg(&subsink, getsel(f, UPB_HANDLER_STARTSUBMSG),
                         &entry_sink);
    upb_sink_startmsg(&entry_sink);

    // Serialize key.
    const char *key = map_iter_key(&it, &len);
    put_optional_value(key, len, key_field, depth + 1, &entry_sink);

    // Serialize value.
    upb_value value = map_iter_value(&it, &len);
    put_optional_value(raw_value(upb_value_memory(&value), value_field),
                       raw_value_len(upb_value_memory(&value), len, value_field),
                       value_field, depth + 1, &entry_sink);

    upb_sink_endmsg(&entry_sink, &status);
    upb_sink_endsubmsg(&subsink, getsel(f, UPB_HANDLER_ENDSUBMSG));
  }

  upb_sink_endseq(sink, getsel(f, UPB_HANDLER_ENDSEQ));
}

static void putmsg(zval* msg_php, const Descriptor* desc, upb_sink* sink,
                   int depth) {
  upb_msg_field_iter i;
  upb_status status;

  upb_sink_startmsg(sink);

  // Protect against cycles (possible because users may freely reassign message
  // and repeated fields) by imposing a maximum recursion depth.
  if (depth > ENCODE_MAX_NESTING) {
    zend_error(E_ERROR,
             "Maximum recursion depth exceeded during encoding.");
  }

  MessageHeader* msg = zend_object_store_get_object(msg_php TSRMLS_CC);

  for (upb_msg_field_begin(&i, desc->msgdef); !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef* f = upb_msg_iter_field(&i);
    uint32_t offset = desc->layout->fields[upb_fielddef_index(f)].offset +
                      sizeof(MessageHeader);

    if (upb_fielddef_containingoneof(f)) {
      uint32_t oneof_case_offset =
          desc->layout->fields[upb_fielddef_index(f)].case_offset +
          sizeof(MessageHeader);
      // For a oneof, check that this field is actually present -- skip all the
      // below if not.
      if (DEREF(msg, oneof_case_offset, uint32_t) != upb_fielddef_number(f)) {
        continue;
      }
      // Otherwise, fall through to the appropriate singular-field handler
      // below.
    }

    if (is_map_field(f)) {
      zval* map = *DEREF(msg, offset, zval**);
      if (map != NULL) {
        putmap(map, f, sink, depth);
      }
    } else if (upb_fielddef_isseq(f)) {
      zval* array = *DEREF(msg, offset, zval**);
      if (array != NULL) {
        putarray(array, f, sink, depth);
      }
    } else if (upb_fielddef_isstring(f)) {
      zval* str = *DEREF(msg, offset, zval**);
      if (Z_STRLEN_P(str) > 0) {
        putstr(str, f, sink);
      }
    } else if (upb_fielddef_issubmsg(f)) {
      putsubmsg(*DEREF(msg, offset, zval**), f, sink, depth);
    } else {
      upb_selector_t sel = getsel(f, upb_handlers_getprimitivehandlertype(f));

#define T(upbtypeconst, upbtype, ctype, default_value) \
  case upbtypeconst: {                                 \
    ctype value = DEREF(msg, offset, ctype);           \
    if (value != default_value) {                      \
      upb_sink_put##upbtype(sink, sel, value);         \
    }                                                  \
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

  // Ensure that the string has the correct encoding. We also check at field-set
  // time, but the user may have mutated the string object since then.
  if (upb_fielddef_type(f) == UPB_TYPE_STRING &&
      !is_structurally_valid_utf8(Z_STRVAL_P(str), Z_STRLEN_P(str))) {
    zend_error(E_USER_ERROR, "Given string is not UTF8 encoded.");
    return;
  }

  upb_sink_startstr(sink, getsel(f, UPB_HANDLER_STARTSTR), Z_STRLEN_P(str),
                    &subsink);
  upb_sink_putstring(&subsink, getsel(f, UPB_HANDLER_STRING), Z_STRVAL_P(str),
                     Z_STRLEN_P(str), NULL);
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

static void putsubmsg(zval* submsg, const upb_fielddef* f, upb_sink* sink,
                      int depth) {
  upb_sink subsink;

  if (Z_TYPE_P(submsg) == IS_NULL) return;

  zval* php_descriptor = get_def_obj(upb_fielddef_msgsubdef(f));
  Descriptor* subdesc =
      (Descriptor*)zend_object_store_get_object(php_descriptor TSRMLS_CC);

  upb_sink_startsubmsg(sink, getsel(f, UPB_HANDLER_STARTSUBMSG), &subsink);
  putmsg(submsg, subdesc, &subsink, depth + 1);
  upb_sink_endsubmsg(sink, getsel(f, UPB_HANDLER_ENDSUBMSG));
}

static void putarray(zval* array, const upb_fielddef* f, upb_sink* sink,
                     int depth) {
  upb_sink subsink;
  upb_fieldtype_t type = upb_fielddef_type(f);
  upb_selector_t sel = 0;
  int size, i;

  assert(array != NULL);
  RepeatedField* intern =
      (RepeatedField*)zend_object_store_get_object(array TSRMLS_CC);
  size = zend_hash_num_elements(HASH_OF(intern->array));
  if (size == 0) return;

  upb_sink_startseq(sink, getsel(f, UPB_HANDLER_STARTSEQ), &subsink);

  if (upb_fielddef_isprimitive(f)) {
    sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  }

  for (i = 0; i < size; i++) {
    void* memory = repeated_field_index_native(intern, i);
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
      case UPB_TYPE_BYTES:
        putstr(*((zval**)memory), f, &subsink);
        break;
      case UPB_TYPE_MESSAGE:
        putsubmsg(*((zval**)memory), f, &subsink, depth);
        break;

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

// -----------------------------------------------------------------------------
// PHP encode/decode methods
// -----------------------------------------------------------------------------

PHP_METHOD(Message, encode) {
  zval* php_descriptor = get_ce_obj(Z_OBJCE_P(getThis()));
  Descriptor* desc =
      (Descriptor*)zend_object_store_get_object(php_descriptor TSRMLS_CC);

  stringsink sink;
  stringsink_init(&sink);

  {
    const upb_handlers* serialize_handlers = msgdef_pb_serialize_handlers(desc);

    stackenv se;
    upb_pb_encoder* encoder;

    stackenv_init(&se, "Error occurred during encoding: %s");
    encoder = upb_pb_encoder_create(&se.env, serialize_handlers, &sink.sink);

    putmsg(getThis(), desc, upb_pb_encoder_input(encoder), 0);

    RETVAL_STRINGL(sink.ptr, sink.len, 1);

    stackenv_uninit(&se);
    stringsink_uninit(&sink);
  }
}

PHP_METHOD(Message, decode) {
  zval* php_descriptor = get_ce_obj(Z_OBJCE_P(getThis()));
  Descriptor* desc =
      (Descriptor*)zend_object_store_get_object(php_descriptor TSRMLS_CC);
  MessageHeader* msg = zend_object_store_get_object(getThis() TSRMLS_CC);

  char *data = NULL;
  int data_len;
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
