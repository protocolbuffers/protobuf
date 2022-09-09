// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/util/internal/json_stream_parser.h"

#include <cstdint>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "google/protobuf/util/internal/expecting_objectwriter.h"
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/protobuf/util/internal/object_writer.h"


namespace google {
namespace protobuf {
namespace util {
namespace converter {

using ParseErrorType =
    ::google::protobuf::util::converter::JsonStreamParser::ParseErrorType;


// Tests for the JSON Stream Parser. These tests are intended to be
// comprehensive and cover the following:
//
// Positive tests:
// - true, false, null
// - empty object or array.
// - negative and positive double and int, unsigned int
// - single and double quoted strings
// - string key, unquoted key, numeric key
// - array containing array, object, value
// - object containing array, object, value
// - unicode handling in strings
// - ascii escaping (\b, \f, \n, \r, \t, \v)
// - trailing commas
//
// Negative tests:
// - illegal literals
// - mismatched quotes failure on strings
// - unterminated string failure
// - unexpected end of string failure
// - mismatched object and array closing
// - Failure to close array or object
// - numbers too large
// - invalid unicode escapes.
// - invalid unicode sequences.
// - numbers as keys
//
// For each test we split the input string on every possible character to ensure
// the parser is able to handle arbitrarily split input for all cases. We also
// do a final test of the entire test case one character at a time.
//
// It is verified that expected calls to the mocked objects are in sequence.
class JsonStreamParserTest : public ::testing::Test {
 protected:
  JsonStreamParserTest() : mock_(), ow_(&mock_) {}
  ~JsonStreamParserTest() override {}

  absl::Status RunTest(absl::string_view json, int split,
                       std::function<void(JsonStreamParser*)> setup) {
    JsonStreamParser parser(&mock_);
    setup(&parser);

    // Special case for split == length, test parsing one character at a time.
    if (split == json.length()) {
      GOOGLE_LOG(INFO) << "Testing split every char: " << json;
      for (int i = 0; i < json.length(); ++i) {
        absl::string_view single = json.substr(i, 1);
        absl::Status result = parser.Parse(single);
        if (!result.ok()) {
          return result;
        }
      }
      return parser.FinishParse();
    }

    // Normal case, split at the split point and parse two substrings.
    absl::string_view first = json.substr(0, split);
    absl::string_view rest = json.substr(split);
    GOOGLE_LOG(INFO) << "Testing split: " << first << "><" << rest;
    absl::Status result = parser.Parse(first);
    if (result.ok()) {
      result = parser.Parse(rest);
      if (result.ok()) {
        result = parser.FinishParse();
      }
    }
    if (result.ok()) {
      EXPECT_EQ(parser.recursion_depth(), 0);
    }
    return result;
  }

  void DoTest(
      absl::string_view json, int split,
      std::function<void(JsonStreamParser*)> setup = [](JsonStreamParser* p) {
      }) {
    absl::Status result = RunTest(json, split, setup);
    if (!result.ok()) {
      GOOGLE_LOG(WARNING) << result;
    }
    EXPECT_TRUE(result.ok());
  }

  void DoErrorTest(
      absl::string_view json, int split, absl::string_view error_prefix,
      std::function<void(JsonStreamParser*)> setup = [](JsonStreamParser* p) {
      }) {
    absl::Status result = RunTest(json, split, setup);
    EXPECT_TRUE(absl::IsInvalidArgument(result));
    absl::string_view error_message(result.message());
    EXPECT_EQ(error_prefix, error_message.substr(0, error_prefix.size()));
  }

