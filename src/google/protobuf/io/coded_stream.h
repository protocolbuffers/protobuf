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
//
// This file contains the CodedInputStream and CodedOutputStream classes,
// which wrap a ZeroCopyInputStream or ZeroCopyOutputStream, respectively,
// and allow you to read or write individual pieces of data in various
// formats.  In particular, these implement the varint encoding for
// integers, a simple variable-length encoding in which smaller numbers
// take fewer bytes.
//
// Typically these classes will only be used internally by the protocol
// buffer library in order to encode and decode protocol buffers.  Clients
// of the library only need to know about this class if they wish to write
// custom message parsing or serialization procedures.
//
// CodedOutputStream example:
//   // Write some data to "myfile".  First we write a 4-byte "magic number"
//   // to identify the file type, then write a length-delimited string.  The
//   // string is composed of a varint giving the length followed by the raw
//   // bytes.
//   int fd = open("myfile", O_WRONLY);
//   ZeroCopyOutputStream* raw_output = new FileOutputStream(fd);
//   CodedOutputStream* coded_output = new CodedOutputStream(raw_output);
//
//   int magic_number = 1234;
//   char text[] = "Hello world!";
//   coded_output->WriteLittleEndian32(magic_number);
//   coded_output->WriteVarint32(strlen(text));
//   coded_output->WriteRaw(text, strlen(text));
//
//   delete coded_output;
//   delete raw_output;
//   close(fd);
//
// CodedInputStream example:
//   // Read a file created by the above code.
//   int fd = open("myfile", O_RDONLY);
//   ZeroCopyInputStream* raw_input = new FileInputStream(fd);
//   CodedInputStream coded_input = new CodedInputStream(raw_input);
//
//   coded_input->ReadLittleEndian32(&magic_number);
//   if (magic_number != 1234) {
//     cerr << "File not in expected format." << endl;
//     return;
//   }
//
//   uint32 size;
//   coded_input->ReadVarint32(&size);
//
//   char* text = new char[size + 1];
//   coded_input->ReadRaw(buffer, size);
//   text[size] = '\0';
//
//   delete coded_input;
//   delete raw_input;
//   close(fd);
//
//   cout << "Text is: " << text << endl;
//   delete [] text;
//
// For those who are interested, varint encoding is defined as follows:
//
// The encoding operates on unsigned integers of up to 64 bits in length.
// Each byte of the encoded value has the format:
// * bits 0-6: Seven bits of the number being encoded.
// * bit 7: Zero if this is the last byte in the encoding (in which
//   case all remaining bits of the number are zero) or 1 if
//   more bytes follow.
// The first byte contains the least-significant 7 bits of the number, the
// second byte (if present) contains the next-least-significant 7 bits,
// and so on.  So, the binary number 1011000101011 would be encoded in two
// bytes as "10101011 00101100".
//
// In theory, varint could be used to encode integers of any length.
// However, for practicality we set a limit at 64 bits.  The maximum encoded
// length of a number is thus 10 bytes.

#ifndef GOOGLE_PROTOBUF_IO_CODED_STREAM_H__
#define GOOGLE_PROTOBUF_IO_CODED_STREAM_H__

#include <string>
#include <google/protobuf/stubs/common.h>

namespace google {

namespace protobuf {
namespace io {

// Defined in this file.
class CodedInputStream;
class CodedOutputStream;

// Defined in other files.
class ZeroCopyInputStream;           // zero_copy_stream.h
class ZeroCopyOutputStream;          // zero_copy_stream.h

// Class which reads and decodes binary data which is composed of varint-
// encoded integers and fixed-width pieces.  Wraps a ZeroCopyInputStream.
// Most users will not need to deal with CodedInputStream.
//
// Most methods of CodedInputStream that return a bool return false if an
// underlying I/O error occurs or if the data is malformed.  Once such a
// failure occurs, the CodedInputStream is broken and is no longer useful.
class LIBPROTOBUF_EXPORT CodedInputStream {
 public:
  // Create a CodedInputStream that reads from the given ZeroCopyInputStream.
  explicit CodedInputStream(ZeroCopyInputStream* input);

