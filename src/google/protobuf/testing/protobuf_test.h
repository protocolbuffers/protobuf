// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_H__
#define GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_H__

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
//     `PB_TEST_CHOOSE_TYPE` in its body, as well as `PB_TEST_CHOOSE_VALUE`
//     inside `TestFixtureName`'s constructor, default member initializers,
//     `SetUp()`, or helper methods.
//
//   PB_TEST_CHOOSE_VALUE((v1, v2, ...))
//   PB_TEST_CHOOSE_VALUE((v1, v2, ...), "prefix")
//   PB_TEST_CHOOSE_VALUE((v1, v2, ...), printer)
//   PB_TEST_CHOOSE_VALUE((v1, v2, ...), "prefix", printer)
//   PB_TEST_CHOOSE_VALUE((v1, v2, ...), {"name1", "name2", ...})
//   PB_TEST_CHOOSE_VALUE((v1, v2, ...), "prefix", {"name1", "name2", ...})
//   PB_TEST_CHOOSE_VALUE(iterable)
//   PB_TEST_CHOOSE_VALUE(iterable, "prefix")
//   PB_TEST_CHOOSE_VALUE(iterable, printer)
//   PB_TEST_CHOOSE_VALUE(iterable, "prefix", printer)
//     Evaluates to `T` (by value) for one of the provided values, specified
//     either as a parenthesized list `(v1, v2, ...)` with common type `T`
//     (`std::common_type_t`), or as any iterable range (such as a container,
//     C-array, or `testing::Bool()`, `testing::Range(...)`,
//     `testing::ValuesIn(...)`). Each lexical call site defines an independent
//     value dimension. Value expressions may depend on types selected by
//     `PB_TEST_CHOOSE_TYPE` in the same test; parenthesized value expressions
//     may also capture local variables or fixture members (`this`).
//     May also be used inside the constructor, default member initializers,
//     `SetUp()`, or helper methods of the direct `PB_TEST_F` fixture class `F`
//     (not in base/grandparent classes of `F`), in which case all `PB_TEST_F`
//     tests on `F` inherit those value dimensions.
//     An optional `"prefix"` string literal can be provided to prepend
//     `<prefix>_` to each value's test name suffix.
//     To customize value names, pass either:
//       - an explicit braced list of names `{"name1", "name2", ...}` with the
//         same number of elements as `(v1, v2, ...)`, or
//       - a `printer` callable with signature
//         `std::string(const T& value, size_t index)`.
//     When no explicit names or printer are provided:
//       - If the value expressions do not capture local state, names are
//         formatted from the values (supporting arithmetic types, character
//         literals, strings, proto enums, and non-message `AbslStringify`
//         types), falling back to best-effort extraction from the literal
//         expressions when the type cannot be formatted (e.g., proto messages,
//         structs, lambdas) or when the formatted name exceeds
//         `DefaultValuePrinter::kMaxPrintedLength`, and then to the underlying
//         integer value (for non-proto enums) or 0-based indices (`0`, `1`,
//         ...) for other unprintable types.
//       - If the value expressions capture local state, names are extracted on
//         a best-effort basis from the literal expressions, falling back to the
//         underlying integer value (for enums) or 0-based indices (`0`, `1`,
//         ...).
//
//   PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...))
//   PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), "prefix")
//   PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), printer)
//   PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), "prefix", printer)
//   PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), {"name1", "name2", ...})
//   PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), "prefix", {"name1", "name2", ...})
//   PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq)
//   PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, "prefix")
//   PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, printer)
//   PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, "prefix", printer)
//   PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, {"name1", "name2", ...})
//   PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, "prefix", {"name1", "name2", ...})
//     Compile-time and/or heterogeneous counterpart to `PB_TEST_CHOOSE_VALUE`.
//     Evaluates to the selected element (usable in `constexpr` contexts when
//     the input expressions are `constexpr`), where elements are specified
//     either as a parenthesized list `(v1, v2, ...)` whose elements may have
//     distinct types, or as a tuple-like object (`std::tuple`, `std::pair`,
//     `std::array`) or `std::integer_sequence`.
//     Supports the same optional `"prefix"`, `printer`, and
//     `{"name1", "name2", ...}` arguments and naming rules as
//     `PB_TEST_CHOOSE_VALUE`.
//
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...))
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...), "prefix")
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...), Printer)
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...), "prefix", Printer)
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...), {"name1", "name2", ...})
//   PB_TEST_CHOOSE_TYPE((T1, T2, ...), "prefix", {"name1", "name2", ...})
//   PB_TEST_CHOOSE_TYPE(Tuple)
//   PB_TEST_CHOOSE_TYPE(Tuple, "prefix")
//   PB_TEST_CHOOSE_TYPE(Tuple, Printer)
//   PB_TEST_CHOOSE_TYPE(Tuple, "prefix", Printer)
//   PB_TEST_CHOOSE_TYPE(Tuple, {"name1", "name2", ...})
//   PB_TEST_CHOOSE_TYPE(Tuple, "prefix", {"name1", "name2", ...})
//     Evaluates to a type (usable in `using T = PB_TEST_CHOOSE_TYPE(...);` or
//     any type context) for one of the provided types `T1, T2, ...`, specified
//     either as a parenthesized list `(T1, T2, ...)` or as a `std::tuple<...>`
//     type `Tuple`. Each lexical call site defines an independent type
//     dimension.
//     An optional `"prefix"` string literal can be provided to prepend
//     `<prefix>_` to each type's test name suffix.
//     To customize type names, pass either:
//       - an explicit braced list of names `{"name1", "name2", ...}` with the
//         same number of elements as the type list, or
//       - a `Printer` type providing
//         `template <typename T> static std::string Print(size_t index)`.
//     When no explicit names or `Printer` are provided, names are obtained via
//     `GetTypeName<T>()`, falling back to best-effort extraction from the
//     literal type list, and then to 0-based indices (`0`, `1`, ...).
//
// Example:
//
//   PB_TEST(MySuite, WorksForAllCombinations) {
//     using T = PB_TEST_CHOOSE_TYPE((int32_t, int64_t, double));
//     T delta = PB_TEST_CHOOSE_VALUE((T{0}, T{1}, T{10}), "delta");
//     bool negate = PB_TEST_CHOOSE_VALUE(testing::Bool(), "negate");
//
//     T value = negate ? -delta : delta;
//     EXPECT_EQ(std::abs(value), delta);
//   }
//
//   class MyFixture : public ::testing::Test {
//    protected:
//     bool use_arena_ = PB_TEST_CHOOSE_VALUE(testing::Bool(), "arena");
//   };
//
//   PB_TEST_F(MyFixture, WorksWithFixture) {
//     using MessageT = PB_TEST_CHOOSE_TYPE((FooProto, BarProto));
//     int count = PB_TEST_CHOOSE_VALUE(testing::Range(1, 5));
//     ...
//   }
//
// Test Naming:
//   Registered test names have the form:
//     `<TestName>/<p1>/<p2>/...`
//   ordered by lexical order of the `PB_TEST_CHOOSE_*` calls (omitting the
//   suffix when no parameters are present), with common
//   qualification/member prefixes (`ns::`, `obj.`, `ptr->`) and unambiguous
//   namespace qualifiers stripped, and names automatically sanitized and
//   disambiguated.
//
// Restrictions:
//   - Do NOT define `PB_TEST` or `PB_TEST_F` in header files.
//   - `PB_TEST_CHOOSE_TYPE` and `PB_TEST_CHOOSE_CONSTEXPR` must be written
//     lexically inside the `PB_TEST` / `PB_TEST_F` body.
//   - `PB_TEST_CHOOSE_VALUE` must only be evaluated during a `PB_TEST` or
//     `PB_TEST_F` test, and only within the test itself or the direct fixture
//     class of a `PB_TEST_F` (constructor, default member initializers,
//     `SetUp()`, or helper methods). Fixtures that evaluate
//     `PB_TEST_CHOOSE_VALUE` in their constructor or `SetUp()` must be used
//     with `PB_TEST_F`, not `TEST_F`.
//   - Do NOT mix `PB_TEST_F` or `PB_TEST_CHOOSE_VALUE` with `TEST_P`
//     (`WithParamInterface` / `TestWithParam`) fixtures.
//   - Iterable arguments to `PB_TEST_CHOOSE_VALUE` and arguments to
//     `PB_TEST_CHOOSE_CONSTEXPR` must not capture local variables (use
//     `static constexpr` for local constants referenced by
//     `PB_TEST_CHOOSE_CONSTEXPR`).
//   - `PB_TEST_CHOOSE_VALUE` dimensions are registered statically before tests
//     run, so a `PB_TEST_CHOOSE_VALUE` inside a runtime conditional (`if`) or a
//     fixture helper method still multiplies the registered test cases even on
//     runs or `PB_TEST_F` tests where that branch or helper method is not
//     called. Any registered `PB_TEST_CHOOSE_VALUE` dimension that is not
//     evaluated during a test run fails the test (unless the test exited early
//     via a fatal assertion such as `ASSERT_*`/`FAIL()` or via `GTEST_SKIP()`).
//     By contrast, `PB_TEST_CHOOSE_TYPE` and `PB_TEST_CHOOSE_CONSTEXPR` inside
//     an `if constexpr` branch only expand combinations for the active
//     compile-time branch.
//   - Do NOT evaluate a `PB_TEST_CHOOSE_VALUE` call site more than once in a
//     single test run (e.g., inside a loop or by calling a helper method
//     multiple times); a single lexical call site cannot yield a new value on
//     a second call, so repeated evaluation fails the test.

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/base/thread_annotations.h"
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/absl_check.h"
#include "absl/meta/type_traits.h"
#include "absl/strings/has_absl_stringify.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/optional.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_enum_reflection.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/testing/get_type_name.h"

