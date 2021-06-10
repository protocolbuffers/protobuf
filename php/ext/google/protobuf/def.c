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

#include "def.h"

#include <php.h>

// This is not self-contained: it must be after other Zend includes.
#include <Zend/zend_exceptions.h>

#include "names.h"
#include "php-upb.h"
#include "protobuf.h"

static void CheckUpbStatus(const upb_status* status, const char* msg) {
  if (!upb_ok(status)) {
    zend_error(E_ERROR, "%s: %s\n", msg, upb_status_errmsg(status));
  }
}

static void FieldDescriptor_FromFieldDef(zval *val, const upb_fielddef *f);

// We use this for objects that should not be created directly from PHP.
static zend_object *CreateHandler_ReturnNull(zend_class_entry *class_type) {
  return NULL;  // Nobody should call this.
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_getByIndex, 0, 0, 1)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

// -----------------------------------------------------------------------------
// EnumValueDescriptor
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  const char *name;
  int32_t number;
} EnumValueDescriptor;

zend_class_entry *EnumValueDescriptor_class_entry;
static zend_object_handlers EnumValueDescriptor_object_handlers;

/*
 * EnumValueDescriptor_Make()
 *
 * Function to create an EnumValueDescriptor object from C.
 */
static void EnumValueDescriptor_Make(zval *val, const char *name,
                                     int32_t number) {
  EnumValueDescriptor *intern = emalloc(sizeof(EnumValueDescriptor));
  zend_object_std_init(&intern->std, EnumValueDescriptor_class_entry);
  intern->std.handlers = &EnumValueDescriptor_object_handlers;
  intern->name = name;
  intern->number = number;
  // Skip object_properties_init(), we don't allow derived classes.
  ZVAL_OBJ(val, &intern->std);
}

/*
 * EnumValueDescriptor::getName()
 *
 * Returns the name for this enum value.
 */
PHP_METHOD(EnumValueDescriptor, getName) {
  EnumValueDescriptor *intern = (EnumValueDescriptor*)Z_OBJ_P(getThis());
  RETURN_STRING(intern->name);
}

/*
 * EnumValueDescriptor::getNumber()
 *
 * Returns the number for this enum value.
 */
PHP_METHOD(EnumValueDescriptor, getNumber) {
  EnumValueDescriptor *intern = (EnumValueDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(intern->number);
}

static zend_function_entry EnumValueDescriptor_methods[] = {
  PHP_ME(EnumValueDescriptor, getName, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValueDescriptor, getNumber, arginfo_void, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// EnumDescriptor
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  const upb_enumdef *enumdef;
  void *cache_key;
} EnumDescriptor;

zend_class_entry *EnumDescriptor_class_entry;
static zend_object_handlers EnumDescriptor_object_handlers;

static void EnumDescriptor_destructor(zend_object* obj) {
  EnumDescriptor *intern = (EnumDescriptor*)obj;
  ObjCache_Delete(intern->cache_key);
}

// Caller owns a ref on the returned zval.
static void EnumDescriptor_FromClassEntry(zval *val, zend_class_entry *ce) {
  // To differentiate enums from classes, we pointer-tag the class entry.
  void* key = (void*)((uintptr_t)ce | 1);
  PBPHP_ASSERT(key != ce);

  if (ce == NULL) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(key, val)) {
    const upb_enumdef *e = NameMap_GetEnum(ce);
    if (!e) {
      ZVAL_NULL(val);
      return;
    }
    EnumDescriptor* ret = emalloc(sizeof(EnumDescriptor));
    zend_object_std_init(&ret->std, EnumDescriptor_class_entry);
    ret->std.handlers = &EnumDescriptor_object_handlers;
    ret->enumdef = e;
    ret->cache_key = key;
    ObjCache_Add(key, &ret->std);
    ZVAL_OBJ(val, &ret->std);
  }
}

// Caller owns a ref on the returned zval.
static void EnumDescriptor_FromEnumDef(zval *val, const upb_enumdef *m) {
  if (!m) {
    ZVAL_NULL(val);
  } else {
    char *classname =
        GetPhpClassname(upb_enumdef_file(m), upb_enumdef_fullname(m));
    zend_string *str = zend_string_init(classname, strlen(classname), 0);
    zend_class_entry *ce = zend_lookup_class(str);  // May autoload the class.

    zend_string_release (str);

    if (!ce) {
      zend_error(E_ERROR, "Couldn't load generated class %s", classname);
    }

    free(classname);
    EnumDescriptor_FromClassEntry(val, ce);
  }
}

/*
 * EnumDescriptor::getValue()
 *
 * Returns an EnumValueDescriptor for this index. Note: we are not looking
 * up by numeric enum value, but by the index in the list of enum values.
 */
PHP_METHOD(EnumDescriptor, getValue) {
  EnumDescriptor *intern = (EnumDescriptor*)Z_OBJ_P(getThis());
  zend_long index;
  zval ret;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) == FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  int field_num = upb_enumdef_numvals(intern->enumdef);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_enum_iter iter;
  int i;
  for(upb_enum_begin(&iter, intern->enumdef), i = 0;
      !upb_enum_done(&iter) && i < index;
      upb_enum_next(&iter), i++);

  EnumValueDescriptor_Make(&ret, upb_enum_iter_name(&iter),
                           upb_enum_iter_number(&iter));
  RETURN_COPY_VALUE(&ret);
}

