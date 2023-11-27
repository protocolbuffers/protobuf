// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/names.h"

#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

TEST(ObjCHelper, IsRetainedName) {
  EXPECT_TRUE(IsRetainedName("new"));
  EXPECT_TRUE(IsRetainedName("alloc"));
  EXPECT_TRUE(IsRetainedName("copy"));
  EXPECT_TRUE(IsRetainedName("mutableCopy"));
  EXPECT_TRUE(IsRetainedName("newFoo"));
  EXPECT_TRUE(IsRetainedName("allocFoo"));
  EXPECT_TRUE(IsRetainedName("copyFoo"));
  EXPECT_TRUE(IsRetainedName("mutableCopyFoo"));
  EXPECT_TRUE(IsRetainedName("new_foo"));
  EXPECT_TRUE(IsRetainedName("alloc_foo"));
  EXPECT_TRUE(IsRetainedName("copy_foo"));
  EXPECT_TRUE(IsRetainedName("mutableCopy_foo"));

  EXPECT_FALSE(IsRetainedName(""));
  EXPECT_FALSE(IsRetainedName("ne"));
  EXPECT_FALSE(IsRetainedName("all"));
  EXPECT_FALSE(IsRetainedName("co"));
  EXPECT_FALSE(IsRetainedName("mutable"));
  EXPECT_FALSE(IsRetainedName("New"));
  EXPECT_FALSE(IsRetainedName("Alloc"));
  EXPECT_FALSE(IsRetainedName("Copy"));
  EXPECT_FALSE(IsRetainedName("MutableCopy"));
  EXPECT_FALSE(IsRetainedName("newer"));
  EXPECT_FALSE(IsRetainedName("alloced"));
  EXPECT_FALSE(IsRetainedName("copying"));
  EXPECT_FALSE(IsRetainedName("mutableCopying"));

  EXPECT_FALSE(IsRetainedName("init"));
  EXPECT_FALSE(IsRetainedName("Create"));
  EXPECT_FALSE(IsRetainedName("Copy"));
}

TEST(ObjCHelper, IsInitName) {
  EXPECT_TRUE(IsInitName("init"));
  EXPECT_TRUE(IsInitName("initFoo"));
  EXPECT_TRUE(IsInitName("init_foo"));

  EXPECT_FALSE(IsInitName(""));
  EXPECT_FALSE(IsInitName("in"));
  EXPECT_FALSE(IsInitName("Init"));
  EXPECT_FALSE(IsInitName("inIt"));
  EXPECT_FALSE(IsInitName("initial"));
  EXPECT_FALSE(IsInitName("initiAl"));
  EXPECT_FALSE(IsInitName("fooInit"));
  EXPECT_FALSE(IsInitName("foo_init"));

  EXPECT_FALSE(IsInitName("new"));
  EXPECT_FALSE(IsInitName("alloc"));
  EXPECT_FALSE(IsInitName("copy"));
  EXPECT_FALSE(IsInitName("mutableCopy"));
  EXPECT_FALSE(IsInitName("Create"));
  EXPECT_FALSE(IsInitName("Copy"));
}

TEST(ObjCHelper, IsCreateName) {
  EXPECT_TRUE(IsCreateName("Create"));
  EXPECT_TRUE(IsCreateName("Copy"));
  EXPECT_TRUE(IsCreateName("CreateFoo"));
  EXPECT_TRUE(IsCreateName("CopyFoo"));
  EXPECT_TRUE(IsCreateName("Create_foo"));
  EXPECT_TRUE(IsCreateName("Copy_foo"));
  EXPECT_TRUE(IsCreateName("ReCreate"));
  EXPECT_TRUE(IsCreateName("ReCopy"));
  EXPECT_TRUE(IsCreateName("FOOCreate"));
  EXPECT_TRUE(IsCreateName("FOOCopy"));
  EXPECT_TRUE(IsCreateName("CreateWithCopy"));

  EXPECT_FALSE(IsCreateName(""));
  EXPECT_FALSE(IsCreateName("Crea"));
  EXPECT_FALSE(IsCreateName("Co"));
  EXPECT_FALSE(IsCreateName("create"));
  EXPECT_FALSE(IsCreateName("recreate"));
  EXPECT_FALSE(IsCreateName("recopy"));
  EXPECT_FALSE(IsCreateName("ReCreated"));
  EXPECT_FALSE(IsCreateName("ReCopying"));

  EXPECT_FALSE(IsCreateName("init"));
  EXPECT_FALSE(IsCreateName("new"));
  EXPECT_FALSE(IsCreateName("alloc"));
  EXPECT_FALSE(IsCreateName("copy"));
  EXPECT_TRUE(IsCreateName("mutableCopy"));
}

// TODO: Should probably add some unittests for all the special cases
// of name mangling (class name, field name, enum names).  Rather than doing
// this with an ObjC test in the objectivec directory, we should be able to
// use src/google/protobuf/compiler/importer* (like other tests) to support a
// virtual file system to feed in protos, once we have the Descriptor tree, the
// tests could use the helper methods for generating names and validate the
// right things are happening.

}  // namespace

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
