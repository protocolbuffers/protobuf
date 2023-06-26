/*
 * Copyright (c) 2009-2023, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef UPB_PROTOS_REPEATED_FIELD_ITERATOR_H_
#define UPB_PROTOS_REPEATED_FIELD_ITERATOR_H_

#include <cstddef>
#include <iterator>
#include <type_traits>

#include "absl/strings/string_view.h"
#include "protos/protos.h"
#include "upb/base/string_view.h"
#include "upb/collections/array.h"
#include "upb/mem/arena.h"
#include "upb/message/copy.h"

// Must be last:
#include "upb/port/def.inc"

namespace protos {

namespace internal {

// TODO(b/279086429): Implement std iterator for strings and messages
template <typename T>
class RepeatedFieldScalarProxy;

template <typename T>
class RepeatedScalarIterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = typename std::remove_const<T>::type;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;

  constexpr RepeatedScalarIterator() noexcept : it_(nullptr) {}
  RepeatedScalarIterator(const RepeatedScalarIterator& other) = default;
  RepeatedScalarIterator& operator=(const RepeatedScalarIterator& other) =
      default;

  // deref TODO(b/286450722) Change to use a proxy.
  constexpr reference operator*() const noexcept { return *it_; }
  constexpr pointer operator->() const noexcept { return it_; }

 private:
  // Hide the internal type.
  using iterator = RepeatedScalarIterator;

 public:
  // {inc,dec}rementable
  constexpr iterator& operator++() noexcept {
    ++it_;
    return *this;
  }
  constexpr iterator operator++(int) noexcept { return iterator(it_++); }
  constexpr iterator& operator--() noexcept {
    --it_;
    return *this;
  }
  constexpr iterator operator--(int) noexcept { return iterator(it_--); }

  // equality_comparable
  friend constexpr bool operator==(const iterator x,
                                   const iterator y) noexcept {
    return x.it_ == y.it_;
  }
  friend constexpr bool operator!=(const iterator x,
                                   const iterator y) noexcept {
    return x.it_ != y.it_;
  }

  // less_than_comparable
  friend constexpr bool operator<(const iterator x, const iterator y) noexcept {
    return x.it_ < y.it_;
  }
  friend constexpr bool operator<=(const iterator x,
                                   const iterator y) noexcept {
    return x.it_ <= y.it_;
  }
  friend constexpr bool operator>(const iterator x, const iterator y) noexcept {
    return x.it_ > y.it_;
  }
  friend constexpr bool operator>=(const iterator x,
                                   const iterator y) noexcept {
    return x.it_ >= y.it_;
  }

  constexpr iterator& operator+=(difference_type d) noexcept {
    it_ += d;
    return *this;
  }
  constexpr iterator operator+(difference_type d) const noexcept {
    return iterator(it_ + d);
  }
  friend constexpr iterator operator+(const difference_type d,
                                      iterator it) noexcept {
    return it + d;
  }

  constexpr iterator& operator-=(difference_type d) noexcept {
    it_ -= d;
    return *this;
  }
  constexpr iterator operator-(difference_type d) const noexcept {
    return iterator(it_ - d);
  }

  // indexable
  constexpr reference operator[](difference_type d) const noexcept {
    return it_[d];
  }

  // random access iterator
  friend constexpr difference_type operator-(iterator x, iterator y) noexcept {
    return x.it_ - y.it_;
  }

 private:
  friend class RepeatedFieldScalarProxy<T>;

  // Create from internal::RepeatedFieldScalarProxy.
  explicit RepeatedScalarIterator(T* it) noexcept : it_(it) {}

  // The internal iterator.
  T* it_;
};

}  // namespace internal

}  // namespace protos

#endif  // UPB_PROTOS_REPEATED_FIELD_ITERATOR_H_
