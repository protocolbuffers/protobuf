#ifndef GOOGLE_PROTOBUF_COMPILER_TMPFILE_UTILS_H__
#define GOOGLE_PROTOBUF_COMPILER_TMPFILE_UTILS_H__

#include <cstdlib>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/file_utils.h"

namespace google {
namespace protobuf {
namespace compiler {

// Writes the text into a file into TEST_TMPDIR.
//
// TEST_TMPDIR environment variable is set by Bazel when running tests.
//
// Returns an absolute path to the temp file.
inline absl::StatusOr<std::string> WriteStringToTestTmpDirFile(
    absl::string_view path, absl::string_view text) {
  absl::string_view tmpdir = std::getenv("TEST_TMPDIR");
  if (tmpdir.empty()) {
    return absl::InvalidArgumentError(
        "TEST_TMPDIR env var is empty, set it or use Blaze/Bazel to drive the "
        "test.");
  }
  std::string absolute_path = absl::StrCat(tmpdir, "/", path);
  if (!WriteStringToFile(absolute_path, text).ok()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Couldn't write file ", path));
  }
  return absolute_path;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_TMPFILE_UTILS_H__
