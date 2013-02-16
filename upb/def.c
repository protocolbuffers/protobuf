/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/def.h"

#include <stdlib.h>
#include <string.h>
#include "upb/bytestream.h"
#include "upb/handlers.h"

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

upb_deftype_t upb_def_type(const upb_def *d) { return d->type; }

const char *upb_def_fullname(const upb_def *d) { return d->fullname; }

bool upb_def_setfullname(upb_def *def, const char *fullname) {
  assert(!upb_def_isfrozen(def));
  if (!upb_isident(fullname, strlen(fullname), true)) return false;
  free((void*)def->fullname);
  def->fullname = upb_strdup(fullname);
  return true;
}

upb_def *upb_def_dup(const upb_def *def, const void *o) {
  switch (def->type) {
    case UPB_DEF_MSG:
      return upb_upcast(upb_msgdef_dup(upb_downcast_msgdef(def), o));
    case UPB_DEF_FIELD:
      return upb_upcast(upb_fielddef_dup(upb_downcast_fielddef(def), o));
    case UPB_DEF_ENUM:
      return upb_upcast(upb_enumdef_dup(upb_downcast_enumdef(def), o));
    default: assert(false); return NULL;
  }
}

bool upb_def_isfrozen(const upb_def *def) {
  return upb_refcounted_isfrozen(upb_upcast(def));
}

void upb_def_ref(const upb_def *def, const void *owner) {
  upb_refcounted_ref(upb_upcast(def), owner);
}

void upb_def_unref(const upb_def *def, const void *owner) {
  upb_refcounted_unref(upb_upcast(def), owner);
}

void upb_def_donateref(const upb_def *def, const void *from, const void *to) {
  upb_refcounted_donateref(upb_upcast(def), from, to);
}

void upb_def_checkref(const upb_def *def, const void *owner) {
  upb_refcounted_checkref(upb_upcast(def), owner);
}

static bool upb_def_init(upb_def *def, upb_deftype_t type,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
  if (!upb_refcounted_init(upb_upcast(def), vtbl, owner)) return false;
  def->type = type;
  def->fullname = NULL;
  def->came_from_user = false;
  return true;
}

static void upb_def_uninit(upb_def *def) {
  free((void*)def->fullname);
}

static const char *msgdef_name(const upb_msgdef *m) {
  const char *name = upb_def_fullname(upb_upcast(m));
  return name ? name : "(anonymous)";
}

static bool upb_validate_field(upb_fielddef *f, upb_status *s) {
  if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
    upb_status_seterrliteral(s, "fielddef must have name and number set");
    return false;
  }
  if (upb_fielddef_hassubdef(f)) {
    if (f->subdef_is_symbolic) {
      upb_status_seterrf(s,
          "field '%s' has not been resolved", upb_fielddef_name(f));
      return false;
    }

    const upb_def *subdef = upb_fielddef_subdef(f);
    if (subdef == NULL) {
      upb_status_seterrf(s,
          "field %s.%s is missing required subdef",
          msgdef_name(f->msgdef), upb_fielddef_name(f));
      return false;
    } else if (!upb_def_isfrozen(subdef) && !subdef->came_from_user) {
      upb_status_seterrf(s,
          "subdef of field %s.%s is not frozen or being frozen",
          msgdef_name(f->msgdef), upb_fielddef_name(f));
      return false;
    } else if (upb_fielddef_default_is_symbolic(f)) {
      upb_status_seterrf(s,
          "enum field %s.%s has not been resolved",
          msgdef_name(f->msgdef), upb_fielddef_name(f));
      return false;
    }
  }
  return true;
}

