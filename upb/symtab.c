/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/symtab.h"

#include <stdlib.h>
#include <string.h>

#include "upb/bytestream.h"

bool upb_symtab_isfrozen(const upb_symtab *s) {
  return upb_refcounted_isfrozen(upb_upcast(s));
}

void upb_symtab_ref(const upb_symtab *s, const void *owner) {
  upb_refcounted_ref(upb_upcast(s), owner);
}

void upb_symtab_unref(const upb_symtab *s, const void *owner) {
  upb_refcounted_unref(upb_upcast(s), owner);
}

void upb_symtab_donateref(
    const upb_symtab *s, const void *from, const void *to) {
  upb_refcounted_donateref(upb_upcast(s), from, to);
}

void upb_symtab_checkref(const upb_symtab *s, const void *owner) {
  upb_refcounted_checkref(upb_upcast(s), owner);
}

static void upb_symtab_free(upb_refcounted *r) {
  upb_symtab *s = (upb_symtab*)r;
  upb_strtable_iter i;
  upb_strtable_begin(&i, &s->symtab);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    const upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
    upb_def_unref(def, s);
  }
  upb_strtable_uninit(&s->symtab);
  free(s);
}

static const struct upb_refcounted_vtbl vtbl = {NULL, &upb_symtab_free};

upb_symtab *upb_symtab_new(const void *owner) {
  upb_symtab *s = malloc(sizeof(*s));
  upb_refcounted_init(upb_upcast(s), &vtbl, owner);
  upb_strtable_init(&s->symtab, UPB_CTYPE_PTR);
  return s;
}

const upb_def **upb_symtab_getdefs(const upb_symtab *s, upb_deftype_t type,
                                   const void *owner, int *n) {
  int total = upb_strtable_count(&s->symtab);
  // We may only use part of this, depending on how many symbols are of the
  // correct type.
  const upb_def **defs = malloc(sizeof(*defs) * total);
  upb_strtable_iter iter;
  upb_strtable_begin(&iter, &s->symtab);
  int i = 0;
  for(; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    upb_def *def = upb_value_getptr(upb_strtable_iter_value(&iter));
    assert(def);
    if(type == UPB_DEF_ANY || def->type == type)
      defs[i++] = def;
  }
  *n = i;
  if (owner)
    for(i = 0; i < *n; i++) upb_def_ref(defs[i], owner);
  return defs;
}

const upb_def *upb_symtab_lookup(const upb_symtab *s, const char *sym,
                                 const void *owner) {
  const upb_value *v = upb_strtable_lookup(&s->symtab, sym);
  upb_def *ret = v ? upb_value_getptr(*v) : NULL;
  if (ret) upb_def_ref(ret, owner);
  return ret;
}

const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym,
                                       const void *owner) {
  const upb_value *v = upb_strtable_lookup(&s->symtab, sym);
  upb_def *def = v ? upb_value_getptr(*v) : NULL;
  upb_msgdef *ret = NULL;
  if(def && def->type == UPB_DEF_MSG) {
    ret = upb_downcast_msgdef_mutable(def);
    upb_def_ref(def, owner);
  }
  return ret;
}

// Given a symbol and the base symbol inside which it is defined, find the
// symbol's definition in t.
static upb_def *upb_resolvename(const upb_strtable *t,
                                const char *base, const char *sym) {
  if(strlen(sym) == 0) return NULL;
  if(sym[0] == UPB_SYMBOL_SEPARATOR) {
    // Symbols starting with '.' are absolute, so we do a single lookup.
    // Slice to omit the leading '.'
    const upb_value *v = upb_strtable_lookup(t, sym + 1);
    return v ? upb_value_getptr(*v) : NULL;
  } else {
    // Remove components from base until we find an entry or run out.
    // TODO: This branch is totally broken, but currently not used.
    (void)base;
    assert(false);
    return NULL;
  }
}