  // Destroy the CodedInputStream and position the underlying
  // ZeroCopyInputStream at the first unread byte.  If an error occurred while
  // reading (causing a method to return false), then the exact position of
  // the input stream may be anywhere between the last value that was read
  // successfully and the stream's byte limit.
  ~CodedInputStream();


  // Skips a number of bytes.  Returns false if an underlying read error
  // occurs.
  bool Skip(int count);

  // Sets *data to point directly at the unread part of the CodedInputStream's
  // underlying buffer, and *size to the size of that buffer, but does not
  // advance the stream's current position.  This will always either produce
  // a non-empty buffer or return false.  If the caller consumes any of
  // this data, it should then call Skip() to skip over the consumed bytes.
  // This may be useful for implementing external fast parsing routines for
  // types of data not covered by the CodedInputStream interface.
  bool GetDirectBufferPointer(const void** data, int* size);

  // Read raw bytes, copying them into the given buffer.
  bool ReadRaw(void* buffer, int size);

  // Like ReadRaw, but reads into a string.
  //
  // Implementation Note:  ReadString() grows the string gradually as it
  // reads in the data, rather than allocating the entire requested size
  // upfront.  This prevents denial-of-service attacks in which a client
  // could claim that a string is going to be MAX_INT bytes long in order to
  // crash the server because it can't allocate this much space at once.
  bool ReadString(string* buffer, int size);


  // Read a 32-bit little-endian integer.
  bool ReadLittleEndian32(uint32* value);
  // Read a 64-bit little-endian integer.
  bool ReadLittleEndian64(uint64* value);

  // Read an unsigned integer with Varint encoding, truncating to 32 bits.
  // Reading a 32-bit value is equivalent to reading a 64-bit one and casting
  // it to uint32, but may be more efficient.
  bool ReadVarint32(uint32* value);
  // Read an unsigned integer with Varint encoding.
  bool ReadVarint64(uint64* value);

  // Read a tag.  This calls ReadVarint32() and returns the result, or returns
  // zero (which is not a valid tag) if ReadVarint32() fails.  Also, it updates
  // the last tag value, which can be checked with LastTagWas().
  // Always inline because this is only called in once place per parse loop
  // but it is called for every iteration of said loop, so it should be fast.
  // GCC doesn't want to inline this by default.
  uint32 ReadTag() GOOGLE_ATTRIBUTE_ALWAYS_INLINE;

  // Usually returns true if calling ReadVarint32() now would produce the given
  // value.  Will always return false if ReadVarint32() would not return the
  // given value.  If ExpectTag() returns true, it also advances past
  // the varint.  For best performance, use a compile-time constant as the
  // parameter.
  // Always inline because this collapses to a small number of instructions
  // when given a constant parameter, but GCC doesn't want to inline by default.
  bool ExpectTag(uint32 expected) GOOGLE_ATTRIBUTE_ALWAYS_INLINE;

  // Usually returns true if no more bytes can be read.  Always returns false
  // if more bytes can be read.  If ExpectAtEnd() returns true, a subsequent
  // call to LastTagWas() will act as if ReadTag() had been called and returned
  // zero, and ConsumedEntireMessage() will return true.
  bool ExpectAtEnd();

  // If the last call to ReadTag() returned the given value, returns true.
  // Otherwise, returns false;
  //
  // This is needed because parsers for some types of embedded messages
  // (with field type TYPE_GROUP) don't actually know that they've reached the
  // end of a message until they see an ENDGROUP tag, which was actually part
  // of the enclosing message.  The enclosing message would like to check that
  // tag to make sure it had the right number, so it calls LastTagWas() on
  // return from the embedded parser to check.
  bool LastTagWas(uint32 expected);

