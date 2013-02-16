//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// Functionality for interoperating with Cord.  Only needed inside Google.

#ifndef UPB_GOOGLE_CORD_H
#define UPB_GOOGLE_CORD_H

#include "strings/cord.h"
#include "upb/bytestream.h"

namespace upb {

namespace proto2_bridge_google3 { class FieldAccessor; }
namespace proto2_bridge_opensource { class FieldAccessor; }

namespace google {

class P2R_Handlers;

class CordSupport {
 private:
  UPB_DISALLOW_POD_OPS(CordSupport);

  inline static void AssignToCord(const upb::ByteRegion* r, Cord* cord) {
    // TODO(haberman): ref source data if source is a cord.
    cord->Clear();
    uint64_t ofs = r->start_ofs();
    while (ofs < r->end_ofs()) {
      size_t len;
      const char *buf = r->GetPtr(ofs, &len);
      cord->Append(StringPiece(buf, len));
      ofs += len;
    }
  }

  friend class ::upb::proto2_bridge_google3::FieldAccessor;
  friend class ::upb::proto2_bridge_opensource::FieldAccessor;
  friend class P2R_Handlers;
};

}  // namespace google
}  // namespace upb

#endif  // UPB_GOOGLE_CORD_H
