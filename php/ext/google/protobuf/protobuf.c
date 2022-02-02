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

#include <php.h>
#include <Zend/zend_interfaces.h>

#include "arena.h"
#include "array.h"
#include "convert.h"
#include "def.h"
#include "map.h"
#include "message.h"
#include "names.h"

// -----------------------------------------------------------------------------
// Module "globals"
// -----------------------------------------------------------------------------

// Despite the name, module "globals" are really thread-locals:
//  * PROTOBUF_G(var) accesses the thread-local variable for 'var'. Either:
//    * PROTOBUF_G(var) -> protobuf_globals.var (Non-ZTS / non-thread-safe)
//    * PROTOBUF_G(var) -> <Zend magic>         (ZTS / thread-safe builds)

#define PROTOBUF_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(protobuf, v)

ZEND_BEGIN_MODULE_GLOBALS(protobuf)
  // Set by the user to make the descriptor pool persist between requests.
  zend_bool keep_descriptor_pool_after_request;

  // Currently we make the generated pool a "global", which means that if a user
  // does explicitly create threads within their request, the other threads will
  // get different results from DescriptorPool::getGeneratedPool(). We require
  // that all descriptors are loaded from the main thread.
  zval generated_pool;

  // A upb_symtab that we are saving for the next request so that we don't have
  // to rebuild it from scratch. When keep_descriptor_pool_after_request==true,
  // we steal the upb_symtab from the global DescriptorPool object just before
  // destroying it.
  upb_symtab *global_symtab;

  // Object cache (see interface in protobuf.h).
  HashTable object_cache;

  // Name cache (see interface in protobuf.h).
  HashTable name_msg_cache;
  HashTable name_enum_cache;

  // An array of descriptor objects constructed during this request. These are
  // logically referenced by the corresponding class entry, but since we can't
  // actually write a class entry destructor, we reference them here, to be
  // destroyed on request shutdown.
  HashTable descriptors;
ZEND_END_MODULE_GLOBALS(protobuf)

void free_protobuf_globals(zend_protobuf_globals *globals) {
  zend_hash_destroy(&globals->name_msg_cache);
  zend_hash_destroy(&globals->name_enum_cache);
  upb_symtab_free(globals->global_symtab);
  globals->global_symtab = NULL;
}

ZEND_DECLARE_MODULE_GLOBALS(protobuf)

const zval *get_generated_pool() {
  return &PROTOBUF_G(generated_pool);
}

// This is a PHP extension (not a Zend extension). What follows is a summary of
// a PHP extension's lifetime and when various handlers are called.
//
//  * PHP_GINIT_FUNCTION(protobuf) / PHP_GSHUTDOWN_FUNCTION(protobuf)
//    are the constructor/destructor for the globals. The sequence over the
//    course of a process lifetime is:
//
//    # Process startup
//    GINIT(<Main Thread Globals>)
//    MINIT
//
//    foreach request:
//      RINIT
//        # Request is processed here.
//      RSHUTDOWN
//
//    foreach thread:
//      GINIT(<This Thread Globals>)
//        # Code for the thread runs here.
//      GSHUTDOWN(<This Thread Globals>)
//
//    # Process Shutdown
//    #
//    # These should be running per the docs, but I have not been able to
//    # actually get the process-wide shutdown functions to run.
//    #
//    # MSHUTDOWN
//    # GSHUTDOWN(<Main Thread Globals>)
//
//  * Threads can be created either explicitly by the user, inside a request,
//    or implicitly by the runtime, to process multiple requests concurrently.
//    If the latter is being used, then the "foreach thread" block above
//    actually looks like this:
//
//    foreach thread:
//      GINIT(<This Thread Globals>)
//      # A non-main thread will only receive requests when using a threaded
//      # MPM with Apache
//      foreach request:
//        RINIT
//          # Request is processed here.
//        RSHUTDOWN
//      GSHUTDOWN(<This Thread Globals>)
//
// That said, it appears that few people use threads with PHP:
//   * The pthread package documented at
//     https://www.php.net/manual/en/class.thread.php nas not been released
//     since 2016, and the current release fails to compile against any PHP
//     newer than 7.0.33.
//     * The GitHub master branch supports 7.2+, but this has not been released
//       to PECL.
//     * Its owner has disavowed it as "broken by design" and "in an untenable
//       position for the future": https://github.com/krakjoe/pthreads/issues/929
//   * The only way to use PHP with requests in different threads is to use the
//     Apache 2 mod_php with the "worker" MPM. But this is explicitly
//     discouraged by the documentation: https://serverfault.com/a/231660

