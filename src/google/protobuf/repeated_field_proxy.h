#ifndef GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
#define GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__

#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_field_proxy_iterator.h"
#include "google/protobuf/repeated_field_proxy_traits.h"
#include "google/protobuf/repeated_ptr_field.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

template <typename ElementType>
class RepeatedFieldProxy;
template <typename ElementType>
class RepeatedFieldOrProxy;

namespace internal {

template <typename ElementType, bool kOrProxy>
class MutableRepeatedFieldProxyImpl;
template <typename ElementType, bool kOrProxy>
class ConstRepeatedFieldProxyImpl;
template <typename ElementType, bool kOrProxy>
class RepeatedFieldProxyInternalPrivateAccessHelper;

namespace string_util {

template <typename StringType, typename T>
inline void CopyToString(StringType& element, T&& value) {
  // We want to explicitly enumerate all types we accept for constructing
  // strings, otherwise this would be a subtle place we'd be leaking the
  // `std::string` backing of `string_view` repeated fields. I.e. whatever new
  // backing we use for `string_view` fields would have to support all
  // `operator=` overloads that `std::string` has.
  //
  // With an explicit list, we no longer have this dependency.
  if constexpr (std::is_convertible_v<T, absl::string_view>) {
    element = absl::implicit_cast<absl::string_view>(value);
  } else if constexpr (std::is_convertible_v<T, const std::string&>) {
    element = absl::implicit_cast<const std::string&>(value);
  } else if constexpr (std::is_convertible_v<T, const char*>) {
    element = absl::implicit_cast<const char*>(value);
  } else {
    element = {value.data(), value.size()};
  }
}

template <typename T>
inline void SetElement(std::string& element, T&& value) {
  if constexpr (std::is_same_v<T&&, std::string&&>) {
    element = std::forward<T>(value);
  } else if constexpr (std::is_convertible_v<T, const absl::Cord&>) {
    const absl::Cord& cord = std::forward<T>(value);
    absl::CopyCordToString(cord, &element);
  } else {
    CopyToString(element, std::forward<T>(value));
  }
}

template <typename T>
inline void SetElement(absl::Cord& element, T&& value) {
  if constexpr (std::is_same_v<T&&, absl::Cord&&>) {
    element = std::forward<T>(value);
  } else if constexpr (std::is_convertible_v<T, const absl::Cord&>) {
    element = absl::implicit_cast<const absl::Cord&>(std::forward<T>(value));
  } else {
    CopyToString(element, std::forward<T>(value));
  }
}

}  // namespace string_util

// The base class for both mutable and const repeated field proxies. Implements
// all of the common methods and dependent types for both classes.
//
// Class structure:
//
// internal {- - - - - - - - - - - - - - - - - - - - - - -
//             RepeatedFieldProxyBase
//              |                   |
//   MutableRepeatedFieldProxyImpl  |
//              |                   |
//              |         ConstRepeatedFieldProxyImpl
// } - - - - - -|- - - - - - - - - - - - | - - - - - - - -
//   RepeatedFieldProxy<E>,              |
//   RepeatedFieldOrProxy<E>             |
//                          RepeatedFieldProxy<const E>,
//                          RepeatedFieldOrProxy<const E>
//
// RepeatedFieldProxyBase has the common implementation and interface between
// both mutable and const proxies, and `Mutable*Impl` and `Const*Impl` have the
// const/mutable-specific implementations + interfaces.
//
// Both `RepeatedFieldProxy` and `RepeatedFieldOrProxy` comprise the public
// API and are thin wrappers around `Mutable*Impl` and `Const*Impl`,
// implementing only the constructors and APIs particular to them.
template <typename ElementType, bool kOrProxy>
class RepeatedFieldProxyBase {
 protected:
  // If true, this is a view into a repeated field, meaning neither the elements
  // nor the container can be modified. If false, both the elements and the
  // container can be modified.
  static constexpr bool kIsConst = std::is_const_v<ElementType>;
  using Traits = RepeatedFieldTraits<std::remove_const_t<ElementType>>;
  using RepeatedFieldType = typename Traits::type;
  using ConstQualifiedRepeatedFieldType =
      std::conditional_t<kIsConst, const RepeatedFieldType, RepeatedFieldType>;

 public:
  using value_type = std::remove_const_t<ElementType>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using const_reference = typename Traits::const_reference;

  using const_iterator =
      internal::RepeatedFieldProxyIteratorImpl<const ElementType,
                                               /*kReverse=*/false, kOrProxy>;
  using iterator =
      internal::RepeatedFieldProxyIteratorImpl<ElementType,
                                               /*kReverse=*/false, kOrProxy>;
  using const_reverse_iterator =
      internal::RepeatedFieldProxyIteratorImpl<const ElementType,
                                               /*kReverse=*/true, kOrProxy>;
  using reverse_iterator =
      internal::RepeatedFieldProxyIteratorImpl<ElementType,
                                               /*kReverse=*/true, kOrProxy>;

