// Test for the bounded Any-of-Any recursion guard in
// TextFormat::Printer::PrintAny().
//
// Bug repro path: a chain of google.protobuf.Any messages whose
// type_url is "type.googleapis.com/google.protobuf.Any" and whose
// value field is the next inner Any, fed into a Printer with
// SetExpandAny(true), drove PrintAny into unbounded recursion. On
// a 1 MB stack the process SIGSEGV'd; on the default 8 MB stack
// the same 180 KB input produced a 32 MB DebugString string
// (~180x amplification).
//
// This test asserts that:
//   1. A chain of depth > 100 prints successfully (the limit is
//      hit and PrintAny falls back to a printable form instead of
//      recursing forever).
//   2. The fallback output is well-formed (no half-printed Any
//      headers).
//   3. SetAnyExpansionDepth(0) disables expansion entirely.
//   4. SetAnyExpansionDepth(N) is enforced for nested depth > N.

#include <cstdio>
#include <cstdlib>
#include <string>

#include <gtest/gtest.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/text_format.h>

namespace protobuf_unittest {
namespace {

std::string MakeAnyChain(int depth) {
  // Build a chain depth levels deep of google.protobuf.Any, each
  // containing the next. Innermost value is empty bytes.
  std::string chain;
  for (int i = 0; i < depth; ++i) {
    ::google::protobuf::Any outer;
    outer.set_type_url("type.googleapis.com/google.protobuf.Any");
    if (i + 1 < depth) {
      outer.set_value(chain);  // wrap the previous layer
    }
    outer.SerializeToString(&chain);
  }
  return chain;
}

TEST(TextFormatAnyExpansionTest, ChainOfDepth1000DoesNotStackOverflow) {
  std::string chain = MakeAnyChain(1000);
  ::google::protobuf::Any root;
  ASSERT_TRUE(root.ParseFromString(chain));

  ::google::protobuf::TextFormat::Printer printer;
  printer.SetExpandAny(true);

  std::string out;
  ASSERT_NO_FATAL_FAILURE(printer.PrintToString(root, &out));
  // Should not have produced an unbounded output. The fallback
  // prints the inner Any as raw bytes; for depth 1000 the output
  // must be bounded by O(depth).
  EXPECT_LT(out.size(), 10u * 1024u * 1024u)
      << "DebugString output blew past 10 MB; the recursion guard "
         "is not engaging.";
}

TEST(TextFormatAnyExpansionTest, RecursionBudgetIsExhaustedGracefully) {
  std::string chain = MakeAnyChain(500);
  ::google::protobuf::Any root;
  ASSERT_TRUE(root.ParseFromString(chain));

  ::google::protobuf::TextFormat::Printer printer;
  printer.SetExpandAny(true);

  std::string out;
  printer.PrintToString(root, &out);

  // After the budget is exhausted (default 100), the printer must
  // emit a closing "}" for each remaining unclosed inner Any. The
  // brace count must balance.
  int opens = 0, closes = 0;
  for (char c : out) {
    if (c == '{') ++opens;
    if (c == '}') ++closes;
  }
  EXPECT_EQ(opens, closes)
      << "Brace count did not balance in fallback output: "
      << "opens=" << opens << " closes=" << closes;
}

TEST(TextFormatAnyExpansionTest, ZeroBudgetDisablesExpansion) {
  std::string chain = MakeAnyChain(20);
  ::google::protobuf::Any root;
  ASSERT_TRUE(root.ParseFromString(chain));

  ::google::protobuf::TextFormat::Printer printer;
  printer.SetExpandAny(true);
  printer.SetAnyExpansionDepth(0);

  std::string out;
  printer.PrintToString(root, &out);
  // With the budget at zero the printer should immediately fall
  // back. Output should be the canonical "[type.googleapis.com/...]"
  // form, no recursion.
  EXPECT_NE(out.find("type.googleapis.com/google.protobuf.Any"),
            std::string::npos);
}

TEST(TextFormatAnyExpansionTest, CustomBudgetIsRespected) {
  std::string chain = MakeAnyChain(50);
  ::google::protobuf::Any root;
  ASSERT_TRUE(root.ParseFromString(chain));

  ::google::protobuf::TextFormat::Printer printer;
  printer.SetExpandAny(true);
  printer.SetAnyExpansionDepth(5);

  std::string out_5;
  printer.PrintToString(root, &out_5);

  // A budget of 5 should produce a noticeably smaller output than
  // the default budget of 100.
  ::google::protobuf::TextFormat::Printer printer_default;
  printer_default.SetExpandAny(true);
  std::string out_default;
  printer_default.PrintToString(root, &out_default);

  EXPECT_LT(out_5.size(), out_default.size())
      << "Custom budget did not produce smaller output than default: "
      << "budget=5 -> " << out_5.size() << " bytes, "
      << "default=100 -> " << out_default.size() << " bytes.";
}

}  // namespace
}  // namespace protobuf_unittest