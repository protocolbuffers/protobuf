# Inline Parameterized Testing (`protobuf_test.h`)

<!--*
# Document freshness: For more information, see go/fresh-source.
freshness: { owner: 'sbenza' reviewed: '2026-10-14' }
*-->

[third_party/protobuf/testing/protobuf_test.h](http://google3/third_party/protobuf/testing/protobuf_test.h)
provides inline value-, `constexpr`-, and type-parameterized test macros built
on top of GoogleTest (`PB_TEST`, `PB_TEST_F`, `PB_TEST_CHOOSE_VALUE`,
`PB_TEST_CHOOSE_CONSTEXPR`, and `PB_TEST_CHOOSE_TYPE`).

Instead of splitting a parameterized test across a fixture definition, a
`TEST_P` or `TYPED_TEST` body, and a separate `INSTANTIATE_TEST_SUITE_P` or
`TYPED_TEST_SUITE` macro call, parameters are declared directly inline where
they are used. When multiple `PB_TEST_CHOOSE_*` calls appear in a test, a
separate GoogleTest case is automatically registered and run for every
combination in their Cartesian product.

[TOC]

## Why `PB_TEST`?

GoogleTest's built-in parameterization (`TEST_P` and `TYPED_TEST`) has several
ergonomic friction points:

*   **Fragmented context**: A `TEST_P` test requires defining a `TestWithParam`
    fixture, unpacking `GetParam()` inside `TEST_P`, and listing the parameter
    values in `INSTANTIATE_TEST_SUITE_P` (often far away at the bottom of the
    file).
*   **Boilerplate for Cartesian products**: Combining multiple parameters in
    `TEST_P` requires `testing::Combine(...)`, `std::tuple<...>` fixtures,
    `std::get<I>(GetParam())` or structured bindings, and custom name-generator
    lambdas so test names aren't just `/0`, `/1`, `/2`.
*   **No mixing of types and values**: GoogleTest cannot combine `TYPED_TEST`
    and `TEST_P` in the same test, forcing authors to fall back to runtime `for`
    loops inside `TYPED_TEST` or preprocessor macros that generate test suites.
*   **Hidden failures in loops**: Replacing parameterization with a `for` loop
    inside `TEST` hides which iteration failed unless every assertion is
    annotated with `SCOPED_TRACE`, runs subsequent iterations after a non-fatal
    failure, and prevents running a single case via `--gtest_filter`.

`PB_TEST` and `PB_TEST_F` keep the Arrange-Act-Assert flow local to the test
body while still registering each parameter combination as a first-class,
individually filterable GoogleTest case with human-readable names.

--------------------------------------------------------------------------------

## API Synopsis

Include the header and depend on `//third_party/protobuf/testing:protobuf_test`:

```c++
#include "third_party/protobuf/testing/protobuf_test.h"
```

### Test Definition Macros

*   `PB_TEST(TestSuiteName, TestName)` Defines a standalone test (analogous to
    `TEST`) that supports `PB_TEST_CHOOSE_VALUE`, `PB_TEST_CHOOSE_CONSTEXPR`,
    and `PB_TEST_CHOOSE_TYPE` in its body. Can also be used with no
    `PB_TEST_CHOOSE_*` calls as a drop-in replacement for `TEST`.
*   `PB_TEST_F(TestFixtureName, TestName)` Defines a test using fixture class
    `TestFixtureName` (derived from `::testing::Test`, analogous to `TEST_F`).
    Supports all `PB_TEST_CHOOSE_*` macros in the test body, as well as
    `PB_TEST_CHOOSE_VALUE` inside `TestFixtureName`'s default member
    initializers, constructor, `SetUp()`, or helper methods.

### Parameter Selection Macros

#### `PB_TEST_CHOOSE_VALUE`: Runtime Value Dimensions

Evaluates to `T` (by value) for one of the provided runtime values. Values can
be specified either as a **parenthesized list** `(v1, v2, ...)` (deducing common
type `std::common_type_t<...>`) or as an **iterable range** (a container,
C-array, `testing::Bool()`, `testing::Range(...)`, `testing::ValuesIn(...)`,
etc.):

```c++
// Parenthesized list of values:
T val = PB_TEST_CHOOSE_VALUE((v1, v2, ...));
T val = PB_TEST_CHOOSE_VALUE((v1, v2, ...), "prefix");
T val = PB_TEST_CHOOSE_VALUE((v1, v2, ...), {"name1", "name2", ...});
T val = PB_TEST_CHOOSE_VALUE((v1, v2, ...), "prefix", {"name1", "name2", ...});
T val = PB_TEST_CHOOSE_VALUE((v1, v2, ...), printer);
T val = PB_TEST_CHOOSE_VALUE((v1, v2, ...), "prefix", printer);

// Iterable range (container, C-array, testing::Bool(), testing::Range(), ...):
T val = PB_TEST_CHOOSE_VALUE(iterable);
T val = PB_TEST_CHOOSE_VALUE(iterable, "prefix");
T val = PB_TEST_CHOOSE_VALUE(iterable, printer);
T val = PB_TEST_CHOOSE_VALUE(iterable, "prefix", printer);
```

*   `printer` is a callable with signature `std::string(const T& value, size_t
    index)`.
*   Value expressions in `(v1, v2, ...)` may depend on types selected by
    `PB_TEST_CHOOSE_TYPE` or constants selected by `PB_TEST_CHOOSE_CONSTEXPR` in
    the same test, and may capture local variables or fixture members (`this`).

#### `PB_TEST_CHOOSE_CONSTEXPR`: Compile-Time and Heterogeneous Value Dimensions

Compile-time and/or heterogeneous counterpart to `PB_TEST_CHOOSE_VALUE`.
Evaluates to the selected element as a compile-time constant expression when the
inputs are `constexpr`. Elements in `(v1, v2, ...)` or a tuple may have
**distinct types** (e.g., different lambdas, member pointers, or static
reflection descriptors):

```c++
// Parenthesized list (homogeneous or heterogeneous):
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...));
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), "prefix");
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), {"name1", "name2", ...});
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), "prefix", {"name1", "name2", ...});
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), printer);
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR((v1, v2, ...), "prefix", printer);

// Tuple-like object (std::tuple, std::pair, std::array) or std::integer_sequence:
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq);
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, "prefix");
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, {"name1", "name2", ...});
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, "prefix", {"name1", "name2", ...});
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, printer);
constexpr auto kVal = PB_TEST_CHOOSE_CONSTEXPR(tuple_or_seq, "prefix", printer);
```

*   For heterogeneous elements, pass a generic lambda `[](const auto& value,
    size_t index) -> std::string` as `printer`.

#### `PB_TEST_CHOOSE_TYPE`: Type Dimensions

Selects one type from a parenthesized list `(T1, T2, ...)` or a
`std::tuple<...>` type alias:

```c++
// Parenthesized list of types:
using T = PB_TEST_CHOOSE_TYPE((T1, T2, ...));
using T = PB_TEST_CHOOSE_TYPE((T1, T2, ...), "prefix");
using T = PB_TEST_CHOOSE_TYPE((T1, T2, ...), {"name1", "name2", ...});
using T = PB_TEST_CHOOSE_TYPE((T1, T2, ...), "prefix", {"name1", "name2", ...});
using T = PB_TEST_CHOOSE_TYPE((T1, T2, ...), Printer);
using T = PB_TEST_CHOOSE_TYPE((T1, T2, ...), "prefix", Printer);

// std::tuple<T1, T2, ...> type alias:
using T = PB_TEST_CHOOSE_TYPE(TupleType);
using T = PB_TEST_CHOOSE_TYPE(TupleType, "prefix");
using T = PB_TEST_CHOOSE_TYPE(TupleType, {"name1", "name2", ...});
using T = PB_TEST_CHOOSE_TYPE(TupleType, "prefix", {"name1", "name2", ...});
using T = PB_TEST_CHOOSE_TYPE(TupleType, Printer);
using T = PB_TEST_CHOOSE_TYPE(TupleType, "prefix", Printer);
```

*   `Printer` is a struct/class providing `template <typename T> static
    std::string Print(size_t index)`.

### Test Naming Rules

Registered GoogleTest names have the form:

```none
TestSuiteName.TestName/<p1>/<p2>/...
```

ordered by the lexical order of the `PB_TEST_CHOOSE_*` calls (if a test has no
`PB_TEST_CHOOSE_*` calls, the `/...` suffix is omitted):

1.  **Values (`PB_TEST_CHOOSE_VALUE` / `PB_TEST_CHOOSE_CONSTEXPR`)**:
    *   Uses explicit names `{"name1", ...}` or `printer(value, index)` if
        provided.
    *   Otherwise, if the value expression does not capture local state, formats
        the value directly (supporting booleans, integers, floats, `char`,
        strings, generated Protobuf enums, and non-message `AbslStringify`
        types), provided the formatted string does not exceed 20 characters
        (`DefaultValuePrinter::kMaxPrintedLength`).
    *   For other types (such as Protobuf messages, lambdas, or structs), values
        whose formatted name exceeds 20 characters, or expressions that capture
        local state, extracts the name from the source expression text,
        stripping common qualification/member prefixes (e.g., `(Foo::kBar,
        Foo::kBaz)` becomes `kBar` and `kBaz`; `(msg.a(), msg.b())` becomes `a`
        and `b`).
    *   Falls back to the underlying integer value for non-proto enums, or
        0-based indices (`0`, `1`, ...).
2.  **Types (`PB_TEST_CHOOSE_TYPE`)**:
    *   Uses explicit names `{"name1", ...}` or `Printer::Print<T>(index)` if
        provided.
    *   Otherwise uses `GetTypeName<T>()`, falling back to the literal type list
        tokens, and strips redundant outer namespace qualifications when short
        type names are unambiguous.
3.  **Prefix, Sanitization, and Disambiguation**:
    *   If `"prefix"` is specified, prepends `<prefix>_` to each element name
        (except for `bool` dimensions using the default printer, which format
        `true` as `<prefix>` and `false` as `No<prefix>` or `no_<prefix>`
        matching the prefix's casing, e.g., `"Arena"` becomes `Arena` and
        `NoArena`, and `"arena"` becomes `arena` and `no_arena`).
    *   Non-alphanumeric characters are sanitized to `_` (with negative numbers
        formatted as `neg_<N>`, e.g., `-1` becomes `neg_1`, and empty strings as
        `_`).
    *   Any duplicate names within a dimension are automatically disambiguated
        by appending `_<index>` (e.g., `_2`, `_3`).

--------------------------------------------------------------------------------

## Before & After Examples

### 1. Replacing `TEST_P` + `testing::Combine` Boilerplate

With `TEST_P`, testing a Cartesian product of parameters requires defining a
`TestWithParam<std::tuple<...>>` fixture, unpacking `GetParam()`, and writing an
`INSTANTIATE_TEST_SUITE_P` block with a custom name generator:

```c++ {.bad}
class MicroStringArenaTest
    : public ::testing::TestWithParam<std::tuple<size_t, bool>> {};

TEST_P(MicroStringArenaTest, CopyTakesPayloadFromDonor) {
  const auto [inline_cap, use_arena] = GetParam();
  Arena arena;
  Arena* target_arena = use_arena ? &arena : nullptr;
  ...
}

INSTANTIATE_TEST_SUITE_P(
    MicroStringArenaSuite, MicroStringArenaTest,
    testing::Combine(testing::Values(0, 8, 16, 32), testing::Bool()),
    [](const testing::TestParamInfo<MicroStringArenaTest::ParamType>& info) {
      return absl::StrCat("cap_", std::get<0>(info.param), "_arena_",
                          std::get<1>(info.param) ? "true" : "false");
    });
```

With `PB_TEST`, the parameters are declared inline right where they are used,
and the test cases (such as
`MicroStringTest.CopyTakesPayloadFromDonor/cap_8/arena_true`) are named
automatically:

```c++ {.good}
PB_TEST(MicroStringTest, CopyTakesPayloadFromDonor) {
  size_t inline_cap = PB_TEST_CHOOSE_VALUE((0, 8, 16, 32), "cap");
  Arena arena;
  Arena* target_arena =
      PB_TEST_CHOOSE_VALUE((nullptr, &arena), {"no_arena", "arena"});
  ...
}
```

### 2. Sharing Fixture Parameters Across `PB_TEST_F` Tests

When every test in a fixture should run across a common parameter dimension
(such as with and without an `Arena`), `TEST_P` forces you to inherit from
`TestWithParam` and remember to add `INSTANTIATE_TEST_SUITE_P` at the end of the
file—and prevents individual tests on that fixture from easily adding their own
extra parameter dimensions:

```c++ {.bad}
class LazyFieldTest : public ::testing::TestWithParam<bool> {
 protected:
  Arena arena_;
  Arena* GetArena() { return GetParam() ? &arena_ : nullptr; }
};

TEST_P(LazyFieldTest, ParseAndSerialize) {
  for (bool eager : {false, true}) {
    SCOPED_TRACE(eager);
    ...
  }
}

INSTANTIATE_TEST_SUITE_P(All, LazyFieldTest, testing::Bool());
```

With `PB_TEST_F`, the fixture can call `PB_TEST_CHOOSE_VALUE` directly in a
default member initializer, constructor, or `SetUp()`, and individual
`PB_TEST_F` tests can freely add their own inline dimensions:

```c++ {.good}
class LazyFieldTest : public ::testing::Test {
 protected:
  Arena arena_;
  bool use_arena_ = PB_TEST_CHOOSE_VALUE(testing::Bool(), "arena");
  Arena* GetArena() { return use_arena_ ? &arena_ : nullptr; }
};

PB_TEST_F(LazyFieldTest, ParseAndSerialize) {
  bool eager = PB_TEST_CHOOSE_VALUE(testing::Bool(), "eager");
  ...
}
```

### 3. Replacing `TYPED_TEST` and Mixing Types with Values

GoogleTest's `TYPED_TEST` requires defining a fixture template, a
`testing::Types` alias, and `TYPED_TEST_SUITE`, and requires accessing fixture
members via `this->`. Worse, `TYPED_TEST` cannot be combined with `TEST_P`, so
value variations have to be written as runtime loops inside the test body:

```c++ {.bad}
template <typename T>
class MapFieldTest : public ::testing::Test {};

using MapTestTypes = ::testing::Types<int32_t, int64_t, uint32_t, double>;
TYPED_TEST_SUITE(MapFieldTest, MapTestTypes);

TYPED_TEST(MapFieldTest, InsertAndLookup) {
  for (TypeParam key : {TypeParam{0}, TypeParam{1}, TypeParam{42}}) {
    SCOPED_TRACE(key);
    Map<TypeParam, TypeParam> map;
    map[key] = key;
    EXPECT_EQ(map.at(key), key);
  }
}
```

With `PB_TEST`, `PB_TEST_CHOOSE_TYPE` and `PB_TEST_CHOOSE_VALUE` compose
naturally in the same test—even when the value expressions depend on the chosen
type `T`:

```c++ {.good}
PB_TEST(MapFieldTest, InsertAndLookup) {
  using T = PB_TEST_CHOOSE_TYPE((int32_t, int64_t, uint32_t, double));
  T key = PB_TEST_CHOOSE_VALUE((T{0}, T{1}, T{42}), "key");

  Map<T, T> map;
  map[key] = key;
  EXPECT_EQ(map.at(key), key);
}
```

### 4. Replacing Runtime Loops and `std::apply` Over Compile-Time Constants

When testing compile-time APIs (such as static reflection descriptors, template
non-type parameters, or heterogeneous setter/getter pairs), tests often resort
to repeating blocks or wrapping a lambda in `std::apply` over a `std::tuple`,
which runs everything inside a single GoogleTest case:

```c++ {.bad}
TEST(HasBitsTest, BasicStrandedHasBitCheck) {
  auto check = [](auto field_info) {
    TestAllTypes msg;
    EXPECT_FALSE(msg.has_bit(field_info));
    ...
  };
  check(FieldInfo<TestAllTypes, "optional_int32">());
  check(FieldInfo<TestAllTypes, "optional_int64">());
  check(FieldInfo<TestAllTypes, "optional_bool">());
}
```

With `PB_TEST_CHOOSE_CONSTEXPR`, heterogeneous compile-time constants are
selected inline, and each element is registered as its own GoogleTest case:

```c++ {.good}
PB_TEST(HasBitsTest, BasicStrandedHasBitCheck) {
  constexpr auto field_info = PB_TEST_CHOOSE_CONSTEXPR(
      (FieldInfo<TestAllTypes, "optional_int32">(),
       FieldInfo<TestAllTypes, "optional_int64">(),
       FieldInfo<TestAllTypes, "optional_bool">()),
      [](const auto& info, size_t) { return std::string(info.name()); });

  TestAllTypes msg;
  EXPECT_FALSE(msg.has_bit(field_info));
  ...
}
```

### 5. Expressing Related Input/Output Pairs Cleanly

Instead of creating a single top-level `TestCase` struct with `TEST_P`, you can
either select a lightweight local struct/pair with `PB_TEST_CHOOSE_VALUE` or
combine orthogonal dimensions directly:

```c++ {.good}
PB_TEST(WireFormatTest, ZigZagEncodeDecode) {
  struct Case {
    int32_t decoded;
    uint32_t encoded;
  };
  auto [decoded, encoded] = PB_TEST_CHOOSE_VALUE(
      (Case{0, 0u}, Case{-1, 1u}, Case{1, 2u}, Case{-2, 3u}),
      [](const Case& c, size_t) { return absl::StrCat(c.decoded); });

  EXPECT_EQ(WireFormatLite::ZigZagEncode32(decoded), encoded);
  EXPECT_EQ(WireFormatLite::ZigZagDecode32(encoded), decoded);
}
```

--------------------------------------------------------------------------------

## Limitations and Pitfalls

### 1. Do Not Evaluate `PB_TEST_CHOOSE_VALUE` More Than Once Per Test Run

Each lexical `PB_TEST_CHOOSE_VALUE` call site represents a single parameter
dimension and selects **one value per test run**. Evaluating the same lexical
`PB_TEST_CHOOSE_VALUE` call site a second time during a single test run (for
example, inside a `for` loop or a helper function called multiple times) will
not yield a new value and **fails the test** to prevent accidental misuse.

```c++ {.bad}
PB_TEST(MySuite, MisusedInLoop) {
  for (int i = 0; i < 3; ++i) {
    // ERROR: Evaluating the same PB_TEST_CHOOSE_VALUE twice in a single test
    // run fails the test!
    int x = PB_TEST_CHOOSE_VALUE((10, 20, 30));
    ...
  }
}
```

```c++ {.good}
PB_TEST(MySuite, SeparateTestRuns) {
  int x = PB_TEST_CHOOSE_VALUE((10, 20, 30));
  for (int i = 0; i < 3; ++i) {
    ...
  }
}
```

### 2. Static Registration vs. Runtime Conditionals and Fixture Helpers

GoogleTest requires all test cases to be registered before `RUN_ALL_TESTS()`
begins. Consequently, `PB_TEST_CHOOSE_VALUE` dimensions are discovered and
registered statically.

*   **Runtime `if` branches**: A `PB_TEST_CHOOSE_VALUE` inside an `if (...)`
    block still multiplies the number of registered test cases even when the
    condition is `false` at runtime.
*   **Fixture helper methods**: A `PB_TEST_CHOOSE_VALUE` inside a fixture helper
    method registers that dimension on the fixture class, multiplying the test
    cases for **every** `PB_TEST_F` using that fixture—even tests that never
    call the helper method.

To prevent silent dimension explosion, any registered `PB_TEST_CHOOSE_VALUE`
dimension that is not evaluated during a test run **fails the test** (unless the
test exited early via a fatal assertion such as `ASSERT_*`/`FAIL()` or via
`GTEST_SKIP()`).

```c++ {.bad}
class MyFixture : public ::testing::Test {
 protected:
  // Called by only one PB_TEST_F, but multiplies test cases for ALL PB_TEST_F
  // tests on MyFixture!
  std::string GetSampleString() {
    return PB_TEST_CHOOSE_VALUE(("short", "a_much_longer_string_on_the_heap"));
  }
};
```

Instead, either place the `PB_TEST_CHOOSE_VALUE` inside the specific test(s)
that need it, or keep the shared non-parameterized setup in `MyFixture` and move
the `PB_TEST_CHOOSE_VALUE` into a dedicated sub-fixture used directly by the
`PB_TEST_F` tests that exercise that dimension:

```c++ {.good}
class MyFixture : public ::testing::Test {
 protected:
  // Shared non-parameterized fixture state/helpers (no PB_TEST_CHOOSE_VALUE).
};

class MyStringFixture : public MyFixture {
 protected:
  absl::string_view sample_ =
      PB_TEST_CHOOSE_VALUE(("short", "a_much_longer_string_on_the_heap"));
};

PB_TEST_F(MyStringFixture, HandlesStrings) {
  ...
}
```

> NOTE: Unlike runtime `if` statements with `PB_TEST_CHOOSE_VALUE`, compile-time
> `if constexpr` branches containing `PB_TEST_CHOOSE_TYPE` or
> `PB_TEST_CHOOSE_CONSTEXPR` **only** expand combinations for the active
> compile-time branch:
>
> ```c++ {.good}
> PB_TEST(MySuite, ConditionalCompileTimeDimension) {
>   using T = PB_TEST_CHOOSE_TYPE((int32_t, double));
>   if constexpr (std::is_integral_v<T>) {
>     // Only expands for T = int32_t; T = double registers a single test case.
>     constexpr int kShift = PB_TEST_CHOOSE_CONSTEXPR((1, 8, 16), "shift");
>     EXPECT_EQ((T{1} << kShift) >> kShift, T{1});
>   } else {
>     EXPECT_GT(T{1.5}, T{1.0});
>   }
> }
> ```

### 3. Lexical Scope Requirements for `PB_TEST_CHOOSE_*`

*   **`PB_TEST_CHOOSE_TYPE` and `PB_TEST_CHOOSE_CONSTEXPR`** rely on template
    parameters of the test body and **must** appear lexically inside a `PB_TEST`
    or `PB_TEST_F` body (they cannot be used inside fixture methods or external
    helper functions; pass the chosen type `T` or `constexpr` value as a
    template argument or function parameter to helpers instead).
*   **`PB_TEST_CHOOSE_VALUE`** may be used inside a `PB_TEST` / `PB_TEST_F` body
    or inside the **direct** `PB_TEST_F` fixture's default member initializers,
    constructor, `SetUp()`, or helper methods. Base classes of a `PB_TEST_F`
    fixture (grandparent fixtures) can share normal setup and state, but cannot
    themselves call `PB_TEST_CHOOSE_VALUE` (only the direct fixture class `F`
    passed to `PB_TEST_F(F, ...)` is linked to the test). Any fixture that
    evaluates `PB_TEST_CHOOSE_VALUE` during construction or `SetUp()` **must**
    be used directly with `PB_TEST_F`, not `TEST_F` or a derived fixture
    (calling `PB_TEST_CHOOSE_VALUE` outside an active `PB_TEST` / `PB_TEST_F` or
    its direct fixture fails the test).
*   **Do not define `PB_TEST` or `PB_TEST_F` in header files.**

### 4. Capturing Local State in Value Expressions

*   **Parenthesized lists in `PB_TEST_CHOOSE_VALUE((e1, e2, ...))`** *may*
    capture local variables or `this` (for example,
    `PB_TEST_CHOOSE_VALUE((nullptr, &local_arena))`). When local state is
    captured, the expressions cannot be evaluated during static test
    registration, so automatic test naming extracts names from the source
    expression text (or uses an explicit `{"name1", "name2"}` list if provided).
*   **Iterable arguments (`PB_TEST_CHOOSE_VALUE(range)`)** and
    **`PB_TEST_CHOOSE_CONSTEXPR(...)`** are evaluated inside a captureless `[]`
    lambda during static test registration so the library can determine the
    number of elements and format their names. They **cannot** capture local
    variables with automatic storage duration. If you define a local constant,
    helper lambda, or array to pass into `PB_TEST_CHOOSE_CONSTEXPR` or
    `PB_TEST_CHOOSE_VALUE(range)`, mark it `static constexpr` (or `static
    const`):

```c++ {.bad}
PB_TEST(MySuite, LocalLambdaInConstexpr) {
  auto make_val = [] { return 42; };
  // Compile error: 'make_val' cannot be implicitly captured in a captureless
  // lambda.
  constexpr int kVal = PB_TEST_CHOOSE_CONSTEXPR((1, make_val()));
  ...
}
```

```c++ {.good}
PB_TEST(MySuite, StaticConstexprLambdaInConstexpr) {
  static constexpr auto make_val = [] { return 42; };
  constexpr int kVal = PB_TEST_CHOOSE_CONSTEXPR((1, make_val()));
  ...
}
```

### 5. Dependent Type Syntax with `PB_TEST_CHOOSE_TYPE`

When `PB_TEST_CHOOSE_TYPE` or `PB_TEST_CHOOSE_CONSTEXPR` is used, the test body
is instantiated as a C++ function template. Any type selected by
`PB_TEST_CHOOSE_TYPE` is therefore a **dependent type**.

Calling a member function template on an expression whose type depends on `T`
requires the `.template` disambiguator (or `->template`):

```c++ {.bad}
PB_TEST(MySuite, DependentMemberTemplate) {
  using T = PB_TEST_CHOOSE_TYPE((FooMessage, BarMessage));
  T msg;
  // Compile error: missing 'template' keyword prior to dependent template name.
  EXPECT_EQ(msg.Get<int>(), 0);
}
```

```c++ {.good}
PB_TEST(MySuite, DependentMemberTemplate) {
  using T = PB_TEST_CHOOSE_TYPE((FooMessage, BarMessage));
  T msg;
  EXPECT_EQ(msg.template Get<int>(), 0);
}
```

In C++17 (which applies to OSS-exported `third_party/protobuf` code), nested
type names on a dependent type `T` may also require `typename` in contexts where
C++20's Down with `typename` (P0634R3) does not apply (though `using Sub =
typename T::Sub;` or `using Sub = T::Sub;` in C++20 works directly).