  // Allow explicit conversion to the backing repeated field type. This will
  // perform a deep copy of the repeated field backed by this proxy.
  //
  // Note that this exposes `RepeatedFieldType`, but will be kept around for
  // backwards compatibility with code that uses `RepeatedField`s or
  // `RepeatedPtrField`s directly. We may freely change `RepeatedFieldType` so
  // long as we maintain this explicit conversion to the legacy container types.
  explicit operator RepeatedFieldType() const {
    return RepeatedFieldType(field());
  }

  RepeatedFieldProxyBase(const RepeatedFieldProxyBase&) = default;

  // Assignment of repeated field proxies simply rebinds the proxy to a
  // different repeated field. It does not modify the underlying field.
  //
  // Copying should be done through `assign()`.
  RepeatedFieldProxyBase& operator=(const RepeatedFieldProxyBase&) = default;

  ~RepeatedFieldProxyBase() = default;

  // Returns true if the repeated field has no elements (size == 0).
  [[nodiscard]] bool empty() const { return field().empty(); }
  // Returns the number of elements in the repeated field.
  [[nodiscard]] size_type size() const {
    return static_cast<size_type>(field().size());
  }

  // Returns a const reference or view into the element at the given index,
  // performing bounds checking in accordance with `bounds_check_mode_*`.
  [[nodiscard]] const_reference get(size_type index) const {
    return field()[index];
  }

  [[nodiscard]] const_iterator cbegin() const { return begin(); }
  [[nodiscard]] const_iterator cend() const { return end(); }
  [[nodiscard]] iterator begin() const { return iterator(field().begin()); }
  [[nodiscard]] iterator end() const { return iterator(field().end()); }
  [[nodiscard]] reverse_iterator rbegin() const {
    return reverse_iterator(end());
  }
  [[nodiscard]] reverse_iterator rend() const {
    return reverse_iterator(begin());
  }

 protected:
  explicit RepeatedFieldProxyBase(ConstQualifiedRepeatedFieldType& field)
      : field_(&field) {}

  ConstQualifiedRepeatedFieldType& field() const { return *field_; }

 private:
  ConstQualifiedRepeatedFieldType* PROTOBUF_NONNULL field_;
};

// The following classes are used to specialize methods of `RepeatedFieldProxy`
// based on the element type. Most methods do not need specialization, since
// they look similar for all element types, maybe only differing in whether
// `const_reference` resolves to a `const T&` or some value type like
// `absl::string_view`.
//
// For methods that do have a different signature based on the element type, we
// make a `*With<MethodName>` class that defines only that method, specialized
// on the element type using whatever conditions make sense for the method. We
// then inherit from this type in `RepeatedFieldProxy`.

// Defines `set()` for primitive element types, which only take by value.
template <typename ElementType, bool kOrProxy, typename Enable = void>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithSet {
 public:
  // Sets the element at the given index to the given value.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, ElementType value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType,
                                                      kOrProxy>::field(this);
    field[index] = value;
  }
};

// Defines `set()` for message element types, which take by const reference or
// rvalue.
template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithSet<
    ElementType, kOrProxy,
    std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
 public:
  // Sets the element at the given index to the given value by move-assignment.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, ElementType&& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType,
                                                      kOrProxy>::field(this);
    field[index] = std::move(value);
  }

  // Sets the element at the given index to the given value by copy-assignment.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, const ElementType& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType,
                                                      kOrProxy>::field(this);
    field[index] = value;
  }
};

// Defines `set()` for string element types, which dispatch to
// `string_util::SetElement` and accept many string-like types.
template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithSet<
    ElementType, kOrProxy,
    std::enable_if_t<RepeatedElementTypeIsString<ElementType>>> {
 public:
  // Sets the element at the given index to the given value.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  template <typename T>
  void set(size_t index, T&& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType,
                                                      kOrProxy>::field(this);
    string_util::SetElement(field[index], std::forward<T>(value));
  }
};

// Defines `push_back()` for primitive element types, which only take by value.
template <typename ElementType, bool kOrProxy, typename Enable = void>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithPushBack {
 public:
  // Appends the given value to the end of the repeated field.
  void push_back(ElementType value) const {
    RepeatedFieldProxyInternalPrivateAccessHelper<ElementType, kOrProxy>::Add(
        this, value);
  }
};