bool upb_def_freeze(upb_def *const* defs, int n, upb_status *s) {
  // First perform validation, in two passes so we can check that we have a
  // transitive closure without needing to search.
  for (int i = 0; i < n; i++) {
    upb_def *def = defs[i];
    if (upb_def_isfrozen(def)) {
      // Could relax this requirement if it's annoying.
      upb_status_seterrliteral(s, "def is already frozen");
      goto err;
    } else if (def->type == UPB_DEF_FIELD) {
      upb_status_seterrliteral(s, "standalone fielddefs can not be frozen");
      goto err;
    } else {
      // Set now to detect transitive closure in the second pass.
      def->came_from_user = true;
    }
  }

  for (int i = 0; i < n; i++) {
    upb_msgdef *m = upb_dyncast_msgdef_mutable(defs[i]);
    upb_enumdef *e = upb_dyncast_enumdef_mutable(defs[i]);
    if (m) {
      upb_inttable_compact(&m->itof);
      upb_msg_iter j;
      uint32_t selector = 0;
      for(upb_msg_begin(&j, m); !upb_msg_done(&j); upb_msg_next(&j)) {
        upb_fielddef *f = upb_msg_iter_field(&j);
        assert(f->msgdef == m);
        if (!upb_validate_field(f, s)) goto err;
        f->selector_base = selector + upb_handlers_selectorbaseoffset(f);
        selector += upb_handlers_selectorcount(f);
      }
      m->selector_count = selector;
    } else if (e) {
      upb_inttable_compact(&e->iton);
    }
  }

  // Validation all passed; freeze the defs.
  return upb_refcounted_freeze((upb_refcounted*const*)defs, n, s);

err:
  for (int i = 0; i < n; i++) {
    defs[i]->came_from_user = false;
  }
  assert(!upb_ok(s));
  return false;
}


/* upb_enumdef ****************************************************************/

static void upb_enumdef_free(upb_refcounted *r) {
  upb_enumdef *e = (upb_enumdef*)r;
  upb_inttable_iter i;
  upb_inttable_begin(&i, &e->iton);
  for( ; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    // To clean up the upb_strdup() from upb_enumdef_addval().
    free(upb_value_getcstr(upb_inttable_iter_value(&i)));
  }
  upb_strtable_uninit(&e->ntoi);
  upb_inttable_uninit(&e->iton);
  upb_def_uninit(upb_upcast(e));
  free(e);
}

upb_enumdef *upb_enumdef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {NULL, &upb_enumdef_free};
  upb_enumdef *e = malloc(sizeof(*e));
  if (!e) return NULL;
  if (!upb_def_init(upb_upcast(e), UPB_DEF_ENUM, &vtbl, owner)) goto err2;
  if (!upb_strtable_init(&e->ntoi, UPB_CTYPE_INT32)) goto err2;
  if (!upb_inttable_init(&e->iton, UPB_CTYPE_CSTR)) goto err1;
  return e;

err1:
  upb_strtable_uninit(&e->ntoi);
err2:
  free(e);
  return NULL;
}

upb_enumdef *upb_enumdef_dup(const upb_enumdef *e, const void *owner) {
  upb_enumdef *new_e = upb_enumdef_new(owner);
  if (!new_e) return NULL;
  upb_enum_iter i;
  for(upb_enum_begin(&i, e); !upb_enum_done(&i); upb_enum_next(&i)) {
    bool success = upb_enumdef_addval(
        new_e, upb_enum_iter_name(&i),upb_enum_iter_number(&i), NULL);
    if (!success) {
      upb_enumdef_unref(new_e, owner);
      return NULL;
    }
  }
  return new_e;
}

bool upb_enumdef_isfrozen(const upb_enumdef *e) {
  return upb_def_isfrozen(upb_upcast(e));
}

void upb_enumdef_ref(const upb_enumdef *e, const void *owner) {
  upb_def_ref(upb_upcast(e), owner);
}

void upb_enumdef_unref(const upb_enumdef *e, const void *owner) {
  upb_def_unref(upb_upcast(e), owner);
}

void upb_enumdef_donateref(
    const upb_enumdef *e, const void *from, const void *to) {
  upb_def_donateref(upb_upcast(e), from, to);
}

void upb_enumdef_checkref(const upb_enumdef *e, const void *owner) {
  upb_def_checkref(upb_upcast(e), owner);
}

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return upb_def_fullname(upb_upcast(e));
}

bool upb_enumdef_setfullname(upb_enumdef *e, const char *fullname) {
  return upb_def_setfullname(upb_upcast(e), fullname);
}

bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num,
                        upb_status *status) {
  if (!upb_isident(name, strlen(name), false)) {
    upb_status_seterrf(status, "name '%s' is not a valid identifier", name);
    return false;
  }
  if (upb_enumdef_ntoi(e, name, NULL)) {
    upb_status_seterrf(status, "name '%s' is already defined", name);
    return false;
  }
  if (!upb_strtable_insert(&e->ntoi, name, upb_value_int32(num))) {
    upb_status_seterrliteral(status, "out of memory");
    return false;
  }
  if (!upb_inttable_lookup(&e->iton, num) &&
      !upb_inttable_insert(&e->iton, num, upb_value_cstr(upb_strdup(name)))) {
    upb_status_seterrliteral(status, "out of memory");
    upb_strtable_remove(&e->ntoi, name, NULL);
    return false;
  }
  return true;
}

int32_t upb_enumdef_default(const upb_enumdef *e) { return e->defaultval; }

void upb_enumdef_setdefault(upb_enumdef *e, int32_t val) {
  assert(!upb_enumdef_isfrozen(e));
  e->defaultval = val;
}

int upb_enumdef_numvals(const upb_enumdef *e) {
  return upb_strtable_count(&e->ntoi);
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
  return v ? upb_value_getcstr(*v) : NULL;
}

const char *upb_enum_iter_name(upb_enum_iter *iter) {
  return upb_strtable_iter_key(iter);
}

int32_t upb_enum_iter_number(upb_enum_iter *iter) {
  return upb_value_getint32(upb_strtable_iter_value(iter));
}


/* upb_fielddef ***************************************************************/

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define TYPE_INFO(ctype, inmemory_type) \
    {alignof(ctype), sizeof(ctype), UPB_CTYPE_ ## inmemory_type}

const upb_typeinfo upb_types[UPB_NUM_TYPES] = {
  TYPE_INFO(void*,     PTR),        // (unused)
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
  TYPE_INFO(int32_t,   INT32),      // ENUM
  TYPE_INFO(int32_t,   INT32),      // SFIXED32
  TYPE_INFO(int64_t,   INT64),      // SFIXED64
  TYPE_INFO(int32_t,   INT32),      // SINT32
  TYPE_INFO(int64_t,   INT64),      // SINT64
};

static void upb_fielddef_init_default(upb_fielddef *f);

static void upb_fielddef_uninit_default(upb_fielddef *f) {
  if (f->default_is_string)
    upb_byteregion_free(upb_value_getbyteregion(f->defaultval));
}

static void visitfield(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const upb_fielddef *f = (const upb_fielddef*)r;
  if (f->msgdef) {
    visit(r, upb_upcast2(f->msgdef), closure);
  }
  if (!f->subdef_is_symbolic && f->sub.def) {
    visit(r, upb_upcast(f->sub.def), closure);
  }
}

static void freefield(upb_refcounted *r) {
  upb_fielddef *f = (upb_fielddef*)r;
  upb_fielddef_uninit_default(f);
  if (f->subdef_is_symbolic)
    free(f->sub.name);
  upb_def_uninit(upb_upcast(f));
  free(f);
}

upb_fielddef *upb_fielddef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {visitfield, freefield};
  upb_fielddef *f = malloc(sizeof(*f));
  if (!f) return NULL;
  if (!upb_def_init(upb_upcast(f), UPB_DEF_FIELD, &vtbl, owner)) {
    free(f);
    return NULL;
  }
  f->msgdef = NULL;
  f->sub.def = NULL;
  f->subdef_is_symbolic = false;
  f->subdef_is_owned = false;
  f->label_ = UPB_LABEL(OPTIONAL);

  // These are initialized to be invalid; the user must set them explicitly.
  // Could relax this later if it's convenient and non-confusing to have a
  // defaults for them.
  f->type_ = UPB_TYPE_NONE;
  f->number_ = 0;

  upb_fielddef_init_default(f);
  return f;
}

upb_fielddef *upb_fielddef_dup(const upb_fielddef *f, const void *owner) {
  upb_fielddef *newf = upb_fielddef_new(owner);
  if (!newf) return NULL;
  upb_fielddef_settype(newf, upb_fielddef_type(f));
  upb_fielddef_setlabel(newf, upb_fielddef_label(f));
  upb_fielddef_setnumber(newf, upb_fielddef_number(f));
  upb_fielddef_setname(newf, upb_fielddef_name(f));
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
    upb_fielddef_setsubdefname(newf, newname);
    free(newname);
  }

  return newf;
}

