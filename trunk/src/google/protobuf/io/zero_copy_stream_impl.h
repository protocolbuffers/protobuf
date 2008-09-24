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
// This file contains common implementations of the interfaces defined in
// zero_copy_stream.h.  These implementations cover I/O on raw arrays,
// strings, and file descriptors.  Of course, many users will probably
// want to write their own implementations of these interfaces specific
// to the particular I/O abstractions they prefer to use, but these
// should cover the most common cases.

#ifndef GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_H__
#define GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_H__

#include <string>
#include <iosfwd>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>


namespace google {
namespace protobuf {
namespace io {

// ===================================================================

// A ZeroCopyInputStream backed by an in-memory array of bytes.
class LIBPROTOBUF_EXPORT ArrayInputStream : public ZeroCopyInputStream {
 public:
  // Create an InputStream that returns the bytes pointed to by "data".
  // "data" remains the property of the caller but must remain valid until
  // the stream is destroyed.  If a block_size is given, calls to Next()
  // will return data blocks no larger than the given size.  Otherwise, the
  // first call to Next() returns the entire array.  block_size is mainly
  // useful for testing; in production you would probably never want to set
  // it.
  ArrayInputStream(const void* data, int size, int block_size = -1);
  ~ArrayInputStream();

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;


 private:
  const uint8* const data_;  // The byte array.
  const int size_;           // Total size of the array.
  const int block_size_;     // How many bytes to return at a time.

  int position_;
  int last_returned_size_;   // How many bytes we returned last time Next()
                             // was called (used for error checking only).

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ArrayInputStream);
};

// ===================================================================

// A ZeroCopyOutputStream backed by an in-memory array of bytes.
class LIBPROTOBUF_EXPORT ArrayOutputStream : public ZeroCopyOutputStream {
 public:
  // Create an OutputStream that writes to the bytes pointed to by "data".
  // "data" remains the property of the caller but must remain valid until
  // the stream is destroyed.  If a block_size is given, calls to Next()
  // will return data blocks no larger than the given size.  Otherwise, the
  // first call to Next() returns the entire array.  block_size is mainly
  // useful for testing; in production you would probably never want to set
  // it.
  ArrayOutputStream(void* data, int size, int block_size = -1);
  ~ArrayOutputStream();

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  uint8* const data_;        // The byte array.
  const int size_;           // Total size of the array.
  const int block_size_;     // How many bytes to return at a time.

  int position_;
  int last_returned_size_;   // How many bytes we returned last time Next()
                             // was called (used for error checking only).

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ArrayOutputStream);
};

// ===================================================================

// A ZeroCopyOutputStream which appends bytes to a string.
class LIBPROTOBUF_EXPORT StringOutputStream : public ZeroCopyOutputStream {
 public:
  // Create a StringOutputStream which appends bytes to the given string.
  // The string remains property of the caller, but it MUST NOT be accessed
  // in any way until the stream is destroyed.
  //
  // Hint:  If you call target->reserve(n) before creating the stream,
  //   the first call to Next() will return at least n bytes of buffer
  //   space.
  explicit StringOutputStream(string* target);
  ~StringOutputStream();

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  static const int kMinimumSize = 16;

  string* target_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(StringOutputStream);
};

// Note:  There is no StringInputStream.  Instead, just create an
// ArrayInputStream as follows:
//   ArrayInputStream input(str.data(), str.size());

// ===================================================================


// ===================================================================

// A generic traditional input stream interface.
//
// Lots of traditional input streams (e.g. file descriptors, C stdio
// streams, and C++ iostreams) expose an interface where every read
// involves copying bytes into a buffer.  If you want to take such an
// interface and make a ZeroCopyInputStream based on it, simply implement
// CopyingInputStream and then use CopyingInputStreamAdaptor.
//
// CopyingInputStream implementations should avoid buffering if possible.
// CopyingInputStreamAdaptor does its own buffering and will read data
// in large blocks.
class LIBPROTOBUF_EXPORT CopyingInputStream {
 public:
  virtual ~CopyingInputStream();

  // Reads up to "size" bytes into the given buffer.  Returns the number of
  // bytes read.  Read() waits until at least one byte is available, or
  // returns zero if no bytes will ever become available (EOF), or -1 if a
  // permanent read error occurred.
  virtual int Read(void* buffer, int size) = 0;