// Defines `push_back()` for message element types, which take by const
// reference or rvalue.
template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithPushBack<
    ElementType, kOrProxy,
    std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
 public:
  // Appends the given value to the end of the repeated field by move
  // construction/assignment.
  void push_back(ElementType&& value) const {
    RepeatedFieldProxyInternalPrivateAccessHelper<ElementType, kOrProxy>::Add(
        this, std::move(value));
  }

  // Appends the given value to the end of the repeated field by copy
  // construction/assignment.
  void push_back(const ElementType& value) const {
    RepeatedFieldProxyInternalPrivateAccessHelper<ElementType, kOrProxy>::Add(
        this, value);
  }
};

// Defines `push_back()` for string element types, which dispatch to
// `string_util::SetElement` and accept many string-like types.
template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithPushBack<
    ElementType, kOrProxy,
    std::enable_if_t<RepeatedElementTypeIsString<ElementType>>> {
 public:
  // Appends the given value to the end of the repeated field.
  template <typename T>
  void push_back(T&& value) const {
    string_util::SetElement(
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType,
                                                      kOrProxy>::Add(this),
        std::forward<T>(value));
  }
};

// Defines `emplace_back()` for all types except `absl::string_view`. Simply
// takes any arguments that can be passed to the constructor of `ElementType`
// and in-place constructs the element at the end of the repeated field.
template <typename ElementType, bool kOrProxy, typename Enable = void>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithEmplaceBack {
 public:
  // In-place constructs an element at the end of the repeated field, returning
  // a reference to the newly constructed element.
  template <typename... Args>
  auto& emplace_back(Args&&... args) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<
        ElementType, kOrProxy>::Emplace(this, std::forward<Args>(args)...);
  }
};

// Defines `emplace_back()` for `absl::string_view` element types. We explicitly
// list all constructors we want to support for repeated `string_views` to not
// leak the `std::string` backing of repeated `string_views`.
template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithEmplaceBack<
    ElementType, kOrProxy,
    std::enable_if_t<std::is_same_v<ElementType, absl::string_view>>> {
 public:
  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back() const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<
        ElementType, kOrProxy>::Emplace(this);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(absl::string_view value) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<
        ElementType, kOrProxy>::Emplace(this, value);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(std::string&& value) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<
        ElementType, kOrProxy>::Emplace(this, std::move(value));
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(const std::string& value) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<
        ElementType, kOrProxy>::Emplace(this, value);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(const char* PROTOBUF_NONNULL value) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<
        ElementType, kOrProxy>::Emplace(this, value);
  }
};

// Defines `resize(new_size, value)` for all non-string repeated fields.
template <typename ElementType, bool kOrProxy, typename Enable = void>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithResize {
 public:
  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  void resize(size_t new_size, const ElementType& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType,
                                                      kOrProxy>::field(this);
    field.resize(new_size, value);
  }
};

// Defines `resize(new_size, value)` for non-Cord string repeated fields.
template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithResize<
    ElementType, kOrProxy,
    std::enable_if_t<RepeatedElementTypeIsString<ElementType> &&
                     !std::is_same_v<ElementType, absl::Cord>>> {
 public:
  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  void resize(size_t new_size, absl::string_view value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType,
                                                      kOrProxy>::field(this);
    field.resize(new_size, value);
  }
};

// Defines `resize(new_size, value)` for repeated Cords.
template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithResize<
    ElementType, kOrProxy,
    std::enable_if_t<std::is_same_v<ElementType, absl::Cord>>> {
 public:
  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  void resize(size_t new_size, const absl::Cord& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType,
                                                      kOrProxy>::field(this);
    field.resize(new_size, value);
  }
};

