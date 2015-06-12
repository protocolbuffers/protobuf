/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/structdefs.int.h"
#include "upb/symtab.h"

#include <stdlib.h>
#include <string.h>

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


upb_symtab *upb_symtab_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {NULL, &upb_symtab_free};
  upb_symtab *s = malloc(sizeof(*s));
  upb_refcounted_init(upb_symtab_upcast_mutable(s), &vtbl, owner);
  upb_strtable_init(&s->symtab, UPB_CTYPE_PTR);
  return s;
}

void upb_symtab_freeze(upb_symtab *s) {
  upb_refcounted *r;
  bool ok;

  assert(!upb_symtab_isfrozen(s));
  r = upb_symtab_upcast_mutable(s);
  /* The symtab does not take ref2's (see refcounted.h) on the defs, because
   * defs cannot refer back to the table and therefore cannot create cycles.  So
   * 0 will suffice for maxdepth here. */
  ok = upb_refcounted_freeze(&r, 1, NULL, 0);
  UPB_ASSERT_VAR(ok, ok);
}

const upb_def *upb_symtab_lookup(const upb_symtab *s, const char *sym) {
  upb_value v;
  upb_def *ret = upb_strtable_lookup(&s->symtab, sym, &v) ?
      upb_value_getptr(v) : NULL;
  return ret;
}

const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym) {
  upb_value v;
  upb_def *def = upb_strtable_lookup(&s->symtab, sym, &v) ?
      upb_value_getptr(v) : NULL;
  return def ? upb_dyncast_msgdef(def) : NULL;
}

