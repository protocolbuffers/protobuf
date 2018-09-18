
#include "upb/def.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "upb/structdefs.int.h"
#include "upb/handlers.h"

typedef struct {
  size_t len;
  char str[1];  /* Null-terminated string data follows. */
} str_t;

static str_t *newstr(const char *data, size_t len) {
  str_t *ret = upb_gmalloc(sizeof(*ret) + len);
  if (!ret) return NULL;
  ret->len = len;
  memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static void freestr(str_t *s) { upb_gfree(s); }

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

static bool upb_isoneof(const upb_refcounted *def) {
  return def->vtbl == &upb_oneofdef_vtbl;
}

static bool upb_isfield(const upb_refcounted *def) {
  return def->vtbl == &upb_fielddef_vtbl;
}

static const upb_oneofdef *upb_trygetoneof(const upb_refcounted *def) {
  return upb_isoneof(def) ? (const upb_oneofdef*)def : NULL;
}

static const upb_fielddef *upb_trygetfield(const upb_refcounted *def) {
  return upb_isfield(def) ? (const upb_fielddef*)def : NULL;
}


/* upb_def ********************************************************************/

upb_deftype_t upb_def_type(const upb_def *d) { return d->type; }

const char *upb_def_fullname(const upb_def *d) { return d->fullname; }

const char *upb_def_name(const upb_def *d) {
  const char *p;

  if (d->fullname == NULL) {
    return NULL;
  } else if ((p = strrchr(d->fullname, '.')) == NULL) {
    /* No '.' in the name, return the full string. */
    return d->fullname;
  } else {
    /* Return one past the last '.'. */
    return p + 1;
  }
}

bool upb_def_setfullname(upb_def *def, const char *fullname, upb_status *s) {
  UPB_ASSERT(!upb_def_isfrozen(def));
  if (!upb_isident(fullname, strlen(fullname), true, s)) {
    return false;
  }

  fullname = upb_gstrdup(fullname);
  if (!fullname) {
    upb_upberr_setoom(s);
    return false;
  }

  upb_gfree((void*)def->fullname);
  def->fullname = fullname;
  return true;
}

const upb_filedef *upb_def_file(const upb_def *d) { return d->file; }

static bool upb_def_init(upb_def *def, upb_deftype_t type,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
  if (!upb_refcounted_init(upb_def_upcast_mutable(def), vtbl, owner)) return false;
  def->type = type;
  def->fullname = NULL;
  def->came_from_user = false;
  def->file = NULL;
  return true;
}

static void upb_def_uninit(upb_def *def) {
  upb_gfree((void*)def->fullname);
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
    UPB_ASSERT(upb_enumdef_numvals(upb_fielddef_enumsubdef(f)) > 0);

    /* We've already validated that we have an associated enumdef and that it
     * has at least one member, so at least one of these should be true.
     * Because if the user didn't set anything, we'll pick up the enum's
     * default, but if the user *did* set something we should at least pick up
     * the one they set (int32 or string). */
    UPB_ASSERT(has_default_name || has_default_number);

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
  UPB_ASSERT(ret < high_bit);
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
  upb_msg_oneof_iter k;
  int i;
  uint32_t selector;
  int n = upb_msgdef_numfields(m);
  upb_fielddef **fields;

  if (n == 0) {
    m->selector_count = UPB_STATIC_SELECTOR_COUNT;
    m->submsg_field_count = 0;
    return true;
  }

  fields = upb_gmalloc(n * sizeof(*fields));
  if (!fields) {
    upb_upberr_setoom(s);
    return false;
  }

  m->submsg_field_count = 0;
  for(i = 0, upb_msg_field_begin(&j, m);
      !upb_msg_field_done(&j);
      upb_msg_field_next(&j), i++) {
    upb_fielddef *f = upb_msg_iter_field(&j);
    UPB_ASSERT(f->msg.def == m);
    if (!upb_validate_field(f, s)) {
      upb_gfree(fields);
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
    upb_inttable_insert(&t, UPB_UNKNOWN_SELECTOR, v);
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

  for(upb_msg_oneof_begin(&k, m), i = 0;
      !upb_msg_oneof_done(&k);
      upb_msg_oneof_next(&k), i++) {
    upb_oneofdef *o = upb_msg_iter_oneof(&k);
    o->index = i;
  }

  upb_gfree(fields);
  return true;
}

static void assign_msg_wellknowntype(upb_msgdef *m) {
  const char *name = upb_msgdef_fullname(m);
  if (name == NULL) {
    m->well_known_type = UPB_WELLKNOWN_UNSPECIFIED;
    return;
  }
  if (!strcmp(name, "google.protobuf.Duration")) {
    m->well_known_type = UPB_WELLKNOWN_DURATION;
  } else if (!strcmp(name, "google.protobuf.Timestamp")) {
    m->well_known_type = UPB_WELLKNOWN_TIMESTAMP;
  } else if (!strcmp(name, "google.protobuf.DoubleValue")) {
    m->well_known_type = UPB_WELLKNOWN_DOUBLEVALUE;
  } else if (!strcmp(name, "google.protobuf.FloatValue")) {
    m->well_known_type = UPB_WELLKNOWN_FLOATVALUE;
  } else if (!strcmp(name, "google.protobuf.Int64Value")) {
    m->well_known_type = UPB_WELLKNOWN_INT64VALUE;
  } else if (!strcmp(name, "google.protobuf.UInt64Value")) {
    m->well_known_type = UPB_WELLKNOWN_UINT64VALUE;
  } else if (!strcmp(name, "google.protobuf.Int32Value")) {
    m->well_known_type = UPB_WELLKNOWN_INT32VALUE;
  } else if (!strcmp(name, "google.protobuf.UInt32Value")) {
    m->well_known_type = UPB_WELLKNOWN_UINT32VALUE;
  } else if (!strcmp(name, "google.protobuf.BoolValue")) {
    m->well_known_type = UPB_WELLKNOWN_BOOLVALUE;
  } else if (!strcmp(name, "google.protobuf.StringValue")) {
    m->well_known_type = UPB_WELLKNOWN_STRINGVALUE;
  } else if (!strcmp(name, "google.protobuf.BytesValue")) {
    m->well_known_type = UPB_WELLKNOWN_BYTESVALUE;
  } else if (!strcmp(name, "google.protobuf.Value")) {
    m->well_known_type = UPB_WELLKNOWN_VALUE;
  } else if (!strcmp(name, "google.protobuf.ListValue")) {
    m->well_known_type = UPB_WELLKNOWN_LISTVALUE;
  } else if (!strcmp(name, "google.protobuf.Struct")) {
    m->well_known_type = UPB_WELLKNOWN_STRUCT;
  } else {
    m->well_known_type = UPB_WELLKNOWN_UNSPECIFIED;
  }
}

bool _upb_def_validate(upb_def *const*defs, size_t n, upb_status *s) {
  size_t i;

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
    } else {
      /* Set now to detect transitive closure in the second pass. */
      def->came_from_user = true;

      if (def->type == UPB_DEF_ENUM &&
          !upb_validate_enumdef(upb_dyncast_enumdef(def), s)) {
        goto err;
      }
    }
  }

  /* Second pass of validation.  Also assign selector bases and indexes, and
   * compact tables. */
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    upb_msgdef *m = upb_dyncast_msgdef_mutable(def);
    upb_enumdef *e = upb_dyncast_enumdef_mutable(def);
    if (m) {
      upb_inttable_compact(&m->itof);
      if (!assign_msg_indices(m, s)) {
        goto err;
      }
      assign_msg_wellknowntype(m);
      /* m->well_known_type = UPB_WELLKNOWN_UNSPECIFIED; */
    } else if (e) {
      upb_inttable_compact(&e->iton);
    }
  }

  return true;

err:
  for (i = 0; i < n; i++) {
    upb_def *def = defs[i];
    def->came_from_user = false;
  }
  UPB_ASSERT(!(s && upb_ok(s)));
  return false;
}

