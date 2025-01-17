// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/command_line_interface.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif
#include <fcntl.h>
#include <sys/stat.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__FreeBSD__)
#include <sys/sysctl.h>
#endif

#include "absl/algorithm/container.h"
#include "absl/base/attributes.h"
#include "absl/base/log_severity.h"
#include "absl/container/btree_map.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/globals.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/importer.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/compiler/retention.h"
#include "google/protobuf/compiler/subprocess.h"
#include "google/protobuf/compiler/versions.h"
#include "google/protobuf/compiler/zip_writer.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/descriptor_visitor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/feature_resolver.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/text_format.h"


#ifdef _WIN32
#include "google/protobuf/io/io_win32.h"
#endif

#include "google/protobuf/stubs/platform_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0  // If this isn't defined, the platform doesn't need it.
#endif
#endif

namespace {
#if defined(_WIN32)
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::access;
using google::protobuf::io::win32::close;
using google::protobuf::io::win32::mkdir;
using google::protobuf::io::win32::open;
using google::protobuf::io::win32::setmode;
using google::protobuf::io::win32::write;
#endif

static const char* kDefaultDirectDependenciesViolationMsg =
    "File is imported but not declared in --direct_dependencies: %s";

// Returns true if the text looks like a Windows-style absolute path, starting
// with a drive letter.  Example:  "C:\foo".  TODO:  Share this with
// copy in importer.cc?
static bool IsWindowsAbsolutePath(const std::string& text) {
#if defined(_WIN32) || defined(__CYGWIN__)
  return text.size() >= 3 && text[1] == ':' && absl::ascii_isalpha(text[0]) &&
         (text[2] == '/' || text[2] == '\\') && text.find_last_of(':') == 1;
#else
  return false;
#endif
}

void SetFdToTextMode(int fd) {
#ifdef _WIN32
  if (setmode(fd, _O_TEXT) == -1) {
    // This should never happen, I think.
    ABSL_LOG(WARNING) << "setmode(" << fd << ", _O_TEXT): " << strerror(errno);
  }
#endif
  // (Text and binary are the same on non-Windows platforms.)
}

void SetFdToBinaryMode(int fd) {
#ifdef _WIN32
  if (setmode(fd, _O_BINARY) == -1) {
    // This should never happen, I think.
    ABSL_LOG(WARNING) << "setmode(" << fd
                      << ", _O_BINARY): " << strerror(errno);
  }
#endif
  // (Text and binary are the same on non-Windows platforms.)
}

void AddTrailingSlash(std::string* path) {
  if (!path->empty() && path->at(path->size() - 1) != '/') {
    path->push_back('/');
  }
}

bool VerifyDirectoryExists(const std::string& path) {
  if (path.empty()) return true;

  if (access(path.c_str(), F_OK) == -1) {
    std::cerr << path << ": " << strerror(errno) << std::endl;
    return false;
  } else {
    return true;
  }
}

// Try to create the parent directory of the given file, creating the parent's
// parent if necessary, and so on.  The full file name is actually
// (prefix + filename), but we assume |prefix| already exists and only create
// directories listed in |filename|.
bool TryCreateParentDirectory(const std::string& prefix,
                              const std::string& filename) {
  // Recursively create parent directories to the output file.
  // On Windows, both '/' and '\' are valid path separators.
  std::vector<std::string> parts =
      absl::StrSplit(filename, absl::ByAnyChar("/\\"), absl::SkipEmpty());
  std::string path_so_far = prefix;
  for (size_t i = 0; i < parts.size() - 1; ++i) {
    path_so_far += parts[i];
    if (mkdir(path_so_far.c_str(), 0777) != 0) {
      if (errno != EEXIST) {
        std::cerr << filename << ": while trying to create directory "
                  << path_so_far << ": " << strerror(errno) << std::endl;
        return false;
      }
    }
    path_so_far += '/';
  }

  return true;
}

// Get the absolute path of this protoc binary.
bool GetProtocAbsolutePath(std::string* path) {
#ifdef _WIN32
  char buffer[MAX_PATH];
  int len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
#elif defined(__APPLE__)
  char buffer[PATH_MAX];
  int len = 0;

  char dirtybuffer[PATH_MAX];
  uint32_t size = sizeof(dirtybuffer);
  if (_NSGetExecutablePath(dirtybuffer, &size) == 0) {
    realpath(dirtybuffer, buffer);
    len = strlen(buffer);
  }
#elif defined(__FreeBSD__)
  char buffer[PATH_MAX];
  size_t len = PATH_MAX;
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
  if (sysctl(mib, 4, &buffer, &len, nullptr, 0) != 0) {
    len = 0;
  }
#else
  char buffer[PATH_MAX];
  int len = readlink("/proc/self/exe", buffer, PATH_MAX);
#endif
  if (len > 0) {
    path->assign(buffer, len);
    return true;
  } else {
    return false;
  }
}

// Whether a path is where google/protobuf/descriptor.proto and other well-known
// type protos are installed.
bool IsInstalledProtoPath(absl::string_view path) {
  // Checking the descriptor.proto file should be good enough.
  std::string file_path =
      absl::StrCat(path, "/google/protobuf/descriptor.proto");
  return access(file_path.c_str(), F_OK) != -1;
}

// Add the paths where google/protobuf/descriptor.proto and other well-known
// type protos are installed.
void AddDefaultProtoPaths(
    std::vector<std::pair<std::string, std::string>>* paths) {
  // TODO: The code currently only checks relative paths of where
  // the protoc binary is installed. We probably should make it handle more
  // cases than that.
  std::string path_str;
  if (!GetProtocAbsolutePath(&path_str)) {
    return;
  }
  absl::string_view path(path_str);
  // Strip the binary name.
  size_t pos = path.find_last_of("/\\");
  if (pos == path.npos || pos == 0) {
    return;
  }
  path = path.substr(0, pos);
  // Check the binary's directory.
  if (IsInstalledProtoPath(path)) {
    paths->emplace_back("", path);
    return;
  }
  // Check if there is an include subdirectory.
  std::string include_path = absl::StrCat(path, "/include");
  if (IsInstalledProtoPath(include_path)) {
    paths->emplace_back("", std::move(include_path));
    return;
  }
  // Check if the upper level directory has an "include" subdirectory.
  pos = path.find_last_of("/\\");
  if (pos == std::string::npos || pos == 0) {
    return;
  }
  path = path.substr(0, pos);
  include_path = absl::StrCat(path, "/include");
  if (IsInstalledProtoPath(include_path)) {
    paths->emplace_back("", std::move(include_path));
    return;
  }
}

std::string PluginName(absl::string_view plugin_prefix,
                       absl::string_view directive) {
  // Assuming the directive starts with "--" and ends with "_out" or "_opt",
  // strip the "--" and "_out/_opt" and add the plugin prefix.
  return absl::StrCat(plugin_prefix, "gen-",
                      directive.substr(2, directive.size() - 6));
}

bool GetBootstrapParam(const std::string& parameter) {
  std::vector<std::string> parts = absl::StrSplit(parameter, ',');
  for (const auto& part : parts) {
    if (part == "bootstrap") {
      return true;
    }
  }
  return false;
}


}  // namespace

void CommandLineInterface::GetTransitiveDependencies(
    const FileDescriptor* file,
    absl::flat_hash_set<const FileDescriptor*>* already_seen,
    RepeatedPtrField<FileDescriptorProto>* output,
    const TransitiveDependencyOptions& options) {
  if (!already_seen->insert(file).second) {
    // Already saw this file.  Skip.
    return;
  }

  // Add all dependencies.
  for (int i = 0; i < file->dependency_count(); ++i) {
    GetTransitiveDependencies(file->dependency(i), already_seen, output,
                              options);
  }

  // Add this file.
  FileDescriptorProto* new_descriptor = output->Add();
  file->CopyTo(new_descriptor);
  if (options.include_source_code_info) {
    file->CopySourceCodeInfoTo(new_descriptor);
  }
  if (!options.retain_options) {
    StripSourceRetentionOptions(*file->pool(), *new_descriptor);
  }
  if (options.include_json_name) {
    file->CopyJsonNameTo(new_descriptor);
  }
}

// A MultiFileErrorCollector that prints errors to stderr.
class CommandLineInterface::ErrorPrinter
    : public MultiFileErrorCollector,
      public io::ErrorCollector,
      public DescriptorPool::ErrorCollector {
 public:
  explicit ErrorPrinter(ErrorFormat format, DiskSourceTree* tree = nullptr)
      : format_(format),
        tree_(tree),
        found_errors_(false),
        found_warnings_(false) {}
  ~ErrorPrinter() override = default;

  // implements MultiFileErrorCollector ------------------------------
  void RecordError(absl::string_view filename, int line, int column,
                   absl::string_view message) override {
    found_errors_ = true;
    AddErrorOrWarning(filename, line, column, message, "error", std::cerr);
  }

  void RecordWarning(absl::string_view filename, int line, int column,
                     absl::string_view message) override {
    found_warnings_ = true;
    AddErrorOrWarning(filename, line, column, message, "warning", std::clog);
  }

  // implements io::ErrorCollector -----------------------------------
  void RecordError(int line, int column, absl::string_view message) override {
    RecordError("input", line, column, message);
  }

  void RecordWarning(int line, int column, absl::string_view message) override {
    AddErrorOrWarning("input", line, column, message, "warning", std::clog);
  }

  // implements DescriptorPool::ErrorCollector-------------------------
  void RecordError(absl::string_view filename, absl::string_view element_name,
                   const Message* descriptor, ErrorLocation location,
                   absl::string_view message) override {
    AddErrorOrWarning(filename, -1, -1, message, "error", std::cerr);
  }

  void RecordWarning(absl::string_view filename, absl::string_view element_name,
                     const Message* descriptor, ErrorLocation location,
                     absl::string_view message) override {
    AddErrorOrWarning(filename, -1, -1, message, "warning", std::clog);
  }

  bool FoundErrors() const { return found_errors_; }

  bool FoundWarnings() const { return found_warnings_; }

 private:
  void AddErrorOrWarning(absl::string_view filename, int line, int column,
                         absl::string_view message, absl::string_view type,
                         std::ostream& out) {
    std::string dfile;
    if (
        tree_ != nullptr && tree_->VirtualFileToDiskFile(filename, &dfile)) {
      out << dfile;
    } else {
      out << filename;
    }

    // Users typically expect 1-based line/column numbers, so we add 1
    // to each here.
    if (line != -1) {
      // Allow for both GCC- and Visual-Studio-compatible output.
      switch (format_) {
        case CommandLineInterface::ERROR_FORMAT_GCC:
          out << ":" << (line + 1) << ":" << (column + 1);
          break;
        case CommandLineInterface::ERROR_FORMAT_MSVS:
          out << "(" << (line + 1) << ") : " << type
              << " in column=" << (column + 1);
          break;
      }
    }

    if (type == "warning") {
      out << ": warning: " << message << std::endl;
    } else {
      out << ": " << message << std::endl;
    }
  }

  const ErrorFormat format_;
  DiskSourceTree* tree_;
  bool found_errors_;
  bool found_warnings_;
};

// -------------------------------------------------------------------

// A GeneratorContext implementation that buffers files in memory, then dumps
// them all to disk on demand.
class CommandLineInterface::GeneratorContextImpl : public GeneratorContext {
 public:
  explicit GeneratorContextImpl(
      const std::vector<const FileDescriptor*>& parsed_files);

  // Write all files in the directory to disk at the given output location,
  // which must end in a '/'.
  bool WriteAllToDisk(const std::string& prefix);

  // Write the contents of this directory to a ZIP-format archive with the
  // given name.
  bool WriteAllToZip(const std::string& filename);

  // Add a boilerplate META-INF/MANIFEST.MF file as required by the Java JAR
  // format, unless one has already been written.
  void AddJarManifest();

  // Get name of all output files.
  void GetOutputFilenames(std::vector<std::string>* output_filenames);
  // implements GeneratorContext --------------------------------------
  io::ZeroCopyOutputStream* Open(const std::string& filename) override;
  io::ZeroCopyOutputStream* OpenForAppend(const std::string& filename) override;
  io::ZeroCopyOutputStream* OpenForInsert(
      const std::string& filename, const std::string& insertion_point) override;
  io::ZeroCopyOutputStream* OpenForInsertWithGeneratedCodeInfo(
      const std::string& filename, const std::string& insertion_point,
      const google::protobuf::GeneratedCodeInfo& info) override;
  void ListParsedFiles(std::vector<const FileDescriptor*>* output) override {
    *output = parsed_files_;
  }

 private:
  friend class MemoryOutputStream;

  // The files_ field maps from path keys to file content values. It's a map
  // instead of an unordered_map so that files are written in order (good when
  // writing zips).
  absl::btree_map<std::string, std::string> files_;
  const std::vector<const FileDescriptor*>& parsed_files_;
  bool had_error_;
};

