#include "google/protobuf/repeated_field_proxy.h"

#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/algorithm/container.h"
#include "absl/meta/type_traits.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_field_proxy_iterator.h"
#include "google/protobuf/repeated_field_proxy_traits.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_protos/repeated_field_proxy_import_message.pb.h"
#include "google/protobuf/test_protos/repeated_field_proxy_test.pb.h"
#include "google/protobuf/test_textproto.h"


namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::proto2_unittest::RepeatedFieldProxyTestImportEnum;
using ::proto2_unittest::RepeatedFieldProxyTestSimpleMessage;
using ::proto2_unittest::TestRepeatedEnumProxy;
using ::proto2_unittest::TestRepeatedImportEnumProxy;
using ::proto2_unittest::TestRepeatedImportMessageProxy;
using ::proto2_unittest::TestRepeatedIntProxy;
using ::proto2_unittest::TestRepeatedMessageProxy;
using ::proto2_unittest::TestRepeatedStdStringProxy;
using ::proto2_unittest::TestRepeatedStringViewProxy;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Lt;
using ::testing::Not;

static constexpr absl::string_view kLongString =
    "long string that will be heap allocated";

// We want to say static_assert(false) in some places, but older compilers
// eagerly evaluate `static_assert`s in `if constexpr` expressions and fail to
// compile before templates are instantiated. `dependent_false_t` is a condition
// that is always false that depends on `T` to force lazy evaluation.
template <typename T>
static constexpr bool dependent_false_t [[maybe_unused]] =
    !std::is_same_v<T, T>;

template <typename T>
auto ToStringLike(const T& val) {
  if constexpr (std::is_same_v<absl::remove_cvref_t<decltype(val)>,
                               absl::Cord>) {
    return std::string(val);
  } else {
    return absl::string_view(val);
  }
}

MATCHER_P(StringEq, expected, "") {
  auto val = ToStringLike(arg);
  *result_listener << "where " << val << " is " << expected;
  return val == expected;
}

template <typename T>
T StrAs(absl::string_view s) {
  return T(s);
}

// Emulate a non-Abseil string_view (e.g. STL when Abseil's alias is disabled).
class CustomStringView {
 public:
  explicit CustomStringView(std::string str) : str_(std::move(str)) {}

  const char* data() const { return str_.data(); }
  size_t size() const { return str_.size(); }

  explicit operator std::string() const { return str_; }

 private:
  std::string str_;
};

// A test-only container for a repeated field that manages construction and
// destruction of the underlying repeated field, and can construct proxies.
//
// This is necessary because RepeatedFieldProxy has no public constructors,
// aside from copy assignment.
template <typename T, bool kUseRepeatedFieldOrProxy>
class TestOnlyRepeatedFieldContainer {
  using FieldType = typename internal::RepeatedFieldTraits<T>::type;
  using ProxyType =
      std::conditional_t<kUseRepeatedFieldOrProxy, RepeatedFieldOrProxy<T>,
                         RepeatedFieldProxy<T>>;
  using ConstProxyType = std::conditional_t<kUseRepeatedFieldOrProxy,
                                            RepeatedFieldOrProxy<const T>,
                                            RepeatedFieldProxy<const T>>;

 public:
  static TestOnlyRepeatedFieldContainer New(Arena* arena) {
    return TestOnlyRepeatedFieldContainer(Arena::MakeUnique<FieldType>(arena),
                                          arena);
  }

  FieldType& operator*() { return *field_; }
  const FieldType& operator*() const { return *field_; }

  FieldType* operator->() { return &*field_; }
  const FieldType* operator->() const { return &*field_; }

  ProxyType MakeProxy() {
    return ProxyType(
        internal::RepeatedFieldProxyInternalPrivateAccessHelper<T>::Construct(
            *field_, arena_));
  }
  ConstProxyType MakeConstProxy() const {
    return ConstProxyType(
        internal::RepeatedFieldProxyInternalPrivateAccessHelper<
            const T>::Construct(*field_));
  }

 private:
  TestOnlyRepeatedFieldContainer(Arena::UniquePtr<FieldType> field,
                                 Arena* arena)
      : field_(std::move(field)), arena_(arena) {}

  Arena::UniquePtr<FieldType> field_;
  Arena* arena_;
};

template <bool UseArena, bool UseRepeatedFieldOrProxy>
struct RepeatedFieldProxyTestParams {
  static constexpr bool kUseArena = UseArena;
  static constexpr bool kUseRepeatedFieldOrProxy = UseRepeatedFieldOrProxy;
};

template <typename Params>
static std::string BaseParamsToString() {
  std::string name = "";
  if constexpr (Params::kUseArena) {
    name += "WithArena";
  } else {
    name += "WithoutArena";
  }
  if constexpr (Params::kUseRepeatedFieldOrProxy) {
    name += "_RepeatedFieldOrProxy";
  }
  return name;
}

template <typename Params>
class RepeatedFieldProxyTestBase {
 protected:
  static constexpr bool kUseRepeatedFieldOrProxy =
      Params::kUseRepeatedFieldOrProxy;

  template <typename T>
  using ProxyType =
      std::conditional_t<kUseRepeatedFieldOrProxy, RepeatedFieldOrProxy<T>,
                         RepeatedFieldProxy<T>>;

  template <typename T>
  using ContainerType =
      TestOnlyRepeatedFieldContainer<T, kUseRepeatedFieldOrProxy>;

  static constexpr bool UseArena() { return Params::kUseArena; }
  Arena* arena() { return UseArena() ? &arena_ : nullptr; }

  template <typename ElementType>
  ContainerType<ElementType> MakeRepeatedFieldContainer() {
    return TestOnlyRepeatedFieldContainer<
        ElementType, kUseRepeatedFieldOrProxy>::New(arena());
  }

 private:
  Arena arena_;
};

// This is a test suite which is not generic over the element type. Tests in
// this suite have to explicitly specify the element type. Testing message or
// enum element types has to be done through this suite.
template <typename Params>
class RepeatedFieldProxyTest : public testing::Test,
                               public RepeatedFieldProxyTestBase<Params> {};

struct RepeatedFieldProxyTestName {
  template <typename T>
  static std::string GetName(int) {
    return BaseParamsToString<T>();
  }
};

using AllRepeatedFieldProxyTestParams = ::testing::Types<
    RepeatedFieldProxyTestParams</*use_arena=*/false,
                                 /*use_repeated_field_or_proxy=*/false>,
    RepeatedFieldProxyTestParams</*use_arena=*/false,
                                 /*use_repeated_field_or_proxy=*/true>,
    RepeatedFieldProxyTestParams</*use_arena=*/true,
                                 /*use_repeated_field_or_proxy=*/false>,
    RepeatedFieldProxyTestParams</*use_arena=*/true,
                                 /*use_repeated_field_or_proxy=*/true>>;

TYPED_TEST_SUITE(RepeatedFieldProxyTest, AllRepeatedFieldProxyTestParams,
                 RepeatedFieldProxyTestName);

template <typename ElementTypeParam, bool UseArena,
          bool UseRepeatedFieldOrProxy>
struct RepeatedFieldProxyTypedTestParams
    : public RepeatedFieldProxyTestParams<UseArena, UseRepeatedFieldOrProxy> {
  using ElementType = ElementTypeParam;
};

template <typename Params>
class RepeatedFieldProxyTypedTestBase
    : public RepeatedFieldProxyTestBase<Params> {
 public:
  using Base = RepeatedFieldProxyTestBase<Params>;
  using ElementType = typename Params::ElementType;
  using ContainerType = typename Base::template ContainerType<ElementType>;

  ContainerType MakeRepeatedFieldContainer() {
    return Base::template MakeRepeatedFieldContainer<ElementType>();
  }
};

template <typename Params>
class RepeatedNumericFieldProxyTest
    : public ::testing::Test,
      public RepeatedFieldProxyTypedTestBase<Params> {
 public:
  using Base = RepeatedFieldProxyTypedTestBase<Params>;
  using ElementType = typename Base::ElementType;
  using ContainerType = typename Base::ContainerType;

  template <typename T, typename U,
            typename = std::enable_if_t<std::is_same_v<
                absl::remove_cvref_t<T>, absl::remove_cvref_t<U>>>>
  static void VerifyLambdaTypeRequirements(const ContainerType& field,
                                           const U& lambda_argument) {
    // Verify that `lambda_argument` is a const lvalue reference. The value from
    // the repeated field was intentionally decayed to avoid exposing a
    // reference to the element, but if the argument type of this lambda is a
    // reference, it will alias the temporary copy. Since mutation of this
    // temporary would not affect the original element, ensure it is const.
    static_assert(std::is_lvalue_reference_v<T>);
    static_assert(std::is_const_v<std::remove_reference_t<T>>);

    // Verify that `lambda_argument` is a copy of an element from the repeated
    // field, meaning it does not lie in the backing array.
    const ElementType* backing_array =
        reinterpret_cast<const ElementType*>(field->data());
    const ElementType* backing_array_end = backing_array + field->size();
    EXPECT_THAT(&lambda_argument,
                AnyOf(Lt(backing_array), Ge(backing_array_end)));
  }
};

struct RepeatedNumericFieldProxyTestName {
  template <typename T>
  static std::string GetName(int) {
    using ElementType = typename T::ElementType;

    std::string name;
    if constexpr (std::is_same_v<ElementType, int32_t>) {
      name = "Int32";
    } else if constexpr (std::is_same_v<ElementType, int64_t>) {
      name = "Int64";
    } else if constexpr (std::is_same_v<ElementType, uint32_t>) {
      name = "Uint32";
    } else if constexpr (std::is_same_v<ElementType, uint64_t>) {
      name = "Uint64";
    } else if constexpr (std::is_same_v<ElementType, float>) {
      name = "Float";
    } else if constexpr (std::is_same_v<ElementType, double>) {
      name = "Double";
    } else {
      static_assert(dependent_false_t<ElementType>, "Unsupported numeric type");
    }

    absl::StrAppend(&name, "_", BaseParamsToString<T>());
    return name;
  }
};

#define TEST_NUMERIC_ALL_TYPES(use_arena, use_repeated_field_or_proxy) \
  RepeatedFieldProxyTypedTestParams<int32_t, use_arena,                \
                                    use_repeated_field_or_proxy>,      \
      RepeatedFieldProxyTypedTestParams<int64_t, use_arena,            \
                                        use_repeated_field_or_proxy>,  \
      RepeatedFieldProxyTypedTestParams<uint32_t, use_arena,           \
                                        use_repeated_field_or_proxy>,  \
      RepeatedFieldProxyTypedTestParams<uint64_t, use_arena,           \
                                        use_repeated_field_or_proxy>,  \
      RepeatedFieldProxyTypedTestParams<float, use_arena,              \
                                        use_repeated_field_or_proxy>,  \
      RepeatedFieldProxyTypedTestParams<double, use_arena,             \
                                        use_repeated_field_or_proxy>

#define TEST_NUMERIC_USE_ARENA(use_repeated_field_or_proxy)   \
  TEST_NUMERIC_ALL_TYPES(false, use_repeated_field_or_proxy), \
      TEST_NUMERIC_ALL_TYPES(true, use_repeated_field_or_proxy)

#define TEST_NUMERIC_USE_REPEATED_FIELD_OR_PROXY() \
  TEST_NUMERIC_USE_ARENA(false), TEST_NUMERIC_USE_ARENA(true)

using AllNumericProxyTestParams =
    ::testing::Types<TEST_NUMERIC_USE_REPEATED_FIELD_OR_PROXY()>;

TYPED_TEST_SUITE(RepeatedNumericFieldProxyTest, AllNumericProxyTestParams,
                 RepeatedNumericFieldProxyTestName);

