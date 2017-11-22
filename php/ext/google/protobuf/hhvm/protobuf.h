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

#include "php.h"
#include "upb.h"

#define PHP_PROTOBUF_EXTNAME "protobuf"
#define PHP_PROTOBUF_VERSION "3.4.1"

struct InternalDescriptorPool;

typedef struct InternalDescriptorPool InternalDescriptorPool;

// -----------------------------------------------------------------------------
// Globals.
// -----------------------------------------------------------------------------

ZEND_BEGIN_MODULE_GLOBALS(protobuf)
ZEND_END_MODULE_GLOBALS(protobuf)

// Init module and PHP classes.
void internal_descriptor_pool_init(TSRMLS_D);

// Define PHP class

#if PHP_MAJOR_VERSION < 7

#define PHP_PROTO_SIZE int

#define PHP_PROTO_WRAP_OBJECT_START(name) \
  class name {                            \
   public:                                \
    zend_object std;
#define PHP_PROTO_WRAP_OBJECT_END \
  };

#define PHP_PROTO_OBJECT_FREE_START(classname, lowername) \
  void lowername##_free(void* object TSRMLS_DC) {         \
    classname* intern = (classname*)object;
#define PHP_PROTO_OBJECT_FREE_END                 \
    zend_object_std_dtor(&intern->std TSRMLS_CC); \
    efree(intern);                                \
  }

#define PHP_PROTO_OBJECT_DTOR_START(classname, lowername)
#define PHP_PROTO_OBJECT_DTOR_END

#define PHP_PROTO_OBJECT_CREATE_START(NAME, LOWWERNAME) \
  static zend_object_value LOWWERNAME##_create(         \
      zend_class_entry* ce TSRMLS_DC) {                 \
    PHP_PROTO_ALLOC_CLASS_OBJECT(NAME, ce);             \
    zend_object_std_init(&intern->std, ce TSRMLS_CC);   \
    object_properties_init(&intern->std, ce);
#define PHP_PROTO_OBJECT_CREATE_END(NAME, LOWWERNAME)                          \
  PHP_PROTO_FREE_CLASS_OBJECT(NAME, LOWWERNAME##_free, LOWWERNAME##_handlers); \
  }

#define PHP_PROTO_INIT_CLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)         \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        LOWWERNAME##_methods);                               \
    LOWWERNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    LOWWERNAME##_type->create_object = LOWWERNAME##_create;                  \
    LOWWERNAME##_handlers = PEMALLOC(zend_object_handlers);                  \
    memcpy(LOWWERNAME##_handlers, zend_get_std_object_handlers(),            \
           sizeof(zend_object_handlers));
#define PHP_PROTO_INIT_CLASS_END \
  }

#define PHP_PROTO_ALLOC_CLASS_OBJECT(class_object, class_type) \
  class_object* intern;                                        \
  intern = (class_object*)emalloc(sizeof(class_object));       \
  memset(intern, 0, sizeof(class_object));

#define PHP_PROTO_FREE_CLASS_OBJECT(class_object, class_object_free, handler) \
  zend_object_value retval = {0};                                             \
  retval.handle = zend_objects_store_put(                                     \
      intern, (zend_objects_store_dtor_t)zend_objects_destroy_object,         \
      class_object_free, NULL TSRMLS_CC);                                     \
  retval.handlers = handler;                                                  \
  return retval;

#define UNBOX(class_name, val) \
  (class_name*)zend_object_store_get_object(val TSRMLS_CC);

#else  // // PHP_MAJOR_VERSION < 7

#define PHP_PROTO_SIZE size_t

#define PHP_PROTO_WRAP_OBJECT_START(name) \
  class name {                            \
   public:
#define PHP_PROTO_WRAP_OBJECT_END \
  zend_object std;                \
  };

#define PHP_PROTO_OBJECT_FREE_START(classname, lowername) \
  void lowername##_free(zend_object* object) {            \
    classname* intern =                                   \
        (classname*)((char*)object - XtOffsetOf(classname, std));
#define PHP_PROTO_OBJECT_FREE_END           \
  }

#define PHP_PROTO_OBJECT_DTOR_START(classname, lowername) \
  void lowername##_dtor(zend_object* object) {            \
    classname* intern =                                   \
        (classname*)((char*)object - XtOffsetOf(classname, std));
#define PHP_PROTO_OBJECT_DTOR_END           \
    zend_object_std_dtor(object TSRMLS_CC); \
  }