class CommandLineInterface::MemoryOutputStream
    : public io::ZeroCopyOutputStream {
 public:
  MemoryOutputStream(GeneratorContextImpl* directory,
                     const std::string& filename, bool append_mode);
  MemoryOutputStream(GeneratorContextImpl* directory,
                     const std::string& filename,
                     const std::string& insertion_point);
  MemoryOutputStream(GeneratorContextImpl* directory,
                     const std::string& filename,
                     const std::string& insertion_point,
                     const google::protobuf::GeneratedCodeInfo& info);
  ~MemoryOutputStream() override;

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size) override {
    return inner_->Next(data, size);
  }
  void BackUp(int count) override { inner_->BackUp(count); }
  int64_t ByteCount() const override { return inner_->ByteCount(); }

 private:
  // Checks to see if "filename_.pb.meta" exists in directory_; if so, fixes the
  // offsets in that GeneratedCodeInfo record to reflect bytes inserted in
  // filename_ at original offset insertion_offset with length insertion_length.
  // Also adds in the data from info_to_insert_ with updated offsets governed by
  // insertion_offset and indent_length. We assume that insertions will not
  // occur within any given annotated span of text. insertion_content must end
  // with an endline.
  void UpdateMetadata(const std::string& insertion_content,
                      size_t insertion_offset, size_t insertion_length,
                      size_t indent_length);

  // Inserts info_to_insert_ into target_info, assuming that the relevant
  // insertion was made at insertion_offset in file_content with the given
  // indent_length. insertion_content must end with an endline.
  void InsertShiftedInfo(const std::string& insertion_content,
                         size_t insertion_offset, size_t indent_length,
                         google::protobuf::GeneratedCodeInfo& target_info);

  // Where to insert the string when it's done.
  GeneratorContextImpl* directory_;
  std::string filename_;
  std::string insertion_point_;

  // The string we're building.
  std::string data_;

  // Whether we should append the output stream to the existing file.
  bool append_mode_;

  // StringOutputStream writing to data_.
  std::unique_ptr<io::StringOutputStream> inner_;

  // The GeneratedCodeInfo to insert at the insertion point.
  google::protobuf::GeneratedCodeInfo info_to_insert_;
};

// -------------------------------------------------------------------

CommandLineInterface::GeneratorContextImpl::GeneratorContextImpl(
    const std::vector<const FileDescriptor*>& parsed_files)
    : parsed_files_(parsed_files), had_error_(false) {}

bool CommandLineInterface::GeneratorContextImpl::WriteAllToDisk(
    const std::string& prefix) {
  if (had_error_) {
    return false;
  }

  if (!VerifyDirectoryExists(prefix)) {
    return false;
  }

  for (const auto& pair : files_) {
    const std::string& relative_filename = pair.first;
    const char* data = pair.second.data();
    int size = pair.second.size();

    if (!TryCreateParentDirectory(prefix, relative_filename)) {
      return false;
    }
    std::string filename = prefix + relative_filename;

    // Create the output file.
    int file_descriptor;
    do {
      file_descriptor =
          open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
    } while (file_descriptor < 0 && errno == EINTR);

    if (file_descriptor < 0) {
      int error = errno;
      std::cerr << filename << ": " << strerror(error);
      return false;
    }

    // Write the file.
    while (size > 0) {
      int write_result;
      do {
        write_result = write(file_descriptor, data, size);
      } while (write_result < 0 && errno == EINTR);

      if (write_result <= 0) {
        // Write error.

        // FIXME(kenton):  According to the man page, if write() returns zero,
        //   there was no error; write() simply did not write anything.  It's
        //   unclear under what circumstances this might happen, but presumably
        //   errno won't be set in this case.  I am confused as to how such an
        //   event should be handled.  For now I'm treating it as an error,
        //   since retrying seems like it could lead to an infinite loop.  I
        //   suspect this never actually happens anyway.

        if (write_result < 0) {
          int error = errno;
          std::cerr << filename << ": write: " << strerror(error);
        } else {
          std::cerr << filename << ": write() returned zero?" << std::endl;
        }
        return false;
      }

      data += write_result;
      size -= write_result;
    }

    if (close(file_descriptor) != 0) {
      int error = errno;
      std::cerr << filename << ": close: " << strerror(error);
      return false;
    }
  }

  return true;
}

bool CommandLineInterface::GeneratorContextImpl::WriteAllToZip(
    const std::string& filename) {
  if (had_error_) {
    return false;
  }

  // Create the output file.
  int file_descriptor;
  do {
    file_descriptor =
        open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  } while (file_descriptor < 0 && errno == EINTR);

  if (file_descriptor < 0) {
    int error = errno;
    std::cerr << filename << ": " << strerror(error);
    return false;
  }

  // Create the ZipWriter
  io::FileOutputStream stream(file_descriptor);
  ZipWriter zip_writer(&stream);

  for (const auto& pair : files_) {
    zip_writer.Write(pair.first, pair.second);
  }

  zip_writer.WriteDirectory();

  if (stream.GetErrno() != 0) {
    std::cerr << filename << ": " << strerror(stream.GetErrno()) << std::endl;
    return false;
  }

  if (!stream.Close()) {
    std::cerr << filename << ": " << strerror(stream.GetErrno()) << std::endl;
    return false;
  }

  return true;
}

void CommandLineInterface::GeneratorContextImpl::AddJarManifest() {
  auto pair = files_.insert({"META-INF/MANIFEST.MF", ""});
  if (pair.second) {
    pair.first->second =
        "Manifest-Version: 1.0\n"
        "Created-By: 1.6.0 (protoc)\n"
        "\n";
  }
}

void CommandLineInterface::GeneratorContextImpl::GetOutputFilenames(
    std::vector<std::string>* output_filenames) {
  for (const auto& pair : files_) {
    output_filenames->push_back(pair.first);
  }
}

io::ZeroCopyOutputStream* CommandLineInterface::GeneratorContextImpl::Open(
    const std::string& filename) {
  return new MemoryOutputStream(this, filename, false);
}

io::ZeroCopyOutputStream*
CommandLineInterface::GeneratorContextImpl::OpenForAppend(
    const std::string& filename) {
  return new MemoryOutputStream(this, filename, true);
}

io::ZeroCopyOutputStream*
CommandLineInterface::GeneratorContextImpl::OpenForInsert(
    const std::string& filename, const std::string& insertion_point) {
  return new MemoryOutputStream(this, filename, insertion_point);
}

io::ZeroCopyOutputStream*
CommandLineInterface::GeneratorContextImpl::OpenForInsertWithGeneratedCodeInfo(
    const std::string& filename, const std::string& insertion_point,
    const google::protobuf::GeneratedCodeInfo& info) {
  return new MemoryOutputStream(this, filename, insertion_point, info);
}

// -------------------------------------------------------------------

CommandLineInterface::MemoryOutputStream::MemoryOutputStream(
    GeneratorContextImpl* directory, const std::string& filename,
    bool append_mode)
    : directory_(directory),
      filename_(filename),
      append_mode_(append_mode),
      inner_(new io::StringOutputStream(&data_)) {}

CommandLineInterface::MemoryOutputStream::MemoryOutputStream(
    GeneratorContextImpl* directory, const std::string& filename,
    const std::string& insertion_point)
    : directory_(directory),
      filename_(filename),
      insertion_point_(insertion_point),
      inner_(new io::StringOutputStream(&data_)) {}

CommandLineInterface::MemoryOutputStream::MemoryOutputStream(
    GeneratorContextImpl* directory, const std::string& filename,
    const std::string& insertion_point, const google::protobuf::GeneratedCodeInfo& info)
    : directory_(directory),
      filename_(filename),
      insertion_point_(insertion_point),
      inner_(new io::StringOutputStream(&data_)),
      info_to_insert_(info) {}

void CommandLineInterface::MemoryOutputStream::InsertShiftedInfo(
    const std::string& insertion_content, size_t insertion_offset,
    size_t indent_length, google::protobuf::GeneratedCodeInfo& target_info) {
  // Keep track of how much extra data was added for indents before the
  // current annotation being inserted. `pos` and `source_annotation.begin()`
  // are offsets in `insertion_content`. `insertion_offset` is updated so that
  // it can be added to an annotation's `begin` field to reflect that
  // annotation's updated location after `insertion_content` was inserted into
  // the target file.
  size_t pos = 0;
  insertion_offset += indent_length;
  for (const auto& source_annotation : info_to_insert_.annotation()) {
    GeneratedCodeInfo::Annotation* annotation = target_info.add_annotation();
    int inner_indent = 0;
    // insertion_content is guaranteed to end in an endline. This last endline
    // has no effect on indentation.
    for (; pos < static_cast<size_t>(source_annotation.end()) &&
           pos < insertion_content.size() - 1;
         ++pos) {
      if (insertion_content[pos] == '\n') {
        if (pos >= static_cast<size_t>(source_annotation.begin())) {
          // The beginning of the annotation is at insertion_offset, but the end
          // can still move further in the target file.
          inner_indent += indent_length;
        } else {
          insertion_offset += indent_length;
        }
      }
    }
    *annotation = source_annotation;
    annotation->set_begin(annotation->begin() + insertion_offset);
    insertion_offset += inner_indent;
    annotation->set_end(annotation->end() + insertion_offset);
  }
}

void CommandLineInterface::MemoryOutputStream::UpdateMetadata(
    const std::string& insertion_content, size_t insertion_offset,
    size_t insertion_length, size_t indent_length) {
  auto it = directory_->files_.find(absl::StrCat(filename_, ".pb.meta"));
  if (it == directory_->files_.end() && info_to_insert_.annotation().empty()) {
    // No metadata was recorded for this file.
    return;
  }
  GeneratedCodeInfo metadata;
  bool is_text_format = false;
  std::string* encoded_data = nullptr;
  if (it != directory_->files_.end()) {
    encoded_data = &it->second;
    // Try to decode a GeneratedCodeInfo proto from the .pb.meta file. It may be
    // in wire or text format. Keep the same format when the data is written out
    // later.
    if (!metadata.ParseFromString(*encoded_data)) {
      if (!TextFormat::ParseFromString(*encoded_data, &metadata)) {
        // The metadata is invalid.
        std::cerr
            << filename_
            << ".pb.meta: Could not parse metadata as wire or text format."
            << std::endl;
        return;
      }
      // Generators that use the public plugin interface emit text-format
      // metadata (because in the public plugin protocol, file content must be
      // UTF8-encoded strings).
      is_text_format = true;
    }
  } else {
    // Create a new file to store the new metadata in info_to_insert_.
    encoded_data =
        &directory_->files_.try_emplace(absl::StrCat(filename_, ".pb.meta"), "")
             .first->second;
  }
  GeneratedCodeInfo new_metadata;
  bool crossed_offset = false;
  size_t to_add = 0;
  for (const auto& source_annotation : metadata.annotation()) {
    // The first time an annotation at or after the insertion point is found,
    // insert the new metadata from info_to_insert_. Shift all annotations
    // after the new metadata by the length of the text that was inserted
    // (including any additional indent length).
    if (static_cast<size_t>(source_annotation.begin()) >= insertion_offset &&
        !crossed_offset) {
      crossed_offset = true;
      InsertShiftedInfo(insertion_content, insertion_offset, indent_length,
                        new_metadata);
      to_add += insertion_length;
    }
    GeneratedCodeInfo::Annotation* annotation = new_metadata.add_annotation();
    *annotation = source_annotation;
    annotation->set_begin(annotation->begin() + to_add);
    annotation->set_end(annotation->end() + to_add);
  }
  // If there were never any annotations at or after the insertion point,
  // make sure to still insert the new metadata from info_to_insert_.
  if (!crossed_offset) {
    InsertShiftedInfo(insertion_content, insertion_offset, indent_length,
                      new_metadata);
  }
  if (is_text_format) {
    TextFormat::PrintToString(new_metadata, encoded_data);
  } else {
    new_metadata.SerializeToString(encoded_data);
  }
}

CommandLineInterface::MemoryOutputStream::~MemoryOutputStream() {
  // Make sure all data has been written.
  inner_.reset();

  // Insert into the directory.
  auto pair = directory_->files_.insert({filename_, ""});
  auto it = pair.first;
  bool already_present = !pair.second;

  if (insertion_point_.empty()) {
    // This was just a regular Open().
    if (already_present) {
      if (append_mode_) {
        it->second.append(data_);
      } else {
        std::cerr << filename_ << ": Tried to write the same file twice."
                  << std::endl;
        directory_->had_error_ = true;
      }
      return;
    }

    it->second.swap(data_);
    return;
  }
  // This was an OpenForInsert().

  // If the data doesn't end with a clean line break, add one.
  if (!data_.empty() && data_[data_.size() - 1] != '\n') {
    data_.push_back('\n');
  }

  // Find the file we are going to insert into.
  if (!already_present) {
    std::cerr << filename_ << ": Tried to insert into file that doesn't exist."
              << std::endl;
    directory_->had_error_ = true;
    return;
  }
  std::string* target = &it->second;

  // Find the insertion point.
  std::string magic_string =
      absl::Substitute("@@protoc_insertion_point($0)", insertion_point_);
  std::string::size_type pos = target->find(magic_string);

  if (pos == std::string::npos) {
    std::cerr << filename_ << ": insertion point \"" << insertion_point_
              << "\" not found." << std::endl;
    directory_->had_error_ = true;
    return;
  }

  if ((pos > 3) && (target->substr(pos - 3, 2) == "/*")) {
    // Support for inline "/* @@protoc_insertion_point() */"
    pos = pos - 3;
  } else {
    // Seek backwards to the beginning of the line, which is where we will
    // insert the data.  Note that this has the effect of pushing the
    // insertion point down, so the data is inserted before it.  This is
    // intentional because it means that multiple insertions at the same point
    // will end up in the expected order in the final output.
    pos = target->find_last_of('\n', pos);
    if (pos == std::string::npos) {
      // Insertion point is on the first line.
      pos = 0;
    } else {
      // Advance to character after '\n'.
      ++pos;
    }
  }

  // Extract indent.
  std::string indent_(*target, pos,
                      target->find_first_not_of(" \t", pos) - pos);

  if (indent_.empty()) {
    // No indent.  This makes things easier.
    target->insert(pos, data_);
    UpdateMetadata(data_, pos, data_.size(), 0);
    return;
  }
  // Calculate how much space we need.
  int indent_size = 0;
  for (size_t i = 0; i < data_.size(); ++i) {
    if (data_[i] == '\n') indent_size += indent_.size();
  }

  // Make a hole for it.
  target->insert(pos, data_.size() + indent_size, '\0');

  // Now copy in the data.
  std::string::size_type data_pos = 0;
  char* target_ptr = &(*target)[pos];
  while (data_pos < data_.size()) {
    // Copy indent.
    memcpy(target_ptr, indent_.data(), indent_.size());
    target_ptr += indent_.size();

    // Copy line from data_.
    // We already guaranteed that data_ ends with a newline (above), so this
    // search can't fail.
    std::string::size_type line_length =
        data_.find_first_of('\n', data_pos) + 1 - data_pos;
    memcpy(target_ptr, data_.data() + data_pos, line_length);
    target_ptr += line_length;
    data_pos += line_length;
  }

  ABSL_CHECK_EQ(target_ptr, &(*target)[pos] + data_.size() + indent_size);

  UpdateMetadata(data_, pos, data_.size() + indent_size, indent_.size());
}

