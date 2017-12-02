#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/vm/native-data.h"
using namespace HPHP;

// Define class.
#define PROTO_WRAP_OBJECT_START(name) \
  class name { \
   public:
#define PROTO_WRAP_OBJECT_END \
  }; 
#define PROTO_INIT_CLASS_START(STRINGNAME, CLASSNAME, LOWWERNAME) \
  const StaticString s_##CLASSNAME(STRINGNAME);                   \
  void LOWWERNAME##_init() {                                      \
    CLASSNAME##_register_methods();                               \
    Native::registerNativeDataInfo<CLASSNAME>(s_##CLASSNAME.get()); 
#define PROTO_INIT_CLASS_END \
  }
#define PROTO_INIT_CLASS(LOWWERNAME) LOWWERNAME##_init()

#define PROTO_DEFINE_INIT_CLASS(STRINGNAME, CLASSNAME, LOWERNAME) \
  PROTO_INIT_CLASS_START(STRINGNAME, CLASSNAME, LOWERNAME)        \
  PROTO_INIT_CLASS_END

#define PROTO_REGISTER_CLASS_METHODS_START(CLASSNAME) \
  void CLASSNAME##_register_methods() {
#define PROTO_REGISTER_CLASS_METHODS_END \
  }

#define PROTO_DEFINE_CLASS(CLASSNAME, LOWERNAME, STRINGNAME) \
  PROTO_DEFINE_INIT_CLASS(STRINGNAME, CLASSNAME, LOWERNAME)


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
