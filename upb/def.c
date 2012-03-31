/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "upb/bytestream.h"
#include "upb/def.h"

// isalpha() etc. from <ctype.h> are locale-dependent, which we don't want.
static bool upb_isbetween(char c, char low, char high) {
  return c >= low && c <= high;
}

static bool upb_isletter(char c) {
  return upb_isbetween(c, 'A', 'Z') || upb_isbetween(c, 'a', 'z') || c == '_';
}

static bool upb_isalphanum(char c) {
  return upb_isletter(c) || upb_isbetween(c, '0', '9');
}

static bool upb_isident(const char *str, size_t len, bool full) {
  bool start = true;
  for (size_t i = 0; i < len; i++) {
    char c = str[i];
    if (c == '.') {
      if (start || !full) return false;
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) return false;
      start = false;
    } else {
      if (!upb_isalphanum(c)) return false;
    }
  }
  return !start;
}


/* upb_def ********************************************************************/

static void upb_msgdef_free(upb_msgdef *m);
static void upb_fielddef_free(upb_fielddef *f);
static void upb_enumdef_free(upb_enumdef *e);

bool upb_def_ismutable(const upb_def *def) { return !def->is_finalized; }
bool upb_def_isfinalized(const upb_def *def) { return def->is_finalized; }

bool upb_def_setfullname(upb_def *def, const char *fullname) {
  assert(upb_def_ismutable(def));
  if (!upb_isident(fullname, strlen(fullname), true)) return false;
  free(def->fullname);
  def->fullname = strdup(fullname);
  return true;
}

void upb_def_ref(const upb_def *_def, const void *owner) {
  upb_def *def = (upb_def*)_def;
  upb_refcount_ref(&def->refcount, owner);
}

void upb_def_unref(const upb_def *_def, const void *owner) {
  upb_def *def = (upb_def*)_def;
  if (!def) return;
  if (!upb_refcount_unref(&def->refcount, owner)) return;
  upb_def *base = def;
  // Free all defs in the SCC.
  do {
    upb_def *next = (upb_def*)def->refcount.next;
    switch (def->type) {
      case UPB_DEF_MSG: upb_msgdef_free(upb_downcast_msgdef(def)); break;
      case UPB_DEF_FIELD: upb_fielddef_free(upb_downcast_fielddef(def)); break;
      case UPB_DEF_ENUM: upb_enumdef_free(upb_downcast_enumdef(def)); break;
      default:
        assert(false);
    }
    def = next;
  } while(def != base);
}

void upb_def_donateref(const upb_def *_def, const void *from, const void *to) {
  upb_def *def = (upb_def*)_def;
  upb_refcount_donateref(&def->refcount, from, to);
}

upb_def *upb_def_dup(const upb_def *def, const void *o) {
  switch (def->type) {
    case UPB_DEF_MSG:
      return UPB_UPCAST(upb_msgdef_dup(upb_downcast_msgdef_const(def), o));
    case UPB_DEF_FIELD:
      return UPB_UPCAST(upb_fielddef_dup(upb_downcast_fielddef_const(def), o));
    case UPB_DEF_ENUM:
      return UPB_UPCAST(upb_enumdef_dup(upb_downcast_enumdef_const(def), o));
    default: assert(false); return NULL;
  }
}

static bool upb_def_init(upb_def *def, upb_deftype_t type, const void *owner) {
  def->type = type;
  def->is_finalized = false;
  def->fullname = NULL;
  return upb_refcount_init(&def->refcount, owner);
}

static void upb_def_uninit(upb_def *def) {
  upb_refcount_uninit(&def->refcount);
  free(def->fullname);
}

static void upb_def_getsuccessors(upb_refcount *refcount, void *closure) {
  upb_def *def = (upb_def*)refcount;
  switch (def->type) {
    case UPB_DEF_MSG: {
      upb_msgdef *m = upb_downcast_msgdef(def);
      upb_msg_iter i;
      for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
        upb_fielddef *f = upb_msg_iter_field(&i);
        upb_refcount_visit(refcount, &f->base.refcount, closure);
      }
      break;
    }
    case UPB_DEF_FIELD: {
      upb_fielddef *f = upb_downcast_fielddef(def);
      assert(f->msgdef);
      upb_refcount_visit(refcount, &f->msgdef->base.refcount, closure);
      upb_def *subdef = f->sub.def;
      if (subdef)
        upb_refcount_visit(refcount, &subdef->refcount, closure);
      break;
    }
    case UPB_DEF_ENUM:
    case UPB_DEF_SERVICE:
    case UPB_DEF_ANY:
      break;
  }
}

