// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: laszlocsomor@google.com (Laszlo Csomor)
//
// Unit tests for long-path-aware open/mkdir/access on Windows.
//
// This file is only used on Windows, it's empty on other platforms.

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wchar.h>
#include <windows.h>

#include <google/protobuf/stubs/io_win32.h>
#include <google/protobuf/stubs/scoped_ptr.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>

namespace google {
namespace protobuf {
namespace internal {
namespace win32 {
namespace {

using std::string;
using std::wstring;

class IoWin32Test : public ::testing::Test {
 public:
  void SetUp();
  void TearDown();

 protected:
  bool CreateAllUnder(wstring path);
  bool DeleteAllUnder(wstring path);

  string test_tmpdir;
  wstring wtest_tmpdir;
};

#define ASSERT_INITIALIZED              \
  {                                     \
    EXPECT_FALSE(test_tmpdir.empty());  \
    EXPECT_FALSE(wtest_tmpdir.empty()); \
  }

namespace {
void StripTrailingSlashes(string* str) {
  int i = str->size() - 1;
  for (; i >= 0 && ((*str)[i] == '/' || (*str)[i] == '\\'); --i) {}
  str->resize(i+1);
}
}  // namespace

void IoWin32Test::SetUp() {
  test_tmpdir = string(TestTempDir());
  wtest_tmpdir.clear();
  if (test_tmpdir.empty()) {
    const char* test_tmpdir_env = getenv("TEST_TMPDIR");
    if (test_tmpdir_env != NULL && *test_tmpdir_env) {
      test_tmpdir = string(test_tmpdir_env);
    }

    // Only Bazel defines TEST_TMPDIR, CMake does not, so look for other
    // suitable environment variables.
    if (test_tmpdir.empty()) {
      static const char* names[] = {"TEMP", "TMP"};
      for (int i = 0; i < sizeof(names)/sizeof(names[0]); ++i) {
        const char* name = names[i];
        test_tmpdir_env = getenv(name);
        if (test_tmpdir_env != NULL && *test_tmpdir_env) {
          test_tmpdir = string(test_tmpdir_env);
          break;
        }
      }
    }

    // No other temp directory was found. Use the current director
    if (test_tmpdir.empty()) {
      char buffer[MAX_PATH];
      // Use GetCurrentDirectoryA instead of GetCurrentDirectoryW, because the
      // current working directory must always be shorter than MAX_PATH, even
      // with
      // "\\?\" prefix (except on Windows 10 version 1607 and beyond, after
      // opting in to long paths by default [1]).
      //
      // [1] https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#maxpath
      DWORD result = ::GetCurrentDirectoryA(MAX_PATH, buffer);
      if (result > 0) {
        test_tmpdir = string(buffer);
      } else {
        // Using assertions in SetUp/TearDown seems to confuse the test
        // framework, so just leave the member variables empty in case of
        // failure.
        GOOGLE_CHECK_OK(false);
        return;
      }
    }
  }

  StripTrailingSlashes(&test_tmpdir);
  test_tmpdir += "\\io_win32_unittest.tmp";

  // CreateDirectoryA's limit is 248 chars, see MSDN.
  // https://msdn.microsoft.com/en-us/library/windows/desktop/aa363855(v=vs.85).aspx
  wtest_tmpdir = testonly_path_to_winpath(test_tmpdir);
  if (!DeleteAllUnder(wtest_tmpdir) || !CreateAllUnder(wtest_tmpdir)) {
    GOOGLE_CHECK_OK(false);
    test_tmpdir.clear();
    wtest_tmpdir.clear();
  }
}

void IoWin32Test::TearDown() {
  if (!wtest_tmpdir.empty()) {
    DeleteAllUnder(wtest_tmpdir);
  }
}

bool IoWin32Test::CreateAllUnder(wstring path) {
  // Prepend UNC prefix if the path doesn't have it already. Don't bother
  // checking if the path is shorter than MAX_PATH, let's just do it
  // unconditionally.
  if (path.find(L"\\\\?\\") != 0) {
    path = wstring(L"\\\\?\\") + path;
  }
  if (::CreateDirectoryW(path.c_str(), NULL) ||
      GetLastError() == ERROR_ALREADY_EXISTS ||
      GetLastError() == ERROR_ACCESS_DENIED) {
    return true;
  }
  if (GetLastError() == ERROR_PATH_NOT_FOUND) {
    size_t pos = path.find_last_of(L'\\');
    if (pos != wstring::npos) {
      wstring parent(path, 0, pos);
      if (CreateAllUnder(parent) && CreateDirectoryW(path.c_str(), NULL)) {
        return true;
      }
    }
  }
  return false;
}

bool IoWin32Test::DeleteAllUnder(wstring path) {
  static const wstring kDot(L".");
  static const wstring kDotDot(L"..");

  // Prepend UNC prefix if the path doesn't have it already. Don't bother
  // checking if the path is shorter than MAX_PATH, let's just do it
  // unconditionally.
  if (path.find(L"\\\\?\\") != 0) {
    path = wstring(L"\\\\?\\") + path;
  }
  // Append "\" if necessary.
  if (path[path.size() - 1] != '\\') {
    path.push_back('\\');
  }

  WIN32_FIND_DATAW metadata;
  HANDLE handle = ::FindFirstFileW((path + L"*").c_str(), &metadata);
  if (handle == INVALID_HANDLE_VALUE) {
    return true;  // directory doesn't exist
  }

  bool result = true;
  do {
    wstring childname = metadata.cFileName;
    if (kDot != childname && kDotDot != childname) {
      wstring childpath = path + childname;
      if ((metadata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        // If this is not a junction, delete its contents recursively.
        // Finally delete this directory/junction too.
        if (((metadata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 &&
             !DeleteAllUnder(childpath)) ||
            !::RemoveDirectoryW(childpath.c_str())) {
          result = false;
          break;
        }
      } else {
        if (!::DeleteFileW(childpath.c_str())) {
          result = false;
          break;
        }
      }
    }
  } while (::FindNextFileW(handle, &metadata));
  ::FindClose(handle);
  return result;
}

TEST_F(IoWin32Test, AccessTest) {
  ASSERT_INITIALIZED;

  string path = test_tmpdir;
  while (path.size() < MAX_PATH - 30) {
    path += "\\accesstest";
    EXPECT_EQ(mkdir(path.c_str(), 0644), 0);
  }
  string file = path + "\\file.txt";
  int fd = open(file.c_str(), O_CREAT | O_WRONLY, 0644);
  if (fd > 0) {
    EXPECT_EQ(close(fd), 0);
  } else {
    EXPECT_TRUE(false);
  }

  EXPECT_EQ(access(test_tmpdir.c_str(), F_OK), 0);
  EXPECT_EQ(access(path.c_str(), F_OK), 0);
  EXPECT_EQ(access(path.c_str(), W_OK), 0);
  EXPECT_EQ(access(file.c_str(), F_OK | W_OK), 0);
  EXPECT_NE(access((file + ".blah").c_str(), F_OK), 0);
  EXPECT_NE(access((file + ".blah").c_str(), W_OK), 0);

  EXPECT_EQ(access(".", F_OK), 0);
  EXPECT_EQ(access(".", W_OK), 0);
  EXPECT_EQ(access((test_tmpdir + "/accesstest").c_str(), F_OK | W_OK), 0);
  ASSERT_EQ(access((test_tmpdir + "/./normalize_me/.././accesstest").c_str(),
                   F_OK | W_OK),
            0);
  EXPECT_NE(access("io_win32_unittest.AccessTest.nonexistent", F_OK), 0);
  EXPECT_NE(access("io_win32_unittest.AccessTest.nonexistent", W_OK), 0);

  ASSERT_EQ(access("c:bad", F_OK), -1);
  ASSERT_EQ(errno, ENOENT);
  ASSERT_EQ(access("/tmp/bad", F_OK), -1);
  ASSERT_EQ(errno, ENOENT);
  ASSERT_EQ(access("\\bad", F_OK), -1);
  ASSERT_EQ(errno, ENOENT);
}

TEST_F(IoWin32Test, OpenTest) {
  ASSERT_INITIALIZED;

  string path = test_tmpdir;
  while (path.size() < MAX_PATH) {
    path += "\\opentest";
    EXPECT_EQ(mkdir(path.c_str(), 0644), 0);
  }
  string file = path + "\\file.txt";
  int fd = open(file.c_str(), O_CREAT | O_WRONLY, 0644);
  if (fd > 0) {
    EXPECT_EQ(write(fd, "hello", 5), 5);
    EXPECT_EQ(close(fd), 0);
  } else {
    EXPECT_TRUE(false);
  }

  ASSERT_EQ(open("c:bad.txt", O_CREAT | O_WRONLY, 0644), -1);
  ASSERT_EQ(errno, ENOENT);
  ASSERT_EQ(open("/tmp/bad.txt", O_CREAT | O_WRONLY, 0644), -1);
  ASSERT_EQ(errno, ENOENT);
  ASSERT_EQ(open("\\bad.txt", O_CREAT | O_WRONLY, 0644), -1);
  ASSERT_EQ(errno, ENOENT);
}

TEST_F(IoWin32Test, MkdirTest) {
  ASSERT_INITIALIZED;

  string path = test_tmpdir;
  do {
    path += "\\mkdirtest";
    ASSERT_EQ(mkdir(path.c_str(), 0644), 0);
  } while (path.size() <= MAX_PATH);

  ASSERT_EQ(mkdir("c:bad", 0644), -1);
  ASSERT_EQ(errno, ENOENT);
  ASSERT_EQ(mkdir("/tmp/bad", 0644), -1);
  ASSERT_EQ(errno, ENOENT);
  ASSERT_EQ(mkdir("\\bad", 0644), -1);
  ASSERT_EQ(errno, ENOENT);
}

TEST_F(IoWin32Test, ChdirTest) {
  char owd[MAX_PATH];
  EXPECT_GT(::GetCurrentDirectoryA(MAX_PATH, owd), 0);
  string path("C:\\");
  EXPECT_EQ(access(path.c_str(), F_OK), 0);
  ASSERT_EQ(chdir(path.c_str()), 0);
  EXPECT_TRUE(::SetCurrentDirectoryA(owd));

  // Do not try to chdir into the test_tmpdir, it may already contain directory
  // names with trailing dots.
  // Instead test here with an obviously dot-trailed path. If the win32_chdir
  // function would not convert the path to absolute and prefix with "\\?\" then
  // the Win32 API would ignore the trailing dot, but because of the prefixing
  // there'll be no path processing done, so we'll actually attempt to chdir
  // into "C:\some\path\foo."
  path = test_tmpdir + "/foo.";
  EXPECT_EQ(mkdir(path.c_str(), 644), 0);
  EXPECT_EQ(access(path.c_str(), F_OK), 0);
  ASSERT_NE(chdir(path.c_str()), 0);
}

TEST_F(IoWin32Test, AsWindowsPathTest) {
  DWORD size = GetCurrentDirectoryW(0, NULL);
  scoped_array<wchar_t> cwd_str(new wchar_t[size]);
  EXPECT_GT(GetCurrentDirectoryW(size, cwd_str.get()), 0);
  wstring cwd = wstring(L"\\\\?\\") + cwd_str.get();

  ASSERT_EQ(testonly_path_to_winpath("relative_mkdirtest"),
            cwd + L"\\relative_mkdirtest");
  ASSERT_EQ(testonly_path_to_winpath("preserve//\\trailing///"),
            cwd + L"\\preserve\\trailing\\");
  ASSERT_EQ(testonly_path_to_winpath("./normalize_me\\/../blah"),
            cwd + L"\\blah");
  std::ostringstream relpath;
  for (wchar_t* p = cwd_str.get(); *p; ++p) {
    if (*p == '/' || *p == '\\') {
      relpath << "../";
    }
  }
  relpath << ".\\/../\\./beyond-toplevel";
  ASSERT_EQ(testonly_path_to_winpath(relpath.str()),
            wstring(L"\\\\?\\") + cwd_str.get()[0] + L":\\beyond-toplevel");

  // Absolute unix paths lack drive letters, driveless absolute windows paths
  // do too. Neither can be converted to a drive-specifying absolute Windows
  // path.
  ASSERT_EQ(testonly_path_to_winpath("/absolute/unix/path"), L"");
  // Though valid on Windows, we also don't support UNC paths (\\UNC\\blah).
  ASSERT_EQ(testonly_path_to_winpath("\\driveless\\absolute"), L"");
  // Though valid in cmd.exe, drive-relative paths are not supported.
  ASSERT_EQ(testonly_path_to_winpath("c:foo"), L"");
  ASSERT_EQ(testonly_path_to_winpath("c:/foo"), L"\\\\?\\c:\\foo");
}

}  // namespace
}  // namespace win32
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // defined(_WIN32)

