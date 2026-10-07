// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_PROTOBUF_TEST_H__
#define GOOGLE_PROTOBUF_PROTOBUF_TEST_H__

// Inline value- and type-parameterized test macros for GoogleTest.
//
// These macros allow tests to parameterize over values and types directly
// inline within the test body, without the boilerplate of `TEST_P` +
// `INSTANTIATE_TEST_SUITE_P` or `TYPED_TEST_SUITE` + `TYPED_TEST`.
// When multiple `PB_TEST_CHOOSE_VALUE` and/or `PB_TEST_CHOOSE_TYPE` statements
// appear in a test body, a separate GoogleTest case is registered and run for
// every combination in their Cartesian product.
//
// Public Macros:
//
//   PB_TEST(TestSuiteName, TestName)
//     Defines a standalone test (analogous to GoogleTest's `TEST`) that
//     supports `PB_TEST_CHOOSE_VALUE` and `PB_TEST_CHOOSE_TYPE` in its body.
//     Can also be used with no `PB_TEST_CHOOSE_*` calls as a normal test.
//
//   PB_TEST_F(TestFixtureName, TestName)
//     Defines a test that uses fixture class `TestFixtureName` (analogous to
//     GoogleTest's `TEST_F`) and supports `PB_TEST_CHOOSE_VALUE` and
//     `PB_TEST_CHOOSE_TYPE` in its body.
//
//   PB_TEST_CHOOSE_VALUE({v1, v2, ...})
//   PB_TEST_CHOOSE_VALUE({v1, v2, ...}, "prefix")
//   PB_TEST_CHOOSE_VALUE({v1, v2, ...}, printer)
//   PB_TEST_CHOOSE_VALUE({v1, v2, ...}, "prefix", printer)
//     Evaluates to `const T&` for one of the provided values `{v1, v2, ...}`,
//     which must share a common type `T`. Each lexical call site defines an
//     independent value dimension. Value expressions may depend on types
//     selected by `PB_TEST_CHOOSE_TYPE` in the same test.
//     An optional `"prefix"` string can be provided to prepend `<prefix>_` to
//     each value's test name suffix. An optional `printer` callable with
//     signature `std::string(const T& value, size_t index)` can also be
//     provided to customize the test name suffix for each value.
//
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...))
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...), "prefix")
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...), Printer)
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...), "prefix", Printer)
//   PB_TEST_CHOOSE_TYPE(Tuple)
//   PB_TEST_CHOOSE_TYPE(Tuple, "prefix")
//   PB_TEST_CHOOSE_TYPE(Tuple, Printer)
//   PB_TEST_CHOOSE_TYPE(Tuple, "prefix", Printer)
//     Evaluates to a type (usable in `using T = PB_TEST_CHOOSE_TYPE(...);` or
//     any type context) for one of the provided types `T1, T2, ...`, specified
//     either as a parenthesized list `(T1, T2, ...)` or as a `std::tuple<...>`
//     type `Tuple`. Each lexical call site defines an independent type
//     dimension.
//     An optional `"prefix"` string literal (requires C++20) can be provided to
//     prepend `<prefix>_` to each type's test name suffix. An optional
//     `Printer` type providing
//     `template <typename T> static std::string Print(size_t index)` can also
//     be passed to customize the test name suffix for each type.
//
// Example:
//
//   PB_TEST(MySuite, WorksForAllCombinations) {
//     using T = PB_TEST_CHOOSE_TYPE((int32_t, int64_t, double));
//     T delta = PB_TEST_CHOOSE_VALUE({T{0}, T{1}, T{10}}, "delta");
//     bool negate = PB_TEST_CHOOSE_VALUE({false, true}, "negate");
//
//     T value = negate ? -delta : delta;
//     EXPECT_EQ(std::abs(value), delta);
//   }
//
//   class MyFixture : public ::testing::Test { ... };
//
//   PB_TEST_F(MyFixture, WorksWithFixture) {
//     using MessageT = PB_TEST_CHOOSE_TYPE((FooProto, BarProto));
//     int count = PB_TEST_CHOOSE_VALUE({1, 5, 10});
//     ...
//   }
//
// Test Naming:
//   Registered test names have the form:
//     `<TestName>/<v1>/<v2>/.../<T1>/<T2>/...`
//   (omitting the value or type suffix when none are present), with value and
//   type names automatically sanitized and disambiguated.
//
// Restrictions:
//   - Do NOT define `PB_TEST` or `PB_TEST_F` in header files.
//   - `PB_TEST_CHOOSE_VALUE` and `PB_TEST_CHOOSE_TYPE` must be written
//     lexically inside the `PB_TEST` / `PB_TEST_F` body (not inside helper
//     functions defined outside the test).
//   - Do NOT place `PB_TEST_CHOOSE_VALUE` inside a runtime loop expecting it to
//     yield different values on each iteration; a single lexical call site
//     always returns the same selected value for a given test run.

