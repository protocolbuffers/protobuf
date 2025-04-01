// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/relative_path.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

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
