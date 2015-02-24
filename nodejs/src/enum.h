#ifndef __PROTOBUF_NODEJS_SRC_ENUM_H__
#define __PROTOBUF_NODEJS_SRC_ENUM_H__

#include <nan.h>
#include "defs.h"
#include "util.h"
#include "jsobject.h"

namespace protobuf_js {

class ProtoEnum : public JSObject {
 public:
  JS_OBJECT(ProtoEnum);

  static void Init(v8::Handle<v8::Object> exports);

  static v8::Persistent<v8::Function> constructor;

 private:
  ProtoEnum() : enumdesc_(NULL) {}
  ~ProtoEnum() {}

  static NAN_METHOD(New);

  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);
  void FillEnumValues(v8::Handle<v8::Object> this_);

  EnumDescriptor* enumdesc_;
  v8::Persistent<v8::Object> enumdesc_obj_;
};

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_ENUM_H__
