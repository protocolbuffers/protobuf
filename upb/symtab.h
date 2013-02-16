/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A symtab (symbol table) stores a name->def map of upb_defs.  Clients could
 * always create such tables themselves, but upb_symtab has logic for resolving
 * symbolic references, and in particular, for keeping a whole set of consistent
 * defs when replacing some subset of those defs.  This logic is nontrivial.
 *
 * This is a mixed C/C++ interface that offers a full API to both languages.
 * See the top-level README for more information.
 */

#ifndef UPB_SYMTAB_H_
#define UPB_SYMTAB_H_

#ifdef __cplusplus
#include <vector>

namespace upb { class SymbolTable; }
typedef upb::SymbolTable upb_symtab;
#else
struct upb_symtab;
typedef struct upb_symtab upb_symtab;
#endif

#include "upb/def.h"

#ifdef __cplusplus

class upb::SymbolTable {
 public:
  // Returns a new symbol table with a single ref owned by "owner."
  // Returns NULL if memory allocation failed.
  static SymbolTable* New(const void* owner);

  // Though not declared as such in C++, upb::RefCounted is the base of
  // SymbolTable and we can upcast to it.
  RefCounted* Upcast();
  const RefCounted* Upcast() const;

  // Functionality from upb::RefCounted.
  bool IsFrozen() const;
  void Ref(const void* owner) const;
  void Unref(const void* owner) const;
  void DonateRef(const void *from, const void *to) const;
  void CheckRef(const void *owner) const;

  // Resolves the given symbol using the rules described in descriptor.proto,
  // namely:
  //
  //    If the name starts with a '.', it is fully-qualified.  Otherwise,
  //    C++-like scoping rules are used to find the type (i.e. first the nested
  //    types within this message are searched, then within the parent, on up
  //    to the root namespace).
  //
  // If a def is found, the caller owns one ref on the returned def, owned by
  // owner.  Otherwise returns NULL.
  const Def* Resolve(const char* base, const char* sym,
                     const void* owner) const;

  // Finds an entry in the symbol table with this exact name.  If a def is
  // found, the caller owns one ref on the returned def, owned by owner.
  // Otherwise returns NULL.
  const Def* Lookup(const char *sym, const void *owner) const;
  const MessageDef* LookupMessage(const char *sym, const void *owner) const;

  // Gets an array of pointers to all currently active defs in this symtab.
  // The caller owns the returned array (which is of length *n) as well as a
  // ref to each symbol inside (owned by owner).  If type is UPB_DEF_ANY then
  // defs of all types are returned, otherwise only defs of the required type
  // are returned.
  const Def** GetDefs(upb_deftype_t type, const void *owner, int *n) const;

  // Adds the given mutable defs to the symtab, resolving all symbols
  // (including enum default values) and finalizing the defs.  Only one def per
  // name may be in the list, but defs can replace existing defs in the symtab.
  // All defs must have a name -- anonymous defs are not allowed.  Anonymous
  // defs can still be frozen by calling upb_def_freeze() directly.
  //
  // Any existing defs that can reach defs that are being replaced will
  // themselves be replaced also, so that the resulting set of defs is fully
  // consistent.
  //
  // This logic implemented in this method is a convenience; ultimately it
  // calls some combination of upb_fielddef_setsubdef(), upb_def_dup(), and
  // upb_freeze(), any of which the client could call themself.  However, since
  // the logic for doing so is nontrivial, we provide it here.
  //
  // The entire operation either succeeds or fails.  If the operation fails,
  // the symtab is unchanged, false is returned, and status indicates the
  // error.  The caller passes a ref on all defs to the symtab (even if the
  // operation fails).
  //
  // TODO(haberman): currently failure will leave the symtab unchanged, but may
  // leave the defs themselves partially resolved.  Does this matter?  If so we
  // could do a prepass that ensures that all symbols are resolvable and bail
  // if not, so we don't mutate anything until we know the operation will
  // succeed.
  //
  // TODO(haberman): since the defs must be mutable, refining a frozen def
  // requires making mutable copies of the entire tree.  This is wasteful if
  // only a few messages are changing.  We may want to add a way of adding a
  // tree of frozen defs to the symtab (perhaps an alternate constructor where
  // you pass the root of the tree?)
  bool Add(Def*const* defs, int n, void* ref_donor, upb_status* status);

  bool Add(const std::vector<Def*>& defs, void *owner, Status* status) {
    return Add((Def*const*)&defs[0], defs.size(), owner, status);
  }

 private:
  UPB_DISALLOW_POD_OPS(SymbolTable);

#else
struct upb_symtab {
#endif
  upb_refcounted base;
  upb_strtable symtab;
};

// Native C API.
#ifdef __cplusplus
extern "C" {
#endif
// From upb_refcounted.
bool upb_symtab_isfrozen(const upb_symtab *s);
void upb_symtab_ref(const upb_symtab *s, const void *owner);
void upb_symtab_unref(const upb_symtab *s, const void *owner);
void upb_symtab_donateref(
    const upb_symtab *s, const void *from, const void *to);
void upb_symtab_checkref(const upb_symtab *s, const void *owner);

upb_symtab *upb_symtab_new(const void *owner);
const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym, const void *owner);
const upb_def *upb_symtab_lookup(
    const upb_symtab *s, const char *sym, const void *owner);
const upb_msgdef *upb_symtab_lookupmsg(
    const upb_symtab *s, const char *sym, const void *owner);
const upb_def **upb_symtab_getdefs(
    const upb_symtab *s, upb_deftype_t type, const void *owner, int *n);
bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, int n, void *ref_donor,
                    upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */

// C++ inline wrappers.
namespace upb {
inline SymbolTable* SymbolTable::New(const void* owner) {
  return upb_symtab_new(owner);
}

inline RefCounted* SymbolTable::Upcast() { return upb_upcast(this); }
inline const RefCounted* SymbolTable::Upcast() const {
  return upb_upcast(this);
}
inline bool SymbolTable::IsFrozen() const {
  return upb_symtab_isfrozen(this);
}
inline void SymbolTable::Ref(const void *owner) const {
  upb_symtab_ref(this, owner);
}
inline void SymbolTable::Unref(const void *owner) const {
  upb_symtab_unref(this, owner);
}
inline void SymbolTable::DonateRef(const void *from, const void *to) const {
  upb_symtab_donateref(this, from, to);
}
inline void SymbolTable::CheckRef(const void *owner) const {
  upb_symtab_checkref(this, owner);
}

inline const Def* SymbolTable::Resolve(
    const char* base, const char* sym, const void* owner) const {
  return upb_symtab_resolve(this, base, sym, owner);
}
inline const Def* SymbolTable::Lookup(
    const char *sym, const void *owner) const {
  return upb_symtab_lookup(this, sym, owner);
}
inline const MessageDef* SymbolTable::LookupMessage(
    const char *sym, const void *owner) const {
  return upb_symtab_lookupmsg(this, sym, owner);
}
inline const Def** SymbolTable::GetDefs(
    upb_deftype_t type, const void *owner, int *n) const {
  return upb_symtab_getdefs(this, type, owner, n);
}
inline bool SymbolTable::Add(
    Def*const* defs, int n, void* ref_donor, upb_status* status) {
  return upb_symtab_add(this, (upb_def*const*)defs, n, ref_donor, status);
}
}  // namespace upb
#endif

#endif  /* UPB_SYMTAB_H_ */