static PHP_GSHUTDOWN_FUNCTION(protobuf) {
  if (protobuf_globals->global_symtab) {
    free_protobuf_globals(protobuf_globals);
  }
}

static PHP_GINIT_FUNCTION(protobuf) {
  ZVAL_NULL(&protobuf_globals->generated_pool);
  protobuf_globals->global_symtab = NULL;
}

/**
 * PHP_RINIT_FUNCTION(protobuf)
 *
 * This function is run at the beginning of processing each request.
 */
static PHP_RINIT_FUNCTION(protobuf) {
  // Create the global generated pool.
  // Reuse the symtab (if any) left to us by the last request.
  upb_symtab *symtab = PROTOBUF_G(global_symtab);
  if (!symtab) {
    PROTOBUF_G(global_symtab) = symtab = upb_symtab_new();
    zend_hash_init(&PROTOBUF_G(name_msg_cache), 64, NULL, NULL, 0);
    zend_hash_init(&PROTOBUF_G(name_enum_cache), 64, NULL, NULL, 0);
  }
  DescriptorPool_CreateWithSymbolTable(&PROTOBUF_G(generated_pool), symtab);

  zend_hash_init(&PROTOBUF_G(object_cache), 64, NULL, NULL, 0);
  zend_hash_init(&PROTOBUF_G(descriptors), 64, NULL, ZVAL_PTR_DTOR, 0);

  return SUCCESS;
}

/**
 * PHP_RSHUTDOWN_FUNCTION(protobuf)
 *
 * This function is run at the end of processing each request.
 */
static PHP_RSHUTDOWN_FUNCTION(protobuf) {
  // Preserve the symtab if requested.
  if (!PROTOBUF_G(keep_descriptor_pool_after_request)) {
    free_protobuf_globals(ZEND_MODULE_GLOBALS_BULK(protobuf));
  }

  zval_dtor(&PROTOBUF_G(generated_pool));
  zend_hash_destroy(&PROTOBUF_G(object_cache));
  zend_hash_destroy(&PROTOBUF_G(descriptors));

  return SUCCESS;
}

// -----------------------------------------------------------------------------
// Object Cache.
// -----------------------------------------------------------------------------

void Descriptors_Add(zend_object *desc) {
  // The hash table will own a ref (it will destroy it when the table is
  // destroyed), but for some reason the insert operation does not add a ref, so
  // we do that here with ZVAL_OBJ_COPY().
  zval zv;
  ZVAL_OBJ_COPY(&zv, desc);
  zend_hash_next_index_insert(&PROTOBUF_G(descriptors), &zv);
}

void ObjCache_Add(const void *upb_obj, zend_object *php_obj) {
  zend_ulong k = (zend_ulong)upb_obj;
  zend_hash_index_add_ptr(&PROTOBUF_G(object_cache), k, php_obj);
}

void ObjCache_Delete(const void *upb_obj) {
  if (upb_obj) {
    zend_ulong k = (zend_ulong)upb_obj;
    int ret = zend_hash_index_del(&PROTOBUF_G(object_cache), k);
    PBPHP_ASSERT(ret == SUCCESS);
  }
}

bool ObjCache_Get(const void *upb_obj, zval *val) {
  zend_ulong k = (zend_ulong)upb_obj;
  zend_object *obj = zend_hash_index_find_ptr(&PROTOBUF_G(object_cache), k);

  if (obj) {
    ZVAL_OBJ_COPY(val, obj);
    return true;
  } else {
    ZVAL_NULL(val);
    return false;
  }
}

