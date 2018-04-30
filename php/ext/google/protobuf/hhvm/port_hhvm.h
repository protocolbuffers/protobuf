#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/vm/native-data.h"
using namespace HPHP;

// Define types.
#define PROTO_SIZE size_t
#define PROTO_OBJECT Object
#define PHP_OBJECT Object*

// Define class.
#define PROTO_CLASS Class 
#define CLASS Class*

#define PROTO_FORWARD_DECLARE_CLASS(CLASSNAME) \
  class CLASSNAME

#define PROTO_WRAP_OBJECT_START(CLASSNAME) \
  class CLASSNAME {                        \
   public:                                 \
    CLASSNAME() {                          \
      CLASSNAME##_init_c_instance(this);   \
    }                                      \
    ~CLASSNAME() {                         \
      CLASSNAME##_free_c(this);            \
    }
#define PROTO_WRAP_OBJECT_END \
  }; 
#define PROTO_INIT_CLASS_START(STRINGNAME, CLASSNAME) \
  const StaticString s_##CLASSNAME(STRINGNAME);       \
  void CLASSNAME##_init() {                           \
    CLASSNAME##_register_methods();                   \
    Native::registerNativeDataInfo<CLASSNAME>(s_##CLASSNAME.get()); 
#define PROTO_INIT_CLASS_END \
  }
#define PROTO_INIT_CLASS(CLASSNAME) CLASSNAME##_init()

#define PROTO_DEFINE_INIT_CLASS(STRINGNAME, CLASSNAME) \
  PROTO_INIT_CLASS_START(STRINGNAME, CLASSNAME)        \
  PROTO_INIT_CLASS_END

#define PROTO_REGISTER_CLASS_METHODS_START(CLASSNAME) \
  void CLASSNAME##_register_methods() {
#define PROTO_REGISTER_CLASS_METHODS_END \
  }

#define PROTO_DEFINE_CLASS(CLASSNAME, STRINGNAME) \
  PROTO_DEFINE_INIT_CLASS(STRINGNAME, CLASSNAME)


// Utilities for defining method.
#define PROTO_METHOD(CLASSNAME, METHODNAME, RETURN_TYPE) \
  RETURN_TYPE HHVM_METHOD(CLASSNAME, METHODNAME)

#define PROTO_REGISTER_METHOD(FULLNAME, CLASSNAME, METHODNAME) \
  HHVM_NAMED_ME(FULLNAME, METHODNAME, HHVM_MN(CLASSNAME, METHODNAME));

#define PROTO_RETURN_BOOL(VALUE) return VALUE;

// Fake macro in php
#define TSRMLS_DC
#define TSRMLS_D
#define TSRMLS_CC
#define TSRMLS_C

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

// Memory management
#define ALLOC(class_name) (class_name*) malloc(sizeof(class_name))
#define ALLOC_N(class_name, n) (class_name*) malloc(sizeof(class_name) * n)
#define FREE(object) free(object)

// Error report
#define CHECK_UPB(code, msg)        \
  do {                              \
    upb_status status;              \
    code;                           \
    if (!upb_ok(&status)) {         \
    }                               \
  } while (0)

// Create and Delete Object
#define PHP_OBJECT_NEW(DEST, WRAPPER, TYPE) \
  {                                         \
    WRAPPER = new Object(Unit::loadClass(s_ ## TYPE.get())); \
    DEST = Native::data<TYPE>(*WRAPPER);    \
  }

#define UNBOX(TYPE, WRAPPER) Native::data<TYPE>(*WRAPPER)

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

#define ARENA Object*

#define ARENA_INIT(WRAPPER, INTERN)                     \
{                                                       \
  WRAPPER = new Object(Unit::loadClass(s_Arena.get())); \
  Arena *cpparena = UNBOX(Arena, WRAPPER);              \
  INTERN = cpparena->arena;                             \
}

#define ARENA_ADDREF(WRAPPER)

#define ARENA_DTOR(WRAPPER)

#define UNBOX_ARENA(WRAPPER) UNBOX(Arena, WRAPPER)

// -----------------------------------------------------------------------------
// Forward Declare
// -----------------------------------------------------------------------------
extern const StaticString s_Arena;

#define PHP_OBJECT_FREE(OBJ)
