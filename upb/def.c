
#include "upb/def.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "upb/handlers.h"

struct upb_fielddef {
  union {
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    void *bytes;
  } defaultval;
  const upb_msgdef *msgdef;
  union {
    const upb_msgdef *msgdef;
    const upb_msgdef *enumdef;
    char *unresolved_name;
  } sub;
  union {
    const upb_oneofdef *oneof;
  }
  uint32_t number_;
  uint32_t selector_base;  /* Used to index into a upb::Handlers table. */
  bool is_extension_;
  bool lazy_;
  bool packed_;
  upb_descriptortype_t type_;
  upb_label_t label_;
};

struct upb_msgdef {
  upb_filedef *file;
  const char *full_name;
  uint32_t selector_count;
  uint32_t submsg_field_count;

  upb_fielddef **fields;
  upb_oneofdef **oneofs;

  /* Tables for looking up fields by number and name.  Built lazily. */
  upb_inttable *itof;  /* int to field */
  upb_strtable *ntof;  /* name to field/oneof */

  /* Is this a map-entry message? */
  bool map_entry;

  /* TODO(haberman): proper extension ranges (there can be multiple). */
};

struct upb_enumdef {
  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
};

struct upb_oneofdef {
  const upb_msgdef *parent;
  const char *name;
  upb_strtable *ntof;
  upb_inttable *itof;
};

struct upb_symtab {
  upb_arena arena;
  upb_strtable symtab;
};

struct upb_filedef {
  const char *name;
  const char *package;
  const char *phpprefix;
  const char *phpnamespace;
  upb_syntax_t syntax;

  upb_inttable defs;
  upb_inttable deps;
};

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

static bool upb_isoneof(const void *def) {
  UPB_UNUSED(def);
  return true;
}

static bool upb_isfield(const void *def) {
  UPB_UNUSED(def);
  return true;
}

static const upb_oneofdef *upb_trygetoneof(const void *def) {
  return upb_isoneof(def) ? (const upb_oneofdef*)def : NULL;
}

static const upb_fielddef *upb_trygetfield(const void *def) {
  return upb_isfield(def) ? (const upb_fielddef*)def : NULL;
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
    UPB_ASSERT(f->msgdef == m);
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

#if 0
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
#endif


/* upb_enumdef ****************************************************************/

#if 0
bool upb_enumdef_init(upb_enumdef *e) {
  upb_def_init(&e->base, UPB_DEF_ENUM);

  if (!upb_strtable_init(&e->ntoi, UPB_CTYPE_INT32)) goto err2;
  if (!upb_inttable_init(&e->iton, UPB_CTYPE_CSTR)) goto err1;
  return true;
}
#endif

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return upb_def_fullname(upb_enumdef_upcast(e));
}

const char *upb_enumdef_name(const upb_enumdef *e) {
  return upb_def_name(upb_enumdef_upcast(e));
}

#if 0
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
#endif

int32_t upb_enumdef_default(const upb_enumdef *e) {
  UPB_ASSERT(upb_enumdef_iton(e, e->defaultval));
  return e->defaultval;
}

#if 0
bool upb_enumdef_setdefault(upb_enumdef *e, int32_t val, upb_status *s) {
  UPB_ASSERT(!upb_enumdef_isfrozen(e));
  if (!upb_enumdef_iton(e, val)) {
    upb_status_seterrf(s, "number '%d' is not in the enum.", val);
    return false;
  }
  e->defaultval = val;
  return true;
}
#endif

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

const char *upb_fielddef_fullname(const upb_fielddef *e) {
  return upb_def_fullname(upb_fielddef_upcast(e));
}

#if 0
upb_fielddef *upb_fielddef_new(const void *o) {
  upb_fielddef *f = upb_gmalloc(sizeof(*f));
  if (!f) return NULL;
  upb_def_init(&f->base, UPB_DEF_FIELD);
  f->msg.def = NULL;
  f->sub.def = NULL;
  f->oneof = NULL;
  f->subdef_is_symbolic = false;
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
#endif

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
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
  return f->msgdef;
}

const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f) {
  return f->oneof;
}

static void chkdefaulttype(const upb_fielddef *f, int ctype) {
  UPB_UNUSED(f);
  UPB_UNUSED(ctype);
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

const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f) {
  const upb_def *def = upb_fielddef_subdef(f);
  return def ? upb_dyncast_msgdef(def) : NULL;
}