bool upb_fielddef_isfrozen(const upb_fielddef *f) {
  return upb_def_isfrozen(upb_upcast(f));
}

void upb_fielddef_ref(const upb_fielddef *f, const void *owner) {
  upb_def_ref(upb_upcast(f), owner);
}

void upb_fielddef_unref(const upb_fielddef *f, const void *owner) {
  upb_def_unref(upb_upcast(f), owner);
}

void upb_fielddef_donateref(
    const upb_fielddef *f, const void *from, const void *to) {
  upb_def_donateref(upb_upcast(f), from, to);
}

void upb_fielddef_checkref(const upb_fielddef *f, const void *owner) {
  upb_def_checkref(upb_upcast(f), owner);
}

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
  return f->type_;
}

upb_label_t upb_fielddef_label(const upb_fielddef *f) {
  return f->label_;
}

uint32_t upb_fielddef_number(const upb_fielddef *f) { return f->number_; }

const char *upb_fielddef_name(const upb_fielddef *f) {
  return upb_def_fullname(upb_upcast(f));
}

const upb_msgdef *upb_fielddef_msgdef(const upb_fielddef *f) {
  return f->msgdef;
}

upb_msgdef *upb_fielddef_msgdef_mutable(upb_fielddef *f) {
  return (upb_msgdef*)f->msgdef;
}

bool upb_fielddef_setname(upb_fielddef *f, const char *name) {
  return upb_def_setfullname(upb_upcast(f), name);
}

upb_value upb_fielddef_default(const upb_fielddef *f) {
  return f->defaultval;
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
    case UPB_TYPE_NONE: break;
  }
}

const upb_def *upb_fielddef_subdef(const upb_fielddef *f) {
  if (upb_fielddef_hassubdef(f) && upb_fielddef_isfrozen(f)) {
    assert(f->sub.def);
    return f->sub.def;
  } else {
    return f->subdef_is_symbolic ? NULL : f->sub.def;
  }
}

upb_def *upb_fielddef_subdef_mutable(upb_fielddef *f) {
  return (upb_def*)upb_fielddef_subdef(f);
}

const char *upb_fielddef_subdefname(const upb_fielddef *f) {
  assert(!upb_fielddef_isfrozen(f));
  return f->subdef_is_symbolic ? f->sub.name : NULL;
}

bool upb_fielddef_setnumber(upb_fielddef *f, uint32_t number) {
  assert(f->msgdef == NULL);
  f->number_ = number;
  return true;
}

bool upb_fielddef_settype(upb_fielddef *f, upb_fieldtype_t type) {
  assert(!upb_fielddef_isfrozen(f));
  upb_fielddef_uninit_default(f);
  f->type_ = type;
  upb_fielddef_init_default(f);
  return true;
}

bool upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label) {
  assert(!upb_fielddef_isfrozen(f));
  f->label_ = label;
  return true;
}

void upb_fielddef_setdefault(upb_fielddef *f, upb_value value) {
  assert(!upb_fielddef_isfrozen(f));
  assert(!upb_fielddef_isstring(f) && !upb_fielddef_issubmsg(f));
  if (f->default_is_string) {
    upb_byteregion *bytes = upb_value_getbyteregion(f->defaultval);
    assert(bytes);
    upb_byteregion_free(bytes);
  }
  f->defaultval = value;
  f->default_is_string = false;
}

bool upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len) {
  assert(upb_fielddef_isstring(f) || f->type_ == UPB_TYPE(ENUM));
  if (f->type_ == UPB_TYPE(ENUM) && !upb_isident(str, len, false)) return false;

  if (f->default_is_string) {
    upb_byteregion *bytes = upb_value_getbyteregion(f->defaultval);
    assert(bytes);
    upb_byteregion_free(bytes);
  } else {
    assert(f->type_ == UPB_TYPE(ENUM));
  }

  upb_byteregion *r = upb_byteregion_newl(str, len);
  upb_value_setbyteregion(&f->defaultval, r);
  upb_bytesuccess_t ret = upb_byteregion_fetch(r);
  UPB_ASSERT_VAR(ret, ret == (len == 0 ? UPB_BYTE_EOF : UPB_BYTE_OK));
  assert(upb_byteregion_available(r, 0) == upb_byteregion_len(r));
  f->default_is_string = true;
  return true;
}

