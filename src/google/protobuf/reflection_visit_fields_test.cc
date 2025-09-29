#include "google/protobuf/reflection_visit_fields.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/map_test_util.h"
#include "google/protobuf/map_unittest.pb.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_mset.pb.h"
#include "google/protobuf/unittest_mset_wire_format.pb.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::proto2_unittest::NestedTestAllTypes;
using ::proto2_unittest::TestAllExtensions;
using ::proto2_unittest::TestAllTypes;
using ::proto2_unittest::TestMap;
using ::proto2_unittest::TestOneof2;
using ::proto2_unittest::TestPackedExtensions;
using ::proto2_unittest::TestPackedTypes;
using ::proto2_wireformat_unittest::TestMessageSet;

struct TestParam {
  absl::string_view name;
  Message* (*create_message)(Arena& arena);
};

class VisitFieldsTest : public testing::TestWithParam<TestParam> {
 public:
  VisitFieldsTest() {
    message_ = GetParam().create_message(arena_);
    reflection_ = message_->GetReflection();
  }

 protected:
  Arena arena_;
  Message* message_;
  const Reflection* reflection_;
};

TEST_P(VisitFieldsTest, VisitedFieldsCountMatchesListFields) {
  uint32_t count = 0;
  VisitFields(*message_, [&](auto info) { ++count; });

  std::vector<const FieldDescriptor*> fields;
  reflection_->ListFields(*message_, &fields);

  EXPECT_EQ(count, fields.size());
}

TEST_P(VisitFieldsTest, VisitedMessageFieldsCountMatchesListFields) {
  uint32_t count = 0;
  VisitFields(*message_, [&](auto info) { ++count; }, FieldMask::kMessage);

  std::vector<const FieldDescriptor*> fields;
  reflection_->ListFields(*message_, &fields);
  int message_count = 0;
  for (auto field : fields) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) ++message_count;
  }

  EXPECT_EQ(count, message_count);
}

// Counts present message fields using ListFields() where:
// --N elements in a repeated message field are counted N times
// --M message values in a map field are counted M times.
// --A map field whose value type is not message is ignored.
int CountAllMessageFieldsViaListFields(const Reflection* reflection,
                                       const Message& message) {
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);

  int message_count = 0;
  for (auto field : fields) {
    if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) continue;
    if (field->is_map()) {
      if (field->message_type()->map_value()->cpp_type() !=
          FieldDescriptor::CPPTYPE_MESSAGE)
        continue;
    }
    if (field->is_repeated()) {
      message_count += reflection->FieldSize(message, field);
    } else {
      ++message_count;
    }
  }
  return message_count;
}

TEST_P(VisitFieldsTest, VisitMessageFieldsCountIncludesRepeatedElements) {
  int count = 0;
  VisitMessageFields(*message_, [&](const Message& msg) { ++count; });

  EXPECT_EQ(count, CountAllMessageFieldsViaListFields(reflection_, *message_));
}

TEST_P(VisitFieldsTest,
       VisitMutableMessageFieldsCountIncludesRepeatedElements) {
  int count = 0;
  VisitMutableMessageFields(*message_, [&](Message& msg) { ++count; });

  EXPECT_EQ(count, CountAllMessageFieldsViaListFields(reflection_, *message_));
}

TEST_P(VisitFieldsTest, ClearByVisitFieldsMustBeEmpty) {
  VisitFields(*message_, [](auto info) { info.Clear(); });

  EXPECT_EQ(message_->ByteSizeLong(), 0);
}

TEST_P(VisitFieldsTest, ClearByVisitFieldsRevisitNone) {
  VisitFields(*message_, [](auto info) { info.Clear(); });

  uint32_t count = 0;
  VisitFields(*message_, [&](auto info) { ++count; });

  EXPECT_EQ(count, 0);
}

