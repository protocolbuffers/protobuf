// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb_generator/tests/child_model.hpb.h"
#include "hpb_generator/tests/test_model.hpb.h"
#include "hpb/arena.h"
#include "hpb/hpb.h"
#include "hpb/ptr.h"

namespace {

using ::hpb_unittest::protos::ChildModel1;
using ::hpb_unittest::protos::TestModel;

#if defined(NON_COMPILE_TEST)

// clang-format off
void TestConstAccessors() {
  TestModel model;
  hpb::Ptr<const TestModel> const_ptr(&model);

  // Calling mutating setters via const Ptr is prohibited.
  // expected-error@+1 {{'set_str1' is a private member}}
  const_ptr->set_str1("illegal");

  // Calling mutating clear methods via const Ptr is prohibited.
  // expected-error@+1 {{'clear_str1' is a private member}}
  const_ptr->clear_str1();

  // Calling scalar field setters via const Ptr is prohibited.
  // expected-error@+1 {{'set_value' is a private member}}
  const_ptr->set_value(42);

  // Calling scalar field clear via const Ptr is prohibited.
  // expected-error@+1 {{'clear_value' is a private member}}
  const_ptr->clear_value();

  // Const submessage accessor returns Ptr<const ChildModel1>, which cannot
  // be mutated.
  // expected-error@+1 {{'set_child_str1' is a private member}}
  model.child_model_1()->set_child_str1("illegal");
}

void TestClearConstMessage() {
  TestModel model;
  hpb::Ptr<const TestModel> const_ptr(&model);

  // b/288491350: Only mutable messages can be cleared, not Ptr<const T>.
  // expected-error@+1 {{no matching function for call to 'ClearMessage'}}
  hpb::ClearMessage(const_ptr);

  // expected-error@+1 {{no matching function for call to 'ClearMessage'}}
  hpb::ClearMessage(model.child_model_1());
}

void TestConstPointerConversion() {
  TestModel model;
  hpb::Ptr<const TestModel> const_ptr(&model);

  // Conversion from Ptr<const T> to Ptr<T> must not compile.
  // expected-error@+1 {{no viable conversion}}
  hpb::Ptr<TestModel> mutable_ptr = const_ptr;
}
// clang-format on

#endif  // NON_COMPILE_TEST

}  // namespace
