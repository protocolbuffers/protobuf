// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This tool takes a PDProto profile as input and prints out the analysis, such
// as the PDProto optimizations that would be applied based on the given
// profile.
//
// It can also take a directory as input and print out the aggregated analysis
// for all the PDProto profiles under the directory. This is useful when we want
// to get some statistics for the fleet.

#include <iostream>
#include <string>

#include "base/init_google.h"
#include "google/protobuf/util/globaldb/global_descriptor_database.h"
#include "absl/flags/flag.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "google/protobuf/compiler/cpp/tools/analyze_profile_proto.h"
#include "google/protobuf/descriptor.h"

ABSL_FLAG(bool, all, false, "Print all fields");
ABSL_FLAG(bool, analysis, false, "Print field analysis");
ABSL_FLAG(std::string, message_filter, "", "Regex match for message name");
ABSL_FLAG(bool, aggregate_analysis, false,
          "If set, will recursively find proto.profile in the given dir and "
          "print the aggregated analysis. Will not print individual analysis.");

int main(int argc, char* argv[]) {
  using google::protobuf::compiler::tools::AnalyzeProfileProtoOptions;
  using google::protobuf::compiler::tools::AnalyzeProfileProtoToText;

  InitGoogle(argv[0], &argc, &argv, true);
  if (argc < 2) return 1;

  google::protobuf::DescriptorPool pool(google::protobuf::util::globaldb::global());
  AnalyzeProfileProtoOptions options;
  options.pool = &pool;
  absl::Status status;
  if (!absl::GetFlag(FLAGS_aggregate_analysis)) {
    options.print_all_fields = absl::GetFlag(FLAGS_all);
    options.print_analysis = absl::GetFlag(FLAGS_analysis);
    options.message_filter = absl::GetFlag(FLAGS_message_filter);
    status = AnalyzeProfileProtoToText(std::cout, argv[1], options);
  } else {
    options.print_unused_threshold = false;
    options.print_optimized = false;
    status =
        AnalyzeAndAggregateProfileProtosToText(std::cout, argv[1], options);
  }
  if (!status.ok()) {
    ABSL_LOG(ERROR) << status;
    return 2;
  }
  return 0;
}
