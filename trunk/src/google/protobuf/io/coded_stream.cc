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
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This implementation is heavily optimized to make reads and writes
// of small values (especially varints) as fast as possible.  In
// particular, we optimize for the common case that a read or a write
// will not cross the end of the buffer, since we can avoid a lot
// of branching in this case.

#include <stack>
#include <limits.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/stl_util-inl.h>


namespace google {
namespace protobuf {
namespace io {

namespace {

static const int kDefaultTotalBytesLimit = 64 << 20;  // 64MB

static const int kDefaultTotalBytesWarningThreshold = 32 << 20;  // 32MB
static const int kDefaultRecursionLimit = 64;

static const int kMaxVarintBytes = 10;
static const int kMaxVarint32Bytes = 5;


}  // namespace

// CodedInputStream ==================================================

CodedInputStream::CodedInputStream(ZeroCopyInputStream* input)
  : input_(input),
    buffer_(NULL),
    buffer_size_(0),
    total_bytes_read_(0),
    overflow_bytes_(0),

    last_tag_(0),
    legitimate_message_end_(false),

    aliasing_enabled_(false),

    current_limit_(INT_MAX),
    buffer_size_after_limit_(0),

    total_bytes_limit_(kDefaultTotalBytesLimit),
    total_bytes_warning_threshold_(kDefaultTotalBytesWarningThreshold),