bool upb_def_freeze(upb_def *const* defs, size_t n, upb_status *s) {
  /* Def graph contains FieldDefs between each MessageDef, so double the
   * limit. */
  const size_t maxdepth = UPB_MAX_MESSAGE_DEPTH * 2;

  if (!_upb_def_validate(defs, n, s)) {
    return false;
  }


  /* Validation all passed; freeze the objects. */
  return upb_refcounted_freeze((upb_refcounted *const*)defs, n, s, maxdepth);
}


/* upb_enumdef ****************************************************************/

static void visitenum(const upb_refcounted *r, upb_refcounted_visit *visit,
                      void *closure) {
  const upb_enumdef *e = (const upb_enumdef*)r;
  const upb_def *def = upb_enumdef_upcast(e);
  if (upb_def_file(def)) {
    visit(r, upb_filedef_upcast(upb_def_file(def)), closure);
  }
}

static void freeenum(upb_refcounted *r) {
  upb_enumdef *e = (upb_enumdef*)r;
  upb_inttable_iter i;
  upb_inttable_begin(&i, &e->iton);
  for( ; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    /* To clean up the upb_gstrdup() from upb_enumdef_addval(). */
    upb_gfree(upb_value_getcstr(upb_inttable_iter_value(&i)));
  }
  upb_strtable_uninit(&e->ntoi);
  upb_inttable_uninit(&e->iton);
  upb_def_uninit(upb_enumdef_upcast_mutable(e));
  upb_gfree(e);
}

const struct upb_refcounted_vtbl upb_enumdef_vtbl = {&visitenum, &freeenum};

