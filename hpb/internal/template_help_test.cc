// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/hpb/internal/template_help.h"

#include <gtest/gtest.h>
#include "google/protobuf/compiler/hpb/tests/test_model.upb.proto.h"

namespace hpb_unittest::protos {

class NonHpbClass {};

TEST(TemplateHelp, TestIsHpbClass) {
  static_assert(hpb::internal::IsHpbClass<TestModel>,
                "TestModel must be an hpb class");
  static_assert(hpb::internal::IsHpbClass<ThemeExtension>,
                "ThemeExtension must be an hpb class");
  static_assert(!hpb::internal::IsHpbClass<NonHpbClass>,
                "NonHpbClass must not be an hpb class");
  static_assert(!hpb::internal::IsHpbClass<int>,
                "primitives like int must not be an hpb class");
}

TEST(TemplateHelp, TestIsHpbExtendedClass) {
  static_assert(
      hpb::internal::IsHpbClassThatHasExtensions<TestModel>,
      "TestModel must be an hpb extension class, for it has extensions");
  static_assert(!hpb::internal::IsHpbClassThatHasExtensions<ThemeExtension>,
                "ThemeExtension must not have extensions");
  static_assert(!hpb::internal::IsHpbClassThatHasExtensions<NonHpbClass>,
                "NonHpbClass must not be an hpb extension class");
  static_assert(!hpb::internal::IsHpbClassThatHasExtensions<int>,
                "primitives like int must not be an hpb extension class");
}

}  // namespace hpb_unittest::protos