// ===================================================================

#if defined(_WIN32) && !defined(__CYGWIN__)
const char* const CommandLineInterface::kPathSeparator = ";";
#else
const char* const CommandLineInterface::kPathSeparator = ":";
#endif

CommandLineInterface::CommandLineInterface()
    : direct_dependencies_violation_msg_(
          kDefaultDirectDependenciesViolationMsg) {}

CommandLineInterface::~CommandLineInterface() = default;

void CommandLineInterface::RegisterGenerator(const std::string& flag_name,
                                             CodeGenerator* generator,
                                             const std::string& help_text) {
  GeneratorInfo info;
  info.flag_name = flag_name;
  info.generator = generator;
  info.help_text = help_text;
  generators_by_flag_name_[flag_name] = info;
}

void CommandLineInterface::RegisterGenerator(
    const std::string& flag_name, const std::string& option_flag_name,
    CodeGenerator* generator, const std::string& help_text) {
  GeneratorInfo info;
  info.flag_name = flag_name;
  info.option_flag_name = option_flag_name;
  info.generator = generator;
  info.help_text = help_text;
  generators_by_flag_name_[flag_name] = info;
  generators_by_option_name_[option_flag_name] = info;
}

void CommandLineInterface::AllowPlugins(const std::string& exe_name_prefix) {
  plugin_prefix_ = exe_name_prefix;
}

namespace {

bool ContainsProto3Optional(const Descriptor* desc) {
  for (int i = 0; i < desc->field_count(); ++i) {
    if (desc->field(i)->real_containing_oneof() == nullptr &&
        desc->field(i)->containing_oneof() != nullptr) {
      return true;
    }
  }
  for (int i = 0; i < desc->nested_type_count(); ++i) {
    if (ContainsProto3Optional(desc->nested_type(i))) {
      return true;
    }
  }
  return false;
}

bool ContainsProto3Optional(Edition edition, const FileDescriptor* file) {
  if (edition == Edition::EDITION_PROTO3) {
    for (int i = 0; i < file->message_type_count(); ++i) {
      if (ContainsProto3Optional(file->message_type(i))) {
        return true;
      }
    }
  }
  return false;
}

bool HasReservedFieldNumber(const FieldDescriptor* field) {
  if (field->number() >= FieldDescriptor::kFirstReservedNumber &&
      field->number() <= FieldDescriptor::kLastReservedNumber) {
    return true;
  }
  return false;
}

}  // namespace

namespace {
std::unique_ptr<SimpleDescriptorDatabase>
PopulateSingleSimpleDescriptorDatabase(const std::string& descriptor_set_name);

// Indicates whether the field is compatible with the given target type.
bool IsFieldCompatible(const FieldDescriptor& field,
                       FieldOptions::OptionTargetType target_type) {
  const RepeatedField<int>& allowed_targets = field.options().targets();
  return allowed_targets.empty() ||
         absl::c_linear_search(allowed_targets, target_type);
}

// Converts the OptionTargetType enum to a string suitable for use in error
// messages.
absl::string_view TargetTypeString(FieldOptions::OptionTargetType target_type) {
  switch (target_type) {
    case FieldOptions::TARGET_TYPE_FILE:
      return "file";
    case FieldOptions::TARGET_TYPE_EXTENSION_RANGE:
      return "extension range";
    case FieldOptions::TARGET_TYPE_MESSAGE:
      return "message";
    case FieldOptions::TARGET_TYPE_FIELD:
      return "field";
    case FieldOptions::TARGET_TYPE_ONEOF:
      return "oneof";
    case FieldOptions::TARGET_TYPE_ENUM:
      return "enum";
    case FieldOptions::TARGET_TYPE_ENUM_ENTRY:
      return "enum entry";
    case FieldOptions::TARGET_TYPE_SERVICE:
      return "service";
    case FieldOptions::TARGET_TYPE_METHOD:
      return "method";
    default:
      return "unknown";
  }
}

// Recursively validates that the options message (or subpiece of an options
// message) is compatible with the given target type.
bool ValidateTargetConstraintsRecursive(
    const Message& m, DescriptorPool::ErrorCollector& error_collector,
    absl::string_view file_name, FieldOptions::OptionTargetType target_type) {
  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = m.GetReflection();
  reflection->ListFields(m, &fields);
  bool success = true;
  for (const auto* field : fields) {
    if (!IsFieldCompatible(*field, target_type)) {
      success = false;
      error_collector.RecordError(
          file_name, "", nullptr, DescriptorPool::ErrorCollector::OPTION_NAME,
          absl::StrCat("Option ", field->full_name(),
                       " cannot be set on an entity of type `",
                       TargetTypeString(target_type), "`."));
    }
    if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
      if (field->is_repeated()) {
        int field_size = reflection->FieldSize(m, field);
        for (int i = 0; i < field_size; ++i) {
          if (!ValidateTargetConstraintsRecursive(
                  reflection->GetRepeatedMessage(m, field, i), error_collector,
                  file_name, target_type)) {
            success = false;
          }
        }
      } else if (!ValidateTargetConstraintsRecursive(
                     reflection->GetMessage(m, field), error_collector,
                     file_name, target_type)) {
        success = false;
      }
    }
  }
  return success;
}

// Validates that the options message is correct with respect to target
// constraints, returning true if successful. This function converts the
// options message to a DynamicMessage so that we have visibility into custom
// options. We take the element name as a FunctionRef so that we do not have to
// pay the cost of constructing it unless there is an error.
bool ValidateTargetConstraints(const Message& options,
                               const DescriptorPool& pool,
                               DescriptorPool::ErrorCollector& error_collector,
                               absl::string_view file_name,
                               FieldOptions::OptionTargetType target_type) {
  const Descriptor* descriptor =
      pool.FindMessageTypeByName(options.GetTypeName());
  if (descriptor == nullptr) {
    // We were unable to find the options message in the descriptor pool. This
    // implies that the proto files we are working with do not depend on
    // descriptor.proto, in which case there are no custom options to worry
    // about. We can therefore skip the use of DynamicMessage.
    return ValidateTargetConstraintsRecursive(options, error_collector,
                                              file_name, target_type);
  } else {
    DynamicMessageFactory factory;
    std::unique_ptr<Message> dynamic_message(
        factory.GetPrototype(descriptor)->New());
    std::string serialized;
    ABSL_CHECK(options.SerializeToString(&serialized));
    ABSL_CHECK(dynamic_message->ParseFromString(serialized));
    return ValidateTargetConstraintsRecursive(*dynamic_message, error_collector,
                                              file_name, target_type);
  }
}

// The overloaded GetTargetType() functions below allow us to map from a
// descriptor type to the associated OptionTargetType enum.
FieldOptions::OptionTargetType GetTargetType(const FileDescriptor*) {
  return FieldOptions::TARGET_TYPE_FILE;
}

FieldOptions::OptionTargetType GetTargetType(
    const Descriptor::ExtensionRange*) {
  return FieldOptions::TARGET_TYPE_EXTENSION_RANGE;
}

FieldOptions::OptionTargetType GetTargetType(const Descriptor*) {
  return FieldOptions::TARGET_TYPE_MESSAGE;
}

FieldOptions::OptionTargetType GetTargetType(const FieldDescriptor*) {
  return FieldOptions::TARGET_TYPE_FIELD;
}

FieldOptions::OptionTargetType GetTargetType(const OneofDescriptor*) {
  return FieldOptions::TARGET_TYPE_ONEOF;
}

FieldOptions::OptionTargetType GetTargetType(const EnumDescriptor*) {
  return FieldOptions::TARGET_TYPE_ENUM;
}

FieldOptions::OptionTargetType GetTargetType(const EnumValueDescriptor*) {
  return FieldOptions::TARGET_TYPE_ENUM_ENTRY;
}

FieldOptions::OptionTargetType GetTargetType(const ServiceDescriptor*) {
  return FieldOptions::TARGET_TYPE_SERVICE;
}

FieldOptions::OptionTargetType GetTargetType(const MethodDescriptor*) {
  return FieldOptions::TARGET_TYPE_METHOD;
}
}  // namespace