  // When parsing message (but NOT a group), this method must be called
  // immediately after MergeFromCodedStream() returns (if it returns true)
  // to further verify that the message ended in a legitimate way.  For
  // example, this verifies that parsing did not end on an end-group tag.
  // It also checks for some cases where, due to optimizations,
  // MergeFromCodedStream() can incorrectly return true.
  bool ConsumedEntireMessage();

  // Limits ----------------------------------------------------------
  // Limits are used when parsing length-delimited embedded messages.
  // After the message's length is read, PushLimit() is used to prevent
  // the CodedInputStream from reading beyond that length.  Once the
  // embedded message has been parsed, PopLimit() is called to undo the
  // limit.

  // Opaque type used with PushLimit() and PopLimit().  Do not modify
  // values of this type yourself.  The only reason that this isn't a
  // struct with private internals is for efficiency.
  typedef int Limit;

  // Places a limit on the number of bytes that the stream may read,
  // starting from the current position.  Once the stream hits this limit,
  // it will act like the end of the input has been reached until PopLimit()
  // is called.
  //
  // As the names imply, the stream conceptually has a stack of limits.  The
  // shortest limit on the stack is always enforced, even if it is not the
  // top limit.
  //
  // The value returned by PushLimit() is opaque to the caller, and must
  // be passed unchanged to the corresponding call to PopLimit().
  Limit PushLimit(int byte_limit);

  // Pops the last limit pushed by PushLimit().  The input must be the value
  // returned by that call to PushLimit().
  void PopLimit(Limit limit);

  // Returns the number of bytes left until the nearest limit on the
  // stack is hit, or -1 if no limits are in place.
  int BytesUntilLimit();

  // Total Bytes Limit -----------------------------------------------
  // To prevent malicious users from sending excessively large messages
  // and causing integer overflows or memory exhaustion, CodedInputStream
  // imposes a hard limit on the total number of bytes it will read.

  // Sets the maximum number of bytes that this CodedInputStream will read
  // before refusing to continue.  To prevent integer overflows in the
  // protocol buffers implementation, as well as to prevent servers from
  // allocating enormous amounts of memory to hold parsed messages, the
  // maximum message length should be limited to the shortest length that
  // will not harm usability.  The theoretical shortest message that could
  // cause integer overflows is 512MB.  The default limit is 64MB.  Apps
  // should set shorter limits if possible.  If warning_threshold is not -1,
  // a warning will be printed to stderr after warning_threshold bytes are
  // read.  An error will always be printed to stderr if the limit is
  // reached.
  //
  // This is unrelated to PushLimit()/PopLimit().
  //
  // Hint:  If you are reading this because your program is printing a
  //   warning about dangerously large protocol messages, you may be
  //   confused about what to do next.  The best option is to change your
  //   design such that excessively large messages are not necessary.
  //   For example, try to design file formats to consist of many small
  //   messages rather than a single large one.  If this is infeasible,
  //   you will need to increase the limit.  Chances are, though, that
  //   your code never constructs a CodedInputStream on which the limit
  //   can be set.  You probably parse messages by calling things like
  //   Message::ParseFromString().  In this case, you will need to change
  //   your code to instead construct some sort of ZeroCopyInputStream
  //   (e.g. an ArrayInputStream), construct a CodedInputStream around
  //   that, then call Message::ParseFromCodedStream() instead.  Then
  //   you can adjust the limit.  Yes, it's more work, but you're doing
  //   something unusual.
  void SetTotalBytesLimit(int total_bytes_limit, int warning_threshold);

  // Recursion Limit -------------------------------------------------
  // To prevent corrupt or malicious messages from causing stack overflows,
  // we must keep track of the depth of recursion when parsing embedded
  // messages and groups.  CodedInputStream keeps track of this because it
  // is the only object that is passed down the stack during parsing.

  // Sets the maximum recursion depth.  The default is 64.
  void SetRecursionLimit(int limit);

  // Increments the current recursion depth.  Returns true if the depth is
  // under the limit, false if it has gone over.
  bool IncrementRecursionDepth();

  // Decrements the recursion depth.
  void DecrementRecursionDepth();

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CodedInputStream);

