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

static void CheckUpbStatus(const upb_Status* status, const char* msg) {
  if (!upb_Status_IsOk(status)) {
    zend_error(E_ERROR, "%s: %s\n", msg, upb_Status_ErrorMessage(status));
  }
}

static void FieldDescriptor_FromFieldDef(zval *val, const upb_FieldDef *f);

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
  const upb_EnumDef *enumdef;
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
    const upb_EnumDef *e = NameMap_GetEnum(ce);
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
static void EnumDescriptor_FromEnumDef(zval *val, const upb_EnumDef *m) {
  if (!m) {
    ZVAL_NULL(val);
  } else {
    char *classname =
        GetPhpClassname(upb_EnumDef_File(m), upb_EnumDef_FullName(m), false);
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

  if (index < 0 || index >= upb_EnumDef_ValueCount(intern->enumdef)) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  const upb_EnumValueDef* ev = upb_EnumDef_Value(intern->enumdef, index);
  EnumValueDescriptor_Make(&ret, upb_EnumValueDef_Name(ev),
                           upb_EnumValueDef_Number(ev));
  RETURN_COPY_VALUE(&ret);
}

/*
 * EnumDescriptor::getValueCount()
 *
 * Returns the number of values in this enum.
 */
PHP_METHOD(EnumDescriptor, getValueCount) {
  EnumDescriptor *intern = (EnumDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_EnumDef_ValueCount(intern->enumdef));
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
  const upb_OneofDef *oneofdef;
} OneofDescriptor;

zend_class_entry *OneofDescriptor_class_entry;
static zend_object_handlers OneofDescriptor_object_handlers;

static void OneofDescriptor_destructor(zend_object* obj) {
  OneofDescriptor *intern = (OneofDescriptor*)obj;
  ObjCache_Delete(intern->oneofdef);
}

static void OneofDescriptor_FromOneofDef(zval *val, const upb_OneofDef *o) {
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
  RETURN_STRING(upb_OneofDef_Name(intern->oneofdef));
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

  if (index < 0 || index >= upb_OneofDef_FieldCount(intern->oneofdef)) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  const upb_FieldDef* field = upb_OneofDef_Field(intern->oneofdef, index);
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
  RETURN_LONG(upb_OneofDef_FieldCount(intern->oneofdef));
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
  const upb_FieldDef *fielddef;
} FieldDescriptor;

zend_class_entry *FieldDescriptor_class_entry;
static zend_object_handlers FieldDescriptor_object_handlers;

static void FieldDescriptor_destructor(zend_object* obj) {
  FieldDescriptor *intern = (FieldDescriptor*)obj;
  ObjCache_Delete(intern->fielddef);
}

// Caller owns a ref on the returned zval.
static void FieldDescriptor_FromFieldDef(zval *val, const upb_FieldDef *f) {
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

upb_CType to_fieldtype(upb_FieldType type) {
  switch (type) {
#define CASE(descriptor_type, type)           \
  case kUpb_FieldType_##descriptor_type: \
    return kUpb_CType_##type;

  CASE(Float,    Float);
  CASE(Double,   Double);
  CASE(Bool,     Bool);
  CASE(String,   String);
  CASE(Bytes,    Bytes);
  CASE(Message,  Message);
  CASE(Group,    Message);
  CASE(Enum,     Enum);
  CASE(Int32,    Int32);
  CASE(Int64,    Int64);
  CASE(UInt32,   UInt32);
  CASE(UInt64,   UInt64);
  CASE(SInt32,   Int32);
  CASE(SInt64,   Int64);
  CASE(Fixed32,  UInt32);
  CASE(Fixed64,  UInt64);
  CASE(SFixed32, Int32);
  CASE(SFixed64, Int64);

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
  RETURN_STRING(upb_FieldDef_Name(intern->fielddef));
}

/*
 * FieldDescriptor::getNumber()
 *
 * Returns the number of this field.
 */
PHP_METHOD(FieldDescriptor, getNumber) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_FieldDef_Number(intern->fielddef));
}

/*
 * FieldDescriptor::getLabel()
 *
 * Returns the label of this field as an integer.
 */
PHP_METHOD(FieldDescriptor, getLabel) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_FieldDef_Label(intern->fielddef));
}

/*
 * FieldDescriptor::getType()
 *
 * Returns the type of this field as an integer.
 */
PHP_METHOD(FieldDescriptor, getType) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_FieldDef_Type(intern->fielddef));
}

/*
 * FieldDescriptor::isMap()
 *
 * Returns true if this field is a map.
 */
PHP_METHOD(FieldDescriptor, isMap) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  RETURN_BOOL(upb_FieldDef_IsMap(intern->fielddef));
}

/*
 * FieldDescriptor::getEnumType()
 *
 * Returns the EnumDescriptor for this field, which must be an enum.
 */
PHP_METHOD(FieldDescriptor, getEnumType) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  const upb_EnumDef *e = upb_FieldDef_EnumSubDef(intern->fielddef);
  zval ret;

  if (!e) {
    zend_throw_exception_ex(NULL, 0,
                            "Cannot get enum type for non-enum field '%s'",
                            upb_FieldDef_Name(intern->fielddef));
    return;
  }

  EnumDescriptor_FromEnumDef(&ret, e);
  RETURN_COPY_VALUE(&ret);
}

