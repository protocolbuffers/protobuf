// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/json_options.pb.h"
#include "google/protobuf/json_options_unittest.pb.h"

namespace json_options_unittest {
namespace {

TEST(JsonOptionsTest, GreatHelmDescriptorAccess) {
  auto desc = google::protobuf::GetEnumDescriptor<Armor>();
  auto great_helm_desc = desc->FindValueByName("ARMOR_GREAT_HELM");
  EXPECT_EQ(great_helm_desc->options().GetExtension(
                pb::json_options::enum_value_name),
            "gr8 helm");
}

TEST(JsonOptionsTest, GreatHelmSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GREAT_HELM);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GREAT_HELM);

  std::string json_res{};
  absl::Status status = google::protobuf::json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":"gr8 helm"})json");
}

TEST(JsonOptionsTest, GreatHelmIntOverride) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GREAT_HELM);
  std::string json_res{};
  absl::Status status = google::protobuf::json::MessageToJsonString(
      msg, &json_res,
      google::protobuf::json::PrintOptions{.always_print_enums_as_ints = true});
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":1})json");
}

}  // namespace
}  // namespace json_options_unittest
