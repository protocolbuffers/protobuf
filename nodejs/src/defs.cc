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
#include "enum.h"
#include "readonlyarray.h"
#include "util.h"

#include <assert.h>
#include <vector>

using namespace v8;
using namespace node;

namespace protobuf_js {

Persistent<Function> Descriptor::constructor;
Persistent<Value> Descriptor::prototype;
Persistent<Function> FieldDescriptor::constructor;
Persistent<Function> OneofDescriptor::constructor;
Persistent<Function> EnumDescriptor::constructor;
Persistent<Value> EnumDescriptor::prototype;
Persistent<Function> DescriptorPool::constructor;

JS_OBJECT_INIT(Descriptor);
JS_OBJECT_INIT(FieldDescriptor);
JS_OBJECT_INIT(OneofDescriptor);
JS_OBJECT_INIT(EnumDescriptor);
JS_OBJECT_INIT(DescriptorPool);

//-------------------------------------
// Descriptor
//-------------------------------------

void Descriptor::Init(Handle<Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>("Descriptor"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("name"),
                                       NameGetter, NameSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("mapentry"),
                                       MapEntryGetter, MapEntrySetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("fields"),
                                       FieldsGetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("oneofs"),
                                       OneofsGetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("msgclass"),
                                       MsgClassGetter);

  tpl->PrototypeTemplate()->Set(
      NanNew<String>("findFieldByName"),
      NanNew<FunctionTemplate>(FindFieldByName)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("findFieldByNumber"),
      NanNew<FunctionTemplate>(FindFieldByNumber)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("addField"),
      NanNew<FunctionTemplate>(AddField)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("findOneof"),
      NanNew<FunctionTemplate>(FindOneof)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("addOneof"),
      NanNew<FunctionTemplate>(AddOneof)->GetFunction());

  NanAssignPersistent(constructor, tpl->GetFunction());
  NanAssignPersistent(
          prototype,
          NanNew(constructor)->NewInstance(0, NULL)->GetPrototype());
  exports->Set(NanNew<String>("Descriptor"), NanNew(constructor));
}

Descriptor::Descriptor() {
  msgdef_ = upb::MessageDef::New();
  slots_ = 0;
  layout_computed_ = false;
  pool_ = NULL;
}

upb::MessageDef* Descriptor::mutable_msgdef() {
  if (msgdef_->IsFrozen()) {
    NanThrowError("Attempting to modify a frozen Descriptor.");
    return NULL;
  }
  return const_cast<upb::MessageDef*>(msgdef_.get());
}

NAN_METHOD(Descriptor::New) {
  NanEscapableScope();
  if (args.IsConstructCall()) {
    Descriptor* self = new Descriptor();
    self->Wrap<Descriptor>(args.This());
    if (!self->HandleCtorArgs(args)) {
      NanReturnUndefined();
    }
    NanReturnValue(args.This());
  } else {
    NanReturnValue(
                NanNew(constructor)->NewInstance(0, NULL));
  }
}

bool Descriptor::DoNameSetter(Local<Value> value) {
  if (!value->IsString()) {
    NanThrowError("Expected string");
    return false;
  }
  upb::MessageDef* msgdef = mutable_msgdef();
  if (!msgdef) {
    return false;
  }

  upb::Status st;
  msgdef->set_full_name(*String::Utf8Value(value), &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    return false;
  }

  return true;
}

bool Descriptor::DoMapEntrySetter(Local<Value> value) {
  if (!value->IsBoolean()) {
    NanThrowError("Expected boolean");
    return false;
  }
  upb::MessageDef* msgdef = mutable_msgdef();
  if (!msgdef) {
    return false;
  }

  msgdef->setmapentry(value->BooleanValue());
  return true;
}

bool Descriptor::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();

  if (args.Length() == 0) {
    return true;
  } else if (args.Length() <= 4) {
    // One-arg form: (name).
    // Two-arg form: (name, [fields]).
    // Three-arg form: (name, [fields], [oneofs]).
    // Four-arg form: (name, [fields], [oneofs], is_mapentry).
    Local<Array> fields;
    Local<Array> oneofs;

    if (args.Length() > 1) {
      if (!args[1]->IsArray()) {
        NanThrowError("Second constructor argument must be an Array of "
              "FieldDescriptor objects");
        return false;
      }
      fields = NanNew(args[1].As<Array>());
    }
    if (args.Length() > 2) {
      if (!args[2]->IsArray()) {
        NanThrowError("Third constructor argument must be an Array of "
              "OneofDescriptor objects");
        return false;
      }
      oneofs = NanNew(args[2].As<Array>());
    }
    if (args.Length() > 3) {
      if (!DoMapEntrySetter(args[3])) {
        return false;
      }
    }

    if (!DoNameSetter(args[0])) {
      return false;
    }
    if (!fields.IsEmpty()) {
      for (int i = 0; i < fields->Length(); i++) {
        if (!fields->Get(i)->IsObject()) {
          NanThrowError("Element in fields array is not an object");
          return false;
        }
        Local<Object> field = fields->Get(i)->ToObject();
        if (!DoAddField(args.This(), field)) {
          return false;
        }
      }
    }
    if (!oneofs.IsEmpty()) {
      for (int i = 0; i < oneofs->Length(); i++) {
        if (!oneofs->Get(i)->IsObject()) {
          NanThrowError("Element in oneofs array is not an object");
          return false;
        }
        Local<Object> oneof = oneofs->Get(i)->ToObject();
        if (!DoAddOneof(args.This(), oneof)) {
          return false;
        }
      }
    }
  } else {
    NanThrowError("Too many arguments to constructor");
    return false;
  }

  return true;
}

