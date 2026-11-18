// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)

#include "google/protobuf/stubs/common.h"

#include <gtest/gtest.h>

#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/stubs/callback.h"
#include "google/protobuf/testing/googletest.h"

namespace google {
namespace protobuf {
namespace {

// TODO:  More tests.

#ifdef PACKAGE_VERSION  // only defined when using automake, not MSVC

TEST(VersionTest, VersionMatchesConfig) {
  // Verify that the version string specified in config.h matches the one
  // in common.h.  The config.h version is a string which may have a suffix
  // like "beta" or "rc1", so we remove that.
  std::string version = PACKAGE_VERSION;
  int pos = 0;
  while (pos < version.size() &&
         (absl::ascii_isdigit(version[pos]) || version[pos] == '.')) {
    ++pos;
  }
  version.erase(pos);

  EXPECT_EQ(version, internal::VersionString(GOOGLE_PROTOBUF_VERSION));
}

#endif  // PACKAGE_VERSION

TEST(CommonTest, IntMinMaxConstants) {
  // kint32min was declared incorrectly in the first release of protobufs.
  // Ugh.
  EXPECT_LT(kint32min, kint32max);
  EXPECT_EQ(static_cast<uint32>(kint32min), static_cast<uint32>(kint32max) + 1);
  EXPECT_LT(kint64min, kint64max);
  EXPECT_EQ(static_cast<uint64>(kint64min), static_cast<uint64>(kint64max) + 1);
  EXPECT_EQ(0, kuint32max + 1);
  EXPECT_EQ(0, kuint64max + 1);
}

class ClosureTest : public testing::Test {
 public:
  void SetA123Method()   { a_ = 123; }
  static void SetA123Function() { current_instance_->a_ = 123; }

  void SetAMethod(int a)         { a_ = a; }
  void SetCMethod(std::string c) { c_ = c; }

  static void SetAFunction(int a)         { current_instance_->a_ = a; }
  static void SetCFunction(std::string c) { current_instance_->c_ = c; }

  void SetABMethod(int a, const char* b)  { a_ = a; b_ = b; }
  static void SetABFunction(int a, const char* b) {
    current_instance_->a_ = a;
    current_instance_->b_ = b;
  }

  virtual void SetUp() {
    current_instance_ = this;
    a_ = 0;
    b_ = nullptr;
    c_.clear();
    permanent_closure_ = nullptr;
  }

  void DeleteClosureInCallback() {
    delete permanent_closure_;
  }

  int a_;
  const char* b_;
  std::string c_;
  Closure* permanent_closure_;

