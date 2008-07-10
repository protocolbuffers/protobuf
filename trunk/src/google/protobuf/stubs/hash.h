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

// Author: kenton@google.com (Kenton Varda)
//
// Deals with the fact that hash_map is not defined everywhere.

#ifndef GOOGLE_PROTOBUF_STUBS_HASH_H__
#define GOOGLE_PROTOBUF_STUBS_HASH_H__

#include <string.h>
#include <google/protobuf/stubs/common.h>
#include "config.h"

#if defined(HAVE_HASH_MAP) && defined(HAVE_HASH_SET)
#include HASH_MAP_H
#include HASH_SET_H
#else
// TODO(kenton):  Deal with non-existence of hash_map somehow.  Maybe emulate
//   it with map?
#error "Your STL implementation lacks hash_map and/or hash_set."
#endif

namespace google {
namespace protobuf {

#ifdef _MSC_VER

template <typename Key>
struct hash : public HASH_NAMESPACE::hash_compare<Key> {
};

// MSVC's hash_compare<const char*> hashes based on the string contents but
// compares based on the string pointer.  WTF?
class CstringLess {
 public:
  inline bool operator()(const char* a, const char* b) const {
    return strcmp(a, b) < 0;
  }
};

template <>
struct hash<const char*>
  : public HASH_NAMESPACE::hash_compare<const char*, CstringLess> {
};

template <typename Key, typename Data,
          typename HashFcn = hash<Key>,
          typename EqualKey = int >
class hash_map : public HASH_NAMESPACE::hash_map<
    Key, Data, HashFcn> {
};

template <typename Key,
          typename HashFcn = hash<Key>,
          typename EqualKey = int >
class hash_set : public HASH_NAMESPACE::hash_set<
    Key, HashFcn> {
};

#else

template <typename Key>
struct hash : public HASH_NAMESPACE::hash<Key> {
};

template <typename Key>
struct hash<const Key*> {
  inline size_t operator()(const Key* key) const {
    return reinterpret_cast<size_t>(key);
  }
};

template <>
struct hash<const char*> : public HASH_NAMESPACE::hash<const char*> {
};

template <typename Key, typename Data,
          typename HashFcn = hash<Key>,
          typename EqualKey = std::equal_to<Key> >
class hash_map : public HASH_NAMESPACE::hash_map<
    Key, Data, HashFcn, EqualKey> {
};

template <typename Key,
          typename HashFcn = hash<Key>,
          typename EqualKey = std::equal_to<Key> >
class hash_set : public HASH_NAMESPACE::hash_set<
    Key, HashFcn, EqualKey> {
};

#endif

template <>
struct hash<string> {
  inline size_t operator()(const string& key) const {
    return hash<const char*>()(key.c_str());
  }

  static const size_t bucket_size = 4;
  static const size_t min_buckets = 8;
  inline size_t operator()(const string& a, const string& b) const {
    return a < b;
  }
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_STUBS_HASH_H__
