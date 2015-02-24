#include <nan.h>
#include "defs.h"
#include "message.h"
#include "int64.h"
#include "map.h"
#include "readonlyarray.h"
#include "util.h"

#include <assert.h>
#include <string>
#include <sstream>

using namespace v8;
using namespace node;

namespace protobuf_js {

Persistent<Function> Map::constructor;
Persistent<Value> Map::prototype;

JS_OBJECT_INIT(Map);

void Map::Init(Handle<Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->SetClassName(NanNew<String>("Map"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->InstanceTemplate()->SetAccessor(
      NanNew("keys"), KeysGetter);
  tpl->InstanceTemplate()->SetAccessor(
      NanNew("values"), ValuesGetter);
  tpl->InstanceTemplate()->SetAccessor(
      NanNew("entries"), EntriesGetter);
  tpl->InstanceTemplate()->SetAccessor(
      NanNew("keyType"), KeyTypeGetter);
  tpl->InstanceTemplate()->SetAccessor(
      NanNew("valueType"), ValueTypeGetter);
  tpl->InstanceTemplate()->SetAccessor(
      NanNew("valueSubDesc"), ValueSubDescGetter);

  // Set indexed and named JavaScript property setter handlers that throw
  // exceptions so that the user cannot mistakenly set regular object properties
  // (which would not actually be added to the map).
  tpl->InstanceTemplate()->SetNamedPropertyHandler(
      0, NamedPropertySetter);
  tpl->InstanceTemplate()->SetIndexedPropertyHandler(
      0, IndexedPropertySetter);


#define M(jsname, cppname)                                   \
  tpl->PrototypeTemplate()->Set(                             \
      NanNew<String>(#jsname),                               \
      NanNew<FunctionTemplate>(cppname)->GetFunction())

  M(get, Get);
  M(set, Set);
  M(delete, Delete);
  M(clear, Clear);
  M(has, Has);
  M(toString, ToString);
  M(newEmpty, NewEmpty);

#undef M

  NanAssignPersistent(constructor, tpl->GetFunction());
  // Construct an instance in order to get the prototype object.
  Local<Value> arg = NanNew<Integer>(UPB_TYPE_INT32);
  Handle<Value> args[2] = { arg, arg };
  NanAssignPersistent(prototype,
                      NanNew(constructor)->NewInstance(2, args)->GetPrototype());

  exports->Set(NanNew<String>("Map"), NanNew(constructor));
}

Map::Map() {
  // These will be set properly by New().
  key_type_ = UPB_TYPE_INT32;
  value_type_ = UPB_TYPE_INT32;
  submsg_ = NULL;
  subenum_ = NULL;
}

NAN_METHOD(Map::New) {
  NanEscapableScope();
  if (!args.IsConstructCall()) {
    NanThrowError("Not called as constructor");
    NanReturnUndefined();
  }

  Map* self = new Map();
  self->Wrap<Map>(args.This());
  if (!self->HandleCtorArgs(args)) {
    NanReturnUndefined();
  }
  NanReturnValue(args.This());
}

bool Map::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();

  if (args.Length() < 2) {
    NanThrowError("Map constructor requires at least two args: "
                  "key and value type");
    return false;
  } else {
    // Two-arg form:   (key_type, value_type).
    // Three-arg form: (key_type, value_type, {initial_contents}).
    // Three-arg form: (key_type, value_type, value_msgclass_or_enum).
    // Four-arg form:  (key_type, value_type, value_msgclass_or_enum,
    //                 {initial_contents}).

    Local<Value> key_type_value = NanNew(args[0]);
    if (!FieldDescriptor::ParseTypeValue(key_type_value, &key_type_)) {
      return false;
    }

    switch (key_type_) {
      case UPB_TYPE_INT32:
      case UPB_TYPE_UINT32:
      case UPB_TYPE_INT64:
      case UPB_TYPE_UINT64:
      case UPB_TYPE_BOOL:
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        // Acceptable key types.
        break;
      default:
        NanThrowError("Invalid key type for Map instance");
        return false;
    }

    Local<Value> value_type_value = NanNew(args[1]);
    if (!FieldDescriptor::ParseTypeValue(value_type_value, &value_type_)) {
      return false;
    }

    if (value_type_ == UPB_TYPE_MESSAGE && args.Length() > 2) {
      Local<Value> msgclass_or_desc = NanNew(args[2]);
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
        NanThrowError("Expected message class or descriptor as third "
                      "argument to Map constructor");
        return false;
      }

      if (descriptor->GetPrototype() != Descriptor::prototype) {
        NanThrowError("Invalid descriptor object");
        return false;
      }

      submsg_ = Descriptor::unwrap(descriptor);
    }

    if (value_type_ == UPB_TYPE_ENUM && args.Length() > 2) {
      if (!args[2]->IsObject()) {
        NanThrowError("Expected EnumDescriptor or enum object");
        return false;
      }
      Local<Object> enumobj_or_desc = NanNew(args[2]->ToObject());
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

    if (value_type_ == UPB_TYPE_MESSAGE && !submsg_) {
      NanThrowError("Map created with message type but no submsg");
      return false;
    }
    if (value_type_ == UPB_TYPE_ENUM && !subenum_) {
      NanThrowError("Map created with enum type but no subenum");
      return false;
    }

    int init_map_idx = (submsg_ != NULL || subenum_ != NULL) ? 3 : 2;
    if (args.Length() > init_map_idx) {
      if (!args[init_map_idx]->IsObject()) {
        NanThrowError("Initial map content arg is not an object");
        return false;
      }
      Local<Object> init_map = NanNew(args[init_map_idx]->ToObject());
      Local<Array> propnames = init_map->GetOwnPropertyNames();
      for (int i = 0; i < propnames->Length(); i++) {
        Local<Value> key = propnames->Get(i);
        Local<Value> value = init_map->Get(key);
        if (!InternalSet(key, value, /* allow_copy = */ false)) {
          return false;
        }
      }
    }

    if (args.Length() > init_map_idx + 1) {
      NanThrowError("Too many arguments to Map constructor");
      return false;
    }

    return true;
  }
}

bool Map::ComputeKey(Handle<Value> key, std::string* keydata) const {
  NanEscapableScope();

  // Perform whatever checks and conversions are generally allowed and/or
  // required for values of this field type.
  Local<Value> k = NanNew(ProtoMessage::CheckConvertElement(
      key_type_, NULL, NanNew(key), /* allow_null = */ false,
      /* allow_copy = */ false));
  if (k.IsEmpty() || k->IsUndefined()) {
    return false;
  }

  switch (key_type_) {
    case UPB_TYPE_INT32: {
      int32_t value = k->Int32Value();
      keydata->assign(reinterpret_cast<char*>(&value), sizeof(value));
      return true;
    }

    case UPB_TYPE_UINT32: {
      uint32_t value = k->Uint32Value();
      keydata->assign(reinterpret_cast<char*>(&value), sizeof(value));
      return true;
    }

    case UPB_TYPE_INT64: {
      Int64* i64 = Int64::unwrap(k);
      if (!i64 || !i64->IsSigned()) {
        NanThrowError("Key is not an Int64 instance");
        return false;
      }
      int64_t value = i64->int64_value();
      keydata->assign(reinterpret_cast<char*>(&value), sizeof(value));
      return true;
    }
    case UPB_TYPE_UINT64: {
      Int64* i64 = Int64::unwrap(k);
      if (!i64 || i64->IsSigned()) {
        NanThrowError("Key is not a UInt64 instance");
        return false;
      }
      uint64_t value = i64->uint64_value();
      keydata->assign(reinterpret_cast<char*>(&value), sizeof(value));
      return true;
    }

    case UPB_TYPE_BOOL: {
      bool value = k->BooleanValue();
      keydata->assign(reinterpret_cast<char*>(&value), sizeof(value));
      return true;
    }

    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING: {
      // For use as a map key, we don't want to force any (potentially costly)
      // UTF-8 encoding/decoding (string) or 16-bit-to-8-bit-character
      // conversion (bytes), so both string and bytes fields simply use the 16
      // bit characters in memory as-is. The string's content will be
      // implementation-dependent (depends on endianness) but it will preserve
      // uniqueness and equality so it's suitable for use as a key.
      Local<String> str = k->ToString();
      String::Value val(str);
      uint16_t* widechars = *val;
      size_t bytelength = str->Length() * sizeof(uint16_t);
      keydata->assign(reinterpret_cast<char*>(widechars), bytelength);
      return true;
    }

    default:
      NanThrowError("Invalid key type");
      return false;
  }
}

Handle<Value> Map::ExtractKey(std::string data) const {
  NanEscapableScope();
  switch (key_type_) {
    case UPB_TYPE_INT32: {
      int32_t value;
      assert(data.size() == sizeof(value));
      memcpy(&value, data.data(), sizeof(int32_t));
      return NanEscapeScope(NanNew<Integer>(value));
    }
    case UPB_TYPE_UINT32: {
      uint32_t value;
      assert(data.size() == sizeof(value));
      memcpy(&value, data.data(), sizeof(uint32_t));
      return NanEscapeScope(NanNewUInt32(value));
    }
    case UPB_TYPE_INT64: {
      int64_t value;
      assert(data.size() == sizeof(value));
      memcpy(&value, data.data(), sizeof(int64_t));
      Local<Object> int64_obj =
          NanNew(Int64::constructor_signed)->NewInstance(0, NULL);
      Int64::unwrap(int64_obj)->set_int64_value(value);
      return NanEscapeScope(int64_obj);
    }
    case UPB_TYPE_UINT64: {
      uint64_t value;
      assert(data.size() == sizeof(value));
      memcpy(&value, data.data(), sizeof(uint64_t));
      Local<Object> uint64_obj =
          NanNew(Int64::constructor_unsigned)->NewInstance(0, NULL);
      Int64::unwrap(uint64_obj)->set_uint64_value(value);
      return NanEscapeScope(uint64_obj);
    }
    case UPB_TYPE_BOOL: {
      bool value;
      assert(data.size() == sizeof(value));
      memcpy(&value, data.data(), sizeof(bool));
      return NanEscapeScope(value ? NanTrue() : NanFalse());
    }
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      assert((data.size() & 1) == 0);  // even number of bytes
      const uint16_t* widechars =
          reinterpret_cast<const uint16_t*>(data.data());
      size_t length = data.size() / 2;
      Local<String> str = NanNew<String>(widechars, length);
      return NanEscapeScope(str);
    }
    default:
      NanThrowError("Invalid key type");
      return Handle<Value>();
  }
}

Handle<Value> Map::InternalGet(Handle<Value> key) {
  NanEscapableScope();
  std::string keydata;
  if (!ComputeKey(key, &keydata)) {
    return Handle<Value>();
  }

  ValueMap::iterator it = map_.find(keydata);
  if (it == map_.end()) {
    return Handle<Value>();
  }
  return NanEscapeScope(NanNew(it->second));
}

bool Map::InternalHas(Handle<Value> key, bool* has) {
  NanEscapableScope();
  std::string keydata;
  if (!ComputeKey(key, &keydata)) {
    return false;
  }

  ValueMap::iterator it = map_.find(keydata);
  *has = (it != map_.end());
  return true;
}

bool Map::InternalSet(Handle<Value> key, Handle<Value> value,
                      bool allow_copy) {
  std::string keydata;
  if (!ComputeKey(key, &keydata)) {
    return false;
  }
  return InternalSet(keydata, value, allow_copy);
}

bool Map::InternalSet(std::string encoded_data, Handle<Value> value,
                      bool allow_copy) {
  NanEscapableScope();
  Local<Value> converted_val =
      NanNew(ProtoMessage::CheckConvertElement(
          value_type_, submsg_, NanNew(value), /* allow_null = */ false,
          allow_copy));
  if (converted_val.IsEmpty() || converted_val->IsUndefined()) {
    return false;
  }

  NanAssignPersistent(map_[encoded_data], converted_val);
  return true;
}

bool Map::InternalDelete(v8::Handle<v8::Value> key, bool* deleted) {
  NanEscapableScope();
  std::string keydata;
  if (!ComputeKey(key, &keydata)) {
    return false;
  }

  ValueMap::iterator it = map_.find(keydata);
  if (it == map_.end()) {
    *deleted = false;
    return true;
  } else {
    map_.erase(it);
    *deleted = true;
    return true;
  }
}

NAN_METHOD(Map::Get) {
  NanEscapableScope();
  if (args.Length() != 1) {
    NanThrowError("Expected one argument");
    NanReturnUndefined();
  }

  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  NanReturnValue(NanNew(self->InternalGet(args[0])));
}

NAN_METHOD(Map::Has) {
  NanEscapableScope();
  if (args.Length() != 1) {
    NanThrowError("Expected one argument");
    NanReturnUndefined();
  }

  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  bool has = false;
  if (!self->InternalHas(args[0], &has)) {
    NanReturnUndefined();
  }
  NanReturnValue(has ? NanTrue() : NanFalse());
}

NAN_METHOD(Map::Set) {
  NanEscapableScope();
  if (args.Length() != 2) {
    NanThrowError("Expected two arguments");
    NanReturnUndefined();
  }

  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  self->InternalSet(args[0], args[1], /* allow_copy = */ false);
  NanReturnValue(args.This());
}

NAN_METHOD(Map::Delete) {
  NanEscapableScope();
  if (args.Length() != 1) {
    NanThrowError("Expected one argument");
    NanReturnUndefined();
  }

  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  bool deleted = false;
  if (!self->InternalDelete(args[0], &deleted)) {
    NanReturnUndefined();
  } else {
    NanReturnValue(deleted ? NanTrue() : NanFalse());
  }
}

NAN_METHOD(Map::Clear) {
  NanEscapableScope();
  if (args.Length() != 0) {
    NanThrowError("Expected zero arguments");
    NanReturnUndefined();
  }

  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  self->map_.clear();
  NanReturnUndefined();
}

NAN_METHOD(Map::ToString) {
  NanEscapableScope();
  if (args.Length() != 0) {
    NanThrowError("Expected no arguments");
    NanReturnUndefined();
  }

  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }

  Local<Object> key_type_desc;  // remains empty
  Local<Object> value_type_desc;
  if (self->value_type_ == UPB_TYPE_MESSAGE) {
    value_type_desc = NanNew(self->submsg_->object());
  } else if (self->value_type_ == UPB_TYPE_ENUM) {
    value_type_desc = NanNew(self->subenum_->object());
  }

  std::ostringstream os;
  os << "[ ";

  bool first = true;
  for (ValueMap::iterator it = self->map_.begin();
       it != self->map_.end(); ++it) {
    NanEscapableScope();
    Local<Value> key = NanNew(self->ExtractKey(it->first));
    if (key.IsEmpty() || key->IsUndefined()) {
      NanReturnUndefined();
    }

    if (first) {
      first = false;
    } else {
      os << ", ";
    }

    os << "{ key: ";
    os << ProtoMessage::ElementString(
        self->key_type_, key_type_desc, key);
    os << " value: ";
    os << ProtoMessage::ElementString(
        self->value_type_, value_type_desc, NanNew(it->second));
    os << " }";
  }

  os << " ]";

  NanReturnValue(
              NanNew<String>(os.str().data(), os.str().size()));
}

NAN_GETTER(Map::KeysGetter) {
  NanEscapableScope();
  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }

  ReadOnlyArray::Builder builder;
  for (ValueMap::iterator it = self->map_.begin();
       it != self->map_.end(); ++it) {
    Local<Value> key = NanNew(self->ExtractKey(it->first));
    if (key.IsEmpty() || key->IsUndefined()) {
      NanReturnUndefined();
    }
    builder.Add(key);
  }

  NanReturnValue(NanNew(builder.Build()));
}

NAN_GETTER(Map::ValuesGetter) {
  NanEscapableScope();
  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }

  ReadOnlyArray::Builder builder;
  for (ValueMap::iterator it = self->map_.begin();
       it != self->map_.end(); ++it) {
    builder.Add(NanNew(it->second));
  }

  NanReturnValue(NanNew(builder.Build()));
}

NAN_GETTER(Map::EntriesGetter) {
  NanEscapableScope();
  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }

  ReadOnlyArray::Builder builder;
  for (ValueMap::iterator it = self->map_.begin();
       it != self->map_.end(); ++it) {
    Local<Value> key = NanNew(self->ExtractKey(it->first));
    if (key.IsEmpty() || key->IsUndefined()) {
      NanReturnUndefined();
    }
    Local<Array> pair = NanNew<Array>(2);
    pair->Set(0, key);
    pair->Set(1, NanNew(it->second));
    builder.Add(pair);
  }

  NanReturnValue(NanNew(builder.Build()));
}

