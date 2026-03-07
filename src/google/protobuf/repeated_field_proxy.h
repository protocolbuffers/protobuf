#ifndef GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
#define GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__

#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/base/no_destructor.h"
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/field_with_arena.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

template <typename ElementType>
class RepeatedFieldProxy;

namespace internal {

template <typename T, typename... Args>
RepeatedFieldProxy<T> ConstructRepeatedFieldProxy(Args&&... args);

// Casts up to a `RepeatedFieldProxy<ElementType>` from a subclass of
// `RepeatedFieldProxy<ElementType>`. This is used to implement the CRTP
// pattern for `*With<MethodName>` classes.
template <template <typename...> class C, typename ElementType>
RepeatedFieldProxy<ElementType> ToProxyType(const C<ElementType>* proxy);

template <typename ElementType, typename Enable>
struct RepeatedFieldProxyEraseImpl;

// A type trait to determine if a repeated field element of type `ElementType`
// is a string type.
template <typename ElementType>
static constexpr bool RepeatedElementTypeIsString =
    std::is_same_v<ElementType, std::string> ||
    std::is_same_v<ElementType, absl::string_view> ||
    std::is_same_v<ElementType, absl::Cord>;

// A type trait to determine if a repeated field element of type `ElementType`
// is a message type.
//
// We would like to use `std::is_base_of_v<MessageLite, ElementType>` to
// determine if `ElementType` is a message type, but that requires `ElementType`
// to be complete. In contexts where `ElementType` is not complete, such as
// generated protobuf source files/headers that forward declare external types,
// we only have the forward declaration of `ElementType`.
//
// Aside from strings, all element types other than messages are primitive
// types. Enums may be incomplete, but they are forward declared as `enum
// <EnumName> : int;`. We therefore can distinguish incomplete message elements
// with `std::is_class_v`.
template <typename ElementType>
static constexpr bool RepeatedElementTypeIsMessage =
    std::is_class_v<ElementType> && !RepeatedElementTypeIsString<ElementType>;

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
  using const_reference = ElementType;
  using reference = ElementType;
  using const_iterator = typename type::const_iterator;
  using iterator = typename type::iterator;
};

// Specialization for message types.
template <typename ElementType>
struct RepeatedFieldTraits<
    ElementType, std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
  static_assert(!std::is_const_v<ElementType>);

  using type = RepeatedPtrField<ElementType>;
  using const_reference = const ElementType&;
  using reference = ElementType&;
  using const_iterator = typename type::const_iterator;
  using iterator = typename type::iterator;
};

// Explicit specializations for string types.
template <>
struct RepeatedFieldTraits<absl::string_view> {
  using type = RepeatedPtrField<std::string>;
  using const_reference = absl::string_view;
  using reference = absl::string_view;
  using const_iterator = RepeatedPtrIterator<absl::string_view>;
  using iterator = RepeatedPtrIterator<absl::string_view>;
};

template <>
struct RepeatedFieldTraits<std::string> {
  using type = RepeatedPtrField<std::string>;
  using const_reference = const std::string&;
  using reference = std::string&;
  using const_iterator = typename type::const_iterator;
  using iterator = typename type::iterator;
};

template <>
struct RepeatedFieldTraits<absl::Cord> {
  using type = RepeatedField<absl::Cord>;
  using const_reference = const absl::Cord&;
  using reference = absl::Cord&;
  using const_iterator = typename type::const_iterator;
  using iterator = typename type::iterator;
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
  using const_iterator = typename Traits::const_iterator;
  using iterator =
      std::conditional_t<kIsConst, const_iterator, typename Traits::iterator>;
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
class RepeatedFieldProxyWithSet {
 public:
  // Sets the element at the given index to the given value.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, ElementType value) const {
    ToProxyType(this).field()[index] = value;
  }
};

// Defines `set()` for message element types, which take by const reference or
// rvalue.
template <typename ElementType>
class RepeatedFieldProxyWithSet<
    ElementType, std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
 public:
  // Sets the element at the given index to the given value by move-assignment.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, ElementType&& value) const {
    ToProxyType(this).field()[index] = std::move(value);
  }

  // Sets the element at the given index to the given value by copy-assignment.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  void set(size_t index, const ElementType& value) const {
    ToProxyType(this).field()[index] = value;
  }
};

