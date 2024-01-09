#ifndef GOOGLE_PROTOBUF_COMPILER_FILE_UTILS_H__
#define GOOGLE_PROTOBUF_COMPILER_FILE_UTILS_H__

#include <fcntl.h>

#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/io_win32.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

namespace google {
namespace protobuf {
namespace compiler {

#if defined(_WIN32)
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::close;
using google::protobuf::io::win32::open;
#endif

// Reads contents of the file at given path to string.
//
// `path` is passed to `open`, therefore it can be absolute, or relative to CWD.
inline absl::StatusOr<std::string> ReadFileToString(const std::string& path) {
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("Couldn't open mapping file ", path));
  }
  io::FileInputStream stream(fd);
  stream.SetCloseOnDelete(true);

  std::string text;
  const void* data;
  int size;
  while (stream.Next(&data, &size)) {
    text.append(reinterpret_cast<const char*>(data), size);
  }
  return text;
}

// Writes `text` to the file at a given path.
//
// `path` is passed to `open`, therefore it can be absolute, or relative to CWD.

inline absl::Status WriteStringToFile(const std::string& path,
                                      absl::string_view text) {
  int fd = open(path.c_str(), O_CREAT | O_WRONLY, 0644);
  if (fd < 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("Couldn't write file ", path));
  }

  io::FileOutputStream stream(fd);
  stream.SetCloseOnDelete(true);
  stream.WriteAliasedRaw(text.data(), text.size());
  return absl::OkStatus();
}
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_FILE_UTILS_H__