static bool upb_validate_field(const upb_fielddef *f, upb_status *s) {
  if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == -1) {
    upb_status_seterrliteral(s, "fielddef must have name and number set");
    return false;
  }
  if (upb_hassubdef(f)) {
    if (f->subdef_is_symbolic) {
      upb_status_seterrf(s,
          "field %s has not been resolved", upb_fielddef_name(f));
      return false;
    } else if (upb_fielddef_subdef(f) == NULL) {
      upb_status_seterrf(s,
          "field is %s missing required subdef", upb_fielddef_name(f));
      return false;
    } else if (!upb_def_isfinalized(upb_fielddef_subdef(f))) {
      upb_status_seterrf(s,
          "field %s subtype is not being finalized", upb_fielddef_name(f));
      return false;
    }
  }
  return true;
}

bool upb_finalize(upb_def *const*defs, int n, upb_status *s) {
  if (n >= UINT16_MAX - 1) {
    upb_status_seterrliteral(s, "too many defs (max is 64k at a time)");
    return false;
  }

  // First perform validation, in two passes so we can check that we have a
  // transitive closure without needing to search.
  for (int i = 0; i < n; i++) {
    upb_def *def = defs[i];
    if (upb_def_isfinalized(def)) {
      // Could relax this requirement if it's annoying.
      upb_status_seterrliteral(s, "def is already finalized");
      goto err;
    } else if (def->type == UPB_DEF_FIELD) {
      upb_status_seterrliteral(s, "standalone fielddefs can not be finalized");
      goto err;
    } else {
      // Set now to detect transitive closure in the second pass.
      def->is_finalized = true;
    }
  }

  for (int i = 0; i < n; i++) {
    upb_msgdef *m = upb_dyncast_msgdef(defs[i]);
    if (!m) continue;
    upb_inttable_compact(&m->itof);
    upb_msg_iter j;
    for(upb_msg_begin(&j, m); !upb_msg_done(&j); upb_msg_next(&j)) {
      upb_fielddef *f = upb_msg_iter_field(&j);
      assert(f->msgdef == m);
      if (!upb_validate_field(f, s)) goto err;
    }
  }

  // Validation all passed, now find strongly-connected components so that
  // our refcounting works with cycles.
  upb_refcount_findscc((upb_refcount**)defs, n, &upb_def_getsuccessors);

  // Now that ref cycles have been removed it is safe to have each fielddef
  // take a ref on its subdef (if any), but only if it's a member of another
  // SCC.
  for (int i = 0; i < n; i++) {
    upb_msgdef *m = upb_dyncast_msgdef(defs[i]);
    if (!m) continue;
    upb_msg_iter j;
    for(upb_msg_begin(&j, m); !upb_msg_done(&j); upb_msg_next(&j)) {
      upb_fielddef *f = upb_msg_iter_field(&j);
      f->base.is_finalized = true;
      // Release the ref taken in upb_msgdef_addfields().
      upb_fielddef_unref(f, m);
      if (!upb_hassubdef(f)) continue;
      assert(upb_fielddef_subdef(f));
      if (!upb_refcount_merged(&f->base.refcount, &f->sub.def->refcount)) {
        // Subdef is part of a different strongly-connected component.
        upb_def_ref(f->sub.def, &f->sub.def);
        f->subdef_is_owned = true;
      }
    }
  }

  return true;

err:
  for (int i = 0; i < n; i++) {
    defs[i]->is_finalized = false;
  }
  return false;
}


/* upb_enumdef ****************************************************************/

upb_enumdef *upb_enumdef_new(const void *owner) {
  upb_enumdef *e = malloc(sizeof(*e));
  if (!e) return NULL;
  if (!upb_def_init(&e->base, UPB_DEF_ENUM, owner)) goto err2;
  if (!upb_strtable_init(&e->ntoi)) goto err2;
  if (!upb_inttable_init(&e->iton)) goto err1;
  return e;

err1:
  upb_strtable_uninit(&e->ntoi);
err2:
  free(e);
  return NULL;
}