  void DoErrorTest(
      absl::string_view json, int split, absl::string_view error_prefix,
      ParseErrorType expected_parse_error_type,
      std::function<void(JsonStreamParser*)> setup = [](JsonStreamParser* p) {
      }) {
    absl::Status result = RunTest(json, split, setup);
    EXPECT_TRUE(absl::IsInvalidArgument(result));
    absl::string_view error_message(result.message());
    EXPECT_EQ(error_prefix, error_message.substr(0, error_prefix.size()));
  }


#ifndef _MSC_VER
  // TODO(xiaofeng): We have to disable InSequence check for MSVC because it
  // causes stack overflow due to its use of a linked list that is destructed
  // recursively.
  ::testing::InSequence in_sequence_;
#endif  // !_MSC_VER
  MockObjectWriter mock_;
  ExpectingObjectWriter ow_;
};


// Positive tests

// - true, false, null
TEST_F(JsonStreamParserTest, SimpleTrue) {
  absl::string_view str = "true";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderBool("", true);
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, SimpleFalse) {
  absl::string_view str = "false";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderBool("", false);
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, SimpleNull) {
  absl::string_view str = "null";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderNull("");
    DoTest(str, i);
  }
}

// - empty object and array.
TEST_F(JsonStreamParserTest, EmptyObject) {
  absl::string_view str = "{}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")->EndObject();
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, EmptyList) {
  absl::string_view str = "[]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")->EndList();
    DoTest(str, i);
  }
}

// - negative and positive double and int, unsigned int
TEST_F(JsonStreamParserTest, SimpleDouble) {
  absl::string_view str = "42.5";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderDouble("", 42.5);
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, ScientificDouble) {
  absl::string_view str = "1.2345e-10";
  for (int i = 0; i < str.length(); ++i) {
    ow_.RenderDouble("", 1.2345e-10);
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, SimpleNegativeDouble) {
  absl::string_view str = "-1045.235";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderDouble("", -1045.235);
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, SimpleInt) {
  absl::string_view str = "123456";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderUint64("", 123456);
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, SimpleNegativeInt) {
  absl::string_view str = "-79497823553162765";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderInt64("", int64_t{-79497823553162765});
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, SimpleUnsignedInt) {
  absl::string_view str = "11779497823553162765";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderUint64("", uint64_t{11779497823553162765u});
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, OctalNumberIsInvalid) {
  absl::string_view str = "01234";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Octal/hex numbers are not valid JSON values.",
                ParseErrorType::OCTAL_OR_HEX_ARE_NOT_VALID_JSON_VALUES);
  }
  str = "-01234";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Octal/hex numbers are not valid JSON values.",
                ParseErrorType::OCTAL_OR_HEX_ARE_NOT_VALID_JSON_VALUES);
  }
}

TEST_F(JsonStreamParserTest, HexNumberIsInvalid) {
  absl::string_view str = "0x1234";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Octal/hex numbers are not valid JSON values.",
                ParseErrorType::OCTAL_OR_HEX_ARE_NOT_VALID_JSON_VALUES);
  }
  str = "-0x1234";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Octal/hex numbers are not valid JSON values.",
                ParseErrorType::OCTAL_OR_HEX_ARE_NOT_VALID_JSON_VALUES);
  }
  str = "12x34";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Unable to parse number.",
                ParseErrorType::UNABLE_TO_PARSE_NUMBER);
  }
}

// - single and double quoted strings
TEST_F(JsonStreamParserTest, EmptyDoubleQuotedString) {
  absl::string_view str = "\"\"";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderString("", "");
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, EmptySingleQuotedString) {
  absl::string_view str = "''";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderString("", "");
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, SimpleDoubleQuotedString) {
  absl::string_view str = "\"Some String\"";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderString("", "Some String");
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, SimpleSingleQuotedString) {
  absl::string_view str = "'Another String'";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderString("", "Another String");
    DoTest(str, i);
  }
}