/*
 * FieldDescriptor::getContainingOneof()
 *
 * Returns the OneofDescriptor for this field, or null if it is not inside
 * a oneof.
 */
PHP_METHOD(FieldDescriptor, getContainingOneof) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  const upb_OneofDef *o = upb_FieldDef_ContainingOneof(intern->fielddef);
  zval ret;

  if (!o) {
    RETURN_NULL();
  }

  OneofDescriptor_FromOneofDef(&ret, o);
  RETURN_COPY_VALUE(&ret);
}

/*
 * FieldDescriptor::getRealContainingOneof()
 *
 * Returns the non-synthetic OneofDescriptor for this field, or null if it is
 * not inside a oneof.
 */
PHP_METHOD(FieldDescriptor, getRealContainingOneof) {
  FieldDescriptor *intern = (FieldDescriptor*)Z_OBJ_P(getThis());
  const upb_OneofDef *o = upb_FieldDef_RealContainingOneof(intern->fielddef);
  zval ret;

  if (!o) {
    RETURN_NULL();
  }

  OneofDescriptor_FromOneofDef(&ret, o);
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
        upb_FieldDef_Name(intern->fielddef));
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
  PHP_ME(FieldDescriptor, getContainingOneof, arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getRealContainingOneof, arginfo_void, ZEND_ACC_PUBLIC)
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

static zend_class_entry *Descriptor_GetGeneratedClass(const upb_MessageDef *m) {
  for (int i = 0; i < 2; ++i) {
    char *classname =
        GetPhpClassname(upb_MessageDef_File(m), upb_MessageDef_FullName(m), (bool)i);
    zend_string *str = zend_string_init(classname, strlen(classname), 0);
    zend_class_entry *ce = zend_lookup_class(str);  // May autoload the class.

    zend_string_release (str);
    free(classname);

    if (ce) {
      return ce;
    }
  }

  char *classname =
    GetPhpClassname(upb_MessageDef_File(m), upb_MessageDef_FullName(m), false);
  zend_error(E_ERROR, "Couldn't load generated class %s", classname);
  return NULL;
}

void Descriptor_FromMessageDef(zval *val, const upb_MessageDef *m) {
  if (m == NULL) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(m, val)) {
    zend_class_entry *ce = NULL;
    if (!upb_MessageDef_IsMapEntry(m)) {  // Map entries don't have a class.
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

Descriptor* Descriptor_GetFromMessageDef(const upb_MessageDef *m) {
  zval desc;
  Descriptor_FromMessageDef(&desc, m);
  return Descriptor_GetFromZval(&desc);
}

Descriptor* Descriptor_GetFromFieldDef(const upb_FieldDef *f) {
  return Descriptor_GetFromMessageDef(upb_FieldDef_MessageSubDef(f));
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
  RETURN_STRING(upb_MessageDef_FullName(intern->msgdef));
}

/*
 * Descriptor::getField()
 *
 * Returns a FieldDescriptor for the given index, which must be in the range
 * [0, getFieldCount()-1].
 */
PHP_METHOD(Descriptor, getField) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  int count = upb_MessageDef_FieldCount(intern->msgdef);
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

  FieldDescriptor_FromFieldDef(&ret, upb_MessageDef_Field(intern->msgdef, index));
  RETURN_COPY_VALUE(&ret);
}

/*
 * Descriptor::getFieldCount()
 *
 * Returns the number of fields in this message.
 */
PHP_METHOD(Descriptor, getFieldCount) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_MessageDef_FieldCount(intern->msgdef));
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

  if (index < 0 || index >= upb_MessageDef_OneofCount(intern->msgdef)) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  OneofDescriptor_FromOneofDef(&ret, upb_MessageDef_Oneof(intern->msgdef, index));
  RETURN_COPY_VALUE(&ret);
}

/*
 * Descriptor::getOneofDeclCount()
 *
 * Returns the number of oneofs in this message.
 */
PHP_METHOD(Descriptor, getOneofDeclCount) {
  Descriptor *intern = (Descriptor*)Z_OBJ_P(getThis());
  RETURN_LONG(upb_MessageDef_OneofCount(intern->msgdef));
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
  upb_DefPool *symtab;
} DescriptorPool;

zend_class_entry *DescriptorPool_class_entry;
static zend_object_handlers DescriptorPool_object_handlers;

static DescriptorPool *GetPool(const zval* this_ptr) {
  return (DescriptorPool*)Z_OBJ_P(this_ptr);
}

/**
 * Object handler to free an DescriptorPool.
 */
static void DescriptorPool_destructor(zend_object* obj) {
  DescriptorPool* intern = (DescriptorPool*)obj;

  // We can't free our underlying symtab here, because user code may create
  // messages from destructors that will refer to it. The symtab will be freed
  // by our RSHUTDOWN() handler in protobuf.c

  zend_object_std_dtor(&intern->std);
}

