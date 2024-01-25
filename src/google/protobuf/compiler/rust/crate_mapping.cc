#include "google/protobuf/compiler/rust/crate_mapping.h"

#include <fcntl.h>

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/rust/context.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

absl::StatusOr<absl::flat_hash_map<std::string, std::string>>
GetImportPathToCrateNameMap(const Options* opts) {
  absl::flat_hash_map<std::string, std::string> mapping;
  if (opts->mapping_file_path.empty()) {
    return mapping;
  }
  std::string mapping_contents;
  absl::Status status =
      File::ReadFileToString(opts->mapping_file_path, &mapping_contents, true);
  if (!status.ok()) {
    return status;
  }

  std::vector<absl::string_view> lines =
      absl::StrSplit(mapping_contents, '\n', absl::SkipEmpty());
  size_t len = lines.size();

  size_t idx = 0;
  while (idx < len) {
    absl::string_view crate_name = lines[idx++];
    size_t files_cnt;
    if (!absl::SimpleAtoi(lines[idx++], &files_cnt)) {
      return absl::InvalidArgumentError(
          "Couldn't parse number of import paths in mapping file");
    }
    for (size_t i = 0; i < files_cnt; ++i) {
      mapping.insert({std::string(lines[idx++]), std::string(crate_name)});
    }
  }
  return mapping;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