// - string key, unquoted key, numeric key
TEST_F(JsonStreamParserTest, ObjectKeyTypes) {
  absl::string_view str =
      "{'s': true, \"d\": false, key: null, snake_key: [], camelKey: {}}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")
        ->RenderBool("s", true)
        ->RenderBool("d", false)
        ->RenderNull("key")
        ->StartList("snake_key")
        ->EndList()
        ->StartObject("camelKey")
        ->EndObject()
        ->EndObject();
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, UnquotedObjectKeyWithReservedPrefxes) {
  absl::string_view str = "{ nullkey: \"a\", truekey: \"b\", falsekey: \"c\"}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")
        ->RenderString("nullkey", "a")
        ->RenderString("truekey", "b")
        ->RenderString("falsekey", "c")
        ->EndObject();
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, UnquotedObjectKeyWithReservedKeyword) {
  absl::string_view str = "{ null: \"a\", true: \"b\", false: \"c\"}";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Expected an object key or }.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, UnquotedObjectKeyWithEmbeddedNonAlphanumeric) {
  absl::string_view str = "{ foo-bar-baz: \"a\"}";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Expected : between key:value pair.",
                ParseErrorType::EXPECTED_COLON);
  }
}


// - array containing primitive values (true, false, null, num, string)
TEST_F(JsonStreamParserTest, ArrayPrimitiveValues) {
  absl::string_view str = "[true, false, null, 'one', \"two\"]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")
        ->RenderBool("", true)
        ->RenderBool("", false)
        ->RenderNull("")
        ->RenderString("", "one")
        ->RenderString("", "two")
        ->EndList();
    DoTest(str, i);
  }
}

// - array containing array, object
TEST_F(JsonStreamParserTest, ArrayComplexValues) {
  absl::string_view str =
      "[[22, -127, 45.3, -1056.4, 11779497823553162765], {'key': true}]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")
        ->StartList("")
        ->RenderUint64("", 22)
        ->RenderInt64("", -127)
        ->RenderDouble("", 45.3)
        ->RenderDouble("", -1056.4)
        ->RenderUint64("", uint64_t{11779497823553162765u})
        ->EndList()
        ->StartObject("")
        ->RenderBool("key", true)
        ->EndObject()
        ->EndList();
    DoTest(str, i);
  }
}


// - object containing array, object, value (true, false, null, num, string)
TEST_F(JsonStreamParserTest, ObjectValues) {
  absl::string_view str =
      "{t: true, f: false, n: null, s: 'a string', d: \"another string\", pi: "
      "22, ni: -127, pd: 45.3, nd: -1056.4, pl: 11779497823553162765, l: [[]], "
      "o: {'key': true}}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")
        ->RenderBool("t", true)
        ->RenderBool("f", false)
        ->RenderNull("n")
        ->RenderString("s", "a string")
        ->RenderString("d", "another string")
        ->RenderUint64("pi", 22)
        ->RenderInt64("ni", -127)
        ->RenderDouble("pd", 45.3)
        ->RenderDouble("nd", -1056.4)
        ->RenderUint64("pl", uint64_t{11779497823553162765u})
        ->StartList("l")
        ->StartList("")
        ->EndList()
        ->EndList()
        ->StartObject("o")
        ->RenderBool("key", true)
        ->EndObject()
        ->EndObject();
    DoTest(str, i);
  }
}


TEST_F(JsonStreamParserTest, RejectNonUtf8WhenNotCoerced) {
  absl::string_view json = "{\"address\":\xFF\"חרושת 23, רעננה, ישראל\"}";
  for (int i = 0; i <= json.length(); ++i) {
    DoErrorTest(json, i, "Encountered non UTF-8 code points.",
                ParseErrorType::NON_UTF_8);
  }
  json = "{\"address\": \"חרושת 23,\xFFרעננה, ישראל\"}";
  for (int i = 0; i <= json.length(); ++i) {
    DoErrorTest(json, i, "Encountered non UTF-8 code points.",
                ParseErrorType::NON_UTF_8);
  }
  DoErrorTest("\xFF{}", 0, "Encountered non UTF-8 code points.",
              ParseErrorType::NON_UTF_8);
}