static void upb_enumdef_free(upb_enumdef *e) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &e->iton);
  for( ; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    // To clean up the strdup() from upb_enumdef_addval().
    free(upb_value_getptr(upb_inttable_iter_value(&i)));
  }
  upb_strtable_uninit(&e->ntoi);
  upb_inttable_uninit(&e->iton);
  upb_def_uninit(&e->base);
  free(e);
}

upb_enumdef *upb_enumdef_dup(const upb_enumdef *e, const void *owner) {
  upb_enumdef *new_e = upb_enumdef_new(owner);
  if (!new_e) return NULL;
  upb_enum_iter i;
  for(upb_enum_begin(&i, e); !upb_enum_done(&i); upb_enum_next(&i)) {
    bool success = upb_enumdef_addval(
        new_e, upb_enum_iter_name(&i),upb_enum_iter_number(&i));
    if (!success) {
      upb_enumdef_unref(new_e, owner);
      return NULL;
    }
  }
  return new_e;
}

bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num) {
  if (!upb_isident(name, strlen(name), false)) return false;
  if (upb_enumdef_ntoi(e, name, NULL))
    return false;
  if (!upb_strtable_insert(&e->ntoi, name, upb_value_int32(num)))
    return false;
  if (!upb_inttable_lookup(&e->iton, num) &&
      !upb_inttable_insert(&e->iton, num, upb_value_ptr(strdup(name))))
    return false;
  return true;
}

void upb_enumdef_setdefault(upb_enumdef *e, int32_t val) {
  assert(upb_def_ismutable(UPB_UPCAST(e)));
  e->defaultval = val;
}

void upb_enum_begin(upb_enum_iter *i, const upb_enumdef *e) {
  // We iterate over the ntoi table, to account for duplicate numbers.
  upb_strtable_begin(i, &e->ntoi);
}

void upb_enum_next(upb_enum_iter *iter) { upb_strtable_next(iter); }
bool upb_enum_done(upb_enum_iter *iter) { return upb_strtable_done(iter); }

bool upb_enumdef_ntoi(const upb_enumdef *def, const char *name, int32_t *num) {
  const upb_value *v = upb_strtable_lookup(&def->ntoi, name);
  if (!v) return false;
  if (num) *num = upb_value_getint32(*v);
  return true;
}

const char *upb_enumdef_iton(const upb_enumdef *def, int32_t num) {
  const upb_value *v = upb_inttable_lookup32(&def->iton, num);
  return v ? upb_value_getptr(*v) : NULL;
}