/*
 * EnumDescriptor::getValueCount()
 *
 * Returns the number of values in this enum.
 */
PHP_METHOD(EnumDescriptor, getValueCount) {
  EnumDescriptor *intern = (EnumDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_enumdef_numvals(intern->enumdef));
}

/*
 * EnumDescriptor::getPublicDescriptor()
 *
 * Returns this EnumDescriptor. Unlike the pure-PHP descriptor, we do not
 * have two separate EnumDescriptor classes. We use a single class for both
 * the public and private descriptor.
 */
PHP_METHOD(EnumDescriptor, getPublicDescriptor) {
  RETURN_COPY(getThis());
}

static zend_function_entry EnumDescriptor_methods[] = {
  PHP_ME(EnumDescriptor, getPublicDescriptor, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(EnumDescriptor, getValueCount, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(EnumDescriptor, getValue, arginfo_getByIndex, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// Oneof
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  const upb_oneofdef *oneofdef;
} OneofDescriptor;

zend_class_entry *OneofDescriptor_class_entry;
static zend_object_handlers OneofDescriptor_object_handlers;

static void OneofDescriptor_destructor(zend_object* obj) {
  OneofDescriptor *intern = (OneofDescriptor*)obj;
  ObjCache_Delete(intern->oneofdef);
}

static void OneofDescriptor_FromOneofDef(zval *val, const upb_oneofdef *o) {
  if (o == NULL) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(o, val)) {
    OneofDescriptor* ret = emalloc(sizeof(OneofDescriptor));
    zend_object_std_init(&ret->std, OneofDescriptor_class_entry);
    ret->std.handlers = &OneofDescriptor_object_handlers;
    ret->oneofdef = o;
    ObjCache_Add(o, &ret->std);
    ZVAL_OBJ(val, &ret->std);
  }
}

/*
 * OneofDescriptor::getName()
 *
 * Returns the name of this oneof.
 */
PHP_METHOD(OneofDescriptor, getName) {
  OneofDescriptor *intern = (OneofDescriptor*)Z_OBJ_P(getThis());
  RETURN_STRING(upb_oneofdef_name(intern->oneofdef));
}

/*
 * OneofDescriptor::getField()
 *
 * Returns a field from this oneof. The given index must be in the range
 *   [0, getFieldCount() - 1].
 */
PHP_METHOD(OneofDescriptor, getField) {
  OneofDescriptor *intern = (OneofDescriptor*)Z_OBJ_P(getThis());
  zend_long index;
  zval ret;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) == FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  int field_num = upb_oneofdef_numfields(intern->oneofdef);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_oneof_iter iter;
  int i;
  for(upb_oneof_begin(&iter, intern->oneofdef), i = 0;
      !upb_oneof_done(&iter) && i < index;
      upb_oneof_next(&iter), i++);
  const upb_fielddef *field = upb_oneof_iter_field(&iter);

  FieldDescriptor_FromFieldDef(&ret, field);
  RETURN_COPY_VALUE(&ret);
}

/*
 * OneofDescriptor::getFieldCount()
 *
 * Returns the number of fields in this oneof.
 */
PHP_METHOD(OneofDescriptor, getFieldCount) {
  OneofDescriptor *intern = (OneofDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_oneofdef_numfields(intern->oneofdef));
}

static zend_function_entry OneofDescriptor_methods[] = {
  PHP_ME(OneofDescriptor, getName,  arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(OneofDescriptor, getField, arginfo_getByIndex, ZEND_ACC_PUBLIC)
  PHP_ME(OneofDescriptor, getFieldCount, arginfo_void, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// FieldDescriptor
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  const upb_fielddef *fielddef;
} FieldDescriptor;

zend_class_entry *FieldDescriptor_class_entry;
static zend_object_handlers FieldDescriptor_object_handlers;

static void FieldDescriptor_destructor(zend_object* obj) {
  FieldDescriptor *intern = (FieldDescriptor*)obj;
  ObjCache_Delete(intern->fielddef);
}

// Caller owns a ref on the returned zval.
static void FieldDescriptor_FromFieldDef(zval *val, const upb_fielddef *f) {
  if (f == NULL) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(f, val)) {
    FieldDescriptor* ret = emalloc(sizeof(FieldDescriptor));
    zend_object_std_init(&ret->std, FieldDescriptor_class_entry);
    ret->std.handlers = &FieldDescriptor_object_handlers;
    ret->fielddef = f;
    ObjCache_Add(f, &ret->std);
    ZVAL_OBJ(val, &ret->std);
  }
}

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

/*
 * FieldDescriptor::getName()
 *
 * Returns the name of this field.
 */
PHP_METHOD(FieldDescriptor, getName) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_STRING(upb_fielddef_name(intern->fielddef));
}

/*
 * FieldDescriptor::getNumber()
 *
 * Returns the number of this field.
 */
PHP_METHOD(FieldDescriptor, getNumber) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_fielddef_number(intern->fielddef));
}