// - unicode handling in strings
TEST_F(JsonStreamParserTest, UnicodeEscaping) {
  absl::string_view str = "[\"\\u0639\\u0631\\u0628\\u0649\"]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")
        ->RenderString("", "\xD8\xB9\xD8\xB1\xD8\xA8\xD9\x89")
        ->EndList();
    DoTest(str, i);
  }
}

// - unicode UTF-16 surrogate pair handling in strings
TEST_F(JsonStreamParserTest, UnicodeSurrogatePairEscaping) {
  absl::string_view str =
      "[\"\\u0bee\\ud800\\uddf1\\uD80C\\uDDA4\\uD83d\\udC1D\\uD83C\\uDF6F\"]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")
        ->RenderString("",
                       "\xE0\xAF\xAE\xF0\x90\x87\xB1\xF0\x93\x86\xA4\xF0"
                       "\x9F\x90\x9D\xF0\x9F\x8D\xAF")
        ->EndList();
    DoTest(str, i);
  }
}


TEST_F(JsonStreamParserTest, UnicodeEscapingInvalidCodePointWhenNotCoerced) {
  // A low surrogate alone.
  absl::string_view str = "[\"\\ude36\"]";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Invalid unicode code point.",
                ParseErrorType::INVALID_UNICODE);
  }
}

TEST_F(JsonStreamParserTest, UnicodeEscapingMissingLowSurrogateWhenNotCoerced) {
  // A high surrogate alone.
  absl::string_view str = "[\"\\ud83d\"]";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Missing low surrogate.",
                ParseErrorType::MISSING_LOW_SURROGATE);
  }
  // A high surrogate with some trailing characters.
  str = "[\"\\ud83d|ude36\"]";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Missing low surrogate.",
                ParseErrorType::MISSING_LOW_SURROGATE);
  }
  // A high surrogate with half a low surrogate.
  str = "[\"\\ud83d\\ude--\"]";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Invalid escape sequence.",
                ParseErrorType::INVALID_ESCAPE_SEQUENCE);
  }
  // Two high surrogates.
  str = "[\"\\ud83d\\ud83d\"]";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Invalid low surrogate.",
                ParseErrorType::INVALID_LOW_SURROGATE);
  }
}

// - ascii escaping (\b, \f, \n, \r, \t, \v)
TEST_F(JsonStreamParserTest, AsciiEscaping) {
  absl::string_view str =
      "[\"\\b\", \"\\ning\", \"test\\f\", \"\\r\\t\", \"test\\\\\\ving\"]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")
        ->RenderString("", "\b")
        ->RenderString("", "\ning")
        ->RenderString("", "test\f")
        ->RenderString("", "\r\t")
        ->RenderString("", "test\\\ving")
        ->EndList();
    DoTest(str, i);
  }
}

// - trailing commas, we support a single trailing comma but no internal commas.
TEST_F(JsonStreamParserTest, TrailingCommas) {
  absl::string_view str = "[['a',true,], {b: null,},]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")
        ->StartList("")
        ->RenderString("", "a")
        ->RenderBool("", true)
        ->EndList()
        ->StartObject("")
        ->RenderNull("b")
        ->EndObject()
        ->EndList();
    DoTest(str, i);
  }
}

// Negative tests

// illegal literals
TEST_F(JsonStreamParserTest, ExtraTextAfterTrue) {
  absl::string_view str = "truee";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderBool("", true);
    DoErrorTest(str, i, "Parsing terminated before end of input.",
                ParseErrorType::PARSING_TERMINATED_BEFORE_END_OF_INPUT);
  }
}

TEST_F(JsonStreamParserTest, InvalidNumberDashOnly) {
  absl::string_view str = "-";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Unable to parse number.",
                ParseErrorType::UNABLE_TO_PARSE_NUMBER);
  }
}

TEST_F(JsonStreamParserTest, InvalidNumberDashName) {
  absl::string_view str = "-foo";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Unable to parse number.",
                ParseErrorType::UNABLE_TO_PARSE_NUMBER);
  }
}

