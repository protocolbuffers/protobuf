// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <errno.h>
#include <iostream>
#include <algorithm>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/stl_util-inl.h>

namespace google {
namespace protobuf {
namespace io {

#ifdef _WIN32
// Win32 lseek is broken:  If invoked on a non-seekable file descriptor, its
// return value is undefined.  We re-define it to always produce an error.
#define lseek(fd, offset, origin) ((off_t)-1)
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

// Default block size for Copying{In,Out}putStreamAdaptor.
static const int kDefaultBlockSize = 8192;

}  // namespace

// ===================================================================

ArrayInputStream::ArrayInputStream(const void* data, int size,
                                   int block_size)
  : data_(reinterpret_cast<const uint8*>(data)),
    size_(size),
    block_size_(block_size > 0 ? block_size : size),
    position_(0),
    last_returned_size_(0) {
}

ArrayInputStream::~ArrayInputStream() {
}

bool ArrayInputStream::Next(const void** data, int* size) {
  if (position_ < size_) {
    last_returned_size_ = min(block_size_, size_ - position_);
    *data = data_ + position_;
    *size = last_returned_size_;
    position_ += last_returned_size_;
    return true;
  } else {
    // We're at the end of the array.
    last_returned_size_ = 0;   // Don't let caller back up.
    return false;
  }
}

void ArrayInputStream::BackUp(int count) {
  GOOGLE_CHECK_GT(last_returned_size_, 0)
      << "BackUp() can only be called after a successful Next().";
  GOOGLE_CHECK_LE(count, last_returned_size_);
  GOOGLE_CHECK_GE(count, 0);
  position_ -= count;
  last_returned_size_ = 0;  // Don't let caller back up further.
}

bool ArrayInputStream::Skip(int count) {
  GOOGLE_CHECK_GE(count, 0);
  last_returned_size_ = 0;   // Don't let caller back up.
  if (count > size_ - position_) {
    position_ = size_;
    return false;
  } else {
    position_ += count;
    return true;
  }
}

int64 ArrayInputStream::ByteCount() const {
  return position_;
}


// ===================================================================

ArrayOutputStream::ArrayOutputStream(void* data, int size, int block_size)
  : data_(reinterpret_cast<uint8*>(data)),
    size_(size),
    block_size_(block_size > 0 ? block_size : size),
    position_(0),
    last_returned_size_(0) {
}

ArrayOutputStream::~ArrayOutputStream() {
}

bool ArrayOutputStream::Next(void** data, int* size) {
  if (position_ < size_) {
    last_returned_size_ = min(block_size_, size_ - position_);
    *data = data_ + position_;
    *size = last_returned_size_;
    position_ += last_returned_size_;
    return true;
  } else {
    // We're at the end of the array.
    last_returned_size_ = 0;   // Don't let caller back up.
    return false;
  }
}

void ArrayOutputStream::BackUp(int count) {
  GOOGLE_CHECK_GT(last_returned_size_, 0)
      << "BackUp() can only be called after a successful Next().";
  GOOGLE_CHECK_LE(count, last_returned_size_);
  GOOGLE_CHECK_GE(count, 0);
  position_ -= count;
  last_returned_size_ = 0;  // Don't let caller back up further.
}

int64 ArrayOutputStream::ByteCount() const {
  return position_;
}

// ===================================================================

StringOutputStream::StringOutputStream(string* target)
  : target_(target) {
}

StringOutputStream::~StringOutputStream() {
}

bool StringOutputStream::Next(void** data, int* size) {
  int old_size = target_->size();

  // Grow the string.
  if (old_size < target_->capacity()) {
    // Resize the string to match its capacity, since we can get away
    // without a memory allocation this way.
    STLStringResizeUninitialized(target_, target_->capacity());
  } else {
    // Size has reached capacity, so double the size.  Also make sure
    // that the new size is at least kMinimumSize.
    STLStringResizeUninitialized(
      target_,
      max(old_size * 2,
          kMinimumSize + 0));  // "+ 0" works around GCC4 weirdness.
  }

  *data = string_as_array(target_) + old_size;
  *size = target_->size() - old_size;
  return true;
}

void StringOutputStream::BackUp(int count) {
  GOOGLE_CHECK_GE(count, 0);
  GOOGLE_CHECK_LE(count, target_->size());
  target_->resize(target_->size() - count);
}

int64 StringOutputStream::ByteCount() const {
  return target_->size();
}

// ===================================================================


// ===================================================================

CopyingInputStream::~CopyingInputStream() {}

int CopyingInputStream::Skip(int count) {
  char junk[4096];
  int skipped = 0;
  while (skipped < count) {
    int bytes = Read(junk, min(count - skipped,
                               implicit_cast<int>(sizeof(junk))));
    if (bytes <= 0) {
      // EOF or read error.
      return skipped;
    }
    skipped += bytes;
  }
  return skipped;
}

CopyingInputStreamAdaptor::CopyingInputStreamAdaptor(
    CopyingInputStream* copying_stream, int block_size)
  : copying_stream_(copying_stream),
    owns_copying_stream_(false),
    failed_(false),
    position_(0),
    buffer_size_(block_size > 0 ? block_size : kDefaultBlockSize),
    buffer_used_(0),
    backup_bytes_(0) {
}

CopyingInputStreamAdaptor::~CopyingInputStreamAdaptor() {
  if (owns_copying_stream_) {
    delete copying_stream_;
  }
}

bool CopyingInputStreamAdaptor::Next(const void** data, int* size) {
  if (failed_) {
    // Already failed on a previous read.
    return false;
  }

  AllocateBufferIfNeeded();

  if (backup_bytes_ > 0) {
    // We have data left over from a previous BackUp(), so just return that.
    *data = buffer_.get() + buffer_used_ - backup_bytes_;
    *size = backup_bytes_;
    backup_bytes_ = 0;
    return true;
  }

  // Read new data into the buffer.
  buffer_used_ = copying_stream_->Read(buffer_.get(), buffer_size_);
  if (buffer_used_ <= 0) {
    // EOF or read error.  We don't need the buffer anymore.
    if (buffer_used_ < 0) {
      // Read error (not EOF).
      failed_ = true;
    }
    FreeBuffer();
    return false;
  }
  position_ += buffer_used_;

  *size = buffer_used_;
  *data = buffer_.get();
  return true;
}

void CopyingInputStreamAdaptor::BackUp(int count) {
  GOOGLE_CHECK(backup_bytes_ == 0 && buffer_.get() != NULL)
    << " BackUp() can only be called after Next().";
  GOOGLE_CHECK_LE(count, buffer_used_)
    << " Can't back up over more bytes than were returned by the last call"
       " to Next().";
  GOOGLE_CHECK_GE(count, 0)
    << " Parameter to BackUp() can't be negative.";

  backup_bytes_ = count;
}

bool CopyingInputStreamAdaptor::Skip(int count) {
  GOOGLE_CHECK_GE(count, 0);

  if (failed_) {
    // Already failed on a previous read.
    return false;
  }

  // First skip any bytes left over from a previous BackUp().
  if (backup_bytes_ >= count) {
    // We have more data left over than we're trying to skip.  Just chop it.
    backup_bytes_ -= count;
    return true;
  }

  count -= backup_bytes_;
  backup_bytes_ = 0;

  int skipped = copying_stream_->Skip(count);
  position_ += skipped;
  return skipped == count;
}

int64 CopyingInputStreamAdaptor::ByteCount() const {
  return position_ - backup_bytes_;
}

void CopyingInputStreamAdaptor::AllocateBufferIfNeeded() {
  if (buffer_.get() == NULL) {
    buffer_.reset(new uint8[buffer_size_]);
  }
}

void CopyingInputStreamAdaptor::FreeBuffer() {
  GOOGLE_CHECK_EQ(backup_bytes_, 0);
  buffer_used_ = 0;
  buffer_.reset();
}

// ===================================================================

CopyingOutputStream::~CopyingOutputStream() {}

CopyingOutputStreamAdaptor::CopyingOutputStreamAdaptor(
    CopyingOutputStream* copying_stream, int block_size)
  : copying_stream_(copying_stream),
    owns_copying_stream_(false),
    failed_(false),
    position_(0),
    buffer_size_(block_size > 0 ? block_size : kDefaultBlockSize),
    buffer_used_(0) {
}

CopyingOutputStreamAdaptor::~CopyingOutputStreamAdaptor() {
  WriteBuffer();
  if (owns_copying_stream_) {
    delete copying_stream_;
  }
}

bool CopyingOutputStreamAdaptor::Flush() {
  return WriteBuffer();
}

bool CopyingOutputStreamAdaptor::Next(void** data, int* size) {
  if (buffer_used_ == buffer_size_) {
    if (!WriteBuffer()) return false;
  }

  AllocateBufferIfNeeded();

  *data = buffer_.get() + buffer_used_;
  *size = buffer_size_ - buffer_used_;
  buffer_used_ = buffer_size_;
  return true;
}

void CopyingOutputStreamAdaptor::BackUp(int count) {
  GOOGLE_CHECK_GE(count, 0);
  GOOGLE_CHECK_EQ(buffer_used_, buffer_size_)
    << " BackUp() can only be called after Next().";
  GOOGLE_CHECK_LE(count, buffer_used_)
    << " Can't back up over more bytes than were returned by the last call"
       " to Next().";

  buffer_used_ -= count;
}

int64 CopyingOutputStreamAdaptor::ByteCount() const {
  return position_ + buffer_used_;
}

bool CopyingOutputStreamAdaptor::WriteBuffer() {
  if (failed_) {
    // Already failed on a previous write.
    return false;
  }

  if (buffer_used_ == 0) return true;

  if (copying_stream_->Write(buffer_.get(), buffer_used_)) {
    position_ += buffer_used_;
    buffer_used_ = 0;
    return true;
  } else {
    failed_ = true;
    FreeBuffer();
    return false;
  }
}

void CopyingOutputStreamAdaptor::AllocateBufferIfNeeded() {
  if (buffer_ == NULL) {
    buffer_.reset(new uint8[buffer_size_]);
  }
}

void CopyingOutputStreamAdaptor::FreeBuffer() {
  buffer_used_ = 0;
  buffer_.reset();
}

// ===================================================================

FileInputStream::FileInputStream(int file_descriptor, int block_size)
  : copying_input_(file_descriptor),
    impl_(&copying_input_, block_size) {
}

FileInputStream::~FileInputStream() {}

bool FileInputStream::Close() {
  return copying_input_.Close();
}

bool FileInputStream::Next(const void** data, int* size) {
  return impl_.Next(data, size);
}

void FileInputStream::BackUp(int count) {
  impl_.BackUp(count);
}

bool FileInputStream::Skip(int count) {
  return impl_.Skip(count);
}

int64 FileInputStream::ByteCount() const {
  return impl_.ByteCount();
}

FileInputStream::CopyingFileInputStream::CopyingFileInputStream(
    int file_descriptor)
  : file_(file_descriptor),
    close_on_delete_(false),
    is_closed_(false),
    errno_(0),
    previous_seek_failed_(false) {
}

FileInputStream::CopyingFileInputStream::~CopyingFileInputStream() {
  if (close_on_delete_) {
    if (!Close()) {
      GOOGLE_LOG(ERROR) << "close() failed: " << strerror(errno_);
    }
  }
}

bool FileInputStream::CopyingFileInputStream::Close() {
  GOOGLE_CHECK(!is_closed_);

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
  GOOGLE_CHECK(!is_closed_);

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
  GOOGLE_CHECK(!is_closed_);

  if (!previous_seek_failed_ &&
      lseek(file_, count, SEEK_CUR) != (off_t)-1) {
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
  : copying_output_(file_descriptor),
    impl_(&copying_output_, block_size) {
}

FileOutputStream::~FileOutputStream() {
  impl_.Flush();
}

bool FileOutputStream::Close() {
  bool flush_succeeded = impl_.Flush();
  return copying_output_.Close() && flush_succeeded;
}

bool FileOutputStream::Flush() {
  return impl_.Flush();
}

bool FileOutputStream::Next(void** data, int* size) {
  return impl_.Next(data, size);
}

void FileOutputStream::BackUp(int count) {
  impl_.BackUp(count);
}

int64 FileOutputStream::ByteCount() const {
  return impl_.ByteCount();
}

FileOutputStream::CopyingFileOutputStream::CopyingFileOutputStream(
    int file_descriptor)
  : file_(file_descriptor),
    close_on_delete_(false),
    is_closed_(false),
    errno_(0) {
}

FileOutputStream::CopyingFileOutputStream::~CopyingFileOutputStream() {
  if (close_on_delete_) {
    if (!Close()) {
      GOOGLE_LOG(ERROR) << "close() failed: " << strerror(errno_);
    }
  }
}

bool FileOutputStream::CopyingFileOutputStream::Close() {
  GOOGLE_CHECK(!is_closed_);

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

bool FileOutputStream::CopyingFileOutputStream::Write(
    const void* buffer, int size) {
  GOOGLE_CHECK(!is_closed_);
  int total_written = 0;

  const uint8* buffer_base = reinterpret_cast<const uint8*>(buffer);

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

IstreamInputStream::IstreamInputStream(istream* input, int block_size)
  : copying_input_(input),
    impl_(&copying_input_, block_size) {
}

IstreamInputStream::~IstreamInputStream() {}

bool IstreamInputStream::Next(const void** data, int* size) {
  return impl_.Next(data, size);
}

void IstreamInputStream::BackUp(int count) {
  impl_.BackUp(count);
}

bool IstreamInputStream::Skip(int count) {
  return impl_.Skip(count);
}

int64 IstreamInputStream::ByteCount() const {
  return impl_.ByteCount();
}

IstreamInputStream::CopyingIstreamInputStream::CopyingIstreamInputStream(
    istream* input)
  : input_(input) {
}

IstreamInputStream::CopyingIstreamInputStream::~CopyingIstreamInputStream() {}

int IstreamInputStream::CopyingIstreamInputStream::Read(
    void* buffer, int size) {
  input_->read(reinterpret_cast<char*>(buffer), size);
  int result = input_->gcount();
  if (result == 0 && input_->fail() && !input_->eof()) {
    return -1;
  }
  return result;
}

// ===================================================================

OstreamOutputStream::OstreamOutputStream(ostream* output, int block_size)
  : copying_output_(output),
    impl_(&copying_output_, block_size) {
}

OstreamOutputStream::~OstreamOutputStream() {
  impl_.Flush();
}

bool OstreamOutputStream::Next(void** data, int* size) {
  return impl_.Next(data, size);
}

void OstreamOutputStream::BackUp(int count) {
  impl_.BackUp(count);
}

int64 OstreamOutputStream::ByteCount() const {
  return impl_.ByteCount();
}

OstreamOutputStream::CopyingOstreamOutputStream::CopyingOstreamOutputStream(
    ostream* output)
  : output_(output) {
}

OstreamOutputStream::CopyingOstreamOutputStream::~CopyingOstreamOutputStream() {
}

bool OstreamOutputStream::CopyingOstreamOutputStream::Write(
    const void* buffer, int size) {
  output_->write(reinterpret_cast<const char*>(buffer), size);
  return output_->good();
}

// ===================================================================

ConcatenatingInputStream::ConcatenatingInputStream(
    ZeroCopyInputStream* const streams[], int count)
  : streams_(streams), stream_count_(count), bytes_retired_(0) {
}

ConcatenatingInputStream::~ConcatenatingInputStream() {
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
    GOOGLE_LOG(DFATAL) << "Can't BackUp() after failed Next().";
  }
}

bool ConcatenatingInputStream::Skip(int count) {
  while (stream_count_ > 0) {
    // Assume that ByteCount() can be used to find out how much we actually
    // skipped when Skip() fails.
    int64 target_byte_count = streams_[0]->ByteCount() + count;
    if (streams_[0]->Skip(count)) return true;

    // Hit the end of the stream.  Figure out how many more bytes we still have
    // to skip.
    int64 final_byte_count = streams_[0]->ByteCount();
    GOOGLE_DCHECK_LT(final_byte_count, target_byte_count);
    count = target_byte_count - final_byte_count;

    // That stream is done.  Advance to the next one.
    bytes_retired_ += final_byte_count;
    ++streams_;
    --stream_count_;
  }

  return false;
}

int64 ConcatenatingInputStream::ByteCount() const {
  if (stream_count_ == 0) {
    return bytes_retired_;
  } else {
    return bytes_retired_ + streams_[0]->ByteCount();
  }
}


// ===================================================================

LimitingInputStream::LimitingInputStream(ZeroCopyInputStream* input,
                                         int64 limit)
  : input_(input), limit_(limit) {}

LimitingInputStream::~LimitingInputStream() {
  // If we overshot the limit, back up.
  if (limit_ < 0) input_->BackUp(-limit_);
}

bool LimitingInputStream::Next(const void** data, int* size) {
  if (limit_ <= 0) return false;
  if (!input_->Next(data, size)) return false;

  limit_ -= *size;
  if (limit_ < 0) {
    // We overshot the limit.  Reduce *size to hide the rest of the buffer.
    *size += limit_;
  }
  return true;
}

void LimitingInputStream::BackUp(int count) {
  if (limit_ < 0) {
    input_->BackUp(count - limit_);
    limit_ = count;
  } else {
    input_->BackUp(count);
    limit_ += count;
  }
}

bool LimitingInputStream::Skip(int count) {
  if (count > limit_) {
    if (limit_ < 0) return false;
    input_->Skip(limit_);
    limit_ = 0;
    return false;
  } else {
    if (!input_->Skip(count)) return false;
    limit_ -= count;
    return true;
  }
}

int64 LimitingInputStream::ByteCount() const {
  if (limit_ < 0) {
    return input_->ByteCount() + limit_;
  } else {
    return input_->ByteCount();
  }
}


// ===================================================================

}  // namespace io
}  // namespace protobuf
}  // namespace google
