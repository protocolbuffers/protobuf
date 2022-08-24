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

// Author: kenton@google.com (Kenton Varda)

#include <google/protobuf/stubs/strutil.h>

#include <locale.h>

#include <google/protobuf/stubs/stl_util.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif

namespace google {
namespace protobuf {
namespace {

// TODO(kenton):  Copy strutil tests from google3?

TEST(StringUtilityTest, ImmuneToLocales) {
  // Remember the old locale.
  char* old_locale_cstr = setlocale(LC_NUMERIC, nullptr);
  ASSERT_TRUE(old_locale_cstr != nullptr);
  std::string old_locale = old_locale_cstr;

  // Set the locale to "C".
  ASSERT_TRUE(setlocale(LC_NUMERIC, "C") != nullptr);

  EXPECT_EQ("1.5", SimpleDtoa(1.5));
  EXPECT_EQ("1.5", SimpleFtoa(1.5));

  if (setlocale(LC_NUMERIC, "es_ES") == nullptr &&
      setlocale(LC_NUMERIC, "es_ES.utf8") == nullptr) {
    // Some systems may not have the desired locale available.
    GOOGLE_LOG(WARNING)
      << "Couldn't set locale to es_ES.  Skipping this test.";
  } else {
    EXPECT_EQ("1.5", SimpleDtoa(1.5));
    EXPECT_EQ("1.5", SimpleFtoa(1.5));
  }

  // Return to original locale.
  setlocale(LC_NUMERIC, old_locale.c_str());
}

class ReplaceChars
    : public ::testing::TestWithParam<
          std::tuple<std::string, std::string, const char*, char>> {};

TEST_P(ReplaceChars, ReplacesAllOccurencesOfAnyCharInReplaceWithAReplaceChar) {
  std::string expected = std::get<0>(GetParam());
  std::string string_to_replace_in = std::get<1>(GetParam());
  const char* what_to_replace = std::get<2>(GetParam());
  char replacement = std::get<3>(GetParam());
  ReplaceCharacters(&string_to_replace_in, what_to_replace, replacement);
  ASSERT_EQ(expected, string_to_replace_in);
}

INSTANTIATE_TEST_CASE_P(
    Replace, ReplaceChars,
    ::testing::Values(
        std::make_tuple("", "", "", '_'),    // empty string should remain empty
        std::make_tuple(" ", " ", "", '_'),  // no replacement string
        std::make_tuple(" ", " ", "_-abcedf",
                        '*'),  // replacement character not in string
        std::make_tuple("replace", "Replace", "R",
                        'r'),  // replace one character
        std::make_tuple("not_spaces__", "not\nspaces\t ", " \t\r\n",
                        '_'),  // replace some special characters
        std::make_tuple("c++", "cxx", "x",
                        '+'),  // same character multiple times
        std::make_tuple("qvvvvvng v T", "queueing a T", "aeiou",
                        'v')));  // replace all voewls

class StripWs
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(StripWs, AlwaysStripsLeadingAndTrailingWhitespace) {
  std::string expected = std::get<0>(GetParam());
  std::string string_to_strip = std::get<1>(GetParam());
  StripWhitespace(&string_to_strip);
  ASSERT_EQ(expected, string_to_strip);
}

INSTANTIATE_TEST_CASE_P(
    Strip, StripWs,
    ::testing::Values(
        std::make_tuple("", ""),   // empty string should remain empty
        std::make_tuple("", " "),  // only ws should become empty
        std::make_tuple("no whitespace",
                        " no whitespace"),  // leading ws removed
        std::make_tuple("no whitespace",
                        "no whitespace "),  // trailing ws removed
        std::make_tuple("no whitespace",
                        " no whitespace "),  // same nb. of leading and trailing
        std::make_tuple(
            "no whitespace",
            "  no whitespace "),  // different nb. of leading/trailing
        std::make_tuple("no whitespace",
                        " no whitespace  ")));  // more trailing than leading

}  // anonymous namespace
}  // namespace protobuf
}  // namespace google
