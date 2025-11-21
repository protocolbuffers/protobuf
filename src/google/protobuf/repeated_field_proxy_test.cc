#include "google/protobuf/repeated_field_proxy.h"

#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/algorithm/container.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_field_proxy_test.pb.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/string_piece_field_support.h"
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace internal {

using ::proto2_unittest::TestMessage;
using ::proto2_unittest::TestRepeatedCordProxy;
using ::proto2_unittest::TestRepeatedEnumProxy;
using ::proto2_unittest::TestRepeatedIntProxy;
using ::proto2_unittest::TestRepeatedMessageProxy;
using ::proto2_unittest::TestRepeatedStdStringProxy;
using ::proto2_unittest::TestRepeatedStringPieceProxy;
using ::proto2_unittest::TestRepeatedStringViewProxy;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

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
    if (arena != nullptr) {
      return TestOnlyRepeatedFieldContainer<T>(Arena::Create<FieldType>(arena),
                                               arena);
    } else {
      return TestOnlyRepeatedFieldContainer<T>(new FieldType(),
                                               /*arena=*/nullptr);
    }
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
    return RepeatedFieldProxy<T>(*field_, arena_);
  }
  RepeatedFieldProxy<const T> MakeConstProxy() const {
    return RepeatedFieldProxy<const T>(*field_);
  }

 private:
  TestOnlyRepeatedFieldContainer(FieldType* field, Arena* arena)
      : field_(field), arena_(arena) {}

  FieldType* field_;
  Arena* arena_;
};

namespace {

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

TEST_P(RepeatedFieldProxyTest, RepeatedInt32) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  RepeatedFieldProxy<int32_t> proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);
  EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3));

  proxy[1] = 4;
  EXPECT_THAT(proxy, ElementsAre(1, 4, 3));
  EXPECT_THAT(*field, ElementsAre(1, 4, 3));
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedInt32) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    RepeatedFieldProxy<const int32_t> proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  }

  {
    RepeatedFieldProxy<const int32_t> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  }
}

TEST_P(RepeatedFieldProxyTest, RepeatedUint32) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();
  RepeatedFieldProxy<uint32_t> proxy = field.MakeProxy();
  proxy.push_back(1);
  EXPECT_THAT(proxy, ElementsAre(1));
}

TEST_P(RepeatedFieldProxyTest, RepeatedInt64) {
  auto field = MakeRepeatedFieldContainer<int64_t>();
  RepeatedFieldProxy<int64_t> proxy = field.MakeProxy();
  proxy.push_back(std::numeric_limits<int64_t>::min());
  EXPECT_THAT(proxy, ElementsAre(std::numeric_limits<int64_t>::min()));
}

TEST_P(RepeatedFieldProxyTest, RepeatedUint64) {
  auto field = MakeRepeatedFieldContainer<uint64_t>();
  RepeatedFieldProxy<uint64_t> proxy = field.MakeProxy();
  proxy.push_back(std::numeric_limits<uint64_t>::max());
  EXPECT_THAT(proxy, ElementsAre(std::numeric_limits<uint64_t>::max()));
}

TEST_P(RepeatedFieldProxyTest, RepeatedFloat) {
  auto field = MakeRepeatedFieldContainer<float>();
  RepeatedFieldProxy<float> proxy = field.MakeProxy();
  proxy.push_back(1.5);
  EXPECT_THAT(proxy, ElementsAre(1.5));
}

TEST_P(RepeatedFieldProxyTest, RepeatedDouble) {
  auto field = MakeRepeatedFieldContainer<double>();
  RepeatedFieldProxy<double> proxy = field.MakeProxy();
  proxy.push_back(std::numeric_limits<double>::max());
  EXPECT_THAT(proxy, ElementsAre(std::numeric_limits<double>::max()));
}