NAN_GETTER(Descriptor::NameGetter) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());
  const char* name = self->msgdef_->full_name();
  if (!name) {
    name = "";
  }
  NanReturnValue(NanNew<String>(name));
}

NAN_SETTER(Descriptor::NameSetter) {
  Descriptor* self = unwrap(args.This());
  self->DoNameSetter(value);
}

NAN_GETTER(Descriptor::MapEntryGetter) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());
  NanReturnValue(self->msgdef_->mapentry() ? NanTrue() : NanFalse());
}

NAN_SETTER(Descriptor::MapEntrySetter) {
  Descriptor* self = unwrap(args.This());
  self->DoMapEntrySetter(value);
}

NAN_GETTER(Descriptor::FieldsGetter) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());

  ReadOnlyArray::Builder builder;
  for (FieldMap::iterator it = self->fields_.begin();
       it != self->fields_.end();
       ++it) {
    builder.Add(NanNew(it->second));
  }

  NanReturnValue(NanNew(builder.Build()));
}

NAN_METHOD(Descriptor::FindFieldByName) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::STRING);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }
  std::string key(*String::Utf8Value(args[0]));
  const upb::FieldDef* field = self->msgdef_->FindFieldByName(key);
  if (!field) {
    NanReturnValue(NanNull());
  }
  NanReturnValue(NanNew(self->fields_[field->number()]));
}

NAN_METHOD(Descriptor::FindFieldByNumber) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::INTEGER);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  int32_t fieldnum = args[0]->Int32Value();
  FieldMap::iterator it = self->fields_.find(fieldnum);
  if (it == self->fields_.end()) {
    NanReturnValue(NanNull());
  }

  NanReturnValue(NanNew(it->second));
}

FieldDescriptor* Descriptor::LookupFieldByName(const std::string& name) {
  NanEscapableScope();

  const upb::FieldDef* field = msgdef_->FindFieldByName(name);
  if (!field) {
    return NULL;
  }

  Local<Object> fieldobj = NanNew(fields_[field->number()]);
  return FieldDescriptor::unwrap(fieldobj);
}

FieldDescriptor* Descriptor::LookupFieldByNumber(int32_t number) {
  NanEscapableScope();
  FieldMap::iterator it = fields_.find(number);
  if (it == fields_.end()) {
    return NULL;
  }
  Local<Object> fieldobj = NanNew(it->second);
  return FieldDescriptor::unwrap(fieldobj);
}

OneofDescriptor* Descriptor::LookupOneofByName(const std::string& name) {
  NanEscapableScope();

  OneofMap::iterator it = oneofs_.find(name);
  if (it == oneofs_.end()) {
    return NULL;
  }

  Local<Object> oneofobj = NanNew(it->second);
  return OneofDescriptor::unwrap(oneofobj);
}

// Split out DoAddField() because we cannot construct a v8::Arguments manually,
// and we need access to the setter implementations from constructors as well.
bool Descriptor::DoAddField(Local<Object> this_,
                            Local<Object> fieldobj) {
  NanEscapableScope();
  FieldDescriptor* field = FieldDescriptor::unwrap(fieldobj);
  if (!field) {
    return false;
  }

  upb::MessageDef* msgdef = mutable_msgdef();
  if (!msgdef) {
    return false;
  }

  upb::Status st;
  upb::FieldDef* fielddef = field->mutable_fielddef();
  if (!fielddef) {
    return false;
  }
  msgdef->AddField(fielddef, &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
  }
  NanAssignPersistent(field->descriptor_, this_);
  NanAssignPersistent(fields_[field->fielddef_->number()], fieldobj);

  return true;
}


NAN_METHOD(Descriptor::AddField) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::OBJECT);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  if (self->DoAddField(args.This(), args[0]->ToObject())) {
    NanReturnValue(args[0]);
  } else {
    NanReturnUndefined();
  }
}

NAN_GETTER(Descriptor::OneofsGetter) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());

  ReadOnlyArray::Builder builder;
  for (OneofMap::iterator it = self->oneofs_.begin();
       it != self->oneofs_.end();
       ++it) {
    builder.Add(NanNew(it->second));
  }

  NanReturnValue(NanNew(builder.Build()));
}

NAN_METHOD(Descriptor::FindOneof) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::STRING);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }
  std::string key(*String::Utf8Value(args[0]));
  OneofMap::iterator it = self->oneofs_.find(key);
  if (it == self->oneofs_.end()) {
    NanReturnValue(NanNull());
  }
  NanReturnValue(NanNew(it->second));
}

bool Descriptor::DoAddOneof(Local<Object> this_,
                            Local<Object> oneofobj) {
  NanEscapableScope();
  upb::MessageDef* msgdef = mutable_msgdef();
  if (!msgdef) {
    return false;
  }

  OneofDescriptor* oneof = OneofDescriptor::unwrap(oneofobj);
  if (!oneof) {
    return false;
  }

  upb::Status st;
  upb::OneofDef* oneofdef = oneof->mutable_oneofdef();
  if (!oneofdef) {
    return false;
  }
  msgdef->AddOneof(oneofdef, &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
  }
  NanAssignPersistent(oneof->descriptor_, this_);
  fields_.insert(oneof->fields_.begin(), oneof->fields_.end());

  for (OneofDescriptor::FieldMap::iterator it = oneof->fields_.begin();
       it != oneof->fields_.end(); ++it) {
    FieldDescriptor* field = FieldDescriptor::unwrap(NanNew(it->second));
    NanAssignPersistent(field->descriptor_, this_);
  }

  NanAssignPersistent(oneofs_[oneof->oneofdef_->name()], oneofobj);

  return true;
}

NAN_METHOD(Descriptor::AddOneof) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::OBJECT);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  if (self->DoAddOneof(args.This(), args[0]->ToObject())) {
    NanReturnValue(args[0]);
  } else {
    NanReturnUndefined();
  }
}