upb_enumdef *upb_enumdef_new(const void *owner) {
  upb_enumdef *e = upb_gmalloc(sizeof(*e));
  if (!e) return NULL;

  if (!upb_def_init(upb_enumdef_upcast_mutable(e), UPB_DEF_ENUM,
                    &upb_enumdef_vtbl, owner)) {
    goto err2;
  }

  if (!upb_strtable_init(&e->ntoi, UPB_CTYPE_INT32)) goto err2;
  if (!upb_inttable_init(&e->iton, UPB_CTYPE_CSTR)) goto err1;
  return e;

err1:
  upb_strtable_uninit(&e->ntoi);
err2:
  upb_gfree(e);
  return NULL;
}

bool upb_enumdef_freeze(upb_enumdef *e, upb_status *status) {
  upb_def *d = upb_enumdef_upcast_mutable(e);
  return upb_def_freeze(&d, 1, status);
}

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return upb_def_fullname(upb_enumdef_upcast(e));
}

const char *upb_enumdef_name(const upb_enumdef *e) {
  return upb_def_name(upb_enumdef_upcast(e));
}

bool upb_enumdef_setfullname(upb_enumdef *e, const char *fullname,
                             upb_status *s) {
  return upb_def_setfullname(upb_enumdef_upcast_mutable(e), fullname, s);
}

bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num,
                        upb_status *status) {
  char *name2;

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

  if (!upb_inttable_lookup(&e->iton, num, NULL)) {
    name2 = upb_gstrdup(name);
    if (!name2 || !upb_inttable_insert(&e->iton, num, upb_value_cstr(name2))) {
      upb_status_seterrmsg(status, "out of memory");
      upb_strtable_remove(&e->ntoi, name, NULL);
      return false;
    }
  }

  if (upb_enumdef_numvals(e) == 1) {
    bool ok = upb_enumdef_setdefault(e, num, NULL);
    UPB_ASSERT(ok);
  }

  return true;
}

int32_t upb_enumdef_default(const upb_enumdef *e) {
  UPB_ASSERT(upb_enumdef_iton(e, e->defaultval));
  return e->defaultval;
}

bool upb_enumdef_setdefault(upb_enumdef *e, int32_t val, upb_status *s) {
  UPB_ASSERT(!upb_enumdef_isfrozen(e));
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

const char *upb_fielddef_fullname(const upb_fielddef *e) {
  return upb_def_fullname(upb_fielddef_upcast(e));
}

static void visitfield(const upb_refcounted *r, upb_refcounted_visit *visit,
                       void *closure) {
  const upb_fielddef *f = (const upb_fielddef*)r;
  const upb_def *def = upb_fielddef_upcast(f);
  if (upb_fielddef_containingtype(f)) {
    visit(r, upb_msgdef_upcast2(upb_fielddef_containingtype(f)), closure);
  }
  if (upb_fielddef_containingoneof(f)) {
    visit(r, upb_oneofdef_upcast(upb_fielddef_containingoneof(f)), closure);
  }
  if (upb_fielddef_subdef(f)) {
    visit(r, upb_def_upcast(upb_fielddef_subdef(f)), closure);
  }
  if (upb_def_file(def)) {
    visit(r, upb_filedef_upcast(upb_def_file(def)), closure);
  }
}

static void freefield(upb_refcounted *r) {
  upb_fielddef *f = (upb_fielddef*)r;
  upb_fielddef_uninit_default(f);
  if (f->subdef_is_symbolic)
    upb_gfree(f->sub.name);
  upb_def_uninit(upb_fielddef_upcast_mutable(f));
  upb_gfree(f);
}

static const char *enumdefaultstr(const upb_fielddef *f) {
  const upb_enumdef *e;
  UPB_ASSERT(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
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
        UPB_ASSERT(name);
        return name;
      }
    }
  }
  return NULL;
}