/* upb_fielddef ***************************************************************/

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define TYPE_INFO(ctype, inmemory_type) \
    {alignof(ctype), sizeof(ctype), UPB_CTYPE_ ## inmemory_type}

const upb_typeinfo upb_types[UPB_NUM_TYPES] = {
  // END_GROUP is not real, but used to signify the pseudo-field that
  // ends a group from within the group.
  TYPE_INFO(void*,     PTR),        // ENDGROUP
  TYPE_INFO(double,    DOUBLE),     // DOUBLE
  TYPE_INFO(float,     FLOAT),      // FLOAT
  TYPE_INFO(int64_t,   INT64),      // INT64
  TYPE_INFO(uint64_t,  UINT64),     // UINT64
  TYPE_INFO(int32_t,   INT32),      // INT32
  TYPE_INFO(uint64_t,  UINT64),     // FIXED64
  TYPE_INFO(uint32_t,  UINT32),     // FIXED32
  TYPE_INFO(bool,      BOOL),       // BOOL
  TYPE_INFO(void*,     BYTEREGION), // STRING
  TYPE_INFO(void*,     PTR),        // GROUP
  TYPE_INFO(void*,     PTR),        // MESSAGE
  TYPE_INFO(void*,     BYTEREGION), // BYTES
  TYPE_INFO(uint32_t,  UINT32),     // UINT32
  TYPE_INFO(uint32_t,  INT32),      // ENUM
  TYPE_INFO(int32_t,   INT32),      // SFIXED32
  TYPE_INFO(int64_t,   INT64),      // SFIXED64
  TYPE_INFO(int32_t,   INT32),      // SINT32
  TYPE_INFO(int64_t,   INT64),      // SINT64
};

static void upb_fielddef_init_default(upb_fielddef *f);

upb_fielddef *upb_fielddef_new(const void *owner) {
  upb_fielddef *f = malloc(sizeof(*f));
  if (!f) return NULL;
  if (!upb_def_init(UPB_UPCAST(f), UPB_DEF_FIELD, owner)) {
    free(f);
    return NULL;
  }
  f->msgdef = NULL;
  f->sub.def = NULL;
  f->subdef_is_symbolic = false;
  f->subdef_is_owned = false;
  f->label = UPB_LABEL(OPTIONAL);
  f->hasbit = -1;
  f->offset = 0;
  f->accessor = NULL;
  upb_value_setfielddef(&f->fval, f);

  // These are initialized to be invalid; the user must set them explicitly.
  // Could relax this later if it's convenient and non-confusing to have a
  // defaults for them.
  f->type = UPB_TYPE_NONE;
  f->number = 0;

  upb_fielddef_init_default(f);
  return f;
}

static void upb_fielddef_uninit_default(upb_fielddef *f) {
  if (f->default_is_string)
    upb_byteregion_free(upb_value_getbyteregion(f->defaultval));
}

static void upb_fielddef_free(upb_fielddef *f) {
  if (f->subdef_is_owned)
    upb_def_unref(f->sub.def, &f->sub.def);
  upb_fielddef_uninit_default(f);
  upb_def_uninit(UPB_UPCAST(f));
  free(f);
}

upb_fielddef *upb_fielddef_dup(const upb_fielddef *f, const void *owner) {
  upb_fielddef *newf = upb_fielddef_new(owner);
  if (!newf) return NULL;
  upb_fielddef_settype(newf, upb_fielddef_type(f));
  upb_fielddef_setlabel(newf, upb_fielddef_label(f));
  upb_fielddef_setnumber(newf, upb_fielddef_number(f));
  upb_fielddef_setname(newf, upb_fielddef_name(f));
  upb_fielddef_sethasbit(newf, upb_fielddef_hasbit(f));
  upb_fielddef_setoffset(newf, upb_fielddef_offset(f));
  upb_fielddef_setaccessor(newf, upb_fielddef_accessor(f));
  upb_fielddef_setfval(newf, upb_fielddef_fval(f));
  if (f->default_is_string) {
    upb_byteregion *r = upb_value_getbyteregion(upb_fielddef_default(f));
    size_t len;
    const char *ptr = upb_byteregion_getptr(r, 0, &len);
    assert(len == upb_byteregion_len(r));
    upb_fielddef_setdefaultstr(newf, ptr, len);
  } else {
    upb_fielddef_setdefault(newf, upb_fielddef_default(f));
  }

  const char *srcname;
  if (f->subdef_is_symbolic) {
    srcname = f->sub.name;  // Might be NULL.
  } else {
    srcname = f->sub.def ? upb_def_fullname(f->sub.def) : NULL;
  }
  if (srcname) {
    char *newname = malloc(strlen(f->sub.def->fullname) + 2);
    if (!newname) {
      upb_fielddef_unref(newf, owner);
      return NULL;
    }
    strcpy(newname, ".");
    strcat(newname, f->sub.def->fullname);
    upb_fielddef_setsubtypename(newf, newname);
    free(newname);
  }

  return newf;
}

static void upb_fielddef_init_default(upb_fielddef *f) {
  f->default_is_string = false;
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE(DOUBLE): upb_value_setdouble(&f->defaultval, 0); break;
    case UPB_TYPE(FLOAT): upb_value_setfloat(&f->defaultval, 0); break;
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64): upb_value_setuint64(&f->defaultval, 0); break;
    case UPB_TYPE(INT64):
    case UPB_TYPE(SFIXED64):
    case UPB_TYPE(SINT64): upb_value_setint64(&f->defaultval, 0); break;
    case UPB_TYPE(ENUM):
    case UPB_TYPE(INT32):
    case UPB_TYPE(SINT32):
    case UPB_TYPE(SFIXED32): upb_value_setint32(&f->defaultval, 0); break;
    case UPB_TYPE(UINT32):
    case UPB_TYPE(FIXED32): upb_value_setuint32(&f->defaultval, 0); break;
    case UPB_TYPE(BOOL): upb_value_setbool(&f->defaultval, false); break;
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
        upb_value_setbyteregion(&f->defaultval, upb_byteregion_new(""));
        f->default_is_string = true;
        break;
    case UPB_TYPE(GROUP):
    case UPB_TYPE(MESSAGE): upb_value_setptr(&f->defaultval, NULL); break;
    case UPB_TYPE_ENDGROUP: assert(false);
    case UPB_TYPE_NONE: break;
  }
}