template <typename Params>
class RepeatedStringFieldProxyTest
    : public ::testing::Test,
      public RepeatedFieldProxyTypedTestBase<Params> {
 protected:
  using Base = RepeatedFieldProxyTypedTestBase<Params>;
  using ElementType = typename Base::ElementType;
  using ContainerType = typename Base::ContainerType;

  template <typename T>
  static constexpr void VerifyLambdaTypeRequirements() {
    if constexpr (std::is_same_v<ElementType, absl::Cord>) {
      static_assert(std::is_same_v<T, const absl::Cord&>);
    } else {
      static_assert(std::is_same_v<T, absl::string_view&&>);
    }
  }

  template <typename Compare>
  static auto StringCompare(Compare&& compare) {
    return [&compare](auto&& lhs, auto&& rhs) {
      VerifyLambdaTypeRequirements<decltype(lhs)>();
      VerifyLambdaTypeRequirements<decltype(rhs)>();

      return compare(lhs, rhs);
    };
  }

  // A helper for adding strings to the legacy repeated field containers. The
  // API is inconsistent across the different string types, so this centralizes
  // the special casing.
  void Add(ContainerType& field, absl::string_view s) {
    if constexpr (std::is_same_v<ElementType, std::string> ||
                  std::is_same_v<ElementType, absl::string_view>) {
      field->Add(std::string(s));
    } else if constexpr (std::is_same_v<ElementType, absl::Cord>) {
      field->Add(absl::Cord(s));
    } else {
      static_assert(dependent_false_t<ElementType>, "Unsupported string type");
    }
  }

  const char* StartAddress(const ElementType& element) {
    if constexpr (std::is_same_v<ElementType, std::string> ||
                  std::is_same_v<ElementType, absl::string_view>) {
      return element.data();
    } else if constexpr (std::is_same_v<ElementType, absl::Cord>) {
      return &*element.char_begin();
    } else {
      static_assert(dependent_false_t<ElementType>, "Unsupported string type");
    }
  }
};

struct RepeatedStringFieldProxyTestName {
  template <typename T>
  static std::string GetName(int) {
    using ElementType = typename T::ElementType;

    std::string name;
    if constexpr (std::is_same_v<ElementType, std::string>) {
      name = "StdString";
    } else if constexpr (std::is_same_v<ElementType, absl::string_view>) {
      name = "StringView";
    } else if constexpr (std::is_same_v<ElementType, absl::Cord>) {
      name = "Cord";
    } else {
      static_assert(dependent_false_t<ElementType>, "Unsupported string type");
    }

    absl::StrAppend(&name, "_", BaseParamsToString<T>());
    return name;
  }
};

#define TEST_STRING_ALL_TYPES(use_arena, use_repeated_field_or_proxy) \
  RepeatedFieldProxyTypedTestParams<std::string, use_arena,           \
                                    use_repeated_field_or_proxy>,     \
      RepeatedFieldProxyTypedTestParams<absl::string_view, use_arena, \
                                        use_repeated_field_or_proxy>, \
      RepeatedFieldProxyTypedTestParams<absl::Cord, use_arena,        \
                                        use_repeated_field_or_proxy>

#define TEST_STRING_USE_ARENA(use_repeated_field_or_proxy)   \
  TEST_STRING_ALL_TYPES(false, use_repeated_field_or_proxy), \
      TEST_STRING_ALL_TYPES(true, use_repeated_field_or_proxy)

#define TEST_STRING_USE_REPEATED_FIELD_OR_PROXY() \
  TEST_STRING_USE_ARENA(false), TEST_STRING_USE_ARENA(true)

using AllStringProxyTestParams =
    ::testing::Types<TEST_STRING_USE_REPEATED_FIELD_OR_PROXY()>;

TYPED_TEST_SUITE(RepeatedStringFieldProxyTest, AllStringProxyTestParams,
                 RepeatedStringFieldProxyTestName);

TYPED_TEST(RepeatedNumericFieldProxyTest, RepeatedNumeric) {
  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);
  EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3));

  proxy.set(1, 4);
  EXPECT_THAT(proxy, ElementsAre(1, 4, 3));
  EXPECT_THAT(*field, ElementsAre(1, 4, 3));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, ConstRepeatedNumeric) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  }

  {
    // Check that mutable proxies can be implicitly converted to const proxies.
    decltype(field.MakeConstProxy()) proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  }
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Limits) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();
  proxy.push_back(std::numeric_limits<ElementType>::min());
  proxy.push_back(std::numeric_limits<ElementType>::max());
  EXPECT_THAT(proxy, ElementsAre(std::numeric_limits<ElementType>::min(),
                                 std::numeric_limits<ElementType>::max()));
  EXPECT_THAT(*field, ElementsAre(std::numeric_limits<ElementType>::min(),
                                  std::numeric_limits<ElementType>::max()));
}

TYPED_TEST(RepeatedStringFieldProxyTest, RepeatedString) {
  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();
  proxy.push_back("one");
  proxy.push_back("two");
  proxy.push_back("three");
  EXPECT_THAT(proxy,
              ElementsAre(StringEq("one"), StringEq("two"), StringEq("three")));
  EXPECT_THAT(*field,
              ElementsAre(StringEq("one"), StringEq("two"), StringEq("three")));
}

TYPED_TEST(RepeatedStringFieldProxyTest, ConstRepeatedString) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "one");
  this->Add(field, "two");
  this->Add(field, "three");

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre(StringEq("one"), StringEq("two"),
                                   StringEq("three")));
  }

  {
    // Check that mutable proxies can be implicitly converted to const proxies.
    decltype(field.MakeConstProxy()) proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(StringEq("one"), StringEq("two"),
                                   StringEq("three")));
  }
}

TYPED_TEST(RepeatedFieldProxyTest, RepeatedMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  proxy.emplace_back().set_value(1);

  RepeatedFieldProxyTestSimpleMessage msg;
  msg.set_value(2);
  proxy.push_back(std::move(msg));
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb")));
}

TYPED_TEST(RepeatedFieldProxyTest, ConstRepeatedMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }

  {
    // Check that mutable proxies can be implicitly converted to const proxies.
    decltype(field.MakeConstProxy()) proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }
}

TYPED_TEST(RepeatedFieldProxyTest, Empty) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  EXPECT_TRUE(proxy.empty());
  EXPECT_THAT(proxy, IsEmpty());

  proxy.emplace_back();
  EXPECT_FALSE(proxy.empty());
  EXPECT_THAT(proxy, Not(IsEmpty()));
}

TYPED_TEST(RepeatedFieldProxyTest, ConstEmpty) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_TRUE(proxy.empty());
  }

  field->Add();
  {
    auto proxy = field.MakeConstProxy();
    EXPECT_FALSE(proxy.empty());
  }
}

TYPED_TEST(RepeatedFieldProxyTest, Size) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  EXPECT_EQ(proxy.size(), 0);

  proxy.emplace_back();
  EXPECT_EQ(proxy.size(), 1);

  proxy.emplace_back();
  proxy.emplace_back();
  EXPECT_EQ(proxy.size(), 3);
}

TYPED_TEST(RepeatedFieldProxyTest, ConstSize) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();

  {
    auto proxy = field.MakeProxy();
    EXPECT_EQ(proxy.size(), 0);
  }

  field->Add();
  {
    auto proxy = field.MakeConstProxy();
    EXPECT_EQ(proxy.size(), 1);
  }
}

TYPED_TEST(RepeatedNumericFieldProxyTest, ArrayIndexing) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    auto proxy = field.MakeProxy();
    EXPECT_EQ(proxy[0], 1);
    EXPECT_EQ(proxy[1], 2);
    EXPECT_EQ(proxy[2], 3);

    EXPECT_EQ(proxy.get(0), 1);
    EXPECT_EQ(proxy.get(1), 2);
    EXPECT_EQ(proxy.get(2), 3);

    // Check that repeated numeric proxies return values, not references.
    static_assert(std::is_same_v<decltype(proxy[0]), ElementType>);
    static_assert(std::is_same_v<decltype(proxy.get(0)), ElementType>);
  }

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_EQ(proxy[0], 1);
    EXPECT_EQ(proxy[1], 2);
    EXPECT_EQ(proxy[2], 3);

    EXPECT_EQ(proxy.get(0), 1);
    EXPECT_EQ(proxy.get(1), 2);
    EXPECT_EQ(proxy.get(2), 3);

    // Check that const repeated numeric proxies return values, not const
    // references.
    static_assert(std::is_same_v<decltype(proxy[0]), ElementType>);
    static_assert(std::is_same_v<decltype(proxy.get(0)), ElementType>);
  }
}

TYPED_TEST(RepeatedFieldProxyTest, ArrayIndexingMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  {
    auto proxy = field.MakeProxy();
    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 3)pb"));

    EXPECT_THAT(proxy.get(0), EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy.get(1), EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy.get(2), EqualsProto(R"pb(value: 3)pb"));

    static_assert(std::is_same_v<decltype(proxy[0]),
                                 RepeatedFieldProxyTestSimpleMessage&>);
    static_assert(std::is_same_v<decltype(proxy.get(0)),
                                 const RepeatedFieldProxyTestSimpleMessage&>);
  }

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 3)pb"));

    EXPECT_THAT(proxy.get(0), EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy.get(1), EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy.get(2), EqualsProto(R"pb(value: 3)pb"));

    static_assert(std::is_same_v<decltype(proxy[0]),
                                 const RepeatedFieldProxyTestSimpleMessage&>);
    static_assert(std::is_same_v<decltype(proxy.get(0)),
                                 const RepeatedFieldProxyTestSimpleMessage&>);
  }
}

TYPED_TEST(RepeatedStringFieldProxyTest, ArrayIndexing) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");
  this->Add(field, "3");

  {
    auto proxy = field.MakeProxy();
    EXPECT_THAT(proxy[0], StringEq("1"));
    EXPECT_THAT(proxy[1], StringEq("2"));
    EXPECT_THAT(proxy[2], StringEq("3"));

    EXPECT_THAT(proxy.get(0), StringEq("1"));
    EXPECT_THAT(proxy.get(1), StringEq("2"));
    EXPECT_THAT(proxy.get(2), StringEq("3"));

    if constexpr (std::is_same_v<ElementType, absl::string_view>) {
      static_assert(std::is_same_v<decltype(proxy[0]), absl::string_view>);
      static_assert(std::is_same_v<decltype(proxy.get(0)), absl::string_view>);
    } else {
      static_assert(std::is_same_v<decltype(proxy[0]), ElementType&>);
      static_assert(std::is_same_v<decltype(proxy.get(0)), const ElementType&>);
    }
  }

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy[0], StringEq("1"));
    EXPECT_THAT(proxy[1], StringEq("2"));
    EXPECT_THAT(proxy[2], StringEq("3"));

    EXPECT_THAT(proxy.get(0), StringEq("1"));
    EXPECT_THAT(proxy.get(1), StringEq("2"));
    EXPECT_THAT(proxy.get(2), StringEq("3"));

    if constexpr (std::is_same_v<ElementType, absl::string_view>) {
      static_assert(std::is_same_v<decltype(proxy[0]), absl::string_view>);
      static_assert(std::is_same_v<decltype(proxy.get(0)), absl::string_view>);
    } else {
      static_assert(std::is_same_v<decltype(proxy[0]), const ElementType&>);
      static_assert(std::is_same_v<decltype(proxy.get(0)), const ElementType&>);
    }
  }
}

TYPED_TEST(RepeatedNumericFieldProxyTest, MutateElementPrimitive) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    auto proxy = field.MakeProxy();
    proxy.set(0, 4);

    EXPECT_THAT(proxy, ElementsAre(4, 2, 3));
  }
}

TYPED_TEST(RepeatedStringFieldProxyTest, MutateElement) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");
  this->Add(field, "3");
  this->Add(field, "4");

  auto proxy = field.MakeProxy();
  if constexpr (std::is_same_v<ElementType, std::string>) {
    proxy[0] = "5";
    proxy[1] = StrAs<std::string>("6");
    const char* c_str = "7";
    proxy[2] = c_str;
    proxy[3] = StrAs<absl::string_view>("8");

    EXPECT_THAT(proxy, ElementsAre(StringEq("5"), StringEq("6"), StringEq("7"),
                                   StringEq("8")));
  }

  auto long_string = std::string(kLongString);
  const char* string_ptr = long_string.c_str();

  proxy.set(0, std::move(long_string));
  EXPECT_THAT(proxy[0], StringEq(kLongString));
  if constexpr (std::is_same_v<ElementType, std::string> ||
                std::is_same_v<ElementType, absl::string_view>) {
    // Since long_string was moved, proxy[0] should point to the same heap data.
    EXPECT_EQ(string_ptr, proxy[0].data());
  } else {
    (void)string_ptr;
  }

  proxy.set(0, "9");
  proxy.set(1, std::string("10"));
  const char* c_str2 = "11";
  proxy.set(2, c_str2);
  proxy.set(3, absl::string_view("12"));

  EXPECT_THAT(proxy, ElementsAre(StringEq("9"), StringEq("10"), StringEq("11"),
                                 StringEq("12")));

  std::string str13 = "13", str14 = "14";
  proxy.set(0, std::ref(str13));
  proxy.set(1, std::ref(str14));
  proxy.set(2, std::ref("15"));
  proxy.set(3, std::cref("16"));

  EXPECT_THAT(proxy, ElementsAre(StringEq("13"), StringEq("14"), StringEq("15"),
                                 StringEq("16")));

  auto cord = absl::Cord(kLongString);
  proxy.set(0, std::move(cord));
  EXPECT_THAT(proxy[0], StringEq(kLongString));
}

