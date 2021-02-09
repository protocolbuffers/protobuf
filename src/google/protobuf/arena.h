// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

// This file defines an Arena allocator for better allocation performance.

#ifndef GOOGLE_PROTOBUF_ARENA_H__
#define GOOGLE_PROTOBUF_ARENA_H__


#include <limits>
#include <type_traits>
#include <utility>
#ifdef max
#undef max  // Visual Studio defines this macro
#endif
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

#include <type_traits>
#include <google/protobuf/arena_impl.h>
#include <google/protobuf/port.h>

#include <google/protobuf/port_def.inc>

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

struct ArenaOptions;  // defined below

}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {

class Arena;    // defined below
class Message;  // defined in message.h
class MessageLite;
template <typename Key, typename T>
class Map;

namespace arena_metrics {

void EnableArenaMetrics(ArenaOptions* options);

}  // namespace arena_metrics

namespace internal {

struct ArenaStringPtr;  // defined in arenastring.h
class LazyField;        // defined in lazy_field.h
class EpsCopyInputStream;  // defined in parse_context.h

template <typename Type>
class GenericTypeHandler;  // defined in repeated_field.h

// Templated cleanup methods.
template <typename T>
void arena_destruct_object(void* object) {
  reinterpret_cast<T*>(object)->~T();
}
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
  size_t start_block_size;

  // This defines the maximum block size requested from system malloc (unless an
  // individual arena allocation request occurs with a size larger than this
  // maximum). Requested block sizes increase up to this value, then remain
  // here.
  size_t max_block_size;

  // An initial block of memory for the arena to use, or NULL for none. If
  // provided, the block must live at least as long as the arena itself. The
  // creator of the Arena retains ownership of the block after the Arena is
  // destroyed.
  char* initial_block;

  // The size of the initial block, if provided.
  size_t initial_block_size;

  // A function pointer to an alloc method that returns memory blocks of size
  // requested. By default, it contains a ptr to the malloc function.
  //
  // NOTE: block_alloc and dealloc functions are expected to behave like
  // malloc and free, including Asan poisoning.
  void* (*block_alloc)(size_t);
  // A function pointer to a dealloc method that takes ownership of the blocks
  // from the arena. By default, it contains a ptr to a wrapper function that
  // calls free.
  void (*block_dealloc)(void*, size_t);

  ArenaOptions()
      : start_block_size(kDefaultStartBlockSize),
        max_block_size(kDefaultMaxBlockSize),
        initial_block(NULL),
        initial_block_size(0),
        block_alloc(kDefaultBlockAlloc),
        block_dealloc(&internal::ArenaFree),
        make_metrics_collector(nullptr) {}

  PROTOBUF_EXPORT static void* (*const kDefaultBlockAlloc)(size_t);

 private:
  // If make_metrics_collector is not nullptr, it will be called at Arena init
  // time. It may return a pointer to a collector instance that will be notified
  // of interesting events related to the arena.
  internal::ArenaMetricsCollector* (*make_metrics_collector)();

  // Constants define default starting block size and max block size for
  // arena allocator behavior -- see descriptions above.
  static const size_t kDefaultStartBlockSize =
      internal::ArenaImpl::kDefaultStartBlockSize;
  static const size_t kDefaultMaxBlockSize =
      internal::ArenaImpl::kDefaultMaxBlockSize;

  friend void arena_metrics::EnableArenaMetrics(ArenaOptions*);

  friend class Arena;
  friend class ArenaOptionsTestFriend;
  friend class internal::ArenaImpl;
};

// Support for non-RTTI environments. (The metrics hooks API uses type
// information.)
#if PROTOBUF_RTTI
#define RTTI_TYPE_ID(type) (&typeid(type))
#else
#define RTTI_TYPE_ID(type) (NULL)
#endif

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
//   NULL arena pointer, it must be equivalent to invoking the first
//   (`args`-only) constructor.
//
// - The type T must have a particular type trait: a nested type
//   |InternalArenaConstructable_|. This is usually a typedef to |void|. If no
//   such type trait exists, then the instantiation CreateMessage<T> will fail
//   to compile.
//
// - The type T *may* have the type trait |DestructorSkippable_|. If this type
//   trait is present in the type, then its destructor will not be called if and
//   only if it was passed a non-NULL arena pointer. If this type trait is not
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
  explicit Arena(const ArenaOptions& options) : impl_(options) {}

  // Block overhead.  Use this as a guide for how much to over-allocate the
  // initial block if you want an allocation of size N to fit inside it.
  //
  // WARNING: if you allocate multiple objects, it is difficult to guarantee
  // that a series of allocations will fit in the initial block, especially if
  // Arena changes its alignment guarantees in the future!
  static const size_t kBlockOverhead = internal::ArenaImpl::kBlockHeaderSize +
                                       internal::ArenaImpl::kSerialArenaSize;

  inline ~Arena() {}

  // TODO(protobuf-team): Fix callers to use constructor and delete this method.
  void Init(const ArenaOptions&) {}

  // API to create proto2 message objects on the arena. If the arena passed in
  // is NULL, then a heap allocated object is returned. Type T must be a message
  // defined in a .proto file with cc_enable_arenas set to true, otherwise a
  // compilation error will occur.
  //
  // RepeatedField and RepeatedPtrField may also be instantiated directly on an
  // arena with this method.
  //
  // This function also accepts any type T that satisfies the arena message
  // allocation protocol, documented above.
  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* CreateMessage(Arena* arena, Args&&... args) {
    static_assert(
        InternalHelper<T>::is_arena_constructable::value,
        "CreateMessage can only construct types that are ArenaConstructable");
    // We must delegate to CreateMaybeMessage() and NOT CreateMessageInternal()
    // because protobuf generated classes specialize CreateMaybeMessage() and we
    // need to use that specialization for code size reasons.
    return Arena::CreateMaybeMessage<T>(arena, std::forward<Args>(args)...);
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
  PROTOBUF_ALWAYS_INLINE static T* Create(Arena* arena, Args&&... args) {
    return CreateNoMessage<T>(arena, is_arena_constructable<T>(),
                              std::forward<Args>(args)...);
  }

  // Create an array of object type T on the arena *without* invoking the
  // constructor of T. If `arena` is null, then the return value should be freed
  // with `delete[] x;` (or `::operator delete[](x);`).
  // To ensure safe uses, this function checks at compile time
  // (when compiled as C++11) that T is trivially default-constructible and
  // trivially destructible.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE static T* CreateArray(Arena* arena,
                                               size_t num_elements) {
    static_assert(std::is_pod<T>::value,
                  "CreateArray requires a trivially constructible type");
    static_assert(std::is_trivially_destructible<T>::value,
                  "CreateArray requires a trivially destructible type");
    GOOGLE_CHECK_LE(num_elements, std::numeric_limits<size_t>::max() / sizeof(T))
        << "Requested size is too large to fit into size_t.";
    if (arena == NULL) {
      return static_cast<T*>(::operator new[](num_elements * sizeof(T)));
    } else {
      return arena->CreateInternalRawArray<T>(num_elements);
    }
  }

  // Returns the total space allocated by the arena, which is the sum of the
  // sizes of the underlying blocks. This method is relatively fast; a counter
  // is kept as blocks are allocated.
  uint64 SpaceAllocated() const { return impl_.SpaceAllocated(); }
  // Returns the total space used by the arena. Similar to SpaceAllocated but
  // does not include free space and block overhead. The total space returned
  // may not include space used by other threads executing concurrently with
  // the call to this method.
  uint64 SpaceUsed() const { return impl_.SpaceUsed(); }

  // Frees all storage allocated by this arena after calling destructors
  // registered with OwnDestructor() and freeing objects registered with Own().
  // Any objects allocated on this arena are unusable after this call. It also
  // returns the total space used by the arena which is the sums of the sizes
  // of the allocated blocks. This method is not thread-safe.
  uint64 Reset() { return impl_.Reset(); }

  // Adds |object| to a list of heap-allocated objects to be freed with |delete|
  // when the arena is destroyed or reset.
  template <typename T>
  PROTOBUF_NOINLINE void Own(T* object) {
    OwnInternal(object, std::is_convertible<T*, Message*>());
  }

  // Adds |object| to a list of objects whose destructors will be manually
  // called when the arena is destroyed or reset. This differs from Own() in
  // that it does not free the underlying memory with |delete|; hence, it is
  // normally only used for objects that are placement-newed into
  // arena-allocated memory.
  template <typename T>
  PROTOBUF_NOINLINE void OwnDestructor(T* object) {
    if (object != NULL) {
      impl_.AddCleanup(object, &internal::arena_destruct_object<T>);
    }
  }

  // Adds a custom member function on an object to the list of destructors that
  // will be manually called when the arena is destroyed or reset. This differs
  // from OwnDestructor() in that any member function may be specified, not only
  // the class destructor.
  PROTOBUF_NOINLINE void OwnCustomDestructor(void* object,
                                             void (*destruct)(void*)) {
    impl_.AddCleanup(object, destruct);
  }

  // Retrieves the arena associated with |value| if |value| is an arena-capable
  // message, or NULL otherwise. If possible, the call resolves at compile time.
  // Note that we can often devirtualize calls to `value->GetArena()` so usually
  // calling this method is unnecessary.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE static Arena* GetArena(const T* value) {
    return GetArenaInternal(value);
  }

  template <typename T>
  class InternalHelper {
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

    template <typename U,
              typename std::enable_if<
                  std::is_same<Arena*, decltype(std::declval<const U>()
                                                    .GetArena())>::value,
                  int>::type = 0>
    static char HasGetArena(decltype(&U::GetArena));
    template <typename U>
    static double HasGetArena(...);

    typedef std::integral_constant<bool, sizeof(HasGetArena<T>(nullptr)) ==
                                             sizeof(char)>
        has_get_arena;

    template <typename... Args>
    static T* Construct(void* ptr, Args&&... args) {
      return new (ptr) T(std::forward<Args>(args)...);
    }

    static Arena* GetArena(const T* p) { return p->GetArena(); }

    friend class Arena;
  };

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
  template <typename T>
  struct has_get_arena : InternalHelper<T>::has_get_arena {};

  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* CreateMessageInternal(Arena* arena,
                                                         Args&&... args) {
    static_assert(
        InternalHelper<T>::is_arena_constructable::value,
        "CreateMessage can only construct types that are ArenaConstructable");
    if (arena == NULL) {
      return new T(nullptr, std::forward<Args>(args)...);
    } else {
      return arena->DoCreateMessage<T>(std::forward<Args>(args)...);
    }
  }

  // This specialization for no arguments is necessary, because its behavior is
  // slightly different.  When the arena pointer is nullptr, it calls T()
  // instead of T(nullptr).
  template <typename T>
  PROTOBUF_ALWAYS_INLINE static T* CreateMessageInternal(Arena* arena) {
    static_assert(
        InternalHelper<T>::is_arena_constructable::value,
        "CreateMessage can only construct types that are ArenaConstructable");
    if (arena == NULL) {
      return new T();
    } else {
      return arena->DoCreateMessage<T>();
    }
  }

  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* CreateInternal(Arena* arena,
                                                  Args&&... args) {
    if (arena == NULL) {
      return new T(std::forward<Args>(args)...);
    } else {
      return arena->DoCreate<T>(std::is_trivially_destructible<T>::value,
                                std::forward<Args>(args)...);
    }
  }

  inline void AllocHook(const std::type_info* allocated_type, size_t n) const {
    impl_.RecordAlloc(allocated_type, n);
  }

  // Allocate and also optionally call collector with the allocated type info
  // when allocation recording is enabled.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE void* AllocateInternal(bool skip_explicit_ownership) {
    const size_t n = internal::AlignUpTo8(sizeof(T));
    // Monitor allocation if needed.
    impl_.RecordAlloc(RTTI_TYPE_ID(T), n);
    if (skip_explicit_ownership) {
      return AllocateAlignedTo<alignof(T)>(sizeof(T));
    } else {
      if (alignof(T) <= 8) {
        return impl_.AllocateAlignedAndAddCleanup(
            n, &internal::arena_destruct_object<T>);
      } else {
        auto ptr =
            reinterpret_cast<uintptr_t>(impl_.AllocateAlignedAndAddCleanup(
                sizeof(T) + alignof(T) - 8,
                &internal::arena_destruct_object<T>));
        return reinterpret_cast<void*>((ptr + alignof(T) - 8) &
                                       (~alignof(T) + 1));
      }
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
    return CreateInternal<T>(arena, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* CreateMaybeMessage(Arena* arena,
                                                      Args&&... args) {
    return DoCreateMaybeMessage<T>(arena, is_arena_constructable<T>(),
                                   std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* CreateNoMessage(Arena* arena, std::true_type,
                                                   Args&&... args) {
    // User is constructing with Create() despite the fact that T supports arena
    // construction.  In this case we have to delegate to CreateInternal(), and
    // we can't use any CreateMaybeMessage() specialization that may be defined.
    return CreateInternal<T>(arena, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE static T* CreateNoMessage(Arena* arena,
                                                   std::false_type,
                                                   Args&&... args) {
    // User is constructing with Create() and the type does not support arena
    // construction.  In this case we can delegate to CreateMaybeMessage() and
    // use any specialization that may be available for that.
    return CreateMaybeMessage<T>(arena, std::forward<Args>(args)...);
  }

  // Just allocate the required size for the given type assuming the
  // type has a trivial constructor.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE T* CreateInternalRawArray(size_t num_elements) {
    GOOGLE_CHECK_LE(num_elements, std::numeric_limits<size_t>::max() / sizeof(T))
        << "Requested size is too large to fit into size_t.";
    // We count on compiler to realize that if sizeof(T) is a multiple of
    // 8 AlignUpTo can be elided.
    const size_t n = internal::AlignUpTo8(sizeof(T) * num_elements);
    // Monitor allocation if needed.
    impl_.RecordAlloc(RTTI_TYPE_ID(T), n);
    return static_cast<T*>(AllocateAlignedTo<alignof(T)>(n));
  }

  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE T* DoCreate(bool skip_explicit_ownership,
                                     Args&&... args) {
    return new (AllocateInternal<T>(skip_explicit_ownership))
        T(std::forward<Args>(args)...);
  }
  template <typename T, typename... Args>
  PROTOBUF_ALWAYS_INLINE T* DoCreateMessage(Args&&... args) {
    return InternalHelper<T>::Construct(
        AllocateInternal<T>(InternalHelper<T>::is_destructor_skippable::value),
        this, std::forward<Args>(args)...);
  }

  // CreateInArenaStorage is used to implement map field. Without it,
  // Map need to call generated message's protected arena constructor,
  // which needs to declare Map as friend of generated message.
  template <typename T, typename... Args>
  static void CreateInArenaStorage(T* ptr, Arena* arena, Args&&... args) {
    CreateInArenaStorageInternal(ptr, arena,
                                 typename is_arena_constructable<T>::type(),
                                 std::forward<Args>(args)...);
    RegisterDestructorInternal(
        ptr, arena,
        typename InternalHelper<T>::is_destructor_skippable::type());
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

  // These implement Own(), which registers an object for deletion (destructor
  // call and operator delete()). The second parameter has type 'true_type' if T
  // is a subtype of Message and 'false_type' otherwise. Collapsing
  // all template instantiations to one for generic Message reduces code size,
  // using the virtual destructor instead.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE void OwnInternal(T* object, std::true_type) {
    if (object != NULL) {
      impl_.AddCleanup(object, &internal::arena_delete_object<Message>);
    }
  }
  template <typename T>
  PROTOBUF_ALWAYS_INLINE void OwnInternal(T* object, std::false_type) {
    if (object != NULL) {
      impl_.AddCleanup(object, &internal::arena_delete_object<T>);
    }
  }

  // Implementation for GetArena(). Only message objects with
  // InternalArenaConstructable_ tags can be associated with an arena, and such
  // objects must implement a GetArena() method.
  template <typename T, typename std::enable_if<
                            is_arena_constructable<T>::value, int>::type = 0>
  PROTOBUF_ALWAYS_INLINE static Arena* GetArenaInternal(const T* value) {
    return InternalHelper<T>::GetArena(value);
  }
  template <typename T,
            typename std::enable_if<!is_arena_constructable<T>::value &&
                                        has_get_arena<T>::value,
                                    int>::type = 0>
  PROTOBUF_ALWAYS_INLINE static Arena* GetArenaInternal(const T* value) {
    return value->GetArena();
  }
  template <typename T,
            typename std::enable_if<!is_arena_constructable<T>::value &&
                                        !has_get_arena<T>::value,
                                    int>::type = 0>
  PROTOBUF_ALWAYS_INLINE static Arena* GetArenaInternal(const T* value) {
    (void)value;
    return nullptr;
  }

  // For friends of arena.
  void* AllocateAligned(size_t n) {
    return AllocateAlignedNoHook(internal::AlignUpTo8(n));
  }
  template<size_t Align>
  void* AllocateAlignedTo(size_t n) {
    static_assert(Align > 0, "Alignment must be greater than 0");
    static_assert((Align & (Align - 1)) == 0, "Alignment must be power of two");
    if (Align <= 8) return AllocateAligned(n);
    // TODO(b/151247138): if the pointer would have been aligned already,
    // this is wasting space. We should pass the alignment down.
    uintptr_t ptr = reinterpret_cast<uintptr_t>(AllocateAligned(n + Align - 8));
    ptr = (ptr + Align - 1) & (~Align + 1);
    return reinterpret_cast<void*>(ptr);
  }

  void* AllocateAlignedNoHook(size_t n);

  internal::ArenaImpl impl_;

  template <typename Type>
  friend class internal::GenericTypeHandler;
  friend struct internal::ArenaStringPtr;  // For AllocateAligned.
  friend class internal::LazyField;        // For CreateMaybeMessage.
  friend class internal::EpsCopyInputStream;  // For parser performance
  friend class MessageLite;
  template <typename Key, typename T>
  friend class Map;
};

// Defined above for supporting environments without RTTI.
#undef RTTI_TYPE_ID

}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_ARENA_H__
