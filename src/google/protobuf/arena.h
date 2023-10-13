// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file defines an Arena allocator for better allocation performance.

#ifndef GOOGLE_PROTOBUF_ARENA_H__
#define GOOGLE_PROTOBUF_ARENA_H__

#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#if defined(_MSC_VER) && !defined(_LIBCPP_STD_VER) && !_HAS_EXCEPTIONS
// Work around bugs in MSVC <typeinfo> header when _HAS_EXCEPTIONS=0.
#include <exception>
#include <typeinfo>
namespace std {
using type_info = ::type_info;
}
#else
#include <typeinfo>
#endif

#include "absl/log/absl_check.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/port.h"
#include "google/protobuf/serial_arena.h"
#include "google/protobuf/thread_safe_arena.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

struct ArenaOptions;  // defined below
class Arena;    // defined below
class Message;  // defined in message.h
class MessageLite;
template <typename Key, typename T>
class Map;

namespace arena_metrics {

void EnableArenaMetrics(ArenaOptions* options);

}  // namespace arena_metrics

namespace TestUtil {
class ReflectionTester;  // defined in test_util.h
}  // namespace TestUtil

namespace internal {

struct ArenaTestPeer;        // defined in arena_test_util.h
class InternalMetadata;      // defined in metadata_lite.h
class LazyField;             // defined in lazy_field.h
class EpsCopyInputStream;    // defined in parse_context.h
class RepeatedPtrFieldBase;  // defined in repeated_ptr_field.h
class TcParser;              // defined in generated_message_tctable_impl.h

template <typename Type>
class GenericTypeHandler;  // defined in repeated_field.h

template <bool destructor_skippable, typename T>
struct ObjectDestructor {
  constexpr static void (*destructor)(void*) =
      &internal::cleanup::arena_destruct_object<T>;
};

template <typename T>
struct ObjectDestructor<true, T> {
  constexpr static void (*destructor)(void*) = nullptr;
};

template <typename T>
void arena_delete_object(void* object) {
  delete reinterpret_cast<T*>(object);
}
}  // namespace internal

// ArenaOptions provides optional additional parameters to arena construction
// that control its block-allocation behavior.
struct ArenaOptions {
  // This defines the size of the first block requested from the system malloc.
  // Subsequent block sizes will increase in a geometric series up to a maximum.
  size_t start_block_size = internal::AllocationPolicy::kDefaultStartBlockSize;

  // This defines the maximum block size requested from system malloc (unless an
  // individual arena allocation request occurs with a size larger than this
  // maximum). Requested block sizes increase up to this value, then remain
  // here.
  size_t max_block_size = internal::AllocationPolicy::kDefaultMaxBlockSize;

  // An initial block of memory for the arena to use, or nullptr for none. If
  // provided, the block must live at least as long as the arena itself. The
  // creator of the Arena retains ownership of the block after the Arena is
  // destroyed.
  char* initial_block = nullptr;

  // The size of the initial block, if provided.
  size_t initial_block_size = 0;

  // A function pointer to an alloc method that returns memory blocks of size
  // requested. By default, it contains a ptr to the malloc function.
  //
  // NOTE: block_alloc and dealloc functions are expected to behave like
  // malloc and free, including Asan poisoning.
  void* (*block_alloc)(size_t) = nullptr;
  // A function pointer to a dealloc method that takes ownership of the blocks
  // from the arena. By default, it contains a ptr to a wrapper function that
  // calls free.
  void (*block_dealloc)(void*, size_t) = nullptr;

 private:
  internal::AllocationPolicy AllocationPolicy() const {
    internal::AllocationPolicy res;
    res.start_block_size = start_block_size;
    res.max_block_size = max_block_size;
    res.block_alloc = block_alloc;
    res.block_dealloc = block_dealloc;
    return res;
  }

  friend class Arena;
  friend class ArenaOptionsTestFriend;
};