TEST_P(RepeatedFieldProxyTest, RepeatedString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  RepeatedFieldProxy<std::string> proxy = field.MakeProxy();
  proxy.push_back("one");
  proxy.push_back("two");
  proxy.push_back("three");
  EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  EXPECT_THAT(*field, ElementsAre("one", "two", "three"));

  proxy[1] = "four";
  EXPECT_THAT(proxy, ElementsAre("one", "four", "three"));
  EXPECT_THAT(*field, ElementsAre("one", "four", "three"));
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("one");
  field->Add("two");
  field->Add("three");

  {
    RepeatedFieldProxy<const std::string> proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  }

  {
    RepeatedFieldProxy<const std::string> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  }
}

TEST_P(RepeatedFieldProxyTest, RepeatedMessage) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  proxy.emplace_back().set_value(1);

  TestMessage msg;
  msg.set_value(2);
  proxy.push_back(std::move(msg));
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb")));
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedMessage) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  {
    RepeatedFieldProxy<const TestMessage> proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }

  {
    RepeatedFieldProxy<const TestMessage> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }
}

TEST_P(RepeatedFieldProxyTest, Empty) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  EXPECT_TRUE(proxy.empty());
  proxy.emplace_back();
  EXPECT_FALSE(proxy.empty());
}

TEST_P(RepeatedFieldProxyTest, ConstEmpty) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();

  {
    RepeatedFieldProxy<const TestMessage> proxy = field.MakeConstProxy();
    EXPECT_TRUE(proxy.empty());
  }

  field->Add();
  {
    RepeatedFieldProxy<const TestMessage> proxy = field.MakeConstProxy();
    EXPECT_FALSE(proxy.empty());
  }
}

TEST_P(RepeatedFieldProxyTest, Size) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  EXPECT_EQ(proxy.size(), 0);

  proxy.emplace_back();
  EXPECT_EQ(proxy.size(), 1);

  proxy.emplace_back();
  proxy.emplace_back();
  EXPECT_EQ(proxy.size(), 3);
}

TEST_P(RepeatedFieldProxyTest, ConstSize) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();

  {
    RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
    EXPECT_EQ(proxy.size(), 0);
  }

  field->Add();
  {
    RepeatedFieldProxy<const TestMessage> proxy = field.MakeConstProxy();
    EXPECT_EQ(proxy.size(), 1);
  }
}

TEST_P(RepeatedFieldProxyTest, Iterators) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();
  RepeatedFieldProxy<uint32_t> proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);

  auto it = proxy.begin();
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(++it), 2);
  EXPECT_EQ(*(++it), 3);
  EXPECT_EQ(++it, proxy.end());

  auto rit = proxy.rbegin();
  EXPECT_EQ(*rit, 3);
  EXPECT_EQ(*(++rit), 2);
  EXPECT_EQ(*(++rit), 1);
  EXPECT_EQ(++rit, proxy.rend());
}

TEST_P(RepeatedFieldProxyTest, IteratorMutation) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();
  RepeatedFieldProxy<uint32_t> proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);

  auto it = proxy.begin();
  *it = 4;
  *(++it) = 5;
  EXPECT_THAT(proxy, ElementsAre(4, 5, 3));
  EXPECT_THAT(*field, ElementsAre(4, 5, 3));

  auto rit = proxy.rbegin();
  *rit = 6;
  *(++rit) = 7;
  EXPECT_THAT(proxy, ElementsAre(4, 7, 6));
  EXPECT_THAT(*field, ElementsAre(4, 7, 6));
}

TEST_P(RepeatedFieldProxyTest, ConstIterators) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  RepeatedFieldProxy<const uint32_t> proxy = field.MakeConstProxy();
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

TEST_P(RepeatedFieldProxyTest, PopBack) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  proxy.pop_back();

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TEST_P(RepeatedFieldProxyTest, Clear) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  proxy.clear();

  EXPECT_THAT(proxy, IsEmpty());
  EXPECT_THAT(*field, IsEmpty());
}