NAN_GETTER(Descriptor::MsgClassGetter) {
  NanEscapableScope();
  Descriptor* self = unwrap(args.This());
  NanReturnValue(NanNew(self->msgclass_));
}

void Descriptor::BuildClass(Local<Object> this_) {
  NanEscapableScope();

  if (msgclass_.IsEmpty()) {
    NanAssignPersistent(msgclass_, ProtoMessage::MakeConstructor(this_));
    Local<Value> prototype = NanNew(msgclass_)->
        NewInstance(0, NULL)->GetPrototype();
    NanAssignPersistent(msgprototype_, prototype->ToObject());
  }
}

void Descriptor::CreateLayout() {
  NanEscapableScope();

  // Reserve slots for the JSObject wrapping abstraction internal field(s).
  slots_ = JS_OBJECT_WRAP_SLOTS;

  // Assign slots to all non-oneof fields.
  for (FieldMap::iterator it = fields_.begin(); it != fields_.end(); ++it) {
    FieldDescriptor* field = FieldDescriptor::unwrap(
        NanNew(it->second));
    if (!field->oneof_.IsEmpty()) {
      continue;
    }
    field->SetSlot(slots_++);
  }

  // Assign slots to all oneof fields, re-using the same slot for each oneof and
  // allocating an additional slot for the oneof case.
  for (OneofMap::iterator it = oneofs_.begin(); it != oneofs_.end(); ++it) {
    OneofDescriptor* oneof = OneofDescriptor::unwrap(NanNew(it->second));
    int slot = slots_++;
    int case_slot = slots_++;
    oneof->SetSlots(slot, case_slot);
  }

  layout_computed_ = true;
}

//-------------------------------------
// FieldDescriptor
//-------------------------------------

