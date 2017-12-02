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

#include "fake_protobuf.h"

// // Forward declare.
// static void internal_descriptor_pool_free_c(
//     InternalDescriptorPool *object TSRMLS_DC);
// static void internal_descriptor_pool_init_c_instance(
//     InternalDescriptorPool *pool TSRMLS_DC);
// 
// // -----------------------------------------------------------------------------
// // InternalDescriptorPool
// // -----------------------------------------------------------------------------
// 
// static zend_function_entry internal_descriptor_pool_methods[] = {
//   PHP_ME(InternalDescriptorPool, internalAddGeneratedFile, NULL, ZEND_ACC_PUBLIC)
//   ZEND_FE_END
// };

static void internal_descriptor_pool_init_c_instance(
    InternalDescriptorPool *pool TSRMLS_DC) {
  pool->symtab = upb_symtab_new();
}

static void internal_descriptor_pool_free_c(
    InternalDescriptorPool *pool TSRMLS_DC) {
  upb_symtab_free(pool->symtab);
}

PROTO_REGISTER_CLASS_METHODS_START(InternalDescriptorPool)
  PROTO_REGISTER_METHOD(Google\\Protobuf\\Internal\\DescriptorPool,
                        InternalDescriptorPool, internalAddGeneratedFile)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_DEFINE_CLASS(InternalDescriptorPool, internal_descriptor_pool,
             "Google\\Protobuf\\Internal\\DescriptorPool");

PROTO_METHOD(InternalDescriptorPool, internalAddGeneratedFile, bool) {
  PROTO_RETURN_BOOL(true);
}