// Arena allocator. Arena allocation replaces ordinary (heap-based) allocation
// with new/delete, and improves performance by aggregating allocations into
// larger blocks and freeing allocations all at once. Protocol messages are
// allocated on an arena by using Arena::CreateMessage<T>(Arena*), below, and
// are automatically freed when the arena is destroyed.
//
// This is a thread-safe implementation: multiple threads may allocate from the
// arena concurrently. Destruction is not thread-safe and the destructing
// thread must synchronize with users of the arena first.
//
// An arena provides two allocation interfaces: CreateMessage<T>, which works
// for arena-enabled proto2 message types as well as other types that satisfy
// the appropriate protocol (described below), and Create<T>, which works for
// any arbitrary type T. CreateMessage<T> is better when the type T supports it,
// because this interface (i) passes the arena pointer to the created object so
// that its sub-objects and internal allocations can use the arena too, and (ii)
// elides the object's destructor call when possible. Create<T> does not place
// any special requirements on the type T, and will invoke the object's
// destructor when the arena is destroyed.
//
// The arena message allocation protocol, required by
// CreateMessage<T>(Arena* arena, Args&&... args), is as follows:
//
// - The type T must have (at least) two constructors: a constructor callable
//   with `args` (without `arena`), called when a T is allocated on the heap;
//   and a constructor callable with `Arena* arena, Args&&... args`, called when
//   a T is allocated on an arena. If the second constructor is called with a
//   null arena pointer, it must be equivalent to invoking the first
//   (`args`-only) constructor.
//
// - The type T must have a particular type trait: a nested type
//   |InternalArenaConstructable_|. This is usually a typedef to |void|. If no
//   such type trait exists, then the instantiation CreateMessage<T> will fail
//   to compile.
//
// - The type T *may* have the type trait |DestructorSkippable_|. If this type
//   trait is present in the type, then its destructor will not be called if and
//   only if it was passed a non-null arena pointer. If this type trait is not
//   present on the type, then its destructor is always called when the
//   containing arena is destroyed.
//
// This protocol is implemented by all arena-enabled proto2 message classes as
// well as protobuf container types like RepeatedPtrField and Map. The protocol
// is internal to protobuf and is not guaranteed to be stable. Non-proto types
// should not rely on this protocol.
class PROTOBUF_EXPORT PROTOBUF_ALIGNAS(8) Arena final {
 public:
  // Default constructor with sensible default options, tuned for average
  // use-cases.
  inline Arena() : impl_() {}

  // Construct an arena with default options, except for the supplied
  // initial block. It is more efficient to use this constructor
  // instead of passing ArenaOptions if the only configuration needed
  // by the caller is supplying an initial block.
  inline Arena(char* initial_block, size_t initial_block_size)
      : impl_(initial_block, initial_block_size) {}

  // Arena constructor taking custom options. See ArenaOptions above for
  // descriptions of the options available.
  explicit Arena(const ArenaOptions& options)
      : impl_(options.initial_block, options.initial_block_size,
              options.AllocationPolicy()) {}

  // Block overhead.  Use this as a guide for how much to over-allocate the
  // initial block if you want an allocation of size N to fit inside it.
  //
  // WARNING: if you allocate multiple objects, it is difficult to guarantee
  // that a series of allocations will fit in the initial block, especially if
  // Arena changes its alignment guarantees in the future!
  static const size_t kBlockOverhead =
      internal::ThreadSafeArena::kBlockHeaderSize +
      internal::ThreadSafeArena::kSerialArenaSize;

  inline ~Arena() {}

  // API to create proto2 message objects on the arena. If the arena passed in
  // is nullptr, then a heap allocated object is returned. Type T must be a
  // message defined in a .proto file with cc_enable_arenas set to true,
  // otherwise a compilation error will occur.
  //
  // RepeatedField and RepeatedPtrField may also be instantiated directly on an
  // arena with this method.
  //
  // This function also accepts any type T that satisfies the arena message
  // allocation protocol, documented above.
  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* CreateMessage(Arena* arena, Args&&... args) {
    static_assert(
        is_arena_constructable<T>::value,
        "CreateMessage can only construct types that are ArenaConstructable");
    // We must delegate to CreateMaybeMessage() and NOT CreateMessageInternal()
    // because protobuf generated classes specialize CreateMaybeMessage() and we
    // need to use that specialization for code size reasons.
    return Arena::CreateMaybeMessage<T>(arena, static_cast<Args&&>(args)...);
  }