// Defines `set()` for string element types, which dispatch to
// `string_util::SetElement` and accept many string-like types.
template <typename ElementType>
class RepeatedFieldProxyWithSet<
    ElementType, std::enable_if_t<RepeatedElementTypeIsString<ElementType>>> {
 public:
  // Sets the element at the given index to the given value.
  //
  // Performs bounds checking in accordance with `bounds_check_mode_*`.
  template <typename T>
  void set(size_t index, T&& value) const {
    string_util::SetElement(ToProxyType(this).field()[index],
                            std::forward<T>(value));
  }
};

// Defines `push_back()` for primitive element types, which only take by value.
template <typename ElementType, typename Enable = void>
class RepeatedFieldProxyWithPushBack {
 public:
  // Appends the given value to the end of the repeated field.
  void push_back(ElementType value) const { ToProxyType(this).Add(value); }
};

// Defines `push_back()` for message element types, which take by const
// reference or rvalue.
template <typename ElementType>
class RepeatedFieldProxyWithPushBack<
    ElementType, std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
 public:
  // Appends the given value to the end of the repeated field by move
  // construction/assignment.
  void push_back(ElementType&& value) const {
    ToProxyType(this).Add(std::move(value));
  }

  // Appends the given value to the end of the repeated field by copy
  // construction/assignment.
  void push_back(const ElementType& value) const {
    ToProxyType(this).Add(value);
  }
};

// Defines `push_back()` for string element types, which dispatch to
// `string_util::SetElement` and accept many string-like types.
template <typename ElementType>
class RepeatedFieldProxyWithPushBack<
    ElementType, std::enable_if_t<RepeatedElementTypeIsString<ElementType>>> {
 public:
  // Appends the given value to the end of the repeated field.
  template <typename T>
  void push_back(T&& value) const {
    string_util::SetElement(ToProxyType(this).Add(), std::forward<T>(value));
  }
};

// Defines `emplace_back()` for all types except `absl::string_view`. Simply
// takes any arguments that can be passed to the constructor of `ElementType`
// and in-place constructs the element at the end of the repeated field.
template <typename ElementType, typename Enable = void>
class RepeatedFieldProxyWithEmplaceBack {
 public:
  // In-place constructs an element at the end of the repeated field, returning
  // a reference to the newly constructed element.
  template <typename... Args>
  auto& emplace_back(Args&&... args) const {
    return ToProxyType(this).Emplace(std::forward<Args>(args)...);
  }
};

