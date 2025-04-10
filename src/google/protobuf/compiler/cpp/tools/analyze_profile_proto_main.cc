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

#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>

#include "base/init_google.h"
#include "base/sysinfo.h"
#include "google/protobuf/util/globaldb/global_descriptor_database.h"
#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/globals.h"
#include "absl/status/status.h"
#include "google/protobuf/compiler/cpp/tools/analyze_profile_proto.h"
#include "google/protobuf/descriptor.h"

ABSL_FLAG(bool, all, false, "Print all fields");
ABSL_FLAG(bool, analysis, false, "Print field analysis");
ABSL_FLAG(bool, analysis_all, false,
          "Print detailed field analysis, such as field presence probability, "
          "for all fields, not just hot or cold ones.");
ABSL_FLAG(std::string, message_filter, "", "Regex match for message name");
ABSL_FLAG(bool, print_unused_threshold, true,
          "Print the 'unlikely used' threshold.");
ABSL_FLAG(bool, print_optimized, true,
          "Print the PDProto optimizations that would be applied to the "
          "field.");
ABSL_FLAG(bool, aggregate_analysis, false,
          "If set, will recursively find proto.profile in the given dir and "
          "print the aggregated analysis.");
ABSL_FLAG(std::string, out_file, "",
          "If set, will write the output to the given file instead of stdout.");
ABSL_FLAG(bool, sort_output_by_file_name, false,
          "If set, will sort the per-file output by the file name. Note that "
          "this will delay writing to the output file until all the files are "
          "processed.");
ABSL_FLAG(int, parallelism, NumCPUs(),
          "Number of threads to use to process proto profiles in parallel.");

int main(int argc, char* argv[]) {
  using google::protobuf::compiler::tools::AnalyzeAndAggregateProfileProtosToText;
  using google::protobuf::compiler::tools::AnalyzeProfileProtoOptions;
  using google::protobuf::compiler::tools::AnalyzeProfileProtoToText;

  InitGoogle(argv[0], &argc, &argv, true);

  ABSL_QCHECK_EQ(argc, 2) << "Usage: " << argv[0]
                          << " [OPTIONS] <profile_file_or_dir>";

  // Direct `ABSL_LOG(INFO)` to stderr (otherwise, it is suppressed by default).
  // Note that `--stderrthreshold=N` on the command line overrides this.
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  std::ostream* stream = nullptr;
  std::ofstream out_file_stream;
  const std::string out_file = absl::GetFlag(FLAGS_out_file);
  if (out_file.empty()) {
    stream = &std::cout;
  } else {
    out_file_stream.open(out_file, std::ios::out | std::ios::trunc);
    ABSL_QCHECK(out_file_stream.is_open()) << "Failed to open: " << out_file;
    stream = &out_file_stream;
  }

  google::protobuf::DescriptorPool pool(google::protobuf::util::globaldb::global());
  const AnalyzeProfileProtoOptions options = {
      .print_unused_threshold = absl::GetFlag(FLAGS_print_unused_threshold),
      .print_optimized = absl::GetFlag(FLAGS_print_optimized),
      .print_all_fields = absl::GetFlag(FLAGS_all),
      .print_analysis = absl::GetFlag(FLAGS_analysis),
      .print_analysis_all = absl::GetFlag(FLAGS_analysis_all),
      .pool = &pool,
      .message_filter = absl::GetFlag(FLAGS_message_filter),
      .sort_output_by_file_name = absl::GetFlag(FLAGS_sort_output_by_file_name),
      .parallelism = absl::GetFlag(FLAGS_parallelism),
  };
  const absl::Status status =
      absl::GetFlag(FLAGS_aggregate_analysis)
          ? AnalyzeAndAggregateProfileProtosToText(*stream, argv[1], options)
          : AnalyzeProfileProtoToText(*stream, argv[1], options);

  if (!status.ok()) {
    ABSL_LOG(ERROR) << "FAILED; LAST FAILURE: " << status;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
