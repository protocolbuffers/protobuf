// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <nan.h>
#include "defs.h"
#include "message.h"
#include "repeatedfield.h"
#include "util.h"

#include <assert.h>
#include <vector>
#include <algorithm>

using namespace v8;
using namespace node;

namespace protobuf_js {

Persistent<Function> RepeatedField::constructor;
Persistent<Value> RepeatedField::prototype;

JS_OBJECT_INIT(RepeatedField);

void RepeatedField::Init(Handle<Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->SetClassName(NanNew<String>("RepeatedField"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->InstanceTemplate()->SetAccessor(
      NanNew("length"), LengthGetter);
  tpl->InstanceTemplate()->SetAccessor(
      NanNew("type"), TypeGetter);
  tpl->InstanceTemplate()->SetAccessor(
      NanNew("subdesc"), SubDescGetter);
  tpl->InstanceTemplate()->SetIndexedPropertyHandler(
      IndexGetter, IndexSetter, 0, IndexDeleter, 0);

#define M(jsname, cppname)                                   \
  tpl->PrototypeTemplate()->Set(                             \
      NanNew<String>(#jsname),                               \
      NanNew<FunctionTemplate>(cppname)->GetFunction())

  M(pop, Pop);
  M(push, Push);
  M(shift, Shift);
  M(unshift, Unshift);
  M(toString, ToString);
  M(resize, Resize);
  M(newEmpty, NewEmpty);

#undef M

  NanAssignPersistent(constructor, tpl->GetFunction());
  // Construct an instance in order to get the prototype object.
  Local<Value> arg = NanNew<Integer>(UPB_TYPE_INT32);
  NanAssignPersistent(prototype,
                      NanNew(constructor)->NewInstance(1, &arg)->GetPrototype());

  exports->Set(NanNew<String>("RepeatedField"), NanNew(constructor));
}

RepeatedField::RepeatedField() {
  // Defaults: will be replaced by HandleCtorArgs() after object is constructed
  // and attached to the JS object.
  type_ = UPB_TYPE_INT32;
  submsg_ = NULL;
  subenum_ = NULL;
}

NAN_METHOD(RepeatedField::New) {
  NanEscapableScope();
  if (!args.IsConstructCall()) {
    NanThrowError("Not called as constructor");
    NanReturnUndefined();
  }

  RepeatedField* self = new RepeatedField();
  self->Wrap<RepeatedField>(args.This());
  if (!self->HandleCtorArgs(args)) {
    NanReturnUndefined();
  }
  NanReturnValue(args.This());
}

bool RepeatedField::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();

  if (args.Length() == 0) {
    NanThrowError("Expected at least one arg to RepeatedField constructor "
                  "(field type, or field type and message class)");
    return false;
  } else if (args.Length() >= 1 && args.Length() <= 3) {
    // One-arg form:   (FieldDescriptor.TYPE_***).
    // Two-arg form:   (FieldDescriptor.TYPE_MESSAGE, MessageClass/Desc).
    // Two-arg form:   (FieldDescriptor.TYPE_***, init_array).
    // Three-arg form: (FieldDescriptor.TYPE_MESSAGE, MessageClass/Desc,
    //                  init_array).
    Local<Value> type_value = NanNew(args[0]);
    if (!FieldDescriptor::ParseTypeValue(type_value, &type_)) {
      return false;
    }

    if (type_ == UPB_TYPE_MESSAGE && args.Length() > 1) {
      Local<Value> msgclass_or_desc = NanNew(args[1]);
      Local<Object> descriptor;
      if (msgclass_or_desc->IsFunction()) {
        Local<Function> msgclass_fn = msgclass_or_desc.As<Function>();
        Local<Value> descriptorval =
            NanNew(msgclass_fn->Get(NanNew<String>("descriptor")));
        if (!descriptorval->IsObject()) {
          NanThrowError("No descriptor property on message class");
          return false;
        }
        descriptor = NanNew(descriptorval->ToObject());
      } else if (msgclass_or_desc->IsObject()) {
        descriptor = msgclass_or_desc->ToObject();
      } else {
        NanThrowError("Expected message class or descriptor as second "
                      "argument to RepeatedField constructor");
        return false;
      }

      if (descriptor->GetPrototype() != Descriptor::prototype) {
        NanThrowError("Invalid descriptor object");
        return false;
      }

      submsg_ = Descriptor::unwrap(descriptor);
    } else {
      submsg_ = NULL;
    }

    if (type_ == UPB_TYPE_ENUM && args.Length() > 1) {
      if (!args[1]->IsObject()) {
        NanThrowError("Expected EnumDescriptor or enum object");
        return false;
      }
      Local<Object> enumobj_or_desc = NanNew(args[1]->ToObject());
      Local<Object> descriptor;
      if (enumobj_or_desc->GetPrototype() == EnumDescriptor::prototype) {
        descriptor = enumobj_or_desc;
      } else {
        Local<Value> descriptor_prop = enumobj_or_desc->Get(
            NanNew<String>("descriptor"));
        if (descriptor_prop->IsObject() &&
            descriptor_prop->ToObject()->GetPrototype() ==
            EnumDescriptor::prototype) {
          descriptor = descriptor_prop->ToObject();
        }
      }

      if (descriptor.IsEmpty()) {
        NanThrowError("Expected enum object or descriptor");
        return false;
      }

      subenum_ = EnumDescriptor::unwrap(descriptor);
    }

    if (type_ == UPB_TYPE_MESSAGE && !submsg_) {
      NanThrowError("RepeatedField created with message type but no submsg");
      return false;
    }
    if (type_ == UPB_TYPE_ENUM && !subenum_) {
      NanThrowError("RepeatedField created with enum type but no subenum");
      return false;
    }

    int init_array_idx = (submsg_ != NULL || subenum_ != NULL) ? 2 : 1;
    if (args.Length() > init_array_idx) {
      Local<Value> init_array_val = NanNew(args[init_array_idx]);
      if (!init_array_val->IsArray()) {
        NanThrowError(
            "Expected array as last arg to RepeatedField constructor");
        return false;
      }
      Local<Array> init_array = init_array_val.As<Array>();

      for (int i = 0; i < init_array->Length(); i++) {
        if (!DoPush(init_array->Get(i), /* allow_copy = */ false)) {
          return false;
        }
      }
    }

    return true;
  } else {
    NanThrowError("Too many args to RepeatedField constructor");
    return false;
  }
}

NAN_GETTER(RepeatedField::LengthGetter) {
  NanEscapableScope();
  RepeatedField* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  NanReturnValue(NanNew<Integer>(self->values_.size()));
}

NAN_INDEX_GETTER(RepeatedField::IndexGetter) {
  NanEscapableScope();
  RepeatedField* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  if (index < self->values_.size()) {
    NanReturnValue(NanNew(self->values_[index]));
  } else {
    // Out-of-bounds access
    NanReturnUndefined();
  }
}

NAN_INDEX_SETTER(RepeatedField::IndexSetter) {
  NanEscapableScope();
  RepeatedField* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }

  if (index >= self->values_.size()) {
    NanThrowError("Out-of-bounds assignment to repeated field");
    NanReturnUndefined();
  }

  Local<Value> converted_val =
      NanNew(ProtoMessage::CheckConvertElement(self->type_,
                                               self->submsg_,
                                               value,
                                               /* allow_null = */ false,
                                               /* allow_copy = */ false));
  if (converted_val.IsEmpty() || converted_val->IsUndefined()) {
    NanReturnUndefined();
  }

  NanAssignPersistent(self->values_[index], converted_val);
  NanReturnValue(converted_val);
}

NAN_INDEX_DELETER(RepeatedField::IndexDeleter) {
  NanEscapableScope();
  NanThrowError("Delete not supported on a repeated field element");
  NanReturnValue(NanTrue());  // true == intercepted request
}

NAN_METHOD(RepeatedField::Pop) {
  NanEscapableScope();
  RepeatedField* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  if (self->values_.empty()) {
    NanReturnUndefined();
  } else {
    Local<Value> last_obj = NanNew(self->values_.back());
    self->values_.pop_back();
    NanReturnValue(last_obj);
  }
}

bool RepeatedField::DoPush(Local<Value> value, bool allow_copy) {
  NanEscapableScope();
  if (value->IsNull()) {
    NanThrowError("Cannot set a value to null in a repeated field");
    return false;
  }
  Local<Value> converted = NanNew(
      ProtoMessage::CheckConvertElement(type_, submsg_, value,
                                        /* allow_null = */ false,
                                        allow_copy));
  if (converted.IsEmpty() || converted->IsUndefined()) {
    return false;
  }

  values_.resize(values_.size() + 1);
  NanAssignPersistent(values_.back(), converted);
  return true;
}

NAN_METHOD(RepeatedField::Push) {
  NanEscapableScope();
  RepeatedField* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  if (args.Length() != 1) {
    NanThrowError("Push expects one argument");
    NanReturnUndefined();
  }
  if (!self->DoPush(args[0], /* allow_copy = */ false)) {
    NanReturnUndefined();
  }
  NanReturnValue(args[0]);
}

NAN_METHOD(RepeatedField::Shift) {
  NanEscapableScope();
  RepeatedField* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  CheckArgs checkargs(args);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  if (self->values_.empty()) {
    NanReturnUndefined();
  }

  Local<Value> front_val = NanNew(self->values_.front());
  for (int i = 0; i < self->values_.size() - 1; i++) {
    NanAssignPersistent(self->values_[i],
            NanNew(self->values_[i + 1]));
  }
  self->values_.resize(self->values_.size() - 1);

  NanReturnValue(front_val);
}

NAN_METHOD(RepeatedField::Unshift) {
  NanEscapableScope();
  RepeatedField* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  if (args.Length() != 1) {
    NanThrowError("Expected one argument to RepeatedField.unshift");
    NanReturnUndefined();
  }

  Local<Value> value = args[0];

  if (value->IsNull()) {
    NanThrowError("Cannot set a value to null in a repeated field");
    NanReturnUndefined();
  }
  Local<Value> converted = NanNew(
      ProtoMessage::CheckConvertElement(self->type_, self->submsg_, value,
                                        /* allow_null = */ false,
                                        /* allow_copy = */ false));
  if (converted.IsEmpty() || converted->IsUndefined()) {
    NanReturnUndefined();
  }

  self->values_.resize(self->values_.size() + 1);
  for (int i = self->values_.size() - 1; i > 0; --i) {
    NanAssignPersistent(self->values_[i], NanNew(self->values_[i - 1]));
  }
  NanAssignPersistent(self->values_[0], converted);

  NanReturnValue(converted);
}

NAN_METHOD(RepeatedField::ToString) {
  NanEscapableScope();
  RepeatedField* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  CheckArgs checkargs(args);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  Local<Object> type_desc;
  if (self->type_ == UPB_TYPE_MESSAGE) {
    type_desc = NanNew(self->submsg_->object());
  } else if (self->type_ == UPB_TYPE_ENUM) {
    type_desc = NanNew(self->subenum_->object());
  }

  std::string value = "[";
  bool first = true;
  for (int i = 0; i < self->values_.size(); i++) {
    if (first) {
      first = false;
    } else {
      value += ", ";
    }
    value += ProtoMessage::ElementString(self->type_,
                                         type_desc,
                                         NanNew(self->values_[i]));
  }
  value += "]";

  NanReturnValue(NanNew<String>(value.data(), value.size()));
}

NAN_METHOD(RepeatedField::Resize) {
  NanEscapableScope();
  RepeatedField* self = RepeatedField::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }

  if (args.Length() != 1 || !args[0]->IsInt32()) {
    NanThrowError("Expected one integer argument");
    NanReturnUndefined();
  }

  size_t newsize = args[0]->Int32Value();
  size_t oldsize = self->values_.size();
  self->values_.resize(newsize);
  if (newsize > oldsize) {
    for (size_t i = oldsize; i < newsize; i++) {
      NanEscapableScope();
      Local<Value> new_element;
      if (self->type_ == UPB_TYPE_MESSAGE) {
        new_element =
            NanNew(self->submsg_->Constructor()->NewInstance(0, NULL));
      } else {
        new_element = NanNew(ProtoMessage::NewElement(self->type_));
      }
      NanAssignPersistent(self->values_[i], new_element);
    }
  }

  NanReturnUndefined();
}

NAN_METHOD(RepeatedField::NewEmpty) {
  NanEscapableScope();
  RepeatedField* self = RepeatedField::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }

  if (args.Length() > 0) {
    NanThrowError("Expected no arguments");
    NanReturnUndefined();
  }

  Handle<Value> argv[2];
  int argc = 1;

  argv[0] = NanNew<Integer>(self->type_);
  if (self->type_ == UPB_TYPE_MESSAGE) {
    argv[1] = NanNew(self->submsg_->object());
    argc = 2;
  } else if (self->type_ == UPB_TYPE_ENUM) {
    argv[1] = NanNew(self->subenum_->object());
    argc = 2;
  }

  Local<Value> new_rptfield =
      NanNew(NanNew(constructor)->NewInstance(argc, argv));
  NanReturnValue(new_rptfield);
}

NAN_GETTER(RepeatedField::TypeGetter) {
  NanEscapableScope();
  RepeatedField* self = RepeatedField::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  NanReturnValue(NanNew<Integer>(self->type_));
}

NAN_GETTER(RepeatedField::SubDescGetter) {
  NanEscapableScope();
  RepeatedField* self = RepeatedField::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  if (self->type_ == UPB_TYPE_MESSAGE) {
    NanReturnValue(NanNew(self->submsg_->object()));
  } else if (self->type_ == UPB_TYPE_ENUM) {
    NanReturnValue(NanNew(self->subenum_->object()));
  } else {
    NanReturnUndefined();
  }
}

}  // namespace protobuf_js
