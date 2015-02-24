#ifndef __PROTOBUF_NODEJS_SRC_REPEATEDFIELD_H__
#define __PROTOBUF_NODEJS_SRC_REPEATEDFIELD_H__

#include <nan.h>
#include <string>
#include <map>
#include <assert.h>
#include "defs.h"
#include "util.h"
#include "jsobject.h"

namespace protobuf_js {

class RepeatedField : public JSObject {
 public:
  JS_OBJECT(RepeatedField);

  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;
  static v8::Persistent<v8::Value> prototype;

  upb_fieldtype_t type() const { return type_; }
  Descriptor* submsg() const { return submsg_; }
  EnumDescriptor* subenum() const { return subenum_; }

  int Length() const {
    return values_.size();
  }
  v8::Handle<v8::Value> operator[](int index) {
    NanEscapableScope();
    return NanEscapeScope(NanNew(values_[index]));
  }

  // Public for use during message parsing.
  bool DoPush(v8::Local<v8::Value> value, bool allow_copy);

 private:
  RepeatedField();
  ~RepeatedField()  {}

  // Methods
  static NAN_METHOD(New);
  static NAN_GETTER(LengthGetter);
  static NAN_INDEX_GETTER(IndexGetter);
  static NAN_INDEX_SETTER(IndexSetter);
  static NAN_INDEX_DELETER(IndexDeleter);

  // Mostly mirroring the standard set of methods on Array:
  static NAN_METHOD(Pop);
  static NAN_METHOD(Push);
  static NAN_METHOD(Shift);
  static NAN_METHOD(Unshift);
  static NAN_METHOD(ToString);

  // .resize() doesn't exist on ordinary JavaScript arrays because they are
  // auto-expanding, but we avoid auto-expanding behavior here because we would
  // have to fill with a properly typed value rather than `undefined` as JS
  // does (and we don't want inconsistent semantics). The explicit resize() call
  // is better and makes clear that we will fill with some appropriate default
  // value.
  static NAN_METHOD(Resize);

  // Creates a new RepeatedField of this type, with no contents.
  static NAN_METHOD(NewEmpty);

  // Provides the element type of this field.
  static NAN_GETTER(TypeGetter);
  // Provides the sub-type (message or enum descriptor).
  static NAN_GETTER(SubDescGetter);

  // Helpers
  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);

  std::vector< PERSISTENT(v8::Value) > values_;
  upb_fieldtype_t type_;
  Descriptor* submsg_;
  EnumDescriptor* subenum_;
};

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_REPEATEDFIELD_H__