int CommandLineInterface::Run(int argc, const char* const argv[]) {
  Clear();

  switch (ParseArguments(argc, argv)) {
    case PARSE_ARGUMENT_DONE_AND_EXIT:
      return 0;
    case PARSE_ARGUMENT_FAIL:
      return 1;
    case PARSE_ARGUMENT_DONE_AND_CONTINUE:
      break;
  }

  std::vector<const FileDescriptor*> parsed_files;
  std::unique_ptr<DiskSourceTree> disk_source_tree;
  std::unique_ptr<ErrorPrinter> error_collector;
  std::unique_ptr<DescriptorPool> descriptor_pool;

  // The SimpleDescriptorDatabases here are the constituents of the
  // MergedDescriptorDatabase descriptor_set_in_database, so this vector is for
  // managing their lifetimes. Its scope should match descriptor_set_in_database
  std::vector<std::unique_ptr<SimpleDescriptorDatabase>>
      databases_per_descriptor_set;
  std::unique_ptr<MergedDescriptorDatabase> descriptor_set_in_database;

  std::unique_ptr<SourceTreeDescriptorDatabase> source_tree_database;

  // Any --descriptor_set_in FileDescriptorSet objects will be used as a
  // fallback to input_files on command line, so create that db first.
  if (!descriptor_set_in_names_.empty()) {
    for (const std::string& name : descriptor_set_in_names_) {
      std::unique_ptr<SimpleDescriptorDatabase> database_for_descriptor_set =
          PopulateSingleSimpleDescriptorDatabase(name);
      if (!database_for_descriptor_set) {
        return EXIT_FAILURE;
      }
      databases_per_descriptor_set.push_back(
          std::move(database_for_descriptor_set));
    }

    std::vector<DescriptorDatabase*> raw_databases_per_descriptor_set;
    raw_databases_per_descriptor_set.reserve(
        databases_per_descriptor_set.size());
    for (const std::unique_ptr<SimpleDescriptorDatabase>& db :
         databases_per_descriptor_set) {
      raw_databases_per_descriptor_set.push_back(db.get());
    }
    descriptor_set_in_database = std::make_unique<MergedDescriptorDatabase>(
        raw_databases_per_descriptor_set);
  }

  if (proto_path_.empty()) {
    // If there are no --proto_path flags, then just look in the specified
    // --descriptor_set_in files.  But first, verify that the input files are
    // there.
    if (!VerifyInputFilesInDescriptors(descriptor_set_in_database.get())) {
      return 1;
    }

    error_collector = std::make_unique<ErrorPrinter>(error_format_);
    descriptor_pool = std::make_unique<DescriptorPool>(
        descriptor_set_in_database.get(), error_collector.get());
  } else {
    disk_source_tree = std::make_unique<DiskSourceTree>();
    if (!InitializeDiskSourceTree(disk_source_tree.get(),
                                  descriptor_set_in_database.get())) {
      return 1;
    }

    error_collector =
        std::make_unique<ErrorPrinter>(error_format_, disk_source_tree.get());

    source_tree_database = std::make_unique<SourceTreeDescriptorDatabase>(
        disk_source_tree.get(), descriptor_set_in_database.get());
    source_tree_database->RecordErrorsTo(error_collector.get());

    descriptor_pool = std::make_unique<DescriptorPool>(
        source_tree_database.get(),
        source_tree_database->GetValidationErrorCollector());
  }

  descriptor_pool->EnforceWeakDependencies(true);

  if (!SetupFeatureResolution(*descriptor_pool)) {
    return EXIT_FAILURE;
  }

  // Enforce extension declarations only when compiling. We want to skip this
  // enforcement when protoc is just being invoked to encode or decode
  // protos. If allowlist is disabled, we will not check for descriptor
  // extensions declarations, either.
  if (mode_ == MODE_COMPILE) {
      descriptor_pool->EnforceExtensionDeclarations(
          ExtDeclEnforcementLevel::kCustomExtensions);
  }
  if (!ParseInputFiles(descriptor_pool.get(), disk_source_tree.get(),
                       &parsed_files)) {
    return 1;
  }

  bool validation_error = false;  // Defer exiting so we log more warnings.

  for (auto& file : parsed_files) {
    google::protobuf::internal::VisitDescriptors(
        *file, [&](const FieldDescriptor& field) {
          if (HasReservedFieldNumber(&field)) {
            const char* error_link = nullptr;
            validation_error = true;
            std::string error;
            if (field.number() >= FieldDescriptor::kFirstReservedNumber &&
                field.number() <= FieldDescriptor::kLastReservedNumber) {
              error = absl::Substitute(
                  "Field numbers $0 through $1 are reserved "
                  "for the protocol buffer library implementation.",
                  FieldDescriptor::kFirstReservedNumber,
                  FieldDescriptor::kLastReservedNumber);
            } else {
              error = absl::Substitute(
                  "Field number $0 is reserved for specific purposes.",
                  field.number());
            }
            if (error_link) {
              absl::StrAppend(&error, "(See ", error_link, ")");
            }
            static_cast<DescriptorPool::ErrorCollector*>(error_collector.get())
                ->RecordError(field.file()->name(), field.full_name(), nullptr,
                              DescriptorPool::ErrorCollector::NUMBER, error);
          }
        });
  }

  // We visit one file at a time because we need to provide the file name for
  // error messages. Usually we can get the file name from any descriptor with
  // something like descriptor->file()->name(), but ExtensionRange does not
  // support this.
  for (const google::protobuf::FileDescriptor* file : parsed_files) {
    FileDescriptorProto proto;
    file->CopyTo(&proto);
    google::protobuf::internal::VisitDescriptors(
        *file, proto, [&](const auto& descriptor, const auto& proto) {
          if (!ValidateTargetConstraints(proto.options(), *descriptor_pool,
                                         *error_collector, file->name(),
                                         GetTargetType(&descriptor))) {
            validation_error = true;
          }
        });
  }


  if (validation_error) {
    return 1;
  }

  if (!EnforceProtocEditionsSupport(parsed_files)) {
    return 1;
  }

  // We construct a separate GeneratorContext for each output location.  Note
  // that two code generators may output to the same location, in which case
  // they should share a single GeneratorContext so that OpenForInsert() works.
  GeneratorContextMap output_directories;

  // Generate output.
  if (mode_ == MODE_COMPILE) {
    for (size_t i = 0; i < output_directives_.size(); ++i) {
      std::string output_location = output_directives_[i].output_location;
      if (!absl::EndsWith(output_location, ".zip") &&
          !absl::EndsWith(output_location, ".jar") &&
          !absl::EndsWith(output_location, ".srcjar")) {
        AddTrailingSlash(&output_location);
      }

      auto& generator = output_directories[output_location];

      if (!generator) {
        // First time we've seen this output location.
        generator = std::make_unique<GeneratorContextImpl>(parsed_files);
      }

      if (!GenerateOutput(parsed_files, output_directives_[i],
                          generator.get())) {
        return 1;
      }
    }
  }

  for (const auto& pair : output_directories) {
    const std::string& location = pair.first;
    GeneratorContextImpl* directory = pair.second.get();
    if (absl::EndsWith(location, "/")) {
      if (!directory->WriteAllToDisk(location)) {
        return 1;
      }
    } else {
      if (absl::EndsWith(location, ".jar")) {
        directory->AddJarManifest();
      }

      if (!directory->WriteAllToZip(location)) {
        return 1;
      }
    }
  }

  if (!dependency_out_name_.empty()) {
    ABSL_DCHECK(disk_source_tree.get());
    if (!GenerateDependencyManifestFile(parsed_files, output_directories,
                                        disk_source_tree.get())) {
      return 1;
    }
  }

  if (!descriptor_set_out_name_.empty()) {
    if (!WriteDescriptorSet(parsed_files)) {
      return 1;
    }
  }

  if (!edition_defaults_out_name_.empty()) {
    if (!WriteEditionDefaults(*descriptor_pool)) {
      return 1;
    }
  }

  if (mode_ == MODE_ENCODE || mode_ == MODE_DECODE) {
    if (codec_type_.empty()) {
      // HACK:  Define an EmptyMessage type to use for decoding.
      DescriptorPool pool;
      FileDescriptorProto file;
      file.set_name("empty_message.proto");
      file.add_message_type()->set_name("EmptyMessage");
      ABSL_CHECK(pool.BuildFile(file) != nullptr);
      codec_type_ = "EmptyMessage";
      if (!EncodeOrDecode(&pool)) {
        return 1;
      }
    } else {
      if (!EncodeOrDecode(descriptor_pool.get())) {
        return 1;
      }
    }
  }

  if (error_collector->FoundErrors() ||
      (fatal_warnings_ && error_collector->FoundWarnings())) {
    return 1;
  }

  if (mode_ == MODE_PRINT) {
    switch (print_mode_) {
      case PRINT_FREE_FIELDS:
        for (size_t i = 0; i < parsed_files.size(); ++i) {
          const FileDescriptor* fd = parsed_files[i];
          for (int j = 0; j < fd->message_type_count(); ++j) {
            PrintFreeFieldNumbers(fd->message_type(j));
          }
        }
        break;
      case PRINT_NONE:
        ABSL_LOG(ERROR)
            << "If the code reaches here, it usually means a bug of "
               "flag parsing in the CommandLineInterface.";
        return 1;

        // Do not add a default case.
    }
  }
  return 0;
}

bool CommandLineInterface::InitializeDiskSourceTree(
    DiskSourceTree* source_tree, DescriptorDatabase* fallback_database) {
  AddDefaultProtoPaths(&proto_path_);

  // Set up the source tree.
  for (size_t i = 0; i < proto_path_.size(); ++i) {
    source_tree->MapPath(proto_path_[i].first, proto_path_[i].second);
  }

  // Map input files to virtual paths if possible.
  if (!MakeInputsBeProtoPathRelative(source_tree, fallback_database)) {
    return false;
  }

  return true;
}

namespace {
std::unique_ptr<SimpleDescriptorDatabase>
PopulateSingleSimpleDescriptorDatabase(const std::string& descriptor_set_name) {
  int fd;
  do {
    fd = open(descriptor_set_name.c_str(), O_RDONLY | O_BINARY);
  } while (fd < 0 && errno == EINTR);
  if (fd < 0) {
    std::cerr << descriptor_set_name << ": " << strerror(ENOENT) << std::endl;
    return nullptr;
  }

  FileDescriptorSet file_descriptor_set;
  bool parsed = file_descriptor_set.ParseFromFileDescriptor(fd);
  if (close(fd) != 0) {
    std::cerr << descriptor_set_name << ": close: " << strerror(errno)
              << std::endl;
    return nullptr;
  }

  if (!parsed) {
    std::cerr << descriptor_set_name << ": Unable to parse." << std::endl;
    return nullptr;
  }

  std::unique_ptr<SimpleDescriptorDatabase> database{
      new SimpleDescriptorDatabase()};

  for (int j = 0; j < file_descriptor_set.file_size(); j++) {
    FileDescriptorProto previously_added_file_descriptor_proto;
    if (database->FindFileByName(file_descriptor_set.file(j).name(),
                                 &previously_added_file_descriptor_proto)) {
      // already present - skip
      continue;
    }
    if (!database->Add(file_descriptor_set.file(j))) {
      return nullptr;
    }
  }
  return database;
}

}  // namespace

bool CommandLineInterface::VerifyInputFilesInDescriptors(
    DescriptorDatabase* database) {
  for (const auto& input_file : input_files_) {
    FileDescriptorProto file_descriptor;
    if (!database->FindFileByName(input_file, &file_descriptor)) {
      std::cerr << "Could not find file in descriptor database: " << input_file
                << ": " << strerror(ENOENT) << std::endl;
      return false;
    }

    // Enforce --disallow_services.
    if (disallow_services_ && file_descriptor.service_size() > 0) {
      std::cerr << file_descriptor.name()
                << ": This file contains services, but "
                   "--disallow_services was used."
                << std::endl;
      return false;
    }
  }
  return true;
}

bool CommandLineInterface::SetupFeatureResolution(DescriptorPool& pool) {
  // Calculate the feature defaults for each built-in generator.  All generators
  // that support editions must agree on the supported edition range.
  std::vector<const FieldDescriptor*> feature_extensions;
  for (const auto& output : output_directives_) {
    if (output.generator == nullptr) continue;
    if (!experimental_editions_ &&
        (output.generator->GetSupportedFeatures() &
         CodeGenerator::FEATURE_SUPPORTS_EDITIONS) != 0) {
      // Only validate min/max edition on generators that advertise editions
      // support.  Generators still under development will always use the
      // correct values.
      if (output.generator->GetMinimumEdition() != ProtocMinimumEdition()) {
        ABSL_LOG(ERROR) << "Built-in generator " << output.name
                        << " specifies a minimum edition "
                        << output.generator->GetMinimumEdition()
                        << " which is not the protoc minimum "
                        << ProtocMinimumEdition() << ".";
        return false;
      }
      if (output.generator->GetMaximumEdition() != ProtocMaximumEdition()) {
        ABSL_LOG(ERROR) << "Built-in generator " << output.name
                        << " specifies a maximum edition "
                        << output.generator->GetMaximumEdition()
                        << " which is not the protoc maximum "
                        << ProtocMaximumEdition() << ".";
        return false;
      }
    }
    for (const FieldDescriptor* ext :
         output.generator->GetFeatureExtensions()) {
      if (ext == nullptr) {
        ABSL_LOG(ERROR) << "Built-in generator " << output.name
                        << " specifies an unknown feature extension.";
        return false;
      }
      feature_extensions.push_back(ext);
    }
  }
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(
          FeatureSet::descriptor(), feature_extensions, ProtocMinimumEdition(),
          MaximumKnownEdition());
  if (!defaults.ok()) {
    ABSL_LOG(ERROR) << defaults.status();
    return false;
  }
  absl::Status status = pool.SetFeatureSetDefaults(std::move(defaults).value());
  ABSL_CHECK(status.ok()) << status.message();
  return true;
}

bool CommandLineInterface::ParseInputFiles(
    DescriptorPool* descriptor_pool, DiskSourceTree* source_tree,
    std::vector<const FileDescriptor*>* parsed_files) {

  if (!proto_path_.empty()) {
    // Track unused imports in all source files that were loaded from the
    // filesystem. We do not track unused imports for files loaded from
    // descriptor sets as they may be programmatically generated in which case
    // exerting this level of rigor is less desirable. We're also making the
    // assumption that the initial parse of the proto from the filesystem
    // was rigorous in checking unused imports and that the descriptor set
    // being parsed was produced then and that it was subsequent mutations
    // of that descriptor set that left unused imports.
    //
    // Note that relying on proto_path exclusively is limited in that we may
    // be loading descriptors from both the filesystem and descriptor sets
    // depending on the invocation. At least for invocations that are
    // exclusively reading from descriptor sets, we can eliminate this failure
    // condition.
    for (const auto& input_file : input_files_) {
      descriptor_pool->AddDirectInputFile(input_file);
    }
  }

  bool result = true;
  // Parse each file.
  for (const auto& input_file : input_files_) {
    // Import the file.
    const FileDescriptor* parsed_file =
        descriptor_pool->FindFileByName(input_file);
    if (parsed_file == nullptr) {
      result = false;
      break;
    }
    parsed_files->push_back(parsed_file);

    // Enforce --disallow_services.
    if (disallow_services_ && parsed_file->service_count() > 0) {
      std::cerr << parsed_file->name()
                << ": This file contains services, but "
                   "--disallow_services was used."
                << std::endl;
      result = false;
      break;
    }

    // Enforce --direct_dependencies
    if (direct_dependencies_explicitly_set_) {
      bool indirect_imports = false;
      for (int i = 0; i < parsed_file->dependency_count(); ++i) {
        if (direct_dependencies_.find(parsed_file->dependency(i)->name()) ==
            direct_dependencies_.end()) {
          indirect_imports = true;
          std::cerr << parsed_file->name() << ": "
                    << absl::StrReplaceAll(
                           direct_dependencies_violation_msg_,
                           {{"%s", parsed_file->dependency(i)->name()}})
                    << std::endl;
        }
      }
      if (indirect_imports) {
        result = false;
        break;
      }
    }
  }
  descriptor_pool->ClearDirectInputFiles();
  return result;
}

void CommandLineInterface::Clear() {
  // Clear all members that are set by Run().  Note that we must not clear
  // members which are set by other methods before Run() is called.
  executable_name_.clear();
  proto_path_.clear();
  input_files_.clear();
  direct_dependencies_.clear();
  direct_dependencies_violation_msg_ = kDefaultDirectDependenciesViolationMsg;
  output_directives_.clear();
  codec_type_.clear();
  descriptor_set_in_names_.clear();
  descriptor_set_out_name_.clear();
  dependency_out_name_.clear();

  experimental_editions_ = false;
  edition_defaults_out_name_.clear();
  edition_defaults_minimum_ = EDITION_UNKNOWN;
  edition_defaults_maximum_ = EDITION_UNKNOWN;

  mode_ = MODE_COMPILE;
  print_mode_ = PRINT_NONE;
  imports_in_descriptor_set_ = false;
  source_info_in_descriptor_set_ = false;
  retain_options_in_descriptor_set_ = false;
  disallow_services_ = false;
  direct_dependencies_explicitly_set_ = false;
  deterministic_output_ = false;
}

