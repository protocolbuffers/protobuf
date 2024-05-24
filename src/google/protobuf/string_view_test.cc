#include <string>
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
// clang-format off
#include "absl/strings/string_view.h"
// clang-format on
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest_string_view.pb.h"

namespace google {
namespace protobuf {
namespace {

using ::protobuf_unittest::TestStringView;
using ::testing::ElementsAre;
using ::testing::StrEq;

TEST(StringViewFieldTest, SingularViewGetter) {
  TestStringView message;

  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        singular_string: "0123456789"
        singular_bytes: "012345678901234567890123456789"
      )pb",
      &message));

  // singular_string
  EXPECT_TRUE(message.has_singular_string());

  auto singular_string = message.singular_string();
  static_assert(
      std::is_same<decltype(singular_string), absl::string_view>::value,
      "unexpected type");
  EXPECT_THAT(singular_string, StrEq("0123456789"));

  // singular_bytes
  EXPECT_TRUE(message.has_singular_bytes());

  auto singular_bytes = message.singular_bytes();
  static_assert(
      std::is_same<decltype(singular_bytes), absl::string_view>::value,
      "unexpected type");
  EXPECT_THAT(singular_bytes, StrEq("012345678901234567890123456789"));
}

template <typename T>
void VerifySingularStringSet(TestStringView& message, T&& value,
                             absl::string_view expected) {
  message.set_singular_string(static_cast<T&&>(value));

  EXPECT_TRUE(message.has_singular_string());
  EXPECT_THAT(message.singular_string(), StrEq(expected));

  message.clear_singular_string();

  EXPECT_FALSE(message.has_singular_string());
  EXPECT_EQ(message.singular_string().size(), 0);
}

#define STRING_PAYLOAD "012345678901234567890123456789"

TEST(StringViewFieldTest, SingularSetByStringView) {
  TestStringView message;

  absl::string_view value = {STRING_PAYLOAD};

  VerifySingularStringSet(message, value, value);
}

TEST(StringViewFieldTest, SingularSetByCharPtr) {
  TestStringView message;

  absl::string_view expected = {STRING_PAYLOAD};
  const char* ptr = STRING_PAYLOAD;

  VerifySingularStringSet(message, ptr, expected);
}

TEST(StringViewFieldTest, SingularSetByConstStringRef) {
  TestStringView message;

  std::string value = STRING_PAYLOAD;
  const std::string& ref = value;

  VerifySingularStringSet(message, ref, STRING_PAYLOAD);
}

TEST(StringViewFieldTest, SingularSetByStringMove) {
  TestStringView message;

  std::string value = STRING_PAYLOAD;

  VerifySingularStringSet(message, std::move(value), STRING_PAYLOAD);
}

TEST(StringViewFieldTest, SingularSetAndGetByReflection) {
  TestStringView message;

  const Reflection* reflection = message.GetReflection();
  const FieldDescriptor* field =
      message.GetDescriptor()->FindFieldByName("singular_string");

  reflection->SetString(&message, field, std::string{STRING_PAYLOAD});

  EXPECT_THAT(reflection->GetString(message, field), StrEq(STRING_PAYLOAD));
  Reflection::ScratchSpace scratch;
  EXPECT_THAT(reflection->GetStringView(message, field, scratch),
              StrEq(STRING_PAYLOAD));
  EXPECT_THAT(message.singular_string(), StrEq(STRING_PAYLOAD));
}

TEST(StringViewFieldTest, RepeatedViewGetter) {
  TestStringView message;

  ASSERT_TRUE(TextFormat::ParseFromString(R"pb(
                                            repeated_string: "foo"
                                            repeated_string: "bar"
                                            repeated_string: "baz"

                                            repeated_bytes: "000"
                                            repeated_bytes: "111"
                                            repeated_bytes: "222"
                                            repeated_bytes: "333"
                                            repeated_bytes: "444"
                                          )pb",
                                          &message));

  EXPECT_EQ(message.repeated_string_size(), 3);

  auto repeated_string_0 = message.repeated_string(0);
  static_assert(
      std::is_same<decltype(repeated_string_0), absl::string_view>::value,
      "unexpected type");
  EXPECT_THAT(repeated_string_0, StrEq("foo"));
  EXPECT_THAT(message.repeated_string(), ElementsAre("foo", "bar", "baz"));

  EXPECT_EQ(message.repeated_bytes_size(), 5);

  auto repeated_bytes_2 = message.repeated_bytes(2);
  static_assert(
      std::is_same<decltype(repeated_bytes_2), absl::string_view>::value,
      "unexpected type");
  EXPECT_THAT(repeated_bytes_2, StrEq("222"));
  EXPECT_THAT(message.repeated_bytes(),
              ElementsAre("000", "111", "222", "333", "444"));
}

TEST(StringViewFieldTest, RepeatedSetByCharPtr) {
  TestStringView message;

  const char* ptr0 = "foo";
  const char* ptr1 = "baz";
  const char* ptr2 = STRING_PAYLOAD;
  message.add_repeated_string(ptr0);
  message.add_repeated_string(ptr1);
  message.add_repeated_string(ptr2);

  EXPECT_THAT(message.repeated_string(),
              ElementsAre("foo", "baz", STRING_PAYLOAD));

  message.set_repeated_string(0, ptr1);
  message.set_repeated_string(1, ptr2);
  message.set_repeated_string(2, ptr0);

  EXPECT_THAT(message.repeated_string(),
              ElementsAre("baz", STRING_PAYLOAD, "foo"));
}

