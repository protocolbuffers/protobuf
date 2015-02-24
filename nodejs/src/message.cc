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
#include "message.h"
#include "defs.h"
#include "int64.h"
#include "readonlyarray.h"
#include "repeatedfield.h"
#include "map.h"
#include "encode_decode.h"
#include "util.h"

#include <assert.h>
#include <vector>
#include <sstream>
#include <ios>
#include <iomanip>

using namespace v8;
using namespace node;

namespace protobuf_js {

JS_OBJECT_INIT(ProtoMessage);

void ProtoMessage::Init(Handle<Object> exports) {
  exports->Set(
      NanNew<String>("encodeBinary"),
      NanNew<FunctionTemplate>(EncodeGlobalFunction)->GetFunction());
  exports->Set(
      NanNew<String>("decodeBinary"),
      NanNew<FunctionTemplate>(DecodeGlobalFunction)->GetFunction());
  exports->Set(
      NanNew<String>("encodeJson"),
      NanNew<FunctionTemplate>(EncodeJsonGlobalFunction)->GetFunction());
  exports->Set(
      NanNew<String>("decodeJson"),
      NanNew<FunctionTemplate>(DecodeJsonGlobalFunction)->GetFunction());
}

Handle<Function> ProtoMessage::MakeConstructor(
    Local<Object> descriptor) {
  NanEscapableScope();
  Descriptor* desc = Descriptor::unwrap(descriptor->ToObject());
  if (!desc) {
    return Handle<Function>();
  }

  if (!desc->msgdef()->IsFrozen()) {
    NanThrowError("Cannot create a message class for a message descriptor not "
          "yet added to a descriptor pool");
    return Handle<Function>();
  }

  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(
      New, /* data = */ descriptor);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>(desc->msgdef()->full_name()));

  tpl->InstanceTemplate()->SetInternalFieldCount(desc->LayoutSlots());

  tpl->PrototypeTemplate()->Set(
      NanNew<String>("toString"),
      NanNew<FunctionTemplate>(ToString)->GetFunction());
  tpl->PrototypeTemplate()->Set(
      NanNew<String>("descriptor"),
      desc->object());

  tpl->InstanceTemplate()->SetNamedPropertyHandler(
      MsgFieldGetter, MsgFieldSetter, 0, 0, MsgFieldEnumerator);

  Local<Function> f = tpl->GetFunction();
  f->Set(NanNew<String>("descriptor"), desc->object());

  f->Set(NanNew<String>("encodeBinary"),
         NanNew<FunctionTemplate>(EncodeMethod)->GetFunction());
  f->Set(NanNew<String>("decodeBinary"),
         NanNew<FunctionTemplate>(DecodeMethod)->GetFunction());

  f->Set(NanNew<String>("encodeJson"),
         NanNew<FunctionTemplate>(EncodeJsonMethod)->GetFunction());
  f->Set(NanNew<String>("decodeJson"),
         NanNew<FunctionTemplate>(DecodeJsonMethod)->GetFunction());

  return NanEscapeScope(f);
}

ProtoMessage::ProtoMessage(Descriptor* desc, Local<Object> descobj) {
  NanAssignPersistent(desc_js_, descobj);
  desc_ = desc;
}

void ProtoMessage::InitFields(Local<Object> thisobj) {
  NanEscapableScope();

  // Set all fields to default values.
  for (Descriptor::FieldIterator it = desc_->fields_begin();
       it != desc_->fields_end(); ++it) {
    Local<Object> fieldobj = NanNew(*it);
    FieldDescriptor* field = FieldDescriptor::unwrap(fieldobj);
    if (!field) {
      return;
    }
    if (field->oneof()) {
      continue;
    }

    thisobj->SetInternalField(field->LayoutSlot(), NanNew(NewField(field)));
  }

  for (Descriptor::OneofIterator it = desc_->oneofs_begin();
       it != desc_->oneofs_end(); ++it) {
    Local<Object> oneofobj = NanNew(*it);
    OneofDescriptor* oneof = OneofDescriptor::unwrap(oneofobj);
    if (!oneof) {
      return;
    }
    thisobj->SetInternalField(
        oneof->LayoutCaseSlot(), NanNew<Integer>(0));
    thisobj->SetInternalField(
        oneof->LayoutSlot(), NanUndefined());
  }
}