TYPED_TEST(RepeatedFieldProxyTest, MutateElementMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  auto proxy = field.MakeProxy();
  proxy[2].set_value(4);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 4)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 4)pb")));

  RepeatedFieldProxyTestSimpleMessage msg;
  msg.set_value(5);
  proxy.set(0, msg);
  // `msg` is copied into the element, so it should be unchanged.
  EXPECT_TRUE(msg.has_value());

  {
    auto msg2 =
        Arena::MakeUnique<RepeatedFieldProxyTestSimpleMessage>(this->arena());
    msg2->set_value(6);
    auto* nested = msg2->mutable_nested();
    nested->set_value(7);
    proxy.set(1, std::move(*msg2));

    // Since `msg2` was moved, `nested` should point to the same object.
    EXPECT_EQ(proxy[1].mutable_nested(), nested);
  }

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 5)pb"),
                                 EqualsProto(R"pb(value: 6,
                                                  nested { value: 7 })pb"),
                                 EqualsProto(R"pb(value: 4)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 5)pb"),
                                  EqualsProto(R"pb(value: 6,
                                                   nested { value: 7 })pb"),
                                  EqualsProto(R"pb(value: 4)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, PushBack) {
  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);

  EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3));
}

TYPED_TEST(RepeatedFieldProxyTest, PushBackMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  auto msg1 = RepeatedFieldProxyTestSimpleMessage();
  msg1.set_value(1);
  proxy.push_back(msg1);
  auto msg2 = RepeatedFieldProxyTestSimpleMessage();
  msg2.set_value(2);
  proxy.push_back(msg2);
  auto msg3 = RepeatedFieldProxyTestSimpleMessage();
  msg3.set_value(3);
  proxy.push_back(msg3);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TYPED_TEST(RepeatedFieldProxyTest, PushBackMessageLvalueCopies) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  auto msg1 =
      Arena::MakeUnique<RepeatedFieldProxyTestSimpleMessage>(this->arena());
  auto* nested = msg1->mutable_nested();
  proxy.push_back(*msg1);
  EXPECT_NE(proxy[0].mutable_nested(), nested);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(nested: {})pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(nested: {})pb")));
}

TYPED_TEST(RepeatedFieldProxyTest, PushBackMessageRvalueDoesNotCopy) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  auto msg1 =
      Arena::MakeUnique<RepeatedFieldProxyTestSimpleMessage>(this->arena());
  auto* nested = msg1->mutable_nested();
  proxy.push_back(std::move(*msg1));
  EXPECT_EQ(proxy[0].mutable_nested(), nested);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(nested: {})pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(nested: {})pb")));
}

TYPED_TEST(RepeatedStringFieldProxyTest, PushBack) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();

  {
    proxy.push_back("1");
    proxy.push_back(StrAs<std::string>("2"));
    const char* c_str = "3";
    proxy.push_back(c_str);
    proxy.push_back(StrAs<absl::string_view>("4"));

    EXPECT_THAT(proxy[0], StringEq("1"));
    EXPECT_THAT(proxy[1], StringEq("2"));
    EXPECT_THAT(proxy[2], StringEq("3"));
    EXPECT_THAT(proxy[3], StringEq("4"));
  }

  {
    auto long_string = std::string(kLongString);
    const char* string_ptr = long_string.c_str();

    proxy.push_back(std::move(long_string));
    EXPECT_THAT(proxy[4], StringEq(kLongString));

    if constexpr (std::is_same_v<ElementType, std::string> ||
                  std::is_same_v<ElementType, absl::string_view>) {
      // Since long_string was moved, proxy[4] should point to the same heap
      // data.
      EXPECT_EQ(string_ptr, proxy[4].data());
    } else {
      (void)string_ptr;
    }
  }

  {
    std::string str6 = "6", str7 = "7";
    proxy.push_back(std::ref(str6));
    proxy.push_back(std::ref(str7));
    proxy.push_back(std::ref("8"));
    proxy.push_back(std::cref("9"));

    EXPECT_THAT(proxy[5], StringEq("6"));
    EXPECT_THAT(proxy[6], StringEq("7"));
    EXPECT_THAT(proxy[7], StringEq("8"));
    EXPECT_THAT(proxy[8], StringEq("9"));
  }

  {
    auto cord = absl::Cord(kLongString);
    proxy.push_back(std::move(cord));
    EXPECT_THAT(proxy[9], StringEq(kLongString));
  }
}

TYPED_TEST(RepeatedNumericFieldProxyTest, EmplaceBack) {
  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();
  proxy.emplace_back(1);
  proxy.emplace_back(2);
  proxy.emplace_back(3);

  EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3));
}

TYPED_TEST(RepeatedFieldProxyTest, EmplaceBackMessageLvalueCopies) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  auto msg1 =
      Arena::MakeUnique<RepeatedFieldProxyTestSimpleMessage>(this->arena());
  auto* nested = msg1->mutable_nested();
  proxy.emplace_back(*msg1);
  EXPECT_NE(proxy[0].mutable_nested(), nested);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(nested: {})pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(nested: {})pb")));
}

TYPED_TEST(RepeatedFieldProxyTest, EmplaceBackMessageRvalueDoesNotCopy) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  auto msg1 =
      Arena::MakeUnique<RepeatedFieldProxyTestSimpleMessage>(this->arena());
  auto* nested = msg1->mutable_nested();
  proxy.emplace_back(std::move(*msg1));
  EXPECT_EQ(proxy[0].mutable_nested(), nested);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(nested: {})pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(nested: {})pb")));
}

template <typename FieldType, template <typename> class ProxyType,
          typename StringType>
void TestEmplaceBackVanillaString(const FieldType& field,
                                  ProxyType<StringType> proxy) {
  // Test that `emplace_back` with no arguments inserts an empty string.
  EXPECT_EQ(proxy.emplace_back(), "");

  // Test that `emplace_back` returns a reference to the inserted element.
  EXPECT_EQ(proxy.emplace_back("1"), "1");

  // Test reflexive insertions.
  proxy.emplace_back(proxy[1]);

  proxy.emplace_back(StrAs<std::string>("2"));
  proxy.emplace_back(StrAs<absl::string_view>("3"));
  const char* c_str = "4";
  proxy.emplace_back(c_str);

  auto moved_string = std::string(kLongString);
  const char* moved_string_ptr = moved_string.data();
  proxy.emplace_back(std::move(moved_string));
  EXPECT_EQ(proxy[6].data(), moved_string_ptr);

  auto copied_string = std::string(kLongString);
  const char* copied_string_ptr = copied_string.data();
  proxy.emplace_back(copied_string);
  EXPECT_NE(proxy[7].data(), copied_string_ptr);
  EXPECT_EQ(copied_string, kLongString);

  if constexpr (std::is_same_v<StringType, std::string>) {
    proxy.emplace_back(10, 'a');

    const char* c_str_with_len = "5";
    proxy.emplace_back(c_str_with_len, std::strlen(c_str_with_len));
    const char* c_str_with_len2 = "60";
    proxy.emplace_back(c_str_with_len2, 2);

    std::string string_to_copy_from = "123456789";
    auto start_it = absl::c_find(string_to_copy_from, '3');
    auto end_it = absl::c_find(string_to_copy_from, '7');
    proxy.emplace_back(start_it, end_it);
    proxy.emplace_back(absl::c_find(string_to_copy_from, '1'),
                       absl::c_find(string_to_copy_from, '4'));
  }

  if constexpr (std::is_same_v<StringType, std::string>) {
    EXPECT_THAT(proxy, ElementsAre("", "1", "1", "2", "3", "4", kLongString,
                                   kLongString, "aaaaaaaaaa", "5", "60", "3456",
                                   "123"));
    EXPECT_THAT(*field, ElementsAre("", "1", "1", "2", "3", "4", kLongString,
                                    kLongString, "aaaaaaaaaa", "5", "60",
                                    "3456", "123"));
  } else {
    EXPECT_THAT(proxy, ElementsAre("", "1", "1", "2", "3", "4", kLongString,
                                   kLongString));
    EXPECT_THAT(*field, ElementsAre("", "1", "1", "2", "3", "4", kLongString,
                                    kLongString));
  }
}

TYPED_TEST(RepeatedFieldProxyTest, EmplaceBackStdString) {
  auto field = this->template MakeRepeatedFieldContainer<std::string>();
  TestEmplaceBackVanillaString(field, field.MakeProxy());
}

TYPED_TEST(RepeatedFieldProxyTest, EmplaceBackStringView) {
  auto field = this->template MakeRepeatedFieldContainer<absl::string_view>();
  auto proxy = field.MakeProxy();
  TestEmplaceBackVanillaString(field, proxy);

  // Check that we don't leak an `std::string` through the `emplace_back` API.
  static_assert(
      std::is_same_v<decltype(proxy.emplace_back("1")), absl::string_view>);
}

TYPED_TEST(RepeatedFieldProxyTest, EmplaceBackCord) {
  auto field = this->template MakeRepeatedFieldContainer<absl::Cord>();
  auto proxy = field.MakeProxy();
  proxy.emplace_back("1");
  proxy.emplace_back(StrAs<absl::string_view>("2"));

  absl::Cord cord3 = absl::Cord(kLongString);
  proxy.emplace_back(std::move(cord3));

  absl::Cord cord4 = absl::Cord(kLongString);
  proxy.emplace_back(cord4);

  EXPECT_THAT(proxy, ElementsAre("1", "2", kLongString, kLongString));
  EXPECT_THAT(*field, ElementsAre("1", "2", kLongString, kLongString));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Iterators) {
  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);

  auto it = proxy.begin();
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(++it), 2);
  EXPECT_EQ(*(++it), 3);
  EXPECT_EQ(++it, proxy.end());

  // Post-increment
  it = proxy.begin();
  EXPECT_EQ(*(it++), 1);

  // Pre-decrement
  it = proxy.end();
  EXPECT_EQ(*(--it), 3);

  // Post-decrement
  EXPECT_EQ(*(it--), 3);
  EXPECT_EQ(*it, 2);

  // Equality
  EXPECT_TRUE(proxy.begin() == proxy.begin());
  EXPECT_TRUE(proxy.end() == proxy.end());
  EXPECT_FALSE(proxy.begin() == proxy.end());

  // Inequality
  EXPECT_FALSE(proxy.begin() != proxy.begin());
  EXPECT_FALSE(proxy.end() != proxy.end());
  EXPECT_TRUE(proxy.begin() != proxy.end());

  // Random access
  it = proxy.begin();
  EXPECT_EQ(*(it + 0), 1);
  EXPECT_EQ(*(it + 1), 2);
  EXPECT_EQ(*(it + 2), 3);

  // Add assign
  it += 2;
  EXPECT_EQ(*(it - 1), 2);

  // Subtract assign
  it -= 1;
  EXPECT_EQ(*it, 2);

  // Indexing
  EXPECT_EQ(it[1], 3);

  // Reverse iterators
  auto rit = proxy.rbegin();
  EXPECT_EQ(*rit, 3);
  EXPECT_EQ(*(++rit), 2);
  EXPECT_EQ(*(++rit), 1);
  EXPECT_EQ(++rit, proxy.rend());

  google::protobuf::sort(proxy.begin(), proxy.end(), std::greater<int>());
  EXPECT_THAT(proxy, ElementsAre(3, 2, 1));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, IteratorsDoNotLeakReferences) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();
  auto const_proxy = field.MakeConstProxy();

  // `cbegin()` on mutable proxies should return the same type as `begin()` on
  // const proxies.
  static_assert(
      std::is_same_v<decltype(proxy.cbegin()), decltype(const_proxy.begin())>);

  // All iterator types should dereference to element values, not references.
  static_assert(std::is_same_v<decltype(*proxy.begin()), ElementType>);
  static_assert(std::is_same_v<decltype(*proxy.cbegin()), ElementType>);
  static_assert(std::is_same_v<decltype(*proxy.rbegin()), ElementType>);

  // Indexing should also dereference to element values, not references.
  static_assert(std::is_same_v<decltype(proxy.begin()[0]), ElementType>);
  static_assert(std::is_same_v<decltype(proxy.cbegin()[0]), ElementType>);
  static_assert(std::is_same_v<decltype(proxy.rbegin()[0]), ElementType>);
}