const upb_def *upb_fielddef_subdef(const upb_fielddef *f) {
  if (upb_hassubdef(f) && upb_fielddef_isfinalized(f)) {
    assert(f->sub.def);
    return f->sub.def;
  } else {
    return f->subdef_is_symbolic ? NULL : f->sub.def;
  }
}

upb_def *upb_fielddef_subdef_mutable(upb_fielddef *f) {
  return (upb_def*)upb_fielddef_subdef(f);
}

const char *upb_fielddef_subtypename(upb_fielddef *f) {
  assert(upb_fielddef_ismutable(f));
  return f->subdef_is_symbolic ? f->sub.name : NULL;
}

// Could expose this to clients if a client wants to call it independently
// of upb_resolve() for whatever reason.
static bool upb_fielddef_resolvedefault(upb_fielddef *f, upb_status *s) {
  if (!f->default_is_string) return true;
  // Resolve the enum's default from a string to an integer.
  upb_byteregion *bytes = upb_value_getbyteregion(f->defaultval);
  assert(bytes);  // Points to either a real default or the empty string.
  upb_enumdef *e = upb_downcast_enumdef(upb_fielddef_subdef_mutable(f));
  int32_t val = 0;
  if (upb_byteregion_len(bytes) == 0) {
    upb_value_setint32(&f->defaultval, e->defaultval);
  } else {
    size_t len;
    // ptr is guaranteed to be NULL-terminated because the byteregion was
    // created with upb_byteregion_newl().
    const char *ptr = upb_byteregion_getptr(
        bytes, upb_byteregion_startofs(bytes), &len);
    assert(len == upb_byteregion_len(bytes));  // Should all be in one chunk.
    bool success = upb_enumdef_ntoi(e, ptr, &val);
    if (!success) {
      upb_status_seterrf(
          s, "Default enum value (%s) is not a member of the enum", ptr);
      return false;
    }
    upb_value_setint32(&f->defaultval, val);
  }
  f->default_is_string = false;
  upb_byteregion_free(bytes);
  return true;
}

bool upb_fielddef_setnumber(upb_fielddef *f, int32_t number) {
  assert(f->msgdef == NULL);
  f->number = number;
  return true;
}

bool upb_fielddef_settype(upb_fielddef *f, upb_fieldtype_t type) {
  assert(upb_fielddef_ismutable(f));
  upb_fielddef_uninit_default(f);
  f->type = type;
  upb_fielddef_init_default(f);
  return true;
}

bool upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label) {
  assert(upb_fielddef_ismutable(f));
  f->label = label;
  return true;
}

void upb_fielddef_setdefault(upb_fielddef *f, upb_value value) {
  assert(upb_fielddef_ismutable(f));
  assert(!upb_isstring(f) && !upb_issubmsg(f));
  if (f->default_is_string) {
    upb_byteregion *bytes = upb_value_getbyteregion(f->defaultval);
    assert(bytes);
    upb_byteregion_free(bytes);
  }
  f->defaultval = value;
  f->default_is_string = false;
}

bool upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len) {
  assert(upb_isstring(f) || f->type == UPB_TYPE(ENUM));
  if (f->default_is_string) {
    upb_byteregion *bytes = upb_value_getbyteregion(f->defaultval);
    assert(bytes);
    upb_byteregion_free(bytes);
  } else {
    assert(f->type == UPB_TYPE(ENUM));
  }
  if (f->type == UPB_TYPE(ENUM) && !upb_isident(str, len, false)) return false;
  upb_byteregion *r = upb_byteregion_newl(str, len);
  upb_value_setbyteregion(&f->defaultval, r);
  upb_bytesuccess_t ret = upb_byteregion_fetch(r);
  (void)ret;
  assert(ret == (len == 0 ? UPB_BYTE_EOF : UPB_BYTE_OK));
  assert(upb_byteregion_available(r, 0) == upb_byteregion_len(r));
  f->default_is_string = true;
  return true;
}

