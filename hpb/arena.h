#ifndef GOOGLE_PROTOBUF_HPB_ARENA_H__
#define GOOGLE_PROTOBUF_HPB_ARENA_H__

#ifdef HPB_BACKEND_UPB
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#else
#error hpb backend must be specified
#endif

namespace hpb {

#ifdef HPB_BACKEND_UPB
using RawArena = upb_Arena;
using RAIIArena = upb::Arena;
#endif

class Arena final {
 public:
  Arena(RawArena* arena) : arena_(arena) {}
  Arena(RAIIArena& raii_arena) : arena_(raii_arena.ptr()) {}
  RawArena* ptr() const { return arena_; }

 private:
  RawArena* arena_;
};
}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_ARENA_H__