#include <algorithm>
#include <cstddef>
#include <deque>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/functional/any_invocable.h"
#include "absl/strings/charset.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_enum_reflection.h"
#include "google/protobuf/get_type_name.h"
#include "google/protobuf/port.h"

#define PB_TEST(test_suite_name, test_name) \
  PB_TEST_(test_suite_name, test_name, ::testing::Test)

#define PB_TEST_F(test_suite_name, test_name) \
  PB_TEST_(test_suite_name, test_name, test_suite_name)

#define PB_TEST_CHOOSE_VALUE(...)                                         \
  ::google::protobuf::internal::TestChooseValueSelector<InternalSelf_, __COUNTER__, \
                                              InternalIndices_>(          \
      this->pb_test_internal_param_idx_,                                  \
      [] { return ::google::protobuf::internal::ChooseValueImpl(__VA_ARGS__); })

#define PB_TEST_CHOOSE_TYPE(...)                           \
  typename ::google::protobuf::internal::TestChooseTypeSelector<     \
      InternalSelf_,                                       \
      decltype(::google::protobuf::internal::TestChooseTypeParams<   \
               PB_TEST_CHOOSE_TYPE_ARGS_(__VA_ARGS__)>()), \
      __COUNTER__, InternalIndices_>::type

// =============================================================================
// Implementation details follow. Do not use directly.
// =============================================================================

namespace google::protobuf::internal {

#define PB_TEST_STRIP_PB_TEST_EXPANDED_
#define PB_TEST_STRIP_PB_TEST_PARENS_TO_TUPLE_
#define PB_TEST_PARENS_TO_TUPLE_(...) \
  PB_TEST_EXPANDED_ ::std::tuple<__VA_ARGS__>
#define PB_TEST_STRIP_PREFIX_IMPL_(...) PB_TEST_STRIP_##__VA_ARGS__
#define PB_TEST_STRIP_PREFIX_(...) PB_TEST_STRIP_PREFIX_IMPL_(__VA_ARGS__)
#define PB_TEST_CHOOSE_TYPE_ARGS_(...) \
  PB_TEST_STRIP_PREFIX_(PB_TEST_PARENS_TO_TUPLE_ __VA_ARGS__)

#define PB_TEST_IMPL_(test_suite_name, test_name, self_class, parent_class)  \
  static_assert(sizeof(GTEST_STRINGIFY_(test_suite_name)) > 1,               \
                "test_suite_name must not be empty");                        \
  static_assert(sizeof(GTEST_STRINGIFY_(test_name)) > 1,                     \
                "test_name must not be empty");                              \
  class self_class : public parent_class {                                   \
   public:                                                                   \
    using InternalParent_ = parent_class;                                    \
    using InternalSelf_ = self_class;                                        \
                                                                             \
    template <typename Indices>                                              \
    explicit self_class(Indices*, size_t idx)                                \
        : pb_test_internal_param_idx_(idx),                                  \
          test_body_(&self_class::TestBodyImpl<Indices>) {}                  \
    ~self_class() override = default;                                        \
    self_class(const self_class&) = delete;                                  \
    void operator=(const self_class&) = delete; /* NOLINT */                 \
    template <typename Indices>                                              \
    static auto Instantiate_() {                                             \
      return &self_class::TestBodyImpl<Indices>;                             \
    }                                                                        \
                                                                             \
   private:                                                                  \
    void TestBody() override { (this->*test_body_)(); }                      \
    template <typename Indices>                                              \
    void TestBodyImpl();                                                     \
    static inline bool reg_ [[maybe_unused]] =                               \
        (::google::protobuf::internal::PartialTestRegistry::Get()                      \
             .SetPrimary<self_class>(#test_suite_name, #test_name, __FILE__, \
                                     __LINE__),                              \
         false);                                                             \
    size_t pb_test_internal_param_idx_ = 0;                                  \
    void (self_class::*test_body_)();                                        \
  };                                                                         \
                                                                             \
  template <typename InternalIndices_>                                       \
  void self_class::TestBodyImpl()

#define PB_TEST_(test_suite_name, test_name, parent_class)                 \
  PB_TEST_IMPL_(test_suite_name, test_name, test_suite_name##_##test_name, \
                parent_class)

struct PerCounterIndex {
  size_t counter;
  size_t value;

