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

#include <map>

#include "protobuf_php.h"

// -----------------------------------------------------------------------------
// Globals.
// -----------------------------------------------------------------------------

static std::map<const void*, const upb_def*>* class2def;
static std::map<const upb_def*, const void*>* def2class;
static std::map<const std::string, zend_class_entry*>* name2classmap;

void register_upbdef(const char* classname, const upb_def* def) {
  PROTO_CE_DECLARE pce;
  TSRMLS_FETCH();
  if (php_proto_zend_lookup_class(classname, strlen(classname), &pce) ==
      FAILURE) {
    return;
  }
  (*class2def)[PROTO_CE_UNREF(pce)] = def;
  (*def2class)[def] = PROTO_CE_UNREF(pce);
  (*name2classmap)[std::string(upb_def_fullname(def))] = PROTO_CE_UNREF(pce);
}

const upb_msgdef* class2msgdef(const void* klass) {
  const upb_def* def = (*class2def)[klass];
  if (def == NULL || upb_def_type(def) != UPB_DEF_MSG) {
    return NULL;
  }
  return upb_downcast_msgdef(def);
}

const upb_enumdef* class2enumdef(const void* klass) {
  const upb_def* def = (*class2def)[klass];
  if (def == NULL || upb_def_type(def) != UPB_DEF_ENUM) {
    return NULL;
  }
  return upb_downcast_enumdef(def);
}

const void* msgdef2class(const upb_msgdef* msgdef) {
  const zend_class_entry* klass = (const zend_class_entry*)
      (*def2class)[upb_msgdef_upcast(msgdef)];
  assert(klass != NULL);
  return klass;
}

zend_class_entry* name2class(const std::string& name) {
  zend_class_entry* klass = (*name2classmap)[name];
  assert(klass != NULL);
  return klass;
}

ZEND_BEGIN_MODULE_GLOBALS(protobuf)
ZEND_END_MODULE_GLOBALS(protobuf)

ZEND_DECLARE_MODULE_GLOBALS(protobuf)
static PHP_GINIT_FUNCTION(protobuf);
static PHP_GSHUTDOWN_FUNCTION(protobuf);
static PHP_RINIT_FUNCTION(protobuf);
static PHP_RSHUTDOWN_FUNCTION(protobuf);
static PHP_MINIT_FUNCTION(protobuf);
static PHP_MSHUTDOWN_FUNCTION(protobuf);

zend_function_entry protobuf_functions[] = {
  ZEND_FE_END
};

zend_module_entry protobuf_module_entry = {
  STANDARD_MODULE_HEADER,
  PHP_PROTOBUF_EXTNAME,     // extension name
  protobuf_functions,       // function list
  PHP_MINIT(protobuf),      // process startup
  PHP_MSHUTDOWN(protobuf),  // process shutdown
  PHP_RINIT(protobuf),      // request shutdown
  PHP_RSHUTDOWN(protobuf),  // request shutdown
  NULL,                 // extension info
  PHP_PROTOBUF_VERSION, // extension version
  PHP_MODULE_GLOBALS(protobuf),  // globals descriptor
  PHP_GINIT(protobuf),  // globals ctor
  PHP_GSHUTDOWN(protobuf),  // globals dtor
  NULL,  // post deactivate
  STANDARD_MODULE_PROPERTIES_EX
};

// install module
ZEND_GET_MODULE(protobuf)

// global variables
static PHP_GINIT_FUNCTION(protobuf) {
}

static PHP_GSHUTDOWN_FUNCTION(protobuf) {
}

static PHP_RINIT_FUNCTION(protobuf) {
  class2def = new std::map<const void*, const upb_def*>();
  def2class = new std::map<const upb_def*, const void*>();
  name2classmap = new std::map<const std::string, zend_class_entry*>();
  internal_generated_pool = NULL;
  return 0;
}

static PHP_RSHUTDOWN_FUNCTION(protobuf) {
  delete class2def;
  delete def2class;
  delete name2classmap;
#if PHP_MAJOR_VERSION < 7
  if (internal_generated_pool != NULL) {
    zval_dtor(internal_generated_pool);
    FREE_ZVAL(internal_generated_pool);
    zval_dtor(generated_pool);
    FREE_ZVAL(generated_pool);
    upb_msgfactory_free(message_factory);
  }
#else
  if (internal_generated_pool != NULL) {
    {
      zval tmp;
      ZVAL_OBJ(&tmp, internal_generated_pool);
      zval_dtor(&tmp);
    }
    {
      zval tmp;
      ZVAL_OBJ(&tmp, generated_pool);
      zval_dtor(&tmp);
    }
    upb_msgfactory_free(message_factory);
  }
#endif
}

static PHP_MINIT_FUNCTION(protobuf) {
  protobuf_module = new ProtobufModule();
  Arena_init(TSRMLS_C);
  DescriptorPool_init(TSRMLS_C);
  InternalDescriptorPool_init(TSRMLS_C);
  MapField_init(TSRMLS_C);
  MapFieldIter_init(TSRMLS_C);
  Message_init(TSRMLS_C);
  RepeatedField_init(TSRMLS_C);
  RepeatedFieldIter_init(TSRMLS_C);
  Type_init(TSRMLS_C);
  Util_init(TSRMLS_C);

  // Descriptors
  Descriptor_init(TSRMLS_C);
  EnumDescriptor_init(TSRMLS_C);
  EnumValueDescriptor_init(TSRMLS_C);
  FieldDescriptor_init(TSRMLS_C);
  OneofDescriptor_init(TSRMLS_C);

  // Well Known Types
  Any_init(TSRMLS_C);
  Api_init(TSRMLS_C);
  BoolValue_init(TSRMLS_C);
  BytesValue_init(TSRMLS_C);
  DoubleValue_init(TSRMLS_C);
  Duration_init(TSRMLS_C);
  Empty_init(TSRMLS_C);
  Enum_init(TSRMLS_C);
  EnumValue_init(TSRMLS_C);
  Field_Cardinality_init(TSRMLS_C);
  Field_init(TSRMLS_C);
  Field_Kind_init(TSRMLS_C);
  FieldMask_init(TSRMLS_C);
  FloatValue_init(TSRMLS_C);
  GPBType_init(TSRMLS_C);
  Int32Value_init(TSRMLS_C);
  Int64Value_init(TSRMLS_C);
  ListValue_init(TSRMLS_C);
  Method_init(TSRMLS_C);
  Mixin_init(TSRMLS_C);
  NullValue_init(TSRMLS_C);
  Option_init(TSRMLS_C);
  SourceContext_init(TSRMLS_C);
  StringValue_init(TSRMLS_C);
  Struct_init(TSRMLS_C);
  Syntax_init(TSRMLS_C);
  Timestamp_init(TSRMLS_C);
  UInt32Value_init(TSRMLS_C);
  UInt64Value_init(TSRMLS_C);
  Value_init(TSRMLS_C);
}

static PHP_MSHUTDOWN_FUNCTION(protobuf) {
  delete protobuf_module;
}