NAN_METHOD(ProtoMessage::New) {
  NanEscapableScope();
  Local<Object> descobj = NanNew(args.Data()->ToObject());
  Descriptor* desc = Descriptor::unwrap(descobj);
  if (!desc) {
    NanReturnUndefined();
  }

  // Allocate a new ProtoMessage and wrap it.
  if (args.IsConstructCall()) {
    ProtoMessage* self = new ProtoMessage(desc, descobj);
    self->Wrap<ProtoMessage>(args.This());
    self->InitFields(args.This());
    if (!self->HandleCtorArgs(args)) {
      NanReturnUndefined();
    }
    NanReturnValue(args.This());
  } else {
    NanReturnValue(desc->Constructor()->NewInstance(0, NULL));
  }
}

bool ProtoMessage::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();

  if (args.Length() == 0) {
    return true;
  } else if (args.Length() == 1) {
    if (!args[0]->IsObject()) {
      NanThrowError("Message class constructor expects object as first argument");
      return false;
    }

    Local<Object> initobj = NanNew(args[0]->ToObject());
    Local<Array> initprops = initobj->GetPropertyNames();
    for (int i = 0; i < initprops->Length(); i++) {
      if (!HandleCtorKeyValue(args.This(),
                              initprops->Get(i),
                              initobj->Get(initprops->Get(i)))) {
        return false;
      }
    }

    return true;
  } else {
    NanThrowError("Message class constructor expects 0 or 1 arguments");
    return false;
  }

  return true;
}

bool ProtoMessage::HandleCtorKeyValue(Local<Object> this_,
                                      Local<Value> key,
                                      Local<Value> value) {
  NanEscapableScope();
  if (!key->IsString()) {
    NanThrowError("Expected string key");
    return false;
  }

  Local<String> key_str = key->ToString();
  return DoFieldSet(this_, key_str, value, /* allow_copy = */ true);
}

static std::string StringEscape(const char* data) {
  std::ostringstream os;
  for (; *data; data++) {
    switch (*data) {
      case '"': os << "\""; break;
      case '\\': os << "\\"; break;
      default: os << *data; break;
    }
  }
  return os.str();
}

static std::string BytesEscape(const char* data, size_t size) {
  std::ostringstream os;
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
  for (size_t i = 0; i < size; i++) {
    os << "\\x" << std::hex << std::setw(2) << std::setfill('0');
    os << (bytes[i] & 0xff);
  }
  return os.str();
}

std::string ProtoMessage::ElementString(upb_fieldtype_t type,
                                        Handle<Object> type_desc,
                                        Handle<Value> value) {
  NanEscapableScope();
  std::ostringstream os;
  Local<String> s;

  switch (type) {
    case UPB_TYPE_STRING:
      s = NanNew(value->ToString());
      os << "\"" << StringEscape(*String::Utf8Value(s)) << "\"";
      break;
    case UPB_TYPE_BYTES:
      os << "\"" << BytesEscape(node::Buffer::Data(value),
                                node::Buffer::Length(value)) << "\"";
      break;
    case UPB_TYPE_ENUM: {
      int32_t int32value = value->Int32Value();
      EnumDescriptor* enumdesc = EnumDescriptor::unwrap(type_desc);
      if (enumdesc) {
        const char* symbolic_name = enumdesc->enumdef()->
            FindValueByNumber(int32value);
        if (symbolic_name) {
          os << symbolic_name;
        } else {
          os << int32value;
        }
      } else {
        os << int32value;
      }
      break;
    }
    default:
      s = NanNew(value->ToString());
      os << *String::Utf8Value(s);
      break;
  }

  return os.str();
}

static void ConvertToString(FieldDescriptor* field,
                            Handle<Value> value,
                            std::ostringstream* os) {
  NanEscapableScope();

  if (field->IsMapField() ||
      field->fielddef()->IsSequence()) {
    Local<String> s = NanNew(value->ToString());
    (*os) << *String::Utf8Value(s);
    return;
  } else {
    (*os) << ProtoMessage::ElementString(field->fielddef()->type(),
                                         field->subtype(), value);
  }
}

