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

// Forward declare.
static zend_object_value descriptor_create(zend_class_entry *ce TSRMLS_DC);
static void descriptor_init_c_instance(Descriptor* intern TSRMLS_DC);
static void descriptor_free_c(Descriptor* object TSRMLS_DC);
static void descriptor_free(void* object TSRMLS_DC);

static zend_object_value enum_descriptor_create(zend_class_entry *ce TSRMLS_DC);
static void enum_descriptor_init_c_instance(EnumDescriptor* intern TSRMLS_DC);
static void enum_descriptor_free_c(EnumDescriptor* object TSRMLS_DC);
static void enum_descriptor_free(void* object TSRMLS_DC);

static zend_object_value descriptor_pool_create(zend_class_entry *ce TSRMLS_DC);
static void descriptor_pool_free_c(DescriptorPool* object TSRMLS_DC);
static void descriptor_pool_free(void* object TSRMLS_DC);
static void descriptor_pool_init_c_instance(DescriptorPool* pool TSRMLS_DC);

// -----------------------------------------------------------------------------
// Common Utilities
// -----------------------------------------------------------------------------

static void check_upb_status(const upb_status* status, const char* msg) {
  if (!upb_ok(status)) {
    zend_error(E_ERROR, "%s: %s\n", msg, upb_status_errmsg(status));
  }
}

static void upb_filedef_free(void *r) {
  upb_filedef *f = *(upb_filedef **)r;
  size_t i;

  for (i = 0; i < upb_filedef_depcount(f); i++) {
    upb_filedef_unref(upb_filedef_dep(f, i), f);
  }

  upb_inttable_uninit(&f->defs);
  upb_inttable_uninit(&f->deps);
  upb_gfree((void *)f->name);
  upb_gfree((void *)f->package);
  upb_gfree(f);
}

// Camel-case the field name and append "Entry" for generated map entry name.
// e.g. map<KeyType, ValueType> foo_map => FooMapEntry
static void append_map_entry_name(char *result, const char *field_name,
                                  int pos) {
  bool cap_next = true;
  int i;

  for (i = 0; i < strlen(field_name); ++i) {
    if (field_name[i] == '_') {
      cap_next = true;
    } else if (cap_next) {
      // Note: Do not use ctype.h due to locales.
      if ('a' <= field_name[i] && field_name[i] <= 'z') {
        result[pos++] = field_name[i] - 'a' + 'A';
      } else {
        result[pos++] = field_name[i];
      }
      cap_next = false;
    } else {
      result[pos++] = field_name[i];
    }
  }
  strcat(result, "Entry");
}

#define CHECK_UPB(code, msg)             \
  do {                                   \
    upb_status status = UPB_STATUS_INIT; \
    code;                                \
    check_upb_status(&status, msg);      \
  } while (0)

// Define PHP class
#define DEFINE_PROTOBUF_INIT_CLASS(name_lower, string_name)                  \
  void name_lower##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    INIT_CLASS_ENTRY(class_type, string_name, name_lower##_methods);         \
    name_lower##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    name_lower##_type->create_object = name_lower##_create;                  \
  }

#define DEFINE_PROTOBUF_CREATE(name, name_lower)                        \
  static zend_object_value name_lower##_create(                         \
      zend_class_entry* ce TSRMLS_DC) {                                 \
    zend_object_value return_value;                                     \
    name* intern = (name*)emalloc(sizeof(name));                        \
    memset(intern, 0, sizeof(name));                                    \
    name_lower##_init_c_instance(intern TSRMLS_CC);                     \
    return_value.handle = zend_objects_store_put(                       \
        intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, \
        name_lower##_free, NULL TSRMLS_CC);                             \
    return_value.handlers = zend_get_std_object_handlers();             \
    return return_value;                                                \
  }

#define DEFINE_PROTOBUF_FREE(name, name_lower)            \
  static void name_lower##_free(void* object TSRMLS_DC) { \
    name* intern = (name*)object;                         \
    name_lower##_free_c(intern TSRMLS_CC);                \
    efree(object);                                        \
  }