  // API to create any objects on the arena. Note that only the object will
  // be created on the arena; the underlying ptrs (in case of a proto2 message)
  // will be still heap allocated. Proto messages should usually be allocated
  // with CreateMessage<T>() instead.
  //
  // Note that even if T satisfies the arena message construction protocol
  // (InternalArenaConstructable_ trait and optional DestructorSkippable_
  // trait), as described above, this function does not follow the protocol;
  // instead, it treats T as a black-box type, just as if it did not have these
  // traits. Specifically, T's constructor arguments will always be only those
  // passed to Create<T>() -- no additional arena pointer is implicitly added.
  // Furthermore, the destructor will always be called at arena destruction time
  // (unless the destructor is trivial). Hence, from T's point of view, it is as
  // if the object were allocated on the heap (except that the underlying memory
  // is obtained from the arena).
  template <typename T, typename... Args>
  PROTOBUF_NDEBUG_INLINE static T* Create(Arena* arena, Args&&... args) {
    if (PROTOBUF_PREDICT_FALSE(arena == nullptr)) {
      return new T(std::forward<Args>(args)...);
    }
    return new (arena->AllocateInternal<T>()) T(std::forward<Args>(args)...);
  }

  // API to delete any objects not on an arena.  This can be used to safely
  // clean up messages or repeated fields without knowing whether or not they're
  // owned by an arena.  The pointer passed to this function should not be used
  // again.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE static void Destroy(T* obj) {
    if (InternalGetOwningArena(obj) == nullptr) delete obj;
  }

  // Allocates memory with the specific size and alignment.
  void* AllocateAligned(size_t size, size_t align = 8) {
    if (align <= internal::ArenaAlignDefault::align) {
      return Allocate(internal::ArenaAlignDefault::Ceil(size));
    } else {
      // We are wasting space by over allocating align - 8 bytes. Compared
      // to a dedicated function that takes current alignment in consideration.
      // Such a scheme would only waste (align - 8)/2 bytes on average, but
      // requires a dedicated function in the outline arena allocation
      // functions. Possibly re-evaluate tradeoffs later.
      auto align_as = internal::ArenaAlignAs(align);
      return align_as.Ceil(Allocate(align_as.Padded(size)));
    }
  }

  // Create an array of object type T on the arena *without* invoking the
  // constructor of T. If `arena` is null, then the return value should be freed
  // with `delete[] x;` (or `::operator delete[](x);`).
  // To ensure safe uses, this function checks at compile time
  // (when compiled as C++11) that T is trivially default-constructible and
  // trivially destructible.
  template <typename T>
  PROTOBUF_NDEBUG_INLINE static T* CreateArray(Arena* arena,
                                               size_t num_elements) {
    static_assert(std::is_trivial<T>::value,
                  "CreateArray requires a trivially constructible type");
    static_assert(std::is_trivially_destructible<T>::value,
                  "CreateArray requires a trivially destructible type");
    ABSL_CHECK_LE(num_elements, std::numeric_limits<size_t>::max() / sizeof(T))
        << "Requested size is too large to fit into size_t.";
    if (PROTOBUF_PREDICT_FALSE(arena == nullptr)) {
      return static_cast<T*>(::operator new[](num_elements * sizeof(T)));
    } else {
      // We count on compiler to realize that if sizeof(T) is a multiple of
      // 8 AlignUpTo can be elided.
      return static_cast<T*>(
          arena->AllocateAlignedForArray(sizeof(T) * num_elements, alignof(T)));
    }
  }

  // The following are routines are for monitoring. They will approximate the
  // total sum allocated and used memory, but the exact value is an
  // implementation deal. For instance allocated space depends on growth
  // policies. Do not use these in unit tests.
  // Returns the total space allocated by the arena, which is the sum of the
  // sizes of the underlying blocks.
  uint64_t SpaceAllocated() const { return impl_.SpaceAllocated(); }
  // Returns the total space used by the arena. Similar to SpaceAllocated but
  // does not include free space and block overhead.  This is a best-effort
  // estimate and may inaccurately calculate space used by other threads
  // executing concurrently with the call to this method.  These inaccuracies
  // are due to race conditions, and are bounded but unpredictable.  Stale data
  // can lead to underestimates of the space used, and race conditions can lead
  // to overestimates (up to the current block size).
  uint64_t SpaceUsed() const { return impl_.SpaceUsed(); }

  // Frees all storage allocated by this arena after calling destructors
  // registered with OwnDestructor() and freeing objects registered with Own().
  // Any objects allocated on this arena are unusable after this call. It also
  // returns the total space used by the arena which is the sums of the sizes
  // of the allocated blocks. This method is not thread-safe.
  uint64_t Reset() { return impl_.Reset(); }

  // Adds |object| to a list of heap-allocated objects to be freed with |delete|
  // when the arena is destroyed or reset.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE void Own(T* object) {
    // Collapsing all template instantiations to one for generic Message reduces
    // code size, using the virtual destructor instead.
    using TypeToUse =
        std::conditional_t<std::is_convertible<T*, MessageLite*>::value,
                           MessageLite, T>;
    if (object != nullptr) {
      impl_.AddCleanup(static_cast<TypeToUse*>(object),
                       &internal::arena_delete_object<TypeToUse>);
    }
  }

  // Adds |object| to a list of objects whose destructors will be manually
  // called when the arena is destroyed or reset. This differs from Own() in
  // that it does not free the underlying memory with |delete|; hence, it is
  // normally only used for objects that are placement-newed into
  // arena-allocated memory.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE void OwnDestructor(T* object) {
    if (object != nullptr) {
      impl_.AddCleanup(object, &internal::cleanup::arena_destruct_object<T>);
    }
  }

  // Adds a custom member function on an object to the list of destructors that
  // will be manually called when the arena is destroyed or reset. This differs
  // from OwnDestructor() in that any member function may be specified, not only
  // the class destructor.
  PROTOBUF_ALWAYS_INLINE void OwnCustomDestructor(void* object,
                                                  void (*destruct)(void*)) {
    impl_.AddCleanup(object, destruct);
  }

  // Retrieves the arena associated with |value| if |value| is an arena-capable
  // message, or nullptr otherwise. If possible, the call resolves at compile
  // time. Note that we can often devirtualize calls to `value->GetArena()` so
  // usually calling this method is unnecessary.
  // TODO: remove this function.
  template <typename T>
  ABSL_DEPRECATED(
      "This will be removed in a future release. Call value->GetArena() "
      "instead.")
  PROTOBUF_ALWAYS_INLINE static Arena* GetArena(T* value) {
    return GetArenaInternal(value);
  }

  template <typename T>
  class InternalHelper {
   private:
    // A SFINAE friendly trait that probes for `U` but always evalues to
    // `Arena*`.
    template <typename U>
    using EnableIfArena =
        typename std::enable_if<std::is_same<Arena*, U>::value, Arena*>::type;

    // Rather than use SFINAE that must fully cover the space of options in a
    // mutually exclusive fashion, we use implicit conversions to base classes
    // to force an explicit ranking for our preferences.  The lowest ranked
    // version that compiles will be accepted.
    struct Rank1 {};
    struct Rank0 : Rank1 {};

    static Arena* GetOwningArena(const T* p) {
      return GetOwningArena(Rank0{}, p);
    }

    template <typename U>
    static auto GetOwningArena(Rank0, const U* p)
        -> EnableIfArena<decltype(p->GetOwningArena())> {
      return p->GetOwningArena();
    }

    template <typename U>
    static Arena* GetOwningArena(Rank1, const U*) {
      return nullptr;
    }

    static void InternalSwap(T* a, T* b) { a->InternalSwap(b); }

    static Arena* GetArena(T* p) { return GetArena(Rank0{}, p); }

    template <typename U>
    static auto GetArena(Rank0, U* p)
        -> EnableIfArena<decltype(p->GetArena())> {
      return p->GetArena();
    }

    template <typename U>
    static Arena* GetArena(Rank1, U*) {
      return nullptr;
    }

    template <typename U>
    static char DestructorSkippable(const typename U::DestructorSkippable_*);
    template <typename U>
    static double DestructorSkippable(...);

    typedef std::integral_constant<
        bool, sizeof(DestructorSkippable<T>(static_cast<const T*>(0))) ==
                      sizeof(char) ||
                  std::is_trivially_destructible<T>::value>
        is_destructor_skippable;

    template <typename U>
    static char ArenaConstructable(
        const typename U::InternalArenaConstructable_*);
    template <typename U>
    static double ArenaConstructable(...);

    typedef std::integral_constant<bool, sizeof(ArenaConstructable<T>(
                                             static_cast<const T*>(0))) ==
                                             sizeof(char)>
        is_arena_constructable;


    template <typename... Args>
    static T* Construct(void* ptr, Args&&... args) {
      return new (ptr) T(static_cast<Args&&>(args)...);
    }

    static inline PROTOBUF_ALWAYS_INLINE T* New() {
      return new T(nullptr);
    }

    friend class Arena;
    friend class TestUtil::ReflectionTester;
  };

  // Provides access to protected GetOwningArena to generated messages.  For
  // internal use only.
  template <typename T>
  static Arena* InternalGetOwningArena(T* p) {
    ABSL_DCHECK_EQ(InternalHelper<T>::GetOwningArena(p),
                   InternalHelper<T>::GetArena(p));
    return InternalHelper<T>::GetOwningArena(p);
  }

  // Wraps InternalGetArena() and will be removed soon.
  // For internal use only.
  template <typename T>
  static Arena* InternalGetArenaForAllocation(T* p) {
    return InternalHelper<T>::GetArena(p);
  }

  // Provides access to protected GetArena to generated messages.
  // For internal use only.
  template <typename T>
  static Arena* InternalGetArena(T* p) {
    ABSL_DCHECK_EQ(InternalHelper<T>::GetOwningArena(p),
                   InternalHelper<T>::GetArena(p));
    return InternalHelper<T>::GetArena(p);
  }

  // Helper typetraits that indicates support for arenas in a type T at compile
  // time. This is public only to allow construction of higher-level templated
  // utilities.
  //
  // is_arena_constructable<T>::value is true if the message type T has arena
  // support enabled, and false otherwise.
  //
  // is_destructor_skippable<T>::value is true if the message type T has told
  // the arena that it is safe to skip the destructor, and false otherwise.
  //
  // This is inside Arena because only Arena has the friend relationships
  // necessary to see the underlying generated code traits.
  template <typename T>
  struct is_arena_constructable : InternalHelper<T>::is_arena_constructable {};
  template <typename T>
  struct is_destructor_skippable : InternalHelper<T>::is_destructor_skippable {
  };

 private:
  internal::ThreadSafeArena impl_;

  void ReturnArrayMemory(void* p, size_t size) {
    impl_.ReturnArrayMemory(p, size);
  }

  template <typename T, typename... Args>
  PROTOBUF_NDEBUG_INLINE static T* CreateMessageInternal(Arena* arena,
                                                         Args&&... args) {
    static_assert(
        is_arena_constructable<T>::value,
        "CreateMessage can only construct types that are ArenaConstructable");
    if (PROTOBUF_PREDICT_FALSE(arena == nullptr)) {
      return new T(nullptr, static_cast<Args&&>(args)...);
    } else {
      return arena->DoCreateMessage<T>(static_cast<Args&&>(args)...);
    }
  }

  // This specialization for no arguments is necessary, because its behavior is
  // slightly different.  When the arena pointer is nullptr, it calls T()
  // instead of T(nullptr).
  template <typename T>
  PROTOBUF_NDEBUG_INLINE static T* CreateMessageInternal(Arena* arena) {
    static_assert(
        is_arena_constructable<T>::value,
        "CreateMessage can only construct types that are ArenaConstructable");
    if (PROTOBUF_PREDICT_FALSE(arena == nullptr)) {
      // Generated arena constructor T(Arena*) is protected. Call via
      // InternalHelper.
      return InternalHelper<T>::New();
    } else {
      return arena->DoCreateMessage<T>();
    }
  }

  template <typename T, bool trivial = std::is_trivially_destructible<T>::value>
  PROTOBUF_NDEBUG_INLINE void* AllocateInternal() {
    if (trivial) {
      return AllocateAligned(sizeof(T), alignof(T));
    } else {
      // We avoid instantiating arena_destruct_object<T> in the trivial case.
      constexpr auto dtor = &internal::cleanup::arena_destruct_object<
          std::conditional_t<trivial, std::string, T>>;
      return AllocateAlignedWithCleanup(sizeof(T), alignof(T), dtor);
    }
  }

  // CreateMessage<T> requires that T supports arenas, but this private method
  // works whether or not T supports arenas. These are not exposed to user code
  // as it can cause confusing API usages, and end up having double free in
  // user code. These are used only internally from LazyField and Repeated
  // fields, since they are designed to work in all mode combinations.
  template <typename Msg, typename... Args>
  PROTOBUF_ALWAYS_INLINE static Msg* DoCreateMaybeMessage(Arena* arena,
                                                          std::true_type,
                                                          Args&&... args) {
    return CreateMessageInternal<Msg>(arena, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* DoCreateMaybeMessage(Arena* arena,
                                                        std::false_type,
                                                        Args&&... args) {
    return Create<T>(arena, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* CreateMaybeMessage(Arena* arena,
                                                      Args&&... args) {
    return DoCreateMaybeMessage<T>(arena, is_arena_constructable<T>(),
                                   std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  PROTOBUF_NDEBUG_INLINE T* DoCreateMessage(Args&&... args) {
    return InternalHelper<T>::Construct(
        AllocateInternal<T, is_destructor_skippable<T>::value>(), this,
        std::forward<Args>(args)...);
  }

  // CreateInArenaStorage is used to implement map field. Without it,
  // Map need to call generated message's protected arena constructor,
  // which needs to declare Map as friend of generated message.
  template <typename T, typename... Args>
  static void CreateInArenaStorage(T* ptr, Arena* arena, Args&&... args) {
    CreateInArenaStorageInternal(ptr, arena, is_arena_constructable<T>(),
                                 std::forward<Args>(args)...);
    if (PROTOBUF_PREDICT_TRUE(arena != nullptr)) {
      RegisterDestructorInternal(ptr, arena, is_destructor_skippable<T>());
    }
  }

  template <typename T, typename... Args>
  static void CreateInArenaStorageInternal(T* ptr, Arena* arena,
                                           std::true_type, Args&&... args) {
    InternalHelper<T>::Construct(ptr, arena, std::forward<Args>(args)...);
  }
  template <typename T, typename... Args>
  static void CreateInArenaStorageInternal(T* ptr, Arena* /* arena */,
                                           std::false_type, Args&&... args) {
    new (ptr) T(std::forward<Args>(args)...);
  }

  template <typename T>
  static void RegisterDestructorInternal(T* /* ptr */, Arena* /* arena */,
                                         std::true_type) {}
  template <typename T>
  static void RegisterDestructorInternal(T* ptr, Arena* arena,
                                         std::false_type) {
    arena->OwnDestructor(ptr);
  }

  // Implementation for GetArena(). Only message objects with
  // InternalArenaConstructable_ tags can be associated with an arena, and such
  // objects must implement a GetArena() method.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE static Arena* GetArenaInternal(T* value) {
    return InternalHelper<T>::GetArena(value);
  }

  void* AllocateAlignedForArray(size_t n, size_t align) {
    if (align <= internal::ArenaAlignDefault::align) {
      return AllocateForArray(internal::ArenaAlignDefault::Ceil(n));
    } else {
      // We are wasting space by over allocating align - 8 bytes. Compared
      // to a dedicated function that takes current alignment in consideration.
      // Such a scheme would only waste (align - 8)/2 bytes on average, but
      // requires a dedicated function in the outline arena allocation
      // functions. Possibly re-evaluate tradeoffs later.
      auto align_as = internal::ArenaAlignAs(align);
      return align_as.Ceil(AllocateForArray(align_as.Padded(n)));
    }
  }

  void* Allocate(size_t n);
  void* AllocateForArray(size_t n);
  void* AllocateAlignedWithCleanup(size_t n, size_t align,
                                   void (*destructor)(void*));

  // Test only API.
  // It returns the objects that are in the cleanup list for the current
  // SerialArena. This API is meant for tests that want to see if something was
  // added or not to the cleanup list. Sometimes adding something to the cleanup
  // list has no visible side effect so peeking into the list is the only way to
  // test.
  std::vector<void*> PeekCleanupListForTesting();

  template <typename Type>
  friend class internal::GenericTypeHandler;
  friend class internal::InternalMetadata;  // For user_arena().
  friend class internal::LazyField;        // For CreateMaybeMessage.
  friend class internal::EpsCopyInputStream;  // For parser performance
  friend class internal::TcParser;            // For parser performance
  friend class MessageLite;
  template <typename Key, typename T>
  friend class Map;
  template <typename>
  friend class RepeatedField;                   // For ReturnArrayMemory
  friend class internal::RepeatedPtrFieldBase;  // For ReturnArrayMemory
  friend struct internal::ArenaTestPeer;
};

template <>
inline void* Arena::AllocateInternal<std::string, false>() {
  return impl_.AllocateFromStringBlock();
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_H__