TEST_F(JsonStreamParserTest, InvalidLiteralInArray) {
  absl::string_view str = "[nule]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("");
    DoErrorTest(str, i, "Unexpected token.", ParseErrorType::UNEXPECTED_TOKEN);
  }
}

TEST_F(JsonStreamParserTest, InvalidLiteralInObject) {
  absl::string_view str = "{123false}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected an object key or }.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}

// mismatched quotes failure on strings
TEST_F(JsonStreamParserTest, MismatchedSingleQuotedLiteral) {
  absl::string_view str = "'Some str\"";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Closing quote expected in string.",
                ParseErrorType::EXPECTED_CLOSING_QUOTE);
  }
}

TEST_F(JsonStreamParserTest, MismatchedDoubleQuotedLiteral) {
  absl::string_view str = "\"Another string that ends poorly!'";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Closing quote expected in string.",
                ParseErrorType::EXPECTED_CLOSING_QUOTE);
  }
}

// unterminated strings
TEST_F(JsonStreamParserTest, UnterminatedLiteralString) {
  absl::string_view str = "\"Forgot the rest of i";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Closing quote expected in string.",
                ParseErrorType::EXPECTED_CLOSING_QUOTE);
  }
}

TEST_F(JsonStreamParserTest, UnterminatedStringEscape) {
  absl::string_view str = "\"Forgot the rest of \\";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Closing quote expected in string.",
                ParseErrorType::EXPECTED_CLOSING_QUOTE);
  }
}

TEST_F(JsonStreamParserTest, UnterminatedStringInArray) {
  absl::string_view str = "[\"Forgot to close the string]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("");
    DoErrorTest(str, i, "Closing quote expected in string.",
                ParseErrorType::EXPECTED_CLOSING_QUOTE);
  }
}

TEST_F(JsonStreamParserTest, UnterminatedStringInObject) {
  absl::string_view str = "{f: \"Forgot to close the string}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Closing quote expected in string.",
                ParseErrorType::EXPECTED_CLOSING_QUOTE);
  }
}

TEST_F(JsonStreamParserTest, UnterminatedObject) {
  absl::string_view str = "{";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Unexpected end of string.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}


// mismatched object and array closing
TEST_F(JsonStreamParserTest, MismatchedCloseObject) {
  absl::string_view str = "{'key': true]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")->RenderBool("key", true);
    DoErrorTest(str, i, "Expected , or } after key:value pair.",
                ParseErrorType::EXPECTED_COMMA_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, MismatchedCloseArray) {
  absl::string_view str = "[true, null}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")->RenderBool("", true)->RenderNull("");
    DoErrorTest(str, i, "Expected , or ] after array value.",
                ParseErrorType::EXPECTED_COMMA_OR_BRACKET);
  }
}

// Invalid object keys.
TEST_F(JsonStreamParserTest, InvalidNumericObjectKey) {
  absl::string_view str = "{42: true}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected an object key or }.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, InvalidLiteralObjectInObject) {
  absl::string_view str = "{{bob: true}}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected an object key or }.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, InvalidLiteralArrayInObject) {
  absl::string_view str = "{[null]}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected an object key or }.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, InvalidLiteralValueInObject) {
  absl::string_view str = "{false}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected an object key or }.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, MissingColonAfterStringInObject) {
  absl::string_view str = "{\"key\"}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected : between key:value pair.",
                ParseErrorType::EXPECTED_COLON);
  }
}

TEST_F(JsonStreamParserTest, MissingColonAfterKeyInObject) {
  absl::string_view str = "{key}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected : between key:value pair.",
                ParseErrorType::EXPECTED_COLON);
  }
}

TEST_F(JsonStreamParserTest, EndOfTextAfterKeyInObject) {
  absl::string_view str = "{key";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Unexpected end of string.",
                ParseErrorType::EXPECTED_COLON);
  }
}