  ZeroCopyInputStream* input_;
  const uint8* buffer_;
  int buffer_size_;       // size of current buffer
  int total_bytes_read_;  // total bytes read from input_, including
                          // the current buffer

  // If total_bytes_read_ surpasses INT_MAX, we record the extra bytes here
  // so that we can BackUp() on destruction.
  int overflow_bytes_;

  // LastTagWas() stuff.
  uint32 last_tag_;         // result of last ReadTag().

  // This is set true by ReadVarint32Fallback() if it is called when exactly
  // at EOF, or by ExpectAtEnd() when it returns true.  This happens when we
  // reach the end of a message and attempt to read another tag.
  bool legitimate_message_end_;

  // See EnableAliasing().
  bool aliasing_enabled_;

  // Limits
  Limit current_limit_;   // if position = -1, no limit is applied

  // For simplicity, if the current buffer crosses a limit (either a normal
  // limit created by PushLimit() or the total bytes limit), buffer_size_
  // only tracks the number of bytes before that limit.  This field
  // contains the number of bytes after it.  Note that this implies that if
  // buffer_size_ == 0 and buffer_size_after_limit_ > 0, we know we've
  // hit a limit.  However, if both are zero, it doesn't necessarily mean
  // we aren't at a limit -- the buffer may have ended exactly at the limit.
  int buffer_size_after_limit_;

  // Maximum number of bytes to read, period.  This is unrelated to
  // current_limit_.  Set using SetTotalBytesLimit().
  int total_bytes_limit_;
  int total_bytes_warning_threshold_;

  // Current recursion depth, controlled by IncrementRecursionDepth() and
  // DecrementRecursionDepth().
  int recursion_depth_;
  // Recursion depth limit, set by SetRecursionLimit().
  int recursion_limit_;

  // Advance the buffer by a given number of bytes.
  void Advance(int amount);

  // Recomputes the value of buffer_size_after_limit_.  Must be called after
  // current_limit_ or total_bytes_limit_ changes.
  void RecomputeBufferLimits();

  // Writes an error message saying that we hit total_bytes_limit_.
  void PrintTotalBytesLimitError();

  // Called when the buffer runs out to request more data.  Implies an
  // Advance(buffer_size_).
  bool Refresh();

  bool ReadVarint32Fallback(uint32* value);
};

// Class which encodes and writes binary data which is composed of varint-
// encoded integers and fixed-width pieces.  Wraps a ZeroCopyOutputStream.
// Most users will not need to deal with CodedOutputStream.
//
// Most methods of CodedOutputStream which return a bool return false if an
// underlying I/O error occurs.  Once such a failure occurs, the
// CodedOutputStream is broken and is no longer useful. The Write* methods do
// not return the stream status, but will invalidate the stream if an error
// occurs. The client can probe HadError() to determine the status.
//
// Note that every method of CodedOutputStream which writes some data has
// a corresponding static "ToArray" version. These versions write directly
// to the provided buffer, returning a pointer past the last written byte.
// They require that the buffer has sufficient capacity for the encoded data.
// This allows an optimization where we check if an output stream has enough
// space for an entire message before we start writing and, if there is, we
// call only the ToArray methods to avoid doing bound checks for each
// individual value.
// i.e., in the example above:
//
//   CodedOutputStream coded_output = new CodedOutputStream(raw_output);
//   int magic_number = 1234;
//   char text[] = "Hello world!";
//
//   int coded_size = sizeof(magic_number) +
//                    CodedOutputStream::Varint32Size(strlen(text)) +
//                    strlen(text);
//
//   uint8* buffer =
//       coded_output->GetDirectBufferForNBytesAndAdvance(coded_size);
//   if (buffer != NULL) {
//     // The output stream has enough space in the buffer: write directly to
//     // the array.
//     buffer = CodedOutputStream::WriteLittleEndian32ToArray(magic_number,
//                                                            buffer);
//     buffer = CodedOutputStream::WriteVarint32ToArray(strlen(text), buffer);
//     buffer = CodedOutputStream::WriteRawToArray(text, strlen(text), buffer);
//   } else {
//     // Make bound-checked writes, which will ask the underlying stream for
//     // more space as needed.
//     coded_output->WriteLittleEndian32(magic_number);
//     coded_output->WriteVarint32(strlen(text));
//     coded_output->WriteRaw(text, strlen(text));
//   }
//
//   delete coded_output;
class LIBPROTOBUF_EXPORT CodedOutputStream {
 public:
  // Create an CodedOutputStream that writes to the given ZeroCopyOutputStream.
  explicit CodedOutputStream(ZeroCopyOutputStream* output);