void MutateNothingByVisit(Message& message) {
  VisitFields(message, [&](auto info) {
    if constexpr (info.is_map) {
      info.VisitElements([&](auto key, auto val) {
        if constexpr (val.cpp_type == FieldDescriptor::CPPTYPE_MESSAGE) {
          auto* msg = val.Mutable();
          (void)msg->ByteSizeLong();
        } else {
          val.Set(val.Get());
        }
      });
    } else if constexpr (info.is_repeated) {
      if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_MESSAGE) {
        size_t byte_size = 0;
        for (auto& it : info.Mutable()) {
          byte_size += it.ByteSizeLong();
        }
      } else if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_STRING) {
        if constexpr (info.is_cord) {
          for (auto& it : info.Mutable()) {
            absl::Cord tmp = it;
            it = tmp;
          }
        } else if constexpr (info.is_string_piece) {
          for (auto& it : info.Mutable()) {
            it.CopyFrom({it.data(), it.size()});
          }
        } else {
          for (auto& it : info.Mutable()) {
            std::string tmp;
            tmp = it;
            it = tmp;
          }
        }
      } else {
        for (auto& it : info.Mutable()) {
          it = *&it;  // Avoid -Wself-assign.
        }
      }
    } else {
      if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_MESSAGE) {
        auto& msg = info.Mutable();
        ABSL_CHECK_GT(msg.ByteSizeLong(), 0);
      } else if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_STRING) {
        if constexpr (info.is_cord) {
          info.Set(info.Get());
        } else {
          absl::string_view view = info.Get();
          info.Set({view.data(), view.size()});
        }
      } else {
        info.Set(info.Get());
      }
    }
  });
}

TEST_P(VisitFieldsTest, MutateNothingByVisitIdempotent) {
  std::string data;
  ASSERT_TRUE(message_->SerializeToString(&data));

  MutateNothingByVisit(*message_);

  // Checking the identity by comparing serialize bytes is discouraged, but this
  // allows us to be type-agnostic for this test. Also, the back to back
  // serialization should be stable.
  EXPECT_EQ(data, message_->SerializeAsString());
}

template <typename InfoT>
inline size_t MapKeyByteSizeLong(FieldDescriptor::Type type, InfoT info) {
  // There is a bug in GCC 9.5 where if-constexpr arguments are not understood
  // if passed into a helper function. A reproduction of the bug can be found
  // at: https://godbolt.org/z/65qW3vGhP
  // This is fixed in GCC 10.1+.
  (void)type;  // Suppress -Wunused-but-set-parameter

  if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_STRING) {
    return WireFormatLite::StringSize(info.Get());
  } else {
    return MapPrimitiveFieldByteSize<info.cpp_type>(type, info.Get());
  }
}

template <typename InfoT>
inline size_t MapValueByteSizeLong(FieldDescriptor::Type type, InfoT info) {
  // There is a bug in GCC 9.5 where if-constexpr arguments are not understood
  // if passed into a helper function. A reproduction of the bug can be found
  // at: https://godbolt.org/z/65qW3vGhP
  // This is fixed in GCC 10.1+.
  (void)type;  // Suppress -Wunused-but-set-parameter

  if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_STRING) {
    return WireFormatLite::StringSize(info.Get());
  } else if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_MESSAGE) {
    ABSL_CHECK_NE(type, FieldDescriptor::TYPE_GROUP);
    return WireFormatLite::MessageSize(info.Get());
  } else {
    return MapPrimitiveFieldByteSize<info.cpp_type>(type, info.Get());
  }
}