void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str) {
  upb_fielddef_setdefaultstr(f, str, str ? strlen(str) : 0);
}

void upb_fielddef_setfval(upb_fielddef *f, upb_value fval) {
  assert(upb_fielddef_ismutable(f));
  // TODO: we need an ownership/freeing mechanism for dynamically-allocated
  // fvals.  One possibility is to let the user supply a free() function
  // and call it when the fval is no longer referenced.  Would have to
  // ensure that no common use cases need cycles.
  //
  // For now the fval has no ownership; the caller must simply guarantee
  // somehow that it outlives any handlers/plan.
  f->fval = fval;
}

void upb_fielddef_sethasbit(upb_fielddef *f, int16_t hasbit) {
  assert(upb_fielddef_ismutable(f));
  f->hasbit = hasbit;
}

void upb_fielddef_setoffset(upb_fielddef *f, uint16_t offset) {
  assert(upb_fielddef_ismutable(f));
  f->offset = offset;
}

void upb_fielddef_setaccessor(upb_fielddef *f, struct _upb_accessor_vtbl *tbl) {
  assert(upb_fielddef_ismutable(f));
  f->accessor = tbl;
}

static bool upb_subtype_typecheck(upb_fielddef *f, const upb_def *subdef) {
  if (f->type == UPB_TYPE(MESSAGE) || f->type == UPB_TYPE(GROUP))
    return upb_dyncast_msgdef_const(subdef) != NULL;
  else if (f->type == UPB_TYPE(ENUM))
    return upb_dyncast_enumdef_const(subdef) != NULL;
  else {
    assert(false);
    return false;
  }
}

bool upb_fielddef_setsubdef(upb_fielddef *f, upb_def *subdef) {
  assert(upb_fielddef_ismutable(f));
  assert(upb_hassubdef(f));
  assert(subdef);
  if (!upb_subtype_typecheck(f, subdef)) return false;
  if (f->subdef_is_symbolic) free(f->sub.name);
  f->sub.def = subdef;
  f->subdef_is_symbolic = false;
  return true;
}

bool upb_fielddef_setsubtypename(upb_fielddef *f, const char *name) {
  assert(upb_fielddef_ismutable(f));
  assert(upb_hassubdef(f));
  if (f->subdef_is_symbolic) free(f->sub.name);
  f->sub.name = strdup(name);
  f->subdef_is_symbolic = true;
  return true;
}


/* upb_msgdef *****************************************************************/

upb_msgdef *upb_msgdef_new(const void *owner) {
  upb_msgdef *m = malloc(sizeof(*m));
  if (!m) return NULL;
  if (!upb_def_init(&m->base, UPB_DEF_MSG, owner)) goto err2;
  if (!upb_inttable_init(&m->itof)) goto err2;
  if (!upb_strtable_init(&m->ntof)) goto err1;
  m->size = 0;
  m->hasbit_bytes = 0;
  m->extstart = 0;
  m->extend = 0;
  return m;

err1:
  upb_inttable_uninit(&m->itof);
err2:
  free(m);
  return NULL;
}

static void upb_msgdef_free(upb_msgdef *m) {
  upb_strtable_uninit(&m->ntof);
  upb_inttable_uninit(&m->itof);
  upb_def_uninit(&m->base);
  free(m);
}

upb_msgdef *upb_msgdef_dup(const upb_msgdef *m, const void *owner) {
  upb_msgdef *newm = upb_msgdef_new(owner);
  if (!newm) return NULL;
  upb_msgdef_setsize(newm, upb_msgdef_size(m));
  upb_msgdef_sethasbit_bytes(newm, upb_msgdef_hasbit_bytes(m));
  upb_msgdef_setextrange(newm, upb_msgdef_extstart(m), upb_msgdef_extend(m));
  upb_def_setfullname(UPB_UPCAST(newm), upb_def_fullname(UPB_UPCAST(m)));
  upb_msg_iter i;
  for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_fielddef_dup(upb_msg_iter_field(&i), &f);
    if (!f || !upb_msgdef_addfield(newm, f, &f)) {
      upb_msgdef_unref(newm, owner);
      return NULL;
    }
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

bool upb_msgdef_addfields(upb_msgdef *m, upb_fielddef *const *fields, int n,
                          const void *ref_donor) {
  // Check constraints for all fields before performing any action.
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = fields[i];
    if (f->msgdef != NULL ||
        upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0 ||
        upb_msgdef_itof(m, upb_fielddef_number(f)) ||
        upb_msgdef_ntof(m, upb_fielddef_name(f)))
      return false;
  }

  // Constraint checks ok, perform the action.
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = fields[i];
    f->msgdef = m;
    upb_inttable_insert(&m->itof, upb_fielddef_number(f), upb_value_ptr(f));
    upb_strtable_insert(&m->ntof, upb_fielddef_name(f), upb_value_ptr(f));
    upb_fielddef_ref(f, m);
    if (ref_donor) upb_fielddef_unref(f, ref_donor);
  }
  return true;
}

