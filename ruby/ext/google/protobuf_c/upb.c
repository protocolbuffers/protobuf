// Amalgamated source file
#include "upb.h"


#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t len;
  char str[1];  /* Null-terminated string data follows. */
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

/* isalpha() etc. from <ctype.h> are locale-dependent, which we don't want. */
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
  size_t i;
  for (i = 0; i < len; i++) {
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
      return upb_msgdef_upcast_mutable(
          upb_msgdef_dup(upb_downcast_msgdef(def), o));
    case UPB_DEF_FIELD:
      return upb_fielddef_upcast_mutable(
          upb_fielddef_dup(upb_downcast_fielddef(def), o));
    case UPB_DEF_ENUM:
      return upb_enumdef_upcast_mutable(
          upb_enumdef_dup(upb_downcast_enumdef(def), o));
    default: assert(false); return NULL;
  }
}

static bool upb_def_init(upb_def *def, upb_deftype_t type,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
  if (!upb_refcounted_init(upb_def_upcast_mutable(def), vtbl, owner)) return false;
  def->type = type;
  def->fullname = NULL;
  def->came_from_user = false;
  return true;
}

static void upb_def_uninit(upb_def *def) {
  free((void*)def->fullname);
}

static const char *msgdef_name(const upb_msgdef *m) {
  const char *name = upb_def_fullname(upb_msgdef_upcast(m));
  return name ? name : "(anonymous)";
}

static bool upb_validate_field(upb_fielddef *f, upb_status *s) {
  if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
    upb_status_seterrmsg(s, "fielddef must have name and number set");
    return false;
  }

  if (!f->type_is_set_) {
    upb_status_seterrmsg(s, "fielddef type was not initialized");
    return false;
  }

  if (upb_fielddef_lazy(f) &&
      upb_fielddef_descriptortype(f) != UPB_DESCRIPTOR_TYPE_MESSAGE) {
    upb_status_seterrmsg(s,
                         "only length-delimited submessage fields may be lazy");
    return false;
  }

  if (upb_fielddef_hassubdef(f)) {
    const upb_def *subdef;

    if (f->subdef_is_symbolic) {
      upb_status_seterrf(s, "field '%s.%s' has not been resolved",
                         msgdef_name(f->msg.def), upb_fielddef_name(f));
      return false;
    }

    subdef = upb_fielddef_subdef(f);
    if (subdef == NULL) {
      upb_status_seterrf(s, "field %s.%s is missing required subdef",
                         msgdef_name(f->msg.def), upb_fielddef_name(f));
      return false;
    }

    if (!upb_def_isfrozen(subdef) && !subdef->came_from_user) {
      upb_status_seterrf(s,
                         "subdef of field %s.%s is not frozen or being frozen",
                         msgdef_name(f->msg.def), upb_fielddef_name(f));
      return false;
    }
  }

  if (upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    bool has_default_name = upb_fielddef_enumhasdefaultstr(f);
    bool has_default_number = upb_fielddef_enumhasdefaultint32(f);

    /* Previously verified by upb_validate_enumdef(). */
    assert(upb_enumdef_numvals(upb_fielddef_enumsubdef(f)) > 0);

    /* We've already validated that we have an associated enumdef and that it
     * has at least one member, so at least one of these should be true.
     * Because if the user didn't set anything, we'll pick up the enum's
     * default, but if the user *did* set something we should at least pick up
     * the one they set (int32 or string). */
    assert(has_default_name || has_default_number);

    if (!has_default_name) {
      upb_status_seterrf(s,
                         "enum default for field %s.%s (%d) is not in the enum",
                         msgdef_name(f->msg.def), upb_fielddef_name(f),
                         upb_fielddef_defaultint32(f));
      return false;
    }

    if (!has_default_number) {
      upb_status_seterrf(s,
                         "enum default for field %s.%s (%s) is not in the enum",
                         msgdef_name(f->msg.def), upb_fielddef_name(f),
                         upb_fielddef_defaultstr(f, NULL));
      return false;
    }

    /* Lift the effective numeric default into the field's default slot, in case
     * we were only getting it "by reference" from the enumdef. */
    upb_fielddef_setdefaultint32(f, upb_fielddef_defaultint32(f));
  }

  /* Ensure that MapEntry submessages only appear as repeated fields, not
   * optional/required (singular) fields. */
  if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE &&
      upb_fielddef_msgsubdef(f) != NULL) {
    const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
    if (upb_msgdef_mapentry(subdef) && !upb_fielddef_isseq(f)) {
      upb_status_seterrf(s,
                         "Field %s refers to mapentry message but is not "
                         "a repeated field",
                         upb_fielddef_name(f) ? upb_fielddef_name(f) :
                         "(unnamed)");
      return false;
    }
  }

  return true;
}

static bool upb_validate_enumdef(const upb_enumdef *e, upb_status *s) {
  if (upb_enumdef_numvals(e) == 0) {
    upb_status_seterrf(s, "enum %s has no members (must have at least one)",
                       upb_enumdef_fullname(e));
    return false;
  }

  return true;
}

/* All submessage fields are lower than all other fields.
 * Secondly, fields are increasing in order. */
uint32_t field_rank(const upb_fielddef *f) {
  uint32_t ret = upb_fielddef_number(f);
  const uint32_t high_bit = 1 << 30;
  assert(ret < high_bit);
  if (!upb_fielddef_issubmsg(f))
    ret |= high_bit;
  return ret;
}

int cmp_fields(const void *p1, const void *p2) {
  const upb_fielddef *f1 = *(upb_fielddef*const*)p1;
  const upb_fielddef *f2 = *(upb_fielddef*const*)p2;
  return field_rank(f1) - field_rank(f2);
}

static bool assign_msg_indices(upb_msgdef *m, upb_status *s) {
  /* Sort fields.  upb internally relies on UPB_TYPE_MESSAGE fields having the
   * lowest indexes, but we do not publicly guarantee this. */
  upb_msg_field_iter j;
  int i;
  uint32_t selector;
  int n = upb_msgdef_numfields(m);
  upb_fielddef **fields = malloc(n * sizeof(*fields));
  if (!fields) return false;

  m->submsg_field_count = 0;
  for(i = 0, upb_msg_field_begin(&j, m);
      !upb_msg_field_done(&j);
      upb_msg_field_next(&j), i++) {
    upb_fielddef *f = upb_msg_iter_field(&j);
    assert(f->msg.def == m);
    if (!upb_validate_field(f, s)) {
      free(fields);
      return false;
    }
    if (upb_fielddef_issubmsg(f)) {
      m->submsg_field_count++;
    }
    fields[i] = f;
  }

  qsort(fields, n, sizeof(*fields), cmp_fields);

  selector = UPB_STATIC_SELECTOR_COUNT + m->submsg_field_count;
  for (i = 0; i < n; i++) {
    upb_fielddef *f = fields[i];
    f->index_ = i;
    f->selector_base = selector + upb_handlers_selectorbaseoffset(f);
    selector += upb_handlers_selectorcount(f);
  }
  m->selector_count = selector;

#ifndef NDEBUG
  {
    /* Verify that all selectors for the message are distinct. */
#define TRY(type) \
    if (upb_handlers_getselector(f, type, &sel)) upb_inttable_insert(&t, sel, v);

    upb_inttable t;
    upb_value v;
    upb_selector_t sel;

    upb_inttable_init(&t, UPB_CTYPE_BOOL);
    v = upb_value_bool(true);
    upb_inttable_insert(&t, UPB_STARTMSG_SELECTOR, v);
    upb_inttable_insert(&t, UPB_ENDMSG_SELECTOR, v);
    for(upb_msg_field_begin(&j, m);
        !upb_msg_field_done(&j);
        upb_msg_field_next(&j)) {
      upb_fielddef *f = upb_msg_iter_field(&j);
      /* These calls will assert-fail in upb_table if the value already
       * exists. */
      TRY(UPB_HANDLER_INT32);
      TRY(UPB_HANDLER_INT64)
      TRY(UPB_HANDLER_UINT32)
      TRY(UPB_HANDLER_UINT64)
      TRY(UPB_HANDLER_FLOAT)
      TRY(UPB_HANDLER_DOUBLE)
      TRY(UPB_HANDLER_BOOL)
      TRY(UPB_HANDLER_STARTSTR)
      TRY(UPB_HANDLER_STRING)
      TRY(UPB_HANDLER_ENDSTR)
      TRY(UPB_HANDLER_STARTSUBMSG)
      TRY(UPB_HANDLER_ENDSUBMSG)
      TRY(UPB_HANDLER_STARTSEQ)
      TRY(UPB_HANDLER_ENDSEQ)
    }
    upb_inttable_uninit(&t);
  }
#undef TRY
#endif

  free(fields);
  return true;
}

bool upb_def_freeze(upb_def *const* defs, int n, upb_status *s) {
  int i;
  int maxdepth;
  bool ret;
  upb_status_clear(s);

  /* First perform validation, in two passes so we can check that we have a
   * transitive closure without needing to search. */
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    if (upb_def_isfrozen(def)) {
      /* Could relax this requirement if it's annoying. */
      upb_status_seterrmsg(s, "def is already frozen");
      goto err;
    } else if (def->type == UPB_DEF_FIELD) {
      upb_status_seterrmsg(s, "standalone fielddefs can not be frozen");
      goto err;
    } else if (def->type == UPB_DEF_ENUM) {
      if (!upb_validate_enumdef(upb_dyncast_enumdef(def), s)) {
        goto err;
      }
    } else {
      /* Set now to detect transitive closure in the second pass. */
      def->came_from_user = true;
    }
  }

  /* Second pass of validation.  Also assign selector bases and indexes, and
   * compact tables. */
  for (i = 0; i < n; i++) {
    upb_msgdef *m = upb_dyncast_msgdef_mutable(defs[i]);
    upb_enumdef *e = upb_dyncast_enumdef_mutable(defs[i]);
    if (m) {
      upb_inttable_compact(&m->itof);
      if (!assign_msg_indices(m, s)) {
        goto err;
      }
    } else if (e) {
      upb_inttable_compact(&e->iton);
    }
  }

  /* Def graph contains FieldDefs between each MessageDef, so double the
   * limit. */
  maxdepth = UPB_MAX_MESSAGE_DEPTH * 2;

  /* Validation all passed; freeze the defs. */
  ret = upb_refcounted_freeze((upb_refcounted * const *)defs, n, s, maxdepth);
  assert(!(s && ret != upb_ok(s)));
  return ret;

err:
  for (i = 0; i < n; i++) {
    defs[i]->came_from_user = false;
  }
  assert(!(s && upb_ok(s)));
  return false;
}


/* upb_enumdef ****************************************************************/

static void upb_enumdef_free(upb_refcounted *r) {
  upb_enumdef *e = (upb_enumdef*)r;
  upb_inttable_iter i;
  upb_inttable_begin(&i, &e->iton);
  for( ; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    /* To clean up the upb_strdup() from upb_enumdef_addval(). */
    free(upb_value_getcstr(upb_inttable_iter_value(&i)));
  }
  upb_strtable_uninit(&e->ntoi);
  upb_inttable_uninit(&e->iton);
  upb_def_uninit(upb_enumdef_upcast_mutable(e));
  free(e);
}

upb_enumdef *upb_enumdef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {NULL, &upb_enumdef_free};
  upb_enumdef *e = malloc(sizeof(*e));
  if (!e) return NULL;
  if (!upb_def_init(upb_enumdef_upcast_mutable(e), UPB_DEF_ENUM, &vtbl, owner))
    goto err2;
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
  upb_enum_iter i;
  upb_enumdef *new_e = upb_enumdef_new(owner);
  if (!new_e) return NULL;
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

bool upb_enumdef_freeze(upb_enumdef *e, upb_status *status) {
  upb_def *d = upb_enumdef_upcast_mutable(e);
  return upb_def_freeze(&d, 1, status);
}

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return upb_def_fullname(upb_enumdef_upcast(e));
}

bool upb_enumdef_setfullname(upb_enumdef *e, const char *fullname,
                             upb_status *s) {
  return upb_def_setfullname(upb_enumdef_upcast_mutable(e), fullname, s);
}

bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num,
                        upb_status *status) {
  if (!upb_isident(name, strlen(name), false, status)) {
    return false;
  }
  if (upb_enumdef_ntoiz(e, name, NULL)) {
    upb_status_seterrf(status, "name '%s' is already defined", name);
    return false;
  }
  if (!upb_strtable_insert(&e->ntoi, name, upb_value_int32(num))) {
    upb_status_seterrmsg(status, "out of memory");
    return false;
  }
  if (!upb_inttable_lookup(&e->iton, num, NULL) &&
      !upb_inttable_insert(&e->iton, num, upb_value_cstr(upb_strdup(name)))) {
    upb_status_seterrmsg(status, "out of memory");
    upb_strtable_remove(&e->ntoi, name, NULL);
    return false;
  }
  if (upb_enumdef_numvals(e) == 1) {
    bool ok = upb_enumdef_setdefault(e, num, NULL);
    UPB_ASSERT_VAR(ok, ok);
  }
  return true;
}

int32_t upb_enumdef_default(const upb_enumdef *e) {
  assert(upb_enumdef_iton(e, e->defaultval));
  return e->defaultval;
}

bool upb_enumdef_setdefault(upb_enumdef *e, int32_t val, upb_status *s) {
  assert(!upb_enumdef_isfrozen(e));
  if (!upb_enumdef_iton(e, val)) {
    upb_status_seterrf(s, "number '%d' is not in the enum.", val);
    return false;
  }
  e->defaultval = val;
  return true;
}

int upb_enumdef_numvals(const upb_enumdef *e) {
  return upb_strtable_count(&e->ntoi);
}

void upb_enum_begin(upb_enum_iter *i, const upb_enumdef *e) {
  /* We iterate over the ntoi table, to account for duplicate numbers. */
  upb_strtable_begin(i, &e->ntoi);
}

void upb_enum_next(upb_enum_iter *iter) { upb_strtable_next(iter); }
bool upb_enum_done(upb_enum_iter *iter) { return upb_strtable_done(iter); }

bool upb_enumdef_ntoi(const upb_enumdef *def, const char *name,
                      size_t len, int32_t *num) {
  upb_value v;
  if (!upb_strtable_lookup2(&def->ntoi, name, len, &v)) {
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
  if (f->type_is_set_ && f->default_is_string && f->defaultval.bytes)
    freestr(f->defaultval.bytes);
}

static void visitfield(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const upb_fielddef *f = (const upb_fielddef*)r;
  if (upb_fielddef_containingtype(f)) {
    visit(r, upb_msgdef_upcast2(upb_fielddef_containingtype(f)), closure);
  }
  if (upb_fielddef_containingoneof(f)) {
    visit(r, upb_oneofdef_upcast2(upb_fielddef_containingoneof(f)), closure);
  }
  if (upb_fielddef_subdef(f)) {
    visit(r, upb_def_upcast(upb_fielddef_subdef(f)), closure);
  }
}

static void freefield(upb_refcounted *r) {
  upb_fielddef *f = (upb_fielddef*)r;
  upb_fielddef_uninit_default(f);
  if (f->subdef_is_symbolic)
    free(f->sub.name);
  upb_def_uninit(upb_fielddef_upcast_mutable(f));
  free(f);
}

static const char *enumdefaultstr(const upb_fielddef *f) {
  const upb_enumdef *e;
  assert(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  e = upb_fielddef_enumsubdef(f);
  if (f->default_is_string && f->defaultval.bytes) {
    /* Default was explicitly set as a string. */
    str_t *s = f->defaultval.bytes;
    return s->str;
  } else if (e) {
    if (!f->default_is_string) {
      /* Default was explicitly set as an integer; look it up in enumdef. */
      const char *name = upb_enumdef_iton(e, f->defaultval.sint);
      if (name) {
        return name;
      }
    } else {
      /* Default is completely unset; pull enumdef default. */
      if (upb_enumdef_numvals(e) > 0) {
        const char *name = upb_enumdef_iton(e, upb_enumdef_default(e));
        assert(name);
        return name;
      }
    }
  }
  return NULL;
}

static bool enumdefaultint32(const upb_fielddef *f, int32_t *val) {
  const upb_enumdef *e;
  assert(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  e = upb_fielddef_enumsubdef(f);
  if (!f->default_is_string) {
    /* Default was explicitly set as an integer. */
    *val = f->defaultval.sint;
    return true;
  } else if (e) {
    if (f->defaultval.bytes) {
      /* Default was explicitly set as a str; try to lookup corresponding int. */
      str_t *s = f->defaultval.bytes;
      if (upb_enumdef_ntoiz(e, s->str, val)) {
        return true;
      }
    } else {
      /* Default is unset; try to pull in enumdef default. */
      if (upb_enumdef_numvals(e) > 0) {
        *val = upb_enumdef_default(e);
        return true;
      }
    }
  }
  return false;
}

upb_fielddef *upb_fielddef_new(const void *o) {
  static const struct upb_refcounted_vtbl vtbl = {visitfield, freefield};
  upb_fielddef *f = malloc(sizeof(*f));
  if (!f) return NULL;
  if (!upb_def_init(upb_fielddef_upcast_mutable(f), UPB_DEF_FIELD, &vtbl, o)) {
    free(f);
    return NULL;
  }
  f->msg.def = NULL;
  f->sub.def = NULL;
  f->oneof = NULL;
  f->subdef_is_symbolic = false;
  f->msg_is_symbolic = false;
  f->label_ = UPB_LABEL_OPTIONAL;
  f->type_ = UPB_TYPE_INT32;
  f->number_ = 0;
  f->type_is_set_ = false;
  f->tagdelim = false;
  f->is_extension_ = false;
  f->lazy_ = false;
  f->packed_ = true;

  /* For the moment we default this to UPB_INTFMT_VARIABLE, since it will work
   * with all integer types and is in some since more "default" since the most
   * normal-looking proto2 types int32/int64/uint32/uint64 use variable.
   *
   * Other options to consider:
   * - there is no default; users must set this manually (like type).
   * - default signed integers to UPB_INTFMT_ZIGZAG, since it's more likely to
   *   be an optimal default for signed integers. */
  f->intfmt = UPB_INTFMT_VARIABLE;
  return f;
}

upb_fielddef *upb_fielddef_dup(const upb_fielddef *f, const void *owner) {
  const char *srcname;
  upb_fielddef *newf = upb_fielddef_new(owner);
  if (!newf) return NULL;
  upb_fielddef_settype(newf, upb_fielddef_type(f));
  upb_fielddef_setlabel(newf, upb_fielddef_label(f));
  upb_fielddef_setnumber(newf, upb_fielddef_number(f), NULL);
  upb_fielddef_setname(newf, upb_fielddef_name(f), NULL);
  if (f->default_is_string && f->defaultval.bytes) {
    str_t *s = f->defaultval.bytes;
    upb_fielddef_setdefaultstr(newf, s->str, s->len, NULL);
  } else {
    newf->default_is_string = f->default_is_string;
    newf->defaultval = f->defaultval;
  }

  if (f->subdef_is_symbolic) {
    srcname = f->sub.name;  /* Might be NULL. */
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

bool upb_fielddef_typeisset(const upb_fielddef *f) {
  return f->type_is_set_;
}

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
  assert(f->type_is_set_);
  return f->type_;
}

uint32_t upb_fielddef_index(const upb_fielddef *f) {
  return f->index_;
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

bool upb_fielddef_isextension(const upb_fielddef *f) {
  return f->is_extension_;
}

bool upb_fielddef_lazy(const upb_fielddef *f) {
  return f->lazy_;
}

bool upb_fielddef_packed(const upb_fielddef *f) {
  return f->packed_;
}

const char *upb_fielddef_name(const upb_fielddef *f) {
  return upb_def_fullname(upb_fielddef_upcast(f));
}

const upb_msgdef *upb_fielddef_containingtype(const upb_fielddef *f) {
  return f->msg_is_symbolic ? NULL : f->msg.def;
}

const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f) {
  return f->oneof;
}

upb_msgdef *upb_fielddef_containingtype_mutable(upb_fielddef *f) {
  return (upb_msgdef*)upb_fielddef_containingtype(f);
}

const char *upb_fielddef_containingtypename(upb_fielddef *f) {
  return f->msg_is_symbolic ? f->msg.name : NULL;
}

static void release_containingtype(upb_fielddef *f) {
  if (f->msg_is_symbolic) free(f->msg.name);
}

bool upb_fielddef_setcontainingtypename(upb_fielddef *f, const char *name,
                                        upb_status *s) {
  assert(!upb_fielddef_isfrozen(f));
  if (upb_fielddef_containingtype(f)) {
    upb_status_seterrmsg(s, "field has already been added to a message.");
    return false;
  }
  /* TODO: validate name (upb_isident() doesn't quite work atm because this name
   * may have a leading "."). */
  release_containingtype(f);
  f->msg.name = upb_strdup(name);
  f->msg_is_symbolic = true;
  return true;
}

bool upb_fielddef_setname(upb_fielddef *f, const char *name, upb_status *s) {
  if (upb_fielddef_containingtype(f) || upb_fielddef_containingoneof(f)) {
    upb_status_seterrmsg(s, "Already added to message or oneof");
    return false;
  }
  return upb_def_setfullname(upb_fielddef_upcast_mutable(f), name, s);
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
  if (f->type_is_set_ && upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    int32_t val;
    bool ok = enumdefaultint32(f, &val);
    UPB_ASSERT_VAR(ok, ok);
    return val;
  } else {
    chkdefaulttype(f, UPB_TYPE_INT32);
    return f->defaultval.sint;
  }
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

  if (upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    const char *ret = enumdefaultstr(f);
    assert(ret);
    /* Enum defaults can't have embedded NULLs. */
    if (len) *len = strlen(ret);
    return ret;
  }

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
    case UPB_TYPE_INT64: f->defaultval.sint = 0; break;
    case UPB_TYPE_UINT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_BOOL: f->defaultval.uint = 0; break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      f->defaultval.bytes = newstr("", 0);
      f->default_is_string = true;
      break;
    case UPB_TYPE_MESSAGE: break;
    case UPB_TYPE_ENUM:
      /* This is our special sentinel that indicates "not set" for an enum. */
      f->default_is_string = true;
      f->defaultval.bytes = NULL;
      break;
  }
}

const upb_def *upb_fielddef_subdef(const upb_fielddef *f) {
  return f->subdef_is_symbolic ? NULL : f->sub.def;
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
  if (f->subdef_is_symbolic) {
    return f->sub.name;
  } else if (f->sub.def) {
    return upb_def_fullname(f->sub.def);
  } else {
    return NULL;
  }
}

bool upb_fielddef_setnumber(upb_fielddef *f, uint32_t number, upb_status *s) {
  if (upb_fielddef_containingtype(f)) {
    upb_status_seterrmsg(
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

void upb_fielddef_setisextension(upb_fielddef *f, bool is_extension) {
  assert(!upb_fielddef_isfrozen(f));
  f->is_extension_ = is_extension;
}

void upb_fielddef_setlazy(upb_fielddef *f, bool lazy) {
  assert(!upb_fielddef_isfrozen(f));
  f->lazy_ = lazy;
}

void upb_fielddef_setpacked(upb_fielddef *f, bool packed) {
  assert(!upb_fielddef_isfrozen(f));
  f->packed_ = packed;
}

void upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label) {
  assert(!upb_fielddef_isfrozen(f));
  assert(upb_fielddef_checklabel(label));
  f->label_ = label;
}

void upb_fielddef_setintfmt(upb_fielddef *f, upb_intfmt_t fmt) {
  assert(!upb_fielddef_isfrozen(f));
  assert(upb_fielddef_checkintfmt(fmt));
  f->intfmt = fmt;
}

void upb_fielddef_settagdelim(upb_fielddef *f, bool tag_delim) {
  assert(!upb_fielddef_isfrozen(f));
  f->tagdelim = tag_delim;
  f->tagdelim = tag_delim;
}

static bool checksetdefault(upb_fielddef *f, upb_fieldtype_t type) {
  if (!f->type_is_set_ || upb_fielddef_isfrozen(f) ||
      upb_fielddef_type(f) != type) {
    assert(false);
    return false;
  }
  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    assert(s || type == UPB_TYPE_ENUM);
    if (s) freestr(s);
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
  str_t *str2;
  assert(upb_fielddef_isstring(f) || f->type_ == UPB_TYPE_ENUM);
  if (f->type_ == UPB_TYPE_ENUM && !upb_isident(str, len, false, s))
    return false;

  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    assert(s || f->type_ == UPB_TYPE_ENUM);
    if (s) freestr(s);
  } else {
    assert(f->type_ == UPB_TYPE_ENUM);
  }

  str2 = newstr(str, len);
  f->defaultval.bytes = str2;
  f->default_is_string = true;
  return true;
}

void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str,
                                 upb_status *s) {
  assert(f->type_is_set_);
  upb_fielddef_setdefaultstr(f, str, str ? strlen(str) : 0, s);
}

bool upb_fielddef_enumhasdefaultint32(const upb_fielddef *f) {
  int32_t val;
  assert(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  return enumdefaultint32(f, &val);
}

bool upb_fielddef_enumhasdefaultstr(const upb_fielddef *f) {
  assert(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  return enumdefaultstr(f) != NULL;
}

static bool upb_subdef_typecheck(upb_fielddef *f, const upb_def *subdef,
                                 upb_status *s) {
  if (f->type_ == UPB_TYPE_MESSAGE) {
    if (upb_dyncast_msgdef(subdef)) return true;
    upb_status_seterrmsg(s, "invalid subdef type for this submessage field");
    return false;
  } else if (f->type_ == UPB_TYPE_ENUM) {
    if (upb_dyncast_enumdef(subdef)) return true;
    upb_status_seterrmsg(s, "invalid subdef type for this enum field");
    return false;
  } else {
    upb_status_seterrmsg(s, "only message and enum fields can have a subdef");
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
  return upb_fielddef_setsubdef(f, upb_msgdef_upcast(subdef), s);
}

bool upb_fielddef_setenumsubdef(upb_fielddef *f, const upb_enumdef *subdef,
                                upb_status *s) {
  return upb_fielddef_setsubdef(f, upb_enumdef_upcast(subdef), s);
}

bool upb_fielddef_setsubdefname(upb_fielddef *f, const char *name,
                                upb_status *s) {
  assert(!upb_fielddef_isfrozen(f));
  if (!upb_fielddef_hassubdef(f)) {
    upb_status_seterrmsg(s, "field type does not accept a subdef");
    return false;
  }
  /* TODO: validate name (upb_isident() doesn't quite work atm because this name
   * may have a leading "."). */
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

bool upb_fielddef_ismap(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) && upb_fielddef_issubmsg(f) &&
         upb_msgdef_mapentry(upb_fielddef_msgsubdef(f));
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
  upb_msg_oneof_iter o;
  const upb_msgdef *m = (const upb_msgdef*)r;
  upb_msg_field_iter i;
  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    visit(r, upb_fielddef_upcast2(f), closure);
  }
  for(upb_msg_oneof_begin(&o, m);
      !upb_msg_oneof_done(&o);
      upb_msg_oneof_next(&o)) {
    upb_oneofdef *f = upb_msg_iter_oneof(&o);
    visit(r, upb_oneofdef_upcast2(f), closure);
  }
}

static void freemsg(upb_refcounted *r) {
  upb_msgdef *m = (upb_msgdef*)r;
  upb_strtable_uninit(&m->ntoo);
  upb_strtable_uninit(&m->ntof);
  upb_inttable_uninit(&m->itof);
  upb_def_uninit(upb_msgdef_upcast_mutable(m));
  free(m);
}

upb_msgdef *upb_msgdef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {visitmsg, freemsg};
  upb_msgdef *m = malloc(sizeof(*m));
  if (!m) return NULL;
  if (!upb_def_init(upb_msgdef_upcast_mutable(m), UPB_DEF_MSG, &vtbl, owner))
    goto err2;
  if (!upb_inttable_init(&m->itof, UPB_CTYPE_PTR)) goto err3;
  if (!upb_strtable_init(&m->ntof, UPB_CTYPE_PTR)) goto err2;
  if (!upb_strtable_init(&m->ntoo, UPB_CTYPE_PTR)) goto err1;
  m->map_entry = false;
  return m;

err1:
  upb_strtable_uninit(&m->ntof);
err2:
  upb_inttable_uninit(&m->itof);
err3:
  free(m);
  return NULL;
}

upb_msgdef *upb_msgdef_dup(const upb_msgdef *m, const void *owner) {
  bool ok;
  upb_msg_field_iter i;
  upb_msg_oneof_iter o;

  upb_msgdef *newm = upb_msgdef_new(owner);
  if (!newm) return NULL;
  ok = upb_def_setfullname(upb_msgdef_upcast_mutable(newm),
                           upb_def_fullname(upb_msgdef_upcast(m)),
                           NULL);
  newm->map_entry = m->map_entry;
  UPB_ASSERT_VAR(ok, ok);
  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_fielddef_dup(upb_msg_iter_field(&i), &f);
    /* Fields in oneofs are dup'd below. */
    if (upb_fielddef_containingoneof(f)) continue;
    if (!f || !upb_msgdef_addfield(newm, f, &f, NULL)) {
      upb_msgdef_unref(newm, owner);
      return NULL;
    }
  }
  for(upb_msg_oneof_begin(&o, m);
      !upb_msg_oneof_done(&o);
      upb_msg_oneof_next(&o)) {
    upb_oneofdef *f = upb_oneofdef_dup(upb_msg_iter_oneof(&o), &f);
    if (!f || !upb_msgdef_addoneof(newm, f, &f, NULL)) {
      upb_msgdef_unref(newm, owner);
      return NULL;
    }
  }
  return newm;
}

bool upb_msgdef_freeze(upb_msgdef *m, upb_status *status) {
  upb_def *d = upb_msgdef_upcast_mutable(m);
  return upb_def_freeze(&d, 1, status);
}

const char *upb_msgdef_fullname(const upb_msgdef *m) {
  return upb_def_fullname(upb_msgdef_upcast(m));
}

bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname,
                            upb_status *s) {
  return upb_def_setfullname(upb_msgdef_upcast_mutable(m), fullname, s);
}

/* Helper: check that the field |f| is safe to add to msgdef |m|. Set an error
 * on status |s| and return false if not. */
static bool check_field_add(const upb_msgdef *m, const upb_fielddef *f,
                            upb_status *s) {
  if (upb_fielddef_containingtype(f) != NULL) {
    upb_status_seterrmsg(s, "fielddef already belongs to a message");
    return false;
  } else if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
    upb_status_seterrmsg(s, "field name or number were not set");
    return false;
  } else if (upb_msgdef_ntofz(m, upb_fielddef_name(f)) ||
             upb_msgdef_itof(m, upb_fielddef_number(f))) {
    upb_status_seterrmsg(s, "duplicate field name or number for field");
    return false;
  }
  return true;
}

static void add_field(upb_msgdef *m, upb_fielddef *f, const void *ref_donor) {
  release_containingtype(f);
  f->msg.def = m;
  f->msg_is_symbolic = false;
  upb_inttable_insert(&m->itof, upb_fielddef_number(f), upb_value_ptr(f));
  upb_strtable_insert(&m->ntof, upb_fielddef_name(f), upb_value_ptr(f));
  upb_ref2(f, m);
  upb_ref2(m, f);
  if (ref_donor) upb_fielddef_unref(f, ref_donor);
}

bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f, const void *ref_donor,
                         upb_status *s) {
  /* TODO: extensions need to have a separate namespace, because proto2 allows a
   * top-level extension (ie. one not in any package) to have the same name as a
   * field from the message.
   *
   * This also implies that there needs to be a separate lookup-by-name method
   * for extensions.  It seems desirable for iteration to return both extensions
   * and non-extensions though.
   *
   * We also need to validate that the field number is in an extension range iff
   * it is an extension.
   *
   * This method is idempotent. Check if |f| is already part of this msgdef and
   * return immediately if so. */
  if (upb_fielddef_containingtype(f) == m) {
    return true;
  }

  /* Check constraints for all fields before performing any action. */
  if (!check_field_add(m, f, s)) {
    return false;
  } else if (upb_fielddef_containingoneof(f) != NULL) {
    /* Fields in a oneof can only be added by adding the oneof to the msgdef. */
    upb_status_seterrmsg(s, "fielddef is part of a oneof");
    return false;
  }

  /* Constraint checks ok, perform the action. */
  add_field(m, f, ref_donor);
  return true;
}

bool upb_msgdef_addoneof(upb_msgdef *m, upb_oneofdef *o, const void *ref_donor,
                         upb_status *s) {
  upb_oneof_iter it;

  /* Check various conditions that would prevent this oneof from being added. */
  if (upb_oneofdef_containingtype(o)) {
    upb_status_seterrmsg(s, "oneofdef already belongs to a message");
    return false;
  } else if (upb_oneofdef_name(o) == NULL) {
    upb_status_seterrmsg(s, "oneofdef name was not set");
    return false;
  } else if (upb_msgdef_ntooz(m, upb_oneofdef_name(o))) {
    upb_status_seterrmsg(s, "duplicate oneof name");
    return false;
  }

  /* Check that all of the oneof's fields do not conflict with names or numbers
   * of fields already in the message. */
  for (upb_oneof_begin(&it, o); !upb_oneof_done(&it); upb_oneof_next(&it)) {
    const upb_fielddef *f = upb_oneof_iter_field(&it);
    if (!check_field_add(m, f, s)) {
      return false;
    }
  }

  /* Everything checks out -- commit now. */

  /* Add oneof itself first. */
  o->parent = m;
  upb_strtable_insert(&m->ntoo, upb_oneofdef_name(o), upb_value_ptr(o));
  upb_ref2(o, m);
  upb_ref2(m, o);

  /* Add each field of the oneof directly to the msgdef. */
  for (upb_oneof_begin(&it, o); !upb_oneof_done(&it); upb_oneof_next(&it)) {
    upb_fielddef *f = upb_oneof_iter_field(&it);
    add_field(m, f, NULL);
  }

  if (ref_donor) upb_oneofdef_unref(o, ref_donor);

  return true;
}

const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  upb_value val;
  return upb_inttable_lookup32(&m->itof, i, &val) ?
      upb_value_getptr(val) : NULL;
}

const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;
  return upb_strtable_lookup2(&m->ntof, name, len, &val) ?
      upb_value_getptr(val) : NULL;
}

const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;
  return upb_strtable_lookup2(&m->ntoo, name, len, &val) ?
      upb_value_getptr(val) : NULL;
}

int upb_msgdef_numfields(const upb_msgdef *m) {
  return upb_strtable_count(&m->ntof);
}

int upb_msgdef_numoneofs(const upb_msgdef *m) {
  return upb_strtable_count(&m->ntoo);
}

void upb_msgdef_setmapentry(upb_msgdef *m, bool map_entry) {
  assert(!upb_msgdef_isfrozen(m));
  m->map_entry = map_entry;
}

bool upb_msgdef_mapentry(const upb_msgdef *m) {
  return m->map_entry;
}

void upb_msg_field_begin(upb_msg_field_iter *iter, const upb_msgdef *m) {
  upb_inttable_begin(iter, &m->itof);
}

void upb_msg_field_next(upb_msg_field_iter *iter) { upb_inttable_next(iter); }

bool upb_msg_field_done(const upb_msg_field_iter *iter) {
  return upb_inttable_done(iter);
}

upb_fielddef *upb_msg_iter_field(const upb_msg_field_iter *iter) {
  return (upb_fielddef*)upb_value_getptr(upb_inttable_iter_value(iter));
}

void upb_msg_field_iter_setdone(upb_msg_field_iter *iter) {
  upb_inttable_iter_setdone(iter);
}

void upb_msg_oneof_begin(upb_msg_oneof_iter *iter, const upb_msgdef *m) {
  upb_strtable_begin(iter, &m->ntoo);
}

void upb_msg_oneof_next(upb_msg_oneof_iter *iter) { upb_strtable_next(iter); }

bool upb_msg_oneof_done(const upb_msg_oneof_iter *iter) {
  return upb_strtable_done(iter);
}

upb_oneofdef *upb_msg_iter_oneof(const upb_msg_oneof_iter *iter) {
  return (upb_oneofdef*)upb_value_getptr(upb_strtable_iter_value(iter));
}

void upb_msg_oneof_iter_setdone(upb_msg_oneof_iter *iter) {
  upb_strtable_iter_setdone(iter);
}

/* upb_oneofdef ***************************************************************/

static void visitoneof(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const upb_oneofdef *o = (const upb_oneofdef*)r;
  upb_oneof_iter i;
  for (upb_oneof_begin(&i, o); !upb_oneof_done(&i); upb_oneof_next(&i)) {
    const upb_fielddef *f = upb_oneof_iter_field(&i);
    visit(r, upb_fielddef_upcast2(f), closure);
  }
  if (o->parent) {
    visit(r, upb_msgdef_upcast2(o->parent), closure);
  }
}

static void freeoneof(upb_refcounted *r) {
  upb_oneofdef *o = (upb_oneofdef*)r;
  upb_strtable_uninit(&o->ntof);
  upb_inttable_uninit(&o->itof);
  upb_def_uninit(upb_oneofdef_upcast_mutable(o));
  free(o);
}

upb_oneofdef *upb_oneofdef_new(const void *owner) {
  static const struct upb_refcounted_vtbl vtbl = {visitoneof, freeoneof};
  upb_oneofdef *o = malloc(sizeof(*o));
  o->parent = NULL;
  if (!o) return NULL;
  if (!upb_def_init(upb_oneofdef_upcast_mutable(o), UPB_DEF_ONEOF, &vtbl,
                    owner))
    goto err2;
  if (!upb_inttable_init(&o->itof, UPB_CTYPE_PTR)) goto err2;
  if (!upb_strtable_init(&o->ntof, UPB_CTYPE_PTR)) goto err1;
  return o;

err1:
  upb_inttable_uninit(&o->itof);
err2:
  free(o);
  return NULL;
}

upb_oneofdef *upb_oneofdef_dup(const upb_oneofdef *o, const void *owner) {
  bool ok;
  upb_oneof_iter i;
  upb_oneofdef *newo = upb_oneofdef_new(owner);
  if (!newo) return NULL;
  ok = upb_def_setfullname(upb_oneofdef_upcast_mutable(newo),
                           upb_def_fullname(upb_oneofdef_upcast(o)), NULL);
  UPB_ASSERT_VAR(ok, ok);
  for (upb_oneof_begin(&i, o); !upb_oneof_done(&i); upb_oneof_next(&i)) {
    upb_fielddef *f = upb_fielddef_dup(upb_oneof_iter_field(&i), &f);
    if (!f || !upb_oneofdef_addfield(newo, f, &f, NULL)) {
      upb_oneofdef_unref(newo, owner);
      return NULL;
    }
  }
  return newo;
}

const char *upb_oneofdef_name(const upb_oneofdef *o) {
  return upb_def_fullname(upb_oneofdef_upcast(o));
}

bool upb_oneofdef_setname(upb_oneofdef *o, const char *fullname,
                             upb_status *s) {
  if (upb_oneofdef_containingtype(o)) {
    upb_status_seterrmsg(s, "oneof already added to a message");
    return false;
  }
  return upb_def_setfullname(upb_oneofdef_upcast_mutable(o), fullname, s);
}

const upb_msgdef *upb_oneofdef_containingtype(const upb_oneofdef *o) {
  return o->parent;
}

int upb_oneofdef_numfields(const upb_oneofdef *o) {
  return upb_strtable_count(&o->ntof);
}

bool upb_oneofdef_addfield(upb_oneofdef *o, upb_fielddef *f,
                           const void *ref_donor,
                           upb_status *s) {
  assert(!upb_oneofdef_isfrozen(o));
  assert(!o->parent || !upb_msgdef_isfrozen(o->parent));

  /* This method is idempotent. Check if |f| is already part of this oneofdef
   * and return immediately if so. */
  if (upb_fielddef_containingoneof(f) == o) {
    return true;
  }

  /* The field must have an OPTIONAL label. */
  if (upb_fielddef_label(f) != UPB_LABEL_OPTIONAL) {
    upb_status_seterrmsg(s, "fields in oneof must have OPTIONAL label");
    return false;
  }

  /* Check that no field with this name or number exists already in the oneof.
   * Also check that the field is not already part of a oneof. */
  if (upb_fielddef_name(f) == NULL || upb_fielddef_number(f) == 0) {
    upb_status_seterrmsg(s, "field name or number were not set");
    return false;
  } else if (upb_oneofdef_itof(o, upb_fielddef_number(f)) ||
             upb_oneofdef_ntofz(o, upb_fielddef_name(f))) {
    upb_status_seterrmsg(s, "duplicate field name or number");
    return false;
  } else if (upb_fielddef_containingoneof(f) != NULL) {
    upb_status_seterrmsg(s, "fielddef already belongs to a oneof");
    return false;
  }

  /* We allow adding a field to the oneof either if the field is not part of a
   * msgdef, or if it is and we are also part of the same msgdef. */
  if (o->parent == NULL) {
    /* If we're not in a msgdef, the field cannot be either. Otherwise we would
     * need to magically add this oneof to a msgdef to remain consistent, which
     * is surprising behavior. */
    if (upb_fielddef_containingtype(f) != NULL) {
      upb_status_seterrmsg(s, "fielddef already belongs to a message, but "
                              "oneof does not");
      return false;
    }
  } else {
    /* If we're in a msgdef, the user can add fields that either aren't in any
     * msgdef (in which case they're added to our msgdef) or already a part of
     * our msgdef. */
    if (upb_fielddef_containingtype(f) != NULL &&
        upb_fielddef_containingtype(f) != o->parent) {
      upb_status_seterrmsg(s, "fielddef belongs to a different message "
                              "than oneof");
      return false;
    }
  }

  /* Commit phase. First add the field to our parent msgdef, if any, because
   * that may fail; then add the field to our own tables. */

  if (o->parent != NULL && upb_fielddef_containingtype(f) == NULL) {
    if (!upb_msgdef_addfield((upb_msgdef*)o->parent, f, NULL, s)) {
      return false;
    }
  }

  release_containingtype(f);
  f->oneof = o;
  upb_inttable_insert(&o->itof, upb_fielddef_number(f), upb_value_ptr(f));
  upb_strtable_insert(&o->ntof, upb_fielddef_name(f), upb_value_ptr(f));
  upb_ref2(f, o);
  upb_ref2(o, f);
  if (ref_donor) upb_fielddef_unref(f, ref_donor);

  return true;
}

const upb_fielddef *upb_oneofdef_ntof(const upb_oneofdef *o,
                                      const char *name, size_t length) {
  upb_value val;
  return upb_strtable_lookup2(&o->ntof, name, length, &val) ?
      upb_value_getptr(val) : NULL;
}

const upb_fielddef *upb_oneofdef_itof(const upb_oneofdef *o, uint32_t num) {
  upb_value val;
  return upb_inttable_lookup32(&o->itof, num, &val) ?
      upb_value_getptr(val) : NULL;
}

void upb_oneof_begin(upb_oneof_iter *iter, const upb_oneofdef *o) {
  upb_inttable_begin(iter, &o->itof);
}

void upb_oneof_next(upb_oneof_iter *iter) {
  upb_inttable_next(iter);
}

bool upb_oneof_done(upb_oneof_iter *iter) {
  return upb_inttable_done(iter);
}

upb_fielddef *upb_oneof_iter_field(const upb_oneof_iter *iter) {
  return (upb_fielddef*)upb_value_getptr(upb_inttable_iter_value(iter));
}

void upb_oneof_iter_setdone(upb_oneof_iter *iter) {
  upb_inttable_iter_setdone(iter);
}


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct cleanup_ent {
  upb_cleanup_func *cleanup;
  void *ud;
  struct cleanup_ent *next;
} cleanup_ent;

static void *seeded_alloc(void *ud, void *ptr, size_t oldsize, size_t size);

/* Default allocator **********************************************************/

/* Just use realloc, keeping all allocated blocks in a linked list to destroy at
 * the end. */

typedef struct mem_block {
  /* List is doubly-linked, because in cases where realloc() moves an existing
   * block, we need to be able to remove the old pointer from the list
   * efficiently. */
  struct mem_block *prev, *next;
#ifndef NDEBUG
  size_t size;  /* Doesn't include mem_block structure. */
#endif
} mem_block;

typedef struct {
  mem_block *head;
} default_alloc_ud;

static void *default_alloc(void *_ud, void *ptr, size_t oldsize, size_t size) {
  default_alloc_ud *ud = _ud;
  mem_block *from, *block;
  void *ret;
  UPB_UNUSED(oldsize);

  from = ptr ? (void*)((char*)ptr - sizeof(mem_block)) : NULL;

#ifndef NDEBUG
  if (from) {
    assert(oldsize <= from->size);
  }
#endif

  /* TODO(haberman): we probably need to provide even better alignment here,
   * like 16-byte alignment of the returned data pointer. */
  block = realloc(from, size + sizeof(mem_block));
  if (!block) return NULL;
  ret = (char*)block + sizeof(*block);

#ifndef NDEBUG
  block->size = size;
#endif

  if (from) {
    if (block != from) {
      /* The block was moved, so pointers in next and prev blocks must be
       * updated to its new location. */
      if (block->next) block->next->prev = block;
      if (block->prev) block->prev->next = block;
      if (ud->head == from) ud->head = block;
    }
  } else {
    /* Insert at head of linked list. */
    block->prev = NULL;
    block->next = ud->head;
    if (block->next) block->next->prev = block;
    ud->head = block;
  }

  return ret;
}

static void default_alloc_cleanup(void *_ud) {
  default_alloc_ud *ud = _ud;
  mem_block *block = ud->head;

  while (block) {
    void *to_free = block;
    block = block->next;
    free(to_free);
  }
}


/* Standard error functions ***************************************************/

static bool default_err(void *ud, const upb_status *status) {
  UPB_UNUSED(ud);
  UPB_UNUSED(status);
  return false;
}

static bool write_err_to(void *ud, const upb_status *status) {
  upb_status *copy_to = ud;
  upb_status_copy(copy_to, status);
  return false;
}


/* upb_env ********************************************************************/

void upb_env_init(upb_env *e) {
  default_alloc_ud *ud = (default_alloc_ud*)&e->default_alloc_ud;
  e->ok_ = true;
  e->bytes_allocated = 0;
  e->cleanup_head = NULL;

  ud->head = NULL;

  /* Set default functions. */
  upb_env_setallocfunc(e, default_alloc, ud);
  upb_env_seterrorfunc(e, default_err, NULL);
}

void upb_env_uninit(upb_env *e) {
  cleanup_ent *ent = e->cleanup_head;

  while (ent) {
    ent->cleanup(ent->ud);
    ent = ent->next;
  }

  /* Must do this after running cleanup functions, because this will delete
     the memory we store our cleanup entries in! */
  if (e->alloc == default_alloc) {
    default_alloc_cleanup(e->alloc_ud);
  }
}

UPB_FORCEINLINE void upb_env_setallocfunc(upb_env *e, upb_alloc_func *alloc,
                                          void *ud) {
  e->alloc = alloc;
  e->alloc_ud = ud;
}

UPB_FORCEINLINE void upb_env_seterrorfunc(upb_env *e, upb_error_func *func,
                                          void *ud) {
  e->err = func;
  e->err_ud = ud;
}

void upb_env_reporterrorsto(upb_env *e, upb_status *status) {
  e->err = write_err_to;
  e->err_ud = status;
}

bool upb_env_ok(const upb_env *e) {
  return e->ok_;
}

bool upb_env_reporterror(upb_env *e, const upb_status *status) {
  e->ok_ = false;
  return e->err(e->err_ud, status);
}

bool upb_env_addcleanup(upb_env *e, upb_cleanup_func *func, void *ud) {
  cleanup_ent *ent = upb_env_malloc(e, sizeof(cleanup_ent));
  if (!ent) return false;

  ent->cleanup = func;
  ent->ud = ud;
  ent->next = e->cleanup_head;
  e->cleanup_head = ent;

  return true;
}

void *upb_env_malloc(upb_env *e, size_t size) {
  e->bytes_allocated += size;
  if (e->alloc == seeded_alloc) {
    /* This is equivalent to the next branch, but allows inlining for a
     * measurable perf benefit. */
    return seeded_alloc(e->alloc_ud, NULL, 0, size);
  } else {
    return e->alloc(e->alloc_ud, NULL, 0, size);
  }
}

void *upb_env_realloc(upb_env *e, void *ptr, size_t oldsize, size_t size) {
  char *ret;
  assert(oldsize <= size);
  ret = e->alloc(e->alloc_ud, ptr, oldsize, size);

#ifndef NDEBUG
  /* Overwrite non-preserved memory to ensure callers are passing the oldsize
   * that they truly require. */
  memset(ret + oldsize, 0xff, size - oldsize);
#endif

  return ret;
}

size_t upb_env_bytesallocated(const upb_env *e) {
  return e->bytes_allocated;
}


/* upb_seededalloc ************************************************************/

/* Be conservative and choose 16 in case anyone is using SSE. */
static const size_t maxalign = 16;

static size_t align_up(size_t size) {
  return ((size + maxalign - 1) / maxalign) * maxalign;
}

UPB_FORCEINLINE static void *seeded_alloc(void *ud, void *ptr, size_t oldsize,
                                          size_t size) {
  upb_seededalloc *a = ud;

  size = align_up(size);

  assert(a->mem_limit >= a->mem_ptr);

  if (oldsize == 0 && size <= (size_t)(a->mem_limit - a->mem_ptr)) {
    /* Fast path: we can satisfy from the initial allocation. */
    void *ret = a->mem_ptr;
    a->mem_ptr += size;
    return ret;
  } else {
    char *chptr = ptr;
    /* Slow path: fallback to other allocator. */
    a->need_cleanup = true;
    /* Is `ptr` part of the user-provided initial block? Don't pass it to the
     * default allocator if so; otherwise, it may try to realloc() the block. */
    if (chptr >= a->mem_base && chptr < a->mem_limit) {
      void *ret;
      assert(chptr + oldsize <= a->mem_limit);
      ret = a->alloc(a->alloc_ud, NULL, 0, size);
      if (ret) memcpy(ret, ptr, oldsize);
      return ret;
    } else {
      return a->alloc(a->alloc_ud, ptr, oldsize, size);
    }
  }
}

void upb_seededalloc_init(upb_seededalloc *a, void *mem, size_t len) {
  default_alloc_ud *ud = (default_alloc_ud*)&a->default_alloc_ud;
  a->mem_base = mem;
  a->mem_ptr = mem;
  a->mem_limit = (char*)mem + len;
  a->need_cleanup = false;
  a->returned_allocfunc = false;

  ud->head = NULL;

  upb_seededalloc_setfallbackalloc(a, default_alloc, ud);
}

void upb_seededalloc_uninit(upb_seededalloc *a) {
  if (a->alloc == default_alloc && a->need_cleanup) {
    default_alloc_cleanup(a->alloc_ud);
  }
}

UPB_FORCEINLINE void upb_seededalloc_setfallbackalloc(upb_seededalloc *a,
                                                      upb_alloc_func *alloc,
                                                      void *ud) {
  assert(!a->returned_allocfunc);
  a->alloc = alloc;
  a->alloc_ud = ud;
}

upb_alloc_func *upb_seededalloc_getallocfunc(upb_seededalloc *a) {
  a->returned_allocfunc = true;
  return seeded_alloc;
}
/*
** TODO(haberman): it's unclear whether a lot of the consistency checks should
** assert() or return false.
*/


#include <stdlib.h>
#include <string.h>



/* Defined for the sole purpose of having a unique pointer value for
 * UPB_NO_CLOSURE. */
char _upb_noclosure;

static void freehandlers(upb_refcounted *r) {
  upb_handlers *h = (upb_handlers*)r;

  upb_inttable_iter i;
  upb_inttable_begin(&i, &h->cleanup_);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    void *val = (void*)upb_inttable_iter_key(&i);
    upb_value func_val = upb_inttable_iter_value(&i);
    upb_handlerfree *func = upb_value_getfptr(func_val);
    func(val);
  }

  upb_inttable_uninit(&h->cleanup_);
  upb_msgdef_unref(h->msg, h);
  free(h->sub);
  free(h);
}

static void visithandlers(const upb_refcounted *r, upb_refcounted_visit *visit,
                          void *closure) {
  const upb_handlers *h = (const upb_handlers*)r;
  upb_msg_field_iter i;
  for(upb_msg_field_begin(&i, h->msg);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    const upb_handlers *sub;
    if (!upb_fielddef_issubmsg(f)) continue;
    sub = upb_handlers_getsubhandlers(h, f);
    if (sub) visit(r, upb_handlers_upcast(sub), closure);
  }
}

static const struct upb_refcounted_vtbl vtbl = {visithandlers, freehandlers};

typedef struct {
  upb_inttable tab;  /* maps upb_msgdef* -> upb_handlers*. */
  upb_handlers_callback *callback;
  const void *closure;
} dfs_state;

/* TODO(haberman): discard upb_handlers* objects that do not actually have any
 * handlers set and cannot reach any upb_handlers* object that does.  This is
 * slightly tricky to do correctly. */
static upb_handlers *newformsg(const upb_msgdef *m, const void *owner,
                               dfs_state *s) {
  upb_msg_field_iter i;
  upb_handlers *h = upb_handlers_new(m, owner);
  if (!h) return NULL;
  if (!upb_inttable_insertptr(&s->tab, m, upb_value_ptr(h))) goto oom;

  s->callback(s->closure, h);

  /* For each submessage field, get or create a handlers object and set it as
   * the subhandlers. */
  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    const upb_msgdef *subdef;
    upb_value subm_ent;

    if (!upb_fielddef_issubmsg(f)) continue;

    subdef = upb_downcast_msgdef(upb_fielddef_subdef(f));
    if (upb_inttable_lookupptr(&s->tab, subdef, &subm_ent)) {
      upb_handlers_setsubhandlers(h, f, upb_value_getptr(subm_ent));
    } else {
      upb_handlers *sub_mh = newformsg(subdef, &sub_mh, s);
      if (!sub_mh) goto oom;
      upb_handlers_setsubhandlers(h, f, sub_mh);
      upb_handlers_unref(sub_mh, &sub_mh);
    }
  }
  return h;

oom:
  upb_handlers_unref(h, owner);
  return NULL;
}

/* Given a selector for a STARTSUBMSG handler, resolves to a pointer to the
 * subhandlers for this submessage field. */
#define SUBH(h, selector) (h->sub[selector])

/* The selector for a submessage field is the field index. */
#define SUBH_F(h, f) SUBH(h, f->index_)

static int32_t trygetsel(upb_handlers *h, const upb_fielddef *f,
                         upb_handlertype_t type) {
  upb_selector_t sel;
  assert(!upb_handlers_isfrozen(h));
  if (upb_handlers_msgdef(h) != upb_fielddef_containingtype(f)) {
    upb_status_seterrf(
        &h->status_, "type mismatch: field %s does not belong to message %s",
        upb_fielddef_name(f), upb_msgdef_fullname(upb_handlers_msgdef(h)));
    return -1;
  }
  if (!upb_handlers_getselector(f, type, &sel)) {
    upb_status_seterrf(
        &h->status_,
        "type mismatch: cannot register handler type %d for field %s",
        type, upb_fielddef_name(f));
    return -1;
  }
  return sel;
}

static upb_selector_t handlers_getsel(upb_handlers *h, const upb_fielddef *f,
                             upb_handlertype_t type) {
  int32_t sel = trygetsel(h, f, type);
  assert(sel >= 0);
  return sel;
}

static const void **returntype(upb_handlers *h, const upb_fielddef *f,
                               upb_handlertype_t type) {
  return &h->table[handlers_getsel(h, f, type)].attr.return_closure_type_;
}

static bool doset(upb_handlers *h, int32_t sel, const upb_fielddef *f,
                  upb_handlertype_t type, upb_func *func,
                  upb_handlerattr *attr) {
  upb_handlerattr set_attr = UPB_HANDLERATTR_INITIALIZER;
  const void *closure_type;
  const void **context_closure_type;

  assert(!upb_handlers_isfrozen(h));

  if (sel < 0) {
    upb_status_seterrmsg(&h->status_,
                         "incorrect handler type for this field.");
    return false;
  }

  if (h->table[sel].func) {
    upb_status_seterrmsg(&h->status_,
                         "cannot change handler once it has been set.");
    return false;
  }

  if (attr) {
    set_attr = *attr;
  }

  /* Check that the given closure type matches the closure type that has been
   * established for this context (if any). */
  closure_type = upb_handlerattr_closuretype(&set_attr);

  if (type == UPB_HANDLER_STRING) {
    context_closure_type = returntype(h, f, UPB_HANDLER_STARTSTR);
  } else if (f && upb_fielddef_isseq(f) &&
             type != UPB_HANDLER_STARTSEQ &&
             type != UPB_HANDLER_ENDSEQ) {
    context_closure_type = returntype(h, f, UPB_HANDLER_STARTSEQ);
  } else {
    context_closure_type = &h->top_closure_type;
  }

  if (closure_type && *context_closure_type &&
      closure_type != *context_closure_type) {
    /* TODO(haberman): better message for debugging. */
    if (f) {
      upb_status_seterrf(&h->status_,
                         "closure type does not match for field %s",
                         upb_fielddef_name(f));
    } else {
      upb_status_seterrmsg(
          &h->status_, "closure type does not match for message-level handler");
    }
    return false;
  }

  if (closure_type)
    *context_closure_type = closure_type;

  /* If this is a STARTSEQ or STARTSTR handler, check that the returned pointer
   * matches any pre-existing expectations about what type is expected. */
  if (type == UPB_HANDLER_STARTSEQ || type == UPB_HANDLER_STARTSTR) {
    const void *return_type = upb_handlerattr_returnclosuretype(&set_attr);
    const void *table_return_type =
        upb_handlerattr_returnclosuretype(&h->table[sel].attr);
    if (return_type && table_return_type && return_type != table_return_type) {
      upb_status_seterrmsg(&h->status_, "closure return type does not match");
      return false;
    }

    if (table_return_type && !return_type)
      upb_handlerattr_setreturnclosuretype(&set_attr, table_return_type);
  }

  h->table[sel].func = (upb_func*)func;
  h->table[sel].attr = set_attr;
  return true;
}

/* Returns the effective closure type for this handler (which will propagate
 * from outer frames if this frame has no START* handler).  Not implemented for
 * UPB_HANDLER_STRING at the moment since this is not needed.  Returns NULL is
 * the effective closure type is unspecified (either no handler was registered
 * to specify it or the handler that was registered did not specify the closure
 * type). */
const void *effective_closure_type(upb_handlers *h, const upb_fielddef *f,
                                   upb_handlertype_t type) {
  const void *ret;
  upb_selector_t sel;

  assert(type != UPB_HANDLER_STRING);
  ret = h->top_closure_type;

  if (upb_fielddef_isseq(f) &&
      type != UPB_HANDLER_STARTSEQ &&
      type != UPB_HANDLER_ENDSEQ &&
      h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSEQ)].func) {
    ret = upb_handlerattr_returnclosuretype(&h->table[sel].attr);
  }

  if (type == UPB_HANDLER_STRING &&
      h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSTR)].func) {
    ret = upb_handlerattr_returnclosuretype(&h->table[sel].attr);
  }

  /* The effective type of the submessage; not used yet.
   * if (type == SUBMESSAGE &&
   *     h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSUBMSG)].func) {
   *   ret = upb_handlerattr_returnclosuretype(&h->table[sel].attr);
   * } */

  return ret;
}

/* Checks whether the START* handler specified by f & type is missing even
 * though it is required to convert the established type of an outer frame
 * ("closure_type") into the established type of an inner frame (represented in
 * the return closure type of this handler's attr. */
bool checkstart(upb_handlers *h, const upb_fielddef *f, upb_handlertype_t type,
                upb_status *status) {
  const void *closure_type;
  const upb_handlerattr *attr;
  const void *return_closure_type;

  upb_selector_t sel = handlers_getsel(h, f, type);
  if (h->table[sel].func) return true;
  closure_type = effective_closure_type(h, f, type);
  attr = &h->table[sel].attr;
  return_closure_type = upb_handlerattr_returnclosuretype(attr);
  if (closure_type && return_closure_type &&
      closure_type != return_closure_type) {
    upb_status_seterrf(status,
                       "expected start handler to return sub type for field %f",
                       upb_fielddef_name(f));
    return false;
  }
  return true;
}

/* Public interface ***********************************************************/

upb_handlers *upb_handlers_new(const upb_msgdef *md, const void *owner) {
  int extra;
  upb_handlers *h;

  assert(upb_msgdef_isfrozen(md));

  extra = sizeof(upb_handlers_tabent) * (md->selector_count - 1);
  h = calloc(sizeof(*h) + extra, 1);
  if (!h) return NULL;

  h->msg = md;
  upb_msgdef_ref(h->msg, h);
  upb_status_clear(&h->status_);
  h->sub = calloc(md->submsg_field_count, sizeof(*h->sub));
  if (!h->sub) goto oom;
  if (!upb_refcounted_init(upb_handlers_upcast_mutable(h), &vtbl, owner))
    goto oom;
  if (!upb_inttable_init(&h->cleanup_, UPB_CTYPE_FPTR)) goto oom;

  /* calloc() above initialized all handlers to NULL. */
  return h;

oom:
  freehandlers(upb_handlers_upcast_mutable(h));
  return NULL;
}

const upb_handlers *upb_handlers_newfrozen(const upb_msgdef *m,
                                           const void *owner,
                                           upb_handlers_callback *callback,
                                           const void *closure) {
  dfs_state state;
  upb_handlers *ret;
  bool ok;
  upb_refcounted *r;

  state.callback = callback;
  state.closure = closure;
  if (!upb_inttable_init(&state.tab, UPB_CTYPE_PTR)) return NULL;

  ret = newformsg(m, owner, &state);

  upb_inttable_uninit(&state.tab);
  if (!ret) return NULL;

  r = upb_handlers_upcast_mutable(ret);
  ok = upb_refcounted_freeze(&r, 1, NULL, UPB_MAX_HANDLER_DEPTH);
  UPB_ASSERT_VAR(ok, ok);

  return ret;
}

const upb_status *upb_handlers_status(upb_handlers *h) {
  assert(!upb_handlers_isfrozen(h));
  return &h->status_;
}

void upb_handlers_clearerr(upb_handlers *h) {
  assert(!upb_handlers_isfrozen(h));
  upb_status_clear(&h->status_);
}

#define SETTER(name, handlerctype, handlertype) \
  bool upb_handlers_set ## name(upb_handlers *h, const upb_fielddef *f, \
                                handlerctype func, upb_handlerattr *attr) { \
    int32_t sel = trygetsel(h, f, handlertype); \
    return doset(h, sel, f, handlertype, (upb_func*)func, attr); \
  }

SETTER(int32,       upb_int32_handlerfunc*,       UPB_HANDLER_INT32)
SETTER(int64,       upb_int64_handlerfunc*,       UPB_HANDLER_INT64)
SETTER(uint32,      upb_uint32_handlerfunc*,      UPB_HANDLER_UINT32)
SETTER(uint64,      upb_uint64_handlerfunc*,      UPB_HANDLER_UINT64)
SETTER(float,       upb_float_handlerfunc*,       UPB_HANDLER_FLOAT)
SETTER(double,      upb_double_handlerfunc*,      UPB_HANDLER_DOUBLE)
SETTER(bool,        upb_bool_handlerfunc*,        UPB_HANDLER_BOOL)
SETTER(startstr,    upb_startstr_handlerfunc*,    UPB_HANDLER_STARTSTR)
SETTER(string,      upb_string_handlerfunc*,      UPB_HANDLER_STRING)
SETTER(endstr,      upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSTR)
SETTER(startseq,    upb_startfield_handlerfunc*,  UPB_HANDLER_STARTSEQ)
SETTER(startsubmsg, upb_startfield_handlerfunc*,  UPB_HANDLER_STARTSUBMSG)
SETTER(endsubmsg,   upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSUBMSG)
SETTER(endseq,      upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSEQ)

#undef SETTER

bool upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handlerfunc *func,
                              upb_handlerattr *attr) {
  return doset(h, UPB_STARTMSG_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handlerfunc *func,
                            upb_handlerattr *attr) {
  assert(!upb_handlers_isfrozen(h));
  return doset(h, UPB_ENDMSG_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setsubhandlers(upb_handlers *h, const upb_fielddef *f,
                                 const upb_handlers *sub) {
  assert(sub);
  assert(!upb_handlers_isfrozen(h));
  assert(upb_fielddef_issubmsg(f));
  if (SUBH_F(h, f)) return false;  /* Can't reset. */
  if (upb_msgdef_upcast(upb_handlers_msgdef(sub)) != upb_fielddef_subdef(f)) {
    return false;
  }
  SUBH_F(h, f) = sub;
  upb_ref2(sub, h);
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f) {
  assert(upb_fielddef_issubmsg(f));
  return SUBH_F(h, f);
}

bool upb_handlers_getattr(const upb_handlers *h, upb_selector_t sel,
                          upb_handlerattr *attr) {
  if (!upb_handlers_gethandler(h, sel))
    return false;
  *attr = h->table[sel].attr;
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers_sel(const upb_handlers *h,
                                                    upb_selector_t sel) {
  /* STARTSUBMSG selector in sel is the field's selector base. */
  return SUBH(h, sel - UPB_STATIC_SELECTOR_COUNT);
}

const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h) { return h->msg; }

bool upb_handlers_addcleanup(upb_handlers *h, void *p, upb_handlerfree *func) {
  bool ok;
  if (upb_inttable_lookupptr(&h->cleanup_, p, NULL)) {
    return false;
  }
  ok = upb_inttable_insertptr(&h->cleanup_, p, upb_value_fptr(func));
  UPB_ASSERT_VAR(ok, ok);
  return true;
}


/* "Static" methods ***********************************************************/

bool upb_handlers_freeze(upb_handlers *const*handlers, int n, upb_status *s) {
  /* TODO: verify we have a transitive closure. */
  int i;
  for (i = 0; i < n; i++) {
    upb_msg_field_iter j;
    upb_handlers *h = handlers[i];

    if (!upb_ok(&h->status_)) {
      upb_status_seterrf(s, "handlers for message %s had error status: %s",
                         upb_msgdef_fullname(upb_handlers_msgdef(h)),
                         upb_status_errmsg(&h->status_));
      return false;
    }

    /* Check that there are no closure mismatches due to missing Start* handlers
     * or subhandlers with different type-level types. */
    for(upb_msg_field_begin(&j, h->msg);
        !upb_msg_field_done(&j);
        upb_msg_field_next(&j)) {

      const upb_fielddef *f = upb_msg_iter_field(&j);
      if (upb_fielddef_isseq(f)) {
        if (!checkstart(h, f, UPB_HANDLER_STARTSEQ, s))
          return false;
      }

      if (upb_fielddef_isstring(f)) {
        if (!checkstart(h, f, UPB_HANDLER_STARTSTR, s))
          return false;
      }

      if (upb_fielddef_issubmsg(f)) {
        bool hashandler = false;
        if (upb_handlers_gethandler(
                h, handlers_getsel(h, f, UPB_HANDLER_STARTSUBMSG)) ||
            upb_handlers_gethandler(
                h, handlers_getsel(h, f, UPB_HANDLER_ENDSUBMSG))) {
          hashandler = true;
        }

        if (upb_fielddef_isseq(f) &&
            (upb_handlers_gethandler(
                 h, handlers_getsel(h, f, UPB_HANDLER_STARTSEQ)) ||
             upb_handlers_gethandler(
                 h, handlers_getsel(h, f, UPB_HANDLER_ENDSEQ)))) {
          hashandler = true;
        }

        if (hashandler && !upb_handlers_getsubhandlers(h, f)) {
          /* For now we add an empty subhandlers in this case.  It makes the
           * decoder code generator simpler, because it only has to handle two
           * cases (submessage has handlers or not) as opposed to three
           * (submessage has handlers in enclosing message but no subhandlers).
           *
           * This makes parsing less efficient in the case that we want to
           * notice a submessage but skip its contents (like if we're testing
           * for submessage presence or counting the number of repeated
           * submessages).  In this case we will end up parsing the submessage
           * field by field and throwing away the results for each, instead of
           * skipping the whole delimited thing at once.  If this is an issue we
           * can revisit it, but do remember that this only arises when you have
           * handlers (startseq/startsubmsg/endsubmsg/endseq) set for the
           * submessage but no subhandlers.  The uses cases for this are
           * limited. */
          upb_handlers *sub = upb_handlers_new(upb_fielddef_msgsubdef(f), &sub);
          upb_handlers_setsubhandlers(h, f, sub);
          upb_handlers_unref(sub, &sub);
        }

        /* TODO(haberman): check type of submessage.
         * This is slightly tricky; also consider whether we should check that
         * they match at setsubhandlers time. */
      }
    }
  }

  if (!upb_refcounted_freeze((upb_refcounted*const*)handlers, n, s,
                             UPB_MAX_HANDLER_DEPTH)) {
    return false;
  }

  return true;
}

upb_handlertype_t upb_handlers_getprimitivehandlertype(const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM: return UPB_HANDLER_INT32;
    case UPB_TYPE_INT64: return UPB_HANDLER_INT64;
    case UPB_TYPE_UINT32: return UPB_HANDLER_UINT32;
    case UPB_TYPE_UINT64: return UPB_HANDLER_UINT64;
    case UPB_TYPE_FLOAT: return UPB_HANDLER_FLOAT;
    case UPB_TYPE_DOUBLE: return UPB_HANDLER_DOUBLE;
    case UPB_TYPE_BOOL: return UPB_HANDLER_BOOL;
    default: assert(false); return -1;  /* Invalid input. */
  }
}

bool upb_handlers_getselector(const upb_fielddef *f, upb_handlertype_t type,
                              upb_selector_t *s) {
  switch (type) {
    case UPB_HANDLER_INT32:
    case UPB_HANDLER_INT64:
    case UPB_HANDLER_UINT32:
    case UPB_HANDLER_UINT64:
    case UPB_HANDLER_FLOAT:
    case UPB_HANDLER_DOUBLE:
    case UPB_HANDLER_BOOL:
      if (!upb_fielddef_isprimitive(f) ||
          upb_handlers_getprimitivehandlertype(f) != type)
        return false;
      *s = f->selector_base;
      break;
    case UPB_HANDLER_STRING:
      if (upb_fielddef_isstring(f)) {
        *s = f->selector_base;
      } else if (upb_fielddef_lazy(f)) {
        *s = f->selector_base + 3;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_STARTSTR:
      if (upb_fielddef_isstring(f) || upb_fielddef_lazy(f)) {
        *s = f->selector_base + 1;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_ENDSTR:
      if (upb_fielddef_isstring(f) || upb_fielddef_lazy(f)) {
        *s = f->selector_base + 2;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_STARTSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = f->selector_base - 2;
      break;
    case UPB_HANDLER_ENDSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = f->selector_base - 1;
      break;
    case UPB_HANDLER_STARTSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      /* Selectors for STARTSUBMSG are at the beginning of the table so that the
       * selector can also be used as an index into the "sub" array of
       * subhandlers.  The indexes for the two into these two tables are the
       * same, except that in the handler table the static selectors come first. */
      *s = f->index_ + UPB_STATIC_SELECTOR_COUNT;
      break;
    case UPB_HANDLER_ENDSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      *s = f->selector_base;
      break;
  }
  assert((size_t)*s < upb_fielddef_containingtype(f)->selector_count);
  return true;
}

uint32_t upb_handlers_selectorbaseoffset(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) ? 2 : 0;
}

uint32_t upb_handlers_selectorcount(const upb_fielddef *f) {
  uint32_t ret = 1;
  if (upb_fielddef_isseq(f)) ret += 2;    /* STARTSEQ/ENDSEQ */
  if (upb_fielddef_isstring(f)) ret += 2; /* [STRING]/STARTSTR/ENDSTR */
  if (upb_fielddef_issubmsg(f)) {
    /* ENDSUBMSG (STARTSUBMSG is at table beginning) */
    ret += 0;
    if (upb_fielddef_lazy(f)) {
      /* STARTSTR/ENDSTR/STRING (for lazy) */
      ret += 3;
    }
  }
  return ret;
}


/* upb_handlerattr ************************************************************/

void upb_handlerattr_init(upb_handlerattr *attr) {
  upb_handlerattr from = UPB_HANDLERATTR_INITIALIZER;
  memcpy(attr, &from, sizeof(*attr));
}

void upb_handlerattr_uninit(upb_handlerattr *attr) {
  UPB_UNUSED(attr);
}

bool upb_handlerattr_sethandlerdata(upb_handlerattr *attr, const void *hd) {
  attr->handler_data_ = hd;
  return true;
}

bool upb_handlerattr_setclosuretype(upb_handlerattr *attr, const void *type) {
  attr->closure_type_ = type;
  return true;
}

const void *upb_handlerattr_closuretype(const upb_handlerattr *attr) {
  return attr->closure_type_;
}

bool upb_handlerattr_setreturnclosuretype(upb_handlerattr *attr,
                                          const void *type) {
  attr->return_closure_type_ = type;
  return true;
}

const void *upb_handlerattr_returnclosuretype(const upb_handlerattr *attr) {
  return attr->return_closure_type_;
}

bool upb_handlerattr_setalwaysok(upb_handlerattr *attr, bool alwaysok) {
  attr->alwaysok_ = alwaysok;
  return true;
}

bool upb_handlerattr_alwaysok(const upb_handlerattr *attr) {
  return attr->alwaysok_;
}

/* upb_bufhandle **************************************************************/

size_t upb_bufhandle_objofs(const upb_bufhandle *h) {
  return h->objofs_;
}

/* upb_byteshandler ***********************************************************/

void upb_byteshandler_init(upb_byteshandler* h) {
  memset(h, 0, sizeof(*h));
}

/* For when we support handlerfree callbacks. */
void upb_byteshandler_uninit(upb_byteshandler* h) {
  UPB_UNUSED(h);
}

bool upb_byteshandler_setstartstr(upb_byteshandler *h,
                                  upb_startstr_handlerfunc *func, void *d) {
  h->table[UPB_STARTSTR_SELECTOR].func = (upb_func*)func;
  h->table[UPB_STARTSTR_SELECTOR].attr.handler_data_ = d;
  return true;
}

bool upb_byteshandler_setstring(upb_byteshandler *h,
                                upb_string_handlerfunc *func, void *d) {
  h->table[UPB_STRING_SELECTOR].func = (upb_func*)func;
  h->table[UPB_STRING_SELECTOR].attr.handler_data_ = d;
  return true;
}

bool upb_byteshandler_setendstr(upb_byteshandler *h,
                                upb_endfield_handlerfunc *func, void *d) {
  h->table[UPB_ENDSTR_SELECTOR].func = (upb_func*)func;
  h->table[UPB_ENDSTR_SELECTOR].attr.handler_data_ = d;
  return true;
}
/*
** upb::RefCounted Implementation
**
** Our key invariants are:
** 1. reference cycles never span groups
** 2. for ref2(to, from), we increment to's count iff group(from) != group(to)
**
** The previous two are how we avoid leaking cycles.  Other important
** invariants are:
** 3. for mutable objects "from" and "to", if there exists a ref2(to, from)
**    this implies group(from) == group(to).  (In practice, what we implement
**    is even stronger; "from" and "to" will share a group if there has *ever*
**    been a ref2(to, from), but all that is necessary for correctness is the
**    weaker one).
** 4. mutable and immutable objects are never in the same group.
*/


#include <setjmp.h>
#include <stdlib.h>

static void freeobj(upb_refcounted *o);

const char untracked_val;
const void *UPB_UNTRACKED_REF = &untracked_val;

/* arch-specific atomic primitives  *******************************************/

#ifdef UPB_THREAD_UNSAFE /*---------------------------------------------------*/

static void atomic_inc(uint32_t *a) { (*a)++; }
static bool atomic_dec(uint32_t *a) { return --(*a) == 0; }

#elif defined(__GNUC__) || defined(__clang__) /*------------------------------*/

static void atomic_inc(uint32_t *a) { __sync_fetch_and_add(a, 1); }
static bool atomic_dec(uint32_t *a) { return __sync_sub_and_fetch(a, 1) == 0; }

#elif defined(WIN32) /*-------------------------------------------------------*/

#include <Windows.h>

static void atomic_inc(upb_atomic_t *a) { InterlockedIncrement(&a->val); }
static bool atomic_dec(upb_atomic_t *a) {
  return InterlockedDecrement(&a->val) == 0;
}

#else
#error Atomic primitives not defined for your platform/CPU.  \
       Implement them or compile with UPB_THREAD_UNSAFE.
#endif

/* All static objects point to this refcount.
 * It is special-cased in ref/unref below.  */
uint32_t static_refcount = -1;

/* We can avoid atomic ops for statically-declared objects.
 * This is a minor optimization but nice since we can avoid degrading under
 * contention in this case. */

static void refgroup(uint32_t *group) {
  if (group != &static_refcount)
    atomic_inc(group);
}

static bool unrefgroup(uint32_t *group) {
  if (group == &static_refcount) {
    return false;
  } else {
    return atomic_dec(group);
  }
}


/* Reference tracking (debug only) ********************************************/

#ifdef UPB_DEBUG_REFS

#ifdef UPB_THREAD_UNSAFE

static void upb_lock() {}
static void upb_unlock() {}

#else

/* User must define functions that lock/unlock a global mutex and link this
 * file against them. */
void upb_lock();
void upb_unlock();

#endif

/* UPB_DEBUG_REFS mode counts on being able to malloc() memory in some
 * code-paths that can normally never fail, like upb_refcounted_ref().  Since
 * we have no way to propagage out-of-memory errors back to the user, and since
 * these errors can only occur in UPB_DEBUG_REFS mode, we immediately fail. */
#define CHECK_OOM(predicate) if (!(predicate)) { assert(predicate); exit(1); }

typedef struct {
  int count;  /* How many refs there are (duplicates only allowed for ref2). */
  bool is_ref2;
} trackedref;

static trackedref *trackedref_new(bool is_ref2) {
  trackedref *ret = malloc(sizeof(*ret));
  CHECK_OOM(ret);
  ret->count = 1;
  ret->is_ref2 = is_ref2;
  return ret;
}

static void track(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;

  assert(owner);
  if (owner == UPB_UNTRACKED_REF) return;

  upb_lock();
  if (upb_inttable_lookupptr(r->refs, owner, &v)) {
    trackedref *ref = upb_value_getptr(v);
    /* Since we allow multiple ref2's for the same to/from pair without
     * allocating separate memory for each one, we lose the fine-grained
     * tracking behavior we get with regular refs.  Since ref2s only happen
     * inside upb, we'll accept this limitation until/unless there is a really
     * difficult upb-internal bug that can't be figured out without it. */
    assert(ref2);
    assert(ref->is_ref2);
    ref->count++;
  } else {
    trackedref *ref = trackedref_new(ref2);
    bool ok = upb_inttable_insertptr(r->refs, owner, upb_value_ptr(ref));
    CHECK_OOM(ok);
    if (ref2) {
      /* We know this cast is safe when it is a ref2, because it's coming from
       * another refcounted object. */
      const upb_refcounted *from = owner;
      assert(!upb_inttable_lookupptr(from->ref2s, r, NULL));
      ok = upb_inttable_insertptr(from->ref2s, r, upb_value_ptr(NULL));
      CHECK_OOM(ok);
    }
  }
  upb_unlock();
}

static void untrack(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;
  bool found;
  trackedref *ref;

  assert(owner);
  if (owner == UPB_UNTRACKED_REF) return;

  upb_lock();
  found = upb_inttable_lookupptr(r->refs, owner, &v);
  /* This assert will fail if an owner attempts to release a ref it didn't have. */
  UPB_ASSERT_VAR(found, found);
  ref = upb_value_getptr(v);
  assert(ref->is_ref2 == ref2);
  if (--ref->count == 0) {
    free(ref);
    upb_inttable_removeptr(r->refs, owner, NULL);
    if (ref2) {
      /* We know this cast is safe when it is a ref2, because it's coming from
       * another refcounted object. */
      const upb_refcounted *from = owner;
      bool removed = upb_inttable_removeptr(from->ref2s, r, NULL);
      assert(removed);
    }
  }
  upb_unlock();
}

static void checkref(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;
  bool found;
  trackedref *ref;

  upb_lock();
  found = upb_inttable_lookupptr(r->refs, owner, &v);
  UPB_ASSERT_VAR(found, found);
  ref = upb_value_getptr(v);
  assert(ref->is_ref2 == ref2);
  upb_unlock();
}

/* Populates the given UPB_CTYPE_INT32 inttable with counts of ref2's that
 * originate from the given owner. */
static void getref2s(const upb_refcounted *owner, upb_inttable *tab) {
  upb_inttable_iter i;

  upb_lock();
  upb_inttable_begin(&i, owner->ref2s);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_value v;
    upb_value count;
    trackedref *ref;
    bool ok;
    bool found;

    upb_refcounted *to = (upb_refcounted*)upb_inttable_iter_key(&i);

    /* To get the count we need to look in the target's table. */
    found = upb_inttable_lookupptr(to->refs, owner, &v);
    assert(found);
    ref = upb_value_getptr(v);
    count = upb_value_int32(ref->count);

    ok = upb_inttable_insertptr(tab, to, count);
    CHECK_OOM(ok);
  }
  upb_unlock();
}

typedef struct {
  upb_inttable ref2;
  const upb_refcounted *obj;
} check_state;

static void visit_check(const upb_refcounted *obj, const upb_refcounted *subobj,
                        void *closure) {
  check_state *s = closure;
  upb_inttable *ref2 = &s->ref2;
  upb_value v;
  bool removed;
  int32_t newcount;

  assert(obj == s->obj);
  assert(subobj);
  removed = upb_inttable_removeptr(ref2, subobj, &v);
  /* The following assertion will fail if the visit() function visits a subobj
   * that it did not have a ref2 on, or visits the same subobj too many times. */
  assert(removed);
  newcount = upb_value_getint32(v) - 1;
  if (newcount > 0) {
    upb_inttable_insert(ref2, (uintptr_t)subobj, upb_value_int32(newcount));
  }
}

static void visit(const upb_refcounted *r, upb_refcounted_visit *v,
                  void *closure) {
  bool ok;

  /* In DEBUG_REFS mode we know what existing ref2 refs there are, so we know
   * exactly the set of nodes that visit() should visit.  So we verify visit()'s
   * correctness here. */
  check_state state;
  state.obj = r;
  ok = upb_inttable_init(&state.ref2, UPB_CTYPE_INT32);
  CHECK_OOM(ok);
  getref2s(r, &state.ref2);

  /* This should visit any children in the ref2 table. */
  if (r->vtbl->visit) r->vtbl->visit(r, visit_check, &state);

  /* This assertion will fail if the visit() function missed any children. */
  assert(upb_inttable_count(&state.ref2) == 0);
  upb_inttable_uninit(&state.ref2);
  if (r->vtbl->visit) r->vtbl->visit(r, v, closure);
}

static bool trackinit(upb_refcounted *r) {
  r->refs = malloc(sizeof(*r->refs));
  r->ref2s = malloc(sizeof(*r->ref2s));
  if (!r->refs || !r->ref2s) goto err1;

  if (!upb_inttable_init(r->refs, UPB_CTYPE_PTR)) goto err1;
  if (!upb_inttable_init(r->ref2s, UPB_CTYPE_PTR)) goto err2;
  return true;

err2:
  upb_inttable_uninit(r->refs);
err1:
  free(r->refs);
  free(r->ref2s);
  return false;
}

static void trackfree(const upb_refcounted *r) {
  upb_inttable_uninit(r->refs);
  upb_inttable_uninit(r->ref2s);
  free(r->refs);
  free(r->ref2s);
}

#else

static void track(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static void untrack(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static void checkref(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static bool trackinit(upb_refcounted *r) {
  UPB_UNUSED(r);
  return true;
}

static void trackfree(const upb_refcounted *r) {
  UPB_UNUSED(r);
}

static void visit(const upb_refcounted *r, upb_refcounted_visit *v,
                  void *closure) {
  if (r->vtbl->visit) r->vtbl->visit(r, v, closure);
}

#endif  /* UPB_DEBUG_REFS */


/* freeze() *******************************************************************/

/* The freeze() operation is by far the most complicated part of this scheme.
 * We compute strongly-connected components and then mutate the graph such that
 * we preserve the invariants documented at the top of this file.  And we must
 * handle out-of-memory errors gracefully (without leaving the graph
 * inconsistent), which adds to the fun. */

/* The state used by the freeze operation (shared across many functions). */
typedef struct {
  int depth;
  int maxdepth;
  uint64_t index;
  /* Maps upb_refcounted* -> attributes (color, etc).  attr layout varies by
   * color. */
  upb_inttable objattr;
  upb_inttable stack;   /* stack of upb_refcounted* for Tarjan's algorithm. */
  upb_inttable groups;  /* array of uint32_t*, malloc'd refcounts for new groups */
  upb_status *status;
  jmp_buf err;
} tarjan;

static void release_ref2(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure);

/* Node attributes -----------------------------------------------------------*/

/* After our analysis phase all nodes will be either GRAY or WHITE. */

typedef enum {
  BLACK = 0,  /* Object has not been seen. */
  GRAY,   /* Object has been found via a refgroup but may not be reachable. */
  GREEN,  /* Object is reachable and is currently on the Tarjan stack. */
  WHITE   /* Object is reachable and has been assigned a group (SCC). */
} color_t;

UPB_NORETURN static void err(tarjan *t) { longjmp(t->err, 1); }
UPB_NORETURN static void oom(tarjan *t) {
  upb_status_seterrmsg(t->status, "out of memory");
  err(t);
}

static uint64_t trygetattr(const tarjan *t, const upb_refcounted *r) {
  upb_value v;
  return upb_inttable_lookupptr(&t->objattr, r, &v) ?
      upb_value_getuint64(v) : 0;
}

static uint64_t getattr(const tarjan *t, const upb_refcounted *r) {
  upb_value v;
  bool found = upb_inttable_lookupptr(&t->objattr, r, &v);
  UPB_ASSERT_VAR(found, found);
  return upb_value_getuint64(v);
}

static void setattr(tarjan *t, const upb_refcounted *r, uint64_t attr) {
  upb_inttable_removeptr(&t->objattr, r, NULL);
  upb_inttable_insertptr(&t->objattr, r, upb_value_uint64(attr));
}

static color_t color(tarjan *t, const upb_refcounted *r) {
  return trygetattr(t, r) & 0x3;  /* Color is always stored in the low 2 bits. */
}

static void set_gray(tarjan *t, const upb_refcounted *r) {
  assert(color(t, r) == BLACK);
  setattr(t, r, GRAY);
}

/* Pushes an obj onto the Tarjan stack and sets it to GREEN. */
static void push(tarjan *t, const upb_refcounted *r) {
  assert(color(t, r) == BLACK || color(t, r) == GRAY);
  /* This defines the attr layout for the GREEN state.  "index" and "lowlink"
   * get 31 bits, which is plenty (limit of 2B objects frozen at a time). */
  setattr(t, r, GREEN | (t->index << 2) | (t->index << 33));
  if (++t->index == 0x80000000) {
    upb_status_seterrmsg(t->status, "too many objects to freeze");
    err(t);
  }
  upb_inttable_push(&t->stack, upb_value_ptr((void*)r));
}

/* Pops an obj from the Tarjan stack and sets it to WHITE, with a ptr to its
 * SCC group. */
static upb_refcounted *pop(tarjan *t) {
  upb_refcounted *r = upb_value_getptr(upb_inttable_pop(&t->stack));
  assert(color(t, r) == GREEN);
  /* This defines the attr layout for nodes in the WHITE state.
   * Top of group stack is [group, NULL]; we point at group. */
  setattr(t, r, WHITE | (upb_inttable_count(&t->groups) - 2) << 8);
  return r;
}

static void tarjan_newgroup(tarjan *t) {
  uint32_t *group = malloc(sizeof(*group));
  if (!group) oom(t);
  /* Push group and empty group leader (we'll fill in leader later). */
  if (!upb_inttable_push(&t->groups, upb_value_ptr(group)) ||
      !upb_inttable_push(&t->groups, upb_value_ptr(NULL))) {
    free(group);
    oom(t);
  }
  *group = 0;
}

static uint32_t idx(tarjan *t, const upb_refcounted *r) {
  assert(color(t, r) == GREEN);
  return (getattr(t, r) >> 2) & 0x7FFFFFFF;
}

static uint32_t lowlink(tarjan *t, const upb_refcounted *r) {
  if (color(t, r) == GREEN) {
    return getattr(t, r) >> 33;
  } else {
    return UINT32_MAX;
  }
}

static void set_lowlink(tarjan *t, const upb_refcounted *r, uint32_t lowlink) {
  assert(color(t, r) == GREEN);
  setattr(t, r, ((uint64_t)lowlink << 33) | (getattr(t, r) & 0x1FFFFFFFF));
}

static uint32_t *group(tarjan *t, upb_refcounted *r) {
  uint64_t groupnum;
  upb_value v;
  bool found;

  assert(color(t, r) == WHITE);
  groupnum = getattr(t, r) >> 8;
  found = upb_inttable_lookup(&t->groups, groupnum, &v);
  UPB_ASSERT_VAR(found, found);
  return upb_value_getptr(v);
}

/* If the group leader for this object's group has not previously been set,
 * the given object is assigned to be its leader. */
static upb_refcounted *groupleader(tarjan *t, upb_refcounted *r) {
  uint64_t leader_slot;
  upb_value v;
  bool found;

  assert(color(t, r) == WHITE);
  leader_slot = (getattr(t, r) >> 8) + 1;
  found = upb_inttable_lookup(&t->groups, leader_slot, &v);
  UPB_ASSERT_VAR(found, found);
  if (upb_value_getptr(v)) {
    return upb_value_getptr(v);
  } else {
    upb_inttable_remove(&t->groups, leader_slot, NULL);
    upb_inttable_insert(&t->groups, leader_slot, upb_value_ptr(r));
    return r;
  }
}


/* Tarjan's algorithm --------------------------------------------------------*/

/* See:
 *   http://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm */
static void do_tarjan(const upb_refcounted *obj, tarjan *t);

static void tarjan_visit(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure) {
  tarjan *t = closure;
  if (++t->depth > t->maxdepth) {
    upb_status_seterrf(t->status, "graph too deep to freeze (%d)", t->maxdepth);
    err(t);
  } else if (subobj->is_frozen || color(t, subobj) == WHITE) {
    /* Do nothing: we don't want to visit or color already-frozen nodes,
     * and WHITE nodes have already been assigned a SCC. */
  } else if (color(t, subobj) < GREEN) {
    /* Subdef has not yet been visited; recurse on it. */
    do_tarjan(subobj, t);
    set_lowlink(t, obj, UPB_MIN(lowlink(t, obj), lowlink(t, subobj)));
  } else if (color(t, subobj) == GREEN) {
    /* Subdef is in the stack and hence in the current SCC. */
    set_lowlink(t, obj, UPB_MIN(lowlink(t, obj), idx(t, subobj)));
  }
  --t->depth;
}

static void do_tarjan(const upb_refcounted *obj, tarjan *t) {
  if (color(t, obj) == BLACK) {
    /* We haven't seen this object's group; mark the whole group GRAY. */
    const upb_refcounted *o = obj;
    do { set_gray(t, o); } while ((o = o->next) != obj);
  }

  push(t, obj);
  visit(obj, tarjan_visit, t);
  if (lowlink(t, obj) == idx(t, obj)) {
    tarjan_newgroup(t);
    while (pop(t) != obj)
      ;
  }
}


/* freeze() ------------------------------------------------------------------*/

static void crossref(const upb_refcounted *r, const upb_refcounted *subobj,
                     void *_t) {
  tarjan *t = _t;
  assert(color(t, r) > BLACK);
  if (color(t, subobj) > BLACK && r->group != subobj->group) {
    /* Previously this ref was not reflected in subobj->group because they
     * were in the same group; now that they are split a ref must be taken. */
    refgroup(subobj->group);
  }
}

static bool freeze(upb_refcounted *const*roots, int n, upb_status *s,
                   int maxdepth) {
  volatile bool ret = false;
  int i;
  upb_inttable_iter iter;

  /* We run in two passes so that we can allocate all memory before performing
   * any mutation of the input -- this allows us to leave the input unchanged
   * in the case of memory allocation failure. */
  tarjan t;
  t.index = 0;
  t.depth = 0;
  t.maxdepth = maxdepth;
  t.status = s;
  if (!upb_inttable_init(&t.objattr, UPB_CTYPE_UINT64)) goto err1;
  if (!upb_inttable_init(&t.stack, UPB_CTYPE_PTR)) goto err2;
  if (!upb_inttable_init(&t.groups, UPB_CTYPE_PTR)) goto err3;
  if (setjmp(t.err) != 0) goto err4;


  for (i = 0; i < n; i++) {
    if (color(&t, roots[i]) < GREEN) {
      do_tarjan(roots[i], &t);
    }
  }

  /* If we've made it this far, no further errors are possible so it's safe to
   * mutate the objects without risk of leaving them in an inconsistent state. */
  ret = true;

  /* The transformation that follows requires care.  The preconditions are:
   * - all objects in attr map are WHITE or GRAY, and are in mutable groups
   *   (groups of all mutable objs)
   * - no ref2(to, from) refs have incremented count(to) if both "to" and
   *   "from" are in our attr map (this follows from invariants (2) and (3)) */

  /* Pass 1: we remove WHITE objects from their mutable groups, and add them to
   * new groups  according to the SCC's we computed.  These new groups will
   * consist of only frozen objects.  None will be immediately collectible,
   * because WHITE objects are by definition reachable from one of "roots",
   * which the caller must own refs on. */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    /* Since removal from a singly-linked list requires access to the object's
     * predecessor, we consider obj->next instead of obj for moving.  With the
     * while() loop we guarantee that we will visit every node's predecessor.
     * Proof:
     *  1. every node's predecessor is in our attr map.
     *  2. though the loop body may change a node's predecessor, it will only
     *     change it to be the node we are currently operating on, so with a
     *     while() loop we guarantee ourselves the chance to remove each node. */
    while (color(&t, obj->next) == WHITE &&
           group(&t, obj->next) != obj->next->group) {
      upb_refcounted *leader;

      /* Remove from old group. */
      upb_refcounted *move = obj->next;
      if (obj == move) {
        /* Removing the last object from a group. */
        assert(*obj->group == obj->individual_count);
        free(obj->group);
      } else {
        obj->next = move->next;
        /* This may decrease to zero; we'll collect GRAY objects (if any) that
         * remain in the group in the third pass. */
        assert(*move->group >= move->individual_count);
        *move->group -= move->individual_count;
      }

      /* Add to new group. */
      leader = groupleader(&t, move);
      if (move == leader) {
        /* First object added to new group is its leader. */
        move->group = group(&t, move);
        move->next = move;
        *move->group = move->individual_count;
      } else {
        /* Group already has at least one object in it. */
        assert(leader->group == group(&t, move));
        move->group = group(&t, move);
        move->next = leader->next;
        leader->next = move;
        *move->group += move->individual_count;
      }

      move->is_frozen = true;
    }
  }

  /* Pass 2: GRAY and WHITE objects "obj" with ref2(to, obj) references must
   * increment count(to) if group(obj) != group(to) (which could now be the
   * case if "to" was just frozen). */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    visit(obj, crossref, &t);
  }

  /* Pass 3: GRAY objects are collected if their group's refcount dropped to
   * zero when we removed its white nodes.  This can happen if they had only
   * been kept alive by virtue of sharing a group with an object that was just
   * frozen.
   *
   * It is important that we do this last, since the GRAY object's free()
   * function could call unref2() on just-frozen objects, which will decrement
   * refs that were added in pass 2. */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    if (obj->group == NULL || *obj->group == 0) {
      if (obj->group) {
        upb_refcounted *o;

        /* We eagerly free() the group's count (since we can't easily determine
         * the group's remaining size it's the easiest way to ensure it gets
         * done). */
        free(obj->group);

        /* Visit to release ref2's (done in a separate pass since release_ref2
         * depends on o->group being unmodified so it can test merged()). */
        o = obj;
        do { visit(o, release_ref2, NULL); } while ((o = o->next) != obj);

        /* Mark "group" fields as NULL so we know to free the objects later in
         * this loop, but also don't try to delete the group twice. */
        o = obj;
        do { o->group = NULL; } while ((o = o->next) != obj);
      }
      freeobj(obj);
    }
  }

err4:
  if (!ret) {
    upb_inttable_begin(&iter, &t.groups);
    for(; !upb_inttable_done(&iter); upb_inttable_next(&iter))
      free(upb_value_getptr(upb_inttable_iter_value(&iter)));
  }
  upb_inttable_uninit(&t.groups);
err3:
  upb_inttable_uninit(&t.stack);
err2:
  upb_inttable_uninit(&t.objattr);
err1:
  return ret;
}


/* Misc internal functions  ***************************************************/

static bool merged(const upb_refcounted *r, const upb_refcounted *r2) {
  return r->group == r2->group;
}

static void merge(upb_refcounted *r, upb_refcounted *from) {
  upb_refcounted *base;
  upb_refcounted *tmp;

  if (merged(r, from)) return;
  *r->group += *from->group;
  free(from->group);
  base = from;

  /* Set all refcount pointers in the "from" chain to the merged refcount.
   *
   * TODO(haberman): this linear algorithm can result in an overall O(n^2) bound
   * if the user continuously extends a group by one object.  Prevent this by
   * using one of the techniques in this paper:
   *     ftp://www.ncedc.org/outgoing/geomorph/dino/orals/p245-tarjan.pdf */
  do { from->group = r->group; } while ((from = from->next) != base);

  /* Merge the two circularly linked lists by swapping their next pointers. */
  tmp = r->next;
  r->next = base->next;
  base->next = tmp;
}

static void unref(const upb_refcounted *r);

static void release_ref2(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure) {
  UPB_UNUSED(closure);
  untrack(subobj, obj, true);
  if (!merged(obj, subobj)) {
    assert(subobj->is_frozen);
    unref(subobj);
  }
}

static void unref(const upb_refcounted *r) {
  if (unrefgroup(r->group)) {
    const upb_refcounted *o;

    free(r->group);

    /* In two passes, since release_ref2 needs a guarantee that any subobjs
     * are alive. */
    o = r;
    do { visit(o, release_ref2, NULL); } while((o = o->next) != r);

    o = r;
    do {
      const upb_refcounted *next = o->next;
      assert(o->is_frozen || o->individual_count == 0);
      freeobj((upb_refcounted*)o);
      o = next;
    } while(o != r);
  }
}

static void freeobj(upb_refcounted *o) {
  trackfree(o);
  o->vtbl->free((upb_refcounted*)o);
}


/* Public interface ***********************************************************/

bool upb_refcounted_init(upb_refcounted *r,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
#ifndef NDEBUG
  /* Endianness check.  This is unrelated to upb_refcounted, it's just a
   * convenient place to put the check that we can be assured will run for
   * basically every program using upb. */
  const int x = 1;
#ifdef UPB_BIG_ENDIAN
  assert(*(char*)&x != 1);
#else
  assert(*(char*)&x == 1);
#endif
#endif

  r->next = r;
  r->vtbl = vtbl;
  r->individual_count = 0;
  r->is_frozen = false;
  r->group = malloc(sizeof(*r->group));
  if (!r->group) return false;
  *r->group = 0;
  if (!trackinit(r)) {
    free(r->group);
    return false;
  }
  upb_refcounted_ref(r, owner);
  return true;
}

bool upb_refcounted_isfrozen(const upb_refcounted *r) {
  return r->is_frozen;
}

void upb_refcounted_ref(const upb_refcounted *r, const void *owner) {
  track(r, owner, false);
  if (!r->is_frozen)
    ((upb_refcounted*)r)->individual_count++;
  refgroup(r->group);
}

void upb_refcounted_unref(const upb_refcounted *r, const void *owner) {
  untrack(r, owner, false);
  if (!r->is_frozen)
    ((upb_refcounted*)r)->individual_count--;
  unref(r);
}

void upb_refcounted_ref2(const upb_refcounted *r, upb_refcounted *from) {
  assert(!from->is_frozen);  /* Non-const pointer implies this. */
  track(r, from, true);
  if (r->is_frozen) {
    refgroup(r->group);
  } else {
    merge((upb_refcounted*)r, from);
  }
}

void upb_refcounted_unref2(const upb_refcounted *r, upb_refcounted *from) {
  assert(!from->is_frozen);  /* Non-const pointer implies this. */
  untrack(r, from, true);
  if (r->is_frozen) {
    unref(r);
  } else {
    assert(merged(r, from));
  }
}

void upb_refcounted_donateref(
    const upb_refcounted *r, const void *from, const void *to) {
  assert(from != to);
  if (to != NULL)
    upb_refcounted_ref(r, to);
  if (from != NULL)
    upb_refcounted_unref(r, from);
}

void upb_refcounted_checkref(const upb_refcounted *r, const void *owner) {
  checkref(r, owner, false);
}

bool upb_refcounted_freeze(upb_refcounted *const*roots, int n, upb_status *s,
                           int maxdepth) {
  int i;
  for (i = 0; i < n; i++) {
    assert(!roots[i]->is_frozen);
  }
  return freeze(roots, n, s, maxdepth);
}


#include <stdlib.h>

/* Fallback implementation if the shim is not specialized by the JIT. */
#define SHIM_WRITER(type, ctype)                                              \
  bool upb_shim_set ## type (void *c, const void *hd, ctype val) {            \
    uint8_t *m = c;                                                           \
    const upb_shim_data *d = hd;                                              \
    if (d->hasbit > 0)                                                        \
      *(uint8_t*)&m[d->hasbit / 8] |= 1 << (d->hasbit % 8);                   \
    *(ctype*)&m[d->offset] = val;                                             \
    return true;                                                              \
  }                                                                           \

SHIM_WRITER(double, double)
SHIM_WRITER(float,  float)
SHIM_WRITER(int32,  int32_t)
SHIM_WRITER(int64,  int64_t)
SHIM_WRITER(uint32, uint32_t)
SHIM_WRITER(uint64, uint64_t)
SHIM_WRITER(bool,   bool)
#undef SHIM_WRITER

bool upb_shim_set(upb_handlers *h, const upb_fielddef *f, size_t offset,
                  int32_t hasbit) {
  upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
  bool ok;

  upb_shim_data *d = malloc(sizeof(*d));
  if (!d) return false;
  d->offset = offset;
  d->hasbit = hasbit;

  upb_handlerattr_sethandlerdata(&attr, d);
  upb_handlerattr_setalwaysok(&attr, true);
  upb_handlers_addcleanup(h, d, free);

#define TYPE(u, l) \
  case UPB_TYPE_##u: \
    ok = upb_handlers_set##l(h, f, upb_shim_set##l, &attr); break;

  ok = false;

  switch (upb_fielddef_type(f)) {
    TYPE(INT64,  int64);
    TYPE(INT32,  int32);
    TYPE(ENUM,   int32);
    TYPE(UINT64, uint64);
    TYPE(UINT32, uint32);
    TYPE(DOUBLE, double);
    TYPE(FLOAT,  float);
    TYPE(BOOL,   bool);
    default: assert(false); break;
  }
#undef TYPE

  upb_handlerattr_uninit(&attr);
  return ok;
}

const upb_shim_data *upb_shim_getdata(const upb_handlers *h, upb_selector_t s,
                                      upb_fieldtype_t *type) {
  upb_func *f = upb_handlers_gethandler(h, s);

  if ((upb_int64_handlerfunc*)f == upb_shim_setint64) {
    *type = UPB_TYPE_INT64;
  } else if ((upb_int32_handlerfunc*)f == upb_shim_setint32) {
    *type = UPB_TYPE_INT32;
  } else if ((upb_uint64_handlerfunc*)f == upb_shim_setuint64) {
    *type = UPB_TYPE_UINT64;
  } else if ((upb_uint32_handlerfunc*)f == upb_shim_setuint32) {
    *type = UPB_TYPE_UINT32;
  } else if ((upb_double_handlerfunc*)f == upb_shim_setdouble) {
    *type = UPB_TYPE_DOUBLE;
  } else if ((upb_float_handlerfunc*)f == upb_shim_setfloat) {
    *type = UPB_TYPE_FLOAT;
  } else if ((upb_bool_handlerfunc*)f == upb_shim_setbool) {
    *type = UPB_TYPE_BOOL;
  } else {
    return NULL;
  }

  return (const upb_shim_data*)upb_handlers_gethandlerdata(h, s);
}


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

/* Starts a depth-first traversal at "def", recursing into any subdefs
 * (ie. submessage types).  Adds duplicates of existing defs to addtab
 * wherever necessary, so that the resulting symtab will be consistent once
 * addtab is added.
 *
 * More specifically, if any def D is found in the DFS that:
 *
 *   1. can reach a def that is being replaced by something in addtab, AND
 *
 *   2. is not itself being replaced already (ie. this name doesn't already
 *      exist in addtab)
 *
 * ...then a duplicate (new copy) of D will be added to addtab.
 *
 * Returns true if this happened for any def reachable from "def."
 *
 * It is slightly tricky to do this correctly in the presence of cycles.  If we
 * detect that our DFS has hit a cycle, we might not yet know if any SCCs on
 * our stack can reach a def in addtab or not.  Once we figure this out, that
 * answer needs to apply to *all* defs in these SCCs, even if we visited them
 * already.  So a straight up one-pass cycle-detecting DFS won't work.
 *
 * To work around this problem, we traverse each SCC (which we already
 * computed, since these defs are frozen) as a single node.  We first compute
 * whether the SCC as a whole can reach any def in addtab, then we dup (or not)
 * the entire SCC.  This requires breaking the encapsulation of upb_refcounted,
 * since that is where we get the data about what SCC we are in. */
static bool upb_resolve_dfs(const upb_def *def, upb_strtable *addtab,
                            const void *new_owner, upb_inttable *seen,
                            upb_status *s) {
  upb_value v;
  bool need_dup;
  const upb_def *base;
  const void* memoize_key;

  /* Memoize results of this function for efficiency (since we're traversing a
   * DAG this is not needed to limit the depth of the search).
   *
   * We memoize by SCC instead of by individual def. */
  memoize_key = def->base.group;

  if (upb_inttable_lookupptr(seen, memoize_key, &v))
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

  upb_inttable_insertptr(seen, memoize_key, upb_value_bool(need_dup));
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
/*
** upb_table Implementation
**
** Implementation is heavily inspired by Lua's ltable.c.
*/


#include <stdlib.h>
#include <string.h>

#define UPB_MAXARRSIZE 16  /* 64k. */

/* From Chromium. */
#define ARRAY_SIZE(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static const double MAX_LOAD = 0.85;

/* The minimum utilization of the array part of a mixed hash/array table.  This
 * is a speed/memory-usage tradeoff (though it's not straightforward because of
 * cache effects).  The lower this is, the more memory we'll use. */
static const double MIN_DENSITY = 0.1;

bool is_pow2(uint64_t v) { return v == 0 || (v & (v - 1)) == 0; }

int log2ceil(uint64_t v) {
  int ret = 0;
  bool pow2 = is_pow2(v);
  while (v >>= 1) ret++;
  ret = pow2 ? ret : ret + 1;  /* Ceiling. */
  return UPB_MIN(UPB_MAXARRSIZE, ret);
}

char *upb_strdup(const char *s) {
  return upb_strdup2(s, strlen(s));
}

char *upb_strdup2(const char *s, size_t len) {
  size_t n;
  char *p;

  /* Prevent overflow errors. */
  if (len == SIZE_MAX) return NULL;
  /* Always null-terminate, even if binary data; but don't rely on the input to
   * have a null-terminating byte since it may be a raw binary buffer. */
  n = len + 1;
  p = malloc(n);
  if (p) {
    memcpy(p, s, len);
    p[len] = 0;
  }
  return p;
}

/* A type to represent the lookup key of either a strtable or an inttable. */
typedef union {
  uintptr_t num;
  struct {
    const char *str;
    size_t len;
  } str;
} lookupkey_t;

static lookupkey_t strkey2(const char *str, size_t len) {
  lookupkey_t k;
  k.str.str = str;
  k.str.len = len;
  return k;
}

static lookupkey_t intkey(uintptr_t key) {
  lookupkey_t k;
  k.num = key;
  return k;
}

typedef uint32_t hashfunc_t(upb_tabkey key);
typedef bool eqlfunc_t(upb_tabkey k1, lookupkey_t k2);

/* Base table (shared code) ***************************************************/

/* For when we need to cast away const. */
static upb_tabent *mutable_entries(upb_table *t) {
  return (upb_tabent*)t->entries;
}

static bool isfull(upb_table *t) {
  return (double)(t->count + 1) / upb_table_size(t) > MAX_LOAD;
}

static bool init(upb_table *t, upb_ctype_t ctype, uint8_t size_lg2) {
  size_t bytes;

  t->count = 0;
  t->ctype = ctype;
  t->size_lg2 = size_lg2;
  t->mask = upb_table_size(t) ? upb_table_size(t) - 1 : 0;
  bytes = upb_table_size(t) * sizeof(upb_tabent);
  if (bytes > 0) {
    t->entries = malloc(bytes);
    if (!t->entries) return false;
    memset(mutable_entries(t), 0, bytes);
  } else {
    t->entries = NULL;
  }
  return true;
}

static void uninit(upb_table *t) { free(mutable_entries(t)); }

static upb_tabent *emptyent(upb_table *t) {
  upb_tabent *e = mutable_entries(t) + upb_table_size(t);
  while (1) { if (upb_tabent_isempty(--e)) return e; assert(e > t->entries); }
}

static upb_tabent *getentry_mutable(upb_table *t, uint32_t hash) {
  return (upb_tabent*)upb_getentry(t, hash);
}

static const upb_tabent *findentry(const upb_table *t, lookupkey_t key,
                                   uint32_t hash, eqlfunc_t *eql) {
  const upb_tabent *e;

  if (t->size_lg2 == 0) return NULL;
  e = upb_getentry(t, hash);
  if (upb_tabent_isempty(e)) return NULL;
  while (1) {
    if (eql(e->key, key)) return e;
    if ((e = e->next) == NULL) return NULL;
  }
}

static upb_tabent *findentry_mutable(upb_table *t, lookupkey_t key,
                                     uint32_t hash, eqlfunc_t *eql) {
  return (upb_tabent*)findentry(t, key, hash, eql);
}

static bool lookup(const upb_table *t, lookupkey_t key, upb_value *v,
                   uint32_t hash, eqlfunc_t *eql) {
  const upb_tabent *e = findentry(t, key, hash, eql);
  if (e) {
    if (v) {
      _upb_value_setval(v, e->val.val, t->ctype);
    }
    return true;
  } else {
    return false;
  }
}

/* The given key must not already exist in the table. */
static void insert(upb_table *t, lookupkey_t key, upb_tabkey tabkey,
                   upb_value val, uint32_t hash,
                   hashfunc_t *hashfunc, eqlfunc_t *eql) {
  upb_tabent *mainpos_e;
  upb_tabent *our_e;

  UPB_UNUSED(eql);
  UPB_UNUSED(key);
  assert(findentry(t, key, hash, eql) == NULL);
  assert(val.ctype == t->ctype);

  t->count++;
  mainpos_e = getentry_mutable(t, hash);
  our_e = mainpos_e;

  if (upb_tabent_isempty(mainpos_e)) {
    /* Our main position is empty; use it. */
    our_e->next = NULL;
  } else {
    /* Collision. */
    upb_tabent *new_e = emptyent(t);
    /* Head of collider's chain. */
    upb_tabent *chain = getentry_mutable(t, hashfunc(mainpos_e->key));
    if (chain == mainpos_e) {
      /* Existing ent is in its main posisiton (it has the same hash as us, and
       * is the head of our chain).  Insert to new ent and append to this chain. */
      new_e->next = mainpos_e->next;
      mainpos_e->next = new_e;
      our_e = new_e;
    } else {
      /* Existing ent is not in its main position (it is a node in some other
       * chain).  This implies that no existing ent in the table has our hash.
       * Evict it (updating its chain) and use its ent for head of our chain. */
      *new_e = *mainpos_e;  /* copies next. */
      while (chain->next != mainpos_e) {
        chain = (upb_tabent*)chain->next;
        assert(chain);
      }
      chain->next = new_e;
      our_e = mainpos_e;
      our_e->next = NULL;
    }
  }
  our_e->key = tabkey;
  our_e->val.val = val.val;
  assert(findentry(t, key, hash, eql) == our_e);
}

static bool rm(upb_table *t, lookupkey_t key, upb_value *val,
               upb_tabkey *removed, uint32_t hash, eqlfunc_t *eql) {
  upb_tabent *chain = getentry_mutable(t, hash);
  if (upb_tabent_isempty(chain)) return false;
  if (eql(chain->key, key)) {
    /* Element to remove is at the head of its chain. */
    t->count--;
    if (val) {
      _upb_value_setval(val, chain->val.val, t->ctype);
    }
    if (chain->next) {
      upb_tabent *move = (upb_tabent*)chain->next;
      *chain = *move;
      if (removed) *removed = move->key;
      move->key = 0;  /* Make the slot empty. */
    } else {
      if (removed) *removed = chain->key;
      chain->key = 0;  /* Make the slot empty. */
    }
    return true;
  } else {
    /* Element to remove is either in a non-head position or not in the
     * table. */
    while (chain->next && !eql(chain->next->key, key))
      chain = (upb_tabent*)chain->next;
    if (chain->next) {
      /* Found element to remove. */
      upb_tabent *rm;

      if (val) {
        _upb_value_setval(val, chain->next->val.val, t->ctype);
      }
      rm = (upb_tabent*)chain->next;
      if (removed) *removed = rm->key;
      rm->key = 0;
      chain->next = rm->next;
      t->count--;
      return true;
    } else {
      return false;
    }
  }
}

static size_t next(const upb_table *t, size_t i) {
  do {
    if (++i >= upb_table_size(t))
      return SIZE_MAX;
  } while(upb_tabent_isempty(&t->entries[i]));

  return i;
}

static size_t begin(const upb_table *t) {
  return next(t, -1);
}


/* upb_strtable ***************************************************************/

/* A simple "subclass" of upb_table that only adds a hash function for strings. */

static upb_tabkey strcopy(lookupkey_t k2) {
  char *str = malloc(k2.str.len + sizeof(uint32_t) + 1);
  if (str == NULL) return 0;
  memcpy(str, &k2.str.len, sizeof(uint32_t));
  memcpy(str + sizeof(uint32_t), k2.str.str, k2.str.len + 1);
  return (uintptr_t)str;
}

static uint32_t strhash(upb_tabkey key) {
  uint32_t len;
  char *str = upb_tabstr(key, &len);
  return MurmurHash2(str, len, 0);
}

static bool streql(upb_tabkey k1, lookupkey_t k2) {
  uint32_t len;
  char *str = upb_tabstr(k1, &len);
  return len == k2.str.len && memcmp(str, k2.str.str, len) == 0;
}

bool upb_strtable_init(upb_strtable *t, upb_ctype_t ctype) {
  return init(&t->t, ctype, 2);
}

void upb_strtable_uninit(upb_strtable *t) {
  size_t i;
  for (i = 0; i < upb_table_size(&t->t); i++)
    free((void*)t->t.entries[i].key);
  uninit(&t->t);
}

bool upb_strtable_resize(upb_strtable *t, size_t size_lg2) {
  upb_strtable new_table;
  upb_strtable_iter i;

  if (!init(&new_table.t, t->t.ctype, size_lg2))
    return false;
  upb_strtable_begin(&i, t);
  for ( ; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_strtable_insert2(
        &new_table,
        upb_strtable_iter_key(&i),
        upb_strtable_iter_keylength(&i),
        upb_strtable_iter_value(&i));
  }
  upb_strtable_uninit(t);
  *t = new_table;
  return true;
}

bool upb_strtable_insert2(upb_strtable *t, const char *k, size_t len,
                          upb_value v) {
  lookupkey_t key;
  upb_tabkey tabkey;
  uint32_t hash;

  if (isfull(&t->t)) {
    /* Need to resize.  New table of double the size, add old elements to it. */
    if (!upb_strtable_resize(t, t->t.size_lg2 + 1)) {
      return false;
    }
  }

  key = strkey2(k, len);
  tabkey = strcopy(key);
  if (tabkey == 0) return false;

  hash = MurmurHash2(key.str.str, key.str.len, 0);
  insert(&t->t, key, tabkey, v, hash, &strhash, &streql);
  return true;
}

bool upb_strtable_lookup2(const upb_strtable *t, const char *key, size_t len,
                          upb_value *v) {
  uint32_t hash = MurmurHash2(key, len, 0);
  return lookup(&t->t, strkey2(key, len), v, hash, &streql);
}

bool upb_strtable_remove2(upb_strtable *t, const char *key, size_t len,
                         upb_value *val) {
  uint32_t hash = MurmurHash2(key, strlen(key), 0);
  upb_tabkey tabkey;
  if (rm(&t->t, strkey2(key, len), val, &tabkey, hash, &streql)) {
    free((void*)tabkey);
    return true;
  } else {
    return false;
  }
}

/* Iteration */

static const upb_tabent *str_tabent(const upb_strtable_iter *i) {
  return &i->t->t.entries[i->index];
}

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t) {
  i->t = t;
  i->index = begin(&t->t);
}

void upb_strtable_next(upb_strtable_iter *i) {
  i->index = next(&i->t->t, i->index);
}

bool upb_strtable_done(const upb_strtable_iter *i) {
  return i->index >= upb_table_size(&i->t->t) ||
         upb_tabent_isempty(str_tabent(i));
}

const char *upb_strtable_iter_key(upb_strtable_iter *i) {
  assert(!upb_strtable_done(i));
  return upb_tabstr(str_tabent(i)->key, NULL);
}

size_t upb_strtable_iter_keylength(upb_strtable_iter *i) {
  uint32_t len;
  assert(!upb_strtable_done(i));
  upb_tabstr(str_tabent(i)->key, &len);
  return len;
}

upb_value upb_strtable_iter_value(const upb_strtable_iter *i) {
  assert(!upb_strtable_done(i));
  return _upb_value_val(str_tabent(i)->val.val, i->t->t.ctype);
}

void upb_strtable_iter_setdone(upb_strtable_iter *i) {
  i->index = SIZE_MAX;
}

bool upb_strtable_iter_isequal(const upb_strtable_iter *i1,
                               const upb_strtable_iter *i2) {
  if (upb_strtable_done(i1) && upb_strtable_done(i2))
    return true;
  return i1->t == i2->t && i1->index == i2->index;
}


/* upb_inttable ***************************************************************/

/* For inttables we use a hybrid structure where small keys are kept in an
 * array and large keys are put in the hash table. */

static uint32_t inthash(upb_tabkey key) { return upb_inthash(key); }

static bool inteql(upb_tabkey k1, lookupkey_t k2) {
  return k1 == k2.num;
}

static upb_tabval *mutable_array(upb_inttable *t) {
  return (upb_tabval*)t->array;
}

static upb_tabval *inttable_val(upb_inttable *t, uintptr_t key) {
  if (key < t->array_size) {
    return upb_arrhas(t->array[key]) ? &(mutable_array(t)[key]) : NULL;
  } else {
    upb_tabent *e =
        findentry_mutable(&t->t, intkey(key), upb_inthash(key), &inteql);
    return e ? &e->val : NULL;
  }
}

static const upb_tabval *inttable_val_const(const upb_inttable *t,
                                            uintptr_t key) {
  return inttable_val((upb_inttable*)t, key);
}

size_t upb_inttable_count(const upb_inttable *t) {
  return t->t.count + t->array_count;
}

static void check(upb_inttable *t) {
  UPB_UNUSED(t);
#if defined(UPB_DEBUG_TABLE) && !defined(NDEBUG)
  {
    /* This check is very expensive (makes inserts/deletes O(N)). */
    size_t count = 0;
    upb_inttable_iter i;
    upb_inttable_begin(&i, t);
    for(; !upb_inttable_done(&i); upb_inttable_next(&i), count++) {
      assert(upb_inttable_lookup(t, upb_inttable_iter_key(&i), NULL));
    }
    assert(count == upb_inttable_count(t));
  }
#endif
}

bool upb_inttable_sizedinit(upb_inttable *t, upb_ctype_t ctype,
                            size_t asize, int hsize_lg2) {
  size_t array_bytes;

  if (!init(&t->t, ctype, hsize_lg2)) return false;
  /* Always make the array part at least 1 long, so that we know key 0
   * won't be in the hash part, which simplifies things. */
  t->array_size = UPB_MAX(1, asize);
  t->array_count = 0;
  array_bytes = t->array_size * sizeof(upb_value);
  t->array = malloc(array_bytes);
  if (!t->array) {
    uninit(&t->t);
    return false;
  }
  memset(mutable_array(t), 0xff, array_bytes);
  check(t);
  return true;
}

bool upb_inttable_init(upb_inttable *t, upb_ctype_t ctype) {
  return upb_inttable_sizedinit(t, ctype, 0, 4);
}

void upb_inttable_uninit(upb_inttable *t) {
  uninit(&t->t);
  free(mutable_array(t));
}

bool upb_inttable_insert(upb_inttable *t, uintptr_t key, upb_value val) {
  /* XXX: Table can't store value (uint64_t)-1.  Need to somehow statically
   * guarantee that this is not necessary, or fix the limitation. */
  upb_tabval tabval;
  tabval.val = val.val;
  UPB_UNUSED(tabval);
  assert(upb_arrhas(tabval));

  if (key < t->array_size) {
    assert(!upb_arrhas(t->array[key]));
    t->array_count++;
    mutable_array(t)[key].val = val.val;
  } else {
    if (isfull(&t->t)) {
      /* Need to resize the hash part, but we re-use the array part. */
      size_t i;
      upb_table new_table;
      if (!init(&new_table, t->t.ctype, t->t.size_lg2 + 1))
        return false;
      for (i = begin(&t->t); i < upb_table_size(&t->t); i = next(&t->t, i)) {
        const upb_tabent *e = &t->t.entries[i];
        uint32_t hash;
        upb_value v;

        _upb_value_setval(&v, e->val.val, t->t.ctype);
        hash = upb_inthash(e->key);
        insert(&new_table, intkey(e->key), e->key, v, hash, &inthash, &inteql);
      }

      assert(t->t.count == new_table.count);

      uninit(&t->t);
      t->t = new_table;
    }
    insert(&t->t, intkey(key), key, val, upb_inthash(key), &inthash, &inteql);
  }
  check(t);
  return true;
}

bool upb_inttable_lookup(const upb_inttable *t, uintptr_t key, upb_value *v) {
  const upb_tabval *table_v = inttable_val_const(t, key);
  if (!table_v) return false;
  if (v) _upb_value_setval(v, table_v->val, t->t.ctype);
  return true;
}

bool upb_inttable_replace(upb_inttable *t, uintptr_t key, upb_value val) {
  upb_tabval *table_v = inttable_val(t, key);
  if (!table_v) return false;
  table_v->val = val.val;
  return true;
}

bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val) {
  bool success;
  if (key < t->array_size) {
    if (upb_arrhas(t->array[key])) {
      upb_tabval empty = UPB_TABVALUE_EMPTY_INIT;
      t->array_count--;
      if (val) {
        _upb_value_setval(val, t->array[key].val, t->t.ctype);
      }
      mutable_array(t)[key] = empty;
      success = true;
    } else {
      success = false;
    }
  } else {
    upb_tabkey removed;
    uint32_t hash = upb_inthash(key);
    success = rm(&t->t, intkey(key), val, &removed, hash, &inteql);
  }
  check(t);
  return success;
}

bool upb_inttable_push(upb_inttable *t, upb_value val) {
  return upb_inttable_insert(t, upb_inttable_count(t), val);
}

upb_value upb_inttable_pop(upb_inttable *t) {
  upb_value val;
  bool ok = upb_inttable_remove(t, upb_inttable_count(t) - 1, &val);
  UPB_ASSERT_VAR(ok, ok);
  return val;
}

bool upb_inttable_insertptr(upb_inttable *t, const void *key, upb_value val) {
  return upb_inttable_insert(t, (uintptr_t)key, val);
}

bool upb_inttable_lookupptr(const upb_inttable *t, const void *key,
                            upb_value *v) {
  return upb_inttable_lookup(t, (uintptr_t)key, v);
}

bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val) {
  return upb_inttable_remove(t, (uintptr_t)key, val);
}

void upb_inttable_compact(upb_inttable *t) {
  /* Create a power-of-two histogram of the table keys. */
  int counts[UPB_MAXARRSIZE + 1] = {0};
  uintptr_t max_key = 0;
  upb_inttable_iter i;
  size_t arr_size;
  int arr_count;
  upb_inttable new_t;

  upb_inttable_begin(&i, t);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    uintptr_t key = upb_inttable_iter_key(&i);
    if (key > max_key) {
      max_key = key;
    }
    counts[log2ceil(key)]++;
  }

  arr_size = 1;
  arr_count = upb_inttable_count(t);

  if (upb_inttable_count(t) >= max_key * MIN_DENSITY) {
    /* We can put 100% of the entries in the array part. */
    arr_size = max_key + 1;
  } else {
    /* Find the largest power of two that satisfies the MIN_DENSITY
     * definition. */
    int size_lg2;
    for (size_lg2 = ARRAY_SIZE(counts) - 1; size_lg2 > 1; size_lg2--) {
      arr_size = 1 << size_lg2;
      arr_count -= counts[size_lg2];
      if (arr_count >= arr_size * MIN_DENSITY) {
        break;
      }
    }
  }

  /* Array part must always be at least 1 entry large to catch lookups of key
   * 0.  Key 0 must always be in the array part because "0" in the hash part
   * denotes an empty entry. */
  arr_size = UPB_MAX(arr_size, 1);

  {
    /* Insert all elements into new, perfectly-sized table. */
    int hash_count = upb_inttable_count(t) - arr_count;
    int hash_size = hash_count ? (hash_count / MAX_LOAD) + 1 : 0;
    int hashsize_lg2 = log2ceil(hash_size);

    assert(hash_count >= 0);
    upb_inttable_sizedinit(&new_t, t->t.ctype, arr_size, hashsize_lg2);
    upb_inttable_begin(&i, t);
    for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
      uintptr_t k = upb_inttable_iter_key(&i);
      upb_inttable_insert(&new_t, k, upb_inttable_iter_value(&i));
    }
    assert(new_t.array_size == arr_size);
    assert(new_t.t.size_lg2 == hashsize_lg2);
  }
  upb_inttable_uninit(t);
  *t = new_t;
}

/* Iteration. */

static const upb_tabent *int_tabent(const upb_inttable_iter *i) {
  assert(!i->array_part);
  return &i->t->t.entries[i->index];
}

static upb_tabval int_arrent(const upb_inttable_iter *i) {
  assert(i->array_part);
  return i->t->array[i->index];
}

void upb_inttable_begin(upb_inttable_iter *i, const upb_inttable *t) {
  i->t = t;
  i->index = -1;
  i->array_part = true;
  upb_inttable_next(i);
}

void upb_inttable_next(upb_inttable_iter *iter) {
  const upb_inttable *t = iter->t;
  if (iter->array_part) {
    while (++iter->index < t->array_size) {
      if (upb_arrhas(int_arrent(iter))) {
        return;
      }
    }
    iter->array_part = false;
    iter->index = begin(&t->t);
  } else {
    iter->index = next(&t->t, iter->index);
  }
}

bool upb_inttable_done(const upb_inttable_iter *i) {
  if (i->array_part) {
    return i->index >= i->t->array_size ||
           !upb_arrhas(int_arrent(i));
  } else {
    return i->index >= upb_table_size(&i->t->t) ||
           upb_tabent_isempty(int_tabent(i));
  }
}

uintptr_t upb_inttable_iter_key(const upb_inttable_iter *i) {
  assert(!upb_inttable_done(i));
  return i->array_part ? i->index : int_tabent(i)->key;
}

upb_value upb_inttable_iter_value(const upb_inttable_iter *i) {
  assert(!upb_inttable_done(i));
  return _upb_value_val(
      i->array_part ? i->t->array[i->index].val : int_tabent(i)->val.val,
      i->t->t.ctype);
}

void upb_inttable_iter_setdone(upb_inttable_iter *i) {
  i->index = SIZE_MAX;
  i->array_part = false;
}

bool upb_inttable_iter_isequal(const upb_inttable_iter *i1,
                                          const upb_inttable_iter *i2) {
  if (upb_inttable_done(i1) && upb_inttable_done(i2))
    return true;
  return i1->t == i2->t && i1->index == i2->index &&
         i1->array_part == i2->array_part;
}

#ifdef UPB_UNALIGNED_READS_OK
/* -----------------------------------------------------------------------------
 * MurmurHash2, by Austin Appleby (released as public domain).
 * Reformatted and C99-ified by Joshua Haberman.
 * Note - This code makes a few assumptions about how your machine behaves -
 *   1. We can read a 4-byte value from any address without crashing
 *   2. sizeof(int) == 4 (in upb this limitation is removed by using uint32_t
 * And it has a few limitations -
 *   1. It will not work incrementally.
 *   2. It will not produce the same results on little-endian and big-endian
 *      machines. */
uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed) {
  /* 'm' and 'r' are mixing constants generated offline.
   * They're not really 'magic', they just happen to work well. */
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;

  /* Initialize the hash to a 'random' value */
  uint32_t h = seed ^ len;

  /* Mix 4 bytes at a time into the hash */
  const uint8_t * data = (const uint8_t *)key;
  while(len >= 4) {
    uint32_t k = *(uint32_t *)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  /* Handle the last few bytes of the input array */
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
  };

  /* Do a few final mixes of the hash to ensure the last few
   * bytes are well-incorporated. */
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

#else /* !UPB_UNALIGNED_READS_OK */

/* -----------------------------------------------------------------------------
 * MurmurHashAligned2, by Austin Appleby
 * Same algorithm as MurmurHash2, but only does aligned reads - should be safer
 * on certain platforms.
 * Performance will be lower than MurmurHash2 */

#define MIX(h,k,m) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

uint32_t MurmurHash2(const void * key, size_t len, uint32_t seed) {
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;
  const uint8_t * data = (const uint8_t *)key;
  uint32_t h = seed ^ len;
  uint8_t align = (uintptr_t)data & 3;

  if(align && (len >= 4)) {
    /* Pre-load the temp registers */
    uint32_t t = 0, d = 0;
    int32_t sl;
    int32_t sr;

    switch(align) {
      case 1: t |= data[2] << 16;
      case 2: t |= data[1] << 8;
      case 3: t |= data[0];
    }

    t <<= (8 * align);

    data += 4-align;
    len -= 4-align;

    sl = 8 * (4-align);
    sr = 8 * align;

    /* Mix */

    while(len >= 4) {
      uint32_t k;

      d = *(uint32_t *)data;
      t = (t >> sr) | (d << sl);

      k = t;

      MIX(h,k,m);

      t = d;

      data += 4;
      len -= 4;
    }

    /* Handle leftover data in temp registers */

    d = 0;

    if(len >= align) {
      uint32_t k;

      switch(align) {
        case 3: d |= data[2] << 16;
        case 2: d |= data[1] << 8;
        case 1: d |= data[0];
      }

      k = (t >> sr) | (d << sl);
      MIX(h,k,m);

      data += align;
      len -= align;

      /* ----------
       * Handle tail bytes */

      switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0]; h *= m;
      };
    } else {
      switch(len) {
        case 3: d |= data[2] << 16;
        case 2: d |= data[1] << 8;
        case 1: d |= data[0];
        case 0: h ^= (t >> sr) | (d << sl); h *= m;
      }
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  } else {
    while(len >= 4) {
      uint32_t k = *(uint32_t *)data;

      MIX(h,k,m);

      data += 4;
      len -= 4;
    }

    /* ----------
     * Handle tail bytes */

    switch(len) {
      case 3: h ^= data[2] << 16;
      case 2: h ^= data[1] << 8;
      case 1: h ^= data[0]; h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }
}
#undef MIX

#endif /* UPB_UNALIGNED_READS_OK */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool upb_dumptostderr(void *closure, const upb_status* status) {
  UPB_UNUSED(closure);
  fprintf(stderr, "%s\n", upb_status_errmsg(status));
  return false;
}

/* Guarantee null-termination and provide ellipsis truncation.
 * It may be tempting to "optimize" this by initializing these final
 * four bytes up-front and then being careful never to overwrite them,
 * this is safer and simpler. */
static void nullz(upb_status *status) {
  const char *ellipsis = "...";
  size_t len = strlen(ellipsis);
  assert(sizeof(status->msg) > len);
  memcpy(status->msg + sizeof(status->msg) - len, ellipsis, len);
}

void upb_status_clear(upb_status *status) {
  if (!status) return;
  status->ok_ = true;
  status->code_ = 0;
  status->msg[0] = '\0';
}

bool upb_ok(const upb_status *status) { return status->ok_; }

upb_errorspace *upb_status_errspace(const upb_status *status) {
  return status->error_space_;
}

int upb_status_errcode(const upb_status *status) { return status->code_; }

const char *upb_status_errmsg(const upb_status *status) { return status->msg; }

void upb_status_seterrmsg(upb_status *status, const char *msg) {
  if (!status) return;
  status->ok_ = false;
  strncpy(status->msg, msg, sizeof(status->msg));
  nullz(status);
}

void upb_status_seterrf(upb_status *status, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_status_vseterrf(status, fmt, args);
  va_end(args);
}

void upb_status_vseterrf(upb_status *status, const char *fmt, va_list args) {
  if (!status) return;
  status->ok_ = false;
  _upb_vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  nullz(status);
}

void upb_status_seterrcode(upb_status *status, upb_errorspace *space,
                           int code) {
  if (!status) return;
  status->ok_ = false;
  status->error_space_ = space;
  status->code_ = code;
  space->set_message(status, code);
}

void upb_status_copy(upb_status *to, const upb_status *from) {
  if (!to) return;
  *to = *from;
}
/* This file was generated by upbc (the upb compiler).
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */


static const upb_msgdef msgs[20];
static const upb_fielddef fields[81];
static const upb_enumdef enums[4];
static const upb_tabent strentries[236];
static const upb_tabent intentries[14];
static const upb_tabval arrays[232];

#ifdef UPB_DEBUG_REFS
static upb_inttable reftables[212];
#endif

static const upb_msgdef msgs[20] = {
  UPB_MSGDEF_INIT("google.protobuf.DescriptorProto", 27, 6, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[0], 8, 7), UPB_STRTABLE_INIT(7, 15, UPB_CTYPE_PTR, 4, &strentries[0]),&reftables[0], &reftables[1]),
  UPB_MSGDEF_INIT("google.protobuf.DescriptorProto.ExtensionRange", 4, 0, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[8], 3, 2), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[16]),&reftables[2], &reftables[3]),
  UPB_MSGDEF_INIT("google.protobuf.EnumDescriptorProto", 11, 2, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[11], 4, 3), UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_PTR, 2, &strentries[20]),&reftables[4], &reftables[5]),
  UPB_MSGDEF_INIT("google.protobuf.EnumOptions", 7, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[0], &arrays[15], 8, 1), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[24]),&reftables[6], &reftables[7]),
  UPB_MSGDEF_INIT("google.protobuf.EnumValueDescriptorProto", 8, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[23], 4, 3), UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_PTR, 2, &strentries[28]),&reftables[8], &reftables[9]),
  UPB_MSGDEF_INIT("google.protobuf.EnumValueOptions", 6, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[2], &arrays[27], 4, 0), UPB_STRTABLE_INIT(1, 3, UPB_CTYPE_PTR, 2, &strentries[32]),&reftables[10], &reftables[11]),
  UPB_MSGDEF_INIT("google.protobuf.FieldDescriptorProto", 19, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[31], 9, 8), UPB_STRTABLE_INIT(8, 15, UPB_CTYPE_PTR, 4, &strentries[36]),&reftables[12], &reftables[13]),
  UPB_MSGDEF_INIT("google.protobuf.FieldOptions", 14, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[4], &arrays[40], 32, 6), UPB_STRTABLE_INIT(7, 15, UPB_CTYPE_PTR, 4, &strentries[52]),&reftables[14], &reftables[15]),
  UPB_MSGDEF_INIT("google.protobuf.FileDescriptorProto", 39, 6, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[72], 12, 11), UPB_STRTABLE_INIT(11, 15, UPB_CTYPE_PTR, 4, &strentries[68]),&reftables[16], &reftables[17]),
  UPB_MSGDEF_INIT("google.protobuf.FileDescriptorSet", 6, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[84], 2, 1), UPB_STRTABLE_INIT(1, 3, UPB_CTYPE_PTR, 2, &strentries[84]),&reftables[18], &reftables[19]),
  UPB_MSGDEF_INIT("google.protobuf.FileOptions", 21, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[6], &arrays[86], 64, 9), UPB_STRTABLE_INIT(10, 15, UPB_CTYPE_PTR, 4, &strentries[88]),&reftables[20], &reftables[21]),
  UPB_MSGDEF_INIT("google.protobuf.MessageOptions", 8, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[8], &arrays[150], 16, 2), UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_PTR, 2, &strentries[104]),&reftables[22], &reftables[23]),
  UPB_MSGDEF_INIT("google.protobuf.MethodDescriptorProto", 13, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[166], 5, 4), UPB_STRTABLE_INIT(4, 7, UPB_CTYPE_PTR, 3, &strentries[108]),&reftables[24], &reftables[25]),
  UPB_MSGDEF_INIT("google.protobuf.MethodOptions", 6, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[10], &arrays[171], 4, 0), UPB_STRTABLE_INIT(1, 3, UPB_CTYPE_PTR, 2, &strentries[116]),&reftables[26], &reftables[27]),
  UPB_MSGDEF_INIT("google.protobuf.ServiceDescriptorProto", 11, 2, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[175], 4, 3), UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_PTR, 2, &strentries[120]),&reftables[28], &reftables[29]),
  UPB_MSGDEF_INIT("google.protobuf.ServiceOptions", 6, 1, UPB_INTTABLE_INIT(1, 1, UPB_CTYPE_PTR, 1, &intentries[12], &arrays[179], 4, 0), UPB_STRTABLE_INIT(1, 3, UPB_CTYPE_PTR, 2, &strentries[124]),&reftables[30], &reftables[31]),
  UPB_MSGDEF_INIT("google.protobuf.SourceCodeInfo", 6, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[183], 2, 1), UPB_STRTABLE_INIT(1, 3, UPB_CTYPE_PTR, 2, &strentries[128]),&reftables[32], &reftables[33]),
  UPB_MSGDEF_INIT("google.protobuf.SourceCodeInfo.Location", 14, 0, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[185], 5, 4), UPB_STRTABLE_INIT(4, 7, UPB_CTYPE_PTR, 3, &strentries[132]),&reftables[34], &reftables[35]),
  UPB_MSGDEF_INIT("google.protobuf.UninterpretedOption", 18, 1, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[190], 9, 7), UPB_STRTABLE_INIT(7, 15, UPB_CTYPE_PTR, 4, &strentries[140]),&reftables[36], &reftables[37]),
  UPB_MSGDEF_INIT("google.protobuf.UninterpretedOption.NamePart", 6, 0, UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_PTR, 0, NULL, &arrays[199], 3, 2), UPB_STRTABLE_INIT(2, 3, UPB_CTYPE_PTR, 2, &strentries[156]),&reftables[38], &reftables[39]),
};

static const upb_fielddef fields[81] = {
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "aggregate_value", 8, &msgs[18], NULL, 15, 6, {0},&reftables[40], &reftables[41]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "allow_alias", 2, &msgs[3], NULL, 6, 1, {0},&reftables[42], &reftables[43]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "cc_generic_services", 16, &msgs[10], NULL, 17, 6, {0},&reftables[44], &reftables[45]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "ctype", 1, &msgs[7], (const upb_def*)(&enums[2]), 6, 1, {0},&reftables[46], &reftables[47]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "default_value", 7, &msgs[6], NULL, 16, 7, {0},&reftables[48], &reftables[49]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_STRING, 0, false, false, false, false, "dependency", 3, &msgs[8], NULL, 30, 8, {0},&reftables[50], &reftables[51]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "deprecated", 3, &msgs[7], NULL, 8, 3, {0},&reftables[52], &reftables[53]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_DOUBLE, 0, false, false, false, false, "double_value", 6, &msgs[18], NULL, 11, 4, {0},&reftables[54], &reftables[55]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "end", 2, &msgs[1], NULL, 3, 1, {0},&reftables[56], &reftables[57]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "enum_type", 4, &msgs[0], (const upb_def*)(&msgs[2]), 16, 2, {0},&reftables[58], &reftables[59]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "enum_type", 5, &msgs[8], (const upb_def*)(&msgs[2]), 13, 1, {0},&reftables[60], &reftables[61]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "experimental_map_key", 9, &msgs[7], NULL, 10, 5, {0},&reftables[62], &reftables[63]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "extendee", 2, &msgs[6], NULL, 7, 2, {0},&reftables[64], &reftables[65]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "extension", 7, &msgs[8], (const upb_def*)(&msgs[6]), 19, 3, {0},&reftables[66], &reftables[67]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "extension", 6, &msgs[0], (const upb_def*)(&msgs[6]), 22, 4, {0},&reftables[68], &reftables[69]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "extension_range", 5, &msgs[0], (const upb_def*)(&msgs[1]), 19, 3, {0},&reftables[70], &reftables[71]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "field", 2, &msgs[0], (const upb_def*)(&msgs[6]), 10, 0, {0},&reftables[72], &reftables[73]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "file", 1, &msgs[9], (const upb_def*)(&msgs[8]), 5, 0, {0},&reftables[74], &reftables[75]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "go_package", 11, &msgs[10], NULL, 14, 5, {0},&reftables[76], &reftables[77]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "identifier_value", 3, &msgs[18], NULL, 6, 1, {0},&reftables[78], &reftables[79]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "input_type", 2, &msgs[12], NULL, 7, 2, {0},&reftables[80], &reftables[81]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REQUIRED, UPB_TYPE_BOOL, 0, false, false, false, false, "is_extension", 2, &msgs[19], NULL, 5, 1, {0},&reftables[82], &reftables[83]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "java_generate_equals_and_hash", 20, &msgs[10], NULL, 20, 9, {0},&reftables[84], &reftables[85]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "java_generic_services", 17, &msgs[10], NULL, 18, 7, {0},&reftables[86], &reftables[87]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "java_multiple_files", 10, &msgs[10], NULL, 13, 4, {0},&reftables[88], &reftables[89]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "java_outer_classname", 8, &msgs[10], NULL, 9, 2, {0},&reftables[90], &reftables[91]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "java_package", 1, &msgs[10], NULL, 6, 1, {0},&reftables[92], &reftables[93]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "label", 4, &msgs[6], (const upb_def*)(&enums[0]), 11, 4, {0},&reftables[94], &reftables[95]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "lazy", 5, &msgs[7], NULL, 9, 4, {0},&reftables[96], &reftables[97]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "leading_comments", 3, &msgs[17], NULL, 8, 2, {0},&reftables[98], &reftables[99]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "location", 1, &msgs[16], (const upb_def*)(&msgs[17]), 5, 0, {0},&reftables[100], &reftables[101]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "message_set_wire_format", 1, &msgs[11], NULL, 6, 1, {0},&reftables[102], &reftables[103]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "message_type", 4, &msgs[8], (const upb_def*)(&msgs[0]), 10, 0, {0},&reftables[104], &reftables[105]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "method", 2, &msgs[14], (const upb_def*)(&msgs[12]), 6, 0, {0},&reftables[106], &reftables[107]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[8], NULL, 22, 6, {0},&reftables[108], &reftables[109]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[14], NULL, 8, 2, {0},&reftables[110], &reftables[111]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "name", 2, &msgs[18], (const upb_def*)(&msgs[19]), 5, 0, {0},&reftables[112], &reftables[113]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[4], NULL, 4, 1, {0},&reftables[114], &reftables[115]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[0], NULL, 24, 6, {0},&reftables[116], &reftables[117]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[12], NULL, 4, 1, {0},&reftables[118], &reftables[119]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[2], NULL, 8, 2, {0},&reftables[120], &reftables[121]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "name", 1, &msgs[6], NULL, 4, 1, {0},&reftables[122], &reftables[123]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REQUIRED, UPB_TYPE_STRING, 0, false, false, false, false, "name_part", 1, &msgs[19], NULL, 2, 0, {0},&reftables[124], &reftables[125]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT64, UPB_INTFMT_VARIABLE, false, false, false, false, "negative_int_value", 5, &msgs[18], NULL, 10, 3, {0},&reftables[126], &reftables[127]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "nested_type", 3, &msgs[0], (const upb_def*)(&msgs[0]), 13, 1, {0},&reftables[128], &reftables[129]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "no_standard_descriptor_accessor", 2, &msgs[11], NULL, 7, 2, {0},&reftables[130], &reftables[131]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "number", 3, &msgs[6], NULL, 10, 3, {0},&reftables[132], &reftables[133]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "number", 2, &msgs[4], NULL, 7, 2, {0},&reftables[134], &reftables[135]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "optimize_for", 9, &msgs[10], (const upb_def*)(&enums[3]), 12, 3, {0},&reftables[136], &reftables[137]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 7, &msgs[0], (const upb_def*)(&msgs[11]), 23, 5, {0},&reftables[138], &reftables[139]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 3, &msgs[2], (const upb_def*)(&msgs[3]), 7, 1, {0},&reftables[140], &reftables[141]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 8, &msgs[6], (const upb_def*)(&msgs[7]), 3, 0, {0},&reftables[142], &reftables[143]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 3, &msgs[4], (const upb_def*)(&msgs[5]), 3, 0, {0},&reftables[144], &reftables[145]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 8, &msgs[8], (const upb_def*)(&msgs[10]), 20, 4, {0},&reftables[146], &reftables[147]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 3, &msgs[14], (const upb_def*)(&msgs[15]), 7, 1, {0},&reftables[148], &reftables[149]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "options", 4, &msgs[12], (const upb_def*)(&msgs[13]), 3, 0, {0},&reftables[150], &reftables[151]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "output_type", 3, &msgs[12], NULL, 10, 3, {0},&reftables[152], &reftables[153]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "package", 2, &msgs[8], NULL, 25, 7, {0},&reftables[154], &reftables[155]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "packed", 2, &msgs[7], NULL, 7, 2, {0},&reftables[156], &reftables[157]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, true, "path", 1, &msgs[17], NULL, 4, 0, {0},&reftables[158], &reftables[159]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_UINT64, UPB_INTFMT_VARIABLE, false, false, false, false, "positive_int_value", 4, &msgs[18], NULL, 9, 2, {0},&reftables[160], &reftables[161]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "public_dependency", 10, &msgs[8], NULL, 35, 9, {0},&reftables[162], &reftables[163]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "py_generic_services", 18, &msgs[10], NULL, 19, 8, {0},&reftables[164], &reftables[165]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "service", 6, &msgs[8], (const upb_def*)(&msgs[14]), 16, 2, {0},&reftables[166], &reftables[167]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_MESSAGE, 0, false, false, false, false, "source_code_info", 9, &msgs[8], (const upb_def*)(&msgs[16]), 21, 5, {0},&reftables[168], &reftables[169]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, true, "span", 2, &msgs[17], NULL, 7, 1, {0},&reftables[170], &reftables[171]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "start", 1, &msgs[1], NULL, 2, 0, {0},&reftables[172], &reftables[173]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BYTES, 0, false, false, false, false, "string_value", 7, &msgs[18], NULL, 12, 5, {0},&reftables[174], &reftables[175]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "trailing_comments", 4, &msgs[17], NULL, 11, 3, {0},&reftables[176], &reftables[177]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_ENUM, 0, false, false, false, false, "type", 5, &msgs[6], (const upb_def*)(&enums[1]), 12, 5, {0},&reftables[178], &reftables[179]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_STRING, 0, false, false, false, false, "type_name", 6, &msgs[6], NULL, 13, 6, {0},&reftables[180], &reftables[181]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[5], (const upb_def*)(&msgs[18]), 5, 0, {0},&reftables[182], &reftables[183]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[15], (const upb_def*)(&msgs[18]), 5, 0, {0},&reftables[184], &reftables[185]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[3], (const upb_def*)(&msgs[18]), 5, 0, {0},&reftables[186], &reftables[187]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[13], (const upb_def*)(&msgs[18]), 5, 0, {0},&reftables[188], &reftables[189]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[10], (const upb_def*)(&msgs[18]), 5, 0, {0},&reftables[190], &reftables[191]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[11], (const upb_def*)(&msgs[18]), 5, 0, {0},&reftables[192], &reftables[193]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "uninterpreted_option", 999, &msgs[7], (const upb_def*)(&msgs[18]), 5, 0, {0},&reftables[194], &reftables[195]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_MESSAGE, 0, false, false, false, false, "value", 2, &msgs[2], (const upb_def*)(&msgs[4]), 6, 0, {0},&reftables[196], &reftables[197]),
  UPB_FIELDDEF_INIT(UPB_LABEL_OPTIONAL, UPB_TYPE_BOOL, 0, false, false, false, false, "weak", 10, &msgs[7], NULL, 13, 6, {0},&reftables[198], &reftables[199]),
  UPB_FIELDDEF_INIT(UPB_LABEL_REPEATED, UPB_TYPE_INT32, UPB_INTFMT_VARIABLE, false, false, false, false, "weak_dependency", 11, &msgs[8], NULL, 38, 10, {0},&reftables[200], &reftables[201]),
};

static const upb_enumdef enums[4] = {
  UPB_ENUMDEF_INIT("google.protobuf.FieldDescriptorProto.Label", UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_INT32, 2, &strentries[160]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[202], 4, 3), 0, &reftables[202], &reftables[203]),
  UPB_ENUMDEF_INIT("google.protobuf.FieldDescriptorProto.Type", UPB_STRTABLE_INIT(18, 31, UPB_CTYPE_INT32, 5, &strentries[164]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[206], 19, 18), 0, &reftables[204], &reftables[205]),
  UPB_ENUMDEF_INIT("google.protobuf.FieldOptions.CType", UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_INT32, 2, &strentries[196]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[225], 3, 3), 0, &reftables[206], &reftables[207]),
  UPB_ENUMDEF_INIT("google.protobuf.FileOptions.OptimizeMode", UPB_STRTABLE_INIT(3, 3, UPB_CTYPE_INT32, 2, &strentries[200]), UPB_INTTABLE_INIT(0, 0, UPB_CTYPE_CSTR, 0, NULL, &arrays[228], 4, 3), 0, &reftables[208], &reftables[209]),
};

static const upb_tabent strentries[236] = {
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "extension"), UPB_TABVALUE_PTR_INIT(&fields[14]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[38]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "field"), UPB_TABVALUE_PTR_INIT(&fields[16]), NULL},
  {UPB_TABKEY_STR("\017", "\000", "\000", "\000", "extension_range"), UPB_TABVALUE_PTR_INIT(&fields[15]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "nested_type"), UPB_TABVALUE_PTR_INIT(&fields[44]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[49]), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "enum_type"), UPB_TABVALUE_PTR_INIT(&fields[9]), &strentries[14]},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "start"), UPB_TABVALUE_PTR_INIT(&fields[66]), NULL},
  {UPB_TABKEY_STR("\003", "\000", "\000", "\000", "end"), UPB_TABVALUE_PTR_INIT(&fields[8]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "value"), UPB_TABVALUE_PTR_INIT(&fields[78]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[50]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[40]), &strentries[22]},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[73]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "allow_alias"), UPB_TABVALUE_PTR_INIT(&fields[1]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "number"), UPB_TABVALUE_PTR_INIT(&fields[47]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[52]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[37]), &strentries[30]},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[71]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "label"), UPB_TABVALUE_PTR_INIT(&fields[27]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[41]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "number"), UPB_TABVALUE_PTR_INIT(&fields[46]), &strentries[49]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "type_name"), UPB_TABVALUE_PTR_INIT(&fields[70]), NULL},
  {UPB_TABKEY_STR("\010", "\000", "\000", "\000", "extendee"), UPB_TABVALUE_PTR_INIT(&fields[12]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "type"), UPB_TABVALUE_PTR_INIT(&fields[69]), &strentries[48]},
  {UPB_TABKEY_STR("\015", "\000", "\000", "\000", "default_value"), UPB_TABVALUE_PTR_INIT(&fields[4]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[51]), NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "experimental_map_key"), UPB_TABVALUE_PTR_INIT(&fields[11]), &strentries[67]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "weak"), UPB_TABVALUE_PTR_INIT(&fields[79]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "packed"), UPB_TABVALUE_PTR_INIT(&fields[58]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "lazy"), UPB_TABVALUE_PTR_INIT(&fields[28]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "ctype"), UPB_TABVALUE_PTR_INIT(&fields[3]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "deprecated"), UPB_TABVALUE_PTR_INIT(&fields[6]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[77]), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "extension"), UPB_TABVALUE_PTR_INIT(&fields[13]), NULL},
  {UPB_TABKEY_STR("\017", "\000", "\000", "\000", "weak_dependency"), UPB_TABVALUE_PTR_INIT(&fields[80]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[34]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "service"), UPB_TABVALUE_PTR_INIT(&fields[63]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "source_code_info"), UPB_TABVALUE_PTR_INIT(&fields[64]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "dependency"), UPB_TABVALUE_PTR_INIT(&fields[5]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "message_type"), UPB_TABVALUE_PTR_INIT(&fields[32]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "package"), UPB_TABVALUE_PTR_INIT(&fields[57]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[53]), &strentries[82]},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "enum_type"), UPB_TABVALUE_PTR_INIT(&fields[10]), NULL},
  {UPB_TABKEY_STR("\021", "\000", "\000", "\000", "public_dependency"), UPB_TABVALUE_PTR_INIT(&fields[61]), &strentries[81]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "file"), UPB_TABVALUE_PTR_INIT(&fields[17]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[75]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\023", "\000", "\000", "\000", "cc_generic_services"), UPB_TABVALUE_PTR_INIT(&fields[2]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\023", "\000", "\000", "\000", "java_multiple_files"), UPB_TABVALUE_PTR_INIT(&fields[24]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\025", "\000", "\000", "\000", "java_generic_services"), UPB_TABVALUE_PTR_INIT(&fields[23]), &strentries[102]},
  {UPB_TABKEY_STR("\035", "\000", "\000", "\000", "java_generate_equals_and_hash"), UPB_TABVALUE_PTR_INIT(&fields[22]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "go_package"), UPB_TABVALUE_PTR_INIT(&fields[18]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "java_package"), UPB_TABVALUE_PTR_INIT(&fields[26]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "optimize_for"), UPB_TABVALUE_PTR_INIT(&fields[48]), NULL},
  {UPB_TABKEY_STR("\023", "\000", "\000", "\000", "py_generic_services"), UPB_TABVALUE_PTR_INIT(&fields[62]), NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "java_outer_classname"), UPB_TABVALUE_PTR_INIT(&fields[25]), NULL},
  {UPB_TABKEY_STR("\027", "\000", "\000", "\000", "message_set_wire_format"), UPB_TABVALUE_PTR_INIT(&fields[31]), &strentries[106]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[76]), NULL},
  {UPB_TABKEY_STR("\037", "\000", "\000", "\000", "no_standard_descriptor_accessor"), UPB_TABVALUE_PTR_INIT(&fields[45]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[39]), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "input_type"), UPB_TABVALUE_PTR_INIT(&fields[20]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "output_type"), UPB_TABVALUE_PTR_INIT(&fields[56]), NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[55]), NULL},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[74]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\007", "\000", "\000", "\000", "options"), UPB_TABVALUE_PTR_INIT(&fields[54]), &strentries[122]},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "method"), UPB_TABVALUE_PTR_INIT(&fields[33]), NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[35]), &strentries[121]},
  {UPB_TABKEY_STR("\024", "\000", "\000", "\000", "uninterpreted_option"), UPB_TABVALUE_PTR_INIT(&fields[72]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\010", "\000", "\000", "\000", "location"), UPB_TABVALUE_PTR_INIT(&fields[30]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "span"), UPB_TABVALUE_PTR_INIT(&fields[65]), &strentries[139]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\021", "\000", "\000", "\000", "trailing_comments"), UPB_TABVALUE_PTR_INIT(&fields[68]), NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "leading_comments"), UPB_TABVALUE_PTR_INIT(&fields[29]), &strentries[137]},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "path"), UPB_TABVALUE_PTR_INIT(&fields[59]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "double_value"), UPB_TABVALUE_PTR_INIT(&fields[7]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "name"), UPB_TABVALUE_PTR_INIT(&fields[36]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\022", "\000", "\000", "\000", "negative_int_value"), UPB_TABVALUE_PTR_INIT(&fields[43]), NULL},
  {UPB_TABKEY_STR("\017", "\000", "\000", "\000", "aggregate_value"), UPB_TABVALUE_PTR_INIT(&fields[0]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\022", "\000", "\000", "\000", "positive_int_value"), UPB_TABVALUE_PTR_INIT(&fields[60]), NULL},
  {UPB_TABKEY_STR("\020", "\000", "\000", "\000", "identifier_value"), UPB_TABVALUE_PTR_INIT(&fields[19]), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "string_value"), UPB_TABVALUE_PTR_INIT(&fields[67]), &strentries[154]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "is_extension"), UPB_TABVALUE_PTR_INIT(&fields[21]), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "name_part"), UPB_TABVALUE_PTR_INIT(&fields[42]), NULL},
  {UPB_TABKEY_STR("\016", "\000", "\000", "\000", "LABEL_REQUIRED"), UPB_TABVALUE_INT_INIT(2), &strentries[162]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\016", "\000", "\000", "\000", "LABEL_REPEATED"), UPB_TABVALUE_INT_INIT(3), NULL},
  {UPB_TABKEY_STR("\016", "\000", "\000", "\000", "LABEL_OPTIONAL"), UPB_TABVALUE_INT_INIT(1), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "TYPE_FIXED64"), UPB_TABVALUE_INT_INIT(6), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_STRING"), UPB_TABVALUE_INT_INIT(9), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_FLOAT"), UPB_TABVALUE_INT_INIT(2), &strentries[193]},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_DOUBLE"), UPB_TABVALUE_INT_INIT(1), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_INT32"), UPB_TABVALUE_INT_INIT(5), NULL},
  {UPB_TABKEY_STR("\015", "\000", "\000", "\000", "TYPE_SFIXED32"), UPB_TABVALUE_INT_INIT(15), NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "TYPE_FIXED32"), UPB_TABVALUE_INT_INIT(7), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "TYPE_MESSAGE"), UPB_TABVALUE_INT_INIT(11), &strentries[194]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_INT64"), UPB_TABVALUE_INT_INIT(3), &strentries[191]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "TYPE_ENUM"), UPB_TABVALUE_INT_INIT(14), NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_UINT32"), UPB_TABVALUE_INT_INIT(13), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_UINT64"), UPB_TABVALUE_INT_INIT(4), &strentries[190]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\015", "\000", "\000", "\000", "TYPE_SFIXED64"), UPB_TABVALUE_INT_INIT(16), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_BYTES"), UPB_TABVALUE_INT_INIT(12), NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_SINT64"), UPB_TABVALUE_INT_INIT(18), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "TYPE_BOOL"), UPB_TABVALUE_INT_INIT(8), NULL},
  {UPB_TABKEY_STR("\012", "\000", "\000", "\000", "TYPE_GROUP"), UPB_TABVALUE_INT_INIT(10), NULL},
  {UPB_TABKEY_STR("\013", "\000", "\000", "\000", "TYPE_SINT32"), UPB_TABVALUE_INT_INIT(17), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\004", "\000", "\000", "\000", "CORD"), UPB_TABVALUE_INT_INIT(1), NULL},
  {UPB_TABKEY_STR("\006", "\000", "\000", "\000", "STRING"), UPB_TABVALUE_INT_INIT(0), &strentries[197]},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "STRING_PIECE"), UPB_TABVALUE_INT_INIT(2), NULL},
  {UPB_TABKEY_STR("\011", "\000", "\000", "\000", "CODE_SIZE"), UPB_TABVALUE_INT_INIT(2), NULL},
  {UPB_TABKEY_STR("\005", "\000", "\000", "\000", "SPEED"), UPB_TABVALUE_INT_INIT(1), &strentries[203]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\014", "\000", "\000", "\000", "LITE_RUNTIME"), UPB_TABVALUE_INT_INIT(3), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\047", "\000", "\000", "\000", "google.protobuf.SourceCodeInfo.Location"), UPB_TABVALUE_PTR_INIT(&msgs[17]), NULL},
  {UPB_TABKEY_STR("\043", "\000", "\000", "\000", "google.protobuf.UninterpretedOption"), UPB_TABVALUE_PTR_INIT(&msgs[18]), NULL},
  {UPB_TABKEY_STR("\043", "\000", "\000", "\000", "google.protobuf.FileDescriptorProto"), UPB_TABVALUE_PTR_INIT(&msgs[8]), NULL},
  {UPB_TABKEY_STR("\045", "\000", "\000", "\000", "google.protobuf.MethodDescriptorProto"), UPB_TABVALUE_PTR_INIT(&msgs[12]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\040", "\000", "\000", "\000", "google.protobuf.EnumValueOptions"), UPB_TABVALUE_PTR_INIT(&msgs[5]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\037", "\000", "\000", "\000", "google.protobuf.DescriptorProto"), UPB_TABVALUE_PTR_INIT(&msgs[0]), &strentries[228]},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\036", "\000", "\000", "\000", "google.protobuf.SourceCodeInfo"), UPB_TABVALUE_PTR_INIT(&msgs[16]), NULL},
  {UPB_TABKEY_STR("\051", "\000", "\000", "\000", "google.protobuf.FieldDescriptorProto.Type"), UPB_TABVALUE_PTR_INIT(&enums[1]), NULL},
  {UPB_TABKEY_STR("\056", "\000", "\000", "\000", "google.protobuf.DescriptorProto.ExtensionRange"), UPB_TABVALUE_PTR_INIT(&msgs[1]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_STR("\050", "\000", "\000", "\000", "google.protobuf.EnumValueDescriptorProto"), UPB_TABVALUE_PTR_INIT(&msgs[4]), NULL},
  {UPB_TABKEY_STR("\034", "\000", "\000", "\000", "google.protobuf.FieldOptions"), UPB_TABVALUE_PTR_INIT(&msgs[7]), NULL},
  {UPB_TABKEY_STR("\033", "\000", "\000", "\000", "google.protobuf.FileOptions"), UPB_TABVALUE_PTR_INIT(&msgs[10]), NULL},
  {UPB_TABKEY_STR("\043", "\000", "\000", "\000", "google.protobuf.EnumDescriptorProto"), UPB_TABVALUE_PTR_INIT(&msgs[2]), &strentries[233]},
  {UPB_TABKEY_STR("\052", "\000", "\000", "\000", "google.protobuf.FieldDescriptorProto.Label"), UPB_TABVALUE_PTR_INIT(&enums[0]), NULL},
  {UPB_TABKEY_STR("\046", "\000", "\000", "\000", "google.protobuf.ServiceDescriptorProto"), UPB_TABVALUE_PTR_INIT(&msgs[14]), NULL},
  {UPB_TABKEY_STR("\042", "\000", "\000", "\000", "google.protobuf.FieldOptions.CType"), UPB_TABVALUE_PTR_INIT(&enums[2]), &strentries[229]},
  {UPB_TABKEY_STR("\041", "\000", "\000", "\000", "google.protobuf.FileDescriptorSet"), UPB_TABVALUE_PTR_INIT(&msgs[9]), &strentries[235]},
  {UPB_TABKEY_STR("\033", "\000", "\000", "\000", "google.protobuf.EnumOptions"), UPB_TABVALUE_PTR_INIT(&msgs[3]), NULL},
  {UPB_TABKEY_STR("\044", "\000", "\000", "\000", "google.protobuf.FieldDescriptorProto"), UPB_TABVALUE_PTR_INIT(&msgs[6]), NULL},
  {UPB_TABKEY_STR("\050", "\000", "\000", "\000", "google.protobuf.FileOptions.OptimizeMode"), UPB_TABVALUE_PTR_INIT(&enums[3]), &strentries[221]},
  {UPB_TABKEY_STR("\036", "\000", "\000", "\000", "google.protobuf.ServiceOptions"), UPB_TABVALUE_PTR_INIT(&msgs[15]), NULL},
  {UPB_TABKEY_STR("\036", "\000", "\000", "\000", "google.protobuf.MessageOptions"), UPB_TABVALUE_PTR_INIT(&msgs[11]), NULL},
  {UPB_TABKEY_STR("\035", "\000", "\000", "\000", "google.protobuf.MethodOptions"), UPB_TABVALUE_PTR_INIT(&msgs[13]), &strentries[226]},
  {UPB_TABKEY_STR("\054", "\000", "\000", "\000", "google.protobuf.UninterpretedOption.NamePart"), UPB_TABVALUE_PTR_INIT(&msgs[19]), NULL},
};

static const upb_tabent intentries[14] = {
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[73]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[71]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[77]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[75]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[76]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[74]), NULL},
  {UPB_TABKEY_NONE, UPB_TABVALUE_EMPTY_INIT, NULL},
  {UPB_TABKEY_NUM(999), UPB_TABVALUE_PTR_INIT(&fields[72]), NULL},
};

static const upb_tabval arrays[232] = {
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[38]),
  UPB_TABVALUE_PTR_INIT(&fields[16]),
  UPB_TABVALUE_PTR_INIT(&fields[44]),
  UPB_TABVALUE_PTR_INIT(&fields[9]),
  UPB_TABVALUE_PTR_INIT(&fields[15]),
  UPB_TABVALUE_PTR_INIT(&fields[14]),
  UPB_TABVALUE_PTR_INIT(&fields[49]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[66]),
  UPB_TABVALUE_PTR_INIT(&fields[8]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[40]),
  UPB_TABVALUE_PTR_INIT(&fields[78]),
  UPB_TABVALUE_PTR_INIT(&fields[50]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[1]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[37]),
  UPB_TABVALUE_PTR_INIT(&fields[47]),
  UPB_TABVALUE_PTR_INIT(&fields[52]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[41]),
  UPB_TABVALUE_PTR_INIT(&fields[12]),
  UPB_TABVALUE_PTR_INIT(&fields[46]),
  UPB_TABVALUE_PTR_INIT(&fields[27]),
  UPB_TABVALUE_PTR_INIT(&fields[69]),
  UPB_TABVALUE_PTR_INIT(&fields[70]),
  UPB_TABVALUE_PTR_INIT(&fields[4]),
  UPB_TABVALUE_PTR_INIT(&fields[51]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[3]),
  UPB_TABVALUE_PTR_INIT(&fields[58]),
  UPB_TABVALUE_PTR_INIT(&fields[6]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[28]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[11]),
  UPB_TABVALUE_PTR_INIT(&fields[79]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[34]),
  UPB_TABVALUE_PTR_INIT(&fields[57]),
  UPB_TABVALUE_PTR_INIT(&fields[5]),
  UPB_TABVALUE_PTR_INIT(&fields[32]),
  UPB_TABVALUE_PTR_INIT(&fields[10]),
  UPB_TABVALUE_PTR_INIT(&fields[63]),
  UPB_TABVALUE_PTR_INIT(&fields[13]),
  UPB_TABVALUE_PTR_INIT(&fields[53]),
  UPB_TABVALUE_PTR_INIT(&fields[64]),
  UPB_TABVALUE_PTR_INIT(&fields[61]),
  UPB_TABVALUE_PTR_INIT(&fields[80]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[17]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[26]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[25]),
  UPB_TABVALUE_PTR_INIT(&fields[48]),
  UPB_TABVALUE_PTR_INIT(&fields[24]),
  UPB_TABVALUE_PTR_INIT(&fields[18]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[2]),
  UPB_TABVALUE_PTR_INIT(&fields[23]),
  UPB_TABVALUE_PTR_INIT(&fields[62]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[22]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[31]),
  UPB_TABVALUE_PTR_INIT(&fields[45]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[39]),
  UPB_TABVALUE_PTR_INIT(&fields[20]),
  UPB_TABVALUE_PTR_INIT(&fields[56]),
  UPB_TABVALUE_PTR_INIT(&fields[55]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[35]),
  UPB_TABVALUE_PTR_INIT(&fields[33]),
  UPB_TABVALUE_PTR_INIT(&fields[54]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[30]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[59]),
  UPB_TABVALUE_PTR_INIT(&fields[65]),
  UPB_TABVALUE_PTR_INIT(&fields[29]),
  UPB_TABVALUE_PTR_INIT(&fields[68]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[36]),
  UPB_TABVALUE_PTR_INIT(&fields[19]),
  UPB_TABVALUE_PTR_INIT(&fields[60]),
  UPB_TABVALUE_PTR_INIT(&fields[43]),
  UPB_TABVALUE_PTR_INIT(&fields[7]),
  UPB_TABVALUE_PTR_INIT(&fields[67]),
  UPB_TABVALUE_PTR_INIT(&fields[0]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT(&fields[42]),
  UPB_TABVALUE_PTR_INIT(&fields[21]),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT("LABEL_OPTIONAL"),
  UPB_TABVALUE_PTR_INIT("LABEL_REQUIRED"),
  UPB_TABVALUE_PTR_INIT("LABEL_REPEATED"),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT("TYPE_DOUBLE"),
  UPB_TABVALUE_PTR_INIT("TYPE_FLOAT"),
  UPB_TABVALUE_PTR_INIT("TYPE_INT64"),
  UPB_TABVALUE_PTR_INIT("TYPE_UINT64"),
  UPB_TABVALUE_PTR_INIT("TYPE_INT32"),
  UPB_TABVALUE_PTR_INIT("TYPE_FIXED64"),
  UPB_TABVALUE_PTR_INIT("TYPE_FIXED32"),
  UPB_TABVALUE_PTR_INIT("TYPE_BOOL"),
  UPB_TABVALUE_PTR_INIT("TYPE_STRING"),
  UPB_TABVALUE_PTR_INIT("TYPE_GROUP"),
  UPB_TABVALUE_PTR_INIT("TYPE_MESSAGE"),
  UPB_TABVALUE_PTR_INIT("TYPE_BYTES"),
  UPB_TABVALUE_PTR_INIT("TYPE_UINT32"),
  UPB_TABVALUE_PTR_INIT("TYPE_ENUM"),
  UPB_TABVALUE_PTR_INIT("TYPE_SFIXED32"),
  UPB_TABVALUE_PTR_INIT("TYPE_SFIXED64"),
  UPB_TABVALUE_PTR_INIT("TYPE_SINT32"),
  UPB_TABVALUE_PTR_INIT("TYPE_SINT64"),
  UPB_TABVALUE_PTR_INIT("STRING"),
  UPB_TABVALUE_PTR_INIT("CORD"),
  UPB_TABVALUE_PTR_INIT("STRING_PIECE"),
  UPB_TABVALUE_EMPTY_INIT,
  UPB_TABVALUE_PTR_INIT("SPEED"),
  UPB_TABVALUE_PTR_INIT("CODE_SIZE"),
  UPB_TABVALUE_PTR_INIT("LITE_RUNTIME"),
};

static const upb_symtab symtab = UPB_SYMTAB_INIT(UPB_STRTABLE_INIT(24, 31, UPB_CTYPE_PTR, 5, &strentries[204]), &reftables[210], &reftables[211]);

const upb_symtab *upbdefs_google_protobuf_descriptor(const void *owner) {
  upb_symtab_ref(&symtab, owner);
  return &symtab;
}

#ifdef UPB_DEBUG_REFS
static upb_inttable reftables[212] = {
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),
};
#endif

/*
** XXX: The routines in this file that consume a string do not currently
** support having the string span buffers.  In the future, as upb_sink and
** its buffering/sharing functionality evolve there should be an easy and
** idiomatic way of correctly handling this case.  For now, we accept this
** limitation since we currently only parse descriptors from single strings.
*/


#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* upb_deflist is an internal-only dynamic array for storing a growing list of
 * upb_defs. */
typedef struct {
  upb_def **defs;
  size_t len;
  size_t size;
  bool owned;
} upb_deflist;

/* We keep a stack of all the messages scopes we are currently in, as well as
 * the top-level file scope.  This is necessary to correctly qualify the
 * definitions that are contained inside.  "name" tracks the name of the
 * message or package (a bare name -- not qualified by any enclosing scopes). */
typedef struct {
  char *name;
  /* Index of the first def that is under this scope.  For msgdefs, the
   * msgdef itself is at start-1. */
  int start;
} upb_descreader_frame;

/* The maximum number of nested declarations that are allowed, ie.
 * message Foo {
 *   message Bar {
 *     message Baz {
 *     }
 *   }
 * }
 *
 * This is a resource limit that affects how big our runtime stack can grow.
 * TODO: make this a runtime-settable property of the Reader instance. */
#define UPB_MAX_MESSAGE_NESTING 64

struct upb_descreader {
  upb_sink sink;
  upb_deflist defs;
  upb_descreader_frame stack[UPB_MAX_MESSAGE_NESTING];
  int stack_len;

  uint32_t number;
  char *name;
  bool saw_number;
  bool saw_name;

  char *default_string;

  upb_fielddef *f;
};

static char *upb_strndup(const char *buf, size_t n) {
  char *ret = malloc(n + 1);
  if (!ret) return NULL;
  memcpy(ret, buf, n);
  ret[n] = '\0';
  return ret;
}

/* Returns a newly allocated string that joins input strings together, for
 * example:
 *   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
 *   join("", "Baz") -> "Baz"
 * Caller owns a ref on the returned string. */
static char *upb_join(const char *base, const char *name) {
  if (!base || strlen(base) == 0) {
    return upb_strdup(name);
  } else {
    char *ret = malloc(strlen(base) + strlen(name) + 2);
    ret[0] = '\0';
    strcat(ret, base);
    strcat(ret, ".");
    strcat(ret, name);
    return ret;
  }
}


/* upb_deflist ****************************************************************/

void upb_deflist_init(upb_deflist *l) {
  l->size = 0;
  l->defs = NULL;
  l->len = 0;
  l->owned = true;
}

void upb_deflist_uninit(upb_deflist *l) {
  size_t i;
  if (l->owned)
    for(i = 0; i < l->len; i++)
      upb_def_unref(l->defs[i], l);
  free(l->defs);
}

bool upb_deflist_push(upb_deflist *l, upb_def *d) {
  if(++l->len >= l->size) {
    size_t new_size = UPB_MAX(l->size, 4);
    new_size *= 2;
    l->defs = realloc(l->defs, new_size * sizeof(void *));
    if (!l->defs) return false;
    l->size = new_size;
  }
  l->defs[l->len - 1] = d;
  return true;
}

void upb_deflist_donaterefs(upb_deflist *l, void *owner) {
  size_t i;
  assert(l->owned);
  for (i = 0; i < l->len; i++)
    upb_def_donateref(l->defs[i], l, owner);
  l->owned = false;
}

static upb_def *upb_deflist_last(upb_deflist *l) {
  return l->defs[l->len-1];
}

/* Qualify the defname for all defs starting with offset "start" with "str". */
static void upb_deflist_qualify(upb_deflist *l, char *str, int32_t start) {
  uint32_t i;
  for (i = start; i < l->len; i++) {
    upb_def *def = l->defs[i];
    char *name = upb_join(str, upb_def_fullname(def));
    upb_def_setfullname(def, name, NULL);
    free(name);
  }
}


/* upb_descreader  ************************************************************/

static upb_msgdef *upb_descreader_top(upb_descreader *r) {
  int index;
  assert(r->stack_len > 1);
  index = r->stack[r->stack_len-1].start - 1;
  assert(index >= 0);
  return upb_downcast_msgdef_mutable(r->defs.defs[index]);
}

static upb_def *upb_descreader_last(upb_descreader *r) {
  return upb_deflist_last(&r->defs);
}

/* Start/end handlers for FileDescriptorProto and DescriptorProto (the two
 * entities that have names and can contain sub-definitions. */
void upb_descreader_startcontainer(upb_descreader *r) {
  upb_descreader_frame *f = &r->stack[r->stack_len++];
  f->start = r->defs.len;
  f->name = NULL;
}

void upb_descreader_endcontainer(upb_descreader *r) {
  upb_descreader_frame *f = &r->stack[--r->stack_len];
  upb_deflist_qualify(&r->defs, f->name, f->start);
  free(f->name);
  f->name = NULL;
}

void upb_descreader_setscopename(upb_descreader *r, char *str) {
  upb_descreader_frame *f = &r->stack[r->stack_len-1];
  free(f->name);
  f->name = str;
}

/* Handlers for google.protobuf.FileDescriptorProto. */
static bool file_startmsg(void *r, const void *hd) {
  UPB_UNUSED(hd);
  upb_descreader_startcontainer(r);
  return true;
}

static bool file_endmsg(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(status);
  upb_descreader_endcontainer(r);
  return true;
}

static size_t file_onpackage(void *closure, const void *hd, const char *buf,
                             size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  /* XXX: see comment at the top of the file. */
  upb_descreader_setscopename(r, upb_strndup(buf, n));
  return n;
}

/* Handlers for google.protobuf.EnumValueDescriptorProto. */
static bool enumval_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  r->saw_number = false;
  r->saw_name = false;
  return true;
}

static size_t enumval_onname(void *closure, const void *hd, const char *buf,
                             size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  /* XXX: see comment at the top of the file. */
  free(r->name);
  r->name = upb_strndup(buf, n);
  r->saw_name = true;
  return n;
}

static bool enumval_onnumber(void *closure, const void *hd, int32_t val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  r->number = val;
  r->saw_number = true;
  return true;
}

static bool enumval_endmsg(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_enumdef *e;
  UPB_UNUSED(hd);

  if(!r->saw_number || !r->saw_name) {
    upb_status_seterrmsg(status, "Enum value missing name or number.");
    return false;
  }
  e = upb_downcast_enumdef_mutable(upb_descreader_last(r));
  upb_enumdef_addval(e, r->name, r->number, status);
  free(r->name);
  r->name = NULL;
  return true;
}


/* Handlers for google.protobuf.EnumDescriptorProto. */
static bool enum_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  upb_deflist_push(&r->defs,
                   upb_enumdef_upcast_mutable(upb_enumdef_new(&r->defs)));
  return true;
}

static bool enum_endmsg(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_enumdef *e;
  UPB_UNUSED(hd);

  e = upb_downcast_enumdef_mutable(upb_descreader_last(r));
  if (upb_def_fullname(upb_descreader_last(r)) == NULL) {
    upb_status_seterrmsg(status, "Enum had no name.");
    return false;
  }
  if (upb_enumdef_numvals(e) == 0) {
    upb_status_seterrmsg(status, "Enum had no values.");
    return false;
  }
  return true;
}

static size_t enum_onname(void *closure, const void *hd, const char *buf,
                          size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *fullname = upb_strndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  /* XXX: see comment at the top of the file. */
  upb_def_setfullname(upb_descreader_last(r), fullname, NULL);
  free(fullname);
  return n;
}

/* Handlers for google.protobuf.FieldDescriptorProto */
static bool field_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  r->f = upb_fielddef_new(&r->defs);
  free(r->default_string);
  r->default_string = NULL;

  /* fielddefs default to packed, but descriptors default to non-packed. */
  upb_fielddef_setpacked(r->f, false);
  return true;
}

/* Converts the default value in string "str" into "d".  Passes a ref on str.
 * Returns true on success. */
static bool parse_default(char *str, upb_fielddef *f) {
  bool success = true;
  char *end;
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32: {
      long val = strtol(str, &end, 0);
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultint32(f, val);
      break;
    }
    case UPB_TYPE_INT64: {
      /* XXX: Need to write our own strtoll, since it's not available in c89. */
      long long val = strtol(str, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultint64(f, val);
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(str, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultuint32(f, val);
      break;
    }
    case UPB_TYPE_UINT64: {
      /* XXX: Need to write our own strtoull, since it's not available in c89. */
      unsigned long long val = strtoul(str, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultuint64(f, val);
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(str, &end);
      if (errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultdouble(f, val);
      break;
    }
    case UPB_TYPE_FLOAT: {
      /* XXX: Need to write our own strtof, since it's not available in c89. */
      float val = strtod(str, &end);
      if (errno == ERANGE || *end)
        success = false;
      else
        upb_fielddef_setdefaultfloat(f, val);
      break;
    }
    case UPB_TYPE_BOOL: {
      if (strcmp(str, "false") == 0)
        upb_fielddef_setdefaultbool(f, false);
      else if (strcmp(str, "true") == 0)
        upb_fielddef_setdefaultbool(f, true);
      else
        success = false;
      break;
    }
    default: abort();
  }
  return success;
}

static bool field_endmsg(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_fielddef *f = r->f;
  UPB_UNUSED(hd);

  /* TODO: verify that all required fields were present. */
  assert(upb_fielddef_number(f) != 0);
  assert(upb_fielddef_name(f) != NULL);
  assert((upb_fielddef_subdefname(f) != NULL) == upb_fielddef_hassubdef(f));

  if (r->default_string) {
    if (upb_fielddef_issubmsg(f)) {
      upb_status_seterrmsg(status, "Submessages cannot have defaults.");
      return false;
    }
    if (upb_fielddef_isstring(f) || upb_fielddef_type(f) == UPB_TYPE_ENUM) {
      upb_fielddef_setdefaultcstr(f, r->default_string, NULL);
    } else {
      if (r->default_string && !parse_default(r->default_string, f)) {
        /* We don't worry too much about giving a great error message since the
         * compiler should have ensured this was correct. */
        upb_status_seterrmsg(status, "Error converting default value.");
        return false;
      }
    }
  }
  return true;
}

static bool field_onlazy(void *closure, const void *hd, bool val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_fielddef_setlazy(r->f, val);
  return true;
}

static bool field_onpacked(void *closure, const void *hd, bool val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_fielddef_setpacked(r->f, val);
  return true;
}

static bool field_ontype(void *closure, const void *hd, int32_t val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_fielddef_setdescriptortype(r->f, val);
  return true;
}

static bool field_onlabel(void *closure, const void *hd, int32_t val) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_fielddef_setlabel(r->f, val);
  return true;
}

static bool field_onnumber(void *closure, const void *hd, int32_t val) {
  upb_descreader *r = closure;
  bool ok = upb_fielddef_setnumber(r->f, val, NULL);
  UPB_UNUSED(hd);

  UPB_ASSERT_VAR(ok, ok);
  return true;
}

static size_t field_onname(void *closure, const void *hd, const char *buf,
                           size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_strndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setname(r->f, name, NULL);
  free(name);
  return n;
}

static size_t field_ontypename(void *closure, const void *hd, const char *buf,
                               size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_strndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setsubdefname(r->f, name, NULL);
  free(name);
  return n;
}

static size_t field_onextendee(void *closure, const void *hd, const char *buf,
                               size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_strndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setcontainingtypename(r->f, name, NULL);
  free(name);
  return n;
}

static size_t field_ondefaultval(void *closure, const void *hd, const char *buf,
                                 size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* Have to convert from string to the correct type, but we might not know the
   * type yet, so we save it as a string until the end of the field.
   * XXX: see comment at the top of the file. */
  free(r->default_string);
  r->default_string = upb_strndup(buf, n);
  return n;
}

/* Handlers for google.protobuf.DescriptorProto (representing a message). */
static bool msg_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_deflist_push(&r->defs,
                   upb_msgdef_upcast_mutable(upb_msgdef_new(&r->defs)));
  upb_descreader_startcontainer(r);
  return true;
}

static bool msg_endmsg(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  UPB_UNUSED(hd);

  if(!upb_def_fullname(upb_msgdef_upcast_mutable(m))) {
    upb_status_seterrmsg(status, "Encountered message with no name.");
    return false;
  }
  upb_descreader_endcontainer(r);
  return true;
}

static size_t msg_onname(void *closure, const void *hd, const char *buf,
                         size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  /* XXX: see comment at the top of the file. */
  char *name = upb_strndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  upb_def_setfullname(upb_msgdef_upcast_mutable(m), name, NULL);
  upb_descreader_setscopename(r, name);  /* Passes ownership of name. */
  return n;
}

static bool msg_onendfield(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  UPB_UNUSED(hd);

  upb_msgdef_addfield(m, r->f, &r->defs, NULL);
  r->f = NULL;
  return true;
}

static bool pushextension(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  assert(upb_fielddef_containingtypename(r->f));
  upb_fielddef_setisextension(r->f, true);
  upb_deflist_push(&r->defs, upb_fielddef_upcast_mutable(r->f));
  r->f = NULL;
  return true;
}

#define D(name) upbdefs_google_protobuf_ ## name(s)

static void reghandlers(const void *closure, upb_handlers *h) {
  const upb_symtab *s = closure;
  const upb_msgdef *m = upb_handlers_msgdef(h);

  if (m == D(DescriptorProto)) {
    upb_handlers_setstartmsg(h, &msg_startmsg, NULL);
    upb_handlers_setendmsg(h, &msg_endmsg, NULL);
    upb_handlers_setstring(h, D(DescriptorProto_name), &msg_onname, NULL);
    upb_handlers_setendsubmsg(h, D(DescriptorProto_field), &msg_onendfield,
                              NULL);
    upb_handlers_setendsubmsg(h, D(DescriptorProto_extension), &pushextension,
                              NULL);
  } else if (m == D(FileDescriptorProto)) {
    upb_handlers_setstartmsg(h, &file_startmsg, NULL);
    upb_handlers_setendmsg(h, &file_endmsg, NULL);
    upb_handlers_setstring(h, D(FileDescriptorProto_package), &file_onpackage,
                           NULL);
    upb_handlers_setendsubmsg(h, D(FileDescriptorProto_extension), &pushextension,
                              NULL);
  } else if (m == D(EnumValueDescriptorProto)) {
    upb_handlers_setstartmsg(h, &enumval_startmsg, NULL);
    upb_handlers_setendmsg(h, &enumval_endmsg, NULL);
    upb_handlers_setstring(h, D(EnumValueDescriptorProto_name), &enumval_onname, NULL);
    upb_handlers_setint32(h, D(EnumValueDescriptorProto_number), &enumval_onnumber,
                          NULL);
  } else if (m == D(EnumDescriptorProto)) {
    upb_handlers_setstartmsg(h, &enum_startmsg, NULL);
    upb_handlers_setendmsg(h, &enum_endmsg, NULL);
    upb_handlers_setstring(h, D(EnumDescriptorProto_name), &enum_onname, NULL);
  } else if (m == D(FieldDescriptorProto)) {
    upb_handlers_setstartmsg(h, &field_startmsg, NULL);
    upb_handlers_setendmsg(h, &field_endmsg, NULL);
    upb_handlers_setint32(h, D(FieldDescriptorProto_type), &field_ontype,
                          NULL);
    upb_handlers_setint32(h, D(FieldDescriptorProto_label), &field_onlabel,
                          NULL);
    upb_handlers_setint32(h, D(FieldDescriptorProto_number), &field_onnumber,
                          NULL);
    upb_handlers_setstring(h, D(FieldDescriptorProto_name), &field_onname,
                           NULL);
    upb_handlers_setstring(h, D(FieldDescriptorProto_type_name),
                           &field_ontypename, NULL);
    upb_handlers_setstring(h, D(FieldDescriptorProto_extendee),
                           &field_onextendee, NULL);
    upb_handlers_setstring(h, D(FieldDescriptorProto_default_value),
                           &field_ondefaultval, NULL);
  } else if (m == D(FieldOptions)) {
    upb_handlers_setbool(h, D(FieldOptions_lazy), &field_onlazy, NULL);
    upb_handlers_setbool(h, D(FieldOptions_packed), &field_onpacked, NULL);
  }
}

#undef D

void descreader_cleanup(void *_r) {
  upb_descreader *r = _r;
  free(r->name);
  upb_deflist_uninit(&r->defs);
  free(r->default_string);
  while (r->stack_len > 0) {
    upb_descreader_frame *f = &r->stack[--r->stack_len];
    free(f->name);
  }
}


/* Public API  ****************************************************************/

upb_descreader *upb_descreader_create(upb_env *e, const upb_handlers *h) {
  upb_descreader *r = upb_env_malloc(e, sizeof(upb_descreader));
  if (!r || !upb_env_addcleanup(e, descreader_cleanup, r)) {
    return NULL;
  }

  upb_deflist_init(&r->defs);
  upb_sink_reset(upb_descreader_input(r), h, r);
  r->stack_len = 0;
  r->name = NULL;
  r->default_string = NULL;

  return r;
}

upb_def **upb_descreader_getdefs(upb_descreader *r, void *owner, int *n) {
  *n = r->defs.len;
  upb_deflist_donaterefs(&r->defs, owner);
  return r->defs.defs;
}

upb_sink *upb_descreader_input(upb_descreader *r) {
  return &r->sink;
}

const upb_handlers *upb_descreader_newhandlers(const void *owner) {
  const upb_symtab *s = upbdefs_google_protobuf_descriptor(&s);
  const upb_handlers *h = upb_handlers_newfrozen(
      upbdefs_google_protobuf_FileDescriptorSet(s), owner, reghandlers, s);
  upb_symtab_unref(s, &s);
  return h;
}
/*
** protobuf decoder bytecode compiler
**
** Code to compile a upb::Handlers into bytecode for decoding a protobuf
** according to that specific schema and destination handlers.
**
** Compiling to bytecode is always the first step.  If we are using the
** interpreted decoder we leave it as bytecode and interpret that.  If we are
** using a JIT decoder we use a code generator to turn the bytecode into native
** code, LLVM IR, etc.
**
** Bytecode definition is in decoder.int.h.
*/

#include <stdarg.h>

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif

#define MAXLABEL 5
#define EMPTYLABEL -1

/* mgroup *********************************************************************/

static void freegroup(upb_refcounted *r) {
  mgroup *g = (mgroup*)r;
  upb_inttable_uninit(&g->methods);
#ifdef UPB_USE_JIT_X64
  upb_pbdecoder_freejit(g);
#endif
  free(g->bytecode);
  free(g);
}

static void visitgroup(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const mgroup *g = (const mgroup*)r;
  upb_inttable_iter i;
  upb_inttable_begin(&i, &g->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    visit(r, upb_pbdecodermethod_upcast(method), closure);
  }
}

mgroup *newgroup(const void *owner) {
  mgroup *g = malloc(sizeof(*g));
  static const struct upb_refcounted_vtbl vtbl = {visitgroup, freegroup};
  upb_refcounted_init(mgroup_upcast_mutable(g), &vtbl, owner);
  upb_inttable_init(&g->methods, UPB_CTYPE_PTR);
  g->bytecode = NULL;
  g->bytecode_end = NULL;
  return g;
}


/* upb_pbdecodermethod ********************************************************/

static void freemethod(upb_refcounted *r) {
  upb_pbdecodermethod *method = (upb_pbdecodermethod*)r;

  if (method->dest_handlers_) {
    upb_handlers_unref(method->dest_handlers_, method);
  }

  upb_inttable_uninit(&method->dispatch);
  free(method);
}

static void visitmethod(const upb_refcounted *r, upb_refcounted_visit *visit,
                        void *closure) {
  const upb_pbdecodermethod *m = (const upb_pbdecodermethod*)r;
  visit(r, m->group, closure);
}

static upb_pbdecodermethod *newmethod(const upb_handlers *dest_handlers,
                                      mgroup *group) {
  static const struct upb_refcounted_vtbl vtbl = {visitmethod, freemethod};
  upb_pbdecodermethod *ret = malloc(sizeof(*ret));
  upb_refcounted_init(upb_pbdecodermethod_upcast_mutable(ret), &vtbl, &ret);
  upb_byteshandler_init(&ret->input_handler_);

  /* The method references the group and vice-versa, in a circular reference. */
  upb_ref2(ret, group);
  upb_ref2(group, ret);
  upb_inttable_insertptr(&group->methods, dest_handlers, upb_value_ptr(ret));
  upb_pbdecodermethod_unref(ret, &ret);

  ret->group = mgroup_upcast_mutable(group);
  ret->dest_handlers_ = dest_handlers;
  ret->is_native_ = false;  /* If we JIT, it will update this later. */
  upb_inttable_init(&ret->dispatch, UPB_CTYPE_UINT64);

  if (ret->dest_handlers_) {
    upb_handlers_ref(ret->dest_handlers_, ret);
  }
  return ret;
}

const upb_handlers *upb_pbdecodermethod_desthandlers(
    const upb_pbdecodermethod *m) {
  return m->dest_handlers_;
}

const upb_byteshandler *upb_pbdecodermethod_inputhandler(
    const upb_pbdecodermethod *m) {
  return &m->input_handler_;
}

bool upb_pbdecodermethod_isnative(const upb_pbdecodermethod *m) {
  return m->is_native_;
}

const upb_pbdecodermethod *upb_pbdecodermethod_new(
    const upb_pbdecodermethodopts *opts, const void *owner) {
  const upb_pbdecodermethod *ret;
  upb_pbcodecache cache;

  upb_pbcodecache_init(&cache);
  ret = upb_pbcodecache_getdecodermethod(&cache, opts);
  upb_pbdecodermethod_ref(ret, owner);
  upb_pbcodecache_uninit(&cache);
  return ret;
}


/* bytecode compiler **********************************************************/

/* Data used only at compilation time. */
typedef struct {
  mgroup *group;

  uint32_t *pc;
  int fwd_labels[MAXLABEL];
  int back_labels[MAXLABEL];

  /* For fields marked "lazy", parse them lazily or eagerly? */
  bool lazy;
} compiler;

static compiler *newcompiler(mgroup *group, bool lazy) {
  compiler *ret = malloc(sizeof(*ret));
  int i;

  ret->group = group;
  ret->lazy = lazy;
  for (i = 0; i < MAXLABEL; i++) {
    ret->fwd_labels[i] = EMPTYLABEL;
    ret->back_labels[i] = EMPTYLABEL;
  }
  return ret;
}

static void freecompiler(compiler *c) {
  free(c);
}

const size_t ptr_words = sizeof(void*) / sizeof(uint32_t);

/* How many words an instruction is. */
static int instruction_len(uint32_t instr) {
  switch (getop(instr)) {
    case OP_SETDISPATCH: return 1 + ptr_words;
    case OP_TAGN: return 3;
    case OP_SETBIGGROUPNUM: return 2;
    default: return 1;
  }
}

bool op_has_longofs(int32_t instruction) {
  switch (getop(instruction)) {
    case OP_CALL:
    case OP_BRANCH:
    case OP_CHECKDELIM:
      return true;
    /* The "tag" instructions only have 8 bytes available for the jump target,
     * but that is ok because these opcodes only require short jumps. */
    case OP_TAG1:
    case OP_TAG2:
    case OP_TAGN:
      return false;
    default:
      assert(false);
      return false;
  }
}

static int32_t getofs(uint32_t instruction) {
  if (op_has_longofs(instruction)) {
    return (int32_t)instruction >> 8;
  } else {
    return (int8_t)(instruction >> 8);
  }
}

static void setofs(uint32_t *instruction, int32_t ofs) {
  if (op_has_longofs(*instruction)) {
    *instruction = getop(*instruction) | ofs << 8;
  } else {
    *instruction = (*instruction & ~0xff00) | ((ofs & 0xff) << 8);
  }
  assert(getofs(*instruction) == ofs);  /* Would fail in cases of overflow. */
}

static uint32_t pcofs(compiler *c) { return c->pc - c->group->bytecode; }

/* Defines a local label at the current PC location.  All previous forward
 * references are updated to point to this location.  The location is noted
 * for any future backward references. */
static void label(compiler *c, unsigned int label) {
  int val;
  uint32_t *codep;

  assert(label < MAXLABEL);
  val = c->fwd_labels[label];
  codep = (val == EMPTYLABEL) ? NULL : c->group->bytecode + val;
  while (codep) {
    int ofs = getofs(*codep);
    setofs(codep, c->pc - codep - instruction_len(*codep));
    codep = ofs ? codep + ofs : NULL;
  }
  c->fwd_labels[label] = EMPTYLABEL;
  c->back_labels[label] = pcofs(c);
}

/* Creates a reference to a numbered label; either a forward reference
 * (positive arg) or backward reference (negative arg).  For forward references
 * the value returned now is actually a "next" pointer into a linked list of all
 * instructions that use this label and will be patched later when the label is
 * defined with label().
 *
 * The returned value is the offset that should be written into the instruction.
 */
static int32_t labelref(compiler *c, int label) {
  assert(label < MAXLABEL);
  if (label == LABEL_DISPATCH) {
    /* No resolving required. */
    return 0;
  } else if (label < 0) {
    /* Backward local label.  Relative to the next instruction. */
    uint32_t from = (c->pc + 1) - c->group->bytecode;
    return c->back_labels[-label] - from;
  } else {
    /* Forward local label: prepend to (possibly-empty) linked list. */
    int *lptr = &c->fwd_labels[label];
    int32_t ret = (*lptr == EMPTYLABEL) ? 0 : *lptr - pcofs(c);
    *lptr = pcofs(c);
    return ret;
  }
}

static void put32(compiler *c, uint32_t v) {
  mgroup *g = c->group;
  if (c->pc == g->bytecode_end) {
    int ofs = pcofs(c);
    size_t oldsize = g->bytecode_end - g->bytecode;
    size_t newsize = UPB_MAX(oldsize * 2, 64);
    /* TODO(haberman): handle OOM. */
    g->bytecode = realloc(g->bytecode, newsize * sizeof(uint32_t));
    g->bytecode_end = g->bytecode + newsize;
    c->pc = g->bytecode + ofs;
  }
  *c->pc++ = v;
}

static void putop(compiler *c, opcode op, ...) {
  va_list ap;
  va_start(ap, op);

  switch (op) {
    case OP_SETDISPATCH: {
      uintptr_t ptr = (uintptr_t)va_arg(ap, void*);
      put32(c, OP_SETDISPATCH);
      put32(c, ptr);
      if (sizeof(uintptr_t) > sizeof(uint32_t))
        put32(c, (uint64_t)ptr >> 32);
      break;
    }
    case OP_STARTMSG:
    case OP_ENDMSG:
    case OP_PUSHLENDELIM:
    case OP_POP:
    case OP_SETDELIM:
    case OP_HALT:
    case OP_RET:
    case OP_DISPATCH:
      put32(c, op);
      break;
    case OP_PARSE_DOUBLE:
    case OP_PARSE_FLOAT:
    case OP_PARSE_INT64:
    case OP_PARSE_UINT64:
    case OP_PARSE_INT32:
    case OP_PARSE_FIXED64:
    case OP_PARSE_FIXED32:
    case OP_PARSE_BOOL:
    case OP_PARSE_UINT32:
    case OP_PARSE_SFIXED32:
    case OP_PARSE_SFIXED64:
    case OP_PARSE_SINT32:
    case OP_PARSE_SINT64:
    case OP_STARTSEQ:
    case OP_ENDSEQ:
    case OP_STARTSUBMSG:
    case OP_ENDSUBMSG:
    case OP_STARTSTR:
    case OP_STRING:
    case OP_ENDSTR:
    case OP_PUSHTAGDELIM:
      put32(c, op | va_arg(ap, upb_selector_t) << 8);
      break;
    case OP_SETBIGGROUPNUM:
      put32(c, op);
      put32(c, va_arg(ap, int));
      break;
    case OP_CALL: {
      const upb_pbdecodermethod *method = va_arg(ap, upb_pbdecodermethod *);
      put32(c, op | (method->code_base.ofs - (pcofs(c) + 1)) << 8);
      break;
    }
    case OP_CHECKDELIM:
    case OP_BRANCH: {
      uint32_t instruction = op;
      int label = va_arg(ap, int);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      break;
    }
    case OP_TAG1:
    case OP_TAG2: {
      int label = va_arg(ap, int);
      uint64_t tag = va_arg(ap, uint64_t);
      uint32_t instruction = op | (tag << 16);
      assert(tag <= 0xffff);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      break;
    }
    case OP_TAGN: {
      int label = va_arg(ap, int);
      uint64_t tag = va_arg(ap, uint64_t);
      uint32_t instruction = op | (upb_value_size(tag) << 16);
      setofs(&instruction, labelref(c, label));
      put32(c, instruction);
      put32(c, tag);
      put32(c, tag >> 32);
      break;
    }
  }

  va_end(ap);
}

#if defined(UPB_USE_JIT_X64) || defined(UPB_DUMP_BYTECODE)

const char *upb_pbdecoder_getopname(unsigned int op) {
#define QUOTE(x) #x
#define EXPAND_AND_QUOTE(x) QUOTE(x)
#define OPNAME(x) OP_##x
#define OP(x) case OPNAME(x): return EXPAND_AND_QUOTE(OPNAME(x));
#define T(x) OP(PARSE_##x)
  /* Keep in sync with list in decoder.int.h. */
  switch ((opcode)op) {
    T(DOUBLE) T(FLOAT) T(INT64) T(UINT64) T(INT32) T(FIXED64) T(FIXED32)
    T(BOOL) T(UINT32) T(SFIXED32) T(SFIXED64) T(SINT32) T(SINT64)
    OP(STARTMSG) OP(ENDMSG) OP(STARTSEQ) OP(ENDSEQ) OP(STARTSUBMSG)
    OP(ENDSUBMSG) OP(STARTSTR) OP(STRING) OP(ENDSTR) OP(CALL) OP(RET)
    OP(PUSHLENDELIM) OP(PUSHTAGDELIM) OP(SETDELIM) OP(CHECKDELIM)
    OP(BRANCH) OP(TAG1) OP(TAG2) OP(TAGN) OP(SETDISPATCH) OP(POP)
    OP(SETBIGGROUPNUM) OP(DISPATCH) OP(HALT)
  }
  return "<unknown op>";
#undef OP
#undef T
}

#endif

#ifdef UPB_DUMP_BYTECODE

static void dumpbc(uint32_t *p, uint32_t *end, FILE *f) {

  uint32_t *begin = p;

  while (p < end) {
    fprintf(f, "%p  %8tx", p, p - begin);
    uint32_t instr = *p++;
    uint8_t op = getop(instr);
    fprintf(f, " %s", upb_pbdecoder_getopname(op));
    switch ((opcode)op) {
      case OP_SETDISPATCH: {
        const upb_inttable *dispatch;
        memcpy(&dispatch, p, sizeof(void*));
        p += ptr_words;
        const upb_pbdecodermethod *method =
            (void *)((char *)dispatch -
                     offsetof(upb_pbdecodermethod, dispatch));
        fprintf(f, " %s", upb_msgdef_fullname(
                              upb_handlers_msgdef(method->dest_handlers_)));
        break;
      }
      case OP_DISPATCH:
      case OP_STARTMSG:
      case OP_ENDMSG:
      case OP_PUSHLENDELIM:
      case OP_POP:
      case OP_SETDELIM:
      case OP_HALT:
      case OP_RET:
        break;
      case OP_PARSE_DOUBLE:
      case OP_PARSE_FLOAT:
      case OP_PARSE_INT64:
      case OP_PARSE_UINT64:
      case OP_PARSE_INT32:
      case OP_PARSE_FIXED64:
      case OP_PARSE_FIXED32:
      case OP_PARSE_BOOL:
      case OP_PARSE_UINT32:
      case OP_PARSE_SFIXED32:
      case OP_PARSE_SFIXED64:
      case OP_PARSE_SINT32:
      case OP_PARSE_SINT64:
      case OP_STARTSEQ:
      case OP_ENDSEQ:
      case OP_STARTSUBMSG:
      case OP_ENDSUBMSG:
      case OP_STARTSTR:
      case OP_STRING:
      case OP_ENDSTR:
      case OP_PUSHTAGDELIM:
        fprintf(f, " %d", instr >> 8);
        break;
      case OP_SETBIGGROUPNUM:
        fprintf(f, " %d", *p++);
        break;
      case OP_CHECKDELIM:
      case OP_CALL:
      case OP_BRANCH:
        fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        break;
      case OP_TAG1:
      case OP_TAG2: {
        fprintf(f, " tag:0x%x", instr >> 16);
        if (getofs(instr)) {
          fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        }
        break;
      }
      case OP_TAGN: {
        uint64_t tag = *p++;
        tag |= (uint64_t)*p++ << 32;
        fprintf(f, " tag:0x%llx", (long long)tag);
        fprintf(f, " n:%d", instr >> 16);
        if (getofs(instr)) {
          fprintf(f, " =>0x%tx", p + getofs(instr) - begin);
        }
        break;
      }
    }
    fputs("\n", f);
  }
}

#endif

static uint64_t get_encoded_tag(const upb_fielddef *f, int wire_type) {
  uint32_t tag = (upb_fielddef_number(f) << 3) | wire_type;
  uint64_t encoded_tag = upb_vencode32(tag);
  /* No tag should be greater than 5 bytes. */
  assert(encoded_tag <= 0xffffffffff);
  return encoded_tag;
}

static void putchecktag(compiler *c, const upb_fielddef *f,
                        int wire_type, int dest) {
  uint64_t tag = get_encoded_tag(f, wire_type);
  switch (upb_value_size(tag)) {
    case 1:
      putop(c, OP_TAG1, dest, tag);
      break;
    case 2:
      putop(c, OP_TAG2, dest, tag);
      break;
    default:
      putop(c, OP_TAGN, dest, tag);
      break;
  }
}

static upb_selector_t getsel(const upb_fielddef *f, upb_handlertype_t type) {
  upb_selector_t selector;
  bool ok = upb_handlers_getselector(f, type, &selector);
  UPB_ASSERT_VAR(ok, ok);
  return selector;
}

/* Takes an existing, primary dispatch table entry and repacks it with a
 * different alternate wire type.  Called when we are inserting a secondary
 * dispatch table entry for an alternate wire type. */
static uint64_t repack(uint64_t dispatch, int new_wt2) {
  uint64_t ofs;
  uint8_t wt1;
  uint8_t old_wt2;
  upb_pbdecoder_unpackdispatch(dispatch, &ofs, &wt1, &old_wt2);
  assert(old_wt2 == NO_WIRE_TYPE);  /* wt2 should not be set yet. */
  return upb_pbdecoder_packdispatch(ofs, wt1, new_wt2);
}

/* Marks the current bytecode position as the dispatch target for this message,
 * field, and wire type. */
static void dispatchtarget(compiler *c, upb_pbdecodermethod *method,
                           const upb_fielddef *f, int wire_type) {
  /* Offset is relative to msg base. */
  uint64_t ofs = pcofs(c) - method->code_base.ofs;
  uint32_t fn = upb_fielddef_number(f);
  upb_inttable *d = &method->dispatch;
  upb_value v;
  if (upb_inttable_remove(d, fn, &v)) {
    /* TODO: prioritize based on packed setting in .proto file. */
    uint64_t repacked = repack(upb_value_getuint64(v), wire_type);
    upb_inttable_insert(d, fn, upb_value_uint64(repacked));
    upb_inttable_insert(d, fn + UPB_MAX_FIELDNUMBER, upb_value_uint64(ofs));
  } else {
    uint64_t val = upb_pbdecoder_packdispatch(ofs, wire_type, NO_WIRE_TYPE);
    upb_inttable_insert(d, fn, upb_value_uint64(val));
  }
}

static void putpush(compiler *c, const upb_fielddef *f) {
  if (upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_MESSAGE) {
    putop(c, OP_PUSHLENDELIM);
  } else {
    uint32_t fn = upb_fielddef_number(f);
    if (fn >= 1 << 24) {
      putop(c, OP_PUSHTAGDELIM, 0);
      putop(c, OP_SETBIGGROUPNUM, fn);
    } else {
      putop(c, OP_PUSHTAGDELIM, fn);
    }
  }
}

static upb_pbdecodermethod *find_submethod(const compiler *c,
                                           const upb_pbdecodermethod *method,
                                           const upb_fielddef *f) {
  const upb_handlers *sub =
      upb_handlers_getsubhandlers(method->dest_handlers_, f);
  upb_value v;
  return upb_inttable_lookupptr(&c->group->methods, sub, &v)
             ? upb_value_getptr(v)
             : NULL;
}

static void putsel(compiler *c, opcode op, upb_selector_t sel,
                   const upb_handlers *h) {
  if (upb_handlers_gethandler(h, sel)) {
    putop(c, op, sel);
  }
}

/* Puts an opcode to call a callback, but only if a callback actually exists for
 * this field and handler type. */
static void maybeput(compiler *c, opcode op, const upb_handlers *h,
                     const upb_fielddef *f, upb_handlertype_t type) {
  putsel(c, op, getsel(f, type), h);
}

static bool haslazyhandlers(const upb_handlers *h, const upb_fielddef *f) {
  if (!upb_fielddef_lazy(f))
    return false;

  return upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_STARTSTR)) ||
         upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_STRING)) ||
         upb_handlers_gethandler(h, getsel(f, UPB_HANDLER_ENDSTR));
}


/* bytecode compiler code generation ******************************************/

/* Symbolic names for our local labels. */
#define LABEL_LOOPSTART 1  /* Top of a repeated field loop. */
#define LABEL_LOOPBREAK 2  /* To jump out of a repeated loop */
#define LABEL_FIELD     3  /* Jump backward to find the most recent field. */
#define LABEL_ENDMSG    4  /* To reach the OP_ENDMSG instr for this msg. */

/* Generates bytecode to parse a single non-lazy message field. */
static void generate_msgfield(compiler *c, const upb_fielddef *f,
                              upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);
  const upb_pbdecodermethod *sub_m = find_submethod(c, method, f);
  int wire_type;

  if (!sub_m) {
    /* Don't emit any code for this field at all; it will be parsed as an
     * unknown field. */
    return;
  }

  label(c, LABEL_FIELD);

  wire_type =
      (upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_MESSAGE)
          ? UPB_WIRE_TYPE_DELIMITED
          : UPB_WIRE_TYPE_START_GROUP;

  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));
   label(c, LABEL_LOOPSTART);
    putpush(c, f);
    putop(c, OP_STARTSUBMSG, getsel(f, UPB_HANDLER_STARTSUBMSG));
    putop(c, OP_CALL, sub_m);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSUBMSG, h, f, UPB_HANDLER_ENDSUBMSG);
    if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
      putop(c, OP_SETDELIM);
    }
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, wire_type, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putpush(c, f);
    putop(c, OP_STARTSUBMSG, getsel(f, UPB_HANDLER_STARTSUBMSG));
    putop(c, OP_CALL, sub_m);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSUBMSG, h, f, UPB_HANDLER_ENDSUBMSG);
    if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
      putop(c, OP_SETDELIM);
    }
  }
}

/* Generates bytecode to parse a single string or lazy submessage field. */
static void generate_delimfield(compiler *c, const upb_fielddef *f,
                                upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);

  label(c, LABEL_FIELD);
  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));
   label(c, LABEL_LOOPSTART);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSTR, getsel(f, UPB_HANDLER_STARTSTR));
    /* Need to emit even if no handler to skip past the string. */
    putop(c, OP_STRING, getsel(f, UPB_HANDLER_STRING));
    putop(c, OP_POP);
    maybeput(c, OP_ENDSTR, h, f, UPB_HANDLER_ENDSTR);
    putop(c, OP_SETDELIM);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSTR, getsel(f, UPB_HANDLER_STARTSTR));
    putop(c, OP_STRING, getsel(f, UPB_HANDLER_STRING));
    putop(c, OP_POP);
    maybeput(c, OP_ENDSTR, h, f, UPB_HANDLER_ENDSTR);
    putop(c, OP_SETDELIM);
  }
}

/* Generates bytecode to parse a single primitive field. */
static void generate_primitivefield(compiler *c, const upb_fielddef *f,
                                    upb_pbdecodermethod *method) {
  const upb_handlers *h = upb_pbdecodermethod_desthandlers(method);
  upb_descriptortype_t descriptor_type = upb_fielddef_descriptortype(f);
  opcode parse_type;
  upb_selector_t sel;
  int wire_type;

  label(c, LABEL_FIELD);

  /* From a decoding perspective, ENUM is the same as INT32. */
  if (descriptor_type == UPB_DESCRIPTOR_TYPE_ENUM)
    descriptor_type = UPB_DESCRIPTOR_TYPE_INT32;

  parse_type = (opcode)descriptor_type;

  /* TODO(haberman): generate packed or non-packed first depending on "packed"
   * setting in the fielddef.  This will favor (in speed) whichever was
   * specified. */

  assert((int)parse_type >= 0 && parse_type <= OP_MAX);
  sel = getsel(f, upb_handlers_getprimitivehandlertype(f));
  wire_type = upb_pb_native_wire_types[upb_fielddef_descriptortype(f)];
  if (upb_fielddef_isseq(f)) {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, UPB_WIRE_TYPE_DELIMITED, LABEL_DISPATCH);
   dispatchtarget(c, method, f, UPB_WIRE_TYPE_DELIMITED);
    putop(c, OP_PUSHLENDELIM);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));  /* Packed */
   label(c, LABEL_LOOPSTART);
    putop(c, parse_type, sel);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   dispatchtarget(c, method, f, wire_type);
    putop(c, OP_PUSHTAGDELIM, 0);
    putop(c, OP_STARTSEQ, getsel(f, UPB_HANDLER_STARTSEQ));  /* Non-packed */
   label(c, LABEL_LOOPSTART);
    putop(c, parse_type, sel);
    putop(c, OP_CHECKDELIM, LABEL_LOOPBREAK);
    putchecktag(c, f, wire_type, LABEL_LOOPBREAK);
    putop(c, OP_BRANCH, -LABEL_LOOPSTART);
   label(c, LABEL_LOOPBREAK);
    putop(c, OP_POP);  /* Packed and non-packed join. */
    maybeput(c, OP_ENDSEQ, h, f, UPB_HANDLER_ENDSEQ);
    putop(c, OP_SETDELIM);  /* Could remove for non-packed by dup ENDSEQ. */
  } else {
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    putchecktag(c, f, wire_type, LABEL_DISPATCH);
   dispatchtarget(c, method, f, wire_type);
    putop(c, parse_type, sel);
  }
}

/* Adds bytecode for parsing the given message to the given decoderplan,
 * while adding all dispatch targets to this message's dispatch table. */
static void compile_method(compiler *c, upb_pbdecodermethod *method) {
  const upb_handlers *h;
  const upb_msgdef *md;
  uint32_t* start_pc;
  upb_msg_field_iter i;
  upb_value val;

  assert(method);

  /* Clear all entries in the dispatch table. */
  upb_inttable_uninit(&method->dispatch);
  upb_inttable_init(&method->dispatch, UPB_CTYPE_UINT64);

  h = upb_pbdecodermethod_desthandlers(method);
  md = upb_handlers_msgdef(h);

 method->code_base.ofs = pcofs(c);
  putop(c, OP_SETDISPATCH, &method->dispatch);
  putsel(c, OP_STARTMSG, UPB_STARTMSG_SELECTOR, h);
 label(c, LABEL_FIELD);
  start_pc = c->pc;
  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    upb_fieldtype_t type = upb_fielddef_type(f);

    if (type == UPB_TYPE_MESSAGE && !(haslazyhandlers(h, f) && c->lazy)) {
      generate_msgfield(c, f, method);
    } else if (type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES ||
               type == UPB_TYPE_MESSAGE) {
      generate_delimfield(c, f, method);
    } else {
      generate_primitivefield(c, f, method);
    }
  }

  /* If there were no fields, or if no handlers were defined, we need to
   * generate a non-empty loop body so that we can at least dispatch for unknown
   * fields and check for the end of the message. */
  if (c->pc == start_pc) {
    /* Check for end-of-message. */
    putop(c, OP_CHECKDELIM, LABEL_ENDMSG);
    /* Unconditionally dispatch. */
    putop(c, OP_DISPATCH, 0);
  }

  /* For now we just loop back to the last field of the message (or if none,
   * the DISPATCH opcode for the message). */
  putop(c, OP_BRANCH, -LABEL_FIELD);

  /* Insert both a label and a dispatch table entry for this end-of-msg. */
 label(c, LABEL_ENDMSG);
  val = upb_value_uint64(pcofs(c) - method->code_base.ofs);
  upb_inttable_insert(&method->dispatch, DISPATCH_ENDMSG, val);

  putsel(c, OP_ENDMSG, UPB_ENDMSG_SELECTOR, h);
  putop(c, OP_RET);

  upb_inttable_compact(&method->dispatch);
}

/* Populate "methods" with new upb_pbdecodermethod objects reachable from "h".
 * Returns the method for these handlers.
 *
 * Generates a new method for every destination handlers reachable from "h". */
static void find_methods(compiler *c, const upb_handlers *h) {
  upb_value v;
  upb_msg_field_iter i;
  const upb_msgdef *md;

  if (upb_inttable_lookupptr(&c->group->methods, h, &v))
    return;
  newmethod(h, c->group);

  /* Find submethods. */
  md = upb_handlers_msgdef(h);
  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    const upb_handlers *sub_h;
    if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE &&
        (sub_h = upb_handlers_getsubhandlers(h, f)) != NULL) {
      /* We only generate a decoder method for submessages with handlers.
       * Others will be parsed as unknown fields. */
      find_methods(c, sub_h);
    }
  }
}

/* (Re-)compile bytecode for all messages in "msgs."
 * Overwrites any existing bytecode in "c". */
static void compile_methods(compiler *c) {
  upb_inttable_iter i;

  /* Start over at the beginning of the bytecode. */
  c->pc = c->group->bytecode;

  upb_inttable_begin(&i, &c->group->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    compile_method(c, method);
  }
}

static void set_bytecode_handlers(mgroup *g) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &g->methods);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *m = upb_value_getptr(upb_inttable_iter_value(&i));
    upb_byteshandler *h = &m->input_handler_;

    m->code_base.ptr = g->bytecode + m->code_base.ofs;

    upb_byteshandler_setstartstr(h, upb_pbdecoder_startbc, m->code_base.ptr);
    upb_byteshandler_setstring(h, upb_pbdecoder_decode, g);
    upb_byteshandler_setendstr(h, upb_pbdecoder_end, m);
  }
}


/* JIT setup. *****************************************************************/

#ifdef UPB_USE_JIT_X64

static void sethandlers(mgroup *g, bool allowjit) {
  g->jit_code = NULL;
  if (allowjit) {
    /* Compile byte-code into machine code, create handlers. */
    upb_pbdecoder_jit(g);
  } else {
    set_bytecode_handlers(g);
  }
}

#else  /* UPB_USE_JIT_X64 */

static void sethandlers(mgroup *g, bool allowjit) {
  /* No JIT compiled in; use bytecode handlers unconditionally. */
  UPB_UNUSED(allowjit);
  set_bytecode_handlers(g);
}

#endif  /* UPB_USE_JIT_X64 */


/* TODO(haberman): allow this to be constructed for an arbitrary set of dest
 * handlers and other mgroups (but verify we have a transitive closure). */
const mgroup *mgroup_new(const upb_handlers *dest, bool allowjit, bool lazy,
                         const void *owner) {
  mgroup *g;
  compiler *c;

  UPB_UNUSED(allowjit);
  assert(upb_handlers_isfrozen(dest));

  g = newgroup(owner);
  c = newcompiler(g, lazy);
  find_methods(c, dest);

  /* We compile in two passes:
   * 1. all messages are assigned relative offsets from the beginning of the
   *    bytecode (saved in method->code_base).
   * 2. forwards OP_CALL instructions can be correctly linked since message
   *    offsets have been previously assigned.
   *
   * Could avoid the second pass by linking OP_CALL instructions somehow. */
  compile_methods(c);
  compile_methods(c);
  g->bytecode_end = c->pc;
  freecompiler(c);

#ifdef UPB_DUMP_BYTECODE
  {
    FILE *f = fopen("/tmp/upb-bytecode", "wb");
    assert(f);
    dumpbc(g->bytecode, g->bytecode_end, stderr);
    dumpbc(g->bytecode, g->bytecode_end, f);
    fclose(f);
  }
#endif

  sethandlers(g, allowjit);
  return g;
}


/* upb_pbcodecache ************************************************************/

void upb_pbcodecache_init(upb_pbcodecache *c) {
  upb_inttable_init(&c->groups, UPB_CTYPE_CONSTPTR);
  c->allow_jit_ = true;
}

void upb_pbcodecache_uninit(upb_pbcodecache *c) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &c->groups);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    const mgroup *group = upb_value_getconstptr(upb_inttable_iter_value(&i));
    mgroup_unref(group, c);
  }
  upb_inttable_uninit(&c->groups);
}

bool upb_pbcodecache_allowjit(const upb_pbcodecache *c) {
  return c->allow_jit_;
}

bool upb_pbcodecache_setallowjit(upb_pbcodecache *c, bool allow) {
  if (upb_inttable_count(&c->groups) > 0)
    return false;
  c->allow_jit_ = allow;
  return true;
}

const upb_pbdecodermethod *upb_pbcodecache_getdecodermethod(
    upb_pbcodecache *c, const upb_pbdecodermethodopts *opts) {
  upb_value v;
  bool ok;

  /* Right now we build a new DecoderMethod every time.
   * TODO(haberman): properly cache methods by their true key. */
  const mgroup *g = mgroup_new(opts->handlers, c->allow_jit_, opts->lazy, c);
  upb_inttable_push(&c->groups, upb_value_constptr(g));

  ok = upb_inttable_lookupptr(&g->methods, opts->handlers, &v);
  UPB_ASSERT_VAR(ok, ok);
  return upb_value_getptr(v);
}


/* upb_pbdecodermethodopts ****************************************************/

void upb_pbdecodermethodopts_init(upb_pbdecodermethodopts *opts,
                                  const upb_handlers *h) {
  opts->handlers = h;
  opts->lazy = false;
}

void upb_pbdecodermethodopts_setlazy(upb_pbdecodermethodopts *opts, bool lazy) {
  opts->lazy = lazy;
}
/*
** upb::Decoder (Bytecode Decoder VM)
**
** Bytecode must previously have been generated using the bytecode compiler in
** compile_decoder.c.  This decoder then walks through the bytecode op-by-op to
** parse the input.
**
** Decoding is fully resumable; we just keep a pointer to the current bytecode
** instruction and resume from there.  A fair amount of the logic here is to
** handle the fact that values can span buffer seams and we have to be able to
** be capable of suspending/resuming from any byte in the stream.  This
** sometimes requires keeping a few trailing bytes from the last buffer around
** in the "residual" buffer.
*/

#include <inttypes.h>
#include <stddef.h>

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif

#define CHECK_SUSPEND(x) if (!(x)) return upb_pbdecoder_suspend(d);

/* Error messages that are shared between the bytecode and JIT decoders. */
const char *kPbDecoderStackOverflow = "Nesting too deep.";
const char *kPbDecoderSubmessageTooLong =
    "Submessage end extends past enclosing submessage.";

/* Error messages shared within this file. */
static const char *kUnterminatedVarint = "Unterminated varint.";

/* upb_pbdecoder **************************************************************/

static opcode halt = OP_HALT;

/* Whether an op consumes any of the input buffer. */
static bool consumes_input(opcode op) {
  switch (op) {
    case OP_SETDISPATCH:
    case OP_STARTMSG:
    case OP_ENDMSG:
    case OP_STARTSEQ:
    case OP_ENDSEQ:
    case OP_STARTSUBMSG:
    case OP_ENDSUBMSG:
    case OP_STARTSTR:
    case OP_ENDSTR:
    case OP_PUSHTAGDELIM:
    case OP_POP:
    case OP_SETDELIM:
    case OP_SETBIGGROUPNUM:
    case OP_CHECKDELIM:
    case OP_CALL:
    case OP_RET:
    case OP_BRANCH:
      return false;
    default:
      return true;
  }
}

static size_t stacksize(upb_pbdecoder *d, size_t entries) {
  UPB_UNUSED(d);
  return entries * sizeof(upb_pbdecoder_frame);
}

static size_t callstacksize(upb_pbdecoder *d, size_t entries) {
  UPB_UNUSED(d);

#ifdef UPB_USE_JIT_X64
  if (d->method_->is_native_) {
    /* Each native stack frame needs two pointers, plus we need a few frames for
     * the enter/exit trampolines. */
    size_t ret = entries * sizeof(void*) * 2;
    ret += sizeof(void*) * 10;
    return ret;
  }
#endif

  return entries * sizeof(uint32_t*);
}


static bool in_residual_buf(const upb_pbdecoder *d, const char *p);

/* It's unfortunate that we have to micro-manage the compiler with
 * UPB_FORCEINLINE and UPB_NOINLINE, especially since this tuning is necessarily
 * specific to one hardware configuration.  But empirically on a Core i7,
 * performance increases 30-50% with these annotations.  Every instance where
 * these appear, gcc 4.2.1 made the wrong decision and degraded performance in
 * benchmarks. */

static void seterr(upb_pbdecoder *d, const char *msg) {
  upb_status status = UPB_STATUS_INIT;
  upb_status_seterrmsg(&status, msg);
  upb_env_reporterror(d->env, &status);
}

void upb_pbdecoder_seterr(upb_pbdecoder *d, const char *msg) {
  seterr(d, msg);
}


/* Buffering ******************************************************************/

/* We operate on one buffer at a time, which is either the user's buffer passed
 * to our "decode" callback or some residual bytes from the previous buffer. */

/* How many bytes can be safely read from d->ptr without reading past end-of-buf
 * or past the current delimited end. */
static size_t curbufleft(const upb_pbdecoder *d) {
  assert(d->data_end >= d->ptr);
  return d->data_end - d->ptr;
}

/* How many bytes are available before end-of-buffer. */
static size_t bufleft(const upb_pbdecoder *d) {
  return d->end - d->ptr;
}

/* Overall stream offset of d->ptr. */
uint64_t offset(const upb_pbdecoder *d) {
  return d->bufstart_ofs + (d->ptr - d->buf);
}

/* How many bytes are available before the end of this delimited region. */
size_t delim_remaining(const upb_pbdecoder *d) {
  return d->top->end_ofs - offset(d);
}

/* Advances d->ptr. */
static void advance(upb_pbdecoder *d, size_t len) {
  assert(curbufleft(d) >= len);
  d->ptr += len;
}

static bool in_buf(const char *p, const char *buf, const char *end) {
  return p >= buf && p <= end;
}

static bool in_residual_buf(const upb_pbdecoder *d, const char *p) {
  return in_buf(p, d->residual, d->residual_end);
}

/* Calculates the delim_end value, which is affected by both the current buffer
 * and the parsing stack, so must be called whenever either is updated. */
static void set_delim_end(upb_pbdecoder *d) {
  size_t delim_ofs = d->top->end_ofs - d->bufstart_ofs;
  if (delim_ofs <= (size_t)(d->end - d->buf)) {
    d->delim_end = d->buf + delim_ofs;
    d->data_end = d->delim_end;
  } else {
    d->data_end = d->end;
    d->delim_end = NULL;
  }
}

static void switchtobuf(upb_pbdecoder *d, const char *buf, const char *end) {
  d->ptr = buf;
  d->buf = buf;
  d->end = end;
  set_delim_end(d);
}

static void advancetobuf(upb_pbdecoder *d, const char *buf, size_t len) {
  assert(curbufleft(d) == 0);
  d->bufstart_ofs += (d->end - d->buf);
  switchtobuf(d, buf, buf + len);
}

static void checkpoint(upb_pbdecoder *d) {
  /* The assertion here is in the interests of efficiency, not correctness.
   * We are trying to ensure that we don't checkpoint() more often than
   * necessary. */
  assert(d->checkpoint != d->ptr);
  d->checkpoint = d->ptr;
}

/* Skips "bytes" bytes in the stream, which may be more than available.  If we
 * skip more bytes than are available, we return a long read count to the caller
 * indicating how many bytes can be skipped over before passing actual data
 * again.  Skipped bytes can pass a NULL buffer and the decoder guarantees they
 * won't actually be read.
 */
static int32_t skip(upb_pbdecoder *d, size_t bytes) {
  assert(!in_residual_buf(d, d->ptr) || d->size_param == 0);
  assert(d->skip == 0);
  if (bytes > delim_remaining(d)) {
    seterr(d, "Skipped value extended beyond enclosing submessage.");
    return upb_pbdecoder_suspend(d);
  } else if (bufleft(d) > bytes) {
    /* Skipped data is all in current buffer, and more is still available. */
    advance(d, bytes);
    d->skip = 0;
    return DECODE_OK;
  } else {
    /* Skipped data extends beyond currently available buffers. */
    d->pc = d->last;
    d->skip = bytes - curbufleft(d);
    d->bufstart_ofs += (d->end - d->buf);
    d->residual_end = d->residual;
    switchtobuf(d, d->residual, d->residual_end);
    return d->size_param + d->skip;
  }
}


/* Resumes the decoder from an initial state or from a previous suspend. */
int32_t upb_pbdecoder_resume(upb_pbdecoder *d, void *p, const char *buf,
                             size_t size, const upb_bufhandle *handle) {
  UPB_UNUSED(p);  /* Useless; just for the benefit of the JIT. */

  d->buf_param = buf;
  d->size_param = size;
  d->handle = handle;

  if (d->residual_end > d->residual) {
    /* We have residual bytes from the last buffer. */
    assert(d->ptr == d->residual);
  } else {
    switchtobuf(d, buf, buf + size);
  }

  d->checkpoint = d->ptr;

  if (d->skip) {
    size_t skip_bytes = d->skip;
    d->skip = 0;
    CHECK_RETURN(skip(d, skip_bytes));
    d->checkpoint = d->ptr;
  }

  if (!buf) {
    /* NULL buf is ok if its entire span is covered by the "skip" above, but
     * by this point we know that "skip" doesn't cover the buffer. */
    seterr(d, "Passed NULL buffer over non-skippable region.");
    return upb_pbdecoder_suspend(d);
  }

  if (d->top->groupnum < 0) {
    CHECK_RETURN(upb_pbdecoder_skipunknown(d, -1, 0));
    d->checkpoint = d->ptr;
  }

  return DECODE_OK;
}

/* Suspends the decoder at the last checkpoint, without saving any residual
 * bytes.  If there are any unconsumed bytes, returns a short byte count. */
size_t upb_pbdecoder_suspend(upb_pbdecoder *d) {
  d->pc = d->last;
  if (d->checkpoint == d->residual) {
    /* Checkpoint was in residual buf; no user bytes were consumed. */
    d->ptr = d->residual;
    return 0;
  } else {
    size_t consumed;
    assert(!in_residual_buf(d, d->checkpoint));
    assert(d->buf == d->buf_param);

    consumed = d->checkpoint - d->buf;
    d->bufstart_ofs += consumed;
    d->residual_end = d->residual;
    switchtobuf(d, d->residual, d->residual_end);
    return consumed;
  }
}

/* Suspends the decoder at the last checkpoint, and saves any unconsumed
 * bytes in our residual buffer.  This is necessary if we need more user
 * bytes to form a complete value, which might not be contiguous in the
 * user's buffers.  Always consumes all user bytes. */
static size_t suspend_save(upb_pbdecoder *d) {
  /* We hit end-of-buffer before we could parse a full value.
   * Save any unconsumed bytes (if any) to the residual buffer. */
  d->pc = d->last;

  if (d->checkpoint == d->residual) {
    /* Checkpoint was in residual buf; append user byte(s) to residual buf. */
    assert((d->residual_end - d->residual) + d->size_param <=
           sizeof(d->residual));
    if (!in_residual_buf(d, d->ptr)) {
      d->bufstart_ofs -= (d->residual_end - d->residual);
    }
    memcpy(d->residual_end, d->buf_param, d->size_param);
    d->residual_end += d->size_param;
  } else {
    /* Checkpoint was in user buf; old residual bytes not needed. */
    size_t save;
    assert(!in_residual_buf(d, d->checkpoint));

    d->ptr = d->checkpoint;
    save = curbufleft(d);
    assert(save <= sizeof(d->residual));
    memcpy(d->residual, d->ptr, save);
    d->residual_end = d->residual + save;
    d->bufstart_ofs = offset(d);
  }

  switchtobuf(d, d->residual, d->residual_end);
  return d->size_param;
}

/* Copies the next "bytes" bytes into "buf" and advances the stream.
 * Requires that this many bytes are available in the current buffer. */
UPB_FORCEINLINE static void consumebytes(upb_pbdecoder *d, void *buf,
                                         size_t bytes) {
  assert(bytes <= curbufleft(d));
  memcpy(buf, d->ptr, bytes);
  advance(d, bytes);
}

/* Slow path for getting the next "bytes" bytes, regardless of whether they are
 * available in the current buffer or not.  Returns a status code as described
 * in decoder.int.h. */
UPB_NOINLINE static int32_t getbytes_slow(upb_pbdecoder *d, void *buf,
                                          size_t bytes) {
  const size_t avail = curbufleft(d);
  consumebytes(d, buf, avail);
  bytes -= avail;
  assert(bytes > 0);
  if (in_residual_buf(d, d->ptr)) {
    advancetobuf(d, d->buf_param, d->size_param);
  }
  if (curbufleft(d) >= bytes) {
    consumebytes(d, (char *)buf + avail, bytes);
    return DECODE_OK;
  } else if (d->data_end == d->delim_end) {
    seterr(d, "Submessage ended in the middle of a value or group");
    return upb_pbdecoder_suspend(d);
  } else {
    return suspend_save(d);
  }
}

/* Gets the next "bytes" bytes, regardless of whether they are available in the
 * current buffer or not.  Returns a status code as described in decoder.int.h.
 */
UPB_FORCEINLINE static int32_t getbytes(upb_pbdecoder *d, void *buf,
                                        size_t bytes) {
  if (curbufleft(d) >= bytes) {
    /* Buffer has enough data to satisfy. */
    consumebytes(d, buf, bytes);
    return DECODE_OK;
  } else {
    return getbytes_slow(d, buf, bytes);
  }
}

UPB_NOINLINE static size_t peekbytes_slow(upb_pbdecoder *d, void *buf,
                                          size_t bytes) {
  size_t ret = curbufleft(d);
  memcpy(buf, d->ptr, ret);
  if (in_residual_buf(d, d->ptr)) {
    size_t copy = UPB_MIN(bytes - ret, d->size_param);
    memcpy((char *)buf + ret, d->buf_param, copy);
    ret += copy;
  }
  return ret;
}

UPB_FORCEINLINE static size_t peekbytes(upb_pbdecoder *d, void *buf,
                                        size_t bytes) {
  if (curbufleft(d) >= bytes) {
    memcpy(buf, d->ptr, bytes);
    return bytes;
  } else {
    return peekbytes_slow(d, buf, bytes);
  }
}


/* Decoding of wire types *****************************************************/

/* Slow path for decoding a varint from the current buffer position.
 * Returns a status code as described in decoder.int.h. */
UPB_NOINLINE int32_t upb_pbdecoder_decode_varint_slow(upb_pbdecoder *d,
                                                      uint64_t *u64) {
  uint8_t byte = 0x80;
  int bitpos;
  *u64 = 0;
  for(bitpos = 0; bitpos < 70 && (byte & 0x80); bitpos += 7) {
    int32_t ret = getbytes(d, &byte, 1);
    if (ret >= 0) return ret;
    *u64 |= (uint64_t)(byte & 0x7F) << bitpos;
  }
  if(bitpos == 70 && (byte & 0x80)) {
    seterr(d, kUnterminatedVarint);
    return upb_pbdecoder_suspend(d);
  }
  return DECODE_OK;
}

/* Decodes a varint from the current buffer position.
 * Returns a status code as described in decoder.int.h. */
UPB_FORCEINLINE static int32_t decode_varint(upb_pbdecoder *d, uint64_t *u64) {
  if (curbufleft(d) > 0 && !(*d->ptr & 0x80)) {
    *u64 = *d->ptr;
    advance(d, 1);
    return DECODE_OK;
  } else if (curbufleft(d) >= 10) {
    /* Fast case. */
    upb_decoderet r = upb_vdecode_fast(d->ptr);
    if (r.p == NULL) {
      seterr(d, kUnterminatedVarint);
      return upb_pbdecoder_suspend(d);
    }
    advance(d, r.p - d->ptr);
    *u64 = r.val;
    return DECODE_OK;
  } else {
    /* Slow case -- varint spans buffer seam. */
    return upb_pbdecoder_decode_varint_slow(d, u64);
  }
}

/* Decodes a 32-bit varint from the current buffer position.
 * Returns a status code as described in decoder.int.h. */
UPB_FORCEINLINE static int32_t decode_v32(upb_pbdecoder *d, uint32_t *u32) {
  uint64_t u64;
  int32_t ret = decode_varint(d, &u64);
  if (ret >= 0) return ret;
  if (u64 > UINT32_MAX) {
    seterr(d, "Unterminated 32-bit varint");
    /* TODO(haberman) guarantee that this function return is >= 0 somehow,
     * so we know this path will always be treated as error by our caller.
     * Right now the size_t -> int32_t can overflow and produce negative values.
     */
    *u32 = 0;
    return upb_pbdecoder_suspend(d);
  }
  *u32 = u64;
  return DECODE_OK;
}

/* Decodes a fixed32 from the current buffer position.
 * Returns a status code as described in decoder.int.h.
 * TODO: proper byte swapping for big-endian machines. */
UPB_FORCEINLINE static int32_t decode_fixed32(upb_pbdecoder *d, uint32_t *u32) {
  return getbytes(d, u32, 4);
}

/* Decodes a fixed64 from the current buffer position.
 * Returns a status code as described in decoder.int.h.
 * TODO: proper byte swapping for big-endian machines. */
UPB_FORCEINLINE static int32_t decode_fixed64(upb_pbdecoder *d, uint64_t *u64) {
  return getbytes(d, u64, 8);
}

/* Non-static versions of the above functions.
 * These are called by the JIT for fallback paths. */
int32_t upb_pbdecoder_decode_f32(upb_pbdecoder *d, uint32_t *u32) {
  return decode_fixed32(d, u32);
}

int32_t upb_pbdecoder_decode_f64(upb_pbdecoder *d, uint64_t *u64) {
  return decode_fixed64(d, u64);
}

static double as_double(uint64_t n) { double d; memcpy(&d, &n, 8); return d; }
static float  as_float(uint32_t n)  { float  f; memcpy(&f, &n, 4); return f; }

/* Pushes a frame onto the decoder stack. */
static bool decoder_push(upb_pbdecoder *d, uint64_t end) {
  upb_pbdecoder_frame *fr = d->top;

  if (end > fr->end_ofs) {
    seterr(d, kPbDecoderSubmessageTooLong);
    return false;
  } else if (fr == d->limit) {
    seterr(d, kPbDecoderStackOverflow);
    return false;
  }

  fr++;
  fr->end_ofs = end;
  fr->dispatch = NULL;
  fr->groupnum = 0;
  d->top = fr;
  return true;
}

static bool pushtagdelim(upb_pbdecoder *d, uint32_t arg) {
  /* While we expect to see an "end" tag (either ENDGROUP or a non-sequence
   * field number) prior to hitting any enclosing submessage end, pushing our
   * existing delim end prevents us from continuing to parse values from a
   * corrupt proto that doesn't give us an END tag in time. */
  if (!decoder_push(d, d->top->end_ofs))
    return false;
  d->top->groupnum = arg;
  return true;
}

/* Pops a frame from the decoder stack. */
static void decoder_pop(upb_pbdecoder *d) { d->top--; }

UPB_NOINLINE int32_t upb_pbdecoder_checktag_slow(upb_pbdecoder *d,
                                                 uint64_t expected) {
  uint64_t data = 0;
  size_t bytes = upb_value_size(expected);
  size_t read = peekbytes(d, &data, bytes);
  if (read == bytes && data == expected) {
    /* Advance past matched bytes. */
    int32_t ok = getbytes(d, &data, read);
    UPB_ASSERT_VAR(ok, ok < 0);
    return DECODE_OK;
  } else if (read < bytes && memcmp(&data, &expected, read) == 0) {
    return suspend_save(d);
  } else {
    return DECODE_MISMATCH;
  }
}

int32_t upb_pbdecoder_skipunknown(upb_pbdecoder *d, int32_t fieldnum,
                                  uint8_t wire_type) {
  if (fieldnum >= 0)
    goto have_tag;

  while (true) {
    uint32_t tag;
    CHECK_RETURN(decode_v32(d, &tag));
    wire_type = tag & 0x7;
    fieldnum = tag >> 3;

have_tag:
    if (fieldnum == 0) {
      seterr(d, "Saw invalid field number (0)");
      return upb_pbdecoder_suspend(d);
    }

    /* TODO: deliver to unknown field callback. */
    switch (wire_type) {
      case UPB_WIRE_TYPE_32BIT:
        CHECK_RETURN(skip(d, 4));
        break;
      case UPB_WIRE_TYPE_64BIT:
        CHECK_RETURN(skip(d, 8));
        break;
      case UPB_WIRE_TYPE_VARINT: {
        uint64_t u64;
        CHECK_RETURN(decode_varint(d, &u64));
        break;
      }
      case UPB_WIRE_TYPE_DELIMITED: {
        uint32_t len;
        CHECK_RETURN(decode_v32(d, &len));
        CHECK_RETURN(skip(d, len));
        break;
      }
      case UPB_WIRE_TYPE_START_GROUP:
        CHECK_SUSPEND(pushtagdelim(d, -fieldnum));
        break;
      case UPB_WIRE_TYPE_END_GROUP:
        if (fieldnum == -d->top->groupnum) {
          decoder_pop(d);
        } else if (fieldnum == d->top->groupnum) {
          return DECODE_ENDGROUP;
        } else {
          seterr(d, "Unmatched ENDGROUP tag.");
          return upb_pbdecoder_suspend(d);
        }
        break;
      default:
        seterr(d, "Invalid wire type");
        return upb_pbdecoder_suspend(d);
    }

    if (d->top->groupnum >= 0) {
      return DECODE_OK;
    }

    /* Unknown group -- continue looping over unknown fields. */
    checkpoint(d);
  }
}

static void goto_endmsg(upb_pbdecoder *d) {
  upb_value v;
  bool found = upb_inttable_lookup32(d->top->dispatch, DISPATCH_ENDMSG, &v);
  UPB_ASSERT_VAR(found, found);
  d->pc = d->top->base + upb_value_getuint64(v);
}

/* Parses a tag and jumps to the corresponding bytecode instruction for this
 * field.
 *
 * If the tag is unknown (or the wire type doesn't match), parses the field as
 * unknown.  If the tag is a valid ENDGROUP tag, jumps to the bytecode
 * instruction for the end of message. */
static int32_t dispatch(upb_pbdecoder *d) {
  upb_inttable *dispatch = d->top->dispatch;
  uint32_t tag;
  uint8_t wire_type;
  uint32_t fieldnum;
  upb_value val;
  int32_t retval;

  /* Decode tag. */
  CHECK_RETURN(decode_v32(d, &tag));
  wire_type = tag & 0x7;
  fieldnum = tag >> 3;

  /* Lookup tag.  Because of packed/non-packed compatibility, we have to
   * check the wire type against two possibilities. */
  if (fieldnum != DISPATCH_ENDMSG &&
      upb_inttable_lookup32(dispatch, fieldnum, &val)) {
    uint64_t v = upb_value_getuint64(val);
    if (wire_type == (v & 0xff)) {
      d->pc = d->top->base + (v >> 16);
      return DECODE_OK;
    } else if (wire_type == ((v >> 8) & 0xff)) {
      bool found =
          upb_inttable_lookup(dispatch, fieldnum + UPB_MAX_FIELDNUMBER, &val);
      UPB_ASSERT_VAR(found, found);
      d->pc = d->top->base + upb_value_getuint64(val);
      return DECODE_OK;
    }
  }

  /* We have some unknown fields (or ENDGROUP) to parse.  The DISPATCH or TAG
   * bytecode that triggered this is preceded by a CHECKDELIM bytecode which
   * we need to back up to, so that when we're done skipping unknown data we
   * can re-check the delimited end. */
  d->last--;  /* Necessary if we get suspended */
  d->pc = d->last;
  assert(getop(*d->last) == OP_CHECKDELIM);

  /* Unknown field or ENDGROUP. */
  retval = upb_pbdecoder_skipunknown(d, fieldnum, wire_type);

  CHECK_RETURN(retval);

  if (retval == DECODE_ENDGROUP) {
    goto_endmsg(d);
    return DECODE_OK;
  }

  return DECODE_OK;
}

/* Callers know that the stack is more than one deep because the opcodes that
 * call this only occur after PUSH operations. */
upb_pbdecoder_frame *outer_frame(upb_pbdecoder *d) {
  assert(d->top != d->stack);
  return d->top - 1;
}


/* The main decoding loop *****************************************************/

/* The main decoder VM function.  Uses traditional bytecode dispatch loop with a
 * switch() statement. */
size_t run_decoder_vm(upb_pbdecoder *d, const mgroup *group,
                      const upb_bufhandle* handle) {

#define VMCASE(op, code) \
  case op: { code; if (consumes_input(op)) checkpoint(d); break; }
#define PRIMITIVE_OP(type, wt, name, convfunc, ctype) \
  VMCASE(OP_PARSE_ ## type, { \
    ctype val; \
    CHECK_RETURN(decode_ ## wt(d, &val)); \
    upb_sink_put ## name(&d->top->sink, arg, (convfunc)(val)); \
  })

  while(1) {
    int32_t instruction;
    opcode op;
    uint32_t arg;
    int32_t longofs;

    d->last = d->pc;
    instruction = *d->pc++;
    op = getop(instruction);
    arg = instruction >> 8;
    longofs = arg;
    assert(d->ptr != d->residual_end);
    UPB_UNUSED(group);
#ifdef UPB_DUMP_BYTECODE
    fprintf(stderr, "s_ofs=%d buf_ofs=%d data_rem=%d buf_rem=%d delim_rem=%d "
                    "%x %s (%d)\n",
            (int)offset(d),
            (int)(d->ptr - d->buf),
            (int)(d->data_end - d->ptr),
            (int)(d->end - d->ptr),
            (int)((d->top->end_ofs - d->bufstart_ofs) - (d->ptr - d->buf)),
            (int)(d->pc - 1 - group->bytecode),
            upb_pbdecoder_getopname(op),
            arg);
#endif
    switch (op) {
      /* Technically, we are losing data if we see a 32-bit varint that is not
       * properly sign-extended.  We could detect this and error about the data
       * loss, but proto2 does not do this, so we pass. */
      PRIMITIVE_OP(INT32,    varint,  int32,  int32_t,      uint64_t)
      PRIMITIVE_OP(INT64,    varint,  int64,  int64_t,      uint64_t)
      PRIMITIVE_OP(UINT32,   varint,  uint32, uint32_t,     uint64_t)
      PRIMITIVE_OP(UINT64,   varint,  uint64, uint64_t,     uint64_t)
      PRIMITIVE_OP(FIXED32,  fixed32, uint32, uint32_t,     uint32_t)
      PRIMITIVE_OP(FIXED64,  fixed64, uint64, uint64_t,     uint64_t)
      PRIMITIVE_OP(SFIXED32, fixed32, int32,  int32_t,      uint32_t)
      PRIMITIVE_OP(SFIXED64, fixed64, int64,  int64_t,      uint64_t)
      PRIMITIVE_OP(BOOL,     varint,  bool,   bool,         uint64_t)
      PRIMITIVE_OP(DOUBLE,   fixed64, double, as_double,    uint64_t)
      PRIMITIVE_OP(FLOAT,    fixed32, float,  as_float,     uint32_t)
      PRIMITIVE_OP(SINT32,   varint,  int32,  upb_zzdec_32, uint64_t)
      PRIMITIVE_OP(SINT64,   varint,  int64,  upb_zzdec_64, uint64_t)

      VMCASE(OP_SETDISPATCH,
        d->top->base = d->pc - 1;
        memcpy(&d->top->dispatch, d->pc, sizeof(void*));
        d->pc += sizeof(void*) / sizeof(uint32_t);
      )
      VMCASE(OP_STARTMSG,
        CHECK_SUSPEND(upb_sink_startmsg(&d->top->sink));
      )
      VMCASE(OP_ENDMSG,
        CHECK_SUSPEND(upb_sink_endmsg(&d->top->sink, d->status));
      )
      VMCASE(OP_STARTSEQ,
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startseq(&outer->sink, arg, &d->top->sink));
      )
      VMCASE(OP_ENDSEQ,
        CHECK_SUSPEND(upb_sink_endseq(&d->top->sink, arg));
      )
      VMCASE(OP_STARTSUBMSG,
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startsubmsg(&outer->sink, arg, &d->top->sink));
      )
      VMCASE(OP_ENDSUBMSG,
        CHECK_SUSPEND(upb_sink_endsubmsg(&d->top->sink, arg));
      )
      VMCASE(OP_STARTSTR,
        uint32_t len = delim_remaining(d);
        upb_pbdecoder_frame *outer = outer_frame(d);
        CHECK_SUSPEND(upb_sink_startstr(&outer->sink, arg, len, &d->top->sink));
        if (len == 0) {
          d->pc++;  /* Skip OP_STRING. */
        }
      )
      VMCASE(OP_STRING,
        uint32_t len = curbufleft(d);
        size_t n = upb_sink_putstring(&d->top->sink, arg, d->ptr, len, handle);
        if (n > len) {
          if (n > delim_remaining(d)) {
            seterr(d, "Tried to skip past end of string.");
            return upb_pbdecoder_suspend(d);
          } else {
            int32_t ret = skip(d, n);
            /* This shouldn't return DECODE_OK, because n > len. */
            assert(ret >= 0);
            return ret;
          }
        }
        advance(d, n);
        if (n < len || d->delim_end == NULL) {
          /* We aren't finished with this string yet. */
          d->pc--;  /* Repeat OP_STRING. */
          if (n > 0) checkpoint(d);
          return upb_pbdecoder_suspend(d);
        }
      )
      VMCASE(OP_ENDSTR,
        CHECK_SUSPEND(upb_sink_endstr(&d->top->sink, arg));
      )
      VMCASE(OP_PUSHTAGDELIM,
        CHECK_SUSPEND(pushtagdelim(d, arg));
      )
      VMCASE(OP_SETBIGGROUPNUM,
        d->top->groupnum = *d->pc++;
      )
      VMCASE(OP_POP,
        assert(d->top > d->stack);
        decoder_pop(d);
      )
      VMCASE(OP_PUSHLENDELIM,
        uint32_t len;
        CHECK_RETURN(decode_v32(d, &len));
        CHECK_SUSPEND(decoder_push(d, offset(d) + len));
        set_delim_end(d);
      )
      VMCASE(OP_SETDELIM,
        set_delim_end(d);
      )
      VMCASE(OP_CHECKDELIM,
        /* We are guaranteed of this assert because we never allow ourselves to
         * consume bytes beyond data_end, which covers delim_end when non-NULL.
         */
        assert(!(d->delim_end && d->ptr > d->delim_end));
        if (d->ptr == d->delim_end)
          d->pc += longofs;
      )
      VMCASE(OP_CALL,
        d->callstack[d->call_len++] = d->pc;
        d->pc += longofs;
      )
      VMCASE(OP_RET,
        assert(d->call_len > 0);
        d->pc = d->callstack[--d->call_len];
      )
      VMCASE(OP_BRANCH,
        d->pc += longofs;
      )
      VMCASE(OP_TAG1,
        uint8_t expected;
        CHECK_SUSPEND(curbufleft(d) > 0);
        expected = (arg >> 8) & 0xff;
        if (*d->ptr == expected) {
          advance(d, 1);
        } else {
          int8_t shortofs;
         badtag:
          shortofs = arg;
          if (shortofs == LABEL_DISPATCH) {
            CHECK_RETURN(dispatch(d));
          } else {
            d->pc += shortofs;
            break; /* Avoid checkpoint(). */
          }
        }
      )
      VMCASE(OP_TAG2,
        uint16_t expected;
        CHECK_SUSPEND(curbufleft(d) > 0);
        expected = (arg >> 8) & 0xffff;
        if (curbufleft(d) >= 2) {
          uint16_t actual;
          memcpy(&actual, d->ptr, 2);
          if (expected == actual) {
            advance(d, 2);
          } else {
            goto badtag;
          }
        } else {
          int32_t result = upb_pbdecoder_checktag_slow(d, expected);
          if (result == DECODE_MISMATCH) goto badtag;
          if (result >= 0) return result;
        }
      )
      VMCASE(OP_TAGN, {
        uint64_t expected;
        int32_t result;
        memcpy(&expected, d->pc, 8);
        d->pc += 2;
        result = upb_pbdecoder_checktag_slow(d, expected);
        if (result == DECODE_MISMATCH) goto badtag;
        if (result >= 0) return result;
      })
      VMCASE(OP_DISPATCH, {
        CHECK_RETURN(dispatch(d));
      })
      VMCASE(OP_HALT, {
        return d->size_param;
      })
    }
  }
}


/* BytesHandler handlers ******************************************************/

void *upb_pbdecoder_startbc(void *closure, const void *pc, size_t size_hint) {
  upb_pbdecoder *d = closure;
  UPB_UNUSED(size_hint);
  d->top->end_ofs = UINT64_MAX;
  d->bufstart_ofs = 0;
  d->call_len = 1;
  d->callstack[0] = &halt;
  d->pc = pc;
  d->skip = 0;
  return d;
}

void *upb_pbdecoder_startjit(void *closure, const void *hd, size_t size_hint) {
  upb_pbdecoder *d = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(size_hint);
  d->top->end_ofs = UINT64_MAX;
  d->bufstart_ofs = 0;
  d->call_len = 0;
  d->skip = 0;
  return d;
}

bool upb_pbdecoder_end(void *closure, const void *handler_data) {
  upb_pbdecoder *d = closure;
  const upb_pbdecodermethod *method = handler_data;
  uint64_t end;
  char dummy;

  if (d->residual_end > d->residual) {
    seterr(d, "Unexpected EOF: decoder still has buffered unparsed data");
    return false;
  }

  if (d->skip) {
    seterr(d, "Unexpected EOF inside skipped data");
    return false;
  }

  if (d->top->end_ofs != UINT64_MAX) {
    seterr(d, "Unexpected EOF inside delimited string");
    return false;
  }

  /* The user's end() call indicates that the message ends here. */
  end = offset(d);
  d->top->end_ofs = end;

#ifdef UPB_USE_JIT_X64
  if (method->is_native_) {
    const mgroup *group = (const mgroup*)method->group;
    if (d->top != d->stack)
      d->stack->end_ofs = 0;
    group->jit_code(closure, method->code_base.ptr, &dummy, 0, NULL);
  } else
#endif
  {
    const uint32_t *p = d->pc;
    d->stack->end_ofs = end;
    /* Check the previous bytecode, but guard against beginning. */
    if (p != method->code_base.ptr) p--;
    if (getop(*p) == OP_CHECKDELIM) {
      /* Rewind from OP_TAG* to OP_CHECKDELIM. */
      assert(getop(*d->pc) == OP_TAG1 ||
             getop(*d->pc) == OP_TAG2 ||
             getop(*d->pc) == OP_TAGN ||
             getop(*d->pc) == OP_DISPATCH);
      d->pc = p;
    }
    upb_pbdecoder_decode(closure, handler_data, &dummy, 0, NULL);
  }

  if (d->call_len != 0) {
    seterr(d, "Unexpected EOF inside submessage or group");
    return false;
  }

  return true;
}

size_t upb_pbdecoder_decode(void *decoder, const void *group, const char *buf,
                            size_t size, const upb_bufhandle *handle) {
  int32_t result = upb_pbdecoder_resume(decoder, NULL, buf, size, handle);

  if (result == DECODE_ENDGROUP) goto_endmsg(decoder);
  CHECK_RETURN(result);

  return run_decoder_vm(decoder, group, handle);
}


/* Public API *****************************************************************/

void upb_pbdecoder_reset(upb_pbdecoder *d) {
  d->top = d->stack;
  d->top->groupnum = 0;
  d->ptr = d->residual;
  d->buf = d->residual;
  d->end = d->residual;
  d->residual_end = d->residual;
}

upb_pbdecoder *upb_pbdecoder_create(upb_env *e, const upb_pbdecodermethod *m,
                                    upb_sink *sink) {
  const size_t default_max_nesting = 64;
#ifndef NDEBUG
  size_t size_before = upb_env_bytesallocated(e);
#endif

  upb_pbdecoder *d = upb_env_malloc(e, sizeof(upb_pbdecoder));
  if (!d) return NULL;

  d->method_ = m;
  d->callstack = upb_env_malloc(e, callstacksize(d, default_max_nesting));
  d->stack = upb_env_malloc(e, stacksize(d, default_max_nesting));
  if (!d->stack || !d->callstack) {
    return NULL;
  }

  d->env = e;
  d->limit = d->stack + default_max_nesting - 1;
  d->stack_size = default_max_nesting;

  upb_pbdecoder_reset(d);
  upb_bytessink_reset(&d->input_, &m->input_handler_, d);

  assert(sink);
  if (d->method_->dest_handlers_) {
    if (sink->handlers != d->method_->dest_handlers_)
      return NULL;
  }
  upb_sink_reset(&d->top->sink, sink->handlers, sink->closure);

  /* If this fails, increase the value in decoder.h. */
  assert(upb_env_bytesallocated(e) - size_before <= UPB_PB_DECODER_SIZE);
  return d;
}

uint64_t upb_pbdecoder_bytesparsed(const upb_pbdecoder *d) {
  return offset(d);
}

const upb_pbdecodermethod *upb_pbdecoder_method(const upb_pbdecoder *d) {
  return d->method_;
}

upb_bytessink *upb_pbdecoder_input(upb_pbdecoder *d) {
  return &d->input_;
}

size_t upb_pbdecoder_maxnesting(const upb_pbdecoder *d) {
  return d->stack_size;
}

bool upb_pbdecoder_setmaxnesting(upb_pbdecoder *d, size_t max) {
  assert(d->top >= d->stack);

  if (max < (size_t)(d->top - d->stack)) {
    /* Can't set a limit smaller than what we are currently at. */
    return false;
  }

  if (max > d->stack_size) {
    /* Need to reallocate stack and callstack to accommodate. */
    size_t old_size = stacksize(d, d->stack_size);
    size_t new_size = stacksize(d, max);
    void *p = upb_env_realloc(d->env, d->stack, old_size, new_size);
    if (!p) {
      return false;
    }
    d->stack = p;

    old_size = callstacksize(d, d->stack_size);
    new_size = callstacksize(d, max);
    p = upb_env_realloc(d->env, d->callstack, old_size, new_size);
    if (!p) {
      return false;
    }
    d->callstack = p;

    d->stack_size = max;
  }

  d->limit = d->stack + max - 1;
  return true;
}
/*
** upb::Encoder
**
** Since we are implementing pure handlers (ie. without any out-of-band access
** to pre-computed lengths), we have to buffer all submessages before we can
** emit even their first byte.
**
** Not knowing the size of submessages also means we can't write a perfect
** zero-copy implementation, even with buffering.  Lengths are stored as
** varints, which means that we don't know how many bytes to reserve for the
** length until we know what the length is.
**
** This leaves us with three main choices:
**
** 1. buffer all submessage data in a temporary buffer, then copy it exactly
**    once into the output buffer.
**
** 2. attempt to buffer data directly into the output buffer, estimating how
**    many bytes each length will take.  When our guesses are wrong, use
**    memmove() to grow or shrink the allotted space.
**
** 3. buffer directly into the output buffer, allocating a max length
**    ahead-of-time for each submessage length.  If we overallocated, we waste
**    space, but no memcpy() or memmove() is required.  This approach requires
**    defining a maximum size for submessages and rejecting submessages that
**    exceed that size.
**
** (2) and (3) have the potential to have better performance, but they are more
** complicated and subtle to implement:
**
**   (3) requires making an arbitrary choice of the maximum message size; it
**       wastes space when submessages are shorter than this and fails
**       completely when they are longer.  This makes it more finicky and
**       requires configuration based on the input.  It also makes it impossible
**       to perfectly match the output of reference encoders that always use the
**       optimal amount of space for each length.
**
**   (2) requires guessing the size upfront, and if multiple lengths are
**       guessed wrong the minimum required number of memmove() operations may
**       be complicated to compute correctly.  Implemented properly, it may have
**       a useful amortized or average cost, but more investigation is required
**       to determine this and what the optimal algorithm is to achieve it.
**
**   (1) makes you always pay for exactly one copy, but its implementation is
**       the simplest and its performance is predictable.
**
** So for now, we implement (1) only.  If we wish to optimize later, we should
** be able to do it without affecting users.
**
** The strategy is to buffer the segments of data that do *not* depend on
** unknown lengths in one buffer, and keep a separate buffer of segment pointers
** and lengths.  When the top-level submessage ends, we can go beginning to end,
** alternating the writing of lengths with memcpy() of the rest of the data.
** At the top level though, no buffering is required.
*/


#include <stdlib.h>

/* The output buffer is divided into segments; a segment is a string of data
 * that is "ready to go" -- it does not need any varint lengths inserted into
 * the middle.  The seams between segments are where varints will be inserted
 * once they are known.
 *
 * We also use the concept of a "run", which is a range of encoded bytes that
 * occur at a single submessage level.  Every segment contains one or more runs.
 *
 * A segment can span messages.  Consider:
 *
 *                  .--Submessage lengths---------.
 *                  |       |                     |
 *                  |       V                     V
 *                  V      | |---------------    | |-----------------
 * Submessages:    | |-----------------------------------------------
 * Top-level msg: ------------------------------------------------------------
 *
 * Segments:          -----   -------------------   -----------------
 * Runs:              *----   *--------------*---   *----------------
 * (* marks the start)
 *
 * Note that the top-level menssage is not in any segment because it does not
 * have any length preceding it.
 *
 * A segment is only interrupted when another length needs to be inserted.  So
 * observe how the second segment spans both the inner submessage and part of
 * the next enclosing message. */
typedef struct {
  uint32_t msglen;  /* The length to varint-encode before this segment. */
  uint32_t seglen;  /* Length of the segment. */
} upb_pb_encoder_segment;

struct upb_pb_encoder {
  upb_env *env;

  /* Our input and output. */
  upb_sink input_;
  upb_bytessink *output_;

  /* The "subclosure" -- used as the inner closure as part of the bytessink
   * protocol. */
  void *subc;

  /* The output buffer and limit, and our current write position.  "buf"
   * initially points to "initbuf", but is dynamically allocated if we need to
   * grow beyond the initial size. */
  char *buf, *ptr, *limit;

  /* The beginning of the current run, or undefined if we are at the top
   * level. */
  char *runbegin;

  /* The list of segments we are accumulating. */
  upb_pb_encoder_segment *segbuf, *segptr, *seglimit;

  /* The stack of enclosing submessages.  Each entry in the stack points to the
   * segment where this submessage's length is being accumulated. */
  int *stack, *top, *stacklimit;

  /* Depth of startmsg/endmsg calls. */
  int depth;
};

/* low-level buffering ********************************************************/

/* Low-level functions for interacting with the output buffer. */

/* TODO(haberman): handle pushback */
static void putbuf(upb_pb_encoder *e, const char *buf, size_t len) {
  size_t n = upb_bytessink_putbuf(e->output_, e->subc, buf, len, NULL);
  UPB_ASSERT_VAR(n, n == len);
}

static upb_pb_encoder_segment *top(upb_pb_encoder *e) {
  return &e->segbuf[*e->top];
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
static bool reserve(upb_pb_encoder *e, size_t bytes) {
  if ((size_t)(e->limit - e->ptr) < bytes) {
    /* Grow buffer. */
    char *new_buf;
    size_t needed = bytes + (e->ptr - e->buf);
    size_t old_size = e->limit - e->buf;

    size_t new_size = old_size;

    while (new_size < needed) {
      new_size *= 2;
    }

    new_buf = upb_env_realloc(e->env, e->buf, old_size, new_size);

    if (new_buf == NULL) {
      return false;
    }

    e->ptr = new_buf + (e->ptr - e->buf);
    e->runbegin = new_buf + (e->runbegin - e->buf);
    e->limit = new_buf + new_size;
    e->buf = new_buf;
  }

  return true;
}

/* Call when "bytes" bytes have been writte at e->ptr.  The caller *must* have
 * previously called reserve() with at least this many bytes. */
static void encoder_advance(upb_pb_encoder *e, size_t bytes) {
  assert((size_t)(e->limit - e->ptr) >= bytes);
  e->ptr += bytes;
}

/* Call when all of the bytes for a handler have been written.  Flushes the
 * bytes if possible and necessary, returning false if this failed. */
static bool commit(upb_pb_encoder *e) {
  if (!e->top) {
    /* We aren't inside a delimited region.  Flush our accumulated bytes to
     * the output.
     *
     * TODO(haberman): in the future we may want to delay flushing for
     * efficiency reasons. */
    putbuf(e, e->buf, e->ptr - e->buf);
    e->ptr = e->buf;
  }

  return true;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static bool encode_bytes(upb_pb_encoder *e, const void *data, size_t len) {
  if (!reserve(e, len)) {
    return false;
  }

  memcpy(e->ptr, data, len);
  encoder_advance(e, len);
  return true;
}

/* Finish the current run by adding the run totals to the segment and message
 * length. */
static void accumulate(upb_pb_encoder *e) {
  size_t run_len;
  assert(e->ptr >= e->runbegin);
  run_len = e->ptr - e->runbegin;
  e->segptr->seglen += run_len;
  top(e)->msglen += run_len;
  e->runbegin = e->ptr;
}

/* Call to indicate the start of delimited region for which the full length is
 * not yet known.  All data will be buffered until the length is known.
 * Delimited regions may be nested; their lengths will all be tracked properly. */
static bool start_delim(upb_pb_encoder *e) {
  if (e->top) {
    /* We are already buffering, advance to the next segment and push it on the
     * stack. */
    accumulate(e);

    if (++e->top == e->stacklimit) {
      /* TODO(haberman): grow stack? */
      return false;
    }

    if (++e->segptr == e->seglimit) {
      /* Grow segment buffer. */
      size_t old_size =
          (e->seglimit - e->segbuf) * sizeof(upb_pb_encoder_segment);
      size_t new_size = old_size * 2;
      upb_pb_encoder_segment *new_buf =
          upb_env_realloc(e->env, e->segbuf, old_size, new_size);

      if (new_buf == NULL) {
        return false;
      }

      e->segptr = new_buf + (e->segptr - e->segbuf);
      e->seglimit = new_buf + (new_size / sizeof(upb_pb_encoder_segment));
      e->segbuf = new_buf;
    }
  } else {
    /* We were previously at the top level, start buffering. */
    e->segptr = e->segbuf;
    e->top = e->stack;
    e->runbegin = e->ptr;
  }

  *e->top = e->segptr - e->segbuf;
  e->segptr->seglen = 0;
  e->segptr->msglen = 0;

  return true;
}

/* Call to indicate the end of a delimited region.  We now know the length of
 * the delimited region.  If we are not nested inside any other delimited
 * regions, we can now emit all of the buffered data we accumulated. */
static bool end_delim(upb_pb_encoder *e) {
  size_t msglen;
  accumulate(e);
  msglen = top(e)->msglen;

  if (e->top == e->stack) {
    /* All lengths are now available, emit all buffered data. */
    char buf[UPB_PB_VARINT_MAX_LEN];
    upb_pb_encoder_segment *s;
    const char *ptr = e->buf;
    for (s = e->segbuf; s <= e->segptr; s++) {
      size_t lenbytes = upb_vencode64(s->msglen, buf);
      putbuf(e, buf, lenbytes);
      putbuf(e, ptr, s->seglen);
      ptr += s->seglen;
    }

    e->ptr = e->buf;
    e->top = NULL;
  } else {
    /* Need to keep buffering; propagate length info into enclosing
     * submessages. */
    --e->top;
    top(e)->msglen += msglen + upb_varint_size(msglen);
  }

  return true;
}


/* tag_t **********************************************************************/

/* A precomputed (pre-encoded) tag and length. */

typedef struct {
  uint8_t bytes;
  char tag[7];
} tag_t;

/* Allocates a new tag for this field, and sets it in these handlerattr. */
static void new_tag(upb_handlers *h, const upb_fielddef *f, upb_wiretype_t wt,
                    upb_handlerattr *attr) {
  uint32_t n = upb_fielddef_number(f);

  tag_t *tag = malloc(sizeof(tag_t));
  tag->bytes = upb_vencode64((n << 3) | wt, tag->tag);

  upb_handlerattr_init(attr);
  upb_handlerattr_sethandlerdata(attr, tag);
  upb_handlers_addcleanup(h, tag, free);
}

static bool encode_tag(upb_pb_encoder *e, const tag_t *tag) {
  return encode_bytes(e, tag->tag, tag->bytes);
}


/* encoding of wire types *****************************************************/

static bool encode_fixed64(upb_pb_encoder *e, uint64_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return encode_bytes(e, &val, sizeof(uint64_t));
}

static bool encode_fixed32(upb_pb_encoder *e, uint32_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return encode_bytes(e, &val, sizeof(uint32_t));
}

static bool encode_varint(upb_pb_encoder *e, uint64_t val) {
  if (!reserve(e, UPB_PB_VARINT_MAX_LEN)) {
    return false;
  }

  encoder_advance(e, upb_vencode64(val, e->ptr));
  return true;
}

static uint64_t dbl2uint64(double d) {
  uint64_t ret;
  memcpy(&ret, &d, sizeof(uint64_t));
  return ret;
}

static uint32_t flt2uint32(float d) {
  uint32_t ret;
  memcpy(&ret, &d, sizeof(uint32_t));
  return ret;
}


/* encoding of proto types ****************************************************/

static bool startmsg(void *c, const void *hd) {
  upb_pb_encoder *e = c;
  UPB_UNUSED(hd);
  if (e->depth++ == 0) {
    upb_bytessink_start(e->output_, 0, &e->subc);
  }
  return true;
}

static bool endmsg(void *c, const void *hd, upb_status *status) {
  upb_pb_encoder *e = c;
  UPB_UNUSED(hd);
  UPB_UNUSED(status);
  if (--e->depth == 0) {
    upb_bytessink_end(e->output_);
  }
  return true;
}

static void *encode_startdelimfield(void *c, const void *hd) {
  bool ok = encode_tag(c, hd) && commit(c) && start_delim(c);
  return ok ? c : UPB_BREAK;
}

static bool encode_enddelimfield(void *c, const void *hd) {
  UPB_UNUSED(hd);
  return end_delim(c);
}

static void *encode_startgroup(void *c, const void *hd) {
  return (encode_tag(c, hd) && commit(c)) ? c : UPB_BREAK;
}

static bool encode_endgroup(void *c, const void *hd) {
  return encode_tag(c, hd) && commit(c);
}

static void *encode_startstr(void *c, const void *hd, size_t size_hint) {
  UPB_UNUSED(size_hint);
  return encode_startdelimfield(c, hd);
}

static size_t encode_strbuf(void *c, const void *hd, const char *buf,
                            size_t len, const upb_bufhandle *h) {
  UPB_UNUSED(hd);
  UPB_UNUSED(h);
  return encode_bytes(c, buf, len) ? len : 0;
}

#define T(type, ctype, convert, encode)                                  \
  static bool encode_scalar_##type(void *e, const void *hd, ctype val) { \
    return encode_tag(e, hd) && encode(e, (convert)(val)) && commit(e);  \
  }                                                                      \
  static bool encode_packed_##type(void *e, const void *hd, ctype val) { \
    UPB_UNUSED(hd);                                                      \
    return encode(e, (convert)(val));                                    \
  }

T(double,   double,   dbl2uint64,   encode_fixed64)
T(float,    float,    flt2uint32,   encode_fixed32)
T(int64,    int64_t,  uint64_t,     encode_varint)
T(int32,    int32_t,  uint32_t,     encode_varint)
T(fixed64,  uint64_t, uint64_t,     encode_fixed64)
T(fixed32,  uint32_t, uint32_t,     encode_fixed32)
T(bool,     bool,     bool,         encode_varint)
T(uint32,   uint32_t, uint32_t,     encode_varint)
T(uint64,   uint64_t, uint64_t,     encode_varint)
T(enum,     int32_t,  uint32_t,     encode_varint)
T(sfixed32, int32_t,  uint32_t,     encode_fixed32)
T(sfixed64, int64_t,  uint64_t,     encode_fixed64)
T(sint32,   int32_t,  upb_zzenc_32, encode_varint)
T(sint64,   int64_t,  upb_zzenc_64, encode_varint)

#undef T


/* code to build the handlers *************************************************/

static void newhandlers_callback(const void *closure, upb_handlers *h) {
  const upb_msgdef *m;
  upb_msg_field_iter i;

  UPB_UNUSED(closure);

  upb_handlers_setstartmsg(h, startmsg, NULL);
  upb_handlers_setendmsg(h, endmsg, NULL);

  m = upb_handlers_msgdef(h);
  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    bool packed = upb_fielddef_isseq(f) && upb_fielddef_isprimitive(f) &&
                  upb_fielddef_packed(f);
    upb_handlerattr attr;
    upb_wiretype_t wt =
        packed ? UPB_WIRE_TYPE_DELIMITED
               : upb_pb_native_wire_types[upb_fielddef_descriptortype(f)];

    /* Pre-encode the tag for this field. */
    new_tag(h, f, wt, &attr);

    if (packed) {
      upb_handlers_setstartseq(h, f, encode_startdelimfield, &attr);
      upb_handlers_setendseq(h, f, encode_enddelimfield, &attr);
    }

#define T(upper, lower, upbtype)                                     \
  case UPB_DESCRIPTOR_TYPE_##upper:                                  \
    if (packed) {                                                    \
      upb_handlers_set##upbtype(h, f, encode_packed_##lower, &attr); \
    } else {                                                         \
      upb_handlers_set##upbtype(h, f, encode_scalar_##lower, &attr); \
    }                                                                \
    break;

    switch (upb_fielddef_descriptortype(f)) {
      T(DOUBLE,   double,   double);
      T(FLOAT,    float,    float);
      T(INT64,    int64,    int64);
      T(INT32,    int32,    int32);
      T(FIXED64,  fixed64,  uint64);
      T(FIXED32,  fixed32,  uint32);
      T(BOOL,     bool,     bool);
      T(UINT32,   uint32,   uint32);
      T(UINT64,   uint64,   uint64);
      T(ENUM,     enum,     int32);
      T(SFIXED32, sfixed32, int32);
      T(SFIXED64, sfixed64, int64);
      T(SINT32,   sint32,   int32);
      T(SINT64,   sint64,   int64);
      case UPB_DESCRIPTOR_TYPE_STRING:
      case UPB_DESCRIPTOR_TYPE_BYTES:
        upb_handlers_setstartstr(h, f, encode_startstr, &attr);
        upb_handlers_setendstr(h, f, encode_enddelimfield, &attr);
        upb_handlers_setstring(h, f, encode_strbuf, &attr);
        break;
      case UPB_DESCRIPTOR_TYPE_MESSAGE:
        upb_handlers_setstartsubmsg(h, f, encode_startdelimfield, &attr);
        upb_handlers_setendsubmsg(h, f, encode_enddelimfield, &attr);
        break;
      case UPB_DESCRIPTOR_TYPE_GROUP: {
        /* Endgroup takes a different tag (wire_type = END_GROUP). */
        upb_handlerattr attr2;
        new_tag(h, f, UPB_WIRE_TYPE_END_GROUP, &attr2);

        upb_handlers_setstartsubmsg(h, f, encode_startgroup, &attr);
        upb_handlers_setendsubmsg(h, f, encode_endgroup, &attr2);

        upb_handlerattr_uninit(&attr2);
        break;
      }
    }

#undef T

    upb_handlerattr_uninit(&attr);
  }
}

void upb_pb_encoder_reset(upb_pb_encoder *e) {
  e->segptr = NULL;
  e->top = NULL;
  e->depth = 0;
}


/* public API *****************************************************************/

const upb_handlers *upb_pb_encoder_newhandlers(const upb_msgdef *m,
                                               const void *owner) {
  return upb_handlers_newfrozen(m, owner, newhandlers_callback, NULL);
}

upb_pb_encoder *upb_pb_encoder_create(upb_env *env, const upb_handlers *h,
                                      upb_bytessink *output) {
  const size_t initial_bufsize = 256;
  const size_t initial_segbufsize = 16;
  /* TODO(haberman): make this configurable. */
  const size_t stack_size = 64;
#ifndef NDEBUG
  const size_t size_before = upb_env_bytesallocated(env);
#endif

  upb_pb_encoder *e = upb_env_malloc(env, sizeof(upb_pb_encoder));
  if (!e) return NULL;

  e->buf = upb_env_malloc(env, initial_bufsize);
  e->segbuf = upb_env_malloc(env, initial_segbufsize * sizeof(*e->segbuf));
  e->stack = upb_env_malloc(env, stack_size * sizeof(*e->stack));

  if (!e->buf || !e->segbuf || !e->stack) {
    return NULL;
  }

  e->limit = e->buf + initial_bufsize;
  e->seglimit = e->segbuf + initial_segbufsize;
  e->stacklimit = e->stack + stack_size;

  upb_pb_encoder_reset(e);
  upb_sink_reset(&e->input_, h, e);

  e->env = env;
  e->output_ = output;
  e->subc = output->closure;
  e->ptr = e->buf;

  /* If this fails, increase the value in encoder.h. */
  assert(upb_env_bytesallocated(env) - size_before <= UPB_PB_ENCODER_SIZE);
  return e;
}

upb_sink *upb_pb_encoder_input(upb_pb_encoder *e) { return &e->input_; }


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

upb_def **upb_load_defs_from_descriptor(const char *str, size_t len, int *n,
                                        void *owner, upb_status *status) {
  /* Create handlers. */
  const upb_pbdecodermethod *decoder_m;
  const upb_handlers *reader_h = upb_descreader_newhandlers(&reader_h);
  upb_env env;
  upb_pbdecodermethodopts opts;
  upb_pbdecoder *decoder;
  upb_descreader *reader;
  bool ok;
  upb_def **ret = NULL;
  upb_def **defs;

  upb_pbdecodermethodopts_init(&opts, reader_h);
  decoder_m = upb_pbdecodermethod_new(&opts, &decoder_m);

  upb_env_init(&env);
  upb_env_reporterrorsto(&env, status);

  reader = upb_descreader_create(&env, reader_h);
  decoder = upb_pbdecoder_create(&env, decoder_m, upb_descreader_input(reader));

  /* Push input data. */
  ok = upb_bufsrc_putbuf(str, len, upb_pbdecoder_input(decoder));

  if (!ok) goto cleanup;
  defs = upb_descreader_getdefs(reader, owner, n);
  ret = malloc(sizeof(upb_def*) * (*n));
  memcpy(ret, defs, sizeof(upb_def*) * (*n));

cleanup:
  upb_env_uninit(&env);
  upb_handlers_unref(reader_h, &reader_h);
  upb_pbdecodermethod_unref(decoder_m, &decoder_m);
  return ret;
}

bool upb_load_descriptor_into_symtab(upb_symtab *s, const char *str, size_t len,
                                     upb_status *status) {
  int n;
  bool success;
  upb_def **defs = upb_load_defs_from_descriptor(str, len, &n, &defs, status);
  if (!defs) return false;
  success = upb_symtab_add(s, defs, n, &defs, status);
  free(defs);
  return success;
}

char *upb_readfile(const char *filename, size_t *len) {
  long size;
  char *buf;
  FILE *f = fopen(filename, "rb");
  if(!f) return NULL;
  if(fseek(f, 0, SEEK_END) != 0) goto error;
  size = ftell(f);
  if(size < 0) goto error;
  if(fseek(f, 0, SEEK_SET) != 0) goto error;
  buf = malloc(size + 1);
  if(size && fread(buf, size, 1, f) != 1) goto error;
  fclose(f);
  if (len) *len = size;
  return buf;

error:
  fclose(f);
  return NULL;
}

bool upb_load_descriptor_file_into_symtab(upb_symtab *symtab, const char *fname,
                                          upb_status *status) {
  size_t len;
  bool success;
  char *data = upb_readfile(fname, &len);
  if (!data) {
    if (status) upb_status_seterrf(status, "Couldn't read file: %s", fname);
    return false;
  }
  success = upb_load_descriptor_into_symtab(symtab, data, len, status);
  free(data);
  return success;
}
/*
 * upb::pb::TextPrinter
 *
 * OPT: This is not optimized at all.  It uses printf() which parses the format
 * string every time, and it allocates memory for every put.
 */


#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct upb_textprinter {
  upb_sink input_;
  upb_bytessink *output_;
  int indent_depth_;
  bool single_line_;
  void *subc;
};

#define CHECK(x) if ((x) < 0) goto err;

static const char *shortname(const char *longname) {
  const char *last = strrchr(longname, '.');
  return last ? last + 1 : longname;
}

static int indent(upb_textprinter *p) {
  int i;
  if (!p->single_line_)
    for (i = 0; i < p->indent_depth_; i++)
      upb_bytessink_putbuf(p->output_, p->subc, "  ", 2, NULL);
  return 0;
}

static int endfield(upb_textprinter *p) {
  const char ch = (p->single_line_ ? ' ' : '\n');
  upb_bytessink_putbuf(p->output_, p->subc, &ch, 1, NULL);
  return 0;
}

static int putescaped(upb_textprinter *p, const char *buf, size_t len,
                      bool preserve_utf8) {
  /* Based on CEscapeInternal() from Google's protobuf release. */
  char dstbuf[4096], *dst = dstbuf, *dstend = dstbuf + sizeof(dstbuf);
  const char *end = buf + len;

  /* I think hex is prettier and more useful, but proto2 uses octal; should
   * investigate whether it can parse hex also. */
  const bool use_hex = false;
  bool last_hex_escape = false; /* true if last output char was \xNN */

  for (; buf < end; buf++) {
    bool is_hex_escape;

    if (dstend - dst < 4) {
      upb_bytessink_putbuf(p->output_, p->subc, dstbuf, dst - dstbuf, NULL);
      dst = dstbuf;
    }

    is_hex_escape = false;
    switch (*buf) {
      case '\n': *(dst++) = '\\'; *(dst++) = 'n';  break;
      case '\r': *(dst++) = '\\'; *(dst++) = 'r';  break;
      case '\t': *(dst++) = '\\'; *(dst++) = 't';  break;
      case '\"': *(dst++) = '\\'; *(dst++) = '\"'; break;
      case '\'': *(dst++) = '\\'; *(dst++) = '\''; break;
      case '\\': *(dst++) = '\\'; *(dst++) = '\\'; break;
      default:
        /* Note that if we emit \xNN and the buf character after that is a hex
         * digit then that digit must be escaped too to prevent it being
         * interpreted as part of the character code by C. */
        if ((!preserve_utf8 || (uint8_t)*buf < 0x80) &&
            (!isprint(*buf) || (last_hex_escape && isxdigit(*buf)))) {
          sprintf(dst, (use_hex ? "\\x%02x" : "\\%03o"), (uint8_t)*buf);
          is_hex_escape = use_hex;
          dst += 4;
        } else {
          *(dst++) = *buf; break;
        }
    }
    last_hex_escape = is_hex_escape;
  }
  /* Flush remaining data. */
  upb_bytessink_putbuf(p->output_, p->subc, dstbuf, dst - dstbuf, NULL);
  return 0;
}

bool putf(upb_textprinter *p, const char *fmt, ...) {
  va_list args;
  va_list args_copy;
  char *str;
  int written;
  int len;
  bool ok;

  va_start(args, fmt);

  /* Run once to get the length of the string. */
  _upb_va_copy(args_copy, args);
  len = _upb_vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  /* + 1 for NULL terminator (vsprintf() requires it even if we don't). */
  str = malloc(len + 1);
  if (!str) return false;
  written = vsprintf(str, fmt, args);
  va_end(args);
  UPB_ASSERT_VAR(written, written == len);

  ok = upb_bytessink_putbuf(p->output_, p->subc, str, len, NULL);
  free(str);
  return ok;
}


/* handlers *******************************************************************/

static bool textprinter_startmsg(void *c, const void *hd) {
  upb_textprinter *p = c;
  UPB_UNUSED(hd);
  if (p->indent_depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc);
  }
  return true;
}

static bool textprinter_endmsg(void *c, const void *hd, upb_status *s) {
  upb_textprinter *p = c;
  UPB_UNUSED(hd);
  UPB_UNUSED(s);
  if (p->indent_depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

#define TYPE(name, ctype, fmt) \
  static bool textprinter_put ## name(void *closure, const void *handler_data, \
                                      ctype val) {                             \
    upb_textprinter *p = closure;                                              \
    const upb_fielddef *f = handler_data;                                      \
    CHECK(indent(p));                                                          \
    putf(p, "%s: " fmt, upb_fielddef_name(f), val);                            \
    CHECK(endfield(p));                                                        \
    return true;                                                               \
  err:                                                                         \
    return false;                                                              \
}

static bool textprinter_putbool(void *closure, const void *handler_data,
                                bool val) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  CHECK(indent(p));
  putf(p, "%s: %s", upb_fielddef_name(f), val ? "true" : "false");
  CHECK(endfield(p));
  return true;
err:
  return false;
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY_MACROVAL(x) STRINGIFY_HELPER(x)

TYPE(int32,  int32_t,  "%" PRId32)
TYPE(int64,  int64_t,  "%" PRId64)
TYPE(uint32, uint32_t, "%" PRIu32)
TYPE(uint64, uint64_t, "%" PRIu64)
TYPE(float,  float,    "%." STRINGIFY_MACROVAL(FLT_DIG) "g")
TYPE(double, double,   "%." STRINGIFY_MACROVAL(DBL_DIG) "g")

#undef TYPE

/* Output a symbolic value from the enum if found, else just print as int32. */
static bool textprinter_putenum(void *closure, const void *handler_data,
                                int32_t val) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  const upb_enumdef *enum_def = upb_downcast_enumdef(upb_fielddef_subdef(f));
  const char *label = upb_enumdef_iton(enum_def, val);
  if (label) {
    indent(p);
    putf(p, "%s: %s", upb_fielddef_name(f), label);
    endfield(p);
  } else {
    if (!textprinter_putint32(closure, handler_data, val))
      return false;
  }
  return true;
}

static void *textprinter_startstr(void *closure, const void *handler_data,
                      size_t size_hint) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  UPB_UNUSED(size_hint);
  indent(p);
  putf(p, "%s: \"", upb_fielddef_name(f));
  return p;
}

static bool textprinter_endstr(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  UPB_UNUSED(handler_data);
  putf(p, "\"");
  endfield(p);
  return true;
}

static size_t textprinter_putstr(void *closure, const void *hd, const char *buf,
                                 size_t len, const upb_bufhandle *handle) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = hd;
  UPB_UNUSED(handle);
  CHECK(putescaped(p, buf, len, upb_fielddef_type(f) == UPB_TYPE_STRING));
  return len;
err:
  return 0;
}

static void *textprinter_startsubmsg(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  const char *name = handler_data;
  CHECK(indent(p));
  putf(p, "%s {%c", name, p->single_line_ ? ' ' : '\n');
  p->indent_depth_++;
  return p;
err:
  return UPB_BREAK;
}

static bool textprinter_endsubmsg(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  UPB_UNUSED(handler_data);
  p->indent_depth_--;
  CHECK(indent(p));
  upb_bytessink_putbuf(p->output_, p->subc, "}", 1, NULL);
  CHECK(endfield(p));
  return true;
err:
  return false;
}

static void onmreg(const void *c, upb_handlers *h) {
  const upb_msgdef *m = upb_handlers_msgdef(h);
  upb_msg_field_iter i;
  UPB_UNUSED(c);

  upb_handlers_setstartmsg(h, textprinter_startmsg, NULL);
  upb_handlers_setendmsg(h, textprinter_endmsg, NULL);

  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
    upb_handlerattr_sethandlerdata(&attr, f);
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_INT32:
        upb_handlers_setint32(h, f, textprinter_putint32, &attr);
        break;
      case UPB_TYPE_INT64:
        upb_handlers_setint64(h, f, textprinter_putint64, &attr);
        break;
      case UPB_TYPE_UINT32:
        upb_handlers_setuint32(h, f, textprinter_putuint32, &attr);
        break;
      case UPB_TYPE_UINT64:
        upb_handlers_setuint64(h, f, textprinter_putuint64, &attr);
        break;
      case UPB_TYPE_FLOAT:
        upb_handlers_setfloat(h, f, textprinter_putfloat, &attr);
        break;
      case UPB_TYPE_DOUBLE:
        upb_handlers_setdouble(h, f, textprinter_putdouble, &attr);
        break;
      case UPB_TYPE_BOOL:
        upb_handlers_setbool(h, f, textprinter_putbool, &attr);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        upb_handlers_setstartstr(h, f, textprinter_startstr, &attr);
        upb_handlers_setstring(h, f, textprinter_putstr, &attr);
        upb_handlers_setendstr(h, f, textprinter_endstr, &attr);
        break;
      case UPB_TYPE_MESSAGE: {
        const char *name =
            upb_fielddef_istagdelim(f)
                ? shortname(upb_msgdef_fullname(upb_fielddef_msgsubdef(f)))
                : upb_fielddef_name(f);
        upb_handlerattr_sethandlerdata(&attr, name);
        upb_handlers_setstartsubmsg(h, f, textprinter_startsubmsg, &attr);
        upb_handlers_setendsubmsg(h, f, textprinter_endsubmsg, &attr);
        break;
      }
      case UPB_TYPE_ENUM:
        upb_handlers_setint32(h, f, textprinter_putenum, &attr);
        break;
    }
  }
}

static void textprinter_reset(upb_textprinter *p, bool single_line) {
  p->single_line_ = single_line;
  p->indent_depth_ = 0;
}


/* Public API *****************************************************************/

upb_textprinter *upb_textprinter_create(upb_env *env, const upb_handlers *h,
                                        upb_bytessink *output) {
  upb_textprinter *p = upb_env_malloc(env, sizeof(upb_textprinter));
  if (!p) return NULL;

  p->output_ = output;
  upb_sink_reset(&p->input_, h, p);
  textprinter_reset(p, false);

  return p;
}

const upb_handlers *upb_textprinter_newhandlers(const upb_msgdef *m,
                                                const void *owner) {
  return upb_handlers_newfrozen(m, owner, &onmreg, NULL);
}

upb_sink *upb_textprinter_input(upb_textprinter *p) { return &p->input_; }

void upb_textprinter_setsingleline(upb_textprinter *p, bool single_line) {
  p->single_line_ = single_line;
}


/* Index is descriptor type. */
const uint8_t upb_pb_native_wire_types[] = {
  UPB_WIRE_TYPE_END_GROUP,     /* ENDGROUP */
  UPB_WIRE_TYPE_64BIT,         /* DOUBLE */
  UPB_WIRE_TYPE_32BIT,         /* FLOAT */
  UPB_WIRE_TYPE_VARINT,        /* INT64 */
  UPB_WIRE_TYPE_VARINT,        /* UINT64 */
  UPB_WIRE_TYPE_VARINT,        /* INT32 */
  UPB_WIRE_TYPE_64BIT,         /* FIXED64 */
  UPB_WIRE_TYPE_32BIT,         /* FIXED32 */
  UPB_WIRE_TYPE_VARINT,        /* BOOL */
  UPB_WIRE_TYPE_DELIMITED,     /* STRING */
  UPB_WIRE_TYPE_START_GROUP,   /* GROUP */
  UPB_WIRE_TYPE_DELIMITED,     /* MESSAGE */
  UPB_WIRE_TYPE_DELIMITED,     /* BYTES */
  UPB_WIRE_TYPE_VARINT,        /* UINT32 */
  UPB_WIRE_TYPE_VARINT,        /* ENUM */
  UPB_WIRE_TYPE_32BIT,         /* SFIXED32 */
  UPB_WIRE_TYPE_64BIT,         /* SFIXED64 */
  UPB_WIRE_TYPE_VARINT,        /* SINT32 */
  UPB_WIRE_TYPE_VARINT,        /* SINT64 */
};

/* A basic branch-based decoder, uses 32-bit values to get good performance
 * on 32-bit architectures (but performs well on 64-bits also).
 * This scheme comes from the original Google Protobuf implementation
 * (proto2). */
upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r) {
  upb_decoderet err = {NULL, 0};
  const char *p = r.p;
  uint32_t low = (uint32_t)r.val;
  uint32_t high = 0;
  uint32_t b;
  b = *(p++); low  |= (b & 0x7fU) << 14; if (!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7fU) << 21; if (!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7fU) << 28;
              high  = (b & 0x7fU) >>  4; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) <<  3; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 10; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 17; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 24; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 31; if (!(b & 0x80)) goto done;
  return err;

done:
  r.val = ((uint64_t)high << 32) | low;
  r.p = p;
  return r;
}

/* Like the previous, but uses 64-bit values. */
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r) {
  const char *p = r.p;
  uint64_t val = r.val;
  uint64_t b;
  upb_decoderet err = {NULL, 0};
  b = *(p++); val |= (b & 0x7fU) << 14; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 21; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 28; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 35; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 42; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 49; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 56; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 63; if (!(b & 0x80)) goto done;
  return err;

done:
  r.val = val;
  r.p = p;
  return r;
}

/* Given an encoded varint v, returns an integer with a single bit set that
 * indicates the end of the varint.  Subtracting one from this value will
 * yield a mask that leaves only bits that are part of the varint.  Returns
 * 0 if the varint is unterminated. */
static uint64_t upb_get_vstopbit(uint64_t v) {
  uint64_t cbits = v | 0x7f7f7f7f7f7f7f7fULL;
  return ~cbits & (cbits+1);
}

/* A branchless decoder.  Credit to Pascal Massimino for the bit-twiddling. */
upb_decoderet upb_vdecode_max8_massimino(upb_decoderet r) {
  uint64_t b;
  uint64_t stop_bit;
  upb_decoderet my_r;
  memcpy(&b, r.p, sizeof(b));
  stop_bit = upb_get_vstopbit(b);
  b =  (b & 0x7f7f7f7f7f7f7f7fULL) & (stop_bit - 1);
  b +=       b & 0x007f007f007f007fULL;
  b +=  3 * (b & 0x0000ffff0000ffffULL);
  b += 15 * (b & 0x00000000ffffffffULL);
  if (stop_bit == 0) {
    /* Error: unterminated varint. */
    upb_decoderet err_r = {(void*)0, 0};
    return err_r;
  }
  my_r = upb_decoderet_make(r.p + ((__builtin_ctzll(stop_bit) + 1) / 8),
                            r.val | (b << 7));
  return my_r;
}

/* A branchless decoder.  Credit to Daniel Wright for the bit-twiddling. */
upb_decoderet upb_vdecode_max8_wright(upb_decoderet r) {
  uint64_t b;
  uint64_t stop_bit;
  upb_decoderet my_r;
  memcpy(&b, r.p, sizeof(b));
  stop_bit = upb_get_vstopbit(b);
  b &= (stop_bit - 1);
  b = ((b & 0x7f007f007f007f00ULL) >> 1) | (b & 0x007f007f007f007fULL);
  b = ((b & 0xffff0000ffff0000ULL) >> 2) | (b & 0x0000ffff0000ffffULL);
  b = ((b & 0xffffffff00000000ULL) >> 4) | (b & 0x00000000ffffffffULL);
  if (stop_bit == 0) {
    /* Error: unterminated varint. */
    upb_decoderet err_r = {(void*)0, 0};
    return err_r;
  }
  my_r = upb_decoderet_make(r.p + ((__builtin_ctzll(stop_bit) + 1) / 8),
                            r.val | (b << 14));
  return my_r;
}

#line 1 "upb/json/parser.rl"
/*
** upb::json::Parser (upb_json_parser)
**
** A parser that uses the Ragel State Machine Compiler to generate
** the finite automata.
**
** Ragel only natively handles regular languages, but we can manually
** program it a bit to handle context-free languages like JSON, by using
** the "fcall" and "fret" constructs.
**
** This parser can handle the basics, but needs several things to be fleshed
** out:
**
** - handling of unicode escape sequences (including high surrogate pairs).
** - properly check and report errors for unknown fields, stack overflow,
**   improper array nesting (or lack of nesting).
** - handling of base64 sequences with padding characters.
** - handling of push-back (non-success returns from sink functions).
** - handling of keys/escape-sequences/etc that span input buffers.
*/

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


#define UPB_JSON_MAX_DEPTH 64

typedef struct {
  upb_sink sink;

  /* The current message in which we're parsing, and the field whose value we're
   * expecting next. */
  const upb_msgdef *m;
  const upb_fielddef *f;

  /* We are in a repeated-field context, ready to emit mapentries as
   * submessages. This flag alters the start-of-object (open-brace) behavior to
   * begin a sequence of mapentry messages rather than a single submessage. */
  bool is_map;

  /* We are in a map-entry message context. This flag is set when parsing the
   * value field of a single map entry and indicates to all value-field parsers
   * (subobjects, strings, numbers, and bools) that the map-entry submessage
   * should end as soon as the value is parsed. */
  bool is_mapentry;

  /* If |is_map| or |is_mapentry| is true, |mapfield| refers to the parent
   * message's map field that we're currently parsing. This differs from |f|
   * because |f| is the field in the *current* message (i.e., the map-entry
   * message itself), not the parent's field that leads to this map. */
  const upb_fielddef *mapfield;
} upb_jsonparser_frame;

struct upb_json_parser {
  upb_env *env;
  upb_byteshandler input_handler_;
  upb_bytessink input_;

  /* Stack to track the JSON scopes we are in. */
  upb_jsonparser_frame stack[UPB_JSON_MAX_DEPTH];
  upb_jsonparser_frame *top;
  upb_jsonparser_frame *limit;

  upb_status status;

  /* Ragel's internal parsing stack for the parsing state machine. */
  int current_state;
  int parser_stack[UPB_JSON_MAX_DEPTH];
  int parser_top;

  /* The handle for the current buffer. */
  const upb_bufhandle *handle;

  /* Accumulate buffer.  See details in parser.rl. */
  const char *accumulated;
  size_t accumulated_len;
  char *accumulate_buf;
  size_t accumulate_buf_size;

  /* Multi-part text data.  See details in parser.rl. */
  int multipart_state;
  upb_selector_t string_selector;

  /* Input capture.  See details in parser.rl. */
  const char *capture;

  /* Intermediate result of parsing a unicode escape sequence. */
  uint32_t digit;
};

#define PARSER_CHECK_RETURN(x) if (!(x)) return false

/* Used to signal that a capture has been suspended. */
static char suspend_capture;

static upb_selector_t getsel_for_handlertype(upb_json_parser *p,
                                             upb_handlertype_t type) {
  upb_selector_t sel;
  bool ok = upb_handlers_getselector(p->top->f, type, &sel);
  UPB_ASSERT_VAR(ok, ok);
  return sel;
}

static upb_selector_t parser_getsel(upb_json_parser *p) {
  return getsel_for_handlertype(
      p, upb_handlers_getprimitivehandlertype(p->top->f));
}

static bool check_stack(upb_json_parser *p) {
  if ((p->top + 1) == p->limit) {
    upb_status_seterrmsg(&p->status, "Nesting too deep");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  return true;
}

/* There are GCC/Clang built-ins for overflow checking which we could start
 * using if there was any performance benefit to it. */

static bool checked_add(size_t a, size_t b, size_t *c) {
  if (SIZE_MAX - a < b) return false;
  *c = a + b;
  return true;
}

static size_t saturating_multiply(size_t a, size_t b) {
  /* size_t is unsigned, so this is defined behavior even on overflow. */
  size_t ret = a * b;
  if (b != 0 && ret / b != a) {
    ret = SIZE_MAX;
  }
  return ret;
}


/* Base64 decoding ************************************************************/

/* TODO(haberman): make this streaming. */

static const signed char b64table[] = {
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      62/*+*/, -1,      -1,      -1,      63/*/ */,
  52/*0*/, 53/*1*/, 54/*2*/, 55/*3*/, 56/*4*/, 57/*5*/, 58/*6*/, 59/*7*/,
  60/*8*/, 61/*9*/, -1,      -1,      -1,      -1,      -1,      -1,
  -1,       0/*A*/,  1/*B*/,  2/*C*/,  3/*D*/,  4/*E*/,  5/*F*/,  6/*G*/,
  07/*H*/,  8/*I*/,  9/*J*/, 10/*K*/, 11/*L*/, 12/*M*/, 13/*N*/, 14/*O*/,
  15/*P*/, 16/*Q*/, 17/*R*/, 18/*S*/, 19/*T*/, 20/*U*/, 21/*V*/, 22/*W*/,
  23/*X*/, 24/*Y*/, 25/*Z*/, -1,      -1,      -1,      -1,      -1,
  -1,      26/*a*/, 27/*b*/, 28/*c*/, 29/*d*/, 30/*e*/, 31/*f*/, 32/*g*/,
  33/*h*/, 34/*i*/, 35/*j*/, 36/*k*/, 37/*l*/, 38/*m*/, 39/*n*/, 40/*o*/,
  41/*p*/, 42/*q*/, 43/*r*/, 44/*s*/, 45/*t*/, 46/*u*/, 47/*v*/, 48/*w*/,
  49/*x*/, 50/*y*/, 51/*z*/, -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1
};

/* Returns the table value sign-extended to 32 bits.  Knowing that the upper
 * bits will be 1 for unrecognized characters makes it easier to check for
 * this error condition later (see below). */
int32_t b64lookup(unsigned char ch) { return b64table[ch]; }

/* Returns true if the given character is not a valid base64 character or
 * padding. */
bool nonbase64(unsigned char ch) { return b64lookup(ch) == -1 && ch != '='; }

static bool base64_push(upb_json_parser *p, upb_selector_t sel, const char *ptr,
                        size_t len) {
  const char *limit = ptr + len;
  for (; ptr < limit; ptr += 4) {
    uint32_t val;
    char output[3];

    if (limit - ptr < 4) {
      upb_status_seterrf(&p->status,
                         "Base64 input for bytes field not a multiple of 4: %s",
                         upb_fielddef_name(p->top->f));
      upb_env_reporterror(p->env, &p->status);
      return false;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12 |
          b64lookup(ptr[2]) << 6  |
          b64lookup(ptr[3]);

    /* Test the upper bit; returns true if any of the characters returned -1. */
    if (val & 0x80000000) {
      goto otherchar;
    }

    output[0] = val >> 16;
    output[1] = (val >> 8) & 0xff;
    output[2] = val & 0xff;
    upb_sink_putstring(&p->top->sink, sel, output, 3, NULL);
  }
  return true;

otherchar:
  if (nonbase64(ptr[0]) || nonbase64(ptr[1]) || nonbase64(ptr[2]) ||
      nonbase64(ptr[3]) ) {
    upb_status_seterrf(&p->status,
                       "Non-base64 characters in bytes field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  } if (ptr[2] == '=') {
    uint32_t val;
    char output;

    /* Last group contains only two input bytes, one output byte. */
    if (ptr[0] == '=' || ptr[1] == '=' || ptr[3] != '=') {
      goto badpadding;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12;

    assert(!(val & 0x80000000));
    output = val >> 16;
    upb_sink_putstring(&p->top->sink, sel, &output, 1, NULL);
    return true;
  } else {
    uint32_t val;
    char output[2];

    /* Last group contains only three input bytes, two output bytes. */
    if (ptr[0] == '=' || ptr[1] == '=' || ptr[2] == '=') {
      goto badpadding;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12 |
          b64lookup(ptr[2]) << 6;

    output[0] = val >> 16;
    output[1] = (val >> 8) & 0xff;
    upb_sink_putstring(&p->top->sink, sel, output, 2, NULL);
    return true;
  }

badpadding:
  upb_status_seterrf(&p->status,
                     "Incorrect base64 padding for field: %s (%.*s)",
                     upb_fielddef_name(p->top->f),
                     4, ptr);
  upb_env_reporterror(p->env, &p->status);
  return false;
}


/* Accumulate buffer **********************************************************/

/* Functionality for accumulating a buffer.
 *
 * Some parts of the parser need an entire value as a contiguous string.  For
 * example, to look up a member name in a hash table, or to turn a string into
 * a number, the relevant library routines need the input string to be in
 * contiguous memory, even if the value spanned two or more buffers in the
 * input.  These routines handle that.
 *
 * In the common case we can just point to the input buffer to get this
 * contiguous string and avoid any actual copy.  So we optimistically begin
 * this way.  But there are a few cases where we must instead copy into a
 * separate buffer:
 *
 *   1. The string was not contiguous in the input (it spanned buffers).
 *
 *   2. The string included escape sequences that need to be interpreted to get
 *      the true value in a contiguous buffer. */

static void assert_accumulate_empty(upb_json_parser *p) {
  UPB_UNUSED(p);
  assert(p->accumulated == NULL);
  assert(p->accumulated_len == 0);
}

static void accumulate_clear(upb_json_parser *p) {
  p->accumulated = NULL;
  p->accumulated_len = 0;
}

/* Used internally by accumulate_append(). */
static bool accumulate_realloc(upb_json_parser *p, size_t need) {
  void *mem;
  size_t old_size = p->accumulate_buf_size;
  size_t new_size = UPB_MAX(old_size, 128);
  while (new_size < need) {
    new_size = saturating_multiply(new_size, 2);
  }

  mem = upb_env_realloc(p->env, p->accumulate_buf, old_size, new_size);
  if (!mem) {
    upb_status_seterrmsg(&p->status, "Out of memory allocating buffer.");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  p->accumulate_buf = mem;
  p->accumulate_buf_size = new_size;
  return true;
}

/* Logically appends the given data to the append buffer.
 * If "can_alias" is true, we will try to avoid actually copying, but the buffer
 * must be valid until the next accumulate_append() call (if any). */
static bool accumulate_append(upb_json_parser *p, const char *buf, size_t len,
                              bool can_alias) {
  size_t need;

  if (!p->accumulated && can_alias) {
    p->accumulated = buf;
    p->accumulated_len = len;
    return true;
  }

  if (!checked_add(p->accumulated_len, len, &need)) {
    upb_status_seterrmsg(&p->status, "Integer overflow.");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  if (need > p->accumulate_buf_size && !accumulate_realloc(p, need)) {
    return false;
  }

  if (p->accumulated != p->accumulate_buf) {
    memcpy(p->accumulate_buf, p->accumulated, p->accumulated_len);
    p->accumulated = p->accumulate_buf;
  }

  memcpy(p->accumulate_buf + p->accumulated_len, buf, len);
  p->accumulated_len += len;
  return true;
}

/* Returns a pointer to the data accumulated since the last accumulate_clear()
 * call, and writes the length to *len.  This with point either to the input
 * buffer or a temporary accumulate buffer. */
static const char *accumulate_getptr(upb_json_parser *p, size_t *len) {
  assert(p->accumulated);
  *len = p->accumulated_len;
  return p->accumulated;
}


/* Mult-part text data ********************************************************/

/* When we have text data in the input, it can often come in multiple segments.
 * For example, there may be some raw string data followed by an escape
 * sequence.  The two segments are processed with different logic.  Also buffer
 * seams in the input can cause multiple segments.
 *
 * As we see segments, there are two main cases for how we want to process them:
 *
 *  1. we want to push the captured input directly to string handlers.
 *
 *  2. we need to accumulate all the parts into a contiguous buffer for further
 *     processing (field name lookup, string->number conversion, etc). */

/* This is the set of states for p->multipart_state. */
enum {
  /* We are not currently processing multipart data. */
  MULTIPART_INACTIVE = 0,

  /* We are processing multipart data by accumulating it into a contiguous
   * buffer. */
  MULTIPART_ACCUMULATE = 1,

  /* We are processing multipart data by pushing each part directly to the
   * current string handlers. */
  MULTIPART_PUSHEAGERLY = 2
};

/* Start a multi-part text value where we accumulate the data for processing at
 * the end. */
static void multipart_startaccum(upb_json_parser *p) {
  assert_accumulate_empty(p);
  assert(p->multipart_state == MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_ACCUMULATE;
}

/* Start a multi-part text value where we immediately push text data to a string
 * value with the given selector. */
static void multipart_start(upb_json_parser *p, upb_selector_t sel) {
  assert_accumulate_empty(p);
  assert(p->multipart_state == MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_PUSHEAGERLY;
  p->string_selector = sel;
}

static bool multipart_text(upb_json_parser *p, const char *buf, size_t len,
                           bool can_alias) {
  switch (p->multipart_state) {
    case MULTIPART_INACTIVE:
      upb_status_seterrmsg(
          &p->status, "Internal error: unexpected state MULTIPART_INACTIVE");
      upb_env_reporterror(p->env, &p->status);
      return false;

    case MULTIPART_ACCUMULATE:
      if (!accumulate_append(p, buf, len, can_alias)) {
        return false;
      }
      break;

    case MULTIPART_PUSHEAGERLY: {
      const upb_bufhandle *handle = can_alias ? p->handle : NULL;
      upb_sink_putstring(&p->top->sink, p->string_selector, buf, len, handle);
      break;
    }
  }

  return true;
}

/* Note: this invalidates the accumulate buffer!  Call only after reading its
 * contents. */
static void multipart_end(upb_json_parser *p) {
  assert(p->multipart_state != MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_INACTIVE;
  accumulate_clear(p);
}


/* Input capture **************************************************************/

/* Functionality for capturing a region of the input as text.  Gracefully
 * handles the case where a buffer seam occurs in the middle of the captured
 * region. */

static void capture_begin(upb_json_parser *p, const char *ptr) {
  assert(p->multipart_state != MULTIPART_INACTIVE);
  assert(p->capture == NULL);
  p->capture = ptr;
}

static bool capture_end(upb_json_parser *p, const char *ptr) {
  assert(p->capture);
  if (multipart_text(p, p->capture, ptr - p->capture, true)) {
    p->capture = NULL;
    return true;
  } else {
    return false;
  }
}

/* This is called at the end of each input buffer (ie. when we have hit a
 * buffer seam).  If we are in the middle of capturing the input, this
 * processes the unprocessed capture region. */
static void capture_suspend(upb_json_parser *p, const char **ptr) {
  if (!p->capture) return;

  if (multipart_text(p, p->capture, *ptr - p->capture, false)) {
    /* We use this as a signal that we were in the middle of capturing, and
     * that capturing should resume at the beginning of the next buffer.
     * 
     * We can't use *ptr here, because we have no guarantee that this pointer
     * will be valid when we resume (if the underlying memory is freed, then
     * using the pointer at all, even to compare to NULL, is likely undefined
     * behavior). */
    p->capture = &suspend_capture;
  } else {
    /* Need to back up the pointer to the beginning of the capture, since
     * we were not able to actually preserve it. */
    *ptr = p->capture;
  }
}

static void capture_resume(upb_json_parser *p, const char *ptr) {
  if (p->capture) {
    assert(p->capture == &suspend_capture);
    p->capture = ptr;
  }
}


/* Callbacks from the parser **************************************************/

/* These are the functions called directly from the parser itself.
 * We define these in the same order as their declarations in the parser. */

static char escape_char(char in) {
  switch (in) {
    case 'r': return '\r';
    case 't': return '\t';
    case 'n': return '\n';
    case 'f': return '\f';
    case 'b': return '\b';
    case '/': return '/';
    case '"': return '"';
    case '\\': return '\\';
    default:
      assert(0);
      return 'x';
  }
}

static bool escape(upb_json_parser *p, const char *ptr) {
  char ch = escape_char(*ptr);
  return multipart_text(p, &ch, 1, false);
}

static void start_hex(upb_json_parser *p) {
  p->digit = 0;
}

static void hexdigit(upb_json_parser *p, const char *ptr) {
  char ch = *ptr;

  p->digit <<= 4;

  if (ch >= '0' && ch <= '9') {
    p->digit += (ch - '0');
  } else if (ch >= 'a' && ch <= 'f') {
    p->digit += ((ch - 'a') + 10);
  } else {
    assert(ch >= 'A' && ch <= 'F');
    p->digit += ((ch - 'A') + 10);
  }
}

static bool end_hex(upb_json_parser *p) {
  uint32_t codepoint = p->digit;

  /* emit the codepoint as UTF-8. */
  char utf8[3]; /* support \u0000 -- \uFFFF -- need only three bytes. */
  int length = 0;
  if (codepoint <= 0x7F) {
    utf8[0] = codepoint;
    length = 1;
  } else if (codepoint <= 0x07FF) {
    utf8[1] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[0] = (codepoint & 0x1F) | 0xC0;
    length = 2;
  } else /* codepoint <= 0xFFFF */ {
    utf8[2] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[1] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[0] = (codepoint & 0x0F) | 0xE0;
    length = 3;
  }
  /* TODO(haberman): Handle high surrogates: if codepoint is a high surrogate
   * we have to wait for the next escape to get the full code point). */

  return multipart_text(p, utf8, length, false);
}

static void start_text(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_text(upb_json_parser *p, const char *ptr) {
  return capture_end(p, ptr);
}

static void start_number(upb_json_parser *p, const char *ptr) {
  multipart_startaccum(p);
  capture_begin(p, ptr);
}

static bool parse_number(upb_json_parser *p);

static bool end_number(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }

  return parse_number(p);
}

static bool parse_number(upb_json_parser *p) {
  size_t len;
  const char *buf;
  const char *myend;
  char *end;

  /* strtol() and friends unfortunately do not support specifying the length of
   * the input string, so we need to force a copy into a NULL-terminated buffer. */
  if (!multipart_text(p, "\0", 1, false)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);
  myend = buf + len - 1;  /* One for NULL. */

  /* XXX: We are using strtol to parse integers, but this is wrong as even
   * integers can be represented as 1e6 (for example), which strtol can't
   * handle correctly.
   *
   * XXX: Also, we can't handle large integers properly because strto[u]ll
   * isn't in C89.
   *
   * XXX: Also, we don't properly check floats for overflow, since strtof
   * isn't in C89. */
  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32: {
      long val = strtol(p->accumulated, &end, 0);
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putint32(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_INT64: {
      long long val = strtol(p->accumulated, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putint64(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(p->accumulated, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putuint32(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_UINT64: {
      unsigned long long val = strtoul(p->accumulated, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putuint64(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(p->accumulated, &end);
      if (errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putdouble(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_FLOAT: {
      float val = strtod(p->accumulated, &end);
      if (errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putfloat(&p->top->sink, parser_getsel(p), val);
      break;
    }
    default:
      assert(false);
  }

  multipart_end(p);

  return true;

err:
  upb_status_seterrf(&p->status, "error parsing number: %s", buf);
  upb_env_reporterror(p->env, &p->status);
  multipart_end(p);
  return false;
}

static bool parser_putbool(upb_json_parser *p, bool val) {
  bool ok;

  if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL) {
    upb_status_seterrf(&p->status,
                       "Boolean value specified for non-bool field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  ok = upb_sink_putbool(&p->top->sink, parser_getsel(p), val);
  UPB_ASSERT_VAR(ok, ok);

  return true;
}

static bool start_stringval(upb_json_parser *p) {
  assert(p->top->f);

  if (upb_fielddef_isstring(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    if (!check_stack(p)) return false;

    /* Start a new parser frame: parser frames correspond one-to-one with
     * handler frames, and string events occur in a sub-frame. */
    inner = p->top + 1;
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
    upb_sink_startstr(&p->top->sink, sel, 0, &inner->sink);
    inner->m = p->top->m;
    inner->f = p->top->f;
    inner->is_map = false;
    inner->is_mapentry = false;
    p->top = inner;

    if (upb_fielddef_type(p->top->f) == UPB_TYPE_STRING) {
      /* For STRING fields we push data directly to the handlers as it is
       * parsed.  We don't do this yet for BYTES fields, because our base64
       * decoder is not streaming.
       *
       * TODO(haberman): make base64 decoding streaming also. */
      multipart_start(p, getsel_for_handlertype(p, UPB_HANDLER_STRING));
      return true;
    } else {
      multipart_startaccum(p);
      return true;
    }
  } else if (upb_fielddef_type(p->top->f) == UPB_TYPE_ENUM) {
    /* No need to push a frame -- symbolic enum names in quotes remain in the
     * current parser frame.
     *
     * Enum string values must accumulate so we can look up the value in a table
     * once it is complete. */
    multipart_startaccum(p);
    return true;
  } else {
    upb_status_seterrf(&p->status,
                       "String specified for non-string/non-enum field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }
}

static bool end_stringval(upb_json_parser *p) {
  bool ok = true;

  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_BYTES:
      if (!base64_push(p, getsel_for_handlertype(p, UPB_HANDLER_STRING),
                       p->accumulated, p->accumulated_len)) {
        return false;
      }
      /* Fall through. */

    case UPB_TYPE_STRING: {
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(&p->top->sink, sel);
      p->top--;
      break;
    }

    case UPB_TYPE_ENUM: {
      /* Resolve enum symbolic name to integer value. */
      const upb_enumdef *enumdef =
          (const upb_enumdef*)upb_fielddef_subdef(p->top->f);

      size_t len;
      const char *buf = accumulate_getptr(p, &len);

      int32_t int_val = 0;
      ok = upb_enumdef_ntoi(enumdef, buf, len, &int_val);

      if (ok) {
        upb_selector_t sel = parser_getsel(p);
        upb_sink_putint32(&p->top->sink, sel, int_val);
      } else {
        upb_status_seterrf(&p->status, "Enum value unknown: '%.*s'", len, buf);
        upb_env_reporterror(p->env, &p->status);
      }

      break;
    }

    default:
      assert(false);
      upb_status_seterrmsg(&p->status, "Internal error in JSON decoder");
      upb_env_reporterror(p->env, &p->status);
      ok = false;
      break;
  }

  multipart_end(p);

  return ok;
}

static void start_member(upb_json_parser *p) {
  assert(!p->top->f);
  multipart_startaccum(p);
}

/* Helper: invoked during parse_mapentry() to emit the mapentry message's key
 * field based on the current contents of the accumulate buffer. */
static bool parse_mapentry_key(upb_json_parser *p) {

  size_t len;
  const char *buf = accumulate_getptr(p, &len);

  /* Emit the key field. We do a bit of ad-hoc parsing here because the
   * parser state machine has already decided that this is a string field
   * name, and we are reinterpreting it as some arbitrary key type. In
   * particular, integer and bool keys are quoted, so we need to parse the
   * quoted string contents here. */

  p->top->f = upb_msgdef_itof(p->top->m, UPB_MAPENTRY_KEY);
  if (p->top->f == NULL) {
    upb_status_seterrmsg(&p->status, "mapentry message has no key");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }
  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      /* Invoke end_number. The accum buffer has the number's text already. */
      if (!parse_number(p)) {
        return false;
      }
      break;
    case UPB_TYPE_BOOL:
      if (len == 4 && !strncmp(buf, "true", 4)) {
        if (!parser_putbool(p, true)) {
          return false;
        }
      } else if (len == 5 && !strncmp(buf, "false", 5)) {
        if (!parser_putbool(p, false)) {
          return false;
        }
      } else {
        upb_status_seterrmsg(&p->status,
                             "Map bool key not 'true' or 'false'");
        upb_env_reporterror(p->env, &p->status);
        return false;
      }
      multipart_end(p);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      upb_sink subsink;
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
      upb_sink_startstr(&p->top->sink, sel, len, &subsink);
      sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
      upb_sink_putstring(&subsink, sel, buf, len, NULL);
      sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(&subsink, sel);
      multipart_end(p);
      break;
    }
    default:
      upb_status_seterrmsg(&p->status, "Invalid field type for map key");
      upb_env_reporterror(p->env, &p->status);
      return false;
  }

  return true;
}

/* Helper: emit one map entry (as a submessage in the map field sequence). This
 * is invoked from end_membername(), at the end of the map entry's key string,
 * with the map key in the accumulate buffer. It parses the key from that
 * buffer, emits the handler calls to start the mapentry submessage (setting up
 * its subframe in the process), and sets up state in the subframe so that the
 * value parser (invoked next) will emit the mapentry's value field and then
 * end the mapentry message. */

static bool handle_mapentry(upb_json_parser *p) {
  const upb_fielddef *mapfield;
  const upb_msgdef *mapentrymsg;
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  /* Map entry: p->top->sink is the seq frame, so we need to start a frame
   * for the mapentry itself, and then set |f| in that frame so that the map
   * value field is parsed, and also set a flag to end the frame after the
   * map-entry value is parsed. */
  if (!check_stack(p)) return false;

  mapfield = p->top->mapfield;
  mapentrymsg = upb_fielddef_msgsubdef(mapfield);

  inner = p->top + 1;
  p->top->f = mapfield;
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
  upb_sink_startsubmsg(&p->top->sink, sel, &inner->sink);
  inner->m = mapentrymsg;
  inner->mapfield = mapfield;
  inner->is_map = false;

  /* Don't set this to true *yet* -- we reuse parsing handlers below to push
   * the key field value to the sink, and these handlers will pop the frame
   * if they see is_mapentry (when invoked by the parser state machine, they
   * would have just seen the map-entry value, not key). */
  inner->is_mapentry = false;
  p->top = inner;

  /* send STARTMSG in submsg frame. */
  upb_sink_startmsg(&p->top->sink);

  parse_mapentry_key(p);

  /* Set up the value field to receive the map-entry value. */
  p->top->f = upb_msgdef_itof(p->top->m, UPB_MAPENTRY_VALUE);
  p->top->is_mapentry = true;  /* set up to pop frame after value is parsed. */
  p->top->mapfield = mapfield;
  if (p->top->f == NULL) {
    upb_status_seterrmsg(&p->status, "mapentry message has no value");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  return true;
}

static bool end_membername(upb_json_parser *p) {
  assert(!p->top->f);

  if (p->top->is_map) {
    return handle_mapentry(p);
  } else {
    size_t len;
    const char *buf = accumulate_getptr(p, &len);
    const upb_fielddef *f = upb_msgdef_ntof(p->top->m, buf, len);

    if (!f) {
      /* TODO(haberman): Ignore unknown fields if requested/configured to do
       * so. */
      upb_status_seterrf(&p->status, "No such field: %.*s\n", (int)len, buf);
      upb_env_reporterror(p->env, &p->status);
      return false;
    }

    p->top->f = f;
    multipart_end(p);

    return true;
  }
}

static void end_member(upb_json_parser *p) {
  /* If we just parsed a map-entry value, end that frame too. */
  if (p->top->is_mapentry) {
    upb_status s = UPB_STATUS_INIT;
    upb_selector_t sel;
    bool ok;
    const upb_fielddef *mapfield;

    assert(p->top > p->stack);
    /* send ENDMSG on submsg. */
    upb_sink_endmsg(&p->top->sink, &s);
    mapfield = p->top->mapfield;

    /* send ENDSUBMSG in repeated-field-of-mapentries frame. */
    p->top--;
    ok = upb_handlers_getselector(mapfield, UPB_HANDLER_ENDSUBMSG, &sel);
    UPB_ASSERT_VAR(ok, ok);
    upb_sink_endsubmsg(&p->top->sink, sel);
  }

  p->top->f = NULL;
}

static bool start_subobject(upb_json_parser *p) {
  assert(p->top->f);

  if (upb_fielddef_ismap(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a map. Start a new parser frame in a repeated-field
     * context. */
    if (!check_stack(p)) return false;

    inner = p->top + 1;
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
    upb_sink_startseq(&p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    inner->mapfield = p->top->f;
    inner->f = NULL;
    inner->is_map = true;
    inner->is_mapentry = false;
    p->top = inner;

    return true;
  } else if (upb_fielddef_issubmsg(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a subobject. Start a new parser frame in the submsg
     * context. */
    if (!check_stack(p)) return false;

    inner = p->top + 1;

    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
    upb_sink_startsubmsg(&p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    inner->f = NULL;
    inner->is_map = false;
    inner->is_mapentry = false;
    p->top = inner;

    return true;
  } else {
    upb_status_seterrf(&p->status,
                       "Object specified for non-message/group field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }
}

static void end_subobject(upb_json_parser *p) {
  if (p->top->is_map) {
    upb_selector_t sel;
    p->top--;
    sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
    upb_sink_endseq(&p->top->sink, sel);
  } else {
    upb_selector_t sel;
    p->top--;
    sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSUBMSG);
    upb_sink_endsubmsg(&p->top->sink, sel);
  }
}

static bool start_array(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  assert(p->top->f);

  if (!upb_fielddef_isseq(p->top->f)) {
    upb_status_seterrf(&p->status,
                       "Array specified for non-repeated field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  if (!check_stack(p)) return false;

  inner = p->top + 1;
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
  upb_sink_startseq(&p->top->sink, sel, &inner->sink);
  inner->m = p->top->m;
  inner->f = p->top->f;
  inner->is_map = false;
  inner->is_mapentry = false;
  p->top = inner;

  return true;
}

static void end_array(upb_json_parser *p) {
  upb_selector_t sel;

  assert(p->top > p->stack);

  p->top--;
  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
  upb_sink_endseq(&p->top->sink, sel);
}

static void start_object(upb_json_parser *p) {
  if (!p->top->is_map) {
    upb_sink_startmsg(&p->top->sink);
  }
}

static void end_object(upb_json_parser *p) {
  if (!p->top->is_map) {
    upb_status status;
    upb_status_clear(&status);
    upb_sink_endmsg(&p->top->sink, &status);
    if (!upb_ok(&status)) {
      upb_env_reporterror(p->env, &status);
    }
  }
}


#define CHECK_RETURN_TOP(x) if (!(x)) goto error


/* The actual parser **********************************************************/

/* What follows is the Ragel parser itself.  The language is specified in Ragel
 * and the actions call our C functions above.
 *
 * Ragel has an extensive set of functionality, and we use only a small part of
 * it.  There are many action types but we only use a few:
 *
 *   ">" -- transition into a machine
 *   "%" -- transition out of a machine
 *   "@" -- transition into a final state of a machine.
 *
 * "@" transitions are tricky because a machine can transition into a final
 * state repeatedly.  But in some cases we know this can't happen, for example
 * a string which is delimited by a final '"' can only transition into its
 * final state once, when the closing '"' is seen. */


#line 1218 "upb/json/parser.rl"



#line 1130 "upb/json/parser.c"
static const char _json_actions[] = {
	0, 1, 0, 1, 2, 1, 3, 1, 
	5, 1, 6, 1, 7, 1, 8, 1, 
	10, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 1, 21, 1, 
	25, 1, 27, 2, 3, 8, 2, 4, 
	5, 2, 6, 2, 2, 6, 8, 2, 
	11, 9, 2, 13, 15, 2, 14, 15, 
	2, 18, 1, 2, 19, 27, 2, 20, 
	9, 2, 22, 27, 2, 23, 27, 2, 
	24, 27, 2, 26, 27, 3, 14, 11, 
	9
};

static const unsigned char _json_key_offsets[] = {
	0, 0, 4, 9, 14, 15, 19, 24, 
	29, 34, 38, 42, 45, 48, 50, 54, 
	58, 60, 62, 67, 69, 71, 80, 86, 
	92, 98, 104, 106, 115, 116, 116, 116, 
	121, 126, 131, 132, 133, 134, 135, 135, 
	136, 137, 138, 138, 139, 140, 141, 141, 
	146, 151, 152, 156, 161, 166, 171, 175, 
	175, 178, 178, 178
};

static const char _json_trans_keys[] = {
	32, 123, 9, 13, 32, 34, 125, 9, 
	13, 32, 34, 125, 9, 13, 34, 32, 
	58, 9, 13, 32, 93, 125, 9, 13, 
	32, 44, 125, 9, 13, 32, 44, 125, 
	9, 13, 32, 34, 9, 13, 45, 48, 
	49, 57, 48, 49, 57, 46, 69, 101, 
	48, 57, 69, 101, 48, 57, 43, 45, 
	48, 57, 48, 57, 48, 57, 46, 69, 
	101, 48, 57, 34, 92, 34, 92, 34, 
	47, 92, 98, 102, 110, 114, 116, 117, 
	48, 57, 65, 70, 97, 102, 48, 57, 
	65, 70, 97, 102, 48, 57, 65, 70, 
	97, 102, 48, 57, 65, 70, 97, 102, 
	34, 92, 34, 45, 91, 102, 110, 116, 
	123, 48, 57, 34, 32, 93, 125, 9, 
	13, 32, 44, 93, 9, 13, 32, 93, 
	125, 9, 13, 97, 108, 115, 101, 117, 
	108, 108, 114, 117, 101, 32, 34, 125, 
	9, 13, 32, 34, 125, 9, 13, 34, 
	32, 58, 9, 13, 32, 93, 125, 9, 
	13, 32, 44, 125, 9, 13, 32, 44, 
	125, 9, 13, 32, 34, 9, 13, 32, 
	9, 13, 0
};

static const char _json_single_lengths[] = {
	0, 2, 3, 3, 1, 2, 3, 3, 
	3, 2, 2, 1, 3, 0, 2, 2, 
	0, 0, 3, 2, 2, 9, 0, 0, 
	0, 0, 2, 7, 1, 0, 0, 3, 
	3, 3, 1, 1, 1, 1, 0, 1, 
	1, 1, 0, 1, 1, 1, 0, 3, 
	3, 1, 2, 3, 3, 3, 2, 0, 
	1, 0, 0, 0
};

static const char _json_range_lengths[] = {
	0, 1, 1, 1, 0, 1, 1, 1, 
	1, 1, 1, 1, 0, 1, 1, 1, 
	1, 1, 1, 0, 0, 0, 3, 3, 
	3, 3, 0, 1, 0, 0, 0, 1, 
	1, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 0, 1, 1, 1, 1, 1, 0, 
	1, 0, 0, 0
};

static const short _json_index_offsets[] = {
	0, 0, 4, 9, 14, 16, 20, 25, 
	30, 35, 39, 43, 46, 50, 52, 56, 
	60, 62, 64, 69, 72, 75, 85, 89, 
	93, 97, 101, 104, 113, 115, 116, 117, 
	122, 127, 132, 134, 136, 138, 140, 141, 
	143, 145, 147, 148, 150, 152, 154, 155, 
	160, 165, 167, 171, 176, 181, 186, 190, 
	191, 194, 195, 196
};

static const char _json_indicies[] = {
	0, 2, 0, 1, 3, 4, 5, 3, 
	1, 6, 7, 8, 6, 1, 9, 1, 
	10, 11, 10, 1, 11, 1, 1, 11, 
	12, 13, 14, 15, 13, 1, 16, 17, 
	8, 16, 1, 17, 7, 17, 1, 18, 
	19, 20, 1, 19, 20, 1, 22, 23, 
	23, 21, 24, 1, 23, 23, 24, 21, 
	25, 25, 26, 1, 26, 1, 26, 21, 
	22, 23, 23, 20, 21, 28, 29, 27, 
	31, 32, 30, 33, 33, 33, 33, 33, 
	33, 33, 33, 34, 1, 35, 35, 35, 
	1, 36, 36, 36, 1, 37, 37, 37, 
	1, 38, 38, 38, 1, 40, 41, 39, 
	42, 43, 44, 45, 46, 47, 48, 43, 
	1, 49, 1, 50, 51, 53, 54, 1, 
	53, 52, 55, 56, 54, 55, 1, 56, 
	1, 1, 56, 52, 57, 1, 58, 1, 
	59, 1, 60, 1, 61, 62, 1, 63, 
	1, 64, 1, 65, 66, 1, 67, 1, 
	68, 1, 69, 70, 71, 72, 70, 1, 
	73, 74, 75, 73, 1, 76, 1, 77, 
	78, 77, 1, 78, 1, 1, 78, 79, 
	80, 81, 82, 80, 1, 83, 84, 75, 
	83, 1, 84, 74, 84, 1, 85, 86, 
	86, 1, 1, 1, 1, 0
};

static const char _json_trans_targs[] = {
	1, 0, 2, 3, 4, 56, 3, 4, 
	56, 5, 5, 6, 7, 8, 9, 56, 
	8, 9, 11, 12, 18, 57, 13, 15, 
	14, 16, 17, 20, 58, 21, 20, 58, 
	21, 19, 22, 23, 24, 25, 26, 20, 
	58, 21, 28, 30, 31, 34, 39, 43, 
	47, 29, 59, 59, 32, 31, 29, 32, 
	33, 35, 36, 37, 38, 59, 40, 41, 
	42, 59, 44, 45, 46, 59, 48, 49, 
	55, 48, 49, 55, 50, 50, 51, 52, 
	53, 54, 55, 53, 54, 59, 56
};

static const char _json_trans_actions[] = {
	0, 0, 0, 21, 77, 53, 0, 47, 
	23, 17, 0, 0, 15, 19, 19, 50, 
	0, 0, 0, 0, 0, 1, 0, 0, 
	0, 0, 0, 3, 13, 0, 0, 35, 
	5, 11, 0, 38, 7, 7, 7, 41, 
	44, 9, 62, 56, 25, 0, 0, 0, 
	31, 29, 33, 59, 15, 0, 27, 0, 
	0, 0, 0, 0, 0, 68, 0, 0, 
	0, 71, 0, 0, 0, 65, 21, 77, 
	53, 0, 47, 23, 17, 0, 0, 15, 
	19, 19, 50, 0, 0, 74, 0
};

static const int json_start = 1;

static const int json_en_number_machine = 10;
static const int json_en_string_machine = 19;
static const int json_en_value_machine = 27;
static const int json_en_main = 1;


#line 1221 "upb/json/parser.rl"

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle) {
  upb_json_parser *parser = closure;

  /* Variables used by Ragel's generated code. */
  int cs = parser->current_state;
  int *stack = parser->parser_stack;
  int top = parser->parser_top;

  const char *p = buf;
  const char *pe = buf + size;

  parser->handle = handle;

  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  capture_resume(parser, buf);

  
#line 1301 "upb/json/parser.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _json_trans_keys + _json_key_offsets[cs];
	_trans = _json_index_offsets[cs];

	_klen = _json_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _json_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _json_indicies[_trans];
	cs = _json_trans_targs[_trans];

	if ( _json_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _json_actions + _json_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 1133 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 1:
#line 1134 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 10; goto _again;} }
	break;
	case 2:
#line 1138 "upb/json/parser.rl"
	{ start_text(parser, p); }
	break;
	case 3:
#line 1139 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_text(parser, p)); }
	break;
	case 4:
#line 1145 "upb/json/parser.rl"
	{ start_hex(parser); }
	break;
	case 5:
#line 1146 "upb/json/parser.rl"
	{ hexdigit(parser, p); }
	break;
	case 6:
#line 1147 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_hex(parser)); }
	break;
	case 7:
#line 1153 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(escape(parser, p)); }
	break;
	case 8:
#line 1159 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 9:
#line 1162 "upb/json/parser.rl"
	{ {stack[top++] = cs; cs = 19; goto _again;} }
	break;
	case 10:
#line 1164 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 27; goto _again;} }
	break;
	case 11:
#line 1169 "upb/json/parser.rl"
	{ start_member(parser); }
	break;
	case 12:
#line 1170 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_membername(parser)); }
	break;
	case 13:
#line 1173 "upb/json/parser.rl"
	{ end_member(parser); }
	break;
	case 14:
#line 1179 "upb/json/parser.rl"
	{ start_object(parser); }
	break;
	case 15:
#line 1182 "upb/json/parser.rl"
	{ end_object(parser); }
	break;
	case 16:
#line 1188 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_array(parser)); }
	break;
	case 17:
#line 1192 "upb/json/parser.rl"
	{ end_array(parser); }
	break;
	case 18:
#line 1197 "upb/json/parser.rl"
	{ start_number(parser, p); }
	break;
	case 19:
#line 1198 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_number(parser, p)); }
	break;
	case 20:
#line 1200 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_stringval(parser)); }
	break;
	case 21:
#line 1201 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_stringval(parser)); }
	break;
	case 22:
#line 1203 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(parser_putbool(parser, true)); }
	break;
	case 23:
#line 1205 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(parser_putbool(parser, false)); }
	break;
	case 24:
#line 1207 "upb/json/parser.rl"
	{ /* null value */ }
	break;
	case 25:
#line 1209 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_subobject(parser)); }
	break;
	case 26:
#line 1210 "upb/json/parser.rl"
	{ end_subobject(parser); }
	break;
	case 27:
#line 1215 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
#line 1487 "upb/json/parser.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 1242 "upb/json/parser.rl"

  if (p != pe) {
    upb_status_seterrf(&parser->status, "Parse error at %s\n", p);
    upb_env_reporterror(parser->env, &parser->status);
  } else {
    capture_suspend(parser, &p);
  }

error:
  /* Save parsing state back to parser. */
  parser->current_state = cs;
  parser->parser_top = top;

  return p - buf;
}

bool end(void *closure, const void *hd) {
  UPB_UNUSED(closure);
  UPB_UNUSED(hd);

  /* Prevent compile warning on unused static constants. */
  UPB_UNUSED(json_start);
  UPB_UNUSED(json_en_number_machine);
  UPB_UNUSED(json_en_string_machine);
  UPB_UNUSED(json_en_value_machine);
  UPB_UNUSED(json_en_main);
  return true;
}

static void json_parser_reset(upb_json_parser *p) {
  int cs;
  int top;

  p->top = p->stack;
  p->top->f = NULL;
  p->top->is_map = false;
  p->top->is_mapentry = false;

  /* Emit Ragel initialization of the parser. */
  
#line 1541 "upb/json/parser.c"
	{
	cs = json_start;
	top = 0;
	}

#line 1282 "upb/json/parser.rl"
  p->current_state = cs;
  p->parser_top = top;
  accumulate_clear(p);
  p->multipart_state = MULTIPART_INACTIVE;
  p->capture = NULL;
  p->accumulated = NULL;
  upb_status_clear(&p->status);
}


/* Public API *****************************************************************/

upb_json_parser *upb_json_parser_create(upb_env *env, upb_sink *output) {
#ifndef NDEBUG
  const size_t size_before = upb_env_bytesallocated(env);
#endif
  upb_json_parser *p = upb_env_malloc(env, sizeof(upb_json_parser));
  if (!p) return false;

  p->env = env;
  p->limit = p->stack + UPB_JSON_MAX_DEPTH;
  p->accumulate_buf = NULL;
  p->accumulate_buf_size = 0;
  upb_byteshandler_init(&p->input_handler_);
  upb_byteshandler_setstring(&p->input_handler_, parse, NULL);
  upb_byteshandler_setendstr(&p->input_handler_, end, NULL);
  upb_bytessink_reset(&p->input_, &p->input_handler_, p);

  json_parser_reset(p);
  upb_sink_reset(&p->top->sink, output->handlers, output->closure);
  p->top->m = upb_handlers_msgdef(output->handlers);

  /* If this fails, uncomment and increase the value in parser.h. */
  /* fprintf(stderr, "%zd\n", upb_env_bytesallocated(env) - size_before); */
  assert(upb_env_bytesallocated(env) - size_before <= UPB_JSON_PARSER_SIZE);
  return p;
}

upb_bytessink *upb_json_parser_input(upb_json_parser *p) {
  return &p->input_;
}
/*
** This currently uses snprintf() to format primitives, and could be optimized
** further.
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

struct upb_json_printer {
  upb_sink input_;
  /* BytesSink closure. */
  void *subc_;
  upb_bytessink *output_;

  /* We track the depth so that we know when to emit startstr/endstr on the
   * output. */
  int depth_;

  /* Have we emitted the first element? This state is necessary to emit commas
   * without leaving a trailing comma in arrays/maps. We keep this state per
   * frame depth.
   *
   * Why max_depth * 2? UPB_MAX_HANDLER_DEPTH counts depth as nested messages.
   * We count frames (contexts in which we separate elements by commas) as both
   * repeated fields and messages (maps), and the worst case is a
   * message->repeated field->submessage->repeated field->... nesting. */
  bool first_elem_[UPB_MAX_HANDLER_DEPTH * 2];
};

/* StringPiece; a pointer plus a length. */
typedef struct {
  const char *ptr;
  size_t len;
} strpc;

strpc *newstrpc(upb_handlers *h, const upb_fielddef *f) {
  strpc *ret = malloc(sizeof(*ret));
  ret->ptr = upb_fielddef_name(f);
  ret->len = strlen(ret->ptr);
  upb_handlers_addcleanup(h, ret, free);
  return ret;
}

/* ------------ JSON string printing: values, maps, arrays ------------------ */

static void print_data(
    upb_json_printer *p, const char *buf, unsigned int len) {
  /* TODO: Will need to change if we support pushback from the sink. */
  size_t n = upb_bytessink_putbuf(p->output_, p->subc_, buf, len, NULL);
  UPB_ASSERT_VAR(n, n == len);
}

static void print_comma(upb_json_printer *p) {
  if (!p->first_elem_[p->depth_]) {
    print_data(p, ",", 1);
  }
  p->first_elem_[p->depth_] = false;
}

/* Helpers that print properly formatted elements to the JSON output stream. */

/* Used for escaping control chars in strings. */
static const char kControlCharLimit = 0x20;

UPB_INLINE bool is_json_escaped(char c) {
  /* See RFC 4627. */
  unsigned char uc = (unsigned char)c;
  return uc < kControlCharLimit || uc == '"' || uc == '\\';
}

UPB_INLINE char* json_nice_escape(char c) {
  switch (c) {
    case '"':  return "\\\"";
    case '\\': return "\\\\";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    default:   return NULL;
  }
}

/* Write a properly escaped string chunk. The surrounding quotes are *not*
 * printed; this is so that the caller has the option of emitting the string
 * content in chunks. */
static void putstring(upb_json_printer *p, const char *buf, unsigned int len) {
  const char* unescaped_run = NULL;
  unsigned int i;
  for (i = 0; i < len; i++) {
    char c = buf[i];
    /* Handle escaping. */
    if (is_json_escaped(c)) {
      /* Use a "nice" escape, like \n, if one exists for this character. */
      const char* escape = json_nice_escape(c);
      /* If we don't have a specific 'nice' escape code, use a \uXXXX-style
       * escape. */
      char escape_buf[8];
      if (!escape) {
        unsigned char byte = (unsigned char)c;
        _upb_snprintf(escape_buf, sizeof(escape_buf), "\\u%04x", (int)byte);
        escape = escape_buf;
      }

      /* N.B. that we assume that the input encoding is equal to the output
       * encoding (both UTF-8 for  now), so for chars >= 0x20 and != \, ", we
       * can simply pass the bytes through. */

      /* If there's a current run of unescaped chars, print that run first. */
      if (unescaped_run) {
        print_data(p, unescaped_run, &buf[i] - unescaped_run);
        unescaped_run = NULL;
      }
      /* Then print the escape code. */
      print_data(p, escape, strlen(escape));
    } else {
      /* Add to the current unescaped run of characters. */
      if (unescaped_run == NULL) {
        unescaped_run = &buf[i];
      }
    }
  }

  /* If the string ended in a run of unescaped characters, print that last run. */
  if (unescaped_run) {
    print_data(p, unescaped_run, &buf[len] - unescaped_run);
  }
}

#define CHKLENGTH(x) if (!(x)) return -1;

/* Helpers that format floating point values according to our custom formats.
 * Right now we use %.8g and %.17g for float/double, respectively, to match
 * proto2::util::JsonFormat's defaults.  May want to change this later. */

static size_t fmt_double(double val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%.17g", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_float(float val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%.8g", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_bool(bool val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%s", (val ? "true" : "false"));
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_int64(long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%ld", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_uint64(unsigned long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%llu", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

/* Print a map key given a field name. Called by scalar field handlers and by
 * startseq for repeated fields. */
static bool putkey(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  const strpc *key = handler_data;
  print_comma(p);
  print_data(p, "\"", 1);
  putstring(p, key->ptr, key->len);
  print_data(p, "\":", 2);
  return true;
}

#define CHKFMT(val) if ((val) == (size_t)-1) return false;
#define CHK(val)    if (!(val)) return false;

#define TYPE_HANDLERS(type, fmt_func)                                        \
  static bool put##type(void *closure, const void *handler_data, type val) { \
    upb_json_printer *p = closure;                                           \
    char data[64];                                                           \
    size_t length = fmt_func(val, data, sizeof(data));                       \
    UPB_UNUSED(handler_data);                                                \
    CHKFMT(length);                                                          \
    print_data(p, data, length);                                             \
    return true;                                                             \
  }                                                                          \
  static bool scalar_##type(void *closure, const void *handler_data,         \
                            type val) {                                      \
    CHK(putkey(closure, handler_data));                                      \
    CHK(put##type(closure, handler_data, val));                              \
    return true;                                                             \
  }                                                                          \
  static bool repeated_##type(void *closure, const void *handler_data,       \
                              type val) {                                    \
    upb_json_printer *p = closure;                                           \
    print_comma(p);                                                          \
    CHK(put##type(closure, handler_data, val));                              \
    return true;                                                             \
  }

#define TYPE_HANDLERS_MAPKEY(type, fmt_func)                                 \
  static bool putmapkey_##type(void *closure, const void *handler_data,      \
                            type val) {                                      \
    upb_json_printer *p = closure;                                           \
    print_data(p, "\"", 1);                                                  \
    CHK(put##type(closure, handler_data, val));                              \
    print_data(p, "\":", 2);                                                 \
    return true;                                                             \
  }

TYPE_HANDLERS(double,   fmt_double)
TYPE_HANDLERS(float,    fmt_float)
TYPE_HANDLERS(bool,     fmt_bool)
TYPE_HANDLERS(int32_t,  fmt_int64)
TYPE_HANDLERS(uint32_t, fmt_int64)
TYPE_HANDLERS(int64_t,  fmt_int64)
TYPE_HANDLERS(uint64_t, fmt_uint64)

/* double and float are not allowed to be map keys. */
TYPE_HANDLERS_MAPKEY(bool,     fmt_bool)
TYPE_HANDLERS_MAPKEY(int32_t,  fmt_int64)
TYPE_HANDLERS_MAPKEY(uint32_t, fmt_int64)
TYPE_HANDLERS_MAPKEY(int64_t,  fmt_int64)
TYPE_HANDLERS_MAPKEY(uint64_t, fmt_uint64)

#undef TYPE_HANDLERS
#undef TYPE_HANDLERS_MAPKEY

typedef struct {
  void *keyname;
  const upb_enumdef *enumdef;
} EnumHandlerData;

static bool scalar_enum(void *closure, const void *handler_data,
                        int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;
  const char *symbolic_name;

  CHK(putkey(closure, hd->keyname));

  symbolic_name = upb_enumdef_iton(hd->enumdef, val);
  if (symbolic_name) {
    print_data(p, "\"", 1);
    putstring(p, symbolic_name, strlen(symbolic_name));
    print_data(p, "\"", 1);
  } else {
    putint32_t(closure, NULL, val);
  }

  return true;
}

static void print_enum_symbolic_name(upb_json_printer *p,
                                     const upb_enumdef *def,
                                     int32_t val) {
  const char *symbolic_name = upb_enumdef_iton(def, val);
  if (symbolic_name) {
    print_data(p, "\"", 1);
    putstring(p, symbolic_name, strlen(symbolic_name));
    print_data(p, "\"", 1);
  } else {
    putint32_t(p, NULL, val);
  }
}

static bool repeated_enum(void *closure, const void *handler_data,
                          int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;
  print_comma(p);

  print_enum_symbolic_name(p, hd->enumdef, val);

  return true;
}

static bool mapvalue_enum(void *closure, const void *handler_data,
                          int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;

  print_enum_symbolic_name(p, hd->enumdef, val);

  return true;
}

static void *scalar_startsubmsg(void *closure, const void *handler_data) {
  return putkey(closure, handler_data) ? closure : UPB_BREAK;
}

static void *repeated_startsubmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_comma(p);
  return closure;
}

static void start_frame(upb_json_printer *p) {
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
}

static void end_frame(upb_json_printer *p) {
  print_data(p, "}", 1);
  p->depth_--;
}

static bool printer_startmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  start_frame(p);
  return true;
}

static bool printer_endmsg(void *closure, const void *handler_data, upb_status *s) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  end_frame(p);
  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

static void *startseq(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  CHK(putkey(closure, handler_data));
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "[", 1);
  return closure;
}

static bool endseq(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "]", 1);
  p->depth_--;
  return true;
}

static void *startmap(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  CHK(putkey(closure, handler_data));
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
  return closure;
}

static bool endmap(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "}", 1);
  p->depth_--;
  return true;
}

static size_t putstr(void *closure, const void *handler_data, const char *str,
                     size_t len, const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);
  putstring(p, str, len);
  return len;
}

/* This has to Base64 encode the bytes, because JSON has no "bytes" type. */
static size_t putbytes(void *closure, const void *handler_data, const char *str,
                       size_t len, const upb_bufhandle *handle) {
  upb_json_printer *p = closure;

  /* This is the regular base64, not the "web-safe" version. */
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  /* Base64-encode. */
  char data[16000];
  const char *limit = data + sizeof(data);
  const unsigned char *from = (const unsigned char*)str;
  char *to = data;
  size_t remaining = len;
  size_t bytes;

  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);

  while (remaining > 2) {
    /* TODO(haberman): handle encoded lengths > sizeof(data) */
    UPB_ASSERT_VAR(limit, (limit - to) >= 4);

    to[0] = base64[from[0] >> 2];
    to[1] = base64[((from[0] & 0x3) << 4) | (from[1] >> 4)];
    to[2] = base64[((from[1] & 0xf) << 2) | (from[2] >> 6)];
    to[3] = base64[from[2] & 0x3f];

    remaining -= 3;
    to += 4;
    from += 3;
  }

  switch (remaining) {
    case 2:
      to[0] = base64[from[0] >> 2];
      to[1] = base64[((from[0] & 0x3) << 4) | (from[1] >> 4)];
      to[2] = base64[(from[1] & 0xf) << 2];
      to[3] = '=';
      to += 4;
      from += 2;
      break;
    case 1:
      to[0] = base64[from[0] >> 2];
      to[1] = base64[((from[0] & 0x3) << 4)];
      to[2] = '=';
      to[3] = '=';
      to += 4;
      from += 1;
      break;
  }

  bytes = to - data;
  print_data(p, "\"", 1);
  putstring(p, data, bytes);
  print_data(p, "\"", 1);
  return len;
}

static void *scalar_startstr(void *closure, const void *handler_data,
                             size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  CHK(putkey(closure, handler_data));
  print_data(p, "\"", 1);
  return p;
}

static size_t scalar_str(void *closure, const void *handler_data,
                         const char *str, size_t len,
                         const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool scalar_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
}

static void *repeated_startstr(void *closure, const void *handler_data,
                               size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_comma(p);
  print_data(p, "\"", 1);
  return p;
}

static size_t repeated_str(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool repeated_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
}

static void *mapkeyval_startstr(void *closure, const void *handler_data,
                                size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_data(p, "\"", 1);
  return p;
}

static size_t mapkey_str(void *closure, const void *handler_data,
                         const char *str, size_t len,
                         const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool mapkey_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\":", 2);
  return true;
}

static bool mapvalue_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
}

static size_t scalar_bytes(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  CHK(putkey(closure, handler_data));
  CHK(putbytes(closure, handler_data, str, len, handle));
  return len;
}

static size_t repeated_bytes(void *closure, const void *handler_data,
                             const char *str, size_t len,
                             const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  print_comma(p);
  CHK(putbytes(closure, handler_data, str, len, handle));
  return len;
}

static size_t mapkey_bytes(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  CHK(putbytes(closure, handler_data, str, len, handle));
  print_data(p, ":", 1);
  return len;
}

static void set_enum_hd(upb_handlers *h,
                        const upb_fielddef *f,
                        upb_handlerattr *attr) {
  EnumHandlerData *hd = malloc(sizeof(EnumHandlerData));
  hd->enumdef = (const upb_enumdef *)upb_fielddef_subdef(f);
  hd->keyname = newstrpc(h, f);
  upb_handlers_addcleanup(h, hd, free);
  upb_handlerattr_sethandlerdata(attr, hd);
}

/* Set up handlers for a mapentry submessage (i.e., an individual key/value pair
 * in a map).
 *
 * TODO: Handle missing key, missing value, out-of-order key/value, or repeated
 * key or value cases properly. The right way to do this is to allocate a
 * temporary structure at the start of a mapentry submessage, store key and
 * value data in it as key and value handlers are called, and then print the
 * key/value pair once at the end of the submessage. If we don't do this, we
 * should at least detect the case and throw an error. However, so far all of
 * our sources that emit mapentry messages do so canonically (with one key
 * field, and then one value field), so this is not a pressing concern at the
 * moment. */
void printer_sethandlers_mapentry(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  /* A mapentry message is printed simply as '"key": value'. Rather than
   * special-case key and value for every type below, we just handle both
   * fields explicitly here. */
  const upb_fielddef* key_field = upb_msgdef_itof(md, UPB_MAPENTRY_KEY);
  const upb_fielddef* value_field = upb_msgdef_itof(md, UPB_MAPENTRY_VALUE);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INITIALIZER;

  UPB_UNUSED(closure);

  switch (upb_fielddef_type(key_field)) {
    case UPB_TYPE_INT32:
      upb_handlers_setint32(h, key_field, putmapkey_int32_t, &empty_attr);
      break;
    case UPB_TYPE_INT64:
      upb_handlers_setint64(h, key_field, putmapkey_int64_t, &empty_attr);
      break;
    case UPB_TYPE_UINT32:
      upb_handlers_setuint32(h, key_field, putmapkey_uint32_t, &empty_attr);
      break;
    case UPB_TYPE_UINT64:
      upb_handlers_setuint64(h, key_field, putmapkey_uint64_t, &empty_attr);
      break;
    case UPB_TYPE_BOOL:
      upb_handlers_setbool(h, key_field, putmapkey_bool, &empty_attr);
      break;
    case UPB_TYPE_STRING:
      upb_handlers_setstartstr(h, key_field, mapkeyval_startstr, &empty_attr);
      upb_handlers_setstring(h, key_field, mapkey_str, &empty_attr);
      upb_handlers_setendstr(h, key_field, mapkey_endstr, &empty_attr);
      break;
    case UPB_TYPE_BYTES:
      upb_handlers_setstring(h, key_field, mapkey_bytes, &empty_attr);
      break;
    default:
      assert(false);
      break;
  }

  switch (upb_fielddef_type(value_field)) {
    case UPB_TYPE_INT32:
      upb_handlers_setint32(h, value_field, putint32_t, &empty_attr);
      break;
    case UPB_TYPE_INT64:
      upb_handlers_setint64(h, value_field, putint64_t, &empty_attr);
      break;
    case UPB_TYPE_UINT32:
      upb_handlers_setuint32(h, value_field, putuint32_t, &empty_attr);
      break;
    case UPB_TYPE_UINT64:
      upb_handlers_setuint64(h, value_field, putuint64_t, &empty_attr);
      break;
    case UPB_TYPE_BOOL:
      upb_handlers_setbool(h, value_field, putbool, &empty_attr);
      break;
    case UPB_TYPE_FLOAT:
      upb_handlers_setfloat(h, value_field, putfloat, &empty_attr);
      break;
    case UPB_TYPE_DOUBLE:
      upb_handlers_setdouble(h, value_field, putdouble, &empty_attr);
      break;
    case UPB_TYPE_STRING:
      upb_handlers_setstartstr(h, value_field, mapkeyval_startstr, &empty_attr);
      upb_handlers_setstring(h, value_field, putstr, &empty_attr);
      upb_handlers_setendstr(h, value_field, mapvalue_endstr, &empty_attr);
      break;
    case UPB_TYPE_BYTES:
      upb_handlers_setstring(h, value_field, putbytes, &empty_attr);
      break;
    case UPB_TYPE_ENUM: {
      upb_handlerattr enum_attr = UPB_HANDLERATTR_INITIALIZER;
      set_enum_hd(h, value_field, &enum_attr);
      upb_handlers_setint32(h, value_field, mapvalue_enum, &enum_attr);
      upb_handlerattr_uninit(&enum_attr);
      break;
    }
    case UPB_TYPE_MESSAGE:
      /* No handler necessary -- the submsg handlers will print the message
       * as appropriate. */
      break;
  }

  upb_handlerattr_uninit(&empty_attr);
}

void printer_sethandlers(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  bool is_mapentry = upb_msgdef_mapentry(md);
  upb_handlerattr empty_attr = UPB_HANDLERATTR_INITIALIZER;
  upb_msg_field_iter i;

  UPB_UNUSED(closure);

  if (is_mapentry) {
    /* mapentry messages are sufficiently different that we handle them
     * separately. */
    printer_sethandlers_mapentry(closure, h);
    return;
  }

  upb_handlers_setstartmsg(h, printer_startmsg, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg, &empty_attr);

#define TYPE(type, name, ctype)                                               \
  case type:                                                                  \
    if (upb_fielddef_isseq(f)) {                                              \
      upb_handlers_set##name(h, f, repeated_##ctype, &empty_attr);            \
    } else {                                                                  \
      upb_handlers_set##name(h, f, scalar_##ctype, &name_attr);               \
    }                                                                         \
    break;

  upb_msg_field_begin(&i, md);
  for(; !upb_msg_field_done(&i); upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);

    upb_handlerattr name_attr = UPB_HANDLERATTR_INITIALIZER;
    upb_handlerattr_sethandlerdata(&name_attr, newstrpc(h, f));

    if (upb_fielddef_ismap(f)) {
      upb_handlers_setstartseq(h, f, startmap, &name_attr);
      upb_handlers_setendseq(h, f, endmap, &name_attr);
    } else if (upb_fielddef_isseq(f)) {
      upb_handlers_setstartseq(h, f, startseq, &name_attr);
      upb_handlers_setendseq(h, f, endseq, &empty_attr);
    }

    switch (upb_fielddef_type(f)) {
      TYPE(UPB_TYPE_FLOAT,  float,  float);
      TYPE(UPB_TYPE_DOUBLE, double, double);
      TYPE(UPB_TYPE_BOOL,   bool,   bool);
      TYPE(UPB_TYPE_INT32,  int32,  int32_t);
      TYPE(UPB_TYPE_UINT32, uint32, uint32_t);
      TYPE(UPB_TYPE_INT64,  int64,  int64_t);
      TYPE(UPB_TYPE_UINT64, uint64, uint64_t);
      case UPB_TYPE_ENUM: {
        /* For now, we always emit symbolic names for enums. We may want an
         * option later to control this behavior, but we will wait for a real
         * need first. */
        upb_handlerattr enum_attr = UPB_HANDLERATTR_INITIALIZER;
        set_enum_hd(h, f, &enum_attr);

        if (upb_fielddef_isseq(f)) {
          upb_handlers_setint32(h, f, repeated_enum, &enum_attr);
        } else {
          upb_handlers_setint32(h, f, scalar_enum, &enum_attr);
        }

        upb_handlerattr_uninit(&enum_attr);
        break;
      }
      case UPB_TYPE_STRING:
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstartstr(h, f, repeated_startstr, &empty_attr);
          upb_handlers_setstring(h, f, repeated_str, &empty_attr);
          upb_handlers_setendstr(h, f, repeated_endstr, &empty_attr);
        } else {
          upb_handlers_setstartstr(h, f, scalar_startstr, &name_attr);
          upb_handlers_setstring(h, f, scalar_str, &empty_attr);
          upb_handlers_setendstr(h, f, scalar_endstr, &empty_attr);
        }
        break;
      case UPB_TYPE_BYTES:
        /* XXX: this doesn't support strings that span buffers yet. The base64
         * encoder will need to be made resumable for this to work properly. */
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstring(h, f, repeated_bytes, &empty_attr);
        } else {
          upb_handlers_setstring(h, f, scalar_bytes, &name_attr);
        }
        break;
      case UPB_TYPE_MESSAGE:
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstartsubmsg(h, f, repeated_startsubmsg, &name_attr);
        } else {
          upb_handlers_setstartsubmsg(h, f, scalar_startsubmsg, &name_attr);
        }
        break;
    }

    upb_handlerattr_uninit(&name_attr);
  }

  upb_handlerattr_uninit(&empty_attr);
#undef TYPE
}

static void json_printer_reset(upb_json_printer *p) {
  p->depth_ = 0;
}


/* Public API *****************************************************************/

upb_json_printer *upb_json_printer_create(upb_env *e, const upb_handlers *h,
                                          upb_bytessink *output) {
#ifndef NDEBUG
  size_t size_before = upb_env_bytesallocated(e);
#endif

  upb_json_printer *p = upb_env_malloc(e, sizeof(upb_json_printer));
  if (!p) return NULL;

  p->output_ = output;
  json_printer_reset(p);
  upb_sink_reset(&p->input_, h, p);

  /* If this fails, increase the value in printer.h. */
  assert(upb_env_bytesallocated(e) - size_before <= UPB_JSON_PRINTER_SIZE);
  return p;
}

upb_sink *upb_json_printer_input(upb_json_printer *p) {
  return &p->input_;
}

const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 const void *owner) {
  return upb_handlers_newfrozen(md, owner, printer_sethandlers, NULL);
}
