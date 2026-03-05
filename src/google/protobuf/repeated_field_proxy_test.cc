#include "google/protobuf/repeated_field_proxy.h"

#include <cstdint>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

template <typename T>
T StrAs(absl::string_view s) {
  return T(s);
}

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
    proxy[0] = 4;

    EXPECT_EQ(proxy[0], 4);
    EXPECT_EQ(proxy[1], 2);
    EXPECT_EQ(proxy[2], 3);
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");

  {
    auto proxy = field.MakeProxy();
    proxy[0] = "5";
    proxy[1] = StrAs<std::string>("6");
    const char* c_str = "7";
    proxy[2] = c_str;
    proxy[3] = StrAs<absl::string_view>("8");

    EXPECT_EQ(proxy[0], "5");
    EXPECT_EQ(proxy[1], "6");
    EXPECT_EQ(proxy[2], "7");
    EXPECT_EQ(proxy[3], "8");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementCord) {
  auto field = MakeRepeatedFieldContainer<absl::Cord>();
  field->Add(absl::Cord("1"));
  field->Add(absl::Cord("2"));
  field->Add(absl::Cord("3"));
  field->Add(absl::Cord("4"));

  {
    auto proxy = field.MakeProxy();

    proxy[0] = "5";
    proxy[1] = std::string("6");
    const char* c_str = "7";
    proxy[2] = c_str;
    proxy[3] = absl::string_view("8");

    EXPECT_EQ(proxy[0], "5");
    EXPECT_EQ(proxy[1], "6");
    EXPECT_EQ(proxy[2], "7");
    EXPECT_EQ(proxy[3], "8");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  {
    auto proxy = field.MakeProxy();
    proxy[2].set_value(4);

    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 4)pb"));
  }
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