void FieldDescriptor::Init(v8::Handle<v8::Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>("FieldDescriptor"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("name"),
                                       NameGetter, NameSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("type"),
                                       TypeGetter, TypeSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("number"),
                                       NumberGetter, NumberSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("label"),
                                       LabelGetter, LabelSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("subtype_name"),
                                       SubTypeNameGetter, SubTypeNameSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("subtype"),
                                       SubTypeGetter, SubTypeSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("descriptor"),
                                       DescriptorGetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("oneof"),
                                       OneofGetter);

  NanAssignPersistent(constructor, tpl->GetFunction());

  Local<Function> ctor = NanNew(constructor);

#define SETTYPE(type)                                             \
  ctor->Set(NanNew<String>("TYPE_" #type),                        \
            NanNew<Integer>(static_cast<int>(UPB_TYPE_ ## type)))

  SETTYPE(INT32);
  SETTYPE(INT64);
  SETTYPE(UINT32);
  SETTYPE(UINT64);
  SETTYPE(BOOL);
  SETTYPE(FLOAT);
  SETTYPE(DOUBLE);
  SETTYPE(ENUM);
  SETTYPE(STRING);
  SETTYPE(BYTES);
  SETTYPE(MESSAGE);

#undef SETTYPE

#define SETLABEL(label)                                            \
  ctor->Set(NanNew<String>("LABEL_" #label),                       \
            NanNew<Integer>(static_cast<int>(UPB_LABEL_ ## label)));

  SETLABEL(OPTIONAL);
  // no REQUIRED -- proto3-only (for now?).
  SETLABEL(REPEATED);

#undef SETLABEL

  exports->Set(NanNew<String>("FieldDescriptor"), NanNew(constructor));
}

FieldDescriptor::FieldDescriptor() {
  fielddef_ = upb::FieldDef::New();
  slot_ = 0;
  slot_set_ = false;
}

upb::FieldDef* FieldDescriptor::mutable_fielddef() {
  if (fielddef_->IsFrozen()) {
    NanThrowError("Attempting to modify a frozen FieldDescriptor.");
    return NULL;
  }
  return const_cast<upb::FieldDef*>(fielddef_.get());
}

void FieldDescriptor::SetSlot(int slot) {
  assert(!slot_set_);
  slot_ = slot;
  slot_set_ = true;
}

Descriptor* FieldDescriptor::submsg() {
  NanEscapableScope();
  if (fielddef()->type() != UPB_TYPE_MESSAGE ||
      subtype_.IsEmpty()) {
    return NULL;
  }
  Local<Object> desc = NanNew(subtype_);
  return Descriptor::unwrap(desc);
}

EnumDescriptor* FieldDescriptor::subenum() {
  NanEscapableScope();
  if (fielddef()->type() != UPB_TYPE_ENUM ||
      subtype_.IsEmpty()) {
    return NULL;
  }
  Local<Object> desc = NanNew(subtype_);
  return EnumDescriptor::unwrap(desc);
}

OneofDescriptor* FieldDescriptor::oneof() const {
  NanEscapableScope();
  if (!oneof_.IsEmpty()) {
    return OneofDescriptor::unwrap(NanNew(oneof_));
  } else {
    return NULL;
  }
}

NAN_METHOD(FieldDescriptor::New) {
  NanEscapableScope();
  if (args.IsConstructCall()) {
    FieldDescriptor* self = new FieldDescriptor();
    self->Wrap<FieldDescriptor>(args.This());
    if (!self->HandleCtorArgs(args)) {
      NanReturnUndefined();
    }
    NanReturnValue(args.This());
  } else {
    NanReturnValue(NanNew(constructor)->NewInstance(0, NULL));
  }
}

bool FieldDescriptor::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();

  if (args.Length() == 0) {
    return true;
  } else if (args.Length() == 1) {
    // We accept keyword args via an anonymous object with |label|, |type|,
    // |name|, |number|, and optionally |subtype_name| fields (properties).
    Local<Value> kwargs_val = args[0];
    if (!kwargs_val->IsObject()) {
      NanThrowError("Expecting keyword-argument object");
      return false;
    }
    Local<Object> kwargs = kwargs_val->ToObject();

    Local<Value> label = kwargs->Get(NanNew<String>("label"));
    if (!label->IsUndefined()) {
      if (!DoLabelSetter(label)) {
        return false;
      }
    }
    Local<Value> type = kwargs->Get(NanNew<String>("type"));
    if (!type->IsUndefined()) {
      if (!DoTypeSetter(type)) {
        return false;
      }
    }
    Local<Value> name = kwargs->Get(NanNew<String>("name"));
    if (!name->IsUndefined()) {
      if (!DoNameSetter(name)) {
        return false;
      }
    }
    Local<Value> number = kwargs->Get(NanNew<String>("number"));
    if (!number->IsUndefined()) {
      if (!DoNumberSetter(number)) {
        return false;
      }
    }
    Local<Value> subtype_name = kwargs->Get(NanNew<String>("subtype_name"));
    if (!subtype_name->IsUndefined()) {
      if (!DoSubTypeNameSetter(subtype_name)) {
        return false;
      }
    }
  } else {
    NanThrowError("FieldDescriptor constructor expects 0 or 1 arguments");
    return false;
  }

  return true;
}

NAN_GETTER(FieldDescriptor::NameGetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());
  const char* name = self->fielddef_->name();
  if (!name) {
    name = "";
  }
  NanReturnValue(NanNew<String>(name));
}

bool FieldDescriptor::DoNameSetter(Local<Value> value) {
  if (!value->IsString()) {
    NanThrowError("Expected string");
    return false;
  }
  upb::FieldDef* fielddef = mutable_fielddef();
  if (!fielddef) {
    return false;
  }

  upb::Status st;
  fielddef->set_name(*String::Utf8Value(value), &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    return false;
  }

  return true;
}

NAN_SETTER(FieldDescriptor::NameSetter) {
  FieldDescriptor* self = unwrap(args.This());
  self->DoNameSetter(value);
}

NAN_GETTER(FieldDescriptor::TypeGetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());
  NanReturnValue(NanNew<Integer>(
              static_cast<int32_t>(self->fielddef_->type())));
}

bool FieldDescriptor::ParseTypeValue(Local<Value> value, upb_fieldtype_t* type) {
  if (!value->IsInt32()) {
    NanThrowError("Type property expects a number "
          "(an enum value FieldDescriptor.TYPE_*)");
    return false;
  }

  switch (value->Int32Value()) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_BOOL:
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_ENUM:
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      break;
    default:
      NanThrowError("Unknown value for type property");
      return false;
  }

  *type = static_cast<upb_fieldtype_t>(value->Int32Value());
  return true;
}

bool FieldDescriptor::DoTypeSetter(Local<Value> value) {
  upb::FieldDef* fielddef = mutable_fielddef();
  if (!fielddef) {
    return false;
  }

  upb_fieldtype_t type;
  if (!ParseTypeValue(value, &type)) {
    return false;
  }
  fielddef->set_type(type);
  return true;
}

NAN_SETTER(FieldDescriptor::TypeSetter) {
  FieldDescriptor* self = unwrap(args.This());
  self->DoTypeSetter(value);
}

NAN_GETTER(FieldDescriptor::NumberGetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());
  NanReturnValue(NanNew<Integer>(self->fielddef_->number()));
}

bool FieldDescriptor::DoNumberSetter(Local<Value> value) {
  upb::FieldDef* fielddef = mutable_fielddef();
  if (!fielddef) {
    return false;
  }
  if (!value->IsInt32() || value->Int32Value() <= 0) {
    NanThrowError("Number property expects a positive integer");
    return false;
  }

  int32_t number = value->Int32Value();
  upb::Status st;
  fielddef->set_number(number, &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    return false;
  }

  return true;
}

NAN_SETTER(FieldDescriptor::NumberSetter) {
  FieldDescriptor* self = unwrap(args.This());
  self->DoNumberSetter(value);
}

NAN_GETTER(FieldDescriptor::LabelGetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());
  NanReturnValue(NanNew<Integer>(
              static_cast<int32_t>(self->fielddef_->label())));
}

bool FieldDescriptor::DoLabelSetter(Local<Value> value) {
  upb::FieldDef* fielddef = mutable_fielddef();
  if (!fielddef) {
    return false;
  }
  if (!value->IsInt32()) {
    NanThrowError("Label property expects a number "
          "(an enum value FieldDescriptor.LABEL_*)");
    return false;
  }

  switch (value->Int32Value()) {
    case 0:  // default / no type
    case UPB_LABEL_OPTIONAL:
    case UPB_LABEL_REPEATED:
      // No REQUIRED -- proto3-only.
      break;
    default:
      NanThrowError("Unknown value for label property");
      return false;
  }

  upb_label_t label = static_cast<upb_label_t>(value->Int32Value());
  fielddef->set_label(label);

  return true;
}

NAN_SETTER(FieldDescriptor::LabelSetter) {
  FieldDescriptor* self = unwrap(args.This());
  self->DoLabelSetter(value);
}

NAN_GETTER(FieldDescriptor::SubTypeNameGetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());

  if (self->fielddef_->type() != UPB_TYPE_MESSAGE &&
      self->fielddef_->type() != UPB_TYPE_ENUM) {
    NanReturnValue(NanNull());
  }

  const char* name = self->fielddef_->subdef_name();
  // Strip off the leading "." if present. It's added to make the subdef
  // reference absolute, as per upb.
  if (name && (name[0] == '.')) {
    name++;
  }

  NanReturnValue(NanNew<String>(name ? name : ""));
}

bool FieldDescriptor::DoSubTypeNameSetter(Local<Value> value) {
  upb::FieldDef* fielddef = mutable_fielddef();
  if (!fielddef) {
    return false;
  }
  if (!value->IsString()) {
    NanThrowError("Subtype property expects a string");
    return false;
  }

  upb::Status st;
  std::string subdef_name(".");  // prepend a "." to make the name absolute.
  subdef_name += *String::Utf8Value(value);
  fielddef->set_subdef_name(subdef_name.c_str(), &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    return false;
  }

  return true;
}

NAN_SETTER(FieldDescriptor::SubTypeNameSetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());
  self->DoSubTypeNameSetter(value);
}

