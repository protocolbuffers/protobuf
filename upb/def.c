/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "upb/def.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)

void upb_deflist_init(upb_deflist *l) {
  l->size = 8;
  l->defs = malloc(l->size * sizeof(void*));
  l->len = 0;
}

void upb_deflist_uninit(upb_deflist *l) {
  for(uint32_t i = 0; i < l->len; i++) upb_def_unref(l->defs[i]);
  free(l->defs);
}

void upb_deflist_push(upb_deflist *l, upb_def *d) {
  if(l->len == l->size) {
    l->size *= 2;
    l->defs = realloc(l->defs, l->size * sizeof(void*));
  }
  l->defs[l->len++] = d;
}


/* upb_def ********************************************************************/

static void upb_msgdef_free(upb_msgdef *m);
static void upb_enumdef_free(upb_enumdef *e);
static void upb_unresolveddef_free(struct _upb_unresolveddef *u);

bool upb_def_ismutable(upb_def *def) { return def->symtab == NULL; }

bool upb_def_setfqname(upb_def *def, const char *fqname) {
  assert(upb_def_ismutable(def));
  free(def->fqname);
  def->fqname = strdup(fqname);
  return true;  // TODO: check for acceptable characters.
}

static void upb_def_free(upb_def *def) {
  switch (def->type) {
    case UPB_DEF_MSG: upb_msgdef_free(upb_downcast_msgdef(def)); break;
    case UPB_DEF_ENUM: upb_enumdef_free(upb_downcast_enumdef(def)); break;
    case UPB_DEF_UNRESOLVED:
        upb_unresolveddef_free(upb_downcast_unresolveddef(def)); break;
    default:
      assert(false);
  }
}

upb_def *upb_def_dup(upb_def *def) {
  switch (def->type) {
    case UPB_DEF_MSG: return UPB_UPCAST(upb_msgdef_dup(upb_downcast_msgdef(def)));
    case UPB_DEF_ENUM: return UPB_UPCAST(upb_enumdef_dup(upb_downcast_enumdef(def)));
    default: assert(false); return NULL;
  }
}

// Prior to being in a symtab, the def's refcount controls the lifetime of the
// def itself.  If the refcount falls to zero, the def is deleted.  Once the
// def belongs to a symtab, the def is owned by the symtab and its refcount
// determines whether the def owns a ref on the symtab or not.
void upb_def_ref(upb_def *def) {
  if (upb_atomic_ref(&def->refcount) && def->symtab)
    upb_symtab_ref(def->symtab);
}

static void upb_def_movetosymtab(upb_def *d, upb_symtab *s) {
  assert(upb_atomic_read(&d->refcount) > 0);
  d->symtab = s;
  upb_symtab_ref(s);
  upb_msgdef *m = upb_dyncast_msgdef(d);
  if (m) upb_inttable_compact(&m->itof);
}

void upb_def_unref(upb_def *def) {
  if (!def) return;
  if (upb_atomic_unref(&def->refcount)) {
    if (def->symtab) {
      upb_symtab_unref(def->symtab);
      // Def might be deleted now.
    } else {
      upb_def_free(def);
    }
  }
}

static void upb_def_init(upb_def *def, upb_deftype_t type) {
  def->type = type;
  def->fqname = NULL;
  def->symtab = NULL;
  upb_atomic_init(&def->refcount, 1);
}

static void upb_def_uninit(upb_def *def) {
  free(def->fqname);
}


/* upb_unresolveddef **********************************************************/

// Unresolved defs are used as temporary placeholders for a def whose name has
// not been resolved yet.  During the name resolution step, all unresolved defs
// are replaced with pointers to the actual def being referenced.
typedef struct _upb_unresolveddef {
  upb_def base;
} upb_unresolveddef;

// Is passed a ref on the string.
static upb_unresolveddef *upb_unresolveddef_new(const char *str) {
  upb_unresolveddef *def = malloc(sizeof(*def));
  upb_def_init(&def->base, UPB_DEF_UNRESOLVED);
  def->base.fqname = strdup(str);
  return def;
}

