// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// from google3/util/gtl/map-util.h
// Author: Anton Carver

#ifndef GOOGLE_PROTOBUF_STUBS_MAP_UTIL_H__
#define GOOGLE_PROTOBUF_STUBS_MAP_UTIL_H__

#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

// Perform a lookup in a map or hash_map.
// If the key is present a const pointer to the associated value is returned,
// otherwise a NULL pointer is returned.
template <class Collection>
const typename Collection::value_type::second_type*
FindOrNull(const Collection& collection,
           const typename Collection::value_type::first_type& key) {
  typename Collection::const_iterator it = collection.find(key);
  if (it == collection.end()) {
    return 0;
  }
  return &it->second;
}

// Perform a lookup in a map or hash_map whose values are pointers.
// If the key is present a const pointer to the associated value is returned,
// otherwise a NULL pointer is returned.
// This function does not distinguish between a missing key and a key mapped
// to a NULL value.
template <class Collection>
const typename Collection::value_type::second_type
FindPtrOrNull(const Collection& collection,
              const typename Collection::value_type::first_type& key) {
  typename Collection::const_iterator it = collection.find(key);
  if (it == collection.end()) {
    return 0;
  }
  return it->second;
}

// Change the value associated with a particular key in a map or hash_map.
// If the key is not present in the map the key and value are inserted,
// otherwise the value is updated to be a copy of the value provided.
// True indicates that an insert took place, false indicates an update.
template <class Collection, class Key, class Value>
bool InsertOrUpdate(Collection * const collection,
                   const Key& key, const Value& value) {
  pair<typename Collection::iterator, bool> ret =
    collection->insert(typename Collection::value_type(key, value));
  if (!ret.second) {
    // update
    ret.first->second = value;
    return false;
  }
  return true;
}

// Insert a new key and value into a map or hash_map.
// If the key is not present in the map the key and value are
// inserted, otherwise nothing happens. True indicates that an insert
// took place, false indicates the key was already present.
template <class Collection, class Key, class Value>
bool InsertIfNotPresent(Collection * const collection,
                        const Key& key, const Value& value) {
  pair<typename Collection::iterator, bool> ret =
    collection->insert(typename Collection::value_type(key, value));
  return ret.second;
}

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_STUBS_MAP_UTIL_H__