NAN_GETTER(FieldDescriptor::SubTypeGetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());
  if (!self->fielddef_->IsFrozen()) {
    NanThrowError("Cannot access subtype property until field's message is "
                  "added to a descriptor pool so that type references are "
                  "resolved");
    NanReturnUndefined();
  }
  NanReturnValue(NanNew(self->subtype_));
}

NAN_SETTER(FieldDescriptor::SubTypeSetter) {
  NanEscapableScope();
  NanThrowError("subtype property is read-only");
}

NAN_GETTER(FieldDescriptor::DescriptorGetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());
  NanReturnValue(NanNew(self->descriptor_));
}

NAN_GETTER(FieldDescriptor::OneofGetter) {
  NanEscapableScope();
  FieldDescriptor* self = unwrap(args.This());
  NanReturnValue(NanNew(self->oneof_));
}

//-------------------------------------
// OneofDescriptor
//-------------------------------------

void OneofDescriptor::Init(Handle<Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>("OneofDescriptor"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("name"),
                                       NameGetter, NameSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("fields"),
                                       FieldsGetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("descriptor"),
                                       DescriptorGetter);

  tpl->PrototypeTemplate()->Set(
      NanNew<String>("findFieldByName"),
      NanNew<FunctionTemplate>(FindFieldByName)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("findFieldByNumber"),
      NanNew<FunctionTemplate>(FindFieldByNumber)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("addField"),
      NanNew<FunctionTemplate>(AddField)->GetFunction());

  NanAssignPersistent(constructor, tpl->GetFunction());
  exports->Set(NanNew<String>("OneofDescriptor"), NanNew(constructor));
}

OneofDescriptor::OneofDescriptor() {
  oneofdef_ = upb::OneofDef::New();
  slot_ = 0;
  case_slot_ = 0;
  slots_set_ = false;
}

upb::OneofDef* OneofDescriptor::mutable_oneofdef() {
  if (oneofdef_->IsFrozen()) {
    NanThrowError("Attempting to modify a frozen OneofDescriptor.");
    return NULL;
  }
  return const_cast<upb::OneofDef*>(oneofdef_.get());
}

void OneofDescriptor::SetSlots(int slot, int case_slot) {
  assert(!slots_set_);
  slot_ = slot;
  case_slot_ = case_slot;

  for (FieldMap::iterator it = fields_.begin(); it != fields_.end(); ++it) {
    FieldDescriptor* field = FieldDescriptor::unwrap(NanNew(it->second));
    field->SetSlot(slot);
  }

  slots_set_ = true;
}

NAN_METHOD(OneofDescriptor::New) {
  NanEscapableScope();
  if (args.IsConstructCall()) {
    OneofDescriptor* self = new OneofDescriptor();
    self->Wrap<OneofDescriptor>(args.This());
    if (!self->HandleCtorArgs(args)) {
      NanReturnUndefined();
    }
    NanReturnValue(args.This());
  } else {
    NanReturnValue(NanNew(constructor)->NewInstance(0, NULL));
  }
}

bool OneofDescriptor::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();

  if (args.Length() == 0) {
    return true;
  } else if (args.Length() == 1 || args.Length() == 2) {
    // One-arg form: (name).
    // Two-arg form: (name, [fields]).

    Local<Array> fields;
    if (args.Length() > 1) {
      if (!args[1]->IsArray()) {
        NanThrowError("Second constructor argument must be an Array of "
              "FieldDescriptor objects");
        return false;
      }
      fields = NanNew(args[1].As<Array>());
    }

    if (!DoNameSetter(args[0])) {
      return false;
    }

    if (!fields.IsEmpty()) {
      for (int i = 0; i < fields->Length(); i++) {
        if (!fields->Get(i)->IsObject()) {
          NanThrowError("Element in fields array is not an object");
          return false;
        }
        Local<Object> field = fields->Get(i)->ToObject();
        if (!DoAddField(args.This(), field)) {
          return false;
        }
      }
    }
  } else {
    NanThrowError("Too many arguments to constructor");
    return false;
  }

  return true;
}

NAN_GETTER(OneofDescriptor::NameGetter) {
  NanEscapableScope();
  OneofDescriptor* self = unwrap(args.This());
  const char* name = self->oneofdef_->name();
  if (!name) {
    name = "";
  }
  NanReturnValue(NanNew<String>(name));
}

bool OneofDescriptor::DoNameSetter(Local<Value> value) {
  if (!value->IsString()) {
    NanThrowError("Expected string");
    return false;
  }
  upb::OneofDef* oneofdef = mutable_oneofdef();
  if (!oneofdef) {
    return false;
  }

  upb::Status st;
  oneofdef->set_name(*String::Utf8Value(value), &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    return false;
  }

  return true;
}

NAN_SETTER(OneofDescriptor::NameSetter) {
  OneofDescriptor* self = unwrap(args.This());
  self->DoNameSetter(value);
}

