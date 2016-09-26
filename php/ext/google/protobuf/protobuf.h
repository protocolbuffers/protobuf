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

#ifndef __GOOGLE_PROTOBUF_PHP_PROTOBUF_H__
#define __GOOGLE_PROTOBUF_PHP_PROTOBUF_H__

#include <php.h>

// ubp.h has to be placed after php.h. Othwise, php.h will introduce NDEBUG.
#include "upb.h"

#define PHP_PROTOBUF_EXTNAME "protobuf"
#define PHP_PROTOBUF_VERSION "3.1.0a1"

// -----------------------------------------------------------------------------
// Forward Declaration
// ----------------------------------------------------------------------------

struct DescriptorPool;
struct Descriptor;
struct EnumDescriptor;
struct FieldDescriptor;
struct MessageField;
struct MessageHeader;
struct MessageLayout;
struct RepeatedField;
struct MapField;

typedef struct DescriptorPool DescriptorPool;
typedef struct Descriptor Descriptor;
typedef struct EnumDescriptor EnumDescriptor;
typedef struct FieldDescriptor FieldDescriptor;
typedef struct MessageField MessageField;
typedef struct MessageHeader MessageHeader;
typedef struct MessageLayout MessageLayout;
typedef struct RepeatedField RepeatedField;
typedef struct MapField MapField;

// -----------------------------------------------------------------------------
// Globals.
// -----------------------------------------------------------------------------

ZEND_BEGIN_MODULE_GLOBALS(protobuf)
ZEND_END_MODULE_GLOBALS(protobuf)

ZEND_DECLARE_MODULE_GLOBALS(protobuf)

#ifdef ZTS
#define PROTOBUF_G(v) TSRMG(protobuf_globals_id, zend_protobuf_globals*, v)
#else
#define PROTOBUF_G(v) (protobuf_globals.v)
#endif

// Init module and PHP classes.
void descriptor_init(TSRMLS_D);
void enum_descriptor_init(TSRMLS_D);
void descriptor_pool_init(TSRMLS_D);
void gpb_type_init(TSRMLS_D);
void map_field_init(TSRMLS_D);
void repeated_field_init(TSRMLS_D);
void util_init(TSRMLS_D);
void message_init(TSRMLS_D);

// Global map from upb {msg,enum}defs to wrapper Descriptor/EnumDescriptor
// instances.
void add_def_obj(const void* def, zval* value);
zval* get_def_obj(const void* def);

// Global map from PHP class entries to wrapper Descriptor/EnumDescriptor
// instances.
void add_ce_obj(const void* ce, zval* value);
zval* get_ce_obj(const void* ce);

extern zend_class_entry* map_field_type;
extern zend_class_entry* repeated_field_type;

// -----------------------------------------------------------------------------
// Descriptor.
// -----------------------------------------------------------------------------

struct DescriptorPool {
  zend_object std;
  upb_symtab* symtab;
  HashTable* pending_list;
};

PHP_METHOD(DescriptorPool, getGeneratedPool);
PHP_METHOD(DescriptorPool, internalAddGeneratedFile);

extern zval* generated_pool_php;  // wrapper of generated pool
extern DescriptorPool* generated_pool;  // The actual generated pool

struct Descriptor {
  zend_object std;
  const upb_msgdef* msgdef;
  MessageLayout* layout;
  zend_class_entry* klass;  // begins as NULL
  const upb_handlers* fill_handlers;
  const upb_pbdecodermethod* fill_method;
  const upb_handlers* pb_serialize_handlers;
};

extern zend_class_entry* descriptor_type;

void descriptor_name_set(Descriptor *desc, const char *name);

struct FieldDescriptor {
  zend_object std;
  const upb_fielddef* fielddef;
};

struct EnumDescriptor {
  zend_object std;
  const upb_enumdef* enumdef;
  zend_class_entry* klass;  // begins as NULL
  // VALUE module;  // begins as nil
};

extern zend_class_entry* enum_descriptor_type;