#define DEFINE_CLASS(name, name_lower, string_name) \
  zend_class_entry* name_lower##_type;              \
  DEFINE_PROTOBUF_FREE(name, name_lower)            \
  DEFINE_PROTOBUF_CREATE(name, name_lower)          \
  DEFINE_PROTOBUF_INIT_CLASS(name_lower, string_name)

// -----------------------------------------------------------------------------
// GPBType
// -----------------------------------------------------------------------------

zend_class_entry* gpb_type_type;

static zend_function_entry gpb_type_methods[] = {
  ZEND_FE_END
};

void gpb_type_init(TSRMLS_D) {
  zend_class_entry class_type;
  INIT_CLASS_ENTRY(class_type, "Google\\Protobuf\\Internal\\GPBType",
                   gpb_type_methods);
  gpb_type_type = zend_register_internal_class(&class_type TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("DOUBLE"),  1 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("FLOAT"),   2 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("INT64"),   3 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("UINT64"),  4 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("INT32"),   5 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("FIXED64"), 6 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("FIXED32"), 7 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("BOOL"),    8 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("STRING"),  9 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("GROUP"),   10 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("MESSAGE"), 11 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("BYTES"),   12 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("UINT32"),  13 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("ENUM"),    14 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("SFIXED32"),
                                   15 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("SFIXED64"),
                                   16 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("SINT32"), 17 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("SINT64"), 18 TSRMLS_CC);
}

// -----------------------------------------------------------------------------
// DescriptorPool
// -----------------------------------------------------------------------------

