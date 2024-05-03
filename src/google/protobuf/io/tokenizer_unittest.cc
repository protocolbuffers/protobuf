// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/io/tokenizer.h"

#include <limits.h>
#include <math.h>

#include <vector>

#include "google/protobuf/stubs/common.h"
#include "absl/strings/escaping.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace io {
namespace {

// ===================================================================
// Data-Driven Test Infrastructure

// TODO:  This is copied from coded_stream_unittest.  This is
//   temporary until these features are integrated into gTest itself.

// TEST_1D and TEST_2D are macros I'd eventually like to see added to
// gTest.  These macros can be used to declare tests which should be
// run multiple times, once for each item in some input array.  TEST_1D
// tests all cases in a single input array.  TEST_2D tests all
// combinations of cases from two arrays.  The arrays must be statically
// defined such that the ABSL_ARRAYSIZE() macro works on them.  Example:
//
// int kCases[] = {1, 2, 3, 4}
// TEST_1D(MyFixture, MyTest, kCases) {
//   EXPECT_GT(kCases_case, 0);
// }
//
// This test iterates through the numbers 1, 2, 3, and 4 and tests that
// they are all grater than zero.  In case of failure, the exact case
// which failed will be printed.  The case type must be printable using
// ostream::operator<<.

#define TEST_1D(FIXTURE, NAME, CASES)                             \
  class FIXTURE##_##NAME##_DD : public FIXTURE {                  \
   protected:                                                     \
    template <typename CaseType>                                  \
    void DoSingleCase(const CaseType& CASES##_case);              \
  };                                                              \
                                                                  \
  TEST_F(FIXTURE##_##NAME##_DD, NAME) {                           \
    for (int i = 0; i < ABSL_ARRAYSIZE(CASES); i++) {             \
      SCOPED_TRACE(testing::Message()                             \
                   << #CASES " case #" << i << ": " << CASES[i]); \
      DoSingleCase(CASES[i]);                                     \
    }                                                             \
  }                                                               \
                                                                  \
  template <typename CaseType>                                    \
  void FIXTURE##_##NAME##_DD::DoSingleCase(const CaseType& CASES##_case)

#define TEST_2D(FIXTURE, NAME, CASES1, CASES2)                              \
  class FIXTURE##_##NAME##_DD : public FIXTURE {                            \
   protected:                                                               \
    template <typename CaseType1, typename CaseType2>                       \
    void DoSingleCase(const CaseType1& CASES1##_case,                       \
                      const CaseType2& CASES2##_case);                      \
  };                                                                        \
                                                                            \
  TEST_F(FIXTURE##_##NAME##_DD, NAME) {                                     \
    for (int i = 0; i < ABSL_ARRAYSIZE(CASES1); i++) {                      \
      for (int j = 0; j < ABSL_ARRAYSIZE(CASES2); j++) {                    \
        SCOPED_TRACE(testing::Message()                                     \
                     << #CASES1 " case #" << i << ": " << CASES1[i] << ", " \
                     << #CASES2 " case #" << j << ": " << CASES2[j]);       \
        DoSingleCase(CASES1[i], CASES2[j]);                                 \
      }                                                                     \
    }                                                                       \
  }                                                                         \
                                                                            \
  template <typename CaseType1, typename CaseType2>                         \
  void FIXTURE##_##NAME##_DD::DoSingleCase(const CaseType1& CASES1##_case,  \
                                           const CaseType2& CASES2##_case)

// -------------------------------------------------------------------

// An input stream that is basically like an ArrayInputStream but sometimes
// returns empty buffers, just to throw us off.
class TestInputStream : public ZeroCopyInputStream {
 public:
  TestInputStream(const void* data, int size, int block_size)
      : array_stream_(data, size, block_size), counter_(0) {}
  ~TestInputStream() {}

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size) override {
    // We'll return empty buffers starting with the first buffer, and every
    // 3 and 5 buffers after that.
    if (counter_ % 3 == 0 || counter_ % 5 == 0) {
      *data = NULL;
      *size = 0;
      ++counter_;
      return true;
    } else {
      ++counter_;
      return array_stream_.Next(data, size);
    }
  }

  void BackUp(int count) override { return array_stream_.BackUp(count); }
  bool Skip(int count) override { return array_stream_.Skip(count); }
  int64_t ByteCount() const override { return array_stream_.ByteCount(); }

 private:
  ArrayInputStream array_stream_;
  int counter_;
};

// -------------------------------------------------------------------

// An error collector which simply concatenates all its errors into a big
// block of text which can be checked.
class TestErrorCollector : public ErrorCollector {
 public:
  TestErrorCollector() {}
  ~TestErrorCollector() {}

  std::string text_;

  // implements ErrorCollector ---------------------------------------
  void RecordError(int line, int column, absl::string_view message) override {
    absl::SubstituteAndAppend(&text_, "$0:$1: $2\n", line, column, message);
  }
};

// -------------------------------------------------------------------

// We test each operation over a variety of block sizes to insure that
// we test cases where reads cross buffer boundaries as well as cases
// where they don't.  This is sort of a brute-force approach to this,
// but it's easy to write and easy to understand.
const int kBlockSizes[] = {1, 2, 3, 5, 7, 13, 32, 1024};

class TokenizerTest : public testing::Test {
 protected:
  // For easy testing.
  uint64_t ParseInteger(const std::string& text) {
    uint64_t result;
    EXPECT_TRUE(Tokenizer::ParseInteger(text, kuint64max, &result))
        << "'" << text << "'";
    return result;
  }
};

// ===================================================================

// These tests causes gcc 3.3.5 (and earlier?) to give the cryptic error:
//   "sorry, unimplemented: `method_call_expr' not supported by dump_expr"
#if !defined(__GNUC__) || __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)

// In each test case, the entire input text should parse as a single token
// of the given type.
struct SimpleTokenCase {
  std::string input;
  Tokenizer::TokenType type;
};

inline std::ostream& operator<<(std::ostream& out,
                                const SimpleTokenCase& test_case) {
  return out << absl::CEscape(test_case.input);
}

SimpleTokenCase kSimpleTokenCases[] = {
    // Test identifiers.
    {"hello", Tokenizer::TYPE_IDENTIFIER},

    // Test integers.
    {"123", Tokenizer::TYPE_INTEGER},
    {"0xab6", Tokenizer::TYPE_INTEGER},
    {"0XAB6", Tokenizer::TYPE_INTEGER},
    {"0X1234567", Tokenizer::TYPE_INTEGER},
    {"0x89abcdef", Tokenizer::TYPE_INTEGER},
    {"0x89ABCDEF", Tokenizer::TYPE_INTEGER},
    {"01234567", Tokenizer::TYPE_INTEGER},

    // Test floats.
    {"123.45", Tokenizer::TYPE_FLOAT},
    {"1.", Tokenizer::TYPE_FLOAT},
    {"1e3", Tokenizer::TYPE_FLOAT},
    {"1E3", Tokenizer::TYPE_FLOAT},
    {"1e-3", Tokenizer::TYPE_FLOAT},
    {"1e+3", Tokenizer::TYPE_FLOAT},
    {"1.e3", Tokenizer::TYPE_FLOAT},
    {"1.2e3", Tokenizer::TYPE_FLOAT},
    {".1", Tokenizer::TYPE_FLOAT},
    {".1e3", Tokenizer::TYPE_FLOAT},
    {".1e-3", Tokenizer::TYPE_FLOAT},
    {".1e+3", Tokenizer::TYPE_FLOAT},

    // Test strings.
    {"'hello'", Tokenizer::TYPE_STRING},
    {"\"foo\"", Tokenizer::TYPE_STRING},
    {"'a\"b'", Tokenizer::TYPE_STRING},
    {"\"a'b\"", Tokenizer::TYPE_STRING},
    {"'a\\'b'", Tokenizer::TYPE_STRING},
    {"\"a\\\"b\"", Tokenizer::TYPE_STRING},
    {"'\\xf'", Tokenizer::TYPE_STRING},
    {"'\\0'", Tokenizer::TYPE_STRING},

    // Test symbols.
    {"+", Tokenizer::TYPE_SYMBOL},
    {".", Tokenizer::TYPE_SYMBOL},
};

TEST_2D(TokenizerTest, SimpleTokens, kSimpleTokenCases, kBlockSizes) {
  // Set up the tokenizer.
  TestInputStream input(kSimpleTokenCases_case.input.data(),
                        kSimpleTokenCases_case.input.size(), kBlockSizes_case);
  TestErrorCollector error_collector;
  Tokenizer tokenizer(&input, &error_collector);

  // Before Next() is called, the initial token should always be TYPE_START.
  EXPECT_EQ(Tokenizer::TYPE_START, tokenizer.current().type);
  EXPECT_EQ("", tokenizer.current().text);
  EXPECT_EQ(0, tokenizer.current().line);
  EXPECT_EQ(0, tokenizer.current().column);
  EXPECT_EQ(0, tokenizer.current().end_column);

  // Parse the token.
  ASSERT_TRUE(tokenizer.Next());

  // Check that it has the right type.
  EXPECT_EQ(kSimpleTokenCases_case.type, tokenizer.current().type);
  // Check that it contains the complete input text.
  EXPECT_EQ(kSimpleTokenCases_case.input, tokenizer.current().text);
  // Check that it is located at the beginning of the input
  EXPECT_EQ(0, tokenizer.current().line);
  EXPECT_EQ(0, tokenizer.current().column);
  EXPECT_EQ(kSimpleTokenCases_case.input.size(),
            tokenizer.current().end_column);

  // There should be no more input.
  EXPECT_FALSE(tokenizer.Next());

  // After Next() returns false, the token should have type TYPE_END.
  EXPECT_EQ(Tokenizer::TYPE_END, tokenizer.current().type);
  EXPECT_EQ("", tokenizer.current().text);
  EXPECT_EQ(0, tokenizer.current().line);
  EXPECT_EQ(kSimpleTokenCases_case.input.size(), tokenizer.current().column);
  EXPECT_EQ(kSimpleTokenCases_case.input.size(),
            tokenizer.current().end_column);

  // There should be no errors.
  EXPECT_TRUE(error_collector.text_.empty());
}

TEST_1D(TokenizerTest, FloatSuffix, kBlockSizes) {
  // Test the "allow_f_after_float" option.

  // Set up the tokenizer.
  const char* text = "1f 2.5f 6e3f 7F";
  TestInputStream input(text, strlen(text), kBlockSizes_case);
  TestErrorCollector error_collector;
  Tokenizer tokenizer(&input, &error_collector);
  tokenizer.set_allow_f_after_float(true);

  // Advance through tokens and check that they are parsed as expected.
  ASSERT_TRUE(tokenizer.Next());
  EXPECT_EQ(tokenizer.current().text, "1f");
  EXPECT_EQ(tokenizer.current().type, Tokenizer::TYPE_FLOAT);
  ASSERT_TRUE(tokenizer.Next());
  EXPECT_EQ(tokenizer.current().text, "2.5f");
  EXPECT_EQ(tokenizer.current().type, Tokenizer::TYPE_FLOAT);
  ASSERT_TRUE(tokenizer.Next());
  EXPECT_EQ(tokenizer.current().text, "6e3f");
  EXPECT_EQ(tokenizer.current().type, Tokenizer::TYPE_FLOAT);
  ASSERT_TRUE(tokenizer.Next());
  EXPECT_EQ(tokenizer.current().text, "7F");
  EXPECT_EQ(tokenizer.current().type, Tokenizer::TYPE_FLOAT);

  // There should be no more input.
  EXPECT_FALSE(tokenizer.Next());
  // There should be no errors.
  EXPECT_TRUE(error_collector.text_.empty());
}

SimpleTokenCase kWhitespaceTokenCases[] = {
    {" ", Tokenizer::TYPE_WHITESPACE},
    {"    ", Tokenizer::TYPE_WHITESPACE},
    {"\t", Tokenizer::TYPE_WHITESPACE},
    {"\v", Tokenizer::TYPE_WHITESPACE},
    {"\t ", Tokenizer::TYPE_WHITESPACE},
    {"\v\t", Tokenizer::TYPE_WHITESPACE},
    {"   \t\r", Tokenizer::TYPE_WHITESPACE},
    // Newlines:
    {"\n", Tokenizer::TYPE_NEWLINE},
};

TEST_2D(TokenizerTest, Whitespace, kWhitespaceTokenCases, kBlockSizes) {
  {
    TestInputStream input(kWhitespaceTokenCases_case.input.data(),
                          kWhitespaceTokenCases_case.input.size(),
                          kBlockSizes_case);
    TestErrorCollector error_collector;
    Tokenizer tokenizer(&input, &error_collector);

    EXPECT_FALSE(tokenizer.Next());
  }
  {
    TestInputStream input(kWhitespaceTokenCases_case.input.data(),
                          kWhitespaceTokenCases_case.input.size(),
                          kBlockSizes_case);
    TestErrorCollector error_collector;
    Tokenizer tokenizer(&input, &error_collector);
    tokenizer.set_report_whitespace(true);
    tokenizer.set_report_newlines(true);

    ASSERT_TRUE(tokenizer.Next());
    EXPECT_EQ(tokenizer.current().text, kWhitespaceTokenCases_case.input);
    EXPECT_EQ(tokenizer.current().type, kWhitespaceTokenCases_case.type);

    EXPECT_FALSE(tokenizer.Next());
  }
}

#endif

// -------------------------------------------------------------------

// In each case, the input is parsed to produce a list of tokens.  The
// last token in "output" must have type TYPE_END.
struct MultiTokenCase {
  std::string input;
  std::vector<Tokenizer::Token> output;
};

inline std::ostream& operator<<(std::ostream& out,
                                const MultiTokenCase& test_case) {
  return out << absl::CEscape(test_case.input);
}

MultiTokenCase kMultiTokenCases[] = {
    // Test empty input.
    {"",
     {
         {Tokenizer::TYPE_END, "", 0, 0, 0},
     }},

    // Test all token types at the same time.
    {"foo 1 1.2 + 'bar'",
     {
         {Tokenizer::TYPE_IDENTIFIER, "foo", 0, 0, 3},
         {Tokenizer::TYPE_INTEGER, "1", 0, 4, 5},
         {Tokenizer::TYPE_FLOAT, "1.2", 0, 6, 9},
         {Tokenizer::TYPE_SYMBOL, "+", 0, 10, 11},
         {Tokenizer::TYPE_STRING, "'bar'", 0, 12, 17},
         {Tokenizer::TYPE_END, "", 0, 17, 17},
     }},

    // Test that consecutive symbols are parsed as separate tokens.
    {"!@+%",
     {
         {Tokenizer::TYPE_SYMBOL, "!", 0, 0, 1},
         {Tokenizer::TYPE_SYMBOL, "@", 0, 1, 2},
         {Tokenizer::TYPE_SYMBOL, "+", 0, 2, 3},
         {Tokenizer::TYPE_SYMBOL, "%", 0, 3, 4},
         {Tokenizer::TYPE_END, "", 0, 4, 4},
     }},

    // Test that newlines affect line numbers correctly.
    {"foo bar\nrab oof",
     {
         {Tokenizer::TYPE_IDENTIFIER, "foo", 0, 0, 3},
         {Tokenizer::TYPE_IDENTIFIER, "bar", 0, 4, 7},
         {Tokenizer::TYPE_IDENTIFIER, "rab", 1, 0, 3},
         {Tokenizer::TYPE_IDENTIFIER, "oof", 1, 4, 7},
         {Tokenizer::TYPE_END, "", 1, 7, 7},
     }},

    // Test that tabs affect column numbers correctly.
    {"foo\tbar  \tbaz",
     {
         {Tokenizer::TYPE_IDENTIFIER, "foo", 0, 0, 3},
         {Tokenizer::TYPE_IDENTIFIER, "bar", 0, 8, 11},
         {Tokenizer::TYPE_IDENTIFIER, "baz", 0, 16, 19},
         {Tokenizer::TYPE_END, "", 0, 19, 19},
     }},

    // Test that tabs in string literals affect column numbers correctly.
    {"\"foo\tbar\" baz",
     {
         {Tokenizer::TYPE_STRING, "\"foo\tbar\"", 0, 0, 12},
         {Tokenizer::TYPE_IDENTIFIER, "baz", 0, 13, 16},
         {Tokenizer::TYPE_END, "", 0, 16, 16},
     }},

    // Test that line comments are ignored.
    {"foo // This is a comment\n"
     "bar // This is another comment",
     {
         {Tokenizer::TYPE_IDENTIFIER, "foo", 0, 0, 3},
         {Tokenizer::TYPE_IDENTIFIER, "bar", 1, 0, 3},
         {Tokenizer::TYPE_END, "", 1, 30, 30},
     }},

    // Test that block comments are ignored.
    {"foo /* This is a block comment */ bar",
     {
         {Tokenizer::TYPE_IDENTIFIER, "foo", 0, 0, 3},
         {Tokenizer::TYPE_IDENTIFIER, "bar", 0, 34, 37},
         {Tokenizer::TYPE_END, "", 0, 37, 37},
     }},

    // Test that sh-style comments are not ignored by default.
    {"foo # bar\n"
     "baz",
     {
         {Tokenizer::TYPE_IDENTIFIER, "foo", 0, 0, 3},
         {Tokenizer::TYPE_SYMBOL, "#", 0, 4, 5},
         {Tokenizer::TYPE_IDENTIFIER, "bar", 0, 6, 9},
         {Tokenizer::TYPE_IDENTIFIER, "baz", 1, 0, 3},
         {Tokenizer::TYPE_END, "", 1, 3, 3},
     }},

    // Test all whitespace chars
    {"foo\n\t\r\v\fbar",
     {
         {Tokenizer::TYPE_IDENTIFIER, "foo", 0, 0, 3},
         {Tokenizer::TYPE_IDENTIFIER, "bar", 1, 11, 14},
         {Tokenizer::TYPE_END, "", 1, 14, 14},
     }},
};

TEST_2D(TokenizerTest, MultipleTokens, kMultiTokenCases, kBlockSizes) {
  // Set up the tokenizer.
  TestInputStream input(kMultiTokenCases_case.input.data(),
                        kMultiTokenCases_case.input.size(), kBlockSizes_case);
  TestErrorCollector error_collector;
  Tokenizer tokenizer(&input, &error_collector);

  // Before Next() is called, the initial token should always be TYPE_START.
  EXPECT_EQ(Tokenizer::TYPE_START, tokenizer.current().type);
  EXPECT_EQ("", tokenizer.current().text);
  EXPECT_EQ(0, tokenizer.current().line);
  EXPECT_EQ(0, tokenizer.current().column);
  EXPECT_EQ(0, tokenizer.current().end_column);

  // Loop through all expected tokens.
  int i = 0;
  Tokenizer::Token token;
  do {
    token = kMultiTokenCases_case.output[i++];

    SCOPED_TRACE(testing::Message() << "Token #" << i << ": " << token.text);

    Tokenizer::Token previous = tokenizer.current();

    // Next() should only return false when it hits the end token.
    if (token.type != Tokenizer::TYPE_END) {
      ASSERT_TRUE(tokenizer.Next());
    } else {
      ASSERT_FALSE(tokenizer.Next());
    }

    // Check that the previous token is set correctly.
    EXPECT_EQ(previous.type, tokenizer.previous().type);
    EXPECT_EQ(previous.text, tokenizer.previous().text);
    EXPECT_EQ(previous.line, tokenizer.previous().line);
    EXPECT_EQ(previous.column, tokenizer.previous().column);
    EXPECT_EQ(previous.end_column, tokenizer.previous().end_column);

    // Check that the token matches the expected one.
    EXPECT_EQ(token.type, tokenizer.current().type);
    EXPECT_EQ(token.text, tokenizer.current().text);
    EXPECT_EQ(token.line, tokenizer.current().line);
    EXPECT_EQ(token.column, tokenizer.current().column);
    EXPECT_EQ(token.end_column, tokenizer.current().end_column);

  } while (token.type != Tokenizer::TYPE_END);

  // There should be no errors.
  EXPECT_TRUE(error_collector.text_.empty());
}

MultiTokenCase kMultiWhitespaceTokenCases[] = {
    // Test all token types at the same time.
    {"foo 1 \t1.2  \n   +\v'bar'",
     {
         {Tokenizer::TYPE_IDENTIFIER, "foo", 0, 0, 3},
         {Tokenizer::TYPE_WHITESPACE, " ", 0, 3, 4},
         {Tokenizer::TYPE_INTEGER, "1", 0, 4, 5},
         {Tokenizer::TYPE_WHITESPACE, " \t", 0, 5, 8},
         {Tokenizer::TYPE_FLOAT, "1.2", 0, 8, 11},
         {Tokenizer::TYPE_WHITESPACE, "  ", 0, 11, 13},
         {Tokenizer::TYPE_NEWLINE, "\n", 0, 13, 0},
         {Tokenizer::TYPE_WHITESPACE, "   ", 1, 0, 3},
         {Tokenizer::TYPE_SYMBOL, "+", 1, 3, 4},
         {Tokenizer::TYPE_WHITESPACE, "\v", 1, 4, 5},
         {Tokenizer::TYPE_STRING, "'bar'", 1, 5, 10},
         {Tokenizer::TYPE_END, "", 1, 10, 10},
     }},

};

TEST_2D(TokenizerTest, MultipleWhitespaceTokens, kMultiWhitespaceTokenCases,
        kBlockSizes) {
  // Set up the tokenizer.
  TestInputStream input(kMultiWhitespaceTokenCases_case.input.data(),
                        kMultiWhitespaceTokenCases_case.input.size(),
                        kBlockSizes_case);
  TestErrorCollector error_collector;
  Tokenizer tokenizer(&input, &error_collector);
  tokenizer.set_report_whitespace(true);
  tokenizer.set_report_newlines(true);

  // Before Next() is called, the initial token should always be TYPE_START.
  EXPECT_EQ(Tokenizer::TYPE_START, tokenizer.current().type);
  EXPECT_EQ("", tokenizer.current().text);
  EXPECT_EQ(0, tokenizer.current().line);
  EXPECT_EQ(0, tokenizer.current().column);
  EXPECT_EQ(0, tokenizer.current().end_column);

  // Loop through all expected tokens.
  int i = 0;
  Tokenizer::Token token;
  do {
    token = kMultiWhitespaceTokenCases_case.output[i++];

    SCOPED_TRACE(testing::Message() << "Token #" << i << ": " << token.text);

    Tokenizer::Token previous = tokenizer.current();

    // Next() should only return false when it hits the end token.
    if (token.type != Tokenizer::TYPE_END) {
      ASSERT_TRUE(tokenizer.Next());
    } else {
      ASSERT_FALSE(tokenizer.Next());
    }

    // Check that the previous token is set correctly.
    EXPECT_EQ(previous.type, tokenizer.previous().type);
    EXPECT_EQ(previous.text, tokenizer.previous().text);
    EXPECT_EQ(previous.line, tokenizer.previous().line);
    EXPECT_EQ(previous.column, tokenizer.previous().column);
    EXPECT_EQ(previous.end_column, tokenizer.previous().end_column);

    // Check that the token matches the expected one.
    EXPECT_EQ(token.type, tokenizer.current().type);
    EXPECT_EQ(token.text, tokenizer.current().text);
    EXPECT_EQ(token.line, tokenizer.current().line);
    EXPECT_EQ(token.column, tokenizer.current().column);
    EXPECT_EQ(token.end_column, tokenizer.current().end_column);

  } while (token.type != Tokenizer::TYPE_END);

  // There should be no errors.
  EXPECT_TRUE(error_collector.text_.empty());
}

// This test causes gcc 3.3.5 (and earlier?) to give the cryptic error:
//   "sorry, unimplemented: `method_call_expr' not supported by dump_expr"
#if !defined(__GNUC__) || __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)

TEST_1D(TokenizerTest, ShCommentStyle, kBlockSizes) {
  // Test the "comment_style" option.

  const char* text =
      "foo # bar\n"
      "baz // qux\n"
      "corge /* grault */\n"
      "garply";
  const char* const kTokens[] = {"foo",  // "# bar" is ignored
                                 "baz", "/",      "/", "qux", "corge", "/",
                                 "*",   "grault", "*", "/",   "garply"};

  // Set up the tokenizer.
  TestInputStream input(text, strlen(text), kBlockSizes_case);
  TestErrorCollector error_collector;
  Tokenizer tokenizer(&input, &error_collector);
  tokenizer.set_comment_style(Tokenizer::SH_COMMENT_STYLE);

  // Advance through tokens and check that they are parsed as expected.
  for (int i = 0; i < ABSL_ARRAYSIZE(kTokens); i++) {
    EXPECT_TRUE(tokenizer.Next());
    EXPECT_EQ(tokenizer.current().text, kTokens[i]);
  }

  // There should be no more input.
  EXPECT_FALSE(tokenizer.Next());
  // There should be no errors.
  EXPECT_TRUE(error_collector.text_.empty());
}

#endif

// -------------------------------------------------------------------

// In each case, the input is expected to have two tokens named "prev" and
// "next" with comments in between.
struct DocCommentCase {
  std::string input;

  const char* prev_trailing_comments;
  const char* detached_comments[10];
  const char* next_leading_comments;
};

inline std::ostream& operator<<(std::ostream& out,
                                const DocCommentCase& test_case) {
  return out << absl::CEscape(test_case.input);
}

// clang-format off
DocCommentCase kDocCommentCases[] = {
    {"prev next",

     "",
     {},
     ""},

    {"prev // no next token\n",

     " no next token\n",
     {},
     ""},

    {"prev // no next token and no trailing newline",

     " no next token and no trailing newline",
     {},
     ""},

    {"prev /* detached */ next",

     "",
     {" detached "},
     ""},

    {"prev // trailing comment\n"
     "next",

     " trailing comment\n",
     {},
     ""},

    {"prev\n"
     "/* leading comment */ next",

     "",
     {},
     " leading comment "},

    {"prev\n"
     "// leading comment\n"
     "// line 2\n"
     "next",

     "",
     {},
     " leading comment\n"
     " line 2\n"},

    {"prev\n"
     "// trailing comment\n"
     "// line 2\n"
     "\n"
     "next",

     " trailing comment\n"
     " line 2\n",
     {},
     ""},

    {"prev // trailing comment\n"
     "// leading comment\n"
     "// line 2\n"
     "next",

     " trailing comment\n",
     {},
     " leading comment\n"
     " line 2\n"},

    {"prev /* trailing block comment */\n"
     "/* leading block comment\n"
     " * line 2\n"
     " * line 3 */"
     "next",

     " trailing block comment ",
     {},
     " leading block comment\n"
     " line 2\n"
     " line 3 "},

    {"prev\n"
     "/* trailing block comment\n"
     " * line 2\n"
     " * line 3\n"
     " */\n"
     "/* leading block comment\n"
     " * line 2\n"
     " * line 3 */"
     "next",

     " trailing block comment\n"
     " line 2\n"
     " line 3\n",
     {},
     " leading block comment\n"
     " line 2\n"
     " line 3 "},

    {"prev\n"
     "// trailing comment\n"
     "\n"
     "// detached comment\n"
     "// line 2\n"
     "\n"
     "// second detached comment\n"
     "/* third detached comment\n"
     " * line 2 */\n"
     "// leading comment\n"
     "next",

     " trailing comment\n",
     {" detached comment\n"
      " line 2\n",
      " second detached comment\n",
      " third detached comment\n"
      " line 2 "},
     " leading comment\n"},

    {"prev /**/\n"
     "\n"
     "// detached comment\n"
     "\n"
     "// leading comment\n"
     "next",

     "",
     {" detached comment\n"},
     " leading comment\n"},

    {"prev /**/\n"
     "// leading comment\n"
     "next",

     "",
     {},
     " leading comment\n"},

    {"prev /* many comments*/ /* all inline */ /* will be handled */ next",

     " many comments",
     {" all inline "},
     " will be handled "},

    {R"pb(
     prev /* a single block comment
         that spans multiple lines
         is detached if it ends
         on the same line as next */ next
     )pb",

     "",
     {" a single block comment\n"
      "that spans multiple lines\n"
      "is detached if it ends\n"
      "on the same line as next "},
     ""},

    {R"pb(
       prev /* trailing */ /* leading */ next
     )pb",

     " trailing ",
     {},
     " leading "},