const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f) {
  const upb_def *def = upb_fielddef_subdef(f);
  return def ? upb_dyncast_enumdef(def) : NULL;
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

upb_descriptortype_t upb_fielddef_descriptortype(const upb_fielddef *f) {
  return f->type;
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

const char *upb_msgdef_fullname(const upb_msgdef *m) {
  return upb_def_fullname(upb_msgdef_upcast(m));
}

const char *upb_msgdef_name(const upb_msgdef *m) {
  return upb_def_name(upb_msgdef_upcast(m));
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

const char *upb_oneofdef_name(const upb_oneofdef *o) { return o->name; }

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

void upb_symtab_free(upb_symtab *s) {
  upb_arena_uninit(&s->arena);
  upb_gfree(s);
}

upb_symtab *upb_symtab_new() {
  upb_symtab *s = upb_gmalloc(sizeof(*s));
  if (!s) {
    return NULL;
  }

  if (!upb_arena_init(&s->arena)) goto err2;
  if (!upb_strtable_init2(&s->symtab, UPB_CTYPE_PTR,
                          upb_arena_alloc(&s->arena))) {
    goto err1;
  }
  return s;

err1:
  upb_arena_uninit(&s->arena);
err2:
  upb_gfree(s);
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

#if 0
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
#endif

static bool create_oneofdef(
    upb_symtab *s, const google_protobuf_OneofDescriptorProto *oneof_proto,
    upb_msgdef *m, upb_status *s) {
  upb_alloc *alloc = upb_arena_alloc(&s->arena);
  upb_oneofdef *o = upb_malloc(alloc, sizeof(upb_oneofdef));
  upb_stringview name = google_protobuf_OneofDescriptorProto_name(oneof_proto);
  upb_value o_ptr = upb_value_ptr(o);

  CHK(o);
  CHK(upb_inttable_init2(&o->itof, UPB_CTYPE_PTR, alloc));
  CHK(upb_strtable_init2(&o->ntof, UPB_CTYPE_PTR, alloc));

  o->index = upb_strtable_count(&m->ntof);
  o->name = upb_strdup2(name.data, name.size, alloc);
  o->parent = m;

  CHK(upb_strtable_insert3(&m->ntof, name.data, name.size, o_ptr, alloc));

  return true;
}

static bool create_field(
    upb_symtab *s, const google_protobuf_FieldDescriptorProto *field_proto,
    upb_msgdef *m, upb_status *s) {
  upb_alloc *alloc = upb_arena_alloc(&s->arena);
  upb_fielddef *f = upb_malloc(alloc, sizeof(upb_fielddef));
  const google_protobuf_MessageOptions *options;
  const upb_array *arr;

  CHK(f);

  f->msgdef = m;

  union {
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    void *bytes;
  } defaultval;
  const upb_msgdef *msgdef;
  union {
    const upb_msgdef *msgdef;
    const upb_msgdef *enumdef;
  } sub;
  const upb_oneofdef *oneof;
  bool is_extension_;
  bool lazy_;
  bool packed_;
  upb_intfmt_t intfmt;
  bool tagdelim;
  upb_fieldtype_t type_;
  upb_label_t label_;
  uint32_t number_;
  uint32_t selector_base;  /* Used to index into a upb::Handlers table. */
  uint32_t index_;
}

static bool create_msgdef(upb_symtab *s,
                          const google_protobuf_DescriptorProto *msg_proto,
                          upb_symtab *addtab, upb_status *status) {
  upb_alloc *alloc = upb_arena_alloc(&s->arena);
  upb_msgdef *m = upb_malloc(alloc, sizeof(upb_msgdef));
  const google_protobuf_MessageOptions *options;
  const upb_array *arr;

  CHK(m);
  CHK(upb_inttable_init2(&m->itof, UPB_CTYPE_PTR, alloc));
  CHK(upb_strtable_init2(&m->ntof, UPB_CTYPE_PTR, alloc));

  m->map_entry = false;

  options = google_protobuf_DescriptorProto_options(msg_proto);

  if (options) {
    m->map_entry = google_protobuf_MessageOptions_map_entry(options);
  }

  arr = google_protobuf_DescriptorProto_oneof_decl(msg_proto);

  for (i = 0; i < upb_array_size(arr); i++) {
    const google_protobuf_OneofDescriptorProto *oneof_proto =
        (const void *)upb_array_get(arr, i);
    CHK(create_oneofdef(s, oneof_proto, m, status));
  }

  arr = google_protobuf_DescriptorProto_field(msg_proto);

  for (i = 0; i < upb_array_size(arr); i++) {
    const google_protobuf_FieldDescriptorProto *field_proto =
        (const void *)upb_array_get(arr, i);
    CHK(create_fielddef(s, field_proto, m, status));
  }

  arr = google_protobuf_DescriptorProto_enum_type(msg_proto);

  for (i = 0; i < upb_array_size(arr); i++) {
    const google_protobuf_EnumDescriptorProto *enum_proto =
        (const void *)upb_array_get(arr, i);
    CHK(create_enumdef(s, enum_proto, addtab, status));
  }

  arr = google_protobuf_DescriptorProto_nested_type(msg_proto);

  for (i = 0; i < upb_array_size(arr); i++) {
    const google_protobuf_DescriptorProto *msg_proto2 =
        (const void *)upb_array_get(arr, i);
    CHK(create_msgdef(s, msg_proto2, addtab, status));
  }

  return true;
}

static char* strviewdup(upb_symtab *symtab, upb_stringview view) {
  if (view.size == 0) {
    return NULL;
  }
  return upb_strdup2(view.data, view.size, upb_arena_alloc(&symtab->arena));
}

bool upb_symtab_addfile(upb_symtab *s, const char *buf, size_t len,
                        upb_status *status) {
  upb_arena tmparena;
  upb_strtable addtab;
  upb_alloc *alloc = upb_arena_alloc(&s->arena);
  upb_stringview serialized = upb_stringview_make(buf, len);
  const google_protobuf_FileDescriptorProto *file_proto;
  const google_protobuf_FileOptions *file_options_proto;
  upb_filedef *file = upb_malloc(alloc, sizeof(*file));
  const upb_array *arr;
  upb_strtable_iter iter;
  size_t i;

  upb_arena_init(&tmparena);
  upb_strtable_init2(&addtab, UPB_CTYPE_PTR, upb_arena_alloc(&tmparena));
  file_proto =
      google_protobuf_FileDescriptorProto_parsenew(serialized, &tmparena);


  if (!file_proto || !file) goto err;

  file->name =
      strviewdup(s, google_protobuf_FileDescriptorProto_name(file_proto));
  file->package =
      strviewdup(s, google_protobuf_FileDescriptorProto_package(file_proto));
  file->phpprefix = NULL;
  file->phpnamespace = NULL;

  file_options_proto = google_protobuf_FileDescriptorProto_options(file_proto);
  if (file_options_proto) {
    file->phpprefix =
        strviewdup(s, google_protobuf_FileOptions_php_class_prefix(file_proto));
    file->phpnamespace =
        strviewdup(s, google_protobuf_FileOptions_php_namespace(file_proto));
  }

  arr = google_protobuf_FileDescriptorProto_message_type(file_proto);

  for (i = 0; i < upb_array_size(arr); i++) {
    const google_protobuf_DescriptorProto *msg_proto =
        (const void *)upb_array_get(arr, i);
    if (!create_msgdef(s, msg_proto, &addtab, status)) goto err;
  }

  arr = google_protobuf_FileDescriptorProto_enum_type(file_proto);

  for (i = 0; i < upb_array_size(arr); i++) {
    const google_protobuf_EnumDescriptorProto *enum_proto =
        (const void *)upb_array_get(arr, i);
    if (!create_enumdef(s, enum_proto, &addtab, status)) goto err;
  }

  /* Now that all names are in the table, resolve references. */
  upb_strtable_begin(&iter, &addtab);
  for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
  }

  /* Success; add addtab to symtab. */
  upb_strtable_begin(&iter, &addtab);
  for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    const char *key = upb_strtable_iter_key(&iter);
    size_t keylen = upb_strtable_iter_keylen(&iter);
    upb_value value = upb_strtable_iter_value(&iter);
    upb_strtable_insert3(&s->symtab, key, keylen, value, alloc);
  }

  return true;

err:
  upb_arena_uninit(&tmparena);
  return false;
}

bool upb_symtab_addset(upb_symtab *s, const char *buf, size_t len,
                       upb_status *status) {
  return true;
}
