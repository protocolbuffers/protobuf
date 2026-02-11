// Copyright (c) 2020-2025, Google LLC
// All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <algorithm>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "google/protobuf/descriptor.pb.h"
#include "gtest/gtest.h"
#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;
using google::protobuf::FileDescriptorProto;
using google::protobuf::FileDescriptorSet;

namespace rulesproto {
constexpr char kWorkspaceRlocation[] = "protobuf/";
constexpr char kWorkspaceRlocationBzlmod[] = "_main/";

namespace {

std::string GetRlocation(const std::string& file) {
  static std::unique_ptr<Runfiles> runfiles(Runfiles::CreateForTest());
  std::string path =
      runfiles->Rlocation(rulesproto::kWorkspaceRlocation + file);
  std::ifstream input(path, std::ifstream::binary);
  if (!input) {
    path = runfiles->Rlocation(rulesproto::kWorkspaceRlocationBzlmod + file);
  }
  return path;
}

template <typename T, typename K>
bool Contains(const T& container, const K& key) {
  return container.find(key) != container.end();
}

std::vector<std::string> ReadFileDescriptorSet(const std::string& path) {
  std::ifstream input(path, std::ifstream::binary);
  EXPECT_TRUE(input) << "Could not open " << path;

  FileDescriptorSet file_descriptor_set;
  EXPECT_TRUE(file_descriptor_set.ParseFromIstream(&input));

  std::set<std::string> unordered_proto_files;
  for (FileDescriptorProto file_descriptor : file_descriptor_set.file()) {
    EXPECT_FALSE(Contains(unordered_proto_files, file_descriptor.name()))
        << "Already saw " << file_descriptor.name();
    unordered_proto_files.insert(file_descriptor.name());
  }

  std::vector<std::string> proto_files(unordered_proto_files.begin(),
                                       unordered_proto_files.end());
  std::sort(proto_files.begin(), proto_files.end());
  return proto_files;
}

void AssertFileDescriptorSetContains(
    const std::string& path,
    const std::vector<std::string>& expected_proto_files) {
  std::vector<std::string> actual_proto_files =
      ReadFileDescriptorSet(GetRlocation(path));
  EXPECT_THAT(actual_proto_files,
              ::testing::IsSupersetOf(expected_proto_files));
}

}  // namespace

TEST(ProtoDescriptorSetTest, NoProtos) {
  AssertFileDescriptorSetContains("bazel/tests/no_protos.pb", {});
}

TEST(ProtoDescriptorSetTest, WellKnownProtos) {
  AssertFileDescriptorSetContains("bazel/tests/well_known_protos.pb",
                                  {
                                      "google/protobuf/any.proto",
                                      "google/protobuf/api.proto",
                                      "google/protobuf/descriptor.proto",
                                      "google/protobuf/duration.proto",
                                      "google/protobuf/empty.proto",
                                      "google/protobuf/field_mask.proto",
                                      "google/protobuf/source_context.proto",
                                      "google/protobuf/struct.proto",
                                      "google/protobuf/timestamp.proto",
                                      "google/protobuf/type.proto",
                                      "google/protobuf/wrappers.proto",
                                  });
}

}  // namespace rulesproto
