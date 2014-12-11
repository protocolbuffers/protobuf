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

// This header is logically internal, but is made public because it is used
// from protocol-compiler-generated code, which may reside in other components.

#ifndef GOOGLE_PROTOBUF_ARENA_H__
#define GOOGLE_PROTOBUF_ARENA_H__

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/atomic_sequence_num.h>
#include <google/protobuf/stubs/atomicops.h>
#include <google/protobuf/stubs/type_traits.h>

namespace google {
namespace protobuf {

class Arena;       // defined below
class Message;     // message.h

namespace internal {
class ArenaString; // arenastring.h
class LazyField;   // lazy_field.h

template<typename Type>
class GenericTypeHandler; // repeated_field.h

// Templated cleanup methods.
template<typename T> void arena_destruct_object(void* object) {
  reinterpret_cast<T*>(object)->~T();
}
template<typename T> void arena_delete_object(void* object) {
  delete reinterpret_cast<T*>(object);
}
inline void arena_free(void* object, size_t size) {
  free(object);
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
        block_alloc(&malloc),
        block_dealloc(&internal::arena_free) {}

 private:
  // Constants define default starting block size and max block size for
  // arena allocator behavior -- see descriptions above.
  static const size_t kDefaultStartBlockSize = 256;
  static const size_t kDefaultMaxBlockSize   = 8192;
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
class LIBPROTOBUF_EXPORT Arena {
 public:
  // Arena constructor taking custom options. See ArenaOptions below for
  // descriptions of the options available.
  explicit Arena(const ArenaOptions& options) {
    Init(options);
  }

  // Default constructor with sensible default options, tuned for average
  // use-cases.
  Arena() {
    Init(ArenaOptions());
  }

  // Destructor deletes all owned heap allocated objects, and destructs objects
  // that have non-trivial destructors, except for proto2 message objects whose
  // destructors can be skipped. Also, frees all blocks except the initial block
  // if it was passed in.
  ~Arena() {
    Reset();
  }

  // API to create proto2 message objects on the arena. If the arena passed in
  // is NULL, then a heap allocated object is returned. Type T must be a message
  // defined in a .proto file with cc_enable_arenas set to true, otherwise a
  // compilation error will occur.
  //
  // RepeatedField and RepeatedPtrField may also be instantiated directly on an
  // arena with this method: they act as "arena-capable message types" for the
  // purposes of the Arena API.
  template <typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static T* CreateMessage(::google::protobuf::Arena* arena) {
    if (arena == NULL) {
      return new T;
    } else {
      return arena->CreateMessageInternal<T>(static_cast<T*>(0));
    }
  }

  // API to create any objects on the arena. Note that only the object will
  // be created on the arena; the underlying ptrs (in case of a proto2 message)
  // will be still heap allocated. Proto messages should usually be allocated
  // with CreateMessage<T>() instead.
  template <typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static T* Create(::google::protobuf::Arena* arena) {
    if (arena == NULL) {
      return new T();
    } else {
      return arena->CreateInternal<T>(
          SkipDeleteList<T>(static_cast<T*>(0)));
    }
  }

  // Version of the above with one constructor argument for the created object.
  template <typename T, typename Arg> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static T* Create(::google::protobuf::Arena* arena, const Arg& arg) {
    if (arena == NULL) {
      return new T(arg);
    } else {
      return arena->CreateInternal<T>(SkipDeleteList<T>(static_cast<T*>(0)),
                                      arg);
    }
  }

  // Version of the above with two constructor arguments for the created object.
  template <typename T, typename Arg1, typename Arg2> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static T* Create(::google::protobuf::Arena* arena, const Arg1& arg1, const Arg2& arg2) {
    if (arena == NULL) {
      return new T(arg1, arg2);
    } else {
      return arena->CreateInternal<T>(SkipDeleteList<T>(static_cast<T*>(0)),
                                      arg1,
                                      arg2);
    }
  }

  // Create an array of object type T on the arena. Type T must have a trivial
  // constructor, as it will not be invoked when created on the arena.
  template <typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static T* CreateArray(::google::protobuf::Arena* arena, size_t num_elements) {
    if (arena == NULL) {
      return new T[num_elements];
    } else {
      return static_cast<T*>(
          arena->AllocateAligned(num_elements * sizeof(T)));
    }
  }

  // Returns the total space used by the arena, which is the sums of the sizes
  // of the underlying blocks. The total space used may not include the new
  // blocks that are allocated by this arena from other threads concurrently
  // with the call to this method.
  uint64 SpaceUsed() const GOOGLE_ATTRIBUTE_NOINLINE;

  // Frees all storage allocated by this arena after calling destructors
  // registered with OwnDestructor() and freeing objects registered with Own().
  // Any objects allocated on this arena are unusable after this call. It also
  // returns the total space used by the arena which is the sums of the sizes
  // of the allocated blocks. This method is not thread-safe.
  uint64 Reset() GOOGLE_ATTRIBUTE_NOINLINE;

  // Adds |object| to a list of heap-allocated objects to be freed with |delete|
  // when the arena is destroyed or reset.
  template <typename T> GOOGLE_ATTRIBUTE_NOINLINE
  void Own(T* object) {
    OwnInternal(object, google::protobuf::internal::is_convertible<T*, ::google::protobuf::Message*>());
  }

  // Adds |object| to a list of objects whose destructors will be manually
  // called when the arena is destroyed or reset. This differs from Own() in
  // that it does not free the underlying memory with |delete|; hence, it is
  // normally only used for objects that are placement-newed into
  // arena-allocated memory.
  template <typename T> GOOGLE_ATTRIBUTE_NOINLINE
  void OwnDestructor(T* object) {
    if (object != NULL) {
      AddListNode(object, &internal::arena_destruct_object<T>);
    }
  }

  // Adds a custom member function on an object to the list of destructors that
  // will be manually called when the arena is destroyed or reset. This differs
  // from OwnDestructor() in that any member function may be specified, not only
  // the class destructor.
  void OwnCustomDestructor(void* object, void (*destruct)(void*))
      GOOGLE_ATTRIBUTE_NOINLINE {
    AddListNode(object, destruct);
  }

  // Retrieves the arena associated with |value| if |value| is an arena-capable
  // message, or NULL otherwise. This differs from value->GetArena() in that the
  // latter is a virtual call, while this method is a templated call that
  // resolves at compile-time.
  template<typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static inline ::google::protobuf::Arena* GetArena(T* value) {
    return GetArenaInternal(value, static_cast<T*>(0));
  }

  // Helper typetrait that indicates support for arenas in a type T at compile
  // time. This is public only to allow construction of higher-level templated
  // utilities. is_arena_constructable<T>::value is an instance of
  // google::protobuf::internal::true_type if the message type T has arena support enabled, and
  // google::protobuf::internal::false_type otherwise.
  //
  // This is inside Arena because only Arena has the friend relationships
  // necessary to see the underlying generated code traits.
  template<typename T>
  struct is_arena_constructable {
    template<typename U>
    static char ArenaConstructable(
        const typename U::InternalArenaConstructable_*);
    template<typename U>
    static double ArenaConstructable(...);

    // This will resolve to either google::protobuf::internal::true_type or google::protobuf::internal::false_type.
    typedef google::protobuf::internal::integral_constant<bool,
              sizeof(ArenaConstructable<const T>(static_cast<const T*>(0))) ==
              sizeof(char)> type;
    static const type value;
  };

 private:
  // Blocks are variable length malloc-ed objects.  The following structure
  // describes the common header for all blocks.
  struct Block {
    void* owner;   // &ThreadCache of thread that owns this block, or
                   // &this->owner if not yet owned by a thread.
    Block* next;   // Next block in arena (may have different owner)
    // ((char*) &block) + pos is next available byte. It is always
    // aligned at a multiple of 8 bytes.
    size_t pos;
    size_t size;  // total size of the block.
    size_t avail() const GOOGLE_ATTRIBUTE_ALWAYS_INLINE { return size - pos; }
    // data follows
  };

  void* (*block_alloc)(size_t);  // Allocates a free block of a particular size.
  void (*block_dealloc)(void*, size_t);  // Deallocates the given block.

  template<typename Type> friend class ::google::protobuf::internal::GenericTypeHandler;
  friend class MockArena;              // For unit-testing.
  friend class internal::ArenaString;  // For AllocateAligned.
  friend class internal::LazyField;    // For CreateMaybeMessage.

  struct ThreadCache {
    // The ThreadCache is considered valid as long as this matches the
    // lifecycle_id of the arena being used.
    int64 last_lifecycle_id_seen;
    Block* last_block_used_;
  };

  static const size_t kHeaderSize = sizeof(Block);
  static google::protobuf::internal::SequenceNumber lifecycle_id_generator_;
#ifdef PROTOBUF_USE_DLLS
  static ThreadCache& thread_cache();
#else
  static GOOGLE_THREAD_LOCAL ThreadCache thread_cache_;
  static ThreadCache& thread_cache() { return thread_cache_; }
#endif

  // SFINAE for skipping addition to delete list for a Type. This is mainly to
  // skip proto2/proto1 message objects with cc_enable_arenas=true from being
  // part of the delete list. Also, note, compiler will optimize out the branch
  // in CreateInternal<T>.
  //
  template<typename T>
  static inline bool SkipDeleteList(typename T::DestructorSkippable_*) {
    return true;
  }

  // For non message objects, we skip addition to delete list if the object has
  // a trivial destructor.
  template<typename T>
  static inline bool SkipDeleteList(...) {
    return google::protobuf::internal::has_trivial_destructor<T>::value;
  }

  // CreateMessage<T> requires that T supports arenas, but this private method
  // works whether or not T supports arenas. These are not exposed to user code
  // as it can cause confusing API usages, and end up having double free in
  // user code. These are used only internally from LazyField and Repeated
  // fields, since they are designed to work in all mode combinations.
  template<typename Msg> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static Msg* CreateMaybeMessage(
      Arena* arena, typename Msg::InternalArenaConstructable_*) {
    return CreateMessage<Msg>(arena);
  }

  template<typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static T* CreateMaybeMessage(Arena* arena, ...) {
    return Create<T>(arena);
  }

  template <typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  inline T* CreateInternal(
      bool skip_explicit_ownership) {
    T* t = new (AllocateAligned(sizeof(T))) T();
    if (!skip_explicit_ownership) {
      AddListNode(t, &internal::arena_destruct_object<T>);
    }
    return t;
  }

  template <typename T, typename Arg> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  inline T* CreateInternal(
      bool skip_explicit_ownership, const Arg& arg) {
    T* t = new (AllocateAligned(sizeof(T))) T(arg);
    if (!skip_explicit_ownership) {
      AddListNode(t, &internal::arena_destruct_object<T>);
    }
    return t;
  }

  template <typename T, typename Arg1, typename Arg2> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  inline T* CreateInternal(
      bool skip_explicit_ownership, const Arg1& arg1, const Arg2& arg2) {
    T* t = new (AllocateAligned(sizeof(T))) T(arg1, arg2);
    if (!skip_explicit_ownership) {
      AddListNode(t, &internal::arena_destruct_object<T>);
    }
    return t;
  }

  template <typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  inline T* CreateMessageInternal(typename T::InternalArenaConstructable_*) {
    return CreateInternal<T, Arena*>(SkipDeleteList<T>(static_cast<T*>(0)),
                                     this);
  }

  // These implement Own(), which registers an object for deletion (destructor
  // call and operator delete()). The second parameter has type 'true_type' if T
  // is a subtype of ::google::protobuf::Message and 'false_type' otherwise. Collapsing
  // all template instantiations to one for generic Message reduces code size,
  // using the virtual destructor instead.
  template<typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  void OwnInternal(T* object, google::protobuf::internal::true_type) {
    if (object != NULL) {
      AddListNode(object, &internal::arena_delete_object< ::google::protobuf::Message >);
    }
  }
  template<typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  void OwnInternal(T* object, google::protobuf::internal::false_type) {
    if (object != NULL) {
      AddListNode(object, &internal::arena_delete_object<T>);
    }
  }

  // Implementation for GetArena(). Only message objects with
  // InternalArenaConstructable_ tags can be associated with an arena, and such
  // objects must implement a GetArenaNoVirtual() method.
  template<typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static inline ::google::protobuf::Arena* GetArenaInternal(T* value,
      typename T::InternalArenaConstructable_*) {
    return value->GetArenaNoVirtual();
  }

  template<typename T> GOOGLE_ATTRIBUTE_ALWAYS_INLINE
  static inline ::google::protobuf::Arena* GetArenaInternal(T* value, ...) {
    return NULL;
  }


  void* AllocateAligned(size_t size);

  void Init(const ArenaOptions& options);

  // Free all blocks and return the total space used which is the sums of sizes
  // of the all the allocated blocks.
  uint64 FreeBlocks();

  // Add object pointer and cleanup function pointer to the list.
  // TODO(rohananil, cfallin): We could pass in a sub-arena into this method
  // to avoid polluting blocks of this arena with list nodes. This would help in
  // mixed mode (where many protobufs have cc_enable_arenas=false), and is an
  // alternative to a chunked linked-list, but with extra overhead of *next.
  void AddListNode(void* elem, void (*cleanup)(void*));
  // Delete or Destruct all objects owned by the arena.
  void CleanupList();

  inline void SetThreadCacheBlock(Block* block) {
    thread_cache().last_block_used_ = block;
    thread_cache().last_lifecycle_id_seen = lifecycle_id_;
  }

  int64 lifecycle_id_;  // Unique for each arena. Changes on Reset().
  size_t start_block_size_;  // Starting block size of the arena.
  size_t max_block_size_;    // Max block size of the arena.

  google::protobuf::internal::AtomicWord blocks_;  // Head of linked list of all allocated blocks
  google::protobuf::internal::AtomicWord hint_;    // Fast thread-local block access

  // Node contains the ptr of the object to be cleaned up and the associated
  // cleanup function ptr.
  struct Node {
    void* elem;              // Pointer to the object to be cleaned up.
    void (*cleanup)(void*);  // Function pointer to the destructor or deleter.
    Node* next;              // Next node in the list.
  };

  google::protobuf::internal::AtomicWord cleanup_list_;  // Head of a linked list of nodes containing object
                             // ptrs and cleanup methods.

  bool owns_first_block_;    // Indicates that arena owns the first block
  Mutex blocks_lock_;

  void AddBlock(Block* b);
  void* SlowAlloc(size_t n);
  Block* FindBlock(void* me);
  Block* NewBlock(void* me, Block* my_last_block, size_t n,
                  size_t start_block_size, size_t max_block_size);
  static void* AllocFromBlock(Block* b, size_t n);
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Arena);
};

template<typename T>
const typename Arena::is_arena_constructable<T>::type
    Arena::is_arena_constructable<T>::value =
        typename Arena::is_arena_constructable<T>::type();

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_ARENA_H__