TEST_F(JsonStreamParserTest, MissingValueAfterColonInObject) {
  absl::string_view str = "{key:}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Unexpected token.", ParseErrorType::UNEXPECTED_TOKEN);
  }
}

TEST_F(JsonStreamParserTest, MissingCommaBetweenObjectEntries) {
  absl::string_view str = "{key:20 'hello': true}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")->RenderUint64("key", 20);
    DoErrorTest(str, i, "Expected , or } after key:value pair.",
                ParseErrorType::EXPECTED_COMMA_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, InvalidLiteralAsObjectKey) {
  absl::string_view str = "{false: 20}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected an object key or }.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, ExtraCharactersAfterObject) {
  absl::string_view str = "{}}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")->EndObject();
    DoErrorTest(str, i, "Parsing terminated before end of input.",
                ParseErrorType::PARSING_TERMINATED_BEFORE_END_OF_INPUT);
  }
}

TEST_F(JsonStreamParserTest, PositiveNumberTooBigIsDouble) {
  absl::string_view str = "18446744073709551616";  // 2^64
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderDouble("", 18446744073709552000.0);
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, NegativeNumberTooBigIsDouble) {
  absl::string_view str = "-18446744073709551616";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderDouble("", -18446744073709551616.0);
    DoTest(str, i);
  }
}

TEST_F(JsonStreamParserTest, DoubleTooBig) {
  absl::string_view str = "[1.89769e+308]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("");
    DoErrorTest(str, i, "Number exceeds the range of double.",
                ParseErrorType::NUMBER_EXCEEDS_RANGE_DOUBLE);
  }
  str = "[-1.89769e+308]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("");
    DoErrorTest(str, i, "Number exceeds the range of double.",
                ParseErrorType::NUMBER_EXCEEDS_RANGE_DOUBLE);
  }
}


// invalid bare backslash.
TEST_F(JsonStreamParserTest, UnfinishedEscape) {
  absl::string_view str = "\"\\";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Closing quote expected in string.",
                ParseErrorType::EXPECTED_CLOSING_QUOTE);
  }
}

// invalid bare backslash u.
TEST_F(JsonStreamParserTest, UnfinishedUnicodeEscape) {
  absl::string_view str = "\"\\u";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Illegal hex string.",
                ParseErrorType::ILLEGAL_HEX_STRING);
  }
}

// invalid unicode sequence.
TEST_F(JsonStreamParserTest, UnicodeEscapeCutOff) {
  absl::string_view str = "\"\\u12";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Illegal hex string.",
                ParseErrorType::ILLEGAL_HEX_STRING);
  }
}

// invalid unicode sequence (valid in modern EcmaScript but not in JSON).
TEST_F(JsonStreamParserTest, BracketedUnicodeEscape) {
  absl::string_view str = "\"\\u{1f36f}\"";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Invalid escape sequence.",
                ParseErrorType::INVALID_ESCAPE_SEQUENCE);
  }
}


TEST_F(JsonStreamParserTest, UnicodeEscapeInvalidCharacters) {
  absl::string_view str = "\"\\u12$4hello";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Invalid escape sequence.",
                ParseErrorType::INVALID_ESCAPE_SEQUENCE);
  }
}

// invalid unicode sequence in low half surrogate: g is not a hex digit.
TEST_F(JsonStreamParserTest, UnicodeEscapeLowHalfSurrogateInvalidCharacters) {
  absl::string_view str = "\"\\ud800\\udcfg\"";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Invalid escape sequence.",
                ParseErrorType::INVALID_ESCAPE_SEQUENCE);
  }
}

// Extra commas with an object or array.
TEST_F(JsonStreamParserTest, ExtraCommaInObject) {
  absl::string_view str = "{'k1': true,,'k2': false}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")->RenderBool("k1", true);
    DoErrorTest(str, i, "Expected an object key or }.",
                ParseErrorType::EXPECTED_OBJECT_KEY_OR_BRACES);
  }
}