void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str) {
  upb_fielddef_setdefaultstr(f, str, str ? strlen(str) : 0);
}

bool upb_fielddef_default_is_symbolic(const upb_fielddef *f) {
  return f->default_is_string && f->type_ == UPB_TYPE_ENUM;
}

bool upb_fielddef_resolvedefault(upb_fielddef *f) {
  if (!upb_fielddef_default_is_symbolic(f)) return true;

  upb_byteregion *bytes = upb_value_getbyteregion(f->defaultval);
  const upb_enumdef *e = upb_downcast_enumdef(upb_fielddef_subdef(f));
  assert(bytes);  // Points to either a real default or the empty string.
  assert(e);
  if (upb_byteregion_len(bytes) == 0) {
    // The "default default" for an enum is the first defined value.
    upb_value_setint32(&f->defaultval, e->defaultval);
  } else {
    size_t len;
    int32_t val = 0;
    // ptr is guaranteed to be NULL-terminated because the byteregion was
    // created with upb_byteregion_newl().
    const char *ptr = upb_byteregion_getptr(
        bytes, upb_byteregion_startofs(bytes), &len);
    assert(len == upb_byteregion_len(bytes));  // Should all be in one chunk
    if (!upb_enumdef_ntoi(e, ptr, &val)) {
      return false;
    }
    upb_value_setint32(&f->defaultval, val);
  }
  f->default_is_string = false;
  upb_byteregion_free(bytes);
  return true;
}

static bool upb_subdef_typecheck(upb_fielddef *f, const upb_def *subdef) {
  if (f->type_ == UPB_TYPE(MESSAGE) || f->type_ == UPB_TYPE(GROUP))
    return upb_dyncast_msgdef(subdef) != NULL;
  else if (f->type_ == UPB_TYPE(ENUM))
    return upb_dyncast_enumdef(subdef) != NULL;
  else {
    assert(false);
    return false;
  }
}

static void release_subdef(upb_fielddef *f) {
  if (f->subdef_is_symbolic) {
    free(f->sub.name);
  } else if (f->sub.def) {
    upb_unref2(f->sub.def, f);
  }
}

bool upb_fielddef_setsubdef(upb_fielddef *f, const upb_def *subdef) {
  assert(!upb_fielddef_isfrozen(f));
  assert(upb_fielddef_hassubdef(f));
  if (subdef && !upb_subdef_typecheck(f, subdef)) return false;
  release_subdef(f);
  f->sub.def = subdef;
  f->subdef_is_symbolic = false;
  if (f->sub.def) upb_ref2(f->sub.def, f);
  return true;
}

bool upb_fielddef_setsubdefname(upb_fielddef *f, const char *name) {
  assert(!upb_fielddef_isfrozen(f));
  assert(upb_fielddef_hassubdef(f));
  release_subdef(f);
  f->sub.name = upb_strdup(name);
  f->subdef_is_symbolic = true;
  return true;
}

bool upb_fielddef_issubmsg(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_GROUP ||
         upb_fielddef_type(f) == UPB_TYPE_MESSAGE;
}

bool upb_fielddef_isstring(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_STRING ||
         upb_fielddef_type(f) == UPB_TYPE_BYTES;
}

bool upb_fielddef_isseq(const upb_fielddef *f) {
  return upb_fielddef_label(f) == UPB_LABEL_REPEATED;
}

bool upb_fielddef_isprimitive(const upb_fielddef *f) {
  return !upb_fielddef_isstring(f) && !upb_fielddef_issubmsg(f);
}

bool upb_fielddef_hassubdef(const upb_fielddef *f) {
  return upb_fielddef_issubmsg(f) || upb_fielddef_type(f) == UPB_TYPE(ENUM);
}


/* upb_msgdef *****************************************************************/

