#include "google/protobuf/repeated_field_proxy.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/meta/type_traits.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_protos/repeated_field_proxy_test.pb.h"
#include "google/protobuf/test_textproto.h"


namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::proto2_unittest::RepeatedFieldProxyTestSimpleMessage;
using ::testing::ElementsAre;

static constexpr absl::string_view kLongString =
    "long string that will be heap allocated";

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
template <typename T>
class TestOnlyRepeatedFieldContainer {
  using FieldType = typename internal::RepeatedFieldTraits<T>::type;

 public:
  static TestOnlyRepeatedFieldContainer<T> New(Arena* arena) {
    return TestOnlyRepeatedFieldContainer<T>(Arena::Create<FieldType>(arena),
                                             arena);
  }

  // Disable copy construction and all forms of assignment.
  TestOnlyRepeatedFieldContainer(const TestOnlyRepeatedFieldContainer& other) =
      delete;
  TestOnlyRepeatedFieldContainer& operator=(
      const TestOnlyRepeatedFieldContainer& other) = delete;
  TestOnlyRepeatedFieldContainer& operator=(
      TestOnlyRepeatedFieldContainer&& other) = delete;

  // Destroying move constructor.
  TestOnlyRepeatedFieldContainer(TestOnlyRepeatedFieldContainer&& other)
      : field_(other.field_), arena_(other.arena_) {
    other.field_ = nullptr;
    other.arena_ = nullptr;
  }

  ~TestOnlyRepeatedFieldContainer() {
    if (arena_ == nullptr) {
      delete field_;
    }
  }

  FieldType& operator*() { return *field_; }
  const FieldType& operator*() const { return *field_; }

  FieldType* operator->() { return &*field_; }
  const FieldType* operator->() const { return &*field_; }

  RepeatedFieldProxy<T> MakeProxy() {
    return internal::ConstructRepeatedFieldProxy<T>(*field_, arena_);
  }
  RepeatedFieldProxy<const T> MakeConstProxy() const {
    return internal::ConstructRepeatedFieldProxy<const T>(*field_);
  }

 private:
  TestOnlyRepeatedFieldContainer(FieldType* field, Arena* arena)
      : field_(field), arena_(arena) {}

  FieldType* field_;
  Arena* arena_;
};

class RepeatedFieldProxyTest : public testing::TestWithParam<bool> {
 protected:
  bool UseArena() const { return GetParam(); }
  Arena* arena() { return UseArena() ? &arena_ : nullptr; }

  template <typename T>
  TestOnlyRepeatedFieldContainer<T> MakeRepeatedFieldContainer() {
    return TestOnlyRepeatedFieldContainer<T>::New(arena());
  }

 private:
  Arena arena_;
};

TEST_P(RepeatedFieldProxyTest, Empty) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  EXPECT_TRUE(proxy.empty());
}

TEST_P(RepeatedFieldProxyTest, ConstEmpty) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();

  {
    RepeatedFieldProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeConstProxy();
    EXPECT_TRUE(proxy.empty());
  }

  field->Add();
  {
    RepeatedFieldProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeConstProxy();
    EXPECT_FALSE(proxy.empty());
  }
}

TEST_P(RepeatedFieldProxyTest, Size) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  EXPECT_EQ(proxy.size(), 0);
}

TEST_P(RepeatedFieldProxyTest, ConstSize) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();

  {
    RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeProxy();
    EXPECT_EQ(proxy.size(), 0);
  }

  field->Add();
  {
    RepeatedFieldProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeConstProxy();
    EXPECT_EQ(proxy.size(), 1);
  }
}

TEST_P(RepeatedFieldProxyTest, ArrayIndexing) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  {
    auto proxy = field.MakeProxy();
    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 3)pb"));
  }

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 3)pb"));
  }
}

TEST_P(RepeatedFieldProxyTest, ArrayIndexingStringView) {
  auto field = MakeRepeatedFieldContainer<absl::string_view>();
  field->Add("1");
  field->Add("2");
  field->Add("3");

  {
    auto proxy = field.MakeProxy();
    EXPECT_THAT(proxy[0], "1");
    EXPECT_THAT(proxy[1], "2");
    EXPECT_THAT(proxy[2], "3");
  }

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy[0], "1");
    EXPECT_THAT(proxy[1], "2");
    EXPECT_THAT(proxy[2], "3");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementPrimitive) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    auto proxy = field.MakeProxy();
    proxy.set(0, 4);

    EXPECT_EQ(proxy[0], 4);
    EXPECT_EQ(proxy[1], 2);
    EXPECT_EQ(proxy[2], 3);
  }
}