TYPED_TEST(RepeatedFieldProxyTest, EnumIterators) {
  auto field =
      this->template MakeRepeatedFieldContainer<TestRepeatedEnumProxy::Enum>();
  auto proxy = field.MakeProxy();
  proxy.push_back(TestRepeatedEnumProxy::FOO);
  proxy.push_back(TestRepeatedEnumProxy::BAR);
  proxy.push_back(TestRepeatedEnumProxy::BAZ);

  auto it = proxy.begin();
  EXPECT_EQ(*it, TestRepeatedEnumProxy::FOO);
  EXPECT_EQ(it[1], TestRepeatedEnumProxy::BAR);

  auto rit = proxy.rbegin();
  EXPECT_EQ(*rit, TestRepeatedEnumProxy::BAZ);
  EXPECT_EQ(rit[1], TestRepeatedEnumProxy::BAR);
}

TYPED_TEST(RepeatedFieldProxyTest, EnumIteratorsDoNotLeakReferences) {
  auto field =
      this->template MakeRepeatedFieldContainer<TestRepeatedEnumProxy::Enum>();
  auto proxy = field.MakeProxy();
  auto const_proxy = field.MakeConstProxy();

  // `cbegin()` on mutable proxies should return the same type as `begin()` on
  // const proxies.
  static_assert(
      std::is_same_v<decltype(proxy.cbegin()), decltype(const_proxy.begin())>);

  // All iterator types should dereference to element values, not references.
  static_assert(
      std::is_same_v<decltype(*proxy.begin()), TestRepeatedEnumProxy::Enum>);
  static_assert(
      std::is_same_v<decltype(*proxy.cbegin()), TestRepeatedEnumProxy::Enum>);
  static_assert(
      std::is_same_v<decltype(*proxy.rbegin()), TestRepeatedEnumProxy::Enum>);

  // Indexing should also dereference to element values, not references.
  static_assert(
      std::is_same_v<decltype(proxy.begin()[0]), TestRepeatedEnumProxy::Enum>);
  static_assert(
      std::is_same_v<decltype(proxy.cbegin()[0]), TestRepeatedEnumProxy::Enum>);
  static_assert(
      std::is_same_v<decltype(proxy.rbegin()[0]), TestRepeatedEnumProxy::Enum>);
}

TYPED_TEST(RepeatedStringFieldProxyTest, Iterators) {
  auto field = this->MakeRepeatedFieldContainer();
  auto proxy = field.MakeProxy();

  proxy.push_back("1");
  proxy.push_back("2");
  proxy.push_back("3");

  auto it = proxy.begin();
  EXPECT_THAT(*it, StringEq("1"));
  EXPECT_THAT(*(++it), StringEq("2"));
  EXPECT_THAT(*(++it), StringEq("3"));
  EXPECT_EQ(++it, proxy.end());

  // Post-increment
  it = proxy.begin();
  EXPECT_THAT(*(it++), StringEq("1"));

  // Pre-decrement
  it = proxy.end();
  EXPECT_THAT(*(--it), StringEq("3"));

  // Post-decrement
  EXPECT_THAT(*(it--), StringEq("3"));
  EXPECT_THAT(*it, StringEq("2"));

  // Equality
  EXPECT_TRUE(proxy.begin() == proxy.begin());
  EXPECT_TRUE(proxy.end() == proxy.end());
  EXPECT_FALSE(proxy.begin() == proxy.end());

  // Inequality
  EXPECT_FALSE(proxy.begin() != proxy.begin());
  EXPECT_FALSE(proxy.end() != proxy.end());
  EXPECT_TRUE(proxy.begin() != proxy.end());

  // Random access
  it = proxy.begin();
  EXPECT_THAT(*(it + 0), StringEq("1"));
  EXPECT_THAT(*(it + 1), StringEq("2"));
  EXPECT_THAT(*(it + 2), StringEq("3"));

  // Add assign
  it += 2;
  EXPECT_THAT(*(it - 1), StringEq("2"));

  // Subtract assign
  it -= 1;
  EXPECT_THAT(*it, StringEq("2"));

  // Indexing
  EXPECT_THAT(it[1], StringEq("3"));

  // Reverse iterators
  auto rit = proxy.rbegin();
  EXPECT_THAT(*rit, StringEq("3"));
  EXPECT_THAT(*(++rit), StringEq("2"));
  EXPECT_THAT(*(++rit), StringEq("1"));
  EXPECT_EQ(++rit, proxy.rend());

  // Const iterators
  auto cit = proxy.cbegin();
  EXPECT_THAT(*cit, StringEq("1"));
  EXPECT_THAT(*(++cit), StringEq("2"));
  EXPECT_THAT(*(++cit), StringEq("3"));
  EXPECT_EQ(++cit, proxy.cend());
}

TYPED_TEST(RepeatedFieldProxyTest, IteratorMutation) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  proxy.emplace_back().set_value(1);
  proxy.emplace_back().set_value(2);
  proxy.emplace_back().set_value(3);

  {
    auto it = proxy.begin();
    RepeatedFieldProxyTestSimpleMessage msg;
    msg.set_value(4);
    *it = msg;
    msg.set_value(5);
    *(++it) = std::move(msg);
  }
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 4)pb"),
                                 EqualsProto(R"pb(value: 5)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 4)pb"),
                                  EqualsProto(R"pb(value: 5)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));

  {
    auto rit = proxy.rbegin();
    RepeatedFieldProxyTestSimpleMessage msg;
    msg.set_value(6);
    *rit = msg;
    msg.set_value(7);
    *(++rit) = std::move(msg);
  }
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 4)pb"),
                                 EqualsProto(R"pb(value: 7)pb"),
                                 EqualsProto(R"pb(value: 6)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 4)pb"),
                                  EqualsProto(R"pb(value: 7)pb"),
                                  EqualsProto(R"pb(value: 6)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, ConstIterators) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  auto proxy = field.MakeConstProxy();
  auto it = proxy.cbegin();
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(++it), 2);
  EXPECT_EQ(*(++it), 3);
  EXPECT_EQ(++it, proxy.cend());

  auto rit = proxy.rbegin();
  EXPECT_EQ(*rit, 3);
  EXPECT_EQ(*(++rit), 2);
  EXPECT_EQ(*(++rit), 1);
  EXPECT_EQ(++rit, proxy.rend());
}

TYPED_TEST(RepeatedFieldProxyTest, StringViewIteratorsElementAccess) {
  auto field = this->template MakeRepeatedFieldContainer<absl::string_view>();
  auto proxy = field.MakeProxy();
  proxy.push_back("first");
  proxy.push_back("second");
  proxy.push_back("3rd");

  auto it = proxy.begin();

  EXPECT_EQ(it->size(), 5);
  EXPECT_EQ(it->at(0), 'f');
  ++it;

  EXPECT_EQ(it->size(), 6);
  ++it;
  EXPECT_EQ(it->size(), 3);
  ++it;
  EXPECT_TRUE(it == proxy.end());

  google::protobuf::sort(proxy.begin(), proxy.end(),
               [](absl::string_view a, absl::string_view b) {
                 return a.size() < b.size();
               });
  EXPECT_THAT(proxy, ElementsAre("3rd", "first", "second"));
}

TYPED_TEST(RepeatedFieldProxyTest, StringViewIteratorsNoStdStringLeak) {
  auto field = this->template MakeRepeatedFieldContainer<absl::string_view>();
  auto proxy = field.MakeProxy();

  static constexpr bool kOrProxy = TypeParam::kUseRepeatedFieldOrProxy;

  // Check that we don't leak an `std::string` through the iterator.
  static_assert(
      std::is_same_v<decltype(proxy.begin()),
                     RepeatedFieldProxyIteratorImpl<
                         absl::string_view, /*kReverse=*/false, kOrProxy>>);
  static_assert(
      std::is_same_v<decltype(proxy.end()),
                     RepeatedFieldProxyIteratorImpl<
                         absl::string_view, /*kReverse=*/false, kOrProxy>>);
  static_assert(std::is_same_v<
                decltype(proxy.cbegin()),
                RepeatedFieldProxyIteratorImpl<const absl::string_view,
                                               /*kReverse=*/false, kOrProxy>>);
  static_assert(std::is_same_v<
                decltype(proxy.cend()),
                RepeatedFieldProxyIteratorImpl<const absl::string_view,
                                               /*kReverse=*/false, kOrProxy>>);
  static_assert(
      std::is_same_v<decltype(proxy.rbegin()),
                     RepeatedFieldProxyIteratorImpl<
                         absl::string_view, /*kReverse=*/true, kOrProxy>>);
  static_assert(
      std::is_same_v<decltype(proxy.rend()),
                     RepeatedFieldProxyIteratorImpl<
                         absl::string_view, /*kReverse=*/true, kOrProxy>>);

  auto it = proxy.begin();

  static_assert(
      std::is_same_v<absl::remove_cvref_t<decltype(*it)>, absl::string_view>);
}

// Helper type trait to check if `T1` and `T2` can be compared with `==`.
//
// This essentially does a "does this compile" check for the equality operator.
// In C++, a template specialization is ignored if, when substituting the
// template parameters, any of the specialization does not compile.
//
// If operator==(T1, T2) is not defined, `equality_comparable` will fall back to
// the default implementation, which is false.
template <typename T1, typename T2, typename = void>
static constexpr bool is_equality_comparable = false;
template <typename T1, typename T2>
static constexpr bool is_equality_comparable<
    T1, T2, std::void_t<decltype(std::declval<T1>() == std::declval<T2>())>> =
    true;

// Verify that equality_comparable works as expected for a few types:
static_assert(is_equality_comparable<int32_t, int32_t>);
static_assert(is_equality_comparable<int32_t, size_t>);
static_assert(!is_equality_comparable<int32_t, std::string>);

template <typename T1, typename T2>
constexpr bool TypesIncompatible() {
  return !std::is_assignable_v<T1, T2> && !std::is_assignable_v<T2, T1> &&
         !std::is_constructible_v<T1, T2> && !std::is_constructible_v<T2, T1> &&
         !is_equality_comparable<T1, T2> && !is_equality_comparable<T2, T1>;
}

template <typename ElementType>
constexpr bool IteratorsIncompatible() {
  return TypesIncompatible<
             decltype(std::declval<RepeatedFieldProxy<ElementType>>().begin()),
             decltype(std::declval<RepeatedFieldOrProxy<ElementType>>()
                          .begin())>() &&
         TypesIncompatible<
             decltype(std::declval<RepeatedFieldProxy<ElementType>>().cbegin()),
             decltype(std::declval<RepeatedFieldOrProxy<ElementType>>()
                          .cbegin())>() &&
         TypesIncompatible<
             decltype(std::declval<RepeatedFieldProxy<ElementType>>().rbegin()),
             decltype(std::declval<RepeatedFieldOrProxy<ElementType>>()
                          .rbegin())>();
}

TEST(RepeatedFieldProxyIteratorTest, ProxyAndOrProxyIteratorsIncompatible) {
  EXPECT_TRUE(IteratorsIncompatible<bool>());
  EXPECT_TRUE(IteratorsIncompatible<int32_t>());
  EXPECT_TRUE(IteratorsIncompatible<int64_t>());
  EXPECT_TRUE(IteratorsIncompatible<uint32_t>());
  EXPECT_TRUE(IteratorsIncompatible<uint32_t>());
  EXPECT_TRUE(IteratorsIncompatible<float>());
  EXPECT_TRUE(IteratorsIncompatible<double>());
  EXPECT_TRUE(IteratorsIncompatible<absl::string_view>());
  EXPECT_TRUE(IteratorsIncompatible<std::string>());
  EXPECT_TRUE(IteratorsIncompatible<absl::Cord>());
  EXPECT_TRUE(IteratorsIncompatible<RepeatedFieldProxyTestSimpleMessage>());
}

TYPED_TEST(RepeatedNumericFieldProxyTest, PopBack) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);

  auto proxy = field.MakeProxy();
  proxy.pop_back();

  EXPECT_THAT(proxy, ElementsAre(1));
  EXPECT_THAT(*field, ElementsAre(1));
}

