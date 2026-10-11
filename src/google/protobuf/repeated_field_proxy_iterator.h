#ifndef GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_ITERATOR_H__
#define GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_ITERATOR_H__

#include <cstddef>
#include <iterator>
#include <type_traits>

#include "google/protobuf/repeated_field_proxy_traits.h"

namespace google {
namespace protobuf {

template <typename T>
class RepeatedFieldProxy;

namespace internal {

template <typename ElementType, bool kOrProxy>
class MutableRepeatedFieldProxyImpl;

template <typename ElementType, bool kOrProxy>
class RepeatedFieldProxyInternalPrivateAccessHelper;

template <typename ElementType>
class RepeatedFieldProxyIteratorInternalPrivateAccessHelper;

// kOrProxy is a phony parameter just to force `RepeatedFieldProxy` and
// `RepeatedFieldOrProxy` to have different iterator types. When the backing
// representation of `RepeatedFieldProxy` changes, we will need these iterators
// to have different types. By making the type different now, it makes writing
// code that mixes iterators from `RepeatedFieldProxy` and
// `RepeatedFieldOrProxy` impossible.
template <typename ElementType, bool kReverse, bool kOrProxy>
class RepeatedFieldProxyIteratorImpl {
  // An alias for self.
  using iterator = RepeatedFieldProxyIteratorImpl;

  using Traits = RepeatedFieldTraits<std::remove_const_t<ElementType>>;
  using InternalForwardIterator =
      std::conditional_t<std::is_const_v<ElementType>,
                         typename Traits::type::const_iterator,
                         typename Traits::type::iterator>;
  using InternalIterator =
      std::conditional_t<kReverse,
                         std::reverse_iterator<InternalForwardIterator>,
                         InternalForwardIterator>;

 public:
  using value_type = std::remove_const_t<ElementType>;
  using difference_type = std::ptrdiff_t;
  using pointer = ElementType*;
  using reference = std::conditional_t<std::is_const_v<ElementType>,
                                       typename Traits::const_reference,
                                       typename Traits::reference>;

 private:
  static constexpr bool kReturnByValue = !std::is_reference_v<reference>;

  struct ArrowProxy {
    value_type view;
    const value_type* operator->() const { return &view; }
  };

 public:
  // When returning elements by value, this iterator does not satisfy the
  // pre-C++20 requirement that "If i and j are both dereferenceable, then i ==
  // j if and only if *i and *j are bound to the same object" from
  // `LegacyForwardIterator`. This is broken because `operator*` returns a
  // temporary.
  using iterator_category =
      std::conditional_t<kReturnByValue, std::input_iterator_tag,
                         std::random_access_iterator_tag>;
  // This object equivalence restriction was relaxed in C++20, allowing us to
  // use `std::random_access_iterator_tag` for `iterator_concept` for
  // value-returning and reference-returning iterators.
  using iterator_concept = std::random_access_iterator_tag;

  explicit RepeatedFieldProxyIteratorImpl(InternalIterator it) : it_(it) {}

  template <typename E = ElementType,
            typename = std::enable_if_t<std::is_const_v<E>>>
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldProxyIteratorImpl(
      const RepeatedFieldProxyIteratorImpl<std::remove_const_t<E>, kReverse,
                                           kOrProxy>& other)
      : it_(other.it_) {}

  // Allow explicit conversion from forward to reverse iterators.
  template <bool R = kReverse, typename = std::enable_if_t<R>>
  explicit RepeatedFieldProxyIteratorImpl(
      RepeatedFieldProxyIteratorImpl<ElementType, !R, kOrProxy> other)
      : it_(std::make_reverse_iterator(other.it_)) {}

  [[nodiscard]] reference operator*() const {
    return static_cast<reference>(*it_);
  }
  [[nodiscard]] auto operator->() const {
    if constexpr (kReturnByValue) {
      // Since operator-> needs to return a pointer-like object, we return a
      // struct that holds a copy of the element value, with an overloaded
      // operator-> returning a pointer to this object.
      return ArrowProxy{static_cast<value_type>(*it_)};
    } else {
      return &(operator*());
    }
  }