    {R"pb(
     prev /* multi-line
          trailing */ /* an oddly
                      placed detached */ /* an oddly
                                         placed leading */ next
     )pb",

     " multi-line\ntrailing ",
     {" an oddly\nplaced detached "},
     " an oddly\nplaced leading "},

    {R"pb(
       prev  // trailing with newline
       // detached
       /* another detached */
       // leading but no next token to attach it to
     )pb",

     " trailing with newline\n",
     {" detached\n", " another detached ",
      " leading but no next token to attach it to\n"},
     ""},
};
// clang-format on

TEST_2D(TokenizerTest, DocComments, kDocCommentCases, kBlockSizes) {
  // Set up the tokenizer.
  TestInputStream input(kDocCommentCases_case.input.data(),
                        kDocCommentCases_case.input.size(), kBlockSizes_case);
  TestErrorCollector error_collector;
  Tokenizer tokenizer(&input, &error_collector);

  // Set up a second tokenizer where we'll pass all NULLs to NextWithComments().
  TestInputStream input2(kDocCommentCases_case.input.data(),
                         kDocCommentCases_case.input.size(), kBlockSizes_case);
  Tokenizer tokenizer2(&input2, &error_collector);

  EXPECT_TRUE(tokenizer.Next());
  EXPECT_TRUE(tokenizer2.Next());

  EXPECT_EQ("prev", tokenizer.current().text);
  EXPECT_EQ("prev", tokenizer2.current().text);

  std::string prev_trailing_comments;
  std::vector<std::string> detached_comments;
  std::string next_leading_comments;
  bool has_next = tokenizer.NextWithComments(
      &prev_trailing_comments, &detached_comments, &next_leading_comments);
  EXPECT_EQ(has_next, tokenizer2.NextWithComments(nullptr, nullptr, nullptr));
  if (has_next) {
    EXPECT_EQ("next", tokenizer.current().text);
    EXPECT_EQ("next", tokenizer2.current().text);
  }

  EXPECT_EQ(kDocCommentCases_case.prev_trailing_comments,
            prev_trailing_comments);

  for (int i = 0; i < detached_comments.size(); i++) {
    ASSERT_LT(i, ABSL_ARRAYSIZE(kDocCommentCases));
    ASSERT_TRUE(kDocCommentCases_case.detached_comments[i] != NULL);
    EXPECT_EQ(kDocCommentCases_case.detached_comments[i], detached_comments[i]);
  }

  // Verify that we matched all the detached comments.
  EXPECT_EQ(NULL,
            kDocCommentCases_case.detached_comments[detached_comments.size()]);

  EXPECT_EQ(kDocCommentCases_case.next_leading_comments, next_leading_comments);
}

