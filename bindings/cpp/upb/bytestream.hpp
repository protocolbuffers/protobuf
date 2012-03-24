//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// This file defines three core interfaces:
// - upb::ByteSink: for writing streams of data.
// - upb::ByteSource: for reading streams of data.
// - upb::ByteRegion: for reading from a specific region of a ByteSource;
//   should be used by decoders instead of using a ByteSource directly.
//
// These interfaces are used by streaming encoders and decoders: for example, a
// protobuf parser gets its input from a upb::ByteRegion.  They are virtual
// base classes so concrete implementations can get the data from a fd, a
// FILE*, a string, etc.
//
// A ByteRegion represents a region of data from a ByteSource.
//
// Parsers get data from this interface instead of a bytesrc because we often
// want to parse only a specific region of the input.  For example, if we parse
// a string from our input but know that the string represents a protobuf, we
// can pass its ByteRegion to an appropriate protobuf parser.
//
// Since the bytes may be coming from a file or network socket, bytes must be
// fetched before they can be read (though in some cases this fetch may be a
// no-op).  "fetch" is the only operation on a byteregion that could fail or
// block, because it is the only operation that actually performs I/O.
//
// Bytes can be discarded when they are no longer needed.  Parsers should
// always discard bytes they no longer need, both so the buffers can be freed
// when possible and to give better visibility into what bytes the parser is
// still using.
//
// start      discard                     read             fetch             end
// ofs          ofs                       ofs               ofs              ofs
// |             |--->Discard()            |                 |--->Fetch()      |
// V             V                         V                 V                 V
// +-------------+-------------------------+-----------------+-----------------+
// |  discarded  |                         |                 |    fetchable    |
// +-------------+-------------------------+-----------------+-----------------+
//               | <------------- loaded ------------------> |
//                                         | <- available -> |
//                                         | <---------- remaining ----------> |
//
// Note that the start offset may be something other than zero!  A byteregion
// is a view into an underlying bytesrc stream, and the region may start
// somewhere other than the beginning of that stream.
//
// The region can be either delimited or nondelimited.  A non-delimited region
// will keep returning data until the underlying data source returns EOF.  A
// delimited region will return EOF at a predetermined offset.
//
//                       end
//                       ofs
//                         |
//                         V
// +-----------------------+
// |  delimited region     |   <-- hard EOF, even if data source has more data.
// +-----------------------+
//
// +------------------------
// | nondelimited region   Z   <-- won't return EOF until data source hits EOF.
// +------------------------

#ifndef UPB_BYTESTREAM_HPP
#define UPB_BYTESTREAM_HPP

#include "upb/bytestream.h"
#include "upb/upb.hpp"
#include <string>

namespace upb {

typedef upb_bytesuccess_t ByteSuccess;

// Implement this interface to vend bytes to ByteRegions which will be used by
// a decoder.
class ByteSourceBase : public upb_bytesrc {
 public:
  ByteSourceBase() { upb_bytesrc_init(this, vtable()); }
  virtual ~ByteSourceBase() { upb_bytesrc_uninit(this); }

  // Fetches at least one byte starting at ofs, setting *len to the actual
  // number of bytes fetched (or 0 on EOF or error: see return value for
  // details).  It is valid for bytes to be fetched multiple times, as long as
  // the bytes have not been previously discarded.
  virtual ByteSuccess Fetch(uint64_t ofs, size_t* len) = 0;

  // Discards all data prior to ofs (except data that is pinned, if pinning
  // support is added -- see TODO below).
  virtual void Discard(uint64_t ofs) = 0;

  // Copies "len" bytes of data from ofs to "dst", which must be at least "len"
  // bytes long.  The given region must not be discarded.
  virtual void Copy(uint64_t ofs, size_t len, char *dst) const = 0;

  // Returns a pointer to the bytesrc's internal buffer, storing in *len how
  // much data is available.  The given offset must not be discarded.  The
  // returned buffer is valid for as long as its bytes are not discarded (in
  // the case that part of the returned buffer is discarded, only the
  // non-discarded bytes remain valid).
  virtual const char *GetPtr(uint64_t ofs, size_t *len) const = 0;

  // TODO: Add if/when there is a demonstrated need:
  //
  // // When the caller pins a region (which must not be already discarded), it
  // // is guaranteed that the region will not be discarded (nor will the
  // // bytesrc be destroyed) until the region is unpinned.  However, not all
  // // bytesrc's support pinning; a false return indicates that a pin was not
  // // possible.
  // virtual bool Pin(uint64_t ofs, size_t len);
  //
  // // Releases some number of pinned bytes from the beginning of a pinned
  // // region (which may be fewer than the total number of bytes pinned).
  // virtual void Unpin(uint64_t ofs, size_t len, size_t bytes_to_release);
  //
  // Adding pinning support would also involve adding a "pin_ofs" parameter to
  // upb_bytesrc_fetch, so that the fetch can extend an already-pinned region.
 private:
  static upb_bytesrc_vtbl* vtable();
  static upb_bytesuccess_t VFetch(void*, uint64_t, size_t*);
  static void VDiscard(void*, uint64_t);
  static void VCopy(const void*, uint64_t, size_t, char*);
  static const char *VGetPtr(const void*, uint64_t, size_t*);
};

class ByteRegion : public upb_byteregion {
 public:
  static const uint64_t kNondelimited = UPB_NONDELIMITED;