static bool enumdefaultint32(const upb_fielddef *f, int32_t *val) {
  const upb_enumdef *e;
  UPB_ASSERT(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
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

const struct upb_refcounted_vtbl upb_fielddef_vtbl = {visitfield, freefield};

upb_fielddef *upb_fielddef_new(const void *o) {
  upb_fielddef *f = upb_gmalloc(sizeof(*f));
  if (!f) return NULL;
  if (!upb_def_init(upb_fielddef_upcast_mutable(f), UPB_DEF_FIELD,
                    &upb_fielddef_vtbl, o)) {
    upb_gfree(f);
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

bool upb_fielddef_typeisset(const upb_fielddef *f) {
  return f->type_is_set_;
}

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
  UPB_ASSERT(f->type_is_set_);
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

size_t upb_fielddef_getjsonname(const upb_fielddef *f, char *buf, size_t len) {
  const char *name = upb_fielddef_name(f);
  size_t src, dst = 0;
  bool ucase_next = false;

#define WRITE(byte) \
  ++dst; \
  if (dst < len) buf[dst - 1] = byte; \
  else if (dst == len) buf[dst - 1] = '\0'

  if (!name) {
    WRITE('\0');
    return 0;
  }

  /* Implement the transformation as described in the spec:
   *   1. upper case all letters after an underscore.
   *   2. remove all underscores.
   */
  for (src = 0; name[src]; src++) {
    if (name[src] == '_') {
      ucase_next = true;
      continue;
    }

    if (ucase_next) {
      WRITE(toupper(name[src]));
      ucase_next = false;
    } else {
      WRITE(name[src]);
    }
  }

  WRITE('\0');
  return dst;

#undef WRITE
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
  if (f->msg_is_symbolic) upb_gfree(f->msg.name);
}

bool upb_fielddef_setcontainingtypename(upb_fielddef *f, const char *name,
                                        upb_status *s) {
  char *name_copy;
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  if (upb_fielddef_containingtype(f)) {
    upb_status_seterrmsg(s, "field has already been added to a message.");
    return false;
  }
  /* TODO: validate name (upb_isident() doesn't quite work atm because this name
   * may have a leading "."). */

  name_copy = upb_gstrdup(name);
  if (!name_copy) {
    upb_upberr_setoom(s);
    return false;
  }

  release_containingtype(f);
  f->msg.name = name_copy;
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
  UPB_ASSERT(f->type_is_set_ && upb_fielddef_type(f) == type);
}

int64_t upb_fielddef_defaultint64(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_INT64);
  return f->defaultval.sint;
}

int32_t upb_fielddef_defaultint32(const upb_fielddef *f) {
  if (f->type_is_set_ && upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    int32_t val;
    bool ok = enumdefaultint32(f, &val);
    UPB_ASSERT(ok);
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
  UPB_ASSERT(f->type_is_set_);
  UPB_ASSERT(upb_fielddef_type(f) == UPB_TYPE_STRING ||
         upb_fielddef_type(f) == UPB_TYPE_BYTES ||
         upb_fielddef_type(f) == UPB_TYPE_ENUM);

  if (upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    const char *ret = enumdefaultstr(f);
    UPB_ASSERT(ret);
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
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  UPB_ASSERT(upb_fielddef_checktype(type));
  upb_fielddef_uninit_default(f);
  f->type_ = type;
  f->type_is_set_ = true;
  upb_fielddef_init_default(f);
}

void upb_fielddef_setdescriptortype(upb_fielddef *f, int type) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
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
    default: UPB_ASSERT(false);
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
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  f->is_extension_ = is_extension;
}

void upb_fielddef_setlazy(upb_fielddef *f, bool lazy) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  f->lazy_ = lazy;
}

void upb_fielddef_setpacked(upb_fielddef *f, bool packed) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  f->packed_ = packed;
}

void upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  UPB_ASSERT(upb_fielddef_checklabel(label));
  f->label_ = label;
}

void upb_fielddef_setintfmt(upb_fielddef *f, upb_intfmt_t fmt) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  UPB_ASSERT(upb_fielddef_checkintfmt(fmt));
  f->intfmt = fmt;
}

void upb_fielddef_settagdelim(upb_fielddef *f, bool tag_delim) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  f->tagdelim = tag_delim;
  f->tagdelim = tag_delim;
}

static bool checksetdefault(upb_fielddef *f, upb_fieldtype_t type) {
  if (!f->type_is_set_ || upb_fielddef_isfrozen(f) ||
      upb_fielddef_type(f) != type) {
    UPB_ASSERT(false);
    return false;
  }
  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    UPB_ASSERT(s || type == UPB_TYPE_ENUM);
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
  UPB_ASSERT(upb_fielddef_isstring(f) || f->type_ == UPB_TYPE_ENUM);
  if (f->type_ == UPB_TYPE_ENUM && !upb_isident(str, len, false, s))
    return false;

  if (f->default_is_string) {
    str_t *s = f->defaultval.bytes;
    UPB_ASSERT(s || f->type_ == UPB_TYPE_ENUM);
    if (s) freestr(s);
  } else {
    UPB_ASSERT(f->type_ == UPB_TYPE_ENUM);
  }

  str2 = newstr(str, len);
  f->defaultval.bytes = str2;
  f->default_is_string = true;
  return true;
}

void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str,
                                 upb_status *s) {
  UPB_ASSERT(f->type_is_set_);
  upb_fielddef_setdefaultstr(f, str, str ? strlen(str) : 0, s);
}

bool upb_fielddef_enumhasdefaultint32(const upb_fielddef *f) {
  int32_t val;
  UPB_ASSERT(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
  return enumdefaultint32(f, &val);
}

bool upb_fielddef_enumhasdefaultstr(const upb_fielddef *f) {
  UPB_ASSERT(f->type_is_set_ && f->type_ == UPB_TYPE_ENUM);
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
    upb_gfree(f->sub.name);
  } else if (f->sub.def) {
    upb_unref2(f->sub.def, f);
  }
}

