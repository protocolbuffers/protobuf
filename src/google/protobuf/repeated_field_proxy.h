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

namespace internal {

template <typename ElementType>
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
template <typename ElementType>
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
      internal::RepeatedFieldProxyIterator<const ElementType>;
  using iterator = internal::RepeatedFieldProxyIterator<ElementType>;
  using const_reverse_iterator =
      internal::RepeatedFieldProxyReverseIterator<const ElementType>;
  using reverse_iterator =
      internal::RepeatedFieldProxyReverseIterator<ElementType>;

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
  ConstQualifiedRepeatedFieldType* field_;
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
template <typename ElementType, typename Enable = void>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithSet {
 public:
  // Sets the element at the given index to the given value.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, ElementType value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::field(this);
    field[index] = value;
  }
};

// Defines `set()` for message element types, which take by const reference or
// rvalue.
template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithSet<
    ElementType, std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
 public:
  // Sets the element at the given index to the given value by move-assignment.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, ElementType&& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::field(this);
    field[index] = std::move(value);
  }

  // Sets the element at the given index to the given value by copy-assignment.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, const ElementType& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::field(this);
    field[index] = value;
  }
};

// Defines `set()` for string element types, which dispatch to
// `string_util::SetElement` and accept many string-like types.
template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithSet<
    ElementType, std::enable_if_t<RepeatedElementTypeIsString<ElementType>>> {
 public:
  // Sets the element at the given index to the given value.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  template <typename T>
  void set(size_t index, T&& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::field(this);
    string_util::SetElement(field[index], std::forward<T>(value));
  }
};

// Defines `push_back()` for primitive element types, which only take by value.
template <typename ElementType, typename Enable = void>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithPushBack {
 public:
  // Appends the given value to the end of the repeated field.
  void push_back(ElementType value) const {
    RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Add(this,
                                                                    value);
  }
};

// Defines `push_back()` for message element types, which take by const
// reference or rvalue.
template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithPushBack<
    ElementType, std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
 public:
  // Appends the given value to the end of the repeated field by move
  // construction/assignment.
  void push_back(ElementType&& value) const {
    RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Add(
        this, std::move(value));
  }

  // Appends the given value to the end of the repeated field by copy
  // construction/assignment.
  void push_back(const ElementType& value) const {
    RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Add(this,
                                                                    value);
  }
};

// Defines `push_back()` for string element types, which dispatch to
// `string_util::SetElement` and accept many string-like types.
template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithPushBack<
    ElementType, std::enable_if_t<RepeatedElementTypeIsString<ElementType>>> {
 public:
  // Appends the given value to the end of the repeated field.
  template <typename T>
  void push_back(T&& value) const {
    string_util::SetElement(
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Add(this),
        std::forward<T>(value));
  }
};

// Defines `emplace_back()` for all types except `absl::string_view`. Simply
// takes any arguments that can be passed to the constructor of `ElementType`
// and in-place constructs the element at the end of the repeated field.
template <typename ElementType, typename Enable = void>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithEmplaceBack {
 public:
  // In-place constructs an element at the end of the repeated field, returning
  // a reference to the newly constructed element.
  template <typename... Args>
  auto& emplace_back(Args&&... args) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Emplace(
        this, std::forward<Args>(args)...);
  }
};

// Defines `emplace_back()` for `absl::string_view` element types. We explicitly
// list all constructors we want to support for repeated `string_views` to not
// leak the `std::string` backing of repeated `string_views`.
template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithEmplaceBack<
    ElementType,
    std::enable_if_t<std::is_same_v<ElementType, absl::string_view>>> {
 public:
  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back() const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Emplace(
        this);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(absl::string_view value) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Emplace(
        this, value);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(std::string&& value) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Emplace(
        this, std::move(value));
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(const std::string& value) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Emplace(
        this, value);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(const char* value) const {
    return RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::Emplace(
        this, value);
  }
};

// Defines `resize(new_size, value)` for all non-string repeated fields.
template <typename ElementType, typename Enable = void>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithResize {
 public:
  void resize(size_t new_size, const ElementType& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::field(this);
    field.resize(new_size, value);
  }
};