/*
 * FieldDescriptor::getLabel()
 *
 * Returns the label of this field as an integer.
 */
PHP_METHOD(FieldDescriptor, getLabel) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_fielddef_label(intern->fielddef));
}

/*
 * FieldDescriptor::getType()
 *
 * Returns the type of this field as an integer.
 */
PHP_METHOD(FieldDescriptor, getType) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_fielddef_descriptortype(intern->fielddef));
}

/*
 * FieldDescriptor::isMap()
 *
 * Returns true if this field is a map.
 */
PHP_METHOD(FieldDescriptor, isMap) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_BOOL(upb_fielddef_ismap(intern->fielddef));
}

/*
 * FieldDescriptor::getEnumType()
 *
 * Returns the EnumDescriptor for this field, which must be an enum.
 */
PHP_METHOD(FieldDescriptor, getEnumType) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  const upb_enumdef *e = upb_fielddef_enumsubdef(intern->fielddef);
  zval ret;

  if (!e) {
    zend_throw_exception_ex(NULL, 0,
                            "Cannot get enum type for non-enum field '%s'",
                            upb_fielddef_name(intern->fielddef));
    return;
  }

  EnumDescriptor_FromEnumDef(&ret, e);
  RETURN_COPY_VALUE(&ret);
}

/*
 * FieldDescriptor::getMessageType()
 *
 * Returns the Descriptor for this field, which must be a message.
 */
PHP_METHOD(FieldDescriptor, getMessageType) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  Descriptor* desc = Descriptor_GetFromFieldDef(intern->fielddef);

  if (!desc) {
    zend_throw_exception_ex(
        NULL, 0, "Cannot get message type for non-message field '%s'",
        upb_fielddef_name(intern->fielddef));
    return;
  }

  RETURN_OBJ_COPY(&desc->std);
}