  // Destroy the CodedOutputStream and position the underlying
  // ZeroCopyOutputStream immediately after the last byte written.
  ~CodedOutputStream();

  // Skips a number of bytes, leaving the bytes unmodified in the underlying
  // buffer.  Returns false if an underlying write error occurs.  This is
  // mainly useful with GetDirectBufferPointer().
  bool Skip(int count);

  // Sets *data to point directly at the unwritten part of the
  // CodedOutputStream's underlying buffer, and *size to the size of that
  // buffer, but does not advance the stream's current position.  This will
  // always either produce a non-empty buffer or return false.  If the caller
  // writes any data to this buffer, it should then call Skip() to skip over
  // the consumed bytes.  This may be useful for implementing external fast
  // serialization routines for types of data not covered by the
  // CodedOutputStream interface.
  bool GetDirectBufferPointer(void** data, int* size);

  // If there are at least "size" bytes available in the current buffer,
  // returns a pointer directly into the buffer and advances over these bytes.
  // The caller may then write directly into this buffer (e.g. using the
  // *ToArray static methods) rather than go through CodedOutputStream.  If
  // there are not enough bytes available, returns NULL.  The return pointer is
  // invalidated as soon as any other non-const method of CodedOutputStream
  // is called.
  inline uint8* GetDirectBufferForNBytesAndAdvance(int size);

  // Write raw bytes, copying them from the given buffer.
  void WriteRaw(const void* buffer, int size);
  // Like WriteRaw()  but writing directly to the target array.
  // This is _not_ inlined, as the compiler often optimizes memcpy into inline
  // copy loops. Since this gets called by every field with string or bytes
  // type, inlining may lead to a significant amount of code bloat, with only a
  // minor performance gain.
  static uint8* WriteRawToArray(const void* buffer, int size, uint8* target);

  // Equivalent to WriteRaw(str.data(), str.size()).
  void WriteString(const string& str);
  // Like WriteString()  but writing directly to the target array.
  static uint8* WriteStringToArray(const string& str, uint8* target);


  // Write a 32-bit little-endian integer.
  void WriteLittleEndian32(uint32 value);
  // Like WriteLittleEndian32()  but writing directly to the target array.
  static uint8* WriteLittleEndian32ToArray(uint32 value, uint8* target);
  // Write a 64-bit little-endian integer.
  void WriteLittleEndian64(uint64 value);
  // Like WriteLittleEndian64()  but writing directly to the target array.
  static uint8* WriteLittleEndian64ToArray(uint64 value, uint8* target);

  // Write an unsigned integer with Varint encoding.  Writing a 32-bit value
  // is equivalent to casting it to uint64 and writing it as a 64-bit value,
  // but may be more efficient.
  void WriteVarint32(uint32 value);
  // Like WriteVarint32()  but writing directly to the target array.
  static uint8* WriteVarint32ToArray(uint32 value, uint8* target);
  // Write an unsigned integer with Varint encoding.
  void WriteVarint64(uint64 value);
  // Like WriteVarint64()  but writing directly to the target array.
  static uint8* WriteVarint64ToArray(uint64 value, uint8* target);

  // Equivalent to WriteVarint32() except when the value is negative,
  // in which case it must be sign-extended to a full 10 bytes.
  void WriteVarint32SignExtended(int32 value);
  // Like WriteVarint32SignExtended()  but writing directly to the target array.
  static uint8* WriteVarint32SignExtendedToArray(int32 value, uint8* target);