template <typename StringType>
void TestMutateStringElement(google::protobuf::RepeatedFieldProxy<StringType> proxy) {
  ASSERT_THAT(proxy[0], StringEq("1"));
  ASSERT_THAT(proxy[1], StringEq("2"));
  ASSERT_THAT(proxy[2], StringEq("3"));
  ASSERT_THAT(proxy[3], StringEq("4"));

  if constexpr (std::is_same_v<StringType, std::string>) {
    proxy[0] = "5";
    proxy[1] = StrAs<std::string>("6");
    const char* c_str = "7";
    proxy[2] = c_str;
    proxy[3] = StrAs<absl::string_view>("8");

    EXPECT_THAT(proxy[0], StringEq("5"));
    EXPECT_THAT(proxy[1], StringEq("6"));
    EXPECT_THAT(proxy[2], StringEq("7"));
    EXPECT_THAT(proxy[3], StringEq("8"));
  }

  auto long_string = std::string(kLongString);
  const char* string_ptr = long_string.c_str();

  proxy.set(0, std::move(long_string));
  EXPECT_THAT(proxy[0], StringEq(kLongString));
  if constexpr (std::is_same_v<StringType, std::string> ||
                std::is_same_v<StringType, absl::string_view>) {
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

  EXPECT_THAT(proxy[0], StringEq("9"));
  EXPECT_THAT(proxy[1], StringEq("10"));
  EXPECT_THAT(proxy[2], StringEq("11"));
  EXPECT_THAT(proxy[3], StringEq("12"));

  std::string str13 = "13", str14 = "14";
  proxy.set(0, std::ref(str13));
  proxy.set(1, std::ref(str14));
  proxy.set(2, std::ref("15"));
  proxy.set(3, std::cref("16"));

  EXPECT_THAT(proxy[0], StringEq("13"));
  EXPECT_THAT(proxy[1], StringEq("14"));
  EXPECT_THAT(proxy[2], StringEq("15"));
  EXPECT_THAT(proxy[3], StringEq("16"));

  auto cord = absl::Cord(kLongString);
  proxy.set(0, std::move(cord));
  EXPECT_THAT(proxy[0], StringEq(kLongString));
}

TEST_P(RepeatedFieldProxyTest, MutateElementString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");
  TestMutateStringElement<std::string>(field.MakeProxy());
}

TEST_P(RepeatedFieldProxyTest, MutateElementStringView) {
  auto field = MakeRepeatedFieldContainer<absl::string_view>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");
  TestMutateStringElement<absl::string_view>(field.MakeProxy());
}

TEST_P(RepeatedFieldProxyTest, MutateElementCord) {
  auto field = MakeRepeatedFieldContainer<absl::Cord>();
  field->Add(absl::Cord("1"));
  field->Add(absl::Cord("2"));
  field->Add(absl::Cord("3"));
  field->Add(absl::Cord("4"));
  TestMutateStringElement<absl::Cord>(field.MakeProxy());
}

TEST_P(RepeatedFieldProxyTest, MutateElementMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  auto proxy = field.MakeProxy();
  proxy[2].set_value(4);

  EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
  EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
  EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 4)pb"));

  RepeatedFieldProxyTestSimpleMessage msg;
  msg.set_value(5);
  proxy.set(0, msg);
  // `msg` is copied into the element, so it should be unchanged.
  EXPECT_TRUE(msg.has_value());

  {
    auto* msg2 = Arena::Create<RepeatedFieldProxyTestSimpleMessage>(arena());
    msg2->set_value(6);
    auto* nested = msg2->mutable_nested();
    nested->set_value(7);
    proxy.set(1, std::move(*msg2));

    // Since `msg2` was moved, `nested` should point to the same object.
    EXPECT_EQ(proxy[1].mutable_nested(), nested);

    if (!UseArena()) {
      delete msg2;
    }
  }

  EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 5)pb"));
  EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 6,
                                         nested { value: 7 })pb"));
  EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 4)pb"));
}

TEST_P(RepeatedFieldProxyTest, PushBackInt) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  auto proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);

  EXPECT_THAT(*field, ElementsAre(1, 2, 3));
}

TEST_P(RepeatedFieldProxyTest, PushBackMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
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

  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, PushBackMessageLvalueCopies) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  auto* msg1 = Arena::Create<RepeatedFieldProxyTestSimpleMessage>(arena());
  auto* nested = msg1->mutable_nested();
  proxy.push_back(*msg1);
  EXPECT_NE(proxy[0].mutable_nested(), nested);

  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(nested: {})pb")));

  if (!UseArena()) {
    delete msg1;
  }
}

TEST_P(RepeatedFieldProxyTest, PushBackMessageRvalueDoesNotCopy) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  auto proxy = field.MakeProxy();
  auto* msg1 = Arena::Create<RepeatedFieldProxyTestSimpleMessage>(arena());
  auto* nested = msg1->mutable_nested();
  proxy.push_back(std::move(*msg1));
  EXPECT_EQ(proxy[0].mutable_nested(), nested);

  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(nested: {})pb")));

  if (!UseArena()) {
    delete msg1;
  }
}

template <typename StringType>
void TestPushBackString(google::protobuf::RepeatedFieldProxy<StringType> proxy) {
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

    if constexpr (std::is_same_v<StringType, std::string> ||
                  std::is_same_v<StringType, absl::string_view>) {
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

TEST_P(RepeatedFieldProxyTest, PushBackStdString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  TestPushBackString<std::string>(field.MakeProxy());
}

TEST_P(RepeatedFieldProxyTest, PushBackStringView) {
  auto field = MakeRepeatedFieldContainer<absl::string_view>();
  TestPushBackString<absl::string_view>(field.MakeProxy());
}

TEST_P(RepeatedFieldProxyTest, PushBackCord) {
  auto field = MakeRepeatedFieldContainer<absl::Cord>();
  TestPushBackString<absl::Cord>(field.MakeProxy());
}

INSTANTIATE_TEST_SUITE_P(RepeatedFieldProxyTest, RepeatedFieldProxyTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "WithArena" : "WithoutArena";
                         });

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