template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES MutableRepeatedFieldProxyImpl
    : public internal::RepeatedFieldProxyBase<ElementType, kOrProxy>,
      public internal::RepeatedFieldProxyWithSet<ElementType, kOrProxy>,
      public internal::RepeatedFieldProxyWithPushBack<ElementType, kOrProxy>,
      public internal::RepeatedFieldProxyWithEmplaceBack<ElementType, kOrProxy>,
      public internal::RepeatedFieldProxyWithResize<ElementType, kOrProxy> {
  static_assert(!std::is_const_v<ElementType>);

 protected:
  using Base = internal::RepeatedFieldProxyBase<ElementType, kOrProxy>;
  using typename Base::RepeatedFieldType;

  using Base::field;

 public:
  using typename Base::const_iterator;
  using typename Base::iterator;
  using typename Base::size_type;

  using reference =
      typename internal::RepeatedFieldTraits<ElementType>::reference;

  MutableRepeatedFieldProxyImpl(const MutableRepeatedFieldProxyImpl& other) =
      default;
  // Mutable proxies are not assignable. This is intentional to avoid confusion
  // with the `assign` method, which reassigns the underlying repeated field.
  MutableRepeatedFieldProxyImpl& operator=(
      const MutableRepeatedFieldProxyImpl&) = delete;

  // Returns a type which references the element at the given index. Performs
  // bounds checking in accordance with `bounds_check_mode_*`.
  [[nodiscard]] reference operator[](size_type index) const {
    return field()[index];
  }

  // Removes the last element from the repeated field.
  void pop_back() const { field().RemoveLast(); }

  // Removes all elements from the repeated field. The field will be empty after
  // this call.
  void clear() const { field().Clear(); }

  // Removes the element at `position` from the repeated field. Returns an
  // iterator to the element immediately following the removed element.
  iterator erase(const_iterator position) const {
    // The internal iterator type may not match the proxy iterator type (for
    // example for `absl::string_view` proxies which are backed by
    // `std::string`). To avoid special casing, we will always cast to the
    // internal iterator type before passing down to erase, then cast back to
    // the proxy iterator type upon return. This conversion is redundant for
    // types which have matching exposed and internal element types.
    using const_internal_iterator = typename RepeatedFieldType::const_iterator;
    return iterator(field().erase(const_internal_iterator(position)));
  }

  // Removes the elements in the range `[first, last)` from the repeated field.
  // Returns an iterator to the element immediately following the last removed
  // element.
  iterator erase(const_iterator first, const_iterator last) const {
    using const_internal_iterator = typename RepeatedFieldType::const_iterator;
    return iterator(field().erase(const_internal_iterator(first),
                                  const_internal_iterator(last)));
  }

  // Copy-assigns the elements in the range `[begin, end)` to the repeated
  // field.
  //
  // If `begin` or `end` is an iterator into this repeated field, the behavior
  // is undefined.
  template <
      typename Iter,
      // A seemingly redundant verification that `Iter` is an iterator type.
      // Even though we use `std::iterator_traits` below, we duplicate the
      // condition here in case the implementation changes.
      typename = std::void_t<typename std::iterator_traits<Iter>::value_type>>
  auto assign(Iter begin, Iter end) const
      // Verify that the iterator value type is assignable to `ElementType`.
      // Pass through `push_back`, which is a catch-all for allowed conversions
      // to the element type.
      -> std::void_t<decltype(this->push_back(*begin))> {
    field().Clear();
    // Forward iterators in C++ are required to model `std::incrementable`,
    // which means they are suitable for multi-pass algorithms, and therefore
    // support `std::distance`.
    if constexpr (std::is_base_of<std::forward_iterator_tag,
                                  typename std::iterator_traits<
                                      Iter>::iterator_category>::value) {
      int distance = static_cast<int>(std::distance(begin, end));
      field().ReserveWithArena(arena(), distance);
    }
    for (; begin != end; ++begin) {
      this->push_back(*begin);
    }
  }

  // A hint to the container to expect to grow/shrink to `new_size` elements.
  // This may allow the container to make optimizations to avoid reallocations,
  // but may also be ignored.
  void reserve(size_type new_size) const {
    field().ReserveWithArena(arena(), new_size);
  }

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with default-valued elements.
  void resize(size_t new_size) const { field().resize(new_size); }

  // Because we have an overload of `resize` in this class, we need to
  // explicitly inherit the overload from the base class to avoid hiding it.
  using internal::RepeatedFieldProxyWithResize<ElementType, kOrProxy>::resize;

 protected:
  MutableRepeatedFieldProxyImpl(RepeatedFieldType& field,
                                Arena* PROTOBUF_NULLABLE arena)
      : Base(field), arena_(arena) {
    ABSL_DCHECK_EQ(arena, field.GetArena());
  }

  Arena* PROTOBUF_NULLABLE arena() const { return arena_; }

  // The following methods all forward to the backing repeated fields. This is
  // done here for access to private members of the legacy containers, which
  // only need to friend `MutableRepeatedFieldProxyImpl`.
  auto& Add() const { return *field().AddWithArena(arena()); }
  auto& Add(ElementType&& value) const {
    return *field().AddWithArena(arena(), std::move(value));
  }
  auto& Add(const ElementType& value) const {
    return *field().AddWithArena(arena(), value);
  }
  template <typename... Args>
  auto& Emplace(Args&&... args) const {
    return *field().EmplaceWithArena(arena(), std::forward<Args>(args)...);
  }

 private:
  friend RepeatedFieldProxyInternalPrivateAccessHelper<ElementType, kOrProxy>;

  Arena* PROTOBUF_NULLABLE const arena_;
};