size_t ByteSizeLongByVisit(const Message& message) {
  size_t byte_size = 0;

  VisitFields(message, [&](auto info) {
    if constexpr (info.is_map) {
      // A map field is encoded as repeated messages like:
      //
      // message MapField {
      //   message MapEntry {
      //     1: key
      //     2: value
      //   };
      //   repeated MapEntry entry = <FIELD_NUMBER>;
      // }
      // where the key and the value tag are always encoded as two bytes. (no
      // group).
      //
      // Visit each element to calculate the inner size (key, value) and the
      // length field for LENGTH_DELIMITED field.
      size_t size = static_cast<size_t>(info.size());
      ABSL_CHECK_GT(size, 0u);
      byte_size += size * WireFormat::TagSize(info.number(),
                                              FieldDescriptor::TYPE_MESSAGE);

      FieldDescriptor::Type key_type = info.key_type();
      FieldDescriptor::Type val_type = info.value_type();

      info.VisitElements([&](auto key, auto val) {
        size_t inner_size = 2 + MapKeyByteSizeLong(key_type, key) +
                            MapValueByteSizeLong(val_type, val);

        byte_size += io::CodedOutputStream::VarintSize32(
                         static_cast<uint32_t>(inner_size)) +
                     inner_size;
      });
    } else if constexpr (info.is_repeated) {
      if (info.is_packed()) {
        size_t payload_size = info.FieldByteSize();
        byte_size +=
            WireFormat::TagSize(info.number(), FieldDescriptor::TYPE_STRING) +
            payload_size +
            io::CodedOutputStream::VarintSize32(
                static_cast<uint32_t>(payload_size));
      } else {
        size_t size = static_cast<size_t>(info.size());
        ABSL_CHECK_GT(size, 0u);
        byte_size += size * WireFormat::TagSize(info.number(), info.type()) +
                     info.FieldByteSize();
      }
    } else {
      if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_MESSAGE) {
        if constexpr (info.is_extension) {
          if (info.type() == FieldDescriptor::TYPE_MESSAGE &&
              info.is_message_set) {
            byte_size +=
                WireFormatLite::kMessageSetItemTagsSize +
                io::CodedOutputStream::VarintSize32(
                    static_cast<uint32_t>(info.number())) +
                WireFormatLite::LengthDelimitedSize(info.FieldByteSize());
            return;
          }
        }
        if (info.type() == FieldDescriptor::TYPE_GROUP) {
          byte_size += WireFormat::TagSize(info.number(), info.type()) +
                       info.FieldByteSize();
        } else if (info.type() == FieldDescriptor::TYPE_MESSAGE) {
          byte_size +=
              WireFormat::TagSize(info.number(), info.type()) +
              WireFormatLite::LengthDelimitedSize(info.FieldByteSize());
        }
      } else {
        byte_size += WireFormat::TagSize(info.number(), info.type()) +
                     info.FieldByteSize();
      }
    }
  });
  return byte_size;
}

TEST_P(VisitFieldsTest, ByteSizeByVisitFieldsMatchesCodegen) {
  EXPECT_EQ(ByteSizeLongByVisit(*message_), message_->ByteSizeLong());
}

TestMessageSet* CreateTestMessageSet(Arena& arena) {
  auto* msg = Arena::Create<TestMessageSet>(&arena);
  auto* ext1 = msg->MutableExtension(
      unittest::TestMessageSetExtension1::message_set_extension);
  ext1->set_i(-1);
  auto* ext3 = ext1->mutable_recursive()->MutableExtension(
      unittest::TestMessageSetExtension3::message_set_extension);
  ext3->set_required_int(-1);
  ext3->mutable_msg()->set_b(0);
  return msg;
}

NestedTestAllTypes* CreateNestedTestAllTypes(Arena& arena) {
  auto* msg = Arena::Create<NestedTestAllTypes>(&arena);
  TestUtil::SetAllFields(msg->mutable_payload());
  TestUtil::SetAllFields(msg->mutable_lazy_child()->mutable_payload());
  return msg;
}