    recursion_depth_(0),
    recursion_limit_(kDefaultRecursionLimit) {
}

CodedInputStream::~CodedInputStream() {
  int backup_bytes = buffer_size_ + buffer_size_after_limit_ + overflow_bytes_;
  if (backup_bytes > 0) {
    // We still have bytes left over from the last buffer.  Back up over
    // them.
    input_->BackUp(backup_bytes);
  }
}


inline void CodedInputStream::RecomputeBufferLimits() {
  buffer_size_ += buffer_size_after_limit_;
  int closest_limit = min(current_limit_, total_bytes_limit_);
  if (closest_limit < total_bytes_read_) {
    // The limit position is in the current buffer.  We must adjust
    // the buffer size accordingly.
    buffer_size_after_limit_ = total_bytes_read_ - closest_limit;
    buffer_size_ -= buffer_size_after_limit_;
  } else {
    buffer_size_after_limit_ = 0;
  }
}

CodedInputStream::Limit CodedInputStream::PushLimit(int byte_limit) {
  // Current position relative to the beginning of the stream.
  int current_position = total_bytes_read_ -
      (buffer_size_ + buffer_size_after_limit_);

  Limit old_limit = current_limit_;

  // security: byte_limit is possibly evil, so check for negative values
  // and overflow.
  if (byte_limit >= 0 &&
      byte_limit <= INT_MAX - current_position) {
    current_limit_ = current_position + byte_limit;
  } else {
    // Negative or overflow.
    current_limit_ = INT_MAX;
  }

  // We need to enforce all limits, not just the new one, so if the previous
  // limit was before the new requested limit, we continue to enforce the
  // previous limit.
  current_limit_ = min(current_limit_, old_limit);

  RecomputeBufferLimits();
  return old_limit;
}

void CodedInputStream::PopLimit(Limit limit) {
  // The limit passed in is actually the *old* limit, which we returned from
  // PushLimit().
  current_limit_ = limit;
  RecomputeBufferLimits();

  // We may no longer be at a legitimate message end.  ReadTag() needs to be
  // called again to find out.
  legitimate_message_end_ = false;
}

int CodedInputStream::BytesUntilLimit() {
  if (current_limit_ == INT_MAX) return -1;
  int current_position = total_bytes_read_ -
      (buffer_size_ + buffer_size_after_limit_);

  return current_limit_ - current_position;
}

void CodedInputStream::SetTotalBytesLimit(
    int total_bytes_limit, int warning_threshold) {
  // Make sure the limit isn't already past, since this could confuse other
  // code.
  int current_position = total_bytes_read_ -
      (buffer_size_ + buffer_size_after_limit_);
  total_bytes_limit_ = max(current_position, total_bytes_limit);
  total_bytes_warning_threshold_ = warning_threshold;
  RecomputeBufferLimits();
}

void CodedInputStream::PrintTotalBytesLimitError() {
  GOOGLE_LOG(ERROR) << "A protocol message was rejected because it was too "
                "big (more than " << total_bytes_limit_
             << " bytes).  To increase the limit (or to disable these "
                "warnings), see CodedInputStream::SetTotalBytesLimit() "
                "in google/protobuf/io/coded_stream.h.";
}

bool CodedInputStream::Skip(int count) {
  if (count < 0) return false;  // security: count is often user-supplied

  if (count <= buffer_size_) {
    // Just skipping within the current buffer.  Easy.
    Advance(count);
    return true;
  }

  if (buffer_size_after_limit_ > 0) {
    // We hit a limit inside this buffer.  Advance to the limit and fail.
    Advance(buffer_size_);
    return false;
  }

  count -= buffer_size_;
  buffer_ = NULL;
  buffer_size_ = 0;

  // Make sure this skip doesn't try to skip past the current limit.
  int closest_limit = min(current_limit_, total_bytes_limit_);
  int bytes_until_limit = closest_limit - total_bytes_read_;
  if (bytes_until_limit < count) {
    // We hit the limit.  Skip up to it then fail.
    total_bytes_read_ = closest_limit;
    input_->Skip(bytes_until_limit);
    return false;
  }

  total_bytes_read_ += count;
  return input_->Skip(count);
}

bool CodedInputStream::ReadRaw(void* buffer, int size) {
  while (buffer_size_ < size) {
    // Reading past end of buffer.  Copy what we have, then refresh.
    memcpy(buffer, buffer_, buffer_size_);
    buffer = reinterpret_cast<uint8*>(buffer) + buffer_size_;
    size -= buffer_size_;
    if (!Refresh()) return false;
  }

  memcpy(buffer, buffer_, size);
  Advance(size);

  return true;
}

bool CodedInputStream::ReadString(string* buffer, int size) {
  if (size < 0) return false;  // security: size is often user-supplied

  if (!buffer->empty()) {
    buffer->clear();
  }

  if (size < buffer_size_) {
    STLStringResizeUninitialized(buffer, size);
    memcpy((uint8*)buffer->data(), buffer_, size);
    Advance(size);
    return true;
  }

  while (buffer_size_ < size) {
    // Some STL implementations "helpfully" crash on buffer->append(NULL, 0).
    if (buffer_size_ != 0) {
      // Note:  string1.append(string2) is O(string2.size()) (as opposed to
      //   O(string1.size() + string2.size()), which would be bad).
      buffer->append(reinterpret_cast<const char*>(buffer_), buffer_size_);
    }
    size -= buffer_size_;
    if (!Refresh()) return false;
  }

  buffer->append(reinterpret_cast<const char*>(buffer_), size);
  Advance(size);

  return true;
}


bool CodedInputStream::ReadLittleEndian32(uint32* value) {
  uint8 bytes[sizeof(*value)];

  const uint8* ptr;
  if (buffer_size_ >= sizeof(*value)) {
    // Fast path:  Enough bytes in the buffer to read directly.
    ptr = buffer_;
    Advance(sizeof(*value));
  } else {
    // Slow path:  Had to read past the end of the buffer.
    if (!ReadRaw(bytes, sizeof(*value))) return false;
    ptr = bytes;
  }

  *value = (static_cast<uint32>(ptr[0])      ) |
           (static_cast<uint32>(ptr[1]) <<  8) |
           (static_cast<uint32>(ptr[2]) << 16) |
           (static_cast<uint32>(ptr[3]) << 24);
  return true;
}

bool CodedInputStream::ReadLittleEndian64(uint64* value) {
  uint8 bytes[sizeof(*value)];

  const uint8* ptr;
  if (buffer_size_ >= sizeof(*value)) {
    // Fast path:  Enough bytes in the buffer to read directly.
    ptr = buffer_;
    Advance(sizeof(*value));
  } else {
    // Slow path:  Had to read past the end of the buffer.
    if (!ReadRaw(bytes, sizeof(*value))) return false;
    ptr = bytes;
  }

  uint32 part0 = (static_cast<uint32>(ptr[0])      ) |
                 (static_cast<uint32>(ptr[1]) <<  8) |
                 (static_cast<uint32>(ptr[2]) << 16) |
                 (static_cast<uint32>(ptr[3]) << 24);
  uint32 part1 = (static_cast<uint32>(ptr[4])      ) |
                 (static_cast<uint32>(ptr[5]) <<  8) |
                 (static_cast<uint32>(ptr[6]) << 16) |
                 (static_cast<uint32>(ptr[7]) << 24);
  *value = static_cast<uint64>(part0) |
          (static_cast<uint64>(part1) << 32);
  return true;
}

bool CodedInputStream::ReadVarint32Fallback(uint32* value) {
  if (buffer_size_ >= kMaxVarintBytes ||
      // Optimization:  If the varint ends at exactly the end of the buffer,
      // we can detect that and still use the fast path.
      (buffer_size_ != 0 && !(buffer_[buffer_size_-1] & 0x80))) {
    // Fast path:  We have enough bytes left in the buffer to guarantee that
    // this read won't cross the end, so we can skip the checks.
    const uint8* ptr = buffer_;
    uint32 b;
    uint32 result;

    b = *(ptr++); result  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); result |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *(ptr++); result |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *(ptr++); result |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *(ptr++); result |=  b         << 28; if (!(b & 0x80)) goto done;

    // If the input is larger than 32 bits, we still need to read it all
    // and discard the high-order bits.
    for (int i = 0; i < kMaxVarintBytes - kMaxVarint32Bytes; i++) {
      b = *(ptr++); if (!(b & 0x80)) goto done;
    }

    // We have overrun the maximum size of a varint (10 bytes).  Assume
    // the data is corrupt.
    return false;

   done:
    Advance(ptr - buffer_);
    *value = result;
    return true;

  } else {
    // Optimization:  If we're at a limit, detect that quickly.  (This is
    // common when reading tags.)
    while (buffer_size_ == 0) {
      // Detect cases where we definitely hit a byte limit without calling
      // Refresh().
      if (// If we hit a limit, buffer_size_after_limit_ will be non-zero.
          buffer_size_after_limit_ > 0 &&
          // Make sure that the limit we hit is not total_bytes_limit_, since
          // in that case we still need to call Refresh() so that it prints an
          // error.
          total_bytes_read_ - buffer_size_after_limit_ < total_bytes_limit_) {
        // We hit a byte limit.
        legitimate_message_end_ = true;
        return false;
      }

      // Call refresh.
      if (!Refresh()) {
        // Refresh failed.  Make sure that it failed due to EOF, not because
        // we hit total_bytes_limit_, which, unlike normal limits, is not a
        // valid place to end a message.
        int current_position = total_bytes_read_ - buffer_size_after_limit_;
        if (current_position >= total_bytes_limit_) {
          // Hit total_bytes_limit_.  But if we also hit the normal limit,
          // we're still OK.
          legitimate_message_end_ = current_limit_ == total_bytes_limit_;
        } else {
          legitimate_message_end_ = true;
        }
        return false;
      }
    }

    // Slow path:  Just do a 64-bit read.
    uint64 result;
    if (!ReadVarint64(&result)) return false;
    *value = (uint32)result;
    return true;
  }
}

bool CodedInputStream::ReadVarint64(uint64* value) {
  if (buffer_size_ >= kMaxVarintBytes ||
      // Optimization:  If the varint ends at exactly the end of the buffer,
      // we can detect that and still use the fast path.
      (buffer_size_ != 0 && !(buffer_[buffer_size_-1] & 0x80))) {
    // Fast path:  We have enough bytes left in the buffer to guarantee that
    // this read won't cross the end, so we can skip the checks.

    const uint8* ptr = buffer_;
    uint32 b;

    // Splitting into 32-bit pieces gives better performance on 32-bit
    // processors.
    uint32 part0 = 0, part1 = 0, part2 = 0;

    b = *(ptr++); part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *(ptr++); part2  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part2 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;

    // We have overrun the maximum size of a varint (10 bytes).  The data
    // must be corrupt.
    return false;

   done:
    Advance(ptr - buffer_);
    *value = (static_cast<uint64>(part0)      ) |
             (static_cast<uint64>(part1) << 28) |
             (static_cast<uint64>(part2) << 56);
    return true;

  } else {
    // Slow path:  This read might cross the end of the buffer, so we
    // need to check and refresh the buffer if and when it does.

    uint64 result = 0;
    int count = 0;
    uint32 b;

    do {
      if (count == kMaxVarintBytes) return false;
      while (buffer_size_ == 0) {
        if (!Refresh()) return false;
      }
      b = *buffer_;
      result |= static_cast<uint64>(b & 0x7F) << (7 * count);
      Advance(1);
      ++count;
    } while(b & 0x80);

    *value = result;
    return true;
  }
}

bool CodedInputStream::Refresh() {
  if (buffer_size_after_limit_ > 0 || overflow_bytes_ > 0) {
    // We've hit a limit.  Stop.
    buffer_ += buffer_size_;
    buffer_size_ = 0;

    int current_position = total_bytes_read_ - buffer_size_after_limit_;

    if (current_position >= total_bytes_limit_ &&
        total_bytes_limit_ != current_limit_) {
      // Hit total_bytes_limit_.
      PrintTotalBytesLimitError();
    }

    return false;
  }

  if (total_bytes_warning_threshold_ >= 0 &&
      total_bytes_read_ >= total_bytes_warning_threshold_) {
      GOOGLE_LOG(WARNING) << "Reading dangerously large protocol message.  If the "
                      "message turns out to be larger than "
                   << total_bytes_limit_ << " bytes, parsing will be halted "
                      "for security reasons.  To increase the limit (or to "
                      "disable these warnings), see "
                      "CodedInputStream::SetTotalBytesLimit() in "
                      "google/protobuf/io/coded_stream.h.";

    // Don't warn again for this stream.
    total_bytes_warning_threshold_ = -1;
  }

  const void* void_buffer;
  if (input_->Next(&void_buffer, &buffer_size_)) {
    buffer_ = reinterpret_cast<const uint8*>(void_buffer);
    GOOGLE_CHECK_GE(buffer_size_, 0);

    if (total_bytes_read_ <= INT_MAX - buffer_size_) {
      total_bytes_read_ += buffer_size_;
    } else {
      // Overflow.  Reset buffer_size_ to not include the bytes beyond INT_MAX.
      // We can't get that far anyway, because total_bytes_limit_ is guaranteed
      // to be less than it.  We need to keep track of the number of bytes
      // we discarded, though, so that we can call input_->BackUp() to back
      // up over them on destruction.

      // The following line is equivalent to:
      //   overflow_bytes_ = total_bytes_read_ + buffer_size_ - INT_MAX;
      // except that it avoids overflows.  Signed integer overflow has
      // undefined results according to the C standard.
      overflow_bytes_ = total_bytes_read_ - (INT_MAX - buffer_size_);
      buffer_size_ -= overflow_bytes_;
      total_bytes_read_ = INT_MAX;
    }

    RecomputeBufferLimits();
    return true;
  } else {
    buffer_ = NULL;
    buffer_size_ = 0;
    return false;
  }
}

// CodedOutputStream =================================================

CodedOutputStream::CodedOutputStream(ZeroCopyOutputStream* output)
  : output_(output),
    buffer_(NULL),
    buffer_size_(0),
    total_bytes_(0) {
}

CodedOutputStream::~CodedOutputStream() {
  if (buffer_size_ > 0) {
    output_->BackUp(buffer_size_);
  }
}

bool CodedOutputStream::WriteRaw(const void* data, int size) {
  while (buffer_size_ < size) {
    memcpy(buffer_, data, buffer_size_);
    size -= buffer_size_;
    data = reinterpret_cast<const uint8*>(data) + buffer_size_;
    if (!Refresh()) return false;
  }

  memcpy(buffer_, data, size);
  Advance(size);
  return true;
}


bool CodedOutputStream::WriteLittleEndian32(uint32 value) {
  uint8 bytes[sizeof(value)];

  bool use_fast = buffer_size_ >= sizeof(value);
  uint8* ptr = use_fast ? buffer_ : bytes;

  ptr[0] = static_cast<uint8>(value      );
  ptr[1] = static_cast<uint8>(value >>  8);
  ptr[2] = static_cast<uint8>(value >> 16);
  ptr[3] = static_cast<uint8>(value >> 24);

  if (use_fast) {
    Advance(sizeof(value));
    return true;
  } else {
    return WriteRaw(bytes, sizeof(value));
  }
}

bool CodedOutputStream::WriteLittleEndian64(uint64 value) {
  uint8 bytes[sizeof(value)];

  uint32 part0 = static_cast<uint32>(value);
  uint32 part1 = static_cast<uint32>(value >> 32);

  bool use_fast = buffer_size_ >= sizeof(value);
  uint8* ptr = use_fast ? buffer_ : bytes;

  ptr[0] = static_cast<uint8>(part0      );
  ptr[1] = static_cast<uint8>(part0 >>  8);
  ptr[2] = static_cast<uint8>(part0 >> 16);
  ptr[3] = static_cast<uint8>(part0 >> 24);
  ptr[4] = static_cast<uint8>(part1      );
  ptr[5] = static_cast<uint8>(part1 >>  8);
  ptr[6] = static_cast<uint8>(part1 >> 16);
  ptr[7] = static_cast<uint8>(part1 >> 24);

  if (use_fast) {
    Advance(sizeof(value));
    return true;
  } else {
    return WriteRaw(bytes, sizeof(value));
  }
}

bool CodedOutputStream::WriteVarint32Fallback(uint32 value) {
  if (buffer_size_ >= kMaxVarint32Bytes) {
    // Fast path:  We have enough bytes left in the buffer to guarantee that
    // this write won't cross the end, so we can skip the checks.
    uint8* target = buffer_;

    target[0] = static_cast<uint8>(value | 0x80);
    if (value >= (1 << 7)) {
      target[1] = static_cast<uint8>((value >>  7) | 0x80);
      if (value >= (1 << 14)) {
        target[2] = static_cast<uint8>((value >> 14) | 0x80);
        if (value >= (1 << 21)) {
          target[3] = static_cast<uint8>((value >> 21) | 0x80);
          if (value >= (1 << 28)) {
            target[4] = static_cast<uint8>(value >> 28);
            Advance(5);
          } else {
            target[3] &= 0x7F;
            Advance(4);
          }
        } else {
          target[2] &= 0x7F;
          Advance(3);
        }
      } else {
        target[1] &= 0x7F;
        Advance(2);
      }
    } else {
      target[0] &= 0x7F;
      Advance(1);
    }

    return true;
  } else {
    // Slow path:  This write might cross the end of the buffer, so we
    // compose the bytes first then use WriteRaw().
    uint8 bytes[kMaxVarint32Bytes];
    int size = 0;
    while (value > 0x7F) {
      bytes[size++] = (static_cast<uint8>(value) & 0x7F) | 0x80;
      value >>= 7;
    }
    bytes[size++] = static_cast<uint8>(value) & 0x7F;
    return WriteRaw(bytes, size);
  }
}

bool CodedOutputStream::WriteVarint64(uint64 value) {
  if (buffer_size_ >= kMaxVarintBytes) {
    // Fast path:  We have enough bytes left in the buffer to guarantee that
    // this write won't cross the end, so we can skip the checks.
    uint8* target = buffer_;

    // Splitting into 32-bit pieces gives better performance on 32-bit
    // processors.
    uint32 part0 = static_cast<uint32>(value      );
    uint32 part1 = static_cast<uint32>(value >> 28);
    uint32 part2 = static_cast<uint32>(value >> 56);

    int size;

    // Here we can't really optimize for small numbers, since the value is
    // split into three parts.  Cheking for numbers < 128, for instance,
    // would require three comparisons, since you'd have to make sure part1
    // and part2 are zero.  However, if the caller is using 64-bit integers,
    // it is likely that they expect the numbers to often be very large, so
    // we probably don't want to optimize for small numbers anyway.  Thus,
    // we end up with a hardcoded binary search tree...
    if (part2 == 0) {
      if (part1 == 0) {
        if (part0 < (1 << 14)) {
          if (part0 < (1 << 7)) {
            size = 1; goto size1;
          } else {
            size = 2; goto size2;
          }
        } else {
          if (part0 < (1 << 21)) {
            size = 3; goto size3;
          } else {
            size = 4; goto size4;
          }
        }
      } else {
        if (part1 < (1 << 14)) {
          if (part1 < (1 << 7)) {
            size = 5; goto size5;
          } else {
            size = 6; goto size6;
          }
        } else {
          if (part1 < (1 << 21)) {
            size = 7; goto size7;
          } else {
            size = 8; goto size8;
          }
        }
      }
    } else {
      if (part2 < (1 << 7)) {
        size = 9; goto size9;
      } else {
        size = 10; goto size10;
      }
    }

    GOOGLE_LOG(FATAL) << "Can't get here.";

    size10: target[9] = static_cast<uint8>((part2 >>  7) | 0x80);
    size9 : target[8] = static_cast<uint8>((part2      ) | 0x80);
    size8 : target[7] = static_cast<uint8>((part1 >> 21) | 0x80);
    size7 : target[6] = static_cast<uint8>((part1 >> 14) | 0x80);
    size6 : target[5] = static_cast<uint8>((part1 >>  7) | 0x80);
    size5 : target[4] = static_cast<uint8>((part1      ) | 0x80);
    size4 : target[3] = static_cast<uint8>((part0 >> 21) | 0x80);
    size3 : target[2] = static_cast<uint8>((part0 >> 14) | 0x80);
    size2 : target[1] = static_cast<uint8>((part0 >>  7) | 0x80);
    size1 : target[0] = static_cast<uint8>((part0      ) | 0x80);

    target[size-1] &= 0x7F;
    Advance(size);
    return true;
  } else {
    // Slow path:  This write might cross the end of the buffer, so we
    // compose the bytes first then use WriteRaw().
    uint8 bytes[kMaxVarintBytes];
    int size = 0;
    while (value > 0x7F) {
      bytes[size++] = (static_cast<uint8>(value) & 0x7F) | 0x80;
      value >>= 7;
    }
    bytes[size++] = static_cast<uint8>(value) & 0x7F;
    return WriteRaw(bytes, size);
  }
}

bool CodedOutputStream::Refresh() {
  void* void_buffer;
  if (output_->Next(&void_buffer, &buffer_size_)) {
    buffer_ = reinterpret_cast<uint8*>(void_buffer);
    total_bytes_ += buffer_size_;
    return true;
  } else {
    buffer_ = NULL;
    buffer_size_ = 0;
    return false;
  }
}

int CodedOutputStream::VarintSize32Fallback(uint32 value) {
  if (value < (1 << 7)) {
    return 1;
  } else if (value < (1 << 14)) {
    return 2;
  } else if (value < (1 << 21)) {
    return 3;
  } else if (value < (1 << 28)) {
    return 4;
  } else {
    return 5;
  }
}

int CodedOutputStream::VarintSize64(uint64 value) {
  if (value < (1ull << 35)) {
    if (value < (1ull << 7)) {
      return 1;
    } else if (value < (1ull << 14)) {
      return 2;
    } else if (value < (1ull << 21)) {
      return 3;
    } else if (value < (1ull << 28)) {
      return 4;
    } else {
      return 5;
    }
  } else {
    if (value < (1ull << 42)) {
      return 6;
    } else if (value < (1ull << 49)) {
      return 7;
    } else if (value < (1ull << 56)) {
      return 8;
    } else if (value < (1ull << 63)) {
      return 9;
    } else {
      return 10;
    }
  }
}

}  // namespace io
}  // namespace protobuf
}  // namespace google