TYPED_TEST(RepeatedFieldProxyTest, PopBackMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  auto proxy = field.MakeProxy();
  proxy.pop_back();

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TYPED_TEST(RepeatedStringFieldProxyTest, PopBack) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");

  auto proxy = field.MakeProxy();
  proxy.pop_back();

  EXPECT_THAT(proxy, ElementsAre(StringEq("1")));
  EXPECT_THAT(*field, ElementsAre(StringEq("1")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Clear) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);

  auto proxy = field.MakeProxy();
  proxy.clear();

  EXPECT_THAT(proxy, IsEmpty());
  EXPECT_THAT(*field, IsEmpty());
}

TYPED_TEST(RepeatedFieldProxyTest, ClearMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  auto proxy = field.MakeProxy();
  proxy.clear();

  EXPECT_THAT(proxy, IsEmpty());
  EXPECT_THAT(*field, IsEmpty());
}

TYPED_TEST(RepeatedStringFieldProxyTest, Clear) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");

  auto proxy = field.MakeProxy();
  proxy.clear();

  EXPECT_THAT(proxy, IsEmpty());
  EXPECT_THAT(*field, IsEmpty());
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Erase) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  auto proxy = field.MakeProxy();
  proxy.erase(absl::c_find_if(proxy, [](auto value) { return value == 2; }));

  EXPECT_THAT(proxy, ElementsAre(1, 3));
  EXPECT_THAT(*field, ElementsAre(1, 3));
}

TYPED_TEST(RepeatedStringFieldProxyTest, Erase) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");
  this->Add(field, "3");

  auto proxy = field.MakeProxy();
  proxy.erase(proxy.begin() + 1);

  EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("3")));
  EXPECT_THAT(*field, ElementsAre(StringEq("1"), StringEq("3")));
}

TYPED_TEST(RepeatedFieldProxyTest, Erase) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  auto proxy = field.MakeProxy();
  proxy.erase(absl::c_find_if(
      proxy, [](const RepeatedFieldProxyTestSimpleMessage& msg) {
        return msg.value() == 2;
      }));

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, EraseRange) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);
  field->Add(3);
  field->Add(4);

  auto proxy = field.MakeProxy();
  auto it = absl::c_find_if(proxy, [](auto value) { return value == 2; });
  proxy.erase(it, it + 2);

  EXPECT_THAT(proxy, ElementsAre(1, 4));
  EXPECT_THAT(*field, ElementsAre(1, 4));
}

TYPED_TEST(RepeatedStringFieldProxyTest, EraseRange) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");
  this->Add(field, "3");
  this->Add(field, "4");

  auto proxy = field.MakeProxy();
  auto it = proxy.begin() + 1;
  proxy.erase(it, it + 2);

  EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("4")));
  EXPECT_THAT(*field, ElementsAre(StringEq("1"), StringEq("4")));
}

TYPED_TEST(RepeatedFieldProxyTest, EraseRange) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);
  field->Add()->set_value(4);

  auto proxy = field.MakeProxy();
  auto it = absl::c_find_if(proxy,
                            [](const RepeatedFieldProxyTestSimpleMessage& msg) {
                              return msg.value() == 2;
                            });
  proxy.erase(it, it + 2);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 4)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 4)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Proto2Erase) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);
  field->Add(3);
  field->Add(4);

  auto proxy = field.MakeProxy();
  size_t count = google::protobuf::erase(proxy, 2);
  EXPECT_EQ(count, 1);
  EXPECT_THAT(proxy, ElementsAre(1, 3, 4));
  EXPECT_THAT(*field, ElementsAre(1, 3, 4));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Proto2EraseIf) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);
  field->Add(3);
  field->Add(4);

  auto proxy = field.MakeProxy();
  size_t count = google::protobuf::erase_if(proxy, [this, &field](auto&& x) {
    this->template VerifyLambdaTypeRequirements<decltype(x)>(field, x);
    return x > ElementType{2};
  });
  EXPECT_EQ(count, 2);
  EXPECT_THAT(proxy, ElementsAre(1, 2));
  EXPECT_THAT(*field, ElementsAre(1, 2));
}

TYPED_TEST(RepeatedStringFieldProxyTest, Proto2Erase) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");
  this->Add(field, "3");
  this->Add(field, "4");

  auto proxy = field.MakeProxy();
  size_t count = google::protobuf::erase(proxy, "3");

  EXPECT_EQ(count, 1);
  EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("2"), StringEq("4")));
  EXPECT_THAT(*field, ElementsAre(StringEq("1"), StringEq("2"), StringEq("4")));
}

TYPED_TEST(RepeatedStringFieldProxyTest, Proto2EraseIf) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");
  this->Add(field, "3");
  this->Add(field, "4");

  auto proxy = field.MakeProxy();
  size_t count = google::protobuf::erase_if(proxy, [this](auto&& s) {
    this->template VerifyLambdaTypeRequirements<decltype(s)>();
    return s == "2" || s == "4";
  });
  EXPECT_EQ(count, 2);
  EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("3")));
  EXPECT_THAT(*field, ElementsAre(StringEq("1"), StringEq("3")));
}

TYPED_TEST(RepeatedFieldProxyTest, Proto2EraseIfMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);
  field->Add()->set_value(4);

  auto proxy = field.MakeProxy();
  size_t count = google::protobuf::erase_if(
      proxy, [](const auto& msg) { return msg.value() % 2 == 0; });
  EXPECT_EQ(count, 2);
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, CopyAssign) {
  auto field1 = this->MakeRepeatedFieldContainer();
  field1->Add(1);

  auto field2 = this->MakeRepeatedFieldContainer();
  field2->Add(2);
  field2->Add(3);

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.assign(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(2, 3));
  EXPECT_THAT(*field1, ElementsAre(2, 3));

  EXPECT_THAT(proxy2, ElementsAre(2, 3));
  EXPECT_THAT(*field2, ElementsAre(2, 3));
}

TYPED_TEST(RepeatedStringFieldProxyTest, CopyAssign) {
  auto field1 = this->MakeRepeatedFieldContainer();
  this->Add(field1, "1");

  auto field2 = this->MakeRepeatedFieldContainer();
  this->Add(field2, "2");
  this->Add(field2, "3");

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.assign(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(StringEq("2"), StringEq("3")));
  EXPECT_THAT(*field1, ElementsAre(StringEq("2"), StringEq("3")));

  EXPECT_THAT(proxy2, ElementsAre(StringEq("2"), StringEq("3")));
  EXPECT_THAT(*field2, ElementsAre(StringEq("2"), StringEq("3")));
}

TYPED_TEST(RepeatedFieldProxyTest, CopyAssignMessage) {
  auto field1 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);
  field2->Add()->set_value(3);

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.assign(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                   EqualsProto(R"pb(value: 3)pb")));

  EXPECT_THAT(proxy2, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field2, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                   EqualsProto(R"pb(value: 3)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, AssignIteratorRange) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);

  auto proxy = field.MakeProxy();

  std::vector<ElementType> ints = {2, 3};
  proxy.assign(ints.begin(), ints.end());

  EXPECT_THAT(proxy, ElementsAre(2, 3));
  EXPECT_THAT(*field, ElementsAre(2, 3));

  // Assign a subrange of `ints`.
  proxy.assign(ints.begin() + 1, ints.end());
  EXPECT_THAT(proxy, ElementsAre(3));
  EXPECT_THAT(*field, ElementsAre(3));
}

TYPED_TEST(RepeatedStringFieldProxyTest, AssignIteratorRange) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");

  auto proxy = field.MakeProxy();

  std::vector<absl::string_view> strs = {"2", "3"};
  proxy.assign(strs.begin(), strs.end());

  EXPECT_THAT(proxy, ElementsAre(StringEq("2"), StringEq("3")));
  EXPECT_THAT(*field, ElementsAre(StringEq("2"), StringEq("3")));

  // Assign a subrange of `strs`.
  proxy.assign(strs.begin() + 1, strs.end());
  EXPECT_THAT(proxy, ElementsAre(StringEq("3")));
  EXPECT_THAT(*field, ElementsAre(StringEq("3")));
}

TYPED_TEST(RepeatedStringFieldProxyTest, AssignIteratorRangeWithStdStrings) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");

  auto proxy = field.MakeProxy();

  std::vector<std::string> strs = {"2", "3"};
  proxy.assign(strs.begin(), strs.end());

  EXPECT_THAT(proxy, ElementsAre(StringEq("2"), StringEq("3")));
  EXPECT_THAT(*field, ElementsAre(StringEq("2"), StringEq("3")));

  // Assign a subrange of `strs`.
  proxy.assign(strs.begin() + 1, strs.end());
  EXPECT_THAT(proxy, ElementsAre(StringEq("3")));
  EXPECT_THAT(*field, ElementsAre(StringEq("3")));
}

TYPED_TEST(RepeatedFieldProxyTest, AssignMessageIteratorRange) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);

  std::vector<RepeatedFieldProxyTestSimpleMessage> msgs(2);
  msgs[0].set_value(2);
  msgs[1].set_value(3);

  auto proxy = field.MakeProxy();
  proxy.assign(msgs.begin(), msgs.end());

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));

  // Assign a subrange of `msgs`.
  proxy.assign(msgs.begin() + 1, msgs.end());
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 3)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, MoveAssign) {
  auto field1 = this->MakeRepeatedFieldContainer();
  field1->Add(1);

  auto field2 = this->MakeRepeatedFieldContainer();
  field2->Add(2);
  field2->Add(3);
  field2->Add(4);

  const void* prev_data = field2->data();

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();

  if (this->UseArena()) {
    size_t arena_memory = this->arena()->SpaceUsed();
    proxy1.move_assign(proxy2);
    // Verify that the move assignment did not allocate any additional memory.
    EXPECT_EQ(this->arena()->SpaceUsed(), arena_memory);
  } else {
    proxy1.move_assign(proxy2);
  }

  // Verify that the move assignment did not allocate any additional memory.
  EXPECT_EQ(field1->data(), prev_data);

  EXPECT_THAT(proxy1, ElementsAre(2, 3, 4));
  EXPECT_THAT(*field1, ElementsAre(2, 3, 4));
}

TYPED_TEST(RepeatedStringFieldProxyTest, MoveAssign) {
  auto field1 = this->MakeRepeatedFieldContainer();
  this->Add(field1, "1");

  auto field2 = this->MakeRepeatedFieldContainer();
  this->Add(field2, "2");
  this->Add(field2, "3");

  const void* prev_data = field2->data();

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();

  if (this->UseArena()) {
    size_t arena_memory = this->arena()->SpaceUsed();
    proxy1.move_assign(proxy2);
    // Verify that the move assignment did not allocate any additional memory.
    EXPECT_EQ(this->arena()->SpaceUsed(), arena_memory);
  } else {
    proxy1.move_assign(proxy2);
  }

  // Verify that the move assignment did not allocate any additional memory.
  EXPECT_EQ(field1->data(), prev_data);

  EXPECT_THAT(proxy1, ElementsAre(StringEq("2"), StringEq("3")));
  EXPECT_THAT(*field1, ElementsAre(StringEq("2"), StringEq("3")));
}

TYPED_TEST(RepeatedFieldProxyTest, MoveAssignMessage) {
  auto field1 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);
  field2->Add()->set_value(3);

  const void* prev_data = field2->data();

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();

  if (this->UseArena()) {
    size_t arena_memory = this->arena()->SpaceUsed();
    proxy1.move_assign(proxy2);
    // Verify that the move assignment did not allocate any additional memory.
    EXPECT_EQ(this->arena()->SpaceUsed(), arena_memory);
  } else {
    proxy1.move_assign(proxy2);
  }

  // Verify that the move assignment did not allocate any additional memory.
  EXPECT_EQ(field1->data(), prev_data);

  EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                   EqualsProto(R"pb(value: 3)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Reserve) {
  auto field = this->MakeRepeatedFieldContainer();

  auto proxy = field.MakeProxy();
  proxy.reserve(10);
  const void* data_before = field->data();

  uint64_t space_used_before = 0;
  if (this->arena() != nullptr) {
    space_used_before = this->arena()->SpaceUsed();
  }

  for (int i = 0; i < 10; ++i) {
    proxy.emplace_back(i);
  }

  // Verify that reserve() prevented resizing.
  EXPECT_EQ(field->data(), data_before);

  EXPECT_THAT(proxy, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
  EXPECT_THAT(*field, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));

  if (this->arena() != nullptr) {
    // In the arena case, we verify that no additional memory was allocated
    // after the initial reserve().
    EXPECT_EQ(space_used_before, this->arena()->SpaceUsed());
  }
}

