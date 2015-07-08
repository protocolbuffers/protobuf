/*
** upb::SymbolTable (upb_symtab)
**
** A symtab (symbol table) stores a name->def map of upb_defs.  Clients could
** always create such tables themselves, but upb_symtab has logic for resolving
** symbolic references, and in particular, for keeping a whole set of consistent
** defs when replacing some subset of those defs.  This logic is nontrivial.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_SYMTAB_H_
#define UPB_SYMTAB_H_

#include "upb/def.h"

#ifdef __cplusplus
#include <vector>
namespace upb { class SymbolTable; }
#endif

UPB_DECLARE_DERIVED_TYPE(upb::SymbolTable, upb::RefCounted,
                         upb_symtab, upb_refcounted)

typedef struct {
 UPB_PRIVATE_FOR_CPP
  upb_strtable_iter iter;
  upb_deftype_t type;
} upb_symtab_iter;

#ifdef __cplusplus

/* Non-const methods in upb::SymbolTable are NOT thread-safe. */
class upb::SymbolTable {
 public:
  /* Returns a new symbol table with a single ref owned by "owner."
   * Returns NULL if memory allocation failed. */
  static reffed_ptr<SymbolTable> New();

  /* Include RefCounted base methods. */
  UPB_REFCOUNTED_CPPMETHODS

  /* For all lookup functions, the returned pointer is not owned by the
   * caller; it may be invalidated by any non-const call or unref of the
   * SymbolTable!  To protect against this, take a ref if desired. */

  /* Freezes the symbol table: prevents further modification of it.
   * After the Freeze() operation is successful, the SymbolTable must only be
   * accessed via a const pointer.
   *
   * Unlike with upb::MessageDef/upb::EnumDef/etc, freezing a SymbolTable is not
   * a necessary step in using a SymbolTable.  If you have no need for it to be
   * immutable, there is no need to freeze it ever.  However sometimes it is
   * useful, and SymbolTables that are statically compiled into the binary are
   * always frozen by nature. */
  void Freeze();

  /* Resolves the given symbol using the rules described in descriptor.proto,
   * namely:
   *
   *    If the name starts with a '.', it is fully-qualified.  Otherwise,
   *    C++-like scoping rules are used to find the type (i.e. first the nested
   *    types within this message are searched, then within the parent, on up
   *    to the root namespace).
   *
   * If not found, returns NULL. */
  const Def* Resolve(const char* base, const char* sym) const;

  /* Finds an entry in the symbol table with this exact name.  If not found,
   * returns NULL. */
  const Def* Lookup(const char *sym) const;
  const MessageDef* LookupMessage(const char *sym) const;
  const EnumDef* LookupEnum(const char *sym) const;

  /* TODO: introduce a C++ iterator, but make it nice and templated so that if
   * you ask for an iterator of MessageDef the iterated elements are strongly
   * typed as MessageDef*. */

  /* Adds the given mutable defs to the symtab, resolving all symbols
   * (including enum default values) and finalizing the defs.  Only one def per
   * name may be in the list, but defs can replace existing defs in the symtab.
   * All defs must have a name -- anonymous defs are not allowed.  Anonymous
   * defs can still be frozen by calling upb_def_freeze() directly.
   *
   * Any existing defs that can reach defs that are being replaced will
   * themselves be replaced also, so that the resulting set of defs is fully
   * consistent.
   *
   * This logic implemented in this method is a convenience; ultimately it
   * calls some combination of upb_fielddef_setsubdef(), upb_def_dup(), and
   * upb_freeze(), any of which the client could call themself.  However, since
   * the logic for doing so is nontrivial, we provide it here.
   *
   * The entire operation either succeeds or fails.  If the operation fails,
   * the symtab is unchanged, false is returned, and status indicates the
   * error.  The caller passes a ref on all defs to the symtab (even if the
   * operation fails).
   *
   * TODO(haberman): currently failure will leave the symtab unchanged, but may
   * leave the defs themselves partially resolved.  Does this matter?  If so we
   * could do a prepass that ensures that all symbols are resolvable and bail
   * if not, so we don't mutate anything until we know the operation will
   * succeed.
   *
   * TODO(haberman): since the defs must be mutable, refining a frozen def
   * requires making mutable copies of the entire tree.  This is wasteful if
   * only a few messages are changing.  We may want to add a way of adding a
   * tree of frozen defs to the symtab (perhaps an alternate constructor where
   * you pass the root of the tree?) */
  bool Add(Def*const* defs, int n, void* ref_donor, upb_status* status);

  bool Add(const std::vector<Def*>& defs, void *owner, Status* status) {
    return Add((Def*const*)&defs[0], defs.size(), owner, status);
  }

 private:
  UPB_DISALLOW_POD_OPS(SymbolTable, upb::SymbolTable)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */

/* Include refcounted methods like upb_symtab_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_symtab, upb_symtab_upcast)

upb_symtab *upb_symtab_new(const void *owner);
void upb_symtab_freeze(upb_symtab *s);
const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym);
const upb_def *upb_symtab_lookup(const upb_symtab *s, const char *sym);
const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym);
const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym);
bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, int n, void *ref_donor,
                    upb_status *status);

/* upb_symtab_iter i;
 * for(upb_symtab_begin(&i, s, type); !upb_symtab_done(&i);
 *     upb_symtab_next(&i)) {
 *   const upb_def *def = upb_symtab_iter_def(&i);
 *    // ...
 * }
 *
 * For C we don't have separate iterators for const and non-const.
 * It is the caller's responsibility to cast the upb_fielddef* to
 * const if the upb_msgdef* is const. */
void upb_symtab_begin(upb_symtab_iter *iter, const upb_symtab *s,
                      upb_deftype_t type);
void upb_symtab_next(upb_symtab_iter *iter);
bool upb_symtab_done(const upb_symtab_iter *iter);
const upb_def *upb_symtab_iter_def(const upb_symtab_iter *iter);

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ inline wrappers. */
namespace upb {
inline reffed_ptr<SymbolTable> SymbolTable::New() {
  upb_symtab *s = upb_symtab_new(&s);
  return reffed_ptr<SymbolTable>(s, &s);
}

inline void SymbolTable::Freeze() {
  return upb_symtab_freeze(this);
}
inline const Def *SymbolTable::Resolve(const char *base,
                                       const char *sym) const {
  return upb_symtab_resolve(this, base, sym);
}
inline const Def* SymbolTable::Lookup(const char *sym) const {
  return upb_symtab_lookup(this, sym);
}
inline const MessageDef *SymbolTable::LookupMessage(const char *sym) const {
  return upb_symtab_lookupmsg(this, sym);
}
inline bool SymbolTable::Add(
    Def*const* defs, int n, void* ref_donor, upb_status* status) {
  return upb_symtab_add(this, (upb_def*const*)defs, n, ref_donor, status);
}
}  /* namespace upb */
#endif

#endif  /* UPB_SYMTAB_H_ */
