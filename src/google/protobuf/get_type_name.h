// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_GET_TYPE_NAME_H__
#define GOOGLE_PROTOBUF_GET_TYPE_NAME_H__

#include <string>
#include <tuple>
#include <type_traits>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"

namespace google::protobuf::internal {

// Returns a human-readable name for `T` (with a best-effort attempt to keep it
// concise and consistent across compilers), or `absl::nullopt` if the type name
// cannot be determined on the current platform.
template <typename T>
absl::optional<std::string> GetTypeName();

// Implementation below.

template <typename T>
constexpr absl::optional<absl::string_view> RawPrettyFunction() {
#if defined(_MSC_VER) && !defined(__clang__)
  return __FUNCSIG__;
#elif defined(__clang__) || defined(__GNUC__)
  return __PRETTY_FUNCTION__;
#else
  return absl::nullopt;
#endif
}

template <template <typename...> class T>
constexpr absl::optional<absl::string_view> RawPrettyFunction() {
#if defined(_MSC_VER) && !defined(__clang__)
  return __FUNCSIG__;
#elif defined(__clang__) || defined(__GNUC__)
  return __PRETTY_FUNCTION__;
#else
  return absl::nullopt;
#endif
}

absl::optional<std::string> CleanRawTypeName(
    absl::optional<absl::string_view> raw);
absl::optional<std::string> FormatTemplateTypeName(
    absl::optional<absl::string_view> raw,
    absl::Span<const absl::optional<std::string>> args);

template <typename T>
struct TypeIdentity {
  using type = T;
};

// Checks whether `Tmpl<Prefix...>` is valid and resolves to `Full`.
template <template <typename...> class Tmpl, typename Full, typename Prefix,
          typename = void>
struct IsSameWithDefaults : std::false_type {};

template <template <typename...> class Tmpl, typename Full, typename... Prefix>
struct IsSameWithDefaults<Tmpl, Full, std::tuple<Prefix...>,
                          std::void_t<Tmpl<Prefix...>>>
    : std::is_same<Tmpl<Prefix...>, Full> {};

// We strip defaults so that
//   std::vector<int>
// is not
//   std::vector<int, std::allocator<int>>
template <template <typename...> class Tmpl, typename Full, typename Prefix,
          typename Remaining>
struct StripDefaultTemplateArgs;

template <template <typename...> class Tmpl, typename Full, typename... Prefix>
struct StripDefaultTemplateArgs<Tmpl, Full, std::tuple<Prefix...>, std::tuple<>>
    : TypeIdentity<std::tuple<Prefix...>> {};

template <template <typename...> class Tmpl, typename Full, typename... Prefix,
          typename Next, typename... Rest>
struct StripDefaultTemplateArgs<Tmpl, Full, std::tuple<Prefix...>,
                                std::tuple<Next, Rest...>>
    : std::conditional_t<
          IsSameWithDefaults<Tmpl, Full, std::tuple<Prefix...>>::value,
          TypeIdentity<std::tuple<Prefix...>>,
          StripDefaultTemplateArgs<Tmpl, Full, std::tuple<Prefix..., Next>,
                                   std::tuple<Rest...>>> {};

template <typename T>
struct TypeNameFormatter {
  static absl::optional<std::string> Get() {
    return CleanRawTypeName(RawPrettyFunction<T>());
  }
};

// Specialization for class templates with type parameters: drops defaulted
// trailing arguments and formats the remaining arguments recursively.
template <template <typename...> class Tmpl, typename... Args>
struct TypeNameFormatter<Tmpl<Args...>> {
  template <typename... Prefix>
  static absl::optional<std::string> Format(std::tuple<Prefix...>*) {
    return FormatTemplateTypeName(RawPrettyFunction<Tmpl>(),
                                  {GetTypeName<Prefix>()...});
  }

  static absl::optional<std::string> Get() {
    using KeptArgs =
        typename StripDefaultTemplateArgs<Tmpl, Tmpl<Args...>, std::tuple<>,
                                          std::tuple<Args...>>::type;
    return Format(static_cast<KeptArgs*>(nullptr));
  }
};

template <typename T>
absl::optional<std::string> GetTypeName() {
  if constexpr (std::is_lvalue_reference_v<T>) {
    if (auto name = GetTypeName<std::remove_reference_t<T>>()) {
      return absl::StrCat(*name, "&");
    }
    return absl::nullopt;
  } else if constexpr (std::is_rvalue_reference_v<T>) {
    if (auto name = GetTypeName<std::remove_reference_t<T>>()) {
      return absl::StrCat(*name, "&&");
    }
    return absl::nullopt;
  } else if constexpr (std::is_const_v<T>) {
    if (auto name = GetTypeName<std::remove_const_t<T>>()) {
      return absl::StrCat("const ", *name);
    }
    return absl::nullopt;
  } else if constexpr (std::is_volatile_v<T>) {
    if (auto name = GetTypeName<std::remove_volatile_t<T>>()) {
      return absl::StrCat("volatile ", *name);
    }
    return absl::nullopt;
  } else if constexpr (std::is_pointer_v<T>) {
    if (auto name = GetTypeName<std::remove_pointer_t<T>>()) {
      return absl::StrCat(*name, "*");
    }
    return absl::nullopt;
  } else if constexpr (std::is_same_v<T, bool>) {
    return "bool";
  } else if constexpr (std::is_same_v<T, char>) {
    return "char";
  } else if constexpr (std::is_integral_v<T>) {
    // Normalize integral types (e.g. `long` vs `long long`) to fixed-width
    // names across platforms.
    return absl::StrCat(std::is_unsigned_v<T> ? "u" : "", "int", sizeof(T) * 8,
                        "_t");
  } else if constexpr (std::is_same_v<T, float>) {
    return "float";
  } else if constexpr (std::is_same_v<T, double>) {
    return "double";
  } else if constexpr (std::is_same_v<T, std::string>) {
    // Hardcode string types so they don't expand to `std::basic_string<char>`.
    return "std::string";
  } else if constexpr (std::is_same_v<T, absl::string_view>) {
    return "absl::string_view";
  } else {
    return TypeNameFormatter<T>::Get();
  }
}

}  // namespace protobuf
}  // namespace google::internal

#endif  // GOOGLE_PROTOBUF_GET_TYPE_NAME_H__