  friend bool operator<(PerCounterIndex a, PerCounterIndex b) {
    return std::tuple(a.counter, a.value) < std::tuple(b.counter, b.value);
  }
};

template <size_t Counter, size_t Value>
struct StaticPerCounterIndex {
  static constexpr PerCounterIndex value = {Counter, Value};
  static_assert(Value != 0);
};

template <typename T, typename Printer>
struct ChooseValues {
  using value_type = T;
  // We use std::deque because std::vector<bool> is problematic
  std::deque<T> values;
  absl::optional<absl::string_view> name_prefix;
};

class PartialTestRegistry {
 public:
  template <typename T, typename Class = PartialTestRegistry>
  static constexpr const void* kKey = &Class::template kKey<T>;

  static auto& Get() {
    static auto* r = new PartialTestRegistry();
    return *r;
  }

  template <typename Fixture, typename Indices>
  void AddDoRegister() {
    auto& pf = fixtures_[kKey<Fixture>];
    auto& t = pf.per_instantiation_[kKey<Indices>];
    t.instantiation_key = IndexAsVector(static_cast<Indices*>(nullptr));
    t.do_register = [this](size_t i) {
      auto& pf = fixtures_[kKey<Fixture>];
      auto& t = pf.per_instantiation_[kKey<Indices>];
      const std::string type_name = t.TypeName();
      const std::string value_name = t.values_.empty() ? "" : t.ValueName(i);
      std::string test_name = pf.test_name;
      if (!value_name.empty()) {
        absl::StrAppend(&test_name, "/", value_name);
      }
      if (!type_name.empty()) {
        absl::StrAppend(&test_name, "/", type_name);
      }

      ::testing::RegisterTest(
          pf.test_suite_name, test_name.c_str(),
          type_name.empty() ? nullptr : type_name.c_str(),
          value_name.empty() ? nullptr : value_name.c_str(), pf.file, pf.line,
          [i]() -> typename Fixture::InternalParent_* {
            return new Fixture(static_cast<Indices*>(nullptr), i);
          });
    };
  }

  template <typename Fixture>
  void SetPrimary(const char* test_suite_name, const char* test_name,
                  const char* file, int line) {
    auto& pf = fixtures_[kKey<Fixture>];
    pf.test_suite_name = test_suite_name;
    pf.test_name = test_name;
    pf.file = file;
    pf.line = line;
    AddDoRegister<Fixture, void()>();
  }

  static void SanitizeNames(std::vector<std::string>& names) {
    for (auto& name : names) {
      const auto replace = [&](absl::string_view needle,
                               absl::string_view rep) {
        while (true) {
          if (auto pos = absl::string_view(name).find(needle);
              pos != name.npos) {
            name.replace(pos, needle.size(), rep.data(), rep.size());
          } else {
            break;
          }
        }
      };

      // Let's keep "pointer" in the name.
      replace("*", "Ptr");

      for (char& c : name) {
        static constexpr absl::CharSet kAllowed =
            absl::CharSet::AsciiAlphanumerics() | absl::CharSet::Char('_');
        if (!kAllowed.contains(c)) {
          c = '_';
        }
      }

      // Collapse duplicate _
      replace("__", "_");

      // Remove leading _
      while (name[0] == '_') name.erase(0, 1);
      // Remove trailing _
      while (!name.empty() && name.back() == '_') name.pop_back();

      // Don't have empty names.
      if (name.empty()) name = "_";
    }

    DedupNames(names);
  }