static void upb_unresolveddef_free(struct _upb_unresolveddef *def) {
  upb_def_uninit(&def->base);
  free(def);
}


/* upb_enumdef ****************************************************************/

upb_enumdef *upb_enumdef_new() {
  upb_enumdef *e = malloc(sizeof(*e));
  upb_def_init(&e->base, UPB_DEF_ENUM);
  upb_strtable_init(&e->ntoi, 0, sizeof(upb_ntoi_ent));
  upb_inttable_init(&e->iton, 0, sizeof(upb_iton_ent));
  return e;
}

static void upb_enumdef_free(upb_enumdef *e) {
  upb_enum_iter i;
  for(i = upb_enum_begin(e); !upb_enum_done(i); i = upb_enum_next(e, i)) {
    // Frees the ref taken when the string was parsed.
    free(upb_enum_iter_name(i));
  }
  upb_strtable_free(&e->ntoi);
  upb_inttable_free(&e->iton);
  upb_def_uninit(&e->base);
  free(e);
}

upb_enumdef *upb_enumdef_dup(upb_enumdef *e) {
  upb_enumdef *new_e = upb_enumdef_new();
  upb_enum_iter i;
  for(i = upb_enum_begin(e); !upb_enum_done(i); i = upb_enum_next(e, i)) {
    assert(upb_enumdef_addval(new_e, upb_enum_iter_name(i),
                                     upb_enum_iter_number(i)));
  }
  return new_e;
}

bool upb_enumdef_addval(upb_enumdef *e, char *name, int32_t num) {
  if (upb_enumdef_iton(e, num) || upb_enumdef_ntoi(e, name, NULL))
    return false;
  upb_iton_ent ent = {0, strdup(name)};
  upb_strtable_insert(&e->ntoi, name, &num);
  upb_inttable_insert(&e->iton, num, &ent);
  return true;
}

void upb_enumdef_setdefault(upb_enumdef *e, int32_t val) {
  assert(upb_def_ismutable(UPB_UPCAST(e)));
  e->defaultval = val;
}

upb_enum_iter upb_enum_begin(upb_enumdef *e) {
  // We could iterate over either table here; the choice is arbitrary.
  return upb_inttable_begin(&e->iton);
}

upb_enum_iter upb_enum_next(upb_enumdef *e, upb_enum_iter iter) {
  return upb_inttable_next(&e->iton, iter);
}

const char *upb_enumdef_iton(upb_enumdef *def, int32_t num) {
  upb_iton_ent *e = upb_inttable_fastlookup(&def->iton, num, sizeof(*e));
  return e ? e->str : NULL;
}

bool upb_enumdef_ntoil(upb_enumdef *def, char *name, size_t len, int32_t *num) {
  upb_ntoi_ent *e = upb_strtable_lookupl(&def->ntoi, name, len);
  if (!e) return false;
  if (num) *num = e->value;
  return true;
}

bool upb_enumdef_ntoi(upb_enumdef *e, char *name, int32_t *num) {
  return upb_enumdef_ntoil(e, name, strlen(name), num);
}


/* upb_fielddef ***************************************************************/

upb_fielddef *upb_fielddef_new() {
  upb_fielddef *f = malloc(sizeof(*f));
  f->msgdef = NULL;
  f->def = NULL;
  upb_atomic_init(&f->refcount, 1);
  f->finalized = false;
  f->type = 0;
  f->label = UPB_LABEL(OPTIONAL);
  f->hasbit = 0;
  f->offset = 0;
  f->number = 0;  // not a valid field number.
  f->hasdefault = false;
  f->name = NULL;
  f->accessor = NULL;
  upb_value_setfielddef(&f->fval, f);
  return f;
}

static void upb_fielddef_free(upb_fielddef *f) {
  if (upb_isstring(f)) {
    free(upb_value_getptr(f->defaultval));
  }
  if (f->def) {
    // We own a ref on the subdef iff we are not part of a msgdef.
    if (f->msgdef == NULL) {
      if (f->def) upb_downcast_unresolveddef(f->def);  // assert() check.
      upb_def_unref(f->def);
    }
  }
  free(f->name);
  free(f);
}

