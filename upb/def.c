/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/def.h"

#include <stdlib.h>
#include <string.h>
#include "upb/descriptor/descriptor.upb.h"
#include "upb/handlers.h"

typedef struct {
  size_t len;
  char str[1];  // Null-terminated string data follows.
} str_t;

static str_t *newstr(const char *data, size_t len) {
  str_t *ret = malloc(sizeof(*ret) + len);
  if (!ret) return NULL;
  ret->len = len;
  memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static void freestr(str_t *s) { free(s); }

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

static bool upb_isident(const char *str, size_t len, bool full, upb_status *s) {
  bool start = true;
  for (size_t i = 0; i < len; i++) {
    char c = str[i];
    if (c == '.') {
      if (start || !full) {
        upb_status_seterrf(s, "invalid name: unexpected '.' (%s)", str);
        return false;
      }
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) {
        upb_status_seterrf(
            s, "invalid name: path components must start with a letter (%s)",
            str);
        return false;
      }
      start = false;
    } else {
      if (!upb_isalphanum(c)) {
        upb_status_seterrf(s, "invalid name: non-alphanumeric character (%s)",
                           str);
        return false;
      }
    }
  }
  return !start;
}


/* upb_def ********************************************************************/

upb_deftype_t upb_def_type(const upb_def *d) { return d->type; }

const char *upb_def_fullname(const upb_def *d) { return d->fullname; }

bool upb_def_setfullname(upb_def *def, const char *fullname, upb_status *s) {
  assert(!upb_def_isfrozen(def));
  if (!upb_isident(fullname, strlen(fullname), true, s)) return false;
  free((void*)def->fullname);
  def->fullname = upb_strdup(fullname);
  return true;
}

upb_def *upb_def_dup(const upb_def *def, const void *o) {
  switch (def->type) {
    case UPB_DEF_MSG:
      return UPB_UPCAST(upb_msgdef_dup(upb_downcast_msgdef(def), o));
    case UPB_DEF_FIELD:
      return UPB_UPCAST(upb_fielddef_dup(upb_downcast_fielddef(def), o));
    case UPB_DEF_ENUM:
      return UPB_UPCAST(upb_enumdef_dup(upb_downcast_enumdef(def), o));
    default: assert(false); return NULL;
  }
}

bool upb_def_isfrozen(const upb_def *def) {
  return upb_refcounted_isfrozen(UPB_UPCAST(def));
}

void upb_def_ref(const upb_def *def, const void *owner) {
  upb_refcounted_ref(UPB_UPCAST(def), owner);
}

void upb_def_unref(const upb_def *def, const void *owner) {
  upb_refcounted_unref(UPB_UPCAST(def), owner);
}

void upb_def_donateref(const upb_def *def, const void *from, const void *to) {
  upb_refcounted_donateref(UPB_UPCAST(def), from, to);
}

void upb_def_checkref(const upb_def *def, const void *owner) {
  upb_refcounted_checkref(UPB_UPCAST(def), owner);
}

static bool upb_def_init(upb_def *def, upb_deftype_t type,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
  if (!upb_refcounted_init(UPB_UPCAST(def), vtbl, owner)) return false;
  def->type = type;
  def->fullname = NULL;
  def->came_from_user = false;
  return true;
}

static void upb_def_uninit(upb_def *def) {
  free((void*)def->fullname);
}

static const char *msgdef_name(const upb_msgdef *m) {
  const char *name = upb_def_fullname(UPB_UPCAST(m));
  return name ? name : "(anonymous)";
}