  static void DedupNames(std::vector<std::string>& names) {
    absl::flat_hash_set<std::string> dupes;
    for (auto& s : names) {
      if (dupes.insert(s).second) continue;
      for (int i = 2;; ++i) {
        std::string dedup =
            s == "_" ? absl::StrCat("_", i) : absl::StrCat(s, "_", i);
        if (dupes.insert(dedup).second) {
          s = std::move(dedup);
          break;
        }
      }
    }
  }

  template <typename Fixture, typename T, typename Printer>
  void AddValueDimension(size_t id, const void* instantiation_key,
                         ChooseValues<T, Printer> values) {
    auto& d = fixtures_[kKey<Fixture>]
                  .per_instantiation_[instantiation_key]
                  .values_[id];
    for (size_t i = 0; i < values.values.size(); ++i) {
      d.names.push_back(Printer{}(values.values[i], i));
    }
    if (values.name_prefix.has_value()) {
      for (auto& n : d.names) {
        n = absl::StrCat(*values.name_prefix, "_", n);
      }
    }
    SanitizeNames(d.names);
    d.values = new auto(std::move(values.values));
  }

  template <typename Fixture, size_t id, typename Printer, typename... Types>
  void AddTypeDimension(const void* instantiation_key, std::tuple<Types...>*,
                        absl::optional<absl::string_view> name_prefix) {
    auto& d = fixtures_[kKey<Fixture>]
                  .per_instantiation_[instantiation_key]
                  .types_[id];
    if (!d.names.empty()) return;
    int i = 0;
    (d.names.push_back(Printer::template Print<Types>(i++)), ...);
    if (name_prefix.has_value()) {
      for (auto& n : d.names) {
        n = absl::StrCat(*name_prefix, "_", n);
      }
    }
    SanitizeNames(d.names);
  }

  template <typename Fixture, typename T>
  const T& GetValue(size_t index, size_t id, const void*,
                    const void* instantiation_key) {
    for (const auto& [i, dim] : fixtures_[kKey<Fixture>]
                                    .per_instantiation_[instantiation_key]
                                    .values_) {
      if (i == id) {
        auto& v = *static_cast<const std::deque<T>*>(dim.values);
        return v[index % v.size()];
      } else {
        index /= dim.names.size();
      }
    }

    Unreachable();
  }

  void RegisterAll() const;

 private:
  PartialTestRegistry() = default;

  template <typename... I>
  static std::vector<PerCounterIndex> IndexAsVector(void (*)(I...)) {
    return {I::value...};
  }

  struct PerFixture {
    const char* test_suite_name;
    const char* test_name;
    const char* file;
    int line;

    struct PerInstantiation {
      absl::AnyInvocable<void(size_t index) const&> do_register;
      struct TypeDimension {
        std::vector<std::string> names;
      };
      struct ValueDimension {
        std::vector<std::string> names;
        void* values;
      };
      std::string TypeName() const;
      std::string ValueName(size_t i) const;

      std::vector<PerCounterIndex> instantiation_key;

      // We want these sorted.
      absl::btree_map<size_t, TypeDimension> types_;
      absl::btree_map<size_t, ValueDimension> values_;
    };
    absl::flat_hash_map<const void*, PerInstantiation> per_instantiation_;
  };
  // We need pointer stability.
  absl::flat_hash_map<const void*, PerFixture> fixtures_;
};

struct DefaultValuePrinter {
  template <typename T>
  std::string operator()(const T& value, size_t i) const {
    if constexpr (std::is_same_v<T, bool>) {
      return value ? "true" : "false";
    } else if constexpr (google::protobuf::is_proto_enum<T>::value) {
      auto* e = google::protobuf::GetEnumDescriptor<T>()->FindValueByNumber(value);
      return e != nullptr ? std::string(e->name())
                          : absl::StrCat(static_cast<int>(value));
    } else {
      return absl::StrCat(value);
    }
  }
};

template <typename T, size_t N, typename Printer = DefaultValuePrinter,
          typename = decltype(Printer{}(std::declval<const T>(), size_t{1}))>
auto ChooseValueImpl(const T (&values)[N], absl::string_view name_prefix,
                     Printer = Printer{}) {
  return ChooseValues<T, Printer>{
      std::deque<T>(std::begin(values), std::end(values)), name_prefix};
}

template <typename T, size_t N, typename Printer = DefaultValuePrinter,
          typename = decltype(Printer{}(std::declval<const T>(), size_t{1}))>
auto ChooseValueImpl(const T (&values)[N], Printer = Printer{}) {
  return ChooseValues<T, Printer>{
      std::deque<T>(std::begin(values), std::end(values))};
}

template <typename F>
inline std::false_type Run = (F{}(), std::false_type{});

template <typename Fixture, size_t id, typename Indices, typename F>
const auto& TestChooseValueSelector(size_t index, F) {
  static constexpr auto reg = [] {
    PartialTestRegistry::Get().AddValueDimension<Fixture>(
        id, PartialTestRegistry::kKey<Indices>, F{}());
  };
  return PartialTestRegistry::Get()
      .GetValue<Fixture, typename decltype(F{}())::value_type>(
          index, id, &Run<decltype(reg)>, PartialTestRegistry::kKey<Indices>);
}

template <size_t id, typename... Indices>
constexpr size_t GetTypeIndexForId(void (*)(Indices...)) {
  return ((Indices::value.counter == id ? Indices::value.value : 0) + ... + 0);
}

template <typename Types, typename Printer, auto Prefix>
struct ChooseTypes {
  using types = Types;
  using printer = Printer;