static zend_function_entry FieldDescriptor_methods[] = {
  PHP_ME(FieldDescriptor, getName,   arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getNumber, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getLabel,  arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getType,   arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, isMap,     arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getEnumType, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getMessageType, arginfo_void, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// Descriptor
// -----------------------------------------------------------------------------

zend_class_entry *Descriptor_class_entry;
static zend_object_handlers Descriptor_object_handlers;

static void Descriptor_destructor(zend_object* obj) {
  // We don't really need to do anything here, we don't allow this to be
  // collected before the end of the request.
}

static zend_class_entry *Descriptor_GetGeneratedClass(const upb_msgdef *m) {
  char *classname =
      GetPhpClassname(upb_msgdef_file(m), upb_msgdef_fullname(m));
  zend_string *str = zend_string_init(classname, strlen(classname), 0);
  zend_class_entry *ce = zend_lookup_class(str);  // May autoload the class.

  zend_string_release (str);

  if (!ce) {
    zend_error(E_ERROR, "Couldn't load generated class %s", classname);
  }

  free(classname);
  return ce;
}

void Descriptor_FromMessageDef(zval *val, const upb_msgdef *m) {
  if (m == NULL) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(m, val)) {
    zend_class_entry *ce = NULL;
    if (!upb_msgdef_mapentry(m)) {  // Map entries don't have a class.
      ce = Descriptor_GetGeneratedClass(m);
      if (!ce) {
        ZVAL_NULL(val);
        return;
      }
    }
    Descriptor* ret = emalloc(sizeof(Descriptor));
    zend_object_std_init(&ret->std, Descriptor_class_entry);
    ret->std.handlers = &Descriptor_object_handlers;
    ret->class_entry = ce;
    ret->msgdef = m;
    ObjCache_Add(m, &ret->std);
    Descriptors_Add(&ret->std);
    ZVAL_OBJ(val, &ret->std);
  }
}

static void Descriptor_FromClassEntry(zval *val, zend_class_entry *ce) {
  if (ce) {
    Descriptor_FromMessageDef(val, NameMap_GetMessage(ce));
  } else {
    ZVAL_NULL(val);
  }
}

static Descriptor* Descriptor_GetFromZval(zval *val) {
  if (Z_TYPE_P(val) == IS_NULL) {
    return NULL;
  } else {
    zend_object* ret = Z_OBJ_P(val);
    zval_ptr_dtor(val);
    return (Descriptor*)ret;
  }
}

// C Functions from def.h //////////////////////////////////////////////////////

// These are documented in the header file.

Descriptor* Descriptor_GetFromClassEntry(zend_class_entry *ce) {
  zval desc;
  Descriptor_FromClassEntry(&desc, ce);
  return Descriptor_GetFromZval(&desc);
}

Descriptor* Descriptor_GetFromMessageDef(const upb_msgdef *m) {
  zval desc;
  Descriptor_FromMessageDef(&desc, m);
  return Descriptor_GetFromZval(&desc);
}

Descriptor* Descriptor_GetFromFieldDef(const upb_fielddef *f) {
  return Descriptor_GetFromMessageDef(upb_fielddef_msgsubdef(f));
}

/*
 * Descriptor::getPublicDescriptor()
 *
 * Returns this EnumDescriptor. Unlike the pure-PHP descriptor, we do not
 * have two separate EnumDescriptor classes. We use a single class for both
 * the public and private descriptor.
 */
PHP_METHOD(Descriptor, getPublicDescriptor) {
  RETURN_COPY(getThis());
}

/*
 * Descriptor::getFullName()
 *
 * Returns the full name for this message type.
 */
PHP_METHOD(Descriptor, getFullName) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  RETURN_STRING(upb_msgdef_fullname(intern->msgdef));
}

/*
 * Descriptor::getField()
 *
 * Returns a FieldDescriptor for the given index, which must be in the range
 * [0, getFieldCount()-1].
 */
PHP_METHOD(Descriptor, getField) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  int count = upb_msgdef_numfields(intern->msgdef);
  zval ret;
  zend_long index;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) == FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  if (index < 0 || index >= count) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  FieldDescriptor_FromFieldDef(&ret, upb_msgdef_field(intern->msgdef, index));
  RETURN_COPY_VALUE(&ret);
}

/*
 * Descriptor::getFieldCount()
 *
 * Returns the number of fields in this message.
 */
PHP_METHOD(Descriptor, getFieldCount) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_msgdef_numfields(intern->msgdef));
}

/*
 * Descriptor::getOneofDecl()
 *
 * Returns a OneofDescriptor for the given index, which must be in the range
 * [0, getOneofDeclCount()].
 */