const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym) {
  upb_value v;
  upb_def *def = upb_strtable_lookup(&s->symtab, sym, &v) ?
      upb_value_getptr(v) : NULL;
  return def ? upb_dyncast_enumdef(def) : NULL;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static upb_def *upb_resolvename(const upb_strtable *t,
                                const char *base, const char *sym) {
  if(strlen(sym) == 0) return NULL;
  if(sym[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    upb_value v;
    return upb_strtable_lookup(t, sym + 1, &v) ? upb_value_getptr(v) : NULL;
  } else {
    /* Remove components from base until we find an entry or run out.
     * TODO: This branch is totally broken, but currently not used. */
    (void)base;
    assert(false);
    return NULL;
  }
}

const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym) {
  upb_def *ret = upb_resolvename(&s->symtab, base, sym);
  return ret;
}

/* Starts a depth-first traversal at def, recursing into any subdefs
 * (ie. submessage types).  Adds duplicates of existing defs to addtab
 * wherever necessary, so that the resulting symtab will be consistent once
 * addtab is added.
 *
 * More specifically, if any defs D is found in the DFS that:
 *
 *   1. can reach a def that is being replaced (because it has the same full
 *      name as a def in addtab, AND
 *
 *   2. is not itself being replaced already (ie. no def with this name exists
 *      in addtab).
 *
 * ...then a duplicate (new copy) of D will be added to addtab.
 *
 * Returns true if "def" can reach any def that is being replaced.
 *
 * It is slightly tricky to do this correctly in the place of cycles.  If we
 * detect that our DFS has hit a cycle, we don't yet know if this SCC can reach
 * a def in addtab or not.  Once we figure this out, that answer needs to apply
 * to *all* defs in the SCC, even if we visited them already.
 *
 * To work around this problem, we traverse each SCC (which we already
 * computed, since these defs are frozen) as a single node.  We first compute
 * whether the SCC as a whole can reach a def in addtab, then we dup (or not)
 * the entire SCC.  This requires breaking the encapsulation of upb_refcounted,
 * since that is where we get the data about what SCC we are in. */
static bool upb_resolve_dfs(const upb_def *def, upb_strtable *addtab,
                            const void *new_owner, upb_inttable *seen,
                            upb_status *s) {
  /* Memoize results of this function for efficiency (since we're traversing a
   * DAG this is not needed to limit the depth of the search). */
  upb_value v;
  bool need_dup;
  const upb_def *base;

  if (upb_inttable_lookup(seen, (uintptr_t)def, &v))
    return upb_value_getbool(v);

  /* Visit submessages for all messages in the SCC. */
  need_dup = false;
  base = def;
  do {
    upb_value v;
    const upb_msgdef *m;

    assert(upb_def_isfrozen(def));
    if (def->type == UPB_DEF_FIELD) continue;
    if (upb_strtable_lookup(addtab, upb_def_fullname(def), &v)) {
      need_dup = true;
    }

    /* For messages, continue the recursion by visiting all subdefs, but only
     * ones in different SCCs. */
    m = upb_dyncast_msgdef(def);
    if (m) {
      upb_msg_field_iter i;
      for(upb_msg_field_begin(&i, m);
          !upb_msg_field_done(&i);
          upb_msg_field_next(&i)) {
        upb_fielddef *f = upb_msg_iter_field(&i);
        const upb_def *subdef;

        if (!upb_fielddef_hassubdef(f)) continue;
        subdef = upb_fielddef_subdef(f);

        /* Skip subdefs in this SCC. */
        if (def->base.group == subdef->base.group) continue;

        /* |= to avoid short-circuit; we need its side-effects. */
        need_dup |= upb_resolve_dfs(subdef, addtab, new_owner, seen, s);
        if (!upb_ok(s)) return false;
      }
    }
  } while ((def = (upb_def*)def->base.next) != base);

  if (need_dup) {
    /* Dup all defs in this SCC that don't already have entries in addtab. */
    def = base;
    do {
      const char *name;

      if (def->type == UPB_DEF_FIELD) continue;
      name = upb_def_fullname(def);
      if (!upb_strtable_lookup(addtab, name, NULL)) {
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
  upb_status_seterrmsg(s, "out of memory");
  return false;
}

/* TODO(haberman): we need a lot more testing of error conditions.
 * The came_from_user stuff in particular is not tested. */
bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, int n, void *ref_donor,
                    upb_status *status) {
  int i;
  upb_strtable_iter iter;
  upb_def **add_defs = NULL;
  upb_strtable addtab;
  upb_inttable seen;

  assert(!upb_symtab_isfrozen(s));
  if (!upb_strtable_init(&addtab, UPB_CTYPE_PTR)) {
    upb_status_seterrmsg(status, "out of memory");
    return false;
  }

  /* Add new defs to our "add" set. */
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    const char *fullname;
    upb_fielddef *f;

    if (upb_def_isfrozen(def)) {
      upb_status_seterrmsg(status, "added defs must be mutable");
      goto err;
    }
    assert(!upb_def_isfrozen(def));
    fullname = upb_def_fullname(def);
    if (!fullname) {
      upb_status_seterrmsg(
          status, "Anonymous defs cannot be added to a symtab");
      goto err;
    }

    f = upb_dyncast_fielddef_mutable(def);

    if (f) {
      if (!upb_fielddef_containingtypename(f)) {
        upb_status_seterrmsg(status,
                             "Standalone fielddefs must have a containing type "
                             "(extendee) name set");
        goto err;
      }
    } else {
      if (upb_strtable_lookup(&addtab, fullname, NULL)) {
        upb_status_seterrf(status, "Conflicting defs named '%s'", fullname);
        goto err;
      }
      /* We need this to back out properly, because if there is a failure we
       * need to donate the ref back to the caller. */
      def->came_from_user = true;
      upb_def_donateref(def, ref_donor, s);
      if (!upb_strtable_insert(&addtab, fullname, upb_value_ptr(def)))
        goto oom_err;
    }
  }

  /* Add standalone fielddefs (ie. extensions) to the appropriate messages.
   * If the appropriate message only exists in the existing symtab, duplicate
   * it so we have a mutable copy we can add the fields to. */
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    upb_fielddef *f = upb_dyncast_fielddef_mutable(def);
    const char *msgname;
    upb_value v;
    upb_msgdef *m;

    if (!f) continue;
    msgname = upb_fielddef_containingtypename(f);
    /* We validated this earlier in this function. */
    assert(msgname);

    /* If the extendee name is absolutely qualified, move past the initial ".".
     * TODO(haberman): it is not obvious what it would mean if this was not
     * absolutely qualified. */
    if (msgname[0] == '.') {
      msgname++;
    }

    if (upb_strtable_lookup(&addtab, msgname, &v)) {
      /* Extendee is in the set of defs the user asked us to add. */
      m = upb_value_getptr(v);
    } else {
      /* Need to find and dup the extendee from the existing symtab. */
      const upb_msgdef *frozen_m = upb_symtab_lookupmsg(s, msgname);
      if (!frozen_m) {
        upb_status_seterrf(status,
                           "Tried to extend message %s that does not exist "
                           "in this SymbolTable.",
                           msgname);
        goto err;
      }
      m = upb_msgdef_dup(frozen_m, s);
      if (!m) goto oom_err;
      if (!upb_strtable_insert(&addtab, msgname, upb_value_ptr(m))) {
        upb_msgdef_unref(m, s);
        goto oom_err;
      }
    }

    if (!upb_msgdef_addfield(m, f, ref_donor, status)) {
      goto err;
    }
  }

  /* Add dups of any existing def that can reach a def with the same name as
   * anything in our "add" set. */
  if (!upb_inttable_init(&seen, UPB_CTYPE_BOOL)) goto oom_err;
  upb_strtable_begin(&iter, &s->symtab);
  for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    upb_def *def = upb_value_getptr(upb_strtable_iter_value(&iter));
    upb_resolve_dfs(def, &addtab, s, &seen, status);
    if (!upb_ok(status)) goto err;
  }
  upb_inttable_uninit(&seen);

  /* Now using the table, resolve symbolic references for subdefs. */
  upb_strtable_begin(&iter, &addtab);
  for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    const char *base;
    upb_def *def = upb_value_getptr(upb_strtable_iter_value(&iter));
    upb_msgdef *m = upb_dyncast_msgdef_mutable(def);
    upb_msg_field_iter j;

    if (!m) continue;
    /* Type names are resolved relative to the message in which they appear. */
    base = upb_msgdef_fullname(m);

    for(upb_msg_field_begin(&j, m);
        !upb_msg_field_done(&j);
        upb_msg_field_next(&j)) {
      upb_fielddef *f = upb_msg_iter_field(&j);
      const char *name = upb_fielddef_subdefname(f);
      if (name && !upb_fielddef_subdef(f)) {
        /* Try the lookup in the current set of to-be-added defs first. If not
         * there, try existing defs. */
        upb_def *subdef = upb_resolvename(&addtab, base, name);
        if (subdef == NULL) {
          subdef = upb_resolvename(&s->symtab, base, name);
        }
        if (subdef == NULL) {
          upb_status_seterrf(
              status, "couldn't resolve name '%s' in message '%s'", name, base);
          goto err;
        } else if (!upb_fielddef_setsubdef(f, subdef, status)) {
          goto err;
        }
      }
    }
  }

  /* We need an array of the defs in addtab, for passing to upb_def_freeze. */
  add_defs = malloc(sizeof(void*) * upb_strtable_count(&addtab));
  if (add_defs == NULL) goto oom_err;
  upb_strtable_begin(&iter, &addtab);
  for (n = 0; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    add_defs[n++] = upb_value_getptr(upb_strtable_iter_value(&iter));
  }

  if (!upb_def_freeze(add_defs, n, status)) goto err;

  /* This must be delayed until all errors have been detected, since error
   * recovery code uses this table to cleanup defs. */
  upb_strtable_uninit(&addtab);

  /* TODO(haberman) we don't properly handle errors after this point (like
   * OOM in upb_strtable_insert() below). */
  for (i = 0; i < n; i++) {
    upb_def *def = add_defs[i];
    const char *name = upb_def_fullname(def);
    upb_value v;
    bool success;

    if (upb_strtable_remove(&s->symtab, name, &v)) {
      const upb_def *def = upb_value_getptr(v);
      upb_def_unref(def, s);
    }
    success = upb_strtable_insert(&s->symtab, name, upb_value_ptr(def));
    UPB_ASSERT_VAR(success, success == true);
  }
  free(add_defs);
  return true;