  ByteRegion() { upb_byteregion_init(this); }
  ~ByteRegion() { upb_byteregion_uninit(this); }

  // Accessors for the regions bounds -- the meaning of these is described in
  // the diagram above.
  uint64_t start_ofs() const { return upb_byteregion_startofs(this); }
  uint64_t discard_ofs() const { return upb_byteregion_discardofs(this); }
  uint64_t fetch_ofs() const { return upb_byteregion_fetchofs(this); }
  uint64_t end_ofs() const { return upb_byteregion_endofs(this); }

  // Returns how many bytes are fetched and available for reading starting from
  // offset "offset".
  uint64_t BytesAvailable(uint64_t offset) const {
    return upb_byteregion_available(this, offset);
  }

  // Returns the total number of bytes remaining after offset "offset", or
  // kNondelimited if the byteregion is non-delimited.
  uint64_t BytesRemaining(uint64_t offset) const {
    return upb_byteregion_remaining(this, offset);
  }

  uint64_t Length() const { return upb_byteregion_len(this); }

  // Sets the value of this byteregion to be a subset of the given byteregion's
  // data.  The caller is responsible for releasing this region before the src
  // region is released (unless the region is first pinned, if pinning support
  // is added.  see below).
  void Reset(const upb_byteregion *src, uint64_t ofs, uint64_t len) {
    upb_byteregion_reset(this, src, ofs, len);
  }
  void Release() { upb_byteregion_release(this); }

  // Attempts to fetch more data, extending the fetched range of this
  // byteregion.  Returns true if the fetched region was extended by at least
  // one byte, false on EOF or error (see *s for details).
  ByteSuccess Fetch() { return upb_byteregion_fetch(this); }

  // Fetches all remaining data, returning false if the operation failed (see
  // *s for details).  May only be used on delimited byteregions.
  ByteSuccess FetchAll() { return upb_byteregion_fetchall(this); }

  // Discards bytes from the byteregion up until ofs (which must be greater or
  // equal to discard_ofs()).  It is valid to discard bytes that have not been
  // fetched (such bytes will never be fetched) but it is an error to discard
  // past the end of a delimited byteregion.
  void Discard(uint64_t ofs) { return upb_byteregion_discard(this, ofs); }

  // Copies "len" bytes of data into "dst", starting at ofs.  The specified
  // region must be available.
  void Copy(uint64_t ofs, size_t len, char *dst) const {
    upb_byteregion_copy(this, ofs, len, dst);
  }

  // Copies all bytes from the byteregion into dst.  Requires that the entire
  // byteregion is fetched and that none has been discarded.
  void CopyAll(char *dst) const {
    upb_byteregion_copyall(this, dst);
  }

  // Returns a pointer to the internal buffer for the byteregion starting at
  // offset "ofs." Stores the number of bytes available in this buffer in *len.
  // The returned buffer is invalidated when the byteregion is reset or
  // released, or when the bytes are discarded.  If the byteregion is not
  // currently pinned, the pointer is only valid for the lifetime of the parent
  // byteregion.
  const char *GetPtr(uint64_t ofs, size_t *len) const {
    return upb_byteregion_getptr(this, ofs, len);
  }

  // Copies the contents of the byteregion into a newly-allocated,
  // NULL-terminated string.  Requires that the byteregion is fully fetched.
  char *StrDup() const {
    return upb_byteregion_strdup(this);
  }

  template <typename T> void AssignToString(T* str) {
    uint64_t ofs = start_ofs();
    str->clear();
    str->reserve(Length());
    while (ofs < end_ofs()) {
      size_t len;
      const char *ptr = GetPtr(ofs, &len);
      str->append(ptr, len);
      ofs += len;
    }
  }

  // TODO: add if/when there is a demonstrated need.
  //
  // // Pins this byteregion's bytes in memory, allowing it to outlive its
  // // parent byteregion.  Normally a byteregion may only be used while its
  // // parent is still valid, but a pinned byteregion may continue to be used
  // // until it is reset or released.  A byteregion must be fully fetched to
  // // be pinned (this implies that the byteregion must be delimited).
  // //
  // // In some cases this operation may cause the input data to be copied.
  // //
  // // void Pin();
};

class StringSource : public upb_stringsrc {
 public:
  StringSource() : upb_stringsrc() { upb_stringsrc_init(this); }
  template <typename T> explicit StringSource(const T& str) {
    upb_stringsrc_init(this);
    Reset(str);
  }
  StringSource(const char *data, size_t len) {
    upb_stringsrc_init(this);
    Reset(data, len);
  }
  ~StringSource() { upb_stringsrc_uninit(this); }

  void Reset(const char* data, size_t len) {
    upb_stringsrc_reset(this, data, len);
  }

  template <typename T> void Reset(const T& str) {
    Reset(str.c_str(), str.size());
  }

  ByteRegion* AllBytes() {
    return static_cast<ByteRegion*>(upb_stringsrc_allbytes(this));
  }

  upb_bytesrc* ByteSource() { return upb_stringsrc_bytesrc(this); }
};

template <> inline ByteRegion* GetValue<ByteRegion*>(Value v) {
  return static_cast<ByteRegion*>(upb_value_getbyteregion(v));
}

template <> inline Value MakeValue<ByteRegion*>(ByteRegion* v) {
  return upb_value_byteregion(v);
}

}  // namespace upb

#endif