PHP_METHOD(Descriptor, getOneofDecl) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  zend_long index;
  zval ret;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) == FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  int field_num = upb_msgdef_numoneofs(intern->msgdef);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_msg_oneof_iter iter;
  int i;
  for(upb_msg_oneof_begin(&iter, intern->msgdef), i = 0;
      !upb_msg_oneof_done(&iter) && i < index;
      upb_msg_oneof_next(&iter), i++);
  const upb_oneofdef *oneof = upb_msg_iter_oneof(&iter);

  OneofDescriptor_FromOneofDef(&ret, oneof);
  RETURN_COPY_VALUE(&ret);
}

/*
 * Descriptor::getOneofDeclCount()
 *
 * Returns the number of oneofs in this message.
 */
PHP_METHOD(Descriptor, getOneofDeclCount) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_msgdef_numoneofs(intern->msgdef));
}

/*
 * Descriptor::getClass()
 *
 * Returns the name of the PHP class for this message.
 */
PHP_METHOD(Descriptor, getClass) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  const char* classname = ZSTR_VAL(intern->class_entry->name);
  RETURN_STRING(classname);
}


static zend_function_entry Descriptor_methods[] = {
  PHP_ME(Descriptor, getClass, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getFullName, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getField, arginfo_getByIndex, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getFieldCount, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getOneofDecl, arginfo_getByIndex, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getOneofDeclCount, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getPublicDescriptor, arginfo_void, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// DescriptorPool
// -----------------------------------------------------------------------------

typedef struct DescriptorPool {
  zend_object std;
  upb_symtab *symtab;
} DescriptorPool;

zend_class_entry *DescriptorPool_class_entry;
static zend_object_handlers DescriptorPool_object_handlers;

static DescriptorPool *GetPool(const zval* this_ptr) {
  return (DescriptorPool*)Z_OBJ_P(this_ptr);
}

/**
 * Object handler to create an DescriptorPool.
 */
static zend_object* DescriptorPool_create(zend_class_entry *class_type) {
  DescriptorPool *intern = emalloc(sizeof(DescriptorPool));
  zend_object_std_init(&intern->std, class_type);
  intern->std.handlers = &DescriptorPool_object_handlers;
  intern->symtab = upb_symtab_new();
  // Skip object_properties_init(), we don't allow derived classes.
  return &intern->std;
}

/**
 * Object handler to free an DescriptorPool.
 */
static void DescriptorPool_destructor(zend_object* obj) {
  DescriptorPool* intern = (DescriptorPool*)obj;
  if (intern->symtab) {
    upb_symtab_free(intern->symtab);
  }
  intern->symtab = NULL;
  zend_object_std_dtor(&intern->std);
}

void DescriptorPool_CreateWithSymbolTable(zval *zv, upb_symtab *symtab) {
  ZVAL_OBJ(zv, DescriptorPool_create(DescriptorPool_class_entry));

  if (symtab) {
    DescriptorPool *intern = GetPool(zv);
    upb_symtab_free(intern->symtab);
    intern->symtab = symtab;
  }
}

upb_symtab *DescriptorPool_Steal(zval *zv) {
  DescriptorPool *intern = GetPool(zv);
  upb_symtab *ret = intern->symtab;
  intern->symtab = NULL;
  return ret;
}

upb_symtab *DescriptorPool_GetSymbolTable() {
  DescriptorPool *intern = GetPool(get_generated_pool());
  return intern->symtab;
}


/*
 * DescriptorPool::getGeneratedPool()
 *
 * Returns the generated DescriptorPool.
 */
PHP_METHOD(DescriptorPool, getGeneratedPool) {
  zval ret;
  ZVAL_COPY(&ret, get_generated_pool());
  RETURN_COPY_VALUE(&ret);
}

/*
 * DescriptorPool::getDescriptorByClassName()
 *
 * Returns a Descriptor object for the given PHP class name.
 */
PHP_METHOD(DescriptorPool, getDescriptorByClassName) {
  char *classname = NULL;
  zend_long classname_len;
  zend_class_entry *ce;
  zend_string *str;
  zval ret;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &classname, &classname_len) ==
      FAILURE) {
    return;
  }

  str = zend_string_init(classname, strlen(classname), 0);
  ce = zend_lookup_class(str);  // May autoload the class.
  zend_string_release (str);

  if (!ce) {
    RETURN_NULL();
  }

  Descriptor_FromClassEntry(&ret, ce);
  RETURN_COPY_VALUE(&ret);
}