TEST_P(RepeatedFieldProxyTest, Erase) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  proxy.erase(absl::c_find_if(
      proxy, [](const TestMessage& msg) { return msg.value() == 2; }));

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, EraseRange) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);
  field->Add()->set_value(4);

  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  auto it = absl::c_find_if(
      proxy, [](const TestMessage& msg) { return msg.value() == 2; });
  proxy.erase(it, it + 2);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 4)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 4)pb")));
}

TEST_P(RepeatedFieldProxyTest, Assign) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  field->Add()->set_value(1);

  std::vector<TestMessage> msgs(2);
  msgs[0].set_value(2);
  msgs[1].set_value(3);

  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  proxy.assign(msgs.begin(), msgs.end());

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, AssignInitializerList) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  field->Add()->set_value(1);

  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  TestMessage msg1;
  msg1.set_value(2);
  TestMessage msg2;
  msg2.set_value(3);
  proxy.assign({msg1, msg2});

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, Reserve) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();

  RepeatedFieldProxy<uint32_t> proxy = field.MakeProxy();
  proxy.reserve(10);

  uint64_t space_used_before = 0;
  if (arena() != nullptr) {
    space_used_before = arena()->SpaceUsed();
  }

  for (int i = 0; i < 10; ++i) {
    proxy.emplace_back(i);
  }

  EXPECT_THAT(proxy, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
  EXPECT_THAT(*field, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));

  if (arena() != nullptr) {
    // In the arena case, we verify that no additional memory was allocated
    // after the initial reserve().
    EXPECT_EQ(space_used_before, arena()->SpaceUsed());
  }
}