  // This is identical to WriteVarint32(), but optimized for writing tags.
  // In particular, if the input is a compile-time constant, this method
  // compiles down to a couple instructions.
  // Always inline because otherwise the aformentioned optimization can't work,
  // but GCC by default doesn't want to inline this.
  void WriteTag(uint32 value);
  // Like WriteTag()  but writing directly to the target array.
  static uint8* WriteTagToArray(
      uint32 value, uint8* target) GOOGLE_ATTRIBUTE_ALWAYS_INLINE;

  // Returns the number of bytes needed to encode the given value as a varint.
  static int VarintSize32(uint32 value);
  // Returns the number of bytes needed to encode the given value as a varint.
  static int VarintSize64(uint64 value);

  // If negative, 10 bytes.  Otheriwse, same as VarintSize32().
  static int VarintSize32SignExtended(int32 value);

  // Returns the total number of bytes written since this object was created.
  inline int ByteCount() const;

  // Returns true if there was an underlying I/O error since this object was
  // created.
  bool HadError() const { return had_error_; }

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CodedOutputStream);

  ZeroCopyOutputStream* output_;
  uint8* buffer_;
  int buffer_size_;
  int total_bytes_;  // Sum of sizes of all buffers seen so far.
  bool had_error_;   // Whether an error occurred during output.

  // Advance the buffer by a given number of bytes.
  void Advance(int amount);

  // Called when the buffer runs out to request more data.  Implies an
  // Advance(buffer_size_).
  bool Refresh();

  static uint8* WriteVarint32FallbackToArray(uint32 value, uint8* target);

  // Always-inlined versions of WriteVarint* functions so that code can be
  // reused, while still controlling size. For instance, WriteVarint32ToArray()
  // should not directly call this: since it is inlined itself, doing so
  // would greatly increase the size of generated code. Instead, it should call
  // WriteVarint32FallbackToArray.  Meanwhile, WriteVarint32() is already
  // out-of-line, so it should just invoke this directly to avoid any extra
  // function call overhead.
  static uint8* WriteVarint32FallbackToArrayInline(
      uint32 value, uint8* target) GOOGLE_ATTRIBUTE_ALWAYS_INLINE;
  static uint8* WriteVarint64ToArrayInline(
      uint64 value, uint8* target) GOOGLE_ATTRIBUTE_ALWAYS_INLINE;