/*
 * DescriptorPool::getEnumDescriptorByClassName()
 *
 * Returns a EnumDescriptor object for the given PHP class name.
 */
PHP_METHOD(DescriptorPool, getEnumDescriptorByClassName) {
  char *classname = NULL;
  zend_long classname_len;
  zend_class_entry *ce;
  zend_string *str;
  zval ret;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &classname, &classname_len) ==
      FAILURE) {
    return;
  }

  str = zend_string_init(classname, strlen(classname), 0);
  ce = zend_lookup_class(str);  // May autoload the class.
  zend_string_release (str);

  if (!ce) {
    RETURN_NULL();
  }

  EnumDescriptor_FromClassEntry(&ret, ce);
  RETURN_COPY_VALUE(&ret);
}

/*
 * DescriptorPool::getEnumDescriptorByProtoName()
 *
 * Returns a Descriptor object for the given protobuf message name.
 */
PHP_METHOD(DescriptorPool, getDescriptorByProtoName) {
  DescriptorPool *intern = GetPool(getThis());
  char *protoname = NULL;
  zend_long protoname_len;
  const upb_msgdef *m;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &protoname, &protoname_len) ==
      FAILURE) {
    return;
  }

  if (*protoname == '.') protoname++;

  m = upb_symtab_lookupmsg(intern->symtab, protoname);

  if (m) {
    RETURN_OBJ_COPY(&Descriptor_GetFromMessageDef(m)->std);
  } else {
    RETURN_NULL();
  }
}

/*
 * depends_on_descriptor()
 *
 * Returns true if this FileDescriptorProto depends on descriptor.proto.
 */
bool depends_on_descriptor(const google_protobuf_FileDescriptorProto* file) {
  const upb_strview *deps;
  upb_strview name = upb_strview_makez("google/protobuf/descriptor.proto");
  size_t i, n;

  deps = google_protobuf_FileDescriptorProto_dependency(file, &n);
  for (i = 0; i < n; i++) {
    if (upb_strview_eql(deps[i], name)) {
      return true;
    }
  }

  return false;
}

/*
 * add_name_mappings()
 *
 * Adds the messages and enums in this file to the NameMap.
 */
static void add_name_mappings(const upb_filedef *file) {
  size_t i;
  for (i = 0; i < upb_filedef_msgcount(file); i++) {
    NameMap_AddMessage(upb_filedef_msg(file, i));
  }

  for (i = 0; i < upb_filedef_enumcount(file); i++) {
    NameMap_AddEnum(upb_filedef_enum(file, i));
  }
}

static void add_descriptor(DescriptorPool *pool,
                           const google_protobuf_FileDescriptorProto *file) {
  upb_strview name = google_protobuf_FileDescriptorProto_name(file);
  upb_status status;
  const upb_filedef *file_def;
  upb_status_clear(&status);

  if (upb_symtab_lookupfile2(pool->symtab, name.data, name.size)) {
    // Already added.
    // TODO(teboring): Re-enable this warning when aggregate metadata is
    // deprecated.
    // zend_error(E_USER_WARNING,
    //            "proto descriptor was previously loaded (included in multiple "
    //            "metadata bundles?): " UPB_STRVIEW_FORMAT,
    //            UPB_STRVIEW_ARGS(name));
    return;
  }

  // The PHP code generator currently special-cases descriptor.proto.  It
  // doesn't add it as a dependency even if the proto file actually does
  // depend on it.
  if (depends_on_descriptor(file)) {
    google_protobuf_FileDescriptorProto_getmsgdef(pool->symtab);
  }

  file_def = upb_symtab_addfile(pool->symtab, file, &status);
  CheckUpbStatus(&status, "Unable to load descriptor");
  add_name_mappings(file_def);
}

/*
 * add_descriptor()
 *
 * Adds the given descriptor data to this DescriptorPool.
 */