NAN_GETTER(OneofDescriptor::FieldsGetter) {
  NanEscapableScope();
  OneofDescriptor* self = unwrap(args.This());

  ReadOnlyArray::Builder builder;
  for (FieldMap::iterator it = self->fields_.begin();
       it != self->fields_.end();
       ++it) {
    builder.Add(NanNew(it->second));
  }

  NanReturnValue(NanNew(builder.Build()));
}

NAN_GETTER(OneofDescriptor::DescriptorGetter) {
  NanEscapableScope();
  OneofDescriptor* self = unwrap(args.This());
  NanReturnValue(NanNew(self->descriptor_));
}

NAN_METHOD(OneofDescriptor::FindFieldByName) {
  NanEscapableScope();
  OneofDescriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::STRING);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }
  std::string key(*String::Utf8Value(args[0]));
  const upb::FieldDef* field = self->oneofdef_->FindFieldByName(key);
  if (!field) {
    NanReturnValue(NanNull());
  }
  NanReturnValue(NanNew(self->fields_[field->number()]));
}

NAN_METHOD(OneofDescriptor::FindFieldByNumber) {
  NanEscapableScope();
  OneofDescriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::INTEGER);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  int32_t fieldnum = args[0]->Int32Value();
  FieldMap::iterator it = self->fields_.find(fieldnum);
  if (it == self->fields_.end()) {
    NanReturnValue(NanNull());
  }

  NanReturnValue(NanNew(it->second));
}

bool OneofDescriptor::DoAddField(Local<Object> _this, Local<Object> fieldobj) {
  NanEscapableScope();
  upb::OneofDef* oneofdef = mutable_oneofdef();
  if (!oneofdef) {
    return false;
  }

  FieldDescriptor* field = FieldDescriptor::unwrap(fieldobj);
  if (!field) {
    return false;
  }

  upb::Status st;
  upb::FieldDef* fielddef = field->mutable_fielddef();
  if (!fielddef) {
    return false;
  }
  oneofdef->AddField(fielddef, &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    return false;
  }

  NanAssignPersistent(field->oneof_, _this);
  if (!descriptor_.IsEmpty()) {
    NanAssignPersistent(field->descriptor_, descriptor_);
  }

  NanAssignPersistent(fields_[field->fielddef_->number()], fieldobj);

  return true;
}

NAN_METHOD(OneofDescriptor::AddField) {
  NanEscapableScope();
  OneofDescriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::OBJECT);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  Local<Object> fieldobj(args[0]->ToObject());
  if (self->DoAddField(args.This(), fieldobj)) {
    NanReturnValue(fieldobj);
  } else {
    NanReturnUndefined();
  }
}

//-------------------------------------
// EnumDescriptor
//-------------------------------------

void EnumDescriptor::Init(Handle<Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>("EnumDescriptor"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("name"),
                                       NameGetter, NameSetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("keys"),
                                       KeysGetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("values"),
                                       ValuesGetter);

  tpl->PrototypeTemplate()->Set(
      NanNew<String>("findByName"),
      NanNew<FunctionTemplate>(FindByName)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("findByValue"),
      NanNew<FunctionTemplate>(FindByValue)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("add"),
      NanNew<FunctionTemplate>(Add)->GetFunction());

  NanAssignPersistent(constructor, tpl->GetFunction());
  exports->Set(NanNew<String>("EnumDescriptor"), NanNew(constructor));
  NanAssignPersistent(
          prototype, NanNew(constructor)->NewInstance(0, NULL)->GetPrototype());
}

EnumDescriptor::EnumDescriptor() {
  enumdef_ = upb::EnumDef::New();
}

upb::EnumDef* EnumDescriptor::mutable_enumdef() {
  if (enumdef_->IsFrozen()) {
    NanThrowError("Attempting to modify a frozen EnumDescriptor.");
    return NULL;
  }
  return const_cast<upb::EnumDef*>(enumdef_.get());
}

NAN_METHOD(EnumDescriptor::New) {
  NanEscapableScope();
  if (args.IsConstructCall()) {
    EnumDescriptor* self = new EnumDescriptor();
    self->Wrap<EnumDescriptor>(args.This());
    if (!self->HandleCtorArgs(args)) {
      NanReturnUndefined();
    }
    NanReturnValue(args.This());
  } else {
    NanReturnValue(NanNew(constructor)->NewInstance(0, NULL));
  }
}

bool EnumDescriptor::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();

  if (args.Length() == 0) {
    return true;
  } else if ((args.Length() % 2) == 1) {
    if (!DoNameSetter(args[0])) {
      return false;
    }

    for (int i = 0; (i + 2) < args.Length(); i += 2) {
      Local<Value> key = NanNew(args[i + 1]);
      Local<Value> value = NanNew(args[i + 2]);
      if (!key->IsString()) {
        NanThrowError("Enum key must be a string");
        return false;
      }
      if (!value->IsNumber()) {
        NanThrowError("Enum value must be an integer");
        return false;
      }

      if (!DoAdd(key->ToString(), value->Int32Value())) {
        return false;
      }
    }
  } else {
    NanThrowError("Incorrect number of arguments to EnumDescriptor constructor: "
          "must be an odd number of arguments, as an enum name followed "
          "by key-value pairs");
    return false;
  }

  return true;
}

NAN_GETTER(EnumDescriptor::NameGetter) {
  NanEscapableScope();
  EnumDescriptor* self = unwrap(args.This());
  const char* name = self->enumdef_->full_name();
  if (!name) {
    name = "";
  }
  NanReturnValue(NanNew<String>(name));
}

bool EnumDescriptor::DoNameSetter(Local<Value> value) {
  if (!value->IsString()) {
    NanThrowError("Expected string");
    return false;
  }
  upb::EnumDef* enumdef = mutable_enumdef();
  if (!enumdef) {
    return false;
  }

  upb::Status st;
  enumdef->set_full_name(*String::Utf8Value(value), &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    return false;
  }

  return true;
}