static bool upb_validate_field(upb_fielddef *f, upb_status *s) {
  if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
    upb_status_seterrliteral(s, "fielddef must have name and number set");
    return false;
  }
  if (!f->type_is_set_) {
    upb_status_seterrliteral(s, "fielddef type was not initialized");
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

  // Second pass of validation.  Also assign selectors, compact tables.
  for (int i = 0; i < n; i++) {
    upb_msgdef *m = upb_dyncast_msgdef_mutable(defs[i]);
    upb_enumdef *e = upb_dyncast_enumdef_mutable(defs[i]);
    if (m) {
      upb_inttable_compact(&m->itof);
      upb_msg_iter j;
      uint32_t selector = UPB_STATIC_SELECTOR_COUNT;
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

  // Def graph contains FieldDefs between each MessageDef, so double the limit.
  int maxdepth = UPB_MAX_MESSAGE_DEPTH * 2;

  // Validation all passed; freeze the defs.
  return upb_refcounted_freeze((upb_refcounted * const *)defs, n, s, maxdepth);

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
  upb_def_uninit(UPB_UPCAST(e));
  free(e);
}

upb_enumdef *upb_enumdef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {NULL, &upb_enumdef_free};
  upb_enumdef *e = malloc(sizeof(*e));
  if (!e) return NULL;
  if (!upb_def_init(UPB_UPCAST(e), UPB_DEF_ENUM, &vtbl, owner)) goto err2;
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
  return upb_def_isfrozen(UPB_UPCAST(e));
}

void upb_enumdef_ref(const upb_enumdef *e, const void *owner) {
  upb_def_ref(UPB_UPCAST(e), owner);
}

void upb_enumdef_unref(const upb_enumdef *e, const void *owner) {
  upb_def_unref(UPB_UPCAST(e), owner);
}

void upb_enumdef_donateref(
    const upb_enumdef *e, const void *from, const void *to) {
  upb_def_donateref(UPB_UPCAST(e), from, to);
}

void upb_enumdef_checkref(const upb_enumdef *e, const void *owner) {
  upb_def_checkref(UPB_UPCAST(e), owner);
}

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return upb_def_fullname(UPB_UPCAST(e));
}

bool upb_enumdef_setfullname(upb_enumdef *e, const char *fullname,
                             upb_status *s) {
  return upb_def_setfullname(UPB_UPCAST(e), fullname, s);
}

bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num,
                        upb_status *status) {
  if (!upb_isident(name, strlen(name), false, status)) {
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
  if (!upb_inttable_lookup(&e->iton, num, NULL) &&
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
  upb_value v;
  if (!upb_strtable_lookup(&def->ntoi, name, &v)) {
    return false;
  }
  if (num) *num = upb_value_getint32(v);
  return true;
}

const char *upb_enumdef_iton(const upb_enumdef *def, int32_t num) {
  upb_value v;
  return upb_inttable_lookup32(&def->iton, num, &v) ?
      upb_value_getcstr(v) : NULL;
}

const char *upb_enum_iter_name(upb_enum_iter *iter) {
  return upb_strtable_iter_key(iter);
}

int32_t upb_enum_iter_number(upb_enum_iter *iter) {
  return upb_value_getint32(upb_strtable_iter_value(iter));
}


/* upb_fielddef ***************************************************************/

static void upb_fielddef_init_default(upb_fielddef *f);

static void upb_fielddef_uninit_default(upb_fielddef *f) {
  if (f->type_is_set_ && f->default_is_string)
    freestr(f->defaultval.bytes);
}

static void visitfield(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const upb_fielddef *f = (const upb_fielddef*)r;
  if (f->msgdef) {
    visit(r, UPB_UPCAST2(f->msgdef), closure);
  }
  if (!f->subdef_is_symbolic && f->sub.def) {
    visit(r, UPB_UPCAST(f->sub.def), closure);
  }
}

static void freefield(upb_refcounted *r) {
  upb_fielddef *f = (upb_fielddef*)r;
  upb_fielddef_uninit_default(f);
  if (f->subdef_is_symbolic)
    free(f->sub.name);
  upb_def_uninit(UPB_UPCAST(f));
  free(f);
}

upb_fielddef *upb_fielddef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {visitfield, freefield};
  upb_fielddef *f = malloc(sizeof(*f));
  if (!f) return NULL;
  if (!upb_def_init(UPB_UPCAST(f), UPB_DEF_FIELD, &vtbl, owner)) {
    free(f);
    return NULL;
  }
  f->msgdef = NULL;
  f->sub.def = NULL;
  f->subdef_is_symbolic = false;
  f->label_ = UPB_LABEL_OPTIONAL;
  f->type_ = UPB_TYPE_INT32;
  f->number_ = 0;
  f->type_is_set_ = false;
  f->tagdelim = false;

  // For the moment we default this to UPB_INTFMT_VARIABLE, since it will work
  // with all integer types and is in some since more "default" since the most
  // normal-looking proto2 types int32/int64/uint32/uint64 use variable.
  //
  // Other options to consider:
  // - there is no default; users must set this manually (like type).
  // - default signed integers to UPB_INTFMT_ZIGZAG, since it's more likely to
  //   be an optimal default for signed integers.
  f->intfmt = UPB_INTFMT_VARIABLE;
  return f;
}

upb_fielddef *upb_fielddef_dup(const upb_fielddef *f, const void *owner) {
  upb_fielddef *newf = upb_fielddef_new(owner);
  if (!newf) return NULL;
  upb_fielddef_settype(newf, upb_fielddef_type(f));
  upb_fielddef_setlabel(newf, upb_fielddef_label(f));
  upb_fielddef_setnumber(newf, upb_fielddef_number(f), NULL);
  upb_fielddef_setname(newf, upb_fielddef_name(f), NULL);
  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    upb_fielddef_setdefaultstr(newf, s->str, s->len, NULL);
  } else {
    newf->defaultval = f->defaultval;
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
    upb_fielddef_setsubdefname(newf, newname, NULL);
    free(newname);
  }

  return newf;
}