// -----------------------------------------------------------------------------
// Name Cache.
// -----------------------------------------------------------------------------

void NameMap_AddMessage(const upb_msgdef *m) {
  char *k = GetPhpClassname(upb_msgdef_file(m), upb_msgdef_fullname(m));
  zend_hash_str_add_ptr(&PROTOBUF_G(name_msg_cache), k, strlen(k), (void*)m);
  free(k);
}

void NameMap_AddEnum(const upb_enumdef *e) {
  char *k = GetPhpClassname(upb_enumdef_file(e), upb_enumdef_fullname(e));
  zend_hash_str_add_ptr(&PROTOBUF_G(name_enum_cache), k, strlen(k), (void*)e);
  free(k);
}

const upb_msgdef *NameMap_GetMessage(zend_class_entry *ce) {
  const upb_msgdef *ret =
      zend_hash_find_ptr(&PROTOBUF_G(name_msg_cache), ce->name);

  if (!ret && ce->create_object) {
#if PHP_VERSION_ID < 80000
    zval tmp;
    zval zv;
    ZVAL_OBJ(&tmp, ce->create_object(ce));
    zend_call_method_with_0_params(&tmp, ce, NULL, "__construct", &zv);
    zval_ptr_dtor(&tmp);
#else
    zval zv;
    zend_object *tmp = ce->create_object(ce);
    zend_call_method_with_0_params(tmp, ce, NULL, "__construct", &zv);
    OBJ_RELEASE(tmp);
#endif
    zval_ptr_dtor(&zv);
    ret = zend_hash_find_ptr(&PROTOBUF_G(name_msg_cache), ce->name);
  }

  return ret;
}

const upb_enumdef *NameMap_GetEnum(zend_class_entry *ce) {
  const upb_enumdef *ret =
      zend_hash_find_ptr(&PROTOBUF_G(name_enum_cache), ce->name);
  return ret;
}

// -----------------------------------------------------------------------------
// Module init.
// -----------------------------------------------------------------------------

zend_function_entry protobuf_functions[] = {
  ZEND_FE_END
};

static const zend_module_dep protobuf_deps[] = {
  ZEND_MOD_OPTIONAL("date")
  ZEND_MOD_END
};

PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("protobuf.keep_descriptor_pool_after_request", "0",
                  PHP_INI_ALL, OnUpdateBool,
                  keep_descriptor_pool_after_request, zend_protobuf_globals,
                  protobuf_globals)
PHP_INI_END()

static PHP_MINIT_FUNCTION(protobuf) {
  REGISTER_INI_ENTRIES();
  Arena_ModuleInit();
  Array_ModuleInit();
  Convert_ModuleInit();
  Def_ModuleInit();
  Map_ModuleInit();
  Message_ModuleInit();
  return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(protobuf) {
  UNREGISTER_INI_ENTRIES();
  return SUCCESS;
}

zend_module_entry protobuf_module_entry = {
  STANDARD_MODULE_HEADER_EX,
  NULL,
  protobuf_deps,
  "protobuf",               // extension name
  protobuf_functions,       // function list
  PHP_MINIT(protobuf),      // process startup
  PHP_MSHUTDOWN(protobuf),  // process shutdown
  PHP_RINIT(protobuf),      // request shutdown
  PHP_RSHUTDOWN(protobuf),  // request shutdown
  NULL,                     // extension info
  PHP_PROTOBUF_VERSION,     // extension version
  PHP_MODULE_GLOBALS(protobuf),  // globals descriptor
  PHP_GINIT(protobuf),      // globals ctor
  PHP_GSHUTDOWN(protobuf),  // globals dtor
  NULL,                     // post deactivate
  STANDARD_MODULE_PROPERTIES_EX
};

ZEND_GET_MODULE(protobuf)
