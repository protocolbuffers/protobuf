// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/file.h"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/unittest.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class FileGeneratorFriendForTesting {
 public:
  static std::vector<const Descriptor*> MessagesInTopologicalOrder(
      const FileGenerator& fgen) {
    return fgen.MessagesInTopologicalOrder();
  }
};

namespace {
// Test that the descriptors are ordered in a topological order.
TEST(FileTest, TopologicallyOrderedDescriptors) {
  const FileDescriptor* fdesc =
      proto2_unittest::TestAllTypes::descriptor()->file();
  FileGenerator fgen(fdesc, /*options=*/{});
  static constexpr absl::string_view kExpectedDescriptorOrder[] = {
      "Uint64Message",
      "Uint32Message",
      "TestVerifyUint32Simple",
      "TestVerifyInt32Simple",
      "TestVerifyBigFieldNumberUint32.Nested",
      "TestUnpackedTypes",
      "TestUnpackedExtensions",
      "TestString",
      "TestReservedFields",
      "TestRequiredOpenEnum",
      "TestRequiredOneof.NestedMessage",
      "TestRequiredNoMaskMulti",
      "TestRequiredEnumNoMask",
      "TestRequiredEnumMulti",
      "TestRequiredEnum",
      "TestRepeatedString",
      "TestRepeatedScalarDifferentTagSizes",
      "TestRecursiveMessage",
      "TestReallyLargeTagNumber",
      "TestPickleNestedMessage.NestedMessage.NestedNestedMessage",
      "TestPickleNestedMessage.NestedMessage",
      "TestPickleNestedMessage",
      "TestPackedTypes",
      "TestPackedExtensions",
      "TestPackedEnumSmallRange",
      "TestOneofBackwardsCompatible.FooGroup",
      "TestOneof2.NestedMessage",
      "TestOneof2.FooGroup",
      "TestOneof.FooGroup",
      "TestNestedMessageRedaction",
      "TestNestedGroupExtensionOuter.Layer1OptionalGroup.Layer2RepeatedGroup",
      "TestNestedGroupExtensionOuter.Layer1OptionalGroup."
      "Layer2AnotherOptionalRepeatedGroup",
      "TestNestedGroupExtensionInnerExtension",
      "TestNestedExtension.OptionalGroup_extension",
      "TestNestedExtension",
      "TestMultipleExtensionRanges",
      "TestMixedFieldsAndExtensions",
      "TestMessageWithManyRepeatedPtrFields",
      "TestMessageSize",
      "TestMessageForMove_Small",
      "TestMessageForMove_Large",
      "TestJsonName",
      "TestIsInitialized.SubMessage.SubGroup",
      "TestHugeFieldNumbers.StringStringMapEntry",
      "TestHugeFieldNumbers.OptionalGroup",
      "TestGroupExtension",
      "TestGroup.OptionalGroup",
      "TestFieldOrderings.NestedMessage",
      "TestExtremeDefaultValues",
      "TestExtensionRangeSerialize",
      "TestExtensionOrderings2.TestExtensionOrderings3",
      "TestExtensionOrderings2",
      "TestExtensionOrderings1",
      "TestExtensionInsideTable",
      "TestEmptyMessageWithExtensions",
      "TestEmptyMessage",
      "TestEagerlyVerifiedLazyMessage.LazyMessage",
      "TestDynamicExtensions.DynamicMessageType",
      "TestDupFieldNumber.Foo",
      "TestDupFieldNumber.Bar",
      "TestDeprecatedMessage",
      "TestCord",
      "TestCommentInjectionMessage",
      "TestChildExtensionData.NestedTestAllExtensionsData."
      "NestedDynamicExtensions",
      "TestAllTypes.RepeatedGroup",
      "TestAllTypes.OptionalGroup",
      "TestAllTypes.NestedMessage",
      "TestAllExtensions",
      "StringParseTester",
      "SparseEnumMessage",
      "RepeatedGroup_extension",
      "RedactedFields.MapUnredactedStringEntry",
      "RedactedFields.MapRedactedStringEntry",
      "OptionalGroup_extension",
      "OpenEnumMessage",
      "OneString",
      "OneBytes",
      "MoreString",
      "MoreBytes",
      "MessageCreatorZeroInit",
      "MessageCreatorMemcpy.M2Entry",
      "MessageCreatorFunc",
      "ManyOptionalString",
      "Int64ParseTester",
      "Int64Message",
      "Int32ParseTester",
      "Int32Message",
      "InlinedStringIdxRegressionProto",
      "ForeignMessage",
      "FooServerMessage",
      "FooResponse",
      "FooRequest",
      "FooClientMessage",
      "FastParseTableCompression",
      "EnumsForBenchmark",
      "EnumParseTester",
      "BoolParseTester",
      "BoolMessage",
      "BarResponse",
      "BarRequest",
      "BadFieldNames",
      "TestVerifyBigFieldNumberUint32",
      "TestRequiredOneof",
      "TestRequired",
      "TestOneof2",
      "TestNestedMessageHasBits.NestedMessage",
      "TestNestedGroupExtensionOuter.Layer1OptionalGroup",
      "TestMergeException",
      "TestIsInitialized.SubMessage",
      "TestGroup",
      "TestForeignNested",
      "TestFieldOrderings",
      "TestEagerlyVerifiedLazyMessage",
      "TestEagerMaybeLazy.NestedMessage",
      "TestDynamicExtensions",
      "TestDupFieldNumber",
      "TestDeprecatedFields",
      "TestChildExtensionData.NestedTestAllExtensionsData",
      "TestChildExtension",
      "TestCamelCaseFieldNames",
      "TestAllTypes",
      "RedactedFields",
      "MessageCreatorMemcpy",
      "TestVerifyUint32BigFieldNumber",
      "TestVerifyUint32",
      "TestVerifyOneUint32",
      "TestVerifyOneInt32BigFieldNumber",
      "TestVerifyMostlyInt32BigFieldNumber",
      "TestVerifyMostlyInt32",
      "TestVerifyInt32BigFieldNumber",
      "TestVerifyInt32",
      "TestRequiredMessage",
      "TestParsingMerge.RepeatedGroup",
      "TestParsingMerge.RepeatedFieldsGenerator.Group2",
      "TestParsingMerge.RepeatedFieldsGenerator.Group1",
      "TestParsingMerge.OptionalGroup",
      "TestOneofBackwardsCompatible",
      "TestOneof",
      "TestNestedMessageHasBits",
      "TestNestedGroupExtensionOuter",
      "TestNestedChildExtension",
      "TestMutualRecursionA.SubGroupR",
      "TestLazyMessage",
      "TestIsInitialized",
      "TestHugeFieldNumbers",
      "TestEagerMessage",
      "TestEagerMaybeLazy",
      "TestChildExtensionData",
      "NestedTestAllTypes",
      "TestRequiredForeign",
      "TestParsingMerge.RepeatedFieldsGenerator",
      "TestParsingMerge",
      "TestNestedChildExtensionData",
      "TestMutualRecursionA",
      "TestMutualRecursionA.SubGroup",
      "TestMutualRecursionA.SubMessage",
      "TestMutualRecursionB",
      "TestLazyMessageRepeated",
      "TestNestedRequiredForeign",
  };
  std::vector<std::string> actual_order;
  for (const Descriptor* desc :
       FileGeneratorFriendForTesting::MessagesInTopologicalOrder(fgen)) {
    actual_order.emplace_back(
        absl::StripPrefix(desc->full_name(), "proto2_unittest."));
  }
  EXPECT_THAT(actual_order,
              ::testing::ElementsAreArray(kExpectedDescriptorOrder));
}

}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