TYPED_TEST(RepeatedStringFieldProxyTest, Reserve) {
  auto field = this->MakeRepeatedFieldContainer();

  auto proxy = field.MakeProxy();
  proxy.reserve(10);
  const void* data_before = field->data();

  for (int i = 0; i < 10; ++i) {
    proxy.emplace_back(absl::StrFormat("%" PRId32, i));
  }

  // Verify that reserve() prevented resizing.
  EXPECT_EQ(field->data(), data_before);

  EXPECT_THAT(proxy, ElementsAre(StringEq("0"), StringEq("1"), StringEq("2"),
                                 StringEq("3"), StringEq("4"), StringEq("5"),
                                 StringEq("6"), StringEq("7"), StringEq("8"),
                                 StringEq("9")));
  EXPECT_THAT(*field, ElementsAre(StringEq("0"), StringEq("1"), StringEq("2"),
                                  StringEq("3"), StringEq("4"), StringEq("5"),
                                  StringEq("6"), StringEq("7"), StringEq("8"),
                                  StringEq("9")));
}

TYPED_TEST(RepeatedFieldProxyTest, ReserveMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();

  auto proxy = field.MakeProxy();
  proxy.reserve(10);
  const void* data_before = field->data();

  for (int i = 0; i < 10; ++i) {
    proxy.emplace_back().set_value(i);
  }

  // Verify that reserve() prevented resizing.
  EXPECT_EQ(field->data(), data_before);

  EXPECT_THAT(
      proxy,
      ElementsAre(
          EqualsProto(R"pb(value: 0)pb"), EqualsProto(R"pb(value: 1)pb"),
          EqualsProto(R"pb(value: 2)pb"), EqualsProto(R"pb(value: 3)pb"),
          EqualsProto(R"pb(value: 4)pb"), EqualsProto(R"pb(value: 5)pb"),
          EqualsProto(R"pb(value: 6)pb"), EqualsProto(R"pb(value: 7)pb"),
          EqualsProto(R"pb(value: 8)pb"), EqualsProto(R"pb(value: 9)pb")));
  EXPECT_THAT(
      *field,
      ElementsAre(
          EqualsProto(R"pb(value: 0)pb"), EqualsProto(R"pb(value: 1)pb"),
          EqualsProto(R"pb(value: 2)pb"), EqualsProto(R"pb(value: 3)pb"),
          EqualsProto(R"pb(value: 4)pb"), EqualsProto(R"pb(value: 5)pb"),
          EqualsProto(R"pb(value: 6)pb"), EqualsProto(R"pb(value: 7)pb"),
          EqualsProto(R"pb(value: 8)pb"), EqualsProto(R"pb(value: 9)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Swap) {
  auto field1 = this->MakeRepeatedFieldContainer();
  field1->Add(1);

  auto field2 = this->MakeRepeatedFieldContainer();
  field2->Add(2);
  field2->Add(3);

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.swap(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(2, 3));
  EXPECT_THAT(proxy2, ElementsAre(1));
}

TYPED_TEST(RepeatedStringFieldProxyTest, Swap) {
  auto field1 = this->MakeRepeatedFieldContainer();
  this->Add(field1, "1");

  auto field2 = this->MakeRepeatedFieldContainer();
  this->Add(field2, "2");
  this->Add(field2, "3");

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.swap(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(StringEq("2"), StringEq("3")));
  EXPECT_THAT(proxy2, ElementsAre(StringEq("1")));
}

TYPED_TEST(RepeatedFieldProxyTest, SwapMessage) {
  auto field1 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);
  field2->Add()->set_value(3);

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.swap(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(proxy2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, Resize) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);

  auto proxy = field.MakeProxy();
  proxy.resize(4);

  EXPECT_THAT(proxy, ElementsAre(1, 2, 0, 0));
  EXPECT_THAT(*field, ElementsAre(1, 2, 0, 0));

  proxy.resize(1);
  EXPECT_THAT(proxy, ElementsAre(1));
  EXPECT_THAT(*field, ElementsAre(1));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, ResizeWithValue) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);

  auto proxy = field.MakeProxy();
  proxy.resize(3, 10);

  EXPECT_THAT(proxy, ElementsAre(1, 10, 10));
  EXPECT_THAT(*field, ElementsAre(1, 10, 10));

  proxy.resize(2, 20);
  EXPECT_THAT(proxy, ElementsAre(1, 10));
  EXPECT_THAT(*field, ElementsAre(1, 10));
}

TYPED_TEST(RepeatedStringFieldProxyTest, Resize) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");

  auto proxy = field.MakeProxy();
  proxy.resize(4);

  EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("2"), StringEq(""),
                                 StringEq("")));
  EXPECT_THAT(*field, ElementsAre(StringEq("1"), StringEq("2"), StringEq(""),
                                  StringEq("")));

  proxy.resize(1);
  EXPECT_THAT(proxy, ElementsAre(StringEq("1")));
  EXPECT_THAT(*field, ElementsAre(StringEq("1")));
}

TYPED_TEST(RepeatedStringFieldProxyTest, ResizeWithValue) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");

  auto proxy = field.MakeProxy();
  if constexpr (std::is_same_v<ElementType, absl::Cord>) {
    const auto value = absl::Cord("10");
    proxy.resize(3, value);
  } else {
    proxy.resize(3, "10");
  }

  EXPECT_THAT(proxy,
              ElementsAre(StringEq("1"), StringEq("10"), StringEq("10")));
  EXPECT_THAT(*field,
              ElementsAre(StringEq("1"), StringEq("10"), StringEq("10")));

  if constexpr (std::is_same_v<ElementType, absl::Cord>) {
    proxy.resize(2, absl::Cord("20"));
  } else {
    proxy.resize(2, "20");
  }
  EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("10")));
  EXPECT_THAT(*field, ElementsAre(StringEq("1"), StringEq("10")));
}

TYPED_TEST(RepeatedFieldProxyTest, ResizeMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  auto proxy = field.MakeProxy();
  proxy.resize(4);

  EXPECT_THAT(
      proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                         EqualsProto(R"pb(value: 2)pb"), EqualsProto(R"pb()pb"),
                         EqualsProto(R"pb()pb")));
  EXPECT_THAT(*field,
              ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                          EqualsProto(R"pb(value: 2)pb"),
                          EqualsProto(R"pb()pb"), EqualsProto(R"pb()pb")));

  proxy.resize(1);
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TYPED_TEST(RepeatedFieldProxyTest, ResizeMessageWithValue) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);

  auto proxy = field.MakeProxy();
  RepeatedFieldProxyTestSimpleMessage msg;
  msg.set_value(10);
  proxy.resize(3, msg);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 10)pb"),
                                 EqualsProto(R"pb(value: 10)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 10)pb"),
                                  EqualsProto(R"pb(value: 10)pb")));

  RepeatedFieldProxyTestSimpleMessage msg2;
  msg2.set_value(20);
  proxy.resize(2, msg2);
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 10)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 10)pb")));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, RebindConstProxy) {
  auto field1 = this->MakeRepeatedFieldContainer();
  field1->Add(1);

  auto field2 = this->MakeRepeatedFieldContainer();
  field2->Add(2);

  auto proxy = field1.MakeConstProxy();
  proxy = field2.MakeConstProxy();

  // Proxy should be rebound to field2 without having modified either field.
  EXPECT_THAT(proxy, ElementsAre(2));
  EXPECT_THAT(*field1, ElementsAre(1));
  EXPECT_THAT(*field2, ElementsAre(2));

  // Verify that mutable proxies can't be rebound.
  static_assert(!std::is_copy_assignable_v<decltype(field1.MakeProxy())>);
}

TYPED_TEST(RepeatedStringFieldProxyTest, RebindConstProxy) {
  auto field1 = this->MakeRepeatedFieldContainer();
  this->Add(field1, "1");

  auto field2 = this->MakeRepeatedFieldContainer();
  this->Add(field2, "2");

  auto proxy = field1.MakeConstProxy();
  proxy = field2.MakeConstProxy();

  // Proxy should be rebound to field2 without having modified either field.
  EXPECT_THAT(proxy, ElementsAre(StringEq("2")));
  EXPECT_THAT(*field1, ElementsAre(StringEq("1")));
  EXPECT_THAT(*field2, ElementsAre(StringEq("2")));

  // Verify that mutable proxies can't be rebound.
  static_assert(!std::is_copy_assignable_v<decltype(field1.MakeProxy())>);
}

TYPED_TEST(RepeatedFieldProxyTest, RebindConstMessageProxy) {
  auto field1 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);

  auto proxy = field1.MakeConstProxy();
  proxy = field2.MakeConstProxy();

  // Proxy should be rebound to field2 without having modified either field.
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
  EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
  EXPECT_THAT(*field2, ElementsAre(EqualsProto(R"pb(value: 2)pb")));

  // Verify that mutable proxies can't be rebound.
  static_assert(!std::is_copy_assignable_v<decltype(field1.MakeProxy())>);
}

TYPED_TEST(RepeatedFieldProxyTest, ExplicitConversionToLegacyRepeatedField) {
  auto field = this->template MakeRepeatedFieldContainer<int>();
  field->Add(1);

  auto proxy = field.MakeProxy();
  // Make an explicit conversion to RepeatedField. This will perform a deep
  // copy.
  RepeatedField<int> field2 = RepeatedField<int>(proxy);
  EXPECT_THAT(field2, ElementsAre(1));

  // `field2` should be a copy of the original field.
  EXPECT_NE(&field2.Get(0), &field->Get(0));
}

TYPED_TEST(RepeatedFieldProxyTest, ExplicitConversionToLegacyRepeatedPtrField) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);

  auto proxy = field.MakeProxy();
  // Make an explicit conversion to RepeatedPtrField. This will perform a deep
  // copy.
  RepeatedPtrField<RepeatedFieldProxyTestSimpleMessage> field2 =
      RepeatedPtrField<RepeatedFieldProxyTestSimpleMessage>(proxy);
  EXPECT_THAT(field2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));

  // `field2` field should be a copy of the original field.
  EXPECT_NE(&field2.Get(0), &field->Get(0));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, CSort) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(4);
  field->Add(1);
  field->Add(3);
  field->Add(2);

  auto proxy = field.MakeProxy();
  google::protobuf::c_sort(proxy);
  EXPECT_THAT(proxy, ElementsAre(1, 2, 3, 4));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3, 4));

  google::protobuf::c_sort(proxy, [this, &field](auto&& lhs, auto&& rhs) {
    this->template VerifyLambdaTypeRequirements<decltype(lhs)>(field, lhs);
    this->template VerifyLambdaTypeRequirements<decltype(rhs)>(field, rhs);
    return std::greater{}(lhs, rhs);
  });
  EXPECT_THAT(proxy, ElementsAre(4, 3, 2, 1));
  EXPECT_THAT(*field, ElementsAre(4, 3, 2, 1));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, StableCSort) {
  auto field = this->MakeRepeatedFieldContainer();
  field->Add(4);
  field->Add(1);
  field->Add(3);
  field->Add(2);

  auto proxy = field.MakeProxy();
  google::protobuf::c_stable_sort(proxy);
  EXPECT_THAT(proxy, ElementsAre(1, 2, 3, 4));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3, 4));

  google::protobuf::c_stable_sort(proxy, [this, &field](auto&& lhs, auto&& rhs) {
    this->template VerifyLambdaTypeRequirements<decltype(lhs)>(field, lhs);
    this->template VerifyLambdaTypeRequirements<decltype(rhs)>(field, rhs);
    return std::greater{}(lhs, rhs);
  });
  EXPECT_THAT(proxy, ElementsAre(4, 3, 2, 1));
  EXPECT_THAT(*field, ElementsAre(4, 3, 2, 1));
}

TYPED_TEST(RepeatedStringFieldProxyTest, CSort) {
  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "d");
  this->Add(field, "a");
  this->Add(field, "c");
  this->Add(field, "b");

  auto proxy = field.MakeProxy();
  google::protobuf::c_sort(proxy);
  EXPECT_THAT(proxy, ElementsAre(StringEq("a"), StringEq("b"), StringEq("c"),
                                 StringEq("d")));
  EXPECT_THAT(*field, ElementsAre(StringEq("a"), StringEq("b"), StringEq("c"),
                                  StringEq("d")));

  google::protobuf::c_sort(proxy, this->StringCompare(std::greater{}));
  EXPECT_THAT(proxy, ElementsAre(StringEq("d"), StringEq("c"), StringEq("b"),
                                 StringEq("a")));
  EXPECT_THAT(*field, ElementsAre(StringEq("d"), StringEq("c"), StringEq("b"),
                                  StringEq("a")));
}

