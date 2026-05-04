// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <string>

#include <gtest/gtest.h>
#include "upb/test/fuzz_util.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"
#include "upb/wire/fuzz_impl.h"
#include "upb/wire/internal/eps_copy_input_stream.h"
#include "upb/wire/types.h"

namespace {

TEST(FuzzTest, DecodeUnknownProto2EnumExtension) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"\256\354Rt\216\3271\234", "\243\243\267\207\336gV\366w"},
       {"z"},
       "}\212\304d\371\363\341\2329\325B\264\377?\215\223\201\201\226y\201%"
       "\321\363\255;",
       {}},
      "\010", -724543908, -591643538);
}

TEST(FuzzTest, DecodeExtensionEnsurePresenceInitialized) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"\031", "S", "\364", "", "", "j", "\303", "", "\224", "\277"},
       {},
       "_C-\236$*)C0C>",
       {4041515984, 2147483647, 1929379871, 0, 3715937258, 4294967295}},
      "\010\002", 342248070, -806315555);
}

TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"\n"}, {""}, ".\244", {}}, "\013\032\005\212a#\365\336\020\001\226",
      14803219, 670718349);
}

TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage2) {
  DecodeEncodeArbitrarySchemaAndPayload({{"\n", "G", "\n", "\274", ""},
                                         {"", "\030"},
                                         "_@",
                                         {4294967295, 2147483647}},
                                        std::string("\013\032\000\220", 4),
                                        279975758, 1647495141);
}

TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage3) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"\n"}, {"B", ""}, "\212:b", {11141121}},
      "\013\032\004\357;7\363\020\001\346\240\200\201\271", 399842149,
      -452966025);
}

TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage4) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"\n", "3\340", "\354"}, {}, "B}G", {4294967295, 4082331310}},
      "\013\032\004\244B\331\255\020\001\220\224\243\350\t", -561523015,
      1683327312);
}

TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage5) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"\n"}, {""}, "kB", {0}},
      "x\203\251\006\013\032\002S\376\010\273\'\020\014\365\207\244\234",
      -696925610, -654590577);
}

TEST(FuzzTest, ExtendMessageSetWithEmptyExtension) {
  DecodeEncodeArbitrarySchemaAndPayload({{"\n"}, {}, "_", {}}, std::string(), 0,
                                        0);
}

TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"\320", "\320", "\320", "\320", "\320", "%2%%%%%"},
       {"", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", ""},
       "\226\226\226\226\226\226\350\351\350\350\350\350\350\350\350\314",
       {4026531839}},
      std::string("\n\n\n\n\272\n======@@%%%%%%%%%%%%%%%@@@(("
                  "qqqqqqqq5555555555qqqqqffq((((((((((((\335@@>"
                  "\ru\360ncppppxxxxxxxxx\025\025\025xxxxxppppppp<="
                  "\2165\275\275\315\217\361\010\t\000\016\013in\n\n\n\256\263",
                  130),
      901979906, 65537);
}

// This test encodes a map field with extra cruft.
TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegressionInvalidMap) {
  DecodeEncodeArbitrarySchemaAndPayload({{"%%%%///////"}, {}, "", {}},
                                        std::string("\035|", 2), 65536, 3);
}

// This test found a case where presence was unset for a mini table field.
TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegressionMsan) {
  DecodeEncodeArbitrarySchemaAndPayload({{"%-#^#"}, {}, "", {}}, std::string(),
                                        -1960166338, 16809991);
}

// This test encodes a map containing a msg wrapping another, empty msg.
TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegressionMapMap) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"%#G"}, {}, "", {}}, std::string("\022\002\022\000", 4), 0, 0);
}

// This test found a case where a fixed-size field caused a read from beyond the
// end of the input buffer when being decoded as an unknown field.
TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadFieldInSlopShort) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"I", "a", "I", "I", "I", "I", ""},
                                    {"P"},
                                    "\316\316\316\316\316\316\316\316",
                                    {}},
      "\201\201X\201F\363", 0, 0, false);
}

// This tests the same thing, except that the input buffer is long enough that
// it triggers the transition to the slop buffer during decoding rather than at
// the beginning.
TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadFieldInSlopLong) {
  size_t buffer_size = kUpb_EpsCopyInputStream_SlopBytes + 1;
  std::string buffer(buffer_size, '\0');
  // Length-delimited unknown field
  buffer[0] = (char)((15u << 3) | kUpb_WireType_Delimited);
  buffer[1] = buffer_size - 3;
  // Split fixed field over end of buffer
  buffer[buffer_size - 1] = (char)((15u << 3) | kUpb_WireType_64Bit);
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"I", "a", "I", "I", "I", "I", ""},
                                    {"P"},
                                    "\316\316\316\316\316\316\316\316",
                                    {}},
      buffer, 0, 0, false);
}

TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadFieldInSlopDelimited) {
  size_t buffer_size = kUpb_EpsCopyInputStream_SlopBytes + 1;
  std::string buffer(buffer_size, '\0');
  // Length-delimited unknown field
  buffer[0] = (char)((15u << 3) | kUpb_WireType_Delimited);
  // Length points one byte beyond the end of the buffer
  buffer[1] = buffer_size - 1;
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"I", "a", "I", "I", "I", "I", ""},
                                    {"P"},
                                    "\316\316\316\316\316\316\316\316",
                                    {}},
      buffer, 0, 0, false);
}

TEST(FuzzTest, GroupMap) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {.mini_descriptors = {"$$FF$", "%-C"},
       .enum_mini_descriptors = {},
       .extensions = "",
       .links = {1}},
      std::string(
          "\023\020\030\233\000\204\330\372#\000`"
          "a\000\000\001\000\000\000ccccccc\030s\273sssssssss\030\030\030\030"
          "\030\030\030\030\215\215\215\215\215\215\215\215\030\030\232\253\253"
          "\232*\334\227\273\231\207\373\t\0051\305\265\335\224\226"),
      0, 0);
}

TEST(FuzzTest, MapUnknownFieldSpanBuffers) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"$   3", "%# "}, {}, "", {1}},
      std::string(
          "\"\002\010\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
          17),
      0, 0);
}

TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression22) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"$2222222222222222222222", "%,&"}, {}, "", {1}},
      std::string("\035\170\170\170\051\263\001\030\000\035\357\357\340\021\035"
                  "\025\331\035\035\035\035\035\035\035\035",
                  25),
      0, 0);
}

TEST(FuzzTest, ExtensionWithoutExt) {
  DecodeEncodeArbitrarySchemaAndPayload({{"$ 3", "", "%#F"}, {}, "", {2, 1}},
                                        std::string("\022\002\010\000", 4), 0,
                                        0);
}

TEST(FuzzTest, MapFieldVerify) {
  DecodeEncodeArbitrarySchemaAndPayload({{"%  ^!"}, {}, "", {}}, "", 0, 0);
}

TEST(FuzzTest, TooManyRequiredFields) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"$ N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N "
        "N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N"},
       {},
       "",
       {}},
      "", 0, 4);

  std::string desc = "$ N";
  for (int i = 0; i < 70; ++i) {
    desc += " N";
    DecodeEncodeArbitrarySchemaAndPayload({{desc}, {}, "", {}}, "", 0, 4);
  }
}

TEST(FuzzTest, LowDecodeDepthLimit) {
  DecodeEncodeArbitrarySchemaAndPayload({{"$ gF"}, {}, "", {}},
                                        std::string("J\000", 2), 0,
                                        upb_EncodeOptions_MaxDepth(1), false);
}

TEST(FuzzTest, EncoderOnlyChecksRequired) {
  DecodeEncodeArbitrarySchemaAndPayload({{"$!N"}, {}, "", {}},
                                        std::string("\010\000", 2), 0,
                                        kUpb_EncodeOption_CheckRequired, false);
}

TEST(FuzzTest, EqualDepthLimits) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"$3"}, {}, "", {}}, std::string("\n\000", 2),
      upb_DecodeOptions_MaxDepth(1), upb_EncodeOptions_MaxDepth(1), false);
}

TEST(FuzzTest, StartGroupExceedsDepthLimit) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$  2"}, {}, "", {}}, "\033\033",
      upb_DecodeOptions_MaxDepth(1), upb_EncodeOptions_MaxDepth(1), false);
}

TEST(FuzzTest, DepthLimitMismatch) {
  DecodeEncodeArbitrarySchemaAndPayload(
      {{"$3"}, {}, "", {}}, std::string("\n\000", 2), 0, 65538, false);
}

TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression123) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$P"}, {""}, "#H", {}},
      std::string("\t\000\000\000\000\000\000\000\000\n$"
                  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000\000\000\000\000",
                  47),
      0, 2, false);
}

