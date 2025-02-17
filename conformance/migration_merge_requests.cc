#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/migration_list.pb.h"
#include "google/protobuf/text_format.h"

ABSL_FLAG(std::vector<std::string>, inputs, {}, "The input files to merge.");
ABSL_FLAG(std::string, output, "", "The output file to write to.");

namespace google {
namespace protobuf {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::MigrationList;

struct RequestLess {
  bool operator()(const ConformanceRequest& a,
                  const ConformanceRequest& b) const {
    if (a.test_category() != b.test_category()) {
      return a.test_category() < b.test_category();
    }
    if (a.payload_case() != b.payload_case()) {
      return a.payload_case() < b.payload_case();
    }
    if (a.requested_output_format() != b.requested_output_format()) {
      return a.requested_output_format() < b.requested_output_format();
    }
    if (a.print_unknown_fields() != b.print_unknown_fields()) {
      return a.print_unknown_fields() < b.print_unknown_fields();
    }
    if (a.message_type() != b.message_type()) {
      return a.message_type() < b.message_type();
    }
    absl::string_view payload_a = a.GetReflection()->GetStringView(
        a, a.GetDescriptor()->FindFieldByNumber(a.payload_case()));
    absl::string_view payload_b = b.GetReflection()->GetStringView(
        b, b.GetDescriptor()->FindFieldByNumber(b.payload_case()));
    return payload_a < payload_b;
  }
};

MigrationList MergeRequests(const std::vector<std::string>& inputs) {
  MigrationList list;
  absl::btree_set<ConformanceRequest, RequestLess> requests;
  for (const std::string& input : inputs) {
    std::ifstream file(input);
    std::stringstream buffer;
    buffer << file.rdbuf();

    MigrationList input_list;
    ABSL_CHECK(TextFormat::ParseFromString(buffer.str(), &input_list));
    for (const ConformanceRequest& request : input_list.requests()) {
      auto [it, inserted] = requests.insert(request);
      ABSL_CHECK(inserted) << "Duplicate request: " << request.DebugString()
                           << "\n"
                           << *it;
    }
  }
  for (const ConformanceRequest& request : requests) {
    *list.add_requests() = request;
  }
  return list;
}

void Write(std::string output, MigrationList list) {
  std::ofstream file(output);
  std::string out;
  ABSL_CHECK(TextFormat::PrintToString(list, &out));
  file << out;
}

}  // namespace
}  // namespace protobuf
}  // namespace google

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  google::protobuf::Write(absl::GetFlag(FLAGS_output),
                google::protobuf::MergeRequests(absl::GetFlag(FLAGS_inputs)));
  return 0;
}
