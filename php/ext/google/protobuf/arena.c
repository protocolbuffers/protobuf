// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <Zend/zend_API.h>

#include "php-upb.h"

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

typedef struct Arena {
  zend_object std;
  upb_Arena* arena;
} Arena;

zend_class_entry* Arena_class_entry;
static zend_object_handlers Arena_object_handlers;

// PHP Object Handlers /////////////////////////////////////////////////////////

static zend_object* Arena_Create(zend_class_entry* class_type) {
  Arena* intern = emalloc(sizeof(Arena));
  zend_object_std_init(&intern->std, class_type);
  intern->std.handlers = &Arena_object_handlers;
  intern->arena = upb_Arena_New();
  // Skip object_properties_init(), we don't allow derived classes.
  return &intern->std;
}

static void Arena_Free(zend_object* obj) {
  Arena* intern = (Arena*)obj;
  upb_Arena_Free(intern->arena);
  zend_object_std_dtor(&intern->std);
}

// C Functions from arena.h ////////////////////////////////////////////////////

void Arena_Init(zval* val) { ZVAL_OBJ(val, Arena_Create(Arena_class_entry)); }

upb_Arena* Arena_Get(zval* val) {
  Arena* a = (Arena*)Z_OBJ_P(val);
  return a->arena;
}

// -----------------------------------------------------------------------------
// Module init.
// -----------------------------------------------------------------------------

// No public methods.
static const zend_function_entry Arena_methods[] = {ZEND_FE_END};

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