  static int VarintSize32Fallback(uint32 value);
};

// inline methods ====================================================
// The vast majority of varints are only one byte.  These inline
// methods optimize for that case.

inline bool CodedInputStream::ReadVarint32(uint32* value) {
  if (buffer_size_ != 0 && *buffer_ < 0x80) {
    *value = *buffer_;
    Advance(1);
    return true;
  } else {
    return ReadVarint32Fallback(value);
  }
}

inline uint32 CodedInputStream::ReadTag() {
  if (buffer_size_ != 0 && buffer_[0] < 0x80) {
    last_tag_ = buffer_[0];
    Advance(1);
    return last_tag_;
  } else if (buffer_size_ >= 2 && buffer_[1] < 0x80) {
    last_tag_ = (buffer_[0] & 0x7f) + (buffer_[1] << 7);
    Advance(2);
    return last_tag_;
  } else if (ReadVarint32Fallback(&last_tag_)) {
    return last_tag_;
  } else {
    last_tag_ = 0;
    return 0;
  }
}

inline bool CodedInputStream::LastTagWas(uint32 expected) {
  return last_tag_ == expected;
}

inline bool CodedInputStream::ConsumedEntireMessage() {
  return legitimate_message_end_;
}

inline bool CodedInputStream::ExpectTag(uint32 expected) {
  if (expected < (1 << 7)) {
    if (buffer_size_ != 0 && buffer_[0] == expected) {
      Advance(1);
      return true;
    } else {
      return false;
    }
  } else if (expected < (1 << 14)) {
    if (buffer_size_ >= 2 &&
        buffer_[0] == static_cast<uint8>(expected | 0x80) &&
        buffer_[1] == static_cast<uint8>(expected >> 7)) {
      Advance(2);
      return true;
    } else {
      return false;
    }
  } else {
    // Don't bother optimizing for larger values.
    return false;
  }
}

inline bool CodedInputStream::ExpectAtEnd() {
  // If we are at a limit we know no more bytes can be read.  Otherwise, it's
  // hard to say without calling Refresh(), and we'd rather not do that.

  if (buffer_size_ == 0 && buffer_size_after_limit_ != 0) {
    last_tag_ = 0;                   // Pretend we called ReadTag()...
    legitimate_message_end_ = true;  // ... and it hit EOF.
    return true;
  } else {
    return false;
  }
}

inline uint8* CodedOutputStream::GetDirectBufferForNBytesAndAdvance(int size) {
  if (buffer_size_ < size) {
    return NULL;
  } else {
    uint8* result = buffer_;
    Advance(size);
    return result;
  }
}

inline uint8* CodedOutputStream::WriteVarint32ToArray(uint32 value,
                                                        uint8* target) {
  if (value < 0x80) {
    *target = value;
    return target + 1;
  } else {
    return WriteVarint32FallbackToArray(value, target);
  }
}

inline void CodedOutputStream::WriteVarint32SignExtended(int32 value) {
  if (value < 0) {
    WriteVarint64(static_cast<uint64>(value));
  } else {
    WriteVarint32(static_cast<uint32>(value));
  }
}

inline uint8* CodedOutputStream::WriteVarint32SignExtendedToArray(
    int32 value, uint8* target) {
  if (value < 0) {
    return WriteVarint64ToArray(static_cast<uint64>(value), target);
  } else {
    return WriteVarint32ToArray(static_cast<uint32>(value), target);
  }
}

inline void CodedOutputStream::WriteTag(uint32 value) {
  WriteVarint32(value);
}

inline uint8* CodedOutputStream::WriteTagToArray(
    uint32 value, uint8* target) {
  if (value < (1 << 7)) {
    target[0] = value;
    return target + 1;
  } else if (value < (1 << 14)) {
    target[0] = static_cast<uint8>(value | 0x80);
    target[1] = static_cast<uint8>(value >> 7);
    return target + 2;
  } else {
    return WriteVarint32FallbackToArray(value, target);
  }
}

inline int CodedOutputStream::VarintSize32(uint32 value) {
  if (value < (1 << 7)) {
    return 1;
  } else  {
    return VarintSize32Fallback(value);
  }
}

inline int CodedOutputStream::VarintSize32SignExtended(int32 value) {
  if (value < 0) {
    return 10;     // TODO(kenton):  Make this a symbolic constant.
  } else {
    return VarintSize32(static_cast<uint32>(value));
  }
}

inline void CodedOutputStream::WriteString(const string& str) {
  WriteRaw(str.data(), str.size());
}

inline uint8* CodedOutputStream::WriteStringToArray(
    const string& str, uint8* target) {
  return WriteRawToArray(str.data(), str.size(), target);
}

inline int CodedOutputStream::ByteCount() const {
  return total_bytes_ - buffer_size_;
}

inline void CodedInputStream::Advance(int amount) {
  buffer_ += amount;
  buffer_size_ -= amount;
}

inline void CodedOutputStream::Advance(int amount) {
  buffer_ += amount;
  buffer_size_ -= amount;
}

inline void CodedInputStream::SetRecursionLimit(int limit) {
  recursion_limit_ = limit;
}

inline bool CodedInputStream::IncrementRecursionDepth() {
  ++recursion_depth_;
  return recursion_depth_ <= recursion_limit_;
}

inline void CodedInputStream::DecrementRecursionDepth() {
  if (recursion_depth_ > 0) --recursion_depth_;
}

}  // namespace io
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_IO_CODED_STREAM_H__
