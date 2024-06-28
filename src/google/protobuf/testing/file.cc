// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
// emulates google3/file/base/file.cc

#include "google/protobuf/testing/file.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN  // yeah, right
#include <windows.h>         // Find*File().  :(
// #include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif
#include <errno.h>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/io_win32.h"
#include "google/protobuf/stubs/status_macros.h"

// Needs to be last.
#include "google/protobuf/port_def.inc"  // NOLINT

namespace google {
namespace protobuf {

#ifdef _WIN32
// Windows doesn't have symbolic links.
#define lstat stat
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
#endif

#ifdef _WIN32
using google::protobuf::io::win32::access;
using google::protobuf::io::win32::chdir;
using google::protobuf::io::win32::fopen;
using google::protobuf::io::win32::mkdir;
using google::protobuf::io::win32::stat;
#endif

bool File::Exists(const std::string& name) {
  return access(name.c_str(), F_OK) == 0;
}

absl::Status File::ReadFileToString(const std::string& name,
                                    std::string* output, bool text_mode) {
  char buffer[1024];
  FILE* file = fopen(name.c_str(), text_mode ? "rt" : "rb");
  if (file == nullptr) return absl::NotFoundError("Could not open file");

  while (true) {
    size_t n = fread(buffer, 1, sizeof(buffer), file);
    if (n <= 0) break;
    output->append(buffer, n);
  }

  int error = ferror(file);
  if (fclose(file) != 0) return absl::InternalError("Failed to close file");
  if (error != 0) return absl::InternalError("Error parsing file");
  return absl::OkStatus();
}

void File::ReadFileToStringOrDie(const std::string& name, std::string* output) {
  ABSL_CHECK_OK(ReadFileToString(name, output)) << "Could not read: " << name;
}

absl::Status File::WriteStringToFile(absl::string_view contents,
                                     const std::string& name) {
  FILE* file = fopen(name.c_str(), "wb");
  if (file == nullptr) {
    return absl::InternalError(
        absl::StrCat("fopen(", name, ", \"wb\"): ", strerror(errno)));
  }

  if (fwrite(contents.data(), 1, contents.size(), file) != contents.size()) {
    fclose(file);
    return absl::InternalError(
        absl::StrCat("fwrite(", name, "): ", strerror(errno)));
  }

  if (fclose(file) != 0) {
    return absl::InternalError("Failed to close file");
  }
  return absl::OkStatus();
}

void File::WriteStringToFileOrDie(absl::string_view contents,
                                  const std::string& name) {
  FILE* file = fopen(name.c_str(), "wb");
  ABSL_CHECK(file != nullptr)
      << "fopen(" << name << ", \"wb\"): " << strerror(errno);
  ABSL_CHECK_EQ(fwrite(contents.data(), 1, contents.size(), file),
                contents.size())
      << "fwrite(" << name << "): " << strerror(errno);
  ABSL_CHECK(fclose(file) == 0)
      << "fclose(" << name << "): " << strerror(errno);
}

absl::Status File::CreateDir(const std::string& name, int mode) {
  if (!name.empty()) {
    ABSL_CHECK(name[name.size() - 1] != '.');
  }
  if (mkdir(name.c_str(), mode) != 0) {
    return absl::InternalError("Failed to create directory");
  }
  return absl::OkStatus();
}

absl::Status File::RecursivelyCreateDir(const std::string& path, int mode) {
  if (CreateDir(path, mode).ok()) return absl::OkStatus();

  if (Exists(path)) return absl::AlreadyExistsError("Path already exists");

  // Try creating the parent.
  std::string::size_type slashpos = path.find_last_of('/');
  if (slashpos == std::string::npos) {
    return absl::FailedPreconditionError("No parent given");
  }

  RETURN_IF_ERROR(RecursivelyCreateDir(path.substr(0, slashpos), mode));
  return CreateDir(path, mode);
}

void File::DeleteRecursively(const std::string& name, void* dummy1,
                             void* dummy2) {
  if (name.empty()) return;

  // We don't care too much about error checking here since this is only used
  // in tests to delete temporary directories that are under /tmp anyway.

#ifdef _MSC_VER
  // This interface is so weird.
  WIN32_FIND_DATAA find_data;
  HANDLE find_handle =
      FindFirstFileA(absl::StrCat(name, "/*").c_str(), &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    // Just delete it, whatever it is.
    DeleteFileA(name.c_str());
    RemoveDirectoryA(name.c_str());
    return;
  }

  do {
    std::string entry_name = find_data.cFileName;
    if (entry_name != "." && entry_name != "..") {
      std::string path = absl::StrCat(name, "/", entry_name);
      if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        DeleteRecursively(path, NULL, NULL);
        RemoveDirectoryA(path.c_str());
      } else {
        DeleteFileA(path.c_str());
      }
    }
  } while(FindNextFileA(find_handle, &find_data));
  FindClose(find_handle);

  RemoveDirectoryA(name.c_str());
#else
  // Use opendir()!  Yay!
  // lstat = Don't follow symbolic links.
  struct stat stats;
  if (lstat(name.c_str(), &stats) != 0) return;

  if (S_ISDIR(stats.st_mode)) {
    DIR* dir = opendir(name.c_str());
    if (dir != NULL) {
      while (true) {
        struct dirent* entry = readdir(dir);
        if (entry == NULL) break;
        std::string entry_name = entry->d_name;
        if (entry_name != "." && entry_name != "..") {
          DeleteRecursively(absl::StrCat(name, "/", entry_name), NULL, NULL);
        }
      }
    }

    closedir(dir);
    rmdir(name.c_str());

  } else if (S_ISREG(stats.st_mode)) {
    remove(name.c_str());
  }
#endif
}

bool File::ChangeWorkingDirectory(const std::string& new_working_directory) {
  return chdir(new_working_directory.c_str()) == 0;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"  // NOLINT
