/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include <stddef.h>
#include "upb_def.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)

/* Search for a character in a string, in reverse. */
static int my_memrchr(char *data, char c, size_t len)
{
  int off = len-1;
  while(off > 0 && data[off] != c) --off;
  return off;
}

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

#ifndef NDEBUG
static bool upb_def_ismutable(upb_def *def) { return def->symtab == NULL; }
#endif

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
  if (!upb_atomic_unref(&d->refcount)) upb_symtab_ref(s);
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
  upb_string_unref(def->fqname);
}


/* upb_unresolveddef **********************************************************/

// Unresolved defs are used as temporary placeholders for a def whose name has
// not been resolved yet.  During the name resolution step, all unresolved defs
// are replaced with pointers to the actual def being referenced.
typedef struct _upb_unresolveddef {
  upb_def base;

  // The target type name.  This may or may not be fully qualified.  It is
  // tempting to want to use base.fqname for this, but that will be qualified
  // which is inappropriate for a name we still have to resolve.
  upb_string *name;
} upb_unresolveddef;

// Is passed a ref on the string.
static upb_unresolveddef *upb_unresolveddef_new(upb_string *str) {
  upb_unresolveddef *def = malloc(sizeof(*def));
  upb_def_init(&def->base, UPB_DEF_UNRESOLVED);
  def->name = upb_string_getref(str);
  return def;
}