bool upb_fielddef_isfrozen(const upb_fielddef *f) {
  return upb_def_isfrozen(UPB_UPCAST(f));
}

void upb_fielddef_ref(const upb_fielddef *f, const void *owner) {
  upb_def_ref(UPB_UPCAST(f), owner);
}

void upb_fielddef_unref(const upb_fielddef *f, const void *owner) {
  upb_def_unref(UPB_UPCAST(f), owner);
}

void upb_fielddef_donateref(
    const upb_fielddef *f, const void *from, const void *to) {
  upb_def_donateref(UPB_UPCAST(f), from, to);
}

void upb_fielddef_checkref(const upb_fielddef *f, const void *owner) {
  upb_def_checkref(UPB_UPCAST(f), owner);
}

bool upb_fielddef_typeisset(const upb_fielddef *f) {
  return f->type_is_set_;
}

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
  assert(f->type_is_set_);
  return f->type_;
}

upb_label_t upb_fielddef_label(const upb_fielddef *f) {
  return f->label_;
}

upb_intfmt_t upb_fielddef_intfmt(const upb_fielddef *f) {
  return f->intfmt;
}

bool upb_fielddef_istagdelim(const upb_fielddef *f) {
  return f->tagdelim;
}

uint32_t upb_fielddef_number(const upb_fielddef *f) {
  return f->number_;
}

const char *upb_fielddef_name(const upb_fielddef *f) {
  return upb_def_fullname(UPB_UPCAST(f));
}

const upb_msgdef *upb_fielddef_containingtype(const upb_fielddef *f) {
  return f->msgdef;
}

upb_msgdef *upb_fielddef_containingtype_mutable(upb_fielddef *f) {
  return (upb_msgdef*)f->msgdef;
}

bool upb_fielddef_setname(upb_fielddef *f, const char *name, upb_status *s) {
  return upb_def_setfullname(UPB_UPCAST(f), name, s);
}

static void chkdefaulttype(const upb_fielddef *f, upb_fieldtype_t type) {
  UPB_UNUSED(f);
  UPB_UNUSED(type);
  assert(f->type_is_set_ && upb_fielddef_type(f) == type);
}

