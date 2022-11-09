// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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

#ifndef GOOGLE_PROTOBUF_LOOKUP_CHUNK_H__
#define GOOGLE_PROTOBUF_LOOKUP_CHUNK_H__

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iterator>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "absl/container/internal/layout.h"
#include "absl/types/span.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

template <typename Key, typename Value>
class LookupChunk {
 public:
  constexpr static size_t AllocSize(size_t n) {
    return sizeof(Header) + Layout(n).AllocSize();
  }

  constexpr LookupChunk() = default;
  explicit LookupChunk(uint32_t capacity, LookupChunk* next = nullptr)
      : header_(capacity, 0, next) {}

  static constexpr LookupChunk* sentinel();

  LookupChunk(uint32_t capacity, Key* key, Value* value,
              LookupChunk* next = nullptr)
      : header_(capacity, 1, next) {
    new (mutable_keys()) std::atomic<Key*>{key};
    new (mutable_values()) std::atomic<Value*>{value};
  }

  LookupChunk* next() { return header_.next.load(std::memory_order_relaxed); }
  const LookupChunk* next() const {
    return header_.next.load(std::memory_order_relaxed);
  }
  void set_next(LookupChunk* next) {
    header_.next.store(next, std::memory_order_relaxed);
  }

  PROTOBUF_NDEBUG_INLINE bool Add(Key* key, Value* value) {
    uint32_t size = header_.size.load(std::memory_order_acquire);
    if (size >= capacity()) return false;
    new (mutable_values() + size) std::atomic<Value*>{value};
    new (mutable_keys() + size) std::atomic<Key*>{key};
    set_size(size + 1);
    return true;
  }

  Value* Find(Key* key) const {
    if (key) {
      auto keys = this->keys();
      for (size_t idx = 0; idx < keys.size(); ++idx) {
        if (key == keys[idx].load(std::memory_order_acquire)) {
          return values()[idx].load(std::memory_order_acquire);
        }
      }
    }
    return nullptr;
  }

  uint32_t capacity() const { return header_.capacity; }
  uint32_t size() const { return header_.size.load(std::memory_order_acquire); }

  absl::Span<const std::atomic<Key*>> keys() const {
    return Layout(capacity()).template Slice<kKeys>(ptr()).first(size());
  }

  absl::Span<const std::atomic<Value*>> values() const {
    return Layout(capacity()).template Slice<kValues>(ptr()).first(size());
  }

  class reverse_iterator
      : std::iterator<std::random_access_iterator_tag, Value> {
   public:
    explicit reverse_iterator(std::atomic<Value*>* pos) : pos_(pos) {}

    reverse_iterator& operator++() {
      --pos_;
      return *this;
    }
    reverse_iterator operator++(int) {
      reverse_iterator ret = *this;
      --(*this);
      return ret;
    }

    bool operator==(reverse_iterator rhs) const { return pos_ == rhs.pos_; }
    bool operator!=(reverse_iterator rhs) const { return !operator==(rhs); }

    Value& operator*() { return *pos_[-1].load(std::memory_order_relaxed); }
    Value* operator->() { return pos_[-1].load(std::memory_order_relaxed); }

    std::atomic<Value*>* pos_;
  };

  reverse_iterator begin() {
    return reverse_iterator{mutable_values() + size()};
  }
  reverse_iterator end() { return reverse_iterator{mutable_values()}; }

 private:
  void set_size(size_t size) {
    header_.size.store(size, std::memory_order_release);
  }

  std::atomic<Key*>* mutable_keys() {
    return Layout(capacity()).template Pointer<kKeys>(ptr());
  }
  std::atomic<Value*>* mutable_values() {
    return Layout(capacity()).template Pointer<kValues>(ptr());
  }

 private:
  struct Header {
    constexpr Header() = default;
    Header(uint32_t capacity, size_t size, LookupChunk* next)
        : capacity(capacity), size(size), next(next) {}
    const uint32_t capacity{0};
    std::atomic<uint32_t> size{0};
    std::atomic<LookupChunk*> next{nullptr};
  };
  struct Sentinel;

  using layout_type =
      absl::container_internal::Layout<std::atomic<Key*>, std::atomic<Value*>>;

  static constexpr int kKeys = 0;
  static constexpr int kValues = 1;

  Header header_;

  const char* ptr() const {
    return reinterpret_cast<const char*>(&header_ + 1);
  }
  char* ptr() { return reinterpret_cast<char*>(&header_ + 1); }

  constexpr static layout_type Layout(size_t n) {
    return layout_type(
        /*keys*/ n,
        /*values*/ n);
  }
};

template <typename Key, typename Value>
struct LookupChunk<Key, Value>::Sentinel {
  static constexpr LookupChunk<Key, Value> sentinel;
};

template <typename Key, typename Value>
constexpr LookupChunk<Key, Value>* LookupChunk<Key, Value>::sentinel() {
  return const_cast<LookupChunk<Key, Value>*>(&Sentinel::sentinel);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_LOOKUP_CHUNK_H__