// -------------------------------------------------------------------

// Test parse helpers.
// TODO: Add a fuzz test for this.
TEST_F(TokenizerTest, ParseInteger) {
  EXPECT_EQ(0, ParseInteger("0"));
  EXPECT_EQ(123, ParseInteger("123"));
  EXPECT_EQ(0xabcdef12u, ParseInteger("0xabcdef12"));
  EXPECT_EQ(0xabcdef12u, ParseInteger("0xABCDEF12"));
  EXPECT_EQ(kuint64max, ParseInteger("0xFFFFFFFFFFFFFFFF"));
  EXPECT_EQ(01234567, ParseInteger("01234567"));
  EXPECT_EQ(0X123, ParseInteger("0X123"));

  // Test invalid integers that may still be tokenized as integers.
  EXPECT_EQ(0, ParseInteger("0x"));

  uint64_t i;

  // Test invalid integers that will never be tokenized as integers.
  EXPECT_FALSE(Tokenizer::ParseInteger("zxy", kuint64max, &i));
  EXPECT_FALSE(Tokenizer::ParseInteger("1.2", kuint64max, &i));
  EXPECT_FALSE(Tokenizer::ParseInteger("08", kuint64max, &i));
  EXPECT_FALSE(Tokenizer::ParseInteger("0xg", kuint64max, &i));
  EXPECT_FALSE(Tokenizer::ParseInteger("-1", kuint64max, &i));

  // Test overflows.
  EXPECT_TRUE(Tokenizer::ParseInteger("0", 0, &i));
  EXPECT_FALSE(Tokenizer::ParseInteger("1", 0, &i));
  EXPECT_TRUE(Tokenizer::ParseInteger("1", 1, &i));
  EXPECT_TRUE(Tokenizer::ParseInteger("12345", 12345, &i));
  EXPECT_FALSE(Tokenizer::ParseInteger("12346", 12345, &i));
  EXPECT_TRUE(Tokenizer::ParseInteger("0xFFFFFFFFFFFFFFFF", kuint64max, &i));
  EXPECT_FALSE(Tokenizer::ParseInteger("0x10000000000000000", kuint64max, &i));

  // Test near the limits of signed parsing (values in kint64max +/- 1600)
  for (int64_t offset = -1600; offset <= 1600; ++offset) {
    // We make sure to perform an unsigned addition so that we avoid signed
    // overflow, which would be undefined behavior.
    uint64_t i = 0x7FFFFFFFFFFFFFFFu + static_cast<uint64_t>(offset);
    char decimal[32];
    snprintf(decimal, 32, "%llu", static_cast<unsigned long long>(i));
    if (offset > 0) {
      uint64_t parsed = -1;
      EXPECT_FALSE(Tokenizer::ParseInteger(decimal, kint64max, &parsed))
          << decimal << "=>" << parsed;
    } else {
      uint64_t parsed = -1;
      EXPECT_TRUE(Tokenizer::ParseInteger(decimal, kint64max, &parsed))
          << decimal << "=>" << parsed;
      EXPECT_EQ(parsed, i);
    }
    char octal[32];
    snprintf(octal, 32, "0%llo", static_cast<unsigned long long>(i));
    if (offset > 0) {
      uint64_t parsed = -1;
      EXPECT_FALSE(Tokenizer::ParseInteger(octal, kint64max, &parsed))
          << octal << "=>" << parsed;
    } else {
      uint64_t parsed = -1;
      EXPECT_TRUE(Tokenizer::ParseInteger(octal, kint64max, &parsed))
          << octal << "=>" << parsed;
      EXPECT_EQ(parsed, i);
    }
    char hex[32];
    snprintf(hex, 32, "0x%llx", static_cast<unsigned long long>(i));
    if (offset > 0) {
      uint64_t parsed = -1;
      EXPECT_FALSE(Tokenizer::ParseInteger(hex, kint64max, &parsed))
          << hex << "=>" << parsed;
    } else {
      uint64_t parsed = -1;
      EXPECT_TRUE(Tokenizer::ParseInteger(hex, kint64max, &parsed)) << hex;
      EXPECT_EQ(parsed, i);
    }
    // EXPECT_NE(offset, -237);
  }

  // Test near the limits of unsigned parsing (values in kuint64max +/- 1600)
  // By definition, values greater than kuint64max cannot be held in a uint64_t
  // variable, so printing them is a little tricky; fortunately all but the
  // last four digits are known, so we can hard-code them in the printf string,
  // and we only need to format the last 4.
  for (int64_t offset = -1600; offset <= 1600; ++offset) {
    {
      uint64_t i = 18446744073709551615u + offset;
      char decimal[32];
      snprintf(decimal, 32, "1844674407370955%04llu",
               static_cast<unsigned long long>(1615 + offset));
      if (offset > 0) {
        uint64_t parsed = -1;
        EXPECT_FALSE(Tokenizer::ParseInteger(decimal, kuint64max, &parsed))
            << decimal << "=>" << parsed;
      } else {
        uint64_t parsed = -1;
        EXPECT_TRUE(Tokenizer::ParseInteger(decimal, kuint64max, &parsed))
            << decimal;
        EXPECT_EQ(parsed, i);
      }
    }
    {
      uint64_t i = 01777777777777777777777u + offset;
      if (offset > 0) {
        char octal[32];
        snprintf(octal, 32, "0200000000000000000%04llo",
                 static_cast<unsigned long long>(offset - 1));
        uint64_t parsed = -1;
        EXPECT_FALSE(Tokenizer::ParseInteger(octal, kuint64max, &parsed))
            << octal << "=>" << parsed;
      } else {
        char octal[32];
        snprintf(octal, 32, "0%llo", static_cast<unsigned long long>(i));
        uint64_t parsed = -1;
        EXPECT_TRUE(Tokenizer::ParseInteger(octal, kuint64max, &parsed))
            << octal;
        EXPECT_EQ(parsed, i);
      }
    }
    {
      uint64_t ui = 0xffffffffffffffffu + offset;
      char hex[32];
      if (offset > 0) {
        snprintf(hex, 32, "0x1000000000000%04llx",
                 static_cast<unsigned long long>(offset - 1));
        uint64_t parsed = -1;
        EXPECT_FALSE(Tokenizer::ParseInteger(hex, kuint64max, &parsed))
            << hex << "=>" << parsed;
      } else {
        snprintf(hex, 32, "0x%llx", static_cast<unsigned long long>(ui));
        uint64_t parsed = -1;
        EXPECT_TRUE(Tokenizer::ParseInteger(hex, kuint64max, &parsed)) << hex;
        EXPECT_EQ(parsed, ui);
      }
    }
  }
}