void upb_msg_begin(upb_msg_iter *iter, const upb_msgdef *m) {
  upb_inttable_begin(iter, &m->itof);
}

void upb_msg_next(upb_msg_iter *iter) { upb_inttable_next(iter); }


/* upb_symtab *****************************************************************/

upb_symtab *upb_symtab_new(const void *owner) {
  upb_symtab *s = malloc(sizeof(*s));
  upb_refcount_init(&s->refcount, owner);
  upb_strtable_init(&s->symtab);
  return s;
}

void upb_symtab_ref(const upb_symtab *s, const void *owner) {
  upb_refcount_ref(&s->refcount, owner);
}

void upb_symtab_unref(const upb_symtab *s, const void *owner) {
  if(s && upb_refcount_unref(&s->refcount, owner)) {
    upb_symtab *destroying = (upb_symtab*)s;
    upb_strtable_iter i;
    upb_strtable_begin(&i, &destroying->symtab);
    for (; !upb_strtable_done(&i); upb_strtable_next(&i))
      upb_def_unref(upb_value_getptr(upb_strtable_iter_value(&i)), s);
    upb_strtable_uninit(&destroying->symtab);
    upb_refcount_uninit(&destroying->refcount);
    free(destroying);
  }
}

void upb_symtab_donateref(
    const upb_symtab *s, const void *from, const void *to) {
  upb_refcount_donateref(&s->refcount, from, to);
}