const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym, const void *owner) {
  upb_def *ret = upb_resolvename(&s->symtab, base, sym);
  if (ret) upb_def_ref(ret, owner);
  return ret;
}

// Searches def and its children to find defs that have the same name as any
// def in "addtab."  Returns true if any where found, and as a side-effect adds
// duplicates of these defs into addtab.
//
// We use a modified depth-first traversal that traverses each SCC (which we
// already computed) as if it were a single node.  This allows us to traverse
// the possibly-cyclic graph as if it were a DAG and to dup the correct set of
// nodes with O(n) time.
static bool upb_resolve_dfs(const upb_def *def, upb_strtable *addtab,
                            const void *new_owner, upb_inttable *seen,
                            upb_status *s) {
  // Memoize results of this function for efficiency (since we're traversing a
  // DAG this is not needed to limit the depth of the search).
  const upb_value *v = upb_inttable_lookup(seen, (uintptr_t)def);
  if (v) return upb_value_getbool(*v);

  // Visit submessages for all messages in the SCC.
  bool need_dup = false;
  const upb_def *base = def;
  do {
    assert(upb_def_isfrozen(def));
    if (def->type == UPB_DEF_FIELD) continue;
    const upb_value *v = upb_strtable_lookup(addtab, upb_def_fullname(def));
    if (v) {
      // Because we memoize we should not visit a node after we have dup'd it.
      assert(((upb_def*)upb_value_getptr(*v))->came_from_user);
      need_dup = true;
    }
    const upb_msgdef *m = upb_dyncast_msgdef(def);
    if (m) {
      upb_msg_iter i;
      for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
        upb_fielddef *f = upb_msg_iter_field(&i);
        if (!upb_fielddef_hassubdef(f)) continue;
        // |= to avoid short-circuit; we need its side-effects.
        need_dup |= upb_resolve_dfs(
            upb_fielddef_subdef(f), addtab, new_owner, seen, s);
        if (!upb_ok(s)) return false;
      }
    }
  } while ((def = (upb_def*)def->base.next) != base);

  if (need_dup) {
    // Dup any defs that don't already have entries in addtab.
    def = base;
    do {
      if (def->type == UPB_DEF_FIELD) continue;
      const char *name = upb_def_fullname(def);
      if (upb_strtable_lookup(addtab, name) == NULL) {
        upb_def *newdef = upb_def_dup(def, new_owner);
        if (!newdef) goto oom;
        newdef->came_from_user = false;
        if (!upb_strtable_insert(addtab, name, upb_value_ptr(newdef)))
          goto oom;
      }
    } while ((def = (upb_def*)def->base.next) != base);
  }

  upb_inttable_insert(seen, (uintptr_t)def, upb_value_bool(need_dup));
  return need_dup;

oom:
  upb_status_seterrliteral(s, "out of memory");
  return false;
}

bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, int n, void *ref_donor,
                    upb_status *status) {
  upb_def **add_defs = NULL;
  upb_strtable addtab;
  if (!upb_strtable_init(&addtab, UPB_CTYPE_PTR)) {
    upb_status_seterrliteral(status, "out of memory");
    return false;
  }

  // Add new defs to table.
  for (int i = 0; i < n; i++) {
    upb_def *def = defs[i];
    if (upb_def_isfrozen(def)) {
      upb_status_seterrliteral(status, "added defs must be mutable");
      goto err;
    }
    assert(!upb_def_isfrozen(def));
    const char *fullname = upb_def_fullname(def);
    if (!fullname) {
      upb_status_seterrliteral(
          status, "Anonymous defs cannot be added to a symtab");
      goto err;
    }
    if (upb_strtable_lookup(&addtab, fullname) != NULL) {
      upb_status_seterrf(status, "Conflicting defs named '%s'", fullname);
      goto err;
    }
    // We need this to back out properly, because if there is a failure we need
    // to donate the ref back to the caller.
    def->came_from_user = true;
    upb_def_donateref(def, ref_donor, s);
    if (!upb_strtable_insert(&addtab, fullname, upb_value_ptr(def)))
      goto oom_err;
  }

  // Add dups of any existing def that can reach a def with the same name as
  // one of "defs."
  upb_inttable seen;
  if (!upb_inttable_init(&seen, UPB_CTYPE_BOOL)) goto oom_err;
  upb_strtable_iter i;
  upb_strtable_begin(&i, &s->symtab);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
    upb_resolve_dfs(def, &addtab, s, &seen, status);
    if (!upb_ok(status)) goto err;
  }
  upb_inttable_uninit(&seen);

  // Now using the table, resolve symbolic references.
  upb_strtable_begin(&i, &addtab);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
    upb_msgdef *m = upb_dyncast_msgdef_mutable(def);
    if (!m) continue;
    // Type names are resolved relative to the message in which they appear.
    const char *base = upb_def_fullname(upb_upcast(m));

    upb_msg_iter j;
    for(upb_msg_begin(&j, m); !upb_msg_done(&j); upb_msg_next(&j)) {
      upb_fielddef *f = upb_msg_iter_field(&j);
      const char *name = upb_fielddef_subdefname(f);
      if (name) {
        upb_def *subdef = upb_resolvename(&addtab, base, name);
        if (subdef == NULL) {
          upb_status_seterrf(
              status, "couldn't resolve name '%s' in message '%s'", name, base);
          goto err;
        } else if (!upb_fielddef_setsubdef(f, subdef)) {
          upb_status_seterrf(
              status, "def '%s' had the wrong type for field '%s'",
              upb_def_fullname(subdef), upb_fielddef_name(f));
          goto err;
        }
      }

      if (!upb_fielddef_resolvedefault(f)) {
        upb_byteregion *r = upb_value_getbyteregion(upb_fielddef_default(f));
        size_t len;
        const char *ptr = upb_byteregion_getptr(r, 0, &len);
        upb_status_seterrf(status, "couldn't resolve enum default '%s'", ptr);
        goto err;
      }
    }
  }

  // We need an array of the defs in addtab, for passing to upb_def_freeze.
  add_defs = malloc(sizeof(void*) * upb_strtable_count(&addtab));
  if (add_defs == NULL) goto oom_err;
  upb_strtable_begin(&i, &addtab);
  for (n = 0; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    add_defs[n++] = upb_value_getptr(upb_strtable_iter_value(&i));
  }

  if (!upb_def_freeze(add_defs, n, status)) goto err;

  // This must be delayed until all errors have been detected, since error
  // recovery code uses this table to cleanup defs.
  upb_strtable_uninit(&addtab);

  // TODO(haberman) we don't properly handle errors after this point (like
  // OOM in upb_strtable_insert() below).
  for (int i = 0; i < n; i++) {
    upb_def *def = add_defs[i];
    const char *name = upb_def_fullname(def);
    upb_value v;
    if (upb_strtable_remove(&s->symtab, name, &v)) {
      const upb_def *def = upb_value_getptr(v);
      upb_def_unref(def, s);
    }
    bool success = upb_strtable_insert(&s->symtab, name, upb_value_ptr(def));
    UPB_ASSERT_VAR(success, success == true);
  }
  free(add_defs);
  return true;

oom_err:
  upb_status_seterrliteral(status, "out of memory");
err: {
    // For defs the user passed in, we need to donate the refs back.  For defs
    // we dup'd, we need to just unref them.
    upb_strtable_iter i;
    upb_strtable_begin(&i, &addtab);
    for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
      upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
      if (def->came_from_user) {
        upb_def_donateref(def, s, ref_donor);
      } else {
        upb_def_unref(def, s);
      }
      def->came_from_user = false;
    }
  }
  upb_strtable_uninit(&addtab);
  free(add_defs);
  assert(!upb_ok(status));
  return false;
}