NAN_METHOD(ProtoMessage::ToString) {
  NanEscapableScope();
  ProtoMessage* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  std::ostringstream os;

  os << "{ ";

  bool first = true;
  for (Descriptor::FieldIterator it = self->desc_->fields_begin();
       it != self->desc_->fields_end(); ++it) {
    Local<Object> fieldobj = NanNew(*it);
    FieldDescriptor* field = FieldDescriptor::unwrap(fieldobj);
    if (!field) {
      NanReturnUndefined();
    }
    Local<Value> fieldval = NanNew(
        args.This()->GetInternalField(field->LayoutSlot()));

    if (first) {
      first = false;
    } else {
      os << " ";
    }

    os << field->fielddef()->name() << ": ";
    ConvertToString(field, fieldval, &os);
  }

  os << " }";

  NanReturnValue(NanNew<String>(os.str().data(), os.str().size()));
}

NAN_PROPERTY_GETTER(ProtoMessage::MsgFieldGetter) {
  NanEscapableScope();
  ProtoMessage* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  std::string key(*String::Utf8Value(property));

  // Refuse to intercept when an access to an existing property or method (e.g.,
  // this.toString()) is called. Ordinary implementations of message-getter
  // interceptors would simply handle their known intercepted names and
  // return "not intercepted" otherwise, allowing ordinary fallthrough to handle
  // this case. However, we want to catch accesses to unknown field names, so we
  // explicitly try a real lookup (which will hit e.g. methods defined in the
  // prototype object) first, then try looking up the field or oneof with this
  // name, and throw an exception if none is found.
  Local<Value> real_lookup =
      args.This()->GetPrototype()->ToObject()->Get(property);
  if (!real_lookup.IsEmpty() && !real_lookup->IsUndefined()) {
    NanReturnValue(real_lookup);
  }

  // Try looking up a field of this name first.
  FieldDescriptor* field = self->desc_->LookupFieldByName(key);
  if (field) {
    if (field->oneof()) {
      int32_t oneof_case = args.This()->GetInternalField(
          field->oneof()->LayoutCaseSlot())->Uint32Value();
      if (oneof_case != field->fielddef()->number()) {
        NanReturnValue(NanUndefined());
      }
    }
    NanReturnValue(args.This()->GetInternalField(field->LayoutSlot()));
  }

  // Try looking up a oneof case next.
  OneofDescriptor* oneof = self->desc_->LookupOneofByName(key);
  if (oneof) {
    // Look up the field and return the field name.
    int32_t oneof_case = args.This()->GetInternalField(
        oneof->LayoutCaseSlot())->Uint32Value();
    if (oneof_case == 0) {
      NanReturnValue(NanUndefined());
    }

    FieldDescriptor* desc = self->desc_->LookupFieldByNumber(oneof_case);
    assert(desc != NULL);
    NanReturnValue(NanNew<String>(desc->fielddef()->name()));
  }

  // Not found: throw an exception.
  NanThrowError("Unknown field name");
  NanReturnUndefined();
}