bool upb_fielddef_setsubdef(upb_fielddef *f, const upb_def *subdef,
                            upb_status *s) {
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  UPB_ASSERT(upb_fielddef_hassubdef(f));
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
  char *name_copy;
  UPB_ASSERT(!upb_fielddef_isfrozen(f));
  if (!upb_fielddef_hassubdef(f)) {
    upb_status_seterrmsg(s, "field type does not accept a subdef");
    return false;
  }

  name_copy = upb_gstrdup(name);
  if (!name_copy) {
    upb_upberr_setoom(s);
    return false;
  }

  /* TODO: validate name (upb_isident() doesn't quite work atm because this name
   * may have a leading "."). */
  release_subdef(f);
  f->sub.name = name_copy;
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

bool upb_fielddef_haspresence(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) return false;
  if (upb_fielddef_issubmsg(f)) return true;

  /* Primitive field: return true unless there is a message that specifies
   * presence should not exist. */
  if (f->msg_is_symbolic || !f->msg.def) return true;
  return f->msg.def->syntax == UPB_SYNTAX_PROTO2;
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
  const upb_def *def = upb_msgdef_upcast(m);
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
    visit(r, upb_oneofdef_upcast(f), closure);
  }
  if (upb_def_file(def)) {
    visit(r, upb_filedef_upcast(upb_def_file(def)), closure);
  }
}

static void freemsg(upb_refcounted *r) {
  upb_msgdef *m = (upb_msgdef*)r;
  upb_strtable_uninit(&m->ntof);
  upb_inttable_uninit(&m->itof);
  upb_def_uninit(upb_msgdef_upcast_mutable(m));
  upb_gfree(m);
}

const struct upb_refcounted_vtbl upb_msgdef_vtbl = {visitmsg, freemsg};

upb_msgdef *upb_msgdef_new(const void *owner) {
  upb_msgdef *m = upb_gmalloc(sizeof(*m));
  if (!m) return NULL;

  if (!upb_def_init(upb_msgdef_upcast_mutable(m), UPB_DEF_MSG, &upb_msgdef_vtbl,
                    owner)) {
    goto err2;
  }

  if (!upb_inttable_init(&m->itof, UPB_CTYPE_PTR)) goto err2;
  if (!upb_strtable_init(&m->ntof, UPB_CTYPE_PTR)) goto err1;
  m->map_entry = false;
  m->syntax = UPB_SYNTAX_PROTO2;
  return m;

err1:
  upb_inttable_uninit(&m->itof);
err2:
  upb_gfree(m);
  return NULL;
}

bool upb_msgdef_freeze(upb_msgdef *m, upb_status *status) {
  upb_def *d = upb_msgdef_upcast_mutable(m);
  return upb_def_freeze(&d, 1, status);
}

const char *upb_msgdef_fullname(const upb_msgdef *m) {
  return upb_def_fullname(upb_msgdef_upcast(m));
}

const char *upb_msgdef_name(const upb_msgdef *m) {
  return upb_def_name(upb_msgdef_upcast(m));
}

bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname,
                            upb_status *s) {
  return upb_def_setfullname(upb_msgdef_upcast_mutable(m), fullname, s);
}

bool upb_msgdef_setsyntax(upb_msgdef *m, upb_syntax_t syntax) {
  if (syntax != UPB_SYNTAX_PROTO2 && syntax != UPB_SYNTAX_PROTO3) {
    return false;
  }

  m->syntax = syntax;
  return true;
}

upb_syntax_t upb_msgdef_syntax(const upb_msgdef *m) {
  return m->syntax;
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
  } else if (upb_msgdef_itof(m, upb_fielddef_number(f))) {
    upb_status_seterrmsg(s, "duplicate field number");
    return false;
  } else if (upb_strtable_lookup(&m->ntof, upb_fielddef_name(f), NULL)) {
    upb_status_seterrmsg(s, "name conflicts with existing field or oneof");
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
    if (ref_donor) upb_fielddef_unref(f, ref_donor);
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
  } else if (upb_strtable_lookup(&m->ntof, upb_oneofdef_name(o), NULL)) {
    upb_status_seterrmsg(s, "name conflicts with existing field or oneof");
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
  upb_strtable_insert(&m->ntof, upb_oneofdef_name(o), upb_value_ptr(o));
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

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return upb_trygetfield(upb_value_getptr(val));
}

const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return upb_trygetoneof(upb_value_getptr(val));
}

bool upb_msgdef_lookupname(const upb_msgdef *m, const char *name, size_t len,
                           const upb_fielddef **f, const upb_oneofdef **o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  *o = upb_trygetoneof(upb_value_getptr(val));
  *f = upb_trygetfield(upb_value_getptr(val));
  UPB_ASSERT((*o != NULL) ^ (*f != NULL));  /* Exactly one of the two should be set. */
  return true;
}

int upb_msgdef_numfields(const upb_msgdef *m) {
  /* The number table contains only fields. */
  return upb_inttable_count(&m->itof);
}

int upb_msgdef_numoneofs(const upb_msgdef *m) {
  /* The name table includes oneofs, and the number table does not. */
  return upb_strtable_count(&m->ntof) - upb_inttable_count(&m->itof);
}

void upb_msgdef_setmapentry(upb_msgdef *m, bool map_entry) {
  UPB_ASSERT(!upb_msgdef_isfrozen(m));
  m->map_entry = map_entry;
}

