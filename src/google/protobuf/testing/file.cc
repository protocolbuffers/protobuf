// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
// emulates google3/file/base/file.cc

#include <google/protobuf/testing/file.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN  // yeah, right
#include <windows.h>         // Find*File().  :(
#include <io.h>
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif
#include <errno.h>

namespace google {
namespace protobuf {

#ifdef _WIN32
#define mkdir(name, mode) mkdir(name)
// Windows doesn't have symbolic links.
#define lstat stat
#ifndef F_OK
#define F_OK 00  // not defined by MSVC for whatever reason
#endif
#endif

bool File::Exists(const string& name) {
  return access(name.c_str(), F_OK) == 0;
}

bool File::ReadFileToString(const string& name, string* output) {
  char buffer[1024];
  FILE* file = fopen(name.c_str(), "rb");
  if (file == NULL) return false;

  while (true) {
    size_t n = fread(buffer, 1, sizeof(buffer), file);
    if (n <= 0) break;
    output->append(buffer, n);
  }

  int error = ferror(file);
  if (fclose(file) != 0) return false;
  return error == 0;
}

void File::ReadFileToStringOrDie(const string& name, string* output) {
  GOOGLE_CHECK(ReadFileToString(name, output)) << "Could not read: " << name;
}

void File::WriteStringToFileOrDie(const string& contents, const string& name) {
  FILE* file = fopen(name.c_str(), "wb");
  GOOGLE_CHECK(file != NULL);
  GOOGLE_CHECK_EQ(fwrite(contents.data(), 1, contents.size(), file),
                  contents.size());
  GOOGLE_CHECK(fclose(file) == 0);
}

bool File::CreateDir(const string& name, int mode) {
  return mkdir(name.c_str(), mode) == 0;
}

bool File::RecursivelyCreateDir(const string& path, int mode) {
  if (CreateDir(path, mode)) return true;

  // Try creating the parent.
  string::size_type slashpos = path.find_first_of('/');
  if (slashpos == string::npos) {
    // No parent given.
    return false;
  }

  return RecursivelyCreateDir(path.substr(0, slashpos), mode) &&
         CreateDir(path, mode);
}

void File::DeleteRecursively(const string& name,
                             void* dummy1, void* dummy2) {
  // We don't care too much about error checking here since this is only used
  // in tests to delete temporary directories that are under /tmp anyway.

#ifdef _MSC_VER
  // This interface is so weird.
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFile((name + "/*").c_str(), &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    // Just delete it, whatever it is.
    DeleteFile(name.c_str());
    RemoveDirectory(name.c_str());
    return;
  }

  do {
    string entry_name = find_data.cFileName;
    if (entry_name != "." && entry_name != "..") {
      string path = name + "/" + entry_name;
      if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        DeleteRecursively(path, NULL, NULL);
        RemoveDirectory(path.c_str());
      } else {
        DeleteFile(path.c_str());
      }
    }
  } while(FindNextFile(find_handle, &find_data));
  FindClose(find_handle);

  RemoveDirectory(name.c_str());
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
        string entry_name = entry->d_name;
        if (entry_name != "." && entry_name != "..") {
          DeleteRecursively(name + "/" + entry_name, NULL, NULL);
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

}  // namespace protobuf
}  // namespace google