Handle<Value> ProtoMessage::CheckConvertElement(
    upb_fieldtype_t type, Descriptor* submsg, Local<Value> value,
    bool allow_null, bool allow_copy) {
  NanEscapableScope();

  switch (type) {
    case UPB_TYPE_INT32: {
      if (value->IsInt32()) {
        return value;
      } else if (value->IsUint32()) {
        if (value->Uint32Value() > INT32_MAX) {
          NanThrowError("Value out of range for int32 field");
          return Handle<Value>();
        }
        return NanEscapeScope(NanNew<Integer>(value->Int32Value()));
      } else if (value->IsNumber()) {
        double val = value->NumberValue();
        if (floor(val) != val) {
          NanThrowError("Non-integral value for int32 field");
          return Handle<Value>();
        }
        if (val < INT32_MIN || val > INT32_MAX) {
          NanThrowError("Value out of range for int32 field");
          return Handle<Value>();
        }
        return NanEscapeScope(NanNew<Integer>(val));
      } else {
        NanThrowError("Invalid type for int32 field");
        return Handle<Value>();
      }
    }
    case UPB_TYPE_UINT32: {
      if (value->IsUint32()) {
        return value;
      } else if (value->IsInt32()) {
        if (value->Int32Value() < 0) {
          NanThrowError("Value out of range for uint32 field");
          return Handle<Value>();
        }
        return NanEscapeScope(NanNewUInt32(value->Uint32Value()));
      } else if (value->IsNumber()) {
        double val = value->NumberValue();
        if (floor(val) != val) {
          NanThrowError("Non-integral value for uint32 field");
          return Handle<Value>();
        }
        if (val < 0 || val > UINT32_MAX) {
          NanThrowError("Value out of range for uint32 field");
          return Handle<Value>();
        }
        return NanEscapeScope(NanNewUInt32(val));
      } else {
        NanThrowError("Invalid type for uint32 field");
        return Handle<Value>();
      }
    }
    case UPB_TYPE_INT64: {
      if (!value->IsObject() ||
          value->ToObject()->GetPrototype() != Int64::prototype_signed) {
        NanThrowError("Expected protobuf.Int64 instance for int64 field");
        return Handle<Value>();
      }
      return value;
    }
    case UPB_TYPE_UINT64: {
      if (!value->IsObject() ||
          value->ToObject()->GetPrototype() != Int64::prototype_unsigned) {
        NanThrowError("Expected protobuf.UInt64 instance for uint64 field");
        return Handle<Value>();
      }
      return value;
    }
    case UPB_TYPE_ENUM: {
      if (!value->IsUint32()) {
        NanThrowError("Expected uint32 value for enum field");
        return Handle<Value>();
      }
      return value;
    }
    case UPB_TYPE_BOOL:
      if (!value->IsBoolean()) {
        NanThrowError("Boolean expected for bool field");
        return Handle<Value>();
      }
      return value;
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_DOUBLE:
      if (!value->IsNumber() && !value->IsInt32() && !value->IsUint32()) {
        NanThrowError("Number expected for float/double field");
        return Handle<Value>();
      }
      if (value->IsInt32()) {
        return NanEscapeScope(NanNew<Number>(value->Int32Value()));
      } else if (value->IsUint32()) {
        return NanEscapeScope(NanNew<Number>(value->Uint32Value()));
      }
      return value;
    case UPB_TYPE_STRING:
      if (!value->IsString()) {
        NanThrowError("String expected for string field");
        return Handle<Value>();
      }
      return value;
    case UPB_TYPE_BYTES:
      if (!value->IsObject() || !node::Buffer::HasInstance(value)) {
        NanThrowError("Buffer expected for bytes field");
        return Handle<Value>();
      }
      return value;
    case UPB_TYPE_MESSAGE: {
      if (value->IsUndefined()) {
        if (allow_null) {
          return value;
        } else {
          NanThrowError("Cannot assign a null (non-present) message in "
                        "this context");
          return Handle<Value>();
        }
      }
      if (!value->IsObject()) {
        NanThrowError("Expected object for submessage field");
        return Handle<Value>();
      }
      Local<Object> obj = value->ToObject();
      if (obj->GetPrototype() != submsg->Prototype()) {
        if (allow_copy) {
          Handle<Value> args[1] = { obj };
          Local<Object> converted = submsg->Constructor()->
              NewInstance(1, args);
          return NanEscapeScope(converted);
        } else {
          NanThrowError("Object of wrong type assigned to submessage field");
          return Handle<Value>();
        }
      }
      return value;
    }
    default:
      assert(false);
      return value;
  }
}

