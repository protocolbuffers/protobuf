//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>

#include "bytestream.hpp"

namespace upb {

upb_bytesrc_vtbl* ByteSourceBase::vtable() {
  static upb_bytesrc_vtbl vtbl = {
    &ByteSourceBase::VFetch,
    &ByteSourceBase::VDiscard,
    &ByteSourceBase::VCopy,
    &ByteSourceBase::VGetPtr,
  };
  return &vtbl;
}

upb_bytesuccess_t ByteSourceBase::VFetch(void *src, uint64_t ofs, size_t *len) {
  return static_cast<ByteSourceBase*>(src)->Fetch(ofs, len);
}

void ByteSourceBase::VCopy(
    const void *src, uint64_t ofs, size_t len, char* dest) {
  static_cast<const ByteSourceBase*>(src)->Copy(ofs, len, dest);
}

void ByteSourceBase::VDiscard(void *src, uint64_t ofs) {
  static_cast<ByteSourceBase*>(src)->Discard(ofs);
}

const char * ByteSourceBase::VGetPtr(
    const void *src, uint64_t ofs, size_t* len) {
  return static_cast<const ByteSourceBase*>(src)->GetPtr(ofs, len);
}

}  // namespace upb