static void add_descriptor_set(DescriptorPool *pool, const char *data,
                               int data_len, upb_arena *arena) {
  size_t i, n;
  google_protobuf_FileDescriptorSet *set;
  const google_protobuf_FileDescriptorProto* const* files;

  set = google_protobuf_FileDescriptorSet_parse(data, data_len, arena);

  if (!set) {
    zend_error(E_ERROR, "Failed to parse binary descriptor\n");
    return;
  }

  files = google_protobuf_FileDescriptorSet_file(set, &n);

  for (i = 0; i < n; i++) {
    const google_protobuf_FileDescriptorProto* file = files[i];
    add_descriptor(pool, file);
  }
}

bool DescriptorPool_HasFile(const char *filename) {
  DescriptorPool *intern = GetPool(get_generated_pool());
  return upb_symtab_lookupfile(intern->symtab, filename) != NULL;
}

void DescriptorPool_AddDescriptor(const char *filename, const char *data,
                                  int size) {
  upb_arena *arena = upb_arena_new();
  const google_protobuf_FileDescriptorProto *file =
      google_protobuf_FileDescriptorProto_parse(data, size, arena);

  if (!file) {
    zend_error(E_ERROR, "Failed to parse binary descriptor for %s\n", filename);
    return;
  }

  add_descriptor(GetPool(get_generated_pool()), file);
  upb_arena_free(arena);
}

/*
 * DescriptorPool::internalAddGeneratedFile()
 *
 * Adds the given descriptor data to this DescriptorPool.
 */
PHP_METHOD(DescriptorPool, internalAddGeneratedFile) {
  DescriptorPool *intern = GetPool(getThis());
  char *data = NULL;
  zend_long data_len;
  zend_bool use_nested_submsg = false;
  upb_arena *arena;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|b", &data, &data_len,
                            &use_nested_submsg) != SUCCESS) {
    return;
  }

  arena = upb_arena_new();
  add_descriptor_set(intern, data, data_len, arena);
  upb_arena_free(arena);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_lookupByName, 0, 0, 1)
  ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_addgeneratedfile, 0, 0, 2)
  ZEND_ARG_INFO(0, data)
  ZEND_ARG_INFO(0, data_len)
ZEND_END_ARG_INFO()

