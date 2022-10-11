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

#include "google/protobuf/stubs/strutil.h"

#include <gtest/gtest.h>
#include <locale.h>

#include "google/protobuf/testing/googletest.h"

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

}  // anonymous namespace
}  // namespace protobuf
}  // namespace google
