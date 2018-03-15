// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#include "protobuf_hhvm.h"

// -----------------------------------------------------------------------------
// MapField
// -----------------------------------------------------------------------------

void HHVM_METHOD(MapField, __construct,
    int64_t key_type, int64_t value_type, const Variant& classname);
bool HHVM_METHOD(MapField, offsetExists, const Variant& key);
Variant HHVM_METHOD(MapField, offsetGet, const Variant& key);
void HHVM_METHOD(MapField, offsetSet, const Variant& key,
                 const Variant& newvalue);
void HHVM_METHOD(MapField, offsetUnset, const Variant& key);
int64_t HHVM_METHOD(MapField, count);
Object HHVM_METHOD(MapField, getIterator);

const StaticString s_MapField("Google\\Protobuf\\Internal\\MapField");
const StaticString s_MapFieldIter(
    "Google\\Protobuf\\Internal\\MapFieldIter");

void MapField_init() {
  // Register methods
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapField,
                __construct, HHVM_MN(MapField, __construct));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapField,
                offsetExists, HHVM_MN(MapField, offsetExists));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapField,
                offsetGet, HHVM_MN(MapField, offsetGet));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapField,
                offsetSet, HHVM_MN(MapField, offsetSet));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapField,
                offsetUnset, HHVM_MN(MapField, offsetUnset));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapField,
                count, HHVM_MN(MapField, count));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapField,
                getIterator, HHVM_MN(MapField, getIterator));

  // Register class
  Native::registerNativeDataInfo<MapField>(s_MapField.get()); 
}

void HHVM_METHOD(MapField, __construct, int64_t key_type, int64_t value_type,
                 const Variant& classname) {
  MapField *intern = Native::data<MapField>(this_);
  Class *subklass = classname.isNull() ? 
      NULL : Unit::loadClass(classname.toCStrRef().get());
  MapField___construct(
      intern, static_cast<upb_descriptortype_t>(key_type),
      static_cast<upb_descriptortype_t>(value_type),
      subklass);
}

bool HHVM_METHOD(MapField, offsetExists, const Variant& key) {
  MapField *intern = Native::data<MapField>(this_);
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map));
  upb_msgval v;
  return upb_map_get(intern->map, k, &v);
}

Variant HHVM_METHOD(MapField, offsetGet, const Variant& key) {
  MapField *intern = Native::data<MapField>(this_);
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map));
  upb_msgval v;
  if(!upb_map_get(intern->map, k, &v)) {
    return uninit_null();
  } else {
    return tophpval(v, upb_map_valuetype(intern->map),
                    static_cast<Class*>(intern->klass));
  }
}

void HHVM_METHOD(MapField, offsetSet, const Variant& key,
                 const Variant& newvalue) {
  MapField *intern = Native::data<MapField>(this_);
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map));
  upb_msgval v = tomsgval(newvalue, upb_map_valuetype(intern->map));
  upb_map_set(intern->map, k, v, NULL);
}

void HHVM_METHOD(MapField, offsetUnset, const Variant& key) {
  MapField *intern = Native::data<MapField>(this_);
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map));
  upb_map_del(intern->map, k);
}

int64_t HHVM_METHOD(MapField, count) {
  MapField *intern = Native::data<MapField>(this_);
  return upb_map_size(intern->map);
}

Object HHVM_METHOD(MapField, getIterator) {
  MapField *intern = Native::data<MapField>(this_);
  Object iterobj = Object(Unit::loadClass(s_MapFieldIter.get()));
  MapFieldIter *iter = Native::data<MapFieldIter>(iterobj);
  iter->map_field = intern;
  iter->iter = upb_mapiter_new(intern->map, upb_map_getalloc(intern->map));
  return iterobj;
}

// -----------------------------------------------------------------------------
// MapFieldIter
// -----------------------------------------------------------------------------

void HHVM_METHOD(MapFieldIter, rewind);
void HHVM_METHOD(MapFieldIter, next);
bool HHVM_METHOD(MapFieldIter, valid);
Variant HHVM_METHOD(MapFieldIter, current);
Variant HHVM_METHOD(MapFieldIter, key);

void MapFieldIter_init() {
  // Register methods
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapFieldIter,
                rewind, HHVM_MN(MapFieldIter, rewind));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapFieldIter,
                current, HHVM_MN(MapFieldIter, current));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapFieldIter,
                key, HHVM_MN(MapFieldIter, key));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapFieldIter,
                next, HHVM_MN(MapFieldIter, next));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\MapFieldIter,
                valid, HHVM_MN(MapFieldIter, valid));

  // Register class
  Native::registerNativeDataInfo<MapFieldIter>(s_MapFieldIter.get()); 
}

void  HHVM_METHOD(MapFieldIter, rewind) {
  MapFieldIter *intern = Native::data<MapFieldIter>(this_);
  upb_alloc *a = upb_map_getalloc(intern->map_field->map);
  upb_mapiter_free(intern->iter, a);
  intern->iter = upb_mapiter_new(intern->map_field->map, a);
}

Variant HHVM_METHOD(MapFieldIter, current) {
  MapFieldIter *intern = Native::data<MapFieldIter>(this_);
  upb_msgval value = upb_mapiter_value(intern->iter);
  return tophpval(value, upb_map_valuetype(intern->map_field->map),
                  static_cast<Class*>(intern->map_field->klass));
}

Variant HHVM_METHOD(MapFieldIter, key) {
  MapFieldIter *intern = Native::data<MapFieldIter>(this_);
  upb_msgval key = upb_mapiter_key(intern->iter);
  return tophpval(key, upb_map_keytype(intern->map_field->map),
                  static_cast<Class*>(intern->map_field->klass));
}

void  HHVM_METHOD(MapFieldIter, next) {
  MapFieldIter *intern = Native::data<MapFieldIter>(this_);
  upb_mapiter_next(intern->iter);
}

bool  HHVM_METHOD(MapFieldIter, valid) {
  MapFieldIter *intern = Native::data<MapFieldIter>(this_);
  return !upb_mapiter_done(intern->iter);
}
