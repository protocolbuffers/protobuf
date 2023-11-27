// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef _MSC_VER
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <errno.h>

#include <algorithm>
#include <iostream>

#include "google/protobuf/stubs/common.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "google/protobuf/io/io_win32.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"


namespace google {
namespace protobuf {
namespace io {

#ifdef _WIN32
// Win32 lseek is broken:  If invoked on a non-seekable file descriptor, its
// return value is undefined.  We re-define it to always produce an error.
#define lseek(fd, offset, origin) ((off_t)-1)
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::access;
using google::protobuf::io::win32::close;
using google::protobuf::io::win32::open;
using google::protobuf::io::win32::read;
using google::protobuf::io::win32::write;
#endif

namespace {

// EINTR sucks.
int close_no_eintr(int fd) {
  int result;
  do {
    result = close(fd);
  } while (result < 0 && errno == EINTR);
  return result;
}

}  // namespace

// ===================================================================

FileInputStream::FileInputStream(int file_descriptor, int block_size)
    : copying_input_(file_descriptor), impl_(&copying_input_, block_size) {}

bool FileInputStream::Close() { return copying_input_.Close(); }

bool FileInputStream::Next(const void** data, int* size) {
  return impl_.Next(data, size);
}

void FileInputStream::BackUp(int count) { impl_.BackUp(count); }

bool FileInputStream::Skip(int count) { return impl_.Skip(count); }

int64_t FileInputStream::ByteCount() const { return impl_.ByteCount(); }

FileInputStream::CopyingFileInputStream::CopyingFileInputStream(
    int file_descriptor)
    : file_(file_descriptor),
      close_on_delete_(false),
      is_closed_(false),
      errno_(0),
      previous_seek_failed_(false) {
#ifndef _WIN32
  int flags = fcntl(file_, F_GETFL);
  flags &= ~O_NONBLOCK;
  fcntl(file_, F_SETFL, flags);
#endif
}

FileInputStream::CopyingFileInputStream::~CopyingFileInputStream() {
  if (close_on_delete_) {
    if (!Close()) {
      ABSL_LOG(ERROR) << "close() failed: " << strerror(errno_);
    }
  }
}

bool FileInputStream::CopyingFileInputStream::Close() {
  ABSL_CHECK(!is_closed_);

  is_closed_ = true;
  if (close_no_eintr(file_) != 0) {
    // The docs on close() do not specify whether a file descriptor is still
    // open after close() fails with EIO.  However, the glibc source code
    // seems to indicate that it is not.
    errno_ = errno;
    return false;
  }

  return true;
}

int FileInputStream::CopyingFileInputStream::Read(void* buffer, int size) {
  ABSL_CHECK(!is_closed_);

  int result;
  do {
    result = read(file_, buffer, size);
  } while (result < 0 && errno == EINTR);

  if (result < 0) {
    // Read error (not EOF).
    errno_ = errno;
  }

  return result;
}

int FileInputStream::CopyingFileInputStream::Skip(int count) {
  ABSL_CHECK(!is_closed_);

  if (!previous_seek_failed_ && lseek(file_, count, SEEK_CUR) != (off_t)-1) {
    // Seek succeeded.
    return count;
  } else {
    // Failed to seek.

    // Note to self:  Don't seek again.  This file descriptor doesn't
    // support it.
    previous_seek_failed_ = true;

    // Use the default implementation.
    return CopyingInputStream::Skip(count);
  }
}

// ===================================================================

FileOutputStream::FileOutputStream(int file_descriptor, int block_size)
    : CopyingOutputStreamAdaptor(&copying_output_, block_size),
      copying_output_(file_descriptor) {}

bool FileOutputStream::Close() {
  bool flush_succeeded = Flush();
  return copying_output_.Close() && flush_succeeded;
}

FileOutputStream::CopyingFileOutputStream::CopyingFileOutputStream(
    int file_descriptor)
    : file_(file_descriptor),
      close_on_delete_(false),
      is_closed_(false),
      errno_(0) {}

FileOutputStream::~FileOutputStream() { Flush(); }

FileOutputStream::CopyingFileOutputStream::~CopyingFileOutputStream() {
  if (close_on_delete_) {
    if (!Close()) {
      ABSL_LOG(ERROR) << "close() failed: " << strerror(errno_);
    }
  }
}

bool FileOutputStream::CopyingFileOutputStream::Close() {
  ABSL_CHECK(!is_closed_);

  is_closed_ = true;
  if (close_no_eintr(file_) != 0) {
    // The docs on close() do not specify whether a file descriptor is still
    // open after close() fails with EIO.  However, the glibc source code
    // seems to indicate that it is not.
    errno_ = errno;
    return false;
  }

  return true;
}

bool FileOutputStream::CopyingFileOutputStream::Write(const void* buffer,
                                                      int size) {
  ABSL_CHECK(!is_closed_);
  int total_written = 0;

  const uint8_t* buffer_base = reinterpret_cast<const uint8_t*>(buffer);

  while (total_written < size) {
    int bytes;
    do {
      bytes = write(file_, buffer_base + total_written, size - total_written);
    } while (bytes < 0 && errno == EINTR);

    if (bytes <= 0) {
      // Write error.

      // FIXME(kenton):  According to the man page, if write() returns zero,
      //   there was no error; write() simply did not write anything.  It's
      //   unclear under what circumstances this might happen, but presumably
      //   errno won't be set in this case.  I am confused as to how such an
      //   event should be handled.  For now I'm treating it as an error, since
      //   retrying seems like it could lead to an infinite loop.  I suspect
      //   this never actually happens anyway.

      if (bytes < 0) {
        errno_ = errno;
      }
      return false;
    }
    total_written += bytes;
  }

  return true;
}

// ===================================================================

IstreamInputStream::IstreamInputStream(std::istream* input, int block_size)
    : copying_input_(input), impl_(&copying_input_, block_size) {}

bool IstreamInputStream::Next(const void** data, int* size) {
  return impl_.Next(data, size);
}

void IstreamInputStream::BackUp(int count) { impl_.BackUp(count); }

bool IstreamInputStream::Skip(int count) { return impl_.Skip(count); }

int64_t IstreamInputStream::ByteCount() const { return impl_.ByteCount(); }

IstreamInputStream::CopyingIstreamInputStream::CopyingIstreamInputStream(
    std::istream* input)
    : input_(input) {}

IstreamInputStream::CopyingIstreamInputStream::~CopyingIstreamInputStream() {}

int IstreamInputStream::CopyingIstreamInputStream::Read(void* buffer,
                                                        int size) {
  input_->read(reinterpret_cast<char*>(buffer), size);
  int result = input_->gcount();
  if (result == 0 && input_->fail() && !input_->eof()) {
    return -1;
  }
  return result;
}

// ===================================================================

OstreamOutputStream::OstreamOutputStream(std::ostream* output, int block_size)
    : copying_output_(output), impl_(&copying_output_, block_size) {}

OstreamOutputStream::~OstreamOutputStream() { impl_.Flush(); }

bool OstreamOutputStream::Next(void** data, int* size) {
  return impl_.Next(data, size);
}

void OstreamOutputStream::BackUp(int count) { impl_.BackUp(count); }

int64_t OstreamOutputStream::ByteCount() const { return impl_.ByteCount(); }

OstreamOutputStream::CopyingOstreamOutputStream::CopyingOstreamOutputStream(
    std::ostream* output)
    : output_(output) {}

OstreamOutputStream::CopyingOstreamOutputStream::~CopyingOstreamOutputStream() {
}

bool OstreamOutputStream::CopyingOstreamOutputStream::Write(const void* buffer,
                                                            int size) {
  output_->write(reinterpret_cast<const char*>(buffer), size);
  return output_->good();
}

// ===================================================================

ConcatenatingInputStream::ConcatenatingInputStream(
    ZeroCopyInputStream* const streams[], int count)
    : streams_(streams), stream_count_(count), bytes_retired_(0) {
}

bool ConcatenatingInputStream::Next(const void** data, int* size) {
  while (stream_count_ > 0) {
    if (streams_[0]->Next(data, size)) return true;

    // That stream is done.  Advance to the next one.
    bytes_retired_ += streams_[0]->ByteCount();
    ++streams_;
    --stream_count_;
  }

  // No more streams.
  return false;
}

void ConcatenatingInputStream::BackUp(int count) {
  if (stream_count_ > 0) {
    streams_[0]->BackUp(count);
  } else {
    ABSL_DLOG(FATAL) << "Can't BackUp() after failed Next().";
  }
}

bool ConcatenatingInputStream::Skip(int count) {
  while (stream_count_ > 0) {
    // Assume that ByteCount() can be used to find out how much we actually
    // skipped when Skip() fails.
    int64_t target_byte_count = streams_[0]->ByteCount() + count;
    if (streams_[0]->Skip(count)) return true;

    // Hit the end of the stream.  Figure out how many more bytes we still have
    // to skip.
    int64_t final_byte_count = streams_[0]->ByteCount();
    ABSL_DCHECK_LT(final_byte_count, target_byte_count);
    count = target_byte_count - final_byte_count;

    // That stream is done.  Advance to the next one.
    bytes_retired_ += final_byte_count;
    ++streams_;
    --stream_count_;
  }

  return false;
}

int64_t ConcatenatingInputStream::ByteCount() const {
  if (stream_count_ == 0) {
    return bytes_retired_;
  } else {
    return bytes_retired_ + streams_[0]->ByteCount();
  }
}


// ===================================================================

}  // namespace io
}  // namespace protobuf
}  // namespace google