TEST_F(JsonStreamParserTest, ExtraCommaInArray) {
  absl::string_view str = "[true,,false}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")->RenderBool("", true);
    DoErrorTest(str, i, "Unexpected token.", ParseErrorType::UNEXPECTED_TOKEN);
  }
}

// Extra text beyond end of value.
TEST_F(JsonStreamParserTest, ExtraTextAfterLiteral) {
  absl::string_view str = "'hello', 'world'";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.RenderString("", "hello");
    DoErrorTest(str, i, "Parsing terminated before end of input.",
                ParseErrorType::PARSING_TERMINATED_BEFORE_END_OF_INPUT);
  }
}

TEST_F(JsonStreamParserTest, ExtraTextAfterObject) {
  absl::string_view str = "{'key': true} 'oops'";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("")->RenderBool("key", true)->EndObject();
    DoErrorTest(str, i, "Parsing terminated before end of input.",
                ParseErrorType::PARSING_TERMINATED_BEFORE_END_OF_INPUT);
  }
}

TEST_F(JsonStreamParserTest, ExtraTextAfterArray) {
  absl::string_view str = "[null] 'oops'";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("")->RenderNull("")->EndList();
    DoErrorTest(str, i, "Parsing terminated before end of input.",
                ParseErrorType::PARSING_TERMINATED_BEFORE_END_OF_INPUT);
  }
}

// Random unknown text in the value.
TEST_F(JsonStreamParserTest, UnknownCharactersAsValue) {
  absl::string_view str = "*&#25";
  for (int i = 0; i <= str.length(); ++i) {
    DoErrorTest(str, i, "Expected a value.", ParseErrorType::EXPECTED_VALUE);
  }
}

TEST_F(JsonStreamParserTest, UnknownCharactersInArray) {
  absl::string_view str = "[*&#25]";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartList("");
    DoErrorTest(str, i, "Expected a value or ] within an array.",
                ParseErrorType::EXPECTED_VALUE_OR_BRACKET);
  }
}

TEST_F(JsonStreamParserTest, UnknownCharactersInObject) {
  absl::string_view str = "{'key': *&#25}";
  for (int i = 0; i <= str.length(); ++i) {
    ow_.StartObject("");
    DoErrorTest(str, i, "Expected a value.", ParseErrorType::EXPECTED_VALUE);
  }
}

TEST_F(JsonStreamParserTest, DeepNestJsonNotExceedLimit) {
  int count = 99;
  std::string str;
  for (int i = 0; i < count; ++i) {
    absl::StrAppend(&str, "{'a':");
  }
  absl::StrAppend(&str, "{'nest64':'v1', 'nest64': false, 'nest64': ['v2']}");
  for (int i = 0; i < count; ++i) {
    absl::StrAppend(&str, "}");
  }
  ow_.StartObject("");
  for (int i = 0; i < count; ++i) {
    ow_.StartObject("a");
  }
  ow_.RenderString("nest64", "v1")
      ->RenderBool("nest64", false)
      ->StartList("nest64")
      ->RenderString("", "v2")
      ->EndList();
  for (int i = 0; i < count; ++i) {
    ow_.EndObject();
  }
  ow_.EndObject();
  DoTest(str, 0);
}

TEST_F(JsonStreamParserTest, DeepNestJsonExceedLimit) {
  int count = 98;
  std::string str;
  for (int i = 0; i < count; ++i) {
    absl::StrAppend(&str, "{'a':");
  }
  // Supports trailing commas.
  absl::StrAppend(&str,
                  "{'nest11' : [{'nest12' : null,},],"
                  "'nest21' : {'nest22' : {'nest23' : false}}}");
  for (int i = 0; i < count; ++i) {
    absl::StrAppend(&str, "}");
  }
  DoErrorTest(str, 0,
              "Message too deep. Max recursion depth reached for key 'nest22'");
}

}  // namespace converter
}  // namespace util
}  // namespace protobuf
}  // namespace google