TEST(FuzzTest, MaxSizeWithinSlopBufferRegion) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{""}},
      std::string(
          // Delimited field with size=INT_MAX-1
          "\x0a\xfe\xff\xff\xff\x07"
          // 10 bytes of garbage, to ensure that the previous field straddles
          // the slop region cutoff.
          "zzzzzzzzzzz"),
      0, 0, false);
}

TEST(FuzzTest, ExtensionNumberZero) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$P"}, {""}, "#_4", {}},
      std::string("\000\000", 2), 0, 0, false);
}

TEST(FuzzTest, UnknownFieldDepth100) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{""}, {}, "", {}},
      "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
      "ccccccccccccccccccccccccccc\013\014ddddddddddddddddddddddddddddddddddddd"
      "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd",
      0, 0, false);
}

TEST(FuzzTest, AliasingMessageSetValue) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"&"}, {}, "#3", {}},
      std::string("\x0a\x05\x0dzzzz", 7), kUpb_DecodeOption_AliasString, 0,
      false);
}

TEST(FuzzTest, AliasingMessageSetOutOfOrder) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"&", "$P"}, {}, "# # #2", {1, 1, 0}},
      std::string("\013\032\001\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\020\001",
                  18),
      0, 0, false);
}

TEST(FuzzTest, FieldNumberZero) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$_<"}, {}, "", {}},
      std::string("\000\000", 2), 0, 0, false);
}

TEST(FuzzTest, FastFieldFollowedBySlow) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$ !"}, {}, "", {}},
      std::string("\021\000\000\000\000\000\000\000\000\025\000\000\000\000",
                  14),
      0, 0, false);
}

TEST(FuzzTest, FastDecoderShouldRejectInvalidWireType) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$ "}, {}, "", {}},
      std::string("\010\000\000\000\000\000\000\000\000", 9), 0, 0, false);
}

TEST(FuzzTest, PackedVarintTruncated) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$<"}, {}, std::string("\000", 1), {}},
      std::string("\010\000\n\010\000\000\000\000\000\000\000\200", 12), 0, 0,
      false);
}

TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression1212122) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{
          {"$AAAAAAAAAAAAA/AAAAA"}, {""}, std::string("\000", 1), {}},
      "\020", 0, 0, false);
}

TEST(FuzzTest, LongDelimitedSize) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$6"}, {}, "", {}},
      std::string("\n\210\200\000\000", 5), 0, 0, false);
}

TEST(FuzzTest, FailedArrayAllocation) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$7"}, {}, "", {}},
      std::string("\n\004\000\000\000\000", 6), 0, 0, false);
}

TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression111) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$ !6"}, {}, "", {}},
      std::string("\032\010\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000",
                  17),
      0, 0, false);
}

TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression111122) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$(_ "}, {}, "", {}},
      std::string("\t\000\000\000\000\000\000\000\000", 9), 0, 0, false);
}

TEST(FuzzTest, FallbackBetweenPackedTagAndPayload) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$<"}, {}, "", {}},
      std::string(
          "\n\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
          17),
      0, 0, false);
}

TEST(FuzzTest, GreaterThanTwoByteSizeVarint) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"\003", "$C"}, {"."}, "", {}},
      "\n\207\207\207\207\207\207\207\207\207\207\207\207\207\207\207\207\207"
      "\207\207\207",
      1958399105, 2147481599, false);
}

TEST(FuzzTest, AppendsToExistingArray) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$7"}, {}, "", {}},
      std::string("\r\000\000\000\000\r\000\000\000\000\r\000\000\000\000\r\000"
                  "\000\000\000\r\000\000\000\000\r\000\000\000\000\n\014\000"
                  "\000\000\000\000\000\000\000\000\000\000\000",
                  44),
      0, 0, false);
}

TEST(FuzzTest, AppendToFullTraceBuffer) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$("}, {}, "", {}},
      std::string(
          "\010\000\r\000\000\000\000\010\000\010\000\010\000\010\000\010\000"
          "\010\000\010\000\010\000\010\000\010\000\010\000\010\000\010\000\010"
          "\000\010\000\010\000\010\000\010\000\010\000\010\000\010\000\010\000"
          "\010\000\010\000\010\000\010\000\010\000\r\000\000\000\000\010\000"
          "\000\000\000\000\000\000\000\000\000\000",
          78),
      0, 0, false);
}

TEST(FuzzTest, PrematureEOFInString) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$  0"}, {}, "", {}},
      std::string("\032`"
                  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000\000\000\000\000\r\000\000\000\000\032\021",
                  105),
      0, 0, false);
}