void upb_fielddef_ref(upb_fielddef *f) {
  // TODO.
  (void)f;
}

void upb_fielddef_unref(upb_fielddef *f) {
  // TODO.
  (void)f;
  if (!f) return;
  if (upb_atomic_unref(&f->refcount)) {
    if (f->msgdef) {
      upb_msgdef_unref(f->msgdef);
      // fielddef might be deleted now.
    } else {
      upb_fielddef_free(f);
    }
  }
}

upb_fielddef *upb_fielddef_dup(upb_fielddef *f) {
  upb_fielddef *newf = upb_fielddef_new();
  newf->msgdef = f->msgdef;
  newf->type = f->type;
  newf->label = f->label;
  newf->number = f->number;
  newf->name = f->name;
  upb_fielddef_settypename(newf, f->def->fqname);
  return f;
}

bool upb_fielddef_ismutable(upb_fielddef *f) {
  return !f->msgdef || upb_def_ismutable(UPB_UPCAST(f->msgdef));
}

upb_def *upb_fielddef_subdef(upb_fielddef *f) {
  if (upb_hassubdef(f) && !upb_fielddef_ismutable(f))
    return f->def;
  else
    return NULL;
}

static bool upb_fielddef_resolve(upb_fielddef *f, upb_def *def, upb_status *s) {
  assert(upb_dyncast_unresolveddef(f->def));
  upb_def_unref(f->def);
  f->def = def;
  if (f->type == UPB_TYPE(ENUM)) {
    // Resolve the enum's default from a string to an integer.
    char *str = upb_value_getptr(f->defaultval);
    assert(str);  // Should point to either a real default or the empty string.
    upb_enumdef *e = upb_downcast_enumdef(f->def);
    int32_t val = 0;
    if (str[0] == '\0') {
      upb_value_setint32(&f->defaultval, e->defaultval);
    } else {
      bool success = upb_enumdef_ntoi(e, str, &val);
      free(str);
      if (!success) {
        upb_status_setf(s, UPB_ERROR, "Default enum value (%s) is not a "
                                      "member of the enum", str);
        return false;
      }
      upb_value_setint32(&f->defaultval, val);
    }
  }
  return true;
}

bool upb_fielddef_setnumber(upb_fielddef *f, int32_t number) {
  assert(f->msgdef == NULL);
  f->number = number;
  return true;
}

bool upb_fielddef_setname(upb_fielddef *f, const char *name) {
  assert(f->msgdef == NULL);
  free(f->name);
  f->name = strdup(name);
  return true;
}

bool upb_fielddef_settype(upb_fielddef *f, uint8_t type) {
  assert(!f->finalized);
  f->type = type;
  return true;
}

bool upb_fielddef_setlabel(upb_fielddef *f, uint8_t label) {
  assert(!f->finalized);
  f->label = label;
  return true;
}

void upb_fielddef_setdefault(upb_fielddef *f, upb_value value) {
  assert(!f->finalized);
  // TODO: string ownership?
  f->defaultval = value;
}

void upb_fielddef_setfval(upb_fielddef *f, upb_value fval) {
  assert(!f->finalized);
  // TODO: string ownership?
  f->fval = fval;
}

void upb_fielddef_setaccessor(upb_fielddef *f, struct _upb_accessor_vtbl *vtbl) {
  assert(!f->finalized);
  f->accessor = vtbl;
}

bool upb_fielddef_settypename(upb_fielddef *f, const char *name) {
  upb_def_unref(f->def);
  f->def = UPB_UPCAST(upb_unresolveddef_new(name));
  return true;
}

// Returns an ordering of fields based on:
// 1. value size (small to large).
// 2. field number.
static int upb_fielddef_cmpval(const void *_f1, const void *_f2) {
  upb_fielddef *f1 = *(void**)_f1;
  upb_fielddef *f2 = *(void**)_f2;
  size_t size1 = upb_types[f1->type].size;
  size_t size2 = upb_types[f2->type].size;
  if (size1 != size2) return size1 - size2;
  // Otherwise return in number order.
  return f1->number - f2->number;
}