template <typename ElementType, bool kOrProxy>
class PROTOBUF_DECLSPEC_EMPTY_BASES ConstRepeatedFieldProxyImpl
    : public internal::RepeatedFieldProxyBase<const ElementType, kOrProxy> {
  // A specialization of RepeatedFieldProxy for const proxies. This is needed
  // for mutating methods to not be exposed on const proxies.

 protected:
  using Base = internal::RepeatedFieldProxyBase<const ElementType, kOrProxy>;

  // Inherit constructors, but don't publicly expose them.
  //
  // Repeated field proxies have no public constructors aside from a copy
  // constructor. This is intentional, as layout of data that is proxied is an
  // implementation detail. By not exposing a way to construct a proxy, we can
  // freely change the layout of the underlying repeated field.
  using Base::Base;

  using Base::field;

 public:
  using typename Base::const_reference;
  using typename Base::size_type;

  ConstRepeatedFieldProxyImpl(const ConstRepeatedFieldProxyImpl& other) =
      default;
  ConstRepeatedFieldProxyImpl& operator=(const ConstRepeatedFieldProxyImpl&) =
      default;

  // Returns a type which references the element at the given index. Performs
  // bounds checking in accordance with `bounds_check_mode_*`.
  [[nodiscard]] const_reference operator[](size_type index) const {
    return field()[index];
  }

 private:
  // Note that we don't need an arena pointer here, since we don't mutate the
  // underlying repeated field.
};

}  // namespace internal

// A proxy for a repeated field of type `ElementType` in a Protobuf message.
// Proxies alias the repeated field and provide an interface to read or modify
// it, following STL naming conventions.
//
// Proxies themselves are value types, meaning they should be passed around by
// value similar to `absl::string_view` or `absl::Span`.
//
// Proxies cannot be constructed directly. They are returned from a message's
// repeated field accessors which have the `features.(pb.cpp).repeated_type =
// PROXY` annotation. This annotation is currently only available in edition
// `UNSTABLE`, but will eventually be available in an upcoming edition.
template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxy final
    : public internal::MutableRepeatedFieldProxyImpl<ElementType,
                                                     /*kOrProxy=*/false> {
  static_assert(!std::is_const_v<ElementType>);

 private:
  using Base = internal::MutableRepeatedFieldProxyImpl<ElementType,
                                                       /*kOrProxy=*/false>;
  using Base::Base;
  using Base::field;

 public:
  // Copy-assigns `other` into this repeated field.
  //
  // This method exists because mutable proxies cannot be rebound.
  void assign(RepeatedFieldProxy<const ElementType> other) const {
    field().CopyFrom(other.field());
  }

  // Because we have an overload of `assign` in this class, we need to
  // explicitly inherit the overload from the base class to avoid hiding it.
  using Base::assign;

  // Move-assigns `other` into this repeated field. `other` is left in a valid
  // but unspecified state.
  void move_assign(RepeatedFieldProxy<ElementType> other) const {
    field() = std::move(other.field());
  }

  // Swaps the contents of this repeated field with `other`.
  //
  // Invalidates all iterators. Pointer stability is not guaranteed across the
  // swap for any element of either repeated field.
  //
  // If the underlying repeated fields are on different arenas, this may force
  // deep copies of the elements.
  void swap(RepeatedFieldProxy other) const { field().Swap(&other.field()); }

 private:
  friend RepeatedFieldProxy<const ElementType>;
  friend RepeatedFieldOrProxy<ElementType>;
  friend RepeatedFieldOrProxy<const ElementType>;
  friend internal::RepeatedFieldProxyInternalPrivateAccessHelper<
      ElementType, /*kOrProxy=*/false>;
};

template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxy<const ElementType> final
    : public internal::ConstRepeatedFieldProxyImpl<ElementType,
                                                   /*kOrProxy=*/false> {
  // A specialization of RepeatedFieldProxy for const proxies. This is needed
  // for mutating methods to not be exposed on const proxies.

 private:
  using Base = internal::ConstRepeatedFieldProxyImpl<ElementType,
                                                     /*kOrProxy=*/false>;

  // Inherit constructors, but don't publicly expose them.
  //
  // Repeated field proxies have no public constructors aside from a copy
  // constructor. This is intentional, as layout of data that is proxied is an
  // implementation detail. By not exposing a way to construct a proxy, we can
  // freely change the layout of the underlying repeated field.
  using Base::Base;

 public:
  // Allow implicit conversion from a mutable RepeatedFieldProxy to a const
  // RepeatedFieldProxy.
  //
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldProxy(RepeatedFieldProxy<ElementType> other)
      : Base(other.field()) {}

 private:
  friend RepeatedFieldProxy<ElementType>;
  friend RepeatedFieldOrProxy<const ElementType>;
  friend internal::RepeatedFieldProxyInternalPrivateAccessHelper<
      const ElementType, /*kOrProxy=*/false>;
};

// The size of proxies is not really important, since they should mostly be
// passed around by value and inlined away to oblivion. Regardless, size
// assertions guarantee that the compiler hasn't introduced invisible members
// that we didn't notice (e.g. `PROTOBUF_DECLSPEC_EMPTY_BASES`).
static_assert(sizeof(RepeatedFieldProxy<int>) == 2 * sizeof(void*));
static_assert(sizeof(RepeatedFieldProxy<const int>) == sizeof(void*));