int64_t upb_fielddef_defaultint64(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_INT64);
  return f->defaultval.sint;
}

int32_t upb_fielddef_defaultint32(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_INT32);
  return f->defaultval.sint;
}

uint64_t upb_fielddef_defaultuint64(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_UINT64);
  return f->defaultval.uint;
}

uint32_t upb_fielddef_defaultuint32(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_UINT32);
  return f->defaultval.uint;
}

bool upb_fielddef_defaultbool(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_BOOL);
  return f->defaultval.uint;
}

float upb_fielddef_defaultfloat(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_FLOAT);
  return f->defaultval.flt;
}

double upb_fielddef_defaultdouble(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_DOUBLE);
  return f->defaultval.dbl;
}

const char *upb_fielddef_defaultstr(const upb_fielddef *f, size_t *len) {
  assert(f->type_is_set_);
  assert(upb_fielddef_type(f) == UPB_TYPE_STRING ||
         upb_fielddef_type(f) == UPB_TYPE_BYTES ||
         upb_fielddef_type(f) == UPB_TYPE_ENUM);
  if (f->default_is_string) {
    str_t *str = f->defaultval.bytes;
    if (len) *len = str->len;
    return str->str;
  }
  return NULL;
}

static void upb_fielddef_init_default(upb_fielddef *f) {
  f->default_is_string = false;
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_DOUBLE: f->defaultval.dbl = 0; break;
    case UPB_TYPE_FLOAT: f->defaultval.flt = 0; break;
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_ENUM: f->defaultval.sint = 0; break;
    case UPB_TYPE_UINT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_BOOL: f->defaultval.uint = 0; break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      f->defaultval.bytes = newstr("", 0);
      f->default_is_string = true;
      break;
    case UPB_TYPE_MESSAGE: break;
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

const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f) {
  const upb_def *def = upb_fielddef_subdef(f);
  return def ? upb_dyncast_msgdef(def) : NULL;
}

const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f) {
  const upb_def *def = upb_fielddef_subdef(f);
  return def ? upb_dyncast_enumdef(def) : NULL;
}

upb_def *upb_fielddef_subdef_mutable(upb_fielddef *f) {
  return (upb_def*)upb_fielddef_subdef(f);
}

const char *upb_fielddef_subdefname(const upb_fielddef *f) {
  assert(!upb_fielddef_isfrozen(f));
  return f->subdef_is_symbolic ? f->sub.name : NULL;
}

bool upb_fielddef_setnumber(upb_fielddef *f, uint32_t number, upb_status *s) {
  if (f->msgdef) {
    upb_status_seterrliteral(
        s, "cannot change field number after adding to a message");
    return false;
  }
  if (number == 0 || number > UPB_MAX_FIELDNUMBER) {
    upb_status_seterrf(s, "invalid field number (%u)", number);
    return false;
  }
  f->number_ = number;
  return true;
}

void upb_fielddef_settype(upb_fielddef *f, upb_fieldtype_t type) {
  assert(!upb_fielddef_isfrozen(f));
  assert(upb_fielddef_checktype(type));
  upb_fielddef_uninit_default(f);
  f->type_ = type;
  f->type_is_set_ = true;
  upb_fielddef_init_default(f);
}