#define PHP_PROTO_OBJECT_CREATE_START(NAME, LOWWERNAME)                     \
  static zend_object* LOWWERNAME##_create(zend_class_entry* ce TSRMLS_DC) { \
    PHP_PROTO_ALLOC_CLASS_OBJECT(NAME, ce);                                 \
    zend_object_std_init(&intern->std, ce TSRMLS_CC);                       \
    object_properties_init(&intern->std, ce);
#define PHP_PROTO_OBJECT_CREATE_END(NAME, LOWWERNAME)                          \
  PHP_PROTO_FREE_CLASS_OBJECT(NAME, LOWWERNAME##_free, LOWWERNAME##_handlers); \
  }

#define PHP_PROTO_INIT_CLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)         \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        LOWWERNAME##_methods);                               \
    LOWWERNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    LOWWERNAME##_type->create_object = LOWWERNAME##_create;                  \
    LOWWERNAME##_handlers = PEMALLOC(zend_object_handlers);                  \
    memcpy(LOWWERNAME##_handlers, zend_get_std_object_handlers(),            \
           sizeof(zend_object_handlers));                                    \
    LOWWERNAME##_handlers->free_obj = LOWWERNAME##_free;                     \
    LOWWERNAME##_handlers->dtor_obj = LOWWERNAME##_dtor;                     \
    LOWWERNAME##_handlers->offset = XtOffsetOf(CAMELNAME, std);
#define PHP_PROTO_INIT_CLASS_END \
  }

#define PHP_PROTO_ALLOC_CLASS_OBJECT(class_object, class_type)               \
  class_object* intern;                                                      \
  int size = sizeof(class_object) + zend_object_properties_size(class_type); \
  intern = ecalloc(1, size);                                                 \
  memset(intern, 0, size);

#define PHP_PROTO_FREE_CLASS_OBJECT(class_object, class_object_free, handler) \
  intern->std.handlers = handler;                                             \
  return &intern->std;

#define UNBOX(class_name, val) \
  (class_name*)((char*)Z_OBJ_P(val) - XtOffsetOf(class_name, std));

#endif  // PHP_MAJOR_VERSION >= 7

#define DEFINE_PROTOBUF_INIT_CLASS(CLASSNAME, CAMELNAME, LOWERNAME) \
  PHP_PROTO_INIT_CLASS_START(CLASSNAME, CAMELNAME, LOWERNAME)       \
  PHP_PROTO_INIT_CLASS_END

#define DEFINE_PROTOBUF_CREATE(NAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_CREATE_START(NAME, LOWERNAME) \
  LOWERNAME##_init_c_instance(intern TSRMLS_CC); \
  PHP_PROTO_OBJECT_CREATE_END(NAME, LOWERNAME)

#define DEFINE_PROTOBUF_FREE(CAMELNAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_FREE_START(CAMELNAME, LOWERNAME) \
  LOWERNAME##_free_c(intern TSRMLS_CC);             \
  PHP_PROTO_OBJECT_FREE_END

#define DEFINE_PROTOBUF_DTOR(CAMELNAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_DTOR_START(CAMELNAME, LOWERNAME) \
  PHP_PROTO_OBJECT_DTOR_END

#define DEFINE_CLASS(NAME, LOWERNAME, string_name) \
  zend_class_entry *LOWERNAME##_type;              \
  zend_object_handlers *LOWERNAME##_handlers;      \
  DEFINE_PROTOBUF_FREE(NAME, LOWERNAME)            \
  DEFINE_PROTOBUF_DTOR(NAME, LOWERNAME)            \
  DEFINE_PROTOBUF_CREATE(NAME, LOWERNAME)          \
  DEFINE_PROTOBUF_INIT_CLASS(string_name, NAME, LOWERNAME)

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

// Memory management
#define ALLOC(class_name) (class_name*) emalloc(sizeof(class_name))
#define PEMALLOC(class_name) (class_name*) pemalloc(sizeof(class_name), 1)
#define ALLOC_N(class_name, n) (class_name*) emalloc(sizeof(class_name) * n)
#define FREE(object) efree(object)
#define PEFREE(object) pefree(object, 1)

// String argument.
#define STR(str) (str), strlen(str)

// -----------------------------------------------------------------------------
// Descriptor.
// -----------------------------------------------------------------------------

PHP_PROTO_WRAP_OBJECT_START(InternalDescriptorPool)
  upb_symtab* symtab;
  HashTable* pending_list;
PHP_PROTO_WRAP_OBJECT_END

PHP_METHOD(InternalDescriptorPool, internalAddGeneratedFile);

#if PHP_MAJOR_VERSION < 7
void internal_descriptor_pool_free(void* object TSRMLS_DC);
#else
void internal_descriptor_pool_free(zend_object* object);
#endif

#endif  // __GOOGLE_PROTOBUF_PHP_PROTOBUF_H__