NAN_SETTER(EnumDescriptor::NameSetter) {
  EnumDescriptor* self = unwrap(args.This());
  self->DoNameSetter(value);
}

NAN_GETTER(EnumDescriptor::KeysGetter) {
  NanEscapableScope();
  EnumDescriptor* self = unwrap(args.This());

  ReadOnlyArray::Builder builder;
  for (upb::EnumDef::Iterator it(self->enumdef_.get()); !it.Done(); it.Next()) {
    builder.Add(NanNew<String>(it.name()));
  }

  NanReturnValue(NanNew(builder.Build()));
}

NAN_GETTER(EnumDescriptor::ValuesGetter) {
  NanEscapableScope();
  EnumDescriptor* self = unwrap(args.This());

  ReadOnlyArray::Builder builder;
  for (upb::EnumDef::Iterator it(self->enumdef_.get()); !it.Done(); it.Next()) {
    builder.Add(NanNew<Integer>(it.number()));
  }

  NanReturnValue(NanNew(builder.Build()));
}

NAN_METHOD(EnumDescriptor::FindByName) {
  NanEscapableScope();
  EnumDescriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::STRING);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  int32_t value = 0;
  std::string name(*String::Utf8Value(args[0]));
  if (self->enumdef_->FindValueByName(name.c_str(), &value)) {
    NanReturnValue(NanNew<Integer>(value));
  } else {
    NanReturnValue(NanNull());
  }
}

NAN_METHOD(EnumDescriptor::FindByValue) {
  NanEscapableScope();
  EnumDescriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::INTEGER);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  int32_t value = args[0]->Int32Value();
  const char* name = self->enumdef_->FindValueByNumber(value);
  if (name) {
    NanReturnValue(NanNew<String>(name));
  } else {
    NanReturnValue(NanNull());
  }
}

bool EnumDescriptor::DoAdd(Local<String> key, int32_t value) {
  upb::EnumDef* enumdef = mutable_enumdef();
  if (!enumdef) {
    return false;
  }

  std::string name(*String::Utf8Value(key));
  upb::Status st;
  enumdef->AddValue(name, value, &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    return false;
  }

  return true;
}

NAN_METHOD(EnumDescriptor::Add) {
  NanEscapableScope();
  EnumDescriptor* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::STRING, CheckArgs::INTEGER);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  Local<String> key = args[0]->ToString();
  int32_t value = args[1]->Int32Value();

  if(!self->DoAdd(key, value)) {
    NanReturnUndefined();
  }

  NanReturnValue(args[0]);
}

void EnumDescriptor::BuildObject(Local<Object> this_) {
  NanEscapableScope();

  if (enumobj_.IsEmpty()) {
    Handle<Value> arg = this_;
    NanAssignPersistent(enumobj_,
                        NanNew(ProtoEnum::constructor)->NewInstance(1, &arg));
    Local<Object> enumobj = NanNew(enumobj_);
    this_->Set(NanNew<String>("enumobject"), enumobj);
    enumobj->Set(NanNew<String>("descriptor"), this_);
  }
}

//-------------------------------------
// DescriptorPool
//-------------------------------------

// We keep references to these so that we can quickly check the type of elements
// in the array given to the add() method.
Persistent<Value> DescriptorPool::descriptor_prototype_;
Persistent<Value> DescriptorPool::enum_prototype_;

void DescriptorPool::Init(Handle<Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>("DescriptorPool"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("descriptors"),
                                       DescriptorsGetter);
  tpl->InstanceTemplate()->SetAccessor(NanNew<String>("enums"),
                                       EnumsGetter);

  tpl->PrototypeTemplate()->Set(
      NanNew<String>("add"),
      NanNew<FunctionTemplate>(Add)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("lookup"),
      NanNew<FunctionTemplate>(Lookup)->GetFunction());

  NanAssignPersistent(constructor, tpl->GetFunction());
  exports->Set(NanNew<String>("DescriptorPool"), NanNew(constructor));

  // We depend on these constructors being already initialized by virtue of
  // setup order in protobuf.cc.
  assert(!Descriptor::constructor.IsEmpty());
  NanAssignPersistent(descriptor_prototype_,
      NanNew(Descriptor::constructor)->NewInstance(0, NULL)->GetPrototype());
  assert(!EnumDescriptor::constructor.IsEmpty());
  NanAssignPersistent(enum_prototype_,
      NanNew(EnumDescriptor::constructor)->NewInstance(0, NULL)->GetPrototype());

  Local<Object> genpool = NanNew(constructor)->NewInstance(0, NULL);
  NanNew(constructor)->Set(NanNew<String>("generatedPool"), genpool);
}

DescriptorPool::DescriptorPool() {
  symtab_ = upb::SymbolTable::New();
}

NAN_METHOD(DescriptorPool::New) {
  NanEscapableScope();
  if (args.IsConstructCall()) {
    DescriptorPool* self = new DescriptorPool();
    self->Wrap<DescriptorPool>(args.This());
    NanReturnValue(args.This());
  } else {
    NanReturnValue(NanNew(constructor)->NewInstance(0, NULL));
  }
}

