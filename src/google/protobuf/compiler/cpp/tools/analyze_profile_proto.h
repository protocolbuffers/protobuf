#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_TOOLS_ANALYZE_PROFILE_PROTO_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_TOOLS_ANALYZE_PROFILE_PROTO_H__

#include <ostream>
#include <string>

#include "base/sysinfo.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace tools {

struct AnalyzeProfileProtoOptions {
  // true to print the 'unlikely used' threshold.
  bool print_unused_threshold = true;

  // true to print the PDProto optimizations that would be applied to the field.
  bool print_optimized = true;

  // true to print all fields instead of optimized fields only.
  bool print_all_fields = false;

  // true to include presence and usage info instead of only optimization info
  bool print_analysis = false;

  // true to include presence probability info
  bool print_analysis_all = false;

  // Descriptor pool to use. Must not be null.
  const DescriptorPool* pool = nullptr;

  // Regular expression for message name matching, empty to include all.
  std::string message_filter;

  // Sort the per-file output by the file name.
  bool sort_output_by_file_name = false;

  // Number of threads to use to process proto profiles in parallel.
  int parallelism = NumCPUs();
};

// Prints analysis for the given proto profile.
absl::Status AnalyzeProfileProtoToText(
    std::ostream& stream, absl::string_view proto_profile,
    const AnalyzeProfileProtoOptions& options);

// Prints aggregated analysis for the proto profiles under the given root
// directory.
absl::Status AnalyzeAndAggregateProfileProtosToText(
    std::ostream& stream, absl::string_view root,
    const AnalyzeProfileProtoOptions& options);

}  // namespace tools
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_TOOLS_ANALYZE_PROFILE_PROTO_H__
