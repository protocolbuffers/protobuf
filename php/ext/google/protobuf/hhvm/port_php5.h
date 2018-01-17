#ifndef __GOOGLE_PROTOBUF_PHP_PORT_PHP5_H__
#define __GOOGLE_PROTOBUF_PHP_PORT_PHP5_H__

#include "php.h"

// Define types.
#define PROTO_SIZE int
#define PROTO_LONG long
#define PROTO_OBJECT zval

#define CACHED_VALUE zval*
#define CACHED_VALUE_TO_ZVAL_PTR(VALUE) (VALUE)
#define CACHED_VALUE_PTR_TO_ZVAL_PTR(VALUE) (*VALUE)

#define PROTO_ZVAL_STRING(zval_ptr, s, copy) \
  ZVAL_STRING(zval_ptr, s, copy)
#define PROTO_ZVAL_STRINGL(zval_ptr, s, len, copy) \
  ZVAL_STRINGL(zval_ptr, s, len, copy)
#define PROTO_RETURN_STRING(s, copy) RETURN_STRING(s, copy)
#define PROTO_RETURN_STRINGL(s, len, copy) RETURN_STRINGL(s, len, copy)

#define PROTO_Z_BVAL_P(VALUE) Z_BVAL_P(VALUE)
#define PROTO_RETURN_BOOL(VALUE) RETURN_BOOL(VALUE);

#define PROTO_GLOBAL_UNINITIALIZED_ZVAL EG(uninitialized_zval_ptr)

#define ZVAL_OBJ(zval_ptr, call_create) \
  Z_TYPE_P(zval_ptr) = IS_OBJECT;       \
  Z_OBJVAL_P(zval_ptr) = call_create;
#define Z_OBJ_P(zval_p)                                       \
  ((zend_object*)(EG(objects_store)                           \
                      .object_buckets[Z_OBJ_HANDLE_P(zval_p)] \
                      .bucket.obj.object))
#define OBJ_PROP(OBJECT, OFFSET) &((OBJECT)->properties_table[OFFSET])

// Define wrapper class.
#define PROTO_CE_DECLARE zend_class_entry**
#define PROTO_CE_UNREF(ce) (*ce)

#define php_proto_zend_lookup_class(name, name_length, ce) \
  zend_lookup_class(name, name_length, ce TSRMLS_CC)

# define PROTO_FORWARD_DECLARE_CLASS(CLASSNAME) \
  struct CLASSNAME

#define PROTO_WRAP_OBJECT_START(name) \
  struct name {                       \
    zend_object std;
#define PROTO_WRAP_OBJECT_END \
  };

#define PROTO_INIT_CLASS_START(FULLNAME, CLASSNAME)                         \
  void CLASSNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                            \
    const char* class_name = FULLNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, FULLNAME, strlen(FULLNAME),             \
                        CLASSNAME##_methods);                               \
    CLASSNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    CLASSNAME##_type->create_object = CLASSNAME##_create;                   \
    CLASSNAME##_handlers = PEMALLOC(zend_object_handlers);                  \
    memcpy(CLASSNAME##_handlers, zend_get_std_object_handlers(),            \
           sizeof(zend_object_handlers));                                   \
    CLASSNAME##_init_handlers(CLASSNAME##_handlers);                        \
    CLASSNAME##_init_type(CLASSNAME##_type);
#define PROTO_INIT_CLASS_END \
  }

#define PROTO_OBJECT_FREE_START(CLASSNAME)        \
  void CLASSNAME##_free(void* object TSRMLS_DC) { \
    CLASSNAME* intern = (CLASSNAME*)object;
#define PROTO_OBJECT_FREE_END                     \
    zend_object_std_dtor(&intern->std TSRMLS_CC); \
    efree(intern);                                \
  }

#define PROTO_OBJECT_DTOR_START(CLASSNAME)
#define PROTO_OBJECT_DTOR_END

#define PROTO_OBJECT_CREATE_START(CLASSNAME)          \
  static zend_object_value CLASSNAME##_create(        \
      zend_class_entry* ce TSRMLS_DC) {               \
    PROTO_ALLOC_CLASS_OBJECT(CLASSNAME, ce);          \
    zend_object_std_init(&intern->std, ce TSRMLS_CC); \
    object_properties_init(&intern->std, ce);
#define PROTO_OBJECT_CREATE_END(CLASSNAME)                                      \
    PROTO_FREE_CLASS_OBJECT(CLASSNAME, CLASSNAME##_free, CLASSNAME##_handlers); \
  }

#define PROTO_ALLOC_CLASS_OBJECT(class_object, class_type) \
  class_object* intern;                                    \
  intern = (class_object*)emalloc(sizeof(class_object));   \
  memset(intern, 0, sizeof(class_object));

#define PROTO_FREE_CLASS_OBJECT(class_object, class_object_free, handler) \
  zend_object_value retval = {0};                                         \
  retval.handle = zend_objects_store_put(                                 \
      intern, (zend_objects_store_dtor_t)zend_objects_destroy_object,     \
      class_object_free, NULL TSRMLS_CC);                                 \
  retval.handlers = handler;                                              \
  return retval;

// Coversion between php and cpp object.
#define UNBOX(class_name, val) \
  (class_name*)zend_object_store_get_object(val TSRMLS_CC);

#endif  // __GOOGLE_PROTOBUF_PHP_PORT_PHP5_H__
