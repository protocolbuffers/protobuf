#ifndef GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
#define GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <type_traits>

#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class Arena;
class MessageLite;
template <typename Element>
class RepeatedField;
template <typename Element>
class RepeatedPtrField;
template <typename ElementType>
class RepeatedFieldProxy;

namespace internal {

struct RepeatedFieldBase;
template <typename ElementType>
class TestOnlyRepeatedFieldContainer;

// RepeatedFieldTraits is a type trait that maps an element type to the concrete
// container type that will back the repeated field in the containing message.
// This is currently either `RepeatedField` or `RepeatedPtrField`.
//
// Note that message and string types are specialized below this base template.
template <typename ElementType, typename Enable = void>
struct RepeatedFieldTraits {
  static_assert(!std::is_const_v<ElementType>);
  // The default specialization is only for primitive types. Messages and
  // strings are specialized below.
  static_assert(std::is_integral_v<ElementType> ||
                std::is_floating_point_v<ElementType>);

  using type = RepeatedField<ElementType>;
  // TODO - Use value/proxy for primitive types.
  using const_reference = const ElementType&;
  using reference = ElementType&;
};

// Here we specialize for message types.
//
// We would like to use `std::is_base_of_v<MessageLite, ElementType>` in the
// enable_if condition, but that requires `ElementType` to be complete. In
// contexts where `ElementType` is not complete, such as generated protobuf
// source files/headers that forward declare external types, we only have the
// forward declaration of `ElementType`.
//
// Aside from strings, which are specialized below, all element types other than
// messages are primitive types. Enums may be incomplete, but they are forward
// declared as `enum <EnumName> : int;`. We therefore can distinguish incomplete
// message elements with `std::is_class_v`.
template <typename ElementType>
struct RepeatedFieldTraits<ElementType,
                           std::enable_if_t<std::is_class_v<ElementType>>> {
  static_assert(!std::is_const_v<ElementType>);

  using type = RepeatedPtrField<ElementType>;
  using const_reference = const ElementType&;
  using reference = ElementType&;
};

// Explicit specializations for string types.
template <>
struct RepeatedFieldTraits<absl::string_view> {
  using type = RepeatedPtrField<std::string>;
  // TODO - Use absl::string_view.
  using const_reference = const std::string&;
  using reference = std::string&;
};

template <>
struct RepeatedFieldTraits<std::string> {
  using type = RepeatedPtrField<std::string>;
  using const_reference = const std::string&;
  using reference = std::string&;
};

template <>
struct RepeatedFieldTraits<absl::Cord> {
  using type = RepeatedField<absl::Cord>;
  using const_reference = const absl::Cord&;
  using reference = absl::Cord&;
};

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

  // Note that the iterator types are all exposed via the iterator methods (e.g.
  // `begin()`). Both `RepeatedField::iterator` and `RepeatedPtrField::iterator`
  // are in the google::protobuf::internal namespace, meaning users are forbidden from
  // actually spelling them.
  //
  // This is important, as the concrete type of the iterator leaks the
  // underlying container type. With a forbidden spelling, we have the
  // flexibility to change the iterator type without breaking user code.
  using const_iterator = typename RepeatedFieldType::const_iterator;
  using iterator = std::conditional_t<kIsConst, const_iterator,
                                      typename RepeatedFieldType::iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  // Allow explicit conversion to the backing repeated field type. This will
  // perform a deep copy of the repeated field backed by this proxy.
  //
  // Note that this exposes `RepeatedFieldType`, but will be kept around for
  // backwards compatibility with code that uses `RepeatedField`s or
  // `RepeatedPtrField`s directly. We may freely change `RepeatedFieldType` so
  // long as we maintain this explicit conversion to the legacy container types.
  explicit operator RepeatedFieldType() const {
    return RepeatedFieldType(begin(), end());
  }

  RepeatedFieldProxyBase(const RepeatedFieldProxyBase&) = default;

  // Assignment of repeated field proxies simply rebinds the proxy to a
  // different repeated field. It does not modify the underlying field.
  RepeatedFieldProxyBase& operator=(const RepeatedFieldProxyBase&) = default;

  ~RepeatedFieldProxyBase() = default;

  // Returns true if the repeated field has no elements (size == 0).
  [[nodiscard]] bool empty() const { return field().empty(); }
  // Returns the number of elements in the repeated field.
  [[nodiscard]] size_type size() const {
    return static_cast<size_type>(field().size());
  }

  // Returns a type which references the element at the given index. Performs
  // bounds checking in accordance with `bounds_check_mode_*`.
  [[nodiscard]] const_reference operator[](size_type index) const {
    return field()[index];
  }

  [[nodiscard]] const_iterator cbegin() const { return field().cbegin(); }
  [[nodiscard]] const_iterator cend() const { return field().cend(); }
  [[nodiscard]] iterator begin() const { return field().begin(); }
  [[nodiscard]] iterator end() const { return field().end(); }
  [[nodiscard]] reverse_iterator rbegin() const { return field().rbegin(); }
  [[nodiscard]] reverse_iterator rend() const { return field().rend(); }

 protected:
  explicit RepeatedFieldProxyBase(ConstQualifiedRepeatedFieldType& field)
      : field_(&field) {}

