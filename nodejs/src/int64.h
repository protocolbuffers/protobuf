#ifndef __PROTOBUF_NODEJS_SRC_INT64_H__
#define __PROTOBUF_NODEJS_SRC_INT64_H__

#include <node.h>
#include <v8.h>
#include <string>
#include "util.h"
#include "jsobject.h"

namespace protobuf_js {

// This C++ class implements both protobuf.Int64 and protobuf.UInt64 in order to
// reduce code duplication.
class Int64 : public JSObject {
 public:
  JS_OBJECT(Int64);

  static void Init(v8::Handle<v8::Object> exports);

  static v8::Persistent<v8::Function> constructor_signed;
  static v8::Persistent<v8::Value> prototype_signed;
  static v8::Persistent<v8::Function> constructor_unsigned;
  static v8::Persistent<v8::Value> prototype_unsigned;

  static v8::Local<v8::Object> NewInt64(int64_t value) {
    NanEscapableScope();
    v8::Local<v8::Function> ctor = NanNew(constructor_signed);
    v8::Local<v8::Object> self = ctor->NewInstance(0, NULL);
    unwrap(self)->value_.s = value;
    return NanEscapeScope(self);
  }

  static v8::Local<v8::Object> NewUInt64(uint64_t value) {
    NanEscapableScope();
    v8::Local<v8::Function> ctor = NanNew(constructor_unsigned);
    v8::Local<v8::Object> self = ctor->NewInstance(0, NULL);
    unwrap(self)->value_.u = value;
    return NanEscapeScope(self);
  }

  int64_t int64_value() const {
    return value_.s;
  }
  uint64_t uint64_value() const {
    return value_.u;
  }

  void set_int64_value(int64_t value) {
    value_.s = value;
  }

  void set_uint64_value(uint64_t value) {
    value_.u = value;
  }

  static bool IsSigned(v8::Local<v8::Object> _this) {
    return _this->GetPrototype() == prototype_signed;
  }
  bool IsSigned() const {
    return const_cast<Int64*>(this)->object()->
        GetPrototype() == prototype_signed;
  }

 private:
  Int64() { value_.s = 0; }
  ~Int64() {}

  static v8::Handle<v8::Function> MakeInt64Class(std::string name,
                                                 bool is_signed);

  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);
  bool DoSet(_NAN_METHOD_ARGS_TYPE args);

  static NAN_METHOD(New);
  static NAN_METHOD(ToString);

  // ES7 UInt64/Int64 compatibility: see
  // http://wiki.ecmascript.org/doku.php?id=harmony:binary_data_discussion
  //
  // These are class methods, so the user writes, e.g.:
  // UInt64.lo(myvalue), or UInt64.join(0xffff, 0xffffffff).
  static NAN_METHOD(Lo);
  static NAN_METHOD(Hi);
  static NAN_METHOD(Join);
  static NAN_METHOD(Compare);

  union {
    int64_t s;
    uint64_t u;
  } value_;
};

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_INT64_H__