bool upb_msgdef_mapentry(const upb_msgdef *m) {
  return m->map_entry;
}

upb_wellknowntype_t upb_msgdef_wellknowntype(const upb_msgdef *m) {
  return m->well_known_type;
}

bool upb_msgdef_isnumberwrapper(const upb_msgdef *m) {
  upb_wellknowntype_t type = upb_msgdef_wellknowntype(m);
  return type >= UPB_WELLKNOWN_DOUBLEVALUE &&
         type <= UPB_WELLKNOWN_UINT32VALUE;
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
  upb_strtable_begin(iter, &m->ntof);
  /* We need to skip past any initial fields. */
  while (!upb_strtable_done(iter) &&
         !upb_isoneof(upb_value_getptr(upb_strtable_iter_value(iter)))) {
    upb_strtable_next(iter);
  }
}

void upb_msg_oneof_next(upb_msg_oneof_iter *iter) {
  /* We need to skip past fields to return only oneofs. */
  do {
    upb_strtable_next(iter);
  } while (!upb_strtable_done(iter) &&
           !upb_isoneof(upb_value_getptr(upb_strtable_iter_value(iter))));
}

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
  upb_gfree((void*)o->name);
  upb_gfree(o);
}

const struct upb_refcounted_vtbl upb_oneofdef_vtbl = {visitoneof, freeoneof};

upb_oneofdef *upb_oneofdef_new(const void *owner) {
  upb_oneofdef *o = upb_gmalloc(sizeof(*o));

  if (!o) {
    return NULL;
  }

  o->parent = NULL;
  o->name = NULL;

  if (!upb_refcounted_init(upb_oneofdef_upcast_mutable(o), &upb_oneofdef_vtbl,
                           owner)) {
    goto err2;
  }

  if (!upb_inttable_init(&o->itof, UPB_CTYPE_PTR)) goto err2;
  if (!upb_strtable_init(&o->ntof, UPB_CTYPE_PTR)) goto err1;

  return o;

err1:
  upb_inttable_uninit(&o->itof);
err2:
  upb_gfree(o);
  return NULL;
}

const char *upb_oneofdef_name(const upb_oneofdef *o) { return o->name; }

bool upb_oneofdef_setname(upb_oneofdef *o, const char *name, upb_status *s) {
  UPB_ASSERT(!upb_oneofdef_isfrozen(o));
  if (upb_oneofdef_containingtype(o)) {
    upb_status_seterrmsg(s, "oneof already added to a message");
    return false;
  }

  if (!upb_isident(name, strlen(name), true, s)) {
    return false;
  }

  name = upb_gstrdup(name);
  if (!name) {
    upb_status_seterrmsg(s, "One of memory");
    return false;
  }

  upb_gfree((void*)o->name);
  o->name = name;
  return true;
}

const upb_msgdef *upb_oneofdef_containingtype(const upb_oneofdef *o) {
  return o->parent;
}

int upb_oneofdef_numfields(const upb_oneofdef *o) {
  return upb_strtable_count(&o->ntof);
}

uint32_t upb_oneofdef_index(const upb_oneofdef *o) {
  return o->index;
}

