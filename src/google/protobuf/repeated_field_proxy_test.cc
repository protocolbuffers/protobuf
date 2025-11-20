#include "google/protobuf/repeated_field_proxy.h"

#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/arena.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {
namespace {

using ::proto2_unittest::TestAllTypes;
using ::testing::ElementsAre;
using ::testing::EqualsProto;

class RepeatedFieldProxyTest : public testing::TestWithParam<bool> {
 protected:
  bool UseArena() const { return GetParam(); }
  Arena* arena() { return UseArena() ? &arena_ : nullptr; }

 private:
  Arena arena_;
};

TEST_P(RepeatedFieldProxyTest, RepeatedInt) {
  auto* field = UseArena() ? Arena::Create<RepeatedField<int>>(arena())
                           : new RepeatedField<int>();
  RepeatedFieldProxy<int> proxy = *field;
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);
  EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3));

  proxy[1] = 4;
  EXPECT_THAT(proxy, ElementsAre(1, 4, 3));
  EXPECT_THAT(*field, ElementsAre(1, 4, 3));

  if (!UseArena()) {
    delete field;
  }
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedInt) {
  auto* field = UseArena() ? Arena::Create<RepeatedField<int>>(arena())
                           : new RepeatedField<int>();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    RepeatedFieldProxy<const int> proxy =
        static_cast<const RepeatedField<int>&>(*field);
    EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  }

  {
    RepeatedFieldProxy<const int> proxy = *field;
    EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  }

  if (!UseArena()) {
    delete field;
  }
}

TEST_P(RepeatedFieldProxyTest, RepeatedString) {
  auto* field = UseArena()
                    ? Arena::Create<RepeatedPtrField<std::string>>(arena())
                    : new RepeatedPtrField<std::string>();
  RepeatedFieldProxy<std::string> proxy = *field;
  proxy.push_back("one");
  proxy.push_back("two");
  proxy.push_back("three");
  EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  EXPECT_THAT(*field, ElementsAre("one", "two", "three"));

  proxy[1] = "four";
  EXPECT_THAT(proxy, ElementsAre("one", "four", "three"));
  EXPECT_THAT(*field, ElementsAre("one", "four", "three"));

  if (!UseArena()) {
    delete field;
  }
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedString) {
  auto* field = UseArena()
                    ? Arena::Create<RepeatedPtrField<std::string>>(arena())
                    : new RepeatedPtrField<std::string>();
  field->Add("one");
  field->Add("two");
  field->Add("three");

  {
    RepeatedFieldProxy<const std::string> proxy =
        static_cast<const RepeatedPtrField<std::string>&>(*field);
    EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  }

  {
    RepeatedFieldProxy<const std::string> proxy = *field;
    EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  }

  if (!UseArena()) {
    delete field;
  }
}

TEST_P(RepeatedFieldProxyTest, RepeatedMessage) {
  RepeatedPtrField<TestAllTypes>* field =
      UseArena() ? Arena::Create<RepeatedPtrField<TestAllTypes>>(arena())
                 : new RepeatedPtrField<TestAllTypes>();
  RepeatedFieldProxy<TestAllTypes> proxy = *field;
  proxy.emplace_back().set_optional_int32(1);

  TestAllTypes msg;
  msg.set_optional_int32(2);
  proxy.push_back(std::move(msg));
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(optional_int32: 1)pb"),
                                 EqualsProto(R"pb(optional_int32: 2)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(optional_int32: 1)pb"),
                                  EqualsProto(R"pb(optional_int32: 2)pb")));

  if (!UseArena()) {
    delete field;
  }
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedMessage) {
  RepeatedPtrField<TestAllTypes>* field =
      UseArena() ? Arena::Create<RepeatedPtrField<TestAllTypes>>(arena())
                 : new RepeatedPtrField<TestAllTypes>();
  field->Add()->set_optional_int32(1);
  field->Add()->set_optional_int32(2);

  {
    RepeatedFieldProxy<const TestAllTypes> proxy =
        static_cast<const RepeatedPtrField<TestAllTypes>&>(*field);
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(optional_int32: 1)pb"),
                                   EqualsProto(R"pb(optional_int32: 2)pb")));
  }

  {
    RepeatedFieldProxy<const TestAllTypes> proxy = *field;
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(optional_int32: 1)pb"),
                                   EqualsProto(R"pb(optional_int32: 2)pb")));
  }

  if (!UseArena()) {
    delete field;
  }
}

INSTANTIATE_TEST_SUITE_P(RepeatedFieldProxyTest, RepeatedFieldProxyTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "WithArena" : "WithoutArena";
                         });

}  // namespace
}  // namespace protobuf
}  // namespace google