INSTANTIATE_TEST_SUITE_P(
    ReflectionVisitFieldsTest, VisitFieldsTest,
    testing::Values(
        TestParam{"TestAllTypes",
                  [](Arena& arena) -> Message* {
                    auto* msg = Arena::Create<TestAllTypes>(&arena);
                    TestUtil::SetAllFields(msg);
                    return msg;
                  }},
        TestParam{"TestAllExtensions",
                  [](Arena& arena) -> Message* {
                    auto* msg = Arena::Create<TestAllExtensions>(&arena);
                    TestUtil::SetAllExtensions(msg);
                    return msg;
                  }},
        TestParam{"TestAllExtensionsLazy",
                  [](Arena& arena) -> Message* {
                    TestAllExtensions original;
                    TestUtil::SetAllExtensions(&original);
                    auto* parsed = Arena::Create<TestAllExtensions>(&arena);
                    ABSL_CHECK(
                        parsed->ParseFromString(original.SerializeAsString()));
                    return parsed;
                  }},
        TestParam{"TestMap",
                  [](Arena& arena) -> Message* {
                    auto* msg = Arena::Create<TestMap>(&arena);
                    MapTestUtil::SetMapFields(msg);
                    return msg;
                  }},
        TestParam{"TestMessageSet",
                  [](Arena& arena) -> Message* {
                    return CreateTestMessageSet(arena);
                  }},
        TestParam{"TestMessageSetLazy",
                  [](Arena& arena) -> Message* {
                    auto* original = CreateTestMessageSet(arena);
                    auto* parsed = Arena::Create<TestMessageSet>(&arena);
                    ABSL_CHECK(
                        parsed->ParseFromString(original->SerializeAsString()));
                    return parsed;
                  }},
        TestParam{"TestOneof2LazyField",
                  [](Arena& arena) -> Message* {
                    auto* msg = Arena::Create<TestOneof2>(&arena);
                    TestUtil::SetOneof2(msg);
                    msg->mutable_foo_lazy_message()->set_moo_int(0);
                    return msg;
                  }},
        TestParam{"TestPacked",
                  [](Arena& arena) -> Message* {
                    auto* msg = Arena::Create<TestPackedTypes>(&arena);
                    TestUtil::SetPackedFields(msg);
                    return msg;
                  }},
        TestParam{"TestPackedExtensions",
                  [](Arena& arena) -> Message* {
                    auto* msg = Arena::Create<TestPackedExtensions>(&arena);
                    TestUtil::SetPackedExtensions(msg);
                    return msg;
                  }},
        TestParam{"NestedTestAllTypes",
                  [](Arena& arena) -> Message* {
                    return CreateNestedTestAllTypes(arena);
                  }},
        TestParam{"NestedTestAllTypesLazy",
                  [](Arena& arena) -> Message* {
                    auto* original = CreateNestedTestAllTypes(arena);
                    auto* parsed = Arena::Create<NestedTestAllTypes>(&arena);
                    ABSL_CHECK(
                        parsed->ParseFromString(original->SerializeAsString()));
                    return parsed;
                  }}),
    [](const testing::TestParamInfo<TestParam>& info) {
      return std::string(info.param.name);
    });

template <typename T>
void MutateMapValue(TestMap& message, absl::string_view name, int index,
                    T&& callback) {
  const Reflection* reflection = message.GetReflection();
  const Descriptor* descriptor = message.GetDescriptor();
  const FieldDescriptor* field = descriptor->FindFieldByName(name);

  auto* map_entry = reflection->MutableRepeatedMessage(&message, field, index);
  const FieldDescriptor* val_field = map_entry->GetDescriptor()->map_value();
  ABSL_CHECK_NE(val_field, nullptr);
  callback(map_entry->GetReflection(), map_entry, val_field);
}

TEST(ReflectionVisitTest, VisitMapAfterMutableRepeated) {
  TestMap message;
  auto& map = *message.mutable_map_int32_int32();
  map[0] = 0;
  map[1] = 0;

  // Reflectively overwrites values to 200 for all entries. This forces
  // conversion to a mutable repeated field.
  auto set_int32_val = [&](const Reflection* reflection, Message* msg,
                           const FieldDescriptor* field) {
    reflection->SetInt32(msg, field, 200);
  };
  MutateMapValue(message, "map_int32_int32", 0, set_int32_val);
  MutateMapValue(message, "map_int32_int32", 1, set_int32_val);

  // Later visit fields must be map fields synced with the change.
  std::vector<std::pair<int32_t, int32_t>> key_val_pairs;
  VisitFields(message, [&](auto info) {
    if constexpr (info.is_map) {
      ASSERT_EQ(info.key_type(), FieldDescriptor::TYPE_INT32);
      ASSERT_EQ(info.value_type(), FieldDescriptor::TYPE_INT32);

      info.VisitElements([&](auto key, auto val) {
        if constexpr (key.cpp_type == FieldDescriptor::CPPTYPE_INT32 &&
                      val.cpp_type == FieldDescriptor::CPPTYPE_INT32) {
          key_val_pairs.emplace_back(key.Get(), val.Get());
        }
      });
    }
  });

  EXPECT_THAT(key_val_pairs, testing::UnorderedElementsAre(
                                 testing::Pair(0, 200), testing::Pair(1, 200)));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