namespace internal {

// A helper class for accessing private members of `RepeatedFieldProxy` in
// Protobuf internal code.
//
// DO NOT USE this class for any reason outside of protobuf internal code.
template <typename ElementType, bool kOrProxy = false>
class RepeatedFieldProxyInternalPrivateAccessHelper {
  using ProxyType =
      std::conditional_t<kOrProxy, RepeatedFieldOrProxy<ElementType>,
                         RepeatedFieldProxy<ElementType>>;

  // Casts up to a `MutableRepeatedFieldProxyImpl<ElementType>` from a subclass
  // of `MutableRepeatedFieldProxyImpl<ElementType>`. This is used to implement
  // the CRTP pattern for `*With<MethodName>` classes.
  template <typename C>
  static MutableRepeatedFieldProxyImpl<ElementType, kOrProxy> ToProxyType(
      const C* PROTOBUF_NONNULL proxy) {
    return *static_cast<
        const MutableRepeatedFieldProxyImpl<ElementType, kOrProxy>*>(proxy);
  }

 public:
  template <typename... Args>
  static RepeatedFieldProxy<ElementType> Construct(Args&&... args) {
    return RepeatedFieldProxy<ElementType>(std::forward<Args>(args)...);
  }

  static auto& field(const ProxyType& proxy) { return proxy.field(); }

  // Takes any subclass of `RepeatedFieldProxy<ElementType>`, upcasts to
  // `RepeatedFieldProxy<ElementType>`, then calls `field()`. This is used to
  // implement the CRTP pattern for `*With<MethodName>` classes.
  template <typename C>
  static auto& field(const C* PROTOBUF_NONNULL proxy) {
    return ToProxyType(proxy).field();
  }

  template <typename C, typename... Args>
  static auto& Add(const C* PROTOBUF_NONNULL proxy, Args&&... args) {
    return ToProxyType(proxy).Add(std::forward<Args>(args)...);
  }
  template <typename C, typename... Args>
  static auto& Emplace(const C* PROTOBUF_NONNULL proxy, Args&&... args) {
    return ToProxyType(proxy).Emplace(std::forward<Args>(args)...);
  }
};

}  // namespace internal

// A mutable `RepeatedFieldOrProxy` for a repeated field of type `ElementType`
// in a Protobuf message. Proxies alias the repeated field and provide an
// interface to read or modify it, following STL naming conventions.
//
// Unlike `RepeatedFieldProxy`, `RepeatedFieldOrProxy` can be constructed from
// the legacy repeated field containers (`google::protobuf::RepeatedField` and
// `google::protobuf::RepeatedPtrField`). This container can be used in code which has not
// yet fully migrated to proxies. It is particularly useful for function
// parameters that have many callers, allowing the callers to be migrated to
// proxies incrementally.
//
// Proxies themselves are value types, meaning they should be passed around by
// value similar to `absl::string_view` or `absl::Span`.
template <typename ElementType>
class RepeatedFieldOrProxy final
    : public internal::MutableRepeatedFieldProxyImpl<ElementType,
                                                     /*kOrProxy=*/true> {
  // `const ElementType` is specialized below.
  static_assert(!std::is_const_v<ElementType>);

 private:
  using Base = internal::MutableRepeatedFieldProxyImpl<ElementType,
                                                       /*kOrProxy=*/true>;
  using RepeatedFieldType = typename Base::RepeatedFieldType;

  // Inherit constructors.
  using Base::Base;

  using Base::field;

 public:
  // Allow implicit conversion from a RepeatedField to a RepeatedFieldOrProxy.
  //
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldOrProxy(RepeatedFieldType& field)
      : Base(field, field.GetArena()) {}

  // Allow implicit conversion from a RepeatedField* to a RepeatedFieldOrProxy,
  // but inline this to call the RepeatedFieldOrProxy(RepeatedFieldType& field)
  // constructor.
  //
  // This will be used to ease migration along for functions that currently take
  // a RepeatedField* parameter. If we allow implicit conversion from
  // RepeatedField* to RepeatedFieldOrProxy, then we can change the type of the
  // parameter from RepeatedField<T>* to RepeatedFieldOrProxy<T> without
  // updating any callers. Then, the C++ inliner will later come along and
  // dereference the repeated field pointer argument to the method at callsites.
  PROTOBUF_REFACTOR_INLINE()
  // NOLINTNEXTLINE
  RepeatedFieldOrProxy(RepeatedFieldType* PROTOBUF_NONNULL field)
      : RepeatedFieldOrProxy(*field) {}

  // Allow implicit conversion from a RepeatedFieldProxy to a
  // RepeatedFieldOrProxy.
  //
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldOrProxy(RepeatedFieldProxy<ElementType> proxy)
      : Base(proxy.field(), proxy.arena()) {}

  // Allow explicit conversion to the legacy repeated field container.
  //
  // Note: this performs a deep copy of the underlying repeated field.
  explicit operator std::remove_const_t<RepeatedFieldType>() const {
    return RepeatedFieldType(static_cast<const Base&>(*this));
  }

  // Note: we do not directly expose the following methods from the base class
  // because they have overloads that take a `RepeatedFieldProxy`. We re-define
  // them below with overloads that take `RepeatedFieldOrProxy`.

  // Copy-assigns `other` into this repeated field.
  //
  // This method exists because mutable proxies cannot be rebound.
  void assign(RepeatedFieldOrProxy<const ElementType> other) const {
    Base::field().CopyFrom(other.field());
  }

  // Because we have an overload of `assign` in this class, we need to
  // explicitly inherit the overload from the base class to avoid hiding it.
  using Base::assign;

  // Move-assigns `other` into this repeated field. `other` is left in a valid
  // but unspecified state.
  //
  // This method only differs from `assign` in that it is a hint that you don't
  // need the contents of `other` anymore. It is not guaranteed that the
  // contents will be efficiently transferred, nor that pointer stability will
  // be preserved for elements of `other`.
  void move_assign(RepeatedFieldOrProxy other) const {
    field() = std::move(other.field());
  }

  // Swaps the contents of this repeated field with `other`.
  //
  // Invalidates all iterators. Pointer stability is not guaranteed across the
  // swap for any element of either repeated field.
  //
  // If the underlying repeated fields are on different arenas, this may force
  // deep copies of the elements.
  void swap(RepeatedFieldOrProxy other) const {
    Base::field().Swap(&other.field());
  }

 private:
  friend RepeatedFieldProxy<ElementType>;
  friend RepeatedFieldOrProxy<const ElementType>;
  friend internal::RepeatedFieldProxyInternalPrivateAccessHelper<
      ElementType, /*kOrProxy=*/true>;
};