bool CommandLineInterface::MakeProtoProtoPathRelative(
    DiskSourceTree* source_tree, std::string* proto,
    DescriptorDatabase* fallback_database) {
  // If it's in the fallback db, don't report non-existent file errors.
  FileDescriptorProto fallback_file;
  bool in_fallback_database =
      fallback_database != nullptr &&
      fallback_database->FindFileByName(*proto, &fallback_file);

  // If the input file path is not a physical file path, it must be a virtual
  // path.
  if (access(proto->c_str(), F_OK) < 0) {
    std::string disk_file;
    if (source_tree->VirtualFileToDiskFile(*proto, &disk_file) ||
        in_fallback_database) {
      return true;
    } else {
      std::cerr << "Could not make proto path relative: " << *proto << ": "
                << strerror(ENOENT) << std::endl;
      return false;
    }
  }

  std::string virtual_file, shadowing_disk_file;
  switch (source_tree->DiskFileToVirtualFile(*proto, &virtual_file,
                                             &shadowing_disk_file)) {
    case DiskSourceTree::SUCCESS:
      *proto = virtual_file;
      break;
    case DiskSourceTree::SHADOWED:
      std::cerr << *proto << ": Input is shadowed in the --proto_path by \""
                << shadowing_disk_file
                << "\".  Either use the latter file as your input or reorder "
                   "the --proto_path so that the former file's location "
                   "comes first."
                << std::endl;
      return false;
    case DiskSourceTree::CANNOT_OPEN: {
      if (in_fallback_database) {
        return true;
      }
      std::string error_str = source_tree->GetLastErrorMessage().empty()
                                  ? strerror(errno)
                                  : source_tree->GetLastErrorMessage();
      std::cerr << "Could not map to virtual file: " << *proto << ": "
                << error_str << std::endl;
      return false;
    }
    case DiskSourceTree::NO_MAPPING: {
      // Try to interpret the path as a virtual path.
      std::string disk_file;
      if (source_tree->VirtualFileToDiskFile(*proto, &disk_file) ||
          in_fallback_database) {
        return true;
      } else {
        // The input file path can't be mapped to any --proto_path and it also
        // can't be interpreted as a virtual path.
        std::cerr
            << *proto
            << ": File does not reside within any path "
               "specified using --proto_path (or -I).  You must specify a "
               "--proto_path which encompasses this file.  Note that the "
               "proto_path must be an exact prefix of the .proto file "
               "names -- protoc is too dumb to figure out when two paths "
               "(e.g. absolute and relative) are equivalent (it's harder "
               "than you think)."
            << std::endl;
        return false;
      }
    }
  }
  return true;
}

bool CommandLineInterface::MakeInputsBeProtoPathRelative(
    DiskSourceTree* source_tree, DescriptorDatabase* fallback_database) {
  for (auto& input_file : input_files_) {
    if (!MakeProtoProtoPathRelative(source_tree, &input_file,
                                    fallback_database)) {
      return false;
    }
  }

  return true;
}


bool CommandLineInterface::ExpandArgumentFile(
    const char* file, std::vector<std::string>* arguments) {
// On windows to force ifstream to handle proper utr-8, we need to convert to
// proper supported utf8 wstring. If we dont then the file can't be opened.
#ifdef _MSC_VER
  // Convert the file name to wide chars.
  int size = MultiByteToWideChar(CP_UTF8, 0, file, strlen(file), nullptr, 0);
  std::wstring file_str;
  file_str.resize(size);
  MultiByteToWideChar(CP_UTF8, 0, file, strlen(file), &file_str[0],
                      file_str.size());
#else
  std::string file_str(file);
#endif

  // The argument file is searched in the working directory only. We don't
  // use the proto import path here.
  std::ifstream file_stream(file_str.c_str());
  if (!file_stream.is_open()) {
    return false;
  }
  std::string argument;
  // We don't support any kind of shell expansion right now.
  while (std::getline(file_stream, argument)) {
    arguments->push_back(argument);
  }
  return true;
}

CommandLineInterface::ParseArgumentStatus CommandLineInterface::ParseArguments(
    int argc, const char* const argv[]) {
  executable_name_ = argv[0];

  std::vector<std::string> arguments;
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '@') {
      if (!ExpandArgumentFile(argv[i] + 1, &arguments)) {
        std::cerr << "Failed to open argument file: " << (argv[i] + 1)
                  << std::endl;
        return PARSE_ARGUMENT_FAIL;
      }
      continue;
    }
    arguments.push_back(argv[i]);
  }

  // if no arguments are given, show help
  if (arguments.empty()) {
    PrintHelpText();
    return PARSE_ARGUMENT_DONE_AND_EXIT;  // Exit without running compiler.
  }

  // Iterate through all arguments and parse them.
  for (size_t i = 0; i < arguments.size(); ++i) {
    std::string name, value;

    if (ParseArgument(arguments[i].c_str(), &name, &value)) {
      // Returned true => Use the next argument as the flag value.
      if (i + 1 == arguments.size() || arguments[i + 1][0] == '-') {
        std::cerr << "Missing value for flag: " << name << std::endl;
        if (name == "--decode") {
          std::cerr << "To decode an unknown message, use --decode_raw."
                    << std::endl;
        }
        return PARSE_ARGUMENT_FAIL;
      } else {
        ++i;
        value = arguments[i];
      }
    }

    ParseArgumentStatus status = InterpretArgument(name, value);
    if (status != PARSE_ARGUMENT_DONE_AND_CONTINUE) return status;
  }

  // Make sure each plugin option has a matching plugin output.
  bool foundUnknownPluginOption = false;
  for (const auto& kv : plugin_parameters_) {
    if (plugins_.find(kv.first) != plugins_.end()) {
      continue;
    }
    bool foundImplicitPlugin = false;
    for (const auto& d : output_directives_) {
      if (d.generator == nullptr) {
        // Infers the plugin name from the plugin_prefix_ and output directive.
        std::string plugin_name = PluginName(plugin_prefix_, d.name);

        // Since plugin_parameters_ is also inferred from --xxx_opt, we check
        // that it actually matches the plugin name inferred from --xxx_out.
        if (plugin_name == kv.first) {
          foundImplicitPlugin = true;
          break;
        }
      }
    }

    // This is a special case for cc_plugin invocations that are only with
    // "--cpp_opt" but no "--cpp_out". In this case, "--cpp_opt" only serves
    // as passing the arguments to cc_plugins, and no C++ generator is required
    // to be present in the invocation. We didn't have to skip for C++ generator
    // previously because the C++ generator was built-in.
    if (!foundImplicitPlugin &&
        kv.first != absl::StrCat(plugin_prefix_, "gen-cpp")) {
      std::cerr << "Unknown flag: "
                // strip prefix + "gen-" and add back "_opt"
                << "--" << kv.first.substr(plugin_prefix_.size() + 4) << "_opt"
                << std::endl;
      foundUnknownPluginOption = true;
    }
  }
  if (foundUnknownPluginOption) {
    return PARSE_ARGUMENT_FAIL;
  }

  // The --proto_path & --descriptor_set_in flags both specify places to look
  // for proto files. If neither were given, use the current working directory.
  if (proto_path_.empty() && descriptor_set_in_names_.empty()) {
    // Don't use make_pair as the old/default standard library on Solaris
    // doesn't support it without explicit template parameters, which are
    // incompatible with C++0x's make_pair.
    proto_path_.push_back(std::pair<std::string, std::string>("", "."));
  }

  // Check error cases that span multiple flag values.
  bool missing_proto_definitions = false;
  switch (mode_) {
    case MODE_COMPILE:
      missing_proto_definitions = input_files_.empty();
      break;
    case MODE_DECODE:
      // Handle --decode_raw separately, since it requires that no proto
      // definitions are specified.
      if (codec_type_.empty()) {
        if (!input_files_.empty() || !descriptor_set_in_names_.empty()) {
          std::cerr
              << "When using --decode_raw, no input files should be given."
              << std::endl;
          return PARSE_ARGUMENT_FAIL;
        }
        missing_proto_definitions = false;
        break;  // only for --decode_raw
      }
      // --decode (not raw) is handled the same way as the rest of the modes.
      ABSL_FALLTHROUGH_INTENDED;
    case MODE_ENCODE:
    case MODE_PRINT:
      missing_proto_definitions =
          input_files_.empty() && descriptor_set_in_names_.empty();
      break;
    default:
      ABSL_LOG(FATAL) << "Unexpected mode: " << mode_;
  }
  if (missing_proto_definitions) {
    std::cerr << "Missing input file." << std::endl;
    return PARSE_ARGUMENT_FAIL;
  }
  if (mode_ == MODE_COMPILE && output_directives_.empty() &&
      descriptor_set_out_name_.empty() && edition_defaults_out_name_.empty()) {
    std::cerr << "Missing output directives." << std::endl;
    return PARSE_ARGUMENT_FAIL;
  }
  if (mode_ != MODE_COMPILE && !dependency_out_name_.empty()) {
    std::cerr << "Can only use --dependency_out=FILE when generating code."
              << std::endl;
    return PARSE_ARGUMENT_FAIL;
  }
  if (mode_ != MODE_ENCODE && deterministic_output_) {
    std::cerr << "Can only use --deterministic_output with --encode."
              << std::endl;
    return PARSE_ARGUMENT_FAIL;
  }
  if (!dependency_out_name_.empty() && input_files_.size() > 1) {
    std::cerr
        << "Can only process one input file when using --dependency_out=FILE."
        << std::endl;
    return PARSE_ARGUMENT_FAIL;
  }
  if (imports_in_descriptor_set_ && descriptor_set_out_name_.empty()) {
    std::cerr << "--include_imports only makes sense when combined with "
                 "--descriptor_set_out."
              << std::endl;
  }
  if (source_info_in_descriptor_set_ && descriptor_set_out_name_.empty()) {
    std::cerr << "--include_source_info only makes sense when combined with "
                 "--descriptor_set_out."
              << std::endl;
  }
  if (retain_options_in_descriptor_set_ && descriptor_set_out_name_.empty()) {
    std::cerr << "--retain_options only makes sense when combined with "
                 "--descriptor_set_out."
              << std::endl;
  }

  return PARSE_ARGUMENT_DONE_AND_CONTINUE;
}

bool CommandLineInterface::ParseArgument(const char* arg, std::string* name,
                                         std::string* value) {
  bool parsed_value = false;

  if (arg[0] != '-') {
    // Not a flag.
    name->clear();
    parsed_value = true;
    *value = arg;
  } else if (arg[1] == '-') {
    // Two dashes:  Multi-character name, with '=' separating name and
    //   value.
    const char* equals_pos = strchr(arg, '=');
    if (equals_pos != nullptr) {
      *name = std::string(arg, equals_pos - arg);
      *value = equals_pos + 1;
      parsed_value = true;
    } else {
      *name = arg;
    }
  } else {
    // One dash:  One-character name, all subsequent characters are the
    //   value.
    if (arg[1] == '\0') {
      // arg is just "-".  We treat this as an input file, except that at
      // present this will just lead to a "file not found" error.
      name->clear();
      *value = arg;
      parsed_value = true;
    } else {
      *name = std::string(arg, 2);
      *value = arg + 2;
      parsed_value = !value->empty();
    }
  }

  // Need to return true iff the next arg should be used as the value for this
  // one, false otherwise.

  if (parsed_value) {
    // We already parsed a value for this flag.
    return false;
  }

  if (*name == "-h" || *name == "--help" || *name == "--disallow_services" ||
      *name == "--include_imports" || *name == "--include_source_info" ||
      *name == "--retain_options" || *name == "--version" ||
      *name == "--decode_raw" ||
      *name == "--experimental_editions" ||
      *name == "--print_free_field_numbers" ||
      *name == "--experimental_allow_proto3_optional" ||
      *name == "--deterministic_output" || *name == "--fatal_warnings") {
    // HACK:  These are the only flags that don't take a value.
    //   They probably should not be hard-coded like this but for now it's
    //   not worth doing better.
    return false;
  }

  // Next argument is the flag value.
  return true;
}

