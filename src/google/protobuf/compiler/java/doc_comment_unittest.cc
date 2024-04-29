// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)

#include "google/protobuf/compiler/java/doc_comment.h"

#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

TEST(JavaDocCommentTest, Escaping) {
  EXPECT_EQ("foo /&#42; bar *&#47; baz", EscapeJavadoc("foo /* bar */ baz"));
  EXPECT_EQ("foo /&#42;&#47; baz", EscapeJavadoc("foo /*/ baz"));
  EXPECT_EQ("{&#64;foo}", EscapeJavadoc("{@foo}"));
  EXPECT_EQ("&lt;i&gt;&amp;&lt;/i&gt;", EscapeJavadoc("<i>&</i>"));
  EXPECT_EQ("foo&#92;u1234bar", EscapeJavadoc("foo\\u1234bar"));
  EXPECT_EQ("&#64;deprecated", EscapeJavadoc("@deprecated"));
}

}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