  // Skips the next "count" bytes of input.  Returns the number of bytes
  // actually skipped.  This will always be exactly equal to "count" unless
  // EOF was reached or a permanent read error occurred.
  //
  // The default implementation just repeatedly calls Read() into a scratch
  // buffer.
  virtual int Skip(int count);
};

// A ZeroCopyInputStream which reads from a CopyingInputStream.  This is
// useful for implementing ZeroCopyInputStreams that read from traditional
// streams.  Note that this class is not really zero-copy.
//
// If you want to read from file descriptors or C++ istreams, this is
// already implemented for you:  use FileInputStream or IstreamInputStream
// respectively.
class LIBPROTOBUF_EXPORT CopyingInputStreamAdaptor : public ZeroCopyInputStream {
 public:
  // Creates a stream that reads from the given CopyingInputStream.
  // If a block_size is given, it specifies the number of bytes that
  // should be read and returned with each call to Next().  Otherwise,
  // a reasonable default is used.  The caller retains ownership of
  // copying_stream unless SetOwnsCopyingStream(true) is called.
  explicit CopyingInputStreamAdaptor(CopyingInputStream* copying_stream,
                                     int block_size = -1);
  ~CopyingInputStreamAdaptor();

  // Call SetOwnsCopyingStream(true) to tell the CopyingInputStreamAdaptor to
  // delete the underlying CopyingInputStream when it is destroyed.
  void SetOwnsCopyingStream(bool value) { owns_copying_stream_ = value; }

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;

 private:
  // Insures that buffer_ is not NULL.
  void AllocateBufferIfNeeded();
  // Frees the buffer and resets buffer_used_.
  void FreeBuffer();

  // The underlying copying stream.
  CopyingInputStream* copying_stream_;
  bool owns_copying_stream_;

  // True if we have seen a permenant error from the underlying stream.
  bool failed_;

  // The current position of copying_stream_, relative to the point where
  // we started reading.
  int64 position_;

  // Data is read into this buffer.  It may be NULL if no buffer is currently
  // in use.  Otherwise, it points to an array of size buffer_size_.
  scoped_array<uint8> buffer_;
  const int buffer_size_;

  // Number of valid bytes currently in the buffer (i.e. the size last
  // returned by Next()).  0 <= buffer_used_ <= buffer_size_.
  int buffer_used_;

  // Number of bytes in the buffer which were backed up over by a call to
  // BackUp().  These need to be returned again.
  // 0 <= backup_bytes_ <= buffer_used_
  int backup_bytes_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingInputStreamAdaptor);
};

// ===================================================================

// A generic traditional output stream interface.
//
// Lots of traditional output streams (e.g. file descriptors, C stdio
// streams, and C++ iostreams) expose an interface where every write
// involves copying bytes from a buffer.  If you want to take such an
// interface and make a ZeroCopyOutputStream based on it, simply implement
// CopyingOutputStream and then use CopyingOutputStreamAdaptor.
//
// CopyingOutputStream implementations should avoid buffering if possible.
// CopyingOutputStreamAdaptor does its own buffering and will write data
// in large blocks.
class LIBPROTOBUF_EXPORT CopyingOutputStream {
 public:
  virtual ~CopyingOutputStream();

  // Writes "size" bytes from the given buffer to the output.  Returns true
  // if successful, false on a write error.
  virtual bool Write(const void* buffer, int size) = 0;
};

// A ZeroCopyOutputStream which writes to a CopyingOutputStream.  This is
// useful for implementing ZeroCopyOutputStreams that write to traditional
// streams.  Note that this class is not really zero-copy.
//
// If you want to write to file descriptors or C++ ostreams, this is
// already implemented for you:  use FileOutputStream or OstreamOutputStream
// respectively.
class LIBPROTOBUF_EXPORT CopyingOutputStreamAdaptor : public ZeroCopyOutputStream {
 public:
  // Creates a stream that writes to the given Unix file descriptor.
  // If a block_size is given, it specifies the size of the buffers
  // that should be returned by Next().  Otherwise, a reasonable default
  // is used.
  explicit CopyingOutputStreamAdaptor(CopyingOutputStream* copying_stream,
                                      int block_size = -1);
  ~CopyingOutputStreamAdaptor();

  // Writes all pending data to the underlying stream.  Returns false if a
  // write error occurred on the underlying stream.  (The underlying
  // stream itself is not necessarily flushed.)
  bool Flush();

  // Call SetOwnsCopyingStream(true) to tell the CopyingOutputStreamAdaptor to
  // delete the underlying CopyingOutputStream when it is destroyed.
  void SetOwnsCopyingStream(bool value) { owns_copying_stream_ = value; }

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  // Write the current buffer, if it is present.
  bool WriteBuffer();
  // Insures that buffer_ is not NULL.
  void AllocateBufferIfNeeded();
  // Frees the buffer.
  void FreeBuffer();

  // The underlying copying stream.
  CopyingOutputStream* copying_stream_;
  bool owns_copying_stream_;

  // True if we have seen a permenant error from the underlying stream.
  bool failed_;

