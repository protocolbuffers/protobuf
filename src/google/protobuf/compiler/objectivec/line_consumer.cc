// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <errno.h>
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <cstring>
#include <string>

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/objectivec/line_consumer.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

#ifdef _WIN32
#include "google/protobuf/io/io_win32.h"
#endif

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// <io.h> is transitively included in this file. Import the functions explicitly
// in this port namespace to avoid ambiguous definition.
namespace posix {
#ifdef _WIN32
using google::protobuf::io::win32::open;
#else   // !_WIN32
using ::open;
#endif  // _WIN32
}  // namespace posix

namespace {

bool ascii_isnewline(char c) { return c == '\n' || c == '\r'; }

bool ReadLine(absl::string_view* input, absl::string_view* line) {
  for (int len = 0; len < input->size(); ++len) {
    if (ascii_isnewline((*input)[len])) {
      *line = absl::string_view(input->data(), len);
      ++len;  // advance over the newline
      *input = absl::string_view(input->data() + len, input->size() - len);
      return true;
    }
  }
  return false;  // Ran out of input with no newline.
}

void RemoveComment(absl::string_view* input) {
  int offset = input->find('#');
  if (offset != absl::string_view::npos) {
    input->remove_suffix(input->length() - offset);
  }
}

class Parser {
 public:
  explicit Parser(LineConsumer* line_consumer)
      : line_consumer_(line_consumer), line_(0) {}

  // Feeds in some input, parse what it can, returning success/failure. Calling
  // again after an error is undefined.
  bool ParseChunk(absl::string_view chunk, std::string* out_error);

  // Should be called to finish parsing (after all input has been provided via
  // successful calls to ParseChunk(), calling after a ParseChunk() failure is
  // undefined). Returns success/failure.
  bool Finish(std::string* out_error);

  int last_line() const { return line_; }

 private:
  LineConsumer* line_consumer_;
  int line_;
  std::string leftover_;
};

bool Parser::ParseChunk(absl::string_view chunk, std::string* out_error) {
  absl::string_view full_chunk;
  if (!leftover_.empty()) {
    leftover_ += std::string(chunk);
    full_chunk = absl::string_view(leftover_);
  } else {
    full_chunk = chunk;
  }

  absl::string_view line;
  while (ReadLine(&full_chunk, &line)) {
    ++line_;
    RemoveComment(&line);
    line = absl::StripAsciiWhitespace(line);
    if (!line.empty() && !line_consumer_->ConsumeLine(line, out_error)) {
      if (out_error->empty()) {
        *out_error = "ConsumeLine failed without setting an error.";
      }
      leftover_.clear();
      return false;
    }
  }

  if (full_chunk.empty()) {
    leftover_.clear();
  } else {
    leftover_ = std::string(full_chunk);
  }
  return true;
}

bool Parser::Finish(std::string* out_error) {
  // If there is still something to go, flush it with a newline.
  if (!leftover_.empty() && !ParseChunk("\n", out_error)) {
    return false;
  }
  // This really should never fail if ParseChunk succeeded, but check to be
  // sure.
  if (!leftover_.empty()) {
    *out_error = "ParseSimple Internal error: finished with pending data.";
    return false;
  }
  return true;
}

}  // namespace

bool ParseSimpleFile(absl::string_view path, LineConsumer* line_consumer,
                     std::string* out_error) {
  int fd;
  do {
    fd = posix::open(std::string(path).c_str(), O_RDONLY);
  } while (fd < 0 && errno == EINTR);
  if (fd < 0) {
    *out_error =
        absl::StrCat("error: Unable to open \"", path, "\", ", strerror(errno));
    return false;
  }
  io::FileInputStream file_stream(fd);
  file_stream.SetCloseOnDelete(true);

  return ParseSimpleStream(file_stream, path, line_consumer, out_error);
}

bool ParseSimpleStream(io::ZeroCopyInputStream& input_stream,
                       absl::string_view stream_name,
                       LineConsumer* line_consumer, std::string* out_error) {
  std::string local_error;
  Parser parser(line_consumer);
  const void* buf;
  int buf_len;
  while (input_stream.Next(&buf, &buf_len)) {
    if (buf_len == 0) {
      continue;
    }

    if (!parser.ParseChunk(
            absl::string_view(static_cast<const char*>(buf), buf_len),
            &local_error)) {
      *out_error = absl::StrCat("error: ", stream_name, " Line ",
                                parser.last_line(), ", ", local_error);
      return false;
    }
  }
  if (!parser.Finish(&local_error)) {
    *out_error = absl::StrCat("error: ", stream_name, " Line ",
                              parser.last_line(), ", ", local_error);
    return false;
  }
  return true;
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
