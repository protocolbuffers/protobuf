/*
** upb::RefCounted (upb_refcounted)
**
** A refcounting scheme that supports circular refs.  It accomplishes this by
** partitioning the set of objects into groups such that no cycle spans groups;
** we can then reference-count the group as a whole and ignore refs within the
** group.  When objects are mutable, these groups are computed very
** conservatively; we group any objects that have ever had a link between them.
** When objects are frozen, we compute strongly-connected components which
** allows us to be precise and only group objects that are actually cyclic.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_REFCOUNTED_H_
#define UPB_REFCOUNTED_H_

#include "upb/table.int.h"

/* Reference tracking will check ref()/unref() operations to make sure the
 * ref ownership is correct.  Where possible it will also make tools like
 * Valgrind attribute ref leaks to the code that took the leaked ref, not
 * the code that originally created the object.
 *
 * Enabling this requires the application to define upb_lock()/upb_unlock()
 * functions that acquire/release a global mutex (or #define UPB_THREAD_UNSAFE).
 * For this reason we don't enable it by default, even in debug builds.
 */

/* #define UPB_DEBUG_REFS */

#ifdef __cplusplus
namespace upb {
class RefCounted;
template <class T> class reffed_ptr;
}
#endif

UPB_DECLARE_TYPE(upb::RefCounted, upb_refcounted)

struct upb_refcounted_vtbl;

#ifdef __cplusplus

class upb::RefCounted {
 public:
  /* Returns true if the given object is frozen. */
  bool IsFrozen() const;

  /* Increases the ref count, the new ref is owned by "owner" which must not
   * already own a ref (and should not itself be a refcounted object if the ref
   * could possibly be circular; see below).
   * Thread-safe iff "this" is frozen. */
  void Ref(const void *owner) const;

  /* Release a ref that was acquired from upb_refcounted_ref() and collects any
   * objects it can. */
  void Unref(const void *owner) const;

  /* Moves an existing ref from "from" to "to", without changing the overall
   * ref count.  DonateRef(foo, NULL, owner) is the same as Ref(foo, owner),
   * but "to" may not be NULL. */
  void DonateRef(const void *from, const void *to) const;

  /* Verifies that a ref to the given object is currently held by the given
   * owner.  Only effective in UPB_DEBUG_REFS builds. */
  void CheckRef(const void *owner) const;

 private:
  UPB_DISALLOW_POD_OPS(RefCounted, upb::RefCounted)
#else
struct upb_refcounted {
#endif
  /* TODO(haberman): move the actual structure definition to structdefs.int.h.
   * The only reason they are here is because inline functions need to see the
   * definition of upb_handlers, which needs to see this definition.  But we
   * can change the upb_handlers inline functions to deal in raw offsets
   * instead.
   */

  /* A single reference count shared by all objects in the group. */
  uint32_t *group;

  /* A singly-linked list of all objects in the group. */
  upb_refcounted *next;

  /* Table of function pointers for this type. */
  const struct upb_refcounted_vtbl *vtbl;

  /* Maintained only when mutable, this tracks the number of refs (but not
   * ref2's) to this object.  *group should be the sum of all individual_count
   * in the group. */
  uint32_t individual_count;