// -----------------------------------------------------------------------------
// Message class creation.
// -----------------------------------------------------------------------------

void* message_data(void* msg);

// Build PHP class for given descriptor. Instead of building from scratch, this
// function modifies existing class which has been partially defined in PHP
// code.
void build_class_from_descriptor(zval* php_descriptor TSRMLS_DC);

extern zend_object_handlers* message_handlers;

// -----------------------------------------------------------------------------
// Message layout / storage.
// -----------------------------------------------------------------------------

/*
 * In c extension, each protobuf message is a zval instance. The zval instance
 * is like union, which can be used to store int, string, zend_object_value and
 * etc. For protobuf message, the zval instance is used to store the
 * zend_object_value.
 *
 * The zend_object_value is composed of handlers and a handle to look up the
 * actual stored data. The handlers are pointers to functions, e.g., read,
 * write, and etc, to access properties.
 *
 * The actual data of protobuf messages is stored as MessageHeader in zend
 * engine's central repository. Each MessageHeader instance is composed of a
 * zend_object, a Descriptor instance and the real message data.
 *
 * For the reason that PHP's native types may not be large enough to store
 * protobuf message's field (e.g., int64), all message's data is stored in
 * custom memory layout and is indexed by the Descriptor instance.
 *
 * The zend_object contains the zend class entry and the properties table. The
 * zend class entry contains all information about protobuf message's
 * corresponding PHP class. The most useful information is the offset table of
 * properties. Because read access to properties requires returning zval
 * instance, we need to convert data from the custom layout to zval instance.
 * Instead of creating zval instance for every read access, we use the zval
 * instances in the properties table in the zend_object as cache.  When
 * accessing properties, the offset is needed to find the zval property in
 * zend_object's properties table. These properties will be updated using the
 * data from custom memory layout only when reading these properties.
 *
 * zval
 * |-zend_object_value obj
 *   |-zend_object_handlers* handlers -> |-read_property_handler
 *   |                                   |-write_property_handler
 *   |                              ++++++++++++++++++++++
 *   |-zend_object_handle handle -> + central repository +
 *                                  ++++++++++++++++++++++
 *  MessageHeader <-----------------|
 *  |-zend_object std
 *  | |-class_entry* ce -> class_entry
 *  | |                    |-HashTable properties_table (name->offset)
 *  | |-zval** properties_table <------------------------------|
 *  |                         |------> zval* property(cache)
 *  |-Descriptor* desc (name->offset)
 *  |-void** data <-----------|
 *           |-----------------------> void* property(data)
 *
 */

#define MESSAGE_FIELD_NO_CASE ((size_t)-1)

struct MessageField {
  size_t offset;
  int cache_index;  // Each field except oneof field has a zval cache to avoid
                    // multiple creation when being accessed.
  size_t case_offset;   // for oneofs, a uint32. Else, MESSAGE_FIELD_NO_CASE.
};

struct MessageLayout {
  const upb_msgdef* msgdef;
  MessageField* fields;
  size_t size;
};

struct MessageHeader {
  zend_object std;  // Stores properties table and class info of PHP instance.
                    // This is needed for MessageHeader to be accessed via PHP.
  Descriptor* descriptor;  // Kept alive by self.class.descriptor reference.
  // The real message data is appended after MessageHeader.
};

MessageLayout* create_layout(const upb_msgdef* msgdef);
void layout_init(MessageLayout* layout, void* storage, zval** properties_table);
zval* layout_get(MessageLayout* layout, const void* storage,
                 const upb_fielddef* field, zval** cache TSRMLS_DC);
void layout_set(MessageLayout* layout, MessageHeader* header,
                const upb_fielddef* field, zval* val);
void free_layout(MessageLayout* layout);

PHP_METHOD(Message, readOneof);
PHP_METHOD(Message, writeOneof);

// -----------------------------------------------------------------------------
// Encode / Decode.
// -----------------------------------------------------------------------------

// Maximum depth allowed during encoding, to avoid stack overflows due to
// cycles.
#define ENCODE_MAX_NESTING 63

