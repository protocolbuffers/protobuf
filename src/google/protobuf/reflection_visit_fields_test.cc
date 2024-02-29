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

#ifdef __cpp_if_constexpr

TEST(ReflectionVisitTest, VisitedFieldCountMatchesListFields) {
  protobuf_unittest::TestAllTypes message;
  TestUtil::SetAllFields(&message);
  const Reflection* reflection = message.GetReflection();

  uint32_t count = 0;
  VisitFields(message, [&](auto info) { count++; });

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);

  EXPECT_EQ(count, fields.size());
}

TEST(ReflectionVisitTest, VisitedFieldCountMatchesListFieldsForExtension) {
  protobuf_unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);
  const Reflection* reflection = message.GetReflection();

  uint32_t count = 0;
  VisitFields(message, [&](auto info) { count++; });

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);

  EXPECT_EQ(count, fields.size());
}

TEST(ReflectionVisitTest, VisitedFieldCountMatchesListFieldsForMessageType) {
  protobuf_unittest::TestAllTypes message;
  TestUtil::SetAllFields(&message);
  const Reflection* reflection = message.GetReflection();

  uint32_t count = 0;
  VisitFields(message, [&](auto info) { count++; }, FieldMask::kMessage);

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  int message_count = 0;
  for (auto field : fields) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) ++message_count;
  }

  EXPECT_EQ(count, message_count);
}

TEST(ReflectionVisitTest, VisitedFieldCountMatchesListFieldsForLazy) {
  protobuf_unittest::NestedTestAllTypes original, parsed;
  TestUtil::SetAllFields(original.mutable_payload());
  TestUtil::SetAllFields(original.mutable_lazy_child()->mutable_payload());
  ASSERT_TRUE(parsed.ParseFromString(original.SerializeAsString()));
  const Reflection* reflection = parsed.GetReflection();

  uint32_t count = 0;
  VisitFields(parsed, [&](auto info) { count++; });

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(parsed, &fields);

  EXPECT_EQ(count, fields.size());
}

TEST(ReflectionVisitTest,
     VisitedFieldCountMatchesListFieldsForExtensionMessageType) {
  protobuf_unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);
  const Reflection* reflection = message.GetReflection();

  uint32_t count = 0;
  VisitFields(message, [&](auto info) { count++; }, FieldMask::kMessage);

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  int message_count = 0;
  for (auto field : fields) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) ++message_count;
  }

  EXPECT_EQ(count, message_count);
}

TEST(ReflectionVisitTest, VisitedFieldCountMatchesListFieldsForMap) {
  protobuf_unittest::TestMap message;
  MapTestUtil::SetMapFields(&message);
  MapTestUtil::ExpectMapFieldsSet(message);
  const Reflection* reflection = message.GetReflection();

  uint32_t count = 0;
  VisitFields(message, [&](auto info) { count++; });

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);

  EXPECT_EQ(count, fields.size());
}

TEST(ReflectionVisitTest, ClearByVisitIsEmpty) {
  protobuf_unittest::TestAllTypes message;
  TestUtil::SetAllFields(&message);
  TestUtil::ExpectAllFieldsSet(message);

  VisitFields(message, [&](auto info) { info.Clear(); });

  TestUtil::ExpectClear(message);
}

TEST(ReflectionVisitTest, ClearByVisitIsEmptyForExtension) {
  protobuf_unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);
  TestUtil::ExpectAllExtensionsSet(message);

  VisitFields(message, [&](auto info) { info.Clear(); });

  TestUtil::ExpectExtensionsClear(message);
}

TEST(ReflectionVisitTest, ClearByVisitHasZeroRevisitForExtension) {
  protobuf_unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);
  TestUtil::ExpectAllExtensionsSet(message);

  // Clear all fields.
  VisitFields(message, [&](auto info) { info.Clear(); });
  // Visiting clear message should yields no fields.
  uint32_t count = 0;
  VisitFields(message, [&](auto info) { ++count; });

  EXPECT_EQ(count, 0);
  TestUtil::ExpectExtensionsClear(message);
}

TEST(ReflectionVisitTest, ClearByVisitHasZeroRevisitForLazy) {
  protobuf_unittest::NestedTestAllTypes original, parsed;
  TestUtil::SetAllFields(original.mutable_payload());
  TestUtil::SetAllFields(original.mutable_lazy_child()->mutable_payload());
  ASSERT_TRUE(parsed.ParseFromString(original.SerializeAsString()));

  VisitFields(parsed, [&](auto info) { info.Clear(); });
  // Visiting clear message should yields no fields.
  uint32_t count = 0;
  VisitFields(parsed, [&](auto info) { ++count; });

  EXPECT_EQ(count, 0);
}

TEST(ReflectionVisitTest, ClearByVisitIsEmptyForMap) {
  protobuf_unittest::TestMap message;
  MapTestUtil::SetMapFields(&message);
  MapTestUtil::ExpectMapFieldsSet(message);

  VisitFields(message, [&](auto info) { info.Clear(); });

  MapTestUtil::ExpectClear(message);
}

