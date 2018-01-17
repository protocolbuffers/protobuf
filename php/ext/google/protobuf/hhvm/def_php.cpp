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

#include "protobuf_php.h"

// -----------------------------------------------------------------------------
// Global Variable
// -----------------------------------------------------------------------------

#if PHP_MAJOR_VERSION < 7
zval* internal_generated_pool;
#else
zend_object *internal_generated_pool;
#endif

// -----------------------------------------------------------------------------
// Define static methods
// -----------------------------------------------------------------------------

static void InternalDescriptorPool_init_handlers(
    zend_object_handlers* handlers) {}
static void InternalDescriptorPool_init_type(
    zend_class_entry* handlers) {}

// -----------------------------------------------------------------------------
// Define PHP class
// -----------------------------------------------------------------------------

PHP_METHOD(InternalDescriptorPool, internalAddGeneratedFile);
PHP_METHOD(InternalDescriptorPool, getGeneratedPool);

PROTO_REGISTER_CLASS_METHODS_START(InternalDescriptorPool)
  PHP_ME(InternalDescriptorPool, internalAddGeneratedFile,
         NULL, ZEND_ACC_PUBLIC)
  PHP_ME(InternalDescriptorPool, getGeneratedPool,
         NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_DEFINE_CLASS(InternalDescriptorPool,
             "Google\\Protobuf\\Internal\\DescriptorPool");

// -----------------------------------------------------------------------------
// Define PHP methods
// -----------------------------------------------------------------------------

PHP_METHOD(InternalDescriptorPool, internalAddGeneratedFile) {
  char *data = NULL;
  PROTO_SIZE data_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) ==
      FAILURE) {
    return;
  }

  InternalDescriptorPool *pool = UNBOX(InternalDescriptorPool, getThis());
  InternalDescriptorPool_add_generated_file(pool, data, data_len);

  PROTO_RETURN_BOOL(true);
}

static void init_generated_pool_once(TSRMLS_D) {
  if (internal_generated_pool == NULL) {
#if PHP_MAJOR_VERSION < 7
    MAKE_STD_ZVAL(internal_generated_pool);
    ZVAL_OBJ(internal_generated_pool,
             InternalDescriptorPool_type->create_object(
                 InternalDescriptorPool_type TSRMLS_CC));
    internal_generated_pool_cpp =
        UNBOX(InternalDescriptorPool, internal_generated_pool);
#else
    internal_generated_pool = InternalDescriptorPool_type->create_object(
        InternalDescriptorPool_type TSRMLS_CC);
    internal_generated_pool_cpp =
        (InternalDescriptorPool *)((char *)internal_generated_pool -
            XtOffsetOf(InternalDescriptorPool, std));
#endif
    message_factory = upb_msgfactory_new(
        internal_generated_pool_cpp->symtab);
  }
}

PHP_METHOD(InternalDescriptorPool, getGeneratedPool) {
  init_generated_pool_once(TSRMLS_C);
#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(internal_generated_pool, 1, 0);
#else
  ++GC_REFCOUNT(internal_generated_pool);
  RETURN_OBJ(internal_generated_pool);
#endif
}