bool upb_oneofdef_addfield(upb_oneofdef *o, upb_fielddef *f,
                           const void *ref_donor,
                           upb_status *s) {
  UPB_ASSERT(!upb_oneofdef_isfrozen(o));
  UPB_ASSERT(!o->parent || !upb_msgdef_isfrozen(o->parent));

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

/* upb_filedef ****************************************************************/

static void visitfiledef(const upb_refcounted *r, upb_refcounted_visit *visit,
                         void *closure) {
  const upb_filedef *f = (const upb_filedef*)r;
  size_t i;

  for(i = 0; i < upb_filedef_defcount(f); i++) {
    visit(r, upb_def_upcast(upb_filedef_def(f, i)), closure);
  }
}

static void freefiledef(upb_refcounted *r) {
  upb_filedef *f = (upb_filedef*)r;
  size_t i;

  for(i = 0; i < upb_filedef_depcount(f); i++) {
    upb_filedef_unref(upb_filedef_dep(f, i), f);
  }

  upb_inttable_uninit(&f->defs);
  upb_inttable_uninit(&f->deps);
  upb_gfree((void*)f->name);
  upb_gfree((void*)f->package);
  upb_gfree((void*)f->phpprefix);
  upb_gfree((void*)f->phpnamespace);
  upb_gfree(f);
}

const struct upb_refcounted_vtbl upb_filedef_vtbl = {visitfiledef, freefiledef};

upb_filedef *upb_filedef_new(const void *owner) {
  upb_filedef *f = upb_gmalloc(sizeof(*f));

  if (!f) {
    return NULL;
  }

  f->package = NULL;
  f->name = NULL;
  f->phpprefix = NULL;
  f->phpnamespace = NULL;
  f->syntax = UPB_SYNTAX_PROTO2;

  if (!upb_refcounted_init(upb_filedef_upcast_mutable(f), &upb_filedef_vtbl,
                           owner)) {
    goto err;
  }

  if (!upb_inttable_init(&f->defs, UPB_CTYPE_CONSTPTR)) {
    goto err;
  }

  if (!upb_inttable_init(&f->deps, UPB_CTYPE_CONSTPTR)) {
    goto err2;
  }

  return f;


err2:
  upb_inttable_uninit(&f->defs);

err:
  upb_gfree(f);
  return NULL;
}

const char *upb_filedef_name(const upb_filedef *f) {
  return f->name;
}

const char *upb_filedef_package(const upb_filedef *f) {
  return f->package;
}

const char *upb_filedef_phpprefix(const upb_filedef *f) {
  return f->phpprefix;
}

const char *upb_filedef_phpnamespace(const upb_filedef *f) {
  return f->phpnamespace;
}

upb_syntax_t upb_filedef_syntax(const upb_filedef *f) {
  return f->syntax;
}

size_t upb_filedef_defcount(const upb_filedef *f) {
  return upb_inttable_count(&f->defs);
}

size_t upb_filedef_depcount(const upb_filedef *f) {
  return upb_inttable_count(&f->deps);
}

const upb_def *upb_filedef_def(const upb_filedef *f, size_t i) {
  upb_value v;

  if (upb_inttable_lookup32(&f->defs, i, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}

const upb_filedef *upb_filedef_dep(const upb_filedef *f, size_t i) {
  upb_value v;

  if (upb_inttable_lookup32(&f->deps, i, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}

bool upb_filedef_setname(upb_filedef *f, const char *name, upb_status *s) {
  name = upb_gstrdup(name);
  if (!name) {
    upb_upberr_setoom(s);
    return false;
  }
  upb_gfree((void*)f->name);
  f->name = name;
  return true;
}

bool upb_filedef_setpackage(upb_filedef *f, const char *package,
                            upb_status *s) {
  if (!upb_isident(package, strlen(package), true, s)) return false;
  package = upb_gstrdup(package);
  if (!package) {
    upb_upberr_setoom(s);
    return false;
  }
  upb_gfree((void*)f->package);
  f->package = package;
  return true;
}

bool upb_filedef_setphpprefix(upb_filedef *f, const char *phpprefix,
                              upb_status *s) {
  phpprefix = upb_gstrdup(phpprefix);
  if (!phpprefix) {
    upb_upberr_setoom(s);
    return false;
  }
  upb_gfree((void*)f->phpprefix);
  f->phpprefix = phpprefix;
  return true;
}

bool upb_filedef_setphpnamespace(upb_filedef *f, const char *phpnamespace,
                                 upb_status *s) {
  phpnamespace = upb_gstrdup(phpnamespace);
  if (!phpnamespace) {
    upb_upberr_setoom(s);
    return false;
  }
  upb_gfree((void*)f->phpnamespace);
  f->phpnamespace = phpnamespace;
  return true;
}

bool upb_filedef_setsyntax(upb_filedef *f, upb_syntax_t syntax,
                           upb_status *s) {
  UPB_UNUSED(s);
  if (syntax != UPB_SYNTAX_PROTO2 &&
      syntax != UPB_SYNTAX_PROTO3) {
    upb_status_seterrmsg(s, "Unknown syntax value.");
    return false;
  }
  f->syntax = syntax;

  {
    /* Set all messages in this file to match. */
    size_t i;
    for (i = 0; i < upb_filedef_defcount(f); i++) {
      /* Casting const away is safe since all defs in mutable filedef must
       * also be mutable. */
      upb_def *def = (upb_def*)upb_filedef_def(f, i);

      upb_msgdef *m = upb_dyncast_msgdef_mutable(def);
      if (m) {
        m->syntax = syntax;
      }
    }
  }

  return true;
}

bool upb_filedef_adddef(upb_filedef *f, upb_def *def, const void *ref_donor,
                        upb_status *s) {
  if (def->file) {
    upb_status_seterrmsg(s, "Def is already part of another filedef.");
    return false;
  }

  if (upb_inttable_push(&f->defs, upb_value_constptr(def))) {
    def->file = f;
    upb_ref2(def, f);
    upb_ref2(f, def);
    if (ref_donor) upb_def_unref(def, ref_donor);
    if (def->type == UPB_DEF_MSG) {
      upb_downcast_msgdef_mutable(def)->syntax = f->syntax;
    }
    return true;
  } else {
    upb_upberr_setoom(s);
    return false;
  }
}

bool upb_filedef_adddep(upb_filedef *f, const upb_filedef *dep) {
  if (upb_inttable_push(&f->deps, upb_value_constptr(dep))) {
    /* Regular ref instead of ref2 because files can't form cycles. */
    upb_filedef_ref(dep, f);
    return true;
  } else {
    return false;
  }
}

void upb_symtab_free(upb_symtab *s) {
  upb_strtable_iter i;
  upb_strtable_begin(&i, &s->symtab);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    const upb_def *def = upb_value_getptr(upb_strtable_iter_value(&i));
    upb_def_unref(def, s);
  }
  upb_strtable_uninit(&s->symtab);
  upb_gfree(s);
}

upb_symtab *upb_symtab_new() {
  upb_symtab *s = upb_gmalloc(sizeof(*s));
  if (!s) {
    return NULL;
  }

  upb_strtable_init(&s->symtab, UPB_CTYPE_PTR);
  return s;
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
    UPB_ASSERT(false);
    return NULL;
  }
}

const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym) {
  upb_def *ret = upb_resolvename(&s->symtab, base, sym);
  return ret;
}

/* TODO(haberman): we need a lot more testing of error conditions. */
static bool symtab_add(upb_symtab *s, upb_def *const*defs, size_t n,
                       void *ref_donor, upb_refcounted *freeze_also,
                       upb_status *status) {
  size_t i;
  size_t add_n;
  size_t freeze_n;
  upb_strtable_iter iter;
  upb_refcounted **add_objs = NULL;
  upb_def **add_defs = NULL;
  size_t add_objs_size;
  upb_strtable addtab;

  if (n == 0 && !freeze_also) {
    return true;
  }

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
    UPB_ASSERT(!upb_def_isfrozen(def));
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
      if (upb_strtable_lookup(&s->symtab, fullname, NULL)) {
        upb_status_seterrf(status, "Symtab already has a def named '%s'",
                           fullname);
        goto err;
      }
      if (!upb_strtable_insert(&addtab, fullname, upb_value_ptr(def)))
        goto oom_err;
      upb_def_donateref(def, ref_donor, s);
    }

    if (upb_dyncast_fielddef_mutable(def)) {
      /* TODO(haberman): allow adding extensions attached to files. */
      upb_status_seterrf(status, "Can't add extensions to symtab.\n");
      goto err;
    }
  }

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

  /* We need an array of the defs in addtab, for passing to
   * upb_refcounted_freeze(). */
  add_objs_size = upb_strtable_count(&addtab);
  if (freeze_also) {
    add_objs_size++;
  }

  add_defs = upb_gmalloc(sizeof(void*) * add_objs_size);
  if (add_defs == NULL) goto oom_err;
  upb_strtable_begin(&iter, &addtab);
  for (add_n = 0; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    add_defs[add_n++] = upb_value_getptr(upb_strtable_iter_value(&iter));
  }

  /* Validate defs. */
  if (!_upb_def_validate(add_defs, add_n, status)) {
    goto err;
  }

  /* Cheat a little and give the array a new type.
   * This is probably undefined behavior, but this code will be deleted soon. */
  add_objs = (upb_refcounted**)add_defs;

  freeze_n = add_n;
  if (freeze_also) {
    add_objs[freeze_n++] = freeze_also;
  }

  if (!upb_refcounted_freeze(add_objs, freeze_n, status,
                             UPB_MAX_MESSAGE_DEPTH * 2)) {
    goto err;
  }

  /* This must be delayed until all errors have been detected, since error
   * recovery code uses this table to cleanup defs. */
  upb_strtable_uninit(&addtab);

  /* TODO(haberman) we don't properly handle errors after this point (like
   * OOM in upb_strtable_insert() below). */
  for (i = 0; i < add_n; i++) {
    upb_def *def = (upb_def*)add_objs[i];
    const char *name = upb_def_fullname(def);
    bool success;
    success = upb_strtable_insert(&s->symtab, name, upb_value_ptr(def));
    UPB_ASSERT(success);
  }
  upb_gfree(add_defs);
  return true;

oom_err:
  upb_status_seterrmsg(status, "out of memory");
err: {
    /* We need to donate the refs back. */
    upb_strtable_begin(&iter, &addtab);
    for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
      upb_def *def = upb_value_getptr(upb_strtable_iter_value(&iter));
      upb_def_donateref(def, s, ref_donor);
    }
  }
  upb_strtable_uninit(&addtab);
  upb_gfree(add_defs);
  UPB_ASSERT(!upb_ok(status));
  return false;
}

bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, size_t n,
                    void *ref_donor, upb_status *status) {
  return symtab_add(s, defs, n, ref_donor, NULL, status);
}

bool upb_symtab_addfile(upb_symtab *s, upb_filedef *file, upb_status *status) {
  size_t n;
  size_t i;
  upb_def **defs;
  bool ret;

  n = upb_filedef_defcount(file);
  if (n == 0) {
    return true;
  }
  defs = upb_gmalloc(sizeof(*defs) * n);

  if (defs == NULL) {
    upb_status_seterrmsg(status, "Out of memory");
    return false;
  }

  for (i = 0; i < n; i++) {
    defs[i] = upb_filedef_mutabledef(file, i);
  }

  ret = symtab_add(s, defs, n, NULL, upb_filedef_upcast_mutable(file), status);

  upb_gfree(defs);
  return ret;
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