void upb_fielddef_setdescriptortype(upb_fielddef *f, int type) {
  assert(!upb_fielddef_isfrozen(f));
  switch (type) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      upb_fielddef_settype(f, UPB_TYPE_DOUBLE);
      break;
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      upb_fielddef_settype(f, UPB_TYPE_FLOAT);
      break;
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_SINT64:
      upb_fielddef_settype(f, UPB_TYPE_INT64);
      break;
    case UPB_DESCRIPTOR_TYPE_UINT64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      upb_fielddef_settype(f, UPB_TYPE_UINT64);
      break;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
    case UPB_DESCRIPTOR_TYPE_SINT32:
      upb_fielddef_settype(f, UPB_TYPE_INT32);
      break;
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
      upb_fielddef_settype(f, UPB_TYPE_UINT32);
      break;
    case UPB_DESCRIPTOR_TYPE_BOOL:
      upb_fielddef_settype(f, UPB_TYPE_BOOL);
      break;
    case UPB_DESCRIPTOR_TYPE_STRING:
      upb_fielddef_settype(f, UPB_TYPE_STRING);
      break;
    case UPB_DESCRIPTOR_TYPE_BYTES:
      upb_fielddef_settype(f, UPB_TYPE_BYTES);
      break;
    case UPB_DESCRIPTOR_TYPE_GROUP:
    case UPB_DESCRIPTOR_TYPE_MESSAGE:
      upb_fielddef_settype(f, UPB_TYPE_MESSAGE);
      break;
    case UPB_DESCRIPTOR_TYPE_ENUM:
      upb_fielddef_settype(f, UPB_TYPE_ENUM);
      break;
    default: assert(false);
  }

  if (type == UPB_DESCRIPTOR_TYPE_FIXED64 ||
      type == UPB_DESCRIPTOR_TYPE_FIXED32 ||
      type == UPB_DESCRIPTOR_TYPE_SFIXED64 ||
      type == UPB_DESCRIPTOR_TYPE_SFIXED32) {
    upb_fielddef_setintfmt(f, UPB_INTFMT_FIXED);
  } else if (type == UPB_DESCRIPTOR_TYPE_SINT64 ||
             type == UPB_DESCRIPTOR_TYPE_SINT32) {
    upb_fielddef_setintfmt(f, UPB_INTFMT_ZIGZAG);
  } else {
    upb_fielddef_setintfmt(f, UPB_INTFMT_VARIABLE);
  }

  upb_fielddef_settagdelim(f, type == UPB_DESCRIPTOR_TYPE_GROUP);
}

upb_descriptortype_t upb_fielddef_descriptortype(const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_FLOAT:  return UPB_DESCRIPTOR_TYPE_FLOAT;
    case UPB_TYPE_DOUBLE: return UPB_DESCRIPTOR_TYPE_DOUBLE;
    case UPB_TYPE_BOOL:   return UPB_DESCRIPTOR_TYPE_BOOL;
    case UPB_TYPE_STRING: return UPB_DESCRIPTOR_TYPE_STRING;
    case UPB_TYPE_BYTES:  return UPB_DESCRIPTOR_TYPE_BYTES;
    case UPB_TYPE_ENUM:   return UPB_DESCRIPTOR_TYPE_ENUM;
    case UPB_TYPE_INT32:
      switch (upb_fielddef_intfmt(f)) {
        case UPB_INTFMT_VARIABLE: return UPB_DESCRIPTOR_TYPE_INT32;
        case UPB_INTFMT_FIXED:    return UPB_DESCRIPTOR_TYPE_SFIXED32;
        case UPB_INTFMT_ZIGZAG:   return UPB_DESCRIPTOR_TYPE_SINT32;
      }
    case UPB_TYPE_INT64:
      switch (upb_fielddef_intfmt(f)) {
        case UPB_INTFMT_VARIABLE: return UPB_DESCRIPTOR_TYPE_INT64;
        case UPB_INTFMT_FIXED:    return UPB_DESCRIPTOR_TYPE_SFIXED64;
        case UPB_INTFMT_ZIGZAG:   return UPB_DESCRIPTOR_TYPE_SINT64;
      }
    case UPB_TYPE_UINT32:
      switch (upb_fielddef_intfmt(f)) {
        case UPB_INTFMT_VARIABLE: return UPB_DESCRIPTOR_TYPE_UINT32;
        case UPB_INTFMT_FIXED:    return UPB_DESCRIPTOR_TYPE_FIXED32;
        case UPB_INTFMT_ZIGZAG:   return -1;
      }
    case UPB_TYPE_UINT64:
      switch (upb_fielddef_intfmt(f)) {
        case UPB_INTFMT_VARIABLE: return UPB_DESCRIPTOR_TYPE_UINT64;
        case UPB_INTFMT_FIXED:    return UPB_DESCRIPTOR_TYPE_FIXED64;
        case UPB_INTFMT_ZIGZAG:   return -1;
      }
    case UPB_TYPE_MESSAGE:
      return upb_fielddef_istagdelim(f) ?
          UPB_DESCRIPTOR_TYPE_GROUP : UPB_DESCRIPTOR_TYPE_MESSAGE;
  }
  return 0;
}

void upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label) {
  assert(!upb_fielddef_isfrozen(f));
  assert(upb_fielddef_checklabel(label));
  f->label_ = label;
}

bool upb_fielddef_setintfmt(upb_fielddef *f, upb_intfmt_t fmt) {
  assert(!upb_fielddef_isfrozen(f));
  assert(upb_fielddef_checkintfmt(fmt));
  f->intfmt = fmt;
  return true;
}

bool upb_fielddef_settagdelim(upb_fielddef *f, bool tag_delim) {
  assert(!upb_fielddef_isfrozen(f));
  f->tagdelim = tag_delim;
  return true;
}

static bool checksetdefault(upb_fielddef *f, upb_fieldtype_t type) {
  if (!f->type_is_set_ || upb_fielddef_isfrozen(f) ||
      upb_fielddef_type(f) != type) {
    assert(false);
    return false;
  }
  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    assert(s);
    freestr(s);
  }
  f->default_is_string = false;
  return true;
}

void upb_fielddef_setdefaultint64(upb_fielddef *f, int64_t value) {
  if (checksetdefault(f, UPB_TYPE_INT64))
    f->defaultval.sint = value;
}

void upb_fielddef_setdefaultint32(upb_fielddef *f, int32_t value) {
  if ((upb_fielddef_type(f) == UPB_TYPE_ENUM &&
       checksetdefault(f, UPB_TYPE_ENUM)) ||
      checksetdefault(f, UPB_TYPE_INT32)) {
    f->defaultval.sint = value;
  }
}

void upb_fielddef_setdefaultuint64(upb_fielddef *f, uint64_t value) {
  if (checksetdefault(f, UPB_TYPE_UINT64))
    f->defaultval.uint = value;
}

void upb_fielddef_setdefaultuint32(upb_fielddef *f, uint32_t value) {
  if (checksetdefault(f, UPB_TYPE_UINT32))
    f->defaultval.uint = value;
}

void upb_fielddef_setdefaultbool(upb_fielddef *f, bool value) {
  if (checksetdefault(f, UPB_TYPE_BOOL))
    f->defaultval.uint = value;
}

void upb_fielddef_setdefaultfloat(upb_fielddef *f, float value) {
  if (checksetdefault(f, UPB_TYPE_FLOAT))
    f->defaultval.flt = value;
}

void upb_fielddef_setdefaultdouble(upb_fielddef *f, double value) {
  if (checksetdefault(f, UPB_TYPE_DOUBLE))
    f->defaultval.dbl = value;
}

bool upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len,
                                upb_status *s) {
  assert(upb_fielddef_isstring(f) || f->type_ == UPB_TYPE_ENUM);
  if (f->type_ == UPB_TYPE_ENUM && !upb_isident(str, len, false, s))
    return false;

  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    assert(s);
    freestr(s);
  } else {
    assert(f->type_ == UPB_TYPE_ENUM);
  }

  str_t *str2 = newstr(str, len);
  f->defaultval.bytes = str2;
  f->default_is_string = true;
  return true;
}

void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str,
                                 upb_status *s) {
  assert(f->type_is_set_);
  upb_fielddef_setdefaultstr(f, str, str ? strlen(str) : 0, s);
}

