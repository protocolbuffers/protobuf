#ifndef __PROTOBUF_NODEJS_SRC_MAP_H__
#define __PROTOBUF_NODEJS_SRC_MAP_H__

#include <nan.h>
#include <string>
#include <map>
#include <assert.h>
#include "defs.h"
#include "util.h"
#include "jsobject.h"

namespace protobuf_js {

class Map : public JSObject {
 public:
  JS_OBJECT(Map);

  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;
  static v8::Persistent<v8::Value> prototype;

  upb_fieldtype_t key_type() const { return key_type_; }
  upb_fieldtype_t value_type() const { return value_type_; }
  Descriptor* submsg() const { return submsg_; }
  EnumDescriptor* subenum() const { return subenum_; }

  int Length() const { return map_.size(); }
  v8::Handle<v8::Value> InternalGet(v8::Handle<v8::Value> key);
  bool InternalHas(v8::Handle<v8::Value> key, bool* has);
  bool InternalSet(v8::Handle<v8::Value> key, v8::Handle<v8::Value> value,
                   bool allow_copy);
  bool InternalSet(std::string encoded_key, v8::Handle<v8::Value> value,
                   bool allow_copy);
  bool InternalDelete(v8::Handle<v8::Value> key, bool* deleted);

  // Convert a key to and from a string of bytes, used to index into the native
  // std::map.
  bool ComputeKey(v8::Handle<v8::Value> key, std::string* keydata) const;
  v8::Handle<v8::Value> ExtractKey(std::string data) const;

  typedef std::map< std::string, PERSISTENT(v8::Value) > ValueMap;

  class Iterator {
   public:
    Iterator(const Map& m) : m_(&m) {
      it_ = m_->map_.begin();
    }
    Iterator(const Iterator& other) : m_(other.m_), it_(other.it_) {}

    bool Done() const { return it_ == m_->map_.end(); }
    void Next() { ++it_; }

    v8::Handle<v8::Value> Key() const {
      NanEscapableScope();
      v8::Local<v8::Value> keyvalue = NanNew(m_->ExtractKey(it_->first));
      return NanEscapeScope(keyvalue);
    }

    v8::Handle<v8::Value> Value() const {
      NanEscapableScope();
      return NanEscapeScope(NanNew(it_->second));
    }

   private:
    const Map* m_;
    Map::ValueMap::const_iterator it_;
  };

 private:
  Map();
  ~Map()  {}

  friend class Iterator;

  static NAN_METHOD(New);
  static NAN_METHOD(Get);
  static NAN_METHOD(Set);
  static NAN_METHOD(Delete);
  static NAN_METHOD(Clear);
  static NAN_METHOD(Has);
  static NAN_GETTER(KeysGetter);
  static NAN_GETTER(ValuesGetter);
  static NAN_GETTER(EntriesGetter);
  static NAN_METHOD(ToString);
  static NAN_METHOD(NewEmpty);
  static NAN_PROPERTY_SETTER(NamedPropertySetter);
  static NAN_INDEX_SETTER(IndexedPropertySetter);

  static NAN_GETTER(KeyTypeGetter);
  static NAN_GETTER(ValueTypeGetter);
  static NAN_GETTER(ValueSubDescGetter);

  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);

  ValueMap map_;
  upb_fieldtype_t key_type_;
  upb_fieldtype_t value_type_;
  Descriptor* submsg_;
  EnumDescriptor* subenum_;
};

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_MAP_H__
