
#ifndef UPB_HPP_
#define UPB_HPP_

#include <memory>

#include "upb/upb.h"

namespace upb {

class Status {
 public:
  Status() { upb_status_clear(&status_); }

  upb_status* ptr() { return &status_; }

  // Returns true if there is no error.
  bool ok() const { return upb_ok(&status_); }

  // Guaranteed to be NULL-terminated.
  const char *error_message() const { return upb_status_errmsg(&status_); }

  // The error message will be truncated if it is longer than
  // UPB_STATUS_MAX_MESSAGE-4.
  void SetErrorMessage(const char *msg) { upb_status_seterrmsg(&status_, msg); }
  void SetFormattedErrorMessage(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    upb_status_vseterrf(&status_, fmt, args);
    va_end(args);
  }

  // Resets the status to a successful state with no message.
  void Clear() { upb_status_clear(&status_); }

 private:
  upb_status status_;
};

class Arena {
 public:
  // A simple arena with no initial memory block and the default allocator.
  Arena() : ptr_(upb_arena_new(), upb_arena_free) {}
  Arena(char *initial_block, size_t size)
      : ptr_(upb_arena_init(initial_block, size, &upb_alloc_global),
             upb_arena_free) {}

  upb_arena* ptr() { return ptr_.get(); }

  // Allows this arena to be used as a generic allocator.
  //
  // The arena does not need free() calls so when using Arena as an allocator
  // it is safe to skip them.  However they are no-ops so there is no harm in
  // calling free() either.
  upb_alloc *allocator() { return upb_arena_alloc(ptr_.get()); }

  // Add a cleanup function to run when the arena is destroyed.
  // Returns false on out-of-memory.
  template <class T>
  bool Own(T *obj) {
    return upb_arena_addcleanup(ptr_.get(), obj, [](void* obj) {
      delete static_cast<T*>(obj);
    });
  }

  void Fuse(Arena& other) { upb_arena_fuse(ptr(), other.ptr()); }

 private:
  std::unique_ptr<upb_arena, decltype(&upb_arena_free)> ptr_;
};

// InlinedArena seeds the arenas with a predefined amount of memory.  No
// heap memory will be allocated until the initial block is exceeded.
template <int N>
class InlinedArena : public Arena {
 public:
  InlinedArena() : Arena(initial_block_, N) {}

 private:
  InlinedArena(const InlinedArena*) = delete;
  InlinedArena& operator=(const InlinedArena*) = delete;

  char initial_block_[N];
};

}  // namespace upb

#endif // UPB_HPP_