// Returns an ordering of all fields based on:
// 1. required/optional (required fields first).
// 2. field number
static int upb_fielddef_cmphasbit(const void *_f1, const void *_f2) {
  upb_fielddef *f1 = *(void**)_f1;
  upb_fielddef *f2 = *(void**)_f2;
  size_t req1 = f1->label == UPB_LABEL(REQUIRED);
  size_t req2 = f2->label == UPB_LABEL(REQUIRED);
  if (req1 != req2) return req1 - req2;
  // Otherwise return in number order.
  return f1->number - f2->number;
}


/* upb_msgdef *****************************************************************/

upb_msgdef *upb_msgdef_new() {
  upb_msgdef *m = malloc(sizeof(*m));
  upb_def_init(&m->base, UPB_DEF_MSG);
  upb_inttable_init(&m->itof, 4, sizeof(upb_itof_ent));
  upb_strtable_init(&m->ntof, 4, sizeof(upb_ntof_ent));
  m->size = 0;
  m->hasbit_bytes = 0;
  m->extstart = 0;
  m->extend = 0;
  return m;
}

static void upb_msgdef_free(upb_msgdef *m) {
  upb_msg_iter i;
  for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i))
    upb_fielddef_free(upb_msg_iter_field(i));
  upb_strtable_free(&m->ntof);
  upb_inttable_free(&m->itof);
  upb_def_uninit(&m->base);
  free(m);
}

upb_msgdef *upb_msgdef_dup(upb_msgdef *m) {
  upb_msgdef *newm = upb_msgdef_new();
  newm->size = m->size;
  newm->hasbit_bytes = m->hasbit_bytes;
  newm->extstart = m->extstart;
  newm->extend = m->extend;
  upb_msg_iter i;
  for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
    upb_msgdef_addfield(newm, upb_fielddef_dup(upb_msg_iter_field(i)));
  }
  return newm;
}

void upb_msgdef_setsize(upb_msgdef *m, uint16_t size) {
  assert(upb_def_ismutable(UPB_UPCAST(m)));
  m->size = size;
}

void upb_msgdef_sethasbit_bytes(upb_msgdef *m, uint16_t bytes) {
  assert(upb_def_ismutable(UPB_UPCAST(m)));
  m->hasbit_bytes = bytes;
}

bool upb_msgdef_setextrange(upb_msgdef *m, uint32_t start, uint32_t end) {
  assert(upb_def_ismutable(UPB_UPCAST(m)));
  if (start == 0 && end == 0) {
    // Clearing the extension range -- ok to fall through.
  } else if (start >= end || start < 1 || end > UPB_MAX_FIELDNUMBER) {
    return false;
  }
  m->extstart = start;
  m->extend = start;
  return true;
}

bool upb_msgdef_addfields(upb_msgdef *m, upb_fielddef **fields, int n) {
  // Check constraints for all fields before performing any action.
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = fields[i];
    assert(upb_atomic_read(&f->refcount) > 0);
    if (f->name == NULL || f->number == 0 ||
        upb_msgdef_itof(m, f->number) || upb_msgdef_ntof(m, f->name))
      return false;
  }

  // Constraint checks ok, perform the action.
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = fields[i];
    upb_msgdef_ref(m);
    assert(f->msgdef == NULL);
    f->msgdef = m;
    upb_itof_ent itof_ent = {0, f};
    upb_inttable_insert(&m->itof, f->number, &itof_ent);
    upb_strtable_insert(&m->ntof, f->name, &f);
  }
  return true;
}

static int upb_div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
}

