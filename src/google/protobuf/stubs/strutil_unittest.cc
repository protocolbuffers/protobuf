// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)

#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace {

// TODO(kenton):  Copy strutil tests from google3?

TEST(StringUtilityTest, ImmuneToLocales) {
  // Remember the old locale.
  char* old_locale_cstr = setlocale(LC_NUMERIC, NULL);
  ASSERT_TRUE(old_locale_cstr != NULL);
  string old_locale = old_locale_cstr;

  // Set the locale to "C".
  ASSERT_TRUE(setlocale(LC_NUMERIC, "C") != NULL);

  EXPECT_EQ(1.5, NoLocaleStrtod("1.5", NULL));
  EXPECT_EQ("1.5", SimpleDtoa(1.5));
  EXPECT_EQ("1.5", SimpleFtoa(1.5));

  // Verify that the endptr is set correctly even if not all text was parsed.
  const char* text = "1.5f";
  char* endptr;
  EXPECT_EQ(1.5, NoLocaleStrtod(text, &endptr));
  EXPECT_EQ(3, endptr - text);

  if (setlocale(LC_NUMERIC, "es_ES") == NULL &&
      setlocale(LC_NUMERIC, "es_ES.utf8") == NULL) {
    // Some systems may not have the desired locale available.
    GOOGLE_LOG(WARNING)
      << "Couldn't set locale to es_ES.  Skipping this test.";
  } else {
    EXPECT_EQ(1.5, NoLocaleStrtod("1.5", NULL));
    EXPECT_EQ("1.5", SimpleDtoa(1.5));
    EXPECT_EQ("1.5", SimpleFtoa(1.5));
    EXPECT_EQ(1.5, NoLocaleStrtod(text, &endptr));
    EXPECT_EQ(3, endptr - text);
  }

  // Return to original locale.
  setlocale(LC_NUMERIC, old_locale.c_str());
}

}  // anonymous namespace
}  // namespace protobuf
}  // namespace google