  bool is_frozen;

#ifdef UPB_DEBUG_REFS
  upb_inttable *refs;  /* Maps owner -> trackedref for incoming refs. */
  upb_inttable *ref2s; /* Set of targets for outgoing ref2s. */
#endif
};

#ifdef UPB_DEBUG_REFS
extern upb_alloc upb_alloc_debugrefs;
#define UPB_REFCOUNT_INIT(vtbl, refs, ref2s) \
    {&static_refcount, NULL, vtbl, 0, true, refs, ref2s}
#else
#define UPB_REFCOUNT_INIT(vtbl, refs, ref2s) \
    {&static_refcount, NULL, vtbl, 0, true}
#endif

UPB_BEGIN_EXTERN_C

/* It is better to use tracked refs when possible, for the extra debugging
 * capability.  But if this is not possible (because you don't have easy access
 * to a stable pointer value that is associated with the ref), you can pass
 * UPB_UNTRACKED_REF instead.  */
extern const void *UPB_UNTRACKED_REF;

/* Native C API. */
bool upb_refcounted_isfrozen(const upb_refcounted *r);
void upb_refcounted_ref(const upb_refcounted *r, const void *owner);
void upb_refcounted_unref(const upb_refcounted *r, const void *owner);
void upb_refcounted_donateref(
    const upb_refcounted *r, const void *from, const void *to);
void upb_refcounted_checkref(const upb_refcounted *r, const void *owner);

#define UPB_REFCOUNTED_CMETHODS(type, upcastfunc) \
  UPB_INLINE bool type ## _isfrozen(const type *v) { \
    return upb_refcounted_isfrozen(upcastfunc(v)); \
  } \
  UPB_INLINE void type ## _ref(const type *v, const void *owner) { \
    upb_refcounted_ref(upcastfunc(v), owner); \
  } \
  UPB_INLINE void type ## _unref(const type *v, const void *owner) { \
    upb_refcounted_unref(upcastfunc(v), owner); \
  } \
  UPB_INLINE void type ## _donateref(const type *v, const void *from, const void *to) { \
    upb_refcounted_donateref(upcastfunc(v), from, to); \
  } \
  UPB_INLINE void type ## _checkref(const type *v, const void *owner) { \
    upb_refcounted_checkref(upcastfunc(v), owner); \
  }

#define UPB_REFCOUNTED_CPPMETHODS \
  bool IsFrozen() const { \
    return upb::upcast_to<const upb::RefCounted>(this)->IsFrozen(); \
  } \
  void Ref(const void *owner) const { \
    return upb::upcast_to<const upb::RefCounted>(this)->Ref(owner); \
  } \
  void Unref(const void *owner) const { \
    return upb::upcast_to<const upb::RefCounted>(this)->Unref(owner); \
  } \
  void DonateRef(const void *from, const void *to) const { \
    return upb::upcast_to<const upb::RefCounted>(this)->DonateRef(from, to); \
  } \
  void CheckRef(const void *owner) const { \
    return upb::upcast_to<const upb::RefCounted>(this)->CheckRef(owner); \
  }

/* Internal-to-upb Interface **************************************************/

typedef void upb_refcounted_visit(const upb_refcounted *r,
                                  const upb_refcounted *subobj,
                                  void *closure);

struct upb_refcounted_vtbl {
  /* Must visit all subobjects that are currently ref'd via upb_refcounted_ref2.
   * Must be longjmp()-safe. */
  void (*visit)(const upb_refcounted *r, upb_refcounted_visit *visit, void *c);

  /* Must free the object and release all references to other objects. */
  void (*free)(upb_refcounted *r);
};

/* Initializes the refcounted with a single ref for the given owner.  Returns
 * false if memory could not be allocated. */
bool upb_refcounted_init(upb_refcounted *r,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner);

/* Adds a ref from one refcounted object to another ("from" must not already
 * own a ref).  These refs may be circular; cycles will be collected correctly
 * (if conservatively).  These refs do not need to be freed in from's free()
 * function. */
void upb_refcounted_ref2(const upb_refcounted *r, upb_refcounted *from);

/* Removes a ref that was acquired from upb_refcounted_ref2(), and collects any
 * object it can.  This is only necessary when "from" no longer points to "r",
 * and not from from's "free" function. */
void upb_refcounted_unref2(const upb_refcounted *r, upb_refcounted *from);

#define upb_ref2(r, from) \
    upb_refcounted_ref2((const upb_refcounted*)r, (upb_refcounted*)from)
#define upb_unref2(r, from) \
    upb_refcounted_unref2((const upb_refcounted*)r, (upb_refcounted*)from)

/* Freezes all mutable object reachable by ref2() refs from the given roots.
 * This will split refcounting groups into precise SCC groups, so that
 * refcounting of frozen objects can be more aggressive.  If memory allocation
 * fails, or if more than 2**31 mutable objects are reachable from "roots", or
 * if the maximum depth of the graph exceeds "maxdepth", false is returned and
 * the objects are unchanged.
 *
 * After this operation succeeds, the objects are frozen/const, and may not be
 * used through non-const pointers.  In particular, they may not be passed as
 * the second parameter of upb_refcounted_{ref,unref}2().  On the upside, all
 * operations on frozen refcounteds are threadsafe, and objects will be freed
 * at the precise moment that they become unreachable.
 *
 * Caller must own refs on each object in the "roots" list. */
