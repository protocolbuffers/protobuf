// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/importer.h"

#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/io/io_win32.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

namespace google {
namespace protobuf {
namespace compiler {

#ifdef _WIN32
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::access;
using google::protobuf::io::win32::open;
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#include "absl/strings/ascii.h"
#endif

// Returns true if the text looks like a Windows-style absolute path, starting
// with a drive letter.  Example:  "C:\foo".  TODO:  Share this with
// copy in command_line_interface.cc?
static bool IsWindowsAbsolutePath(absl::string_view text) {
#if defined(_WIN32) || defined(__CYGWIN__)
  return text.size() >= 3 && text[1] == ':' && absl::ascii_isalpha(text[0]) &&
         (text[2] == '/' || text[2] == '\\') && text.find_last_of(':') == 1;
#else
  return false;
#endif
}

MultiFileErrorCollector::~MultiFileErrorCollector() {}

// This class serves two purposes:
// - It implements the ErrorCollector interface (used by Tokenizer and Parser)
//   in terms of MultiFileErrorCollector, using a particular filename.
// - It lets us check if any errors have occurred.
class SourceTreeDescriptorDatabase::SingleFileErrorCollector
    : public io::ErrorCollector {
 public:
  SingleFileErrorCollector(const std::string& filename,
                           MultiFileErrorCollector* multi_file_error_collector)
      : filename_(filename),
        multi_file_error_collector_(multi_file_error_collector),
        had_errors_(false) {}
  ~SingleFileErrorCollector() override {}

  bool had_errors() { return had_errors_; }

  // implements ErrorCollector ---------------------------------------
  void RecordError(int line, int column, absl::string_view message) override {
    if (multi_file_error_collector_ != nullptr) {
      multi_file_error_collector_->RecordError(filename_, line, column,
                                               message);
    }
    had_errors_ = true;
  }

 private:
  std::string filename_;
  MultiFileErrorCollector* multi_file_error_collector_;
  bool had_errors_;
};

// ===================================================================

SourceTreeDescriptorDatabase::SourceTreeDescriptorDatabase(
    SourceTree* source_tree)
    : source_tree_(source_tree),
      fallback_database_(nullptr),
      error_collector_(nullptr),
      using_validation_error_collector_(false),
      validation_error_collector_(this) {}

SourceTreeDescriptorDatabase::SourceTreeDescriptorDatabase(
    SourceTree* source_tree, DescriptorDatabase* fallback_database)
    : source_tree_(source_tree),
      fallback_database_(fallback_database),
      error_collector_(nullptr),
      using_validation_error_collector_(false),
      validation_error_collector_(this) {}

SourceTreeDescriptorDatabase::~SourceTreeDescriptorDatabase() {}

bool SourceTreeDescriptorDatabase::FindFileByName(const std::string& filename,
                                                  FileDescriptorProto* output) {
  std::unique_ptr<io::ZeroCopyInputStream> input(source_tree_->Open(filename));
  if (input == nullptr) {
    if (fallback_database_ != nullptr &&
        fallback_database_->FindFileByName(filename, output)) {
      return true;
    }
    if (error_collector_ != nullptr) {
      error_collector_->RecordError(filename, -1, 0,
                                    source_tree_->GetLastErrorMessage());
    }
    return false;
  }

  // Set up the tokenizer and parser.
  SingleFileErrorCollector file_error_collector(filename, error_collector_);
  io::Tokenizer tokenizer(input.get(), &file_error_collector);

  Parser parser;
  if (error_collector_ != nullptr) {
    parser.RecordErrorsTo(&file_error_collector);
  }
  if (using_validation_error_collector_) {
    parser.RecordSourceLocationsTo(&source_locations_);
  }

  // Parse it.
  output->set_name(filename);
  return parser.Parse(&tokenizer, output) && !file_error_collector.had_errors();
}

bool SourceTreeDescriptorDatabase::FindFileContainingSymbol(
    const std::string& symbol_name, FileDescriptorProto* output) {
  return false;
}

bool SourceTreeDescriptorDatabase::FindFileContainingExtension(
    const std::string& containing_type, int field_number,
    FileDescriptorProto* output) {
  return false;
}

// -------------------------------------------------------------------

SourceTreeDescriptorDatabase::ValidationErrorCollector::
    ValidationErrorCollector(SourceTreeDescriptorDatabase* owner)
    : owner_(owner) {}

SourceTreeDescriptorDatabase::ValidationErrorCollector::
    ~ValidationErrorCollector() {}

void SourceTreeDescriptorDatabase::ValidationErrorCollector::RecordError(
    absl::string_view filename, absl::string_view element_name,
    const Message* descriptor, ErrorLocation location,
    absl::string_view message) {
  if (owner_->error_collector_ == nullptr) return;

  int line, column;
  if (location == DescriptorPool::ErrorCollector::IMPORT) {
    owner_->source_locations_.FindImport(descriptor, element_name, &line,
                                         &column);
  } else {
    owner_->source_locations_.Find(descriptor, location, &line, &column);
  }
  owner_->error_collector_->RecordError(filename, line, column, message);
}

void SourceTreeDescriptorDatabase::ValidationErrorCollector::RecordWarning(
    absl::string_view filename, absl::string_view element_name,
    const Message* descriptor, ErrorLocation location,
    absl::string_view message) {
  if (owner_->error_collector_ == nullptr) return;

  int line, column;
  if (location == DescriptorPool::ErrorCollector::IMPORT) {
    owner_->source_locations_.FindImport(descriptor, element_name, &line,
                                         &column);
  } else {
    owner_->source_locations_.Find(descriptor, location, &line, &column);
  }
  owner_->error_collector_->RecordWarning(filename, line, column, message);
}

// ===================================================================

Importer::Importer(SourceTree* source_tree,
                   MultiFileErrorCollector* error_collector)
    : database_(source_tree),
      pool_(&database_, database_.GetValidationErrorCollector()) {
  pool_.EnforceWeakDependencies(true);
  database_.RecordErrorsTo(error_collector);
}

Importer::~Importer() {}

const FileDescriptor* Importer::Import(const std::string& filename) {
  return pool_.FindFileByName(filename);
}

void Importer::AddUnusedImportTrackFile(const std::string& file_name,
                                        bool is_error) {
  pool_.AddUnusedImportTrackFile(file_name, is_error);
}

void Importer::ClearUnusedImportTrackFiles() {
  pool_.ClearUnusedImportTrackFiles();
}


// ===================================================================

SourceTree::~SourceTree() {}

std::string SourceTree::GetLastErrorMessage() { return "File not found."; }

DiskSourceTree::DiskSourceTree() {}

DiskSourceTree::~DiskSourceTree() {}

// Given a path, returns an equivalent path with these changes:
// - On Windows, any backslashes are replaced with forward slashes.
// - Any instances of the directory "." are removed.
// - Any consecutive '/'s are collapsed into a single slash.
// Note that the resulting string may be empty.
//
// TODO:  It would be nice to handle "..", e.g. so that we can figure
//   out that "foo/bar.proto" is inside "baz/../foo".  However, if baz is a
//   symlink or doesn't exist, then things get complicated, and we can't
//   actually determine this without investigating the filesystem, probably
//   in non-portable ways.  So, we punt.
//
// TODO:  It would be nice to use realpath() here except that it
//   resolves symbolic links.  This could cause problems if people place
//   symbolic links in their source tree.  For example, if you executed:
//     protoc --proto_path=foo foo/bar/baz.proto
//   then if foo/bar is a symbolic link, foo/bar/baz.proto will canonicalize
//   to a path which does not appear to be under foo, and thus the compiler
//   will complain that baz.proto is not inside the --proto_path.
static std::string CanonicalizePath(absl::string_view path) {
#ifdef _WIN32
  // The Win32 API accepts forward slashes as a path delimiter even though
  // backslashes are standard.  Let's avoid confusion and use only forward
  // slashes.
  std::string path_str;
  if (absl::StartsWith(path, "\\\\")) {
    // Avoid converting two leading backslashes.
    path_str = absl::StrCat("\\\\",
                            absl::StrReplaceAll(path.substr(2), {{"\\", "/"}}));
  } else {
    path_str = absl::StrReplaceAll(path, {{"\\", "/"}});
  }
  path = path_str;
#endif

  std::vector<absl::string_view> canonical_parts;
  if (!path.empty() && path.front() == '/') canonical_parts.push_back("");
  for (absl::string_view part : absl::StrSplit(path, '/', absl::SkipEmpty())) {
    if (part == ".") {
      // Ignore.
    } else {
      canonical_parts.push_back(part);
    }
  }
  if (!path.empty() && path.back() == '/') canonical_parts.push_back("");

  return absl::StrJoin(canonical_parts, "/");
}

static inline bool ContainsParentReference(absl::string_view path) {
  return path == ".." || absl::StartsWith(path, "../") ||
         absl::EndsWith(path, "/..") || absl::StrContains(path, "/../");
}

// Maps a file from an old location to a new one.  Typically, old_prefix is
// a virtual path and new_prefix is its corresponding disk path.  Returns
// false if the filename did not start with old_prefix, otherwise replaces
// old_prefix with new_prefix and stores the result in *result.  Examples:
//   string result;
//   assert(ApplyMapping("foo/bar", "", "baz", &result));
//   assert(result == "baz/foo/bar");
//
//   assert(ApplyMapping("foo/bar", "foo", "baz", &result));
//   assert(result == "baz/bar");
//
//   assert(ApplyMapping("foo", "foo", "bar", &result));
//   assert(result == "bar");
//
//   assert(!ApplyMapping("foo/bar", "baz", "qux", &result));
//   assert(!ApplyMapping("foo/bar", "baz", "qux", &result));
//   assert(!ApplyMapping("foobar", "foo", "baz", &result));
static bool ApplyMapping(absl::string_view filename,
                         absl::string_view old_prefix,
                         absl::string_view new_prefix, std::string* result) {
  if (old_prefix.empty()) {
    // old_prefix matches any relative path.
    if (ContainsParentReference(filename)) {
      // We do not allow the file name to use "..".
      return false;
    }
    if (absl::StartsWith(filename, "/") || IsWindowsAbsolutePath(filename)) {
      // This is an absolute path, so it isn't matched by the empty string.
      return false;
    }
    result->assign(std::string(new_prefix));
    if (!result->empty()) result->push_back('/');
    result->append(std::string(filename));
    return true;
  } else if (absl::StartsWith(filename, old_prefix)) {
    // old_prefix is a prefix of the filename.  Is it the whole filename?
    if (filename.size() == old_prefix.size()) {
      // Yep, it's an exact match.
      *result = std::string(new_prefix);
      return true;
    } else {
      // Not an exact match.  Is the next character a '/'?  Otherwise,
      // this isn't actually a match at all.  E.g. the prefix "foo/bar"
      // does not match the filename "foo/barbaz".
      int after_prefix_start = -1;
      if (filename[old_prefix.size()] == '/') {
        after_prefix_start = old_prefix.size() + 1;
      } else if (filename[old_prefix.size() - 1] == '/') {
        // old_prefix is never empty, and canonicalized paths never have
        // consecutive '/' characters.
        after_prefix_start = old_prefix.size();
      }
      if (after_prefix_start != -1) {
        // Yep.  So the prefixes are directories and the filename is a file
        // inside them.
        absl::string_view after_prefix = filename.substr(after_prefix_start);
        if (ContainsParentReference(after_prefix)) {
          // We do not allow the file name to use "..".
          return false;
        }
        result->assign(std::string(new_prefix));
        if (!result->empty()) result->push_back('/');
        result->append(std::string(after_prefix));
        return true;
      }
    }
  }

  return false;
}

void DiskSourceTree::MapPath(absl::string_view virtual_path,
                             absl::string_view disk_path) {
  mappings_.push_back(
      Mapping(std::string(virtual_path), CanonicalizePath(disk_path)));
}

DiskSourceTree::DiskFileToVirtualFileResult
DiskSourceTree::DiskFileToVirtualFile(absl::string_view disk_file,
                                      std::string* virtual_file,
                                      std::string* shadowing_disk_file) {
  int mapping_index = -1;
  std::string canonical_disk_file = CanonicalizePath(disk_file);

  for (int i = 0; i < mappings_.size(); i++) {
    // Apply the mapping in reverse.
    if (ApplyMapping(canonical_disk_file, mappings_[i].disk_path,
                     mappings_[i].virtual_path, virtual_file)) {
      // Success.
      mapping_index = i;
      break;
    }
  }

  if (mapping_index == -1) {
    return NO_MAPPING;
  }

  // Iterate through all mappings with higher precedence and verify that none
  // of them map this file to some other existing file.
  for (int i = 0; i < mapping_index; i++) {
    if (ApplyMapping(*virtual_file, mappings_[i].virtual_path,
                     mappings_[i].disk_path, shadowing_disk_file)) {
      if (access(shadowing_disk_file->c_str(), F_OK) >= 0) {
        // File exists.
        return SHADOWED;
      }
    }
  }
  shadowing_disk_file->clear();

  // Verify that we can open the file.  Note that this also has the side-effect
  // of verifying that we are not canonicalizing away any non-existent
  // directories.
  std::unique_ptr<io::ZeroCopyInputStream> stream(OpenDiskFile(disk_file));
  if (stream == nullptr) {
    return CANNOT_OPEN;
  }

  return SUCCESS;
}

bool DiskSourceTree::VirtualFileToDiskFile(absl::string_view virtual_file,
                                           std::string* disk_file) {
  std::unique_ptr<io::ZeroCopyInputStream> stream(
      OpenVirtualFile(virtual_file, disk_file));
  return stream != nullptr;
}

io::ZeroCopyInputStream* DiskSourceTree::Open(absl::string_view filename) {
  return OpenVirtualFile(filename, nullptr);
}

std::string DiskSourceTree::GetLastErrorMessage() {
  return last_error_message_;
}

io::ZeroCopyInputStream* DiskSourceTree::OpenVirtualFile(
    absl::string_view virtual_file, std::string* disk_file) {
  if (virtual_file != CanonicalizePath(virtual_file) ||
      ContainsParentReference(virtual_file)) {
    // We do not allow importing of paths containing things like ".." or
    // consecutive slashes since the compiler expects files to be uniquely
    // identified by file name.
    last_error_message_ =
        "Backslashes, consecutive slashes, \".\", or \"..\" "
        "are not allowed in the virtual path";
    return nullptr;
  }

  for (const auto& mapping : mappings_) {
    std::string temp_disk_file;
    if (ApplyMapping(virtual_file, mapping.virtual_path, mapping.disk_path,
                     &temp_disk_file)) {
      io::ZeroCopyInputStream* stream = OpenDiskFile(temp_disk_file);
      if (stream != nullptr) {
        if (disk_file != nullptr) {
          *disk_file = temp_disk_file;
        }
        return stream;
      }

      if (errno == EACCES) {
        // The file exists but is not readable.
        last_error_message_ =
            absl::StrCat("Read access is denied for file: ", temp_disk_file);
        return nullptr;
      }
    }
  }
  last_error_message_ = "File not found.";
  return nullptr;
}

io::ZeroCopyInputStream* DiskSourceTree::OpenDiskFile(
    absl::string_view filename) {
  struct stat sb;
  int ret = 0;
  do {
    ret = stat(std::string(filename).c_str(), &sb);
  } while (ret != 0 && errno == EINTR);
#if defined(_WIN32)
  if (ret == 0 && sb.st_mode & S_IFDIR) {
    last_error_message_ = "Input file is a directory.";
    return nullptr;
  }
#else
  if (ret == 0 && S_ISDIR(sb.st_mode)) {
    last_error_message_ = "Input file is a directory.";
    return nullptr;
  }
#endif
  int file_descriptor;
  do {
    file_descriptor = open(std::string(filename).c_str(), O_RDONLY);
  } while (file_descriptor < 0 && errno == EINTR);
  if (file_descriptor >= 0) {
    io::FileInputStream* result = new io::FileInputStream(file_descriptor);
    result->SetCloseOnDelete(true);
    return result;
  } else {
    return nullptr;
  }
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