bool upb_fielddef_default_is_symbolic(const upb_fielddef *f) {
  return f->type_is_set_ &&
      f->default_is_string &&
      f->type_ == UPB_TYPE_ENUM;
}

bool upb_fielddef_resolveenumdefault(upb_fielddef *f, upb_status *s) {
  if (!upb_fielddef_default_is_symbolic(f)) return true;

  str_t *str = f->defaultval.bytes;
  const upb_enumdef *e = upb_downcast_enumdef(upb_fielddef_subdef(f));
  assert(str);  // Points to either a real default or the empty string.
  assert(e);
  if (str->len == 0) {
    // The "default default" for an enum is the first defined value.
    f->defaultval.sint = e->defaultval;
  } else {
    int32_t val = 0;
    if (!upb_enumdef_ntoi(e, str->str, &val)) {
      upb_status_seterrf(s, "enum default not found in enum (%s)", str->str);
      return false;
    }
    f->defaultval.sint = val;
  }
  f->default_is_string = false;
  freestr(str);
  return true;
}

static bool upb_subdef_typecheck(upb_fielddef *f, const upb_def *subdef,
                                 upb_status *s) {
  if (f->type_ == UPB_TYPE_MESSAGE) {
    if (upb_dyncast_msgdef(subdef)) return true;
    upb_status_seterrliteral(s,
                             "invalid subdef type for this submessage field");
    return false;
  } else if (f->type_ == UPB_TYPE_ENUM) {
    if (upb_dyncast_enumdef(subdef)) return true;
    upb_status_seterrliteral(s, "invalid subdef type for this enum field");
    return false;
  } else {
    upb_status_seterrliteral(s,
                             "only message and enum fields can have a subdef");
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

bool upb_fielddef_setsubdef(upb_fielddef *f, const upb_def *subdef,
                            upb_status *s) {
  assert(!upb_fielddef_isfrozen(f));
  assert(upb_fielddef_hassubdef(f));
  if (subdef && !upb_subdef_typecheck(f, subdef, s)) return false;
  release_subdef(f);
  f->sub.def = subdef;
  f->subdef_is_symbolic = false;
  if (f->sub.def) upb_ref2(f->sub.def, f);
  return true;
}

bool upb_fielddef_setmsgsubdef(upb_fielddef *f, const upb_msgdef *subdef,
                               upb_status *s) {
  return upb_fielddef_setsubdef(f, UPB_UPCAST(subdef), s);
}

bool upb_fielddef_setenumsubdef(upb_fielddef *f, const upb_enumdef *subdef,
                                upb_status *s) {
  return upb_fielddef_setsubdef(f, UPB_UPCAST(subdef), s);
}

bool upb_fielddef_setsubdefname(upb_fielddef *f, const char *name,
                                upb_status *s) {
  assert(!upb_fielddef_isfrozen(f));
  if (!upb_fielddef_hassubdef(f)) {
    upb_status_seterrliteral(s, "field type does not accept a subdef");
    return false;
  }
  release_subdef(f);
  f->sub.name = upb_strdup(name);
  f->subdef_is_symbolic = true;
  return true;
}

bool upb_fielddef_issubmsg(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_MESSAGE;
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
  return upb_fielddef_issubmsg(f) || upb_fielddef_type(f) == UPB_TYPE_ENUM;
}

static bool between(int32_t x, int32_t low, int32_t high) {
  return x >= low && x <= high;
}

bool upb_fielddef_checklabel(int32_t label) { return between(label, 1, 3); }
bool upb_fielddef_checktype(int32_t type) { return between(type, 1, 11); }
bool upb_fielddef_checkintfmt(int32_t fmt) { return between(fmt, 1, 3); }

bool upb_fielddef_checkdescriptortype(int32_t type) {
  return between(type, 1, 18);
}

/* upb_msgdef *****************************************************************/

static void visitmsg(const upb_refcounted *r, upb_refcounted_visit *visit,
                     void *closure) {
  const upb_msgdef *m = (const upb_msgdef*)r;
  upb_msg_iter i;
  for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    visit(r, UPB_UPCAST2(f), closure);
  }
}

static void freemsg(upb_refcounted *r) {
  upb_msgdef *m = (upb_msgdef*)r;
  upb_strtable_uninit(&m->ntof);
  upb_inttable_uninit(&m->itof);
  upb_def_uninit(UPB_UPCAST(m));
  free(m);
}

upb_msgdef *upb_msgdef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {visitmsg, freemsg};
  upb_msgdef *m = malloc(sizeof(*m));
  if (!m) return NULL;
  if (!upb_def_init(UPB_UPCAST(m), UPB_DEF_MSG, &vtbl, owner)) goto err2;
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
  bool ok = upb_def_setfullname(UPB_UPCAST(newm),
                                upb_def_fullname(UPB_UPCAST(m)), NULL);
  UPB_ASSERT_VAR(ok, ok);
  upb_msg_iter i;
  for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_fielddef_dup(upb_msg_iter_field(&i), &f);
    if (!f || !upb_msgdef_addfield(newm, f, &f, NULL)) {
      upb_msgdef_unref(newm, owner);
      return NULL;
    }
  }
  return newm;
}

