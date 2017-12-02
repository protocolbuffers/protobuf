#include "php.h"

// Register php class methods.
#define PROTO_REGISTER_CLASS_METHODS_START(CLASSNAME) \
  static zend_function_entry CLASSNAME##_methods[] = {
#define PROTO_REGISTER_CLASS_METHODS_END \
    ZEND_FE_END                          \
  };
#define PROTO_REGISTER_METHOD(FULLNAME, CLASSNAME, METHODNAME) \
  PHP_ME(CLASSNAME, METHODNAME, NULL, ZEND_ACC_PUBLIC)

#define PROTO_METHOD(CLASSNAME, METHODNAME, RETURN_TYPE) \
  PHP_METHOD(CLASSNAME, METHODNAME)

#define PROTO_RETURN_BOOL(VALUE) RETURN_BOOL(VALUE);

// Define wrapper class.
#define PROTO_WRAP_OBJECT_START(name) struct name {
#define PROTO_WRAP_OBJECT_END \
    zend_object std;              \
  };

#define PROTO_DEFINE_CLASS(CLASSNAME, LOWERNAME, STRINGNAME) \
  zend_class_entry *LOWERNAME##_type;                        \
  zend_object_handlers *LOWERNAME##_handlers;                \
  PROTO_IMPL_DEFINE_FREE(CLASSNAME, LOWERNAME)               \
  PROTO_IMPL_DEFINE_DTOR(CLASSNAME, LOWERNAME)               \
  PROTO_IMPL_DEFINE_CREATE(CLASSNAME, LOWERNAME)             \
  PROTO_IMPL_DEFINE_INIT_CLASS(STRINGNAME, CLASSNAME, LOWERNAME)

#define PROTO_IMPL_DEFINE_CREATE(NAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_CREATE_START(NAME, LOWERNAME) \
  LOWERNAME##_init_c_instance(intern TSRMLS_CC); \
  PHP_PROTO_OBJECT_CREATE_END(NAME, LOWERNAME)
#define PROTO_IMPL_DEFINE_FREE(CAMELNAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_FREE_START(CAMELNAME, LOWERNAME) \
  LOWERNAME##_free_c(intern TSRMLS_CC);             \
  PHP_PROTO_OBJECT_FREE_END
#define PROTO_IMPL_DEFINE_DTOR(CAMELNAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_DTOR_START(CAMELNAME, LOWERNAME) \
  PHP_PROTO_OBJECT_DTOR_END
#define PROTO_IMPL_DEFINE_INIT_CLASS(CLASSNAME, CAMELNAME, LOWERNAME) \
  PHP_PROTO_INIT_CLASS_START(CLASSNAME, CAMELNAME, LOWERNAME)       \
  PHP_PROTO_INIT_CLASS_END

#define PHP_PROTO_INIT_CLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)         \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        CAMELNAME##_methods);                                \
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

///////////////////////////////////
#define PHP_PROTO_ALLOC_CLASS_OBJECT(class_object, class_type)               \
  class_object* intern;                                                      \
  int size = sizeof(class_object) + zend_object_properties_size(class_type); \
  intern = (class_object*)ecalloc(1, size);                                  \
  memset(intern, 0, size);

#define PHP_PROTO_FREE_CLASS_OBJECT(class_object, class_object_free, handler) \
  intern->std.handlers = handler;                                             \
  return &intern->std;

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

// Memory management
#define ALLOC(class_name) (class_name*) emalloc(sizeof(class_name))
#define PEMALLOC(class_name) (class_name*) pemalloc(sizeof(class_name), 1)
#define ALLOC_N(class_name, n) (class_name*) emalloc(sizeof(class_name) * n)
#define FREE(object) efree(object)
#define PEFREE(object) pefree(object, 1)
