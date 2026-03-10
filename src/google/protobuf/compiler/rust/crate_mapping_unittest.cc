#include "google/protobuf/compiler/rust/crate_mapping.h"

#include <string>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/googletest.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/rust/context.h"

using google::protobuf::compiler::rust::Options;
using testing::Eq;
using testing::IsEmpty;
using testing::UnorderedElementsAreArray;

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

namespace {

std::string WriteStringToTempFile(absl::string_view text) {
  std::string path = absl::StrCat(TestTempDir(), "crate_mapping");
  File::WriteStringToFileOrDie(text, path);
  return path;
}

std::string SkipLeadingWhitespace(absl::string_view text) {
  std::string result;
  for (absl::string_view line : absl::StrSplit(text, '\n', absl::SkipEmpty())) {
    absl::string_view stripped_line = absl::StripLeadingAsciiWhitespace(line);
    // Deal with old libc++ on OSS Protobuf CI workers
    result.append(stripped_line.data(), stripped_line.size());
    result.append("\n");
  }
  return result;
}

TEST(RustGenerator, GetImportPathToCrateNameMapEmpty) {
  std::string mapping_file_path = WriteStringToTempFile("");
  const Options opts = {Kernel::kUpb, mapping_file_path};
  absl::flat_hash_map<std::string, std::string> expected = {};
  auto result = GetImportPathToCrateNameMap(&opts);
  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), Eq(expected));
}

TEST(RustGenerator, GetImportPathToCrateNameMapSimple) {
  std::string mapping_file_path =
      WriteStringToTempFile(SkipLeadingWhitespace(R"mapping(
    crate_name
    3
    proto1.proto
    google.protobuf.proto
    proto3.proto
  )mapping"));
  const Options opts = {Kernel::kUpb, mapping_file_path};
  absl::flat_hash_map<std::string, std::string> expected = {
      {"proto1.proto", "crate_name"},
      {"google.protobuf.proto", "crate_name"},
      {"proto3.proto", "crate_name"},
  };
  auto result = GetImportPathToCrateNameMap(&opts);
  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), UnorderedElementsAreArray(expected));
}

TEST(RustGenerator, GetImportPathToCrateNameMapForEmptyMappingFile) {
  const Options opts = {Kernel::kUpb};
  auto result = GetImportPathToCrateNameMap(&opts);
  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), IsEmpty());
}

TEST(RustGenerator, GetImportPathToCrateNameMapErrorsOnMissingFile) {
  const Options opts = {Kernel::kUpb, "missing_file_path"};
  auto result = GetImportPathToCrateNameMap(&opts);
  EXPECT_FALSE(result.ok());
  EXPECT_THAT(result.status().code(), Eq(absl::StatusCode::kNotFound));
  EXPECT_THAT(result.status().message(), Eq("Could not open file"));
}

TEST(RustGenerator, GetImportPathToCrateNameMapInvalidFormat) {
  std::string mapping_file_path =
      WriteStringToTempFile("crate_name\nnot_a_number");
  const Options opts = {Kernel::kUpb, mapping_file_path};
  absl::flat_hash_map<std::string, std::string> expected = {};
  auto result = GetImportPathToCrateNameMap(&opts);
  EXPECT_FALSE(result.ok());
  EXPECT_THAT(result.status().code(), Eq(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(result.status().message(),
              Eq("Couldn't parse number of import paths in mapping file"));
}
}  // namespace
}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
