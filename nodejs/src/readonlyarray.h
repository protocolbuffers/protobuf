#ifndef __PROTOBUF_NODEJS_SRC_READONLYARRAY_H__
#define __PROTOBUF_NODEJS_SRC_READONLYARRAY_H__

#include <node.h>
#include <nan.h>
#include <vector>
#include "util.h"
#include "jsobject.h"

namespace protobuf_js {

class ReadOnlyArray : public JSObject {
 public:
  JS_OBJECT(ReadOnlyArray);

  static void Init(v8::Handle<v8::Object> exports);

  static v8::Handle<v8::Value> Create();
  static v8::Handle<v8::Value> Create(
      const std::vector< v8::Handle<v8::Value> >& values);

  class Builder {
   public:
    v8::Handle<v8::Value> Build() {
      NanEscapableScope();
      return NanEscapeScope(NanNew(ReadOnlyArray::Create(elems_)));
    }

    void Add(v8::Handle<v8::Value> value) {
      elems_.push_back(value);
    }

   private:
    // This is an array of raw Handles, but a Builder is only used within one
    // HandleScope so everything should be fine. The objects are added to the
    // array's contents and subsequently kept alive when Build is called in
    // this HandleScope.
    std::vector< v8::Handle<v8::Value> > elems_;
  };

 private:
  ReadOnlyArray();
  explicit ReadOnlyArray(v8::Handle<v8::Array> array);
  explicit ReadOnlyArray(int32_t size);
  ~ReadOnlyArray()  {}

  static NAN_METHOD(New);
  static NAN_GETTER(LengthGetter);
  static NAN_INDEX_GETTER(IndexGetter);
  static NAN_INDEX_SETTER(IndexSetter);
  static NAN_INDEX_DELETER(IndexDeleter);

  // Iteration protocol: the ReadOnlyArray itself acts as the iterator object.
  // It exposes "value" and "done" properties and a "next" method that advances
  // the iterator pointer and returns itself.
  static NAN_GETTER(ValueGetter);
  static NAN_GETTER(DoneGetter);
  static NAN_METHOD(Next);

  static v8::Persistent<v8::Function> constructor;

  v8::Persistent<v8::Array> array_;
  int iterator_index_;
};

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_READONLYARRAY_H__