TEST_P(RepeatedFieldProxyTest, Swap) {
  auto field1 = MakeRepeatedFieldContainer<TestMessage>();
  field1->Add()->set_value(1);

  auto field2 = MakeRepeatedFieldContainer<TestMessage>();
  field2->Add()->set_value(2);
  field2->Add()->set_value(3);

  RepeatedFieldProxy<TestMessage> proxy1 = field1.MakeProxy();
  RepeatedFieldProxy<TestMessage> proxy2 = field2.MakeProxy();
  proxy1.swap(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(proxy2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TEST_P(RepeatedFieldProxyTest, Resize) {
  auto field = MakeRepeatedFieldContainer<int>();
  field->Add(1);

  RepeatedFieldProxy<int> proxy = field.MakeProxy();
  proxy.resize(3, 10);

  EXPECT_THAT(proxy, ElementsAre(1, 10, 10));
  EXPECT_THAT(*field, ElementsAre(1, 10, 10));

  proxy.resize(2, 20);
  EXPECT_THAT(proxy, ElementsAre(1, 10));
  EXPECT_THAT(*field, ElementsAre(1, 10));
}

TEST_P(RepeatedFieldProxyTest, ExplicitConversionToLegacyRepeatedField) {
  auto field = MakeRepeatedFieldContainer<int>();
  field->Add(1);

  RepeatedFieldProxy<int> proxy = field.MakeProxy();
  // Make an explicit conversion to RepeatedField. This will perform a deep
  // copy.
  RepeatedField<int> field2 = RepeatedField<int>(proxy);
  EXPECT_THAT(field2, ElementsAre(1));

  // Verify that field2 is a copy.
  proxy.clear();
  EXPECT_THAT(field2, ElementsAre(1));
}

TEST_P(RepeatedFieldProxyTest, ExplicitConversionToLegacyRepeatedPtrField) {
  auto field = MakeRepeatedFieldContainer<TestMessage>();
  field->Add()->set_value(1);

  RepeatedFieldProxy<TestMessage> proxy = field.MakeProxy();
  // Make an explicit conversion to RepeatedPtrField. This will perform a deep
  // copy.
  RepeatedPtrField<TestMessage> field2 = RepeatedPtrField<TestMessage>(proxy);
  EXPECT_THAT(field2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));

  // Verify that field2 is a copy.
  proxy.clear();
  EXPECT_THAT(field2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

INSTANTIATE_TEST_SUITE_P(RepeatedFieldProxyTest, RepeatedFieldProxyTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "WithArena" : "WithoutArena";
                         });

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

// Repeated cords:
static_assert(
    std::is_same_v<decltype(std::declval<TestRepeatedCordProxy>().cords()),
                   const RepeatedField<absl::Cord>&>);
static_assert(std::is_same_v<
              decltype(std::declval<TestRepeatedCordProxy>().mutable_cords()),
              RepeatedField<absl::Cord>*>);
static_assert(std::is_same_v<
              decltype(std::declval<TestRepeatedCordProxy>().cords_proxy()),
              RepeatedFieldProxy<const absl::Cord>>);
static_assert(std::is_same_v<decltype(std::declval<TestRepeatedCordProxy>()
                                          .mutable_cords_proxy()),
                             RepeatedFieldProxy<absl::Cord>>);

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
              RepeatedFieldProxy<const int>>);
static_assert(std::is_same_v<decltype(std::declval<TestRepeatedEnumProxy>()
                                          .mutable_enums_proxy()),
                             RepeatedFieldProxy<int>>);

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
  auto proxy = msg.mutable_nested_messages_proxy();
  proxy.emplace_back().set_value(1);
  proxy.emplace_back().set_value(2);
  proxy.emplace_back().set_value(3);

  EXPECT_THAT(msg.nested_messages_proxy(),
              ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                          EqualsProto(R"pb(value: 2)pb"),
                          EqualsProto(R"pb(value: 3)pb")));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedCordProxy) {
  TestRepeatedCordProxy msg;
  auto proxy = msg.mutable_cords_proxy();
  proxy.emplace_back("1");
  proxy.emplace_back("2");
  proxy.emplace_back("3");

  EXPECT_THAT(msg.cords_proxy(), ElementsAre("1", "2", "3"));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedIntProxy) {
  TestRepeatedIntProxy msg;
  msg.mutable_ints_proxy().assign({1, 2, 3});

  EXPECT_THAT(msg.ints_proxy(), ElementsAre(1, 2, 3));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedEnumProxy) {
  TestRepeatedEnumProxy msg;
  msg.mutable_enums_proxy().assign({TestRepeatedEnumProxy::FOO,
                                    TestRepeatedEnumProxy::BAR,
                                    TestRepeatedEnumProxy::BAZ});

  EXPECT_THAT(msg.enums_proxy(), ElementsAre(TestRepeatedEnumProxy::FOO,
                                             TestRepeatedEnumProxy::BAR,
                                             TestRepeatedEnumProxy::BAZ));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedLegacyStringProxy) {
  TestRepeatedStdStringProxy msg;
  msg.mutable_strings_proxy().assign({"1", "2", "3"});

  EXPECT_THAT(msg.strings_proxy(), ElementsAre("1", "2", "3"));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedStringViewProxy) {
  TestRepeatedStringViewProxy msg;
  msg.mutable_string_views_proxy().assign({"1", "2", "3"});

  EXPECT_THAT(msg.string_views_proxy(), ElementsAre("1", "2", "3"));
}

TEST(RepeatedFieldProxyInterfaceTest, RepeatedStringPieceProxy) {
  TestRepeatedStringPieceProxy msg;
  {
    auto proxy = msg.mutable_string_pieces_proxy();
    proxy.emplace_back("1");
    proxy.emplace_back("2");
    proxy.emplace_back("3");
  }

  {
    auto proxy = msg.mutable_string_pieces_proxy();
    EXPECT_EQ(proxy.size(), 3);
    EXPECT_EQ(proxy[0].Get(), "1");
    EXPECT_EQ(proxy[1].Get(), "2");
    EXPECT_EQ(proxy[2].Get(), "3");
  }
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