// Helper: check the value type assigned to a field, and convert it to a
// canonical type if necessary.
Handle<Value> ProtoMessage::CheckField(FieldDescriptor* field,
                                       Local<Value> value,
                                       bool allow_copy,
                                       bool allow_null) {
  NanEscapableScope();
  if (field->IsMapField()) {
    if (allow_copy && value->IsObject()) {
      Local<Object> obj = value->ToObject();

      // Value given is an object -- this is OK if we allow for copying
      // conversion (e.g., in a message's constructor arguments). Copy to a Map
      // as long as all elements have the correct type.
      Local<Object> map_obj = NanNew(NewField(field)->ToObject());
      Map* m = Map::unwrap(map_obj);
      if (!m) {
        return Handle<Value>();
      }

      Local<Array> propnames = obj->GetOwnPropertyNames();
      for (int i = 0; i < propnames->Length(); i++) {
        Local<Value> key = propnames->Get(i);
        Local<Value> val = obj->Get(key);
        if (!m->InternalSet(key, val, allow_copy)) {
          return Handle<Value>();
        }
      }

      return NanEscapeScope(map_obj);
    } else if (!value->IsObject()) {
      NanThrowError("Value assigned to map field is not an object");
      return Handle<Value>();
    } else {
      // Any other object -- must be a Map instance of the correct type.
      Map* m = Map::unwrap(value);
      if (!m) {
        return Handle<Value>();
      }
      if (m->key_type() != field->key_field()->fielddef()->type() ||
          m->value_type() != field->value_field()->fielddef()->type() ||
          (m->value_type() == UPB_TYPE_MESSAGE &&
           m->submsg() != field->value_field()->submsg()) ||
          (m->value_type() == UPB_TYPE_ENUM &&
           m->subenum() != field->value_field()->subenum())) {
        NanThrowError("Map instance assigned to map field "
                      "does not match map field's type");
        return Handle<Value>();
      }

      return value;
    }
  } else if (field->fielddef()->IsSequence()) {
    if (allow_copy && value->IsArray()) {
      // Value given is an array -- this is OK if we allow for copying
      // conversion (e.g., in a message's constructor arguments). Copy to a
      // RepeatedField as long as all elements have the correct type.
      Local<Array> arr = value.As<Array>();
      Local<Object> rptfield_obj = NanNew(NewField(field)->ToObject());
      RepeatedField* rptfield = RepeatedField::unwrap(rptfield_obj);
      if (!rptfield) {
        return Handle<Value>();
      }
      for (int i = 0; i < arr->Length(); i++) {
        if (!rptfield->DoPush(NanNew(arr->Get(i)), allow_copy)) {
          return Handle<Value>();
        }
      }
      return NanEscapeScope(rptfield_obj);
    } else if (!value->IsObject()) {
      // Not an array, not an object -- invalid.
      NanThrowError("Value assigned to repeated field is not an object");
      return Handle<Value>();
    } else {
      // Any other object -- must be a RepeatedField instance of the correct
      // type.
      RepeatedField* rptfield = RepeatedField::unwrap(value);
      if (!rptfield) {
        return Handle<Value>();
      }
      if (rptfield->type() != field->fielddef()->type() ||
          (rptfield->type() == UPB_TYPE_MESSAGE &&
           rptfield->submsg() != field->submsg()) ||
          (rptfield->type() == UPB_TYPE_ENUM &&
           rptfield->subenum() != field->subenum())) {
        NanThrowError("RepeatedField instance assigned to repeated field "
                      "does not match repeated field's type");
        return Handle<Value>();
      }

      return value;
    }
  } else {
    return NanEscapeScope(NanNew(
                CheckConvertElement(field->fielddef()->type(),
                    field->submsg(), value, allow_null, allow_copy)));
  }
}

Handle<Value> ProtoMessage::NewElement(upb_fieldtype_t type) {
  NanEscapableScope();
  switch (type) {
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:
      return NanEscapeScope(NanNew<Integer>(0));
    case UPB_TYPE_UINT32:
      return NanEscapeScope(NanNewUInt32(0));
    case UPB_TYPE_INT64:
      return NanEscapeScope(Int64::NewInt64(0));
    case UPB_TYPE_UINT64:
      return NanEscapeScope(Int64::NewUInt64(0));
    case UPB_TYPE_BOOL:
      return NanEscapeScope(NanFalse());
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_DOUBLE:
      return NanEscapeScope(NanNew<Number>(0.0));
    case UPB_TYPE_STRING:
      return NanEscapeScope(NanNew<String>(""));
    case UPB_TYPE_BYTES:
      return NanEscapeScope(NewNodeBuffer(NULL, 0));
    case UPB_TYPE_MESSAGE:
      return NanEscapeScope(NanUndefined());
    default:
      assert(false);
      return NanEscapeScope(NanUndefined());
  }
}