void upb_msgdef_layout(upb_msgdef *m) {
  // Create an ordering over the fields, but only include fields with accessors.
  upb_fielddef **sorted_fields =
      malloc(sizeof(upb_fielddef*) * upb_msgdef_numfields(m));
  int n = 0;
  upb_msg_iter i;
  for (i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    if (f->accessor) sorted_fields[n++] = f;
  }

  m->hasbit_bytes = upb_div_round_up(n, 8);
  m->size = m->hasbit_bytes;  // + header_size?

  // Assign hasbits.
  qsort(sorted_fields, n, sizeof(*sorted_fields), upb_fielddef_cmphasbit);
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = sorted_fields[i];
    f->hasbit = i;
  }

  // Assign value offsets.
  qsort(sorted_fields, n, sizeof(*sorted_fields), upb_fielddef_cmpval);
  size_t max_align = 0;
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = sorted_fields[i];
    const upb_type_info *type_info = &upb_types[f->type];
    size_t size = type_info->size;
    size_t align = type_info->align;
    if (upb_isseq(f)) {
      size = sizeof(void*);
      align = alignof(void*);
    }

    // General alignment rules are: each member must be at an address that is a
    // multiple of that type's alignment.  Also, the size of the structure as a
    // whole must be a multiple of the greatest alignment of any member.
    f->offset = upb_align_up(m->size, align);
    m->size = f->offset + size;
    max_align = UPB_MAX(max_align, align);
  }
  if (max_align > 0) m->size = upb_align_up(m->size, max_align);

  free(sorted_fields);
}

upb_msg_iter upb_msg_begin(upb_msgdef *m) {
  return upb_inttable_begin(&m->itof);
}

upb_msg_iter upb_msg_next(upb_msgdef *m, upb_msg_iter iter) {
  return upb_inttable_next(&m->itof, iter);
}


/* upb_symtab *****************************************************************/

typedef struct {
  upb_def *def;
} upb_symtab_ent;

// Given a symbol and the base symbol inside which it is defined, find the
// symbol's definition in t.
static upb_symtab_ent *upb_resolve(upb_strtable *t,
                                   const char *base, const char *sym) {
  if(strlen(sym) == 0) return NULL;
  if(sym[0] == UPB_SYMBOL_SEPARATOR) {
    // Symbols starting with '.' are absolute, so we do a single lookup.
    // Slice to omit the leading '.'
    return upb_strtable_lookup(t, sym + 1);
  } else {
    // Remove components from base until we find an entry or run out.
    // TODO: This branch is totally broken, but currently not used.
    (void)base;
    assert(false);
    return NULL;
  }
}

static void _upb_symtab_free(upb_strtable *t) {
  upb_strtable_iter i;
  upb_strtable_begin(&i, t);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    const upb_symtab_ent *e = upb_strtable_iter_value(&i);
    assert(upb_atomic_read(&e->def->refcount) == 0);
    upb_def_free(e->def);
  }
  upb_strtable_free(t);
}

static void upb_symtab_free(upb_symtab *s) {
  _upb_symtab_free(&s->symtab);
  for (uint32_t i = 0; i < s->olddefs.len; i++) {
    upb_def *d = s->olddefs.defs[i];
    assert(upb_atomic_read(&d->refcount) == 0);
    upb_def_free(d);
  }
  upb_rwlock_destroy(&s->lock);
  upb_deflist_uninit(&s->olddefs);
  free(s);
}

void upb_symtab_unref(upb_symtab *s) {
  if(s && upb_atomic_unref(&s->refcount)) {
    upb_symtab_free(s);
  }
}

upb_symtab *upb_symtab_new() {
  upb_symtab *s = malloc(sizeof(*s));
  upb_atomic_init(&s->refcount, 1);
  upb_rwlock_init(&s->lock);
  upb_strtable_init(&s->symtab, 16, sizeof(upb_symtab_ent));
  upb_deflist_init(&s->olddefs);
  return s;
}