  static constexpr absl::optional<absl::string_view> prefix() {
    if constexpr (std::is_same_v<decltype(Prefix), int>) {
      return absl::nullopt;
    } else {
      return Prefix.buf;
    }
  }
};

struct DefaultTypePrinter {
  template <typename T>
  static std::string Print(size_t index) {
    return GetTypeName<T>().value_or(absl::StrCat(index));
  }
};

template <typename Tuple, typename Printer = DefaultTypePrinter>
ChooseTypes<Tuple, Printer, 0> TestChooseTypeParams();

struct TypeParamsPrefix {
  template <size_t N>
  constexpr TypeParamsPrefix(const char (&n)[N]) : buf{} {
    static_assert(N <= sizeof(buf), "Prefix is too long");
    std::copy(n, n + N, buf);
  }
  char buf[64];
};

template <typename Tuple,
          std::enable_if_t<!std::is_void_v<Tuple>, TypeParamsPrefix> prefix,
          typename Printer = DefaultTypePrinter>
ChooseTypes<Tuple, Printer, prefix> TestChooseTypeParams();

template <typename T, auto*>
using WithRegistration = T;

template <typename... Indices>
constexpr absl::optional<size_t> LargestCounter(void (*)(Indices...)) {
  if constexpr (sizeof...(Indices) == 0) {
    return absl::nullopt;
  } else {
    return std::max({Indices::value.counter...});
  }
}

template <typename I, typename Indices>
struct AppendIndex;
template <typename I, typename... Indices>
struct AppendIndex<I, void(Indices...)> {
  using type = void(Indices..., I);
};

template <typename Fixture, size_t id, size_t i, size_t size, typename Indices>
void InstantiateAllTypes() {
  if constexpr (id <= LargestCounter(static_cast<Indices*>(nullptr))) {
    // We instantiate in only one direction.
  } else if constexpr (i == size) {
    return;
  } else {
    if constexpr (i != 0) {
      Fixture::template Instantiate_<
          typename AppendIndex<StaticPerCounterIndex<id, i>, Indices>::type>();
    }
    InstantiateAllTypes<Fixture, id, i + 1, size, Indices>();
  }
}

template <typename Fixture, typename Types, size_t id, typename Indices>
struct TestChooseTypeSelector {
  static constexpr auto Register = []() {
    PartialTestRegistry::Get()
        .AddTypeDimension<Fixture, id, typename Types::printer>(
            PartialTestRegistry::kKey<Indices>,
            static_cast<typename Types::types*>(nullptr), Types::prefix());
    InstantiateAllTypes<Fixture, id, 0,
                        std::tuple_size_v<typename Types::types>, Indices>();
    PartialTestRegistry::Get().AddDoRegister<Fixture, Indices>();
  };
  using type =
      WithRegistration<std::tuple_element_t<GetTypeIndexForId<id>(
                                                static_cast<Indices*>(nullptr)),
                                            typename Types::types>,
                       &Run<decltype(Register)>>;
};

}  // namespace protobuf
}  // namespace google::internal

#endif  // GOOGLE_PROTOBUF_PROTOBUF_TEST_H__
