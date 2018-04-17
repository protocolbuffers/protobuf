#ifndef __GOOGLE_PROTOBUF_PHP_PORT_PHP_H__
#define __GOOGLE_PROTOBUF_PHP_PORT_PHP_H__

#include "php.h"
#include "Zend/zend_exceptions.h"

#if PHP_MAJOR_VERSION < 7
#include "port_php5.h"
#else  // php5
#include "port_php7.h"
#endif  // php7

#define CLASS zend_class_entry*

#define PROTO_DEFINE_CLASS(CLASSNAME, STRINGNAME) \
  zend_class_entry *CLASSNAME##_type;             \
  zend_object_handlers *CLASSNAME##_handlers;     \
  PROTO_IMPL_DEFINE_FREE(CLASSNAME)               \
  PROTO_IMPL_DEFINE_DTOR(CLASSNAME)               \
  PROTO_IMPL_DEFINE_CREATE(CLASSNAME)             \
  PROTO_IMPL_DEFINE_INIT_CLASS(STRINGNAME, CLASSNAME)

#define PROTO_IMPL_DEFINE_CREATE(CLASSNAME)      \
  PROTO_OBJECT_CREATE_START(CLASSNAME)           \
  CLASSNAME##_init_c_instance(intern TSRMLS_CC); \
  PROTO_OBJECT_CREATE_END(CLASSNAME)
#define PROTO_IMPL_DEFINE_FREE(CLASSNAME) \
  PROTO_OBJECT_FREE_START(CLASSNAME)      \
  CLASSNAME##_free_c(intern TSRMLS_CC);   \
  PROTO_OBJECT_FREE_END
#define PROTO_IMPL_DEFINE_DTOR(CLASSNAME) \
  PROTO_OBJECT_DTOR_START(CLASSNAME)      \
  PROTO_OBJECT_DTOR_END
#define PROTO_IMPL_DEFINE_INIT_CLASS(FULLNAME, CLASSNAME) \
  PROTO_INIT_CLASS_START(FULLNAME, CLASSNAME)             \
  PROTO_INIT_CLASS_END

// Register php class methods.
#define PROTO_REGISTER_CLASS_METHODS_START(CLASSNAME) \
  static zend_function_entry CLASSNAME##_methods[] = {
#define PROTO_REGISTER_CLASS_METHODS_END \
    ZEND_FE_END                          \
  };

#define PROTO_METHOD(CLASSNAME, METHODNAME, RETURN_TYPE) \
  PHP_METHOD(CLASSNAME, METHODNAME)

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

// Error report
#define CHECK_UPB(code, msg)        \
  do {                              \
    upb_status status;              \
    code;                           \
    if (!upb_ok(&status)) {         \
      zend_error(E_ERROR, "%s: %s\n", msg, upb_status_errmsg(&status)); \
    }                               \
  } while (0)

// -----------------------------------------------------------------------------
// Forward Declare
// -----------------------------------------------------------------------------
extern zend_class_entry *Arena_type;
extern zend_class_entry *MapField_type;
extern zend_class_entry *Message_type;
extern zend_class_entry *RepeatedField_type;

#endif  // __GOOGLE_PROTOBUF_PHP_PORT_PHP_H__
