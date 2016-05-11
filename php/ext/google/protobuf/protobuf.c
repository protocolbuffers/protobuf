#include "protobuf.h"

#include <zend_hash.h>

ZEND_DECLARE_MODULE_GLOBALS(protobuf)
static PHP_GINIT_FUNCTION(protobuf);
static PHP_GSHUTDOWN_FUNCTION(protobuf);

// -----------------------------------------------------------------------------
// Global map from upb {msg,enum}defs to wrapper Descriptor/EnumDescriptor
// instances.
// -----------------------------------------------------------------------------

void add_def_obj(const void* def, zval* value) {
  uint nIndex = (ulong)def & PROTOBUF_G(upb_def_to_php_obj_map).nTableMask;

  zval* pDest = NULL;
  Z_ADDREF_P(value);
  zend_hash_index_update(&PROTOBUF_G(upb_def_to_php_obj_map), (zend_ulong)def,
                         &value, sizeof(zval*), &pDest);
}

zval* get_def_obj(const void* def) {
  zval** value;
  if (zend_hash_index_find(&PROTOBUF_G(upb_def_to_php_obj_map), (zend_ulong)def,
                           &value) == FAILURE) {
    zend_error(E_ERROR, "PHP object not found for given definition.\n");
    return NULL;
  }
  return *value;
}

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

// define the function(s) we want to add
zend_function_entry protobuf_functions[] = {
  ZEND_FE(get_generated_pool, NULL)
  ZEND_FE_END
};

// "protobuf_functions" refers to the struct defined above
// we'll be filling in more of this later: you can use this to specify
// globals, php.ini info, startup and teardown functions, etc.
zend_module_entry protobuf_module_entry = {
  STANDARD_MODULE_HEADER,
  PHP_PROTOBUF_EXTNAME, // extension name
  protobuf_functions,   // function list
  PHP_MINIT(protobuf),  // process startup
  NULL,  // process shutdown
  NULL,  // request startup
  NULL,  // request shutdown
  NULL,  // extension info
  PHP_PROTOBUF_VERSION, // extension version
  PHP_MODULE_GLOBALS(protobuf),  // globals descriptor
  PHP_GINIT(protobuf), // globals ctor
  PHP_GSHUTDOWN(protobuf),  // globals dtor
  NULL,  // post deactivate
  STANDARD_MODULE_PROPERTIES_EX
};

// install module
ZEND_GET_MODULE(protobuf)

// global variables
static PHP_GINIT_FUNCTION(protobuf) {
  protobuf_globals->generated_pool = NULL;
  generated_pool = NULL;
  protobuf_globals->message_handlers = NULL;
  zend_hash_init(&protobuf_globals->upb_def_to_php_obj_map, 16, NULL,
                 ZVAL_PTR_DTOR, 0);
}

static PHP_GSHUTDOWN_FUNCTION(protobuf) {
  if (protobuf_globals->generated_pool != NULL) {
    FREE_ZVAL(protobuf_globals->generated_pool);
  }
  if (protobuf_globals->message_handlers != NULL) {
    FREE(protobuf_globals->message_handlers);
  }
  zend_hash_destroy(&protobuf_globals->upb_def_to_php_obj_map);
}

PHP_MINIT_FUNCTION(protobuf) {
  descriptor_pool_init(TSRMLS_C);
  descriptor_init(TSRMLS_C);
  message_builder_context_init(TSRMLS_C);
}