// Constructs the upb decoder method for parsing messages of this type.
// This is called from the message class creation code.
const upb_pbdecodermethod *new_fillmsg_decodermethod(Descriptor *desc,
                                                     const void *owner);

PHP_METHOD(Message, encode);
PHP_METHOD(Message, decode);

// -----------------------------------------------------------------------------
// Type check / conversion.
// -----------------------------------------------------------------------------

bool protobuf_convert_to_int32(zval* from, int32_t* to);
bool protobuf_convert_to_uint32(zval* from, uint32_t* to);
bool protobuf_convert_to_int64(zval* from, int64_t* to);
bool protobuf_convert_to_uint64(zval* from, uint64_t* to);
bool protobuf_convert_to_float(zval* from, float* to);
bool protobuf_convert_to_double(zval* from, double* to);
bool protobuf_convert_to_bool(zval* from, int8_t* to);
bool protobuf_convert_to_string(zval* from);

PHP_METHOD(Util, checkInt32);
PHP_METHOD(Util, checkUint32);
PHP_METHOD(Util, checkInt64);
PHP_METHOD(Util, checkUint64);
PHP_METHOD(Util, checkEnum);
PHP_METHOD(Util, checkFloat);
PHP_METHOD(Util, checkDouble);
PHP_METHOD(Util, checkBool);
PHP_METHOD(Util, checkString);
PHP_METHOD(Util, checkBytes);
PHP_METHOD(Util, checkMessage);
PHP_METHOD(Util, checkRepeatedField);

// -----------------------------------------------------------------------------
// Native slot storage abstraction.
// -----------------------------------------------------------------------------

#define NATIVE_SLOT_MAX_SIZE sizeof(uint64_t)

size_t native_slot_size(upb_fieldtype_t type);
bool native_slot_set(upb_fieldtype_t type, const zend_class_entry* klass,
                     void* memory, zval* value);
void native_slot_init(upb_fieldtype_t type, void* memory, zval** cache);
// For each property, in order to avoid conversion between the zval object and
// the actual data type during parsing/serialization, the containing message
// object use the custom memory layout to store the actual data type for each
// property inside of it.  To access a property from php code, the property
// needs to be converted to a zval object. The message object is not responsible
// for providing such a zval object. Instead the caller needs to provide one
// (cache) and update it with the actual data (memory).
void native_slot_get(upb_fieldtype_t type, const void* memory,
                     zval** cache TSRMLS_DC);
void native_slot_get_default(upb_fieldtype_t type, zval** cache TSRMLS_DC);

// -----------------------------------------------------------------------------
// Map Field.
// -----------------------------------------------------------------------------

extern zend_object_handlers* map_field_handlers;

typedef struct {
  zend_object std;
  upb_fieldtype_t key_type;
  upb_fieldtype_t value_type;
  const zend_class_entry* msg_ce;  // class entry for value message
  upb_strtable table;
} Map;

typedef struct {
  Map* self;
  upb_strtable_iter it;
} MapIter;

void map_begin(zval* self, MapIter* iter);
void map_next(MapIter* iter);
bool map_done(MapIter* iter);
const char* map_iter_key(MapIter* iter, int* len);
upb_value map_iter_value(MapIter* iter, int* len);

// These operate on a map-entry msgdef.
const upb_fielddef* map_entry_key(const upb_msgdef* msgdef);
const upb_fielddef* map_entry_value(const upb_msgdef* msgdef);

zend_object_value map_field_create(zend_class_entry *ce TSRMLS_DC);
void map_field_create_with_type(zend_class_entry *ce, const upb_fielddef *field,
                                zval **map_field TSRMLS_DC);
void map_field_free(void* object TSRMLS_DC);
void* upb_value_memory(upb_value* v);

#define MAP_KEY_FIELD 1
#define MAP_VALUE_FIELD 2

// These operate on a map field (i.e., a repeated field of submessages whose
// submessage type is a map-entry msgdef).
const upb_fielddef* map_field_key(const upb_fielddef* field);
const upb_fielddef* map_field_value(const upb_fielddef* field);