static zend_function_entry DescriptorPool_methods[] = {
  PHP_ME(DescriptorPool, getGeneratedPool, arginfo_void,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(DescriptorPool, getDescriptorByClassName, arginfo_lookupByName, ZEND_ACC_PUBLIC)
  PHP_ME(DescriptorPool, getDescriptorByProtoName, arginfo_lookupByName, ZEND_ACC_PUBLIC)
  PHP_ME(DescriptorPool, getEnumDescriptorByClassName, arginfo_lookupByName, ZEND_ACC_PUBLIC)
  PHP_ME(DescriptorPool, internalAddGeneratedFile, arginfo_addgeneratedfile, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// InternalDescriptorPool
// -----------------------------------------------------------------------------

// For the C extension, Google\Protobuf\Internal\DescriptorPool is not a
// separate instantiable object, it just returns a
// Google\Protobuf\DescriptorPool.

zend_class_entry *InternalDescriptorPool_class_entry;

/*
 * InternalDescriptorPool::getGeneratedPool()
 *
 * Returns the generated DescriptorPool. Note that this is identical to
 * DescriptorPool::getGeneratedPool(), and in fact returns a DescriptorPool
 * instance.
 */
PHP_METHOD(InternalDescriptorPool, getGeneratedPool) {
  RETURN_COPY(get_generated_pool());
}

static zend_function_entry InternalDescriptorPool_methods[] = {
  PHP_ME(InternalDescriptorPool, getGeneratedPool, arginfo_void,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// GPBType
// -----------------------------------------------------------------------------

zend_class_entry* gpb_type_type;

static zend_function_entry gpb_type_methods[] = {
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// Module Init
// -----------------------------------------------------------------------------

void Def_ModuleInit() {
  zend_class_entry tmp_ce;
  zend_object_handlers *h;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\OneofDescriptor",
                   OneofDescriptor_methods);
  OneofDescriptor_class_entry = zend_register_internal_class(&tmp_ce);
  OneofDescriptor_class_entry->ce_flags |= ZEND_ACC_FINAL;
  OneofDescriptor_class_entry->create_object = CreateHandler_ReturnNull;
  h = &OneofDescriptor_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = &OneofDescriptor_destructor;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\EnumValueDescriptor",
                   EnumValueDescriptor_methods);
  EnumValueDescriptor_class_entry = zend_register_internal_class(&tmp_ce);
  EnumValueDescriptor_class_entry->ce_flags |= ZEND_ACC_FINAL;
  EnumValueDescriptor_class_entry->create_object = CreateHandler_ReturnNull;
  h = &EnumValueDescriptor_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\EnumDescriptor",
                   EnumDescriptor_methods);
  EnumDescriptor_class_entry = zend_register_internal_class(&tmp_ce);
  EnumDescriptor_class_entry->ce_flags |= ZEND_ACC_FINAL;
  EnumDescriptor_class_entry->create_object = CreateHandler_ReturnNull;
  h = &EnumDescriptor_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = &EnumDescriptor_destructor;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Descriptor",
                   Descriptor_methods);

  Descriptor_class_entry = zend_register_internal_class(&tmp_ce);
  Descriptor_class_entry->ce_flags |= ZEND_ACC_FINAL;
  Descriptor_class_entry->create_object = CreateHandler_ReturnNull;
  h = &Descriptor_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = Descriptor_destructor;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\FieldDescriptor",
                   FieldDescriptor_methods);
  FieldDescriptor_class_entry = zend_register_internal_class(&tmp_ce);
  FieldDescriptor_class_entry->ce_flags |= ZEND_ACC_FINAL;
  FieldDescriptor_class_entry->create_object = CreateHandler_ReturnNull;
  h = &FieldDescriptor_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = &FieldDescriptor_destructor;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\DescriptorPool",
                   DescriptorPool_methods);
  DescriptorPool_class_entry = zend_register_internal_class(&tmp_ce);
  DescriptorPool_class_entry->ce_flags |= ZEND_ACC_FINAL;
  DescriptorPool_class_entry->create_object = DescriptorPool_create;
  h = &DescriptorPool_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = DescriptorPool_destructor;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Internal\\DescriptorPool",
                   InternalDescriptorPool_methods);
  InternalDescriptorPool_class_entry = zend_register_internal_class(&tmp_ce);

  // GPBType.
#define STR(str) (str), strlen(str)
  zend_class_entry class_type;
  INIT_CLASS_ENTRY(class_type, "Google\\Protobuf\\Internal\\GPBType",
                   gpb_type_methods);
  gpb_type_type = zend_register_internal_class(&class_type);
  zend_declare_class_constant_long(gpb_type_type, STR("DOUBLE"), 1);
  zend_declare_class_constant_long(gpb_type_type, STR("FLOAT"), 2);
  zend_declare_class_constant_long(gpb_type_type, STR("INT64"), 3);
  zend_declare_class_constant_long(gpb_type_type, STR("UINT64"), 4);
  zend_declare_class_constant_long(gpb_type_type, STR("INT32"), 5);
  zend_declare_class_constant_long(gpb_type_type, STR("FIXED64"), 6);
  zend_declare_class_constant_long(gpb_type_type, STR("FIXED32"), 7);
  zend_declare_class_constant_long(gpb_type_type, STR("BOOL"), 8);
  zend_declare_class_constant_long(gpb_type_type, STR("STRING"), 9);
  zend_declare_class_constant_long(gpb_type_type, STR("GROUP"), 10);
  zend_declare_class_constant_long(gpb_type_type, STR("MESSAGE"), 11);
  zend_declare_class_constant_long(gpb_type_type, STR("BYTES"), 12);
  zend_declare_class_constant_long(gpb_type_type, STR("UINT32"), 13);
  zend_declare_class_constant_long(gpb_type_type, STR("ENUM"), 14);
  zend_declare_class_constant_long(gpb_type_type, STR("SFIXED32"), 15);
  zend_declare_class_constant_long(gpb_type_type, STR("SFIXED64"), 16);
  zend_declare_class_constant_long(gpb_type_type, STR("SINT32"), 17);
  zend_declare_class_constant_long(gpb_type_type, STR("SINT64"), 18);
#undef STR
}