TEST_F(TokenizerTest, ParseFloat) {
  EXPECT_DOUBLE_EQ(1, Tokenizer::ParseFloat("1."));
  EXPECT_DOUBLE_EQ(1e3, Tokenizer::ParseFloat("1e3"));
  EXPECT_DOUBLE_EQ(1e3, Tokenizer::ParseFloat("1E3"));
  EXPECT_DOUBLE_EQ(1.5e3, Tokenizer::ParseFloat("1.5e3"));
  EXPECT_DOUBLE_EQ(.1, Tokenizer::ParseFloat(".1"));
  EXPECT_DOUBLE_EQ(.25, Tokenizer::ParseFloat(".25"));
  EXPECT_DOUBLE_EQ(.1e3, Tokenizer::ParseFloat(".1e3"));
  EXPECT_DOUBLE_EQ(.25e3, Tokenizer::ParseFloat(".25e3"));
  EXPECT_DOUBLE_EQ(.1e+3, Tokenizer::ParseFloat(".1e+3"));
  EXPECT_DOUBLE_EQ(.1e-3, Tokenizer::ParseFloat(".1e-3"));
  EXPECT_DOUBLE_EQ(5, Tokenizer::ParseFloat("5"));
  EXPECT_DOUBLE_EQ(6e-12, Tokenizer::ParseFloat("6e-12"));
  EXPECT_DOUBLE_EQ(1.2, Tokenizer::ParseFloat("1.2"));
  EXPECT_DOUBLE_EQ(1.e2, Tokenizer::ParseFloat("1.e2"));

  // Test invalid integers that may still be tokenized as integers.
  EXPECT_DOUBLE_EQ(1, Tokenizer::ParseFloat("1e"));
  EXPECT_DOUBLE_EQ(1, Tokenizer::ParseFloat("1e-"));
  EXPECT_DOUBLE_EQ(1, Tokenizer::ParseFloat("1.e"));

  // Test 'f' suffix.
  EXPECT_DOUBLE_EQ(1, Tokenizer::ParseFloat("1f"));
  EXPECT_DOUBLE_EQ(1, Tokenizer::ParseFloat("1.0f"));
  EXPECT_DOUBLE_EQ(1, Tokenizer::ParseFloat("1F"));

  // These should parse successfully even though they are out of range.
  // Overflows become infinity and underflows become zero.
  EXPECT_EQ(0.0, Tokenizer::ParseFloat("1e-9999999999999999999999999999"));
  EXPECT_EQ(HUGE_VAL, Tokenizer::ParseFloat("1e+9999999999999999999999999999"));

#if GTEST_HAS_DEATH_TEST  // death tests do not work on Windows yet
  // Test invalid integers that will never be tokenized as integers.
  EXPECT_DEBUG_DEATH(
      Tokenizer::ParseFloat("zxy"),
      "passed text that could not have been tokenized as a float");
  EXPECT_DEBUG_DEATH(
      Tokenizer::ParseFloat("1-e0"),
      "passed text that could not have been tokenized as a float");
  EXPECT_DEBUG_DEATH(
      Tokenizer::ParseFloat("-1.0"),
      "passed text that could not have been tokenized as a float");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST_F(TokenizerTest, ParseString) {
  std::string output;
  Tokenizer::ParseString("'hello'", &output);
  EXPECT_EQ("hello", output);
  Tokenizer::ParseString("\"blah\\nblah2\"", &output);
  EXPECT_EQ("blah\nblah2", output);
  Tokenizer::ParseString("'\\1x\\1\\123\\739\\52\\334n\\3'", &output);
  EXPECT_EQ("\1x\1\123\739\52\334n\3", output);
  Tokenizer::ParseString("'\\x20\\x4'", &output);
  EXPECT_EQ("\x20\x4", output);
  Tokenizer::ParseString("'\\X20\\X4'", &output);
  EXPECT_EQ("\x20\x4", output);

  // Test invalid strings that may still be tokenized as strings.
  Tokenizer::ParseString("\"\\a\\l\\v\\t", &output);  // \l is invalid
  EXPECT_EQ("\a?\v\t", output);
  Tokenizer::ParseString("'", &output);
  EXPECT_EQ("", output);
  Tokenizer::ParseString("'\\", &output);
  EXPECT_EQ("\\", output);

  // Experiment with Unicode escapes. Here are one-, two- and three-byte Unicode
  // characters.
  Tokenizer::ParseString("'\\u0024\\u00a2\\u20ac\\U00024b62XX'", &output);
  EXPECT_EQ("$¢€𤭢XX", output);
  // Same thing encoded using UTF16.
  Tokenizer::ParseString("'\\u0024\\u00a2\\u20ac\\ud852\\udf62XX'", &output);
  EXPECT_EQ("$¢€𤭢XX", output);
  // Here's some broken UTF16; there's a head surrogate with no tail surrogate.
  // We just output this as if it were UTF8; it's not a defined code point, but
  // it has a defined encoding.
  Tokenizer::ParseString("'\\ud852XX'", &output);
  EXPECT_EQ("\xed\xa1\x92XX", output);
  // Malformed escape: Demons may fly out of the nose.
  Tokenizer::ParseString("'\\u0'", &output);
  EXPECT_EQ("u0", output);
  // Beyond the range of valid UTF-32 code units.
  Tokenizer::ParseString("'\\U00110000\\U00200000\\UFFFFFFFF'", &output);
  EXPECT_EQ("\\U00110000\\U00200000\\Uffffffff", output);

  // Test invalid strings that will never be tokenized as strings.
#if GTEST_HAS_DEATH_TEST  // death tests do not work on Windows yet
  EXPECT_DEBUG_DEATH(
      Tokenizer::ParseString("", &output),
      "passed text that could not have been tokenized as a string");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST_F(TokenizerTest, ParseStringAppend) {
  // Check that ParseString and ParseStringAppend differ.
  std::string output("stuff+");
  Tokenizer::ParseStringAppend("'hello'", &output);
  EXPECT_EQ("stuff+hello", output);
  Tokenizer::ParseString("'hello'", &output);
  EXPECT_EQ("hello", output);
}

// -------------------------------------------------------------------

// Each case parses some input text, ignoring the tokens produced, and
// checks that the error output matches what is expected.
struct ErrorCase {
  std::string input;
  bool recoverable;  // True if the tokenizer should be able to recover and
                     // parse more tokens after seeing this error.  Cases
                     // for which this is true must end with "foo" as
                     // the last token, which the test will check for.
  const char* errors;
};

inline std::ostream& operator<<(std::ostream& out, const ErrorCase& test_case) {
  return out << absl::CEscape(test_case.input);
}

ErrorCase kErrorCases[] = {
    // String errors.
    {"'\\l' foo", true, "0:2: Invalid escape sequence in string literal.\n"},
    {"'\\X' foo", true, "0:3: Expected hex digits for escape sequence.\n"},
    {"'\\x' foo", true, "0:3: Expected hex digits for escape sequence.\n"},
    {"'foo", false, "0:4: Unexpected end of string.\n"},
    {"'bar\nfoo", true,
     "0:4: Multiline strings are not allowed. Did you miss a \"?.\n"},
    {"'\\u01' foo", true,
     "0:5: Expected four hex digits for \\u escape sequence.\n"},
    {"'\\u01' foo", true,
     "0:5: Expected four hex digits for \\u escape sequence.\n"},
    {"'\\uXYZ' foo", true,
     "0:3: Expected four hex digits for \\u escape sequence.\n"},

    // Integer errors.
    {"123foo", true, "0:3: Need space between number and identifier.\n"},

    // Hex/octal errors.
    {"0x foo", true, "0:2: \"0x\" must be followed by hex digits.\n"},
    {"0541823 foo", true,
     "0:4: Numbers starting with leading zero must be in octal.\n"},
    {"0x123z foo", true, "0:5: Need space between number and identifier.\n"},
    {"0x123.4 foo", true, "0:5: Hex and octal numbers must be integers.\n"},
    {"0123.4 foo", true, "0:4: Hex and octal numbers must be integers.\n"},

    // Float errors.
    {"1e foo", true, "0:2: \"e\" must be followed by exponent.\n"},
    {"1e- foo", true, "0:3: \"e\" must be followed by exponent.\n"},
    {"1.2.3 foo", true,
     "0:3: Already saw decimal point or exponent; can't have another one.\n"},
    {"1e2.3 foo", true,
     "0:3: Already saw decimal point or exponent; can't have another one.\n"},
    {"a.1 foo", true,
     "0:1: Need space between identifier and decimal point.\n"},
    // allow_f_after_float not enabled, so this should be an error.
    {"1.0f foo", true, "0:3: Need space between number and identifier.\n"},

    // Block comment errors.
    {"/*", false,
     "0:2: End-of-file inside block comment.\n"
     "0:0:   Comment started here.\n"},
    {"/*/*/ foo", true,
     "0:3: \"/*\" inside block comment.  Block comments cannot be nested.\n"},

    // Control characters.  Multiple consecutive control characters should only
    // produce one error.
    {"\b foo", true, "0:0: Invalid control characters encountered in text.\n"},
    {"\b\b foo", true,
     "0:0: Invalid control characters encountered in text.\n"},

    // Check that control characters at end of input don't result in an
    // infinite loop.
    {"\b", false, "0:0: Invalid control characters encountered in text.\n"},

    // Check recovery from '\0'.  We have to explicitly specify the length of
    // these strings because otherwise the string constructor will just call
    // strlen() which will see the first '\0' and think that is the end of the
    // string.
    {std::string("\0foo", 4), true,
     "0:0: Invalid control characters encountered in text.\n"},
    {std::string("\0\0foo", 5), true,
     "0:0: Invalid control characters encountered in text.\n"},

    // Check error from high order bits set
    {"\300foo", true, "0:0: Interpreting non ascii codepoint 192.\n"},
};

TEST_2D(TokenizerTest, Errors, kErrorCases, kBlockSizes) {
  // Set up the tokenizer.
  TestInputStream input(kErrorCases_case.input.data(),
                        kErrorCases_case.input.size(), kBlockSizes_case);
  TestErrorCollector error_collector;
  Tokenizer tokenizer(&input, &error_collector);

  // Ignore all input, except remember if the last token was "foo".
  bool last_was_foo = false;
  while (tokenizer.Next()) {
    last_was_foo = tokenizer.current().text == "foo";
  }

  // Check that the errors match what was expected.
  EXPECT_EQ(kErrorCases_case.errors, error_collector.text_);

  // If the error was recoverable, make sure we saw "foo" after it.
  if (kErrorCases_case.recoverable) {
    EXPECT_TRUE(last_was_foo);
  }
}

// -------------------------------------------------------------------

TEST_1D(TokenizerTest, BackUpOnDestruction, kBlockSizes) {
  std::string text = "foo bar";
  TestInputStream input(text.data(), text.size(), kBlockSizes_case);

  // Create a tokenizer, read one token, then destroy it.
  {
    TestErrorCollector error_collector;
    Tokenizer tokenizer(&input, &error_collector);

    tokenizer.Next();
  }

  // Only "foo" should have been read.
  EXPECT_EQ(strlen("foo"), input.ByteCount());
}


}  // namespace
}  // namespace io
}  // namespace protobuf
}  // namespace google