oom_err:
  upb_status_seterrmsg(status, "out of memory");
err: {
    /* For defs the user passed in, we need to donate the refs back.  For defs
     * we dup'd, we need to just unref them. */
    upb_strtable_begin(&iter, &addtab);
    for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
      upb_def *def = upb_value_getptr(upb_strtable_iter_value(&iter));
      bool came_from_user = def->came_from_user;
      def->came_from_user = false;
      if (came_from_user) {
        upb_def_donateref(def, s, ref_donor);
      } else {
        upb_def_unref(def, s);
      }
    }
  }
  upb_strtable_uninit(&addtab);
  free(add_defs);
  assert(!upb_ok(status));
  return false;
}

/* Iteration. */

static void advance_to_matching(upb_symtab_iter *iter) {
  if (iter->type == UPB_DEF_ANY)
    return;

  while (!upb_strtable_done(&iter->iter) &&
         iter->type != upb_symtab_iter_def(iter)->type) {
    upb_strtable_next(&iter->iter);
  }
}

void upb_symtab_begin(upb_symtab_iter *iter, const upb_symtab *s,
                      upb_deftype_t type) {
  upb_strtable_begin(&iter->iter, &s->symtab);
  iter->type = type;
  advance_to_matching(iter);
}

void upb_symtab_next(upb_symtab_iter *iter) {
  upb_strtable_next(&iter->iter);
  advance_to_matching(iter);
}

bool upb_symtab_done(const upb_symtab_iter *iter) {
  return upb_strtable_done(&iter->iter);
}

const upb_def *upb_symtab_iter_def(const upb_symtab_iter *iter) {
  return upb_value_getptr(upb_strtable_iter_value(&iter->iter));
}
