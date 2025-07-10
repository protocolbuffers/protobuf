// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/descriptor.h"

#include <fcntl.h>
#include <limits.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>  // IWYU pragma: keep
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/base/attributes.h"
#include "absl/base/call_once.h"
#include "absl/base/casts.h"
#include "absl/base/const_init.h"
#include "absl/base/dynamic_annotations.h"
#include "absl/base/optimization.h"
#include "absl/base/thread_annotations.h"
#include "absl/cleanup/cleanup.h"
#include "absl/container/btree_map.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/functional/function_ref.h"
#include "absl/hash/hash.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/charset.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/span.h"
#include "google/protobuf/any.h"
#include "google/protobuf/cpp_edition_defaults.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/descriptor_lite.h"
#include "google/protobuf/descriptor_visitor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/feature_resolver.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/internal_feature_helper.h"
#include "google/protobuf/io/strtod.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unknown_field_set.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace {

const int kPackageLimit = 100;


size_t CamelCaseSize(const absl::string_view input) {
  return input.size() - absl::c_count(input, '_');
}

std::string ToCamelCase(const absl::string_view input, bool lower_first) {
  bool capitalize_next = !lower_first;
  std::string result;
  result.reserve(input.size());

  for (char character : input) {
    if (character == '_') {
      capitalize_next = true;
    } else if (capitalize_next) {
      result.push_back(absl::ascii_toupper(character));
      capitalize_next = false;
    } else {
      result.push_back(character);
    }
  }

  // Lower-case the first letter.
  if (lower_first && !result.empty()) {
    result[0] = absl::ascii_tolower(result[0]);
  }

  ABSL_DCHECK_EQ(CamelCaseSize(input), result.size());

  return result;
}

size_t JsonNameSize(const absl::string_view input) {
  return input.size() - absl::c_count(input, '_');
}

std::string ToJsonName(const absl::string_view input) {
  bool capitalize_next = false;
  std::string result;
  result.reserve(input.size());

  for (char character : input) {
    if (character == '_') {
      capitalize_next = true;
    } else if (capitalize_next) {
      result.push_back(absl::ascii_toupper(character));
      capitalize_next = false;
    } else {
      result.push_back(character);
    }
  }

  ABSL_DCHECK_EQ(JsonNameSize(input), result.size());

  return result;
}

template <typename OptionsT>
bool IsLegacyJsonFieldConflictEnabled(const OptionsT& options) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  return options.deprecated_legacy_json_field_conflicts();
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

// Backport of fold expressions for the comma operator to C++11.
// Usage:  Fold({expr...});
// Guaranteed to evaluate left-to-right
struct ExpressionEater {
  template <typename T>
  ExpressionEater(T&&) {}  // NOLINT
};
void Fold(std::initializer_list<ExpressionEater>) {}

template <int R>
constexpr size_t RoundUpTo(size_t n) {
  static_assert((R & (R - 1)) == 0, "Must be power of two");
  return (n + (R - 1)) & ~(R - 1);
}

constexpr size_t Max(size_t a, size_t b) { return a > b ? a : b; }
template <typename T, typename... Ts>
constexpr size_t Max(T a, Ts... b) {
  return Max(a, Max(b...));
}

template <typename T>
constexpr size_t EffectiveAlignof() {
  // `char` is special in that it gets aligned to 8. It is where we drop the
  // trivial structs.
  return std::is_same<T, char>::value ? 8 : alignof(T);
}

template <int align, typename U, typename... T>
using AppendIfAlign =
    typename std::conditional<EffectiveAlignof<U>() == align, void (*)(T..., U),
                              void (*)(T...)>::type;

// Metafunction to sort types in descending order of alignment.
// Useful for the flat allocator to ensure proper alignment of all elements
// without having to add padding.
// Instead of implementing a proper sort metafunction we just do a
// filter+merge, which is much simpler to write as a metafunction.
// We have a fixed set of alignments we can filter on.
// For simplicity we use a function pointer as a type list.
template <typename In, typename T16, typename T8, typename T4, typename T2,
          typename T1>
struct TypeListSortImpl;

template <typename... T16, typename... T8, typename... T4, typename... T2,
          typename... T1>
struct TypeListSortImpl<void (*)(), void (*)(T16...), void (*)(T8...),
                        void (*)(T4...), void (*)(T2...), void (*)(T1...)> {
  using type = void (*)(T16..., T8..., T4..., T2..., T1...);
};

template <typename First, typename... Rest, typename... T16, typename... T8,
          typename... T4, typename... T2, typename... T1>
struct TypeListSortImpl<void (*)(First, Rest...), void (*)(T16...),
                        void (*)(T8...), void (*)(T4...), void (*)(T2...),
                        void (*)(T1...)> {
  using type = typename TypeListSortImpl<
      void (*)(Rest...), AppendIfAlign<16, First, T16...>,
      AppendIfAlign<8, First, T8...>, AppendIfAlign<4, First, T4...>,
      AppendIfAlign<2, First, T2...>, AppendIfAlign<1, First, T1...>>::type;
};

template <typename... T>
using SortByAlignment =
    typename TypeListSortImpl<void (*)(T...), void (*)(), void (*)(),
                              void (*)(), void (*)(), void (*)()>::type;

template <template <typename...> class C, typename... T>
auto ApplyTypeList(void (*)(T...)) -> C<T...>;

template <typename T>
constexpr int FindTypeIndex() {
  return -1;
}

template <typename T, typename T1, typename... Ts>
constexpr int FindTypeIndex() {
  return std::is_same<T, T1>::value ? 0 : FindTypeIndex<T, Ts...>() + 1;
}

// A type to value map, where the possible keys as specified in `Keys...`.
// The values for key `K` is `ValueT<K>`
template <template <typename> class ValueT, typename... Keys>
class TypeMap {
 public:
  template <typename K>
  ValueT<K>& Get() {
    return static_cast<Base<K>&>(payload_).value;
  }

  template <typename K>
  const ValueT<K>& Get() const {
    return static_cast<const Base<K>&>(payload_).value;
  }

 private:
  template <typename K>
  struct Base {
    ValueT<K> value{};
  };
  struct Payload : Base<Keys>... {};
  Payload payload_;
};

template <typename T>
using IntT = int;
template <typename T>
using PointerT = T*;

// State that we gather during EstimatedMemoryUsed while on the lock, but we
// will use outside the lock.
struct EstimatedMemoryUsedState {
  // We can't call SpaceUsed under the lock because it uses reflection and can
  // potentially deadlock if it requires to load new files into the pool.
  // NOTE: Messages added here must not be modified or destroyed outside the
  // lock while the pool is alive.
  std::vector<const Message*> messages;
};

// Manages an allocation of sequential arrays of type `T...`.
// It is more space efficient than storing N (ptr, size) pairs, by storing only
// the pointer to the head and the boundaries between the arrays.
template <typename... T>
class FlatAllocation {
 public:
  static constexpr size_t kMaxAlign = Max(alignof(T)...);

  explicit FlatAllocation(const TypeMap<IntT, T...>& ends) : ends_(ends) {
    // The arrays start just after FlatAllocation, so adjust the ends.
    Fold({(ends_.template Get<T>() +=
           RoundUpTo<kMaxAlign>(sizeof(FlatAllocation)))...});
    Fold({Init<T>()...});
  }

  void Destroy() {
    Fold({Destroy<T>()...});
    internal::SizedDelete(this, total_bytes());
  }

  template <int I>
  using type = typename std::tuple_element<I, std::tuple<T...>>::type;

  // Gets a tuple of the head pointers for the arrays
  TypeMap<PointerT, T...> Pointers() const {
    TypeMap<PointerT, T...> out;
    Fold({(out.template Get<T>() = Begin<T>())...});
    return out;
  }


 private:
  // Total number of bytes used by all arrays.
  int total_bytes() const {
    // Get the last end.
    return ends_.template Get<typename std::tuple_element<
        sizeof...(T) - 1, std::tuple<T...>>::type>();
  }


  template <typename U>
  int BeginOffset() const {
    constexpr int type_index = FindTypeIndex<U, T...>();
    // Avoid a negative value here to keep it compiling when type_index == 0
    constexpr int prev_type_index = type_index == 0 ? 0 : type_index - 1;
    using PrevType =
        typename std::tuple_element<prev_type_index, std::tuple<T...>>::type;
    // Ensure the types are properly aligned.
    static_assert(EffectiveAlignof<PrevType>() >= EffectiveAlignof<U>(), "");
    return type_index == 0 ? RoundUpTo<kMaxAlign>(sizeof(FlatAllocation))
                           : ends_.template Get<PrevType>();
  }

  template <typename U>
  int EndOffset() const {
    return ends_.template Get<U>();
  }

  // Avoid the reinterpret_cast if the array is empty.
  // Clang's Control Flow Integrity does not like the cast pointing to memory
  // that is not yet initialized to be of that type.
  // (from -fsanitize=cfi-unrelated-cast)
  template <typename U>
  U* Begin() const {
    int begin = BeginOffset<U>(), end = EndOffset<U>();
    if (begin == end) return nullptr;
    return reinterpret_cast<U*>(data() + begin);
  }

  template <typename U>
  U* End() const {
    int begin = BeginOffset<U>(), end = EndOffset<U>();
    if (begin == end) return nullptr;
    return reinterpret_cast<U*>(data() + end);
  }

  template <typename U>
  bool Init() {
    // Skip for the `char` block. No need to zero initialize it.
    if (std::is_same<U, char>::value) return true;
    for (char *p = data() + BeginOffset<U>(), *end = data() + EndOffset<U>();
         p != end; p += sizeof(U)) {
      ::new (p) U{};
    }
    return true;
  }

  template <typename U>
  bool Destroy() {
    if (std::is_trivially_destructible<U>::value) return true;
    for (U *it = Begin<U>(), *end = End<U>(); it != end; ++it) {
      it->~U();
    }
    return true;
  }

  char* data() const {
    return const_cast<char*>(reinterpret_cast<const char*>(this));
  }

  TypeMap<IntT, T...> ends_;
};

template <typename... T>
TypeMap<IntT, T...> CalculateEnds(const TypeMap<IntT, T...>& sizes) {
  int total = 0;
  TypeMap<IntT, T...> out;
  Fold({(out.template Get<T>() = total +=
         sizeof(T) * sizes.template Get<T>())...});
  return out;
}

// The implementation for FlatAllocator below.
// This separate class template makes it easier to have methods that fold on
// `T...`.
template <typename... T>
class FlatAllocatorImpl {
 public:
  using Allocation = FlatAllocation<T...>;

  template <typename U>
  void PlanArray(int array_size) {
    // We can't call PlanArray after FinalizePlanning has been called.
    ABSL_CHECK(!has_allocated());
    if (std::is_trivially_destructible<U>::value) {
      // Trivial types are aligned to 8 bytes.
      static_assert(alignof(U) <= 8, "");
      total_.template Get<char>() += RoundUpTo<8>(array_size * sizeof(U));
    } else {
      // Since we can't use `if constexpr`, just make the expression compile
      // when this path is not taken.
      using TypeToUse =
          typename std::conditional<std::is_trivially_destructible<U>::value,
                                    char, U>::type;
      total_.template Get<TypeToUse>() += array_size;
    }
  }

  template <typename U>
  U* AllocateArray(int array_size) {
    constexpr bool trivial = std::is_trivially_destructible<U>::value;
    using TypeToUse = typename std::conditional<trivial, char, U>::type;

    // We can only allocate after FinalizePlanning has been called.
    ABSL_CHECK(has_allocated());

    TypeToUse*& data = pointers_.template Get<TypeToUse>();
    int& used = used_.template Get<TypeToUse>();
    U* res = reinterpret_cast<U*>(data + used);
    used += trivial ? RoundUpTo<8>(array_size * sizeof(U)) : array_size;
    ABSL_CHECK_LE(used, total_.template Get<TypeToUse>());
    return res;
  }

  // TODO: Remove the NULL terminators to save memory and simplify
  // the code.
  std::optional<internal::DescriptorNames> CreateDescriptorNames(
      std::initializer_list<absl::string_view> bytes,
      std::initializer_list<size_t> sizes) {
    for (size_t size : sizes) {
      // Name too long.
      if (size != static_cast<uint16_t>(size)) {
        return std::nullopt;
      }
    }

    size_t total_size = 0;
    for (auto b : bytes) total_size += b.size();
    total_size += sizes.size() * sizeof(uint16_t);
    char* out = AllocateArray<char>(total_size);
    for (absl::string_view b : bytes) {
      memcpy(out, b.data(), b.size());
      out += b.size();
    }
    auto res = internal::DescriptorNames(out);
    for (size_t size : sizes) {
      uint16_t size16 = static_cast<uint16_t>(size);
      memcpy(out, &size16, sizeof(size16));
      out += sizeof(size16);
    }
    return res;
  }

  void PlanEntityNames(size_t full_name_size) {
    PlanArray<char>(internal::DescriptorNames::AllocationSizeForSimpleNames(
        full_name_size));
  }
  std::optional<internal::DescriptorNames> AllocateEntityNames(
      absl::string_view scope, absl::string_view name) {
    static constexpr absl::string_view kNullChar("\0", 1);
    if (scope.empty()) {
      return CreateDescriptorNames({name, kNullChar},
                                   {name.size(), name.size()});
    } else {
      return CreateDescriptorNames(
          {scope, ".", name, kNullChar},
          {name.size(), scope.size() + 1 + name.size()});
    }
  }

  internal::DescriptorNames AllocatePlaceholderNames(
      absl::string_view full_name, size_t name_size) {
    static constexpr absl::string_view kNullChar("\0", 1);
    auto out = CreateDescriptorNames({full_name, kNullChar},
                                     {name_size, full_name.size()});
    if (out.has_value()) return *out;
    // Return any valid name for the caller to use and keep going.
    return AllocateEntityNames("", "unknown").value();
  }

  template <typename... In>
  const std::string* AllocateStrings(In&&... in) {
    std::string* strings = AllocateArray<std::string>(sizeof...(in));
    std::string* res = strings;
    Fold({(*strings++ = std::string(std::forward<In>(in)))...});
    return res;
  }

  absl::string_view AllocateStringView(const absl::string_view name) {
    char* res = AllocateArray<char>(name.size());
    memcpy(res, name.data(), name.size());
    return {res, name.size()};
  }

  // Allocate all 5 names of the field:
  // name, full name, lowercase, camelcase and json.
  // It will dedup the strings when following the naming style guide.
  void PlanFieldNames(size_t parent_scope_size, const absl::string_view name,
                      const std::string* opt_json_name) {
    ABSL_CHECK(!has_allocated());

    // name size, full_name size, lowercase (offset, size),
    // camelcase (offset, size), json (offset, size),
    constexpr int kIndexSize = 8 * sizeof(uint16_t);

    constexpr int kNullCharSize = 1;
    size_t total_bytes = kIndexSize + name.size() + kNullCharSize;
    if (parent_scope_size != 0) {
      total_bytes += parent_scope_size + /* '.' */ 1;
    }

    // Fast path for snake_case names, which follow the style guide.
    if (opt_json_name == nullptr) {
      switch (GetFieldNameCase(name)) {
        case FieldNameCase::kAllLower:
          // Case 1: they are all the same.
          return PlanArray<char>(total_bytes);
        case FieldNameCase::kSnakeCase: {
          // Case 2: name==lower, camel==json
          return PlanArray<char>(total_bytes + CamelCaseSize(name) +
                                 kNullCharSize);
        }
        default:
          break;
      }
    }

    // lowercase
    total_bytes += name.size() + kNullCharSize;
    // camelcase
    total_bytes += CamelCaseSize(name) + kNullCharSize;
    // json_name
    total_bytes += (opt_json_name != nullptr ? opt_json_name->size()
                                             : JsonNameSize(name)) +
                   kNullCharSize;
    PlanArray<char>(total_bytes);
  }

  std::optional<internal::DescriptorNames> AllocateFieldNames(
      const absl::string_view name, const absl::string_view scope,
      const std::string* opt_json_name) {
    ABSL_CHECK(has_allocated());

    const absl::string_view scope_dot = scope.empty() ? "" : ".";
    const size_t full_name_size = scope.size() + scope_dot.size() + name.size();

    static constexpr absl::string_view kNullChar("\0", 1);

    if (opt_json_name == nullptr) {
      switch (GetFieldNameCase(name)) {
        case FieldNameCase::kAllLower:
          PROTOBUF_DEBUG_COUNTER("AllocateFieldNames.AllLower").Inc();
          // Case 1: they are all the same.
          return CreateDescriptorNames(
              {scope, scope_dot, name, kNullChar},
              {name.size(), full_name_size,
               /* lowercase */ name.size() + 1, name.size(),
               /* camelcase */ name.size() + 1, name.size(),
               /* json */ name.size() + 1, name.size()});
        case FieldNameCase::kSnakeCase: {
          PROTOBUF_DEBUG_COUNTER("AllocateFieldNames.SnakeCase").Inc();
          // Case 2: name==lower, camel==json
          std::string camelcase_name =
              ToCamelCase(name, /* lower_first = */ true);
          const size_t camelcase_offset =
              full_name_size + camelcase_name.size();
          return CreateDescriptorNames(
              {camelcase_name, kNullChar, scope, scope_dot, name, kNullChar},
              {name.size(), full_name_size,
               /* lowercase */ name.size() + 1, name.size(),
               /* camelcase */ camelcase_offset + 2, camelcase_name.size(),
               /* json */ camelcase_offset + 2, camelcase_name.size()});
        }
        default:
          break;
      }
    }

    PROTOBUF_DEBUG_COUNTER("AllocateFieldNames.Fallback").Inc();
    std::string lowercase_name = std::string(name);
    absl::AsciiStrToLower(&lowercase_name);
    const std::string camelcase_name =
        ToCamelCase(name, /* lower_first = */ true);
    const std::string json_name =
        opt_json_name != nullptr ? *opt_json_name : ToJsonName(name);

    size_t offset = full_name_size + 1;
    return CreateDescriptorNames(
        {json_name, kNullChar, camelcase_name, kNullChar,  //
         lowercase_name, kNullChar, scope, scope_dot, name, kNullChar},
        {name.size(), full_name_size,  //
         offset += lowercase_name.size() + 1, lowercase_name.size(),
         offset += camelcase_name.size() + 1, camelcase_name.size(),
         offset += json_name.size() + 1, json_name.size()});
  }

  template <typename Alloc>
  void FinalizePlanning(Alloc& alloc) {
    ABSL_CHECK(!has_allocated());

    pointers_ = alloc->CreateFlatAlloc(total_)->Pointers();

    ABSL_CHECK(has_allocated());
  }

  void ExpectConsumed() const {
    // We verify that we consumed all the memory requested if there was no
    // error in processing.
    Fold({ExpectConsumed<T>()...});
  }

 private:
  bool has_allocated() const {
    return pointers_.template Get<char>() != nullptr;
  }

  enum class FieldNameCase { kAllLower, kSnakeCase, kOther };
  FieldNameCase GetFieldNameCase(const absl::string_view name) {
    if (!name.empty() && !absl::ascii_islower(name[0])) {
      return FieldNameCase::kOther;
    }
    FieldNameCase best = FieldNameCase::kAllLower;
    for (char c : name) {
      if (absl::ascii_isupper(c)) {
        return FieldNameCase::kOther;
      } else if (c == '_') {
        best = FieldNameCase::kSnakeCase;
      }
    }
    return best;
  }

  template <typename U>
  bool ExpectConsumed() const {
    ABSL_CHECK_EQ(total_.template Get<U>(), used_.template Get<U>());
    return true;
  }

  TypeMap<PointerT, T...> pointers_;
  TypeMap<IntT, T...> total_;
  TypeMap<IntT, T...> used_;
};

static auto DisableTracking() {
  bool old_value = internal::cpp::IsTrackingEnabled();
  internal::cpp::IsTrackingEnabledVar() = false;
  return absl::MakeCleanup(
      [=] { internal::cpp::IsTrackingEnabledVar() = old_value; });
}

}  // namespace

class Symbol {
 public:
  enum Type {
    NULL_SYMBOL,
    MESSAGE,
    FIELD,
    ONEOF,
    ENUM,
    ENUM_VALUE,
    ENUM_VALUE_OTHER_PARENT,
    SERVICE,
    METHOD,
    FULL_PACKAGE,
    SUB_PACKAGE,
  };

  Symbol() {
    static constexpr internal::SymbolBase null_symbol{};
    static_assert(null_symbol.symbol_type_ == NULL_SYMBOL, "");
    // Initialize with a sentinel to make sure `ptr_` is never null.
    ptr_ = &null_symbol;
  }

  // Every object we store derives from internal::SymbolBase, where we store the
  // symbol type enum.
  // Storing in the object can be done without using more space in most cases,
  // while storing it in the Symbol type would require 8 bytes.
#define DEFINE_MEMBERS(TYPE, TYPE_CONSTANT, FIELD)                             \
  explicit Symbol(TYPE* value) : ptr_(value) {                                 \
    value->symbol_type_ = TYPE_CONSTANT;                                       \
  }                                                                            \
  const TYPE* FIELD() const {                                                  \
    return type() == TYPE_CONSTANT ? static_cast<const TYPE*>(ptr_) : nullptr; \
  }

  DEFINE_MEMBERS(Descriptor, MESSAGE, descriptor)
  DEFINE_MEMBERS(FieldDescriptor, FIELD, field_descriptor)
  DEFINE_MEMBERS(OneofDescriptor, ONEOF, oneof_descriptor)
  DEFINE_MEMBERS(EnumDescriptor, ENUM, enum_descriptor)
  DEFINE_MEMBERS(ServiceDescriptor, SERVICE, service_descriptor)
  DEFINE_MEMBERS(MethodDescriptor, METHOD, method_descriptor)
  DEFINE_MEMBERS(FileDescriptor, FULL_PACKAGE, file_descriptor)

  // We use a special node for subpackage FileDescriptor.
  // It is potentially added to the table with multiple different names, so we
  // need a separate place to put the name.
  struct Subpackage : internal::SymbolBase {
    int name_size;
    const FileDescriptor* file;
  };
  DEFINE_MEMBERS(Subpackage, SUB_PACKAGE, sub_package_file_descriptor)

  // Enum values have two different parents.
  // We use two different identitied for the same object to determine the two
  // different insertions in the map.
  static Symbol EnumValue(EnumValueDescriptor* value, int n) {
    Symbol s;
    internal::SymbolBase* ptr;
    if (n == 0) {
      ptr = static_cast<internal::SymbolBaseN<0>*>(value);
      ptr->symbol_type_ = ENUM_VALUE;
    } else {
      ptr = static_cast<internal::SymbolBaseN<1>*>(value);
      ptr->symbol_type_ = ENUM_VALUE_OTHER_PARENT;
    }
    s.ptr_ = ptr;
    return s;
  }

  const EnumValueDescriptor* enum_value_descriptor() const {
    return type() == ENUM_VALUE
               ? static_cast<const EnumValueDescriptor*>(
                     static_cast<const internal::SymbolBaseN<0>*>(ptr_))
           : type() == ENUM_VALUE_OTHER_PARENT
               ? static_cast<const EnumValueDescriptor*>(
                     static_cast<const internal::SymbolBaseN<1>*>(ptr_))
               : nullptr;
  }

#undef DEFINE_MEMBERS

  Type type() const { return static_cast<Type>(ptr_->symbol_type_); }
  bool IsNull() const { return type() == NULL_SYMBOL; }
  bool IsType() const { return type() == MESSAGE || type() == ENUM; }
  bool IsAggregate() const {
    return IsType() || IsPackage() || type() == SERVICE;
  }
  bool IsPackage() const {
    return type() == FULL_PACKAGE || type() == SUB_PACKAGE;
  }

  const FileDescriptor* GetFile() const {
    switch (type()) {
      case MESSAGE:
        return descriptor()->file();
      case FIELD:
        return field_descriptor()->file();
      case ONEOF:
        return oneof_descriptor()->containing_type()->file();
      case ENUM:
        return enum_descriptor()->file();
      case ENUM_VALUE:
        return enum_value_descriptor()->type()->file();
      case SERVICE:
        return service_descriptor()->file();
      case METHOD:
        return method_descriptor()->service()->file();
      case FULL_PACKAGE:
        return file_descriptor();
      case SUB_PACKAGE:
        return sub_package_file_descriptor()->file;
      default:
        return nullptr;
    }
  }

  absl::string_view full_name() const {
    switch (type()) {
      case MESSAGE:
        return descriptor()->full_name();
      case FIELD:
        return field_descriptor()->full_name();
      case ONEOF:
        return oneof_descriptor()->full_name();
      case ENUM:
        return enum_descriptor()->full_name();
      case ENUM_VALUE:
        return enum_value_descriptor()->full_name();
      case SERVICE:
        return service_descriptor()->full_name();
      case METHOD:
        return method_descriptor()->full_name();
      case FULL_PACKAGE:
        return file_descriptor()->package();
      case SUB_PACKAGE:
        return sub_package_file_descriptor()->file->package().substr(
            0, sub_package_file_descriptor()->name_size);
      default:
        ABSL_CHECK(false);
    }
    return "";
  }

  std::pair<const void*, absl::string_view> parent_name_key() const {
    const auto or_file = [&](const void* p) { return p ? p : GetFile(); };
    switch (type()) {
      case MESSAGE:
        return {or_file(descriptor()->containing_type()), descriptor()->name()};
      case FIELD: {
        auto* field = field_descriptor();
        return {or_file(field->is_extension() ? field->extension_scope()
                                              : field->containing_type()),
                field->name()};
      }
      case ONEOF:
        return {oneof_descriptor()->containing_type(),
                oneof_descriptor()->name()};
      case ENUM:
        return {or_file(enum_descriptor()->containing_type()),
                enum_descriptor()->name()};
      case ENUM_VALUE:
        return {or_file(enum_value_descriptor()->type()->containing_type()),
                enum_value_descriptor()->name()};
      case ENUM_VALUE_OTHER_PARENT:
        return {enum_value_descriptor()->type(),
                enum_value_descriptor()->name()};
      case SERVICE:
        return {GetFile(), service_descriptor()->name()};
      case METHOD:
        return {method_descriptor()->service(), method_descriptor()->name()};
      default:
        ABSL_CHECK(false);
    }
    return {};
  }

  const FeatureSet& features() const {
    switch (type()) {
      case MESSAGE:
        return descriptor()->features();
      case FIELD:
        return field_descriptor()->features();
      case ONEOF:
        return oneof_descriptor()->features();
      case ENUM:
        return enum_descriptor()->features();
      case ENUM_VALUE:
        return enum_value_descriptor()->features();
      case SERVICE:
        return service_descriptor()->features();
      case METHOD:
        return method_descriptor()->features();
      case FULL_PACKAGE:
        return file_descriptor()->features();
      case SUB_PACKAGE:
      default:
        internal::Unreachable();
    }
  }

  bool is_placeholder() const {
    switch (type()) {
      case MESSAGE:
        return descriptor()->is_placeholder();
      case ENUM:
        return enum_descriptor()->is_placeholder();
      case FULL_PACKAGE:
        return file_descriptor()->is_placeholder();
      default:
        return false;
    }
  }

  SymbolVisibility visibility_keyword() const {
    switch (type()) {
      case MESSAGE:
        return descriptor()->visibility_keyword();
      case ENUM:
        return enum_descriptor()->visibility_keyword();
      default:
        return SymbolVisibility::VISIBILITY_UNSET;
    }
  }

  bool IsNestedDefinition() const {
    switch (type()) {
      case MESSAGE:
        return descriptor()->containing_type() != nullptr;
      case ENUM:
        return enum_descriptor()->containing_type() != nullptr;
      case FIELD:  // For extension fields
        return field_descriptor()->containing_type() != nullptr;
      default:
        return false;
    }
  }

  SymbolVisibility GetEffectiveVisibility() const {
    // Only Types have visibility
    if (!IsType()) {
      return SymbolVisibility::VISIBILITY_UNSET;
    }

    SymbolVisibility effective_visibility = visibility_keyword();

    // If our visibility is specifically set we can return that.  We'll validate
    // whether it's reasonable or not later.
    if (effective_visibility == SymbolVisibility::VISIBILITY_UNSET) {
      switch (features().default_symbol_visibility()) {
        case FeatureSet::VisibilityFeature::EXPORT_ALL:
          return SymbolVisibility::VISIBILITY_EXPORT;
        case FeatureSet::VisibilityFeature::EXPORT_TOP_LEVEL:
          return IsNestedDefinition() ? SymbolVisibility::VISIBILITY_LOCAL
                                      : SymbolVisibility::VISIBILITY_EXPORT;
        case FeatureSet::VisibilityFeature::LOCAL_ALL:
        case FeatureSet::VisibilityFeature::STRICT:
          return SymbolVisibility::VISIBILITY_LOCAL;

        // Unset shouldn't be possible from the compiler without there being an
        // error (recursive import, for example), but happens in unit
        // tests so assume it represents pre-edition 2024 defaults. In either
        // case we want to fail open.  We have a DCHECK here to make sure it can
        // fail in tests, but not released code.
        case FeatureSet::VisibilityFeature::DEFAULT_SYMBOL_VISIBILITY_UNKNOWN:
        default:
          ABSL_DCHECK(false);
          return SymbolVisibility::VISIBILITY_EXPORT;
      }
    }

    return effective_visibility;
  }

  /*
   * Calculate whether this symbol can be accessed from the given
   * FileDescriptor*.
   *
   * Returns true if the symbol is in the same file OR the symbol is `export`
   */
  bool IsVisibleFrom(FileDescriptor* other) const {
    if (GetFile() == nullptr || other == nullptr) {
      return false;
    }

    // Only Types (message/enum) have visibility.
    if (!IsType()) {
      return true;
    }

    // If we're dealing with a placeholder then just stop now, visibility can't
    // be determined and we have to rely on the proto-compiler previously having
    // checked the validity.
    if (is_placeholder()) {
      return true;
    }

    if (GetFile() == other) {
      return true;
    }

    SymbolVisibility effective_visibility = GetEffectiveVisibility();

    return effective_visibility == SymbolVisibility::VISIBILITY_EXPORT;
  }

  std::string GetVisibilityError(FileDescriptor* other,
                                 absl::string_view usage = "") const {
    const absl::string_view file_path =
        GetFile() != nullptr ? GetFile()->name() : "unknown_file";
    const absl::string_view symbol_name = full_name();

    if (!IsType()) {
      return absl::StrCat(
          "Attempt to get a visibility error for a non-message/enum symbol ",
          symbol_name, "\", defined in \"", file_path);
    }

    SymbolVisibility explicit_visibility = visibility_keyword();

    std::string reason =
        explicit_visibility == SymbolVisibility::VISIBILITY_LOCAL
            ? "It is explicitly marked 'local'"
            : absl::StrCat(
                  "It defaulted to local from file-level 'option "
                  "features.default_symbol_visibility = '",
                  FeatureSet_VisibilityFeature_DefaultSymbolVisibility_Name(
                      features().default_symbol_visibility()),
                  "';");

    return absl::StrCat("Symbol \"", symbol_name, "\", defined in \"",
                        file_path, "\" ", usage,
                        " is "
                        "not visible from \"",
                        other->name(), "\". ", reason,
                        " and cannot be accessed outside its own file");
  }

 private:
  const internal::SymbolBase* ptr_;
};

const FieldDescriptor::CppType
    FieldDescriptor::kTypeToCppTypeMap[MAX_TYPE + 1] = {
        static_cast<CppType>(0),  // 0 is reserved for errors

        CPPTYPE_DOUBLE,   // TYPE_DOUBLE
        CPPTYPE_FLOAT,    // TYPE_FLOAT
        CPPTYPE_INT64,    // TYPE_INT64
        CPPTYPE_UINT64,   // TYPE_UINT64
        CPPTYPE_INT32,    // TYPE_INT32
        CPPTYPE_UINT64,   // TYPE_FIXED64
        CPPTYPE_UINT32,   // TYPE_FIXED32
        CPPTYPE_BOOL,     // TYPE_BOOL
        CPPTYPE_STRING,   // TYPE_STRING
        CPPTYPE_MESSAGE,  // TYPE_GROUP
        CPPTYPE_MESSAGE,  // TYPE_MESSAGE
        CPPTYPE_STRING,   // TYPE_BYTES
        CPPTYPE_UINT32,   // TYPE_UINT32
        CPPTYPE_ENUM,     // TYPE_ENUM
        CPPTYPE_INT32,    // TYPE_SFIXED32
        CPPTYPE_INT64,    // TYPE_SFIXED64
        CPPTYPE_INT32,    // TYPE_SINT32
        CPPTYPE_INT64,    // TYPE_SINT64
};

const char* const FieldDescriptor::kTypeToName[MAX_TYPE + 1] = {
    "ERROR",  // 0 is reserved for errors

    "double",    // TYPE_DOUBLE
    "float",     // TYPE_FLOAT
    "int64",     // TYPE_INT64
    "uint64",    // TYPE_UINT64
    "int32",     // TYPE_INT32
    "fixed64",   // TYPE_FIXED64
    "fixed32",   // TYPE_FIXED32
    "bool",      // TYPE_BOOL
    "string",    // TYPE_STRING
    "group",     // TYPE_GROUP
    "message",   // TYPE_MESSAGE
    "bytes",     // TYPE_BYTES
    "uint32",    // TYPE_UINT32
    "enum",      // TYPE_ENUM
    "sfixed32",  // TYPE_SFIXED32
    "sfixed64",  // TYPE_SFIXED64
    "sint32",    // TYPE_SINT32
    "sint64",    // TYPE_SINT64
};

const char* const FieldDescriptor::kCppTypeToName[MAX_CPPTYPE + 1] = {
    "ERROR",  // 0 is reserved for errors

    "int32",    // CPPTYPE_INT32
    "int64",    // CPPTYPE_INT64
    "uint32",   // CPPTYPE_UINT32
    "uint64",   // CPPTYPE_UINT64
    "double",   // CPPTYPE_DOUBLE
    "float",    // CPPTYPE_FLOAT
    "bool",     // CPPTYPE_BOOL
    "enum",     // CPPTYPE_ENUM
    "string",   // CPPTYPE_STRING
    "message",  // CPPTYPE_MESSAGE
};

const char* const FieldDescriptor::kLabelToName[MAX_LABEL + 1] = {
    "ERROR",  // 0 is reserved for errors

    "optional",  // LABEL_OPTIONAL
    "required",  // LABEL_REQUIRED
    "repeated",  // LABEL_REPEATED
};

static const char* const kNonLinkedWeakMessageReplacementName = "google.protobuf.Empty";

#if !defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912)
const int FieldDescriptor::kMaxNumber;
const int FieldDescriptor::kFirstReservedNumber;
const int FieldDescriptor::kLastReservedNumber;
#endif

namespace {

std::string EnumValueToPascalCase(const std::string& input) {
  bool next_upper = true;
  std::string result;
  result.reserve(input.size());

  for (char character : input) {
    if (character == '_') {
      next_upper = true;
    } else {
      if (next_upper) {
        result.push_back(absl::ascii_toupper(character));
      } else {
        result.push_back(absl::ascii_tolower(character));
      }
      next_upper = false;
    }
  }

  return result;
}

// Class to remove an enum prefix from enum values.
class PrefixRemover {
 public:
  explicit PrefixRemover(absl::string_view prefix) {
    // Strip underscores and lower-case the prefix.
    for (char character : prefix) {
      if (character != '_') {
        prefix_ += absl::ascii_tolower(character);
      }
    }
  }

  // Tries to remove the enum prefix from this enum value.
  // If this is not possible, returns the input verbatim.
  std::string MaybeRemove(absl::string_view str) {
    // We can't just lowercase and strip str and look for a prefix.
    // We need to properly recognize the difference between:
    //
    //   enum Foo {
    //     FOO_BAR_BAZ = 0;
    //     FOO_BARBAZ = 1;
    //   }
    //
    // This is acceptable (though perhaps not advisable) because even when
    // we PascalCase, these two will still be distinct (BarBaz vs. Barbaz).
    size_t i, j;

    // Skip past prefix_ in str if we can.
    for (i = 0, j = 0; i < str.size() && j < prefix_.size(); i++) {
      if (str[i] == '_') {
        continue;
      }

      if (absl::ascii_tolower(str[i]) != prefix_[j++]) {
        return std::string(str);
      }
    }

    // If we didn't make it through the prefix, we've failed to strip the
    // prefix.
    if (j < prefix_.size()) {
      return std::string(str);
    }

    // Skip underscores between prefix and further characters.
    while (i < str.size() && str[i] == '_') {
      i++;
    }

    // Enum label can't be the empty string.
    if (i == str.size()) {
      return std::string(str);
    }

    // We successfully stripped the prefix.
    str.remove_prefix(i);
    return std::string(str);
  }

 private:
  std::string prefix_;
};

// A DescriptorPool contains a bunch of hash-maps to implement the
// various Find*By*() methods.  Since hashtable lookups are O(1), it's
// most efficient to construct a fixed set of large hash-maps used by
// all objects in the pool rather than construct one or more small
// hash-maps for each object.
//
// The keys to these hash-maps are (parent, name) or (parent, number) pairs.
struct FullNameQuery {
  absl::string_view query;
  absl::string_view full_name() const { return query; }
};
struct SymbolByFullNameHash {
  using is_transparent = void;

  template <typename T>
  size_t operator()(const T& s) const {
    return absl::HashOf(s.full_name());
  }
};
struct SymbolByFullNameEq {
  using is_transparent = void;

  template <typename T, typename U>
  bool operator()(const T& a, const U& b) const {
    return a.full_name() == b.full_name();
  }
};
using SymbolsByNameSet =
    absl::flat_hash_set<Symbol, SymbolByFullNameHash, SymbolByFullNameEq>;

struct ParentNameQueryBase {
  std::pair<const void*, absl::string_view> query;
  std::pair<const void*, absl::string_view> parent_name_key() const {
    return query;
  }
};
struct ParentNameQuery : public ParentNameQueryBase {
  using SymbolT = Symbol;

  template <typename It>
  static SymbolT IterToSymbol(It it) {
    return *it;
  }
};
struct ParentNameFieldQuery : public ParentNameQueryBase {
  using SymbolT = const FieldDescriptor*;

  template <typename It>
  static SymbolT IterToSymbol(It it) {
    SymbolT field = it->field_descriptor();
    ABSL_ASSUME(field != nullptr);
    return field;
  }
};
struct SymbolByParentHash {
  using is_transparent = void;

  template <typename T>
  size_t operator()(const T& s) const {
    return absl::HashOf(s.parent_name_key());
  }
};
struct SymbolByParentEq {
  using is_transparent = void;

  bool operator()(const Symbol& symbol,
                  const ParentNameFieldQuery& query) const {
    const FieldDescriptor* field = symbol.field_descriptor();
    return field != nullptr && !field->is_extension() &&
           field->containing_type() == query.query.first &&
           field->name() == query.query.second;
  }

  template <typename T, typename U>
  bool operator()(const T& a, const U& b) const {
    return a.parent_name_key() == b.parent_name_key();
  }
};
using SymbolsByParentSet =
    absl::flat_hash_set<Symbol, SymbolByParentHash, SymbolByParentEq>;

template <typename DescriptorT>
struct DescriptorsByNameHash {
  using is_transparent = void;

  size_t operator()(absl::string_view name) const { return absl::HashOf(name); }

  size_t operator()(const DescriptorT* file) const {
    return absl::HashOf(file->name());
  }
};

template <typename DescriptorT>
struct DescriptorsByNameEq {
  using is_transparent = void;

  bool operator()(absl::string_view lhs, absl::string_view rhs) const {
    return lhs == rhs;
  }
  bool operator()(absl::string_view lhs, const DescriptorT* rhs) const {
    return lhs == rhs->name();
  }
  bool operator()(const DescriptorT* lhs, absl::string_view rhs) const {
    return lhs->name() == rhs;
  }
  bool operator()(const DescriptorT* lhs, const DescriptorT* rhs) const {
    return lhs == rhs || lhs->name() == rhs->name();
  }
};

template <typename DescriptorT>
using DescriptorsByNameSet =
    absl::flat_hash_set<const DescriptorT*, DescriptorsByNameHash<DescriptorT>,
                        DescriptorsByNameEq<DescriptorT>>;

using FieldsByNameMap =
    absl::flat_hash_map<std::pair<const void*, absl::string_view>,
                        const FieldDescriptor*>;

struct ParentNumberQuery {
  std::pair<const void*, int> query;
};
std::pair<const void*, int> ObjectToParentNumber(const FieldDescriptor* field) {
  return {field->containing_type(), field->number()};
}
std::pair<const void*, int> ObjectToParentNumber(
    const EnumValueDescriptor* enum_value) {
  return {enum_value->type(), enum_value->number()};
}
std::pair<const void*, int> ObjectToParentNumber(ParentNumberQuery query) {
  return query.query;
}
struct ParentNumberHash {
  using is_transparent = void;

  template <typename T>
  size_t operator()(const T& t) const {
    return absl::HashOf(ObjectToParentNumber(t));
  }
};
struct ParentNumberEq {
  using is_transparent = void;

  template <typename T, typename U>
  bool operator()(const T& a, const U& b) const {
    return ObjectToParentNumber(a) == ObjectToParentNumber(b);
  }
};
using FieldsByNumberSet = absl::flat_hash_set<const FieldDescriptor*,
                                              ParentNumberHash, ParentNumberEq>;
using EnumValuesByNumberSet =
    absl::flat_hash_set<const EnumValueDescriptor*, ParentNumberHash,
                        ParentNumberEq>;

// This is a map rather than a hash-map, since we use it to iterate
// through all the extensions that extend a given Descriptor, and an
// ordered data structure that implements lower_bound is convenient
// for that.
using ExtensionsGroupedByDescriptorMap =
    absl::btree_map<std::pair<const Descriptor*, int>, const FieldDescriptor*>;
using LocationsByPathMap =
    absl::flat_hash_map<std::string, const SourceCodeInfo_Location*>;

absl::flat_hash_set<std::string>* AllowedCustomOptionExtendees() {
  const char* kOptionNames[] = {
      "FileOptions",   "MessageOptions",   "FieldOptions",
      "EnumOptions",   "EnumValueOptions", "ServiceOptions",
      "MethodOptions", "OneofOptions",     "ExtensionRangeOptions"};
  auto allowed_proto3_extendees = new absl::flat_hash_set<std::string>();
  allowed_proto3_extendees->reserve(sizeof(kOptionNames) /
                                    sizeof(kOptionNames[0]));

  for (const char* option_name : kOptionNames) {
    // descriptor.proto has a different package name in opensource. We allow
    // both so the opensource protocol compiler can also compile internal
    // proto3 files with custom options. See: b/27567912
    allowed_proto3_extendees->insert(std::string("google.protobuf.") +
                                     option_name);
    // Split the word to trick the opensource processing scripts so they
    // will keep the original package name.
    allowed_proto3_extendees->insert(std::string("proto2.") + option_name);
  }
  return allowed_proto3_extendees;
}

// Checks whether field is an extension for a descriptor option. Use name
// comparison instead of comparing the descriptor directly because the
// extensions may be defined in a different pool.
bool IsCustomOptionExtension(const FieldDescriptor* desc) {
  if (!desc->is_extension()) {
    return false;
  }
  static auto custom_option_extendees =
      internal::OnShutdownDelete(AllowedCustomOptionExtendees());
  return custom_option_extendees->find(desc->containing_type()->full_name()) !=
         custom_option_extendees->end();
}

template <typename ProtoT>
void RestoreFeaturesToOptions(const FeatureSet* features, ProtoT* proto) {
  if (features != &FeatureSet::default_instance()) {
    *proto->mutable_options()->mutable_features() = *features;
  }
}

template <typename DescriptorT>
absl::string_view GetFullName(const DescriptorT& desc) {
  return desc.full_name();
}

absl::string_view GetFullName(const FileDescriptor& desc) {
  return desc.name();
}

template <typename DescriptorT>
const FileDescriptor* GetFile(const DescriptorT& desc) {
  return desc.file();
}

const FileDescriptor* GetFile(const FileDescriptor& desc) { return &desc; }

const FeatureSet& GetParentFeatures(const FileDescriptor* file) {
  return FeatureSet::default_instance();
}

const FeatureSet& GetParentFeatures(const Descriptor* message) {
  if (message->containing_type() == nullptr) {
    return internal::InternalFeatureHelper::GetFeatures(*message->file());
  }
  return internal::InternalFeatureHelper::GetFeatures(
      *message->containing_type());
}

const FeatureSet& GetParentFeatures(const OneofDescriptor* oneof) {
  return internal::InternalFeatureHelper::GetFeatures(
      *oneof->containing_type());
}

const FeatureSet& GetParentFeatures(const Descriptor::ExtensionRange* range) {
  return internal::InternalFeatureHelper::GetFeatures(
      *range->containing_type());
}

const FeatureSet& GetParentFeatures(const FieldDescriptor* field) {
  if (field->containing_oneof() != nullptr) {
    return internal::InternalFeatureHelper::GetFeatures(
        *field->containing_oneof());
  } else if (field->is_extension()) {
    if (field->extension_scope() == nullptr) {
      return internal::InternalFeatureHelper::GetFeatures(*field->file());
    }
    return internal::InternalFeatureHelper::GetFeatures(
        *field->extension_scope());
  }
  return internal::InternalFeatureHelper::GetFeatures(
      *field->containing_type());
}

const FeatureSet& GetParentFeatures(const EnumDescriptor* enm) {
  if (enm->containing_type() == nullptr) {
    return internal::InternalFeatureHelper::GetFeatures(*enm->file());
  }
  return internal::InternalFeatureHelper::GetFeatures(*enm->containing_type());
}

const FeatureSet& GetParentFeatures(const EnumValueDescriptor* value) {
  return internal::InternalFeatureHelper::GetFeatures(*value->type());
}

const FeatureSet& GetParentFeatures(const ServiceDescriptor* service) {
  return internal::InternalFeatureHelper::GetFeatures(*service->file());
}

const FeatureSet& GetParentFeatures(const MethodDescriptor* method) {
  return internal::InternalFeatureHelper::GetFeatures(*method->service());
}

bool IsLegacyEdition(Edition edition) {
  return edition < Edition::EDITION_2023;
}

}  // anonymous namespace

// Contains tables specific to a particular file.  These tables are not
// modified once the file has been constructed, so they need not be
// protected by a mutex.  This makes operations that depend only on the
// contents of a single file -- e.g. Descriptor::FindFieldByName() --
// lock-free.
//
// For historical reasons, the definitions of the methods of
// FileDescriptorTables and DescriptorPool::Tables are interleaved below.
// These used to be a single class.
class FileDescriptorTables {
 public:
  FileDescriptorTables();
  ~FileDescriptorTables();

  // Empty table, used with placeholder files.
  inline static const FileDescriptorTables& GetEmptyInstance();

  // -----------------------------------------------------------------
  // Finding items.

  // Returns a null Symbol (symbol.IsNull() is true) if not found.
  // TODO: All callers to this function know the type they are looking
  // for. If we propagate that information statically we can make the query
  // faster.
  template <typename K = ParentNameQuery>
  inline auto FindNestedSymbol(const void* parent,
                               absl::string_view name) const;

  // These return nullptr if not found.
  inline const FieldDescriptor* FindFieldByNumber(const Descriptor* parent,
                                                  int number) const;
  inline const FieldDescriptor* FindFieldByLowercaseName(
      const void* parent, absl::string_view lowercase_name) const;
  inline const FieldDescriptor* FindFieldByCamelcaseName(
      const void* parent, absl::string_view camelcase_name) const;
  inline const EnumValueDescriptor* FindEnumValueByNumber(
      const EnumDescriptor* parent, int number) const;
  // This creates a new EnumValueDescriptor if not found, in a thread-safe way.
  inline const EnumValueDescriptor* FindEnumValueByNumberCreatingIfUnknown(
      const EnumDescriptor* parent, int number) const;

  // -----------------------------------------------------------------
  // Adding items.

  // These add items to the corresponding tables.  They return false if
  // the key already exists in the table.
  bool AddAliasUnderParent(const void* parent, absl::string_view name,
                           Symbol symbol);
  bool AddFieldByNumber(FieldDescriptor* field);
  bool AddEnumValueByNumber(EnumValueDescriptor* value);

  // Populates p->first->locations_by_path_ from p->second.
  // Unusual signature dictated by absl::call_once.
  static void BuildLocationsByPath(
      std::pair<const FileDescriptorTables*, const SourceCodeInfo*>* p);

  // Returns the location denoted by the specified path through info,
  // or nullptr if not found.
  // The value of info must be that of the corresponding FileDescriptor.
  // (Conceptually a pure function, but stateful as an optimisation.)
  const SourceCodeInfo_Location* GetSourceLocation(
      const std::vector<int>& path, const SourceCodeInfo* info) const;

  // Must be called after BuildFileImpl(), even if the build failed and
  // we are going to roll back to the last checkpoint.
  void FinalizeTables();

 private:
  const void* FindParentForFieldsByMap(const FieldDescriptor* field) const;
  static void FieldsByLowercaseNamesLazyInitStatic(
      const FileDescriptorTables* tables);
  void FieldsByLowercaseNamesLazyInitInternal() const;
  static void FieldsByCamelcaseNamesLazyInitStatic(
      const FileDescriptorTables* tables);
  void FieldsByCamelcaseNamesLazyInitInternal() const;

  SymbolsByParentSet symbols_by_parent_;
  mutable absl::once_flag fields_by_lowercase_name_once_;
  mutable absl::once_flag fields_by_camelcase_name_once_;
  // Make these fields atomic to avoid race conditions with
  // GetEstimatedOwnedMemoryBytesSize. Once the pointer is set the map won't
  // change anymore.
  mutable std::atomic<const FieldsByNameMap*> fields_by_lowercase_name_{};
  mutable std::atomic<const FieldsByNameMap*> fields_by_camelcase_name_{};
  FieldsByNumberSet fields_by_number_;  // Not including extensions.
  EnumValuesByNumberSet enum_values_by_number_;
  mutable EnumValuesByNumberSet unknown_enum_values_by_number_
      ABSL_GUARDED_BY(unknown_enum_values_mu_);

  // Populated on first request to save space, hence constness games.
  mutable absl::once_flag locations_by_path_once_;
  mutable LocationsByPathMap locations_by_path_;

  // Mutex to protect the unknown-enum-value map due to dynamic
  // EnumValueDescriptor creation on unknown values.
  mutable absl::Mutex unknown_enum_values_mu_;
};

namespace internal {

// Small sequential allocator to be used within a single file.
// Most of the memory for a single FileDescriptor and everything under it is
// allocated in a single block of memory, with the FlatAllocator giving it out
// in parts later.
// The code first plans the total number of bytes needed by calling PlanArray
// with all the allocations that will happen afterwards, then calls
// FinalizePlanning passing the underlying allocator (the DescriptorPool::Tables
// instance), and then proceeds to get the memory via
// `AllocateArray`/`AllocateString` calls. The calls to PlanArray and
// The calls have to match between planning and allocating, though not
// necessarily in the same order.
class FlatAllocator
    : public decltype(ApplyTypeList<FlatAllocatorImpl>(
          SortByAlignment<char, std::string, SourceCodeInfo,
                          FileDescriptorTables, FeatureSet,
                          // Option types
                          MessageOptions, FieldOptions, EnumOptions,
                          EnumValueOptions, ExtensionRangeOptions, OneofOptions,
                          ServiceOptions, MethodOptions, FileOptions>())) {};

}  // namespace internal

// ===================================================================
// DescriptorPool::DeferredValidation

// This class stores information required to defer validation until we're
// outside the mutex lock.  These are reflective checks that also require us to
// acquire the lock.
class DescriptorPool::DeferredValidation {
 public:
  DeferredValidation(const DescriptorPool* pool,
                     ErrorCollector* error_collector)
      : pool_(pool), error_collector_(error_collector) {}
  explicit DeferredValidation(const DescriptorPool* pool)
      : pool_(pool), error_collector_(pool->default_error_collector_) {}

  DeferredValidation(const DeferredValidation&) = delete;
  DeferredValidation& operator=(const DeferredValidation&) = delete;
  DeferredValidation(DeferredValidation&&) = delete;
  DeferredValidation& operator=(DeferredValidation&&) = delete;

  ~DeferredValidation() {
    ABSL_CHECK(lifetimes_info_map_.empty())
        << "DeferredValidation destroyed with unvalidated features";
  }

  struct LifetimesInfo {
    const FeatureSet* proto_features;
    const Message* proto;
    absl::string_view full_name;
    absl::string_view filename;
  };
  void ValidateFeatureLifetimes(const FileDescriptor* file,
                                LifetimesInfo info) {
    lifetimes_info_map_[file].emplace_back(std::move(info));
  }

  void RollbackFile(const FileDescriptor* file) {
    lifetimes_info_map_.erase(file);
  }

  // Create a new file proto with an extended lifetime for deferred error
  // reporting.  If any temporary file protos don't outlive this object, the
  // reported errors won't be able to safely reference a location in the
  // original proto file.
  FileDescriptorProto& CreateProto() {
    if (first_proto_ != nullptr) {
      return *std::exchange(first_proto_, nullptr);
    }
    return *Arena::Create<FileDescriptorProto>(&arena_);
  }

  bool Validate() {
    if (lifetimes_info_map_.empty()) return true;

    static absl::string_view feature_set_name = "google.protobuf.FeatureSet";
    const Descriptor* feature_set =
        pool_->FindMessageTypeByName(feature_set_name);

    bool has_errors = false;
    for (const auto& it : lifetimes_info_map_) {
      const FileDescriptor* file = it.first;

      for (const auto& info : it.second) {
        auto results = FeatureResolver::ValidateFeatureLifetimes(
            file->edition(), *info.proto_features, feature_set);
        for (const auto& error : results.errors) {
          has_errors = true;
          if (error_collector_ == nullptr) {
            ABSL_LOG(ERROR)
                << info.filename << " " << info.full_name << ": " << error;
          } else {
            error_collector_->RecordError(
                info.filename, info.full_name, info.proto,
                DescriptorPool::ErrorCollector::NAME, error);
          }
        }
        if (pool_->direct_input_files_.find(file->name()) !=
            pool_->direct_input_files_.end()) {
          for (const auto& warning : results.warnings) {
            if (error_collector_ == nullptr) {
              ABSL_LOG(WARNING)
                  << info.filename << " " << info.full_name << ": " << warning;
            } else {
              error_collector_->RecordWarning(
                  info.filename, info.full_name, info.proto,
                  DescriptorPool::ErrorCollector::NAME, warning);
            }
          }
        }
      }
    }
    lifetimes_info_map_.clear();
    return !has_errors;
  }

 private:
  // Pass an initial buffer to save the first memory allocation.
  // This can speed up lookup misses because we fail fast and won't need extra
  // memory.
  char initial_buffer_[512];
  Arena arena_{initial_buffer_, sizeof(initial_buffer_)};
  // We create the first proto eagerly.
  // We will need it and this way we do it outside the lock to reduce
  // contention.
  FileDescriptorProto* first_proto_ =
      Arena::Create<FileDescriptorProto>(&arena_);
  const DescriptorPool* pool_;
  ErrorCollector* error_collector_;
  absl::flat_hash_map<const FileDescriptor*, std::vector<LifetimesInfo>>
      lifetimes_info_map_;
};

// ===================================================================
// DescriptorPool::Tables

class DescriptorPool::Tables {
 public:
  Tables();
  ~Tables();

  // Record the current state of the tables to the stack of checkpoints.
  // Each call to AddCheckpoint() must be paired with exactly one call to either
  // ClearLastCheckpoint() or RollbackToLastCheckpoint().
  //
  // This is used when building files, since some kinds of validation errors
  // cannot be detected until the file's descriptors have already been added to
  // the tables.
  //
  // This supports recursive checkpoints, since building a file may trigger
  // recursive building of other files. Note that recursive checkpoints are not
  // normally necessary; explicit dependencies are built prior to checkpointing.
  // So although we recursively build transitive imports, there is at most one
  // checkpoint in the stack during dependency building.
  //
  // Recursive checkpoints only arise during cross-linking of the descriptors.
  // Symbol references must be resolved, via DescriptorBuilder::FindSymbol and
  // friends. If the pending file references an unknown symbol
  // (e.g., it is not defined in the pending file's explicit dependencies), and
  // the pool is using a fallback database, and that database contains a file
  // defining that symbol, and that file has not yet been built by the pool,
  // the pool builds the file during cross-linking, leading to another
  // checkpoint.
  void AddCheckpoint();

  // Mark the last checkpoint as having cleared successfully, removing it from
  // the stack. If the stack is empty, all pending symbols will be committed.
  //
  // Note that this does not guarantee that the symbols added since the last
  // checkpoint won't be rolled back: if a checkpoint gets rolled back,
  // everything past that point gets rolled back, including symbols added after
  // checkpoints that were pushed onto the stack after it and marked as cleared.
  void ClearLastCheckpoint();

  // Roll back the Tables to the state of the checkpoint at the top of the
  // stack, removing everything that was added after that point.
  void RollbackToLastCheckpoint(DeferredValidation& deferred_validation);

  // The stack of files which are currently being built.  Used to detect
  // cyclic dependencies when loading files from a DescriptorDatabase.  Not
  // used when fallback_database_ == nullptr.
  std::vector<std::string> pending_files_;

  // A set of files which we have tried to load from the fallback database
  // and encountered errors.  We will not attempt to load them again during
  // execution of the current public API call, but for compatibility with
  // legacy clients, this is cleared at the beginning of each public API call.
  // Not used when fallback_database_ == nullptr.
  absl::flat_hash_set<std::string> known_bad_files_;

  // A set of symbols which we have tried to load from the fallback database
  // and encountered errors. We will not attempt to load them again during
  // execution of the current public API call, but for compatibility with
  // legacy clients, this is cleared at the beginning of each public API call.
  absl::flat_hash_set<std::string> known_bad_symbols_;

  // The set of descriptors for which we've already loaded the full
  // set of extensions numbers from fallback_database_.
  absl::flat_hash_set<const Descriptor*> extensions_loaded_from_db_;

  // Maps type name to Descriptor::WellKnownType.  This is logically global
  // and const, but we make it a member here to simplify its construction and
  // destruction.  This only has 20-ish entries and is one per DescriptorPool,
  // so the overhead is small.
  absl::flat_hash_map<std::string, Descriptor::WellKnownType> well_known_types_;

  // -----------------------------------------------------------------
  // Finding items.

  // Find symbols.  This returns a null Symbol (symbol.IsNull() is true)
  // if not found.
  inline Symbol FindSymbol(absl::string_view key) const;

  // This implements the body of DescriptorPool::Find*ByName().  It should
  // really be a private method of DescriptorPool, but that would require
  // declaring Symbol in descriptor.h, which would drag all kinds of other
  // stuff into the header.  Yay C++.
  Symbol FindByNameHelper(const DescriptorPool* pool, absl::string_view name);

  // These return nullptr if not found.
  inline const FileDescriptor* FindFile(absl::string_view key) const;
  inline const FieldDescriptor* FindExtension(const Descriptor* extendee,
                                              int number) const;
  inline void FindAllExtensions(const Descriptor* extendee,
                                std::vector<const FieldDescriptor*>* out) const;

  // -----------------------------------------------------------------
  // Adding items.

  // These add items to the corresponding tables.  They return false if
  // the key already exists in the table.  For AddSymbol(), the string passed
  // in must be one that was constructed using AllocateString(), as it will
  // be used as a key in the symbols_by_name_ map without copying.
  bool AddSymbol(absl::string_view full_name, Symbol symbol);
  bool AddFile(const FileDescriptor* file);
  bool AddExtension(const FieldDescriptor* field);

  // Caches a feature set and returns a stable reference to the cached
  // allocation owned by the pool.
  const FeatureSet* InternFeatureSet(FeatureSet&& features);

  // -----------------------------------------------------------------
  // Allocating memory.

  // Allocate an object which will be reclaimed when the pool is
  // destroyed.  Note that the object's destructor will never be called,
  // so its fields must be plain old data (primitive data types and
  // pointers).  All of the descriptor types are such objects.
  template <typename Type>
  Type* Allocate();

  // Allocate some bytes which will be reclaimed when the pool is
  // destroyed. Memory is aligned to 8 bytes.
  void* AllocateBytes(int size);

  // Create a FlatAllocation for the corresponding sizes.
  // All objects within it will be default constructed.
  // The whole allocation, including the non-trivial objects within, will be
  // destroyed with the pool.
  template <typename... T>
  internal::FlatAllocator::Allocation* CreateFlatAlloc(
      const TypeMap<IntT, T...>& sizes);


 private:
  // All memory allocated in the pool.  Must be first as other objects can
  // point into these.
  struct MiscDeleter {
    void operator()(int* p) const { internal::SizedDelete(p, *p + 8); }
  };
  // Miscellaneous allocations are length prefixed. The paylaod is 8 bytes after
  // the `int` that contains the size. This keeps the payload aligned.
  std::vector<std::unique_ptr<int, MiscDeleter>> misc_allocs_;
  struct FlatAllocDeleter {
    void operator()(internal::FlatAllocator::Allocation* p) const {
      p->Destroy();
    }
  };
  std::vector<
      std::unique_ptr<internal::FlatAllocator::Allocation, FlatAllocDeleter>>
      flat_allocs_;

  SymbolsByNameSet symbols_by_name_;
  DescriptorsByNameSet<FileDescriptor> files_by_name_;
  ExtensionsGroupedByDescriptorMap extensions_;

  // A cache of all unique feature sets seen.  Since we expect this number to be
  // relatively low compared to descriptors, it's significantly cheaper to share
  // these within the pool than have each file create its own feature sets.
  absl::flat_hash_map<std::string, std::unique_ptr<FeatureSet>>
      feature_set_cache_;

  struct CheckPoint {
    explicit CheckPoint(const Tables* tables)
        : flat_allocations_before_checkpoint(
              static_cast<int>(tables->flat_allocs_.size())),
          misc_allocations_before_checkpoint(
              static_cast<int>(tables->misc_allocs_.size())),
          pending_symbols_before_checkpoint(
              tables->symbols_after_checkpoint_.size()),
          pending_files_before_checkpoint(
              tables->files_after_checkpoint_.size()),
          pending_extensions_before_checkpoint(
              tables->extensions_after_checkpoint_.size()) {}
    int flat_allocations_before_checkpoint;
    int misc_allocations_before_checkpoint;
    int pending_symbols_before_checkpoint;
    int pending_files_before_checkpoint;
    int pending_extensions_before_checkpoint;
  };
  std::vector<CheckPoint> checkpoints_;
  std::vector<Symbol> symbols_after_checkpoint_;
  std::vector<const FileDescriptor*> files_after_checkpoint_;
  std::vector<std::pair<const Descriptor*, int>> extensions_after_checkpoint_;
};

DescriptorPool::Tables::Tables() {
  well_known_types_.insert({
      {"google.protobuf.DoubleValue", Descriptor::WELLKNOWNTYPE_DOUBLEVALUE},
      {"google.protobuf.FloatValue", Descriptor::WELLKNOWNTYPE_FLOATVALUE},
      {"google.protobuf.Int64Value", Descriptor::WELLKNOWNTYPE_INT64VALUE},
      {"google.protobuf.UInt64Value", Descriptor::WELLKNOWNTYPE_UINT64VALUE},
      {"google.protobuf.Int32Value", Descriptor::WELLKNOWNTYPE_INT32VALUE},
      {"google.protobuf.UInt32Value", Descriptor::WELLKNOWNTYPE_UINT32VALUE},
      {"google.protobuf.StringValue", Descriptor::WELLKNOWNTYPE_STRINGVALUE},
      {"google.protobuf.BytesValue", Descriptor::WELLKNOWNTYPE_BYTESVALUE},
      {"google.protobuf.BoolValue", Descriptor::WELLKNOWNTYPE_BOOLVALUE},
      {"google.protobuf.Any", Descriptor::WELLKNOWNTYPE_ANY},
      {"google.protobuf.FieldMask", Descriptor::WELLKNOWNTYPE_FIELDMASK},
      {"google.protobuf.Duration", Descriptor::WELLKNOWNTYPE_DURATION},
      {"google.protobuf.Timestamp", Descriptor::WELLKNOWNTYPE_TIMESTAMP},
      {"google.protobuf.Value", Descriptor::WELLKNOWNTYPE_VALUE},
      {"google.protobuf.ListValue", Descriptor::WELLKNOWNTYPE_LISTVALUE},
      {"google.protobuf.Struct", Descriptor::WELLKNOWNTYPE_STRUCT},
  });
}

DescriptorPool::Tables::~Tables() { ABSL_DCHECK(checkpoints_.empty()); }

FileDescriptorTables::FileDescriptorTables() = default;

FileDescriptorTables::~FileDescriptorTables() {
  delete fields_by_lowercase_name_.load(std::memory_order_acquire);
  delete fields_by_camelcase_name_.load(std::memory_order_acquire);
}

inline const FileDescriptorTables& FileDescriptorTables::GetEmptyInstance() {
  static auto file_descriptor_tables =
      internal::OnShutdownDelete(new FileDescriptorTables());
  return *file_descriptor_tables;
}

void DescriptorPool::Tables::AddCheckpoint() {
  checkpoints_.emplace_back(this);
}

void DescriptorPool::Tables::ClearLastCheckpoint() {
  ABSL_DCHECK(!checkpoints_.empty());
  checkpoints_.pop_back();
  if (checkpoints_.empty()) {
    // All checkpoints have been cleared: we can now commit all of the pending
    // data.
    symbols_after_checkpoint_.clear();
    files_after_checkpoint_.clear();
    extensions_after_checkpoint_.clear();
  }
}

void DescriptorPool::Tables::RollbackToLastCheckpoint(
    DeferredValidation& deferred_validation) {
  ABSL_DCHECK(!checkpoints_.empty());
  const CheckPoint& checkpoint = checkpoints_.back();

  for (size_t i = checkpoint.pending_symbols_before_checkpoint;
       i < symbols_after_checkpoint_.size(); i++) {
    symbols_by_name_.erase(symbols_after_checkpoint_[i]);
  }
  for (size_t i = checkpoint.pending_files_before_checkpoint;
       i < files_after_checkpoint_.size(); i++) {
    deferred_validation.RollbackFile(files_after_checkpoint_[i]);
    files_by_name_.erase(files_after_checkpoint_[i]);
  }
  for (size_t i = checkpoint.pending_extensions_before_checkpoint;
       i < extensions_after_checkpoint_.size(); i++) {
    extensions_.erase(extensions_after_checkpoint_[i]);
  }

  symbols_after_checkpoint_.resize(
      checkpoint.pending_symbols_before_checkpoint);
  files_after_checkpoint_.resize(checkpoint.pending_files_before_checkpoint);
  extensions_after_checkpoint_.resize(
      checkpoint.pending_extensions_before_checkpoint);

  flat_allocs_.resize(checkpoint.flat_allocations_before_checkpoint);
  misc_allocs_.resize(checkpoint.misc_allocations_before_checkpoint);
  checkpoints_.pop_back();
}

// -------------------------------------------------------------------

inline Symbol DescriptorPool::Tables::FindSymbol(absl::string_view key) const {
  auto it = symbols_by_name_.find(FullNameQuery{key});
  return it == symbols_by_name_.end() ? Symbol() : *it;
}

template <typename K>
inline auto FileDescriptorTables::FindNestedSymbol(
    const void* parent, absl::string_view name) const {
  auto it = symbols_by_parent_.find(K{{{parent, name}}});
  return it == symbols_by_parent_.end() ? typename K::SymbolT()
                                        : K::IterToSymbol(it);
}

Symbol DescriptorPool::Tables::FindByNameHelper(const DescriptorPool* pool,
                                                absl::string_view name) {
  if (pool->mutex_ != nullptr) {
    // Fast path: the Symbol is already cached.  This is just a hash lookup.
    absl::ReaderMutexLock lock(pool->mutex_);
    if (known_bad_symbols_.empty() && known_bad_files_.empty()) {
      Symbol result = FindSymbol(name);
      if (!result.IsNull()) return result;
    }
  }
  DescriptorPool::DeferredValidation deferred_validation(pool);
  Symbol result;
  {
    absl::MutexLockMaybe lock(pool->mutex_);
    if (pool->fallback_database_ != nullptr) {
      known_bad_symbols_.clear();
      known_bad_files_.clear();
    }
    result = FindSymbol(name);

    if (result.IsNull() && pool->underlay_ != nullptr) {
      // Symbol not found; check the underlay.
      result =
          pool->underlay_->tables_->FindByNameHelper(pool->underlay_, name);
    }

    if (result.IsNull()) {
      // Symbol still not found, so check fallback database.
      if (pool->TryFindSymbolInFallbackDatabase(name, deferred_validation)) {
        result = FindSymbol(name);
      }
    }
  }

  if (!deferred_validation.Validate()) {
    return Symbol();
  }
  return result;
}

inline const FileDescriptor* DescriptorPool::Tables::FindFile(
    absl::string_view key) const {
  auto it = files_by_name_.find(key);
  if (it == files_by_name_.end()) return nullptr;
  return *it;
}

inline const FieldDescriptor* FileDescriptorTables::FindFieldByNumber(
    const Descriptor* parent, int number) const {
  // If `number` is within the sequential range, just index into the parent
  // without doing a table lookup.
  if (parent != nullptr &&  //
      1 <= number && number <= parent->sequential_field_limit_) {
    return parent->field(number - 1);
  }

  auto it = fields_by_number_.find(ParentNumberQuery{{parent, number}});
  return it == fields_by_number_.end() ? nullptr : *it;
}

const void* FileDescriptorTables::FindParentForFieldsByMap(
    const FieldDescriptor* field) const {
  if (field->is_extension()) {
    if (field->extension_scope() == nullptr) {
      return field->file();
    } else {
      return field->extension_scope();
    }
  } else {
    return field->containing_type();
  }
}

void FileDescriptorTables::FieldsByLowercaseNamesLazyInitStatic(
    const FileDescriptorTables* tables) {
  tables->FieldsByLowercaseNamesLazyInitInternal();
}

void FileDescriptorTables::FieldsByLowercaseNamesLazyInitInternal() const {
  auto* map = new FieldsByNameMap;
  for (Symbol symbol : symbols_by_parent_) {
    const FieldDescriptor* field = symbol.field_descriptor();
    if (!field) continue;
    (*map)[{FindParentForFieldsByMap(field), field->lowercase_name()}] = field;
  }
  fields_by_lowercase_name_.store(map, std::memory_order_release);
}

inline const FieldDescriptor* FileDescriptorTables::FindFieldByLowercaseName(
    const void* parent, absl::string_view lowercase_name) const {
  absl::call_once(fields_by_lowercase_name_once_,
                  &FileDescriptorTables::FieldsByLowercaseNamesLazyInitStatic,
                  this);
  const auto* fields =
      fields_by_lowercase_name_.load(std::memory_order_acquire);
  auto it = fields->find({parent, lowercase_name});
  if (it == fields->end()) return nullptr;
  return it->second;
}

void FileDescriptorTables::FieldsByCamelcaseNamesLazyInitStatic(
    const FileDescriptorTables* tables) {
  tables->FieldsByCamelcaseNamesLazyInitInternal();
}

void FileDescriptorTables::FieldsByCamelcaseNamesLazyInitInternal() const {
  auto* map = new FieldsByNameMap;
  for (Symbol symbol : symbols_by_parent_) {
    const FieldDescriptor* field = symbol.field_descriptor();
    if (!field) continue;
    const void* parent = FindParentForFieldsByMap(field);
    // If we already have a field with this camelCase name, keep the field with
    // the smallest field number. This way we get a deterministic mapping.
    const FieldDescriptor*& found = (*map)[{parent, field->camelcase_name()}];
    if (found == nullptr || found->number() > field->number()) {
      found = field;
    }
  }
  fields_by_camelcase_name_.store(map, std::memory_order_release);
}

inline const FieldDescriptor* FileDescriptorTables::FindFieldByCamelcaseName(
    const void* parent, absl::string_view camelcase_name) const {
  absl::call_once(fields_by_camelcase_name_once_,
                  FileDescriptorTables::FieldsByCamelcaseNamesLazyInitStatic,
                  this);
  auto* fields = fields_by_camelcase_name_.load(std::memory_order_acquire);
  auto it = fields->find({parent, camelcase_name});
  if (it == fields->end()) return nullptr;
  return it->second;
}

inline const EnumValueDescriptor* FileDescriptorTables::FindEnumValueByNumber(
    const EnumDescriptor* parent, int number) const {
  // If `number` is within the sequential range, just index into the parent
  // without doing a table lookup.
  const int base = parent->value(0)->number();
  if (base <= number &&
      number <= static_cast<int64_t>(base) + parent->sequential_value_limit_) {
    return parent->value(number - base);
  }

  auto it = enum_values_by_number_.find(ParentNumberQuery{{parent, number}});
  return it == enum_values_by_number_.end() ? nullptr : *it;
}

inline const EnumValueDescriptor*
FileDescriptorTables::FindEnumValueByNumberCreatingIfUnknown(
    const EnumDescriptor* parent, int number) const {
  // First try, with map of compiled-in values.
  {
    const auto* value = FindEnumValueByNumber(parent, number);
    if (value != nullptr) {
      return value;
    }
  }

  const ParentNumberQuery query{{parent, number}};

  // Second try, with reader lock held on unknown enum values: common case.
  {
    absl::ReaderMutexLock l(&unknown_enum_values_mu_);
    auto it = unknown_enum_values_by_number_.find(query);
    if (it != unknown_enum_values_by_number_.end()) {
      return *it;
    }
  }
  // If not found, try again with writer lock held, and create new descriptor if
  // necessary.
  {
    absl::WriterMutexLock l(&unknown_enum_values_mu_);
    auto it = unknown_enum_values_by_number_.find(query);
    if (it != unknown_enum_values_by_number_.end()) {
      return *it;
    }

    // Create an EnumValueDescriptor dynamically. We don't insert it into the
    // EnumDescriptor (it's not a part of the enum as originally defined), but
    // we do insert it into the table so that we can return the same pointer
    // later.
    std::string enum_value_name =
        absl::StrFormat("UNKNOWN_ENUM_VALUE_%s_%d", parent->name(), number);
    auto* pool = DescriptorPool::generated_pool();
    auto* tables = const_cast<DescriptorPool::Tables*>(pool->tables_.get());
    internal::FlatAllocator alloc;
    alloc.PlanArray<EnumValueDescriptor>(1);
    alloc.PlanArray<std::string>(2);

    {
      // Must lock the pool because we will do allocations in the shared arena.
      absl::MutexLockMaybe l2(pool->mutex_);
      alloc.FinalizePlanning(tables);
    }
    EnumValueDescriptor* result = alloc.AllocateArray<EnumValueDescriptor>(1);
    result->all_names_ = alloc.AllocateStrings(
        enum_value_name,
        absl::StrCat(parent->full_name(), ".", enum_value_name));
    result->number_ = number;
    result->type_ = parent;
    result->options_ = &EnumValueOptions::default_instance();
    unknown_enum_values_by_number_.insert(result);
    return result;
  }
}

inline const FieldDescriptor* DescriptorPool::Tables::FindExtension(
    const Descriptor* extendee, int number) const {
  auto it = extensions_.find({extendee, number});
  if (it == extensions_.end()) return nullptr;
  return it->second;
}

inline void DescriptorPool::Tables::FindAllExtensions(
    const Descriptor* extendee,
    std::vector<const FieldDescriptor*>* out) const {
  ExtensionsGroupedByDescriptorMap::const_iterator it =
      extensions_.lower_bound(std::make_pair(extendee, 0));
  for (; it != extensions_.end() && it->first.first == extendee; ++it) {
    out->push_back(it->second);
  }
}

// -------------------------------------------------------------------

bool DescriptorPool::Tables::AddSymbol(absl::string_view full_name,
                                       Symbol symbol) {
  ABSL_DCHECK_EQ(full_name, symbol.full_name());
  if (symbols_by_name_.insert(symbol).second) {
    symbols_after_checkpoint_.push_back(symbol);
    return true;
  } else {
    return false;
  }
}

bool FileDescriptorTables::AddAliasUnderParent(const void* parent,
                                               absl::string_view name,
                                               Symbol symbol) {
  ABSL_DCHECK_EQ(name, symbol.parent_name_key().second);
  ABSL_DCHECK_EQ(parent, symbol.parent_name_key().first);
  return symbols_by_parent_.insert(symbol).second;
}

bool DescriptorPool::Tables::AddFile(const FileDescriptor* file) {
  if (files_by_name_.insert(file).second) {
    files_after_checkpoint_.push_back(file);
    return true;
  } else {
    return false;
  }
}

void FileDescriptorTables::FinalizeTables() {}

bool FileDescriptorTables::AddFieldByNumber(FieldDescriptor* field) {
  // Skip fields that are at the start of the sequence.
  if (field->containing_type() != nullptr && field->number() >= 1 &&
      field->number() <= field->containing_type()->sequential_field_limit_) {
    if (field->is_extension()) {
      // Conflicts with the field that already exists in the sequential range.
      return false;
    }
    // Only return true if the field at that index matches. Otherwise it
    // conflicts with the existing field in the sequential range.
    return field->containing_type()->field(field->number() - 1) == field;
  }

  return fields_by_number_.insert(field).second;
}

bool FileDescriptorTables::AddEnumValueByNumber(EnumValueDescriptor* value) {
  // Skip values that are at the start of the sequence.
  const int base = value->type()->value(0)->number();
  if (base <= value->number() &&
      value->number() <=
          static_cast<int64_t>(base) + value->type()->sequential_value_limit_)
    return true;
  return enum_values_by_number_.insert(value).second;
}

bool DescriptorPool::Tables::AddExtension(const FieldDescriptor* field) {
  auto it_inserted =
      extensions_.insert({{field->containing_type(), field->number()}, field});
  if (it_inserted.second) {
    extensions_after_checkpoint_.push_back(it_inserted.first->first);
    return true;
  } else {
    return false;
  }
}

const FeatureSet* DescriptorPool::Tables::InternFeatureSet(
    FeatureSet&& features) {
  // Use the serialized feature set as the cache key.  If multiple equivalent
  // feature sets serialize to different strings, that just bloats the cache a
  // little.
  auto& result = feature_set_cache_[features.SerializeAsString()];
  if (result == nullptr) {
    result = absl::make_unique<FeatureSet>(std::move(features));
  }
  return result.get();
}

// -------------------------------------------------------------------

template <typename Type>
Type* DescriptorPool::Tables::Allocate() {
  static_assert(std::is_trivially_destructible<Type>::value, "");
  static_assert(alignof(Type) <= 8, "");
  return ::new (AllocateBytes(sizeof(Type))) Type{};
}

void* DescriptorPool::Tables::AllocateBytes(int size) {
  if (size == 0) return nullptr;
  void* p = ::operator new(size + RoundUpTo<8>(sizeof(int)));
  int* sizep = static_cast<int*>(p);
  misc_allocs_.emplace_back(sizep);
  *sizep = size;
  return static_cast<char*>(p) + RoundUpTo<8>(sizeof(int));
}

template <typename... T>
internal::FlatAllocator::Allocation* DescriptorPool::Tables::CreateFlatAlloc(
    const TypeMap<IntT, T...>& sizes) {
  auto ends = CalculateEnds(sizes);
  using FlatAlloc = internal::FlatAllocator::Allocation;

  int last_end = ends.template Get<
      typename std::tuple_element<sizeof...(T) - 1, std::tuple<T...>>::type>();
  size_t total_size =
      last_end + RoundUpTo<FlatAlloc::kMaxAlign>(sizeof(FlatAlloc));
  char* data = static_cast<char*>(::operator new(total_size));
  auto* res = ::new (data) FlatAlloc(ends);
  flat_allocs_.emplace_back(res);

  return res;
}

void FileDescriptorTables::BuildLocationsByPath(
    std::pair<const FileDescriptorTables*, const SourceCodeInfo*>* p) {
  for (int i = 0, len = p->second->location_size(); i < len; ++i) {
    const SourceCodeInfo_Location* loc = &p->second->location().Get(i);
    p->first->locations_by_path_[absl::StrJoin(loc->path(), ",")] = loc;
  }
}

const SourceCodeInfo_Location* FileDescriptorTables::GetSourceLocation(
    const std::vector<int>& path, const SourceCodeInfo* info) const {
  std::pair<const FileDescriptorTables*, const SourceCodeInfo*> p(
      std::make_pair(this, info));
  absl::call_once(locations_by_path_once_,
                  FileDescriptorTables::BuildLocationsByPath, &p);
  auto it = locations_by_path_.find(absl::StrJoin(path, ","));
  if (it == locations_by_path_.end()) return nullptr;
  return it->second;
}

// ===================================================================
// DescriptorPool

DescriptorPool::ErrorCollector::~ErrorCollector() = default;

absl::string_view DescriptorPool::ErrorCollector::ErrorLocationName(
    ErrorLocation location) {
  switch (location) {
    case NAME:
      return "NAME";
    case NUMBER:
      return "NUMBER";
    case TYPE:
      return "TYPE";
    case EXTENDEE:
      return "EXTENDEE";
    case DEFAULT_VALUE:
      return "DEFAULT_VALUE";
    case OPTION_NAME:
      return "OPTION_NAME";
    case OPTION_VALUE:
      return "OPTION_VALUE";
    case INPUT_TYPE:
      return "INPUT_TYPE";
    case OUTPUT_TYPE:
      return "OUTPUT_TYPE";
    case IMPORT:
      return "IMPORT";
    case EDITIONS:
      return "EDITIONS";
    case OTHER:
      return "OTHER";
  }
  return "UNKNOWN";
}

DescriptorPool::DescriptorPool()
    : mutex_(nullptr),
      fallback_database_(nullptr),
      default_error_collector_(nullptr),
      underlay_(nullptr),
      tables_(new Tables),
      enforce_dependencies_(true),
      lazily_build_dependencies_(false),
      allow_unknown_(false),
      enforce_weak_(false),
      enforce_extension_declarations_(ExtDeclEnforcementLevel::kNoEnforcement),
      disallow_enforce_utf8_(false),
      deprecated_legacy_json_field_conflicts_(false),
      enforce_naming_style_(false) {}

DescriptorPool::DescriptorPool(DescriptorDatabase* fallback_database,
                               ErrorCollector* error_collector)
    : mutex_(new absl::Mutex),
      fallback_database_(fallback_database),
      default_error_collector_(error_collector),
      underlay_(nullptr),
      tables_(new Tables),
      enforce_dependencies_(true),
      lazily_build_dependencies_(false),
      allow_unknown_(false),
      enforce_weak_(false),
      enforce_extension_declarations_(ExtDeclEnforcementLevel::kNoEnforcement),
      disallow_enforce_utf8_(false),
      deprecated_legacy_json_field_conflicts_(false),
      enforce_naming_style_(false) {}

DescriptorPool::DescriptorPool(const DescriptorPool* underlay)
    : mutex_(nullptr),
      fallback_database_(nullptr),
      default_error_collector_(nullptr),
      underlay_(underlay),
      tables_(new Tables),
      enforce_dependencies_(true),
      lazily_build_dependencies_(false),
      allow_unknown_(false),
      enforce_weak_(false),
      enforce_extension_declarations_(ExtDeclEnforcementLevel::kNoEnforcement),
      disallow_enforce_utf8_(false),
      deprecated_legacy_json_field_conflicts_(false),
      enforce_naming_style_(false) {}

DescriptorPool::~DescriptorPool() {
  if (mutex_ != nullptr) delete mutex_;
}

// DescriptorPool::BuildFile() defined later.
// DescriptorPool::BuildFileCollectingErrors() defined later.

void DescriptorPool::InternalDontEnforceDependencies() {
  enforce_dependencies_ = false;
}

void DescriptorPool::AddDirectInputFile(absl::string_view file_name,
                                        bool is_error) {
  direct_input_files_[file_name] = is_error;
}

bool DescriptorPool::IsReadyForCheckingDescriptorExtDecl(
    absl::string_view message_name) const {
  static const auto& kDescriptorTypes = *new absl::flat_hash_set<std::string>({
      "google.protobuf.EnumOptions",
      "google.protobuf.EnumValueOptions",
      "google.protobuf.ExtensionRangeOptions",
      "google.protobuf.FieldOptions",
      "google.protobuf.FileOptions",
      "google.protobuf.MessageOptions",
      "google.protobuf.MethodOptions",
      "google.protobuf.OneofOptions",
      "google.protobuf.ServiceOptions",
      "google.protobuf.StreamOptions",
  });
  return kDescriptorTypes.contains(message_name);
}


void DescriptorPool::ClearDirectInputFiles() { direct_input_files_.clear(); }

bool DescriptorPool::InternalIsFileLoaded(absl::string_view filename) const {
  absl::MutexLockMaybe lock(mutex_);
  return tables_->FindFile(filename) != nullptr;
}

// generated_pool ====================================================

namespace {


EncodedDescriptorDatabase* GeneratedDatabase() {
  static auto generated_database =
      internal::OnShutdownDelete(new EncodedDescriptorDatabase());
  return generated_database;
}

DescriptorPool* NewGeneratedPool() {
  auto generated_pool = new DescriptorPool(GeneratedDatabase());
  generated_pool->InternalSetLazilyBuildDependencies();
  return generated_pool;
}

}  // anonymous namespace

DescriptorDatabase* DescriptorPool::internal_generated_database() {
  return GeneratedDatabase();
}

DescriptorPool* DescriptorPool::internal_generated_pool() {
  static DescriptorPool* generated_pool =
      internal::OnShutdownDelete(NewGeneratedPool());
  return generated_pool;
}

const DescriptorPool* DescriptorPool::generated_pool() {
  const DescriptorPool* pool = internal_generated_pool();
  // Ensure that descriptor.proto and cpp_features.proto get registered in the
  // generated pool. They're special cases because they're included in the full
  // runtime. We have to avoid registering it pre-main, because we need to
  // ensure that the linker --gc-sections step can strip out the full runtime if
  // it is unused.
  DescriptorProto::descriptor();
  pb::CppFeatures::descriptor();
  return pool;
}


void DescriptorPool::InternalAddGeneratedFile(
    const void* encoded_file_descriptor, int size) {
  // So, this function is called in the process of initializing the
  // descriptors for generated proto classes.  Each generated .pb.cc file
  // has an internal procedure called AddDescriptors() which is called at
  // process startup, and that function calls this one in order to register
  // the raw bytes of the FileDescriptorProto representing the file.
  //
  // We do not actually construct the descriptor objects right away.  We just
  // hang on to the bytes until they are actually needed.  We actually construct
  // the descriptor the first time one of the following things happens:
  // * Someone calls a method like descriptor(), GetDescriptor(), or
  //   GetReflection() on the generated types, which requires returning the
  //   descriptor or an object based on it.
  // * Someone looks up the descriptor in DescriptorPool::generated_pool().
  //
  // Once one of these happens, the DescriptorPool actually parses the
  // FileDescriptorProto and generates a FileDescriptor (and all its children)
  // based on it.
  //
  // Note that FileDescriptorProto is itself a generated protocol message.
  // Therefore, when we parse one, we have to be very careful to avoid using
  // any descriptor-based operations, since this might cause infinite recursion
  // or deadlock.
  absl::MutexLockMaybe lock(internal_generated_pool()->mutex_);
  ABSL_CHECK(GeneratedDatabase()->Add(encoded_file_descriptor, size));
}


// Find*By* methods ==================================================

// TODO:  There's a lot of repeated code here, but I'm not sure if
//   there's any good way to factor it out.  Think about this some time when
//   there's nothing more important to do (read: never).

const FileDescriptor* DescriptorPool::FindFileByName(
    absl::string_view name) const {
  DeferredValidation deferred_validation(this);
  const FileDescriptor* result = nullptr;
  {
    absl::MutexLockMaybe lock(mutex_);
    if (fallback_database_ != nullptr) {
      tables_->known_bad_symbols_.clear();
      tables_->known_bad_files_.clear();
    }
    result = tables_->FindFile(name);
    if (result != nullptr) return result;
    if (underlay_ != nullptr) {
      result = underlay_->FindFileByName(name);
      if (result != nullptr) return result;
    }
    if (TryFindFileInFallbackDatabase(name, deferred_validation)) {
      result = tables_->FindFile(name);
    }
  }
  if (!deferred_validation.Validate()) {
    return nullptr;
  }
  return result;
}

const FileDescriptor* DescriptorPool::FindFileContainingSymbol(
    absl::string_view symbol_name) const {
  const FileDescriptor* file_result = nullptr;
  DeferredValidation deferred_validation(this);
  {
    absl::MutexLockMaybe lock(mutex_);
    if (fallback_database_ != nullptr) {
      tables_->known_bad_symbols_.clear();
      tables_->known_bad_files_.clear();
    }
    Symbol result = tables_->FindSymbol(symbol_name);
    if (!result.IsNull()) return result.GetFile();
    if (underlay_ != nullptr) {
      file_result = underlay_->FindFileContainingSymbol(symbol_name);
      if (file_result != nullptr) return file_result;
    }
    if (TryFindSymbolInFallbackDatabase(symbol_name, deferred_validation)) {
      result = tables_->FindSymbol(symbol_name);
      if (!result.IsNull()) file_result = result.GetFile();
    }
  }
  if (!deferred_validation.Validate()) {
    return nullptr;
  }
  return file_result;
}

const Descriptor* DescriptorPool::FindMessageTypeByName(
    absl::string_view name) const {
  return tables_->FindByNameHelper(this, name).descriptor();
}

const FieldDescriptor* DescriptorPool::FindFieldByName(
    absl::string_view name) const {
  if (const FieldDescriptor* field =
          tables_->FindByNameHelper(this, name).field_descriptor()) {
    if (!field->is_extension()) {
      return field;
    }
  }
  return nullptr;
}

const FieldDescriptor* DescriptorPool::FindExtensionByName(
    absl::string_view name) const {
  if (const FieldDescriptor* field =
          tables_->FindByNameHelper(this, name).field_descriptor()) {
    if (field->is_extension()) {
      return field;
    }
  }
  return nullptr;
}

const OneofDescriptor* DescriptorPool::FindOneofByName(
    absl::string_view name) const {
  return tables_->FindByNameHelper(this, name).oneof_descriptor();
}

const EnumDescriptor* DescriptorPool::FindEnumTypeByName(
    absl::string_view name) const {
  return tables_->FindByNameHelper(this, name).enum_descriptor();
}

const EnumValueDescriptor* DescriptorPool::FindEnumValueByName(
    absl::string_view name) const {
  return tables_->FindByNameHelper(this, name).enum_value_descriptor();
}

const ServiceDescriptor* DescriptorPool::FindServiceByName(
    absl::string_view name) const {
  return tables_->FindByNameHelper(this, name).service_descriptor();
}

const MethodDescriptor* DescriptorPool::FindMethodByName(
    absl::string_view name) const {
  return tables_->FindByNameHelper(this, name).method_descriptor();
}

const FieldDescriptor* DescriptorPool::FindExtensionByNumber(
    const Descriptor* extendee, int number) const {
  if (extendee->extension_range_count() == 0) return nullptr;
  // A faster path to reduce lock contention in finding extensions, assuming
  // most extensions will be cache hit.
  if (mutex_ != nullptr) {
    absl::ReaderMutexLock lock(mutex_);
    const FieldDescriptor* result = tables_->FindExtension(extendee, number);
    if (result != nullptr) {
      return result;
    }
  }
  const FieldDescriptor* result = nullptr;
  DeferredValidation deferred_validation(this);
  {
    absl::MutexLockMaybe lock(mutex_);
    if (fallback_database_ != nullptr) {
      tables_->known_bad_symbols_.clear();
      tables_->known_bad_files_.clear();
    }
    result = tables_->FindExtension(extendee, number);
    if (result != nullptr) {
      return result;
    }
    if (underlay_ != nullptr) {
      result = underlay_->FindExtensionByNumber(extendee, number);
      if (result != nullptr) return result;
    }
    if (TryFindExtensionInFallbackDatabase(extendee, number,
                                           deferred_validation)) {
      result = tables_->FindExtension(extendee, number);
    }
  }
  if (!deferred_validation.Validate()) {
    return nullptr;
  }
  return result;
}

const FieldDescriptor* DescriptorPool::InternalFindExtensionByNumberNoLock(
    const Descriptor* extendee, int number) const {
  if (extendee->extension_range_count() == 0) return nullptr;

  const FieldDescriptor* result = tables_->FindExtension(extendee, number);
  if (result != nullptr) {
    return result;
  }

  if (underlay_ != nullptr) {
    result = underlay_->InternalFindExtensionByNumberNoLock(extendee, number);
    if (result != nullptr) return result;
  }

  return nullptr;
}

const FieldDescriptor* DescriptorPool::FindExtensionByPrintableName(
    const Descriptor* extendee, absl::string_view printable_name) const {
  if (extendee->extension_range_count() == 0) return nullptr;
  const FieldDescriptor* result = FindExtensionByName(printable_name);
  if (result != nullptr && result->containing_type() == extendee) {
    return result;
  }
  if (extendee->options().message_set_wire_format()) {
    // MessageSet extensions may be identified by type name.
    const Descriptor* type = FindMessageTypeByName(printable_name);
    if (type != nullptr) {
      // Look for a matching extension in the foreign type's scope.
      const int type_extension_count = type->extension_count();
      for (int i = 0; i < type_extension_count; i++) {
        const FieldDescriptor* extension = type->extension(i);
        if (extension->containing_type() == extendee &&
            extension->type() == FieldDescriptor::TYPE_MESSAGE &&
            !extension->is_required() && !extension->is_repeated() &&
            extension->message_type() == type) {
          // Found it.
          return extension;
        }
      }
    }
  }
  return nullptr;
}

void DescriptorPool::FindAllExtensions(
    const Descriptor* extendee,
    std::vector<const FieldDescriptor*>* out) const {
  DeferredValidation deferred_validation(this);
  std::vector<const FieldDescriptor*> extensions;
  {
    absl::MutexLockMaybe lock(mutex_);
    if (fallback_database_ != nullptr) {
      tables_->known_bad_symbols_.clear();
      tables_->known_bad_files_.clear();
    }

    // Initialize tables_->extensions_ from the fallback database first
    // (but do this only once per descriptor).
    if (fallback_database_ != nullptr &&
        tables_->extensions_loaded_from_db_.count(extendee) == 0) {
      std::vector<int> numbers;
      if (fallback_database_->FindAllExtensionNumbers(
              std::string(extendee->full_name()), &numbers)) {
        for (int number : numbers) {
          if (tables_->FindExtension(extendee, number) == nullptr) {
            TryFindExtensionInFallbackDatabase(extendee, number,
                                               deferred_validation);
          }
        }
        tables_->extensions_loaded_from_db_.insert(extendee);
      }
    }

    tables_->FindAllExtensions(extendee, &extensions);
    if (underlay_ != nullptr) {
      underlay_->FindAllExtensions(extendee, &extensions);
    }
  }
  if (deferred_validation.Validate()) {
    out->insert(out->end(), extensions.begin(), extensions.end());
  }
}


// -------------------------------------------------------------------

const FieldDescriptor* Descriptor::FindFieldByNumber(int number) const {
  const FieldDescriptor* result =
      file()->tables_->FindFieldByNumber(this, number);
  if (result == nullptr || result->is_extension()) {
    return nullptr;
  } else {
    return result;
  }
}

const FieldDescriptor* Descriptor::FindFieldByLowercaseName(
    absl::string_view lowercase_name) const {
  const FieldDescriptor* result =
      file()->tables_->FindFieldByLowercaseName(this, lowercase_name);
  if (result == nullptr || result->is_extension()) {
    return nullptr;
  } else {
    return result;
  }
}

const FieldDescriptor* Descriptor::FindFieldByCamelcaseName(
    absl::string_view camelcase_name) const {
  const FieldDescriptor* result =
      file()->tables_->FindFieldByCamelcaseName(this, camelcase_name);
  if (result == nullptr || result->is_extension()) {
    return nullptr;
  } else {
    return result;
  }
}

const FieldDescriptor* Descriptor::FindFieldByName(
    absl::string_view name) const {
  return file()->tables_->FindNestedSymbol<ParentNameFieldQuery>(this, name);
}

const OneofDescriptor* Descriptor::FindOneofByName(
    absl::string_view name) const {
  return file()->tables_->FindNestedSymbol(this, name).oneof_descriptor();
}

const FieldDescriptor* Descriptor::FindExtensionByName(
    absl::string_view name) const {
  const FieldDescriptor* field =
      file()->tables_->FindNestedSymbol(this, name).field_descriptor();
  return field != nullptr && field->is_extension() ? field : nullptr;
}

const FieldDescriptor* Descriptor::FindExtensionByLowercaseName(
    absl::string_view name) const {
  const FieldDescriptor* result =
      file()->tables_->FindFieldByLowercaseName(this, name);
  if (result == nullptr || !result->is_extension()) {
    return nullptr;
  } else {
    return result;
  }
}

const FieldDescriptor* Descriptor::FindExtensionByCamelcaseName(
    absl::string_view name) const {
  const FieldDescriptor* result =
      file()->tables_->FindFieldByCamelcaseName(this, name);
  if (result == nullptr || !result->is_extension()) {
    return nullptr;
  } else {
    return result;
  }
}

const Descriptor* Descriptor::FindNestedTypeByName(
    absl::string_view name) const {
  return file()->tables_->FindNestedSymbol(this, name).descriptor();
}

const EnumDescriptor* Descriptor::FindEnumTypeByName(
    absl::string_view name) const {
  return file()->tables_->FindNestedSymbol(this, name).enum_descriptor();
}

const EnumValueDescriptor* Descriptor::FindEnumValueByName(
    absl::string_view name) const {
  return file()->tables_->FindNestedSymbol(this, name).enum_value_descriptor();
}

const FieldDescriptor* Descriptor::map_key() const {
  if (!options().map_entry()) return nullptr;
  ABSL_DCHECK_EQ(field_count(), 2);
  return field(0);
}

const FieldDescriptor* Descriptor::map_value() const {
  if (!options().map_entry()) return nullptr;
  ABSL_DCHECK_EQ(field_count(), 2);
  return field(1);
}

const EnumValueDescriptor* EnumDescriptor::FindValueByName(
    absl::string_view name) const {
  return file()->tables_->FindNestedSymbol(this, name).enum_value_descriptor();
}

const EnumValueDescriptor* EnumDescriptor::FindValueByNumber(int number) const {
  return file()->tables_->FindEnumValueByNumber(this, number);
}

const EnumValueDescriptor* EnumDescriptor::FindValueByNumberCreatingIfUnknown(
    int number) const {
  return file()->tables_->FindEnumValueByNumberCreatingIfUnknown(this, number);
}

const MethodDescriptor* ServiceDescriptor::FindMethodByName(
    absl::string_view name) const {
  return file()->tables_->FindNestedSymbol(this, name).method_descriptor();
}

const Descriptor* FileDescriptor::FindMessageTypeByName(
    absl::string_view name) const {
  return tables_->FindNestedSymbol(this, name).descriptor();
}

const EnumDescriptor* FileDescriptor::FindEnumTypeByName(
    absl::string_view name) const {
  return tables_->FindNestedSymbol(this, name).enum_descriptor();
}

const EnumValueDescriptor* FileDescriptor::FindEnumValueByName(
    absl::string_view name) const {
  return tables_->FindNestedSymbol(this, name).enum_value_descriptor();
}

const ServiceDescriptor* FileDescriptor::FindServiceByName(
    absl::string_view name) const {
  return tables_->FindNestedSymbol(this, name).service_descriptor();
}

const FieldDescriptor* FileDescriptor::FindExtensionByName(
    absl::string_view name) const {
  const FieldDescriptor* field =
      tables_->FindNestedSymbol(this, name).field_descriptor();
  return field != nullptr && field->is_extension() ? field : nullptr;
}

const FieldDescriptor* FileDescriptor::FindExtensionByLowercaseName(
    absl::string_view name) const {
  const FieldDescriptor* result = tables_->FindFieldByLowercaseName(this, name);
  if (result == nullptr || !result->is_extension()) {
    return nullptr;
  } else {
    return result;
  }
}

const FieldDescriptor* FileDescriptor::FindExtensionByCamelcaseName(
    absl::string_view name) const {
  const FieldDescriptor* result = tables_->FindFieldByCamelcaseName(this, name);
  if (result == nullptr || !result->is_extension()) {
    return nullptr;
  } else {
    return result;
  }
}

void Descriptor::ExtensionRange::CopyTo(
    DescriptorProto_ExtensionRange* proto) const {
  proto->set_start(start_);
  proto->set_end(end_);
  if (options_ != &ExtensionRangeOptions::default_instance()) {
    *proto->mutable_options() = *options_;
  }
  RestoreFeaturesToOptions(proto_features_, proto);
}

const Descriptor::ExtensionRange*
Descriptor::FindExtensionRangeContainingNumber(int number) const {
  // Linear search should be fine because we don't expect a message to have
  // more than a couple extension ranges.
  for (int i = 0; i < extension_range_count(); i++) {
    if (number >= extension_range(i)->start_number() &&
        number < extension_range(i)->end_number()) {
      return extension_range(i);
    }
  }
  return nullptr;
}

const Descriptor::ReservedRange* Descriptor::FindReservedRangeContainingNumber(
    int number) const {
  // TODO: Consider a non-linear search.
  for (int i = 0; i < reserved_range_count(); i++) {
    if (number >= reserved_range(i)->start && number < reserved_range(i)->end) {
      return reserved_range(i);
    }
  }
  return nullptr;
}

const EnumDescriptor::ReservedRange*
EnumDescriptor::FindReservedRangeContainingNumber(int number) const {
  // TODO: Consider a non-linear search.
  for (int i = 0; i < reserved_range_count(); i++) {
    if (number >= reserved_range(i)->start &&
        number <= reserved_range(i)->end) {
      return reserved_range(i);
    }
  }
  return nullptr;
}

// -------------------------------------------------------------------

bool DescriptorPool::TryFindFileInFallbackDatabase(
    absl::string_view name, DeferredValidation& deferred_validation) const {
  if (fallback_database_ == nullptr) return false;

  if (tables_->known_bad_files_.contains(name)) return false;

  // NOINLINE to reduce the stack cost of the operation in the caller.
  const auto find_file = [](DescriptorDatabase& database,
                            absl::string_view filename,
                            FileDescriptorProto& output) PROTOBUF_NOINLINE {
    return database.FindFileByName(std::string(filename), &output);
  };

  auto& file_proto = deferred_validation.CreateProto();
  if (!find_file(*fallback_database_, name, file_proto) ||
      BuildFileFromDatabase(file_proto, deferred_validation) == nullptr) {
    tables_->known_bad_files_.emplace(name);
    return false;
  }
  return true;
}

bool DescriptorPool::IsSubSymbolOfBuiltType(absl::string_view name) const {
  for (size_t pos = name.find('.'); pos != name.npos;
       pos = name.find('.', pos + 1)) {
    auto prefix = name.substr(0, pos);
    Symbol symbol = tables_->FindSymbol(prefix);
    if (symbol.IsNull()) {
      break;
    }
    if (!symbol.IsPackage()) {
      // If the symbol type is anything other than PACKAGE, then its complete
      // definition is already known.
      return true;
    }
  }
  if (underlay_ != nullptr) {
    // Check to see if any prefix of this symbol exists in the underlay.
    return underlay_->IsSubSymbolOfBuiltType(name);
  }
  return false;
}

bool DescriptorPool::TryFindSymbolInFallbackDatabase(
    absl::string_view name, DeferredValidation& deferred_validation) const {
  if (fallback_database_ == nullptr) return false;

  if (tables_->known_bad_symbols_.contains(name)) return false;

  std::string name_string(name);
  auto& file_proto = deferred_validation.CreateProto();
  if (  // We skip looking in the fallback database if the name is a sub-symbol
        // of any descriptor that already exists in the descriptor pool (except
        // for package descriptors).  This is valid because all symbols except
        // for packages are defined in a single file, so if the symbol exists
        // then we should already have its definition.
        //
        // The other reason to do this is to support "overriding" type
        // definitions by merging two databases that define the same type. (Yes,
        // people do this.)  The main difficulty with making this work is that
        // FindFileContainingSymbol() is allowed to return both false positives
        // (e.g., SimpleDescriptorDatabase, UpgradedDescriptorDatabase) and
        // false negatives (e.g. ProtoFileParser, SourceTreeDescriptorDatabase).
        // When two such databases are merged, looking up a non-existent
        // sub-symbol of a type that already exists in the descriptor pool can
        // result in an attempt to load multiple definitions of the same type.
        // The check below avoids this.
      IsSubSymbolOfBuiltType(name)

      // Look up file containing this symbol in fallback database.
      || !fallback_database_->FindFileContainingSymbol(name_string, &file_proto)

      // Check if we've already built this file. If so, it apparently doesn't
      // contain the symbol we're looking for.  Some DescriptorDatabases
      // return false positives.
      || tables_->FindFile(file_proto.name()) != nullptr

      // Build the file.
      || BuildFileFromDatabase(file_proto, deferred_validation) == nullptr) {
    tables_->known_bad_symbols_.insert(std::move(name_string));
    return false;
  }

  return true;
}

bool DescriptorPool::TryFindExtensionInFallbackDatabase(
    const Descriptor* containing_type, int field_number,
    DeferredValidation& deferred_validation) const {
  if (fallback_database_ == nullptr) return false;

  auto& file_proto = deferred_validation.CreateProto();
  if (!fallback_database_->FindFileContainingExtension(
          std::string(containing_type->full_name()), field_number,
          &file_proto)) {
    return false;
  }

  if (tables_->FindFile(file_proto.name()) != nullptr) {
    // We've already loaded this file, and it apparently doesn't contain the
    // extension we're looking for.  Some DescriptorDatabases return false
    // positives.
    return false;
  }

  if (BuildFileFromDatabase(file_proto, deferred_validation) == nullptr) {
    return false;
  }

  return true;
}

// ===================================================================

bool FieldDescriptor::is_map_message_type() const {
  return message_type()->options().map_entry();
}

std::string FieldDescriptor::DefaultValueAsString(
    bool quote_string_type) const {
  ABSL_CHECK(has_default_value()) << "No default value";
  switch (cpp_type()) {
    case CPPTYPE_INT32:
      return absl::StrCat(default_value_int32_t());
    case CPPTYPE_INT64:
      return absl::StrCat(default_value_int64_t());
    case CPPTYPE_UINT32:
      return absl::StrCat(default_value_uint32_t());
    case CPPTYPE_UINT64:
      return absl::StrCat(default_value_uint64_t());
    case CPPTYPE_FLOAT:
      return io::SimpleFtoa(default_value_float());
    case CPPTYPE_DOUBLE:
      return io::SimpleDtoa(default_value_double());
    case CPPTYPE_BOOL:
      return default_value_bool() ? "true" : "false";
    case CPPTYPE_STRING:
      if (quote_string_type) {
        return absl::StrCat("\"", absl::CEscape(default_value_string()), "\"");
      } else {
        if (type() == TYPE_BYTES) {
          return absl::CEscape(default_value_string());
        } else {
          return std::string(default_value_string());
        }
      }
    case CPPTYPE_ENUM:
      return std::string(default_value_enum()->name());
    case CPPTYPE_MESSAGE:
      ABSL_DLOG(FATAL) << "Messages can't have default values!";
      break;
  }
  ABSL_LOG(FATAL) << "Can't get here: failed to get default value as string";
  return "";
}

// Out-of-line constructor definitions ==============================
// When using constructor type homing in Clang, debug info for a type
// is only emitted when a constructor definition is emitted, as an
// optimization. These constructors are never called, so we define them
// out of line to make sure the debug info is emitted somewhere.

Descriptor::Descriptor() = default;
FieldDescriptor::FieldDescriptor() {}
OneofDescriptor::OneofDescriptor() = default;
EnumDescriptor::EnumDescriptor() = default;
EnumValueDescriptor::EnumValueDescriptor() = default;
ServiceDescriptor::ServiceDescriptor() = default;
MethodDescriptor::MethodDescriptor() = default;
FileDescriptor::FileDescriptor() = default;

// CopyTo methods ====================================================

void FileDescriptor::CopyTo(FileDescriptorProto* proto) const {
  CopyHeadingTo(proto);

  for (int i = 0; i < dependency_count(); i++) {
    proto->add_dependency(dependency(i)->name());
  }

  for (int i = 0; i < public_dependency_count(); i++) {
    proto->add_public_dependency(public_dependencies_[i]);
  }

  for (int i = 0; i < weak_dependency_count(); i++) {
    proto->add_weak_dependency(weak_dependencies_[i]);
  }

  for (int i = 0; i < option_dependency_count(); i++) {
    proto->add_option_dependency(option_dependency_name(i));
  }

  for (int i = 0; i < message_type_count(); i++) {
    message_type(i)->CopyTo(proto->add_message_type());
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->CopyTo(proto->add_enum_type());
  }
  for (int i = 0; i < service_count(); i++) {
    service(i)->CopyTo(proto->add_service());
  }
  for (int i = 0; i < extension_count(); i++) {
    extension(i)->CopyTo(proto->add_extension());
  }
}

void FileDescriptor::CopyHeadingTo(FileDescriptorProto* proto) const {
  proto->set_name(name());
  if (!package().empty()) {
    proto->set_package(package());
  }

  if (edition() == Edition::EDITION_PROTO3) {
    proto->set_syntax("proto3");
  } else if (!IsLegacyEdition(edition())) {
    proto->set_syntax("editions");
    proto->set_edition(edition());
  }

  if (&options() != &FileOptions::default_instance()) {
    *proto->mutable_options() = options();
  }
  RestoreFeaturesToOptions(proto_features_, proto);
}

void FileDescriptor::CopyJsonNameTo(FileDescriptorProto* proto) const {
  if (message_type_count() != proto->message_type_size() ||
      extension_count() != proto->extension_size()) {
    ABSL_LOG(ERROR) << "Cannot copy json_name to a proto of a different size.";
    return;
  }
  for (int i = 0; i < message_type_count(); i++) {
    message_type(i)->CopyJsonNameTo(proto->mutable_message_type(i));
  }
  for (int i = 0; i < extension_count(); i++) {
    extension(i)->CopyJsonNameTo(proto->mutable_extension(i));
  }
}

void FileDescriptor::CopySourceCodeInfoTo(FileDescriptorProto* proto) const {
  if (source_code_info_ &&
      source_code_info_ != &SourceCodeInfo::default_instance()) {
    *proto->mutable_source_code_info() = *source_code_info_;
  }
}

void Descriptor::CopyTo(DescriptorProto* proto) const {
  CopyHeadingTo(proto);

  for (int i = 0; i < field_count(); i++) {
    field(i)->CopyTo(proto->add_field());
  }
  for (int i = 0; i < oneof_decl_count(); i++) {
    oneof_decl(i)->CopyTo(proto->add_oneof_decl());
  }
  for (int i = 0; i < nested_type_count(); i++) {
    nested_type(i)->CopyTo(proto->add_nested_type());
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->CopyTo(proto->add_enum_type());
  }
  for (int i = 0; i < extension_range_count(); i++) {
    extension_range(i)->CopyTo(proto->add_extension_range());
  }
  for (int i = 0; i < extension_count(); i++) {
    extension(i)->CopyTo(proto->add_extension());
  }
}

void Descriptor::CopyHeadingTo(DescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < reserved_range_count(); i++) {
    DescriptorProto::ReservedRange* range = proto->add_reserved_range();
    range->set_start(reserved_range(i)->start);
    range->set_end(reserved_range(i)->end);
  }
  for (int i = 0; i < reserved_name_count(); i++) {
    proto->add_reserved_name(reserved_name(i));
  }

  if (&options() != &MessageOptions::default_instance()) {
    *proto->mutable_options() = options();
  }

  if (visibility_keyword() != SymbolVisibility::VISIBILITY_UNSET) {
    proto->set_visibility(visibility_keyword());
  }

  RestoreFeaturesToOptions(proto_features_, proto);
}

void Descriptor::CopyJsonNameTo(DescriptorProto* proto) const {
  if (field_count() != proto->field_size() ||
      nested_type_count() != proto->nested_type_size() ||
      extension_count() != proto->extension_size()) {
    ABSL_LOG(ERROR) << "Cannot copy json_name to a proto of a different size.";
    return;
  }
  for (int i = 0; i < field_count(); i++) {
    field(i)->CopyJsonNameTo(proto->mutable_field(i));
  }
  for (int i = 0; i < nested_type_count(); i++) {
    nested_type(i)->CopyJsonNameTo(proto->mutable_nested_type(i));
  }
  for (int i = 0; i < extension_count(); i++) {
    extension(i)->CopyJsonNameTo(proto->mutable_extension(i));
  }
}

void FieldDescriptor::CopyTo(FieldDescriptorProto* proto) const {
  proto->set_name(name());
  proto->set_number(number());
  if (has_json_name_) {
    proto->set_json_name(json_name());
  }
  if (proto3_optional_) {
    proto->set_proto3_optional(true);
  }
  // Some compilers do not allow static_cast directly between two enum types,
  // so we must cast to int first.
  if (is_required() && !IsLegacyEdition(file()->edition())) {
    // Editions files have no required keyword, and we only set this label
    // during descriptor build.
    proto->set_label(static_cast<FieldDescriptorProto::Label>(
        absl::implicit_cast<int>(LABEL_OPTIONAL)));
  } else {
    proto->set_label(static_cast<FieldDescriptorProto::Label>(
        absl::implicit_cast<int>(static_cast<Label>(label_))));
  }
  if (type() == TYPE_GROUP && !IsLegacyEdition(file()->edition())) {
    // Editions files have no group keyword, and we only set this label
    // during descriptor build.
    proto->set_type(static_cast<FieldDescriptorProto::Type>(
        absl::implicit_cast<int>(TYPE_MESSAGE)));
  } else {
    proto->set_type(static_cast<FieldDescriptorProto::Type>(
        absl::implicit_cast<int>(type())));
  }

  if (is_extension()) {
    if (!containing_type()->is_unqualified_placeholder_) {
      proto->set_extendee(".");
    }
    absl::StrAppend(proto->mutable_extendee(), containing_type()->full_name());
  }

  if (cpp_type() == CPPTYPE_MESSAGE) {
    if (message_type()->is_placeholder_) {
      // We don't actually know if the type is a message type.  It could be
      // an enum.
      proto->clear_type();
    }

    if (!message_type()->is_unqualified_placeholder_) {
      proto->set_type_name(".");
    }
    absl::StrAppend(proto->mutable_type_name(), message_type()->full_name());
  } else if (cpp_type() == CPPTYPE_ENUM) {
    if (!enum_type()->is_unqualified_placeholder_) {
      proto->set_type_name(".");
    }
    absl::StrAppend(proto->mutable_type_name(), enum_type()->full_name());
  }

  if (has_default_value()) {
    proto->set_default_value(DefaultValueAsString(false));
  }

  if (containing_oneof() != nullptr && !is_extension()) {
    proto->set_oneof_index(containing_oneof()->index());
  }

  if (&options() != &FieldOptions::default_instance()) {
    *proto->mutable_options() = options();
  }
  if (has_legacy_proto_ctype()) {
    proto->mutable_options()->set_ctype(
        static_cast<FieldOptions::CType>(legacy_proto_ctype()));
  }

  RestoreFeaturesToOptions(proto_features_, proto);
}

void FieldDescriptor::CopyJsonNameTo(FieldDescriptorProto* proto) const {
  proto->set_json_name(json_name());
}

void OneofDescriptor::CopyTo(OneofDescriptorProto* proto) const {
  proto->set_name(name());
  if (&options() != &OneofOptions::default_instance()) {
    *proto->mutable_options() = options();
  }
  RestoreFeaturesToOptions(proto_features_, proto);
}

void EnumDescriptor::CopyTo(EnumDescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < value_count(); i++) {
    value(i)->CopyTo(proto->add_value());
  }
  for (int i = 0; i < reserved_range_count(); i++) {
    EnumDescriptorProto::EnumReservedRange* range = proto->add_reserved_range();
    range->set_start(reserved_range(i)->start);
    range->set_end(reserved_range(i)->end);
  }
  for (int i = 0; i < reserved_name_count(); i++) {
    proto->add_reserved_name(reserved_name(i));
  }

  if (visibility_keyword() != SymbolVisibility::VISIBILITY_UNSET) {
    proto->set_visibility(visibility_keyword());
  }

  if (&options() != &EnumOptions::default_instance()) {
    *proto->mutable_options() = options();
  }
  RestoreFeaturesToOptions(proto_features_, proto);
}

void EnumValueDescriptor::CopyTo(EnumValueDescriptorProto* proto) const {
  proto->set_name(name());
  proto->set_number(number());

  if (&options() != &EnumValueOptions::default_instance()) {
    *proto->mutable_options() = options();
  }
  RestoreFeaturesToOptions(proto_features_, proto);
}

void ServiceDescriptor::CopyTo(ServiceDescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < method_count(); i++) {
    method(i)->CopyTo(proto->add_method());
  }

  if (&options() != &ServiceOptions::default_instance()) {
    *proto->mutable_options() = options();
  }
  RestoreFeaturesToOptions(proto_features_, proto);
}

void MethodDescriptor::CopyTo(MethodDescriptorProto* proto) const {
  proto->set_name(name());

  if (!input_type()->is_unqualified_placeholder_) {
    proto->set_input_type(".");
  }
  absl::StrAppend(proto->mutable_input_type(), input_type()->full_name());

  if (!output_type()->is_unqualified_placeholder_) {
    proto->set_output_type(".");
  }
  absl::StrAppend(proto->mutable_output_type(), output_type()->full_name());

  if (&options() != &MethodOptions::default_instance()) {
    *proto->mutable_options() = options();
  }

  if (client_streaming_) {
    proto->set_client_streaming(true);
  }
  if (server_streaming_) {
    proto->set_server_streaming(true);
  }
  RestoreFeaturesToOptions(proto_features_, proto);
}

// DebugString methods ===============================================

namespace {

bool IsGroupSyntax(Edition edition, const FieldDescriptor* desc) {
  return IsLegacyEdition(edition) &&
         desc->type() == FieldDescriptor::TYPE_GROUP;
}

template <typename OptionsT>
void CopyFeaturesToOptions(const FeatureSet* features, OptionsT* options) {
  if (features != &FeatureSet::default_instance()) {
    *options->mutable_features() = *features;
  }
}

bool RetrieveOptionsAssumingRightPool(
    int depth, const Message& options,
    std::vector<std::string>* option_entries) {
  option_entries->clear();
  const Reflection* reflection = options.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(options, &fields);
  for (const FieldDescriptor* field : fields) {
    int count = 1;
    bool repeated = false;
    if (field->is_repeated()) {
      count = reflection->FieldSize(options, field);
      repeated = true;
    }
    for (int j = 0; j < count; j++) {
      std::string fieldval;
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        std::string tmp;
        TextFormat::Printer printer;
        printer.SetExpandAny(true);
        printer.SetInitialIndentLevel(depth + 1);
        printer.PrintFieldValueToString(options, field, repeated ? j : -1,
                                        &tmp);
        fieldval.append("{\n");
        fieldval.append(tmp);
        fieldval.append(depth * 2, ' ');
        fieldval.append("}");
      } else {
        TextFormat::PrintFieldValueToString(options, field, repeated ? j : -1,
                                            &fieldval);
      }
      std::string name;
      if (field->is_extension()) {
        name = absl::StrCat("(.", field->full_name(), ")");
      } else {
        name = std::string(field->name());
      }
      option_entries->push_back(absl::StrCat(name, " = ", fieldval));
    }
  }
  return !option_entries->empty();
}

// Used by each of the option formatters.
bool RetrieveOptions(int depth, const Message& options,
                     const DescriptorPool* pool,
                     std::vector<std::string>* option_entries) {
  // When printing custom options for a descriptor, we must use an options
  // message built on top of the same DescriptorPool where the descriptor
  // is coming from. This is to ensure we are interpreting custom options
  // against the right pool.
  if (options.GetDescriptor()->file()->pool() == pool) {
    return RetrieveOptionsAssumingRightPool(depth, options, option_entries);
  } else {
    const Descriptor* option_descriptor =
        pool->FindMessageTypeByName(options.GetDescriptor()->full_name());
    if (option_descriptor == nullptr) {
      // descriptor.proto is not in the pool. This means no custom options are
      // used so we are safe to proceed with the compiled options message type.
      return RetrieveOptionsAssumingRightPool(depth, options, option_entries);
    }
    DynamicMessageFactory factory;
    std::unique_ptr<Message> dynamic_options(
        factory.GetPrototype(option_descriptor)->New());
    std::string serialized = options.SerializeAsString();
    io::CodedInputStream input(
        reinterpret_cast<const uint8_t*>(serialized.data()), serialized.size());
    input.SetExtensionRegistry(pool, &factory);
    if (dynamic_options->ParseFromCodedStream(&input)) {
      return RetrieveOptionsAssumingRightPool(depth, *dynamic_options,
                                              option_entries);
    } else {
      ABSL_LOG(ERROR) << "Found invalid proto option data for: "
                      << options.GetDescriptor()->full_name();
      return RetrieveOptionsAssumingRightPool(depth, options, option_entries);
    }
  }
}

// Formats options that all appear together in brackets. Does not include
// brackets.
bool FormatBracketedOptions(int depth, const Message& options,
                            const DescriptorPool* pool, std::string* output) {
  std::vector<std::string> all_options;
  if (RetrieveOptions(depth, options, pool, &all_options)) {
    output->append(absl::StrJoin(all_options, ", "));
  }
  return !all_options.empty();
}

// Formats options one per line
bool FormatLineOptions(int depth, const Message& options,
                       const DescriptorPool* pool, std::string* output) {
  std::string prefix(depth * 2, ' ');
  std::vector<std::string> all_options;
  if (RetrieveOptions(depth, options, pool, &all_options)) {
    for (const std::string& option : all_options) {
      absl::SubstituteAndAppend(output, "$0option $1;\n", prefix, option);
    }
  }
  return !all_options.empty();
}

static std::string GetLegacySyntaxName(Edition edition) {
  if (edition == Edition::EDITION_PROTO3) {
    return "proto3";
  }
  return "proto2";
}


class SourceLocationCommentPrinter {
 public:
  template <typename DescType>
  SourceLocationCommentPrinter(const DescType* desc, const std::string& prefix,
                               const DebugStringOptions& options)
      : options_(options), prefix_(prefix) {
    // Perform the SourceLocation lookup only if we're including user comments,
    // because the lookup is fairly expensive.
    have_source_loc_ =
        options.include_comments && desc->GetSourceLocation(&source_loc_);
  }
  SourceLocationCommentPrinter(const FileDescriptor* file,
                               const std::vector<int>& path,
                               const std::string& prefix,
                               const DebugStringOptions& options)
      : options_(options), prefix_(prefix) {
    // Perform the SourceLocation lookup only if we're including user comments,
    // because the lookup is fairly expensive.
    have_source_loc_ =
        options.include_comments && file->GetSourceLocation(path, &source_loc_);
  }
  void AddPreComment(std::string* output) {
    if (have_source_loc_) {
      // Detached leading comments.
      for (const std::string& leading_detached_comment :
           source_loc_.leading_detached_comments) {
        absl::StrAppend(output, FormatComment(leading_detached_comment), "\n");
      }
      // Attached leading comments.
      if (!source_loc_.leading_comments.empty()) {
        absl::StrAppend(output, FormatComment(source_loc_.leading_comments));
      }
    }
  }
  void AddPostComment(std::string* output) {
    if (have_source_loc_ && source_loc_.trailing_comments.size() > 0) {
      absl::StrAppend(output, FormatComment(source_loc_.trailing_comments));
    }
  }

  // Format comment such that each line becomes a full-line C++-style comment in
  // the DebugString() output.
  std::string FormatComment(const std::string& comment_text) {
    std::string stripped_comment = comment_text;
    absl::StripAsciiWhitespace(&stripped_comment);
    std::string output;
    for (absl::string_view line : absl::StrSplit(stripped_comment, '\n')) {
      absl::SubstituteAndAppend(&output, "$0// $1\n", prefix_, line);
    }
    return output;
  }

 private:

  bool have_source_loc_;
  SourceLocation source_loc_;
  DebugStringOptions options_;
  std::string prefix_;
};

}  // anonymous namespace

std::string FileDescriptor::DebugString() const {
  DebugStringOptions options;  // default options
  return DebugStringWithOptions(options);
}

std::string FileDescriptor::DebugStringWithOptions(
    const DebugStringOptions& debug_string_options) const {
  std::string contents;
  {
    std::vector<int> path;
    path.push_back(FileDescriptorProto::kSyntaxFieldNumber);
    SourceLocationCommentPrinter syntax_comment(this, path, "",
                                                debug_string_options);
    syntax_comment.AddPreComment(&contents);
    if (IsLegacyEdition(edition())) {
      absl::SubstituteAndAppend(&contents, "syntax = \"$0\";\n\n",
                                GetLegacySyntaxName(edition()));
    } else {
      absl::SubstituteAndAppend(&contents, "edition = \"$0\";\n\n", edition());
    }
    syntax_comment.AddPostComment(&contents);
  }

  SourceLocationCommentPrinter comment_printer(this, "", debug_string_options);
  comment_printer.AddPreComment(&contents);

  absl::flat_hash_set<int> public_dependencies(
      public_dependencies_, public_dependencies_ + public_dependency_count_);
  absl::flat_hash_set<int> weak_dependencies(
      weak_dependencies_, weak_dependencies_ + weak_dependency_count_);

  for (int i = 0; i < dependency_count(); i++) {
    if (public_dependencies.contains(i)) {
      absl::SubstituteAndAppend(&contents, "import public \"$0\";\n",
                                dependency(i)->name());
    } else if (weak_dependencies.contains(i)) {
      absl::SubstituteAndAppend(&contents, "import weak \"$0\";\n",
                                dependency(i)->name());
    } else {
      absl::SubstituteAndAppend(&contents, "import \"$0\";\n",
                                dependency(i)->name());
    }
  }
  for (int i = 0; i < option_dependency_count(); i++) {
    absl::SubstituteAndAppend(&contents, "import option \"$0\";\n",
                              option_dependency_name(i));
  }

  if (!package().empty()) {
    std::vector<int> path;
    path.push_back(FileDescriptorProto::kPackageFieldNumber);
    SourceLocationCommentPrinter package_comment(this, path, "",
                                                 debug_string_options);
    package_comment.AddPreComment(&contents);
    absl::SubstituteAndAppend(&contents, "package $0;\n\n", package());
    package_comment.AddPostComment(&contents);
  }

  FileOptions full_options = options();
  CopyFeaturesToOptions(proto_features_, &full_options);
  if (FormatLineOptions(0, full_options, pool(), &contents)) {
    contents.append("\n");  // add some space if we had options
  }

  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->DebugString(0, &contents, debug_string_options);
    contents.append("\n");
  }

  // Find all the 'group' type extensions; we will not output their nested
  // definitions (those will be done with their group field descriptor).
  absl::flat_hash_set<const Descriptor*> groups;
  for (int i = 0; i < extension_count(); i++) {
    if (IsGroupSyntax(edition(), extension(i))) {
      groups.insert(extension(i)->message_type());
    }
  }

  for (int i = 0; i < message_type_count(); i++) {
    if (!groups.contains(message_type(i))) {
      message_type(i)->DebugString(0, &contents, debug_string_options,
                                   /* include_opening_clause */ true);
      contents.append("\n");
    }
  }

  for (int i = 0; i < service_count(); i++) {
    service(i)->DebugString(&contents, debug_string_options);
    contents.append("\n");
  }

  const Descriptor* containing_type = nullptr;
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->containing_type() != containing_type) {
      if (i > 0) contents.append("}\n\n");
      containing_type = extension(i)->containing_type();
      absl::SubstituteAndAppend(&contents, "extend .$0 {\n",
                                containing_type->full_name());
    }
    extension(i)->DebugString(1, &contents, debug_string_options);
  }
  if (extension_count() > 0) contents.append("}\n\n");

  comment_printer.AddPostComment(&contents);

  return contents;
}

std::string Descriptor::DebugString() const {
  DebugStringOptions options;  // default options
  return DebugStringWithOptions(options);
}

std::string Descriptor::DebugStringWithOptions(
    const DebugStringOptions& options) const {
  std::string contents;
  DebugString(0, &contents, options, /* include_opening_clause */ true);
  return contents;
}

namespace {

std::string VisibilityToKeyword(const SymbolVisibility& visibility) {
  switch (visibility) {
    case SymbolVisibility::VISIBILITY_EXPORT:
      return "export ";
      break;
    case SymbolVisibility::VISIBILITY_LOCAL:
      return "local ";
      break;
    default:
      break;
  }
  return "";
}

}  // namespace

void Descriptor::DebugString(int depth, std::string* contents,
                             const DebugStringOptions& debug_string_options,
                             bool include_opening_clause) const {
  if (options().map_entry()) {
    // Do not generate debug string for auto-generated map-entry type.
    return;
  }
  std::string prefix(depth * 2, ' ');
  ++depth;

  SourceLocationCommentPrinter comment_printer(this, prefix,
                                               debug_string_options);
  comment_printer.AddPreComment(contents);

  if (include_opening_clause) {
    absl::SubstituteAndAppend(contents, "$0$1message $2", prefix,
                              VisibilityToKeyword(visibility_keyword()),
                              name());
  }
  contents->append(" {\n");

  MessageOptions full_options = options();
  CopyFeaturesToOptions(proto_features_, &full_options);
  FormatLineOptions(depth, full_options, file()->pool(), contents);

  // Find all the 'group' types for fields and extensions; we will not output
  // their nested definitions (those will be done with their group field
  // descriptor).
  absl::flat_hash_set<const Descriptor*> groups;
  for (int i = 0; i < field_count(); i++) {
    if (IsGroupSyntax(file()->edition(), field(i))) {
      groups.insert(field(i)->message_type());
    }
  }
  for (int i = 0; i < extension_count(); i++) {
    if (IsGroupSyntax(file()->edition(), extension(i))) {
      groups.insert(extension(i)->message_type());
    }
  }

  for (int i = 0; i < nested_type_count(); i++) {
    if (!groups.contains(nested_type(i))) {
      nested_type(i)->DebugString(depth, contents, debug_string_options,
                                  /* include_opening_clause */ true);
    }
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->DebugString(depth, contents, debug_string_options);
  }
  for (int i = 0; i < field_count(); i++) {
    if (field(i)->real_containing_oneof() == nullptr) {
      field(i)->DebugString(depth, contents, debug_string_options);
    } else if (field(i)->containing_oneof()->field(0) == field(i)) {
      // This is the first field in this oneof, so print the whole oneof.
      field(i)->containing_oneof()->DebugString(depth, contents,
                                                debug_string_options);
    }
  }

  for (int i = 0; i < extension_range_count(); i++) {
    absl::SubstituteAndAppend(contents, "$0  extensions $1", prefix,
                              extension_range(i)->start_number());
    if (extension_range(i)->end_number() >
        extension_range(i)->start_number() + 1) {
      absl::SubstituteAndAppend(contents, " to $0",
                                extension_range(i)->end_number() - 1);
    }
    ExtensionRangeOptions range_options = extension_range(i)->options();
    CopyFeaturesToOptions(extension_range(i)->proto_features_, &range_options);
    std::string formatted_options;
    if (FormatBracketedOptions(depth, range_options, file()->pool(),
                               &formatted_options)) {
      absl::StrAppend(contents, " [", formatted_options, "]");
    }
    absl::StrAppend(contents, ";\n");
  }

  // Group extensions by what they extend, so they can be printed out together.
  const Descriptor* containing_type = nullptr;
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->containing_type() != containing_type) {
      if (i > 0) absl::SubstituteAndAppend(contents, "$0  }\n", prefix);
      containing_type = extension(i)->containing_type();
      absl::SubstituteAndAppend(contents, "$0  extend .$1 {\n", prefix,
                                containing_type->full_name());
    }
    extension(i)->DebugString(depth + 1, contents, debug_string_options);
  }
  if (extension_count() > 0)
    absl::SubstituteAndAppend(contents, "$0  }\n", prefix);

  if (reserved_range_count() > 0) {
    absl::SubstituteAndAppend(contents, "$0  reserved ", prefix);
    for (int i = 0; i < reserved_range_count(); i++) {
      const Descriptor::ReservedRange* range = reserved_range(i);
      if (range->end == range->start + 1) {
        absl::SubstituteAndAppend(contents, "$0, ", range->start);
      } else if (range->end > FieldDescriptor::kMaxNumber) {
        absl::SubstituteAndAppend(contents, "$0 to max, ", range->start);
      } else {
        absl::SubstituteAndAppend(contents, "$0 to $1, ", range->start,
                                  range->end - 1);
      }
    }
    contents->replace(contents->size() - 2, 2, ";\n");
  }

  if (reserved_name_count() > 0) {
    absl::SubstituteAndAppend(contents, "$0  reserved ", prefix);
    for (int i = 0; i < reserved_name_count(); i++) {
      absl::SubstituteAndAppend(
          contents,
          file()->edition() < Edition::EDITION_2023 ? "\"$0\", " : "$0, ",
          absl::CEscape(reserved_name(i)));
    }
    contents->replace(contents->size() - 2, 2, ";\n");
  }

  absl::SubstituteAndAppend(contents, "$0}\n", prefix);
  comment_printer.AddPostComment(contents);
}

std::string FieldDescriptor::DebugString() const {
  DebugStringOptions options;  // default options
  return DebugStringWithOptions(options);
}

std::string FieldDescriptor::DebugStringWithOptions(
    const DebugStringOptions& debug_string_options) const {
  std::string contents;
  int depth = 0;
  if (is_extension()) {
    absl::SubstituteAndAppend(&contents, "extend .$0 {\n",
                              containing_type()->full_name());
    depth = 1;
  }
  DebugString(depth, &contents, debug_string_options);
  if (is_extension()) {
    contents.append("}\n");
  }
  return contents;
}

// The field type string used in FieldDescriptor::DebugString()
std::string FieldDescriptor::FieldTypeNameDebugString() const {
  switch (type()) {
    case TYPE_MESSAGE:
    case TYPE_GROUP:
      if (IsGroupSyntax(file()->edition(), this)) {
        return kTypeToName[type()];
      }
      return absl::StrCat(".", message_type()->full_name());
    case TYPE_ENUM:
      return absl::StrCat(".", enum_type()->full_name());
    default:
      return kTypeToName[type()];
  }
}

void FieldDescriptor::DebugString(
    int depth, std::string* contents,
    const DebugStringOptions& debug_string_options) const {
  std::string prefix(depth * 2, ' ');
  std::string field_type;

  // Special case map fields.
  if (is_map()) {
    absl::SubstituteAndAppend(
        &field_type, "map<$0, $1>",
        message_type()->field(0)->FieldTypeNameDebugString(),
        message_type()->field(1)->FieldTypeNameDebugString());
  } else {
    field_type = FieldTypeNameDebugString();
  }

  std::string label =
      absl::StrCat(kLabelToName[static_cast<Label>(label_)], " ");

  // Label is omitted for maps, oneof, and plain proto3 fields.
  if (is_map() || real_containing_oneof() ||
      (!is_required() && !is_repeated() && !has_optional_keyword())) {
    label.clear();
  }
  // Label is omitted for optional and required fields under editions.
  if (!is_repeated() && !IsLegacyEdition(file()->edition())) {
    label.clear();
  }

  SourceLocationCommentPrinter comment_printer(this, prefix,
                                               debug_string_options);
  comment_printer.AddPreComment(contents);

  absl::SubstituteAndAppend(
      contents, "$0$1$2 $3 = $4", prefix, label, field_type,
      IsGroupSyntax(file()->edition(), this) ? message_type()->name() : name(),
      number());

  bool bracketed = false;
  if (has_default_value()) {
    bracketed = true;
    absl::SubstituteAndAppend(contents, " [default = $0",
                              DefaultValueAsString(true));
  }
  if (has_json_name_) {
    if (!bracketed) {
      bracketed = true;
      contents->append(" [");
    } else {
      contents->append(", ");
    }
    contents->append("json_name = \"");
    contents->append(absl::CEscape(json_name()));
    contents->append("\"");
  }

  FieldOptions full_options = options();
  CopyFeaturesToOptions(proto_features_, &full_options);
  if (has_legacy_proto_ctype()) {
    full_options.set_ctype(
        static_cast<FieldOptions::CType>(legacy_proto_ctype()));
  }
  std::string formatted_options;
  if (FormatBracketedOptions(depth, full_options, file()->pool(),
                             &formatted_options)) {
    contents->append(bracketed ? ", " : " [");
    bracketed = true;
    contents->append(formatted_options);
  }

  if (bracketed) {
    contents->append("]");
  }

  if (IsGroupSyntax(file()->edition(), this)) {
    if (debug_string_options.elide_group_body) {
      contents->append(" { ... };\n");
    } else {
      message_type()->DebugString(depth, contents, debug_string_options,
                                  /* include_opening_clause */ false);
    }
  } else {
    contents->append(";\n");
  }

  comment_printer.AddPostComment(contents);
}

std::string OneofDescriptor::DebugString() const {
  DebugStringOptions options;  // default values
  return DebugStringWithOptions(options);
}

std::string OneofDescriptor::DebugStringWithOptions(
    const DebugStringOptions& options) const {
  std::string contents;
  DebugString(0, &contents, options);
  return contents;
}

void OneofDescriptor::DebugString(
    int depth, std::string* contents,
    const DebugStringOptions& debug_string_options) const {
  std::string prefix(depth * 2, ' ');
  ++depth;
  SourceLocationCommentPrinter comment_printer(this, prefix,
                                               debug_string_options);
  comment_printer.AddPreComment(contents);
  absl::SubstituteAndAppend(contents, "$0oneof $1 {", prefix, name());

  OneofOptions full_options = options();
  CopyFeaturesToOptions(proto_features_, &full_options);
  FormatLineOptions(depth, full_options, containing_type()->file()->pool(),
                    contents);

  if (debug_string_options.elide_oneof_body) {
    contents->append(" ... }\n");
  } else {
    contents->append("\n");
    for (int i = 0; i < field_count(); i++) {
      field(i)->DebugString(depth, contents, debug_string_options);
    }
    absl::SubstituteAndAppend(contents, "$0}\n", prefix);
  }
  comment_printer.AddPostComment(contents);
}

std::string EnumDescriptor::DebugString() const {
  DebugStringOptions options;  // default values
  return DebugStringWithOptions(options);
}

std::string EnumDescriptor::DebugStringWithOptions(
    const DebugStringOptions& options) const {
  std::string contents;
  DebugString(0, &contents, options);
  return contents;
}

void EnumDescriptor::DebugString(
    int depth, std::string* contents,
    const DebugStringOptions& debug_string_options) const {
  std::string prefix(depth * 2, ' ');
  ++depth;

  SourceLocationCommentPrinter comment_printer(this, prefix,
                                               debug_string_options);
  comment_printer.AddPreComment(contents);

  absl::SubstituteAndAppend(contents, "$0$1enum $2 {\n", prefix,
                            VisibilityToKeyword(visibility_keyword()), name());

  EnumOptions full_options = options();
  CopyFeaturesToOptions(proto_features_, &full_options);
  FormatLineOptions(depth, full_options, file()->pool(), contents);

  for (int i = 0; i < value_count(); i++) {
    value(i)->DebugString(depth, contents, debug_string_options);
  }

  if (reserved_range_count() > 0) {
    absl::SubstituteAndAppend(contents, "$0  reserved ", prefix);
    for (int i = 0; i < reserved_range_count(); i++) {
      const EnumDescriptor::ReservedRange* range = reserved_range(i);
      if (range->end == range->start) {
        absl::SubstituteAndAppend(contents, "$0, ", range->start);
      } else if (range->end == INT_MAX) {
        absl::SubstituteAndAppend(contents, "$0 to max, ", range->start);
      } else {
        absl::SubstituteAndAppend(contents, "$0 to $1, ", range->start,
                                  range->end);
      }
    }
    contents->replace(contents->size() - 2, 2, ";\n");
  }

  if (reserved_name_count() > 0) {
    absl::SubstituteAndAppend(contents, "$0  reserved ", prefix);
    for (int i = 0; i < reserved_name_count(); i++) {
      absl::SubstituteAndAppend(
          contents,
          file()->edition() < Edition::EDITION_2023 ? "\"$0\", " : "$0, ",
          absl::CEscape(reserved_name(i)));
    }
    contents->replace(contents->size() - 2, 2, ";\n");
  }

  absl::SubstituteAndAppend(contents, "$0}\n", prefix);

  comment_printer.AddPostComment(contents);
}

std::string EnumValueDescriptor::DebugString() const {
  DebugStringOptions options;  // default values
  return DebugStringWithOptions(options);
}

std::string EnumValueDescriptor::DebugStringWithOptions(
    const DebugStringOptions& options) const {
  std::string contents;
  DebugString(0, &contents, options);
  return contents;
}

void EnumValueDescriptor::DebugString(
    int depth, std::string* contents,
    const DebugStringOptions& debug_string_options) const {
  std::string prefix(depth * 2, ' ');

  SourceLocationCommentPrinter comment_printer(this, prefix,
                                               debug_string_options);
  comment_printer.AddPreComment(contents);

  absl::SubstituteAndAppend(contents, "$0$1 = $2", prefix, name(), number());

  EnumValueOptions full_options = options();
  CopyFeaturesToOptions(proto_features_, &full_options);
  std::string formatted_options;
  if (FormatBracketedOptions(depth, full_options, type()->file()->pool(),
                             &formatted_options)) {
    absl::SubstituteAndAppend(contents, " [$0]", formatted_options);
  }
  contents->append(";\n");

  comment_printer.AddPostComment(contents);
}

std::string ServiceDescriptor::DebugString() const {
  DebugStringOptions options;  // default values
  return DebugStringWithOptions(options);
}

std::string ServiceDescriptor::DebugStringWithOptions(
    const DebugStringOptions& options) const {
  std::string contents;
  DebugString(&contents, options);
  return contents;
}

void ServiceDescriptor::DebugString(
    std::string* contents,
    const DebugStringOptions& debug_string_options) const {
  SourceLocationCommentPrinter comment_printer(this, /* prefix */ "",
                                               debug_string_options);
  comment_printer.AddPreComment(contents);

  absl::SubstituteAndAppend(contents, "service $0 {\n", name());

  ServiceOptions full_options = options();
  CopyFeaturesToOptions(proto_features_, &full_options);
  FormatLineOptions(1, full_options, file()->pool(), contents);

  for (int i = 0; i < method_count(); i++) {
    method(i)->DebugString(1, contents, debug_string_options);
  }

  contents->append("}\n");

  comment_printer.AddPostComment(contents);
}

std::string MethodDescriptor::DebugString() const {
  DebugStringOptions options;  // default values
  return DebugStringWithOptions(options);
}

std::string MethodDescriptor::DebugStringWithOptions(
    const DebugStringOptions& options) const {
  std::string contents;
  DebugString(0, &contents, options);
  return contents;
}

void MethodDescriptor::DebugString(
    int depth, std::string* contents,
    const DebugStringOptions& debug_string_options) const {
  std::string prefix(depth * 2, ' ');
  ++depth;

  SourceLocationCommentPrinter comment_printer(this, prefix,
                                               debug_string_options);
  comment_printer.AddPreComment(contents);

  absl::SubstituteAndAppend(
      contents, "$0rpc $1($4.$2) returns ($5.$3)", prefix, name(),
      input_type()->full_name(), output_type()->full_name(),
      client_streaming() ? "stream " : "", server_streaming() ? "stream " : "");

  MethodOptions full_options = options();
  CopyFeaturesToOptions(proto_features_, &full_options);
  std::string formatted_options;
  if (FormatLineOptions(depth, full_options, service()->file()->pool(),
                        &formatted_options)) {
    absl::SubstituteAndAppend(contents, " {\n$0$1}\n", formatted_options,
                              prefix);
  } else {
    contents->append(";\n");
  }

  comment_printer.AddPostComment(contents);
}

// Feature methods ===============================================

bool FieldDescriptor::has_legacy_proto_ctype() const {
  return legacy_proto_ctype_ <= FieldOptions::CType_MAX;
}

bool EnumDescriptor::is_closed() const {
  return features().enum_type() == FeatureSet::CLOSED;
}

bool FieldDescriptor::is_packed() const {
  if (!is_packable()) return false;
  return features().repeated_field_encoding() == FeatureSet::PACKED;
}

static bool IsStrictUtf8(const FieldDescriptor* field) {
  return internal::InternalFeatureHelper::GetFeatures(*field)
             .utf8_validation() == FeatureSet::VERIFY;
}

bool FieldDescriptor::requires_utf8_validation() const {
  return type() == TYPE_STRING && IsStrictUtf8(this);
}

bool FieldDescriptor::has_presence() const {
  if (is_repeated()) return false;
  return cpp_type() == CPPTYPE_MESSAGE || is_extension() ||
         containing_oneof() ||
         features().field_presence() != FeatureSet::IMPLICIT;
}

bool FieldDescriptor::is_required() const {
  return features().field_presence() == FeatureSet::LEGACY_REQUIRED;
}

bool FieldDescriptor::legacy_enum_field_treated_as_closed() const {
  return type() == TYPE_ENUM &&
         (features().GetExtension(pb::cpp).legacy_closed_enum() ||
          enum_type()->is_closed());
}

bool FieldDescriptor::has_optional_keyword() const {
  return proto3_optional_ ||
         (file()->edition() == Edition::EDITION_PROTO2 && !is_required() &&
          !is_repeated() && !containing_oneof());
}

FieldDescriptor::CppStringType FieldDescriptor::CalculateCppStringType() const {
  ABSL_DCHECK(cpp_type() == FieldDescriptor::CPPTYPE_STRING);

  if (internal::cpp::IsStringFieldWithPrivatizedAccessors(*this)) {
    return CppStringType::kString;
  }

  switch (features().GetExtension(pb::cpp).string_type()) {
    case pb::CppFeatures::VIEW:
      return CppStringType::kView;
    case pb::CppFeatures::CORD:
      return CppStringType::kCord;
    case pb::CppFeatures::STRING:
      return CppStringType::kString;
    default:
      // If features haven't been resolved, this is a dynamic build not for C++
      // codegen.  Just use string type.
      ABSL_DCHECK(!features().GetExtension(pb::cpp).has_string_type());
      return CppStringType::kString;
  }
}

// Location methods ===============================================

bool FileDescriptor::GetSourceLocation(const std::vector<int>& path,
                                       SourceLocation* out_location) const {
  ABSL_CHECK(out_location != nullptr);
  if (source_code_info_) {
    if (const SourceCodeInfo_Location* loc =
            tables_->GetSourceLocation(path, source_code_info_)) {
      const RepeatedField<int32_t>& span = loc->span();
      if (span.size() == 3 || span.size() == 4) {
        out_location->start_line = span.Get(0);
        out_location->start_column = span.Get(1);
        out_location->end_line = span.Get(span.size() == 3 ? 0 : 2);
        out_location->end_column = span.Get(span.size() - 1);

        out_location->leading_comments = loc->leading_comments();
        out_location->trailing_comments = loc->trailing_comments();
        out_location->leading_detached_comments.assign(
            loc->leading_detached_comments().begin(),
            loc->leading_detached_comments().end());
        return true;
      }
    }
  }
  return false;
}

bool FileDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  std::vector<int> path;  // empty path for root FileDescriptor
  return GetSourceLocation(path, out_location);
}

bool Descriptor::GetSourceLocation(SourceLocation* out_location) const {
  std::vector<int> path;
  GetLocationPath(&path);
  return file()->GetSourceLocation(path, out_location);
}

bool FieldDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  std::vector<int> path;
  GetLocationPath(&path);
  return file()->GetSourceLocation(path, out_location);
}

bool OneofDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  std::vector<int> path;
  GetLocationPath(&path);
  return containing_type()->file()->GetSourceLocation(path, out_location);
}

bool EnumDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  std::vector<int> path;
  GetLocationPath(&path);
  return file()->GetSourceLocation(path, out_location);
}

bool MethodDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  std::vector<int> path;
  GetLocationPath(&path);
  return service()->file()->GetSourceLocation(path, out_location);
}

bool ServiceDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  std::vector<int> path;
  GetLocationPath(&path);
  return file()->GetSourceLocation(path, out_location);
}

bool EnumValueDescriptor::GetSourceLocation(
    SourceLocation* out_location) const {
  std::vector<int> path;
  GetLocationPath(&path);
  return type()->file()->GetSourceLocation(path, out_location);
}

void Descriptor::GetLocationPath(std::vector<int>* output) const {
  if (containing_type()) {
    containing_type()->GetLocationPath(output);
    output->push_back(DescriptorProto::kNestedTypeFieldNumber);
    output->push_back(index());
  } else {
    output->push_back(FileDescriptorProto::kMessageTypeFieldNumber);
    output->push_back(index());
  }
}

void FieldDescriptor::GetLocationPath(std::vector<int>* output) const {
  if (is_extension()) {
    if (extension_scope() == nullptr) {
      output->push_back(FileDescriptorProto::kExtensionFieldNumber);
      output->push_back(index());
    } else {
      extension_scope()->GetLocationPath(output);
      output->push_back(DescriptorProto::kExtensionFieldNumber);
      output->push_back(index());
    }
  } else {
    containing_type()->GetLocationPath(output);
    output->push_back(DescriptorProto::kFieldFieldNumber);
    output->push_back(index());
  }
}

void OneofDescriptor::GetLocationPath(std::vector<int>* output) const {
  containing_type()->GetLocationPath(output);
  output->push_back(DescriptorProto::kOneofDeclFieldNumber);
  output->push_back(index());
}

void Descriptor::ExtensionRange::GetLocationPath(
    std::vector<int>* output) const {
  containing_type()->GetLocationPath(output);
  output->push_back(DescriptorProto::kExtensionRangeFieldNumber);
  output->push_back(index());
}

void EnumDescriptor::GetLocationPath(std::vector<int>* output) const {
  if (containing_type()) {
    containing_type()->GetLocationPath(output);
    output->push_back(DescriptorProto::kEnumTypeFieldNumber);
    output->push_back(index());
  } else {
    output->push_back(FileDescriptorProto::kEnumTypeFieldNumber);
    output->push_back(index());
  }
}

void EnumValueDescriptor::GetLocationPath(std::vector<int>* output) const {
  type()->GetLocationPath(output);
  output->push_back(EnumDescriptorProto::kValueFieldNumber);
  output->push_back(index());
}

void ServiceDescriptor::GetLocationPath(std::vector<int>* output) const {
  output->push_back(FileDescriptorProto::kServiceFieldNumber);
  output->push_back(index());
}

void MethodDescriptor::GetLocationPath(std::vector<int>* output) const {
  service()->GetLocationPath(output);
  output->push_back(ServiceDescriptorProto::kMethodFieldNumber);
  output->push_back(index());
}

// ===================================================================

namespace {

// Represents an options message to interpret. Extension names in the option
// name are resolved relative to name_scope. element_name and orig_opt are
// used only for error reporting (since the parser records locations against
// pointers in the original options, not the mutable copy). The Message must be
// one of the Options messages in descriptor.proto.
struct OptionsToInterpret {
  OptionsToInterpret(absl::string_view ns, absl::string_view el,
                     absl::Span<const int> path, const Message* orig_opt,
                     Message* opt)
      : name_scope(ns),
        element_name(el),
        element_path(path.begin(), path.end()),
        original_options(orig_opt),
        options(opt) {}
  std::string name_scope;
  std::string element_name;
  std::vector<int> element_path;
  const Message* original_options;
  Message* options;
};

}  // namespace

class DescriptorBuilder {
 public:
  static std::unique_ptr<DescriptorBuilder> New(
      const DescriptorPool* pool, DescriptorPool::Tables* tables,
      DescriptorPool::DeferredValidation& deferred_validation,
      DescriptorPool::ErrorCollector* error_collector) {
    return std::unique_ptr<DescriptorBuilder>(new DescriptorBuilder(
        pool, tables, deferred_validation, error_collector));
  }

  ~DescriptorBuilder();

  const FileDescriptor* BuildFile(const FileDescriptorProto& proto);

 private:
  DescriptorBuilder(const DescriptorPool* pool, DescriptorPool::Tables* tables,
                    DescriptorPool::DeferredValidation& deferred_validation,
                    DescriptorPool::ErrorCollector* error_collector);

  friend class OptionInterpreter;

  // Non-recursive part of BuildFile functionality.
  FileDescriptor* BuildFileImpl(const FileDescriptorProto& proto,
                                internal::FlatAllocator& alloc);

  const DescriptorPool* pool_;
  DescriptorPool::Tables* tables_;  // for convenience
  DescriptorPool::DeferredValidation& deferred_validation_;
  DescriptorPool::ErrorCollector* error_collector_;

  std::optional<FeatureResolver> feature_resolver_ = std::nullopt;

  // As we build descriptors we store copies of the options messages in
  // them. We put pointers to those copies in this vector, as we build, so we
  // can later (after cross-linking) interpret those options.
  std::vector<OptionsToInterpret> options_to_interpret_;

  bool had_errors_;
  std::string filename_;
  FileDescriptor* file_;
  FileDescriptorTables* file_tables_;
  absl::flat_hash_set<const FileDescriptor*> dependencies_;
  absl::flat_hash_set<const FileDescriptor*> option_dependencies_;

  struct MessageHints {
    int fields_to_suggest = 0;
    const Message* first_reason = nullptr;
    DescriptorPool::ErrorCollector::ErrorLocation first_reason_location =
        DescriptorPool::ErrorCollector::ErrorLocation::OTHER;

    void RequestHintOnFieldNumbers(
        const Message& reason,
        DescriptorPool::ErrorCollector::ErrorLocation reason_location,
        int range_start = 0, int range_end = 1) {
      auto fit = [](int value) {
        return std::min(std::max(value, 0), FieldDescriptor::kMaxNumber);
      };
      fields_to_suggest =
          fit(fields_to_suggest + fit(fit(range_end) - fit(range_start)));
      if (first_reason) return;
      first_reason = &reason;
      first_reason_location = reason_location;
    }
  };

  absl::flat_hash_map<const Descriptor*, MessageHints> message_hints_;

  // unused_dependency_ is used to record the unused imported files.
  // Note: public import is not considered.
  absl::flat_hash_set<const FileDescriptor*> unused_dependency_;

  // If LookupSymbol() finds a symbol that is in a file which is not a declared
  // dependency of this file, it will fail, but will set
  // possible_undeclared_dependency_ to point at that file.  This is only used
  // by AddNotDefinedError() to report a more useful error message.
  // possible_undeclared_dependency_name_ is the name of the symbol that was
  // actually found in possible_undeclared_dependency_, which may be a parent
  // of the symbol actually looked for.
  const FileDescriptor* possible_undeclared_dependency_;
  std::string possible_undeclared_dependency_name_;

  // If LookupSymbol() could resolve a symbol which is not defined,
  // record the resolved name.  This is only used by AddNotDefinedError()
  // to report a more useful error message.
  std::string undefine_resolved_name_;

  // Tracker for current recursion depth to implement recursion protection.
  //
  // Counts down to 0 when there is no depth remaining.
  //
  // Maximum recursion depth corresponds to 32 nested message declarations.
  int recursion_depth_ = internal::cpp::MaxMessageDeclarationNestingDepth();

  // Note: Both AddError and AddWarning functions are extremely sensitive to
  // the *caller* stack space used. We call these functions many times in
  // complex code paths that are hot and likely to be inlined heavily. However,
  // these calls themselves are cold error paths. But stack space used by the
  // code that sets up the call in many cases is paid for even when the call
  // isn't reached. To optimize this, we use `absl::string_view` to reuse
  // string objects where possible for the inputs and for the error message
  // itself we use a closure to build the error message inside these routines.
  // The routines themselves are marked to prevent inlining and this lets us
  // move the large code sometimes required to produce a useful error message
  // entirely into a helper closure rather than the immediate caller.
  //
  // The `const char*` overload should only be used for string literal messages
  // where this is a frustrating amount of overhead and there is no harm in
  // directly using the literal.
  void AddError(absl::string_view element_name, const Message& descriptor,
                DescriptorPool::ErrorCollector::ErrorLocation location,
                absl::FunctionRef<std::string()> make_error);
  void AddError(absl::string_view element_name, const Message& descriptor,
                DescriptorPool::ErrorCollector::ErrorLocation location,
                const char* error);
  void AddRecursiveImportError(const FileDescriptorProto& proto, int from_here);
  void AddTwiceListedError(const FileDescriptorProto& proto,
                           absl::string_view import_name);
  void AddImportError(const FileDescriptorProto& proto,
                      absl::string_view import_name);

  // Adds an error indicating that undefined_symbol was not defined.  Must
  // only be called after LookupSymbol() fails.
  void AddNotDefinedError(
      absl::string_view element_name, const Message& descriptor,
      DescriptorPool::ErrorCollector::ErrorLocation location,
      absl::string_view undefined_symbol);

  void AddWarning(absl::string_view element_name, const Message& descriptor,
                  DescriptorPool::ErrorCollector::ErrorLocation location,
                  absl::FunctionRef<std::string()> make_error);
  void AddWarning(absl::string_view element_name, const Message& descriptor,
                  DescriptorPool::ErrorCollector::ErrorLocation location,
                  const char* error);

  // Silly helper which determines if the given file is in the given package.
  // I.e., either file->package() == package_name or file->package() is a
  // nested package within package_name.
  bool IsInPackage(const FileDescriptor* file, absl::string_view package_name);

  // Helper function which finds all public dependencies of the given file, and
  // stores them in the dependencies_ set in the builder.
  void RecordPublicDependencies(const FileDescriptor* file);

  // Helper function which finds all public option dependencies of the given
  // file, and stores them in the option_dependencies_ set in the builder.
  void RecordPublicOptionDependencies(const FileDescriptor* file);

  // Like tables_->FindSymbol(), but additionally:
  // - Search the pool's underlay if not found in tables_.
  // - Insure that the resulting Symbol is from one of the file's declared
  //   dependencies.
  Symbol FindSymbol(absl::string_view name, bool build_it = true);

  // Like FindSymbol() but does not require that the symbol is in one of the
  // file's declared dependencies.
  Symbol FindSymbolNotEnforcingDeps(absl::string_view name,
                                    bool build_it = true);

  // This implements the body of FindSymbolNotEnforcingDeps().
  Symbol FindSymbolNotEnforcingDepsHelper(const DescriptorPool* pool,
                                          absl::string_view name,
                                          bool build_it = true);

  // Like FindSymbol(), but looks up the name relative to some other symbol
  // name.  This first searches siblings of relative_to, then siblings of its
  // parents, etc.  For example, LookupSymbol("foo.bar", "baz.moo.corge") makes
  // the following calls, returning the first non-null result:
  // FindSymbol("baz.moo.foo.bar"), FindSymbol("baz.foo.bar"),
  // FindSymbol("foo.bar").  If AllowUnknownDependencies() has been called
  // on the DescriptorPool, this will generate a placeholder type if
  // the name is not found (unless the name itself is malformed).  The
  // placeholder_type parameter indicates what kind of placeholder should be
  // constructed in this case.  The resolve_mode parameter determines whether
  // any symbol is returned, or only symbols that are types.  Note, however,
  // that LookupSymbol may still return a non-type symbol in LOOKUP_TYPES mode,
  // if it believes that's all it could refer to.  The caller should always
  // check that it receives the type of symbol it was expecting.
  enum ResolveMode { LOOKUP_ALL, LOOKUP_TYPES };
  Symbol LookupSymbol(absl::string_view name, absl::string_view relative_to,
                      DescriptorPool::PlaceholderType placeholder_type =
                          DescriptorPool::PLACEHOLDER_MESSAGE,
                      ResolveMode resolve_mode = LOOKUP_ALL,
                      bool build_it = true);

  // Like LookupSymbol() but will not return a placeholder even if
  // AllowUnknownDependencies() has been used.
  Symbol LookupSymbolNoPlaceholder(absl::string_view name,
                                   absl::string_view relative_to,
                                   ResolveMode resolve_mode = LOOKUP_ALL,
                                   bool build_it = true);

  // Calls tables_->AddSymbol() and records an error if it fails.  Returns
  // true if successful or false if failed, though most callers can ignore
  // the return value since an error has already been recorded.
  bool AddSymbol(absl::string_view full_name, const void* parent,
                 absl::string_view name, const Message& proto, Symbol symbol);

  // Like AddSymbol(), but succeeds if the symbol is already defined as long
  // as the existing definition is also a package (because it's OK to define
  // the same package in two different files).  Also adds all parents of the
  // package to the symbol table (e.g. AddPackage("foo.bar", ...) will add
  // "foo.bar" and "foo" to the table).
  void AddPackage(absl::string_view name, const Message& proto,
                  FileDescriptor* file, bool toplevel);

  // Checks that the symbol name contains only alphanumeric characters and
  // underscores.  Records an error otherwise.
  void ValidateSymbolName(absl::string_view name, absl::string_view full_name,
                          const Message& proto);

  // Allocates a copy of orig_options in tables_ and stores it in the
  // descriptor. Remembers its uninterpreted options, to be interpreted
  // later. DescriptorT must be one of the Descriptor messages from
  // descriptor.proto.
  template <class DescriptorT>
  void AllocateOptions(const typename DescriptorT::Proto& proto,
                       DescriptorT* descriptor, int options_field_tag,
                       absl::string_view option_name,
                       internal::FlatAllocator& alloc);
  // Specialization for FileOptions.
  void AllocateOptions(const FileDescriptorProto& proto,
                       FileDescriptor* descriptor,
                       internal::FlatAllocator& alloc);

  // Implementation for AllocateOptions(). Don't call this directly.
  template <class DescriptorT>
  const typename DescriptorT::OptionsType* AllocateOptionsImpl(
      absl::string_view name_scope, absl::string_view element_name,
      const typename DescriptorT::Proto& proto,
      absl::Span<const int> options_path, absl::string_view option_name,
      internal::FlatAllocator& alloc);

  // Allocates and resolves any feature sets that need to be owned by a given
  // descriptor. This also strips features out of the mutable options message to
  // prevent leaking of unresolved features.
  // Note: This must be used during a pre-order traversal of the
  // descriptor tree, so that each descriptor's parent has a fully resolved
  // feature set already.
  template <class DescriptorT>
  void ResolveFeatures(const typename DescriptorT::Proto& proto,
                       DescriptorT* descriptor,
                       typename DescriptorT::OptionsType* options,
                       internal::FlatAllocator& alloc);
  void ResolveFeatures(const FileDescriptorProto& proto,
                       FileDescriptor* descriptor, FileOptions* options,
                       internal::FlatAllocator& alloc);
  template <class DescriptorT>
  void ResolveFeaturesImpl(
      Edition edition, const typename DescriptorT::Proto& proto,
      DescriptorT* descriptor, typename DescriptorT::OptionsType* options,
      internal::FlatAllocator& alloc,
      DescriptorPool::ErrorCollector::ErrorLocation error_location,
      bool force_merge = false);

  void PostProcessFieldFeatures(FieldDescriptor& field,
                                const FieldDescriptorProto& proto);

  // Allocates an array of two strings, the first one is a copy of
  // `proto_name`, and the second one is the full name. Full proto name is
  // "scope.proto_name" if scope is non-empty and "proto_name" otherwise.
  auto AllocateNameStrings(absl::string_view scope,
                           absl::string_view proto_name, const Message& entity,
                           internal::FlatAllocator& alloc);

  // These methods all have the same signature for the sake of the BUILD_ARRAY
  // macro, below.
  void BuildMessage(const DescriptorProto& proto, const Descriptor* parent,
                    Descriptor* result, internal::FlatAllocator& alloc);
  void BuildFieldOrExtension(const FieldDescriptorProto& proto,
                             Descriptor* parent, FieldDescriptor* result,
                             bool is_extension, internal::FlatAllocator& alloc);
  void BuildField(const FieldDescriptorProto& proto, Descriptor* parent,
                  FieldDescriptor* result, internal::FlatAllocator& alloc) {
    BuildFieldOrExtension(proto, parent, result, false, alloc);
  }
  void BuildExtension(const FieldDescriptorProto& proto, Descriptor* parent,
                      FieldDescriptor* result, internal::FlatAllocator& alloc) {
    BuildFieldOrExtension(proto, parent, result, true, alloc);
  }
  void BuildExtensionRange(const DescriptorProto::ExtensionRange& proto,
                           const Descriptor* parent,
                           Descriptor::ExtensionRange* result,
                           internal::FlatAllocator& alloc);
  void BuildReservedRange(const DescriptorProto::ReservedRange& proto,
                          const Descriptor* parent,
                          Descriptor::ReservedRange* result,
                          internal::FlatAllocator& alloc);
  void BuildReservedRange(const EnumDescriptorProto::EnumReservedRange& proto,
                          const EnumDescriptor* parent,
                          EnumDescriptor::ReservedRange* result,
                          internal::FlatAllocator& alloc);
  void BuildOneof(const OneofDescriptorProto& proto, Descriptor* parent,
                  OneofDescriptor* result, internal::FlatAllocator& alloc);
  void BuildEnum(const EnumDescriptorProto& proto, const Descriptor* parent,
                 EnumDescriptor* result, internal::FlatAllocator& alloc);
  void BuildEnumValue(const EnumValueDescriptorProto& proto,
                      const EnumDescriptor* parent, EnumValueDescriptor* result,
                      internal::FlatAllocator& alloc);
  void BuildService(const ServiceDescriptorProto& proto, const void* dummy,
                    ServiceDescriptor* result, internal::FlatAllocator& alloc);
  void BuildMethod(const MethodDescriptorProto& proto,
                   const ServiceDescriptor* parent, MethodDescriptor* result,
                   internal::FlatAllocator& alloc);

  void CheckFieldJsonNameUniqueness(const DescriptorProto& proto,
                                    const Descriptor* result);
  void CheckFieldJsonNameUniqueness(absl::string_view message_name,
                                    const DescriptorProto& message,
                                    const Descriptor* descriptor,
                                    bool use_custom_names);
  void CheckEnumValueUniqueness(const EnumDescriptorProto& proto,
                                const EnumDescriptor* result);

  void LogUnusedDependency(const FileDescriptorProto& proto,
                           const FileDescriptor* result);

  // Must be run only after building.
  //
  // NOTE: Options will not be available during cross-linking, as they
  // have not yet been interpreted. Defer any handling of options to the
  // Validate*Options methods.
  void CrossLinkFile(FileDescriptor* file, const FileDescriptorProto& proto);
  void CrossLinkMessage(Descriptor* message, const DescriptorProto& proto);
  void CrossLinkField(FieldDescriptor* field,
                      const FieldDescriptorProto& proto);
  void CrossLinkService(ServiceDescriptor* service,
                        const ServiceDescriptorProto& proto);
  void CrossLinkMethod(MethodDescriptor* method,
                       const MethodDescriptorProto& proto);
  void SuggestFieldNumbers(FileDescriptor* file,
                           const FileDescriptorProto& proto);
  void CheckVisibilityRules(FileDescriptor* file,
                            const FileDescriptorProto& proto);

  // Internal State used for checking visibility rules.
  struct DescriptorAndProto {
    const Descriptor* descriptor;
    const DescriptorProto* proto;
  };

  struct EnumDescriptorAndProto {
    const EnumDescriptor* descriptor;
    const EnumDescriptorProto* proto;
  };

  struct VisibilityCheckerState {
    FileDescriptor* containing_file;

    std::vector<DescriptorAndProto> nested_messages;
    std::vector<EnumDescriptorAndProto> nested_enums;
    std::vector<EnumDescriptorAndProto> namespaced_enums;
  };

  void CheckVisibilityRulesVisit(const Descriptor& message,
                                 const DescriptorProto& proto,
                                 VisibilityCheckerState& state);
  void CheckVisibilityRulesVisit(const EnumDescriptor& enm,
                                 const EnumDescriptorProto& proto,
                                 VisibilityCheckerState& state);
  void CheckVisibilityRulesVisit(const FileDescriptor&,
                                 const FileDescriptorProto& proto,
                                 VisibilityCheckerState& state) {}
  void CheckVisibilityRulesVisit(const FieldDescriptor&,
                                 const FieldDescriptorProto& proto,
                                 VisibilityCheckerState& state) {}
  void CheckVisibilityRulesVisit(const EnumValueDescriptor&,
                                 const EnumValueDescriptorProto& proto,
                                 VisibilityCheckerState& state) {}
  void CheckVisibilityRulesVisit(const OneofDescriptor&,
                                 const OneofDescriptorProto& proto,
                                 VisibilityCheckerState& state) {}
  void CheckVisibilityRulesVisit(const Descriptor::ExtensionRange&,
                                 const DescriptorProto::ExtensionRange& proto,
                                 VisibilityCheckerState& state) {}
  void CheckVisibilityRulesVisit(const MethodDescriptor&,
                                 const MethodDescriptorProto& proto,
                                 VisibilityCheckerState& state) {}
  void CheckVisibilityRulesVisit(const ServiceDescriptor&,
                                 const ServiceDescriptorProto& proto,
                                 VisibilityCheckerState& state) {}

  bool IsEnumNamespaceMessage(const EnumDescriptor& enm) const;


  // Checks that the extension field matches what is declared.
  void CheckExtensionDeclaration(const FieldDescriptor& field,
                                 const FieldDescriptorProto& proto,
                                 absl::string_view declared_full_name,
                                 absl::string_view declared_type_name,
                                 bool is_repeated);
  // Checks that the extension field type matches the declared type. It also
  // handles message types that look like non-message types such as "fixed64" vs
  // ".fixed64".
  void CheckExtensionDeclarationFieldType(const FieldDescriptor& field,
                                          const FieldDescriptorProto& proto,
                                          absl::string_view type);

  // A helper class for interpreting options.
  class OptionInterpreter {
   public:
    // Creates an interpreter that operates in the context of the pool of the
    // specified builder, which must not be nullptr. We don't take ownership of
    // the builder.
    explicit OptionInterpreter(DescriptorBuilder* builder);
    OptionInterpreter(const OptionInterpreter&) = delete;
    OptionInterpreter& operator=(const OptionInterpreter&) = delete;

    ~OptionInterpreter();

    // Interprets the uninterpreted options in the specified Options message.
    // On error, calls AddError() on the underlying builder and returns false.
    // Otherwise returns true.
    bool InterpretOptionExtensions(OptionsToInterpret* options_to_interpret);

    // Interprets the uninterpreted feature options in the specified Options
    // message. On error, calls AddError() on the underlying builder and returns
    // false. Otherwise returns true.
    bool InterpretNonExtensionOptions(OptionsToInterpret* options_to_interpret);

    // Updates the given source code info by re-writing uninterpreted option
    // locations to refer to the corresponding interpreted option.
    void UpdateSourceCodeInfo(SourceCodeInfo* info);

    class AggregateOptionFinder;

   private:
    bool InterpretOptionsImpl(OptionsToInterpret* options_to_interpret,
                              bool skip_extensions);

    // Interprets uninterpreted_option_ on the specified message, which
    // must be the mutable copy of the original options message to which
    // uninterpreted_option_ belongs. The given src_path is the source
    // location path to the uninterpreted option, and options_path is the
    // source location path to the options message. The location paths are
    // recorded and then used in UpdateSourceCodeInfo.
    // The features boolean controls whether or not we should only interpret
    // feature options or skip them entirely.
    bool InterpretSingleOption(Message* options,
                               const std::vector<int>& src_path,
                               const std::vector<int>& options_path,
                               bool skip_extensions);

    // Adds the uninterpreted_option to the given options message verbatim.
    // Used when AllowUnknownDependencies() is in effect and we can't find
    // the option's definition.
    void AddWithoutInterpreting(const UninterpretedOption& uninterpreted_option,
                                Message* options);

    // A recursive helper function that drills into the intermediate fields
    // in unknown_fields to check if field innermost_field is set on the
    // innermost message. Returns false and sets an error if so.
    bool ExamineIfOptionIsSet(
        std::vector<const FieldDescriptor*>::const_iterator
            intermediate_fields_iter,
        std::vector<const FieldDescriptor*>::const_iterator
            intermediate_fields_end,
        const FieldDescriptor* innermost_field,
        const std::string& debug_msg_name,
        const UnknownFieldSet& unknown_fields);

    // Validates the value for the option field of the currently interpreted
    // option and then sets it on the unknown_field.
    bool SetOptionValue(const FieldDescriptor* option_field,
                        UnknownFieldSet* unknown_fields);

    // Parses an aggregate value for a CPPTYPE_MESSAGE option and
    // saves it into *unknown_fields.
    bool SetAggregateOption(const FieldDescriptor* option_field,
                            UnknownFieldSet* unknown_fields);

    // Convenience functions to set an int field the right way, depending on
    // its wire type (a single int CppType can represent multiple wire types).
    void SetInt32(int number, int32_t value, FieldDescriptor::Type type,
                  UnknownFieldSet* unknown_fields);
    void SetInt64(int number, int64_t value, FieldDescriptor::Type type,
                  UnknownFieldSet* unknown_fields);
    void SetUInt32(int number, uint32_t value, FieldDescriptor::Type type,
                   UnknownFieldSet* unknown_fields);
    void SetUInt64(int number, uint64_t value, FieldDescriptor::Type type,
                   UnknownFieldSet* unknown_fields);

    // A helper function that adds an error at the specified location of the
    // option we're currently interpreting, and returns false.
    bool AddOptionError(DescriptorPool::ErrorCollector::ErrorLocation location,
                        absl::FunctionRef<std::string()> make_error) {
      builder_->AddError(options_to_interpret_->element_name,
                         *uninterpreted_option_, location, make_error);
      return false;
    }

    // A helper function that adds an error at the location of the option name
    // and returns false.
    bool AddNameError(absl::FunctionRef<std::string()> make_error) {
#ifdef PROTOBUF_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
      return true;
#else   // PROTOBUF_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
      return AddOptionError(DescriptorPool::ErrorCollector::OPTION_NAME,
                            make_error);
#endif  // PROTOBUF_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
    }

    // A helper function that adds an error at the location of the option name
    // and returns false.
    bool AddValueError(absl::FunctionRef<std::string()> make_error) {
      return AddOptionError(DescriptorPool::ErrorCollector::OPTION_VALUE,
                            make_error);
    }

    // We interpret against this builder's pool. Is never nullptr. We don't own
    // this pointer.
    DescriptorBuilder* builder_;

    // The options we're currently interpreting, or nullptr if we're not in a
    // call to InterpretOptions.
    const OptionsToInterpret* options_to_interpret_;

    // The option we're currently interpreting within options_to_interpret_, or
    // nullptr if we're not in a call to InterpretOptions(). This points to a
    // submessage of the original option, not the mutable copy. Therefore we
    // can use it to find locations recorded by the parser.
    const UninterpretedOption* uninterpreted_option_;

    // This maps the element path of uninterpreted options to the element path
    // of the resulting interpreted option. This is used to modify a file's
    // source code info to account for option interpretation.
    absl::flat_hash_map<std::vector<int>, std::vector<int>> interpreted_paths_;

    // This maps the path to a repeated option field to the known number of
    // elements the field contains. This is used to track the compute the
    // index portion of the element path when interpreting a single option.
    absl::flat_hash_map<std::vector<int>, int> repeated_option_counts_;

    // Factory used to create the dynamic messages we need to parse
    // any aggregate option values we encounter.
    DynamicMessageFactory dynamic_factory_;
  };

  // Work-around for broken compilers:  According to the C++ standard,
  // OptionInterpreter should have access to the private members of any class
  // which has declared DescriptorBuilder as a friend.  Unfortunately some old
  // versions of GCC and other compilers do not implement this correctly.  So,
  // we have to have these intermediate methods to provide access.  We also
  // redundantly declare OptionInterpreter a friend just to make things extra
  // clear for these bad compilers.
  friend class OptionInterpreter;
  friend class OptionInterpreter::AggregateOptionFinder;

  static inline bool get_allow_unknown(const DescriptorPool* pool) {
    return pool->allow_unknown_;
  }
  static inline bool get_enforce_weak(const DescriptorPool* pool) {
    return pool->enforce_weak_;
  }
  static inline bool get_is_placeholder(const Descriptor* descriptor) {
    return descriptor != nullptr && descriptor->is_placeholder_;
  }
  static inline void assert_mutex_held(const DescriptorPool* pool) {
    if (pool->mutex_ != nullptr) {
      pool->mutex_->AssertHeld();
    }
  }

  // Must be run only after options have been interpreted.
  //
  // NOTE: Validation code must only reference the options in the mutable
  // descriptors, which are the ones that have been interpreted. The const
  // proto references are passed in only so they can be provided to calls to
  // AddError(). Do not look at their options, which have not been interpreted.
  void ValidateOptions(const FileDescriptor* file,
                       const FileDescriptorProto& proto);
  void ValidateFileFeatures(const FileDescriptor* file,
                            const FileDescriptorProto& proto);
  void ValidateOptions(const Descriptor* message, const DescriptorProto& proto);
  void ValidateOptions(const OneofDescriptor* oneof,
                       const OneofDescriptorProto& proto);
  void ValidateOptions(const FieldDescriptor* field,
                       const FieldDescriptorProto& proto);
  void ValidateFieldFeatures(const FieldDescriptor* field,
                             const FieldDescriptorProto& proto);
  void ValidateOptions(const EnumDescriptor* enm,
                       const EnumDescriptorProto& proto);
  void ValidateOptions(const EnumValueDescriptor* enum_value,
                       const EnumValueDescriptorProto& proto);
  void ValidateOptions(const Descriptor::ExtensionRange* range,
                       const DescriptorProto::ExtensionRange& proto) {}
  void ValidateExtensionRangeOptions(const DescriptorProto& proto,
                                     const Descriptor& message);
  void ValidateExtensionDeclaration(
      absl::string_view full_name,
      const RepeatedPtrField<ExtensionRangeOptions_Declaration>& declarations,
      const DescriptorProto_ExtensionRange& proto,
      absl::flat_hash_set<absl::string_view>& full_name_set);
  void ValidateOptions(const ServiceDescriptor* service,
                       const ServiceDescriptorProto& proto);
  void ValidateOptions(const MethodDescriptor* method,
                       const MethodDescriptorProto& proto);
  void ValidateProto3(const FileDescriptor* file,
                      const FileDescriptorProto& proto);
  void ValidateProto3Message(const Descriptor* message,
                             const DescriptorProto& proto);
  void ValidateProto3Field(const FieldDescriptor* field,
                           const FieldDescriptorProto& proto);

  // Returns true if the map entry message is compatible with the
  // auto-generated entry message from map fields syntax.
  bool ValidateMapEntry(const FieldDescriptor* field,
                        const FieldDescriptorProto& proto);

  // Recursively detects naming conflicts with map entry types for a
  // better error message.
  void DetectMapConflicts(const Descriptor* message,
                          const DescriptorProto& proto);

  void ValidateJSType(const FieldDescriptor* field,
                      const FieldDescriptorProto& proto);

  template <typename DescriptorT, typename DescriptorProtoT>
  void ValidateNamingStyle(const DescriptorT* file,
                           const DescriptorProtoT& proto);

  // Nothing to validate for extension ranges. This overload only exists
  // so that VisitDescriptors can be exhaustive.
  void ValidateNamingStyle(const Descriptor::ExtensionRange* ext_range,
                           const DescriptorProto::ExtensionRange& proto) {}
};

const FileDescriptor* DescriptorPool::BuildFile(
    const FileDescriptorProto& proto) {
  return BuildFileCollectingErrors(proto, nullptr);
}

const FileDescriptor* DescriptorPool::BuildFileCollectingErrors(
    const FileDescriptorProto& proto, ErrorCollector* error_collector) {
  ABSL_CHECK(fallback_database_ == nullptr)
      << "Cannot call BuildFile on a DescriptorPool that uses a "
         "DescriptorDatabase.  You must instead find a way to get your file "
         "into the underlying database.";
  ABSL_CHECK(mutex_ == nullptr);  // Implied by the above ABSL_CHECK.
  tables_->known_bad_symbols_.clear();
  tables_->known_bad_files_.clear();
  build_started_ = true;
  DeferredValidation deferred_validation(this, error_collector);
  const FileDescriptor* file =
      DescriptorBuilder::New(this, tables_.get(), deferred_validation,
                             error_collector)
          ->BuildFile(proto);
  if (deferred_validation.Validate()) {
    return file;
  }
  return nullptr;
}

const FileDescriptor* DescriptorPool::BuildFileFromDatabase(
    const FileDescriptorProto& proto,
    DeferredValidation& deferred_validation) const {
  mutex_->AssertHeld();
  build_started_ = true;
  if (tables_->known_bad_files_.contains(proto.name())) {
    return nullptr;
  }
  const FileDescriptor* result;
  const auto build_file = [&] {
    result = DescriptorBuilder::New(this, tables_.get(), deferred_validation,
                                    default_error_collector_)
                 ->BuildFile(proto);
  };
  if (dispatcher_ != nullptr) {
    (*dispatcher_)(build_file);
  } else {
    build_file();
  }
  if (result == nullptr) {
    tables_->known_bad_files_.insert(proto.name());
  }
  return result;
}

absl::Status DescriptorPool::SetFeatureSetDefaults(FeatureSetDefaults spec) {
  if (build_started_) {
    return absl::FailedPreconditionError(
        "Feature set defaults can't be changed once the pool has started "
        "building.");
  }
  if (spec.minimum_edition() > spec.maximum_edition()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Invalid edition range ", spec.minimum_edition(), " to ",
                     spec.maximum_edition(), "."));
  }
  Edition prev_edition = EDITION_UNKNOWN;
  for (const auto& edition_default : spec.defaults()) {
    if (edition_default.edition() == EDITION_UNKNOWN) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Invalid edition ", edition_default.edition(), " specified."));
    }
    if (edition_default.edition() <= prev_edition) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Feature set defaults are not strictly increasing.  Edition ",
          prev_edition, " is greater than or equal to edition ",
          edition_default.edition(), "."));
    }
    prev_edition = edition_default.edition();
  }
  feature_set_defaults_spec_ =
      absl::make_unique<FeatureSetDefaults>(std::move(spec));
  return absl::OkStatus();
}

const FeatureSetDefaults& DescriptorPool::GetFeatureSetDefaults() const {
  if (feature_set_defaults_spec_ != nullptr) return *feature_set_defaults_spec_;
  static const FeatureSetDefaults* cpp_default_spec =
      internal::OnShutdownDelete([] {
        auto* defaults = new FeatureSetDefaults();
        internal::ParseNoReflection(
            absl::string_view{
                PROTOBUF_INTERNAL_CPP_EDITION_DEFAULTS,
                sizeof(PROTOBUF_INTERNAL_CPP_EDITION_DEFAULTS) - 1},
            *defaults);
        return defaults;
      }());
  return *cpp_default_spec;
}

bool DescriptorPool::ResolvesFeaturesForImpl(int extension_number) const {
  for (const auto& edition_default : GetFeatureSetDefaults().defaults()) {
    std::vector<const FieldDescriptor*> fields;
    auto features = edition_default.fixed_features();
    features.MergeFrom(edition_default.overridable_features());
    features.GetReflection()->ListFields(features, &fields);
    if (absl::c_find_if(fields, [&](const FieldDescriptor* field) {
          return field->number() == extension_number;
        }) == fields.end()) {
      return false;
    }
  }
  return true;
}

DescriptorBuilder::DescriptorBuilder(
    const DescriptorPool* pool, DescriptorPool::Tables* tables,
    DescriptorPool::DeferredValidation& deferred_validation,
    DescriptorPool::ErrorCollector* error_collector)
    : pool_(pool),
      tables_(tables),
      deferred_validation_(deferred_validation),
      error_collector_(error_collector),
      had_errors_(false),
      possible_undeclared_dependency_(nullptr),
      undefine_resolved_name_("") {
  // Ensure that any lazily loaded static initializers from the generated pool
  // (e.g. from bootstrapped protos) are run before building any descriptors. We
  // have to avoid registering these pre-main, because we need to ensure that
  // the linker --gc-sections step can strip out the full runtime if it is
  // unused.
  [[maybe_unused]] static std::true_type lazy_register =
      (internal::ExtensionSet::RegisterMessageExtension(
           &FeatureSet::default_instance(), pb::cpp.number(),
           FieldDescriptor::TYPE_MESSAGE, false, false,
           &pb::CppFeatures::default_instance(),
           nullptr,
           internal::LazyAnnotation::kUndefined),
       std::true_type{});
}

DescriptorBuilder::~DescriptorBuilder() = default;

PROTOBUF_NOINLINE void DescriptorBuilder::AddError(
    const absl::string_view element_name, const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    absl::FunctionRef<std::string()> make_error) {
  std::string error = make_error();
  if (error_collector_ == nullptr) {
    if (!had_errors_) {
      ABSL_LOG(ERROR) << "Invalid proto descriptor for file \"" << filename_
                      << "\":";
    }
    ABSL_LOG(ERROR) << "  " << element_name << ": " << error;
  } else {
    error_collector_->RecordError(filename_, element_name, &descriptor,
                                  location, error);
  }
  had_errors_ = true;
}

PROTOBUF_NOINLINE void DescriptorBuilder::AddError(
    const absl::string_view element_name, const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location, const char* error) {
  AddError(element_name, descriptor, location, [error] { return error; });
}

PROTOBUF_NOINLINE void DescriptorBuilder::AddNotDefinedError(
    const absl::string_view element_name, const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const absl::string_view undefined_symbol) {
  if (possible_undeclared_dependency_ == nullptr &&
      undefine_resolved_name_.empty()) {
    AddError(element_name, descriptor, location, [&] {
      return absl::StrCat("\"", undefined_symbol, "\" is not defined.");
    });
  } else {
    if (possible_undeclared_dependency_ != nullptr) {
      AddError(element_name, descriptor, location, [&] {
        return absl::StrCat("\"", possible_undeclared_dependency_name_,
                            "\" seems to be defined in \"",
                            possible_undeclared_dependency_->name(),
                            "\", which is not "
                            "imported by \"",
                            filename_,
                            "\".  To use it here, please "
                            "add the necessary import.");
      });
    }
    if (!undefine_resolved_name_.empty()) {
      AddError(element_name, descriptor, location, [&] {
        return absl::StrCat(
            "\"", undefined_symbol, "\" is resolved to \"",
            undefine_resolved_name_,
            "\", which is not defined. "
            "The innermost scope is searched first in name resolution. "
            "Consider using a leading '.'(i.e., \".",
            undefined_symbol, "\") to start from the outermost scope.");
      });
    }
  }
}

PROTOBUF_NOINLINE void DescriptorBuilder::AddWarning(
    const absl::string_view element_name, const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    absl::FunctionRef<std::string()> make_error) {
  std::string error = make_error();
  if (error_collector_ == nullptr) {
    ABSL_LOG(WARNING) << filename_ << " " << element_name << ": " << error;
  } else {
    error_collector_->RecordWarning(filename_, element_name, &descriptor,
                                    location, error);
  }
}

PROTOBUF_NOINLINE void DescriptorBuilder::AddWarning(
    const absl::string_view element_name, const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location, const char* error) {
  AddWarning(element_name, descriptor, location,
             [error]() -> std::string { return error; });
}

bool DescriptorBuilder::IsInPackage(const FileDescriptor* file,
                                    absl::string_view package_name) {
  return absl::StartsWith(file->package(), package_name) &&
         (file->package().size() == package_name.size() ||
          file->package()[package_name.size()] == '.');
}

void DescriptorBuilder::RecordPublicDependencies(const FileDescriptor* file) {
  if (file == nullptr || !dependencies_.insert(file).second) return;
  for (int i = 0; file != nullptr && i < file->public_dependency_count(); i++) {
    RecordPublicDependencies(file->public_dependency(i));
  }
}

void DescriptorBuilder::RecordPublicOptionDependencies(
    const FileDescriptor* file) {
  if (file == nullptr || !option_dependencies_.insert(file).second) return;
  for (int i = 0; i < file->public_dependency_count(); i++) {
    RecordPublicOptionDependencies(file->public_dependency(i));
  }
}

Symbol DescriptorBuilder::FindSymbolNotEnforcingDepsHelper(
    const DescriptorPool* pool, const absl::string_view name, bool build_it) {
  // If we are looking at an underlay, we must lock its mutex_, since we are
  // accessing the underlay's tables_ directly.
  absl::MutexLockMaybe lock((pool == pool_) ? nullptr : pool->mutex_);

  Symbol result = pool->tables_->FindSymbol(name);
  if (result.IsNull() && pool->underlay_ != nullptr) {
    // Symbol not found; check the underlay.
    result = FindSymbolNotEnforcingDepsHelper(pool->underlay_, name);
  }

  if (result.IsNull()) {
    // With lazily_build_dependencies_, a symbol lookup at cross link time is
    // not guaranteed to be successful. In most cases, build_it will be false,
    // which intentionally prevents us from building an import until it's
    // actually needed. In some cases, like registering an extension, we want
    // to build the file containing the symbol, and build_it will be set.
    // Also, build_it will be true when !lazily_build_dependencies_, to provide
    // better error reporting of missing dependencies.
    if (build_it &&
        pool->TryFindSymbolInFallbackDatabase(name, deferred_validation_)) {
      result = pool->tables_->FindSymbol(name);
    }
  }

  return result;
}

Symbol DescriptorBuilder::FindSymbolNotEnforcingDeps(
    const absl::string_view name, bool build_it) {
  Symbol result = FindSymbolNotEnforcingDepsHelper(pool_, name, build_it);
  // Only find symbols which were defined in this file or one of its
  // dependencies.
  const FileDescriptor* file = result.GetFile();
  if ((file == file_ || dependencies_.contains(file) ||
       option_dependencies_.contains(file)) &&
      !result.IsPackage()) {
    unused_dependency_.erase(file);
  }
  return result;
}

Symbol DescriptorBuilder::FindSymbol(const absl::string_view name,
                                     bool build_it) {
  Symbol result = FindSymbolNotEnforcingDeps(name, build_it);

  if (result.IsNull()) return result;

  if (!pool_->enforce_dependencies_) {
    // Hack for CompilerUpgrader, and also used for lazily_build_dependencies_
    return result;
  }

  // Only find symbols which were defined in this file or one of its
  // dependencies.
  const FileDescriptor* file = result.GetFile();
  if (file == file_ || dependencies_.contains(file) ||
      (option_dependencies_.contains(file) &&
       result.field_descriptor() != nullptr)) {
    return result;
  }

  if (result.IsPackage()) {
    // Arg, this is overcomplicated.  The symbol is a package name.  It could
    // be that the package was defined in multiple files.  result.GetFile()
    // returns the first file we saw that used this package.  We've determined
    // that that file is not a direct dependency of the file we are currently
    // building, but it could be that some other file which *is* a direct
    // dependency also defines the same package.  We can't really rule out this
    // symbol unless none of the dependencies define it.
    if (IsInPackage(file_, name)) return result;
    for (const auto* dep : dependencies_) {
      // Note:  A dependency may be nullptr if it was not found or had errors.
      if (dep != nullptr && IsInPackage(dep, name)) return result;
    }
    for (const auto* dep : option_dependencies_) {
      // Note:  A dependency may be nullptr if it was not found or had errors.
      if (dep != nullptr && IsInPackage(dep, name)) return result;
    }
  }

  possible_undeclared_dependency_ = file;
  possible_undeclared_dependency_name_ = std::string(name);
  return Symbol();
}

Symbol DescriptorBuilder::LookupSymbolNoPlaceholder(
    const absl::string_view name, const absl::string_view relative_to,
    ResolveMode resolve_mode, bool build_it) {
  possible_undeclared_dependency_ = nullptr;
  undefine_resolved_name_.clear();

  if (!name.empty() && name[0] == '.') {
    // Fully-qualified name.
    return FindSymbol(name.substr(1), build_it);
  }

  // If name is something like "Foo.Bar.baz", and symbols named "Foo" are
  // defined in multiple parent scopes, we only want to find "Bar.baz" in the
  // innermost one.  E.g., the following should produce an error:
  //   message Bar { message Baz {} }
  //   message Foo {
  //     message Bar {
  //     }
  //     optional Bar.Baz baz = 1;
  //   }
  // So, we look for just "Foo" first, then look for "Bar.baz" within it if
  // found.
  std::string::size_type name_dot_pos = name.find_first_of('.');
  absl::string_view first_part_of_name;
  if (name_dot_pos == std::string::npos) {
    first_part_of_name = name;
  } else {
    first_part_of_name = name.substr(0, name_dot_pos);
  }

  std::string scope_to_try(relative_to);

  while (true) {
    // Chop off the last component of the scope.
    std::string::size_type dot_pos = scope_to_try.find_last_of('.');
    if (dot_pos == std::string::npos) {
      return FindSymbol(name, build_it);
    } else {
      scope_to_try.erase(dot_pos);
    }

    // Append ".first_part_of_name" and try to find.
    std::string::size_type old_size = scope_to_try.size();
    absl::StrAppend(&scope_to_try, ".", first_part_of_name);
    Symbol result = FindSymbol(scope_to_try, build_it);
    if (!result.IsNull()) {
      if (first_part_of_name.size() < name.size()) {
        // name is a compound symbol, of which we only found the first part.
        // Now try to look up the rest of it.
        if (result.IsAggregate()) {
          absl::StrAppend(&scope_to_try,
                          name.substr(first_part_of_name.size()));
          result = FindSymbol(scope_to_try, build_it);
          if (result.IsNull()) {
            undefine_resolved_name_ = scope_to_try;
          }
          return result;
        } else {
          // We found a symbol but it's not an aggregate.  Continue the loop.
        }
      } else {
        if (resolve_mode == LOOKUP_TYPES && !result.IsType()) {
          // We found a symbol but it's not a type.  Continue the loop.
        } else {
          return result;
        }
      }
    }

    // Not found.  Remove the name so we can try again.
    scope_to_try.erase(old_size);
  }
}

Symbol DescriptorBuilder::LookupSymbol(
    const absl::string_view name, const absl::string_view relative_to,
    DescriptorPool::PlaceholderType placeholder_type, ResolveMode resolve_mode,
    bool build_it) {
  Symbol result =
      LookupSymbolNoPlaceholder(name, relative_to, resolve_mode, build_it);
  if (result.IsNull() && pool_->allow_unknown_) {
    // Not found, but AllowUnknownDependencies() is enabled.  Return a
    // placeholder instead.
    result = pool_->NewPlaceholderWithMutexHeld(name, placeholder_type);
  }
  return result;
}

static bool ValidateQualifiedName(absl::string_view name) {
  bool last_was_period = false;

  for (char character : name) {
    // I don't trust isalnum() due to locales.  :(
    if (('a' <= character && character <= 'z') ||
        ('A' <= character && character <= 'Z') ||
        ('0' <= character && character <= '9') || (character == '_')) {
      last_was_period = false;
    } else if (character == '.') {
      if (last_was_period) return false;
      last_was_period = true;
    } else {
      return false;
    }
  }

  return !name.empty() && !last_was_period;
}

Symbol DescriptorPool::NewPlaceholder(absl::string_view name,
                                      PlaceholderType placeholder_type) const {
  absl::MutexLockMaybe lock(mutex_);
  return NewPlaceholderWithMutexHeld(name, placeholder_type);
}

Symbol DescriptorPool::NewPlaceholderWithMutexHeld(
    absl::string_view name, PlaceholderType placeholder_type) const {
  if (mutex_) {
    mutex_->AssertHeld();
  }
  // Compute names.
  absl::string_view placeholder_full_name;
  absl::string_view placeholder_name;
  const std::string* placeholder_package;

  if (!ValidateQualifiedName(name)) return Symbol();
  if (name[0] == '.') {
    // Fully-qualified.
    placeholder_full_name = name.substr(1);
  } else {
    placeholder_full_name = name;
  }

  // Create the placeholders.
  internal::FlatAllocator alloc;
  alloc.PlanArray<FileDescriptor>(1);
  alloc.PlanArray<std::string>(2);
  if (placeholder_type == PLACEHOLDER_ENUM) {
    alloc.PlanArray<EnumDescriptor>(1);
    alloc.PlanArray<EnumValueDescriptor>(1);
    // names for the descriptor.
    alloc.PlanEntityNames(placeholder_full_name.size());
    alloc.PlanArray<std::string>(2);  // names for the value.
  } else {
    alloc.PlanArray<Descriptor>(1);
    // names for the descriptor.
    alloc.PlanEntityNames(placeholder_full_name.size());
    if (placeholder_type == PLACEHOLDER_EXTENDABLE_MESSAGE) {
      alloc.PlanArray<Descriptor::ExtensionRange>(1);
    }
  }
  alloc.FinalizePlanning(tables_);

  const std::string::size_type dotpos = placeholder_full_name.find_last_of('.');
  if (dotpos != std::string::npos) {
    placeholder_package =
        alloc.AllocateStrings(placeholder_full_name.substr(0, dotpos));
    placeholder_name = placeholder_full_name.substr(dotpos + 1);
  } else {
    placeholder_package = alloc.AllocateStrings("");
    placeholder_name = placeholder_full_name;
  }

  FileDescriptor* placeholder_file = NewPlaceholderFileWithMutexHeld(
      absl::StrCat(placeholder_full_name, ".placeholder.proto"), alloc);
  placeholder_file->package_ = placeholder_package;

  if (placeholder_type == PLACEHOLDER_ENUM) {
    placeholder_file->enum_type_count_ = 1;
    placeholder_file->enum_types_ = alloc.AllocateArray<EnumDescriptor>(1);

    EnumDescriptor* placeholder_enum = &placeholder_file->enum_types_[0];
    memset(static_cast<void*>(placeholder_enum), 0, sizeof(*placeholder_enum));

    placeholder_enum->all_names_ = alloc.AllocatePlaceholderNames(
        placeholder_full_name, placeholder_name.size());
    placeholder_enum->file_ = placeholder_file;
    placeholder_enum->options_ = &EnumOptions::default_instance();
    placeholder_enum->proto_features_ = &FeatureSet::default_instance();
    placeholder_enum->merged_features_ = &FeatureSet::default_instance();
    placeholder_enum->is_placeholder_ = true;
    placeholder_enum->is_unqualified_placeholder_ = (name[0] != '.');

    // Enums must have at least one value.
    placeholder_enum->value_count_ = 1;
    placeholder_enum->values_ = alloc.AllocateArray<EnumValueDescriptor>(1);
    // Disable fast-path lookup for this enum.
    placeholder_enum->sequential_value_limit_ = -1;

    EnumValueDescriptor* placeholder_value = &placeholder_enum->values_[0];
    memset(static_cast<void*>(placeholder_value), 0,
           sizeof(*placeholder_value));

    // Note that enum value names are siblings of their type, not children.
    placeholder_value->all_names_ = alloc.AllocateStrings(
        "PLACEHOLDER_VALUE",
        placeholder_package->empty()
            ? "PLACEHOLDER_VALUE"
            : absl::StrCat(*placeholder_package, ".PLACEHOLDER_VALUE"));

    placeholder_value->number_ = 0;
    placeholder_value->type_ = placeholder_enum;
    placeholder_value->options_ = &EnumValueOptions::default_instance();

    return Symbol(placeholder_enum);
  } else {
    placeholder_file->message_type_count_ = 1;
    placeholder_file->message_types_ = alloc.AllocateArray<Descriptor>(1);

    Descriptor* placeholder_message = &placeholder_file->message_types_[0];
    memset(static_cast<void*>(placeholder_message), 0,
           sizeof(*placeholder_message));

    placeholder_message->all_names_ = alloc.AllocatePlaceholderNames(
        placeholder_full_name, placeholder_name.size());
    placeholder_message->file_ = placeholder_file;
    placeholder_message->options_ = &MessageOptions::default_instance();
    placeholder_message->proto_features_ = &FeatureSet::default_instance();
    placeholder_message->merged_features_ = &FeatureSet::default_instance();
    placeholder_message->is_placeholder_ = true;
    placeholder_message->is_unqualified_placeholder_ = (name[0] != '.');

    if (placeholder_type == PLACEHOLDER_EXTENDABLE_MESSAGE) {
      placeholder_message->extension_range_count_ = 1;
      placeholder_message->extension_ranges_ =
          alloc.AllocateArray<Descriptor::ExtensionRange>(1);
      placeholder_message->extension_ranges_[0].start_ = 1;
      // kMaxNumber + 1 because ExtensionRange::end is exclusive.
      placeholder_message->extension_ranges_[0].end_ =
          FieldDescriptor::kMaxNumber + 1;
      placeholder_message->extension_ranges_[0].options_ = nullptr;
      placeholder_message->extension_ranges_[0].proto_features_ =
          &FeatureSet::default_instance();
      placeholder_message->extension_ranges_[0].merged_features_ =
          &FeatureSet::default_instance();
    }

    return Symbol(placeholder_message);
  }
}

FileDescriptor* DescriptorPool::NewPlaceholderFile(
    const absl::string_view name) const {
  absl::MutexLockMaybe lock(mutex_);
  internal::FlatAllocator alloc;
  alloc.PlanArray<FileDescriptor>(1);
  alloc.PlanArray<std::string>(1);
  alloc.FinalizePlanning(tables_);

  return NewPlaceholderFileWithMutexHeld(name, alloc);
}

FileDescriptor* DescriptorPool::NewPlaceholderFileWithMutexHeld(
    const absl::string_view name, internal::FlatAllocator& alloc) const {
  if (mutex_) {
    mutex_->AssertHeld();
  }
  FileDescriptor* placeholder = alloc.AllocateArray<FileDescriptor>(1);
  memset(static_cast<void*>(placeholder), 0, sizeof(*placeholder));

  placeholder->name_ = alloc.AllocateStrings(name);
  placeholder->package_ = &internal::GetEmptyString();
  placeholder->pool_ = this;
  placeholder->options_ = &FileOptions::default_instance();
  placeholder->proto_features_ = &FeatureSet::default_instance();
  placeholder->merged_features_ = &FeatureSet::default_instance();
  placeholder->tables_ = &FileDescriptorTables::GetEmptyInstance();
  placeholder->source_code_info_ = &SourceCodeInfo::default_instance();
  placeholder->is_placeholder_ = true;
  placeholder->finished_building_ = true;
  // All other fields are zero or nullptr.

  return placeholder;
}

bool DescriptorBuilder::AddSymbol(const absl::string_view full_name,
                                  const void* parent,
                                  const absl::string_view name,
                                  const Message& proto, Symbol symbol) {
  // If the caller passed nullptr for the parent, the symbol is at file scope.
  // Use its file as the parent instead.
  if (parent == nullptr) parent = file_;

  if (absl::StrContains(full_name, '\0')) {
    AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("\"", full_name, "\" contains null character.");
    });
    return false;
  }
  if (tables_->AddSymbol(full_name, symbol)) {
    if (!file_tables_->AddAliasUnderParent(parent, name, symbol)) {
      // This is only possible if there was already an error adding something of
      // the same name.
      if (!had_errors_) {
        ABSL_DLOG(FATAL) << "\"" << full_name
                         << "\" not previously defined in "
                            "symbols_by_name_, but was defined in "
                            "symbols_by_parent_; this shouldn't be possible.";
      }
      return false;
    }
    return true;
  } else {
    const FileDescriptor* other_file = tables_->FindSymbol(full_name).GetFile();
    if (other_file == file_) {
      std::string::size_type dot_pos = full_name.find_last_of('.');
      if (dot_pos == std::string::npos) {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
          return absl::StrCat("\"", full_name, "\" is already defined.");
        });
      } else {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
          return absl::StrCat("\"", full_name.substr(dot_pos + 1),
                              "\" is already defined in \"",
                              full_name.substr(0, dot_pos), "\".");
        });
      }
    } else {
      // Symbol seems to have been defined in a different file.
      AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
        return absl::StrCat(
            "\"", full_name, "\" is already defined in file \"",
            (other_file == nullptr ? "null" : other_file->name()), "\".");
      });
    }
    return false;
  }
}

void DescriptorBuilder::AddPackage(const absl::string_view name,
                                   const Message& proto, FileDescriptor* file,
                                   bool toplevel) {
  if (absl::StrContains(name, '\0')) {
    AddError(name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("\"", name, "\" contains null character.");
    });
    return;
  }

  Symbol existing_symbol = tables_->FindSymbol(name);
  // It's OK to redefine a package.
  if (existing_symbol.IsNull()) {
    if (toplevel) {
      // It is the toplevel package name, so insert the descriptor directly.
      tables_->AddSymbol(file->package(), Symbol(file));
    } else {
      auto* package = tables_->Allocate<Symbol::Subpackage>();
      // If the name is the package name, then it is already in the arena.
      // If not, copy it there. It came from the call to AddPackage below.
      package->name_size = static_cast<int>(name.size());
      package->file = file;
      tables_->AddSymbol(name, Symbol(package));
    }
    // Also add parent package, if any.
    std::string::size_type dot_pos = name.find_last_of('.');
    if (dot_pos == std::string::npos) {
      // No parents.
      ValidateSymbolName(name, name, proto);
    } else {
      // Has parent.
      AddPackage(name.substr(0, dot_pos), proto, file, false);
      ValidateSymbolName(name.substr(dot_pos + 1), name, proto);
    }
  } else if (!existing_symbol.IsPackage()) {
    // Symbol seems to have been defined in a different file.
    const FileDescriptor* other_file = existing_symbol.GetFile();
    AddError(name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("\"", name,
                          "\" is already defined (as something other than "
                          "a package) in file \"",
                          (other_file == nullptr ? "null" : other_file->name()),
                          "\".");
    });
  }
}

void DescriptorBuilder::ValidateSymbolName(const absl::string_view name,
                                           const absl::string_view full_name,
                                           const Message& proto) {
  if (name.empty()) {
    AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
             "Missing name.");
  } else {
    for (char character : name) {
      // I don't trust isalnum() due to locales.  :(
      if ((character < 'a' || 'z' < character) &&
          (character < 'A' || 'Z' < character) &&
          (character < '0' || '9' < character) && (character != '_')) {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
          return absl::StrCat("\"", name, "\" is not a valid identifier.");
        });
        return;
      }
    }
  }
}

// -------------------------------------------------------------------

// This generic implementation is good for all descriptors except
// FileDescriptor.
template <class DescriptorT>
void DescriptorBuilder::AllocateOptions(
    const typename DescriptorT::Proto& proto, DescriptorT* descriptor,
    int options_field_tag, absl::string_view option_name,
    internal::FlatAllocator& alloc) {
  std::vector<int> options_path;
  descriptor->GetLocationPath(&options_path);
  options_path.push_back(options_field_tag);
  auto options = AllocateOptionsImpl<DescriptorT>(
      descriptor->full_name(), descriptor->full_name(), proto, options_path,
      option_name, alloc);
  descriptor->options_ = options;
  descriptor->proto_features_ = &FeatureSet::default_instance();
  descriptor->merged_features_ = &FeatureSet::default_instance();
}

// We specialize for FileDescriptor.
void DescriptorBuilder::AllocateOptions(const FileDescriptorProto& proto,
                                        FileDescriptor* descriptor,
                                        internal::FlatAllocator& alloc) {
  std::vector<int> options_path;
  options_path.push_back(FileDescriptorProto::kOptionsFieldNumber);
  // We add the dummy token so that LookupSymbol does the right thing.
  auto options = AllocateOptionsImpl<FileDescriptor>(
      absl::StrCat(descriptor->package(), ".dummy"), descriptor->name(), proto,
      options_path, "google.protobuf.FileOptions", alloc);
  descriptor->options_ = options;
  descriptor->proto_features_ = &FeatureSet::default_instance();
  descriptor->merged_features_ = &FeatureSet::default_instance();
}

template <class DescriptorT>
const typename DescriptorT::OptionsType* DescriptorBuilder::AllocateOptionsImpl(
    absl::string_view name_scope, absl::string_view element_name,
    const typename DescriptorT::Proto& proto,
    absl::Span<const int> options_path, absl::string_view option_name,
    internal::FlatAllocator& alloc) {
  if (!proto.has_options()) {
    return &DescriptorT::OptionsType::default_instance();
  }
  const typename DescriptorT::OptionsType& orig_options = proto.options();

  auto* options = alloc.AllocateArray<typename DescriptorT::OptionsType>(1);

  if (!orig_options.IsInitialized()) {
    AddError(absl::StrCat(name_scope, ".", element_name), orig_options,
             DescriptorPool::ErrorCollector::OPTION_NAME,
             "Uninterpreted option is missing name or value.");
    return &DescriptorT::OptionsType::default_instance();
  }

  const bool parse_success =
      internal::ParseNoReflection(orig_options.SerializeAsString(), *options);
  ABSL_DCHECK(parse_success);

  // Don't add to options_to_interpret_ unless there were uninterpreted
  // options.  This not only avoids unnecessary work, but prevents a
  // bootstrapping problem when building descriptors for descriptor.proto.
  // descriptor.proto does not contain any uninterpreted options, but
  // attempting to interpret options anyway will cause
  // OptionsType::GetDescriptor() to be called which may then deadlock since
  // we're still trying to build it.
  if (options->uninterpreted_option_size() > 0) {
    options_to_interpret_.push_back(OptionsToInterpret(
        name_scope, element_name, options_path, &orig_options, options));
  }

  // If the custom option is in unknown fields, no need to interpret it.
  // Remove the dependency file from unused_dependency.
  const UnknownFieldSet& unknown_fields = orig_options.unknown_fields();
  if (!unknown_fields.empty()) {
    // Can not use options->GetDescriptor() which may case deadlock.
    Symbol msg_symbol = tables_->FindSymbol(option_name);
    if (msg_symbol.type() == Symbol::MESSAGE) {
      for (int i = 0; i < unknown_fields.field_count(); ++i) {
        assert_mutex_held(pool_);
        const FieldDescriptor* field =
            pool_->InternalFindExtensionByNumberNoLock(
                msg_symbol.descriptor(), unknown_fields.field(i).number());
        if (field) {
          unused_dependency_.erase(field->file());
        }
      }
    }
  }
  return options;
}

template <class ProtoT, class OptionsT>
static void InferLegacyProtoFeatures(const ProtoT& proto,
                                     const OptionsT& options, Edition edition,
                                     FeatureSet& features) {}

static void InferLegacyProtoFeatures(const FieldDescriptorProto& proto,
                                     const FieldOptions& options,
                                     Edition edition, FeatureSet& features) {
  if (!features.GetExtension(pb::cpp).has_string_type()) {
    if (options.ctype() == FieldOptions::CORD) {
      features.MutableExtension(pb::cpp)->set_string_type(
          pb::CppFeatures::CORD);
    }
  }

  // Everything below is specifically for proto2/proto.
  if (!IsLegacyEdition(edition)) return;

  if (proto.label() == FieldDescriptorProto::LABEL_REQUIRED) {
    features.set_field_presence(FeatureSet::LEGACY_REQUIRED);
  }
  if (proto.type() == FieldDescriptorProto::TYPE_GROUP) {
    features.set_message_encoding(FeatureSet::DELIMITED);
  }
  if (options.packed()) {
    features.set_repeated_field_encoding(FeatureSet::PACKED);
  }
  if (edition == Edition::EDITION_PROTO3) {
    if (options.has_packed() && !options.packed()) {
      features.set_repeated_field_encoding(FeatureSet::EXPANDED);
    }
  }
}

template <class DescriptorT>
void DescriptorBuilder::ResolveFeaturesImpl(
    Edition edition, const typename DescriptorT::Proto& proto,
    DescriptorT* descriptor, typename DescriptorT::OptionsType* options,
    internal::FlatAllocator& alloc,
    DescriptorPool::ErrorCollector::ErrorLocation error_location,
    bool force_merge) {
  const FeatureSet& parent_features = GetParentFeatures(descriptor);
  descriptor->proto_features_ = &FeatureSet::default_instance();
  descriptor->merged_features_ = &FeatureSet::default_instance();

  ABSL_CHECK(feature_resolver_.has_value());

  if (options->has_features()) {
    // Remove the features from the child's options proto to avoid leaking
    // internal details.
    descriptor->proto_features_ =
        tables_->InternFeatureSet(std::move(*options->mutable_features()));
    options->clear_features();
  }

  FeatureSet base_features = *descriptor->proto_features_;

  // Handle feature inference from proto2/proto3.
  if (IsLegacyEdition(edition)) {
    if (descriptor->proto_features_ != &FeatureSet::default_instance()) {
      AddError(descriptor->name(), proto, error_location,
               "Features are only valid under editions.");
    }
  }
  InferLegacyProtoFeatures(proto, *options, edition, base_features);

  if (base_features.ByteSizeLong() == 0 && !force_merge) {
    // Nothing to merge, and we aren't forcing it.
    descriptor->merged_features_ = &parent_features;
    return;
  }

  // Calculate the merged features for this target.
  absl::StatusOr<FeatureSet> merged =
      feature_resolver_->MergeFeatures(parent_features, base_features);
  if (!merged.ok()) {
    AddError(descriptor->name(), proto, error_location,
             [&] { return std::string(merged.status().message()); });
    return;
  }

  descriptor->merged_features_ = tables_->InternFeatureSet(*std::move(merged));
}

template <class DescriptorT>
void DescriptorBuilder::ResolveFeatures(
    const typename DescriptorT::Proto& proto, DescriptorT* descriptor,
    typename DescriptorT::OptionsType* options,
    internal::FlatAllocator& alloc) {
  ResolveFeaturesImpl(descriptor->file()->edition(), proto, descriptor, options,
                      alloc, DescriptorPool::ErrorCollector::NAME);
}

void DescriptorBuilder::ResolveFeatures(const FileDescriptorProto& proto,
                                        FileDescriptor* descriptor,
                                        FileOptions* options,
                                        internal::FlatAllocator& alloc) {
  // File descriptors always need their own merged feature set, even without
  // any explicit features.
  ResolveFeaturesImpl(descriptor->edition(), proto, descriptor, options, alloc,
                      DescriptorPool::ErrorCollector::EDITIONS,
                      /*force_merge=*/true);
}

void DescriptorBuilder::PostProcessFieldFeatures(
    FieldDescriptor& field, const FieldDescriptorProto& proto) {
  // TODO This can be replace by a runtime check in `is_required`
  // once the `label` getter is hidden.
  if (field.features().field_presence() == FeatureSet::LEGACY_REQUIRED &&
      field.label_ == FieldDescriptor::LABEL_OPTIONAL) {
    field.label_ = FieldDescriptor::LABEL_REQUIRED;
  }
  // TODO This can be replace by a runtime check of `is_delimited`
  // once the `TYPE_GROUP` value is removed.
  if (field.type_ == FieldDescriptor::TYPE_MESSAGE &&
      !field.containing_type()->options().map_entry() &&
      field.features().message_encoding() == FeatureSet::DELIMITED) {
    Symbol type =
        LookupSymbol(proto.type_name(), field.full_name(),
                     DescriptorPool::PLACEHOLDER_MESSAGE, LOOKUP_TYPES, false);
    if (type.descriptor() == nullptr ||
        !type.descriptor()->options().map_entry()) {
      field.type_ = FieldDescriptor::TYPE_GROUP;
    }
  }

  if (field.cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
    auto string_type = field.CalculateCppStringType();
    field.cpp_string_type_ = static_cast<uint8_t>(string_type);
    // Check that there was no narrowing.
    ABSL_DCHECK_EQ(field.cpp_string_type_, static_cast<uint8_t>(string_type));
  }

  if (field.options_->has_ctype()) {
    field.legacy_proto_ctype_ = field.options_->ctype();
    const_cast<FieldOptions*>(  // NOLINT(google3-runtime-proto-const-cast)
        field.options_)
        ->clear_ctype();
  }
}

// A common pattern:  We want to convert a repeated field in the descriptor
// to an array of values, calling some method to build each value.
#define BUILD_ARRAY(INPUT, OUTPUT, NAME, METHOD, PARENT)               \
  OUTPUT->NAME##_count_ = INPUT.NAME##_size();                         \
  OUTPUT->NAME##s_ = alloc.AllocateArray<                              \
      typename std::remove_pointer<decltype(OUTPUT->NAME##s_)>::type>( \
      INPUT.NAME##_size());                                            \
  for (int i = 0; i < INPUT.NAME##_size(); i++) {                      \
    METHOD(INPUT.NAME(i), PARENT, OUTPUT->NAME##s_ + i, alloc);        \
  }

PROTOBUF_NOINLINE void DescriptorBuilder::AddRecursiveImportError(
    const FileDescriptorProto& proto, int from_here) {
  auto make_error = [&] {
    std::string error_message("File recursively imports itself: ");
    for (size_t i = from_here; i < tables_->pending_files_.size(); i++) {
      error_message.append(tables_->pending_files_[i]);
      error_message.append(" -> ");
    }
    error_message.append(proto.name());
    return error_message;
  };

  if (static_cast<size_t>(from_here) < tables_->pending_files_.size() - 1) {
    AddError(tables_->pending_files_[from_here + 1], proto,
             DescriptorPool::ErrorCollector::IMPORT, make_error);
  } else {
    AddError(proto.name(), proto, DescriptorPool::ErrorCollector::IMPORT,
             make_error);
  }
}

void DescriptorBuilder::AddTwiceListedError(const FileDescriptorProto& proto,
                                            absl::string_view import_name) {
  AddError(import_name, proto, DescriptorPool::ErrorCollector::IMPORT, [&] {
    return absl::StrCat("Import \"", import_name, "\" was listed twice.");
  });
}

void DescriptorBuilder::AddImportError(const FileDescriptorProto& proto,
                                       absl::string_view import_name) {
  auto make_error = [&] {
    if (pool_->fallback_database_ == nullptr) {
      return absl::StrCat("Import \"", import_name, "\" has not been loaded.");
    }
    return absl::StrCat("Import \"", import_name,
                        "\" was not found or had errors.");
  };
  AddError(import_name, proto, DescriptorPool::ErrorCollector::IMPORT,
           make_error);
}

PROTOBUF_NOINLINE static bool ExistingFileMatchesProto(
    Edition edition, const FileDescriptor* existing_file,
    const FileDescriptorProto& proto) {
  FileDescriptorProto existing_proto;
  existing_file->CopyTo(&existing_proto);
  if (edition == Edition::EDITION_PROTO2 && proto.has_syntax()) {
    existing_proto.set_syntax("proto2");
  }

  return existing_proto.SerializeAsString() == proto.SerializeAsString();
}

// These PlanAllocationSize functions will gather into the FlatAllocator all the
// necessary memory allocations that BuildXXX functions below will do on the
// Tables object.
// They *must* be kept in sync. If we miss some PlanArray call we won't have
// enough memory and will ABSL_CHECK-fail.
static void PlanAllocationSize(
    const RepeatedPtrField<EnumValueDescriptorProto>& values,
    internal::FlatAllocator& alloc) {
  alloc.PlanArray<EnumValueDescriptor>(values.size());
  alloc.PlanArray<std::string>(2 * values.size());  // name + full_name
  for (const auto& v : values) {
    if (v.has_options()) alloc.PlanArray<EnumValueOptions>(1);
  }
}

static void PlanAllocationSize(
    const RepeatedPtrField<EnumDescriptorProto>& enums,
    size_t parent_scope_size, internal::FlatAllocator& alloc) {
  alloc.PlanArray<EnumDescriptor>(enums.size());
  for (const auto& e : enums) {
    alloc.PlanEntityNames(parent_scope_size
                              ? parent_scope_size + 1 + e.name().size()
                              : e.name().size());
    if (e.has_options()) alloc.PlanArray<EnumOptions>(1);
    PlanAllocationSize(e.value(), alloc);
    alloc.PlanArray<EnumDescriptor::ReservedRange>(e.reserved_range_size());
    alloc.PlanArray<const std::string*>(e.reserved_name_size());
    alloc.PlanArray<std::string>(e.reserved_name_size());
  }
}

static void PlanAllocationSize(
    const RepeatedPtrField<OneofDescriptorProto>& oneofs,
    size_t parent_scope_size, internal::FlatAllocator& alloc) {
  alloc.PlanArray<OneofDescriptor>(oneofs.size());
  for (const auto& oneof : oneofs) {
    alloc.PlanEntityNames(parent_scope_size
                              ? parent_scope_size + 1 + oneof.name().size()
                              : oneof.name().size());
    if (oneof.has_options()) alloc.PlanArray<OneofOptions>(1);
  }
}

static void PlanAllocationSize(
    const RepeatedPtrField<FieldDescriptorProto>& fields,
    size_t parent_scope_size, internal::FlatAllocator& alloc) {
  alloc.PlanArray<FieldDescriptor>(fields.size());
  for (const auto& field : fields) {
    if (field.has_options()) alloc.PlanArray<FieldOptions>(1);
    alloc.PlanFieldNames(parent_scope_size, field.name(),
                         field.has_json_name() ? &field.json_name() : nullptr);
    if (field.has_default_value() && field.has_type() &&
        (field.type() == FieldDescriptorProto::TYPE_STRING ||
         field.type() == FieldDescriptorProto::TYPE_BYTES)) {
      // For the default string value.
      alloc.PlanArray<std::string>(1);
    }
  }
}

static void PlanAllocationSize(
    const RepeatedPtrField<DescriptorProto::ExtensionRange>& ranges,
    internal::FlatAllocator& alloc) {
  alloc.PlanArray<Descriptor::ExtensionRange>(ranges.size());
  for (const auto& r : ranges) {
    if (r.has_options()) alloc.PlanArray<ExtensionRangeOptions>(1);
  }
}

static void PlanAllocationSize(
    const RepeatedPtrField<DescriptorProto>& messages, size_t parent_scope_size,
    internal::FlatAllocator& alloc) {
  alloc.PlanArray<Descriptor>(messages.size());

  for (const auto& message : messages) {
    const size_t full_name_size =
        parent_scope_size ? parent_scope_size + 1 + message.name().size()
                          : message.name().size();
    alloc.PlanEntityNames(full_name_size);

    if (message.has_options()) alloc.PlanArray<MessageOptions>(1);
    PlanAllocationSize(message.nested_type(), full_name_size, alloc);
    PlanAllocationSize(message.field(), full_name_size, alloc);
    PlanAllocationSize(message.extension(), full_name_size, alloc);
    PlanAllocationSize(message.extension_range(), alloc);
    alloc.PlanArray<Descriptor::ReservedRange>(message.reserved_range_size());
    alloc.PlanArray<const std::string*>(message.reserved_name_size());
    alloc.PlanArray<std::string>(message.reserved_name_size());
    PlanAllocationSize(message.enum_type(), full_name_size, alloc);
    PlanAllocationSize(message.oneof_decl(), full_name_size, alloc);
  }
}

static void PlanAllocationSize(
    const RepeatedPtrField<MethodDescriptorProto>& methods,
    size_t parent_scope_size, internal::FlatAllocator& alloc) {
  alloc.PlanArray<MethodDescriptor>(methods.size());
  for (const auto& m : methods) {
    alloc.PlanEntityNames(parent_scope_size + 1 + m.name().size());
    if (m.has_options()) alloc.PlanArray<MethodOptions>(1);
  }
}

static void PlanAllocationSize(
    const RepeatedPtrField<ServiceDescriptorProto>& services,
    size_t parent_scope_size, internal::FlatAllocator& alloc) {
  alloc.PlanArray<ServiceDescriptor>(services.size());
  for (const auto& service : services) {
    if (service.has_options()) alloc.PlanArray<ServiceOptions>(1);
    const size_t full_name_size =
        parent_scope_size ? parent_scope_size + 1 + service.name().size()
                          : service.name().size();
    alloc.PlanEntityNames(full_name_size);
    PlanAllocationSize(service.method(), full_name_size, alloc);
  }
}

static void PlanAllocationSize(const FileDescriptorProto& proto,
                               internal::FlatAllocator& alloc) {
  alloc.PlanArray<FileDescriptor>(1);
  alloc.PlanArray<FileDescriptorTables>(1);
  alloc.PlanArray<std::string>(2);  // name + package
  if (proto.has_options()) alloc.PlanArray<FileOptions>(1);
  if (proto.has_source_code_info()) alloc.PlanArray<SourceCodeInfo>(1);

  PlanAllocationSize(proto.service(), proto.package().size(), alloc);
  PlanAllocationSize(proto.message_type(), proto.package().size(), alloc);
  PlanAllocationSize(proto.enum_type(), proto.package().size(), alloc);
  PlanAllocationSize(proto.extension(), proto.package().size(), alloc);

  alloc.PlanArray<int>(proto.weak_dependency_size());
  alloc.PlanArray<int>(proto.public_dependency_size());
  alloc.PlanArray<const FileDescriptor*>(proto.dependency_size());
  alloc.PlanArray<absl::string_view>(proto.option_dependency_size());
  for (int i = 0; i < proto.option_dependency_size(); i++) {
    alloc.PlanArray<char>(proto.option_dependency(i).size());
  }
}

const FileDescriptor* DescriptorBuilder::BuildFile(
    const FileDescriptorProto& proto) {
  // Ensure the generated pool has been lazily initialized.  This is most
  // important for protos that use C++-specific features, since that extension
  // is only registered lazily and we always parse options into the generated
  // pool.
  if (pool_ != DescriptorPool::internal_generated_pool()) {
    DescriptorPool::generated_pool();
  }

  filename_ = proto.name();

  // Check if the file already exists and is identical to the one being built.
  // Note:  This only works if the input is canonical -- that is, it
  //   fully-qualifies all type names, has no UninterpretedOptions, etc.
  //   This is fine, because this idempotency "feature" really only exists to
  //   accommodate one hack in the proto1->proto2 migration layer.
  const FileDescriptor* existing_file = tables_->FindFile(filename_);
  if (existing_file != nullptr) {
    // File already in pool.  Compare the existing one to the input.
    if (ExistingFileMatchesProto(existing_file->edition(), existing_file,
                                 proto)) {
      // They're identical.  Return the existing descriptor.
      return existing_file;
    }

    // Not a match.  The error will be detected and handled later.
  }

  // Check to see if this file is already on the pending files list.
  // TODO:  Allow recursive imports?  It may not work with some
  //   (most?) programming languages.  E.g., in C++, a forward declaration
  //   of a type is not sufficient to allow it to be used even in a
  //   generated header file due to inlining.  This could perhaps be
  //   worked around using tricks involving inserting #include statements
  //   mid-file, but that's pretty ugly, and I'm pretty sure there are
  //   some languages out there that do not allow recursive dependencies
  //   at all.
  for (size_t i = 0; i < tables_->pending_files_.size(); i++) {
    if (tables_->pending_files_[i] == proto.name()) {
      AddRecursiveImportError(proto, i);
      return nullptr;
    }
  }

  static const int kMaximumPackageLength = 511;
  if (proto.package().size() > kMaximumPackageLength) {
    AddError(proto.package(), proto, DescriptorPool::ErrorCollector::NAME,
             "Package name is too long");
    return nullptr;
  }

  // If we have a fallback_database_, and we aren't doing lazy import building,
  // attempt to load all dependencies now, before checkpointing tables_.  This
  // avoids confusion with recursive checkpoints.
  if (!pool_->lazily_build_dependencies_) {
    if (pool_->fallback_database_ != nullptr) {
      tables_->pending_files_.push_back(proto.name());
      for (int i = 0;
           i < proto.dependency_size() + proto.option_dependency_size(); i++) {
        absl::string_view name =
            i >= proto.dependency_size()
                ? proto.option_dependency(i - proto.dependency_size())
                : proto.dependency(i);
        if (tables_->FindFile(name) == nullptr &&
            (pool_->underlay_ == nullptr ||
             pool_->underlay_->FindFileByName(name) == nullptr)) {
          // We don't care what this returns since we'll find out below anyway.
          pool_->TryFindFileInFallbackDatabase(name, deferred_validation_);
        }
      }
      tables_->pending_files_.pop_back();
    }
  }

  // Checkpoint the tables so that we can roll back if something goes wrong.
  tables_->AddCheckpoint();

  auto alloc = absl::make_unique<internal::FlatAllocator>();
  PlanAllocationSize(proto, *alloc);
  alloc->FinalizePlanning(tables_);
  FileDescriptor* result = BuildFileImpl(proto, *alloc);

  file_tables_->FinalizeTables();
  if (result) {
    tables_->ClearLastCheckpoint();
    result->finished_building_ = true;
    alloc->ExpectConsumed();
  } else {
    tables_->RollbackToLastCheckpoint(deferred_validation_);
  }

  return result;
}

FileDescriptor* DescriptorBuilder::BuildFileImpl(
    const FileDescriptorProto& proto, internal::FlatAllocator& alloc) {
  FileDescriptor* result = alloc.AllocateArray<FileDescriptor>(1);
  file_ = result;

  if (proto.has_edition()) {
    file_->edition_ = proto.edition();
  } else if (proto.syntax().empty() || proto.syntax() == "proto2") {
    file_->edition_ = Edition::EDITION_PROTO2;
  } else if (proto.syntax() == "proto3") {
    file_->edition_ = Edition::EDITION_PROTO3;
  } else {
    file_->edition_ = Edition::EDITION_UNKNOWN;
    AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER, [&] {
      return absl::StrCat("Unrecognized syntax: ", proto.syntax());
    });
  }

  const FeatureSetDefaults& defaults = pool_->GetFeatureSetDefaults();

  absl::StatusOr<FeatureResolver> feature_resolver =
      FeatureResolver::Create(file_->edition_, defaults);
  if (!feature_resolver.ok()) {
    AddError(proto.name(), proto, DescriptorPool::ErrorCollector::EDITIONS,
             [&] { return std::string(feature_resolver.status().message()); });
  } else {
    feature_resolver_.emplace(std::move(feature_resolver).value());
  }

  result->is_placeholder_ = false;
  result->finished_building_ = false;
  SourceCodeInfo* info = nullptr;
  if (proto.has_source_code_info()) {
    info = alloc.AllocateArray<SourceCodeInfo>(1);
    *info = proto.source_code_info();
    result->source_code_info_ = info;
  } else {
    result->source_code_info_ = &SourceCodeInfo::default_instance();
  }

  file_tables_ = alloc.AllocateArray<FileDescriptorTables>(1);
  file_->tables_ = file_tables_;

  if (!proto.has_name()) {
    AddError("", proto, DescriptorPool::ErrorCollector::OTHER,
             "Missing field: FileDescriptorProto.name.");
  }

  result->name_ = alloc.AllocateStrings(proto.name());
  if (proto.has_package()) {
    result->package_ = alloc.AllocateStrings(proto.package());
  } else {
    // We cannot rely on proto.package() returning a valid string if
    // proto.has_package() is false, because we might be running at static
    // initialization time, in which case default values have not yet been
    // initialized.
    result->package_ = alloc.AllocateStrings("");
  }
  result->pool_ = pool_;

  if (absl::StrContains(result->name(), '\0')) {
    AddError(result->name(), proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("\"", result->name(), "\" contains null character.");
    });
    return nullptr;
  }

  // Add to tables.
  if (!tables_->AddFile(result)) {
    AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
             "A file with this name is already in the pool.");
    // Bail out early so that if this is actually the exact same file, we
    // don't end up reporting that every single symbol is already defined.
    return nullptr;
  }
  if (!result->package().empty()) {
    if (std::count(result->package().begin(), result->package().end(), '.') >
        kPackageLimit) {
      AddError(result->package(), proto, DescriptorPool::ErrorCollector::NAME,
               "Exceeds Maximum Package Depth");
      return nullptr;
    }
    AddPackage(result->package(), proto, result, true);
  }

  // Make sure all dependencies are loaded.
  absl::flat_hash_set<absl::string_view> seen_dependencies;
  result->dependency_count_ = proto.dependency_size();
  result->dependencies_ =
      alloc.AllocateArray<const FileDescriptor*>(proto.dependency_size());
  result->option_dependency_count_ = proto.option_dependency_size();
  result->option_dependencies_ =
      alloc.AllocateArray<absl::string_view>(proto.option_dependency_size());
  // Copy option dependency names to result->option_dependencies_.
  for (int i = 0; i < proto.option_dependency_size(); ++i) {
    result->option_dependencies_[i] =
        alloc.AllocateStringView(proto.option_dependency(i));
  }

  std::vector<const FileDescriptor*> result_option_dependencies(
      proto.option_dependency_size());
  result->dependencies_once_ = nullptr;
  unused_dependency_.clear();
  absl::flat_hash_set<int> weak_deps;
  for (int i = 0; i < proto.weak_dependency_size(); ++i) {
    weak_deps.insert(proto.weak_dependency(i));
  }

  bool need_lazy_deps = false;
  for (int i = 0; i < proto.dependency_size() + proto.option_dependency_size();
       i++) {
    bool is_option_dep = i >= proto.dependency_size();
    absl::string_view name =
        is_option_dep ? proto.option_dependency(i - proto.dependency_size())
                      : proto.dependency(i);
    if (!seen_dependencies.insert(name).second) {
      AddTwiceListedError(proto, name);
    }

    const FileDescriptor* dependency = tables_->FindFile(name);
    if (dependency == nullptr && pool_->underlay_ != nullptr) {
      dependency = pool_->underlay_->FindFileByName(name);
    }

    if (dependency == result) {
      // Recursive import.  dependency/result is not fully initialized, and it's
      // dangerous to try to do anything with it.  The recursive import error
      // will be detected and reported in DescriptorBuilder::BuildFile().
      return nullptr;
    }

    if (dependency == nullptr) {
      if (!pool_->lazily_build_dependencies_) {
        if (pool_->allow_unknown_ ||
            (!pool_->enforce_weak_ && weak_deps.contains(i)) ||
            (!pool_->enforce_option_ && is_option_dep)) {
          internal::FlatAllocator lazy_dep_alloc;
          lazy_dep_alloc.PlanArray<FileDescriptor>(1);
          lazy_dep_alloc.PlanArray<std::string>(1);
          lazy_dep_alloc.FinalizePlanning(tables_);
          dependency =
              pool_->NewPlaceholderFileWithMutexHeld(name, lazy_dep_alloc);
        } else {
          AddImportError(proto, name);
        }
      }
    } else {
      // Add to unused_dependency_ to track unused imported files.
      // Note: do not track unused imported files for public import.
      if (pool_->enforce_dependencies_ &&
          (pool_->direct_input_files_.find(proto.name()) !=
           pool_->direct_input_files_.end()) &&
          (dependency->public_dependency_count() == 0)) {
        unused_dependency_.insert(dependency);
      }
    }
    if (is_option_dep) {
      result_option_dependencies[i - proto.dependency_size()] = dependency;
    } else {
      result->dependencies_[i] = dependency;
    }
    if (pool_->lazily_build_dependencies_ && !dependency) {
      need_lazy_deps = true;
    }
  }

  if (need_lazy_deps) {
    // Options dependencies may be missing. They are no longer needed after
    // options interpretation and may be discarded by protoc.
    int total_char_size = 0;
    for (int i = 0; i < proto.dependency_size(); i++) {
      const FileDescriptor* result_dependency = result->dependencies_[i];
      if (result_dependency == nullptr) {
        total_char_size += static_cast<int>(proto.dependency(i).size());
      }
      ++total_char_size;  // For NUL char
    }

    void* data = tables_->AllocateBytes(
        static_cast<int>(sizeof(absl::once_flag)) + total_char_size);
    result->dependencies_once_ = ::new (data) absl::once_flag{};
    char* name_data = reinterpret_cast<char*>(result->dependencies_once_ + 1);
    for (int i = 0; i < proto.dependency_size(); i++) {
      if (result->dependencies_[i] == nullptr) {
        memcpy(name_data, proto.dependency(i).data(),
               proto.dependency(i).size());
        name_data += proto.dependency(i).size();
      }
      *name_data++ = '\0';
    }
  }

  // Check public dependencies.
  int public_dependency_count = 0;
  result->public_dependencies_ =
      alloc.AllocateArray<int>(proto.public_dependency_size());
  for (int i = 0; i < proto.public_dependency_size(); i++) {
    // Only put valid public dependency indexes.
    int index = proto.public_dependency(i);
    if (index >= 0 && index < proto.dependency_size()) {
      result->public_dependencies_[public_dependency_count++] = index;
      // Do not track unused imported files for public import.
      // Calling dependency(i) builds that file when doing lazy imports,
      // need to avoid doing this. Unused dependency detection isn't done
      // when building lazily, anyways.
      if (!pool_->lazily_build_dependencies_) {
        unused_dependency_.erase(result->dependency(index));
      }
    } else {
      AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
               "Invalid public dependency index.");
    }
  }
  result->public_dependency_count_ = public_dependency_count;

  // Build dependency set
  dependencies_.clear();
  option_dependencies_.clear();
  // We don't/can't do proper dependency error checking when
  // lazily_build_dependencies_, and calling dependency(i) will force
  // a dependency to be built, which we don't want.
  if (!pool_->lazily_build_dependencies_) {
    for (int i = 0; i < result->dependency_count(); i++) {
      RecordPublicDependencies(result->dependency(i));
    }
    for (const FileDescriptor* option_dependency : result_option_dependencies) {
      RecordPublicOptionDependencies(option_dependency);
    }
  }

  // Check weak dependencies.
  int weak_dependency_count = 0;
  result->weak_dependencies_ =
      alloc.AllocateArray<int>(proto.weak_dependency_size());
  for (int i = 0; i < proto.weak_dependency_size(); i++) {
    int index = proto.weak_dependency(i);
    if (index >= 0 && index < proto.dependency_size()) {
      result->weak_dependencies_[weak_dependency_count++] = index;
    } else {
      AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
               "Invalid weak dependency index.");
    }
  }
  result->weak_dependency_count_ = weak_dependency_count;

  // Convert children.
  BUILD_ARRAY(proto, result, message_type, BuildMessage, nullptr);
  BUILD_ARRAY(proto, result, enum_type, BuildEnum, nullptr);
  BUILD_ARRAY(proto, result, service, BuildService, nullptr);
  BUILD_ARRAY(proto, result, extension, BuildExtension, nullptr);

  // Copy options.
  AllocateOptions(proto, result, alloc);

  // Note that the following steps must occur in exactly the specified order.

  // Cross-link.
  CrossLinkFile(result, proto);

  if (!message_hints_.empty()) {
    SuggestFieldNumbers(result, proto);
  }

  // Interpret only the non-extension options first, including features and
  // their extensions.  This has to be done in two passes, since option
  // extensions defined in this file may have features attached to them.
  if (!had_errors_) {
    OptionInterpreter option_interpreter(this);
    for (std::vector<OptionsToInterpret>::iterator iter =
             options_to_interpret_.begin();
         iter != options_to_interpret_.end(); ++iter) {
      option_interpreter.InterpretNonExtensionOptions(&(*iter));
    }

    // Handle feature resolution.  This must occur after option interpretation,
    // but before validation.
    {
      auto cleanup = DisableTracking();
      internal::VisitDescriptors(
          *result, proto, [&](const auto& descriptor, const auto& proto) {
            using OptionsT =
                typename std::remove_const<typename std::remove_pointer<
                    decltype(descriptor.options_)>::type>::type;
            using DescriptorT =
                typename std::remove_const<typename std::remove_reference<
                    decltype(descriptor)>::type>::type;

            ResolveFeatures(
                proto, const_cast<DescriptorT*>(&descriptor),
                const_cast<  // NOLINT(google3-runtime-proto-const-cast)
                    OptionsT*>(descriptor.options_),
                alloc);
          });
    }

    // Post-process cleanup for field features.
    internal::VisitDescriptors(
        *result, proto,
        [&](const FieldDescriptor& field, const FieldDescriptorProto& proto) {
          PostProcessFieldFeatures(const_cast<FieldDescriptor&>(field), proto);
        });

    // Interpret any remaining uninterpreted options gathered into
    // options_to_interpret_ during descriptor building.  Cross-linking has made
    // extension options known, so all interpretations should now succeed.
    for (std::vector<OptionsToInterpret>::iterator iter =
             options_to_interpret_.begin();
         iter != options_to_interpret_.end(); ++iter) {
      option_interpreter.InterpretOptionExtensions(&(*iter));
    }
    options_to_interpret_.clear();
    if (info != nullptr) {
      option_interpreter.UpdateSourceCodeInfo(info);
    }
  }

  // Validate options. See comments at InternalSetLazilyBuildDependencies about
  // error checking and lazy import building.
  if (!had_errors_ && !pool_->lazily_build_dependencies_) {
    internal::VisitDescriptors(
        *result, proto, [&](const auto& descriptor, const auto& desc_proto) {
          ValidateOptions(&descriptor, desc_proto);
        });
  }

  // Additional naming conflict check for map entry types. Only need to check
  // this if there are already errors.
  if (had_errors_) {
    for (int i = 0; i < proto.message_type_size(); ++i) {
      DetectMapConflicts(result->message_type(i), proto.message_type(i));
    }
  }


  // Again, see comments at InternalSetLazilyBuildDependencies about error
  // checking. Also, don't log unused dependencies if there were previous
  // errors, since the results might be inaccurate.
  if (!had_errors_ && !unused_dependency_.empty() &&
      !pool_->lazily_build_dependencies_) {
    LogUnusedDependency(proto, result);
  }

  // Store feature information for deferred validation outside of the database
  // mutex.
  if (!had_errors_ && !pool_->lazily_build_dependencies_) {
    internal::VisitDescriptors(
        *result, proto, [&](const auto& descriptor, const auto& desc_proto) {
          if (descriptor.proto_features_ != &FeatureSet::default_instance()) {
            deferred_validation_.ValidateFeatureLifetimes(
                GetFile(descriptor), {descriptor.proto_features_, &desc_proto,
                                      GetFullName(descriptor), proto.name()});
          }
        });
  }

  if (!had_errors_ && pool_->enforce_naming_style_) {
    internal::VisitDescriptors(
        *result, proto, [&](const auto& descriptor, const auto& desc_proto) {
          if (internal::InternalFeatureHelper::GetFeatures(descriptor)
                  .enforce_naming_style() == FeatureSet::STYLE2024) {
            ValidateNamingStyle(&descriptor, desc_proto);
          }
        });
  }
  if (!had_errors_) {
    // Check Symbol Visibility Rules.
    CheckVisibilityRules(result, proto);
  }

  if (had_errors_) {
    return nullptr;
  } else {
    return result;
  }
}


auto DescriptorBuilder::AllocateNameStrings(const absl::string_view scope,
                                            const absl::string_view proto_name,
                                            const Message& entity,
                                            internal::FlatAllocator& alloc) {
  if (auto names = alloc.AllocateEntityNames(scope, proto_name)) {
    return *names;
  }

  AddError(scope.empty() ? proto_name : absl::StrCat(scope, ".", proto_name),
           entity, DescriptorPool::ErrorCollector::NAME, "Name too long.");
  // Return any valid name for the caller to use and keep going.
  return alloc.AllocateEntityNames("", "unknown").value();
}

namespace {

// Helper for BuildMessage below.
struct IncrementWhenDestroyed {
  ~IncrementWhenDestroyed() { ++to_increment; }
  int& to_increment;
};

}  // namespace

namespace {
bool IsNonMessageType(absl::string_view type) {
  static const auto* non_message_types =
      new absl::flat_hash_set<absl::string_view>(
          {"double", "float", "int64", "uint64", "int32", "fixed32", "fixed64",
           "bool", "string", "bytes", "uint32", "enum", "sfixed32", "sfixed64",
           "sint32", "sint64"});
  return non_message_types->contains(type);
}
}  // namespace


void DescriptorBuilder::BuildMessage(const DescriptorProto& proto,
                                     const Descriptor* parent,
                                     Descriptor* result,
                                     internal::FlatAllocator& alloc) {
  const absl::string_view scope =
      (parent == nullptr) ? file_->package() : parent->full_name();
  result->all_names_ = AllocateNameStrings(scope, proto.name(), proto, alloc);
  ValidateSymbolName(proto.name(), result->full_name(), proto);

  result->file_ = file_;
  result->containing_type_ = parent;
  result->is_placeholder_ = false;
  result->is_unqualified_placeholder_ = false;
  result->well_known_type_ = Descriptor::WELLKNOWNTYPE_UNSPECIFIED;
  result->options_ = nullptr;  // Set to default_instance later if necessary.
  result->visibility_ = static_cast<uint8_t>(proto.visibility());

  auto it = pool_->tables_->well_known_types_.find(result->full_name());
  if (it != pool_->tables_->well_known_types_.end()) {
    result->well_known_type_ = it->second;
  }

  // Calculate the continuous sequence of fields.
  // These can be fast-path'd during lookup and don't need to be added to the
  // tables.
  // We use uint16_t to save space for sequential_field_limit_, so stop before
  // overflowing it. Worst case, we are not taking full advantage on huge
  // messages, but it is unlikely.
  result->sequential_field_limit_ = 0;
  for (int i = 0; i < std::numeric_limits<uint16_t>::max() &&
                  i < proto.field_size() && proto.field(i).number() == i + 1;
       ++i) {
    result->sequential_field_limit_ = i + 1;
  }

  // Build oneofs first so that fields and extension ranges can refer to them.
  BUILD_ARRAY(proto, result, oneof_decl, BuildOneof, result);
  BUILD_ARRAY(proto, result, field, BuildField, result);
  BUILD_ARRAY(proto, result, enum_type, BuildEnum, result);
  BUILD_ARRAY(proto, result, extension_range, BuildExtensionRange, result);
  BUILD_ARRAY(proto, result, extension, BuildExtension, result);
  BUILD_ARRAY(proto, result, reserved_range, BuildReservedRange, result);

  // Copy options.
  AllocateOptions(proto, result, DescriptorProto::kOptionsFieldNumber,
                  "google.protobuf.MessageOptions", alloc);

  // Before building submessages, check recursion limit.
  --recursion_depth_;
  IncrementWhenDestroyed revert{recursion_depth_};
  if (recursion_depth_ <= 0) {
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::OTHER,
             "Reached maximum recursion limit for nested messages.");
    result->nested_types_ = nullptr;
    result->nested_type_count_ = 0;
    return;
  }
  BUILD_ARRAY(proto, result, nested_type, BuildMessage, result);

  // Copy reserved names.
  int reserved_name_count = proto.reserved_name_size();
  result->reserved_name_count_ = reserved_name_count;
  result->reserved_names_ =
      alloc.AllocateArray<const std::string*>(reserved_name_count);
  for (int i = 0; i < reserved_name_count; ++i) {
    result->reserved_names_[i] = alloc.AllocateStrings(proto.reserved_name(i));
  }

  AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));

  for (int i = 0; i < proto.reserved_range_size(); i++) {
    const DescriptorProto_ReservedRange& range1 = proto.reserved_range(i);
    for (int j = i + 1; j < proto.reserved_range_size(); j++) {
      const DescriptorProto_ReservedRange& range2 = proto.reserved_range(j);
      if (range1.end() > range2.start() && range2.end() > range1.start()) {
        AddError(result->full_name(), proto.reserved_range(i),
                 DescriptorPool::ErrorCollector::NUMBER, [&] {
                   return absl::Substitute(
                       "Reserved range $0 to $1 overlaps with "
                       "already-defined range $2 to $3.",
                       range2.start(), range2.end() - 1, range1.start(),
                       range1.end() - 1);
                 });
      }
    }
  }

  absl::flat_hash_set<absl::string_view> reserved_name_set;
  for (const std::string& name : proto.reserved_name()) {
    if (!reserved_name_set.insert(name).second) {
      AddError(name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
        return absl::Substitute("Field name \"$0\" is reserved multiple times.",
                                name);
      });
    }
  }
  // Check that fields aren't using reserved names or numbers and that they
  // aren't using extension numbers.
  for (int i = 0; i < result->field_count(); i++) {
    const FieldDescriptor* field = result->field(i);
    for (int j = 0; j < result->extension_range_count(); j++) {
      const Descriptor::ExtensionRange* range = result->extension_range(j);
      if (range->start_number() <= field->number() &&
          field->number() < range->end_number()) {
        message_hints_[result].RequestHintOnFieldNumbers(
            proto.extension_range(j), DescriptorPool::ErrorCollector::NUMBER);
        AddError(field->full_name(), proto.extension_range(j),
                 DescriptorPool::ErrorCollector::NUMBER, [&] {
                   return absl::Substitute(
                       "Extension range $0 to $1 includes field \"$2\" ($3).",
                       range->start_number(), range->end_number() - 1,
                       field->name(), field->number());
                 });
      }
    }
    for (int j = 0; j < result->reserved_range_count(); j++) {
      const Descriptor::ReservedRange* range = result->reserved_range(j);
      if (range->start <= field->number() && field->number() < range->end) {
        message_hints_[result].RequestHintOnFieldNumbers(
            proto.reserved_range(j), DescriptorPool::ErrorCollector::NUMBER);
        AddError(field->full_name(), proto.reserved_range(j),
                 DescriptorPool::ErrorCollector::NUMBER, [&] {
                   return absl::Substitute(
                       "Field \"$0\" uses reserved number $1.", field->name(),
                       field->number());
                 });
      }
    }
    if (reserved_name_set.contains(field->name())) {
      AddError(field->full_name(), proto.field(i),
               DescriptorPool::ErrorCollector::NAME, [&] {
                 return absl::Substitute("Field name \"$0\" is reserved.",
                                         field->name());
               });
    }
  }

  // Check that extension ranges don't overlap and don't include
  // reserved field numbers or names.
  for (int i = 0; i < result->extension_range_count(); i++) {
    const Descriptor::ExtensionRange* range1 = result->extension_range(i);
    for (int j = 0; j < result->reserved_range_count(); j++) {
      const Descriptor::ReservedRange* range2 = result->reserved_range(j);
      if (range1->end_number() > range2->start &&
          range2->end > range1->start_number()) {
        AddError(result->full_name(), proto.extension_range(i),
                 DescriptorPool::ErrorCollector::NUMBER, [&] {
                   return absl::Substitute(
                       "Extension range $0 to $1 overlaps with "
                       "reserved range $2 to $3.",
                       range1->start_number(), range1->end_number() - 1,
                       range2->start, range2->end - 1);
                 });
      }
    }
    for (int j = i + 1; j < result->extension_range_count(); j++) {
      const Descriptor::ExtensionRange* range2 = result->extension_range(j);
      if (range1->end_number() > range2->start_number() &&
          range2->end_number() > range1->start_number()) {
        AddError(result->full_name(), proto.extension_range(i),
                 DescriptorPool::ErrorCollector::NUMBER, [&] {
                   return absl::Substitute(
                       "Extension range $0 to $1 overlaps with "
                       "already-defined range $2 to $3.",
                       range2->start_number(), range2->end_number() - 1,
                       range1->start_number(), range1->end_number() - 1);
                 });
      }
    }
  }
}

void DescriptorBuilder::CheckFieldJsonNameUniqueness(
    const DescriptorProto& proto, const Descriptor* result) {
  const absl::string_view message_name = result->full_name();
  if (!pool_->deprecated_legacy_json_field_conflicts_ &&
      !IsLegacyJsonFieldConflictEnabled(result->options())) {
    // Check both with and without taking json_name into consideration.  This is
    // needed for field masks, which don't use json_name.
    CheckFieldJsonNameUniqueness(message_name, proto, result, false);
    CheckFieldJsonNameUniqueness(message_name, proto, result, true);
  }
}

namespace {
// Helpers for function below

struct JsonNameDetails {
  const FieldDescriptorProto* field;
  std::string orig_name;
  bool is_custom;
};

JsonNameDetails GetJsonNameDetails(const FieldDescriptorProto* field,
                                   bool use_custom) {
  std::string default_json_name = ToJsonName(field->name());
  if (use_custom && field->has_json_name() &&
      field->json_name() != default_json_name) {
    return {field, field->json_name(), true};
  }
  return {field, std::move(default_json_name), false};
}

bool JsonNameLooksLikeExtension(std::string name) {
  return !name.empty() && name.front() == '[' && name.back() == ']';
}

}  // namespace

void DescriptorBuilder::CheckFieldJsonNameUniqueness(
    const absl::string_view message_name, const DescriptorProto& message,
    const Descriptor* descriptor, bool use_custom_names) {
  absl::flat_hash_map<std::string, JsonNameDetails> name_to_field;
  for (const FieldDescriptorProto& field : message.field()) {
    JsonNameDetails details = GetJsonNameDetails(&field, use_custom_names);
    if (details.is_custom && JsonNameLooksLikeExtension(details.orig_name)) {
      auto make_error = [&] {
        return absl::StrFormat(
            "The custom JSON name of field \"%s\" (\"%s\") is invalid: "
            "JSON names may not start with '[' and end with ']'.",
            field.name(), details.orig_name);
      };
      AddError(message_name, field, DescriptorPool::ErrorCollector::NAME,
               make_error);
      continue;
    }
    auto it_inserted = name_to_field.try_emplace(details.orig_name, details);
    if (it_inserted.second) {
      continue;
    }
    JsonNameDetails& match = it_inserted.first->second;
    if (use_custom_names && !details.is_custom && !match.is_custom) {
      // if this pass is considering custom JSON names, but neither of the
      // names involved in the conflict are custom, don't bother with a
      // message. That will have been reported from other pass (non-custom
      // JSON names).
      continue;
    }
    auto make_error = [&] {
      absl::string_view this_type = details.is_custom ? "custom" : "default";
      absl::string_view existing_type = match.is_custom ? "custom" : "default";
      // If the matched name differs (which it can only differ in case), include
      // it in the error message, for maximum clarity to user.
      std::string name_suffix = "";
      if (details.orig_name != match.orig_name) {
        name_suffix = absl::StrCat(" (\"", match.orig_name, "\")");
      }
      return absl::StrFormat(
          "The %s JSON name of field \"%s\" (\"%s\") conflicts "
          "with the %s JSON name of field \"%s\"%s.",
          this_type, field.name(), details.orig_name, existing_type,
          match.field->name(), name_suffix);
    };

    bool involves_default = !details.is_custom || !match.is_custom;
    if (descriptor->features().json_format() ==
            FeatureSet::LEGACY_BEST_EFFORT &&
        involves_default) {
      // TODO Upgrade this to an error once downstream protos
      // have been fixed.
      AddWarning(message_name, field, DescriptorPool::ErrorCollector::NAME,
                 make_error);
    } else {
      AddError(message_name, field, DescriptorPool::ErrorCollector::NAME,
               make_error);
    }
  }
}

void DescriptorBuilder::BuildFieldOrExtension(const FieldDescriptorProto& proto,
                                              Descriptor* parent,
                                              FieldDescriptor* result,
                                              bool is_extension,
                                              internal::FlatAllocator& alloc) {
  const absl::string_view scope =
      (parent == nullptr) ? file_->package() : parent->full_name();

  // We allocate all names in a single array, and dedup them.
  if (auto names = alloc.AllocateFieldNames(
          proto.name(), scope,
          proto.has_json_name() ? &proto.json_name() : nullptr)) {
    result->all_names_ = *names;
  } else {
    AddError(
        scope.empty() ? proto.name() : absl::StrCat(scope, ".", proto.name()),
        proto, DescriptorPool::ErrorCollector::NAME, "Name too long.");
    result->all_names_ = alloc.AllocateEntityNames("", "unknown").value();
  }

  ValidateSymbolName(proto.name(), result->full_name(), proto);

  result->file_ = file_;
  result->number_ = proto.number();
  result->is_extension_ = is_extension;
  result->is_oneof_ = false;
  result->in_real_oneof_ = false;
  result->is_map_ = false;
  result->proto3_optional_ = proto.proto3_optional();
  result->legacy_proto_ctype_ = FieldOptions::CType_MAX + 1;
  // We initialize to STRING because descriptor.proto needs it for
  // bootstrapping.
  result->cpp_string_type_ =
      static_cast<uint8_t>(FieldDescriptor::CppStringType::kString);

  if (proto.proto3_optional() && file_->edition() != Edition::EDITION_PROTO3) {
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             [&] {
               return absl::StrCat(
                   "The [proto3_optional=true] option may only be set on proto3"
                   "fields, not ",
                   result->full_name());
             });
  }

  result->has_json_name_ = proto.has_json_name();

  result->type_ = proto.type();
  result->label_ = proto.label();
  result->is_repeated_ = result->label_ == FieldDescriptor::LABEL_REPEATED;

  if (result->label_ == FieldDescriptor::LABEL_REQUIRED) {
    // An extension cannot have a required field (b/13365836).
    if (result->is_extension_) {
      AddError(result->full_name(), proto,
               // Error location `TYPE`: we would really like to indicate
               // `LABEL`, but the `ErrorLocation` enum has no entry for this,
               // and we don't necessarily know about all implementations of the
               // `ErrorCollector` interface to extend them to handle the new
               // error location type properly.
               DescriptorPool::ErrorCollector::TYPE, [&] {
                 return absl::StrCat("The extension ", result->full_name(),
                                     " cannot be required.");
               });
    }
  }

  // Some of these may be filled in when cross-linking.
  result->containing_type_ = nullptr;
  result->type_once_ = nullptr;
  result->default_value_enum_ = nullptr;

  result->has_default_value_ = proto.has_default_value();
  if (proto.has_default_value() && result->is_repeated()) {
    AddError(result->full_name(), proto,
             DescriptorPool::ErrorCollector::DEFAULT_VALUE,
             "Repeated fields can't have default values.");
  }

  if (proto.has_type()) {
    if (proto.has_default_value()) {
      char* end_pos = nullptr;
      switch (result->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
          result->default_value_int32_t_ =
              std::strtol(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_INT64:
          static_assert(sizeof(int64_t) == sizeof(long long),  // NOLINT
                        "sizeof int64_t is not sizeof long long");
          result->default_value_int64_t_ =
              std::strtoll(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_UINT32:
          result->default_value_uint32_t_ =
              std::strtoul(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_UINT64:
          static_assert(
              sizeof(uint64_t) == sizeof(unsigned long long),  // NOLINT
              "sizeof uint64_t is not sizeof unsigned long long");
          result->default_value_uint64_t_ =
              std::strtoull(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_FLOAT:
          if (proto.default_value() == "inf") {
            result->default_value_float_ =
                std::numeric_limits<float>::infinity();
          } else if (proto.default_value() == "-inf") {
            result->default_value_float_ =
                -std::numeric_limits<float>::infinity();
          } else if (proto.default_value() == "nan") {
            result->default_value_float_ =
                std::numeric_limits<float>::quiet_NaN();
          } else {
            result->default_value_float_ = io::SafeDoubleToFloat(
                io::NoLocaleStrtod(proto.default_value().c_str(), &end_pos));
          }
          break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
          if (proto.default_value() == "inf") {
            result->default_value_double_ =
                std::numeric_limits<double>::infinity();
          } else if (proto.default_value() == "-inf") {
            result->default_value_double_ =
                -std::numeric_limits<double>::infinity();
          } else if (proto.default_value() == "nan") {
            result->default_value_double_ =
                std::numeric_limits<double>::quiet_NaN();
          } else {
            result->default_value_double_ =
                io::NoLocaleStrtod(proto.default_value().c_str(), &end_pos);
          }
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          if (proto.default_value() == "true") {
            result->default_value_bool_ = true;
          } else if (proto.default_value() == "false") {
            result->default_value_bool_ = false;
          } else {
            AddError(result->full_name(), proto,
                     DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                     "Boolean default must be true or false.");
          }
          break;
        case FieldDescriptor::CPPTYPE_ENUM:
          // This will be filled in when cross-linking.
          result->default_value_enum_ = nullptr;
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          if (result->type() == FieldDescriptor::TYPE_BYTES) {
            std::string value;
            if (absl::CUnescape(proto.default_value(), &value)) {
              result->default_value_string_ = alloc.AllocateStrings(value);
            } else {
              AddError(result->full_name(), proto,
                       DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                       "Invalid escaping in default value.");
            }
          } else {
            result->default_value_string_ =
                alloc.AllocateStrings(proto.default_value());
          }
          break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
          AddError(result->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                   "Messages can't have default values.");
          result->has_default_value_ = false;
          result->default_generated_instance_ = nullptr;
          break;
      }

      if (end_pos != nullptr) {
        // end_pos is only set non-null by the parsers for numeric types,
        // above. This checks that the default was non-empty and had no extra
        // junk after the end of the number.
        if (proto.default_value().empty() || *end_pos != '\0') {
          AddError(result->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE, [&] {
                     return absl::StrCat("Couldn't parse default value \"",
                                         proto.default_value(), "\".");
                   });
        }
      }
    } else {
      // No explicit default value
      switch (result->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
          result->default_value_int32_t_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_INT64:
          result->default_value_int64_t_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_UINT32:
          result->default_value_uint32_t_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_UINT64:
          result->default_value_uint64_t_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_FLOAT:
          result->default_value_float_ = 0.0f;
          break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
          result->default_value_double_ = 0.0;
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          result->default_value_bool_ = false;
          break;
        case FieldDescriptor::CPPTYPE_ENUM:
          // This will be filled in when cross-linking.
          result->default_value_enum_ = nullptr;
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          result->default_value_string_ = &internal::GetEmptyString();
          break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
          result->default_generated_instance_ = nullptr;
          break;
      }
    }
  }

  if (result->number() <= 0) {
    message_hints_[parent].RequestHintOnFieldNumbers(
        proto, DescriptorPool::ErrorCollector::NUMBER);
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             "Field numbers must be positive integers.");
  } else if (!is_extension && result->number() > FieldDescriptor::kMaxNumber) {
    // Only validate that the number is within the valid field range if it is
    // not an extension. Since extension numbers are validated with the
    // extendee's valid set of extension numbers, and those are in turn
    // validated against the max allowed number, the check is unnecessary for
    // extension fields.
    // This avoids cross-linking issues that arise when attempting to check if
    // the extendee is a message_set_wire_format message, which has a higher max
    // on extension numbers.
    message_hints_[parent].RequestHintOnFieldNumbers(
        proto, DescriptorPool::ErrorCollector::NUMBER);
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             [&] {
               return absl::Substitute(
                   "Field numbers cannot be greater than $0.",
                   FieldDescriptor::kMaxNumber);
             });
  }

  if (is_extension) {
    if (!proto.has_extendee()) {
      AddError(result->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE,
               "FieldDescriptorProto.extendee not set for extension field.");
    }

    result->scope_.extension_scope = parent;

    if (proto.has_oneof_index()) {
      AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "FieldDescriptorProto.oneof_index should not be set for "
               "extensions.");
    }
  } else {
    if (proto.has_extendee()) {
      AddError(result->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE,
               "FieldDescriptorProto.extendee set for non-extension field.");
    }

    result->containing_type_ = parent;

    if (proto.has_oneof_index()) {
      if (proto.oneof_index() < 0 ||
          proto.oneof_index() >= parent->oneof_decl_count()) {
        AddError(result->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE, [&] {
                   return absl::Substitute(
                       "FieldDescriptorProto.oneof_index $0 is "
                       "out of range for type \"$1\".",
                       proto.oneof_index(), parent->name());
                 });
      } else {
        result->is_oneof_ = true;
        result->scope_.containing_oneof =
            parent->oneof_decl(proto.oneof_index());
        result->in_real_oneof_ = !result->proto3_optional_;
      }
    }
  }

  // Copy options.
  AllocateOptions(proto, result, FieldDescriptorProto::kOptionsFieldNumber,
                  "google.protobuf.FieldOptions", alloc);

  AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));
}

void DescriptorBuilder::BuildExtensionRange(
    const DescriptorProto::ExtensionRange& proto, const Descriptor* parent,
    Descriptor::ExtensionRange* result, internal::FlatAllocator& alloc) {
  result->start_ = proto.start();
  result->end_ = proto.end();
  result->containing_type_ = parent;

  if (result->start_number() <= 0) {
    message_hints_[parent].RequestHintOnFieldNumbers(
        proto, DescriptorPool::ErrorCollector::NUMBER, result->start_number(),
        result->end_number());
    AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             "Extension numbers must be positive integers.");
  }

  // Checking of the upper bound of the extension range is deferred until after
  // options interpreting. This allows messages with message_set_wire_format to
  // have extensions beyond FieldDescriptor::kMaxNumber, since the extension
  // numbers are actually used as int32s in the message_set_wire_format.

  if (result->start_number() >= result->end_number()) {
    AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             "Extension range end number must be greater than start number.");
  }

  // Copy options
  AllocateOptions(proto, result,
                  DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                  "google.protobuf.ExtensionRangeOptions", alloc);
}

void DescriptorBuilder::BuildReservedRange(
    const DescriptorProto::ReservedRange& proto, const Descriptor* parent,
    Descriptor::ReservedRange* result, internal::FlatAllocator&) {
  result->start = proto.start();
  result->end = proto.end();
  if (result->start <= 0) {
    message_hints_[parent].RequestHintOnFieldNumbers(
        proto, DescriptorPool::ErrorCollector::NUMBER, result->start,
        result->end);
    AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             "Reserved numbers must be positive integers.");
  }
  if (result->start >= result->end) {
    AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             "Reserved range end number must be greater than start number.");
  }
}

void DescriptorBuilder::BuildReservedRange(
    const EnumDescriptorProto::EnumReservedRange& proto,
    const EnumDescriptor* parent, EnumDescriptor::ReservedRange* result,
    internal::FlatAllocator&) {
  result->start = proto.start();
  result->end = proto.end();

  if (result->start > result->end) {
    AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             "Reserved range end number must be greater than start number.");
  }
}

void DescriptorBuilder::BuildOneof(const OneofDescriptorProto& proto,
                                   Descriptor* parent, OneofDescriptor* result,
                                   internal::FlatAllocator& alloc) {
  result->all_names_ =
      AllocateNameStrings(parent->full_name(), proto.name(), proto, alloc);
  ValidateSymbolName(proto.name(), result->full_name(), proto);

  result->containing_type_ = parent;

  // We need to fill these in later.
  result->field_count_ = 0;
  result->fields_ = nullptr;

  // Copy options.
  AllocateOptions(proto, result, OneofDescriptorProto::kOptionsFieldNumber,
                  "google.protobuf.OneofOptions", alloc);

  AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));
}

void DescriptorBuilder::CheckEnumValueUniqueness(
    const EnumDescriptorProto& proto, const EnumDescriptor* result) {

  // Check that enum labels are still unique when we remove the enum prefix from
  // values that have it.
  //
  // This will fail for something like:
  //
  //   enum MyEnum {
  //     MY_ENUM_FOO = 0;
  //     FOO = 1;
  //   }
  //
  // By enforcing this reasonable constraint, we allow code generators to strip
  // the prefix and/or PascalCase it without creating conflicts.  This can lead
  // to much nicer language-specific enums like:
  //
  //   enum NameType {
  //     FirstName = 1,
  //     LastName = 2,
  //   }
  //
  // Instead of:
  //
  //   enum NameType {
  //     NAME_TYPE_FIRST_NAME = 1,
  //     NAME_TYPE_LAST_NAME = 2,
  //   }
  PrefixRemover remover(result->name());
  absl::flat_hash_map<std::string, const EnumValueDescriptor*> values;
  for (int i = 0; i < result->value_count(); i++) {
    const EnumValueDescriptor* value = result->value(i);
    std::string stripped =
        EnumValueToPascalCase(remover.MaybeRemove(value->name()));
    auto insert_result = values.try_emplace(std::move(stripped), value);
    bool inserted = insert_result.second;

    // We don't throw the error if the two conflicting symbols are identical, or
    // if they map to the same number.  In the former case, the normal symbol
    // duplication error will fire so we don't need to (and its error message
    // will make more sense). We allow the latter case so users can create
    // aliases which add or remove the prefix (code generators that do prefix
    // stripping should de-dup the labels in this case).
    if (!inserted && insert_result.first->second->name() != value->name() &&
        insert_result.first->second->number() != value->number()) {
      auto make_error = [&] {
        return absl::StrFormat(
            "Enum name %s has the same name as %s if you ignore case and strip "
            "out the enum name prefix (if any). (If you are using allow_alias, "
            "please assign the same number to each enum value name.)",
            value->name(), insert_result.first->second->name());
      };
      // There are proto2 enums out there with conflicting names, so to preserve
      // compatibility we issue only a warning for proto2.
      if ((pool_->deprecated_legacy_json_field_conflicts_ ||
           IsLegacyJsonFieldConflictEnabled(result->options())) &&
          result->file()->edition() == Edition::EDITION_PROTO2) {
        AddWarning(value->full_name(), proto.value(i),
                   DescriptorPool::ErrorCollector::NAME, make_error);
        continue;
      }
      AddError(value->full_name(), proto.value(i),
               DescriptorPool::ErrorCollector::NAME, make_error);
    }
  }
}

void DescriptorBuilder::BuildEnum(const EnumDescriptorProto& proto,
                                  const Descriptor* parent,
                                  EnumDescriptor* result,
                                  internal::FlatAllocator& alloc) {
  const absl::string_view scope =
      (parent == nullptr) ? file_->package() : parent->full_name();

  result->all_names_ = AllocateNameStrings(scope, proto.name(), proto, alloc);
  ValidateSymbolName(proto.name(), result->full_name(), proto);
  result->file_ = file_;
  result->containing_type_ = parent;
  result->is_placeholder_ = false;
  result->is_unqualified_placeholder_ = false;
  result->visibility_ = static_cast<uint8_t>(proto.visibility());

  if (proto.value_size() == 0) {
    // We cannot allow enums with no values because this would mean there
    // would be no valid default value for fields of this type.
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Enums must contain at least one value.");
  }

  // Calculate the continuous sequence of the labels.
  // These can be fast-path'd during lookup and don't need to be added to the
  // tables.
  // We use uint16_t to save space for sequential_value_limit_, so stop before
  // overflowing it. Worst case, we are not taking full advantage on huge
  // enums, but it is unlikely.
  for (int i = 0;
       i < std::numeric_limits<uint16_t>::max() && i < proto.value_size() &&
       // We do the math in int64_t to avoid overflows.
       proto.value(i).number() ==
           static_cast<int64_t>(i) + proto.value(0).number();
       ++i) {
    result->sequential_value_limit_ = i;
  }

  BUILD_ARRAY(proto, result, value, BuildEnumValue, result);
  BUILD_ARRAY(proto, result, reserved_range, BuildReservedRange, result);

  // Copy reserved names.
  int reserved_name_count = proto.reserved_name_size();
  result->reserved_name_count_ = reserved_name_count;
  result->reserved_names_ =
      alloc.AllocateArray<const std::string*>(reserved_name_count);
  for (int i = 0; i < reserved_name_count; ++i) {
    result->reserved_names_[i] = alloc.AllocateStrings(proto.reserved_name(i));
  }

  // Copy options.
  AllocateOptions(proto, result, EnumDescriptorProto::kOptionsFieldNumber,
                  "google.protobuf.EnumOptions", alloc);

  AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));

  for (int i = 0; i < proto.reserved_range_size(); i++) {
    const EnumDescriptorProto_EnumReservedRange& range1 =
        proto.reserved_range(i);
    for (int j = i + 1; j < proto.reserved_range_size(); j++) {
      const EnumDescriptorProto_EnumReservedRange& range2 =
          proto.reserved_range(j);
      if (range1.end() >= range2.start() && range2.end() >= range1.start()) {
        AddError(result->full_name(), proto.reserved_range(i),
                 DescriptorPool::ErrorCollector::NUMBER, [&] {
                   return absl::Substitute(
                       "Reserved range $0 to $1 overlaps with "
                       "already-defined range $2 to $3.",
                       range2.start(), range2.end(), range1.start(),
                       range1.end());
                 });
      }
    }
  }

  absl::flat_hash_set<absl::string_view> reserved_name_set;
  for (const std::string& name : proto.reserved_name()) {
    if (!reserved_name_set.insert(name).second) {
      AddError(name, proto, DescriptorPool::ErrorCollector::NAME, [&] {
        return absl::Substitute("Enum value \"$0\" is reserved multiple times.",
                                name);
      });
    }
  }

  for (int i = 0; i < result->value_count(); i++) {
    const EnumValueDescriptor* value = result->value(i);
    for (int j = 0; j < result->reserved_range_count(); j++) {
      const EnumDescriptor::ReservedRange* range = result->reserved_range(j);
      if (range->start <= value->number() && value->number() <= range->end) {
        AddError(value->full_name(), proto.reserved_range(j),
                 DescriptorPool::ErrorCollector::NUMBER, [&] {
                   return absl::Substitute(
                       "Enum value \"$0\" uses reserved number $1.",
                       value->name(), value->number());
                 });
      }
    }
    if (reserved_name_set.contains(value->name())) {
      AddError(value->full_name(), proto.value(i),
               DescriptorPool::ErrorCollector::NAME, [&] {
                 return absl::Substitute("Enum value \"$0\" is reserved.",
                                         value->name());
               });
    }
  }
}

void DescriptorBuilder::BuildEnumValue(const EnumValueDescriptorProto& proto,
                                       const EnumDescriptor* parent,
                                       EnumValueDescriptor* result,
                                       internal::FlatAllocator& alloc) {
  // Note:  full_name for enum values is a sibling to the parent's name, not a
  //   child of it.
  std::string full_name;
  size_t scope_len = parent->full_name().size() - parent->name().size();
  full_name.reserve(scope_len + proto.name().size());
  full_name.append(parent->full_name().data(), scope_len);
  full_name.append(proto.name());

  result->all_names_ =
      alloc.AllocateStrings(proto.name(), std::move(full_name));
  result->number_ = proto.number();
  result->type_ = parent;

  ValidateSymbolName(proto.name(), result->full_name(), proto);

  // Copy options.
  AllocateOptions(proto, result, EnumValueDescriptorProto::kOptionsFieldNumber,
                  "google.protobuf.EnumValueOptions", alloc);

  // Again, enum values are weird because we makes them appear as siblings
  // of the enum type instead of children of it.  So, we use
  // parent->containing_type() as the value's parent.
  bool added_to_outer_scope =
      AddSymbol(result->full_name(), parent->containing_type(), result->name(),
                proto, Symbol::EnumValue(result, 0));

  // However, we also want to be able to search for values within a single
  // enum type, so we add it as a child of the enum type itself, too.
  // Note:  This could fail, but if it does, the error has already been
  //   reported by the above AddSymbol() call, so we ignore the return code.
  bool added_to_inner_scope = file_tables_->AddAliasUnderParent(
      parent, result->name(), Symbol::EnumValue(result, 1));

  if (added_to_inner_scope && !added_to_outer_scope) {
    // This value did not conflict with any values defined in the same enum,
    // but it did conflict with some other symbol defined in the enum type's
    // scope.  Let's print an additional error to explain this.
    std::string outer_scope;
    if (parent->containing_type() == nullptr) {
      outer_scope = std::string(file_->package());
    } else {
      outer_scope = std::string(parent->containing_type()->full_name());
    }

    if (outer_scope.empty()) {
      outer_scope = "the global scope";
    } else {
      outer_scope = absl::StrCat("\"", outer_scope, "\"");
    }

    AddError(
        result->full_name(), proto, DescriptorPool::ErrorCollector::NAME, [&] {
          return absl::StrCat(
              "Note that enum values use C++ scoping rules, meaning that "
              "enum values are siblings of their type, not children of it.  "
              "Therefore, \"",
              result->name(), "\" must be unique within ", outer_scope,
              ", not just within \"", parent->name(), "\".");
        });
  }

  // An enum is allowed to define two numbers that refer to the same value.
  // FindValueByNumber() should return the first such value, so we simply
  // ignore AddEnumValueByNumber()'s return code.
  file_tables_->AddEnumValueByNumber(result);
}

void DescriptorBuilder::BuildService(const ServiceDescriptorProto& proto,
                                     const void* /* dummy */,
                                     ServiceDescriptor* result,
                                     internal::FlatAllocator& alloc) {
  result->all_names_ =
      AllocateNameStrings(file_->package(), proto.name(), proto, alloc);
  result->file_ = file_;
  ValidateSymbolName(proto.name(), result->full_name(), proto);

  BUILD_ARRAY(proto, result, method, BuildMethod, result);

  // Copy options.
  AllocateOptions(proto, result, ServiceDescriptorProto::kOptionsFieldNumber,
                  "google.protobuf.ServiceOptions", alloc);

  AddSymbol(result->full_name(), nullptr, result->name(), proto,
            Symbol(result));
}

void DescriptorBuilder::BuildMethod(const MethodDescriptorProto& proto,
                                    const ServiceDescriptor* parent,
                                    MethodDescriptor* result,
                                    internal::FlatAllocator& alloc) {
  result->service_ = parent;
  result->all_names_ =
      AllocateNameStrings(parent->full_name(), proto.name(), proto, alloc);

  ValidateSymbolName(proto.name(), result->full_name(), proto);

  // These will be filled in when cross-linking.
  result->input_type_.Init();
  result->output_type_.Init();

  // Copy options.
  AllocateOptions(proto, result, MethodDescriptorProto::kOptionsFieldNumber,
                  "google.protobuf.MethodOptions", alloc);

  result->client_streaming_ = proto.client_streaming();
  result->server_streaming_ = proto.server_streaming();

  AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));
}

#undef BUILD_ARRAY

// -------------------------------------------------------------------

void DescriptorBuilder::CrossLinkFile(FileDescriptor* file,
                                      const FileDescriptorProto& proto) {
  for (int i = 0; i < file->message_type_count(); i++) {
    CrossLinkMessage(&file->message_types_[i], proto.message_type(i));
  }

  for (int i = 0; i < file->extension_count(); i++) {
    CrossLinkField(&file->extensions_[i], proto.extension(i));
  }

  for (int i = 0; i < file->service_count(); i++) {
    CrossLinkService(&file->services_[i], proto.service(i));
  }
}

void DescriptorBuilder::CrossLinkMessage(Descriptor* message,
                                         const DescriptorProto& proto) {
  for (int i = 0; i < message->nested_type_count(); i++) {
    CrossLinkMessage(&message->nested_types_[i], proto.nested_type(i));
  }

  for (int i = 0; i < message->field_count(); i++) {
    CrossLinkField(&message->fields_[i], proto.field(i));
  }

  for (int i = 0; i < message->extension_count(); i++) {
    CrossLinkField(&message->extensions_[i], proto.extension(i));
  }

  // Set up field array for each oneof.

  // First count the number of fields per oneof.
  for (int i = 0; i < message->field_count(); i++) {
    const OneofDescriptor* oneof_decl = message->field(i)->containing_oneof();
    if (oneof_decl != nullptr) {
      // Make sure fields belonging to the same oneof are defined consecutively.
      // This enables optimizations in codegens and reflection libraries to
      // skip fields in the oneof group, as only one of the field can be set.
      // Note that field_count() returns how many fields in this oneof we have
      // seen so far. field_count() > 0 guarantees that i > 0, so field(i-1) is
      // safe.
      if (oneof_decl->field_count() > 0 &&
          message->field(i - 1)->containing_oneof() != oneof_decl) {
        AddError(
            absl::StrCat(message->full_name(), ".",
                         message->field(i - 1)->name()),
            proto.field(i - 1), DescriptorPool::ErrorCollector::TYPE, [&] {
              return absl::Substitute(
                  "Fields in the same oneof must be defined consecutively. "
                  "\"$0\" cannot be defined before the completion of the "
                  "\"$1\" oneof definition.",
                  message->field(i - 1)->name(), oneof_decl->name());
            });
      }
      // Must go through oneof_decls_ array to get a non-const version of the
      // OneofDescriptor.
      auto& out_oneof_decl = message->oneof_decls_[oneof_decl->index()];
      if (out_oneof_decl.field_count_ == 0) {
        out_oneof_decl.fields_ = message->field(i);
      }

      if (!had_errors_) {
        // Verify that they are contiguous.
        // This is assumed by OneofDescriptor::field(i).
        // But only if there are no errors.
        ABSL_CHECK_EQ(out_oneof_decl.fields_ + out_oneof_decl.field_count_,
                      message->field(i));
      }
      ++out_oneof_decl.field_count_;
    }
  }

  // Then verify the sizes.
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    OneofDescriptor* oneof_decl = &message->oneof_decls_[i];

    if (oneof_decl->field_count() == 0) {
      AddError(absl::StrCat(message->full_name(), ".", oneof_decl->name()),
               proto.oneof_decl(i), DescriptorPool::ErrorCollector::NAME,
               "Oneof must have at least one field.");
    }
  }

  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    if (field->proto3_optional_) {
      if (!field->containing_oneof() ||
          !field->containing_oneof()->is_synthetic()) {
        AddError(message->full_name(), proto.field(i),
                 DescriptorPool::ErrorCollector::OTHER,
                 "Fields with proto3_optional set must be "
                 "a member of a one-field oneof");
      }
    }
  }

  // Synthetic oneofs must be last.
  int first_synthetic = -1;
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    if (message->oneof_decl(i)->is_synthetic()) {
      if (first_synthetic == -1) {
        first_synthetic = i;
      }
    } else {
      if (first_synthetic != -1) {
        AddError(message->full_name(), proto.oneof_decl(i),
                 DescriptorPool::ErrorCollector::OTHER,
                 "Synthetic oneofs must be after all other oneofs");
      }
    }
  }

  if (first_synthetic == -1) {
    message->real_oneof_decl_count_ = message->oneof_decl_count_;
  } else {
    message->real_oneof_decl_count_ = first_synthetic;
  }
}

void DescriptorBuilder::CheckExtensionDeclarationFieldType(
    const FieldDescriptor& field, const FieldDescriptorProto& proto,
    absl::string_view type) {
  if (had_errors_) return;
  std::string actual_type(field.type_name());
  std::string expected_type(type);
  if (field.message_type() || field.enum_type()) {
    // Field message type descriptor can be in a partial state which will cause
    // segmentation fault if it is being accessed.
    if (had_errors_) return;
    absl::string_view full_name = field.message_type() != nullptr
                                      ? field.message_type()->full_name()
                                      : field.enum_type()->full_name();
    actual_type = absl::StrCat(".", full_name);
  }
  if (!IsNonMessageType(type) && !absl::StartsWith(type, ".")) {
    expected_type = absl::StrCat(".", type);
  }
  if (expected_type != actual_type) {
    AddError(field.full_name(), proto, DescriptorPool::ErrorCollector::EXTENDEE,
             [&] {
               return absl::Substitute(
                   "\"$0\" extension field $1 is expected to be type "
                   "\"$2\", not \"$3\".",
                   field.containing_type()->full_name(), field.number(),
                   expected_type, actual_type);
             });
  }
}


void DescriptorBuilder::CheckExtensionDeclaration(
    const FieldDescriptor& field, const FieldDescriptorProto& proto,
    absl::string_view declared_full_name, absl::string_view declared_type_name,
    bool is_repeated) {
  if (!declared_type_name.empty()) {
    CheckExtensionDeclarationFieldType(field, proto, declared_type_name);
  }
  if (!declared_full_name.empty()) {
    std::string actual_full_name = absl::StrCat(".", field.full_name());
    if (declared_full_name != actual_full_name) {
      AddError(field.full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE, [&] {
                 return absl::Substitute(
                     "\"$0\" extension field $1 is expected to have field name "
                     "\"$2\", not \"$3\".",
                     field.containing_type()->full_name(), field.number(),
                     declared_full_name, actual_full_name);
               });
    }
  }

  if (is_repeated != field.is_repeated()) {
    AddError(field.full_name(), proto, DescriptorPool::ErrorCollector::EXTENDEE,
             [&] {
               return absl::Substitute(
                   "\"$0\" extension field $1 is expected to be $2.",
                   field.containing_type()->full_name(), field.number(),
                   is_repeated ? "repeated" : "optional");
             });
  }
}

void DescriptorBuilder::CrossLinkField(FieldDescriptor* field,
                                       const FieldDescriptorProto& proto) {
  if (proto.has_extendee() && field->is_extension()) {
    Symbol extendee =
        LookupSymbol(proto.extendee(), field->full_name(),
                     DescriptorPool::PLACEHOLDER_EXTENDABLE_MESSAGE);
    if (extendee.IsNull()) {
      AddNotDefinedError(field->full_name(), proto,
                         DescriptorPool::ErrorCollector::EXTENDEE,
                         proto.extendee());
      return;
    } else if (extendee.type() != Symbol::MESSAGE) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE, [&] {
                 return absl::StrCat("\"", proto.extendee(),
                                     "\" is not a message type.");
               });
      return;
    } else if (!extendee.IsVisibleFrom(file_)) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE, [&] {
                 return extendee.GetVisibilityError(file_, "target of extend");
               });
      return;
    }

    field->containing_type_ = extendee.descriptor();

    const Descriptor::ExtensionRange* extension_range =
        field->containing_type()->FindExtensionRangeContainingNumber(
            field->number());

    if (extension_range == nullptr) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER, [&] {
                 return absl::Substitute(
                     "\"$0\" does not declare $1 as an "
                     "extension number.",
                     field->containing_type()->full_name(), field->number());
               });
    }
  }

  if (field->containing_oneof() != nullptr) {
    if (field->label_ != FieldDescriptor::LABEL_OPTIONAL) {
      // Note that this error will never happen when parsing .proto files.
      // It can only happen if you manually construct a FileDescriptorProto
      // that is incorrect.
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
               "Fields of oneofs must themselves have label LABEL_OPTIONAL.");
    }
  }

  if (proto.has_type_name()) {
    // Assume we are expecting a message type unless the proto contains some
    // evidence that it expects an enum type.  This only makes a difference if
    // we end up creating a placeholder.
    bool expecting_enum = (proto.type() == FieldDescriptorProto::TYPE_ENUM) ||
                          proto.has_default_value();

    // In case of weak fields we force building the dependency. We need to know
    // if the type exist or not. If it doesn't exist we substitute Empty which
    // should only be done if the type can't be found in the generated pool.
    // TODO Ideally we should query the database directly to check
    // if weak fields exist or not so that we don't need to force building
    // weak dependencies. However the name lookup rules for symbols are
    // somewhat complicated, so I defer it too another CL.
    bool is_weak = !pool_->enforce_weak_ && proto.options().weak();
    bool is_lazy = pool_->lazily_build_dependencies_ && !is_weak;

    Symbol type =
        LookupSymbol(proto.type_name(), field->full_name(),
                     expecting_enum ? DescriptorPool::PLACEHOLDER_ENUM
                                    : DescriptorPool::PLACEHOLDER_MESSAGE,
                     LOOKUP_TYPES, !is_lazy);

    if (type.IsNull()) {
      if (is_lazy) {
        ABSL_CHECK(field->type_ == FieldDescriptor::TYPE_MESSAGE ||
                   field->type_ == FieldDescriptor::TYPE_GROUP ||
                   field->type_ == FieldDescriptor::TYPE_ENUM)
            << proto;
        // Save the symbol names for later for lookup, and allocate the once
        // object needed for the accessors.
        const std::string& name = proto.type_name();

        int name_sizes = static_cast<int>(name.size() + 1 +
                                          proto.default_value().size() + 1);

        field->type_once_ = ::new (tables_->AllocateBytes(
            static_cast<int>(sizeof(absl::once_flag)) + name_sizes))
            absl::once_flag{};
        char* names = reinterpret_cast<char*>(field->type_once_ + 1);

        memcpy(names, name.c_str(), name.size() + 1);
        memcpy(names + name.size() + 1, proto.default_value().c_str(),
               proto.default_value().size() + 1);

        // AddFieldByNumber and AddExtension are done later in this function,
        // and can/must be done if the field type was not found. The related
        // error checking is not necessary when in lazily_build_dependencies_
        // mode, and can't be done without building the type's descriptor,
        // which we don't want to do.
        file_tables_->AddFieldByNumber(field);
        if (field->is_extension()) {
          tables_->AddExtension(field);
        }
        return;
      } else {
        // If the type is a weak type, we change the type to a google.protobuf.Empty
        // field.
        if (is_weak) {
          type = FindSymbol(kNonLinkedWeakMessageReplacementName);
        }
        if (type.IsNull()) {
          AddNotDefinedError(field->full_name(), proto,
                             DescriptorPool::ErrorCollector::TYPE,
                             proto.type_name());
          return;
        }
      }
    }

    // Map entries must be in the same file, so we can populate it directly if
    // the descriptor is already known. If it is not known, then it must not be
    // a map entry.
    if (auto* sub_message = type.descriptor()) {
      field->is_map_ = sub_message->options().map_entry();
    }

    if (!type.IsVisibleFrom(file_)) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               [&] { return type.GetVisibilityError(file_); });
      return;
    }

    if (!proto.has_type()) {
      // Choose field type based on symbol.
      if (type.type() == Symbol::MESSAGE) {
        field->type_ = FieldDescriptor::TYPE_MESSAGE;
      } else if (type.type() == Symbol::ENUM) {
        field->type_ = FieldDescriptor::TYPE_ENUM;
      } else {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE, [&] {
                   return absl::StrCat("\"", proto.type_name(),
                                       "\" is not a type.");
                 });
        return;
      }
    }

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      field->type_descriptor_.message_type = type.descriptor();
      if (field->type_descriptor_.message_type == nullptr) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE, [&] {
                   return absl::StrCat("\"", proto.type_name(),
                                       "\" is not a message type.");
                 });
        return;
      }

      if (field->has_default_value()) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                 "Messages can't have default values.");
      }
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      field->type_descriptor_.enum_type = type.enum_descriptor();
      if (field->type_descriptor_.enum_type == nullptr) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE, [&] {
                   return absl::StrCat("\"", proto.type_name(),
                                       "\" is not an enum type.");
                 });
        return;
      }

      if (field->enum_type()->is_placeholder_) {
        // We can't look up default values for placeholder types.  We'll have
        // to just drop them.
        field->has_default_value_ = false;
      }

      if (field->has_default_value()) {
        // Ensure that the default value is an identifier. Parser cannot always
        // verify this because it does not have complete type information.
        // N.B. that this check yields better error messages but is not
        // necessary for correctness (an enum symbol must be a valid identifier
        // anyway), only for better errors.
        if (!io::Tokenizer::IsIdentifier(proto.default_value())) {
          AddError(field->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                   "Default value for an enum field must be an identifier.");
        } else {
          // We can't just use field->enum_type()->FindValueByName() here
          // because that locks the pool's mutex, which we have already locked
          // at this point.
          const EnumValueDescriptor* default_value =
              LookupSymbolNoPlaceholder(proto.default_value(),
                                        field->enum_type()->full_name())
                  .enum_value_descriptor();

          if (default_value != nullptr &&
              default_value->type() == field->enum_type()) {
            field->default_value_enum_ = default_value;
          } else {
            AddError(field->full_name(), proto,
                     DescriptorPool::ErrorCollector::DEFAULT_VALUE, [&] {
                       return absl::StrCat("Enum type \"",
                                           field->enum_type()->full_name(),
                                           "\" has no value named \"",
                                           proto.default_value(), "\".");
                     });
          }
        }
      } else if (field->enum_type()->value_count() > 0) {
        // All enums must have at least one value, or we would have reported
        // an error elsewhere.  We use the first defined value as the default
        // if a default is not explicitly defined.
        field->default_value_enum_ = field->enum_type()->value(0);
      }
    } else {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "Field with primitive type has type_name.");
    }
  } else {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
        field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "Field with message or enum type missing type_name.");
    }
  }

  // Add the field to the fields-by-number table.
  // Note:  We have to do this *after* cross-linking because extensions do not
  // know their containing type until now. If we're in
  // lazily_build_dependencies_ mode, we're guaranteed there's no errors, so no
  // risk to calling containing_type() or other accessors that will build
  // dependencies.
  if (!file_tables_->AddFieldByNumber(field)) {
    const FieldDescriptor* conflicting_field = file_tables_->FindFieldByNumber(
        field->containing_type(), field->number());
    const absl::string_view containing_type_name =
        field->containing_type() == nullptr
            ? absl::string_view("unknown")
            : field->containing_type()->full_name();
    if (field->is_extension()) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER, [&] {
                 return absl::Substitute(
                     "Extension number $0 has already been used "
                     "in \"$1\" by extension \"$2\".",
                     field->number(), containing_type_name,
                     conflicting_field->full_name());
               });
    } else {
      absl::btree_set<std::pair<int64_t, int64_t>> fields_used;
      auto* parent = field->containing_type();
      for (int i = 0; i < parent->field_count(); ++i) {
        int n = parent->field(i)->number();
        fields_used.insert({n, n});
      }
      for (int i = 0; i < parent->extension_range_count(); ++i) {
        auto* range = parent->extension_range(i);
        fields_used.insert({range->start_number(),
                            static_cast<int64_t>(range->end_number()) - 1});
      }
      for (int i = 0; i < parent->reserved_range_count(); ++i) {
        auto* range = parent->reserved_range(i);
        fields_used.insert(
            {range->start, static_cast<int64_t>(range->end) - 1});
      }
      int64_t proposed_number = 1;
      for (auto [start, end] : fields_used) {
        if (start <= proposed_number && proposed_number <= end) {
          proposed_number = end + 1;
        } else {
          break;
        }
      }

      const std::string proposed_message =
          proposed_number <= FieldDescriptor::kMaxNumber
              ? absl::StrCat("Next available field number is ", proposed_number)
              : "There are no available field numbers";

      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER, [&] {
                 return absl::Substitute(
                     "Field number $0 has already been used in "
                     "\"$1\" by field \"$2\". $3.",
                     field->number(), containing_type_name,
                     conflicting_field->name(), proposed_message);
               });
    }
  } else {
    if (field->is_extension()) {
      if (!tables_->AddExtension(field)) {
        auto make_error = [&] {
          const FieldDescriptor* conflicting_field =
              tables_->FindExtension(field->containing_type(), field->number());
          const absl::string_view containing_type_name =
              field->containing_type() == nullptr
                  ? absl::string_view("unknown")
                  : field->containing_type()->full_name();
          return absl::Substitute(
              "Extension number $0 has already been used in \"$1\" by "
              "extension "
              "\"$2\" defined in $3.",
              field->number(), containing_type_name,
              conflicting_field->full_name(),
              conflicting_field->file()->name());
        };
        // Conflicting extension numbers should be an error. However, before
        // turning this into an error we need to fix all existing broken
        // protos first.
        // TODO: Change this to an error.
        AddWarning(field->full_name(), proto,
                   DescriptorPool::ErrorCollector::NUMBER, make_error);
      }

    }
  }
}

void DescriptorBuilder::CrossLinkService(ServiceDescriptor* service,
                                         const ServiceDescriptorProto& proto) {
  for (int i = 0; i < service->method_count(); i++) {
    CrossLinkMethod(&service->methods_[i], proto.method(i));
  }
}

void DescriptorBuilder::CrossLinkMethod(MethodDescriptor* method,
                                        const MethodDescriptorProto& proto) {
  Symbol input_type =
      LookupSymbol(proto.input_type(), method->full_name(),
                   DescriptorPool::PLACEHOLDER_MESSAGE, LOOKUP_ALL,
                   !pool_->lazily_build_dependencies_);
  if (input_type.IsNull()) {
    if (!pool_->lazily_build_dependencies_) {
      AddNotDefinedError(method->full_name(), proto,
                         DescriptorPool::ErrorCollector::INPUT_TYPE,
                         proto.input_type());
    } else {
      method->input_type_.SetLazy(proto.input_type(), file_);
    }
  } else if (input_type.type() != Symbol::MESSAGE) {
    AddError(method->full_name(), proto,
             DescriptorPool::ErrorCollector::INPUT_TYPE, [&] {
               return absl::StrCat("\"", proto.input_type(),
                                   "\" is not a message type.");
             });
  } else {
    method->input_type_.Set(input_type.descriptor());
  }

  Symbol output_type =
      LookupSymbol(proto.output_type(), method->full_name(),
                   DescriptorPool::PLACEHOLDER_MESSAGE, LOOKUP_ALL,
                   !pool_->lazily_build_dependencies_);
  if (output_type.IsNull()) {
    if (!pool_->lazily_build_dependencies_) {
      AddNotDefinedError(method->full_name(), proto,
                         DescriptorPool::ErrorCollector::OUTPUT_TYPE,
                         proto.output_type());
    } else {
      method->output_type_.SetLazy(proto.output_type(), file_);
    }
  } else if (output_type.type() != Symbol::MESSAGE) {
    AddError(method->full_name(), proto,
             DescriptorPool::ErrorCollector::OUTPUT_TYPE, [&] {
               return absl::StrCat("\"", proto.output_type(),
                                   "\" is not a message type.");
             });
  } else {
    method->output_type_.Set(output_type.descriptor());
  }
}

void DescriptorBuilder::SuggestFieldNumbers(FileDescriptor* file,
                                            const FileDescriptorProto& proto) {
  for (int message_index = 0; message_index < file->message_type_count();
       message_index++) {
    const Descriptor* message = &file->message_types_[message_index];
    auto hints_it = message_hints_.find(message);
    if (hints_it == message_hints_.end()) continue;
    auto* hints = &hints_it->second;
    constexpr int kMaxSuggestions = 3;
    int fields_to_suggest = std::min(kMaxSuggestions, hints->fields_to_suggest);
    if (fields_to_suggest <= 0) continue;
    struct Range {
      int from;
      int to;
    };
    std::vector<Range> used_ordinals;
    auto add_ordinal = [&](int ordinal) {
      if (ordinal <= 0 || ordinal > FieldDescriptor::kMaxNumber) return;
      if (!used_ordinals.empty() && ordinal == used_ordinals.back().to) {
        used_ordinals.back().to = ordinal + 1;
      } else {
        used_ordinals.push_back({ordinal, ordinal + 1});
      }
    };
    auto add_range = [&](int from, int to) {
      from = std::max(0, std::min(FieldDescriptor::kMaxNumber + 1, from));
      to = std::max(0, std::min(FieldDescriptor::kMaxNumber + 1, to));
      if (from >= to) return;
      used_ordinals.push_back({from, to});
    };
    for (int i = 0; i < message->field_count(); i++) {
      add_ordinal(message->field(i)->number());
    }
    for (int i = 0; i < message->extension_count(); i++) {
      add_ordinal(message->extension(i)->number());
    }
    for (int i = 0; i < message->reserved_range_count(); i++) {
      auto range = message->reserved_range(i);
      add_range(range->start, range->end);
    }
    for (int i = 0; i < message->extension_range_count(); i++) {
      auto range = message->extension_range(i);
      add_range(range->start_number(), range->end_number());
    }
    used_ordinals.push_back(
        {FieldDescriptor::kMaxNumber, FieldDescriptor::kMaxNumber + 1});
    used_ordinals.push_back({FieldDescriptor::kFirstReservedNumber,
                             FieldDescriptor::kLastReservedNumber});
    std::sort(used_ordinals.begin(), used_ordinals.end(),
              [](Range lhs, Range rhs) {
                return std::tie(lhs.from, lhs.to) < std::tie(rhs.from, rhs.to);
              });
    int current_ordinal = 1;
    if (hints->first_reason) {
      auto make_error = [&] {
        std::stringstream id_list;
        id_list << "Suggested field numbers for " << message->full_name()
                << ": ";
        const char* separator = "";
        for (auto& current_range : used_ordinals) {
          while (current_ordinal < current_range.from &&
                 fields_to_suggest > 0) {
            id_list << separator << current_ordinal++;
            separator = ", ";
            fields_to_suggest--;
          }
          if (fields_to_suggest == 0) break;
          current_ordinal = std::max(current_ordinal, current_range.to);
        }
        return id_list.str();
      };
      AddError(message->full_name(), *hints->first_reason,
               hints->first_reason_location, make_error);
    }
  }
}

// Populate VisibilityCheckerState for messages.
void DescriptorBuilder::CheckVisibilityRulesVisit(
    const Descriptor& message, const DescriptorProto& proto,
    VisibilityCheckerState& state) {
  if (message.containing_type() != nullptr) {
    state.nested_messages.push_back(DescriptorAndProto{&message, &proto});
  }
}

// Populate VisibilityCheckerState for enums.
void DescriptorBuilder::CheckVisibilityRulesVisit(
    const EnumDescriptor& enm, const EnumDescriptorProto& proto,
    VisibilityCheckerState& state) {
  if (enm.containing_type() != nullptr) {
    if (IsEnumNamespaceMessage(enm)) {
      state.namespaced_enums.push_back(EnumDescriptorAndProto{&enm, &proto});
    } else {
      state.nested_enums.push_back(EnumDescriptorAndProto{&enm, &proto});
    }
  }
}

// Returns true iff the message is a pure zero field message used only for Enum
// namespacing.  AKA it is:
// * top-level
// * visibility local either explicitly or by file default
// * has reserved range of 1 to max.
bool DescriptorBuilder::IsEnumNamespaceMessage(
    const EnumDescriptor& enm) const {
  const Descriptor* container = enm.containing_type();
  const FeatureSet::VisibilityFeature::DefaultSymbolVisibility
      default_visibility = enm.features().default_symbol_visibility();
  // Only allowed for top-level messages
  if (container->containing_type() != nullptr) {
    return false;
  }

  bool default_to_local =
      default_visibility == FeatureSet::VisibilityFeature::STRICT ||
      default_visibility == FeatureSet::VisibilityFeature::LOCAL_ALL;

  bool is_local =
      container->visibility_keyword() == SymbolVisibility::VISIBILITY_LOCAL ||
      (container->visibility_keyword() == SymbolVisibility::VISIBILITY_UNSET &&
       default_to_local);

  // must either be marked local, or unset with file default making it local
  if (!is_local) {
    return false;
  }

  if (container->reserved_range_count() != 1) {
    return false;
  }

  const Descriptor::ReservedRange* range = container->reserved_range(0);
  if (range == nullptr ||
      (range->start != 1 &&
       range->end != FieldDescriptor::kLastReservedNumber)) {
    return false;
  }

  return true;
}

// Enforce File-wide visibility and co-location rules.
void DescriptorBuilder::CheckVisibilityRules(FileDescriptor* file,
                                             const FileDescriptorProto& proto) {
  // Check DefaultSymbolVisibility first.
  //
  // For Edition 2024:
  // If DefaultSymbolVisibility is STRICT enforce it with caveats for:
  //

  VisibilityCheckerState state;

  // Build our state object so we can apply rules based on type.
  internal::VisitDescriptors(
      *file, proto, [&](const auto& descriptor, const auto& proto) {
        CheckVisibilityRulesVisit(descriptor, proto, state);
      });

  // In edition 2024 we only enforce STRICT visibilty rules. There are possibly
  // more rules to come in future editions, but for now just apply the rule for
  // enforcing nested symbol local visibilty. There is a single caveat for,
  // allowing nested enums to have visibility set only when
  //
  // local msg { export enum {} reserved 1 to max; }
  for (auto& nested : state.nested_messages) {
    if (nested.descriptor->visibility_keyword() ==
            SymbolVisibility::VISIBILITY_EXPORT &&
        nested.descriptor->features().default_symbol_visibility() ==
            FeatureSet::VisibilityFeature::STRICT) {
      AddError(
          nested.descriptor->full_name(), *nested.proto,
          DescriptorPool::ErrorCollector::INPUT_TYPE, [&] {
            return absl::StrCat(
                "\"", nested.descriptor->name(),
                "\" is a nested message and cannot be `export` with STRICT "
                "default_symbol_visibility. It must be moved to top-level, "
                "ideally "
                "in its own file in order to be `export`.");
          });
    }
  }

  for (auto& nested : state.nested_enums) {
    if (nested.descriptor->visibility_keyword() ==
            SymbolVisibility::VISIBILITY_EXPORT &&
        nested.descriptor->features().default_symbol_visibility() ==
            FeatureSet::VisibilityFeature::STRICT) {
      // This list contains only enums not considered 'namespaced' by
      // IsEnumNamespaceMessage

      AddError(nested.descriptor->full_name(), *nested.proto,
               DescriptorPool::ErrorCollector::INPUT_TYPE, [&] {
                 return absl::StrCat(
                     "\"", nested.descriptor->name(),
                     "\" is a nested enum and cannot be marked `export` with "
                     "STRICT "
                     "default_symbol_visibility. It must be moved to "
                     "top-level, ideally "
                     "in its own file in order to be `export`. For C++ "
                     "namespacing of enums in a messages use: `local "
                     "message <OuterNamespace> { export enum ",
                     nested.descriptor->name(), " {...} reserved 1 to max; }`");
               });
    }

    // Enforce Future rules here:
  }
}

// -------------------------------------------------------------------

// Determine if the file uses optimize_for = LITE_RUNTIME, being careful to
// avoid problems that exist at init time.
static bool IsLite(const FileDescriptor* file) {
  // TODO:  I don't even remember how many of these conditions are
  //   actually possible.  I'm just being super-safe.
  return file != nullptr &&
         &file->options() != &FileOptions::default_instance() &&
         file->options().optimize_for() == FileOptions::LITE_RUNTIME;
}

void DescriptorBuilder::ValidateOptions(const FileDescriptor* file,
                                        const FileDescriptorProto& proto) {
  ValidateFileFeatures(file, proto);

  // Lite files can only be imported by other Lite files.
  if (!IsLite(file)) {
    for (int i = 0; i < file->dependency_count(); i++) {
      if (IsLite(file->dependency(i))) {
        AddError(file->dependency(i)->name(), proto,
                 DescriptorPool::ErrorCollector::IMPORT, [&] {
                   return absl::StrCat(
                       "Files that do not use optimize_for = LITE_RUNTIME "
                       "cannot import files which do use this option.  This "
                       "file is not lite, but it imports \"",
                       file->dependency(i)->name(), "\" which is.");
                 });
        break;
      }
    }
  }
  if (file->edition() == Edition::EDITION_PROTO3) {
    ValidateProto3(file, proto);
  }

  if (file->edition() < Edition::EDITION_2024 &&
      file->option_dependency_count() > 0) {
    AddError("option", proto, DescriptorPool::ErrorCollector::IMPORT,
             "option imports are not supported before edition 2024.");
  }

  if (file->edition() >= Edition::EDITION_2024) {
    if (file->options().has_java_multiple_files()) {
      AddError(file->name(), proto, DescriptorPool::ErrorCollector::OPTION_NAME,
               "The file option `java_multiple_files` is not supported in "
               "editions 2024 and above, which defaults to the feature value of"
               " `nest_in_file_class = NO` (equivalent to "
               "`java_multiple_files = true`).");
    }
    if (file->weak_dependency_count() > 0) {
      AddError("weak", proto, DescriptorPool::ErrorCollector::IMPORT,
               "weak imports are not allowed under edition 2024 and beyond.");
    }
  }
}

void DescriptorBuilder::ValidateProto3(const FileDescriptor* file,
                                       const FileDescriptorProto& proto) {
  for (int i = 0; i < file->extension_count(); ++i) {
    ValidateProto3Field(file->extensions_ + i, proto.extension(i));
  }
  for (int i = 0; i < file->message_type_count(); ++i) {
    ValidateProto3Message(file->message_types_ + i, proto.message_type(i));
  }
}

void DescriptorBuilder::ValidateProto3Message(const Descriptor* message,
                                              const DescriptorProto& proto) {
  for (int i = 0; i < message->nested_type_count(); ++i) {
    ValidateProto3Message(message->nested_types_ + i, proto.nested_type(i));
  }
  for (int i = 0; i < message->field_count(); ++i) {
    ValidateProto3Field(message->fields_ + i, proto.field(i));
  }
  for (int i = 0; i < message->extension_count(); ++i) {
    ValidateProto3Field(message->extensions_ + i, proto.extension(i));
  }
  if (message->extension_range_count() > 0) {
    AddError(message->full_name(), proto.extension_range(0),
             DescriptorPool::ErrorCollector::NUMBER,
             "Extension ranges are not allowed in proto3.");
  }
  if (message->options().message_set_wire_format()) {
    // Using MessageSet doesn't make sense since we disallow extensions.
    AddError(message->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "MessageSet is not supported in proto3.");
  }
}

void DescriptorBuilder::ValidateProto3Field(const FieldDescriptor* field,
                                            const FieldDescriptorProto& proto) {
  if (field->is_extension() && !IsCustomOptionExtension(field)) {
    AddError(field->full_name(), proto,
             DescriptorPool::ErrorCollector::EXTENDEE,
             "Extensions in proto3 are only allowed for defining options.");
  }
  if (field->is_required()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "Required fields are not allowed in proto3.");
  }
  if (field->has_default_value()) {
    AddError(field->full_name(), proto,
             DescriptorPool::ErrorCollector::DEFAULT_VALUE,
             "Explicit default values are not allowed in proto3.");
  }
  if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM &&
      field->enum_type() && field->enum_type()->is_closed()) {
    // Proto3 messages can only use open enum types; otherwise we can't
    // guarantee that the default value is zero.
    AddError(
        field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE, [&] {
          return absl::StrCat("Enum type \"", field->enum_type()->full_name(),
                              "\" is not an open enum, but is used in \"",
                              field->containing_type()->full_name(),
                              "\" which is a proto3 message type.");
        });
  }
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "Groups are not supported in proto3 syntax.");
  }
}

void DescriptorBuilder::ValidateOptions(const Descriptor* message,
                                        const DescriptorProto& proto) {
  CheckFieldJsonNameUniqueness(proto, message);
  ValidateExtensionRangeOptions(proto, *message);
}

void DescriptorBuilder::ValidateOptions(const OneofDescriptor* /*oneof*/,
                                        const OneofDescriptorProto& /*proto*/) {
}


void DescriptorBuilder::ValidateOptions(const FieldDescriptor* field,
                                        const FieldDescriptorProto& proto) {
  if (pool_->lazily_build_dependencies_ && (!field || !field->message_type())) {
    return;
  }

  ValidateFieldFeatures(field, proto);


  if (field->file()->edition() >= Edition::EDITION_2024 &&
      field->has_legacy_proto_ctype()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "ctype option is not allowed under edition 2024 and beyond. Use "
             "the feature string_type = VIEW|CORD|STRING|... instead.");
  }

  // Only message type fields may be lazy.
  if (field->options().lazy() || field->options().unverified_lazy()) {
    if (field->type() != FieldDescriptor::TYPE_MESSAGE) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "[lazy = true] can only be specified for submessage fields.");
    }
  }

  // Only repeated primitive fields may be packed.
  if (field->options().packed() && !field->is_packable()) {
    AddError(
        field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
        "[packed = true] can only be specified for repeated primitive fields.");
  }

  // Note:  Default instance may not yet be initialized here, so we have to
  //   avoid reading from it.
  if (field->containing_type_ != nullptr &&
      &field->containing_type()->options() !=
          &MessageOptions::default_instance() &&
      field->containing_type()->options().message_set_wire_format()) {
    if (field->is_extension()) {
      if ((field->is_required() || field->is_repeated()) ||
          field->type() != FieldDescriptor::TYPE_MESSAGE) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "Extensions of MessageSets must be optional messages.");
      }
    } else {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
               "MessageSets cannot have fields, only extensions.");
    }
  }

  // Lite extensions can only be of Lite types.
  if (IsLite(field->file()) && field->containing_type_ != nullptr &&
      !IsLite(field->containing_type()->file())) {
    AddError(field->full_name(), proto,
             DescriptorPool::ErrorCollector::EXTENDEE,
             "Extensions to non-lite types can only be declared in non-lite "
             "files.  Note that you cannot extend a non-lite type to contain "
             "a lite type, but the reverse is allowed.");
  }

  // Validate map types.
  if (field->is_map()) {
    if (!ValidateMapEntry(field, proto)) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "map_entry should not be set explicitly. Use map<KeyType, "
               "ValueType> instead.");
    }
  }

  ValidateJSType(field, proto);

  // json_name option is not allowed on extension fields. Note that the
  // json_name field in FieldDescriptorProto is always populated by protoc
  // when it sends descriptor data to plugins (calculated from field name if
  // the option is not explicitly set) so we can't rely on its presence to
  // determine whether the json_name option is set on the field. Here we
  // compare it against the default calculated json_name value and consider
  // the option set if they are different. This won't catch the case when
  // a user explicitly sets json_name to the default value, but should be
  // good enough to catch common misuses.
  if (field->is_extension() &&
      (field->has_json_name() &&
       field->json_name() != ToJsonName(field->name()))) {
    AddError(field->full_name(), proto,
             DescriptorPool::ErrorCollector::OPTION_NAME,
             "option json_name is not allowed on extension fields.");
  }

  if (absl::StrContains(field->json_name(), '\0')) {
    AddError(field->full_name(), proto,
             DescriptorPool::ErrorCollector::OPTION_NAME,
             "json_name cannot have embedded null characters.");
  }


  // If this is a declared extension, validate that the actual name and type
  // match the declaration.
  if (field->is_extension()) {
    if (pool_->IsReadyForCheckingDescriptorExtDecl(
            field->containing_type()->full_name())) {
      return;
    }
    const Descriptor::ExtensionRange* extension_range =
        field->containing_type()->FindExtensionRangeContainingNumber(
            field->number());

    if (extension_range->options_ == nullptr) {
      return;
    }

    if (pool_->EnforceCustomExtensionDeclarations()) {
      for (const auto& declaration : extension_range->options_->declaration()) {
        if (declaration.number() != field->number()) continue;
        if (declaration.reserved()) {
          AddError(
              field->full_name(), proto,
              DescriptorPool::ErrorCollector::EXTENDEE, [&] {
                return absl::Substitute(
                    "Cannot use number $0 for extension field $1, as it is "
                    "reserved in the extension declarations for message $2.",
                    field->number(), field->full_name(),
                    field->containing_type()->full_name());
              });
          return;
        }
        CheckExtensionDeclaration(*field, proto, declaration.full_name(),
                                  declaration.type(), declaration.repeated());
        return;
      }

      // Either no declarations, or there are but no matches. If there are no
      // declarations, we check its verification state. If there are other
      // non-matching declarations, we enforce that this extension must also be
      // declared.
      if (!extension_range->options_->declaration().empty() ||
          (extension_range->options_->verification() ==
           ExtensionRangeOptions::DECLARATION)) {
        AddError(
            field->full_name(), proto, DescriptorPool::ErrorCollector::EXTENDEE,
            [&] {
              return absl::Substitute(
                  "Missing extension declaration for field $0 with number $1 "
                  "in extendee message $2. An extension range must declare for "
                  "all extension fields if its verification state is "
                  "DECLARATION or there's any declaration in the range "
                  "already. Otherwise, consider splitting up the range.",
                  field->full_name(), field->number(),
                  field->containing_type()->full_name());
            });
        return;
      }
    }
  }
}

static bool IsStringMapType(const FieldDescriptor& field) {
  if (!field.is_map()) return false;
  for (int i = 0; i < field.message_type()->field_count(); ++i) {
    if (field.message_type()->field(i)->type() ==
        FieldDescriptor::TYPE_STRING) {
      return true;
    }
  }
  return false;
}

void DescriptorBuilder::ValidateFileFeatures(const FileDescriptor* file,
                                             const FileDescriptorProto& proto) {
  // Rely on our legacy validation for proto2/proto3 files.
  if (IsLegacyEdition(file->edition())) {
    return;
  }

  if (file->features().field_presence() == FeatureSet::LEGACY_REQUIRED) {
    AddError(file->name(), proto, DescriptorPool::ErrorCollector::EDITIONS,
             "Required presence can't be specified by default.");
  }
  if (file->options().java_string_check_utf8()) {
    AddError(
        file->name(), proto, DescriptorPool::ErrorCollector::EDITIONS,
        "File option java_string_check_utf8 is not allowed under editions. Use "
        "the (pb.java).utf8_validation feature to control this behavior.");
  }
}

void DescriptorBuilder::ValidateFieldFeatures(
    const FieldDescriptor* field, const FieldDescriptorProto& proto) {
  // Rely on our legacy validation for proto2/proto3 files.
  if (field->file()->edition() < Edition::EDITION_2023) {
    return;
  }

  // Double check proto descriptors in editions.  These would usually be caught
  // by the parser, but may not be for dynamically built descriptors.
  if (proto.label() == FieldDescriptorProto::LABEL_REQUIRED) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Required label is not allowed under editions.  Use the feature "
             "field_presence = LEGACY_REQUIRED to control this behavior.");
  }
  if (proto.type() == FieldDescriptorProto::TYPE_GROUP) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Group types are not allowed under editions.  Use the feature "
             "message_encoding = DELIMITED to control this behavior.");
  }

  auto& field_options = field->options();
  // Validate legacy options that have been migrated to features.
  if (field_options.has_packed()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Field option packed is not allowed under editions.  Use the "
             "repeated_field_encoding feature to control this behavior.");
  }

  // Validate fully resolved features.
  if (!field->is_repeated() && !field->has_presence()) {
    if (field->has_default_value()) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
               "Implicit presence fields can't specify defaults.");
    }
    if (field->enum_type() != nullptr && field->enum_type()->is_closed()) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
               "Implicit presence enum fields must always be open.");
    }
  }
  if (field->is_extension() && field->is_required()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Extensions can't be required.");
  }

  if (field->containing_type() != nullptr &&
      field->containing_type()->options().map_entry()) {
    // Skip validation of explicit features on generated map fields.  These will
    // be blindly propagated from the original map field, and may violate a lot
    // of these conditions.  Note: we do still validate the user-specified map
    // field.
    return;
  }

  // Validate explicitly specified features on the field proto.
  if (field->proto_features_->has_field_presence()) {
    if (field->containing_oneof() != nullptr) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
               "Oneof fields can't specify field presence.");
    } else if (field->is_repeated()) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
               "Repeated fields can't specify field presence.");
    } else if (field->is_extension() &&
               field->proto_features_->field_presence() !=
                   FeatureSet::LEGACY_REQUIRED) {
      // Note: required extensions will fail elsewhere, so we skip reporting a
      // second error here.
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
               "Extensions can't specify field presence.");
    } else if (field->message_type() != nullptr &&
               field->proto_features_->field_presence() ==
                   FeatureSet::IMPLICIT) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
               "Message fields can't specify implicit presence.");
    }
  }
  if (!field->is_repeated() &&
      field->proto_features_->has_repeated_field_encoding()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Only repeated fields can specify repeated field encoding.");
  }
  if (field->type() != FieldDescriptor::TYPE_STRING &&
      !IsStringMapType(*field) &&
      field->proto_features_->has_utf8_validation()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Only string fields can specify utf8 validation.");
  }
  if (!field->is_packable() &&
      field->proto_features_->repeated_field_encoding() == FeatureSet::PACKED) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Only repeated primitive fields can specify PACKED repeated field "
             "encoding.");
  }
  if ((field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE ||
       field->is_map_message_type()) &&
      field->proto_features_->has_message_encoding()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Only message fields can specify message encoding.");
  }
}

void DescriptorBuilder::ValidateOptions(const EnumDescriptor* enm,
                                        const EnumDescriptorProto& proto) {
  CheckEnumValueUniqueness(proto, enm);

  if (!enm->is_closed() && enm->value_count() > 0 &&
      enm->value(0)->number() != 0) {
    AddError(enm->full_name(), proto.value(0),
             DescriptorPool::ErrorCollector::NUMBER,
             "The first enum value must be zero for open enums.");
  }

  if (!enm->options().has_allow_alias() || !enm->options().allow_alias()) {
    absl::flat_hash_map<int, std::string> used_values;
    for (int i = 0; i < enm->value_count(); ++i) {
      const EnumValueDescriptor* enum_value = enm->value(i);
      auto insert_result =
          used_values.emplace(enum_value->number(), enum_value->full_name());
      bool inserted = insert_result.second;
      if (!inserted) {
        if (!enm->options().allow_alias()) {
          // Generate error if duplicated enum values are explicitly disallowed.
          auto make_error = [&] {
            // Find the next free number.
            absl::flat_hash_set<int64_t> used;
            for (int j = 0; j < enm->value_count(); ++j) {
              used.insert(enm->value(j)->number());
            }
            int64_t next_value = static_cast<int64_t>(enum_value->number()) + 1;
            while (used.contains(next_value)) ++next_value;

            std::string error = absl::StrCat(
                "\"", enum_value->full_name(),
                "\" uses the same enum value as \"",
                insert_result.first->second,
                "\". If this is intended, set "
                "'option allow_alias = true;' to the enum definition.");
            if (next_value < std::numeric_limits<int32_t>::max()) {
              absl::StrAppend(&error, " The next available enum value is ",
                              next_value, ".");
            }
            return error;
          };
          AddError(enm->full_name(), proto.value(i),
                   DescriptorPool::ErrorCollector::NUMBER, make_error);
        }
      }
    }
  }
}

void DescriptorBuilder::ValidateOptions(
    const EnumValueDescriptor* /* enum_value */,
    const EnumValueDescriptorProto& /* proto */) {
  // Nothing to do so far.
}

namespace {
// Validates that a fully-qualified symbol for extension declaration must
// have a leading dot and valid identifiers.
std::optional<std::string> ValidateSymbolForDeclaration(
    absl::string_view symbol) {
  if (!absl::StartsWith(symbol, ".")) {
    return absl::StrCat("\"", symbol,
                        "\" must have a leading dot to indicate the "
                        "fully-qualified scope.");
  }
  if (!ValidateQualifiedName(symbol)) {
    return absl::StrCat("\"", symbol, "\" contains invalid identifiers.");
  }
  return std::nullopt;
}
}  // namespace


void DescriptorBuilder::ValidateExtensionDeclaration(
    const absl::string_view full_name,
    const RepeatedPtrField<ExtensionRangeOptions_Declaration>& declarations,
    const DescriptorProto_ExtensionRange& proto,
    absl::flat_hash_set<absl::string_view>& full_name_set) {
  absl::flat_hash_set<int> extension_number_set;
  for (const auto& declaration : declarations) {
    if (declaration.number() < proto.start() ||
        declaration.number() >= proto.end()) {
      AddError(full_name, proto, DescriptorPool::ErrorCollector::NUMBER, [&] {
        return absl::Substitute(
            "Extension declaration number $0 is not in the "
            "extension range.",
            declaration.number());
      });
    }

    if (!extension_number_set.insert(declaration.number()).second) {
      AddError(full_name, proto, DescriptorPool::ErrorCollector::NUMBER, [&] {
        return absl::Substitute(
            "Extension declaration number $0 is declared multiple times.",
            declaration.number());
      });
    }

    // Both full_name and type should be present. If none of them is set,
    // add an error unless reserved is set to true. If only one of them is set,
    // add an error whether or not reserved is set to true.
    if (!declaration.has_full_name() || !declaration.has_type()) {
      if (declaration.has_full_name() != declaration.has_type() ||
          !declaration.reserved()) {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::EXTENDEE,
                 [&] {
                   return absl::StrCat(
                       "Extension declaration #", declaration.number(),
                       " should have both \"full_name\" and \"type\" set.");
                 });
      }
    } else {
      if (!full_name_set.insert(declaration.full_name()).second) {
        AddError(
            declaration.full_name(), proto,
            DescriptorPool::ErrorCollector::NAME, [&] {
              return absl::Substitute(
                  "Extension field name \"$0\" is declared multiple times.",
                  declaration.full_name());
            });
        return;
      }
      std::optional<std::string> err =
          ValidateSymbolForDeclaration(declaration.full_name());
      if (err.has_value()) {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                 [err] { return *err; });
      }
      if (!IsNonMessageType(declaration.type())) {
        err = ValidateSymbolForDeclaration(declaration.type());
        if (err.has_value()) {
          AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                   [err] { return *err; });
        }
      }
    }
  }
}

void DescriptorBuilder::ValidateExtensionRangeOptions(
    const DescriptorProto& proto, const Descriptor& message) {
  const int64_t max_extension_range =
      static_cast<int64_t>(message.options().message_set_wire_format()
                               ? std::numeric_limits<int32_t>::max()
                               : FieldDescriptor::kMaxNumber);

  size_t num_declarations = 0;
  for (int i = 0; i < message.extension_range_count(); i++) {
    if (message.extension_range(i)->options_ == nullptr) continue;
    num_declarations +=
        message.extension_range(i)->options_->declaration_size();
  }

  // Contains the full names from both "declaration" and "metadata".
  absl::flat_hash_set<absl::string_view> declaration_full_name_set;
  declaration_full_name_set.reserve(num_declarations);

  for (int i = 0; i < message.extension_range_count(); i++) {
    const auto& range = *message.extension_range(i);
    if (range.end_number() > max_extension_range + 1) {
      AddError(message.full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER, [&] {
                 return absl::Substitute(
                     "Extension numbers cannot be greater than $0.",
                     max_extension_range);
               });
    }
    const auto& range_options = *range.options_;


    if (!range_options.declaration().empty()) {
      // TODO: remove the "has_verification" check once the default
      // is flipped to DECLARATION.
      if (range_options.has_verification() &&
          range_options.verification() == ExtensionRangeOptions::UNVERIFIED) {
        AddError(message.full_name(), proto.extension_range(i),
                 DescriptorPool::ErrorCollector::EXTENDEE, [&] {
                   return "Cannot mark the extension range as UNVERIFIED when "
                          "it has extension(s) declared.";
                 });
        return;
      }
      ValidateExtensionDeclaration(
          message.full_name(), range_options.declaration(),
          proto.extension_range(i), declaration_full_name_set);
    }
  }
}

void DescriptorBuilder::ValidateOptions(const ServiceDescriptor* service,
                                        const ServiceDescriptorProto& proto) {
  if (IsLite(service->file()) &&
      (service->file()->options().cc_generic_services() ||
       service->file()->options().java_generic_services())) {
    AddError(service->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
             "Files with optimize_for = LITE_RUNTIME cannot define services "
             "unless you set both options cc_generic_services and "
             "java_generic_services to false.");
  }
}

void DescriptorBuilder::ValidateOptions(
    const MethodDescriptor* /* method */,
    const MethodDescriptorProto& /* proto */) {
  // Nothing to do so far.
}

bool DescriptorBuilder::ValidateMapEntry(const FieldDescriptor* field,
                                         const FieldDescriptorProto& proto) {
  const Descriptor* message = field->message_type();
  if (  // Must not contain extensions, extension range or nested message or
        // enums
      message->extension_count() != 0 ||
      field->label_ != FieldDescriptor::LABEL_REPEATED ||
      message->extension_range_count() != 0 ||
      message->nested_type_count() != 0 || message->enum_type_count() != 0 ||
      // Must contain exactly two fields
      message->field_count() != 2 ||
      // Field name and message name must match
      message->name() !=
          absl::StrCat(ToCamelCase(field->name(), false), "Entry") ||
      // Entry message must be in the same containing type of the field.
      field->containing_type() != message->containing_type()) {
    return false;
  }

  const FieldDescriptor* key = message->map_key();
  const FieldDescriptor* value = message->map_value();
  if (key->label_ != FieldDescriptor::LABEL_OPTIONAL || key->number() != 1 ||
      key->name() != "key") {
    return false;
  }
  if (value->label_ != FieldDescriptor::LABEL_OPTIONAL ||
      value->number() != 2 || value->name() != "value") {
    return false;
  }

  // Check key types are legal.
  switch (key->type()) {
    case FieldDescriptor::TYPE_ENUM:
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "Key in map fields cannot be enum types.");
      break;
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_BYTES:
      AddError(
          field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
          "Key in map fields cannot be float/double, bytes or message types.");
      break;
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
      // Legal cases
      break;
      // Do not add a default, so that the compiler will complain when new types
      // are added.
  }

  if (value->type() == FieldDescriptor::TYPE_ENUM) {
    if (value->enum_type()->value(0)->number() != 0) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "Enum value in map must define 0 as the first value.");
    }
  }

  return true;
}

void DescriptorBuilder::DetectMapConflicts(const Descriptor* message,
                                           const DescriptorProto& proto) {
  DescriptorsByNameSet<Descriptor> seen_types;
  for (int i = 0; i < message->nested_type_count(); ++i) {
    const Descriptor* nested = message->nested_type(i);
    auto insert_result = seen_types.insert(nested);
    bool inserted = insert_result.second;
    if (!inserted) {
      if ((*insert_result.first)->options().map_entry() ||
          nested->options().map_entry()) {
        AddError(message->full_name(), proto,
                 DescriptorPool::ErrorCollector::NAME, [&] {
                   return absl::StrCat(
                       "Expanded map entry type ", nested->name(),
                       " conflicts with an existing nested message type.");
                 });
        break;
      }
    }
    // Recursively test on the nested types.
    DetectMapConflicts(message->nested_type(i), proto.nested_type(i));
  }
  // Check for conflicted field names.
  for (int i = 0; i < message->field_count(); ++i) {
    const FieldDescriptor* field = message->field(i);
    auto iter = seen_types.find(field->name());
    if (iter != seen_types.end() && (*iter)->options().map_entry()) {
      AddError(message->full_name(), proto,
               DescriptorPool::ErrorCollector::NAME, [&] {
                 return absl::StrCat("Expanded map entry type ",
                                     (*iter)->name(),
                                     " conflicts with an existing field.");
               });
    }
  }
  // Check for conflicted enum names.
  for (int i = 0; i < message->enum_type_count(); ++i) {
    const EnumDescriptor* enum_desc = message->enum_type(i);
    auto iter = seen_types.find(enum_desc->name());
    if (iter != seen_types.end() && (*iter)->options().map_entry()) {
      AddError(message->full_name(), proto,
               DescriptorPool::ErrorCollector::NAME, [&] {
                 return absl::StrCat("Expanded map entry type ",
                                     (*iter)->name(),
                                     " conflicts with an existing enum type.");
               });
    }
  }
  // Check for conflicted oneof names.
  for (int i = 0; i < message->oneof_decl_count(); ++i) {
    const OneofDescriptor* oneof_desc = message->oneof_decl(i);
    auto iter = seen_types.find(oneof_desc->name());
    if (iter != seen_types.end() && (*iter)->options().map_entry()) {
      AddError(message->full_name(), proto,
               DescriptorPool::ErrorCollector::NAME, [&] {
                 return absl::StrCat("Expanded map entry type ",
                                     (*iter)->name(),
                                     " conflicts with an existing oneof type.");
               });
    }
  }
}

void DescriptorBuilder::ValidateJSType(const FieldDescriptor* field,
                                       const FieldDescriptorProto& proto) {
  FieldOptions::JSType jstype = field->options().jstype();
  // The default is always acceptable.
  if (jstype == FieldOptions::JS_NORMAL) {
    return;
  }

  switch (field->type()) {
    // Integral 64-bit types may be represented as JavaScript numbers or
    // strings.
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      if (jstype == FieldOptions::JS_STRING ||
          jstype == FieldOptions::JS_NUMBER) {
        return;
      }
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               [&] {
                 return absl::StrCat(
                     "Illegal jstype for int64, uint64, sint64, fixed64 "
                     "or sfixed64 field: ",
                     FieldOptions_JSType_descriptor()->value(jstype)->name());
               });
      break;

    // No other types permit a jstype option.
    default:
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "jstype is only allowed on int64, uint64, sint64, fixed64 "
               "or sfixed64 fields.");
      break;
  }
}

namespace {

// Whether the name contains underscores that violate the naming style guide (
// a leading or trailing underscore, or an underscore which is not followed by
// a letter)
bool ContainsBadUnderscores(absl::string_view name) {
  if (name.empty()) {
    return false;
  }
  if (name[0] == '_' || name[name.size() - 1] == '_') {
    return true;
  }
  for (size_t i = 1; i < name.size(); ++i) {
    if (name[i - 1] == '_' && !absl::ascii_isalpha(name[i])) {
      return true;
    }
  }
  return false;
}

bool IsValidTitleCaseName(absl::string_view name, std::string* error) {
  ABSL_CHECK(!name.empty());
  for (char c : name) {
    if (!absl::ascii_isalnum(c)) {
      *error = "should be TitleCase";
      return false;
    }
  }
  if (!absl::ascii_isupper(name[0])) {
    *error = "should begin with a capital letter";
    return false;
  }
  return true;
}

bool IsValidLowerSnakeCaseName(absl::string_view name, std::string* error) {
  ABSL_CHECK(!name.empty());

  constexpr absl::CharSet kLowerSnakeCaseChars =
      absl::CharSet::Range('a', 'z') | absl::CharSet::Range('0', '9') |
      absl::CharSet::Char('_') | absl::CharSet::Char('.');
  for (char c : name) {
    if (!kLowerSnakeCaseChars.contains(c)) {
      *error = "should be lower_snake_case";
      return false;
    }
  }
  if (!absl::ascii_islower(name[0])) {
    *error = "should begin with a lower case letter";
    return false;
  }
  if (ContainsBadUnderscores(name)) {
    *error = "contains style violating underscores";
    return false;
  }
  return true;
}

bool IsValidUpperSnakeCaseName(absl::string_view name, std::string* error) {
  ABSL_CHECK(!name.empty());

  constexpr absl::CharSet kUpperSnakeCaseChars =
      absl::CharSet::Range('A', 'Z') | absl::CharSet::Range('0', '9') |
      absl::CharSet::Char('_');
  for (char c : name) {
    if (!kUpperSnakeCaseChars.contains(c)) {
      *error = "should be UPPER_SNAKE_CASE";
      return false;
    }
  }
  if (!absl::ascii_isupper(name[0])) {
    *error = "should begin with an upper case letter";
    return false;
  }
  if (ContainsBadUnderscores(name)) {
    *error = "contains style violating underscores";
    return false;
  }
  return true;
}

constexpr absl::string_view kNamingStyleOptOutMessage =
    " (feature.enforce_naming_style = STYLE_LEGACY can be used to opt out of "
    "this check)";

}  // namespace

template <>
void DescriptorBuilder::ValidateNamingStyle(const FileDescriptor* file,
                                            const FileDescriptorProto& proto) {
  // Ignore empty packages for style checks.
  if (file->package().empty()) {
    return;
  }
  std::string error;
  if (!IsValidLowerSnakeCaseName(file->package(), &error)) {
    AddError(file->name(), proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("Package name ", file->package(), " ", error,
                          kNamingStyleOptOutMessage);
    });
  }
}

template <>
void DescriptorBuilder::ValidateNamingStyle(const Descriptor* message,
                                            const DescriptorProto& proto) {
  std::string error;
  if (!IsValidTitleCaseName(message->name(), &error)) {
    AddError(message->name(), proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("Message name ", message->name(), " ", error,
                          kNamingStyleOptOutMessage);
    });
  }
}

template <>
void DescriptorBuilder::ValidateNamingStyle(const OneofDescriptor* oneof,
                                            const OneofDescriptorProto& proto) {
  std::string error;
  if (!IsValidLowerSnakeCaseName(oneof->name(), &error)) {
    AddError(oneof->name(), proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("Oneof name ", oneof->name(), " ", error,
                          kNamingStyleOptOutMessage);
    });
  }
}

template <>
void DescriptorBuilder::ValidateNamingStyle(const FieldDescriptor* field,
                                            const FieldDescriptorProto& proto) {
  std::string error;
  if (!IsValidLowerSnakeCaseName(field->name(), &error)) {
    AddError(field->name(), proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("Field name ", field->name(), " ", error,
                          kNamingStyleOptOutMessage);
    });
  }
}

template <>
void DescriptorBuilder::ValidateNamingStyle(
    const EnumDescriptor* enum_descriptor, const EnumDescriptorProto& proto) {
  std::string error;
  if (!IsValidTitleCaseName(enum_descriptor->name(), &error)) {
    AddError(enum_descriptor->name(), proto,
             DescriptorPool::ErrorCollector::NAME, [&] {
               return absl::StrCat("Enum name ", enum_descriptor->name(), " ",
                                   error, kNamingStyleOptOutMessage);
             });
  }
}

template <>
void DescriptorBuilder::ValidateNamingStyle(
    const EnumValueDescriptor* enum_value,
    const EnumValueDescriptorProto& proto) {
  std::string error;
  if (!IsValidUpperSnakeCaseName(enum_value->name(), &error)) {
    AddError(enum_value->name(), proto, DescriptorPool::ErrorCollector::NAME,
             [&] {
               return absl::StrCat("Enum value name ", enum_value->name(), " ",
                                   error, kNamingStyleOptOutMessage);
             });
  }
}

template <>
void DescriptorBuilder::ValidateNamingStyle(
    const ServiceDescriptor* service, const ServiceDescriptorProto& proto) {
  std::string error;
  if (!IsValidTitleCaseName(service->name(), &error)) {
    AddError(service->name(), proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("Service name ", service->name(), " ", error,
                          kNamingStyleOptOutMessage);
    });
  }
}

template <>
void DescriptorBuilder::ValidateNamingStyle(
    const MethodDescriptor* method, const MethodDescriptorProto& proto) {
  std::string error;
  if (!IsValidTitleCaseName(method->name(), &error)) {
    AddError(method->name(), proto, DescriptorPool::ErrorCollector::NAME, [&] {
      return absl::StrCat("Method name ", method->name(), " ", error,
                          kNamingStyleOptOutMessage);
    });
  }
}

// -------------------------------------------------------------------

DescriptorBuilder::OptionInterpreter::OptionInterpreter(
    DescriptorBuilder* builder)
    : builder_(builder) {
  ABSL_CHECK(builder_);
}

DescriptorBuilder::OptionInterpreter::~OptionInterpreter() = default;

bool DescriptorBuilder::OptionInterpreter::InterpretOptionExtensions(
    OptionsToInterpret* options_to_interpret) {
  return InterpretOptionsImpl(options_to_interpret, /*skip_extensions=*/false);
}
bool DescriptorBuilder::OptionInterpreter::InterpretNonExtensionOptions(
    OptionsToInterpret* options_to_interpret) {
  return InterpretOptionsImpl(options_to_interpret, /*skip_extensions=*/true);
}
bool DescriptorBuilder::OptionInterpreter::InterpretOptionsImpl(
    OptionsToInterpret* options_to_interpret, bool skip_extensions) {
  // Note that these may be in different pools, so we can't use the same
  // descriptor and reflection objects on both.
  Message* options = options_to_interpret->options;
  const Message* original_options = options_to_interpret->original_options;

  bool failed = false;
  options_to_interpret_ = options_to_interpret;

  // Find the uninterpreted_option field in the mutable copy of the options
  // and clear them, since we're about to interpret them.
  const FieldDescriptor* uninterpreted_options_field =
      options->GetDescriptor()->FindFieldByName("uninterpreted_option");
  ABSL_CHECK(uninterpreted_options_field != nullptr)
      << "No field named \"uninterpreted_option\" in the Options proto.";
  options->GetReflection()->ClearField(options, uninterpreted_options_field);

  std::vector<int> src_path = options_to_interpret->element_path;
  src_path.push_back(uninterpreted_options_field->number());

  // Find the uninterpreted_option field in the original options.
  const FieldDescriptor* original_uninterpreted_options_field =
      original_options->GetDescriptor()->FindFieldByName(
          "uninterpreted_option");
  ABSL_CHECK(original_uninterpreted_options_field != nullptr)
      << "No field named \"uninterpreted_option\" in the Options proto.";

  const int num_uninterpreted_options =
      original_options->GetReflection()->FieldSize(
          *original_options, original_uninterpreted_options_field);
  for (int i = 0; i < num_uninterpreted_options; ++i) {
    src_path.push_back(i);
    uninterpreted_option_ = DownCastMessage<UninterpretedOption>(
        &original_options->GetReflection()->GetRepeatedMessage(
            *original_options, original_uninterpreted_options_field, i));
    if (!InterpretSingleOption(options, src_path,
                               options_to_interpret->element_path,
                               skip_extensions)) {
      // Error already added by InterpretSingleOption().
      failed = true;
      break;
    }
    src_path.pop_back();
  }
  // Reset these, so we don't have any dangling pointers.
  uninterpreted_option_ = nullptr;
  options_to_interpret_ = nullptr;

  if (!failed) {
    // InterpretSingleOption() added the interpreted options in the
    // UnknownFieldSet, in case the option isn't yet known to us.  Now we
    // serialize the options message and deserialize it back.  That way, any
    // option fields that we do happen to know about will get moved from the
    // UnknownFieldSet into the real fields, and thus be available right away.
    // If they are not known, that's OK too. They will get reparsed into the
    // UnknownFieldSet and wait there until the message is parsed by something
    // that does know about the options.

    // Keep the unparsed options around in case the reparsing fails.
    std::unique_ptr<Message> unparsed_options(options->New());
    options->GetReflection()->Swap(unparsed_options.get(), options);

    std::string buf;
    if (!unparsed_options->AppendToString(&buf) ||
        !options->ParseFromString(buf)) {
      builder_->AddError(
          options_to_interpret->element_name, *original_options,
          DescriptorPool::ErrorCollector::OTHER, [&] {
            return absl::StrCat(
                "Some options could not be correctly parsed using the proto "
                "descriptors compiled into this binary.\n"
                "Unparsed options: ",
                unparsed_options->ShortDebugString(),
                "\n"
                "Parsing attempt:  ",
                options->ShortDebugString());
          });
      // Restore the unparsed options.
      options->GetReflection()->Swap(unparsed_options.get(), options);
    }
  }

  return !failed;
}

bool DescriptorBuilder::OptionInterpreter::InterpretSingleOption(
    Message* options, const std::vector<int>& src_path,
    const std::vector<int>& options_path, bool skip_extensions) {
  // First do some basic validation.
  if (uninterpreted_option_->name_size() == 0) {
    // This should never happen unless the parser has gone seriously awry or
    // someone has manually created the uninterpreted option badly.
    if (skip_extensions) {
      // Come back to it later.
      return true;
    }
    return AddNameError(
        []() -> std::string { return "Option must have a name."; });
  }
  if (uninterpreted_option_->name(0).name_part() == "uninterpreted_option") {
    if (skip_extensions) {
      // Come back to it later.
      return true;
    }
    return AddNameError([]() -> std::string {
      return "Option must not use reserved name \"uninterpreted_option\".";
    });
  }

  if (skip_extensions == uninterpreted_option_->name(0).is_extension()) {
    // Allow feature and option interpretation to occur in two phases.  This is
    // necessary because features *are* options and need to be interpreted
    // before resolving them.  However, options can also *have* features
    // attached to them.
    return true;
  }

  const Descriptor* options_descriptor = nullptr;
  // Get the options message's descriptor from the builder's pool, so that we
  // get the version that knows about any extension options declared in the file
  // we're currently building. The descriptor should be there as long as the
  // file we're building imported descriptor.proto.

  // Note that we use DescriptorBuilder::FindSymbolNotEnforcingDeps(), not
  // DescriptorPool::FindMessageTypeByName() because we're already holding the
  // pool's mutex, and the latter method locks it again.  We don't use
  // FindSymbol() because files that use custom options only need to depend on
  // the file that defines the option, not descriptor.proto itself.
  Symbol symbol = builder_->FindSymbolNotEnforcingDeps(
      options->GetDescriptor()->full_name());
  options_descriptor = symbol.descriptor();
  if (options_descriptor == nullptr) {
    // The options message's descriptor was not in the builder's pool, so use
    // the standard version from the generated pool. We're not holding the
    // generated pool's mutex, so we can search it the straightforward way.
    options_descriptor = options->GetDescriptor();
  }
  ABSL_CHECK(options_descriptor);

  // We iterate over the name parts to drill into the submessages until we find
  // the leaf field for the option. As we drill down we remember the current
  // submessage's descriptor in |descriptor| and the next field in that
  // submessage in |field|. We also track the fields we're drilling down
  // through in |intermediate_fields|. As we go, we reconstruct the full option
  // name in |debug_msg_name|, for use in error messages.
  const Descriptor* descriptor = options_descriptor;
  const FieldDescriptor* field = nullptr;
  std::vector<const FieldDescriptor*> intermediate_fields;
  std::string debug_msg_name = "";

  std::vector<int> dest_path = options_path;

  for (int i = 0; i < uninterpreted_option_->name_size(); ++i) {
    builder_->undefine_resolved_name_.clear();
    const std::string& name_part = uninterpreted_option_->name(i).name_part();
    if (!debug_msg_name.empty()) {
      absl::StrAppend(&debug_msg_name, ".");
    }
    if (uninterpreted_option_->name(i).is_extension()) {
      absl::StrAppend(&debug_msg_name, "(", name_part, ")");
      // Search for the extension's descriptor as an extension in the builder's
      // pool. Note that we use DescriptorBuilder::LookupSymbol(), not
      // DescriptorPool::FindExtensionByName(), for two reasons: 1) It allows
      // relative lookups, and 2) because we're already holding the pool's
      // mutex, and the latter method locks it again.
      symbol =
          builder_->LookupSymbol(name_part, options_to_interpret_->name_scope);
      field = symbol.field_descriptor();
      // If we don't find the field then the field's descriptor was not in the
      // builder's pool, but there's no point in looking in the generated
      // pool. We require that you import the file that defines any extensions
      // you use, so they must be present in the builder's pool.
    } else {
      absl::StrAppend(&debug_msg_name, name_part);
      // Search for the field's descriptor as a regular field.
      field = descriptor->FindFieldByName(name_part);
    }

    if (field == nullptr) {
      if (get_allow_unknown(builder_->pool_)) {
        // We can't find the option, but AllowUnknownDependencies() is enabled,
        // so we will just leave it as uninterpreted.
        AddWithoutInterpreting(*uninterpreted_option_, options);
        return true;
      } else if (!(builder_->undefine_resolved_name_).empty()) {
        // Option is resolved to a name which is not defined.
        return AddNameError([&] {
          return absl::StrCat(
              "Option \"", debug_msg_name, "\" is resolved to \"(",
              builder_->undefine_resolved_name_,
              ")\", which is not defined. The innermost scope is searched "
              "first "
              "in name resolution. Consider using a leading '.'(i.e., \"(.",
              debug_msg_name.substr(1),
              "\") to start from the outermost scope.");
        });
      } else {
        return AddNameError([&] {
          return absl::StrCat("Option \"", debug_msg_name,
                              "\" unknown. Ensure that your proto",
                              " definition file imports the proto which "
                              "defines the option (i.e. via import option "
                              "after edition 2024).");
        });
      }
    } else if (field->containing_type() != descriptor) {
      if (get_is_placeholder(field->containing_type())) {
        // The field is an extension of a placeholder type, so we can't
        // reliably verify whether it is a valid extension to use here (e.g.
        // we don't know if it is an extension of the correct *Options message,
        // or if it has a valid field number, etc.).  Just leave it as
        // uninterpreted instead.
        AddWithoutInterpreting(*uninterpreted_option_, options);
        return true;
      } else {
        // This can only happen if, due to some insane misconfiguration of the
        // pools, we find the options message in one pool but the field in
        // another. This would probably imply a hefty bug somewhere.
        return AddNameError([&] {
          return absl::StrCat("Option field \"", debug_msg_name,
                              "\" is not a field or extension of message \"",
                              descriptor->name(), "\".");
        });
      }
    } else {
      // accumulate field numbers to form path to interpreted option
      dest_path.push_back(field->number());

      // Special handling to prevent feature use in the same file as the
      // definition.
      // TODO Add proper support for cases where this can work.
      if (field->file() == builder_->file_ &&
          uninterpreted_option_->name(0).name_part() == "features" &&
          !uninterpreted_option_->name(0).is_extension()) {
        return AddNameError([&] {
          return absl::StrCat(
              "Feature \"", debug_msg_name,
              "\" can't be used in the same file it's defined in.");
        });
      }

      if (i < uninterpreted_option_->name_size() - 1) {
        if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
          return AddNameError([&] {
            return absl::StrCat("Option \"", debug_msg_name,
                                "\" is an atomic type, not a message.");
          });
        } else if (field->is_repeated()) {
          return AddNameError([&] {
            return absl::StrCat("Option field \"", debug_msg_name,
                                "\" is a repeated message. Repeated message "
                                "options must be initialized using an "
                                "aggregate value.");
          });
        } else {
          // Drill down into the submessage.
          intermediate_fields.push_back(field);
          descriptor = field->message_type();
        }
      }
    }
  }

  // We've found the leaf field. Now we use UnknownFieldSets to set its value
  // on the options message. We do so because the message may not yet know
  // about its extension fields, so we may not be able to set the fields
  // directly. But the UnknownFieldSets will serialize to the same wire-format
  // message, so reading that message back in once the extension fields are
  // known will populate them correctly.

  // First see if the option is already set.
  if (!field->is_repeated() &&
      !ExamineIfOptionIsSet(
          intermediate_fields.begin(), intermediate_fields.end(), field,
          debug_msg_name,
          options->GetReflection()->GetUnknownFields(*options))) {
    return false;  // ExamineIfOptionIsSet() already added the error.
  }

  // First set the value on the UnknownFieldSet corresponding to the
  // innermost message.
  std::unique_ptr<UnknownFieldSet> unknown_fields(new UnknownFieldSet());
  if (!SetOptionValue(field, unknown_fields.get())) {
    return false;  // SetOptionValue() already added the error.
  }

  // Now wrap the UnknownFieldSet with UnknownFieldSets corresponding to all
  // the intermediate messages.
  for (std::vector<const FieldDescriptor*>::reverse_iterator iter =
           intermediate_fields.rbegin();
       iter != intermediate_fields.rend(); ++iter) {
    std::unique_ptr<UnknownFieldSet> parent_unknown_fields(
        new UnknownFieldSet());
    switch ((*iter)->type()) {
      case FieldDescriptor::TYPE_MESSAGE: {
        std::string outstr;
        ABSL_CHECK(unknown_fields->SerializeToString(&outstr))
            << "Unexpected failure while serializing option submessage "
            << debug_msg_name << "\".";
        parent_unknown_fields->AddLengthDelimited((*iter)->number(),
                                                  std::move(outstr));
        break;
      }

      case FieldDescriptor::TYPE_GROUP: {
        parent_unknown_fields->AddGroup((*iter)->number())
            ->MergeFrom(*unknown_fields);
        break;
      }

      default:
        ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_MESSAGE: "
                        << (*iter)->type();
        return false;
    }
    unknown_fields = std::move(parent_unknown_fields);
  }

  // Now merge the UnknownFieldSet corresponding to the top-level message into
  // the options message.
  options->GetReflection()->MutableUnknownFields(options)->MergeFrom(
      *unknown_fields);

  // record the element path of the interpreted option
  if (field->is_repeated()) {
    int index = repeated_option_counts_[dest_path]++;
    dest_path.push_back(index);
  }
  interpreted_paths_[src_path] = dest_path;

  return true;
}

void DescriptorBuilder::OptionInterpreter::UpdateSourceCodeInfo(
    SourceCodeInfo* info) {
  if (interpreted_paths_.empty()) {
    // nothing to do!
    return;
  }

  // We find locations that match keys in interpreted_paths_ and
  // 1) replace the path with the corresponding value in interpreted_paths_
  // 2) remove any subsequent sub-locations (sub-location is one whose path
  //    has the parent path as a prefix)
  //
  // To avoid quadratic behavior of removing interior rows as we go,
  // we keep a copy. But we don't actually copy anything until we've
  // found the first match (so if the source code info has no locations
  // that need to be changed, there is zero copy overhead).

  RepeatedPtrField<SourceCodeInfo_Location>* locs = info->mutable_location();
  RepeatedPtrField<SourceCodeInfo_Location> new_locs;
  bool copying = false;

  std::vector<int> pathv;
  bool matched = false;

  for (RepeatedPtrField<SourceCodeInfo_Location>::iterator loc = locs->begin();
       loc != locs->end(); loc++) {
    if (matched) {
      // see if this location is in the range to remove
      bool loc_matches = true;
      if (loc->path_size() < static_cast<int64_t>(pathv.size())) {
        loc_matches = false;
      } else {
        for (size_t j = 0; j < pathv.size(); j++) {
          if (loc->path(j) != pathv[j]) {
            loc_matches = false;
            break;
          }
        }
      }

      if (loc_matches) {
        // don't copy this row since it is a sub-location that we're removing
        continue;
      }

      matched = false;
    }

    pathv.clear();
    for (int j = 0; j < loc->path_size(); j++) {
      pathv.push_back(loc->path(j));
    }

    auto entry = interpreted_paths_.find(pathv);

    if (entry == interpreted_paths_.end()) {
      // not a match
      if (copying) {
        *new_locs.Add() = *loc;
      }
      continue;
    }

    matched = true;

    if (!copying) {
      // initialize the copy we are building
      copying = true;
      new_locs.Reserve(locs->size());
      for (RepeatedPtrField<SourceCodeInfo_Location>::iterator it =
               locs->begin();
           it != loc; it++) {
        *new_locs.Add() = *it;
      }
    }

    // add replacement and update its path
    SourceCodeInfo_Location* replacement = new_locs.Add();
    *replacement = *loc;
    replacement->clear_path();
    for (std::vector<int>::iterator rit = entry->second.begin();
         rit != entry->second.end(); rit++) {
      replacement->add_path(*rit);
    }
  }

  // if we made a changed copy, put it in place
  if (copying) {
    *locs = std::move(new_locs);
  }
}

void DescriptorBuilder::OptionInterpreter::AddWithoutInterpreting(
    const UninterpretedOption& uninterpreted_option, Message* options) {
  const FieldDescriptor* field =
      options->GetDescriptor()->FindFieldByName("uninterpreted_option");
  ABSL_CHECK(field != nullptr);

  options->GetReflection()
      ->AddMessage(options, field)
      ->CopyFrom(uninterpreted_option);
}

bool DescriptorBuilder::OptionInterpreter::ExamineIfOptionIsSet(
    std::vector<const FieldDescriptor*>::const_iterator
        intermediate_fields_iter,
    std::vector<const FieldDescriptor*>::const_iterator intermediate_fields_end,
    const FieldDescriptor* innermost_field, const std::string& debug_msg_name,
    const UnknownFieldSet& unknown_fields) {
  // We do linear searches of the UnknownFieldSet and its sub-groups.  This
  // should be fine since it's unlikely that any one options structure will
  // contain more than a handful of options.

  if (intermediate_fields_iter == intermediate_fields_end) {
    // We're at the innermost submessage.
    for (int i = 0; i < unknown_fields.field_count(); i++) {
      if (unknown_fields.field(i).number() == innermost_field->number()) {
        return AddNameError([&] {
          return absl::StrCat("Option \"", debug_msg_name,
                              "\" was already set.");
        });
      }
    }
    return true;
  }

  for (int i = 0; i < unknown_fields.field_count(); i++) {
    if (unknown_fields.field(i).number() ==
        (*intermediate_fields_iter)->number()) {
      const UnknownField* unknown_field = &unknown_fields.field(i);
      FieldDescriptor::Type type = (*intermediate_fields_iter)->type();
      // Recurse into the next submessage.
      switch (type) {
        case FieldDescriptor::TYPE_MESSAGE:
          if (unknown_field->type() == UnknownField::TYPE_LENGTH_DELIMITED) {
            UnknownFieldSet intermediate_unknown_fields;
            if (intermediate_unknown_fields.ParseFromString(
                    unknown_field->length_delimited()) &&
                !ExamineIfOptionIsSet(intermediate_fields_iter + 1,
                                      intermediate_fields_end, innermost_field,
                                      debug_msg_name,
                                      intermediate_unknown_fields)) {
              return false;  // Error already added.
            }
          }
          break;

        case FieldDescriptor::TYPE_GROUP:
          if (unknown_field->type() == UnknownField::TYPE_GROUP) {
            if (!ExamineIfOptionIsSet(intermediate_fields_iter + 1,
                                      intermediate_fields_end, innermost_field,
                                      debug_msg_name, unknown_field->group())) {
              return false;  // Error already added.
            }
          }
          break;

        default:
          ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_MESSAGE: " << type;
          return false;
      }
    }
  }
  return true;
}

namespace {
// Helpers for method below

template <typename T>
std::string ValueOutOfRange(absl::string_view type_name,
                            absl::string_view option_name) {
  return absl::StrFormat("Value out of range, %d to %d, for %s option \"%s\".",
                         std::numeric_limits<T>::min(),
                         std::numeric_limits<T>::max(), type_name, option_name);
}

template <typename T>
std::string ValueMustBeInt(absl::string_view type_name,
                           absl::string_view option_name) {
  return absl::StrFormat(
      "Value must be integer, from %d to %d, for %s option \"%s\".",
      std::numeric_limits<T>::min(), std::numeric_limits<T>::max(), type_name,
      option_name);
}

}  // namespace

bool DescriptorBuilder::OptionInterpreter::SetOptionValue(
    const FieldDescriptor* option_field, UnknownFieldSet* unknown_fields) {
  // We switch on the CppType to validate.
  switch (option_field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() >
            static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
          return AddValueError([&] {
            return ValueOutOfRange<int32_t>("int32", option_field->full_name());
          });
        } else {
          SetInt32(option_field->number(),
                   uninterpreted_option_->positive_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else if (uninterpreted_option_->has_negative_int_value()) {
        if (uninterpreted_option_->negative_int_value() <
            static_cast<int64_t>(std::numeric_limits<int32_t>::min())) {
          return AddValueError([&] {
            return ValueOutOfRange<int32_t>("int32", option_field->full_name());
          });
        } else {
          SetInt32(option_field->number(),
                   uninterpreted_option_->negative_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else {
        return AddValueError([&] {
          return ValueMustBeInt<int32_t>("int32", option_field->full_name());
        });
      }
      break;

    case FieldDescriptor::CPPTYPE_INT64:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() >
            static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
          return AddValueError([&] {
            return ValueOutOfRange<int64_t>("int64", option_field->full_name());
          });
        } else {
          SetInt64(option_field->number(),
                   uninterpreted_option_->positive_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else if (uninterpreted_option_->has_negative_int_value()) {
        SetInt64(option_field->number(),
                 uninterpreted_option_->negative_int_value(),
                 option_field->type(), unknown_fields);
      } else {
        return AddValueError([&] {
          return ValueMustBeInt<int64_t>("int64", option_field->full_name());
        });
      }
      break;

    case FieldDescriptor::CPPTYPE_UINT32:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() >
            std::numeric_limits<uint32_t>::max()) {
          return AddValueError([&] {
            return ValueOutOfRange<uint32_t>("uint32",
                                             option_field->full_name());
          });
        } else {
          SetUInt32(option_field->number(),
                    uninterpreted_option_->positive_int_value(),
                    option_field->type(), unknown_fields);
        }
      } else {
        return AddValueError([&] {
          return ValueMustBeInt<uint32_t>("uint32", option_field->full_name());
        });
      }
      break;

    case FieldDescriptor::CPPTYPE_UINT64:
      if (uninterpreted_option_->has_positive_int_value()) {
        SetUInt64(option_field->number(),
                  uninterpreted_option_->positive_int_value(),
                  option_field->type(), unknown_fields);
      } else {
        return AddValueError([&] {
          return ValueMustBeInt<uint64_t>("uint64", option_field->full_name());
        });
      }
      break;

    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value;
      if (uninterpreted_option_->has_double_value()) {
        value = uninterpreted_option_->double_value();
      } else if (uninterpreted_option_->has_positive_int_value()) {
        value = uninterpreted_option_->positive_int_value();
      } else if (uninterpreted_option_->has_negative_int_value()) {
        value = uninterpreted_option_->negative_int_value();
      } else if (uninterpreted_option_->identifier_value() == "inf") {
        value = std::numeric_limits<float>::infinity();
      } else if (uninterpreted_option_->identifier_value() == "nan") {
        value = std::numeric_limits<float>::quiet_NaN();
      } else {
        return AddValueError([&] {
          return absl::StrCat("Value must be number for float option \"",
                              option_field->full_name(), "\".");
        });
      }
      unknown_fields->AddFixed32(option_field->number(),
                                 internal::WireFormatLite::EncodeFloat(value));
      break;
    }

    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value;
      if (uninterpreted_option_->has_double_value()) {
        value = uninterpreted_option_->double_value();
      } else if (uninterpreted_option_->has_positive_int_value()) {
        value = uninterpreted_option_->positive_int_value();
      } else if (uninterpreted_option_->has_negative_int_value()) {
        value = uninterpreted_option_->negative_int_value();
      } else if (uninterpreted_option_->identifier_value() == "inf") {
        value = std::numeric_limits<double>::infinity();
      } else if (uninterpreted_option_->identifier_value() == "nan") {
        value = std::numeric_limits<double>::quiet_NaN();
      } else {
        return AddValueError([&] {
          return absl::StrCat("Value must be number for double option \"",
                              option_field->full_name(), "\".");
        });
      }
      unknown_fields->AddFixed64(option_field->number(),
                                 internal::WireFormatLite::EncodeDouble(value));
      break;
    }

    case FieldDescriptor::CPPTYPE_BOOL:
      uint64_t value;
      if (!uninterpreted_option_->has_identifier_value()) {
        return AddValueError([&] {
          return absl::StrCat("Value must be identifier for boolean option \"",
                              option_field->full_name(), "\".");
        });
      }
      if (uninterpreted_option_->identifier_value() == "true") {
        value = 1;
      } else if (uninterpreted_option_->identifier_value() == "false") {
        value = 0;
      } else {
        return AddValueError([&] {
          return absl::StrCat(
              "Value must be \"true\" or \"false\" for boolean option \"",
              option_field->full_name(), "\".");
        });
      }
      unknown_fields->AddVarint(option_field->number(), value);
      break;

    case FieldDescriptor::CPPTYPE_ENUM: {
      if (!uninterpreted_option_->has_identifier_value()) {
        return AddValueError([&] {
          return absl::StrCat(
              "Value must be identifier for enum-valued option \"",
              option_field->full_name(), "\".");
        });
      }
      const EnumDescriptor* enum_type = option_field->enum_type();
      const std::string& value_name = uninterpreted_option_->identifier_value();
      const EnumValueDescriptor* enum_value = nullptr;

      if (enum_type->file()->pool() != DescriptorPool::generated_pool()) {
        // Note that the enum value's fully-qualified name is a sibling of the
        // enum's name, not a child of it.
        std::string fully_qualified_name = std::string(enum_type->full_name());
        fully_qualified_name.resize(fully_qualified_name.size() -
                                    enum_type->name().size());
        fully_qualified_name += value_name;

        // Search for the enum value's descriptor in the builder's pool. Note
        // that we use DescriptorBuilder::FindSymbolNotEnforcingDeps(), not
        // DescriptorPool::FindEnumValueByName() because we're already holding
        // the pool's mutex, and the latter method locks it again.
        Symbol symbol =
            builder_->FindSymbolNotEnforcingDeps(fully_qualified_name);
        if (auto* candidate_descriptor = symbol.enum_value_descriptor()) {
          if (candidate_descriptor->type() != enum_type) {
            return AddValueError([&] {
              return absl::StrCat(
                  "Enum type \"", enum_type->full_name(),
                  "\" has no value named \"", value_name, "\" for option \"",
                  option_field->full_name(),
                  "\". This appears to be a value from a sibling type.");
            });
          } else {
            enum_value = candidate_descriptor;
          }
        }
      } else {
        // The enum type is in the generated pool, so we can search for the
        // value there.
        enum_value = enum_type->FindValueByName(value_name);
      }

      if (enum_value == nullptr) {
        return AddValueError([&] {
          return absl::StrCat(
              "Enum type \"", option_field->enum_type()->full_name(),
              "\" has no value named \"", value_name, "\" for option \"",
              option_field->full_name(), "\".");
        });
      } else {
        // Sign-extension is not a problem, since we cast directly from int32_t
        // to uint64_t, without first going through uint32_t.
        unknown_fields->AddVarint(
            option_field->number(),
            static_cast<uint64_t>(static_cast<int64_t>(enum_value->number())));
      }
      break;
    }

    case FieldDescriptor::CPPTYPE_STRING:
      if (!uninterpreted_option_->has_string_value()) {
        return AddValueError([&] {
          return absl::StrCat(
              "Value must be quoted string for string option \"",
              option_field->full_name(), "\".");
        });
      }
      // The string has already been unquoted and unescaped by the parser.
      unknown_fields->AddLengthDelimited(option_field->number(),
                                         uninterpreted_option_->string_value());
      break;

    case FieldDescriptor::CPPTYPE_MESSAGE:
      if (!SetAggregateOption(option_field, unknown_fields)) {
        return false;
      }
      break;
  }

  return true;
}

class DescriptorBuilder::OptionInterpreter::AggregateOptionFinder
    : public TextFormat::Finder {
 public:
  DescriptorBuilder* builder_;

  const Descriptor* FindAnyType(const Message& /*message*/,
                                const std::string& prefix,
                                const std::string& name) const override {
    if (prefix != internal::kTypeGoogleApisComPrefix &&
        prefix != internal::kTypeGoogleProdComPrefix) {
      return nullptr;
    }
    assert_mutex_held(builder_->pool_);
    return builder_->FindSymbol(name).descriptor();
  }

  const FieldDescriptor* FindExtension(Message* message,
                                       const std::string& name) const override {
    assert_mutex_held(builder_->pool_);
    const Descriptor* descriptor = message->GetDescriptor();
    Symbol result =
        builder_->LookupSymbolNoPlaceholder(name, descriptor->full_name());
    if (auto* field = result.field_descriptor()) {
      return field;
    } else if (result.type() == Symbol::MESSAGE &&
               descriptor->options().message_set_wire_format()) {
      const Descriptor* foreign_type = result.descriptor();
      // The text format allows MessageSet items to be specified using
      // the type name, rather than the extension identifier. If the symbol
      // lookup returned a Message, and the enclosing Message has
      // message_set_wire_format = true, then return the message set
      // extension, if one exists.
      for (int i = 0; i < foreign_type->extension_count(); i++) {
        const FieldDescriptor* extension = foreign_type->extension(i);
        if (extension->containing_type() == descriptor &&
            extension->type() == FieldDescriptor::TYPE_MESSAGE &&
            extension->label_ == FieldDescriptor::LABEL_OPTIONAL &&
            extension->message_type() == foreign_type) {
          // Found it.
          return extension;
        }
      }
    }
    return nullptr;
  }
};

// A custom error collector to record any text-format parsing errors
namespace {
class AggregateErrorCollector : public io::ErrorCollector {
 public:
  std::string error_;

  void RecordError(int /* line */, int /* column */,
                   const absl::string_view message) override {
    if (!error_.empty()) {
      absl::StrAppend(&error_, "; ");
    }
    absl::StrAppend(&error_, message);
  }

  void RecordWarning(int /* line */, int /* column */,
                     const absl::string_view /* message */) override {
    // Ignore warnings
  }
};
}  // namespace

// We construct a dynamic message of the type corresponding to
// option_field, parse the supplied text-format string into this
// message, and serialize the resulting message to produce the value.
bool DescriptorBuilder::OptionInterpreter::SetAggregateOption(
    const FieldDescriptor* option_field, UnknownFieldSet* unknown_fields) {
  if (!uninterpreted_option_->has_aggregate_value()) {
    return AddValueError([&] {
      return absl::StrCat("Option \"", option_field->full_name(),
                          "\" is a message. "
                          "To set the entire message, use syntax like \"",
                          option_field->name(),
                          " = { <proto text format> }\". "
                          "To set fields within it, use syntax like \"",
                          option_field->name(), ".foo = value\".");
    });
  }

  const Descriptor* type = option_field->message_type();
  std::unique_ptr<Message> dynamic(dynamic_factory_.GetPrototype(type)->New());
  ABSL_CHECK(dynamic.get() != nullptr)
      << "Could not create an instance of " << option_field->DebugString();

  AggregateErrorCollector collector;
  AggregateOptionFinder finder;
  finder.builder_ = builder_;
  TextFormat::Parser parser;
  parser.RecordErrorsTo(&collector);
  parser.SetFinder(&finder);
  if (!parser.ParseFromString(uninterpreted_option_->aggregate_value(),
                              dynamic.get())) {
    AddValueError([&] {
      return absl::StrCat("Error while parsing option value for \"",
                          option_field->name(), "\": ", collector.error_);
    });
    return false;
  } else {
    std::string serial;
    dynamic->SerializeToString(&serial);  // Never fails
    if (option_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      unknown_fields->AddLengthDelimited(option_field->number(), serial);
    } else {
      ABSL_CHECK_EQ(option_field->type(), FieldDescriptor::TYPE_GROUP);
      UnknownFieldSet* group = unknown_fields->AddGroup(option_field->number());
      group->ParseFromString(serial);
    }
    return true;
  }
}

void DescriptorBuilder::OptionInterpreter::SetInt32(
    int number, int32_t value, FieldDescriptor::Type type,
    UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
      unknown_fields->AddVarint(
          number, static_cast<uint64_t>(static_cast<int64_t>(value)));
      break;

    case FieldDescriptor::TYPE_SFIXED32:
      unknown_fields->AddFixed32(number, static_cast<uint32_t>(value));
      break;

    case FieldDescriptor::TYPE_SINT32:
      unknown_fields->AddVarint(
          number, internal::WireFormatLite::ZigZagEncode32(value));
      break;

    default:
      ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_INT32: " << type;
      break;
  }
}

void DescriptorBuilder::OptionInterpreter::SetInt64(
    int number, int64_t value, FieldDescriptor::Type type,
    UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_INT64:
      unknown_fields->AddVarint(number, static_cast<uint64_t>(value));
      break;

    case FieldDescriptor::TYPE_SFIXED64:
      unknown_fields->AddFixed64(number, static_cast<uint64_t>(value));
      break;

    case FieldDescriptor::TYPE_SINT64:
      unknown_fields->AddVarint(
          number, internal::WireFormatLite::ZigZagEncode64(value));
      break;

    default:
      ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_INT64: " << type;
      break;
  }
}

void DescriptorBuilder::OptionInterpreter::SetUInt32(
    int number, uint32_t value, FieldDescriptor::Type type,
    UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_UINT32:
      unknown_fields->AddVarint(number, static_cast<uint64_t>(value));
      break;

    case FieldDescriptor::TYPE_FIXED32:
      unknown_fields->AddFixed32(number, static_cast<uint32_t>(value));
      break;

    default:
      ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_UINT32: " << type;
      break;
  }
}

void DescriptorBuilder::OptionInterpreter::SetUInt64(
    int number, uint64_t value, FieldDescriptor::Type type,
    UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_UINT64:
      unknown_fields->AddVarint(number, value);
      break;

    case FieldDescriptor::TYPE_FIXED64:
      unknown_fields->AddFixed64(number, value);
      break;

    default:
      ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_UINT64: " << type;
      break;
  }
}

void DescriptorBuilder::LogUnusedDependency(const FileDescriptorProto& proto,
                                            const FileDescriptor* result) {
  (void)result;  // Parameter is used by Google-internal code.

  if (!unused_dependency_.empty()) {
    auto itr = pool_->direct_input_files_.find(proto.name());
    bool is_error = itr != pool_->direct_input_files_.end() && itr->second;
    for (const auto* unused : unused_dependency_) {
      auto make_error = [&] {
        return absl::StrCat("Import ", unused->name(), " is unused.");
      };
      if (is_error) {
        AddError(unused->name(), proto, DescriptorPool::ErrorCollector::IMPORT,
                 make_error);
      } else {
        AddWarning(unused->name(), proto,
                   DescriptorPool::ErrorCollector::IMPORT, make_error);
      }
    }
  }
}

Symbol DescriptorPool::CrossLinkOnDemandHelper(absl::string_view name,
                                               bool expecting_enum) const {
  (void)expecting_enum;  // Parameter is used by Google-internal code.
  auto lookup_name = std::string(name);
  if (!lookup_name.empty() && lookup_name[0] == '.') {
    lookup_name = lookup_name.substr(1);
  }
  Symbol result = tables_->FindByNameHelper(this, lookup_name);
  return result;
}

// Handle the lazy import building for a message field whose type wasn't built
// at cross link time. If that was the case, we saved the name of the type to
// be looked up when the accessor for the type was called. Set type_,
// enum_type_, message_type_, and default_value_enum_ appropriately.
void FieldDescriptor::InternalTypeOnceInit() const {
  ABSL_CHECK(file()->finished_building_ == true);
  const EnumDescriptor* enum_type = nullptr;
  const char* lazy_type_name = reinterpret_cast<const char*>(type_once_ + 1);
  const char* lazy_default_value_enum_name =
      lazy_type_name + strlen(lazy_type_name) + 1;
  Symbol result = file()->pool()->CrossLinkOnDemandHelper(
      lazy_type_name, type_ == FieldDescriptor::TYPE_ENUM);
  if (result.type() == Symbol::MESSAGE) {
    ABSL_CHECK(type_ == FieldDescriptor::TYPE_MESSAGE ||
               type_ == FieldDescriptor::TYPE_GROUP);
    type_descriptor_.message_type = result.descriptor();
  } else if (result.type() == Symbol::ENUM) {
    ABSL_CHECK(type_ == FieldDescriptor::TYPE_ENUM);
    enum_type = type_descriptor_.enum_type = result.enum_descriptor();
  }

  if (enum_type) {
    if (lazy_default_value_enum_name[0] != '\0') {
      // Have to build the full name now instead of at CrossLink time,
      // because enum_type may not be known at the time.
      std::string name = std::string(enum_type->full_name());
      // Enum values reside in the same scope as the enum type.
      std::string::size_type last_dot = name.find_last_of('.');
      if (last_dot != std::string::npos) {
        name = absl::StrCat(name.substr(0, last_dot), ".",
                            lazy_default_value_enum_name);
      } else {
        name = lazy_default_value_enum_name;
      }
      Symbol result_enum = file()->pool()->CrossLinkOnDemandHelper(name, true);
      default_value_enum_ = result_enum.enum_value_descriptor();
    } else {
      default_value_enum_ = nullptr;
    }
    if (!default_value_enum_) {
      // We use the first defined value as the default
      // if a default is not explicitly defined.
      ABSL_CHECK(enum_type->value_count());
      default_value_enum_ = enum_type->value(0);
    }
  }
}

void FieldDescriptor::TypeOnceInit(const FieldDescriptor* to_init) {
  to_init->InternalTypeOnceInit();
}

// message_type(), enum_type(), default_value_enum(), and type()
// all share the same absl::call_once init path to do lazy
// import building and cross linking of a field of a message.
const Descriptor* FieldDescriptor::message_type() const {
  if (type_ == TYPE_MESSAGE || type_ == TYPE_GROUP) {
    if (type_once_) {
      absl::call_once(*type_once_, FieldDescriptor::TypeOnceInit, this);
    }
    return type_descriptor_.message_type;
  }
  return nullptr;
}

const EnumDescriptor* FieldDescriptor::enum_type() const {
  if (type_ == TYPE_ENUM) {
    if (type_once_) {
      absl::call_once(*type_once_, FieldDescriptor::TypeOnceInit, this);
    }
    return type_descriptor_.enum_type;
  }
  return nullptr;
}

const EnumValueDescriptor* FieldDescriptor::default_value_enum() const {
  if (type_once_) {
    absl::call_once(*type_once_, FieldDescriptor::TypeOnceInit, this);
  }
  return default_value_enum_;
}

absl::string_view FieldDescriptor::PrintableNameForExtension() const {
  const bool is_message_set_extension =
      is_extension() &&
      containing_type()->options().message_set_wire_format() &&
      type() == FieldDescriptor::TYPE_MESSAGE && !is_required() &&
      !is_repeated() && extension_scope() == message_type();
  return is_message_set_extension ? message_type()->full_name() : full_name();
}

void FileDescriptor::InternalDependenciesOnceInit() const {
  ABSL_CHECK(finished_building_ == true);
  const char* names_ptr = reinterpret_cast<const char*>(dependencies_once_ + 1);
  for (int i = 0; i < dependency_count(); i++) {
    const char* name = names_ptr;
    names_ptr += strlen(name) + 1;
    if (name[0] != '\0') {
      dependencies_[i] = pool_->FindFileByName(name);
    }
  }
}

void FileDescriptor::DependenciesOnceInit(const FileDescriptor* to_init) {
  to_init->InternalDependenciesOnceInit();
}

const FileDescriptor* FileDescriptor::dependency(int index) const {
  if (dependencies_once_) {
    // Do once init for all indices, as it's unlikely only a single index would
    // be called, and saves on absl::call_once allocations.
    absl::call_once(*dependencies_once_, FileDescriptor::DependenciesOnceInit,
                    this);
  }
  return dependencies_[index];
}

absl::string_view FileDescriptor::option_dependency_name(int index) const {
  ABSL_DCHECK_LT(index, option_dependency_count());
  return option_dependencies_[index];
}

const Descriptor* MethodDescriptor::input_type() const {
  return input_type_.Get(service());
}

const Descriptor* MethodDescriptor::output_type() const {
  return output_type_.Get(service());
}

namespace internal {
void LazyDescriptor::Set(const Descriptor* descriptor) {
  ABSL_CHECK(!once_);
  descriptor_ = descriptor;
}

void LazyDescriptor::SetLazy(absl::string_view name,
                             const FileDescriptor* file) {
  // verify Init() has been called and Set hasn't been called yet.
  ABSL_CHECK(!descriptor_);
  ABSL_CHECK(!once_);
  ABSL_CHECK(file && file->pool_);
  ABSL_CHECK(file->pool_->lazily_build_dependencies_);
  ABSL_CHECK(!file->finished_building_);
  once_ = ::new (file->pool_->tables_->AllocateBytes(static_cast<int>(
      sizeof(absl::once_flag) + name.size() + 1))) absl::once_flag{};
  char* lazy_name = reinterpret_cast<char*>(once_ + 1);
  memcpy(lazy_name, name.data(), name.size());
  lazy_name[name.size()] = 0;
}

void LazyDescriptor::Once(const ServiceDescriptor* service) {
  if (once_) {
    absl::call_once(*once_, [&] {
      auto* file = service->file();
      ABSL_CHECK(file->finished_building_);
      const char* lazy_name = reinterpret_cast<const char*>(once_ + 1);
      descriptor_ =
          file->pool_->CrossLinkOnDemandHelper(lazy_name, false).descriptor();
    });
  }
}

bool ParseNoReflection(absl::string_view from, google::protobuf::MessageLite& to) {
  auto cleanup = DisableTracking();

  to.Clear();
  const char* ptr;
  internal::ParseContext ctx(io::CodedInputStream::GetDefaultRecursionLimit(),
                             false, &ptr, from);
  ptr = to._InternalParse(ptr, &ctx);
  if (ptr == nullptr || !ctx.EndedAtLimit()) return false;
  return to.IsInitializedWithErrors();
}

namespace cpp {
bool HasPreservingUnknownEnumSemantics(const FieldDescriptor* field) {
  if (field->legacy_enum_field_treated_as_closed()) {
    return false;
  }

  return field->enum_type() != nullptr && !field->enum_type()->is_closed();
}

HasbitMode GetFieldHasbitMode(const FieldDescriptor* field) {
  // Do not generate hasbits for "real-oneof", weak, or extension fields.
  if (field->real_containing_oneof() || field->options().weak() ||
      field->is_extension()) {
    return HasbitMode::kNoHasbit;
  }

  // Explicit-presence fields always have true hasbits.
  if (field->has_presence()) {
    return HasbitMode::kTrueHasbit;
  }

  // Both implicit-presence and repeated/map fields have hint hasbits.
  return HasbitMode::kHintHasbit;
}

bool HasHasbit(const FieldDescriptor* field) {
  return GetFieldHasbitMode(field) != HasbitMode::kNoHasbit;
}

static bool IsVerifyUtf8(const FieldDescriptor* field, bool is_lite) {
  if (is_lite) return false;
  return true;
}

// Which level of UTF-8 enforcemant is placed on this file.
Utf8CheckMode GetUtf8CheckMode(const FieldDescriptor* field, bool is_lite) {
  if (field->type() == FieldDescriptor::TYPE_STRING ||
      (field->is_map() && (field->message_type()->map_key()->type() ==
                               FieldDescriptor::TYPE_STRING ||
                           field->message_type()->map_value()->type() ==
                               FieldDescriptor::TYPE_STRING))) {
    if (IsStrictUtf8(field)) {
      return Utf8CheckMode::kStrict;
    } else if (IsVerifyUtf8(field, is_lite)) {
      return Utf8CheckMode::kVerify;
    }
  }
  return Utf8CheckMode::kNone;
}

bool IsGroupLike(const FieldDescriptor& field) {
  // Groups are always tag-delimited, currently specified by a TYPE_GROUP type.
  if (field.type() != FieldDescriptor::TYPE_GROUP) return false;
  // Group fields always are always the lowercase type name.
  if (field.name() != absl::AsciiStrToLower(field.message_type()->name())) {
    return false;
  }

  if (field.message_type()->file() != field.file()) return false;

  // Group messages are always defined in the same scope as the field.  File
  // level extensions will compare NULL == NULL here, which is why the file
  // comparison above is necessary to ensure both come from the same file.
  return field.is_extension() ? field.message_type()->containing_type() ==
                                    field.extension_scope()
                              : field.message_type()->containing_type() ==
                                    field.containing_type();
}

bool IsLazilyInitializedFile(absl::string_view filename) {
  if (filename == "third_party/protobuf/cpp_features.proto" ||
      filename == "google/protobuf/cpp_features.proto") {
    return true;
  }
  if (filename == "third_party/protobuf/internal_options.proto" ||
      filename == "google/protobuf/internal_options.proto") {
    return true;
  }
  return filename == "net/proto2/proto/descriptor.proto" ||
         filename == "google/protobuf/descriptor.proto";
}

bool IsStringFieldWithPrivatizedAccessors(const FieldDescriptor& field) {
  // In open-source, protobuf CORD is only supported for singular bytes
  // fields.
  if (field.cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
      InternalFeatureHelper::GetFeatures(field)
              .GetExtension(pb::cpp)
              .string_type() == pb::CppFeatures::CORD &&
      (field.type() != FieldDescriptor::TYPE_BYTES || field.is_repeated() ||
       field.is_extension())
  ) {
    return true;
  }

  return false;
}

}  // namespace cpp
}  // namespace internal

Edition FileDescriptor::edition() const { return edition_; }

namespace internal {
absl::string_view ShortEditionName(Edition edition) {
  return absl::StripPrefix(Edition_Name(edition), "EDITION_");
}
}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
