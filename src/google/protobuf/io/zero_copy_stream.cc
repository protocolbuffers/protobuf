// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/io/zero_copy_stream.h"

#include <cstring>
#include <utility>

#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/cord_buffer.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace io {

bool ZeroCopyInputStream::ReadCord(absl::Cord* cord, int count) {
  if (count <= 0) return true;

  absl::CordBuffer cord_buffer = cord->GetAppendBuffer(count);
  absl::Span<char> out = cord_buffer.available_up_to(count);

  auto FetchNextChunk = [&]() -> absl::Span<const char> {
    const void* buffer;
    int size;
    if (!Next(&buffer, &size)) return {};

    if (size > count) {
      BackUp(size - count);
      size = count;
    }
    return absl::MakeConstSpan(static_cast<const char*>(buffer), size);
  };

  auto AppendFullBuffer = [&]() -> absl::Span<char> {
    cord->Append(std::move(cord_buffer));
    cord_buffer = absl::CordBuffer::CreateWithDefaultLimit(count);
    return cord_buffer.available_up_to(count);
  };

  auto CopyBytes = [&](absl::Span<char>& dst, absl::Span<const char>& src,
                       size_t bytes) {
    memcpy(dst.data(), src.data(), bytes);
    dst.remove_prefix(bytes);
    src.remove_prefix(bytes);
    count -= bytes;
    cord_buffer.IncreaseLengthBy(bytes);
  };

  do {
    absl::Span<const char> in = FetchNextChunk();
    if (in.empty()) {
      // Append whatever we have pending so far.
      cord->Append(std::move(cord_buffer));
      return false;
    }

    if (out.empty()) out = AppendFullBuffer();

    while (in.size() > out.size()) {
      CopyBytes(out, in, out.size());
      out = AppendFullBuffer();
    }

    CopyBytes(out, in, in.size());
  } while (count > 0);

  cord->Append(std::move(cord_buffer));
  return true;
}

bool ZeroCopyOutputStream::WriteCord(const absl::Cord& cord) {
  if (cord.empty()) return true;

  void* buffer;
  int buffer_size = 0;
  if (!Next(&buffer, &buffer_size)) return false;

  for (absl::string_view fragment : cord.Chunks()) {
    while (fragment.size() > static_cast<size_t>(buffer_size)) {
      std::memcpy(buffer, fragment.data(), buffer_size);

      fragment.remove_prefix(buffer_size);

      if (!Next(&buffer, &buffer_size)) return false;
    }
    std::memcpy(buffer, fragment.data(), fragment.size());

    // Advance the buffer.
    buffer = static_cast<char*>(buffer) + fragment.size();
    buffer_size -= static_cast<int>(fragment.size());
  }
  BackUp(buffer_size);
  return true;
}


bool ZeroCopyOutputStream::WriteAliasedRaw(const void* /* data */,
                                           int /* size */) {
  ABSL_LOG(FATAL) << "This ZeroCopyOutputStream doesn't support aliasing. "
                     "Reaching here usually means a ZeroCopyOutputStream "
                     "implementation bug.";
  return false;
}

}  // namespace io
}  // namespace protobuf
}  // namespace google