// Defines `resize(new_size, value)` for non-Cord string repeated fields.
template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithResize<
    ElementType, std::enable_if_t<RepeatedElementTypeIsString<ElementType> &&
                                  !std::is_same_v<ElementType, absl::Cord>>> {
 public:
  void resize(size_t new_size, absl::string_view value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::field(this);
    field.resize(new_size, value);
  }
};

// Defines `resize(new_size, value)` for repeated Cords.
template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxyWithResize<
    ElementType, std::enable_if_t<std::is_same_v<ElementType, absl::Cord>>> {
 public:
  void resize(size_t new_size, const absl::Cord& value) const {
    auto& field =
        RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>::field(this);
    field.resize(new_size, value);
  }
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
    : public internal::RepeatedFieldProxyBase<ElementType>,
      public internal::RepeatedFieldProxyWithSet<ElementType>,
      public internal::RepeatedFieldProxyWithPushBack<ElementType>,
      public internal::RepeatedFieldProxyWithEmplaceBack<ElementType>,
      public internal::RepeatedFieldProxyWithResize<ElementType> {
  static_assert(!std::is_const_v<ElementType>);

 protected:
  using Base = internal::RepeatedFieldProxyBase<ElementType>;

  using typename Base::const_iterator;
  using typename Base::iterator;
  using typename Base::RepeatedFieldType;
  using typename Base::size_type;

  using reference =
      typename internal::RepeatedFieldTraits<ElementType>::reference;

  using Base::field;

 public:
  RepeatedFieldProxy(const RepeatedFieldProxy& other) = default;
  // Mutable proxies are not assignable. This is intentional to avoid confusion
  // with the `assign` method, which reassigns the underlying repeated field.
  RepeatedFieldProxy& operator=(const RepeatedFieldProxy&) = delete;

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

  // Copy-assigns `other` into this repeated field.
  //
  // This method exists because proxies cannot be reassigned through the `=`
  // assignment operator.
  void assign(RepeatedFieldProxy<const ElementType> other) const {
    field().CopyFrom(other.field());
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

  // Move-assigns `other` into this repeated field. `other` is left in a valid
  // but unspecified state.
  void move_assign(RepeatedFieldProxy<ElementType> other) const {
    field() = std::move(other.field());
  }

  // A hint to the container to expect to grow/shrink to `new_size` elements.
  // This may allow the container to make optimizations to avoid reallocations,
  // but may also be ignored.
  void reserve(size_type new_size) const {
    field().ReserveWithArena(arena(), new_size);
  }

  // Swaps the contents of this repeated field with `other`.
  //
  // Invalidates all iterators. Pointer stability is not guaranteed across the
  // swap for any element of either repeated field.
  //
  // If the underlying repeated fields are on different arenas, this may force
  // deep copies of the elements.
  void swap(RepeatedFieldProxy other) const { field().Swap(&other.field()); }

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with default-valued elements.
  void resize(size_t new_size) const { field().resize(new_size); }

  // Because we have an overload of `resize` in this class, we need to
  // explicitly inherit the overload from the base class to avoid hiding it.

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  using internal::RepeatedFieldProxyWithResize<ElementType>::resize;

 private:
  friend RepeatedFieldProxy<const ElementType>;

  friend internal::RepeatedFieldProxyInternalPrivateAccessHelper<ElementType>;

  RepeatedFieldProxy(RepeatedFieldType& field, Arena* arena)
      : Base(field), arena_(arena) {
    ABSL_DCHECK_EQ(arena, field.GetArena());
  }

  // The following methods all forward to the backing repeated fields. This is
  // done here for access to private members of the legacy containers, which
  // only need to friend `RepeatedFieldProxy`.
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

  Arena* arena() const { return arena_; }

  Arena* const arena_;
};

template <typename ElementType>
class PROTOBUF_DECLSPEC_EMPTY_BASES RepeatedFieldProxy<const ElementType> final
    : public internal::RepeatedFieldProxyBase<const ElementType> {
  // A specialization of RepeatedFieldProxy for const proxies. This is needed
  // for mutating methods to not be exposed on const proxies.

 protected:
  using Base = internal::RepeatedFieldProxyBase<const ElementType>;
  using typename Base::const_reference;
  using typename Base::size_type;

  // Inherit constructors, but don't publicly expose them.
  //
  // Repeated field proxies have no public constructors aside from a copy
  // constructor. This is intentional, as layout of data that is proxied is an
  // implementation detail. By not exposing a way to construct a proxy, we can
  // freely change the layout of the underlying repeated field.
  using Base::Base;

  using Base::field;

 public:
  RepeatedFieldProxy(const RepeatedFieldProxy& other) = default;
  RepeatedFieldProxy& operator=(const RepeatedFieldProxy&) = default;

  // Allow implicit conversion from a mutable RepeatedFieldProxy to a const
  // RepeatedFieldProxy.
  //
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldProxy(RepeatedFieldProxy<ElementType> other)
      : Base(other.field()) {}

  // Returns a type which references the element at the given index. Performs
  // bounds checking in accordance with `bounds_check_mode_*`.
  [[nodiscard]] const_reference operator[](size_type index) const {
    return field()[index];
  }

 private:
  friend RepeatedFieldProxy<ElementType>;

  friend internal::RepeatedFieldProxyInternalPrivateAccessHelper<
      const ElementType>;

  // Note that we don't need an arena pointer here, since we don't mutate the
  // underlying repeated field.
};

namespace internal {

// The size of proxies is not really important, since they should mostly be
// passed around by value and inlined away to oblivion. Regardless, size
// assertions guarantee that the compiler hasn't introduced invisible members
// that we didn't notice (e.g. `PROTOBUF_DECLSPEC_EMPTY_BASES`).
static_assert(sizeof(RepeatedFieldProxy<int>) == 2 * sizeof(void*));
static_assert(sizeof(RepeatedFieldProxy<const int>) == sizeof(void*));

// A helper class for accessing private members of `RepeatedFieldProxy` in
// Protobuf internal code.
//
// DO NOT USE this class for any reason outside of protobuf internal code.
template <typename ElementType>
class RepeatedFieldProxyInternalPrivateAccessHelper {
  // Casts up to a `RepeatedFieldProxy<ElementType>` from a subclass of
  // `RepeatedFieldProxy<ElementType>`. This is used to implement the CRTP
  // pattern for `*With<MethodName>` classes.
  template <template <typename...> class C>
  static RepeatedFieldProxy<ElementType> ToProxyType(
      const C<ElementType, void>* proxy) {
    return *static_cast<const RepeatedFieldProxy<ElementType>*>(proxy);
  }

 public:
  template <typename... Args>
  static RepeatedFieldProxy<ElementType> Construct(Args&&... args) {
    return RepeatedFieldProxy<ElementType>(std::forward<Args>(args)...);
  }

  static auto& field(const RepeatedFieldProxy<ElementType>& proxy) {
    return proxy.field();
  }

  // Takes any subclass of `RepeatedFieldProxy<ElementType>`, upcasts to
  // `RepeatedFieldProxy<ElementType>`, then calls `field()`. This is used to
  // implement the CRTP pattern for `*With<MethodName>` classes.
  template <template <typename...> class C>
  static auto& field(const C<ElementType, void>* proxy) {
    return ToProxyType(proxy).field();
  }

  template <template <typename...> class C, typename... Args>
  static auto& Add(const C<ElementType, void>* proxy, Args&&... args) {
    return ToProxyType(proxy).Add(std::forward<Args>(args)...);
  }
  template <template <typename...> class C, typename... Args>
  static auto& Emplace(const C<ElementType, void>* proxy, Args&&... args) {
    return ToProxyType(proxy).Emplace(std::forward<Args>(args)...);
  }
};

}  // namespace internal

// Like C++20's std::erase_if, for RepeatedFieldProxy
template <int&... DeductionBarrier, typename T, typename Pred>
size_t erase_if(RepeatedFieldProxy<T> cont, Pred pred) {
  return google::protobuf::erase_if(
      internal::RepeatedFieldProxyInternalPrivateAccessHelper<T>::field(cont),
      pred);
}

// Like C++20's std::erase, for RepeatedFieldProxy
template <int&... DeductionBarrier, typename T, typename U>
size_t erase(RepeatedFieldProxy<T> cont, const U& value) {
  return google::protobuf::erase(
      internal::RepeatedFieldProxyInternalPrivateAccessHelper<T>::field(cont),
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

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