TEST(StringViewFieldTest, RepeatedSetByStringView) {
  TestStringView message;

  absl::string_view view0 = "foo";
  absl::string_view view1 = "baz";
  absl::string_view view2 = STRING_PAYLOAD;
  message.add_repeated_string(view0);
  message.add_repeated_string(view1);
  message.add_repeated_string(view2);

  EXPECT_THAT(message.repeated_string(),
              ElementsAre("foo", "baz", STRING_PAYLOAD));

  message.set_repeated_string(0, view1);
  message.set_repeated_string(1, view2);
  message.set_repeated_string(2, view0);

  EXPECT_THAT(message.repeated_string(),
              ElementsAre("baz", STRING_PAYLOAD, "foo"));
}

TEST(StringViewFieldTest, RepeatedSetByConstStringRef) {
  TestStringView message;

  std::string str0 = "foo";
  std::string str1 = "baz";
  std::string str2 = STRING_PAYLOAD;
  message.add_repeated_string(str0);
  message.add_repeated_string(str1);
  message.add_repeated_string(str2);

  EXPECT_THAT(message.repeated_string(),
              ElementsAre("foo", "baz", STRING_PAYLOAD));

  message.set_repeated_string(0, str1);
  message.set_repeated_string(1, str2);
  message.set_repeated_string(2, str0);

  EXPECT_THAT(message.repeated_string(),
              ElementsAre("baz", STRING_PAYLOAD, "foo"));
}

TEST(StringViewFieldTest, RepeatedSetByStringMove) {
  TestStringView message;

  message.add_repeated_string(std::string{"foo"});
  message.add_repeated_string(std::string{"baz"});
  message.add_repeated_string(std::string{STRING_PAYLOAD});

  EXPECT_THAT(message.repeated_string(),
              ElementsAre("foo", "baz", STRING_PAYLOAD));

  message.set_repeated_string(0, std::string{"baz"});
  message.set_repeated_string(1, std::string{STRING_PAYLOAD});
  message.set_repeated_string(2, std::string{"foo"});

  EXPECT_THAT(message.repeated_string(),
              ElementsAre("baz", STRING_PAYLOAD, "foo"));
}

TEST(StringViewFieldTest, RepeatedViewSetter) {
  TestStringView message;

  message.add_repeated_string("000");
  message.add_repeated_string("111");
  message.add_repeated_string("222");

  EXPECT_EQ(message.repeated_string_size(), 3);
  EXPECT_THAT(message.repeated_string(), ElementsAre("000", "111", "222"));

  for (auto& it : *message.mutable_repeated_string()) {
    it.append(it);
  }

  EXPECT_EQ(message.repeated_string_size(), 3);
  EXPECT_THAT(message.repeated_string(),
              ElementsAre("000000", "111111", "222222"));

  message.clear_repeated_string();
  EXPECT_EQ(message.repeated_string_size(), 0);
}

TEST(StringViewFieldTest, RepeatedSetAndGetByReflection) {
  TestStringView message;

  const Reflection* reflection = message.GetReflection();
  const FieldDescriptor* field =
      message.GetDescriptor()->FindFieldByName("repeated_string");

  // AddString().
  reflection->AddString(&message, field, std::string{"000"});
  reflection->AddString(&message, field, std::string{"111"});
  reflection->AddString(&message, field, std::string{"222"});
  {
    const auto& rep_str =
        reflection->GetRepeatedFieldRef<std::string>(message, field);
    EXPECT_EQ(rep_str.size(), 3);
    EXPECT_THAT(rep_str, ElementsAre("000", "111", "222"));
  }

  // SetRepeatedString().
  reflection->SetRepeatedString(&message, field, 0, std::string{"000000"});
  reflection->SetRepeatedString(&message, field, 1, std::string{"111111"});
  reflection->SetRepeatedString(&message, field, 2, std::string{"222222"});
  {
    const auto& rep_str =
        reflection->GetRepeatedFieldRef<std::string>(message, field);
    EXPECT_EQ(rep_str.size(), 3);
    EXPECT_THAT(rep_str, ElementsAre("000000", "111111", "222222"));
  }

  // MutableRepeatedPtrField().
  for (auto& it :
       *reflection->MutableRepeatedPtrField<std::string>(&message, field)) {
    it.append(it);
  }
  {
    const auto& rep_str =
        reflection->GetRepeatedFieldRef<std::string>(message, field);
    EXPECT_EQ(rep_str.size(), 3);
    EXPECT_THAT(rep_str,
                ElementsAre("000000000000", "111111111111", "222222222222"));
  }

  // GetRepeatedStringView.
  Reflection::ScratchSpace scratch;
  EXPECT_THAT(reflection->GetRepeatedStringView(message, field, 0, scratch),
              StrEq("000000000000"));
  EXPECT_THAT(reflection->GetRepeatedStringView(message, field, 1, scratch),
              StrEq("111111111111"));
  EXPECT_THAT(reflection->GetRepeatedStringView(message, field, 2, scratch),
              StrEq("222222222222"));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