  iterator& operator++() {
    ++it_;
    return *this;
  }
  iterator operator++(int) { return iterator(it_++); }
  iterator& operator--() {
    --it_;
    return *this;
  }
  iterator operator--(int) { return iterator(it_--); }

  friend bool operator==(const iterator& x, const iterator& y) {
    return x.it_ == y.it_;
  }
  friend bool operator!=(const iterator& x, const iterator& y) {
    return x.it_ != y.it_;
  }

  friend bool operator<(const iterator& x, const iterator& y) {
    return x.it_ < y.it_;
  }
  friend bool operator<=(const iterator& x, const iterator& y) {
    return x.it_ <= y.it_;
  }
  friend bool operator>(const iterator& x, const iterator& y) {
    return x.it_ > y.it_;
  }
  friend bool operator>=(const iterator& x, const iterator& y) {
    return x.it_ >= y.it_;
  }

  iterator& operator+=(difference_type d) {
    it_ += d;
    return *this;
  }
  friend iterator operator+(iterator it, const difference_type d) {
    it += d;
    return it;
  }
  friend iterator operator+(const difference_type d, iterator it) {
    it += d;
    return it;
  }
  iterator& operator-=(difference_type d) {
    it_ -= d;
    return *this;
  }
  friend iterator operator-(iterator it, difference_type d) {
    it -= d;
    return it;
  }

  [[nodiscard]] reference operator[](difference_type d) const {
    return static_cast<reference>(it_[d]);
  }

  friend difference_type operator-(iterator it1, iterator it2) {
    return it1.it_ - it2.it_;
  }

 private:
  template <typename, bool, bool>
  friend class RepeatedFieldProxyIteratorImpl;
  friend RepeatedFieldProxyIteratorInternalPrivateAccessHelper<ElementType>;
  friend MutableRepeatedFieldProxyImpl<std::remove_const_t<ElementType>,
                                       kOrProxy>;

  // Allow explicit conversion to the internal iterator.
  explicit operator InternalIterator() const { return it_; }

  InternalIterator it_;
};

template <typename ElementType>
class RepeatedFieldProxyIteratorInternalPrivateAccessHelper {
  using Traits = RepeatedFieldTraits<std::remove_const_t<ElementType>>;

 public:
  template <bool kReverse, bool kOrProxy>
  static auto iterator(
      const RepeatedFieldProxyIteratorImpl<ElementType, kReverse, kOrProxy>&
          it) {
    return it.it_;
  }
};

template <typename ElementType, bool kOrProxy>
class RepeatedFieldProxyBackInsertIteratorImpl {
  using Proxy = MutableRepeatedFieldProxyImpl<ElementType, kOrProxy>;

 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = std::remove_const_t<ElementType>;
  using pointer = void;
  using reference = void;
  using difference_type = std::ptrdiff_t;

  explicit RepeatedFieldProxyBackInsertIteratorImpl(Proxy proxy)
      : proxy_(proxy) {}

  RepeatedFieldProxyBackInsertIteratorImpl(
      const RepeatedFieldProxyBackInsertIteratorImpl&) = default;
  RepeatedFieldProxyBackInsertIteratorImpl(
      RepeatedFieldProxyBackInsertIteratorImpl&&) = default;

  // Since operator= is deleted for mutable proxies, we have to manually copy
  // the iterator.
  RepeatedFieldProxyBackInsertIteratorImpl& operator=(
      const RepeatedFieldProxyBackInsertIteratorImpl& other) {
    RepeatedFieldProxyInternalPrivateAccessHelper<
        ElementType, kOrProxy>::rebind(proxy_, other.proxy_);
    return *this;
  }
  RepeatedFieldProxyBackInsertIteratorImpl& operator=(
      RepeatedFieldProxyBackInsertIteratorImpl&& other) {
    RepeatedFieldProxyInternalPrivateAccessHelper<
        ElementType, kOrProxy>::rebind(proxy_, other.proxy_);
    return *this;
  }

  template <typename T = ElementType,
            typename = std::enable_if_t<RepeatedElementTypeIsPrimitive<T>>>
  RepeatedFieldProxyBackInsertIteratorImpl& operator=(ElementType value) {
    proxy_.push_back(value);
    return *this;
  }