upb_def **upb_symtab_getdefs(upb_symtab *s, int *count, upb_deftype_t type) {
  upb_rwlock_rdlock(&s->lock);
  int total = upb_strtable_count(&s->symtab);
  // We may only use part of this, depending on how many symbols are of the
  // correct type.
  upb_def **defs = malloc(sizeof(*defs) * total);
  upb_strtable_iter iter;
  upb_strtable_begin(&iter, &s->symtab);
  int i = 0;
  for(; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    const upb_symtab_ent *e = upb_strtable_iter_value(&iter);
    upb_def *def = e->def;
    assert(def);
    if(type == UPB_DEF_ANY || def->type == type)
      defs[i++] = def;
  }
  upb_rwlock_unlock(&s->lock);
  *count = i;
  for(i = 0; i < *count; i++) upb_def_ref(defs[i]);
  return defs;
}

upb_def *upb_symtab_lookup(upb_symtab *s, const char *sym) {
  upb_rwlock_rdlock(&s->lock);
  upb_symtab_ent *e = upb_strtable_lookup(&s->symtab, sym);
  upb_def *ret = NULL;
  if(e) {
    ret = e->def;
    upb_def_ref(ret);
  }
  upb_rwlock_unlock(&s->lock);
  return ret;
}

upb_def *upb_symtab_resolve(upb_symtab *s, const char *base, const char *sym) {
  upb_rwlock_rdlock(&s->lock);
  upb_symtab_ent *e = upb_resolve(&s->symtab, base, sym);
  upb_def *ret = NULL;
  if(e) {
    ret = e->def;
    upb_def_ref(ret);
  }
  upb_rwlock_unlock(&s->lock);
  return ret;
}

bool upb_symtab_dfs(upb_def *def, upb_def **open_defs, int n,
                    upb_strtable *addtab) {
  // This linear search makes the DFS O(n^2) in the length of the paths.
  // Could make this O(n) with a hash table, but n is small.
  for (int i = 0; i < n; i++) {
    if (def == open_defs[i]) return false;
  }

  bool needcopy = false;
  upb_msgdef *m = upb_dyncast_msgdef(def);
  if (m) {
    upb_msg_iter i;
    open_defs[n++] = def;
    for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
      upb_fielddef *f = upb_msg_iter_field(i);
      if (!upb_hassubdef(f)) continue;
      needcopy |= upb_symtab_dfs(f->def, open_defs, n, addtab);
    }
  }

  bool replacing = (upb_strtable_lookup(addtab, m->base.fqname) != NULL);
  if (needcopy && !replacing) {
    upb_symtab_ent e = {upb_def_dup(def)};
    //fprintf(stderr, "Replacing def: %p\n", e.def);
    upb_strtable_insert(addtab, def->fqname, &e);
    replacing = true;
  }
  return replacing;
}