  ConstQualifiedRepeatedFieldType& field() const { return *field_; }

 private:
  ConstQualifiedRepeatedFieldType* field_;
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
class RepeatedFieldProxy final
    : public internal::RepeatedFieldProxyBase<ElementType> {
  static_assert(!std::is_const_v<ElementType>);

 protected:
  using Base = internal::RepeatedFieldProxyBase<ElementType>;

  using typename Base::const_iterator;
  using typename Base::iterator;
  using typename Base::RepeatedFieldType;
  using typename Base::size_type;

  using reference =
      typename internal::RepeatedFieldTraits<ElementType>::reference;

 public:
  RepeatedFieldProxy(const RepeatedFieldProxy& other) = default;
  RepeatedFieldProxy& operator=(const RepeatedFieldProxy&) = default;

  [[nodiscard]] reference operator[](size_type index) const {
    return Base::field()[index];
  }

  // Inserts `value` at the end of the repeated field.
  void push_back(ElementType&& value) const {
    Base::field().AddWithArena(arena(), std::move(value));
  }
  // Inserts `value` at the end of the repeated field.
  void push_back(const ElementType& value) const {
    Base::field().AddWithArena(arena(), value);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a reference to the newly constructed element.
  template <int&... DeductionBarrier, typename... Args>
  reference emplace_back(Args&&... args) const {
    // TODO - This is wasteful since it default-constructs an
    // element in the repeated field, only to copy/move-assign a new element
    // into it.
    return *Base::field().AddWithArena(arena()) =
               ElementType(std::forward<Args>(args)...);
  }

  // Removes the last element from the repeated field.
  void pop_back() const { Base::field().RemoveLast(); }

  // Removes all elements from the repeated field. The field will be empty after
  // this call.
  void clear() const { Base::field().Clear(); }

  // Removes the element at `position` from the repeated field. Returns an
  // iterator to the element immediately following the removed element.
  iterator erase(const_iterator position) const {
    return Base::field().erase(position);
  }

  // Removes the elements in the range `[first, last)` from the repeated field.
  // Returns an iterator to the element immediately following the last removed
  // element.
  iterator erase(const_iterator first, const_iterator last) const {
    return Base::field().erase(first, last);
  }

  // Copy-assigns the elements in the range `[begin, end)` to the repeated
  // field.
  template <typename Iter>
  void assign(Iter begin, Iter end) const {
    // A seemingly redundant verification that the iterator dereferences to a
    // compatible type. We do this to require that `Iter` is indeed an iterator
    // type, not any type that incidentally works with the operations we use
    // right now on `Iter`.
    static_assert(
        std::is_assignable_v<ElementType,
                             typename std::iterator_traits<Iter>::value_type> ||
        std::is_constructible_v<
            ElementType, typename std::iterator_traits<Iter>::value_type>);
    Base::field().Clear();
    Base::field().AddWithArena(arena(), begin, end);
  }

  // Copy-assigns the elements in the initializer list to the repeated field.
  void assign(std::initializer_list<ElementType> values) const {
    assign(values.begin(), values.end());
  }

  // A hint to the container to expect to grow/shrink to `new_size` elements.
  // This may allow the container to make optimizations to avoid reallocations,
  // but may also be ignored.
  void reserve(size_type new_size) const {
    Base::field().ReserveWithArena(arena(), new_size);
  }

  // Swaps the contents of this repeated field with `other`.
  void swap(RepeatedFieldProxy& other) const {
    Base::field().Swap(&other.field());
  }

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with default-valued elements.
  void resize(size_type new_size) const { Base::field().resize(new_size); }

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  void resize(size_type new_size, const ElementType& value) const {
    Base::field().resize(new_size, value);
  }

 private:
  friend RepeatedFieldProxy<const ElementType>;
  friend internal::TestOnlyRepeatedFieldContainer<ElementType>;

  RepeatedFieldProxy(RepeatedFieldType& field, Arena* arena)
      : Base(field), arena_(arena) {
    ABSL_DCHECK_EQ(arena, field.GetArena());
  }

  Arena* arena() const { return arena_; }

  Arena* arena_;
};

template <typename ElementType>
class RepeatedFieldProxy<const ElementType> final
    : public internal::RepeatedFieldProxyBase<const ElementType> {
  // A specialization of RepeatedFieldProxy for const proxies. This is needed
  // for mutating methods to not be exposed on const proxies.

 protected:
  using Base = internal::RepeatedFieldProxyBase<const ElementType>;

  // Inherit constructors, but don't publicly expose them.
  //
  // Repeated field proxies have no public constructors aside from a copy
  // constructor. This is intentional, as layout of data that is proxied is an
  // implementation detail. By not exposing a way to construct a proxy, we can
  // freely change the layout of the underlying repeated field.
  using Base::Base;

 public:
  RepeatedFieldProxy(const RepeatedFieldProxy& other) = default;
  RepeatedFieldProxy& operator=(const RepeatedFieldProxy&) = default;

  // Allow implicit conversion from a mutable RepeatedFieldProxy to a const
  // RepeatedFieldProxy.
  RepeatedFieldProxy(RepeatedFieldProxy<ElementType> other)
      : Base(other.field()) {}

 private:
  friend internal::TestOnlyRepeatedFieldContainer<ElementType>;
};

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