Handle<Value> ProtoMessage::NewField(FieldDescriptor* desc) {
  NanEscapableScope();
  if (desc->IsMapField()) {
    Local<Object> m;
    Local<Value> key_type_arg =
        NanNew<Integer>(static_cast<int>(desc->key_field()->
                                         fielddef()->type()));
    Local<Value> value_type_arg =
        NanNew<Integer>(static_cast<int>(desc->value_field()->
                                         fielddef()->type()));
    Local<Value> value_type_desc = NanNew(desc->value_field()->subtype());
    if (!value_type_desc.IsEmpty()) {
      Handle<Value> args[3] = {
        key_type_arg,
        value_type_arg,
        value_type_desc,
      };
      m = NanNew(NanNew(Map::constructor)->NewInstance(3, args));
    } else {
      Handle<Value> args[2] = {
        key_type_arg,
        value_type_arg,
      };
      m = NanNew(NanNew(Map::constructor)->NewInstance(2, args));
    }

    return NanEscapeScope(m);
  } else if (desc->fielddef()->IsSequence()) {
    Local<Object> rptfield;
    Local<Value> type_arg =
        NanNew<Integer>(static_cast<int>(desc->fielddef()->type()));
    Local<Value> type_desc = NanNew(desc->subtype());
    if (!type_desc.IsEmpty()) {
      Handle<Value> args[2] = {
        type_arg,
        type_desc,
      };
      rptfield = NanNew(NanNew(RepeatedField::constructor)->NewInstance(2, args));
    } else {
      rptfield = NanNew(NanNew(RepeatedField::constructor)->NewInstance(1, &type_arg));
    }

    return NanEscapeScope(rptfield);
  } else {
    // We need to create a local (NanNew) in the current HandleScope and then
    // escape, rather than simply pass the Handle<Value> through, because
    // NewElement's EscapeScope on return only passes ownership up *one* level.
    return NanEscapeScope(NanNew(NewElement(desc->fielddef()->type())));
  }
}

bool ProtoMessage::DoFieldSet(Local<Object> this_,
                              Local<String> property,
                              Local<Value> value,
                              bool allow_copy) {
  NanEscapableScope();
  ProtoMessage* self = unwrap(this_);
  if (!self) {
    return false;
  }
  std::string key(*String::Utf8Value(property));

  // Try looking up a field of this name first.
  FieldDescriptor* field = self->desc_->LookupFieldByName(key);
  if (field) {

    if (field->oneof()) {
      // Assign the oneof case. Note that assigning 'null' to a field that's
      // part of a oneof clears the oneof.
      int32_t new_case = field->fielddef()->number();
      if (value->IsUndefined()) {
        new_case = 0;
      }
      this_->SetInternalField(
          field->oneof()->LayoutCaseSlot(), NanNew<Integer>(new_case));

      if (new_case == 0) {
        return true;
      }
    }

    // Type-check the new value.
    value = NanNew(CheckField(field, value, allow_copy, true));
    if (value.IsEmpty()) {
      return false;
    }

    this_->SetInternalField(field->LayoutSlot(), value);
    return true;
  }

  // Not found: throw an exception. (We don't allow assignment to oneof
  // properties so there is no case for oneofs here corresponding to the oneof
  // getter case above.)
  NanThrowError("Unknown field name");
  return false;
}

NAN_PROPERTY_SETTER(ProtoMessage::MsgFieldSetter) {
  NanEscapableScope();
  ProtoMessage* self = unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  self->DoFieldSet(args.This(), property, value, /* allow_copy = */ false);
  NanReturnValue(value);
}

NAN_PROPERTY_ENUMERATOR(ProtoMessage::MsgFieldEnumerator) {
  NanEscapableScope();
  ProtoMessage* self = unwrap(args.This());
  if (!self) {
    NanReturnValue(NanNew<Array>());
  }
  std::vector<std::string> names;

  for (Descriptor::FieldIterator it = self->desc_->fields_begin();
       it != self->desc_->fields_end(); ++it) {
    FieldDescriptor* desc = FieldDescriptor::unwrap(NanNew(*it));
    names.push_back(desc->fielddef()->name());
  }
  for (Descriptor::OneofIterator it = self->desc_->oneofs_begin();
       it != self->desc_->oneofs_end(); ++it) {
    OneofDescriptor* desc = OneofDescriptor::unwrap(NanNew(*it));
    names.push_back(desc->oneofdef()->name());
  }

  Local<Array> names_ary = NanNew<Array>(names.size());
  for (int i = 0; i < names.size(); i++) {
    names_ary->Set(i, NanNew<String>(names[i]));
  }

  NanReturnValue(names_ary);
}

}  // namespace protobuf_js
