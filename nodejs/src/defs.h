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

#ifndef __PROTOBUF_NODEJS_SRC_DEFS_H__
#define __PROTOBUF_NODEJS_SRC_DEFS_H__

#include <nan.h>
#include <string>
#include <map>
#include <assert.h>
#include "util.h"
#include "jsobject.h"

namespace protobuf_js {

// Forward decls.
class Descriptor;
class FieldDescriptor;
class OneofDescriptor;
class EnumDescriptor;
class DescriptorPool;

// Iterator adaptor to provide an iterator over fields in descriptors/oneofs and
// oneofs in descriptors.
template<typename K>
class V8ObjMapIterator :
  public std::iterator<std::forward_iterator_tag, PERSISTENT(v8::Object) > {
 public:
  typedef std::map<K, PERSISTENT(v8::Object) > StorageMap;
  V8ObjMapIterator(typename StorageMap::iterator it) : it_(it) {}
  V8ObjMapIterator& operator++() { ++it_; return *this; }
  bool operator==(const V8ObjMapIterator& other) const {
    return it_ == other.it_;
  }
  bool operator!=(const V8ObjMapIterator& other) const {
    return it_ != other.it_;
  }
  PERSISTENT(v8::Object) operator*() {
    return it_->second;
  }
  v8::Handle<v8::Object> get() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(it_->second));
  }

 private:
  typename StorageMap::iterator it_;
};

class Descriptor : public JSObject {
 public:
  JS_OBJECT(Descriptor);

  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;
  static v8::Persistent<v8::Value> prototype;

  const upb::MessageDef* msgdef() const { return msgdef_.get(); }

  int LayoutSlots() const {
    assert(layout_computed_);
    return slots_;
  }

  typedef V8ObjMapIterator<int32_t> FieldIterator;
  typedef V8ObjMapIterator<std::string> OneofIterator;

  FieldIterator fields_begin() {
    return FieldIterator(fields_.begin());
  }
  FieldIterator fields_end() {
    return FieldIterator(fields_.end());
  }
  OneofIterator oneofs_begin() {
    return OneofIterator(oneofs_.begin());
  }
  OneofIterator oneofs_end() {
    return OneofIterator(oneofs_.end());
  }

  FieldDescriptor* LookupFieldByName(const std::string& name);
  FieldDescriptor* LookupFieldByNumber(int32_t number);
  OneofDescriptor* LookupOneofByName(const std::string& name);

  v8::Handle<v8::Function> Constructor() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(msgclass_));
  }
  v8::Handle<v8::Object> Prototype() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(msgprototype_));
  }

  // Called at freeze (add-to-pool) time.
  void BuildClass(v8::Local<v8::Object> this_);

  // Generate serialization handlers.
  const upb::Handlers* PbSerializeHandlers();
  const upb::Handlers* JsonSerializeHandlers();
  // Generate parsing handlers.
  const upb::Handlers* FillHandlers();
  const upb::pb::DecoderMethod* DecoderMethod();

  const DescriptorPool* pool() const { return pool_; }

 private:
  friend class DescriptorPool;

  Descriptor();
  ~Descriptor()  {}

  // Methods
  static NAN_METHOD(New);

  static NAN_GETTER(NameGetter);
  static NAN_SETTER(NameSetter);

  static NAN_GETTER(MapEntryGetter);
  static NAN_SETTER(MapEntrySetter);

  static NAN_GETTER(FieldsGetter);
  static NAN_METHOD(FindFieldByName);
  static NAN_METHOD(FindFieldByNumber);
  static NAN_METHOD(AddField);

  static NAN_GETTER(OneofsGetter);
  static NAN_METHOD(FindOneof);
  static NAN_METHOD(AddOneof);

  static NAN_GETTER(MsgClassGetter);

  // Helper to parse/check constructor arguments.
  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);
  // Helpers.
  bool DoNameSetter(v8::Local<v8::Value> value);
  bool DoMapEntrySetter(v8::Local<v8::Value> value);
  bool DoAddField(v8::Local<v8::Object> this_, v8::Local<v8::Object> field);
  bool DoAddOneof(v8::Local<v8::Object> this_, v8::Local<v8::Object> oneof);

  // Data
  upb::reffed_ptr<const upb::MessageDef> msgdef_;
  upb::MessageDef* mutable_msgdef();

  upb::reffed_ptr<const upb::Handlers> pb_serialize_handlers_;
  upb::reffed_ptr<const upb::Handlers> json_serialize_handlers_;
  upb::reffed_ptr<const upb::Handlers> fill_handlers_;
  upb::reffed_ptr<const upb::pb::DecoderMethod> decoder_method_;

  typedef std::map< int32_t, PERSISTENT(v8::Object) > FieldMap;
  FieldMap fields_;
  typedef std::map< std::string, PERSISTENT(v8::Object) > OneofMap;
  OneofMap oneofs_;

  v8::Persistent<v8::Function> msgclass_;
  v8::Persistent<v8::Object> msgprototype_;

  // Create the layout by assigning slot IDs to all fields and oneofs.
  void CreateLayout();

  // How many object slots does a message of this type require?
  int slots_;
  bool layout_computed_;

  DescriptorPool* pool_;
};