void DescriptorPool_CreateWithSymbolTable(zval *zv, upb_DefPool *symtab) {
  DescriptorPool *intern = emalloc(sizeof(DescriptorPool));
  zend_object_std_init(&intern->std, DescriptorPool_class_entry);
  intern->std.handlers = &DescriptorPool_object_handlers;
  intern->symtab = symtab;

  ZVAL_OBJ(zv, &intern->std);
}

upb_DefPool *DescriptorPool_GetSymbolTable() {
  return get_global_symtab();
}

/*
 * DescriptorPool::getGeneratedPool()
 *
 * Returns the generated DescriptorPool.
 */
PHP_METHOD(DescriptorPool, getGeneratedPool) {
  DescriptorPool_CreateWithSymbolTable(return_value, get_global_symtab());
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
  const upb_MessageDef *m;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &protoname, &protoname_len) ==
      FAILURE) {
    return;
  }

  if (*protoname == '.') protoname++;

  m = upb_DefPool_FindMessageByName(intern->symtab, protoname);

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
  const upb_StringView *deps;
  upb_StringView name = upb_StringView_FromString("google/protobuf/descriptor.proto");
  size_t i, n;

  deps = google_protobuf_FileDescriptorProto_dependency(file, &n);
  for (i = 0; i < n; i++) {
    if (upb_StringView_IsEqual(deps[i], name)) {
      return true;
    }
  }

  return false;
}

static void add_message_name_mappings(const upb_MessageDef *message) {
  NameMap_AddMessage(message);
  int msg_n = upb_MessageDef_NestedMessageCount(message);
  for (int i = 0; i < msg_n; i++) {
    add_message_name_mappings(upb_MessageDef_NestedMessage(message, i));
  }
  int enum_n = upb_MessageDef_NestedEnumCount(message);
  for (int i = 0; i < enum_n; i++) {
    NameMap_AddEnum(upb_MessageDef_NestedEnum(message, i));
  }
}

/*
 * add_name_mappings()
 *
 * Adds the messages and enums in this file to the NameMap.
 */
static void add_name_mappings(const upb_FileDef *file) {
  for (int i = 0; i < upb_FileDef_TopLevelMessageCount(file); i++) {
    add_message_name_mappings(upb_FileDef_TopLevelMessage(file, i));
  }

  for (int i = 0; i < upb_FileDef_TopLevelEnumCount(file); i++) {
    NameMap_AddEnum(upb_FileDef_TopLevelEnum(file, i));
  }
}

static void add_descriptor(upb_DefPool *symtab,
                           const google_protobuf_FileDescriptorProto *file) {
  upb_StringView name = google_protobuf_FileDescriptorProto_name(file);
  upb_Status status;
  const upb_FileDef *file_def;
  upb_Status_Clear(&status);

  if (upb_DefPool_FindFileByNameWithSize(symtab, name.data, name.size)) {
    // Already added.
    // TODO(teboring): Re-enable this warning when aggregate metadata is
    // deprecated.
    // zend_error(E_USER_WARNING,
    //            "proto descriptor was previously loaded (included in multiple "
    //            "metadata bundles?): " UPB_STRINGVIEW_FORMAT,
    //            UPB_STRINGVIEW_ARGS(name));
    return;
  }

  // The PHP code generator currently special-cases descriptor.proto.  It
  // doesn't add it as a dependency even if the proto file actually does
  // depend on it.
  if (depends_on_descriptor(file)) {
    google_protobuf_FileDescriptorProto_getmsgdef(symtab);
  }

  file_def = upb_DefPool_AddFile(symtab, file, &status);
  CheckUpbStatus(&status, "Unable to load descriptor");
  add_name_mappings(file_def);
}

/*
 * add_descriptor()
 *
 * Adds the given descriptor data to this DescriptorPool.
 */
static void add_descriptor_set(upb_DefPool *symtab, const char *data,
                               int data_len, upb_Arena *arena) {
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
    add_descriptor(symtab, file);
  }
}

bool DescriptorPool_HasFile(const char *filename) {
  return upb_DefPool_FindFileByName(get_global_symtab(), filename) != NULL;
}

void DescriptorPool_AddDescriptor(const char *filename, const char *data,
                                  int size) {
  upb_Arena *arena = upb_Arena_New();
  const google_protobuf_FileDescriptorProto *file =
      google_protobuf_FileDescriptorProto_parse(data, size, arena);

  if (!file) {
    zend_error(E_ERROR, "Failed to parse binary descriptor for %s\n", filename);
    return;
  }

  add_descriptor(get_global_symtab(), file);
  upb_Arena_Free(arena);
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
  upb_Arena *arena;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|b", &data, &data_len,
                            &use_nested_submsg) != SUCCESS) {
    return;
  }

  arena = upb_Arena_New();
  add_descriptor_set(intern->symtab, data, data_len, arena);
  upb_Arena_Free(arena);
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
  DescriptorPool_CreateWithSymbolTable(return_value, get_global_symtab());
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
  DescriptorPool_class_entry->create_object = CreateHandler_ReturnNull;
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
