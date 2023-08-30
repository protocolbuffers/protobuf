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

#ifndef GOOGLE_PROTOBUF_MAP_FIELD_LITE_H__
#define GOOGLE_PROTOBUF_MAP_FIELD_LITE_H__

#include <type_traits>

#include "absl/log/absl_check.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/map.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/wire_format_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {
namespace internal {

// This class provides access to map field using generated api. It is used for
// internal generated message implementation only. Users should never use this
// directly.
template <typename Key, typename T>
class MapFieldLite {
 public:
  typedef Map<Key, T> MapType;

  constexpr MapFieldLite() : map_() {}
  explicit MapFieldLite(Arena* arena) : map_(arena) {}
  MapFieldLite(ArenaInitialized, Arena* arena) : MapFieldLite(arena) {}

  MapFieldLite(InternalVisibility, Arena* arena) : map_(arena) {}
  MapFieldLite(InternalVisibility, Arena* arena, const MapFieldLite& from)
      : map_(arena) {
    MergeFrom(from);
  }

#ifdef NDEBUG
  ~MapFieldLite() { map_.~Map(); }
#else
  ~MapFieldLite() {
    ABSL_DCHECK_EQ(map_.arena(), nullptr);
    // We want to destruct the map in such a way that we can verify
    // that we've done that, but also be sure that we've deallocated
    // everything (as opposed to leaving an allocation behind with no
    // data in it, as would happen if a vector was resize'd to zero.
    // Map::Swap with an empty map accomplishes that.
    decltype(map_) swapped_map(map_.arena());
    map_.InternalSwap(&swapped_map);
  }
#endif
  // Accessors
  const Map<Key, T>& GetMap() const { return map_; }
  Map<Key, T>* MutableMap() { return &map_; }

  // Convenient methods for generated message implementation.
  int size() const { return static_cast<int>(map_.size()); }
  void Clear() { return map_.clear(); }
  void MergeFrom(const MapFieldLite& other) {
    internal::MapMergeFrom(map_, other.map_);
  }
  void Swap(MapFieldLite* other) { map_.swap(other->map_); }
  void InternalSwap(MapFieldLite* other) { map_.InternalSwap(&other->map_); }

 private:
  typedef void DestructorSkippable_;

  // map_ is inside an anonymous union so we can explicitly control its
  // destruction
  union {
    Map<Key, T> map_;
  };

  friend class google::protobuf::Arena;
};

// True if IsInitialized() is true for value field in all elements of t. T is
// expected to be message.  It's useful to have this helper here to keep the
// protobuf compiler from ever having to emit loops in IsInitialized() methods.
// We want the C++ compiler to inline this or not as it sees fit.
template <typename Key, typename T>
bool AllAreInitialized(const MapFieldLite<Key, T>& field) {
  const auto& t = field.GetMap();
  for (typename Map<Key, T>::const_iterator it = t.begin(); it != t.end();
       ++it) {
    if (!it->second.IsInitialized()) return false;
  }
  return true;
}

template <typename MEntry>
struct MapEntryToMapField : MapEntryToMapField<typename MEntry::SuperType> {};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_FIELD_LITE_H__