class FieldDescriptor : public JSObject {
 public:
  JS_OBJECT(FieldDescriptor);

  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;

  const upb::FieldDef* fielddef() const { return fielddef_.get(); }

  int LayoutSlot() const {
    assert(slot_set_);
    return slot_;
  }

  OneofDescriptor* oneof() const;

  Descriptor* submsg();
  EnumDescriptor* subenum();

  v8::Handle<v8::Object> subtype() const {
    NanEscapableScope();
    return NanEscapeScope(NanNew(subtype_));
  }

  // Exposed for use by RepeatedField and other containers.
  static bool ParseTypeValue(v8::Local<v8::Value> value, upb_fieldtype_t* type);

  static const int kMapKeyField = 1;
  static const int kMapValueField = 2;

  // Is this a map field?
  bool IsMapField() {
    return fielddef_->IsFrozen() &&
           fielddef_->type() == UPB_TYPE_MESSAGE &&
           fielddef_->label() == UPB_LABEL_REPEATED &&
           fielddef_->message_subdef()->mapentry();
  }
  FieldDescriptor* key_field() {
    if (!IsMapField()) {
      return NULL;
    }
    return submsg()->LookupFieldByNumber(kMapKeyField);
  }
  FieldDescriptor* value_field() {
    if (!IsMapField()) {
      return NULL;
    }
    return submsg()->LookupFieldByNumber(kMapValueField);
  }

 private:
  FieldDescriptor();
  ~FieldDescriptor()  {}

  // Methods
  static NAN_METHOD(New);
  static NAN_GETTER(NameGetter);
  static NAN_SETTER(NameSetter);
  static NAN_GETTER(TypeGetter);
  static NAN_SETTER(TypeSetter);
  static NAN_GETTER(NumberGetter);
  static NAN_SETTER(NumberSetter);
  static NAN_GETTER(LabelGetter);
  static NAN_SETTER(LabelSetter);
  static NAN_GETTER(SubTypeNameGetter);
  static NAN_SETTER(SubTypeNameSetter);
  static NAN_GETTER(SubTypeGetter);
  static NAN_SETTER(SubTypeSetter);
  static NAN_GETTER(DescriptorGetter);
  static NAN_GETTER(OneofGetter);

  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);
  bool DoNameSetter(v8::Local<v8::Value> value);
  bool DoTypeSetter(v8::Local<v8::Value> value);
  bool DoNumberSetter(v8::Local<v8::Value> value);
  bool DoLabelSetter(v8::Local<v8::Value> value);
  bool DoSubTypeNameSetter(v8::Local<v8::Value> value);

  void SetSlot(int slot);

  // Data
  upb::reffed_ptr<const upb::FieldDef> fielddef_;
  upb::FieldDef* mutable_fielddef();
  v8::Persistent<v8::Object> descriptor_;
  v8::Persistent<v8::Object> oneof_;
  v8::Persistent<v8::Object> subtype_;

  // Slot in containing message's object layout
  int slot_;
  bool slot_set_;

  friend class Descriptor;
  friend class OneofDescriptor;
  friend class DescriptorPool;
};

class OneofDescriptor : public JSObject {
 public:
  JS_OBJECT(OneofDescriptor);

  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;

  const upb::OneofDef* oneofdef() const { return oneofdef_.get(); }

