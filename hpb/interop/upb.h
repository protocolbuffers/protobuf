#ifndef GOOGLE_PROTOBUF_HPB_INTEROP_UPB_H__
#define GOOGLE_PROTOBUF_HPB_INTEROP_UPB_H__

#include "google/protobuf/hpb/ptr.h"
#include "upb/mini_table/message.h"

namespace hpb::interop::upb {
template <typename T>
const upb_MiniTable* GetMiniTable(const T*) {
  return T::minitable();
}

template <typename T>
const upb_MiniTable* GetMiniTable(Ptr<T>) {
  return T::minitable();
}
}  // namespace hpb::interop::upb

#endif  // GOOGLE_PROTOBUF_HPB_INTEROP_UPB_H__