bool upb_symtab_add(upb_symtab *s, upb_def **defs, int n, upb_status *status) {
  upb_rwlock_wrlock(&s->lock);

  // Add all defs to a table for resolution.
  upb_strtable addtab;
  upb_strtable_init(&addtab, n, sizeof(upb_symtab_ent));
  for (int i = 0; i < n; i++) {
    upb_def *def = defs[i];
    if (upb_strtable_lookup(&addtab, def->fqname)) {
      upb_status_setf(status, UPB_ERROR,
                      "Conflicting defs named '%s'", def->fqname);
      upb_strtable_free(&addtab);
      return false;
    }
    upb_strtable_insert(&addtab, def->fqname, &def);
  }

  // All existing defs that can reach defs that are being replaced must
  // themselves be replaced with versions that will point to the new defs.
  // Do a DFS -- any path that finds a new def must replace all ancestors.
  upb_strtable *symtab = &s->symtab;
  upb_strtable_iter i;
  upb_strtable_begin(&i, symtab);
  for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_def *open_defs[UPB_MAX_TYPE_DEPTH];
    const upb_symtab_ent *e = upb_strtable_iter_value(&i);
    upb_symtab_dfs(e->def, open_defs, 0, &addtab);
  }

  // Resolve all refs.
  upb_strtable_begin(&i, &addtab);
  for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    const upb_symtab_ent *e = upb_strtable_iter_value(&i);
    upb_msgdef *m = upb_dyncast_msgdef(e->def);
    if(!m) continue;
    // Type names are resolved relative to the message in which they appear.
    const char *base = m->base.fqname;

    upb_msg_iter j;
    for(j = upb_msg_begin(m); !upb_msg_done(j); j = upb_msg_next(m, j)) {
      upb_fielddef *f = upb_msg_iter_field(j);
      if (f->type == 0) {
        upb_status_setf(status, UPB_ERROR, "Field type was not set.");
        return false;
      }

      // Set default default if none was set explicitly.
      if (!f->hasdefault) {
        switch (upb_fielddef_type(f)) {
          case UPB_TYPE(DOUBLE): upb_value_setdouble(&f->defaultval, 0); break;
          case UPB_TYPE(FLOAT): upb_value_setfloat(&f->defaultval, 0); break;
          case UPB_TYPE(UINT64):
          case UPB_TYPE(FIXED64): upb_value_setuint64(&f->defaultval, 0); break;
          case UPB_TYPE(INT64):
          case UPB_TYPE(SFIXED64):
          case UPB_TYPE(SINT64): upb_value_setint64(&f->defaultval, 0); break;
          case UPB_TYPE(INT32):
          case UPB_TYPE(SINT32):
          case UPB_TYPE(SFIXED32): upb_value_setint32(&f->defaultval, 0); break;
          case UPB_TYPE(UINT32):
          case UPB_TYPE(FIXED32): upb_value_setuint32(&f->defaultval, 0); break;
          case UPB_TYPE(BOOL): upb_value_setbool(&f->defaultval, false); break;
          case UPB_TYPE(ENUM):  // Will be resolved by upb_resolve().
          case UPB_TYPE(STRING):
          case UPB_TYPE(BYTES):
          case UPB_TYPE(GROUP):
          case UPB_TYPE(MESSAGE): break;  // do nothing for now.
        }
      }

      if (!upb_hassubdef(f)) continue;  // No resolving necessary.
      upb_downcast_unresolveddef(f->def);  // Type check.
      const char *name = f->def->fqname;

      // Resolve from either the addtab (pending adds) or symtab (existing
      // defs).  If both exist, prefer the pending add, because it will be
      // overwriting the existing def.
      upb_symtab_ent *found;
      if(!(found = upb_resolve(&addtab, base, name)) &&
         !(found = upb_resolve(symtab, base, name))) {
        upb_status_setf(status, UPB_ERROR, "could not resolve symbol '%s' "
                                           "in context '%s'", name, base);
        return false;
      }

      // Check the type of the found def.
      upb_fieldtype_t expected = upb_issubmsg(f) ? UPB_DEF_MSG : UPB_DEF_ENUM;
      //fprintf(stderr, "found: %p\n", found);
      //fprintf(stderr, "found->def: %p\n", found->def);
      //fprintf(stderr, "found->def->type: %d\n", found->def->type);
      if(found->def->type != expected) {
        upb_status_setf(status, UPB_ERROR, "Unexpected type");
        return false;
      }
      if (!upb_fielddef_resolve(f, found->def, status)) return false;
    }
  }

  // The defs in the transaction have been vetted, and can be moved to the
  // symtab without causing errors.
  upb_strtable_begin(&i, &addtab);
  for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    const upb_symtab_ent *tmptab_e = upb_strtable_iter_value(&i);
    upb_def_movetosymtab(tmptab_e->def, s);
    upb_symtab_ent *symtab_e =
        upb_strtable_lookup(&s->symtab, tmptab_e->def->fqname);
    if(symtab_e) {
      upb_deflist_push(&s->olddefs, symtab_e->def);
      symtab_e->def = tmptab_e->def;
    } else {
      //fprintf(stderr, "Inserting def: %p\n", tmptab_e->def);
      upb_strtable_insert(&s->symtab, tmptab_e->def->fqname, tmptab_e);
    }
  }

  upb_strtable_free(&addtab);
  upb_rwlock_unlock(&s->lock);
  upb_symtab_gc(s);
  return true;
}

void upb_symtab_gc(upb_symtab *s) {
  (void)s;
  // TODO.
}