// A const proxy for a repeated field of type `ElementType` in a Protobuf
// message. Proxies alias the repeated field and provide an interface to read or
// modify it, following STL naming conventions.
//
// Unlike `RepeatedFieldProxy`, `RepeatedFieldOrProxy` can be constructed from
// the legacy repeated field containers (`google::protobuf::RepeatedField` and
// `google::protobuf::RepeatedPtrField`). This container can be used in code which has not
// yet fully migrated to proxies. It is particularly useful for function
// parameters that have many callers, allowing the callers to be migrated to
// proxies incrementally.
//
// Proxies themselves are value types, meaning they should be passed around by
// value similar to `absl::string_view` or `absl::Span`.
template <typename ElementType>
class RepeatedFieldOrProxy<const ElementType> final
    : public internal::ConstRepeatedFieldProxyImpl<ElementType,
                                                   /*kOrProxy=*/true> {
  using Base = internal::ConstRepeatedFieldProxyImpl<ElementType,
                                                     /*kOrProxy=*/true>;
  using RepeatedFieldType = typename Base::RepeatedFieldType;

  // Inherit constructors.
  using Base::Base;

  using Base::field;

 public:
  // Allow implicit conversion from a mutable RepeatedFieldOrProxy to a const
  // RepeatedFieldOrProxy.
  //
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldOrProxy(RepeatedFieldOrProxy<ElementType> other)
      : Base(other.field()) {}

  // Allow implicit conversion from a RepeatedField to a RepeatedFieldOrProxy.
  //
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldOrProxy(const RepeatedFieldType& field) : Base(field) {}

  // Allow implicit conversion from a const RepeatedField* to a
  // RepeatedFieldOrProxy, but inline this to call the
  // RepeatedFieldOrProxy(RepeatedFieldType& field) constructor.
  //
  // This will be used to ease migration along for functions that currently take
  // a const RepeatedField* parameter. If we allow implicit conversion from
  // const RepeatedField* to RepeatedFieldOrProxy, then we can change the type
  // of the parameter from const RepeatedField<T>* to
  // RepeatedFieldOrProxy<const T> without updating any callers. Then, the C++
  // inliner will later come along and dereference the repeated field pointer
  // argument to the method at callsites.
  PROTOBUF_REFACTOR_INLINE()
  // NOLINTNEXTLINE
  RepeatedFieldOrProxy(RepeatedFieldType* PROTOBUF_NONNULL field)
      : RepeatedFieldOrProxy(*field) {}

  // Allow implicit conversion from a RepeatedFieldProxy to a
  // RepeatedFieldOrProxy.
  //
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldOrProxy(RepeatedFieldProxy<ElementType> proxy)
      : Base(proxy.field()) {}

  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldOrProxy(RepeatedFieldProxy<const ElementType> proxy)
      : Base(proxy.field()) {}

  // Allow explicit conversion to the legacy repeated field container.
  //
  // Note: this performs a deep copy of the underlying repeated field.
  explicit operator RepeatedFieldType() const {
    return RepeatedFieldType(static_cast<const Base&>(*this));
  }

 private:
  friend RepeatedFieldOrProxy<ElementType>;
};

