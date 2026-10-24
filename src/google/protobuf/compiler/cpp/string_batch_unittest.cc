// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "testing/fuzzing/fuzztest.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/compiler/cpp/string_batch_unittest.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

using ::proto2_unittest::TestBatchStringFields;
using ::proto2_unittest::TestBatchStringFieldsFallback;
using ::proto2_unittest::TestNonZeroFirstBitStrings;
using ::proto2_unittest::TestNonZeroFirstBitStringsFallback;

// Verifies that a batched message and its unbatched fallback message have
// identical field presence, field values, ByteSizeLong(), and wire
// serialization.
void ExpectEquivalent(const TestBatchStringFields& b,
                      const TestBatchStringFieldsFallback& f) {
  EXPECT_EQ(b.ByteSizeLong(), f.ByteSizeLong());

  std::string b_wire = b.SerializeAsString();
  std::string f_wire = f.SerializeAsString();
  EXPECT_EQ(b_wire, f_wire);

#define EXPECT_SAME_FIELD(name)        \
  EXPECT_EQ(b.has_##name(), f.has_##name()) << #name; \
  EXPECT_EQ(b.name(), f.name()) << #name

  EXPECT_SAME_FIELD(str1);
  EXPECT_SAME_FIELD(str2);
  EXPECT_SAME_FIELD(str_def1);
  EXPECT_SAME_FIELD(int1);
  EXPECT_SAME_FIELD(str3);
  EXPECT_SAME_FIELD(bool1);
  EXPECT_SAME_FIELD(str_cord1);
  EXPECT_SAME_FIELD(str4);
  EXPECT_SAME_FIELD(int2);
  EXPECT_SAME_FIELD(bytes1);
  EXPECT_SAME_FIELD(str5);
  EXPECT_SAME_FIELD(str6);
  EXPECT_EQ(b.has_double1(), f.has_double1());
  if (std::isnan(b.double1()) || std::isnan(f.double1())) {
    EXPECT_EQ(std::isnan(b.double1()), std::isnan(f.double1()));
  } else {
    EXPECT_DOUBLE_EQ(b.double1(), f.double1());
  }
  EXPECT_SAME_FIELD(str7);

  EXPECT_SAME_FIELD(str8);
  EXPECT_SAME_FIELD(int3);
  EXPECT_SAME_FIELD(str_def2);
  EXPECT_SAME_FIELD(str9);
  EXPECT_SAME_FIELD(bytes2);
  EXPECT_SAME_FIELD(bool2);
  EXPECT_SAME_FIELD(str10);
  EXPECT_SAME_FIELD(uint1);
  EXPECT_SAME_FIELD(str11);
  EXPECT_SAME_FIELD(str12);
  EXPECT_SAME_FIELD(fixed1);
  EXPECT_SAME_FIELD(str13);

  EXPECT_SAME_FIELD(hot_str1);
  EXPECT_SAME_FIELD(hot_int1);
  EXPECT_SAME_FIELD(hot_str2);
  EXPECT_SAME_FIELD(hot_str_def);
  EXPECT_SAME_FIELD(hot_str3);
  EXPECT_SAME_FIELD(hot_bool1);
  EXPECT_SAME_FIELD(hot_bytes1);
  EXPECT_SAME_FIELD(hot_str4);
  EXPECT_SAME_FIELD(hot_int2);
  EXPECT_SAME_FIELD(hot_str5);
  EXPECT_SAME_FIELD(hot_str6);
  EXPECT_SAME_FIELD(hot_str7);
  EXPECT_SAME_FIELD(hot_int3);
  EXPECT_SAME_FIELD(hot_str8);
  EXPECT_SAME_FIELD(hot_str9);
  EXPECT_SAME_FIELD(hot_str10);
  EXPECT_SAME_FIELD(hot_int4);

#undef EXPECT_SAME_FIELD

#define EXPECT_SAME_SUBMSG(sub)                                    \
  EXPECT_EQ(b.has_##sub(), f.has_##sub()) << #sub;                 \
  if (b.has_##sub() && f.has_##sub()) {                            \
    EXPECT_EQ(b.sub().has_id(), f.sub().has_id());                 \
    EXPECT_EQ(b.sub().id(), f.sub().id());                         \
    EXPECT_EQ(b.sub().has_name(), f.sub().has_name());             \
    EXPECT_EQ(b.sub().name(), f.sub().name());                     \
    EXPECT_EQ(b.sub().has_value(), f.sub().has_value());           \
    EXPECT_EQ(b.sub().value(), f.sub().value());                   \
    EXPECT_EQ(b.sub().has_extra(), f.sub().has_extra());           \
    EXPECT_EQ(b.sub().extra(), f.sub().extra());                   \
  }

  EXPECT_SAME_SUBMSG(sub1);
  EXPECT_SAME_SUBMSG(sub2);
  EXPECT_SAME_SUBMSG(hot_sub1);

#undef EXPECT_SAME_SUBMSG

  ASSERT_EQ(b.rep_str_size(), f.rep_str_size());
  for (int i = 0; i < b.rep_str_size(); ++i) {
    EXPECT_EQ(b.rep_str(i), f.rep_str(i));
  }

  EXPECT_EQ(static_cast<int>(b.test_oneof_case()),
            static_cast<int>(f.test_oneof_case()));
  if (b.has_oneof_str() && f.has_oneof_str()) {
    EXPECT_EQ(b.oneof_str(), f.oneof_str());
  }
  if (b.has_oneof_int() && f.has_oneof_int()) {
    EXPECT_EQ(b.oneof_int(), f.oneof_int());
  }
}

void ExpectEquivalentNonZeroFirstBit(
    const TestNonZeroFirstBitStrings& b,
    const TestNonZeroFirstBitStringsFallback& f) {
  EXPECT_EQ(b.ByteSizeLong(), f.ByteSizeLong());
  EXPECT_EQ(b.SerializeAsString(), f.SerializeAsString());

  EXPECT_EQ(b.has_lead_def(), f.has_lead_def());
  EXPECT_EQ(b.lead_def(), f.lead_def());
  EXPECT_EQ(b.has_s1(), f.has_s1());
  EXPECT_EQ(b.s1(), f.s1());
  EXPECT_EQ(b.has_mid_cord(), f.has_mid_cord());
  EXPECT_EQ(b.mid_cord(), f.mid_cord());
  EXPECT_EQ(b.has_s2(), f.has_s2());
  EXPECT_EQ(b.s2(), f.s2());
  EXPECT_EQ(b.has_mid_def(), f.has_mid_def());
  EXPECT_EQ(b.mid_def(), f.mid_def());
  EXPECT_EQ(b.has_b1(), f.has_b1());
  EXPECT_EQ(b.b1(), f.b1());
  EXPECT_EQ(b.has_s3(), f.has_s3());
  EXPECT_EQ(b.s3(), f.s3());
  EXPECT_EQ(b.has_tail_def(), f.has_tail_def());
  EXPECT_EQ(b.tail_def(), f.tail_def());
  EXPECT_EQ(b.has_tail_int1(), f.has_tail_int1());
  EXPECT_EQ(b.tail_int1(), f.tail_int1());
  EXPECT_EQ(b.has_tail_int2(), f.has_tail_int2());
  EXPECT_EQ(b.tail_int2(), f.tail_int2());
}

// Helper to set a specific string field by index [0..25] on any message type M
// (TestBatchStringFields or TestBatchStringFieldsFallback).
template <typename M>
void SetStringFieldByIndex(M* msg, int idx, absl::string_view val) {
  switch (idx % 26) {
    case 0: msg->set_str1(val); break;
    case 1: msg->set_str2(val); break;
    case 2: msg->set_str3(val); break;
    case 3: msg->set_str4(val); break;
    case 4: msg->set_bytes1(val); break;
    case 5: msg->set_str5(val); break;
    case 6: msg->set_str6(val); break;
    case 7: msg->set_str7(val); break;
    case 8: msg->set_str8(val); break;
    case 9: msg->set_str9(val); break;
    case 10: msg->set_bytes2(val); break;
    case 11: msg->set_str10(val); break;
    case 12: msg->set_str11(val); break;
    case 13: msg->set_str12(val); break;
    case 14: msg->set_str13(val); break;
    case 15: msg->set_hot_str1(val); break;
    case 16: msg->set_hot_str2(val); break;
    case 17: msg->set_hot_str3(val); break;
    case 18: msg->set_hot_bytes1(val); break;
    case 19: msg->set_hot_str4(val); break;
    case 20: msg->set_hot_str5(val); break;
    case 21: msg->set_hot_str6(val); break;
    case 22: msg->set_hot_str7(val); break;
    case 23: msg->set_hot_str8(val); break;
    case 24: msg->set_hot_str9(val); break;
    case 25: msg->set_hot_str10(val); break;
  }
}

template <typename M>
void MutateStringFieldByIndex(M* msg, int idx, absl::string_view val) {
  switch (idx % 26) {
    case 0: *msg->mutable_str1() = std::string(val); break;
    case 1: *msg->mutable_str2() = std::string(val); break;
    case 2: *msg->mutable_str3() = std::string(val); break;
    case 3: *msg->mutable_str4() = std::string(val); break;
    case 4: *msg->mutable_bytes1() = std::string(val); break;
    case 5: *msg->mutable_str5() = std::string(val); break;
    case 6: *msg->mutable_str6() = std::string(val); break;
    case 7: *msg->mutable_str7() = std::string(val); break;
    case 8: *msg->mutable_str8() = std::string(val); break;
    case 9: *msg->mutable_str9() = std::string(val); break;
    case 10: *msg->mutable_bytes2() = std::string(val); break;
    case 11: *msg->mutable_str10() = std::string(val); break;
    case 12: *msg->mutable_str11() = std::string(val); break;
    case 13: *msg->mutable_str12() = std::string(val); break;
    case 14: *msg->mutable_str13() = std::string(val); break;
    case 15: *msg->mutable_hot_str1() = std::string(val); break;
    case 16: *msg->mutable_hot_str2() = std::string(val); break;
    case 17: *msg->mutable_hot_str3() = std::string(val); break;
    case 18: *msg->mutable_hot_bytes1() = std::string(val); break;
    case 19: *msg->mutable_hot_str4() = std::string(val); break;
    case 20: *msg->mutable_hot_str5() = std::string(val); break;
    case 21: *msg->mutable_hot_str6() = std::string(val); break;
    case 22: *msg->mutable_hot_str7() = std::string(val); break;
    case 23: *msg->mutable_hot_str8() = std::string(val); break;
    case 24: *msg->mutable_hot_str9() = std::string(val); break;
    case 25: *msg->mutable_hot_str10() = std::string(val); break;
  }
}

template <typename M>
void ClearStringFieldByIndex(M* msg, int idx) {
  switch (idx % 26) {
    case 0: msg->clear_str1(); break;
    case 1: msg->clear_str2(); break;
    case 2: msg->clear_str3(); break;
    case 3: msg->clear_str4(); break;
    case 4: msg->clear_bytes1(); break;
    case 5: msg->clear_str5(); break;
    case 6: msg->clear_str6(); break;
    case 7: msg->clear_str7(); break;
    case 8: msg->clear_str8(); break;
    case 9: msg->clear_str9(); break;
    case 10: msg->clear_bytes2(); break;
    case 11: msg->clear_str10(); break;
    case 12: msg->clear_str11(); break;
    case 13: msg->clear_str12(); break;
    case 14: msg->clear_str13(); break;
    case 15: msg->clear_hot_str1(); break;
    case 16: msg->clear_hot_str2(); break;
    case 17: msg->clear_hot_str3(); break;
    case 18: msg->clear_hot_bytes1(); break;
    case 19: msg->clear_hot_str4(); break;
    case 20: msg->clear_hot_str5(); break;
    case 21: msg->clear_hot_str6(); break;
    case 22: msg->clear_hot_str7(); break;
    case 23: msg->clear_hot_str8(); break;
    case 24: msg->clear_hot_str9(); break;
    case 25: msg->clear_hot_str10(); break;
  }
}

template <typename M>
void SetNonStringFieldByIndex(M* msg, int idx, int32_t seed) {
  switch (idx % 18) {
    case 0: msg->set_str_def1(absl::StrCat("def1_", seed)); break;
    case 1: msg->set_int1(seed); break;
    case 2: msg->set_bool1((seed & 1) != 0); break;
    case 3: msg->set_str_cord1(absl::StrCat("cord_", seed)); break;
    case 4: msg->set_int2(static_cast<int64_t>(seed) * 1000000LL); break;
    case 5:
      msg->mutable_sub1()->set_id(seed);
      msg->mutable_sub1()->set_name(absl::StrCat("sub1_name_", seed));
      if (seed & 1) msg->mutable_sub1()->set_value(std::string(80, 'v'));
      if (seed & 2) msg->mutable_sub1()->set_extra("");
      break;
    case 6: msg->set_double1(seed * 1.5); break;
    case 7: msg->set_int3(-seed); break;
    case 8: msg->set_str_def2(absl::StrCat("def2_", seed)); break;
    case 9: msg->set_bool2((seed & 2) != 0); break;
    case 10:
      msg->mutable_sub2()->set_id(seed + 10);
      msg->mutable_sub2()->set_value(absl::StrCat("sub2_val_", seed));
      break;
    case 11: msg->set_uint1(static_cast<uint32_t>(seed)); break;
    case 12: msg->set_fixed1(static_cast<uint64_t>(seed) * 42ULL); break;
    case 13: msg->add_rep_str(absl::StrCat("rep_", seed)); break;
    case 14: msg->set_hot_int1(seed * 3); break;
    case 15: msg->set_hot_str_def(absl::StrCat("hot_def_", seed)); break;
    case 16:
      msg->set_hot_bool1((seed & 4) != 0);
      msg->set_hot_int2(seed * 7);
      msg->set_hot_int3(seed * 11);
      msg->set_hot_int4(seed * 13);
      break;
    case 17:
      if (seed & 1) {
        msg->set_oneof_str(absl::StrCat("oneof_", seed));
      } else {
        msg->set_oneof_int(seed);
      }
      break;
  }
}

// Exercises all lifecycle methods (CopyConstructor on heap & Arena, MergeFrom,
// CopyFrom, Swap, Clear, ByteSizeLong, SerializeAsString) on a pair of
// (TestBatchStringFields, TestBatchStringFieldsFallback) messages.
void ExerciseAllOperations(const TestBatchStringFields& b_src,
                           const TestBatchStringFieldsFallback& f_src,
                           const TestBatchStringFields& b_other,
                           const TestBatchStringFieldsFallback& f_other) {
  ExpectEquivalent(b_src, f_src);
  ExpectEquivalent(b_other, f_other);

  // 1. Copy constructor on stack/heap.
  {
    TestBatchStringFields b_copy(b_src);
    TestBatchStringFieldsFallback f_copy(f_src);
    ExpectEquivalent(b_copy, f_copy);

    // Clear the copy and verify.
    b_copy.Clear();
    f_copy.Clear();
    ExpectEquivalent(b_copy, f_copy);

    // MergeFrom after Clear.
    b_copy.MergeFrom(b_src);
    f_copy.MergeFrom(f_src);
    ExpectEquivalent(b_copy, f_copy);

    // MergeFrom with second message (overlapping and non-overlapping fields).
    b_copy.MergeFrom(b_other);
    f_copy.MergeFrom(f_other);
    ExpectEquivalent(b_copy, f_copy);

    // CopyFrom second message.
    b_copy.CopyFrom(b_other);
    f_copy.CopyFrom(f_other);
    ExpectEquivalent(b_copy, f_copy);
  }

  // 2. Swap on heap.
  {
    TestBatchStringFields b1(b_src);
    TestBatchStringFields b2(b_other);
    TestBatchStringFieldsFallback f1(f_src);
    TestBatchStringFieldsFallback f2(f_other);

    b1.Swap(&b2);
    f1.Swap(&f2);
    ExpectEquivalent(b1, f1);
    ExpectEquivalent(b2, f2);

    b1.Swap(&b2);
    f1.Swap(&f2);
    ExpectEquivalent(b1, f1);
    ExpectEquivalent(b2, f2);
  }

  // 3. Arena copy construction, MergeFrom, CopyFrom, Swap, and Clear.
  {
    Arena arena;
    auto* b1 = Arena::Create<TestBatchStringFields>(&arena, b_src);
    auto* f1 = Arena::Create<TestBatchStringFieldsFallback>(&arena, f_src);
    ExpectEquivalent(*b1, *f1);

    auto* b2 = Arena::Create<TestBatchStringFields>(&arena, b_other);
    auto* f2 = Arena::Create<TestBatchStringFieldsFallback>(&arena, f_other);
    ExpectEquivalent(*b2, *f2);

    b1->Swap(b2);
    f1->Swap(f2);
    ExpectEquivalent(*b1, *f1);
    ExpectEquivalent(*b2, *f2);

    b1->MergeFrom(*b2);
    f1->MergeFrom(*f2);
    ExpectEquivalent(*b1, *f1);

    b1->CopyFrom(b_src);
    f1->CopyFrom(f_src);
    ExpectEquivalent(*b1, *f1);

    b1->Clear();
    f1->Clear();
    ExpectEquivalent(*b1, *f1);
  }

  // 4. Cross-arena copy construction and MergeFrom.
  {
    Arena arena1;
    Arena arena2;
    auto* b1 = Arena::Create<TestBatchStringFields>(&arena1, b_src);
    auto* f1 = Arena::Create<TestBatchStringFieldsFallback>(&arena1, f_src);
    auto* b2 = Arena::Create<TestBatchStringFields>(&arena2, *b1);
    auto* f2 = Arena::Create<TestBatchStringFieldsFallback>(&arena2, *f1);
    ExpectEquivalent(*b2, *f2);

    b2->MergeFrom(b_other);
    f2->MergeFrom(f_other);
    ExpectEquivalent(*b2, *f2);
  }
}

TEST(StringBatchTest, AllAbsent) {
  TestBatchStringFields b1, b2;
  TestBatchStringFieldsFallback f1, f2;
  ExerciseAllOperations(b1, f1, b2, f2);
}

TEST(StringBatchTest, SingleStringFieldPresentEachPosition) {
  const std::vector<std::string> test_values = {
      "",
      "short",
      std::string(256, 'L'),
  };
  for (int i = 0; i < 26; ++i) {
    for (const std::string& val : test_values) {
      TestBatchStringFields b1, b2;
      TestBatchStringFieldsFallback f1, f2;
      SetStringFieldByIndex(&b1, i, val);
      SetStringFieldByIndex(&f1, i, val);

      int other_idx = (i + 7) % 26;
      SetStringFieldByIndex(&b2, other_idx, "other_value_12345678901234567890");
      SetStringFieldByIndex(&f2, other_idx, "other_value_12345678901234567890");

      ExerciseAllOperations(b1, f1, b2, f2);
    }
  }
}

TEST(StringBatchTest, PresentEmptyVsAbsentStrings) {
  TestBatchStringFields b1, b2;
  TestBatchStringFieldsFallback f1, f2;

  // Set every 3rd string field to empty string "" (present, has_bit = 1),
  // every 3rd + 1 string field to a non-empty string, and leave every 3rd + 2
  // string field absent (has_bit = 0).
  for (int i = 0; i < 26; ++i) {
    if (i % 3 == 0) {
      SetStringFieldByIndex(&b1, i, "");
      SetStringFieldByIndex(&f1, i, "");
    } else if (i % 3 == 1) {
      SetStringFieldByIndex(&b1, i, absl::StrCat("val_", i));
      SetStringFieldByIndex(&f1, i, absl::StrCat("val_", i));
    }

    if (i % 2 == 0) {
      SetStringFieldByIndex(&b2, i, "");
      SetStringFieldByIndex(&f2, i, "");
    }
  }

  ExerciseAllOperations(b1, f1, b2, f2);
}

TEST(StringBatchTest, BitmaskPatternsAndInterleavedNonStrings) {
  const uint32_t masks[] = {
      0x00000000U,
      0x00000001U,
      0x80000000U,
      0x03ffffffU,
      0x00005555U,
      0x0000aaaaU,
      0x03ff0000U,
      0x00007fffU,
      0x01249249U,
      0x02db6db6U,
  };

  for (uint32_t mask1 : masks) {
    for (uint32_t mask2 : masks) {
      TestBatchStringFields b1, b2;
      TestBatchStringFieldsFallback f1, f2;

      for (int i = 0; i < 26; ++i) {
        if ((mask1 >> i) & 1) {
          std::string s = (i % 2 == 0) ? std::string(120, 'a' + (i % 26))
                                       : absl::StrCat("s1_", i);
          SetStringFieldByIndex(&b1, i, s);
          SetStringFieldByIndex(&f1, i, s);
        }
        if ((mask2 >> i) & 1) {
          std::string s = (i % 2 == 1) ? std::string(120, 'A' + (i % 26))
                                       : absl::StrCat("s2_", i);
          SetStringFieldByIndex(&b2, i, s);
          SetStringFieldByIndex(&f2, i, s);
        }
      }

      // Also populate non-string / default-string / cord / submessage fields
      // in the middle.
      for (int j = 0; j < 18; ++j) {
        if ((mask1 >> j) & 1) {
          SetNonStringFieldByIndex(&b1, j, static_cast<int32_t>(mask1 + j));
          SetNonStringFieldByIndex(&f1, j, static_cast<int32_t>(mask1 + j));
        }
        if ((mask2 >> j) & 1) {
          SetNonStringFieldByIndex(&b2, j, static_cast<int32_t>(mask2 + j));
          SetNonStringFieldByIndex(&f2, j, static_cast<int32_t>(mask2 + j));
        }
      }

      ExerciseAllOperations(b1, f1, b2, f2);
    }
  }
}

TEST(StringBatchTest, ClearFieldAndReuseAllocatedString) {
  // Exercises the case where a field was set to a heap-allocated string and
  // then cleared via clear_<field>() (so ArenaStringPtr still holds a cleared
  // std::string* on the heap while its has_bit is 0), followed by Swap,
  // MergeFrom, CopyFrom, copy construction, and destruction.
  TestBatchStringFields b1, b2;
  TestBatchStringFieldsFallback f1, f2;

  for (int i = 0; i < 26; ++i) {
    std::string long_str(200, 'x');
    SetStringFieldByIndex(&b1, i, long_str);
    SetStringFieldByIndex(&f1, i, long_str);
  }
  // Clear half the string fields individually so has_bit becomes 0 while the
  // underlying std::string remains allocated.
  for (int i = 0; i < 26; i += 2) {
    ClearStringFieldByIndex(&b1, i);
    ClearStringFieldByIndex(&f1, i);
  }
  // Populate overlapping and non-overlapping fields in b2/f2 and clear some.
  for (int i = 0; i < 26; i += 3) {
    SetStringFieldByIndex(&b2, i, std::string(150, 'y'));
    SetStringFieldByIndex(&f2, i, std::string(150, 'y'));
  }
  ClearStringFieldByIndex(&b2, 0);
  ClearStringFieldByIndex(&f2, 0);

  ExerciseAllOperations(b1, f1, b2, f2);
}

TEST(StringBatchTest, NonZeroFirstBitAllCombinations) {
  // Exhaustively test all 2^6 = 64 presence combinations for
  // TestNonZeroFirstBitStrings vs TestNonZeroFirstBitStringsFallback.
  auto populate = [](auto* msg, uint32_t mask, absl::string_view tag) {
    if (mask & 1) msg->set_lead_def(absl::StrCat(tag, "_lead"));
    if (mask & 2) msg->set_s1(absl::StrCat(tag, "_s1_", std::string(64, 'a')));
    if (mask & 4) msg->set_mid_cord(absl::StrCat(tag, "_cord"));
    if (mask & 8) msg->set_s2("");
    if (mask & 16) msg->set_b1(absl::StrCat(tag, "_b1"));
    if (mask & 32) msg->set_s3(absl::StrCat(tag, "_s3_", std::string(64, 'z')));
    if (mask & 64) {
      msg->set_mid_def(absl::StrCat(tag, "_mid_def"));
      msg->set_tail_def(absl::StrCat(tag, "_tail_def"));
      msg->set_tail_int1(42);
      msg->set_tail_int2(999);
    }
  };

  for (uint32_t m1 = 0; m1 < 128; m1 += 3) {
    for (uint32_t m2 = 0; m2 < 128; m2 += 5) {
      TestNonZeroFirstBitStrings b1, b2;
      TestNonZeroFirstBitStringsFallback f1, f2;
      populate(&b1, m1, "m1");
      populate(&f1, m1, "m1");
      populate(&b2, m2, "m2");
      populate(&f2, m2, "m2");

      ExpectEquivalentNonZeroFirstBit(b1, f1);
      ExpectEquivalentNonZeroFirstBit(b2, f2);

      TestNonZeroFirstBitStrings b_copy(b1);
      TestNonZeroFirstBitStringsFallback f_copy(f1);
      ExpectEquivalentNonZeroFirstBit(b_copy, f_copy);

      b_copy.MergeFrom(b2);
      f_copy.MergeFrom(f2);
      ExpectEquivalentNonZeroFirstBit(b_copy, f_copy);

      b_copy.Swap(&b2);
      f_copy.Swap(&f2);
      ExpectEquivalentNonZeroFirstBit(b_copy, f_copy);
      ExpectEquivalentNonZeroFirstBit(b2, f2);

      b_copy.Clear();
      f_copy.Clear();
      ExpectEquivalentNonZeroFirstBit(b_copy, f_copy);

      Arena arena;
      auto* ba = Arena::Create<TestNonZeroFirstBitStrings>(&arena, b1);
      auto* fa = Arena::Create<TestNonZeroFirstBitStringsFallback>(&arena, f1);
      ExpectEquivalentNonZeroFirstBit(*ba, *fa);
    }
  }
}

// Executes a sequence of random operations on a pool of 4 messages comparing
// TestBatchStringFields against TestBatchStringFieldsFallback at every step.
void RunOperationSequence(absl::string_view ops, bool use_arena) {
  Arena arena;
  Arena* target_arena = use_arena ? &arena : nullptr;

  std::vector<std::unique_ptr<TestBatchStringFields>> b_heap;
  std::vector<std::unique_ptr<TestBatchStringFieldsFallback>> f_heap;
  std::vector<TestBatchStringFields*> b(4);
  std::vector<TestBatchStringFieldsFallback*> f(4);

  for (int i = 0; i < 4; ++i) {
    if (target_arena != nullptr) {
      b[i] = Arena::Create<TestBatchStringFields>(target_arena);
      f[i] = Arena::Create<TestBatchStringFieldsFallback>(target_arena);
    } else {
      b_heap.push_back(std::make_unique<TestBatchStringFields>());
      f_heap.push_back(std::make_unique<TestBatchStringFieldsFallback>());
      b[i] = b_heap.back().get();
      f[i] = f_heap.back().get();
    }
  }

  for (size_t pc = 0; pc < ops.size(); ++pc) {
    uint8_t byte = static_cast<uint8_t>(ops[pc]);
    uint8_t arg = (pc + 1 < ops.size()) ? static_cast<uint8_t>(ops[++pc]) : 0;
    int slot = byte & 3;
    int other_slot = (arg >> 6) & 3;
    int op = (byte >> 2) % 10;

    switch (op) {
      case 0: {
        // Set string field to empty, short, or long string.
        std::string val;
        if ((arg & 3) == 1) {
          val = absl::StrCat("short_", arg);
        } else if ((arg & 3) >= 2) {
          val = std::string(64 + (arg % 128), 'a' + (arg % 26));
        }
        SetStringFieldByIndex(b[slot], arg, val);
        SetStringFieldByIndex(f[slot], arg, val);
        break;
      }
      case 1: {
        // Mutate string field via mutable_<field>().
        std::string val = (arg & 1)
                              ? std::string(100, 'M')
                              : absl::StrCat("mut_", static_cast<int>(arg));
        MutateStringFieldByIndex(b[slot], arg, val);
        MutateStringFieldByIndex(f[slot], arg, val);
        break;
      }
      case 2: {
        // Clear individual string field.
        ClearStringFieldByIndex(b[slot], arg);
        ClearStringFieldByIndex(f[slot], arg);
        break;
      }
      case 3: {
        // Set non-string field in the middle.
        SetNonStringFieldByIndex(b[slot], arg, static_cast<int32_t>(arg));
        SetNonStringFieldByIndex(f[slot], arg, static_cast<int32_t>(arg));
        break;
      }
      case 4: {
        // Clear whole message.
        b[slot]->Clear();
        f[slot]->Clear();
        break;
      }
      case 5: {
        // MergeFrom another slot.
        if (slot != other_slot) {
          b[slot]->MergeFrom(*b[other_slot]);
          f[slot]->MergeFrom(*f[other_slot]);
        }
        break;
      }
      case 6: {
        // CopyFrom another slot.
        b[slot]->CopyFrom(*b[other_slot]);
        f[slot]->CopyFrom(*f[other_slot]);
        break;
      }
      case 7: {
        // Swap two slots.
        b[slot]->Swap(b[other_slot]);
        f[slot]->Swap(f[other_slot]);
        break;
      }
      case 8: {
        // Copy-construct into slot.
        if (target_arena != nullptr) {
          b[slot] = Arena::Create<TestBatchStringFields>(target_arena,
                                                         *b[other_slot]);
          f[slot] = Arena::Create<TestBatchStringFieldsFallback>(
              target_arena, *f[other_slot]);
        } else {
          auto new_b =
              std::make_unique<TestBatchStringFields>(*b[other_slot]);
          auto new_f =
              std::make_unique<TestBatchStringFieldsFallback>(*f[other_slot]);
          b[slot] = new_b.get();
          f[slot] = new_f.get();
          b_heap[slot] = std::move(new_b);
          f_heap[slot] = std::move(new_f);
        }
        break;
      }
      case 9: {
        // Serialize and re-parse into other_slot.
        std::string b_wire = b[slot]->SerializeAsString();
        std::string f_wire = f[slot]->SerializeAsString();
        EXPECT_EQ(b_wire, f_wire);
        EXPECT_TRUE(b[other_slot]->ParseFromString(b_wire));
        EXPECT_TRUE(f[other_slot]->ParseFromString(f_wire));
        break;
      }
    }

    ExpectEquivalent(*b[slot], *f[slot]);
    ExpectEquivalent(*b[other_slot], *f[other_slot]);
  }
}

TEST(StringBatchTest, StressTestRandomOperationSequences) {
  uint64_t rng = 0x123456789abcdef0ULL;
  auto next_byte = [&]() -> char {
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return static_cast<char>(rng & 0xff);
  };

  for (int iter = 0; iter < 100; ++iter) {
    std::string ops;
    ops.resize(250);
    for (char& c : ops) {
      c = next_byte();
    }
    RunOperationSequence(ops, /*use_arena=*/false);
    RunOperationSequence(ops, /*use_arena=*/true);
  }
}

void FuzzBatchedVsFallbackMessages(const TestBatchStringFields& msg1,
                                   const TestBatchStringFields& msg2) {
  std::string wire1 = msg1.SerializeAsString();
  std::string wire2 = msg2.SerializeAsString();

  TestBatchStringFields b1, b2;
  TestBatchStringFieldsFallback f1, f2;
  ASSERT_TRUE(b1.ParseFromString(wire1));
  ASSERT_TRUE(f1.ParseFromString(wire1));
  ASSERT_TRUE(b2.ParseFromString(wire2));
  ASSERT_TRUE(f2.ParseFromString(wire2));

  ExerciseAllOperations(b1, f1, b2, f2);
}

FUZZ_TEST(StringBatchFuzzTest, FuzzBatchedVsFallbackMessages);

void FuzzNonZeroFirstBitMessages(const TestNonZeroFirstBitStrings& msg1,
                                 const TestNonZeroFirstBitStrings& msg2) {
  std::string wire1 = msg1.SerializeAsString();
  std::string wire2 = msg2.SerializeAsString();

  TestNonZeroFirstBitStrings b1, b2;
  TestNonZeroFirstBitStringsFallback f1, f2;
  ASSERT_TRUE(b1.ParseFromString(wire1));
  ASSERT_TRUE(f1.ParseFromString(wire1));
  ASSERT_TRUE(b2.ParseFromString(wire2));
  ASSERT_TRUE(f2.ParseFromString(wire2));

  ExpectEquivalentNonZeroFirstBit(b1, f1);
  ExpectEquivalentNonZeroFirstBit(b2, f2);

  TestNonZeroFirstBitStrings b_copy(b1);
  TestNonZeroFirstBitStringsFallback f_copy(f1);
  ExpectEquivalentNonZeroFirstBit(b_copy, f_copy);

  b_copy.MergeFrom(b2);
  f_copy.MergeFrom(f2);
  ExpectEquivalentNonZeroFirstBit(b_copy, f_copy);

  b_copy.Swap(&b2);
  f_copy.Swap(&f2);
  ExpectEquivalentNonZeroFirstBit(b_copy, f_copy);
  ExpectEquivalentNonZeroFirstBit(b2, f2);

  b_copy.Clear();
  f_copy.Clear();
  ExpectEquivalentNonZeroFirstBit(b_copy, f_copy);
}

FUZZ_TEST(StringBatchFuzzTest, FuzzNonZeroFirstBitMessages);

void FuzzRandomOperationSequence(absl::string_view ops, bool use_arena) {
  RunOperationSequence(ops, use_arena);
}

FUZZ_TEST(StringBatchFuzzTest, FuzzRandomOperationSequence);

}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