bool map_index_set(Map *intern, const char* keyval, int length, upb_value v);

PHP_METHOD(MapField, __construct);
PHP_METHOD(MapField, offsetExists);
PHP_METHOD(MapField, offsetGet);
PHP_METHOD(MapField, offsetSet);
PHP_METHOD(MapField, offsetUnset);
PHP_METHOD(MapField, count);

// -----------------------------------------------------------------------------
// Repeated Field.
// -----------------------------------------------------------------------------

extern zend_object_handlers* repeated_field_handlers;

struct RepeatedField {
  zend_object std;
  zval* array;
  upb_fieldtype_t type;
  const zend_class_entry* msg_ce;  // class entry for containing message
                                   // (for message field only).
};

void repeated_field_create_with_type(zend_class_entry* ce,
                                     const upb_fielddef* field,
                                     zval** repeated_field TSRMLS_DC);
// Return the element at the index position from the repeated field. There is
// not restriction on the type of stored elements.
void *repeated_field_index_native(RepeatedField *intern, int index);
// Add the element to the end of the repeated field. There is not restriction on
// the type of stored elements.
void repeated_field_push_native(RepeatedField *intern, void *value);

PHP_METHOD(RepeatedField, __construct);
PHP_METHOD(RepeatedField, append);
PHP_METHOD(RepeatedField, offsetExists);
PHP_METHOD(RepeatedField, offsetGet);
PHP_METHOD(RepeatedField, offsetSet);
PHP_METHOD(RepeatedField, offsetUnset);
PHP_METHOD(RepeatedField, count);

// -----------------------------------------------------------------------------
// Oneof Field.
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  upb_oneofdef* oneofdef;
  int index;    // Index of field in oneof. -1 if not set.
  char value[NATIVE_SLOT_MAX_SIZE];
} Oneof;

// Oneof case slot value to indicate that no oneof case is set. The value `0` is
// safe because field numbers are used as case identifiers, and no field can
// have a number of 0.
#define ONEOF_CASE_NONE 0

// -----------------------------------------------------------------------------
// Upb.
// -----------------------------------------------------------------------------

upb_fieldtype_t to_fieldtype(upb_descriptortype_t type);
const zend_class_entry *field_type_class(const upb_fielddef *field);

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

// PHP <-> C conversion.
#define UNBOX(class_name, val) \
  (class_name*)zend_object_store_get_object(val TSRMLS_CC);

#define BOX(class_name, wrapper, intern, free_func)                    \
  MAKE_STD_ZVAL(wrapper);                                              \
  Z_TYPE_P(wrapper) = IS_OBJECT;                                       \
  Z_OBJVAL_P(wrapper)                                                  \
      .handle =                                                        \
      zend_objects_store_put(intern, NULL, free_func, NULL TSRMLS_CC); \
  Z_OBJVAL_P(wrapper).handlers = zend_get_std_object_handlers();

// Memory management
#define ALLOC(class_name) (class_name*) emalloc(sizeof(class_name))
#define PEMALLOC(class_name) (class_name*) pemalloc(sizeof(class_name), 1)
#define ALLOC_N(class_name, n) (class_name*) emalloc(sizeof(class_name) * n)
#define FREE(object) efree(object)
#define PEFREE(object) pefree(object, 1)

// Create PHP internal instance.
#define CREATE(class_name, intern, init_func) \
  intern = ALLOC(class_name);                 \
  memset(intern, 0, sizeof(class_name));      \
  init_func(intern TSRMLS_CC);

// String argument.
#define STR(str) (str), strlen(str)

// Zend Value
#define Z_OBJ_P(zval_p)                                       \
  ((zend_object*)(EG(objects_store)                           \
                      .object_buckets[Z_OBJ_HANDLE_P(zval_p)] \
                      .bucket.obj.object))

#endif  // __GOOGLE_PROTOBUF_PHP_PROTOBUF_H__
