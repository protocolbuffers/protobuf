// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC. nor the names of its
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

#include "google/protobuf/compiler/rust/relative_path.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

using testing::Eq;

TEST(RelativePathTest, GetRelativePath) {
  auto relative = [](absl::string_view from_path, absl::string_view to_path) {
    return RelativePath(from_path).Relative(RelativePath(to_path));
  };
  EXPECT_EQ(relative("foo/bar/baz.txt", "foo/bar/file.txt"), "file.txt");
  EXPECT_EQ(relative("foo/bar/", "foo/bar/file.txt"), "file.txt");

  EXPECT_EQ(relative("foo/bar/baz.txt", "foo/file.txt"), "../file.txt");
  EXPECT_EQ(relative("foo/bar/", "foo/file.txt"), "../file.txt");

  EXPECT_EQ(relative("foo/baz.txt", "foo/bar/baz/file.txt"),
            "bar/baz/file.txt");
  EXPECT_EQ(relative("foo/", "foo/bar/baz/file.txt"), "bar/baz/file.txt");

  EXPECT_EQ(relative("baz.txt", "foo/bar/file.txt"), "foo/bar/file.txt");
  EXPECT_EQ(relative("", "foo/bar/file.txt"), "foo/bar/file.txt");
}

}  // namespace

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