NAN_METHOD(DescriptorPool::Add) {
  NanEscapableScope();
  DescriptorPool* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::ARRAY);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  Local<Array> arr = args[0].As<Array>();

  // Build an array of upb defs and a map from def names to V8 objects. We'll
  // commit the addition of the defs to the symtab first, and if (and only if)
  // that succeeds, we will add the V8 objects to our descriptor map.
  std::vector<upb::Def*> defs;
  std::map< std::string, Local<Object> > new_objs;
  std::vector< Local<Object> > new_descs;
  std::vector< Local<Object> > new_enums;

  for (int i = 0; i < arr->Length(); i++) {
    Local<Value> elem = arr->Get(i);
    if (!elem->IsObject()) {
      NanThrowError("Unexpected non-object in array");
      NanReturnUndefined();
    }
    Local<Object> defobj = elem->ToObject();

    if (defobj->GetPrototype() == descriptor_prototype_) {
      Descriptor* desc = Descriptor::unwrap(defobj);
      upb::MessageDef* msgdef = desc->mutable_msgdef();
      if (!msgdef) {
        NanReturnUndefined();
      }
      defs.push_back(upb::upcast(msgdef));
      new_objs.insert(std::make_pair(msgdef->full_name(),
                                     defobj));
      new_descs.push_back(defobj);
    } else if (defobj->GetPrototype() == enum_prototype_) {
      EnumDescriptor* enumdesc = EnumDescriptor::unwrap(defobj);
      upb::EnumDef* enumdef = enumdesc->mutable_enumdef();
      if (!enumdef) {
        NanReturnUndefined();
      }
      defs.push_back(upb::upcast(enumdef));
      new_objs.insert(std::make_pair(enumdef->full_name(),
                                     defobj));
      new_enums.push_back(defobj);
    }
  }

  // Try adding all defs to the symtab atomically.
  upb::Status st;
  self->symtab_->Add(defs, NULL, &st);
  if (!st.ok()) {
    NanThrowError(st.error_message());
    NanReturnUndefined();
  }

  // If that succeeded, merge all name->V8 object mappings into our ObjMap,
  // ObjPtrMap, and descs and enums lists.
  for (int i = 0; i < new_descs.size(); i++) {
    Local<Object> defobj = new_descs[i];
    Descriptor* desc = Descriptor::unwrap(defobj);
    const upb::Def* def = upb::upcast(desc->msgdef_.get());
    NanAssignPersistent(self->objs_[desc->msgdef_->full_name()], defobj);
    NanAssignPersistent(self->objptr_[def], defobj);
    Persistent<Object> p;
    NanAssignPersistent(p, defobj);
    self->descs_.push_back(p);
    desc->pool_ = self;
  }
  for (int i = 0; i < new_enums.size(); i++) {
    Local<Object> defobj = new_enums[i];
    EnumDescriptor* enumdesc = EnumDescriptor::unwrap(defobj);
    const upb::Def* def = upb::upcast(enumdesc->enumdef_.get());
    NanAssignPersistent(self->objs_[enumdesc->enumdef_->full_name()], defobj);
    NanAssignPersistent(self->objptr_[def], defobj);
    Persistent<Object> p;
    NanAssignPersistent(p, defobj);
    self->enums_.push_back(p);
  }

  // For every field in every Descriptor, set subtype_ pointers appropriately if
  // needed.
  for (int i = 0; i < new_descs.size(); i++) {
    Local<Object> defobj = new_descs[i];
    Descriptor* desc = Descriptor::unwrap(defobj);
    for (Descriptor::FieldMap::iterator it = desc->fields_.begin();
         it != desc->fields_.end(); ++it) {
      Local<Object> fieldobj = NanNew(it->second);
      FieldDescriptor* fielddesc = FieldDescriptor::unwrap(fieldobj);
      switch (fielddesc->fielddef_->type()) {
        case UPB_TYPE_MESSAGE:
        case UPB_TYPE_ENUM: {
          const upb::Def* subdef = fielddesc->fielddef_->subdef();
          NanAssignPersistent(fielddesc->subtype_, NanNew(self->objptr_[subdef]));
          break;
        }
        default:
          break;
      }
    }
  }

  // Build message object instance layout information. (TODO: do this lazily.)
  for (int i = 0; i < new_descs.size(); i++) {
    Local<Object> defobj = new_descs[i];
    Descriptor* desc = Descriptor::unwrap(defobj);
    desc->CreateLayout();
  }

  // Build message classes.
  for (int i = 0; i < new_descs.size(); i++) {
    Local<Object> defobj = new_descs[i];
    Descriptor* desc = Descriptor::unwrap(defobj);
    desc->BuildClass(defobj);
  }
  // Build enum objects.
  for (int i = 0; i < new_enums.size(); i++) {
    Local<Object> defobj = new_enums[i];
    EnumDescriptor* enumdesc = EnumDescriptor::unwrap(defobj);
    enumdesc->BuildObject(defobj);
  }

  NanReturnValue(args[0]);
}

NAN_METHOD(DescriptorPool::Lookup) {
  NanEscapableScope();
  DescriptorPool* self = unwrap(args.This());
  CheckArgs checkargs(args, CheckArgs::STRING);
  if (!checkargs.ok()) {
    NanReturnUndefined();
  }

  std::string key(*String::Utf8Value(args[0]));
  ObjMap::iterator it = self->objs_.find(key);
  if (it != self->objs_.end()) {
    NanReturnValue(NanNew(it->second));
  }
  NanReturnValue(NanNull());
}

NAN_GETTER(DescriptorPool::DescriptorsGetter) {
  NanEscapableScope();
  DescriptorPool* self = unwrap(args.This());
  ReadOnlyArray::Builder builder;
  for (int i = 0; i < self->descs_.size(); i++) {
    builder.Add(NanNew(self->descs_[i]));
  }
  NanReturnValue(NanNew(builder.Build()));
}

NAN_GETTER(DescriptorPool::EnumsGetter) {
  NanEscapableScope();
  DescriptorPool* self = unwrap(args.This());
  ReadOnlyArray::Builder builder;
  for (int i = 0; i < self->enums_.size(); i++) {
    builder.Add(NanNew(self->enums_[i]));
  }
  NanReturnValue(NanNew(builder.Build()));
}

}  // namespace protobuf_js
