#include <nan.h>
#include "enum.h"

using namespace v8;
using namespace node;

namespace protobuf_js {

JS_OBJECT_INIT(ProtoEnum);

Persistent<Function> ProtoEnum::constructor;

// TODO: make sure enum name is shown in .toString() for messages.

void ProtoEnum::Init(Handle<Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>("ProtoEnum"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);
  NanAssignPersistent(constructor, tpl->GetFunction());
}

NAN_METHOD(ProtoEnum::New) {
  NanEscapableScope();
  if (args.IsConstructCall()) {
    ProtoEnum* self = new ProtoEnum();
    self->Wrap<ProtoEnum>(args.This());
    if (!self->HandleCtorArgs(args)) {
      NanReturnUndefined();
    }
    self->FillEnumValues(args.This());
    NanReturnValue(args.This());
  } else {
    NanThrowError("Must be called as constructor");
    NanReturnUndefined();
  }
}

bool ProtoEnum::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();
  if (args.Length() != 1) {
    NanThrowError("Expected one constructor arg: an EnumDescriptor instance");
    return false;
  }
  if (!args[0]->IsObject()) {
    NanThrowError("First constructor arg must be an object");
    return false;
  }
  Local<Object> enumdesc_obj = NanNew(args[0]->ToObject());
  if (enumdesc_obj->GetPrototype() != EnumDescriptor::prototype) {
    NanThrowError("Expected an EnumDescriptor instance as constructor arg");
    return false;
  }

  NanAssignPersistent(enumdesc_obj_, enumdesc_obj);
  enumdesc_ = EnumDescriptor::unwrap(enumdesc_obj);

  return true;
}

void ProtoEnum::FillEnumValues(v8::Handle<v8::Object> this_) {
  NanEscapableScope();
  for (upb::EnumDef::Iterator it(enumdesc_->enumdef());
       !it.Done(); it.Next()) {
    this_->Set(NanNew<String>(it.name()),
               NanNew<Integer>(it.number()));
  }
}

}  // namespace protobuf_js