  // The current position of copying_stream_, relative to the point where
  // we started writing.
  int64 position_;

  // Data is written from this buffer.  It may be NULL if no buffer is
  // currently in use.  Otherwise, it points to an array of size buffer_size_.
  scoped_array<uint8> buffer_;
  const int buffer_size_;

  // Number of valid bytes currently in the buffer (i.e. the size last
  // returned by Next()).  When BackUp() is called, we just reduce this.
  // 0 <= buffer_used_ <= buffer_size_.
  int buffer_used_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingOutputStreamAdaptor);
};

// ===================================================================

// A ZeroCopyInputStream which reads from a file descriptor.
//
// FileInputStream is preferred over using an ifstream with IstreamInputStream.
// The latter will introduce an extra layer of buffering, harming performance.
// Also, it's conceivable that FileInputStream could someday be enhanced
// to use zero-copy file descriptors on OSs which support them.
class LIBPROTOBUF_EXPORT FileInputStream : public ZeroCopyInputStream {
 public:
  // Creates a stream that reads from the given Unix file descriptor.
  // If a block_size is given, it specifies the number of bytes that
  // should be read and returned with each call to Next().  Otherwise,
  // a reasonable default is used.
  explicit FileInputStream(int file_descriptor, int block_size = -1);
  ~FileInputStream();

  // Flushes any buffers and closes the underlying file.  Returns false if
  // an error occurs during the process; use GetErrno() to examine the error.
  // Even if an error occurs, the file descriptor is closed when this returns.
  bool Close();

  // By default, the file descriptor is not closed when the stream is
  // destroyed.  Call SetCloseOnDelete(true) to change that.  WARNING:
  // This leaves no way for the caller to detect if close() fails.  If
  // detecting close() errors is important to you, you should arrange
  // to close the descriptor yourself.
  void SetCloseOnDelete(bool value) { copying_input_.SetCloseOnDelete(value); }

  // If an I/O error has occurred on this file descriptor, this is the
  // errno from that error.  Otherwise, this is zero.  Once an error
  // occurs, the stream is broken and all subsequent operations will
  // fail.
  int GetErrno() { return copying_input_.GetErrno(); }

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;

 private:
  class LIBPROTOBUF_EXPORT CopyingFileInputStream : public CopyingInputStream {
   public:
    CopyingFileInputStream(int file_descriptor);
    ~CopyingFileInputStream();

    bool Close();
    void SetCloseOnDelete(bool value) { close_on_delete_ = value; }
    int GetErrno() { return errno_; }

    // implements CopyingInputStream ---------------------------------
    int Read(void* buffer, int size);
    int Skip(int count);

   private:
    // The file descriptor.
    const int file_;
    bool close_on_delete_;
    bool is_closed_;

    // The errno of the I/O error, if one has occurred.  Otherwise, zero.
    int errno_;

    // Did we try to seek once and fail?  If so, we assume this file descriptor
    // doesn't support seeking and won't try again.
    bool previous_seek_failed_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingFileInputStream);
  };

  CopyingFileInputStream copying_input_;
  CopyingInputStreamAdaptor impl_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FileInputStream);
};

// ===================================================================

// A ZeroCopyOutputStream which writes to a file descriptor.
//
// FileInputStream is preferred over using an ofstream with OstreamOutputStream.
// The latter will introduce an extra layer of buffering, harming performance.
// Also, it's conceivable that FileInputStream could someday be enhanced
// to use zero-copy file descriptors on OSs which support them.
class LIBPROTOBUF_EXPORT FileOutputStream : public ZeroCopyOutputStream {
 public:
  // Creates a stream that writes to the given Unix file descriptor.
  // If a block_size is given, it specifies the size of the buffers
  // that should be returned by Next().  Otherwise, a reasonable default
  // is used.
  explicit FileOutputStream(int file_descriptor, int block_size = -1);
  ~FileOutputStream();

  // Flushes any buffers and closes the underlying file.  Returns false if
  // an error occurs during the process; use GetErrno() to examine the error.
  // Even if an error occurs, the file descriptor is closed when this returns.
  bool Close();

  // By default, the file descriptor is not closed when the stream is
  // destroyed.  Call SetCloseOnDelete(true) to change that.  WARNING:
  // This leaves no way for the caller to detect if close() fails.  If
  // detecting close() errors is important to you, you should arrange
  // to close the descriptor yourself.
  void SetCloseOnDelete(bool value) { copying_output_.SetCloseOnDelete(value); }

