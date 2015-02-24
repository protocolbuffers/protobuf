#include <v8.h>
#include "readonlyarray.h"
#include "util.h"

using namespace v8;
using namespace node;

namespace protobuf_js {

Persistent<Function> ReadOnlyArray::constructor;

JS_OBJECT_INIT(ReadOnlyArray);

void ReadOnlyArray::Init(Handle<Object> exports) {
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->SetClassName(NanNew<String>("ReadOnlyArray"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->InstanceTemplate()->SetAccessor(
      NanNew<String>("length"), LengthGetter);
  tpl->InstanceTemplate()->SetIndexedPropertyHandler(
      IndexGetter, IndexSetter, 0, IndexDeleter, 0);

  tpl->InstanceTemplate()->SetAccessor(
      NanNew<String>("value"), ValueGetter);
  tpl->InstanceTemplate()->SetAccessor(
      NanNew<String>("done"), DoneGetter);
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("next"), NanNew<FunctionTemplate>(Next)->GetFunction());

  NanAssignPersistent(constructor, tpl->GetFunction());
  exports->Set(NanNew<String>("ReadOnlyArray"), NanNew(constructor));
}

ReadOnlyArray::ReadOnlyArray() {
  iterator_index_ = -1;
  NanAssignPersistent(array_, NanNew<Array>());
}

ReadOnlyArray::ReadOnlyArray(Handle<Array> array) {
  iterator_index_ = -1;
  NanAssignPersistent(array_, array);
}

ReadOnlyArray::ReadOnlyArray(int32_t size) {
  iterator_index_ = -1;
  NanAssignPersistent(array_, NanNew<Array>(size));
}

Handle<Value> ReadOnlyArray::Create() {
  NanEscapableScope();
  return NanEscapeScope(NanNew(constructor)->NewInstance());
}

Handle<Value> ReadOnlyArray::Create(
    const std::vector< v8::Handle<v8::Value> >& values) {
  NanEscapableScope();
  Local<Value> argv[1] = { NanNew<Integer>(values.size()) };
  Local<Value> ret = NanNew(constructor)->NewInstance(1, argv);
  ReadOnlyArray* self = unwrap(Handle<Object>::Cast(ret));
  Local<Array> arr = NanNew(self->array_);

  for (int i = 0; i < values.size(); i++) {
    arr->Set(i, values[i]);
  }

  return NanEscapeScope(ret);
}

NAN_METHOD(ReadOnlyArray::New) {
  NanEscapableScope();
  if (!args.IsConstructCall()) {
    NanThrowError("Not called as constructor");
    NanReturnUndefined();
  }

  ReadOnlyArray* self;
  if (args.Length() == 1 && args[0]->IsArray()) {
    self = new ReadOnlyArray(Handle<Array>::Cast(args[0]));
  } else if (args.Length() == 1 && args[0]->IsInt32()) {
    self = new ReadOnlyArray(args[0]->Int32Value());
  } else if (args.Length() == 0) {
    self = new ReadOnlyArray();
  } else {
    NanThrowError("Too many arguments to constructor, or arg is not array");
    NanReturnUndefined();
  }
  self->Wrap<ReadOnlyArray>(args.This());
  NanReturnValue(args.This());
}

NAN_GETTER(ReadOnlyArray::LengthGetter) {
  NanEscapableScope();
  ReadOnlyArray* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  NanReturnValue(NanNew<Number>(NanNew(self->array_)->Length()));
}

NAN_INDEX_GETTER(ReadOnlyArray::IndexGetter) {
  NanEscapableScope();
  ReadOnlyArray* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  Local<Array> arr = NanNew(self->array_);
  if (index >= arr->Length()) {
    NanReturnUndefined();
  }
  NanReturnValue(NanNew(arr->Get(index)));
}

NAN_INDEX_SETTER(ReadOnlyArray::IndexSetter) {
  NanEscapableScope();
  NanThrowError("ReadOnlyArray: elements cannot be changed");
  NanReturnValue(NanFalse());
}

NAN_INDEX_DELETER(ReadOnlyArray::IndexDeleter) {
  NanEscapableScope();
  NanThrowError("ReadOnlyArray: elements cannot be deleted");
  NanReturnValue(NanFalse());
}

NAN_GETTER(ReadOnlyArray::ValueGetter) {
  NanEscapableScope();
  ReadOnlyArray* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  Local<Array> arr = NanNew(self->array_);
  if (self->iterator_index_ < 0) {
    NanThrowError("Iterator not started yet");
    NanReturnUndefined();
  }
  if (self->iterator_index_ >= arr->Length()) {
    NanReturnUndefined();
  } else {
    NanReturnValue(NanNew(arr->Get(self->iterator_index_)));
  }
}

NAN_GETTER(ReadOnlyArray::DoneGetter) {
  NanEscapableScope();
  ReadOnlyArray* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  Local<Array> arr = NanNew(self->array_);
  if (self->iterator_index_ < 0) {
    NanThrowError("Iterator not started yet");
    NanReturnUndefined();
  }
  NanReturnValue(
      (self->iterator_index_ >= arr->Length()) ?
      NanTrue() : NanFalse());
}

NAN_METHOD(ReadOnlyArray::Next) {
  NanEscapableScope();
  ReadOnlyArray* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  Local<Array> arr = NanNew(self->array_);
  if (self->iterator_index_ == -1 ||
      self->iterator_index_ < arr->Length()) {
    self->iterator_index_++;
  }
  NanReturnValue(args.This());
}

}  // namespace protobuf_js