template <typename T>
void MutateMapValue(protobuf_unittest::TestMap& message, absl::string_view name,
                    int index, T&& callback) {
  const Reflection* reflection = message.GetReflection();
  const Descriptor* descriptor = message.GetDescriptor();
  const FieldDescriptor* field = descriptor->FindFieldByName(name);

  auto* map_entry = reflection->MutableRepeatedMessage(&message, field, index);
  const FieldDescriptor* val_field = map_entry->GetDescriptor()->map_value();
  ABSL_CHECK_NE(val_field, nullptr);
  callback(map_entry->GetReflection(), map_entry, val_field);
}

TEST(ReflectionVisitTest, VisitMapAfterMutableRepeated) {
  protobuf_unittest::TestMap message;
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
          it = it;
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

TEST(ReflectionVisitTest, ReadAndWriteBackIdempotent) {
  protobuf_unittest::TestAllTypes message;
  TestUtil::SetAllFields(&message);

  MutateNothingByVisit(message);

  TestUtil::ExpectAllFieldsSet(message);
}

TEST(ReflectionVisitTest, ReadAndWriteBackIdempotentForExtension) {
  protobuf_unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);

  MutateNothingByVisit(message);

  TestUtil::ExpectAllExtensionsSet(message);
}

template <typename InfoT>
inline size_t MapKeyByteSizeLong(FieldDescriptor::Type type, InfoT info) {
  if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_STRING) {
    return WireFormatLite::StringSize(info.Get());
  } else {
    return MapPrimitiveFieldByteSize<info.cpp_type>(type, info.Get());
  }
}

template <typename InfoT>
inline size_t MapValueByteSizeLong(FieldDescriptor::Type type, InfoT info) {
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

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegen) {
  protobuf_unittest::TestAllTypes message;
  TestUtil::SetAllFields(&message);
  TestUtil::ExpectAllFieldsSet(message);

  EXPECT_EQ(ByteSizeLongByVisit(message), message.ByteSizeLong());
}

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegenForPacked) {
  protobuf_unittest::TestPackedTypes message;
  TestUtil::SetPackedFields(&message);
  TestUtil::ExpectPackedFieldsSet(message);

  EXPECT_EQ(ByteSizeLongByVisit(message), message.ByteSizeLong());
}

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegenForExtension) {
  protobuf_unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);
  TestUtil::ExpectAllExtensionsSet(message);

  EXPECT_EQ(ByteSizeLongByVisit(message), message.ByteSizeLong());
}

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegenForPackedExtensions) {
  protobuf_unittest::TestPackedExtensions message;
  TestUtil::SetPackedExtensions(&message);
  TestUtil::ExpectPackedExtensionsSet(message);

  EXPECT_EQ(ByteSizeLongByVisit(message), message.ByteSizeLong());
}

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegenForLazyExtension) {
  protobuf_unittest::TestAllExtensions original, parsed;
  TestUtil::SetAllExtensions(&original);
  TestUtil::ExpectAllExtensionsSet(original);
  std::string data;
  ASSERT_TRUE(original.SerializeToString(&data));
  ASSERT_TRUE(parsed.ParseFromString(data));

  EXPECT_EQ(ByteSizeLongByVisit(parsed), parsed.ByteSizeLong());
}

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegenForMessageSet) {
  proto2_wireformat_unittest::TestMessageSet message;
  auto* ext1 = message.MutableExtension(
      unittest::TestMessageSetExtension1::message_set_extension);
  ext1->set_i(-1);
  ext1->mutable_recursive()
      ->MutableExtension(
          unittest::TestMessageSetExtension3::message_set_extension)
      ->mutable_msg()
      ->set_b(0);

  EXPECT_EQ(ByteSizeLongByVisit(message), message.ByteSizeLong());
}

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegenForLazyMessageSet) {
  proto2_wireformat_unittest::TestMessageSet original, parsed;
  auto* ext1 = original.MutableExtension(
      unittest::TestMessageSetExtension1::message_set_extension);
  ext1->set_i(-1);

  auto* ext3 = ext1->mutable_recursive()->MutableExtension(
      unittest::TestMessageSetExtension3::message_set_extension);
  ext3->mutable_msg()->set_b(0);
  ext3->set_required_int(-1);

  std::string data;
  ASSERT_TRUE(original.SerializeToString(&data));
  ASSERT_TRUE(parsed.ParseFromString(data));

  EXPECT_EQ(ByteSizeLongByVisit(parsed), parsed.ByteSizeLong());
}

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegenForLazy) {
  protobuf_unittest::NestedTestAllTypes original, parsed;
  TestUtil::SetAllFields(original.mutable_payload());
  TestUtil::SetAllFields(original.mutable_lazy_child()->mutable_payload());
  std::string data;
  ASSERT_TRUE(original.SerializeToString(&data));
  ASSERT_TRUE(parsed.ParseFromString(data));

  size_t byte_size_visit = ByteSizeLongByVisit(parsed);

  EXPECT_EQ(byte_size_visit, parsed.ByteSizeLong());
  EXPECT_EQ(byte_size_visit, data.size());
}

TEST(ReflectionVisitTest, ByteSizeByVisitMatchesCodegenForMap) {
  protobuf_unittest::TestMap message;
  MapTestUtil::SetMapFields(&message);
  MapTestUtil::ExpectMapFieldsSet(message);

  EXPECT_EQ(ByteSizeLongByVisit(message), message.ByteSizeLong());
}

#endif  // __cpp_if_constexpr

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
