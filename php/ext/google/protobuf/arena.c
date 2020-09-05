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

#include <Zend/zend_API.h>

#include "php-upb.h"

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

typedef struct Arena {
  zend_object std;
  upb_arena* arena;
} Arena;

zend_class_entry *Arena_class_entry;
static zend_object_handlers Arena_object_handlers;

// PHP Object Handlers /////////////////////////////////////////////////////////

static zend_object* Arena_Create(zend_class_entry *class_type) {
  Arena *intern = emalloc(sizeof(Arena));
  zend_object_std_init(&intern->std, class_type);
  intern->std.handlers = &Arena_object_handlers;
  intern->arena = upb_arena_new();
  // Skip object_properties_init(), we don't allow derived classes.
  return &intern->std;
}

static void Arena_Free(zend_object* obj) {
  Arena* intern = (Arena*)obj;
  upb_arena_free(intern->arena);
  zend_object_std_dtor(&intern->std);
}

// C Functions from arena.h ////////////////////////////////////////////////////

void Arena_Init(zval* val) {
  ZVAL_OBJ(val, Arena_Create(Arena_class_entry));
}

upb_arena *Arena_Get(zval *val) {
  Arena *a = (Arena*)Z_OBJ_P(val);
  return a->arena;
}

// -----------------------------------------------------------------------------
// Module init.
// -----------------------------------------------------------------------------

// No public methods.
static const zend_function_entry Arena_methods[] = {
  ZEND_FE_END
};

void Arena_ModuleInit() {
  zend_class_entry tmp_ce;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Internal\\Arena", Arena_methods);
  Arena_class_entry = zend_register_internal_class(&tmp_ce);
  Arena_class_entry->create_object = Arena_Create;
  Arena_class_entry->ce_flags |= ZEND_ACC_FINAL;

  memcpy(&Arena_object_handlers, &std_object_handlers,
         sizeof(zend_object_handlers));
  Arena_object_handlers.free_obj = Arena_Free;
}
