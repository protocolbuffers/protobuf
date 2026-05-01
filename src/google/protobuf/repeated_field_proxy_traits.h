#ifndef GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_TRAITS_H__
#define GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_TRAITS_H__

#include <string>
#include <type_traits>

#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"


namespace google {
namespace protobuf {
namespace internal {

template <typename ElementType>
static constexpr bool RepeatedElementTypeIsPrimitive =
    std::is_integral_v<ElementType> || std::is_floating_point_v<ElementType>;

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

// RepeatedFieldTraits is a type trait that maps an element type to the concrete
// container type that will back the repeated field in the containing message.
// This is currently either `RepeatedField` or `RepeatedPtrField`.
//
// Note that message and string types are specialized below this base template.
template <typename ElementType, typename Enable = void>
struct RepeatedFieldTraits {
  static_assert(!std::is_const_v<ElementType>);
  // The default specialization is only for primitive types. Messages, strings,
  // and enums are specialized below.
  static_assert(std::is_integral_v<ElementType> ||
                std::is_floating_point_v<ElementType>);

  using type = ::google::protobuf::RepeatedField<ElementType>;
  using const_reference = ElementType;
  using reference = ElementType;
};

// Specialization for enum types.
template <typename ElementType>
struct RepeatedFieldTraits<ElementType,
                           std::enable_if_t<std::is_enum_v<ElementType>>> {
  static_assert(!std::is_const_v<ElementType>);

  using type = ::google::protobuf::RepeatedField<int>;
  using const_reference = ElementType;
  using reference = ElementType;
};

// Specialization for message types.
template <typename ElementType>
struct RepeatedFieldTraits<
    ElementType, std::enable_if_t<RepeatedElementTypeIsMessage<ElementType>>> {
  static_assert(!std::is_const_v<ElementType>);

  using type = ::google::protobuf::RepeatedPtrField<ElementType>;
  using const_reference = const ElementType&;
  using reference = ElementType&;
};

// Explicit specializations for string types.
template <>
struct RepeatedFieldTraits<absl::string_view> {
  using type = ::google::protobuf::RepeatedPtrField<std::string>;
  using const_reference = absl::string_view;
  using reference = absl::string_view;
};

template <>
struct RepeatedFieldTraits<std::string> {
  using type = ::google::protobuf::RepeatedPtrField<std::string>;
  using const_reference = const std::string&;
  using reference = std::string&;
};

template <>
struct RepeatedFieldTraits<absl::Cord> {
  using type = ::google::protobuf::RepeatedField<absl::Cord>;
  using const_reference = const absl::Cord&;
  using reference = absl::Cord&;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_TRAITS_H__