  int LayoutSlot() const {
    assert(slots_set_);
    return slot_;
  }

  int LayoutCaseSlot() const {
    assert(slots_set_);
    return case_slot_;
  }

  typedef V8ObjMapIterator<int32_t> FieldIterator;
  FieldIterator fields_begin() {
    return FieldIterator(fields_.begin());
  }
  FieldIterator fields_end() {
    return FieldIterator(fields_.end());
  }

 private:
  OneofDescriptor();
  ~OneofDescriptor()  {}

  // Methods
  static NAN_METHOD(New);
  static NAN_GETTER(NameGetter);
  static NAN_SETTER(NameSetter);
  static NAN_GETTER(FieldsGetter);
  static NAN_GETTER(DescriptorGetter);
  static NAN_METHOD(AddField);
  static NAN_METHOD(FindFieldByName);
  static NAN_METHOD(FindFieldByNumber);

  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);
  bool DoNameSetter(v8::Local<v8::Value> value);
  bool DoAddField(v8::Local<v8::Object> _this, v8::Local<v8::Object> fieldobj);

  void SetSlots(int slot, int case_slot);

  // Data
  upb::reffed_ptr<const upb::OneofDef> oneofdef_;
  upb::OneofDef* mutable_oneofdef();
  v8::Persistent<v8::Object> descriptor_;

  friend class Descriptor;
  friend class FieldDescriptor;

  typedef std::map< int32_t, PERSISTENT(v8::Object) > FieldMap;
  FieldMap fields_;

  int slot_;
  int case_slot_;
  bool slots_set_;
};

class EnumDescriptor : public JSObject {
 public:
  JS_OBJECT(EnumDescriptor);

  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;
  static v8::Persistent<v8::Value> prototype;

  const upb::EnumDef* enumdef() const { return enumdef_.get(); }

  v8::Handle<v8::Object> EnumObject() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(enumobj_));
  }

 private:
  friend class DescriptorPool;

  EnumDescriptor();
  ~EnumDescriptor()  {}

  // Methods
  static NAN_METHOD(New);
  static NAN_GETTER(NameGetter);
  static NAN_SETTER(NameSetter);
  static NAN_GETTER(KeysGetter);
  static NAN_GETTER(ValuesGetter);
  static NAN_METHOD(FindByName);
  static NAN_METHOD(FindByValue);
  static NAN_METHOD(Add);

  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);
  bool DoNameSetter(v8::Local<v8::Value> value);
  bool DoAdd(v8::Local<v8::String> key, int32_t value);

  // Data
  upb::reffed_ptr<const upb::EnumDef> enumdef_;
  upb::EnumDef* mutable_enumdef();
  v8::Persistent<v8::Object> enumobj_;

  void BuildObject(v8::Local<v8::Object> this_);
};

class DescriptorPool : public JSObject {
 public:
  JS_OBJECT(DescriptorPool);

  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;

  Descriptor* FindDescByDef(const upb::MessageDef* def) {
    NanEscapableScope();
    ObjPtrMap::iterator it = objptr_.find(upb::upcast(def));
    if (it == objptr_.end()) {
      return NULL;
    }
    v8::Local<v8::Object> obj = NanNew(it->second);
    if (obj->GetPrototype() != Descriptor::prototype) {
      return NULL;
    }
    return Descriptor::unwrap(obj);
  }

 private:
  DescriptorPool();
  ~DescriptorPool()  {}

  // Methods
  static NAN_METHOD(New);
  static NAN_METHOD(Add);
  static NAN_METHOD(Lookup);
  static NAN_GETTER(DescriptorsGetter);
  static NAN_GETTER(EnumsGetter);

  // Data
  upb::reffed_ptr<upb::SymbolTable> symtab_;

  static v8::Persistent<v8::Value> descriptor_prototype_;
  static v8::Persistent<v8::Value> enum_prototype_;

  typedef std::map< std::string, PERSISTENT(v8::Object) > ObjMap;
  ObjMap objs_;
  typedef std::map< const upb::Def*, PERSISTENT(v8::Object) > ObjPtrMap;
  ObjPtrMap objptr_;

  std::vector< PERSISTENT(v8::Object) > descs_;
  std::vector< PERSISTENT(v8::Object) > enums_;
};

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_DEFS_H__