  template <typename T = ElementType,
            typename = std::enable_if_t<RepeatedElementTypeIsMessage<T>>>
  RepeatedFieldProxyBackInsertIteratorImpl& operator=(
      const ElementType& value) {
    proxy_.push_back(value);
    return *this;
  }
  template <typename T = ElementType,
            typename = std::enable_if_t<RepeatedElementTypeIsMessage<T>>>
  RepeatedFieldProxyBackInsertIteratorImpl& operator=(ElementType&& value) {
    proxy_.push_back(std::move(value));
    return *this;
  }

  template <typename U, typename T = ElementType,
            typename = std::enable_if_t<RepeatedElementTypeIsString<T>>>
  RepeatedFieldProxyBackInsertIteratorImpl& operator=(U&& value) {
    proxy_.push_back(std::forward<U>(value));
    return *this;
  }

  RepeatedFieldProxyBackInsertIteratorImpl& operator*() { return *this; }
  RepeatedFieldProxyBackInsertIteratorImpl& operator++() { return *this; }
  RepeatedFieldProxyBackInsertIteratorImpl& operator++(int) { return *this; }

 private:
  Proxy proxy_;
};

}  // namespace internal

// Like C++20's std::sort, for RepeatedFieldProxy.
template <int&..., typename T, bool kReverse, bool kOrProxy, typename Compare>
void sort(internal::RepeatedFieldProxyIteratorImpl<T, kReverse, kOrProxy> begin,
          internal::RepeatedFieldProxyIteratorImpl<T, kReverse, kOrProxy> end,
          Compare cmp) {
  auto begin_it =
      internal::RepeatedFieldProxyIteratorInternalPrivateAccessHelper<
          T>::iterator(begin);
  auto end_it = internal::RepeatedFieldProxyIteratorInternalPrivateAccessHelper<
      T>::iterator(end);
  google::protobuf::sort(begin_it, end_it, cmp);
}
// Like C++20's std::sort, for RepeatedFieldProxy, with default comparison.
template <int&..., typename T, bool kReverse, bool kOrProxy>
void sort(internal::RepeatedFieldProxyIteratorImpl<T, kReverse, kOrProxy> begin,
          internal::RepeatedFieldProxyIteratorImpl<T, kReverse, kOrProxy> end) {
  auto begin_it =
      internal::RepeatedFieldProxyIteratorInternalPrivateAccessHelper<
          T>::iterator(begin);
  auto end_it = internal::RepeatedFieldProxyIteratorInternalPrivateAccessHelper<
      T>::iterator(end);
  google::protobuf::sort(begin_it, end_it);
}
// Like C++20's std::stable_sort, for RepeatedFieldProxy.
template <int&..., typename T, bool kReverse, bool kOrProxy, typename Compare>
void stable_sort(
    internal::RepeatedFieldProxyIteratorImpl<T, kReverse, kOrProxy> begin,
    internal::RepeatedFieldProxyIteratorImpl<T, kReverse, kOrProxy> end,
    Compare cmp) {
  auto begin_it =
      internal::RepeatedFieldProxyIteratorInternalPrivateAccessHelper<
          T>::iterator(begin);
  auto end_it = internal::RepeatedFieldProxyIteratorInternalPrivateAccessHelper<
      T>::iterator(end);
  google::protobuf::stable_sort(begin_it, end_it, cmp);
}
// Like C++20's std::stable_sort, for RepeatedFieldProxy, with default
// comparison.
template <int&..., typename T, bool kReverse, bool kOrProxy>
void stable_sort(
    internal::RepeatedFieldProxyIteratorImpl<T, kReverse, kOrProxy> begin,
    internal::RepeatedFieldProxyIteratorImpl<T, kReverse, kOrProxy> end) {
  auto begin_it =
      internal::RepeatedFieldProxyIteratorInternalPrivateAccessHelper<
          T>::iterator(begin);
  auto end_it = internal::RepeatedFieldProxyIteratorInternalPrivateAccessHelper<
      T>::iterator(end);
  google::protobuf::stable_sort(begin_it, end_it);
}

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_ITERATOR_H__
