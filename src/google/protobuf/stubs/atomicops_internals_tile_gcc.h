#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_TILE_GCC_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_TILE_GCC_H_

namespace google {
namespace protobuf {
namespace internal {

inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
    asm volatile ("st4 %0, %1"
                  : "=r" (*ptr)
                  : "m" (value));
}

inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr) {
    Atomic32 dest;
    asm volatile ("ld4s %0, %1"
                  : "=r" (dest)
                  : "m" (*ptr));
    return dest;
}

inline void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value) {
    asm volatile ("st %0, %1"
                  : "=r" (*ptr)
                  : "m" (value));
}

inline Atomic64 NoBarrier_Load(volatile const Atomic64* ptr) {
    Atomic64 dest;
    asm volatile ("ld %0, %1"
                  : "=r" (dest)
                  : "m" (*ptr));
    return dest;
}

inline Atomic64 Acquire_Load(volatile const Atomic64* ptr) {
    return NoBarrier_Load(ptr);
}

inline void Release_Store(volatile Atomic64* ptr, Atomic64 value) {
    NoBarrier_Store(ptr, value);
}

inline Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
    Atomic64 tmp;
    asm volatile ("mtspr CMPEXCH_VALUE, %3\n\t"
                  "cmpexch %0, %1, %2"
                  : "=r" (tmp), "=m" (*ptr)
                  : "r" (new_value), "r" (old_value));
    return tmp;
}

inline Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr,
                                         Atomic64 new_value) {
    Atomic64 old_value;
    asm volatile ("exch %0, %1, %2"
                  : "=r" (old_value), "=m" (*ptr)
                  : "r" (new_value));
    return old_value;
}

inline Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr,
                                          Atomic64 value) {
    Atomic64 dest;
    asm volatile ("fetchadd %0, %1, %2"
                  : "=r" (dest), "=m" (*ptr)
                  : "r" (value));
    return *ptr;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_TILE_GCC_H_