static void visitmsg(const upb_refcounted *r, upb_refcounted_visit *visit,
                     void *closure) {
  const upb_msgdef *m = (const upb_msgdef*)r;
  upb_msg_iter i;
  for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    visit(r, upb_upcast2(f), closure);
  }
}

static void freemsg(upb_refcounted *r) {
  upb_msgdef *m = (upb_msgdef*)r;
  upb_strtable_uninit(&m->ntof);
  upb_inttable_uninit(&m->itof);
  upb_def_uninit(upb_upcast(m));
  free(m);
}

upb_msgdef *upb_msgdef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {visitmsg, freemsg};
  upb_msgdef *m = malloc(sizeof(*m));
  if (!m) return NULL;
  if (!upb_def_init(upb_upcast(m), UPB_DEF_MSG, &vtbl, owner)) goto err2;
  if (!upb_inttable_init(&m->itof, UPB_CTYPE_PTR)) goto err2;
  if (!upb_strtable_init(&m->ntof, UPB_CTYPE_PTR)) goto err1;
  return m;

err1:
  upb_inttable_uninit(&m->itof);
err2:
  free(m);
  return NULL;
}

upb_msgdef *upb_msgdef_dup(const upb_msgdef *m, const void *owner) {
  upb_msgdef *newm = upb_msgdef_new(owner);
  if (!newm) return NULL;
  upb_def_setfullname(upb_upcast(newm), upb_def_fullname(upb_upcast(m)));
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

bool upb_msgdef_isfrozen(const upb_msgdef *m) {
  return upb_def_isfrozen(upb_upcast(m));
}

void upb_msgdef_ref(const upb_msgdef *m, const void *owner) {
  upb_def_ref(upb_upcast(m), owner);
}

void upb_msgdef_unref(const upb_msgdef *m, const void *owner) {
  upb_def_unref(upb_upcast(m), owner);
}

void upb_msgdef_donateref(
    const upb_msgdef *m, const void *from, const void *to) {
  upb_def_donateref(upb_upcast(m), from, to);
}

void upb_msgdef_checkref(const upb_msgdef *m, const void *owner) {
  upb_def_checkref(upb_upcast(m), owner);
}

const char *upb_msgdef_fullname(const upb_msgdef *m) {
  return upb_def_fullname(upb_upcast(m));
}

bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname) {
  return upb_def_setfullname(upb_upcast(m), fullname);
}

bool upb_msgdef_addfields(upb_msgdef *m, upb_fielddef *const *fields, int n,
                          const void *ref_donor) {
  // Check constraints for all fields before performing any action.
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = fields[i];
    // TODO(haberman): handle the case where two fields of the input duplicate
    // name or number.
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
    upb_ref2(f, m);
    upb_ref2(m, f);
    if (ref_donor) upb_fielddef_unref(f, ref_donor);
  }
  return true;
}

bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f,
                         const void *ref_donor) {
  return upb_msgdef_addfields(m, &f, 1, ref_donor);
}

const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  const upb_value *val = upb_inttable_lookup32(&m->itof, i);
  return val ? (const upb_fielddef*)upb_value_getptr(*val) : NULL;
}

const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name) {
  const upb_value *val = upb_strtable_lookup(&m->ntof, name);
  return val ? (upb_fielddef*)upb_value_getptr(*val) : NULL;
}

upb_fielddef *upb_msgdef_itof_mutable(upb_msgdef *m, uint32_t i) {
  return (upb_fielddef*)upb_msgdef_itof(m, i);
}

upb_fielddef *upb_msgdef_ntof_mutable(upb_msgdef *m, const char *name) {
  return (upb_fielddef*)upb_msgdef_ntof(m, name);
}

int upb_msgdef_numfields(const upb_msgdef *m) {
  return upb_strtable_count(&m->ntof);
}

void upb_msg_begin(upb_msg_iter *iter, const upb_msgdef *m) {
  upb_inttable_begin(iter, &m->itof);
}

void upb_msg_next(upb_msg_iter *iter) { upb_inttable_next(iter); }

bool upb_msg_done(upb_msg_iter *iter) { return upb_inttable_done(iter); }

upb_fielddef *upb_msg_iter_field(upb_msg_iter *iter) {
  return (upb_fielddef*)upb_value_getptr(upb_inttable_iter_value(iter));
}