NAN_PROPERTY_SETTER(Map::NamedPropertySetter) {
  NanThrowError("Maps do not accept ordinary JavaScript properties. "
                "Please use set() to set map entries.");
  NanReturnUndefined();
}

NAN_INDEX_SETTER(Map::IndexedPropertySetter) {
  NanThrowError("Maps do not accept ordinary JavaScript properties. "
                "Please use set() to set map entries.");
  NanReturnUndefined();
}

NAN_GETTER(Map::KeyTypeGetter) {
  NanEscapableScope();
  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  NanReturnValue(NanNew<Integer>(self->key_type_));
}

NAN_GETTER(Map::ValueTypeGetter) {
  NanEscapableScope();
  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  NanReturnValue(NanNew<Integer>(self->value_type_));
}

NAN_GETTER(Map::ValueSubDescGetter) {
  NanEscapableScope();
  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  if (self->value_type_ == UPB_TYPE_MESSAGE) {
    NanReturnValue(NanNew(self->submsg_->object()));
  } else if (self->value_type_ == UPB_TYPE_ENUM) {
    NanReturnValue(NanNew(self->subenum_->object()));
  } else {
    NanReturnUndefined();
  }
}

NAN_METHOD(Map::NewEmpty) {
  NanEscapableScope();
  Map* self = Map::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }

  Handle<Value> argv[3];
  int argc = 2;
  argv[0] = NanNew<Integer>(self->key_type_);
  argv[1] = NanNew<Integer>(self->value_type_);
  if (self->value_type_ == UPB_TYPE_MESSAGE) {
    argv[2] = self->submsg_->object();
    argc = 3;
  } else if (self->value_type_ == UPB_TYPE_ENUM) {
    argv[2] = self->subenum_->object();
    argc = 3;
  }

  Local<Value> new_map =
      NanNew(NanNew(constructor)->NewInstance(argc, argv));
  NanReturnValue(new_map);
}

}  // namespace protobuf_js
