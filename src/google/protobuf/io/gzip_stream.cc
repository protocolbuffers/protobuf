// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: brianolson@google.com (Brian Olson)
//
// This file contains the implementation of classes GzipInputStream and
// GzipOutputStream.


#if HAVE_ZLIB
#include "google/protobuf/io/gzip_stream.h"

#include "base/types.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "google/protobuf/port.h"

namespace google {
namespace protobuf {
namespace io {

static const int kDefaultBufferSize = 65536;

GzipInputStream::GzipInputStream(ZeroCopyInputStream* sub_stream, Format format,
                                 int buffer_size)
    : format_(format), sub_stream_(sub_stream), zerror_(Z_OK), byte_count_(0) {
  zcontext_.state = Z_NULL;
  zcontext_.zalloc = Z_NULL;
  zcontext_.zfree = Z_NULL;
  zcontext_.opaque = Z_NULL;
  zcontext_.total_out = 0;
  zcontext_.next_in = nullptr;
  zcontext_.avail_in = 0;
  zcontext_.total_in = 0;
  zcontext_.msg = nullptr;
  if (buffer_size == -1) {
    output_buffer_length_ = kDefaultBufferSize;
  } else {
    output_buffer_length_ = buffer_size;
  }
  output_buffer_ = operator new(output_buffer_length_);
  ABSL_CHECK(output_buffer_ != nullptr);
  zcontext_.next_out = static_cast<Bytef*>(output_buffer_);
  zcontext_.avail_out = output_buffer_length_;
  output_position_ = output_buffer_;
}
GzipInputStream::~GzipInputStream() {
  internal::SizedDelete(output_buffer_, output_buffer_length_);
  zerror_ = inflateEnd(&zcontext_);
}

static inline int internalInflateInit2(z_stream* zcontext,
                                       GzipInputStream::Format format) {
  int windowBitsFormat = 0;
  switch (format) {
    case GzipInputStream::GZIP:
      windowBitsFormat = 16;
      break;
    case GzipInputStream::AUTO:
      windowBitsFormat = 32;
      break;
    case GzipInputStream::ZLIB:
      windowBitsFormat = 0;
      break;
  }
  return inflateInit2(zcontext, /* windowBits */ 15 | windowBitsFormat);
}

int GzipInputStream::Inflate(int flush) {
  if ((zerror_ == Z_OK) && (zcontext_.avail_out == 0)) {
    // previous inflate filled output buffer. don't change input params yet.
  } else if (zcontext_.avail_in == 0) {
    const void* in;
    int in_size;
    bool first = zcontext_.next_in == NULL;
    bool ok = sub_stream_->Next(&in, &in_size);
    if (!ok) {
      zcontext_.next_out = NULL;
      zcontext_.avail_out = 0;
      return Z_STREAM_END;
    }
    zcontext_.next_in = static_cast<Bytef*>(const_cast<void*>(in));
    zcontext_.avail_in = in_size;
    if (first) {
      int error = internalInflateInit2(&zcontext_, format_);
      if (error != Z_OK) {
        return error;
      }
    }
  }
  zcontext_.next_out = static_cast<Bytef*>(output_buffer_);
  zcontext_.avail_out = output_buffer_length_;
  output_position_ = output_buffer_;
  int error = inflate(&zcontext_, flush);
  return error;
}

void GzipInputStream::DoNextOutput(const void** data, int* size) {
  *data = output_position_;
  *size = ((uintptr_t)zcontext_.next_out) - ((uintptr_t)output_position_);
  output_position_ = zcontext_.next_out;
}

// implements ZeroCopyInputStream ----------------------------------
bool GzipInputStream::Next(const void** data, int* size) {
  bool ok = (zerror_ == Z_OK) || (zerror_ == Z_STREAM_END) ||
            (zerror_ == Z_BUF_ERROR);
  if ((!ok) || (zcontext_.next_out == NULL)) {
    return false;
  }
  if (zcontext_.next_out != output_position_) {
    DoNextOutput(data, size);
    return true;
  }
  if (zerror_ == Z_STREAM_END) {
    if (zcontext_.next_out != NULL) {
      // sub_stream_ may have concatenated streams to follow
      zerror_ = inflateEnd(&zcontext_);
      byte_count_ += zcontext_.total_out;
      if (zerror_ != Z_OK) {
        return false;
      }
      zerror_ = internalInflateInit2(&zcontext_, format_);
      if (zerror_ != Z_OK) {
        return false;
      }
    } else {
      *data = NULL;
      *size = 0;
      return false;
    }
  }
  zerror_ = Inflate(Z_NO_FLUSH);
  if ((zerror_ == Z_STREAM_END) && (zcontext_.next_out == NULL)) {
    // The underlying stream's Next returned false inside Inflate.
    return false;
  }
  ok = (zerror_ == Z_OK) || (zerror_ == Z_STREAM_END) ||
       (zerror_ == Z_BUF_ERROR);
  if (!ok) {
    return false;
  }
  DoNextOutput(data, size);
  return true;
}
void GzipInputStream::BackUp(int count) {
  output_position_ = reinterpret_cast<void*>(
      reinterpret_cast<uintptr_t>(output_position_) - count);
}
bool GzipInputStream::Skip(int count) {
  const void* data;
  int size = 0;
  bool ok = Next(&data, &size);
  while (ok && (size < count)) {
    count -= size;
    ok = Next(&data, &size);
  }
  if (size > count) {
    BackUp(size - count);
  }
  return ok;
}
int64_t GzipInputStream::ByteCount() const {
  int64_t ret = byte_count_ + zcontext_.total_out;
  if (zcontext_.next_out != NULL && output_position_ != NULL) {
    ret += reinterpret_cast<uintptr_t>(zcontext_.next_out) -
           reinterpret_cast<uintptr_t>(output_position_);
  }
  return ret;
}

// =========================================================================

GzipOutputStream::Options::Options()
    : format(GZIP),
      buffer_size(kDefaultBufferSize),
      compression_level(Z_DEFAULT_COMPRESSION),
      compression_strategy(Z_DEFAULT_STRATEGY) {}

GzipOutputStream::GzipOutputStream(ZeroCopyOutputStream* sub_stream) {
  Init(sub_stream, Options());
}

GzipOutputStream::GzipOutputStream(ZeroCopyOutputStream* sub_stream,
                                   const Options& options) {
  Init(sub_stream, options);
}

void GzipOutputStream::Init(ZeroCopyOutputStream* sub_stream,
                            const Options& options) {
  sub_stream_ = sub_stream;
  sub_data_ = NULL;
  sub_data_size_ = 0;

  input_buffer_length_ = options.buffer_size;
  input_buffer_ = operator new(input_buffer_length_);
  ABSL_CHECK(input_buffer_ != NULL);

  zcontext_.zalloc = Z_NULL;
  zcontext_.zfree = Z_NULL;
  zcontext_.opaque = Z_NULL;
  zcontext_.next_out = NULL;
  zcontext_.avail_out = 0;
  zcontext_.total_out = 0;
  zcontext_.next_in = NULL;
  zcontext_.avail_in = 0;
  zcontext_.total_in = 0;
  zcontext_.msg = NULL;
  // default to GZIP format
  int windowBitsFormat = 16;
  if (options.format == ZLIB) {
    windowBitsFormat = 0;
  }
  zerror_ =
      deflateInit2(&zcontext_, options.compression_level, Z_DEFLATED,
                   /* windowBits */ 15 | windowBitsFormat,
                   /* memLevel (default) */ 8, options.compression_strategy);
}

GzipOutputStream::~GzipOutputStream() {
  Close();
  internal::SizedDelete(input_buffer_, input_buffer_length_);
}

// private
int GzipOutputStream::Deflate(int flush) {
  int error = Z_OK;
  do {
    if ((sub_data_ == NULL) || (zcontext_.avail_out == 0)) {
      bool ok = sub_stream_->Next(&sub_data_, &sub_data_size_);
      if (!ok) {
        sub_data_ = NULL;
        sub_data_size_ = 0;
        return Z_BUF_ERROR;
      }
      ABSL_CHECK_GT(sub_data_size_, 0);
      zcontext_.next_out = static_cast<Bytef*>(sub_data_);
      zcontext_.avail_out = sub_data_size_;
    }
    error = deflate(&zcontext_, flush);
  } while (error == Z_OK && zcontext_.avail_out == 0);
  if ((flush == Z_FULL_FLUSH) || (flush == Z_FINISH)) {
    // Notify lower layer of data.
    sub_stream_->BackUp(zcontext_.avail_out);
    // We don't own the buffer anymore.
    sub_data_ = NULL;
    sub_data_size_ = 0;
  }
  return error;
}

// implements ZeroCopyOutputStream ---------------------------------
bool GzipOutputStream::Next(void** data, int* size) {
  if ((zerror_ != Z_OK) && (zerror_ != Z_BUF_ERROR)) {
    return false;
  }
  if (zcontext_.avail_in != 0) {
    zerror_ = Deflate(Z_NO_FLUSH);
    if (zerror_ != Z_OK) {
      return false;
    }
  }
  if (zcontext_.avail_in == 0) {
    // all input was consumed. reset the buffer.
    zcontext_.next_in = static_cast<Bytef*>(input_buffer_);
    zcontext_.avail_in = input_buffer_length_;
    *data = input_buffer_;
    *size = input_buffer_length_;
  } else {
    // The loop in Deflate should consume all avail_in
    ABSL_DLOG(FATAL) << "Deflate left bytes unconsumed";
  }
  return true;
}
void GzipOutputStream::BackUp(int count) {
  ABSL_CHECK_GE(zcontext_.avail_in, static_cast<uInt>(count));
  zcontext_.avail_in -= count;
}
int64_t GzipOutputStream::ByteCount() const {
  return zcontext_.total_in + zcontext_.avail_in;
}

bool GzipOutputStream::Flush() {
  zerror_ = Deflate(Z_FULL_FLUSH);
  // Return true if the flush succeeded or if it was a no-op.
  return (zerror_ == Z_OK) ||
         (zerror_ == Z_BUF_ERROR && zcontext_.avail_in == 0 &&
          zcontext_.avail_out != 0);
}

bool GzipOutputStream::Close() {
  if ((zerror_ != Z_OK) && (zerror_ != Z_BUF_ERROR)) {
    return false;
  }
  do {
    zerror_ = Deflate(Z_FINISH);
  } while (zerror_ == Z_OK);
  zerror_ = deflateEnd(&zcontext_);
  bool ok = zerror_ == Z_OK;
  zerror_ = Z_STREAM_END;
  return ok;
}

}  // namespace io
}  // namespace protobuf
}  // namespace google

#endif  // HAVE_ZLIB
