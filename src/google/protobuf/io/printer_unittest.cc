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
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <vector>

#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace io {
namespace {

// Each test repeats over several block sizes in order to test both cases
// where particular writes cross a buffer boundary and cases where they do
// not.

TEST(Printer, EmptyPrinter) {
  char buffer[8192];
  const int block_size = 100;
  ArrayOutputStream output(buffer, GOOGLE_ARRAYSIZE(buffer), block_size);
  Printer printer(&output, '\0');
  EXPECT_TRUE(!printer.failed());
}

TEST(Printer, BasicPrinting) {
  char buffer[8192];

  for (int block_size = 1; block_size < 512; block_size *= 2) {
    ArrayOutputStream output(buffer, sizeof(buffer), block_size);

    {
      Printer printer(&output, '\0');

      printer.Print("Hello World!");
      printer.Print("  This is the same line.\n");
      printer.Print("But this is a new one.\nAnd this is another one.");

      EXPECT_FALSE(printer.failed());
    }

    buffer[output.ByteCount()] = '\0';

    EXPECT_STREQ(buffer,
      "Hello World!  This is the same line.\n"
      "But this is a new one.\n"
      "And this is another one.");
  }
}

TEST(Printer, VariableSubstitution) {
  char buffer[8192];

  for (int block_size = 1; block_size < 512; block_size *= 2) {
    ArrayOutputStream output(buffer, sizeof(buffer), block_size);

    {
      Printer printer(&output, '$');
      map<string, string> vars;

      vars["foo"] = "World";
      vars["bar"] = "$foo$";
      vars["abcdefg"] = "1234";

      printer.Print(vars, "Hello $foo$!\nbar = $bar$\n");
      printer.Print(vars, "$abcdefg$\nA literal dollar sign:  $$");

      vars["foo"] = "blah";
      printer.Print(vars, "\nNow foo = $foo$.");

      EXPECT_FALSE(printer.failed());
    }

    buffer[output.ByteCount()] = '\0';

    EXPECT_STREQ(buffer,
      "Hello World!\n"
      "bar = $foo$\n"
      "1234\n"
      "A literal dollar sign:  $\n"
      "Now foo = blah.");
  }
}

TEST(Printer, InlineVariableSubstitution) {
  char buffer[8192];

  ArrayOutputStream output(buffer, sizeof(buffer));

  {
    Printer printer(&output, '$');
    printer.Print("Hello $foo$!\n", "foo", "World");
    printer.Print("$foo$ $bar$\n", "foo", "one", "bar", "two");
    EXPECT_FALSE(printer.failed());
  }

  buffer[output.ByteCount()] = '\0';

  EXPECT_STREQ(buffer,
    "Hello World!\n"
    "one two\n");
}

TEST(Printer, Indenting) {
  char buffer[8192];

  for (int block_size = 1; block_size < 512; block_size *= 2) {
    ArrayOutputStream output(buffer, sizeof(buffer), block_size);

    {
      Printer printer(&output, '$');
      map<string, string> vars;

      vars["newline"] = "\n";

      printer.Print("This is not indented.\n");
      printer.Indent();
      printer.Print("This is indented\nAnd so is this\n");
      printer.Outdent();
      printer.Print("But this is not.");
      printer.Indent();
      printer.Print("  And this is still the same line.\n"
                    "But this is indented.\n");
      printer.Print(vars, "Note that a newline in a variable will break "
                    "indenting, as we see$newline$here.\n");
      printer.Indent();
      printer.Print("And this");
      printer.Outdent();
      printer.Outdent();
      printer.Print(" is double-indented\nBack to normal.");

      EXPECT_FALSE(printer.failed());
    }

    buffer[output.ByteCount()] = '\0';

    EXPECT_STREQ(buffer,
      "This is not indented.\n"
      "  This is indented\n"
      "  And so is this\n"
      "But this is not.  And this is still the same line.\n"
      "  But this is indented.\n"
      "  Note that a newline in a variable will break indenting, as we see\n"
      "here.\n"
      "    And this is double-indented\n"
      "Back to normal.");
  }
}

// Death tests do not work on Windows as of yet.
#ifdef GTEST_HAS_DEATH_TEST
TEST(Printer, Death) {
  char buffer[8192];

  ArrayOutputStream output(buffer, sizeof(buffer));
  Printer printer(&output, '$');

  EXPECT_DEBUG_DEATH(printer.Print("$nosuchvar$"), "Undefined variable");
  EXPECT_DEBUG_DEATH(printer.Print("$unclosed"), "Unclosed variable name");
  EXPECT_DEBUG_DEATH(printer.Outdent(), "without matching Indent");
}
#endif  // GTEST_HAS_DEATH_TEST

TEST(Printer, WriteFailure) {
  char buffer[16];

  ArrayOutputStream output(buffer, sizeof(buffer));
  Printer printer(&output, '$');

  // Print 16 bytes to fill the buffer exactly (should not fail).
  printer.Print("0123456789abcdef");
  EXPECT_FALSE(printer.failed());

  // Try to print one more byte (should fail).
  printer.Print(" ");
  EXPECT_TRUE(printer.failed());

  // Should not crash
  printer.Print("blah");
  EXPECT_TRUE(printer.failed());

  // Buffer should contain the first 16 bytes written.
  EXPECT_EQ("0123456789abcdef", string(buffer, sizeof(buffer)));
}

}  // namespace
}  // namespace io
}  // namespace protobuf
}  // namespace google