const upb_def **upb_symtab_getdefs(const upb_symtab *s, int *count,
                                   upb_deftype_t type, const void *owner) {
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
  *count = i;
  if (owner)
    for(i = 0; i < *count; i++) upb_def_ref(defs[i], owner);
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
    ret = upb_downcast_msgdef(def);
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

// Adds dups of any existing def that can reach a def with the same name as one
// of "defs."  This is to provide a consistent output graph as documented in
// the header file.  We use a modified depth-first traversal that traverses
// each SCC (which we already computed) as if it were a single node.  This
// allows us to traverse the possibly-cyclic graph as if it were a DAG and to
// easily dup the correct set of nodes with O(n) time.
//
// Returns true if defs that can reach "def" need to be duplicated into deftab.
static bool upb_resolve_dfs(const upb_def *def, upb_strtable *deftab,
                            const void *new_owner, upb_inttable *seen,
                            upb_status *s) {
  // Memoize results of this function for efficiency (since we're traversing a
  // DAG this is not needed to limit the depth of the search).
  upb_value *v = upb_inttable_lookup(seen, (uintptr_t)def);
  if (v) return upb_value_getbool(*v);

  // Visit submessages for all messages in the SCC.
  bool need_dup = false;
  const upb_def *base = def;
  do {
    assert(upb_def_isfinalized(def));
    if (def->type == UPB_DEF_FIELD) continue;
    upb_value *v = upb_strtable_lookup(deftab, upb_def_fullname(def));
    if (v) {
      upb_def *add_def = upb_value_getptr(*v);
      if (add_def->refcount.next && add_def->refcount.next != &def->refcount) {
        upb_status_seterrf(s, "conflicting existing defs for name: '%s'",
                           upb_def_fullname(def));
        return false;
      }
      need_dup = true;
    }
    const upb_msgdef *m = upb_dyncast_msgdef_const(def);
    if (m) {
      upb_msg_iter i;
      for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
        upb_fielddef *f = upb_msg_iter_field(&i);
        if (!upb_hassubdef(f)) continue;
        // |= to avoid short-circuit; we need its side-effects.
        need_dup |= upb_resolve_dfs(
            upb_fielddef_subdef_mutable(f), deftab, new_owner, seen, s);
        if (!upb_ok(s)) return false;
      }
    }
  } while ((def = (upb_def*)def->refcount.next) != base);

  if (need_dup) {
    // Dup any defs that don't already have entries in deftab.
    def = base;
    do {
      if (def->type == UPB_DEF_FIELD) continue;
      const char *name = upb_def_fullname(def);
      if (upb_strtable_lookup(deftab, name) == NULL) {
        upb_def *newdef = upb_def_dup(def, new_owner);
        if (!newdef) goto oom;
        // We temporarily use this field to track who we were dup'd from.
        newdef->refcount.next = (upb_refcount*)def;
        if (!upb_strtable_insert(deftab, name, upb_value_ptr(newdef)))
          goto oom;
      }
    } while ((def = (upb_def*)def->refcount.next) != base);
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
  if (!upb_strtable_init(&addtab)) {
    upb_status_seterrliteral(status, "out of memory");
    return false;
  }

  // Add new defs to table.
  for (int i = 0; i < n; i++) {
    upb_def *def = defs[i];
    assert(upb_def_ismutable(def));
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
    if (!upb_strtable_insert(&addtab, fullname, upb_value_ptr(def)))
      goto oom_err;
    // We temporarily use this field to indicate that we came from the user's
    // list rather than being dup'd.
    def->refcount.next = NULL;
  }

  // Add dups of any existing def that can reach a def with the same name as
  // one of "defs."
  upb_inttable seen;
  if (!upb_inttable_init(&seen)) goto oom_err;
  upb_strtable_iter i;
  upb_strtable_begin(&i, &s->symtab);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
    upb_resolve_dfs(def, &addtab, ref_donor, &seen, status);
    if (!upb_ok(status)) goto err;
  }
  upb_inttable_uninit(&seen);

  // Now using the table, resolve symbolic references.
  upb_strtable_begin(&i, &addtab);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
    upb_msgdef *m = upb_dyncast_msgdef(def);
    if (!m) continue;
    // Type names are resolved relative to the message in which they appear.
    const char *base = upb_def_fullname(UPB_UPCAST(m));

    upb_msg_iter j;
    for(upb_msg_begin(&j, m); !upb_msg_done(&j); upb_msg_next(&j)) {
      upb_fielddef *f = upb_msg_iter_field(&j);
      const char *name = upb_fielddef_subtypename(f);
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

      if (upb_fielddef_type(f) == UPB_TYPE(ENUM) && upb_fielddef_subdef(f) &&
          !upb_fielddef_resolvedefault(f, status))
        goto err;
    }
  }

  // We need an array of the defs in addtab, for passing to upb_finalize.
  add_defs = malloc(sizeof(void*) * upb_strtable_count(&addtab));
  if (add_defs == NULL) goto oom_err;
  upb_strtable_begin(&i, &addtab);
  for (n = 0; !upb_strtable_done(&i); upb_strtable_next(&i))
    add_defs[n++] = upb_value_getptr(upb_strtable_iter_value(&i));

  // Restore the next pointer that we stole.
  for (int i = 0; i < n; i++)
    add_defs[i]->refcount.next = &add_defs[i]->refcount;

  if (!upb_finalize(add_defs, n, status)) goto err;
  upb_strtable_uninit(&addtab);

  for (int i = 0; i < n; i++) {
    upb_def *def = add_defs[i];
    const char *name = upb_def_fullname(def);
    upb_def_donateref(def, ref_donor, s);
    upb_value *v = upb_strtable_lookup(&s->symtab, name);
    if(v) {
      upb_def_unref(upb_value_getptr(*v), s);
      upb_value_setptr(v, def);
    } else {
      upb_strtable_insert(&s->symtab, name, upb_value_ptr(def));
    }
  }
  free(add_defs);
  return true;

oom_err:
  upb_status_seterrliteral(status, "out of memory");
err: {
    // Need to unref any defs we dup'd (we can distinguish them from defs that
    // the user passed in by their def->refcount.next pointers).
    upb_strtable_iter i;
    upb_strtable_begin(&i, &addtab);
    for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
      upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
      if (def->refcount.next) upb_def_unref(def, s);
    }
  }
  upb_strtable_uninit(&addtab);
  free(add_defs);
  return false;
}