static void upb_unresolveddef_free(struct _upb_unresolveddef *def) {
  upb_string_unref(def->name);
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
    upb_string_unref(upb_enum_iter_name(i));
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

bool upb_enumdef_addval(upb_enumdef *e, upb_string *name, int32_t num) {
  if (upb_enumdef_iton(e, num) || upb_enumdef_ntoi(e, name, NULL)) return false;
  upb_ntoi_ent ntoi_ent = {{name, 0}, num};
  upb_iton_ent iton_ent = {0, name};
  upb_strtable_insert(&e->ntoi, &ntoi_ent.e);
  upb_inttable_insert(&e->iton, num, &iton_ent); // Uses strtable's ref on name
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

upb_string *upb_enumdef_iton(upb_enumdef *def, int32_t num) {
  upb_iton_ent *e =
      (upb_iton_ent*)upb_inttable_fastlookup(&def->iton, num, sizeof(*e));
  return e ? e->string : NULL;
}

bool upb_enumdef_ntoi(upb_enumdef *def, upb_string *name, int32_t *num) {
  upb_ntoi_ent *e = (upb_ntoi_ent*)upb_strtable_lookup(&def->ntoi, name);
  if (!e) return false;
  if (num) *num = e->value;
  return true;
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
  f->name = NULL;
  f->accessor = NULL;
  upb_value_setfielddef(&f->fval, f);
  return f;
}

static void upb_fielddef_free(upb_fielddef *f) {
  if (upb_isstring(f)) {
    upb_string_unref(upb_value_getstr(f->defaultval));
  }
  upb_string_unref(f->name);
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

static bool upb_fielddef_resolve(upb_fielddef *f, upb_def *def, upb_status *s) {
  assert(upb_dyncast_unresolveddef(f->def));
  upb_def_unref(f->def);
  f->def = def;
  if (f->type == UPB_TYPE(ENUM)) {
    // Resolve the enum's default from a string to an integer.
    upb_string *str = upb_value_getstr(f->defaultval);
    assert(str);  // Should point to either a real default or the empty string.
    upb_enumdef *e = upb_downcast_enumdef(f->def);
    int32_t val = 0;
    if (str == upb_emptystring()) {
      upb_value_setint32(&f->defaultval, e->defaultval);
    } else {
      bool success = upb_enumdef_ntoi(e, str, &val);
      upb_string_unref(str);
      if (!success) {
        upb_seterr(s, UPB_ERROR, "Default enum value (" UPB_STRFMT ") is not a "
                   "member of the enum", UPB_STRARG(str));
        return false;
      }
      upb_value_setint32(&f->defaultval, val);
    }
  }
  return true;
}

void upb_fielddef_setnumber(upb_fielddef *f, int32_t number) {
  assert(f->msgdef == NULL);
  f->number = number;
}

void upb_fielddef_setname(upb_fielddef *f, upb_string *name) {
  assert(f->msgdef == NULL);
  f->name = upb_string_getref(name);
}

void upb_fielddef_settype(upb_fielddef *f, uint8_t type) {
  assert(!f->finalized);
  f->type = type;
}

void upb_fielddef_setlabel(upb_fielddef *f, uint8_t label) {
  assert(!f->finalized);
  f->label = label;
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

void upb_fielddef_settypename(upb_fielddef *f, upb_string *name) {
  upb_def_unref(f->def);
  f->def = UPB_UPCAST(upb_unresolveddef_new(name));
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
  m->extension_start = 0;
  m->extension_end = 0;
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
  newm->extension_start = m->extension_start;
  newm->extension_end = m->extension_end;
  upb_msg_iter i;
  for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i))
    upb_msgdef_addfield(newm, upb_fielddef_dup(upb_msg_iter_field(i)));
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

void upb_msgdef_setextension_start(upb_msgdef *m, uint32_t start) {
  assert(upb_def_ismutable(UPB_UPCAST(m)));
  m->extension_start = start;
}

void upb_msgdef_setextension_end(upb_msgdef *m, uint32_t end) {
  assert(upb_def_ismutable(UPB_UPCAST(m)));
  m->extension_end = end;
}

bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f) {
  assert(upb_atomic_read(&f->refcount) > 0);
  if (!upb_atomic_unref(&f->refcount)) upb_msgdef_ref(m);
  if (upb_msgdef_itof(m, f->number) || upb_msgdef_ntof(m, f->name)) {
    upb_fielddef_unref(f);
    return false;
  }
  assert(f->msgdef == NULL);
  f->msgdef = m;
  upb_itof_ent itof_ent = {0, f};
  upb_ntof_ent ntof_ent = {{f->name, 0}, f};
  upb_inttable_insert(&m->itof, f->number, &itof_ent);
  upb_strtable_insert(&m->ntof, &ntof_ent.e);
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


/* upb_symtabtxn **************************************************************/

typedef struct {
  upb_strtable_entry e;
  upb_def *def;
} upb_symtab_ent;

void upb_symtabtxn_init(upb_symtabtxn *t) {
  upb_strtable_init(&t->deftab, 16, sizeof(upb_symtab_ent));
}

void upb_symtabtxn_uninit(upb_symtabtxn *txn) {
  upb_strtable *t = &txn->deftab;
  upb_symtab_ent *e;
  for(e = upb_strtable_begin(t); e; e = upb_strtable_next(t, &e->e))
    upb_def_unref(e->def);
  upb_strtable_free(t);
}

bool upb_symtabtxn_add(upb_symtabtxn *t, upb_def *def) {
  // TODO: check if already present.
  upb_symtab_ent e = {{def->fqname, 0}, def};
  upb_strtable_insert(&t->deftab, &e.e);
  return true;
}

#if 0
err:
  // We need to free all defs from "tmptab."
  upb_rwlock_unlock(&s->lock);
  for(upb_symtab_ent *e = upb_strtable_begin(&tmptab); e;
      e = upb_strtable_next(&tmptab, &e->e)) {
    upb_def_unref(e->def);
  }
  upb_strtable_free(&tmptab);
  return false;
#endif

// Given a symbol and the base symbol inside which it is defined, find the
// symbol's definition in t.
static upb_symtab_ent *upb_resolve(upb_strtable *t,
                                   upb_string *base, upb_string *sym) {
  if(upb_string_len(sym) == 0) return NULL;
  if(upb_string_getrobuf(sym)[0] == UPB_SYMBOL_SEPARATOR) {
    // Symbols starting with '.' are absolute, so we do a single lookup.
    // Slice to omit the leading '.'
    upb_string *sym_str = upb_strslice(sym, 1, upb_string_len(sym) - 1);
    upb_symtab_ent *e = upb_strtable_lookup(t, sym_str);
    upb_string_unref(sym_str);
    return e;
  } else {
    // Remove components from base until we find an entry or run out.
    // TODO: This branch is totally broken, but currently not used.
    upb_string *sym_str = upb_string_new();
    int baselen = upb_string_len(base);
    upb_symtab_ent *ret = NULL;
    while(1) {
      // sym_str = base[0...base_len] + UPB_SYMBOL_SEPARATOR + sym
      upb_strlen_t len = baselen + upb_string_len(sym) + 1;
      char *buf = upb_string_getrwbuf(sym_str, len);
      memcpy(buf, upb_string_getrobuf(base), baselen);
      buf[baselen] = UPB_SYMBOL_SEPARATOR;
      memcpy(buf + baselen + 1, upb_string_getrobuf(sym), upb_string_len(sym));

      upb_symtab_ent *e = upb_strtable_lookup(t, sym_str);
      if (e) {
        ret = e;
        break;
      } else if(baselen == 0) {
        // No more scopes to try.
        ret = NULL;
        break;
      }
      baselen = my_memrchr(buf, UPB_SYMBOL_SEPARATOR, baselen);
    }
    upb_string_unref(sym_str);
    return ret;
  }
}

upb_symtabtxn_iter upb_symtabtxn_begin(upb_symtabtxn *t) {
  return upb_strtable_begin(&t->deftab);
}

upb_symtabtxn_iter upb_symtabtxn_next(upb_symtabtxn *t, upb_symtabtxn_iter i) {
  return upb_strtable_next(&t->deftab, i);
}

bool upb_symtabtxn_done(upb_symtabtxn_iter i) {
  return i == NULL;
}

upb_def *upb_symtabtxn_iter_def(upb_symtabtxn_iter iter) {
  upb_symtab_ent *e = iter;
  return e->def;
}


/* upb_symtab public interface ************************************************/

static void _upb_symtab_free(upb_strtable *t) {
  upb_symtab_ent *e;
  for (e = upb_strtable_begin(t); e; e = upb_strtable_next(t, &e->e)) {
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
  upb_symtab_ent *e = upb_strtable_begin(&s->symtab);
  int i = 0;
  for(; e; e = upb_strtable_next(&s->symtab, &e->e)) {
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

upb_def *upb_symtab_lookup(upb_symtab *s, upb_string *sym) {
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

upb_def *upb_symtab_resolve(upb_symtab *s, upb_string *base, upb_string *symbol) {
  upb_rwlock_rdlock(&s->lock);
  upb_symtab_ent *e = upb_resolve(&s->symtab, base, symbol);
  upb_def *ret = NULL;
  if(e) {
    ret = e->def;
    upb_def_ref(ret);
  }
  upb_rwlock_unlock(&s->lock);
  return ret;
}

bool upb_symtab_dfs(upb_def *def, upb_def **open_defs, int n,
                    upb_symtabtxn *txn) {
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
      if (!upb_hasdef(f)) continue;
      needcopy |= upb_symtab_dfs(f->def, open_defs, n, txn);
    }
  }

  bool replacing = (upb_strtable_lookup(&txn->deftab, m->base.fqname) != NULL);
  if (needcopy && !replacing) {
    upb_symtab_ent e = {{def->fqname, 0}, upb_def_dup(def)};
    upb_strtable_insert(&txn->deftab, &e.e);
    replacing = true;
  }
  return replacing;
}

bool upb_symtab_commit(upb_symtab *s, upb_symtabtxn *txn, upb_status *status) {
  upb_rwlock_wrlock(&s->lock);

  // All existing defs that can reach defs that are being replaced must
  // themselves be replaced with versions that will point to the new defs.
  // Do a DFS -- any path that finds a new def must replace all ancestors.
  upb_strtable *symtab = &s->symtab;
  upb_symtab_ent *e;
  for(e = upb_strtable_begin(symtab); e; e = upb_strtable_next(symtab, &e->e)) {
    upb_def *open_defs[UPB_MAX_TYPE_DEPTH];
    upb_symtab_dfs(e->def, open_defs, 0, txn);
  }

  // Resolve all refs.
  upb_strtable *txntab = &txn->deftab;
  for(e = upb_strtable_begin(txntab); e; e = upb_strtable_next(txntab, &e->e)) {
    upb_msgdef *m = upb_dyncast_msgdef(e->def);
    if(!m) continue;
    // Type names are resolved relative to the message in which they appear.
    upb_string *base = m->base.fqname;

    upb_msg_iter i;
    for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
      upb_fielddef *f = upb_msg_iter_field(i);
      if(!upb_hasdef(f)) continue;  // No resolving necessary.
      upb_string *name = upb_downcast_unresolveddef(f->def)->name;

      // Resolve from either the txntab (pending adds) or symtab (existing
      // defs).  If both exist, prefer the pending add, because it will be
      // overwriting the existing def.
      upb_symtab_ent *found;
      if(!(found = upb_resolve(txntab, base, name)) &&
         !(found = upb_resolve(symtab, base, name))) {
        upb_seterr(status, UPB_ERROR,
                   "could not resolve symbol '" UPB_STRFMT "'"
                   " in context '" UPB_STRFMT "'",
                   UPB_STRARG(name), UPB_STRARG(base));
        return false;
      }

      // Check the type of the found def.
      upb_fieldtype_t expected = upb_issubmsg(f) ? UPB_DEF_MSG : UPB_DEF_ENUM;
      if(found->def->type != expected) {
        upb_seterr(status, UPB_ERROR, "Unexpected type");
        return false;
      }
      if (!upb_fielddef_resolve(f, found->def, status)) return false;
    }
  }

  // The defs in the transaction have been vetted, and can be moved to the
  // symtab without causing errors.
  upb_symtab_ent *tmptab_e;
  for(tmptab_e = upb_strtable_begin(txntab); tmptab_e;
      tmptab_e = upb_strtable_next(txntab, &tmptab_e->e)) {
    upb_def_movetosymtab(tmptab_e->def, s);
    upb_symtab_ent *symtab_e =
        upb_strtable_lookup(&s->symtab, tmptab_e->def->fqname);
    if(symtab_e) {
      upb_deflist_push(&s->olddefs, symtab_e->def);
      symtab_e->def = tmptab_e->def;
    } else {
      upb_strtable_insert(&s->symtab, &tmptab_e->e);
    }
  }

  upb_strtable_clear(txntab);
  upb_rwlock_unlock(&s->lock);
  upb_symtab_gc(s);
  return true;
}

void upb_symtab_gc(upb_symtab *s) {
  (void)s;
  // TODO.
}