TYPED_TEST(RepeatedStringFieldProxyTest, StableCSort) {
  auto field = this->MakeRepeatedFieldContainer();
  // Use long strings to ensure the backing string does not use SSO, and we can
  // compare `.data()` before and after stable sorting to determine relative
  // order of equivalent strings.
  const auto long_string_a = absl::StrCat("a", kLongString);
  const auto long_string_b = absl::StrCat("b", kLongString);
  this->Add(field, long_string_b);
  this->Add(field, long_string_b);
  this->Add(field, long_string_a);
  this->Add(field, long_string_a);

  const char* str1 = this->StartAddress(field->Get(0));
  const char* str2 = this->StartAddress(field->Get(1));
  const char* str3 = this->StartAddress(field->Get(2));
  const char* str4 = this->StartAddress(field->Get(3));

  auto proxy = field.MakeProxy();
  google::protobuf::c_stable_sort(proxy);
  EXPECT_THAT(proxy,
              ElementsAre(StringEq(long_string_a), StringEq(long_string_a),
                          StringEq(long_string_b), StringEq(long_string_b)));
  EXPECT_THAT(*field,
              ElementsAre(StringEq(long_string_a), StringEq(long_string_a),
                          StringEq(long_string_b), StringEq(long_string_b)));

  // Stable sort should preserve the relative order of elements that compare
  // equal.
  EXPECT_EQ(this->StartAddress(field->Get(0)), str3);
  EXPECT_EQ(this->StartAddress(field->Get(1)), str4);
  EXPECT_EQ(this->StartAddress(field->Get(2)), str1);
  EXPECT_EQ(this->StartAddress(field->Get(3)), str2);

  google::protobuf::c_stable_sort(proxy, this->StringCompare(std::greater{}));
  EXPECT_THAT(proxy,
              ElementsAre(StringEq(long_string_b), StringEq(long_string_b),
                          StringEq(long_string_a), StringEq(long_string_a)));
  EXPECT_THAT(*field,
              ElementsAre(StringEq(long_string_b), StringEq(long_string_b),
                          StringEq(long_string_a), StringEq(long_string_a)));

  // Stable sort should preserve the relative order of elements that compare
  // equal.
  EXPECT_EQ(this->StartAddress(field->Get(0)), str1);
  EXPECT_EQ(this->StartAddress(field->Get(1)), str2);
  EXPECT_EQ(this->StartAddress(field->Get(2)), str3);
  EXPECT_EQ(this->StartAddress(field->Get(3)), str4);
}

TYPED_TEST(RepeatedFieldProxyTest, CSortMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(4);
  field->Add()->set_value(1);
  field->Add()->set_value(3);
  field->Add()->set_value(2);

  auto proxy = field.MakeProxy();
  google::protobuf::c_sort(proxy, [](const auto& a, const auto& b) {
    return a.value() < b.value();
  });
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb"),
                                 EqualsProto(R"pb(value: 4)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb"),
                                  EqualsProto(R"pb(value: 4)pb")));
}

TYPED_TEST(RepeatedFieldProxyTest, StableCSortMessage) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(2);
  field->Add()->set_value(2);
  field->Add()->set_value(1);
  field->Add()->set_value(1);

  const auto* msg1 = &field->Get(0);
  const auto* msg2 = &field->Get(1);
  const auto* msg3 = &field->Get(2);
  const auto* msg4 = &field->Get(3);

  auto proxy = field.MakeProxy();
  google::protobuf::c_stable_sort(proxy, [](const auto& a, const auto& b) {
    return a.value() < b.value();
  });
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 2)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 2)pb")));

  // Stable sort should preserve the relative order of elements that compare
  // equal.
  EXPECT_EQ(&field->Get(0), msg3);
  EXPECT_EQ(&field->Get(1), msg4);
  EXPECT_EQ(&field->Get(2), msg1);
  EXPECT_EQ(&field->Get(3), msg2);
}

TYPED_TEST(RepeatedNumericFieldProxyTest,
           RepeatedFieldOrProxyImplicitConversion) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);

  {
    // Implicit conversion from RepeatedPtrField to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<ElementType> proxy = *field;
    EXPECT_THAT(proxy, ElementsAre(1, 2));
  }

  {
    // Implicit conversion from RepeatedPtrField* to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<ElementType> proxy = &*field;
    EXPECT_THAT(proxy, ElementsAre(1, 2));
  }

  {
    // Implicit conversion from RepeatedPtrField to const RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const ElementType> proxy = *field;
    EXPECT_THAT(proxy, ElementsAre(1, 2));
  }

  {
    // Implicit conversion from const RepeatedPtrField to const
    // RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const ElementType> proxy = std::as_const(*field);
    EXPECT_THAT(proxy, ElementsAre(1, 2));
  }

  {
    // Implicit conversion from RepeatedFieldProxy to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<ElementType> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(1, 2));
  }

  {
    // Implicit conversion from RepeatedFieldProxy to const
    // RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const ElementType> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(1, 2));
  }
}

TYPED_TEST(RepeatedStringFieldProxyTest,
           RepeatedFieldOrProxyImplicitConversion) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");

  {
    // Implicit conversion from RepeatedPtrField to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<ElementType> proxy = *field;
    EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("2")));
  }

  {
    // Implicit conversion from RepeatedPtrField* to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<ElementType> proxy = &*field;
    EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("2")));
  }

  {
    // Implicit conversion from RepeatedPtrField to const RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const ElementType> proxy = *field;
    EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("2")));
  }

  {
    // Implicit conversion from const RepeatedPtrField to const
    // RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const ElementType> proxy = std::as_const(*field);
    EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("2")));
  }

  {
    // Implicit conversion from RepeatedFieldProxy to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<ElementType> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("2")));
  }

  {
    // Implicit conversion from RepeatedFieldProxy to const
    // RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const ElementType> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(StringEq("1"), StringEq("2")));
  }
}

TYPED_TEST(RepeatedFieldProxyTest, RepeatedFieldOrProxyImplicitConversion) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  {
    // Implicit conversion from RepeatedPtrField to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy = *field;
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }

  {
    // Implicit conversion from RepeatedPtrField* to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy = &*field;
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }

  {
    // Implicit conversion from RepeatedPtrField to const RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        *field;
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }

  {
    // Implicit conversion from const RepeatedPtrField to const
    // RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        std::as_const(*field);
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }

  {
    // Implicit conversion from RepeatedFieldProxy to RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }

  {
    // Implicit conversion from RepeatedFieldProxy to const
    // RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }
}

TYPED_TEST(RepeatedNumericFieldProxyTest,
           RepeatedFieldOrProxyExplicitConversionToLegacy) {
  using ElementType = typename TypeParam::ElementType;

  auto field = this->MakeRepeatedFieldContainer();
  field->Add(1);
  field->Add(2);

  RepeatedFieldOrProxy<ElementType> proxy = *field;
  auto legacy = google::protobuf::RepeatedField<ElementType>(proxy);
  EXPECT_THAT(legacy, ElementsAre(1, 2));

  // `legacy` field should be a copy of the original field.
  EXPECT_NE(&legacy.Get(0), &field->Get(0));
}

TYPED_TEST(RepeatedStringFieldProxyTest,
           RepeatedFieldOrProxyExplicitConversionToLegacy) {
  using ElementType = typename TypeParam::ElementType;
  using LegacyType = typename RepeatedFieldTraits<ElementType>::type;

  auto field = this->MakeRepeatedFieldContainer();
  this->Add(field, "1");
  this->Add(field, "2");

  RepeatedFieldOrProxy<ElementType> proxy = *field;
  auto legacy = LegacyType(proxy);
  EXPECT_THAT(legacy, ElementsAre(StringEq("1"), StringEq("2")));

  // `legacy` field should be a copy of the original field.
  EXPECT_NE(&legacy.Get(0), &field->Get(0));
}

TYPED_TEST(RepeatedFieldProxyTest,
           RepeatedFieldOrProxyExplicitConversionToLegacy) {
  auto field = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy = *field;
  auto legacy =
      google::protobuf::RepeatedPtrField<RepeatedFieldProxyTestSimpleMessage>(proxy);
  EXPECT_THAT(legacy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb")));

  // `legacy` field should be a copy of the original field.
  EXPECT_NE(&legacy.Get(0), &field->Get(0));
}

TYPED_TEST(RepeatedNumericFieldProxyTest, RepeatedFieldOrProxyAssign) {
  using ElementType = typename TypeParam::ElementType;

  auto field1 = this->MakeRepeatedFieldContainer();
  field1->Add(1);

  auto field2 = this->MakeRepeatedFieldContainer();
  field2->Add(2);

  {
    // Assign a RepeatedFieldOrProxy to a RepeatedFieldOrProxy:
    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    RepeatedFieldOrProxy<ElementType> proxy2 = *field2;
    proxy1.assign(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(2));
    EXPECT_THAT(*field1, ElementsAre(2));
    EXPECT_THAT(proxy2, ElementsAre(2));
    EXPECT_THAT(*field2, ElementsAre(2));
  }

  field2->Add(3);

  {
    // Assign a RepeatedField to a RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    proxy1.assign(*field2);
    EXPECT_THAT(proxy1, ElementsAre(2, 3));
    EXPECT_THAT(*field1, ElementsAre(2, 3));
    EXPECT_THAT(*field2, ElementsAre(2, 3));
  }
}

TYPED_TEST(RepeatedStringFieldProxyTest, RepeatedFieldOrProxyAssign) {
  using ElementType = typename TypeParam::ElementType;

  auto field1 = this->MakeRepeatedFieldContainer();
  this->Add(field1, "1");

  auto field2 = this->MakeRepeatedFieldContainer();
  this->Add(field2, "2");

  {
    // Assign a RepeatedFieldOrProxy to a RepeatedFieldOrProxy:
    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    RepeatedFieldOrProxy<ElementType> proxy2 = *field2;
    proxy1.assign(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(StringEq("2")));
    EXPECT_THAT(*field1, ElementsAre(StringEq("2")));
    EXPECT_THAT(proxy2, ElementsAre(StringEq("2")));
    EXPECT_THAT(*field2, ElementsAre(StringEq("2")));
  }

  this->Add(field2, "3");

  {
    // Assign a RepeatedField to a RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    proxy1.assign(*field2);
    EXPECT_THAT(proxy1, ElementsAre(StringEq("2"), StringEq("3")));
    EXPECT_THAT(*field1, ElementsAre(StringEq("2"), StringEq("3")));
    EXPECT_THAT(*field2, ElementsAre(StringEq("2"), StringEq("3")));
  }
}

TYPED_TEST(RepeatedFieldProxyTest, RepeatedFieldOrProxyAssign) {
  auto field1 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);

  {
    // Assign a RepeatedFieldOrProxy to a RepeatedFieldOrProxy:
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy1 = *field1;
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy2 = *field2;
    proxy1.assign(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
    EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
    EXPECT_THAT(proxy2, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
    EXPECT_THAT(*field2, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
  }

  field2->Add()->set_value(3);

  {
    // Assign a RepeatedField to a RepeatedFieldOrProxy.
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy1 = *field1;
    proxy1.assign(*field2);
    EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                    EqualsProto(R"pb(value: 3)pb")));
    EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                     EqualsProto(R"pb(value: 3)pb")));
    EXPECT_THAT(*field2, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                     EqualsProto(R"pb(value: 3)pb")));
  }
}

TYPED_TEST(RepeatedNumericFieldProxyTest, RepeatedFieldOrProxyMoveAssign) {
  using ElementType = typename TypeParam::ElementType;

  auto field1 = this->MakeRepeatedFieldContainer();
  field1->Add(1);

  {
    // Move-assign a RepeatedFieldOrProxy to a RepeatedFieldOrProxy:
    auto field2 = this->MakeRepeatedFieldContainer();
    field2->Add(2);

    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    RepeatedFieldOrProxy<ElementType> proxy2 = *field2;
    proxy1.move_assign(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(2));
    EXPECT_THAT(*field1, ElementsAre(2));
  }

  {
    // Move-assign a RepeatedField to a RepeatedFieldOrProxy.
    auto field2 = this->MakeRepeatedFieldContainer();
    field2->Add(3);

    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    proxy1.move_assign(*field2);
    EXPECT_THAT(proxy1, ElementsAre(3));
    EXPECT_THAT(*field1, ElementsAre(3));
  }
}