TEST(FuzzTest, ClosedEnumExtensionUnknownValue) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$P"}, {""}, "#4", {}},
      std::string("\010\000", 2), 0, 0, false);
}

TEST(FuzzTest, MapEntryWithoutKeyOrValue) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$G", "%#3"}, {}, "", {1, 0}},
      std::string("\n\000", 2), 65536, 0, false);
}

TEST(FuzzTest, MapValueIsEnumLackingZero) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$ G", "%#4"}, {""}, "", {1}},
      std::string("\022\000", 2), 0, 0, false);
}

TEST(FuzzTest, ClosedEnumWithNoEnumDescriptor) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$4"}, {}, "", {}}, "", 0, 0, false);
}

TEST(FuzzTest, PrematureEofInUnknownVarint) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{""}, {}, "", {}}, "\010", 0, 0, false);
}

TEST(FuzzTest, EmptyInput) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$4"}, {}, "", {}}, "", 0, 0, false);
}

TEST(FuzzTest, DelimintedFieldPrematureEofPastSlopRegion) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$<"}, {}, "", {}}, "\n\021", 0, 0, false);
}

TEST(FuzzTest, PrematureEofInPackedFixedField) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$7"}, {}, "", {}}, "\n\024", 0, 0, false);
}

TEST(FuzzTest, EnumFieldWithNoEnumTable) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$4"}, {}, "", {}}, "", 0, 0, false);
}

TEST(FuzzTest, PrematureEofInStringField) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$>D@1"}, {}, "", {}}, "\"\203\004", 8, 0,
      false);
}

TEST(FuzzTest, PrematureEofInRepeatedString) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$D"}, {}, "", {}}, "\n\021", 0, 0, false);
}

TEST(FuzzTest, PrematureEofInSubMessage) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$3"}, {}, "", {}}, "\n\002\010", 0, 0,
      false);
}

TEST(FuzzTest, SkipsEntireSlopRegion) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$3"}, {}, "", {}}, "\012\024\013\012\020",
      0, 0, false);
}

TEST(FuzzTest, StartGroupDepthOverflowKnownToUnknown) {
  const char payload1[] =
      "\"\000\"\000\"\000\"\000\"\000\"\000\"\000\"\000\"\000\"\000\"\000######"
      "####################################\212\000#"
      "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
      "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000####"
      "###################"
      "\"\000\"\000\"\000\"\000\013";
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$   F"}, {}, "", {}},
      std::string(payload1, sizeof(payload1) - 1) + std::string(150000, '\003'),
      0, 0, false);
}

TEST(FuzzTest, MissingRequiredFieldInNonLastMessage) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$ 2N"}, {}, "", {}}, "\023\024", 65538, 4,
      false);
}

TEST(FuzzTest, UnknownFieldMaxFieldNumber) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{""}, {}, "", {}},
      "\xf8\xff\xff\xff\x0f\x00", 0, 0, false);
}

TEST(FuzzTest, InvalidEndForUnknownGroup) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{""}, {}, "", {}}, "\r", 0, 0, false);
}

TEST(FuzzTest, WireErrorInsideUnknownGroup) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{""}, {}, "", {}}, "\013\002", 1, 0, false);
}

TEST(FuzzTest, WireErrorInsideUnknownGroup2) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"&"}, {}, std::string("\000", 1), {}},
      "\013\002", 0, 0, false);
}

TEST(FuzzTest, UnknownGroupExceededDepthLimit) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$2"}, {}, "", {}},
      std::string("\n\n\000\000\000\000\000\000\000\000\000\000\013\023", 14),
      65537, 0, false);
}

TEST(FuzzTest, MapValueIsMessageHasMissingRequiredField) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$BgGGHG$>(N", "%&3"}, {}, "", {1, 0}},
      std::string("b\000\t\000\000\000\000\000\000\000\000\t\000\000\000\000"
                  "\000\000\000\000x\000",
                  22),
      2, 0, false);
}

TEST(FuzzTest, UnmatchedEndGroupTag) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$  3"}, {}, "", {}},
      std::string("\032\002\004\000\000\000\000\000\000\000\000\000\000\000\000"
                  "\000\000",
                  17),
      0, 0, false);
}

TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression11112) {
  DecodeEncodeArbitrarySchemaAndPayload(
      upb::fuzz::MiniTableFuzzInput{{"$D"}, {}, "", {}}, "\012\001", 1, 0,
      false);
}

}  // namespace