// Defines `emplace_back()` for `absl::string_view` element types. We explicitly
// list all constructors we want to support for repeated `string_views` to not
// leak the `std::string` backing of repeated `string_views`.
template <typename ElementType>
class RepeatedFieldProxyWithEmplaceBack<
    ElementType,
    std::enable_if_t<std::is_same_v<ElementType, absl::string_view>>> {
 public:
  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back() const { return ToProxyType(this).Emplace(); }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(absl::string_view value) const {
    return ToProxyType(this).Emplace(value);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(std::string&& value) const {
    return ToProxyType(this).Emplace(std::move(value));
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(const std::string& value) const {
    return ToProxyType(this).Emplace(value);
  }

  // In-place constructs an element at the end of the repeated field, returning
  // a string_view of the newly constructed element.
  absl::string_view emplace_back(const char* value) const {
    return ToProxyType(this).Emplace(value);
  }
};

// Defines `erase()` for all types except `absl::string_view`.
template <typename ElementType, typename Enable = void>
class RepeatedFieldProxyWithErase {
  using Traits = RepeatedFieldTraits<ElementType>;
  using const_iterator = typename Traits::const_iterator;
  using iterator = typename Traits::iterator;

 public:
  // Removes the element at `position` from the repeated field. Returns an
  // iterator to the element immediately following the removed element.
  iterator erase(const_iterator position) const {
    return ToProxyType(this).field().erase(position);
  }

  // Removes the elements in the range `[first, last)` from the repeated field.
  // Returns an iterator to the element immediately following the last removed
  // element.
  iterator erase(const_iterator first, const_iterator last) const {
    return ToProxyType(this).field().erase(first, last);
  }
};

// A specialization of `RepeatedFieldProxyWithErase` for `absl::string_view`
// element types. We need this because the iterator type for repeated
// string_views (`RepeatedPtrIterator<absl::string_view>`) is not the same
// iterator type used by the backing repeated field
// (`RepeatedPtrField<std::string>`). This requires explicit conversions between
// the two iterator types.
template <typename ElementType>
class RepeatedFieldProxyWithErase<
    ElementType,
    std::enable_if_t<std::is_same_v<ElementType, absl::string_view>>> {
  using Traits = RepeatedFieldTraits<absl::string_view>;
  using const_iterator = typename Traits::const_iterator;
  using iterator = typename Traits::iterator;

  using const_string_iterator =
      typename RepeatedFieldTraits<std::string>::const_iterator;

 public:
  // Removes the element at `position` from the repeated field. Returns an
  // iterator to the element immediately following the removed element.
  iterator erase(const_iterator position) const {
    return iterator(
        ToProxyType(this).field().erase(const_string_iterator(position)));
  }

  // Removes the elements in the range `[first, last)` from the repeated field.
  // Returns an iterator to the element immediately following the last removed
  // element.
  iterator erase(const_iterator first, const_iterator last) const {
    return iterator(ToProxyType(this).field().erase(
        const_string_iterator(first), const_string_iterator(last)));
  }
};

// Defines `resize()` for all non-string repeated fields.
template <typename ElementType, typename Enable = void>
class RepeatedFieldProxyWithResize {
 public:
  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with default-valued elements.
  void resize(size_t new_size) const {
    ToProxyType(this).field().resize(new_size);
  }

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  void resize(size_t new_size, const ElementType& value) const {
    ToProxyType(this).field().resize(new_size, value);
  }
};

// Defines `resize()` for non-Cord string repeated fields.
template <typename ElementType>
class RepeatedFieldProxyWithResize<
    ElementType, std::enable_if_t<RepeatedElementTypeIsString<ElementType> &&
                                  !std::is_same_v<ElementType, absl::Cord>>> {
 public:
  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with default-valued elements.
  void resize(size_t new_size) const {
    ToProxyType(this).field().resize(new_size);
  }

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  void resize(size_t new_size, absl::string_view value) const {
    ToProxyType(this).field().resize(new_size, value);
  }
};

// Defines `resize()` for repeated Cords.
template <typename ElementType>
class RepeatedFieldProxyWithResize<
    ElementType, std::enable_if_t<std::is_same_v<ElementType, absl::Cord>>> {
 public:
  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with default-valued elements.
  void resize(size_t new_size) const {
    ToProxyType(this).field().resize(new_size);
  }

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  void resize(size_t new_size, absl::string_view value) const {
    this->resize(new_size, absl::Cord(value));
  }

  // Resizes the repeated field to `new_size` elements. If `new_size` is smaller
  // than the current size, the field is truncated. Otherwise, the field is
  // extended with copies of `value`.
  void resize(size_t new_size, const absl::Cord& value) const {
    ToProxyType(this).field().resize(new_size, value);
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
      public internal::RepeatedFieldProxyWithErase<ElementType>,
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
  RepeatedFieldProxy& operator=(const RepeatedFieldProxy&) = default;

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

  // Copy-assigns `other` into this repeated field.
  void assign(RepeatedFieldProxy<const ElementType> other) const {
    field().CopyFrom(other.field());
  }

  // Copy-assigns the elements in the range `[begin, end)` to the repeated
  // field.
  template <
      int&... DeductionBarrier, typename Iter,
      // A seemingly redundant verification that the iterator dereferences to a
      // type compatible with `push_back`. We do this for two reasons:
      // * As a requirement that `Iter` is indeed an iterator type, not any type
      //   that incidentally works with the operations we use right now on
      //   `Iter`.
      // * To guard against leakage of the backing repeated field type, by
      //   requiring that the element type of `Iter` is compatible with the
      //   `push_back` API.
      //
      // Even though we immediately exercise this condition below when calling
      // `push_back`, we keep the condition here in case the implementation
      // changes.
      typename = std::enable_if_t<std::is_same_v<
          decltype(std::declval<
                       internal::RepeatedFieldProxyWithPushBack<ElementType>>()
                       .push_back(std::declval<typename std::iterator_traits<
                                      Iter>::value_type>())),
          void>>>
  void assign(Iter begin, Iter end) const {
    field().Clear();
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

  // Move-assigns `other` into this repeated field. `other` is cleared
  // afterwards.
  void move_assign(RepeatedFieldProxy<ElementType> other) const {
    // Repeated field move assignment is implemented as a swap. `swap` + `clear`
    // would leave a bunch of cleared objects in `other` when
    // `RepeatedFieldType = RepeatedPtrField`. Since moved objects are typically
    // not used afterwards, there is no benefit to keeping cleared elements
    // around.
    //
    // To avoid the `clear`, we first swap out of `this` into a temporary that
    // is immediately destroyed, and then swap `this` and `other`, which leaves
    // `other` empty with no cleared elements.
    if (arena() == nullptr) {
      RepeatedFieldType temp_field;
      temp_field.UnsafeArenaSwap(&field());
    } else {
      // `FieldWithArena` can't be destroyed when on an arena, so skip its
      // destructor.
      absl::NoDestructor<internal::FieldWithArena<RepeatedFieldType>>
          temp_field(arena());
      temp_field->field().UnsafeArenaSwap(&field());
    }
    field().Swap(&other.field());
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
  // If the underlying repeated fields are on different arenas, this will force
  // deep copies of the elements.
  void swap(RepeatedFieldProxy other) const { field().Swap(&other.field()); }

 private:
  friend RepeatedFieldProxy<const ElementType>;

  friend internal::RepeatedFieldProxyWithSet<ElementType, void>;
  friend internal::RepeatedFieldProxyWithPushBack<ElementType, void>;
  friend internal::RepeatedFieldProxyWithEmplaceBack<ElementType, void>;
  friend internal::RepeatedFieldProxyWithErase<ElementType, void>;
  friend internal::RepeatedFieldProxyWithResize<ElementType, void>;

  friend internal::RepeatedFieldProxyEraseImpl<ElementType, void>;

  template <typename T, typename... Args>
  friend RepeatedFieldProxy<T> internal::ConstructRepeatedFieldProxy(
      Args&&... args);

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

  Arena* arena_;
};

template <typename ElementType>
class RepeatedFieldProxy<const ElementType> final
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

  template <typename T, typename... Args>
  friend RepeatedFieldProxy<T> internal::ConstructRepeatedFieldProxy(
      Args&&... args);

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

// A helper function to construct a `RepeatedFieldProxy`. This is more scalable
// than friending all places that need to construct `RepeatedFieldProxy`.
template <typename T, typename... Args>
inline RepeatedFieldProxy<T> ConstructRepeatedFieldProxy(Args&&... args) {
  return RepeatedFieldProxy<T>(std::forward<Args>(args)...);
}

template <template <typename...> class C, typename ElementType>
RepeatedFieldProxy<ElementType> ToProxyType(const C<ElementType>* proxy) {
  return *static_cast<const RepeatedFieldProxy<ElementType>*>(proxy);
}

// Specialization of erase/erase_if for repeated numeric fields.
template <typename ElementType, typename Enable = void>
struct RepeatedFieldProxyEraseImpl {
 public:
  template <typename Pred>
  static size_t erase_if(RepeatedFieldProxy<ElementType> field, Pred pred) {
    return google::protobuf::erase_if(field.field(),
                            // Intentionally decay `elem` to avoid exposing a
                            // reference directly to the element.
                            [&pred](auto elem) { return pred(elem); });
  }

  static size_t erase(RepeatedFieldProxy<ElementType> field,
                      ElementType value) {
    return google::protobuf::erase(field.field(), value);
  }
};

// Specialization of erase/erase_if for repeated string fields.
template <typename ElementType>
struct RepeatedFieldProxyEraseImpl<
    ElementType, std::enable_if_t<RepeatedElementTypeIsString<ElementType>>> {
 public:
  template <typename Pred>
  static size_t erase_if(RepeatedFieldProxy<ElementType> field, Pred pred) {
    return google::protobuf::erase_if(field.field(), [&pred](const auto& elem) {
      return pred(absl::string_view(elem));
    });
  }

  static size_t erase(RepeatedFieldProxy<ElementType> field,
                      absl::string_view value) {
    return google::protobuf::erase(field.field(), value);
  }
};

// Specialization of erase/erase_if for repeated message fields.
template <typename ElementType>
struct RepeatedFieldProxyEraseImpl<
    ElementType, std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
 public:
  template <typename Pred>
  static size_t erase_if(RepeatedFieldProxy<ElementType> field, Pred pred) {
    return google::protobuf::erase_if(field.field(), pred);
  }

  static size_t erase(RepeatedFieldProxy<ElementType> field,
                      const ElementType& value) {
    return google::protobuf::erase(field.field(), value);
  }
};

}  // namespace internal

// Like C++20's std::erase_if, for RepeatedFieldProxy
template <int&... DeductionBarrier, typename T, typename Pred>
size_t erase_if(RepeatedFieldProxy<T> cont, Pred pred) {
  return internal::RepeatedFieldProxyEraseImpl<T>::erase_if(cont, pred);
}

// Like C++20's std::erase, for RepeatedFieldProxy
template <int&... DeductionBarrier, typename T, typename U>
size_t erase(RepeatedFieldProxy<T> cont, const U& value) {
  return internal::RepeatedFieldProxyEraseImpl<T>::erase(cont, value);
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