static_assert(sizeof(RepeatedFieldOrProxy<int>) ==
                  sizeof(RepeatedFieldProxy<int>),
              "Mutable `RepeatedFieldOrProxy` is not the expected size");
static_assert(sizeof(RepeatedFieldOrProxy<const int>) ==
                  sizeof(RepeatedFieldProxy<const int>),
              "Const `RepeatedFieldOrProxy` is not the expected size");

// Like C++20's std::erase_if, for RepeatedFieldProxy
template <int&... DeductionBarrier, typename T, typename Pred>
size_t erase_if(RepeatedFieldProxy<T> cont, Pred pred) {
  return google::protobuf::erase_if(
      internal::RepeatedFieldProxyInternalPrivateAccessHelper<
          T, /*kOrProxy=*/false>::field(cont),
      pred);
}

// Like C++20's std::erase, for RepeatedFieldProxy
template <int&... DeductionBarrier, typename T, typename U>
size_t erase(RepeatedFieldProxy<T> cont, const U& value) {
  return google::protobuf::erase(internal::RepeatedFieldProxyInternalPrivateAccessHelper<
                           T, /*kOrProxy=*/false>::field(cont),
                       value);
}

// Like C++20's std::erase_if, for RepeatedFieldOrProxy.
template <int&... DeductionBarrier, typename T, typename Pred>
size_t erase_if(RepeatedFieldOrProxy<T> cont, Pred pred) {
  return google::protobuf::erase_if(
      internal::RepeatedFieldProxyInternalPrivateAccessHelper<
          T, /*kOrProxy=*/true>::field(cont),
      pred);
}

// Like C++20's std::erase, for RepeatedFieldOrProxy.
template <int&... DeductionBarrier, typename T, typename U>
size_t erase(RepeatedFieldOrProxy<T> cont, const U& value) {
  return google::protobuf::erase(internal::RepeatedFieldProxyInternalPrivateAccessHelper<
                           T, /*kOrProxy=*/true>::field(cont),
                       value);
}

// Like C++20's std::sort, for RepeatedFieldProxy.
template <int&... DeductionBarrier, typename T, typename Compare>
void c_sort(RepeatedFieldProxy<T> cont, Compare cmp) {
  google::protobuf::sort(cont.begin(), cont.end(), cmp);
}
// Like C++20's std::sort, for RepeatedFieldProxy, with default comparison.
template <int&... DeductionBarrier, typename T>
void c_sort(RepeatedFieldProxy<T> cont) {
  google::protobuf::sort(cont.begin(), cont.end());
}
// Like C++20's std::stable_sort, for RepeatedFieldProxy.
template <int&... DeductionBarrier, typename T, typename Compare>
void c_stable_sort(RepeatedFieldProxy<T> cont, Compare cmp) {
  google::protobuf::stable_sort(cont.begin(), cont.end(), cmp);
}
// Like C++20's std::stable_sort, for RepeatedFieldProxy, with default
// comparison.
template <int&... DeductionBarrier, typename T>
void c_stable_sort(RepeatedFieldProxy<T> cont) {
  google::protobuf::stable_sort(cont.begin(), cont.end());
}

// Like C++20's std::sort, for RepeatedFieldOrProxy.
template <int&..., typename T, typename Compare>
void c_sort(RepeatedFieldOrProxy<T> cont, Compare cmp) {
  google::protobuf::sort(cont.begin(), cont.end(), cmp);
}
// Like C++20's std::sort, for RepeatedFieldOrProxy, with default comparison.
template <int&..., typename T>
void c_sort(RepeatedFieldOrProxy<T> cont) {
  google::protobuf::sort(cont.begin(), cont.end());
}
// Like C++20's std::stable_sort, for RepeatedFieldOrProxy.
template <int&..., typename T, typename Compare>
void c_stable_sort(RepeatedFieldOrProxy<T> cont, Compare cmp) {
  google::protobuf::stable_sort(cont.begin(), cont.end(), cmp);
}
// Like C++20's std::stable_sort, for RepeatedFieldOrProxy, with default
// comparison.
template <int&..., typename T>
void c_stable_sort(RepeatedFieldOrProxy<T> cont) {
  google::protobuf::stable_sort(cont.begin(), cont.end());
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
