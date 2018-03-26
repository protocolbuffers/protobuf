#ifndef __GOOGLE_PROTOBUF_PHP_PORT_PHP7_H__
#define __GOOGLE_PROTOBUF_PHP_PORT_PHP7_H__

#include "php.h"

// Define types.
#define PROTO_SIZE size_t
#define PROTO_LONG zend_long
#define PROTO_OBJECT zval

#define CACHED_VALUE zval
#define CACHED_VALUE_TO_ZVAL_PTR(VALUE) (&VALUE)
#define CACHED_VALUE_PTR_TO_ZVAL_PTR(VALUE) (VALUE)

#define PROTO_ZVAL_STRING(zval_ptr, s, copy) \
  ZVAL_STRING(zval_ptr, s)
#define PROTO_ZVAL_STRINGL(zval_ptr, s, len, copy) \
  ZVAL_STRINGL(zval_ptr, s, len)
#define PROTO_RETURN_STRINGL(s, len, copy) RETURN_STRINGL(s, len)
#define PROTO_RETVAL_STRINGL(s, len, copy) RETVAL_STRINGL(s, len)
#define php_proto_zend_make_printable_zval(from, to) \
  zend_make_printable_zval(from, to)

#define PROTO_Z_BVAL_P(VALUE) (Z_TYPE_INFO_P(VALUE) == IS_TRUE ? true : false)
#define PROTO_RETURN_BOOL(VALUE) RETURN_BOOL(VALUE);

#define PROTO_GLOBAL_UNINITIALIZED_ZVAL &EG(uninitialized_zval)

// Define wrapper class.
#define PROTO_CE_DECLARE zend_class_entry*
#define PROTO_CE_UNREF(ce) (ce)

static inline int php_proto_zend_lookup_class(
    const char* name, int name_length, zend_class_entry** ce TSRMLS_DC) {
  zend_string *zstr_name = zend_string_init(name, name_length, 0);
  *ce = zend_lookup_class(zstr_name);
  zend_string_release(zstr_name);
  return *ce != NULL ? SUCCESS : FAILURE;
}

# define PROTO_FORWARD_DECLARE_CLASS(CLASSNAME) \
  struct CLASSNAME

#define PROTO_WRAP_OBJECT_START(CLASSNAME) struct CLASSNAME {
#define PROTO_WRAP_OBJECT_END \
    zend_object std;          \
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
    CLASSNAME##_handlers->free_obj = CLASSNAME##_free;                      \
    CLASSNAME##_handlers->dtor_obj = CLASSNAME##_dtor;                      \
    CLASSNAME##_handlers->offset = XtOffsetOf(CLASSNAME, std);              \
    CLASSNAME##_init_handlers(CLASSNAME##_handlers);                        \
    CLASSNAME##_init_type(CLASSNAME##_type);
#define PROTO_INIT_CLASS_END \
  }
#define PROTO_OBJECT_FREE_START(CLASSNAME)     \
  void CLASSNAME##_free(zend_object* object) { \
    CLASSNAME* intern =                        \
        (CLASSNAME*)((char*)object - XtOffsetOf(CLASSNAME, std));
#define PROTO_OBJECT_FREE_END \
  }
#define PROTO_OBJECT_DTOR_START(CLASSNAME)     \
  void CLASSNAME##_dtor(zend_object* object) { \
    CLASSNAME* intern =                        \
        (CLASSNAME*)((char*)object - XtOffsetOf(CLASSNAME, std));
#define PROTO_OBJECT_DTOR_END               \
    zend_object_std_dtor(object TSRMLS_CC); \
  }
#define PROTO_OBJECT_CREATE_START(CLASSNAME)                               \
  static zend_object* CLASSNAME##_create(zend_class_entry* ce TSRMLS_DC) { \
    PROTO_ALLOC_CLASS_OBJECT(CLASSNAME, ce);                               \
    zend_object_std_init(&intern->std, ce TSRMLS_CC);                      \
    object_properties_init(&intern->std, ce);
#define PROTO_OBJECT_CREATE_END(CLASSNAME)                                        \
    PROTO_FREE_CLASS_OBJECT(CLASSNAME, CLASSNAME##_free, CLASSNAME##_handlers);   \
  }

// Coversion between php and cpp object.
#define UNBOX(class_name, val) \
  (class_name*)((char*)Z_OBJ_P(val) - XtOffsetOf(class_name, std));

///////////////////////////////////
#define PROTO_ALLOC_CLASS_OBJECT(class_object, class_type)                   \
  class_object* intern;                                                      \
  int size = sizeof(class_object) + zend_object_properties_size(class_type); \
  intern = (class_object*)ecalloc(1, size);                                  \
  memset(intern, 0, size);

#define PROTO_FREE_CLASS_OBJECT(class_object, class_object_free, handler) \
  intern->std.handlers = handler;                                         \
  return &intern->std;

/////////////////////////////////////

#define ARENA zend_object*

#define UNBOX_ARENA(WRAPPER) \
  (Arena*)((char*)WRAPPER - XtOffsetOf(Arena, std))

#define ARENA_INIT(WRAPPER, INTERN)                \
{                                                  \
  ARENA phparena;                                  \
  WRAPPER = Arena_type->create_object(Arena_type); \
  Arena *cpparena = UNBOX_ARENA(WRAPPER); \
  INTERN = cpparena->arena;                        \
}

#define ARENA_ADDREF(WRAPPER) \
  ++GC_REFCOUNT(WRAPPER)

#define ARENA_DTOR(WRAPPER)            \
  {                                    \
    if(--GC_REFCOUNT(WRAPPER) == 0) {  \
      zend_objects_store_del(WRAPPER); \
    }                                  \
  }

/////////////////////////////////////

#define PHP_OBJECT zend_object*

#define ZVAL_PTR_TO_PHP_OBJECT(ZPTR) \
  Z_OBJ_P(ZPTR)

#define PHP_OBJECT_NEW(DEST, TYPE)        \
  {                                       \
    DEST = (TYPE)->create_object((TYPE)); \
  }

#define PHP_OBJECT_FREE(OBJ)       \
  {                                \
    if(--GC_REFCOUNT(OBJ) == 0) {  \
      zend_objects_store_del(OBJ); \
    }                              \
  }

#define PHP_OBJECT_ADDREF(DEST) \
  ++GC_REFCOUNT(DEST)

#define PHP_OBJECT_DELREF(DEST) \
  --GC_REFCOUNT(DEST)

#define PHP_OBJECT_UNBOX(TYPE, DEST) \
  (TYPE*)((char*)DEST - XtOffsetOf(TYPE, std))

#define RETURN_PHP_OBJECT(OBJ)   \
  {                              \
    ++GC_REFCOUNT(OBJ);          \
    ZVAL_OBJ(return_value, OBJ); \
  }


#endif  // __GOOGLE_PROTOBUF_PHP_PORT_PHP7_H__