  // If an I/O error has occurred on this file descriptor, this is the
  // errno from that error.  Otherwise, this is zero.  Once an error
  // occurs, the stream is broken and all subsequent operations will
  // fail.
  int GetErrno() { return copying_output_.GetErrno(); }

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  class LIBPROTOBUF_EXPORT CopyingFileOutputStream : public CopyingOutputStream {
   public:
    CopyingFileOutputStream(int file_descriptor);
    ~CopyingFileOutputStream();

    bool Close();
    void SetCloseOnDelete(bool value) { close_on_delete_ = value; }
    int GetErrno() { return errno_; }

    // implements CopyingOutputStream --------------------------------
    bool Write(const void* buffer, int size);

   private:
    // The file descriptor.
    const int file_;
    bool close_on_delete_;
    bool is_closed_;

    // The errno of the I/O error, if one has occurred.  Otherwise, zero.
    int errno_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingFileOutputStream);
  };

  CopyingFileOutputStream copying_output_;
  CopyingOutputStreamAdaptor impl_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FileOutputStream);
};

// ===================================================================

// A ZeroCopyInputStream which reads from a C++ istream.
//
// Note that for reading files (or anything represented by a file descriptor),
// FileInputStream is more efficient.
class LIBPROTOBUF_EXPORT IstreamInputStream : public ZeroCopyInputStream {
 public:
  // Creates a stream that reads from the given C++ istream.
  // If a block_size is given, it specifies the number of bytes that
  // should be read and returned with each call to Next().  Otherwise,
  // a reasonable default is used.
  explicit IstreamInputStream(istream* stream, int block_size = -1);
  ~IstreamInputStream();

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;

 private:
  class LIBPROTOBUF_EXPORT CopyingIstreamInputStream : public CopyingInputStream {
   public:
    CopyingIstreamInputStream(istream* input);
    ~CopyingIstreamInputStream();

    // implements CopyingInputStream ---------------------------------
    int Read(void* buffer, int size);
    // (We use the default implementation of Skip().)

   private:
    // The stream.
    istream* input_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingIstreamInputStream);
  };

  CopyingIstreamInputStream copying_input_;
  CopyingInputStreamAdaptor impl_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(IstreamInputStream);
};

// ===================================================================

// A ZeroCopyOutputStream which writes to a C++ ostream.
//
// Note that for writing files (or anything represented by a file descriptor),
// FileOutputStream is more efficient.
class LIBPROTOBUF_EXPORT OstreamOutputStream : public ZeroCopyOutputStream {
 public:
  // Creates a stream that writes to the given C++ ostream.
  // If a block_size is given, it specifies the size of the buffers
  // that should be returned by Next().  Otherwise, a reasonable default
  // is used.
  explicit OstreamOutputStream(ostream* stream, int block_size = -1);
  ~OstreamOutputStream();

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  class LIBPROTOBUF_EXPORT CopyingOstreamOutputStream : public CopyingOutputStream {
   public:
    CopyingOstreamOutputStream(ostream* output);
    ~CopyingOstreamOutputStream();

    // implements CopyingOutputStream --------------------------------
    bool Write(const void* buffer, int size);

   private:
    // The stream.
    ostream* output_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingOstreamOutputStream);
  };

  CopyingOstreamOutputStream copying_output_;
  CopyingOutputStreamAdaptor impl_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(OstreamOutputStream);
};

// ===================================================================

// A ZeroCopyInputStream which reads from several other streams in sequence.
// ConcatenatingInputStream is unable to distinguish between end-of-stream
// and read errors in the underlying streams, so it assumes any errors mean
// end-of-stream.  So, if the underlying streams fail for any other reason,
// ConcatenatingInputStream may do odd things.  It is suggested that you do
// not use ConcatenatingInputStream on streams that might produce read errors
// other than end-of-stream.
class LIBPROTOBUF_EXPORT ConcatenatingInputStream : public ZeroCopyInputStream {
 public:
  // All streams passed in as well as the array itself must remain valid
  // until the ConcatenatingInputStream is destroyed.
  ConcatenatingInputStream(ZeroCopyInputStream* const streams[], int count);
  ~ConcatenatingInputStream();

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;


 private:
  // As streams are retired, streams_ is incremented and count_ is
  // decremented.
  ZeroCopyInputStream* const* streams_;
  int stream_count_;
  int64 bytes_retired_;  // Bytes read from previous streams.

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ConcatenatingInputStream);
};

// ===================================================================

// A ZeroCopyInputStream which wraps some other stream and limits it to
// a particular byte count.
class LIBPROTOBUF_EXPORT LimitingInputStream : public ZeroCopyInputStream {
 public:
  LimitingInputStream(ZeroCopyInputStream* input, int64 limit);
  ~LimitingInputStream();

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;


 private:
  ZeroCopyInputStream* input_;
  int64 limit_;  // Decreases as we go, becomes negative if we overshoot.

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LimitingInputStream);
};

// ===================================================================

}  // namespace io
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_H__