  static ClosureTest* current_instance_;
};

ClosureTest* ClosureTest::current_instance_ = nullptr;

TEST_F(ClosureTest, TestClosureFunction0) {
  Closure* closure = NewCallback(&SetA123Function);
  EXPECT_NE(123, a_);
  closure->Run();
  EXPECT_EQ(123, a_);
}

TEST_F(ClosureTest, TestClosureMethod0) {
  Closure* closure = NewCallback(current_instance_,
                                 &ClosureTest::SetA123Method);
  EXPECT_NE(123, a_);
  closure->Run();
  EXPECT_EQ(123, a_);
}

TEST_F(ClosureTest, TestClosureFunction1) {
  Closure* closure = NewCallback(&SetAFunction, 456);
  EXPECT_NE(456, a_);
  closure->Run();
  EXPECT_EQ(456, a_);
}

TEST_F(ClosureTest, TestClosureMethod1) {
  Closure* closure = NewCallback(current_instance_,
                                 &ClosureTest::SetAMethod, 456);
  EXPECT_NE(456, a_);
  closure->Run();
  EXPECT_EQ(456, a_);
}

TEST_F(ClosureTest, TestClosureFunction1String) {
  Closure* closure = NewCallback(&SetCFunction, std::string("test"));
  EXPECT_NE("test", c_);
  closure->Run();
  EXPECT_EQ("test", c_);
}

TEST_F(ClosureTest, TestClosureMethod1String) {
  Closure* closure = NewCallback(current_instance_, &ClosureTest::SetCMethod,
                                 std::string("test"));
  EXPECT_NE("test", c_);
  closure->Run();
  EXPECT_EQ("test", c_);
}

TEST_F(ClosureTest, TestClosureFunction2) {
  const char* cstr = "hello";
  Closure* closure = NewCallback(&SetABFunction, 789, cstr);
  EXPECT_NE(789, a_);
  EXPECT_NE(cstr, b_);
  closure->Run();
  EXPECT_EQ(789, a_);
  EXPECT_EQ(cstr, b_);
}

TEST_F(ClosureTest, TestClosureMethod2) {
  const char* cstr = "hello";
  Closure* closure = NewCallback(current_instance_,
                                 &ClosureTest::SetABMethod, 789, cstr);
  EXPECT_NE(789, a_);
  EXPECT_NE(cstr, b_);
  closure->Run();
  EXPECT_EQ(789, a_);
  EXPECT_EQ(cstr, b_);
}

// Repeat all of the above with NewPermanentCallback()

TEST_F(ClosureTest, TestPermanentClosureFunction0) {
  Closure* closure = NewPermanentCallback(&SetA123Function);
  EXPECT_NE(123, a_);
  closure->Run();
  EXPECT_EQ(123, a_);
  a_ = 0;
  closure->Run();
  EXPECT_EQ(123, a_);
  delete closure;
}

TEST_F(ClosureTest, TestPermanentClosureMethod0) {
  Closure* closure = NewPermanentCallback(current_instance_,
                                          &ClosureTest::SetA123Method);
  EXPECT_NE(123, a_);
  closure->Run();
  EXPECT_EQ(123, a_);
  a_ = 0;
  closure->Run();
  EXPECT_EQ(123, a_);
  delete closure;
}

TEST_F(ClosureTest, TestPermanentClosureFunction1) {
  Closure* closure = NewPermanentCallback(&SetAFunction, 456);
  EXPECT_NE(456, a_);
  closure->Run();
  EXPECT_EQ(456, a_);
  a_ = 0;
  closure->Run();
  EXPECT_EQ(456, a_);
  delete closure;
}

TEST_F(ClosureTest, TestPermanentClosureMethod1) {
  Closure* closure = NewPermanentCallback(current_instance_,
                                          &ClosureTest::SetAMethod, 456);
  EXPECT_NE(456, a_);
  closure->Run();
  EXPECT_EQ(456, a_);
  a_ = 0;
  closure->Run();
  EXPECT_EQ(456, a_);
  delete closure;
}

TEST_F(ClosureTest, TestPermanentClosureFunction2) {
  const char* cstr = "hello";
  Closure* closure = NewPermanentCallback(&SetABFunction, 789, cstr);
  EXPECT_NE(789, a_);
  EXPECT_NE(cstr, b_);
  closure->Run();
  EXPECT_EQ(789, a_);
  EXPECT_EQ(cstr, b_);
  a_ = 0;
  b_ = nullptr;
  closure->Run();
  EXPECT_EQ(789, a_);
  EXPECT_EQ(cstr, b_);
  delete closure;
}

TEST_F(ClosureTest, TestPermanentClosureMethod2) {
  const char* cstr = "hello";
  Closure* closure = NewPermanentCallback(current_instance_,
                                          &ClosureTest::SetABMethod, 789, cstr);
  EXPECT_NE(789, a_);
  EXPECT_NE(cstr, b_);
  closure->Run();
  EXPECT_EQ(789, a_);
  EXPECT_EQ(cstr, b_);
  a_ = 0;
  b_ = nullptr;
  closure->Run();
  EXPECT_EQ(789, a_);
  EXPECT_EQ(cstr, b_);
  delete closure;
}

TEST_F(ClosureTest, TestPermanentClosureDeleteInCallback) {
  permanent_closure_ = NewPermanentCallback((ClosureTest*) this,
      &ClosureTest::DeleteClosureInCallback);
  permanent_closure_->Run();
}

}  // anonymous namespace
}  // namespace protobuf
}  // namespace google