static zend_function_entry descriptor_pool_methods[] = {
  PHP_ME(DescriptorPool, getGeneratedPool, NULL,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(DescriptorPool, internalAddGeneratedFile, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

DEFINE_CLASS(DescriptorPool, descriptor_pool,
             "Google\\Protobuf\\Internal\\DescriptorPool");

zval* generated_pool_php;  // wrapper of generated pool
DescriptorPool *generated_pool;  // The actual generated pool

static void init_generated_pool_once(TSRMLS_D) {
  if (generated_pool_php == NULL) {
    MAKE_STD_ZVAL(generated_pool_php);
    Z_TYPE_P(generated_pool_php) = IS_OBJECT;
    generated_pool = ALLOC(DescriptorPool);
    descriptor_pool_init_c_instance(generated_pool TSRMLS_CC);
    Z_OBJ_HANDLE_P(generated_pool_php) = zend_objects_store_put(
        generated_pool, NULL,
        (zend_objects_free_object_storage_t)descriptor_pool_free,
        NULL TSRMLS_CC);
    Z_OBJ_HT_P(generated_pool_php) = zend_get_std_object_handlers();
  }
}

static void descriptor_pool_init_c_instance(DescriptorPool *pool TSRMLS_DC) {
  zend_object_std_init(&pool->std, descriptor_pool_type TSRMLS_CC);
  pool->symtab = upb_symtab_new(&pool->symtab);

  ALLOC_HASHTABLE(pool->pending_list);
  zend_hash_init(pool->pending_list, 1, NULL, ZVAL_PTR_DTOR, 0);
}

static void descriptor_pool_free_c(DescriptorPool *pool TSRMLS_DC) {
  upb_symtab_unref(pool->symtab, &pool->symtab);

  zend_hash_destroy(pool->pending_list);
  FREE_HASHTABLE(pool->pending_list);
}

static void validate_enumdef(const upb_enumdef *enumdef) {
  // Verify that an entry exists with integer value 0. (This is the default
  // value.)
  const char *lookup = upb_enumdef_iton(enumdef, 0);
  if (lookup == NULL) {
    zend_error(E_USER_ERROR,
               "Enum definition does not contain a value for '0'.");
  }
}

static void validate_msgdef(const upb_msgdef* msgdef) {
  // Verify that no required fields exist. proto3 does not support these.
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    if (upb_fielddef_label(field) == UPB_LABEL_REQUIRED) {
      zend_error(E_ERROR, "Required fields are unsupported in proto3.");
    }
  }
}

PHP_METHOD(DescriptorPool, getGeneratedPool) {
  init_generated_pool_once(TSRMLS_C);
  RETURN_ZVAL(generated_pool_php, 1, 0);
}

static void convert_to_class_name_inplace(char *class_name,
                                          const char* fullname,
                                          const char* package_name) {
  size_t i;
  bool first_char = false;
  size_t pkg_name_len = package_name == NULL ? 0 : strlen(package_name);

  // In php, class name cannot be Empty.
  if (strcmp("google.protobuf.Empty", fullname) == 0) {
    fullname = "google.protobuf.GPBEmpty";
  }

  if (pkg_name_len == 0) {
    strcpy(class_name, fullname);
  } else {
    class_name[0] = '.';
    strcpy(&class_name[1], fullname);
    for (i = 0; i <= pkg_name_len + 1; i++) {
      // PHP package uses camel case.
      if (!first_char && class_name[i] != '.') {
        first_char = true;
        class_name[i] += 'A' - 'a';
      }
      // php packages are divided by '\'.
      if (class_name[i] == '.') {
        first_char = false;
        class_name[i] = '\\';
      }
    }
  }

  // Submessage is concatenated with its containing messages by '_'.
  for (i = pkg_name_len; i < strlen(class_name); i++) {
    if (class_name[i] == '.') {
      class_name[i] = '_';
    }
  }
}

PHP_METHOD(DescriptorPool, internalAddGeneratedFile) {
  char *data = NULL;
  int data_len;
  upb_filedef **files;
  size_t i;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) ==
      FAILURE) {
    return;
  }

  DescriptorPool *pool = UNBOX(DescriptorPool, getThis());
  CHECK_UPB(files = upb_loaddescriptor(data, data_len, &pool, &status),
            "Parse binary descriptors to internal descriptors failed");

  // This method is called only once in each file.
  assert(files[0] != NULL);
  assert(files[1] == NULL);

  CHECK_UPB(upb_symtab_addfile(pool->symtab, files[0], &status),
            "Unable to add file to DescriptorPool");

  // For each enum/message, we need its PHP class, upb descriptor and its PHP
  // wrapper. These information are needed later for encoding, decoding and type
  // checking. However, sometimes we just have one of them. In order to find
  // them quickly, here, we store the mapping for them.
  for (i = 0; i < upb_filedef_defcount(files[0]); i++) {
    const upb_def *def = upb_filedef_def(files[0], i);
    switch (upb_def_type(def)) {
#define CASE_TYPE(def_type, def_type_lower, desc_type, desc_type_lower)        \
  case UPB_DEF_##def_type: {                                                   \
    desc_type *desc;                                                           \
    zval *desc_php;                                                            \
    CREATE(desc_type, desc, desc_type_lower##_init_c_instance);                \
    BOX(desc_type, desc_php, desc, desc_type_lower##_free);                    \
    Z_DELREF_P(desc_php);                                                      \
    const upb_##def_type_lower *def_type_lower =                               \
        upb_downcast_##def_type_lower(def);                                    \
    desc->def_type_lower = def_type_lower;                                     \
    add_def_obj(desc->def_type_lower, desc_php);                               \
    /* Unlike other messages, MapEntry is shared by all map fields and doesn't \
     * have generated PHP class.*/                                             \
    if (upb_def_type(def) == UPB_DEF_MSG &&                                    \
        upb_msgdef_mapentry(upb_downcast_msgdef(def))) {                       \
      break;                                                                   \
    }                                                                          \
    /* Prepend '.' to package name to make it absolute. In the 5 additional    \
     * bytes allocated, one for '.', one for trailing 0, and 3 for 'GPB' if    \
     * given message is google.protobuf.Empty.*/                               \
    const char *fullname = upb_##def_type_lower##_fullname(def_type_lower);    \
    char *klass_name = ecalloc(sizeof(char), 5 + strlen(fullname));            \
    convert_to_class_name_inplace(klass_name, fullname,                        \
                                  upb_filedef_package(files[0]));              \
    zend_class_entry **pce;                                                    \
    if (zend_lookup_class(klass_name, strlen(klass_name), &pce TSRMLS_CC) ==   \
        FAILURE) {                                                             \
      zend_error(E_ERROR, "Generated message class %s hasn't been defined",    \
                 klass_name);                                                  \
      return;                                                                  \
    } else {                                                                   \
      desc->klass = *pce;                                                      \
    }                                                                          \
    add_ce_obj(desc->klass, desc_php);                                         \
    efree(klass_name);                                                         \
    break;                                                                     \
  }

      CASE_TYPE(MSG, msgdef, Descriptor, descriptor)
      CASE_TYPE(ENUM, enumdef, EnumDescriptor, enum_descriptor)
#undef CASE_TYPE

      default:
        break;
    }
  }

  for (i = 0; i < upb_filedef_defcount(files[0]); i++) {
    const upb_def *def = upb_filedef_def(files[0], i);
    if (upb_def_type(def) == UPB_DEF_MSG) {
      const upb_msgdef *msgdef = upb_downcast_msgdef(def);
      zval *desc_php = get_def_obj(msgdef);
      build_class_from_descriptor(desc_php TSRMLS_CC);
    }
  }

  upb_filedef_unref(files[0], &pool);
  upb_gfree(files);
}

// -----------------------------------------------------------------------------
// Descriptor
// -----------------------------------------------------------------------------

static zend_function_entry descriptor_methods[] = {
  ZEND_FE_END
};

DEFINE_CLASS(Descriptor, descriptor, "Google\\Protobuf\\Internal\\Descriptor");

static void descriptor_free_c(Descriptor *self TSRMLS_DC) {
  if (self->layout) {
    free_layout(self->layout);
  }
  if (self->fill_handlers) {
    upb_handlers_unref(self->fill_handlers, &self->fill_handlers);
  }
  if (self->fill_method) {
    upb_pbdecodermethod_unref(self->fill_method, &self->fill_method);
  }
  if (self->pb_serialize_handlers) {
    upb_handlers_unref(self->pb_serialize_handlers,
                       &self->pb_serialize_handlers);
  }
}

static void descriptor_init_c_instance(Descriptor *desc TSRMLS_DC) {
  zend_object_std_init(&desc->std, descriptor_type TSRMLS_CC);
  desc->msgdef = NULL;
  desc->layout = NULL;
  desc->klass = NULL;
  desc->fill_handlers = NULL;
  desc->fill_method = NULL;
  desc->pb_serialize_handlers = NULL;
}

// -----------------------------------------------------------------------------
// EnumDescriptor
// -----------------------------------------------------------------------------

static zend_function_entry enum_descriptor_methods[] = {
  ZEND_FE_END
};

DEFINE_CLASS(EnumDescriptor, enum_descriptor,
             "Google\\Protobuf\\Internal\\EnumDescriptor");

static void enum_descriptor_free_c(EnumDescriptor *self TSRMLS_DC) {
}

static void enum_descriptor_init_c_instance(EnumDescriptor *self TSRMLS_DC) {
  zend_object_std_init(&self->std, enum_descriptor_type TSRMLS_CC);
  self->enumdef = NULL;
  self->klass = NULL;
}

// -----------------------------------------------------------------------------
// FieldDescriptor
// -----------------------------------------------------------------------------

upb_fieldtype_t to_fieldtype(upb_descriptortype_t type) {
  switch (type) {
#define CASE(descriptor_type, type)           \
  case UPB_DESCRIPTOR_TYPE_##descriptor_type: \
    return UPB_TYPE_##type;

  CASE(FLOAT,    FLOAT);
  CASE(DOUBLE,   DOUBLE);
  CASE(BOOL,     BOOL);
  CASE(STRING,   STRING);
  CASE(BYTES,    BYTES);
  CASE(MESSAGE,  MESSAGE);
  CASE(GROUP,    MESSAGE);
  CASE(ENUM,     ENUM);
  CASE(INT32,    INT32);
  CASE(INT64,    INT64);
  CASE(UINT32,   UINT32);
  CASE(UINT64,   UINT64);
  CASE(SINT32,   INT32);
  CASE(SINT64,   INT64);
  CASE(FIXED32,  UINT32);
  CASE(FIXED64,  UINT64);
  CASE(SFIXED32, INT32);
  CASE(SFIXED64, INT64);

#undef CONVERT

  }

  zend_error(E_ERROR, "Unknown field type.");
  return 0;
}