TYPED_TEST(RepeatedStringFieldProxyTest, RepeatedFieldOrProxyMoveAssign) {
  using ElementType = typename TypeParam::ElementType;

  auto field1 = this->MakeRepeatedFieldContainer();
  this->Add(field1, "1");

  {
    // Move-assign a RepeatedFieldOrProxy to a RepeatedFieldOrProxy:
    auto field2 = this->MakeRepeatedFieldContainer();
    this->Add(field2, "2");

    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    RepeatedFieldOrProxy<ElementType> proxy2 = *field2;
    proxy1.move_assign(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(StringEq("2")));
    EXPECT_THAT(*field1, ElementsAre(StringEq("2")));
  }

  {
    // Move-assign a RepeatedField to a RepeatedFieldOrProxy.
    auto field2 = this->MakeRepeatedFieldContainer();
    this->Add(field2, "3");

    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    proxy1.move_assign(*field2);
    EXPECT_THAT(proxy1, ElementsAre(StringEq("3")));
    EXPECT_THAT(*field1, ElementsAre(StringEq("3")));
  }
}

TYPED_TEST(RepeatedFieldProxyTest, RepeatedFieldOrProxyMoveAssign) {
  auto field1 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  {
    // Move-assign a RepeatedFieldOrProxy to a RepeatedFieldOrProxy:
    auto field2 = this->template MakeRepeatedFieldContainer<
        RepeatedFieldProxyTestSimpleMessage>();
    field2->Add()->set_value(2);

    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy1 = *field1;
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy2 = *field2;
    proxy1.move_assign(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
    EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
  }

  {
    // Move-assign a RepeatedField to a RepeatedFieldOrProxy.
    auto field2 = this->template MakeRepeatedFieldContainer<
        RepeatedFieldProxyTestSimpleMessage>();
    field2->Add()->set_value(3);

    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy1 = *field1;
    proxy1.move_assign(*field2);
    EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 3)pb")));
    EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 3)pb")));
  }
}

TYPED_TEST(RepeatedNumericFieldProxyTest, RepeatedFieldOrProxySwap) {
  using ElementType = typename TypeParam::ElementType;

  auto field1 = this->MakeRepeatedFieldContainer();
  field1->Add(1);

  auto field2 = this->MakeRepeatedFieldContainer();
  field2->Add(2);

  {
    // Swap two RepeatedFieldOrProxies:
    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    RepeatedFieldOrProxy<ElementType> proxy2 = *field2;
    proxy1.swap(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(2));
    EXPECT_THAT(*field1, ElementsAre(2));
    EXPECT_THAT(proxy2, ElementsAre(1));
    EXPECT_THAT(*field2, ElementsAre(1));
  }

  {
    // Swap a RepeatedFieldOrProxy with a RepeatedField.
    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    proxy1.swap(*field2);
    EXPECT_THAT(proxy1, ElementsAre(1));
    EXPECT_THAT(*field1, ElementsAre(1));
    EXPECT_THAT(*field2, ElementsAre(2));
  }
}

TYPED_TEST(RepeatedStringFieldProxyTest, RepeatedFieldOrProxySwap) {
  using ElementType = typename TypeParam::ElementType;

  auto field1 = this->MakeRepeatedFieldContainer();
  this->Add(field1, "1");

  auto field2 = this->MakeRepeatedFieldContainer();
  this->Add(field2, "2");

  {
    // Swap two RepeatedFieldOrProxies:
    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    RepeatedFieldOrProxy<ElementType> proxy2 = *field2;
    proxy1.swap(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(StringEq("2")));
    EXPECT_THAT(*field1, ElementsAre(StringEq("2")));
    EXPECT_THAT(proxy2, ElementsAre(StringEq("1")));
    EXPECT_THAT(*field2, ElementsAre(StringEq("1")));
  }

  {
    // Swap a RepeatedFieldOrProxy with a RepeatedField.
    RepeatedFieldOrProxy<ElementType> proxy1 = *field1;
    proxy1.swap(*field2);
    EXPECT_THAT(proxy1, ElementsAre(StringEq("1")));
    EXPECT_THAT(*field1, ElementsAre(StringEq("1")));
    EXPECT_THAT(*field2, ElementsAre(StringEq("2")));
  }
}

TYPED_TEST(RepeatedFieldProxyTest, RepeatedFieldOrProxySwap) {
  auto field1 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 = this->template MakeRepeatedFieldContainer<
      RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);

  {
    // Swap two RepeatedFieldOrProxies:
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy1 = *field1;
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy2 = *field2;
    proxy1.swap(proxy2);
    EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
    EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
    EXPECT_THAT(proxy2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
    EXPECT_THAT(*field2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
  }

  {
    // Swap a RepeatedFieldOrProxy with a RepeatedField.
    RepeatedFieldOrProxy<RepeatedFieldProxyTestSimpleMessage> proxy1 = *field1;
    proxy1.swap(*field2);
    EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
    EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
    EXPECT_THAT(*field2, ElementsAre(EqualsProto(R"pb(value: 2)pb")));
  }
}

// Verify the return types of all accessors for legacy and proxy repeated
// fields:

// Repeated messages:
static_assert(
    std::is_same_v<
        decltype(std::declval<TestRepeatedMessageProxy>().nested_messages()),
        const RepeatedPtrField<TestRepeatedMessageProxy::NestedMessage>&>);
static_assert(
    std::is_same_v<decltype(std::declval<TestRepeatedMessageProxy>()
                                .mutable_nested_messages()),
                   RepeatedPtrField<TestRepeatedMessageProxy::NestedMessage>*>);

static_assert(
    std::is_same_v<
        decltype(std::declval<TestRepeatedMessageProxy>()
                     .nested_messages_proxy()),
        RepeatedFieldProxy<const TestRepeatedMessageProxy::NestedMessage>>);
static_assert(std::is_same_v<
              decltype(std::declval<TestRepeatedMessageProxy>()
                           .mutable_nested_messages_proxy()),
              RepeatedFieldProxy<TestRepeatedMessageProxy::NestedMessage>>);

// Repeated ints:
static_assert(
    std::is_same_v<decltype(std::declval<TestRepeatedIntProxy>().ints()),
                   const RepeatedField<int32_t>&>);
static_assert(std::is_same_v<
              decltype(std::declval<TestRepeatedIntProxy>().mutable_ints()),
              RepeatedField<int32_t>*>);

static_assert(
    std::is_same_v<decltype(std::declval<TestRepeatedIntProxy>().ints_proxy()),
                   RepeatedFieldProxy<const int32_t>>);
static_assert(std::is_same_v<decltype(std::declval<TestRepeatedIntProxy>()
                                          .mutable_ints_proxy()),
                             RepeatedFieldProxy<int32_t>>);

// Repeated enums:
static_assert(
    std::is_same_v<decltype(std::declval<TestRepeatedEnumProxy>().enums()),
                   const RepeatedField<int>&>);
static_assert(std::is_same_v<
              decltype(std::declval<TestRepeatedEnumProxy>().mutable_enums()),
              RepeatedField<int>*>);

static_assert(std::is_same_v<
              decltype(std::declval<TestRepeatedEnumProxy>().enums_proxy()),
              RepeatedFieldProxy<const TestRepeatedEnumProxy::Enum>>);
static_assert(std::is_same_v<decltype(std::declval<TestRepeatedEnumProxy>()
                                          .mutable_enums_proxy()),
                             RepeatedFieldProxy<TestRepeatedEnumProxy::Enum>>);

// Repeated std::string:
static_assert(std::is_same_v<
              decltype(std::declval<TestRepeatedStdStringProxy>().strings()),
              const RepeatedPtrField<std::string>&>);
static_assert(std::is_same_v<decltype(std::declval<TestRepeatedStdStringProxy>()
                                          .mutable_strings()),
                             RepeatedPtrField<std::string>*>);

static_assert(std::is_same_v<decltype(std::declval<TestRepeatedStdStringProxy>()
                                          .strings_proxy()),
                             RepeatedFieldProxy<const std::string>>);
static_assert(std::is_same_v<decltype(std::declval<TestRepeatedStdStringProxy>()
                                          .mutable_strings_proxy()),
                             RepeatedFieldProxy<std::string>>);

// Repeated absl::string_view:
static_assert(
    std::is_same_v<
        decltype(std::declval<TestRepeatedStringViewProxy>().string_views()),
        const RepeatedPtrField<std::string>&>);
static_assert(
    std::is_same_v<decltype(std::declval<TestRepeatedStringViewProxy>()
                                .mutable_string_views()),
                   RepeatedPtrField<std::string>*>);

static_assert(
    std::is_same_v<decltype(std::declval<TestRepeatedStringViewProxy>()
                                .string_views_proxy()),
                   RepeatedFieldProxy<const absl::string_view>>);
static_assert(
    std::is_same_v<decltype(std::declval<TestRepeatedStringViewProxy>()
                                .mutable_string_views_proxy()),
                   RepeatedFieldProxy<absl::string_view>>);

TEST(RepeatedFieldProxyInterfaceTest, RepeatedMessageProxy) {
  TestRepeatedMessageProxy msg;
  {
    auto proxy = msg.mutable_nested_messages_proxy();
    proxy.emplace_back().set_value(1);
    proxy.emplace_back().set_value(2);
    proxy.emplace_back().set_value(3);
  }

  auto proxy = msg.nested_messages_proxy();
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedImportMessageProxy) {
  TestRepeatedImportMessageProxy msg;
  {
    auto proxy = msg.mutable_import_messages_proxy();
    proxy.emplace_back().set_value(1);
    proxy.emplace_back().set_value(2);
    proxy.emplace_back().set_value(3);
  }

  auto proxy = msg.import_messages_proxy();
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedIntProxy) {
  TestRepeatedIntProxy msg;
  {
    auto proxy = msg.mutable_ints_proxy();
    proxy.push_back(1);
    proxy.push_back(2);
    proxy.push_back(3);
  }

  auto proxy = msg.ints_proxy();
  EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedEnumProxy) {
  TestRepeatedEnumProxy msg;
  {
    auto proxy = msg.mutable_enums_proxy();
    proxy.push_back(TestRepeatedEnumProxy::FOO);
    proxy.push_back(TestRepeatedEnumProxy::BAR);
    proxy.push_back(TestRepeatedEnumProxy::BAZ);
  }

  auto proxy = msg.enums_proxy();
  EXPECT_THAT(
      proxy, ElementsAre(TestRepeatedEnumProxy::FOO, TestRepeatedEnumProxy::BAR,
                         TestRepeatedEnumProxy::BAZ));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedImportEnumProxy) {
  TestRepeatedImportEnumProxy msg;
  {
    auto proxy = msg.mutable_enums_proxy();
    proxy.push_back(
        RepeatedFieldProxyTestImportEnum::REPEATED_FIELD_PROXY_TEST_IMPORT_FOO);
    proxy.push_back(
        RepeatedFieldProxyTestImportEnum::REPEATED_FIELD_PROXY_TEST_IMPORT_BAR);
    proxy.push_back(
        RepeatedFieldProxyTestImportEnum::REPEATED_FIELD_PROXY_TEST_IMPORT_BAZ);
  }

  auto proxy = msg.enums_proxy();
  EXPECT_THAT(proxy, ElementsAre(RepeatedFieldProxyTestImportEnum::
                                     REPEATED_FIELD_PROXY_TEST_IMPORT_FOO,
                                 RepeatedFieldProxyTestImportEnum::
                                     REPEATED_FIELD_PROXY_TEST_IMPORT_BAR,
                                 RepeatedFieldProxyTestImportEnum::
                                     REPEATED_FIELD_PROXY_TEST_IMPORT_BAZ));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedLegacyStringProxy) {
  TestRepeatedStdStringProxy msg;
  {
    auto proxy = msg.mutable_strings_proxy();
    proxy.emplace_back("1");
    proxy.emplace_back("2");
    proxy.emplace_back("3");
  }

  auto proxy = msg.strings_proxy();
  EXPECT_THAT(proxy, ElementsAre("1", "2", "3"));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedStringViewProxy) {
  TestRepeatedStringViewProxy msg;
  {
    auto proxy = msg.mutable_string_views_proxy();
    proxy.emplace_back("1");
    proxy.emplace_back("2");
    proxy.emplace_back("3");
  }

  auto proxy = msg.string_views_proxy();
  EXPECT_THAT(proxy, ElementsAre("1", "2", "3"));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