#define PB_TEST(test_suite_name, test_name) \
  PB_TEST_(test_suite_name, test_name, ::testing::Test)

#define PB_TEST_F(test_suite_name, test_name) \
  PB_TEST_(test_suite_name, test_name, test_suite_name)

#define PB_TEST_CHOOSE_VALUE(...)                                   \
  (::google::protobuf::internal::TestChooseValueSelector<                     \
      std::decay_t<decltype(*this)>,                                \
      static_cast<::google::protobuf::internal::CounterT>(__COUNTER__)>(      \
      [] {                                                          \
        return ::google::protobuf::internal::PartialTestRegistry::SourceInfo{ \
            #__VA_ARGS__, {__FILE__, __LINE__}};                    \
      },                                                            \
      InternalIndicesProbe_(::google::protobuf::internal::ADLImporter{}),     \
      [&] {                                                         \
        return ::google::protobuf::internal::ChooseValueImpl(                 \
            PB_TEST_CHOOSE_VALUE_ARGS_(__VA_ARGS__));               \
      }))

#define PB_TEST_CHOOSE_CONSTEXPR(...)                                        \
  (::google::protobuf::internal::TestChooseConstexprSelector<                          \
      InternalSelf_, static_cast<::google::protobuf::internal::CounterT>(__COUNTER__), \
      InternalIndices_>([] { return #__VA_ARGS__; },                         \
                        [] {                                                 \
                          return ::google::protobuf::internal::ChooseConstexprImpl(    \
                              PB_TEST_CHOOSE_CONSTEXPR_ARGS_(__VA_ARGS__));  \
                        }))

#define PB_TEST_CHOOSE_TYPE(...)                              \
  typename ::google::protobuf::internal::TestChooseTypeSelector<        \
      InternalSelf_,                                          \
      decltype(::google::protobuf::internal::TestChooseTypeParams<      \
               PB_TEST_CHOOSE_TYPE_ARGS_(__VA_ARGS__)>()),    \
      PB_TEST_CHOOSE_TYPE_LITERAL_(__VA_ARGS__),              \
      static_cast<::google::protobuf::internal::CounterT>(__COUNTER__), \
      InternalIndices_>::type

// =============================================================================
// Implementation details follow. Do not use directly.
// =============================================================================

namespace google {
namespace protobuf {
namespace internal {

#if __cplusplus >= 202002L
#define PB_TEST_CHOOSE_TYPE_LITERAL_(...) decltype([] { return #__VA_ARGS__; })
#else
#define PB_TEST_CHOOSE_TYPE_LITERAL_(...) ::google::protobuf::internal::EmptyLiteral
#endif

struct EmptyLiteral {
  constexpr absl::string_view operator()() const { return ""; }
};

enum class CounterT : size_t {};

template <typename T>
std::true_type IsWithParamInterface(const ::testing::WithParamInterface<T>*);
std::false_type IsWithParamInterface(const void*);

template <typename T>
inline constexpr bool kIsValidProtobufTestFixture =
    std::is_base_of_v<::testing::Test, T> &&
    !decltype(IsWithParamInterface(static_cast<const T*>(nullptr)))::value;

#define PB_TEST_STRIP_PB_TEST_EXPANDED_
#define PB_TEST_STRIP_PB_TEST_PARENS_TO_TUPLE_
#define PB_TEST_STRIP_PB_TEST_PARENS_TO_VALUES_
#define PB_TEST_STRIP_PB_TEST_PARENS_TO_CONSTEXPR_
#define PB_TEST_PARENS_TO_TUPLE_(...) \
  PB_TEST_EXPANDED_ ::std::tuple<__VA_ARGS__>
#define PB_TEST_PARENS_TO_VALUES_(...) \
  PB_TEST_EXPANDED_ ::google::protobuf::internal::MakeChooseValueArray(__VA_ARGS__)
#define PB_TEST_PARENS_TO_CONSTEXPR_(...) \
  PB_TEST_EXPANDED_ ::google::protobuf::internal::MakeChooseConstexprTuple(__VA_ARGS__)
#define PB_TEST_STRIP_PREFIX_IMPL_(...) PB_TEST_STRIP_##__VA_ARGS__
#define PB_TEST_STRIP_PREFIX_(...) PB_TEST_STRIP_PREFIX_IMPL_(__VA_ARGS__)
#define PB_TEST_CHOOSE_TYPE_ARGS_(...) \
  PB_TEST_STRIP_PREFIX_(PB_TEST_PARENS_TO_TUPLE_ __VA_ARGS__)
#define PB_TEST_CHOOSE_VALUE_ARGS_(...) \
  PB_TEST_STRIP_PREFIX_(PB_TEST_PARENS_TO_VALUES_ __VA_ARGS__)
#define PB_TEST_CHOOSE_CONSTEXPR_ARGS_(...) \
  PB_TEST_STRIP_PREFIX_(PB_TEST_PARENS_TO_CONSTEXPR_ __VA_ARGS__)

struct ADLImporter {};
template <typename Sig>
struct Signature {
  constexpr Signature(ADLImporter) {}  // NOLINT
};

// This function is called via ADL if we can't see it redeclared inside the
// fixture.
inline Signature<void()> InternalIndicesProbe_(ADLImporter p) { return p; }

#define PB_TEST_IMPL_(test_suite_name, test_name, self_class, parent_class)    \
  static_assert(sizeof(GTEST_STRINGIFY_(test_suite_name)) > 1,                 \
                "test_suite_name must not be empty");                          \
  static_assert(sizeof(GTEST_STRINGIFY_(test_name)) > 1,                       \
                "test_name must not be empty");                                \
  static_assert(::google::protobuf::internal::kIsValidProtobufTestFixture<parent_class>, \
                "PB_TEST fixture must derive from ::testing::Test and "        \
                "cannot be a parameterized (TEST_P) fixture");                 \
  class self_class : public parent_class {                                     \
   public:                                                                     \
    using InternalParent_ = parent_class;                                      \
    using InternalSelf_ = self_class;                                          \
                                                                               \
    template <typename Indices>                                                \
    explicit self_class(Indices*, size_t idx)                                  \
        : test_body_(&self_class::TestBodyImpl<Indices>) {}                    \
    ~self_class() override {                                                   \
      ::google::protobuf::internal::PartialTestRegistry::Get().UnsetCurrentTest();       \
    }                                                                          \
    self_class(const self_class&) = delete;                                    \
    void operator=(const self_class&) = delete; /* NOLINT */                   \
    template <typename Indices>                                                \
    static auto Instantiate_() {                                               \
      return &self_class::TestBodyImpl<Indices>;                               \
    }                                                                          \
                                                                               \
   private:                                                                    \
    void TestBody() override { (this->*test_body_)(); }                        \
    template <typename InternalIndices_,                                       \
              typename InternalIndicesProbe_ =                                 \
                  ::google::protobuf::internal::Signature<InternalIndices_>>             \
    void TestBodyImpl();                                                       \
    static inline bool reg_ [[maybe_unused]] =                                 \
        (::google::protobuf::internal::PartialTestRegistry::Get()                        \
             .SetPrimary<self_class>(#test_suite_name, #test_name, __FILE__,   \
                                     __LINE__),                                \
         false);                                                               \
    void (self_class::*test_body_)();                                          \
  };                                                                           \
                                                                               \
  template <typename InternalIndices_, typename InternalIndicesProbe_>         \
  void self_class::TestBodyImpl()

#define PB_TEST_(test_suite_name, test_name, parent_class)                 \
  PB_TEST_IMPL_(test_suite_name, test_name, test_suite_name##_##test_name, \
                parent_class)

struct PerCounterIndex {
  CounterT counter;
  size_t value;

  friend bool operator<(PerCounterIndex a, PerCounterIndex b) {
    return std::tuple(a.counter, a.value) < std::tuple(b.counter, b.value);
  }
};

template <CounterT Counter, size_t Value>
struct StaticPerCounterIndex {
  static constexpr PerCounterIndex value = {Counter, Value};
  static_assert(Value != 0);
};

template <typename T, size_t N, typename Printer>
struct ChooseValues {
  using value_type = T;
  static constexpr size_t kSize = N;
  std::vector<T> values;
  absl::optional<absl::string_view> name_prefix;
  Printer printer;
};

template <typename Tuple, typename Printer>
struct ChooseConstexpr {
  using tuple_type = Tuple;
  Tuple values;
  absl::optional<absl::string_view> name_prefix;
  Printer printer;
};

struct DefaultConstruct {
  template <typename T, typename C = DefaultConstruct>
  static constexpr T value = C::template value<T>;
};
template <typename T>
inline constexpr T kDefault = DefaultConstruct::value<T>;

template <typename F>
inline std::false_type Run = (kDefault<F>(), std::false_type{});

template <typename T>
struct IsTupleOrPair : std::false_type {};
template <typename... Ts>
struct IsTupleOrPair<std::tuple<Ts...>> : std::true_type {};
template <typename T, typename U>
struct IsTupleOrPair<std::pair<T, U>> : std::true_type {};

struct DefaultValuePrinter {
  static constexpr size_t kMaxPrintedLength = 20;

  template <typename T>
  absl::optional<std::string> operator()(const T& value, size_t i) const {
    absl::optional<std::string> out = Print(value, i);
    if (out.has_value() && out->size() > kMaxPrintedLength) {
      return absl::nullopt;
    }
    return out;
  }

 private:
  template <typename T>
  absl::optional<std::string> Print(const T& value, size_t i) const {
    if constexpr (std::is_same_v<T, bool>) {
      return value ? "true" : "false";
    } else if constexpr (std::is_same_v<T, char>) {
      return Print(static_cast<int>(value), i);
    } else if constexpr (google::protobuf::is_proto_enum<T>::value) {
      auto* e = google::protobuf::GetEnumDescriptor<T>()->FindValueByNumber(value);
      return e != nullptr ? std::string(e->name())
                          : Print(static_cast<int>(value), i);
    } else if constexpr (std::is_arithmetic_v<T>) {
      if constexpr (std::is_signed_v<T>) {
        if (value < 0) {
          if constexpr (std::is_integral_v<T>) {
            using U = std::make_unsigned_t<T>;
            return absl::StrCat(
                "neg_", *Print(static_cast<U>(0 - static_cast<U>(value)), i));
          } else {
            return absl::StrCat("neg_", *Print(-value, i));
          }
        }
      }
      return absl::StrCat(value);
    } else if constexpr (std::is_base_of_v<MessageLite, T>) {
      return absl::nullopt;
    } else if constexpr (absl::HasAbslStringify<T>::value ||
                         std::is_convertible_v<T, absl::string_view>) {
      return absl::StrCat(value);
    } else if constexpr (IsTupleOrPair<T>::value) {
      return std::apply(
          [&](const auto&... elems) -> absl::optional<std::string> {
            std::string res;
            bool ok = true;
            auto append = [&](const auto& elem) {
              if (auto s = Print(elem, i)) {
                if (!res.empty()) absl::StrAppend(&res, "_");
                absl::StrAppend(&res, *s);
              } else {
                ok = false;
              }
            };
            (append(elems), ...);
            return ok ? absl::optional<std::string>(std::move(res))
                      : absl::nullopt;
          },
          value);
    } else {
      return absl::nullopt;
    }
  }
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
    auto& t = pf.per_instantiation[kKey<Indices>];
    t.instantiation_key = IndexAsVector(static_cast<Indices*>(nullptr));
    t.do_register = [this](size_t i) {
      auto& pf = fixtures_[kKey<Fixture>];
      auto& t = pf.per_instantiation[kKey<Indices>];
      const auto names = t.GetNames(pf.test_name, i);

      ::testing::RegisterTest(
          std::string(pf.test_suite_name).c_str(), names.test_name.c_str(),
          names.type_name.empty() ? nullptr : names.type_name.c_str(),
          names.value_name.empty() ? nullptr : names.value_name.c_str(),
          std::string(pf.file).c_str(), pf.line,
          [this, i]() -> typename Fixture::InternalParent_* {
            // set it before we start constructing the instance so that it is
            // available on the fixture already.
            SetCurrentTest<Fixture, Indices>(i);
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
    pf.parent_key = kKey<typename Fixture::InternalParent_>;
    AddDoRegister<Fixture, void()>();
  }

  static void SanitizeNames(
      std::vector<std::string>& names,
      absl::optional<absl::string_view> name_prefix = absl::nullopt,
      bool is_bool = false);
  static void DedupNames(std::vector<std::string>& names);

  // Consumes a comma-separated list of tokens enclosed in `open` and `close`
  // from the beginning of `literal`, respecting nested `()`, `{}`, `[]`, string
  // and character literals, and (when `allow_angle_brackets` is true) `<>`.
  // On success, advances `literal` past `close` and returns the trimmed items.
  static absl::optional<std::vector<std::string>> ConsumeList(
      absl::string_view& literal, char open = '(', char close = ')',
      bool allow_angle_brackets = false);

  // Consumes a parenthesized list of `expected_size` expressions from
  // `literal`. First attempts parsing without treating `<` and `>` as brackets
  // (to handle comparison operators like `a < b`), then retries with angle
  // bracket matching enabled (to handle template arguments with commas like
  // `Foo<int, int>()`).
  static absl::optional<std::vector<std::string>> ConsumeExpressionList(
      absl::string_view& literal, size_t expected_size);

  // Extracts and sanitizes `size` element names from the stringified macro
  // arguments `literal` (e.g. `(e1, e2), "prefix", {"n1", "n2"}`), falling back
  // to 0-based indices if the expression list cannot be parsed into `size`
  // elements.
  static std::vector<std::string> ParseNamesFromLiteral(
      absl::string_view literal, size_t size);

  template <typename Printer, typename Elem>
  static std::string GetValueElementName(
      Printer& printer, const Elem& elem, size_t i, size_t size,
      absl::string_view literal,
      absl::optional<std::vector<std::string>>& literal_names) {
    if (absl::optional<std::string> res = printer(elem, i)) {
      return *std::move(res);
    }
    if (!literal_names.has_value()) {
      absl::string_view copy = literal;
      literal_names = ConsumeExpressionList(copy, size)
                          .value_or(std::vector<std::string>{});
    }
    if (literal_names->size() == size) return (*literal_names)[i];
    if constexpr (std::is_enum_v<Elem>) {
      return *DefaultValuePrinter{}(
          static_cast<std::underlying_type_t<Elem>>(elem), i);
    }
    return absl::StrCat(i);
  }

  struct Location {
    const char* file = "";
    int line = 0;
  };
  struct SourceInfo {
    absl::string_view literal;
    Location loc;
  };

  template <typename Fixture>
  void AddValueDimension(CounterT id, const void* instantiation_key,
                         size_t size, SourceInfo info) {
    ABSL_CHECK_GT(size, 0u)
        << "PB_TEST_CHOOSE_VALUE requires at least one element";
    auto& pf = fixtures_[kKey<Fixture>];
    pf.locations.try_emplace(id, info.loc);
    auto& d = pf.per_instantiation[instantiation_key].dimensions[id];
    if (!d.names.empty()) return;
    d = {ParseNamesFromLiteral(info.literal, size), DimensionKind::kValue};
  }

  template <typename Fixture, typename T, size_t N, typename Printer>
  void AddValueDimension(CounterT id, const void* instantiation_key,
                         ChooseValues<T, N, Printer> values, SourceInfo info) {
    ABSL_CHECK(!values.values.empty())
        << "PB_TEST_CHOOSE_VALUE requires at least one element";
    auto& pf = fixtures_[kKey<Fixture>];
    pf.locations.try_emplace(id, info.loc);
    auto& d = pf.per_instantiation[instantiation_key].dimensions[id];
    if (!d.names.empty()) return;
    d.kind = DimensionKind::kValue;
    absl::optional<std::vector<std::string>> literal_names;
    const size_t size = values.values.size();
    for (size_t i = 0; i < size; ++i) {
      d.names.push_back(
          GetValueElementName<Printer, T>(values.printer, values.values[i], i,
                                          size, info.literal, literal_names));
    }
    SanitizeNames(d.names, values.name_prefix,
                  std::is_same_v<T, bool> &&
                      std::is_same_v<Printer, DefaultValuePrinter>);
  }

  template <typename Fixture, CounterT id, typename Tuple, typename Printer>
  void AddConstexprDimension(const void* instantiation_key,
                             ChooseConstexpr<Tuple, Printer> values,
                             absl::string_view literal) {
    static constexpr size_t kSize = std::tuple_size_v<Tuple>;
    auto& d = fixtures_[kKey<Fixture>]
                  .per_instantiation[instantiation_key]
                  .dimensions[id];
    if (!d.names.empty()) return;
    d.kind = DimensionKind::kConstexprValue;
    absl::optional<std::vector<std::string>> literal_names;
    bool is_bool = false;
    std::apply(
        [&](const auto&... elems) {
          is_bool =
              std::is_same_v<Printer, DefaultValuePrinter> &&
              (std::is_same_v<absl::remove_cvref_t<decltype(elems)>, bool> &&
               ...);
          size_t i = 0;
          ((d.names.push_back(GetValueElementName(
                values.printer, elems, i, kSize, literal, literal_names)),
            ++i),
           ...);
        },
        values.values);
    SanitizeNames(d.names, values.name_prefix, is_bool);
  }

  template <typename Fixture, CounterT id, typename Printer, typename... Types>
  void AddTypeDimension(const void* instantiation_key, std::tuple<Types...>*,
                        absl::optional<absl::string_view> name_prefix,
                        absl::string_view literal) {
    auto& d = fixtures_[kKey<Fixture>]
                  .per_instantiation[instantiation_key]
                  .dimensions[id];
    if (!d.names.empty()) return;
    d.kind = DimensionKind::kType;
    absl::optional<std::vector<std::string>> literal_names;
    const auto get_name = [&](absl::optional<std::string> name, size_t i) {
      if (name.has_value()) return *std::move(name);
      if (!literal_names.has_value()) {
        literal_names =
            ConsumeList(literal, '(', ')', /*allow_angle_brackets=*/true)
                .value_or(std::vector<std::string>{});
      }
      if (literal_names->size() == sizeof...(Types)) {
        return (*literal_names)[i];
      }
      return absl::StrCat(i);
    };
    size_t i = 0;
    ((d.names.push_back(get_name(Printer::template Print<Types>(i), i)), ++i),
     ...);
    SanitizeNames(d.names, name_prefix);
  }

  size_t GetValueIndex(CounterT id, const void*, Location loc);

  void RegisterAll();

  template <typename Fixture, typename Indices>
  void SetCurrentTest(size_t index) {
    auto& pf = fixtures_[kKey<Fixture>];
    current_fixture_ = &pf;
    current_test_ = &pf.per_instantiation[kKey<Indices>];
    current_test_index_ = index;
  }

  void UnsetCurrentTest();

 private:
  PartialTestRegistry() = default;

  template <typename... I>
  static std::vector<PerCounterIndex> IndexAsVector(void (*)(I...)) {
    return {I::value...};
  }

  enum class DimensionKind { kValue, kConstexprValue, kType };
  struct Dimension {
    std::vector<std::string> names;
    DimensionKind kind = DimensionKind::kValue;
  };

  struct PerFixture {
    absl::string_view test_suite_name;
    absl::string_view test_name;
    absl::string_view file;
    int line = 0;
    const void* parent_key = nullptr;
    absl::flat_hash_map<CounterT, Location> locations;

    struct PerInstantiation {
      absl::AnyInvocable<void(size_t index) const&> do_register;
      struct TestNames {
        std::string test_name;
        std::string type_name;
        std::string value_name;
      };
      TestNames GetNames(absl::string_view base_test_name, size_t i) const;

      std::vector<PerCounterIndex> instantiation_key;

      // We want these sorted.
      absl::btree_map<CounterT, Dimension> dimensions;
    };
    absl::flat_hash_map<const void*, PerInstantiation> per_instantiation;
  };
  // We need pointer stability.
  absl::flat_hash_map<const void*, PerFixture> fixtures_;
  const PerFixture* current_fixture_ = nullptr;
  const PerFixture::PerInstantiation* current_test_ = nullptr;
  size_t current_test_index_ = ~size_t{};
  absl::Mutex used_values_mu_;
  absl::flat_hash_set<CounterT> used_values_ ABSL_GUARDED_BY(used_values_mu_);
};

template <size_t N>
struct ExplicitValuePrinter {
  std::array<absl::string_view, N> names;
  template <typename T>
  absl::optional<std::string> operator()(const T&, size_t n) const {
    return std::string(names[n]);
  }
};

template <size_t N, size_t... Is>
constexpr ExplicitValuePrinter<N> MakeExplicitValuePrinter(
    const char* const (&names)[N], std::index_sequence<Is...>) {
  return {{names[Is]...}};
}

template <typename T, size_t N>
struct ChooseValueArray {
  std::vector<T> values;
};

template <typename First, typename... Ts>
ChooseValueArray<
    std::common_type_t<std::decay_t<const First&>, std::decay_t<const Ts&>...>,
    1 + sizeof...(Ts)>
MakeChooseValueArray(const First& v, const Ts&... values) {
  return {{v, values...}};
}

template <typename C, typename = void>
struct ChooseValueTraitsImpl;

template <typename T, size_t N>
struct ChooseValueTraitsImpl<ChooseValueArray<T, N>> {
  using value_type = T;
  static constexpr size_t kSize = N;
  static std::vector<T> ToVector(ChooseValueArray<T, N> v) {
    return std::move(v.values);
  }
};

template <typename C>
struct ChooseValueTraitsImpl<
    C, std::void_t<decltype(std::begin(std::declval<const C&>()),
                            std::end(std::declval<const C&>()))>> {
  using value_type =
      std::decay_t<decltype(*std::begin(std::declval<const C&>()))>;
  static constexpr size_t kSize = ~size_t{};
  static std::vector<value_type> ToVector(const C& v) {
    std::vector<value_type> out;
    for (const value_type& elem : v) {
      out.push_back(elem);
    }
    return out;
  }
};

template <typename C>
using ChooseValueTraits = ChooseValueTraitsImpl<absl::remove_cvref_t<C>>;

template <
    typename C, typename Char, size_t M, typename Printer = DefaultValuePrinter,
    typename Traits = ChooseValueTraits<C>,
    typename T = typename Traits::value_type,
    typename = std::enable_if_t<std::is_same_v<Char, char> &&
                                std::is_invocable_v<Printer, const T, size_t>>>
auto ChooseValueImpl(C&& values, const Char (&name_prefix)[M],
                     Printer printer = Printer{}) {
  return ChooseValues<T, Traits::kSize, Printer>{
      Traits::ToVector(std::forward<C>(values)), name_prefix,
      std::move(printer)};
}

template <
    typename C, typename Printer = DefaultValuePrinter,
    typename Traits = ChooseValueTraits<C>,
    typename T = typename Traits::value_type,
    typename = std::enable_if_t<std::is_invocable_v<Printer, const T, size_t>>>
auto ChooseValueImpl(C&& values, Printer printer = Printer{}) {
  return ChooseValues<T, Traits::kSize, Printer>{
      Traits::ToVector(std::forward<C>(values)), absl::nullopt,
      std::move(printer)};
}

template <typename C, size_t N, typename Char, size_t M,
          typename Traits = ChooseValueTraits<C>,
          typename T = typename Traits::value_type,
          typename = std::enable_if_t<std::is_same_v<Char, char> &&
                                      N == Traits::kSize>>
auto ChooseValueImpl(C&& values, const Char (&name_prefix)[M],
                     const char* const (&names)[N]) {
  return ChooseValues<T, N, ExplicitValuePrinter<N>>{
      Traits::ToVector(std::forward<C>(values)), name_prefix,
      MakeExplicitValuePrinter(names, std::make_index_sequence<N>{})};
}

template <typename C, size_t N, typename Traits = ChooseValueTraits<C>,
          typename T = typename Traits::value_type,
          typename = std::enable_if_t<N == Traits::kSize>>
auto ChooseValueImpl(C&& values, const char* const (&names)[N]) {
  return ChooseValues<T, N, ExplicitValuePrinter<N>>{
      Traits::ToVector(std::forward<C>(values)), absl::nullopt,
      MakeExplicitValuePrinter(names, std::make_index_sequence<N>{})};
}

template <typename... Ts>
constexpr std::tuple<Ts...> MakeChooseConstexprTuple(Ts... values) {
  return std::tuple<Ts...>(std::move(values)...);
}

template <typename C>
struct ChooseConstexprTraitsImpl {
  using tuple_type = C;
  static constexpr size_t kSize = std::tuple_size_v<C>;
  static constexpr C ToTuple(C v) { return v; }
};

template <typename T, T... Is>
struct ChooseConstexprTraitsImpl<std::integer_sequence<T, Is...>> {
  using tuple_type = std::tuple<decltype(Is)...>;
  static constexpr size_t kSize = sizeof...(Is);
  static constexpr tuple_type ToTuple(std::integer_sequence<T, Is...>) {
    return tuple_type{Is...};
  }
};

template <typename C>
using ChooseConstexprTraits =
    ChooseConstexprTraitsImpl<absl::remove_cvref_t<C>>;

template <typename Printer, typename Tuple,
          typename = std::make_index_sequence<std::tuple_size_v<Tuple>>>
struct IsConstexprPrinter : std::false_type {};

template <typename Printer, typename Tuple, size_t... Is>
struct IsConstexprPrinter<Printer, Tuple, std::index_sequence<Is...>>
    : std::bool_constant<(
          std::is_invocable_v<Printer, const std::tuple_element_t<Is, Tuple>&,
                              size_t> &&
          ...)> {};

template <
    typename C, typename Char, size_t M, typename Printer = DefaultValuePrinter,
    typename Traits = ChooseConstexprTraits<C>,
    typename Tuple = typename Traits::tuple_type,
    typename = std::enable_if_t<std::is_same_v<Char, char> &&
                                IsConstexprPrinter<Printer, Tuple>::value>>
constexpr auto ChooseConstexprImpl(C values, const Char (&name_prefix)[M],
                                   Printer printer = Printer{}) {
  return ChooseConstexpr<Tuple, Printer>{Traits::ToTuple(std::move(values)),
                                         name_prefix, printer};
}

template <
    typename C, typename Printer = DefaultValuePrinter,
    typename Traits = ChooseConstexprTraits<C>,
    typename Tuple = typename Traits::tuple_type,
    typename = std::enable_if_t<IsConstexprPrinter<Printer, Tuple>::value>>
constexpr auto ChooseConstexprImpl(C values, Printer printer = Printer{}) {
  return ChooseConstexpr<Tuple, Printer>{Traits::ToTuple(std::move(values)),
                                         absl::nullopt, printer};
}

template <typename C, size_t N, typename Char, size_t M,
          typename Traits = ChooseConstexprTraits<C>,
          typename Tuple = typename Traits::tuple_type,
          typename = std::enable_if_t<std::is_same_v<Char, char> &&
                                      N == Traits::kSize>>
constexpr auto ChooseConstexprImpl(C values, const Char (&name_prefix)[M],
                                   const char* const (&names)[N]) {
  return ChooseConstexpr<Tuple, ExplicitValuePrinter<N>>{
      Traits::ToTuple(std::move(values)), name_prefix,
      MakeExplicitValuePrinter(names, std::make_index_sequence<N>{})};
}

template <typename C, size_t N, typename Traits = ChooseConstexprTraits<C>,
          typename Tuple = typename Traits::tuple_type,
          typename = std::enable_if_t<N == Traits::kSize>>
constexpr auto ChooseConstexprImpl(C values, const char* const (&names)[N]) {
  return ChooseConstexpr<Tuple, ExplicitValuePrinter<N>>{
      Traits::ToTuple(std::move(values)), absl::nullopt,
      MakeExplicitValuePrinter(names, std::make_index_sequence<N>{})};
}

template <typename Fixture, CounterT id, typename Indices, typename Literal,
          typename F>
auto TestChooseValueSelector(Literal, Signature<Indices>, F f) {
  static_assert(!decltype(IsWithParamInterface(
                    static_cast<const Fixture*>(nullptr)))::value,
                "PB_TEST_CHOOSE_VALUE cannot be used inside a TEST_P fixture "
                "or test");
  using ChooseValueT = std::invoke_result_t<F>;
  if constexpr (std::is_empty_v<F>) {
    // If the lambda is an empty struct then we can run it eagerly during
    // registration.
    static constexpr auto reg = [] {
      PartialTestRegistry::Get().AddValueDimension<Fixture>(
          id, PartialTestRegistry::kKey<Indices>, kDefault<F>(),
          kDefault<Literal>());
    };
    static const auto* const values = new auto(f().values);
    return (*values)[PartialTestRegistry::Get().GetValueIndex(
        id, &Run<decltype(reg)>, kDefault<Literal>().loc)];
  } else {
    static_assert(ChooseValueT::kSize != ~size_t{},
                  "Iterable in PB_TEST_CHOOSE_VALUE cannot capture local "
                  "variables");
    // Otherwise, we can't use the values for registration. We can still
    // register from the textual input.
    static constexpr auto reg = [] {
      PartialTestRegistry::Get().AddValueDimension<Fixture>(
          id, PartialTestRegistry::kKey<Indices>, ChooseValueT::kSize,
          kDefault<Literal>());
    };
    return f().values[PartialTestRegistry::Get().GetValueIndex(
        id, &Run<decltype(reg)>, kDefault<Literal>().loc)];
  }
}

template <CounterT id, typename... Indices>
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
  static absl::optional<std::string> Print(size_t) {
    return GetTypeName<T>();
  }
};

template <typename Tuple, typename Printer = DefaultTypePrinter>
ChooseTypes<Tuple, Printer, 0> TestChooseTypeParams();

#if __cplusplus >= 202002L
struct FixedString {
  template <size_t N>
  constexpr FixedString(  // NOLINT(google-explicit-constructor)
      const char (&n)[N])
      : buf{} {
    static_assert(N <= sizeof(buf), "String is too long");
    std::copy(n, n + N, buf);
  }
  template <typename T>
  constexpr FixedString(std::initializer_list<T>) = delete;
  char buf[64];
};

template <size_t ExpectedSize>
struct FixedStringList {
  FixedString list[ExpectedSize];
};

template <auto names>
struct ExplicitTypePrinter {
  template <typename T>
  static std::string Print(size_t index) {
    return names.list[index].buf;
  }
};

template <typename Tuple,
          std::enable_if_t<!std::is_void_v<Tuple>,
                           FixedStringList<std::tuple_size_v<Tuple>>>
              names>
ChooseTypes<Tuple, ExplicitTypePrinter<names>, 0> TestChooseTypeParams();

template <typename Tuple,
          std::enable_if_t<!std::is_void_v<Tuple>, FixedString> prefix,
          typename Printer = DefaultTypePrinter>
ChooseTypes<Tuple, Printer, prefix> TestChooseTypeParams();

template <typename Tuple,
          std::enable_if_t<!std::is_void_v<Tuple>, FixedString> prefix,
          FixedStringList<std::tuple_size_v<Tuple>> names>
ChooseTypes<Tuple, ExplicitTypePrinter<names>, prefix> TestChooseTypeParams();
#endif  // __cplusplus >= 202002L

template <typename T, auto*>
using WithRegistration = T;

template <typename... Indices>
constexpr absl::optional<CounterT> LargestCounter(void (*)(Indices...)) {
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

template <typename Fixture, CounterT id, size_t i, size_t size,
          typename Indices>
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

template <typename Fixture, CounterT id, typename Indices, typename Literal,
          typename F>
constexpr auto TestChooseConstexprSelector(Literal, F f) {
  using ChooseT = std::invoke_result_t<F>;
  using Tuple = typename ChooseT::tuple_type;
  constexpr size_t kSize = std::tuple_size_v<Tuple>;
  static_assert(kSize > 0,
                "PB_TEST_CHOOSE_CONSTEXPR requires at least one element");
  constexpr auto reg = []() {
    PartialTestRegistry::Get().AddConstexprDimension<Fixture, id>(
        PartialTestRegistry::kKey<Indices>, kDefault<F>(), kDefault<Literal>());
    InstantiateAllTypes<Fixture, id, 0, std::tuple_size_v<Tuple>, Indices>();
    PartialTestRegistry::Get().AddDoRegister<Fixture, Indices>();
  };
  (void)&Run<decltype(reg)>;
  constexpr size_t kIndex =
      GetTypeIndexForId<id>(static_cast<Indices*>(nullptr));
  return std::get<kIndex>(f().values);
}

template <typename Fixture, typename Types, typename Literal, CounterT id,
          typename Indices>
struct TestChooseTypeSelector {
  static constexpr auto reg = []() {
    PartialTestRegistry::Get()
        .AddTypeDimension<Fixture, id, typename Types::printer>(
            PartialTestRegistry::kKey<Indices>,
            static_cast<typename Types::types*>(nullptr), Types::prefix(),
            kDefault<Literal>());
    InstantiateAllTypes<Fixture, id, 0,
                        std::tuple_size_v<typename Types::types>, Indices>();
    PartialTestRegistry::Get().AddDoRegister<Fixture, Indices>();
  };
  using type =
      WithRegistration<std::tuple_element_t<GetTypeIndexForId<id>(
                                                static_cast<Indices*>(nullptr)),
                                            typename Types::types>,
                       &Run<decltype(reg)>>;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_H__