bool upb_msgdef_isfrozen(const upb_msgdef *m) {
  return upb_def_isfrozen(UPB_UPCAST(m));
}

void upb_msgdef_ref(const upb_msgdef *m, const void *owner) {
  upb_def_ref(UPB_UPCAST(m), owner);
}

void upb_msgdef_unref(const upb_msgdef *m, const void *owner) {
  upb_def_unref(UPB_UPCAST(m), owner);
}

void upb_msgdef_donateref(
    const upb_msgdef *m, const void *from, const void *to) {
  upb_def_donateref(UPB_UPCAST(m), from, to);
}

void upb_msgdef_checkref(const upb_msgdef *m, const void *owner) {
  upb_def_checkref(UPB_UPCAST(m), owner);
}

const char *upb_msgdef_fullname(const upb_msgdef *m) {
  return upb_def_fullname(UPB_UPCAST(m));
}

bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname,
                            upb_status *s) {
  return upb_def_setfullname(UPB_UPCAST(m), fullname, s);
}

bool upb_msgdef_addfields(upb_msgdef *m, upb_fielddef *const *fields, int n,
                          const void *ref_donor, upb_status *s) {
  // Check constraints for all fields before performing any action.
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = fields[i];
    // TODO(haberman): handle the case where two fields of the input duplicate
    // name or number.
    if (f->msgdef != NULL) {
      upb_status_seterrliteral(s, "fielddef already belongs to a message");
      return false;
    } else if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
      upb_status_seterrliteral(s, "field name or number were not set");
      return false;
    } else if(upb_msgdef_itof(m, upb_fielddef_number(f)) ||
              upb_msgdef_ntof(m, upb_fielddef_name(f))) {
      upb_status_seterrliteral(s, "duplicate field name or number");
      return false;
    }
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

bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f, const void *ref_donor,
                         upb_status *s) {
  return upb_msgdef_addfields(m, &f, 1, ref_donor, s);
}

const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  upb_value val;
  return upb_inttable_lookup32(&m->itof, i, &val) ?
      upb_value_getptr(val) : NULL;
}

const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name) {
  upb_value val;
  return upb_strtable_lookup(&m->ntof, name, &val) ?
      upb_value_getptr(val) : NULL;
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

bool upb_msg_done(const upb_msg_iter *iter) { return upb_inttable_done(iter); }

upb_fielddef *upb_msg_iter_field(const upb_msg_iter *iter) {
  return (upb_fielddef*)upb_value_getptr(upb_inttable_iter_value(iter));
}