CommandLineInterface::ParseArgumentStatus
CommandLineInterface::InterpretArgument(const std::string& name,
                                        const std::string& value) {
  if (name.empty()) {
    // Not a flag.  Just a filename.
    if (value.empty()) {
      std::cerr
          << "You seem to have passed an empty string as one of the "
             "arguments to "
          << executable_name_
          << ".  This is actually "
             "sort of hard to do.  Congrats.  Unfortunately it is not valid "
             "input so the program is going to die now."
          << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }

#if defined(_WIN32)
    // On Windows, the shell (typically cmd.exe) does not expand wildcards in
    // file names (e.g. foo\*.proto), so we do it ourselves.
    switch (google::protobuf::io::win32::ExpandWildcards(
        value, [this](const std::string& path) {
          this->input_files_.push_back(path);
        })) {
      case google::protobuf::io::win32::ExpandWildcardsResult::kSuccess:
        break;
      case google::protobuf::io::win32::ExpandWildcardsResult::kErrorNoMatchingFile:
        // Path does not exist, is not a file, or it's longer than MAX_PATH and
        // long path handling is disabled.
        std::cerr << "Invalid file name pattern or missing input file \""
                  << value << "\"" << std::endl;
        return PARSE_ARGUMENT_FAIL;
      default:
        std::cerr << "Cannot convert path \"" << value
                  << "\" to or from Windows style" << std::endl;
        return PARSE_ARGUMENT_FAIL;
    }
#else   // not _WIN32
    // On other platforms than Windows (e.g. Linux, Mac OS) the shell (typically
    // Bash) expands wildcards.
    input_files_.push_back(value);
#endif  // _WIN32

  } else if (name == "-I" || name == "--proto_path") {
    // Java's -classpath (and some other languages) delimits path components
    // with colons.  Let's accept that syntax too just to make things more
    // intuitive.
    std::vector<std::string> parts = absl::StrSplit(
        value, absl::ByAnyChar(CommandLineInterface::kPathSeparator),
        absl::SkipEmpty());

    for (size_t i = 0; i < parts.size(); ++i) {
      std::string virtual_path;
      std::string disk_path;

      std::string::size_type equals_pos = parts[i].find_first_of('=');
      if (equals_pos == std::string::npos) {
        virtual_path = "";
        disk_path = parts[i];
      } else {
        virtual_path = parts[i].substr(0, equals_pos);
        disk_path = parts[i].substr(equals_pos + 1);
      }

      if (disk_path.empty()) {
        std::cerr
            << "--proto_path passed empty directory name.  (Use \".\" for "
               "current directory.)"
            << std::endl;
        return PARSE_ARGUMENT_FAIL;
      }

      // Make sure disk path exists, warn otherwise.
      if (access(disk_path.c_str(), F_OK) < 0) {
        // Try the original path; it may have just happened to have a '=' in it.
        if (access(parts[i].c_str(), F_OK) < 0) {
          std::cerr << disk_path << ": warning: directory does not exist."
                    << std::endl;
        } else {
          virtual_path = "";
          disk_path = parts[i];
        }
      }

      // Don't use make_pair as the old/default standard library on Solaris
      // doesn't support it without explicit template parameters, which are
      // incompatible with C++0x's make_pair.
      proto_path_.push_back(
          std::pair<std::string, std::string>(virtual_path, disk_path));
    }

  } else if (name == "--direct_dependencies") {
    if (direct_dependencies_explicitly_set_) {
      std::cerr << name
                << " may only be passed once. To specify multiple "
                   "direct dependencies, pass them all as a single "
                   "parameter separated by ':'."
                << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }

    direct_dependencies_explicitly_set_ = true;
    std::vector<std::string> direct =
        absl::StrSplit(value, ':', absl::SkipEmpty());
    ABSL_DCHECK(direct_dependencies_.empty());
    direct_dependencies_.insert(direct.begin(), direct.end());

  } else if (name == "--direct_dependencies_violation_msg") {
    direct_dependencies_violation_msg_ = value;

  } else if (name == "--descriptor_set_in") {
    if (!descriptor_set_in_names_.empty()) {
      std::cerr << name
                << " may only be passed once. To specify multiple "
                   "descriptor sets, pass them all as a single "
                   "parameter separated by '"
                << CommandLineInterface::kPathSeparator << "'." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (value.empty()) {
      std::cerr << name << " requires a non-empty value." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (!dependency_out_name_.empty()) {
      std::cerr << name << " cannot be used with --dependency_out."
                << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }

    descriptor_set_in_names_ = absl::StrSplit(
        value, absl::ByAnyChar(CommandLineInterface::kPathSeparator),
        absl::SkipEmpty());

  } else if (name == "-o" || name == "--descriptor_set_out") {
    if (!descriptor_set_out_name_.empty()) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (value.empty()) {
      std::cerr << name << " requires a non-empty value." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (mode_ != MODE_COMPILE) {
      std::cerr
          << "Cannot use --encode or --decode and generate descriptors at the "
             "same time."
          << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    descriptor_set_out_name_ = value;

  } else if (name == "--dependency_out") {
    if (!dependency_out_name_.empty()) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (value.empty()) {
      std::cerr << name << " requires a non-empty value." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (!descriptor_set_in_names_.empty()) {
      std::cerr << name << " cannot be used with --descriptor_set_in."
                << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    dependency_out_name_ = value;

  } else if (name == "--include_imports") {
    if (imports_in_descriptor_set_) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    imports_in_descriptor_set_ = true;

  } else if (name == "--include_source_info") {
    if (source_info_in_descriptor_set_) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    source_info_in_descriptor_set_ = true;

  } else if (name == "--retain_options") {
    if (retain_options_in_descriptor_set_) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    retain_options_in_descriptor_set_ = true;

  } else if (name == "-h" || name == "--help") {
    PrintHelpText();
    return PARSE_ARGUMENT_DONE_AND_EXIT;  // Exit without running compiler.

  } else if (name == "--version") {
    if (!version_info_.empty()) {
      std::cout << version_info_ << std::endl;
    }
    std::cout << "libprotoc "
              << ::google::protobuf::internal::ProtocVersionString(
                     PROTOBUF_VERSION)
              << PROTOBUF_VERSION_SUFFIX << std::endl;
    return PARSE_ARGUMENT_DONE_AND_EXIT;  // Exit without running compiler.

  } else if (name == "--disallow_services") {
    disallow_services_ = true;
  } else if (name == "--experimental_allow_proto3_optional") {
    // Flag is no longer observed, but we allow it for backward compat.
  } else if (name == "--encode" || name == "--decode" ||
             name == "--decode_raw") {
    if (mode_ != MODE_COMPILE) {
      std::cerr << "Only one of --encode and --decode can be specified."
                << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (!output_directives_.empty() || !descriptor_set_out_name_.empty()) {
      std::cerr << "Cannot use " << name
                << " and generate code or descriptors at the same time."
                << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }

    mode_ = (name == "--encode") ? MODE_ENCODE : MODE_DECODE;

    if (value.empty() && name != "--decode_raw") {
      std::cerr << "Type name for " << name << " cannot be blank." << std::endl;
      if (name == "--decode") {
        std::cerr << "To decode an unknown message, use --decode_raw."
                  << std::endl;
      }
      return PARSE_ARGUMENT_FAIL;
    } else if (!value.empty() && name == "--decode_raw") {
      std::cerr << "--decode_raw does not take a parameter." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }

    codec_type_ = value;

  } else if (name == "--deterministic_output") {
    deterministic_output_ = true;

  } else if (name == "--error_format") {
    if (value == "gcc") {
      error_format_ = ERROR_FORMAT_GCC;
    } else if (value == "msvs") {
      error_format_ = ERROR_FORMAT_MSVS;
    } else {
      std::cerr << "Unknown error format: " << value << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }

  } else if (name == "--fatal_warnings") {
    if (fatal_warnings_) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    fatal_warnings_ = true;
  } else if (name == "--plugin") {
    if (plugin_prefix_.empty()) {
      std::cerr << "This compiler does not support plugins." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }

    std::string plugin_name;
    std::string path;

    std::string::size_type equals_pos = value.find_first_of('=');
    if (equals_pos == std::string::npos) {
      // Use the basename of the file.
      std::string::size_type slash_pos = value.find_last_of('/');
      if (slash_pos == std::string::npos) {
        plugin_name = value;
      } else {
        plugin_name = value.substr(slash_pos + 1);
      }
      path = value;
    } else {
      plugin_name = value.substr(0, equals_pos);
      path = value.substr(equals_pos + 1);
    }

    plugins_[plugin_name] = path;

  } else if (name == "--print_free_field_numbers") {
    if (mode_ != MODE_COMPILE) {
      std::cerr << "Cannot use " << name
                << " and use --encode, --decode or print "
                << "other info at the same time." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (!output_directives_.empty() || !descriptor_set_out_name_.empty()) {
      std::cerr << "Cannot use " << name
                << " and generate code or descriptors at the same time."
                << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    mode_ = MODE_PRINT;
    print_mode_ = PRINT_FREE_FIELDS;
  } else if (name == "--enable_codegen_trace") {
    // We use environment variables here so that subprocesses see this setting
    // when we spawn them.
    //
    // Setting environment variables is more-or-less asking for a data race,
    // because C got this wrong and did not mandate synchronization.
    // In practice, this code path is "only" in the main thread of protoc, and
    // it is common knowledge that touching setenv in a library is asking for
    // life-ruining bugs *anyways*. As such, there is a reasonable probability
    // that there isn't another thread kicking environment variables at this
    // moment.

#ifdef _WIN32
    ::_putenv(absl::StrCat(io::Printer::kProtocCodegenTrace, "=yes").c_str());
#else
    ::setenv(io::Printer::kProtocCodegenTrace.data(), "yes", 0);
#endif
  } else if (name == "--experimental_editions") {
    // If you're reading this, you're probably wondering what
    // --experimental_editions is for and thinking of turning it on. This is an
    // experimental, undocumented, unsupported flag. Enable it at your own risk
    // (or, just don't!).
    experimental_editions_ = true;
  } else if (name == "--edition_defaults_out") {
    if (!edition_defaults_out_name_.empty()) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (value.empty()) {
      std::cerr << name << " requires a non-empty value." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (mode_ != MODE_COMPILE) {
      std::cerr
          << "Cannot use --encode or --decode and generate defaults at the "
             "same time."
          << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    edition_defaults_out_name_ = value;
  } else if (name == "--edition_defaults_minimum") {
    if (edition_defaults_minimum_ != EDITION_UNKNOWN) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (!Edition_Parse(absl::StrCat("EDITION_", value),
                       &edition_defaults_minimum_)) {
      std::cerr << name << " unknown edition \"" << value << "\"." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
  } else if (name == "--edition_defaults_maximum") {
    if (edition_defaults_maximum_ != EDITION_UNKNOWN) {
      std::cerr << name << " may only be passed once." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
    if (!Edition_Parse(absl::StrCat("EDITION_", value),
                       &edition_defaults_maximum_)) {
      std::cerr << name << " unknown edition \"" << value << "\"." << std::endl;
      return PARSE_ARGUMENT_FAIL;
    }
  } else {
    // Some other flag.  Look it up in the generators list.
    const GeneratorInfo* generator_info = FindGeneratorByFlag(name);
    if (generator_info == nullptr &&
        (plugin_prefix_.empty() || !absl::EndsWith(name, "_out"))) {
      // Check if it's a generator option flag.
      generator_info = FindGeneratorByOption(name);
      if (generator_info != nullptr) {
        std::string* parameters =
            &generator_parameters_[generator_info->flag_name];
        if (!parameters->empty()) {
          parameters->append(",");
        }
        parameters->append(value);
      } else if (absl::StartsWith(name, "--") && absl::EndsWith(name, "_opt")) {
        std::string* parameters =
            &plugin_parameters_[PluginName(plugin_prefix_, name)];
        if (!parameters->empty()) {
          parameters->append(",");
        }
        parameters->append(value);
      } else {
        std::cerr << "Unknown flag: " << name << std::endl;
        return PARSE_ARGUMENT_FAIL;
      }
    } else {
      // It's an output flag.  Add it to the output directives.
      if (mode_ != MODE_COMPILE) {
        std::cerr << "Cannot use --encode, --decode or print .proto info and "
                     "generate code at the same time."
                  << std::endl;
        return PARSE_ARGUMENT_FAIL;
      }

      OutputDirective directive;
      directive.name = name;
      if (generator_info == nullptr) {
        directive.generator = nullptr;
      } else {
        directive.generator = generator_info->generator;
      }

      // Split value at ':' to separate the generator parameter from the
      // filename.  However, avoid doing this if the colon is part of a valid
      // Windows-style absolute path.
      std::string::size_type colon_pos = value.find_first_of(':');
      if (colon_pos == std::string::npos || IsWindowsAbsolutePath(value)) {
        directive.output_location = value;
      } else {
        directive.parameter = value.substr(0, colon_pos);
        directive.output_location = value.substr(colon_pos + 1);
      }

      output_directives_.push_back(directive);
    }
  }

  return PARSE_ARGUMENT_DONE_AND_CONTINUE;
}

void CommandLineInterface::PrintHelpText() {
  // Sorry for indentation here; line wrapping would be uglier.
  std::cout << "Usage: " << executable_name_ << " [OPTION] PROTO_FILES";
  std::cout << R"(
Parse PROTO_FILES and generate output based on the options given:
  -IPATH, --proto_path=PATH   Specify the directory in which to search for
                              imports.  May be specified multiple times;
                              directories will be searched in order.  If not
                              given, the current working directory is used.
                              If not found in any of the these directories,
                              the --descriptor_set_in descriptors will be
                              checked for required proto file.
  --version                   Show version info and exit.
  -h, --help                  Show this text and exit.
  --encode=MESSAGE_TYPE       Read a text-format message of the given type
                              from standard input and write it in binary
                              to standard output.  The message type must
                              be defined in PROTO_FILES or their imports.
  --deterministic_output      When using --encode, ensure map fields are
                              deterministically ordered. Note that this order
                              is not canonical, and changes across builds or
                              releases of protoc.
  --decode=MESSAGE_TYPE       Read a binary message of the given type from
                              standard input and write it in text format
                              to standard output.  The message type must
                              be defined in PROTO_FILES or their imports.
  --decode_raw                Read an arbitrary protocol message from
                              standard input and write the raw tag/value
                              pairs in text format to standard output.  No
                              PROTO_FILES should be given when using this
                              flag.
  --descriptor_set_in=FILES   Specifies a delimited list of FILES
                              each containing a FileDescriptorSet (a
                              protocol buffer defined in descriptor.proto).
                              The FileDescriptor for each of the PROTO_FILES
                              provided will be loaded from these
                              FileDescriptorSets. If a FileDescriptor
                              appears multiple times, the first occurrence
                              will be used.
  -oFILE,                     Writes a FileDescriptorSet (a protocol buffer,
    --descriptor_set_out=FILE defined in descriptor.proto) containing all of
                              the input files to FILE.
  --include_imports           When using --descriptor_set_out, also include
                              all dependencies of the input files in the
                              set, so that the set is self-contained.
  --include_source_info       When using --descriptor_set_out, do not strip
                              SourceCodeInfo from the FileDescriptorProto.
                              This results in vastly larger descriptors that
                              include information about the original
                              location of each decl in the source file as
                              well as surrounding comments.
  --retain_options            When using --descriptor_set_out, do not strip
                              any options from the FileDescriptorProto.
                              This results in potentially larger descriptors
                              that include information about options that were
                              only meant to be useful during compilation.
  --dependency_out=FILE       Write a dependency output file in the format
                              expected by make. This writes the transitive
                              set of input file paths to FILE
  --error_format=FORMAT       Set the format in which to print errors.
                              FORMAT may be 'gcc' (the default) or 'msvs'
                              (Microsoft Visual Studio format).
  --fatal_warnings            Make warnings be fatal (similar to -Werr in
                              gcc). This flag will make protoc return
                              with a non-zero exit code if any warnings
                              are generated.
  --print_free_field_numbers  Print the free field numbers of the messages
                              defined in the given proto files. Extension ranges
                              are counted as occupied fields numbers.
  --enable_codegen_trace      Enables tracing which parts of protoc are
                              responsible for what codegen output. Not supported
                              by all backends or on all platforms.)";
  if (!plugin_prefix_.empty()) {
    std::cout << R"(
  --plugin=EXECUTABLE         Specifies a plugin executable to use.
                              Normally, protoc searches the PATH for
                              plugins, but you may specify additional
                              executables not in the path using this flag.
                              Additionally, EXECUTABLE may be of the form
                              NAME=PATH, in which case the given plugin name
                              is mapped to the given executable even if
                              the executable's own name differs.)";
  }

  for (const auto& kv : generators_by_flag_name_) {
    // FIXME(kenton):  If the text is long enough it will wrap, which is ugly,
    //   but fixing this nicely (e.g. splitting on spaces) is probably more
    //   trouble than it's worth.
    std::cout << std::endl
              << "  " << kv.first << "=OUT_DIR "
              << std::string(19 - kv.first.size(),
                             ' ')  // Spaces for alignment.
              << kv.second.help_text;
  }
  std::cout << R"(
  @<filename>                 Read options and filenames from file. If a
                              relative file path is specified, the file
                              will be searched in the working directory.
                              The --proto_path option will not affect how
                              this argument file is searched. Content of
                              the file will be expanded in the position of
                              @<filename> as in the argument list. Note
                              that shell expansion is not applied to the
                              content of the file (i.e., you cannot use
                              quotes, wildcards, escapes, commands, etc.).
                              Each line corresponds to a single argument,
                              even if it contains spaces.)";
  std::cout << std::endl;
}

bool CommandLineInterface::EnforceProto3OptionalSupport(
    const std::string& codegen_name, uint64_t supported_features,
    const std::vector<const FileDescriptor*>& parsed_files) const {
  bool supports_proto3_optional =
      supported_features & CodeGenerator::FEATURE_PROTO3_OPTIONAL;
  if (!supports_proto3_optional) {
    for (const auto fd : parsed_files) {
      if (ContainsProto3Optional(
              ::google::protobuf::internal::InternalFeatureHelper::GetEdition(*fd), fd)) {
        std::cerr << fd->name()
                  << ": is a proto3 file that contains optional fields, but "
                     "code generator "
                  << codegen_name
                  << " hasn't been updated to support optional fields in "
                     "proto3. Please ask the owner of this code generator to "
                     "support proto3 optional."
                  << std::endl;
        return false;
      }
    }
  }
  return true;
}

bool CommandLineInterface::EnforceEditionsSupport(
    const std::string& codegen_name, uint64_t supported_features,
    Edition minimum_edition, Edition maximum_edition,
    const std::vector<const FileDescriptor*>& parsed_files) const {
  if (experimental_editions_) {
    // The user has explicitly specified the experimental flag.
    return true;
  }
  for (const auto* fd : parsed_files) {
    Edition edition =
        ::google::protobuf::internal::InternalFeatureHelper::GetEdition(*fd);
    if (edition < Edition::EDITION_2023 || CanSkipEditionCheck(fd->name())) {
      // Legacy proto2/proto3 or exempted files don't need any checks.
      continue;
    }

    if ((supported_features & CodeGenerator::FEATURE_SUPPORTS_EDITIONS) == 0) {
      std::cerr << absl::Substitute(
          "$0: is an editions file, but code generator $1 hasn't been "
          "updated to support editions yet.  Please ask the owner of this code "
          "generator to add support or switch back to proto2/proto3.\n\nSee "
          "https://protobuf.dev/editions/overview/ for more information.",
          fd->name(), codegen_name);
      return false;
    }
    if (edition < minimum_edition) {
      std::cerr << absl::Substitute(
          "$0: is a file using edition $2, which isn't supported by code "
          "generator $1.  Please upgrade your file to at least edition $3.",
          fd->name(), codegen_name, edition, minimum_edition);
      return false;
    }
    if (edition > maximum_edition) {
      std::cerr << absl::Substitute(
          "$0: is a file using edition $2, which isn't supported by code "
          "generator $1.  Please ask the owner of this code generator to add "
          "support or switch back to a maximum of edition $3.",
          fd->name(), codegen_name, edition, maximum_edition);
      return false;
    }
  }
  return true;
}

bool CommandLineInterface::EnforceProtocEditionsSupport(
    const std::vector<const FileDescriptor*>& parsed_files) const {
  if (experimental_editions_) {
    // The user has explicitly specified the experimental flag.
    return true;
  }
  for (const auto* fd : parsed_files) {
    Edition edition =
        ::google::protobuf::internal::InternalFeatureHelper::GetEdition(*fd);
    if (CanSkipEditionCheck(fd->name())) {
      // Legacy proto2/proto3 or exempted files don't need any checks.
      continue;
    }

    if (edition > ProtocMaximumEdition()) {
      std::cerr << absl::Substitute(
          "$0: is a file using edition $1, which is later than the protoc "
          "maximum supported edition $2.",
          fd->name(), edition, ProtocMaximumEdition());
      return false;
    }
  }
  return true;
}

bool CommandLineInterface::GenerateOutput(
    const std::vector<const FileDescriptor*>& parsed_files,
    const OutputDirective& output_directive,
    GeneratorContext* generator_context) {
  // Call the generator.
  std::string error;
  if (output_directive.generator == nullptr) {
    // This is a plugin.
    ABSL_CHECK(absl::StartsWith(output_directive.name, "--") &&
               absl::EndsWith(output_directive.name, "_out"))
        << "Bad name for plugin generator: " << output_directive.name;

    std::string plugin_name = PluginName(plugin_prefix_, output_directive.name);
    std::string parameters = output_directive.parameter;
    if (!plugin_parameters_[plugin_name].empty()) {
      if (!parameters.empty()) {
        parameters.append(",");
      }
      parameters.append(plugin_parameters_[plugin_name]);
    }
    if (!GeneratePluginOutput(parsed_files, plugin_name, parameters,
                              generator_context, &error)) {
      std::cerr << output_directive.name << ": " << error << std::endl;
      return false;
    }
  } else {
    // Regular generator.
    std::string parameters = output_directive.parameter;
    if (!generator_parameters_[output_directive.name].empty()) {
      if (!parameters.empty()) {
        parameters.append(",");
      }
      parameters.append(generator_parameters_[output_directive.name]);
    }
    if (!EnforceProto3OptionalSupport(
            output_directive.name,
            output_directive.generator->GetSupportedFeatures(), parsed_files)) {
      return false;
    }

    if (!EnforceEditionsSupport(
            output_directive.name,
            output_directive.generator->GetSupportedFeatures(),
            output_directive.generator->GetMinimumEdition(),
            output_directive.generator->GetMaximumEdition(), parsed_files)) {
      return false;
    }

    if (!output_directive.generator->GenerateAll(parsed_files, parameters,
                                                 generator_context, &error)) {
      // Generator returned an error.
      std::cerr << output_directive.name << ": " << error << std::endl;
      return false;
    }
  }

  return true;
}

bool CommandLineInterface::GenerateDependencyManifestFile(
    const std::vector<const FileDescriptor*>& parsed_files,
    const GeneratorContextMap& output_directories,
    DiskSourceTree* source_tree) {
  FileDescriptorSet file_set;

  absl::flat_hash_set<const FileDescriptor*> already_seen;
  for (size_t i = 0; i < parsed_files.size(); ++i) {
    GetTransitiveDependencies(parsed_files[i], &already_seen,
                              file_set.mutable_file());
  }

  std::vector<std::string> output_filenames;
  for (const auto& pair : output_directories) {
    const std::string& location = pair.first;
    GeneratorContextImpl* directory = pair.second.get();
    std::vector<std::string> relative_output_filenames;
    directory->GetOutputFilenames(&relative_output_filenames);
    for (size_t i = 0; i < relative_output_filenames.size(); ++i) {
      std::string output_filename = location + relative_output_filenames[i];
      if (output_filename.compare(0, 2, "./") == 0) {
        output_filename = output_filename.substr(2);
      }
      output_filenames.push_back(output_filename);
    }
  }

  if (!descriptor_set_out_name_.empty()) {
    output_filenames.push_back(descriptor_set_out_name_);
  }

  if (!edition_defaults_out_name_.empty()) {
    output_filenames.push_back(edition_defaults_out_name_);
  }

  // Create the depfile, even if it will be empty.
  int fd;
  do {
    fd = open(dependency_out_name_.c_str(),
              O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  } while (fd < 0 && errno == EINTR);

  if (fd < 0) {
    perror(dependency_out_name_.c_str());
    return false;
  }

  // Only write to the depfile if there is at least one output_filename.
  // Otherwise, the depfile will be malformed.
  if (!output_filenames.empty()) {
    io::FileOutputStream out(fd);
    io::Printer printer(&out, '$');

    for (size_t i = 0; i < output_filenames.size(); ++i) {
      printer.Print(output_filenames[i]);
      if (i == output_filenames.size() - 1) {
        printer.Print(":");
      } else {
        printer.Print(" \\\n");
      }
    }

    for (int i = 0; i < file_set.file_size(); ++i) {
      const FileDescriptorProto& file = file_set.file(i);
      const std::string& virtual_file = file.name();
      std::string disk_file;
      if (source_tree &&
          source_tree->VirtualFileToDiskFile(virtual_file, &disk_file)) {
        printer.Print(" $disk_file$", "disk_file", disk_file);
        if (i < file_set.file_size() - 1) printer.Print("\\\n");
      } else {
        std::cerr << "Unable to identify path for file " << virtual_file
                  << std::endl;
        return false;
      }
    }
  }

  return true;
}

bool CommandLineInterface::GeneratePluginOutput(
    const std::vector<const FileDescriptor*>& parsed_files,
    const std::string& plugin_name, const std::string& parameter,
    GeneratorContext* generator_context, std::string* error) {
  CodeGeneratorRequest request;
  CodeGeneratorResponse response;
  std::string processed_parameter = parameter;

  bool bootstrap = GetBootstrapParam(processed_parameter);

  // Build the request.
  if (!processed_parameter.empty()) {
    request.set_parameter(processed_parameter);
  }


  absl::flat_hash_set<const FileDescriptor*> already_seen;
  for (const FileDescriptor* file : parsed_files) {
    request.add_file_to_generate(file->name());
    GetTransitiveDependencies(file, &already_seen, request.mutable_proto_file(),
                              {/*.include_json_name =*/true,
                               /*.include_source_code_info =*/true,
                               /*.retain_options =*/true});
  }

  // Populate source_file_descriptors and remove source-retention options from
  // proto_file.
  ABSL_CHECK(!parsed_files.empty());
  const DescriptorPool* pool = parsed_files[0]->pool();
  absl::flat_hash_set<std::string> files_to_generate(input_files_.begin(),
                                                     input_files_.end());
  static const auto builtin_plugins = new absl::flat_hash_set<std::string>(
      {"protoc-gen-cpp", "protoc-gen-java", "protoc-gen-mutable_java",
       "protoc-gen-python"});
  for (FileDescriptorProto& file_proto : *request.mutable_proto_file()) {
    if (files_to_generate.contains(file_proto.name())) {
      const FileDescriptor* file = pool->FindFileByName(file_proto.name());
      *request.add_source_file_descriptors() = std::move(file_proto);
      file->CopyTo(&file_proto);
      // Don't populate source code info or json_name for bootstrap protos.
      if (!bootstrap) {
        file->CopySourceCodeInfoTo(&file_proto);

        // The built-in code generators didn't use the json names.
        if (!builtin_plugins->contains(plugin_name)) {
          file->CopyJsonNameTo(&file_proto);
        }
      }
      StripSourceRetentionOptions(*file->pool(), file_proto);
    }
  }

  google::protobuf::compiler::Version* version =
      request.mutable_compiler_version();
  version->set_major(PROTOBUF_VERSION / 1000000);
  version->set_minor(PROTOBUF_VERSION / 1000 % 1000);
  version->set_patch(PROTOBUF_VERSION % 1000);
  version->set_suffix(PROTOBUF_VERSION_SUFFIX);

  // Invoke the plugin.
  Subprocess subprocess;

  if (plugins_.count(plugin_name) > 0) {
    subprocess.Start(plugins_[plugin_name], Subprocess::EXACT_NAME);
  } else {
    subprocess.Start(plugin_name, Subprocess::SEARCH_PATH);
  }

  std::string communicate_error;
  if (!subprocess.Communicate(request, &response, &communicate_error)) {
    *error = absl::Substitute("$0: $1", plugin_name, communicate_error);
    return false;
  }

  // Write the files.  We do this even if there was a generator error in order
  // to match the behavior of a compiled-in generator.
  std::unique_ptr<io::ZeroCopyOutputStream> current_output;
  for (int i = 0; i < response.file_size(); ++i) {
    const CodeGeneratorResponse::File& output_file = response.file(i);

    if (!output_file.insertion_point().empty()) {
      std::string filename = output_file.name();
      // Open a file for insert.
      // We reset current_output to nullptr first so that the old file is closed
      // before the new one is opened.
      current_output.reset();
      current_output.reset(
          generator_context->OpenForInsertWithGeneratedCodeInfo(
              filename, output_file.insertion_point(),
              output_file.generated_code_info()));
    } else if (!output_file.name().empty()) {
      // Starting a new file.  Open it.
      // We reset current_output to nullptr first so that the old file is closed
      // before the new one is opened.
      current_output.reset();
      current_output.reset(generator_context->Open(output_file.name()));
    } else if (current_output == nullptr) {
      *error = absl::Substitute(
          "$0: First file chunk returned by plugin did not specify a file "
          "name.",
          plugin_name);
      return false;
    }

    // Use CodedOutputStream for convenience; otherwise we'd need to provide
    // our own buffer-copying loop.
    io::CodedOutputStream writer(current_output.get());
    writer.WriteString(output_file.content());
  }

  // Check for errors.
  bool success = true;
  if (!EnforceProto3OptionalSupport(plugin_name, response.supported_features(),
                                    parsed_files)) {
    success = false;
  }
  if (!EnforceEditionsSupport(plugin_name, response.supported_features(),
                              static_cast<Edition>(response.minimum_edition()),
                              static_cast<Edition>(response.maximum_edition()),
                              parsed_files)) {
    success = false;
  }
  if (!response.error().empty()) {
    // Generator returned an error.
    *error = response.error();
    success = false;
  }

  return success;
}

bool CommandLineInterface::EncodeOrDecode(const DescriptorPool* pool) {
  // Look up the type.
  const Descriptor* type = pool->FindMessageTypeByName(codec_type_);
  if (type == nullptr) {
    std::cerr << "Type not defined: " << codec_type_ << std::endl;
    return false;
  }

  DynamicMessageFactory dynamic_factory(pool);
  std::unique_ptr<Message> message(dynamic_factory.GetPrototype(type)->New());

  if (mode_ == MODE_ENCODE) {
    SetFdToTextMode(STDIN_FILENO);
    SetFdToBinaryMode(STDOUT_FILENO);
  } else {
    SetFdToBinaryMode(STDIN_FILENO);
    SetFdToTextMode(STDOUT_FILENO);
  }

  io::FileInputStream in(STDIN_FILENO);
  io::FileOutputStream out(STDOUT_FILENO);

  if (mode_ == MODE_ENCODE) {
    // Input is text.
    ErrorPrinter error_collector(error_format_);
    TextFormat::Parser parser;
    parser.RecordErrorsTo(&error_collector);
    parser.AllowPartialMessage(true);

    if (!parser.Parse(&in, message.get())) {
      std::cerr << "Failed to parse input." << std::endl;
      return false;
    }
  } else {
    // Input is binary.
    if (!message->ParsePartialFromZeroCopyStream(&in)) {
      std::cerr << "Failed to parse input." << std::endl;
      return false;
    }
  }

  if (!message->IsInitialized()) {
    std::cerr << "warning:  Input message is missing required fields:  "
              << message->InitializationErrorString() << std::endl;
  }

  if (mode_ == MODE_ENCODE) {
    // Output is binary.
    io::CodedOutputStream coded_out(&out);
    coded_out.SetSerializationDeterministic(deterministic_output_);
    if (!message->SerializePartialToCodedStream(&coded_out)) {
      std::cerr << "output: I/O error." << std::endl;
      return false;
    }
  } else {
    // Output is text.
    if (!TextFormat::Print(*message, &out)) {
      std::cerr << "output: I/O error." << std::endl;
      return false;
    }
  }

  return true;
}

bool CommandLineInterface::WriteDescriptorSet(
    const std::vector<const FileDescriptor*>& parsed_files) {
  FileDescriptorSet file_set;

  absl::flat_hash_set<const FileDescriptor*> already_seen;
  if (!imports_in_descriptor_set_) {
    // Since we don't want to output transitive dependencies, but we do want
    // things to be in dependency order, add all dependencies that aren't in
    // parsed_files to already_seen.  This will short circuit the recursion
    // in GetTransitiveDependencies.
    absl::flat_hash_set<const FileDescriptor*> to_output;
    to_output.insert(parsed_files.begin(), parsed_files.end());
    for (size_t i = 0; i < parsed_files.size(); ++i) {
      const FileDescriptor* file = parsed_files[i];
      for (int j = 0; j < file->dependency_count(); j++) {
        const FileDescriptor* dependency = file->dependency(j);
        // if the dependency isn't in parsed files, mark it as already seen
        if (to_output.find(dependency) == to_output.end()) {
          already_seen.insert(dependency);
        }
      }
    }
  }
  TransitiveDependencyOptions options;
  options.include_json_name = true;
  options.include_source_code_info = source_info_in_descriptor_set_;
  options.retain_options = retain_options_in_descriptor_set_;
  for (size_t i = 0; i < parsed_files.size(); ++i) {
    GetTransitiveDependencies(parsed_files[i], &already_seen,
                              file_set.mutable_file(), options);
  }

  int fd;
  do {
    fd = open(descriptor_set_out_name_.c_str(),
              O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  } while (fd < 0 && errno == EINTR);

  if (fd < 0) {
    perror(descriptor_set_out_name_.c_str());
    return false;
  }

  io::FileOutputStream out(fd);

  {
    io::CodedOutputStream coded_out(&out);
    // Determinism is useful here because build outputs are sometimes checked
    // into version control.
    coded_out.SetSerializationDeterministic(true);
    if (!file_set.SerializeToCodedStream(&coded_out)) {
      std::cerr << descriptor_set_out_name_ << ": " << strerror(out.GetErrno())
                << std::endl;
      out.Close();
      return false;
    }
  }

  if (!out.Close()) {
    std::cerr << descriptor_set_out_name_ << ": " << strerror(out.GetErrno())
              << std::endl;
    return false;
  }

  return true;
}

bool CommandLineInterface::WriteEditionDefaults(const DescriptorPool& pool) {
  const Descriptor* feature_set;
  if (opensource_runtime_) {
    feature_set = pool.FindMessageTypeByName("google.protobuf.FeatureSet");
  } else {
    feature_set = pool.FindMessageTypeByName("google.protobuf.FeatureSet");
  }
  if (feature_set == nullptr) {
    std::cerr << edition_defaults_out_name_
              << ": Could not find FeatureSet in descriptor pool.  Please make "
                 "sure descriptor.proto is in your import path"
              << std::endl;
    return false;
  }
  std::vector<const FieldDescriptor*> extensions;
  pool.FindAllExtensions(feature_set, &extensions);

  Edition minimum = ProtocMinimumEdition();
  if (edition_defaults_minimum_ != EDITION_UNKNOWN) {
    minimum = edition_defaults_minimum_;
  }
  Edition maximum = ProtocMaximumEdition();
  if (edition_defaults_maximum_ != EDITION_UNKNOWN) {
    maximum = edition_defaults_maximum_;
  }

  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(feature_set, extensions, minimum,
                                       maximum);
  if (!defaults.ok()) {
    std::cerr << edition_defaults_out_name_ << ": "
              << defaults.status().message() << std::endl;
    return false;
  }

  int fd;
  do {
    fd = open(edition_defaults_out_name_.c_str(),
              O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  } while (fd < 0 && errno == EINTR);

  if (fd < 0) {
    perror(edition_defaults_out_name_.c_str());
    return false;
  }

  io::FileOutputStream out(fd);

  {
    io::CodedOutputStream coded_out(&out);
    // Determinism is useful here because build outputs are sometimes checked
    // into version control.
    coded_out.SetSerializationDeterministic(true);
    if (!defaults->SerializeToCodedStream(&coded_out)) {
      std::cerr << edition_defaults_out_name_ << ": "
                << strerror(out.GetErrno()) << std::endl;
      out.Close();
      return false;
    }
  }

  if (!out.Close()) {
    std::cerr << edition_defaults_out_name_ << ": " << strerror(out.GetErrno())
              << std::endl;
    return false;
  }

  return true;
}

const CommandLineInterface::GeneratorInfo*
CommandLineInterface::FindGeneratorByFlag(const std::string& name) const {
  auto it = generators_by_flag_name_.find(name);
  if (it == generators_by_flag_name_.end()) return nullptr;
  return &it->second;
}

const CommandLineInterface::GeneratorInfo*
CommandLineInterface::FindGeneratorByOption(const std::string& option) const {
  auto it = generators_by_option_name_.find(option);
  if (it == generators_by_option_name_.end()) return nullptr;
  return &it->second;
}

namespace {

// Utility function for PrintFreeFieldNumbers.
// Stores occupied ranges into the ranges parameter, and next level of sub
// message types into the nested_messages parameter.  The FieldRange is left
// inclusive, right exclusive. i.e. [a, b).
//
// Nested Messages:
// Note that it only stores the nested message type, iff the nested type is
// either a direct child of the given descriptor, or the nested type is a
// descendant of the given descriptor and all the nodes between the
// nested type and the given descriptor are group types. e.g.
//
// message Foo {
//   message Bar {
//     message NestedBar {}
//   }
//   group Baz = 1 {
//     group NestedBazGroup = 2 {
//       message Quz {
//         message NestedQuz {}
//       }
//     }
//     message NestedBaz {}
//   }
// }
//
// In this case, Bar, Quz and NestedBaz will be added into the nested types.
// Since free field numbers of group types will not be printed, this makes sure
// the nested message types in groups will not be dropped. The nested_messages
// parameter will contain the direct children (when groups are ignored in the
// tree) of the given descriptor for the caller to traverse. The declaration
// order of the nested messages is also preserved.
typedef std::pair<int, int> FieldRange;
void GatherOccupiedFieldRanges(
    const Descriptor* descriptor, absl::btree_set<FieldRange>* ranges,
    std::vector<const Descriptor*>* nested_messages) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    const FieldDescriptor* fd = descriptor->field(i);
    ranges->insert(FieldRange(fd->number(), fd->number() + 1));
  }
  for (int i = 0; i < descriptor->extension_range_count(); ++i) {
    ranges->insert(FieldRange(descriptor->extension_range(i)->start_number(),
                              descriptor->extension_range(i)->end_number()));
  }
  for (int i = 0; i < descriptor->reserved_range_count(); ++i) {
    ranges->insert(FieldRange(descriptor->reserved_range(i)->start,
                              descriptor->reserved_range(i)->end));
  }
  // Handle the nested messages/groups in declaration order to make it
  // post-order strict.
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    const Descriptor* nested_desc = descriptor->nested_type(i);
    nested_messages->push_back(nested_desc);
  }
}

// Utility function for PrintFreeFieldNumbers.
// Actually prints the formatted free field numbers for given message name and
// occupied ranges.
void FormatFreeFieldNumbers(absl::string_view name,
                            const absl::btree_set<FieldRange>& ranges) {
  std::string output;
  absl::StrAppendFormat(&output, "%-35s free:", name);
  int next_free_number = 1;
  for (const auto& range : ranges) {
    // This happens when groups re-use parent field numbers, in which
    // case we skip the FieldRange entirely.
    if (next_free_number >= range.second) continue;

    if (next_free_number < range.first) {
      if (next_free_number + 1 == range.first) {
        // Singleton
        absl::StrAppendFormat(&output, " %d", next_free_number);
      } else {
        // Range
        absl::StrAppendFormat(&output, " %d-%d", next_free_number,
                              range.first - 1);
      }
    }
    next_free_number = range.second;
  }
  if (next_free_number <= FieldDescriptor::kMaxNumber) {
    absl::StrAppendFormat(&output, " %d-INF", next_free_number);
  }
  std::cout << output << std::endl;
}

}  // namespace

void CommandLineInterface::PrintFreeFieldNumbers(const Descriptor* descriptor) {
  absl::btree_set<FieldRange> ranges;
  std::vector<const Descriptor*> nested_messages;
  GatherOccupiedFieldRanges(descriptor, &ranges, &nested_messages);

  for (size_t i = 0; i < nested_messages.size(); ++i) {
    PrintFreeFieldNumbers(nested_messages[i]);
  }
  FormatFreeFieldNumbers(descriptor->full_name(), ranges);
}


}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