bool upb_refcounted_freeze(upb_refcounted *const*roots, int n, upb_status *s,
                           int maxdepth);

/* Shared by all compiled-in refcounted objects. */
extern uint32_t static_refcount;

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ Wrappers. */
namespace upb {
inline bool RefCounted::IsFrozen() const {
  return upb_refcounted_isfrozen(this);
}
inline void RefCounted::Ref(const void *owner) const {
  upb_refcounted_ref(this, owner);
}
inline void RefCounted::Unref(const void *owner) const {
  upb_refcounted_unref(this, owner);
}
inline void RefCounted::DonateRef(const void *from, const void *to) const {
  upb_refcounted_donateref(this, from, to);
}
inline void RefCounted::CheckRef(const void *owner) const {
  upb_refcounted_checkref(this, owner);
}
}  /* namespace upb */
#endif


/* upb::reffed_ptr ************************************************************/

#ifdef __cplusplus

#include <algorithm>  /* For std::swap(). */

/* Provides RAII semantics for upb refcounted objects.  Each reffed_ptr owns a
 * ref on whatever object it points to (if any). */
template <class T> class upb::reffed_ptr {
 public:
  reffed_ptr() : ptr_(NULL) {}

  /* If ref_donor is NULL, takes a new ref, otherwise adopts from ref_donor. */
  template <class U>
  reffed_ptr(U* val, const void* ref_donor = NULL)
      : ptr_(upb::upcast(val)) {
    if (ref_donor) {
      UPB_ASSERT(ptr_);
      ptr_->DonateRef(ref_donor, this);
    } else if (ptr_) {
      ptr_->Ref(this);
    }
  }

  template <class U>
  reffed_ptr(const reffed_ptr<U>& other)
      : ptr_(upb::upcast(other.get())) {
    if (ptr_) ptr_->Ref(this);
  }

  reffed_ptr(const reffed_ptr& other)
      : ptr_(upb::upcast(other.get())) {
    if (ptr_) ptr_->Ref(this);
  }

  ~reffed_ptr() { if (ptr_) ptr_->Unref(this); }

  template <class U>
  reffed_ptr& operator=(const reffed_ptr<U>& other) {
    reset(other.get());
    return *this;
  }

  reffed_ptr& operator=(const reffed_ptr& other) {
    reset(other.get());
    return *this;
  }

  /* TODO(haberman): add C++11 move construction/assignment for greater
   * efficiency. */

  void swap(reffed_ptr& other) {
    if (ptr_ == other.ptr_) {
      return;
    }

    if (ptr_) ptr_->DonateRef(this, &other);
    if (other.ptr_) other.ptr_->DonateRef(&other, this);
    std::swap(ptr_, other.ptr_);
  }

  T& operator*() const {
    UPB_ASSERT(ptr_);
    return *ptr_;
  }

  T* operator->() const {
    UPB_ASSERT(ptr_);
    return ptr_;
  }

  T* get() const { return ptr_; }

  /* If ref_donor is NULL, takes a new ref, otherwise adopts from ref_donor. */
  template <class U>
  void reset(U* ptr = NULL, const void* ref_donor = NULL) {
    reffed_ptr(ptr, ref_donor).swap(*this);
  }

  template <class U>
  reffed_ptr<U> down_cast() {
    return reffed_ptr<U>(upb::down_cast<U*>(get()));
  }

  template <class U>
  reffed_ptr<U> dyn_cast() {
    return reffed_ptr<U>(upb::dyn_cast<U*>(get()));
  }

  /* Plain release() is unsafe; if we were the only owner, it would leak the
   * object.  Instead we provide this: */
  T* ReleaseTo(const void* new_owner) {
    T* ret = NULL;
    ptr_->DonateRef(this, new_owner);
    std::swap(ret, ptr_);
    return ret;
  }

 private:
  T* ptr_;
};

#endif  /* __cplusplus */

#endif  /* UPB_REFCOUNT_H_ */
