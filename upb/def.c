
#include "upb/def.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "google/protobuf/descriptor.upb.h"

#include "upb/port_def.inc"

typedef struct {
  size_t len;
  char str[1];  /* Null-terminated string data follows. */
} str_t;

static str_t *newstr(upb_alloc *alloc, const char *data, size_t len) {
  str_t *ret = upb_malloc(alloc, sizeof(*ret) + len);
  if (!ret) return NULL;
  ret->len = len;
  memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

struct upb_fielddef {
  const upb_filedef *file;
  const upb_msgdef *msgdef;
  const char *full_name;
  union {
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    bool boolean;
    str_t *str;
  } defaultval;
  const upb_oneofdef *oneof;
  union {
    const upb_msgdef *msgdef;
    const upb_enumdef *enumdef;
    const google_protobuf_FieldDescriptorProto *unresolved;
  } sub;
  uint32_t number_;
  uint32_t index_;
  uint32_t selector_base;  /* Used to index into a upb::Handlers table. */
  bool is_extension_;
  bool lazy_;
  bool packed_;
  upb_descriptortype_t type_;
  upb_label_t label_;
};

struct upb_msgdef {
  const upb_filedef *file;
  const char *full_name;
  uint32_t selector_count;
  uint32_t submsg_field_count;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;
  upb_strtable ntof;

  const upb_fielddef *fields;
  const upb_oneofdef *oneofs;
  int field_count;
  int oneof_count;

  /* Is this a map-entry message? */
  bool map_entry;
  upb_wellknowntype_t well_known_type;

  /* TODO(haberman): proper extension ranges (there can be multiple). */
};

struct upb_enumdef {
  const upb_filedef *file;
  const char *full_name;
  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
};

struct upb_oneofdef {
  const upb_msgdef *parent;
  const char *full_name;
  uint32_t index;
  upb_strtable ntof;
  upb_inttable itof;
};

struct upb_filedef {
  const char *name;
  const char *package;
  const char *phpprefix;
  const char *phpnamespace;
  upb_syntax_t syntax;

  const upb_filedef **deps;
  const upb_msgdef *msgs;
  const upb_enumdef *enums;
  const upb_fielddef *exts;

  int dep_count;
  int msg_count;
  int enum_count;
  int ext_count;
};

struct upb_symtab {
  upb_arena *arena;
  upb_strtable syms;  /* full_name -> packed def ptr */
  upb_strtable files;  /* file_name -> upb_filedef* */
};

/* Inside a symtab we store tagged pointers to specific def types. */
typedef enum {
  UPB_DEFTYPE_MSG = 0,
  UPB_DEFTYPE_ENUM = 1,
  UPB_DEFTYPE_FIELD = 2,
  UPB_DEFTYPE_ONEOF = 3
} upb_deftype_t;

static const void *unpack_def(upb_value v, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return (num & 3) == type ? (const void*)(num & ~3) : NULL;
}

static upb_value pack_def(const void *ptr, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)ptr | type;
  return upb_value_constptr((const void*)num);
}

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

static bool upb_isident(upb_strview name, bool full, upb_status *s) {
  const char *str = name.data;
  size_t len = name.size;
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

static const char *shortdefname(const char *fullname) {
  const char *p;

  if (fullname == NULL) {
    return NULL;
  } else if ((p = strrchr(fullname, '.')) == NULL) {
    /* No '.' in the name, return the full string. */
    return fullname;
  } else {
    /* Return one past the last '.'. */
    return p + 1;
  }
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

/* A few implementation details of handlers.  We put these here to avoid
 * a def -> handlers dependency. */

#define UPB_STATIC_SELECTOR_COUNT 3  /* Warning: also in upb/handlers.h. */

static uint32_t upb_handlers_selectorbaseoffset(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) ? 2 : 0;
}

static uint32_t upb_handlers_selectorcount(const upb_fielddef *f) {
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
    upb_status_setoom(s);
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

  for(upb_msg_oneof_begin(&k, m), i = 0;
      !upb_msg_oneof_done(&k);
      upb_msg_oneof_next(&k), i++) {
    upb_oneofdef *o = (upb_oneofdef*)upb_msg_iter_oneof(&k);
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
  if (!strcmp(name, "google.protobuf.Any")) {
    m->well_known_type = UPB_WELLKNOWN_ANY;
  } else if (!strcmp(name, "google.protobuf.FieldMask")) {
    m->well_known_type = UPB_WELLKNOWN_FIELDMASK;
  } else if (!strcmp(name, "google.protobuf.Duration")) {
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


/* upb_enumdef ****************************************************************/

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return e->full_name;
}

const char *upb_enumdef_name(const upb_enumdef *e) {
  return shortdefname(e->full_name);
}

const upb_filedef *upb_enumdef_file(const upb_enumdef *e) {
  return e->file;
}

int32_t upb_enumdef_default(const upb_enumdef *e) {
  UPB_ASSERT(upb_enumdef_iton(e, e->defaultval));
  return e->defaultval;
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

const char *upb_fielddef_fullname(const upb_fielddef *f) {
  return f->full_name;
}

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f) {
  switch (f->type_) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      return UPB_TYPE_DOUBLE;
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      return UPB_TYPE_FLOAT;
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_SINT64:
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
      return UPB_TYPE_INT64;
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
    case UPB_DESCRIPTOR_TYPE_SINT32:
      return UPB_TYPE_INT32;
    case UPB_DESCRIPTOR_TYPE_UINT64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      return UPB_TYPE_UINT64;
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_FIXED32:
      return UPB_TYPE_UINT32;
    case UPB_DESCRIPTOR_TYPE_ENUM:
      return UPB_TYPE_ENUM;
    case UPB_DESCRIPTOR_TYPE_BOOL:
      return UPB_TYPE_BOOL;
    case UPB_DESCRIPTOR_TYPE_STRING:
      return UPB_TYPE_STRING;
    case UPB_DESCRIPTOR_TYPE_BYTES:
      return UPB_TYPE_BYTES;
    case UPB_DESCRIPTOR_TYPE_GROUP:
    case UPB_DESCRIPTOR_TYPE_MESSAGE:
      return UPB_TYPE_MESSAGE;
  }
  UPB_UNREACHABLE();
}

upb_descriptortype_t upb_fielddef_descriptortype(const upb_fielddef *f) {
  return f->type_;
}

uint32_t upb_fielddef_index(const upb_fielddef *f) {
  return f->index_;
}

upb_label_t upb_fielddef_label(const upb_fielddef *f) {
  return f->label_;
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
  return shortdefname(f->full_name);
}

uint32_t upb_fielddef_selectorbase(const upb_fielddef *f) {
  return f->selector_base;
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
  return f->defaultval.boolean;
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
  str_t *str = f->defaultval.str;
  UPB_ASSERT(upb_fielddef_type(f) == UPB_TYPE_STRING ||
         upb_fielddef_type(f) == UPB_TYPE_BYTES ||
         upb_fielddef_type(f) == UPB_TYPE_ENUM);
  if (str) {
    if (len) *len = str->len;
    return str->str;
  } else {
    if (len) *len = 0;
    return NULL;
  }
}

const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f) {
  UPB_ASSERT(upb_fielddef_type(f) == UPB_TYPE_MESSAGE);
  return f->sub.msgdef;
}

const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f) {
  UPB_ASSERT(upb_fielddef_type(f) == UPB_TYPE_ENUM);
  return f->sub.enumdef;
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

bool upb_fielddef_haspresence(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) return false;
  if (upb_fielddef_issubmsg(f)) return true;
  return f->file->syntax == UPB_SYNTAX_PROTO2;
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
  return m->full_name;
}

const upb_filedef *upb_msgdef_file(const upb_msgdef *m) {
  return m->file;
}

const char *upb_msgdef_name(const upb_msgdef *m) {
  return shortdefname(m->full_name);
}

upb_syntax_t upb_msgdef_syntax(const upb_msgdef *m) {
  return m->file->syntax;
}

size_t upb_msgdef_selectorcount(const upb_msgdef *m) {
  return m->selector_count;
}

uint32_t upb_msgdef_submsgfieldcount(const upb_msgdef *m) {
  return m->submsg_field_count;
}

const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  upb_value val;
  return upb_inttable_lookup32(&m->itof, i, &val) ?
      upb_value_getconstptr(val) : NULL;
}

const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return unpack_def(val, UPB_DEFTYPE_FIELD);
}

const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return unpack_def(val, UPB_DEFTYPE_ONEOF);
}

bool upb_msgdef_lookupname(const upb_msgdef *m, const char *name, size_t len,
                           const upb_fielddef **f, const upb_oneofdef **o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  *o = unpack_def(val, UPB_DEFTYPE_ONEOF);
  *f = unpack_def(val, UPB_DEFTYPE_FIELD);
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
  return (upb_fielddef *)upb_value_getconstptr(upb_inttable_iter_value(iter));
}

void upb_msg_field_iter_setdone(upb_msg_field_iter *iter) {
  upb_inttable_iter_setdone(iter);
}

bool upb_msg_field_iter_isequal(const upb_msg_field_iter * iter1,
                                const upb_msg_field_iter * iter2) {
  return upb_inttable_iter_isequal(iter1, iter2);
}

void upb_msg_oneof_begin(upb_msg_oneof_iter *iter, const upb_msgdef *m) {
  upb_strtable_begin(iter, &m->ntof);
  /* We need to skip past any initial fields. */
  while (!upb_strtable_done(iter) &&
         !unpack_def(upb_strtable_iter_value(iter), UPB_DEFTYPE_ONEOF)) {
    upb_strtable_next(iter);
  }
}

void upb_msg_oneof_next(upb_msg_oneof_iter *iter) {
  /* We need to skip past fields to return only oneofs. */
  do {
    upb_strtable_next(iter);
  } while (!upb_strtable_done(iter) &&
           !unpack_def(upb_strtable_iter_value(iter), UPB_DEFTYPE_ONEOF));
}

bool upb_msg_oneof_done(const upb_msg_oneof_iter *iter) {
  return upb_strtable_done(iter);
}

const upb_oneofdef *upb_msg_iter_oneof(const upb_msg_oneof_iter *iter) {
  return unpack_def(upb_strtable_iter_value(iter), UPB_DEFTYPE_ONEOF);
}

void upb_msg_oneof_iter_setdone(upb_msg_oneof_iter *iter) {
  upb_strtable_iter_setdone(iter);
}

bool upb_msg_oneof_iter_isequal(const upb_msg_oneof_iter *iter1,
                                const upb_msg_oneof_iter *iter2) {
  return upb_strtable_iter_isequal(iter1, iter2);
}

/* upb_oneofdef ***************************************************************/

const char *upb_oneofdef_name(const upb_oneofdef *o) {
  return shortdefname(o->full_name);
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
  return (upb_fielddef *)upb_value_getconstptr(upb_inttable_iter_value(iter));
}

void upb_oneof_iter_setdone(upb_oneof_iter *iter) {
  upb_inttable_iter_setdone(iter);
}

/* Code to build defs from descriptor protos. *********************************/

/* There is a question of how much validation to do here.  It will be difficult
 * to perfectly match the amount of validation performed by proto2.  But since
 * this code is used to directly build defs from Ruby (for example) we do need
 * to validate important constraints like uniqueness of names and numbers. */

#define CHK(x) if (!(x)) { return false; }
#define CHK_OOM(x) if (!(x)) { upb_status_setoom(ctx->status); return false; }

typedef struct {
  const upb_symtab *symtab;
  upb_filedef *file;  /* File we are building. */
  upb_alloc *alloc;    /* Allocate defs here. */
  upb_alloc *tmp;      /* Alloc for addtab and any other tmp data. */
  upb_strtable *addtab;  /* full_name -> packed def ptr for new defs. */
  upb_status *status;  /* Record errors here. */
} symtab_addctx;

static char* strviewdup(const symtab_addctx *ctx, upb_strview view) {
  return upb_strdup2(view.data, view.size, ctx->alloc);
}

static bool streql2(const char *a, size_t n, const char *b) {
  return n == strlen(b) && memcmp(a, b, n) == 0;
}

static bool streql_view(upb_strview view, const char *b) {
  return streql2(view.data, view.size, b);
}

static const char *makefullname(const symtab_addctx *ctx, const char *prefix,
                                upb_strview name) {
  if (prefix) {
    /* ret = prefix + '.' + name; */
    size_t n = strlen(prefix);
    char *ret = upb_malloc(ctx->alloc, n + name.size + 2);
    CHK_OOM(ret);
    strcpy(ret, prefix);
    ret[n] = '.';
    memcpy(&ret[n + 1], name.data, name.size);
    ret[n + 1 + name.size] = '\0';
    return ret;
  } else {
    return strviewdup(ctx, name);
  }
}

static bool symtab_add(const symtab_addctx *ctx, const char *name,
                       upb_value v) {
  upb_value tmp;
  if (upb_strtable_lookup(ctx->addtab, name, &tmp) ||
      upb_strtable_lookup(&ctx->symtab->syms, name, &tmp)) {
    upb_status_seterrf(ctx->status, "duplicate symbol '%s'", name);
    return false;
  }

  CHK_OOM(upb_strtable_insert3(ctx->addtab, name, strlen(name), v, ctx->tmp));
  return true;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static bool resolvename(const upb_strtable *t, const upb_fielddef *f,
                        const char *base, upb_strview sym,
                        upb_deftype_t type, upb_status *status,
                        const void **def) {
  if(sym.size == 0) return NULL;
  if(sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    upb_value v;
    if (!upb_strtable_lookup2(t, sym.data + 1, sym.size - 1, &v)) {
      return false;
    }

    *def = unpack_def(v, type);

    if (!*def) {
      upb_status_seterrf(status,
                         "type mismatch when resolving field %s, name %s",
                         f->full_name, sym.data);
      return false;
    }

    return true;
  } else {
    /* Remove components from base until we find an entry or run out.
     * TODO: This branch is totally broken, but currently not used. */
    (void)base;
    UPB_ASSERT(false);
    return false;
  }
}

const void *symtab_resolve(const symtab_addctx *ctx, const upb_fielddef *f,
                           const char *base, upb_strview sym,
                           upb_deftype_t type) {
  const void *ret;
  if (!resolvename(ctx->addtab, f, base, sym, type, ctx->status, &ret) &&
      !resolvename(&ctx->symtab->syms, f, base, sym, type, ctx->status, &ret)) {
    if (upb_ok(ctx->status)) {
      upb_status_seterrf(ctx->status, "couldn't resolve name '%s'", sym.data);
    }
    return false;
  }
  return ret;
}

static bool create_oneofdef(
    const symtab_addctx *ctx, upb_msgdef *m,
    const google_protobuf_OneofDescriptorProto *oneof_proto) {
  upb_oneofdef *o;
  upb_strview name = google_protobuf_OneofDescriptorProto_name(oneof_proto);
  upb_value v;

  o = (upb_oneofdef*)&m->oneofs[m->oneof_count++];
  o->parent = m;
  o->full_name = makefullname(ctx, m->full_name, name);

  v = pack_def(o, UPB_DEFTYPE_ONEOF);
  CHK_OOM(symtab_add(ctx, o->full_name, v));
  CHK_OOM(upb_strtable_insert3(&m->ntof, name.data, name.size, v, ctx->alloc));

  CHK_OOM(upb_inttable_init2(&o->itof, UPB_CTYPE_CONSTPTR, ctx->alloc));
  CHK_OOM(upb_strtable_init2(&o->ntof, UPB_CTYPE_CONSTPTR, ctx->alloc));

  return true;
}

static bool parse_default(const symtab_addctx *ctx, const char *str, size_t len,
                          upb_fielddef *f) {
  char *end;
  char nullz[64];
  errno = 0;

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_FLOAT:
      /* Standard C number parsing functions expect null-terminated strings. */
      if (len >= sizeof(nullz) - 1) {
        return false;
      }
      memcpy(nullz, str, len);
      nullz[len] = '\0';
      str = nullz;
      break;
    default:
      break;
  }

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32: {
      long val = strtol(str, &end, 0);
      CHK(val <= INT32_MAX && val >= INT32_MIN && errno != ERANGE && !*end);
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_ENUM: {
      const upb_enumdef *e = f->sub.enumdef;
      int32_t val;
      CHK(upb_enumdef_ntoi(e, str, len, &val));
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_INT64: {
      /* XXX: Need to write our own strtoll, since it's not available in c89. */
      long long val = strtol(str, &end, 0);
      CHK(val <= INT64_MAX && val >= INT64_MIN && errno != ERANGE && !*end);
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(str, &end, 0);
      CHK(val <= UINT32_MAX && errno != ERANGE && !*end);
      f->defaultval.uint = val;
      break;
    }
    case UPB_TYPE_UINT64: {
      /* XXX: Need to write our own strtoull, since it's not available in c89. */
      unsigned long long val = strtoul(str, &end, 0);
      CHK(val <= UINT64_MAX && errno != ERANGE && !*end);
      f->defaultval.uint = val;
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(str, &end);
      CHK(errno != ERANGE && !*end);
      f->defaultval.dbl = val;
      break;
    }
    case UPB_TYPE_FLOAT: {
      /* XXX: Need to write our own strtof, since it's not available in c89. */
      float val = strtod(str, &end);
      CHK(errno != ERANGE && !*end);
      f->defaultval.flt = val;
      break;
    }
    case UPB_TYPE_BOOL: {
      if (streql2(str, len, "false")) {
        f->defaultval.boolean = false;
      } else if (streql2(str, len, "true")) {
        f->defaultval.boolean = true;
      } else {
        return false;
      }
    }
    case UPB_TYPE_STRING:
      f->defaultval.str = newstr(ctx->alloc, str, len);
      break;
    case UPB_TYPE_BYTES:
      /* XXX: need to interpret the C-escaped value. */
      f->defaultval.str = newstr(ctx->alloc, str, len);
      break;
    case UPB_TYPE_MESSAGE:
      /* Should not have a default value. */
      return false;
  }
  return true;
}

static void set_default_default(const symtab_addctx *ctx, upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_ENUM:
      f->defaultval.sint = 0;
      break;
    case UPB_TYPE_UINT64:
    case UPB_TYPE_UINT32:
      f->defaultval.uint = 0;
      break;
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_FLOAT:
      f->defaultval.dbl = 0;
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      f->defaultval.str = newstr(ctx->alloc, NULL, 0);
      break;
    case UPB_TYPE_BOOL:
      f->defaultval.boolean = false;
      break;
    case UPB_TYPE_MESSAGE:
      break;
  }
}

static bool create_fielddef(
    const symtab_addctx *ctx, const char *prefix, upb_msgdef *m,
    const google_protobuf_FieldDescriptorProto *field_proto) {
  upb_alloc *alloc = ctx->alloc;
  upb_fielddef *f;
  const google_protobuf_FieldOptions *options;
  upb_strview name;
  const char *full_name;
  const char *shortname;
  uint32_t field_number;

  if (!google_protobuf_FieldDescriptorProto_has_name(field_proto)) {
    upb_status_seterrmsg(ctx->status, "field has no name");
    return false;
  }

  name = google_protobuf_FieldDescriptorProto_name(field_proto);
  CHK(upb_isident(name, false, ctx->status));
  full_name = makefullname(ctx, prefix, name);
  shortname = shortdefname(full_name);

  field_number = google_protobuf_FieldDescriptorProto_number(field_proto);

  if (field_number == 0 || field_number > UPB_MAX_FIELDNUMBER) {
    upb_status_seterrf(ctx->status, "invalid field number (%u)", field_number);
    return false;
  }

  if (m) {
    /* direct message field. */
    upb_value v, packed_v;

    f = (upb_fielddef*)&m->fields[m->field_count++];
    f->msgdef = m;
    f->is_extension_ = false;

    packed_v = pack_def(f, UPB_DEFTYPE_FIELD);
    v = upb_value_constptr(f);

    if (!upb_strtable_insert3(&m->ntof, name.data, name.size, packed_v, alloc)) {
      upb_status_seterrf(ctx->status, "duplicate field name (%s)", shortname);
      return false;
    }

    if (!upb_inttable_insert2(&m->itof, field_number, v, alloc)) {
      upb_status_seterrf(ctx->status, "duplicate field number (%u)",
                         field_number);
      return false;
    }
  } else {
    /* extension field. */
    f = (upb_fielddef*)&ctx->file->exts[ctx->file->ext_count];
    f->is_extension_ = true;
    CHK_OOM(symtab_add(ctx, full_name, pack_def(f, UPB_DEFTYPE_FIELD)));
  }

  f->full_name = full_name;
  f->file = ctx->file;
  f->type_ = (int)google_protobuf_FieldDescriptorProto_type(field_proto);
  f->label_ = (int)google_protobuf_FieldDescriptorProto_label(field_proto);
  f->number_ = field_number;
  f->oneof = NULL;

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (f->label_ == UPB_LABEL_REQUIRED && f->file->syntax == UPB_SYNTAX_PROTO3) {
    upb_status_seterrf(ctx->status, "proto3 fields cannot be required (%s)",
                       f->full_name);
    return false;
  }

  if (google_protobuf_FieldDescriptorProto_has_oneof_index(field_proto)) {
    int oneof_index =
        google_protobuf_FieldDescriptorProto_oneof_index(field_proto);
    upb_oneofdef *oneof;
    upb_value v = upb_value_constptr(f);

    if (upb_fielddef_label(f) != UPB_LABEL_OPTIONAL) {
      upb_status_seterrf(ctx->status,
                         "fields in oneof must have OPTIONAL label (%s)",
                         f->full_name);
      return false;
    }

    if (!m) {
      upb_status_seterrf(ctx->status,
                         "oneof_index provided for extension field (%s)",
                         f->full_name);
      return false;
    }

    if (oneof_index >= m->oneof_count) {
      upb_status_seterrf(ctx->status, "oneof_index out of range (%s)",
                         f->full_name);
      return false;
    }

    oneof = (upb_oneofdef*)&m->oneofs[oneof_index];
    f->oneof = oneof;

    CHK(upb_inttable_insert2(&oneof->itof, f->number_, v, alloc));
    CHK(upb_strtable_insert3(&oneof->ntof, name.data, name.size, v, alloc));
  } else {
    f->oneof = NULL;
  }

  if (google_protobuf_FieldDescriptorProto_has_options(field_proto)) {
    options = google_protobuf_FieldDescriptorProto_options(field_proto);
    f->lazy_ = google_protobuf_FieldOptions_lazy(options);
    f->packed_ = google_protobuf_FieldOptions_packed(options);
  } else {
    f->lazy_ = false;
    f->packed_ = false;
  }

  return true;
}

static bool create_enumdef(
    const symtab_addctx *ctx, const char *prefix,
    const google_protobuf_EnumDescriptorProto *enum_proto) {
  upb_enumdef *e;
  const google_protobuf_EnumValueDescriptorProto *const *values;
  upb_strview name;
  size_t i, n;

  name = google_protobuf_EnumDescriptorProto_name(enum_proto);
  CHK(upb_isident(name, false, ctx->status));

  e = (upb_enumdef*)&ctx->file->enums[ctx->file->enum_count++];
  e->full_name = makefullname(ctx, prefix, name);
  CHK_OOM(symtab_add(ctx, e->full_name, pack_def(e, UPB_DEFTYPE_ENUM)));

  CHK_OOM(upb_strtable_init2(&e->ntoi, UPB_CTYPE_INT32, ctx->alloc));
  CHK_OOM(upb_inttable_init2(&e->iton, UPB_CTYPE_CSTR, ctx->alloc));

  e->file = ctx->file;
  e->defaultval = 0;

  values = google_protobuf_EnumDescriptorProto_value(enum_proto, &n);

  if (n == 0) {
    upb_status_seterrf(ctx->status,
                       "enums must contain at least one value (%s)",
                       e->full_name);
    return false;
  }

  for (i = 0; i < n; i++) {
    const google_protobuf_EnumValueDescriptorProto *value = values[i];
    upb_strview name = google_protobuf_EnumValueDescriptorProto_name(value);
    char *name2 = strviewdup(ctx, name);
    int32_t num = google_protobuf_EnumValueDescriptorProto_number(value);
    upb_value v = upb_value_int32(num);

    if (i == 0 && e->file->syntax == UPB_SYNTAX_PROTO3 && num != 0) {
      upb_status_seterrf(ctx->status,
                         "for proto3, the first enum value must be zero (%s)",
                         e->full_name);
      return false;
    }

    if (upb_strtable_lookup(&e->ntoi, name2, NULL)) {
      upb_status_seterrf(ctx->status, "duplicate enum label '%s'", name2);
      return false;
    }

    CHK_OOM(name2)
    CHK_OOM(
        upb_strtable_insert3(&e->ntoi, name2, strlen(name2), v, ctx->alloc));

    if (!upb_inttable_lookup(&e->iton, num, NULL)) {
      upb_value v = upb_value_cstr(name2);
      CHK_OOM(upb_inttable_insert2(&e->iton, num, v, ctx->alloc));
    }
  }

  upb_inttable_compact2(&e->iton, ctx->alloc);

  return true;
}

static bool create_msgdef(const symtab_addctx *ctx, const char *prefix,
                          const google_protobuf_DescriptorProto *msg_proto) {
  upb_msgdef *m;
  const google_protobuf_MessageOptions *options;
  const google_protobuf_OneofDescriptorProto *const *oneofs;
  const google_protobuf_FieldDescriptorProto *const *fields;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;
  upb_strview name;

  name = google_protobuf_DescriptorProto_name(msg_proto);
  CHK(upb_isident(name, false, ctx->status));

  m = (upb_msgdef*)&ctx->file->msgs[ctx->file->msg_count++];
  m->full_name = makefullname(ctx, prefix, name);
  CHK_OOM(symtab_add(ctx, m->full_name, pack_def(m, UPB_DEFTYPE_MSG)));

  CHK_OOM(upb_inttable_init2(&m->itof, UPB_CTYPE_CONSTPTR, ctx->alloc));
  CHK_OOM(upb_strtable_init2(&m->ntof, UPB_CTYPE_CONSTPTR, ctx->alloc));

  m->file = ctx->file;
  m->map_entry = false;

  options = google_protobuf_DescriptorProto_options(msg_proto);

  if (options) {
    m->map_entry = google_protobuf_MessageOptions_map_entry(options);
  }

  oneofs = google_protobuf_DescriptorProto_oneof_decl(msg_proto, &n);
  m->oneof_count = 0;
  m->oneofs = upb_malloc(ctx->alloc, sizeof(*m->oneofs) * n);
  for (i = 0; i < n; i++) {
    CHK(create_oneofdef(ctx, m, oneofs[i]));
  }

  fields = google_protobuf_DescriptorProto_field(msg_proto, &n);
  m->field_count = 0;
  m->fields = upb_malloc(ctx->alloc, sizeof(*m->fields) * n);
  for (i = 0; i < n; i++) {
    CHK(create_fielddef(ctx, m->full_name, m, fields[i]));
  }

  CHK(assign_msg_indices(m, ctx->status));
  assign_msg_wellknowntype(m);
  upb_inttable_compact2(&m->itof, ctx->alloc);

  /* This message is built.  Now build nested messages and enums. */

  enums = google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    CHK(create_enumdef(ctx, m->full_name, enums[i]));
  }

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    CHK(create_msgdef(ctx, m->full_name, msgs[i]));
  }

  return true;
}

typedef struct {
  int msg_count;
  int enum_count;
  int ext_count;
} decl_counts;

static void count_types_in_msg(const google_protobuf_DescriptorProto *msg_proto,
                               decl_counts *counts) {
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;

  counts->msg_count++;

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    count_types_in_msg(msgs[i], counts);
  }

  google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  counts->enum_count += n;

  google_protobuf_DescriptorProto_extension(msg_proto, &n);
  counts->ext_count += n;
}

static void count_types_in_file(
    const google_protobuf_FileDescriptorProto *file_proto,
    decl_counts *counts) {
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;

  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    count_types_in_msg(msgs[i], counts);
  }

  google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  counts->enum_count += n;

  google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  counts->ext_count += n;
}

static bool resolve_fielddef(const symtab_addctx *ctx, const char *prefix,
                             upb_fielddef *f) {
  upb_strview name;
  const google_protobuf_FieldDescriptorProto *field_proto = f->sub.unresolved;

  if (f->is_extension_) {
    if (!google_protobuf_FieldDescriptorProto_has_extendee(field_proto)) {
      upb_status_seterrf(ctx->status,
                         "extension for field '%s' had no extendee",
                         f->full_name);
      return false;
    }

    name = google_protobuf_FieldDescriptorProto_extendee(field_proto);
    f->msgdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_MSG);
    CHK(f->msgdef);
  }

  if ((upb_fielddef_issubmsg(f) || f->type_ == UPB_DESCRIPTOR_TYPE_ENUM) &&
      !google_protobuf_FieldDescriptorProto_has_type_name(field_proto)) {
    upb_status_seterrf(ctx->status, "field '%s' is missing type name",
                       f->full_name);
    return false;
  }

  name = google_protobuf_FieldDescriptorProto_type_name(field_proto);

  if (upb_fielddef_issubmsg(f)) {
    f->sub.msgdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_MSG);
    CHK(f->sub.msgdef);
  } else if (f->type_ == UPB_DESCRIPTOR_TYPE_ENUM) {
    f->sub.enumdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_ENUM);
    CHK(f->sub.enumdef);
  }

  /* Have to delay resolving of the default value until now because of the enum
   * case, since enum defaults are specified with a label. */
  if (google_protobuf_FieldDescriptorProto_has_default_value(field_proto)) {
    upb_strview defaultval =
        google_protobuf_FieldDescriptorProto_default_value(field_proto);

    if (f->file->syntax == UPB_SYNTAX_PROTO3) {
      upb_status_seterrf(ctx->status,
                         "proto3 fields cannot have explicit defaults (%s)",
                         f->full_name);
      return false;
    }

    if (upb_fielddef_issubmsg(f)) {
      upb_status_seterrf(ctx->status,
                         "message fields cannot have explicit defaults (%s)",
                         f->full_name);
      return false;
    }

    if (!parse_default(ctx, defaultval.data, defaultval.size, f)) {
      upb_status_seterrf(ctx->status,
                         "couldn't parse default '" UPB_STRVIEW_FORMAT
                         "' for field (%s)",
                         UPB_STRVIEW_ARGS(defaultval), f->full_name);
      return false;
    }
  } else {
    set_default_default(ctx, f);
  }

  return true;
}

static bool build_filedef(
    const symtab_addctx *ctx, upb_filedef *file,
    const google_protobuf_FileDescriptorProto *file_proto) {
  upb_alloc *alloc = ctx->alloc;
  const google_protobuf_FileOptions *file_options_proto;
  const google_protobuf_DescriptorProto *const *msgs;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_FieldDescriptorProto *const *exts;
  const upb_strview* strs;
  size_t i, n;
  decl_counts counts = {0};

  count_types_in_file(file_proto, &counts);

  file->msgs = upb_malloc(alloc, sizeof(*file->msgs) * counts.msg_count);
  file->enums = upb_malloc(alloc, sizeof(*file->enums) * counts.enum_count);
  file->exts = upb_malloc(alloc, sizeof(*file->exts) * counts.ext_count);

  CHK_OOM(counts.msg_count == 0 || file->msgs);
  CHK_OOM(counts.enum_count == 0 || file->enums);
  CHK_OOM(counts.ext_count == 0 || file->exts);

  /* We increment these as defs are added. */
  file->msg_count = 0;
  file->enum_count = 0;
  file->ext_count = 0;

  if (!google_protobuf_FileDescriptorProto_has_name(file_proto)) {
    upb_status_seterrmsg(ctx->status, "File has no name");
    return false;
  }

  file->name =
      strviewdup(ctx, google_protobuf_FileDescriptorProto_name(file_proto));
  file->phpprefix = NULL;
  file->phpnamespace = NULL;

  if (google_protobuf_FileDescriptorProto_has_package(file_proto)) {
    upb_strview package =
        google_protobuf_FileDescriptorProto_package(file_proto);
    CHK(upb_isident(package, true, ctx->status));
    file->package = strviewdup(ctx, package);
  } else {
    file->package = NULL;
  }

  if (google_protobuf_FileDescriptorProto_has_syntax(file_proto)) {
    upb_strview syntax =
        google_protobuf_FileDescriptorProto_syntax(file_proto);

    if (streql_view(syntax, "proto2")) {
      file->syntax = UPB_SYNTAX_PROTO2;
    } else if (streql_view(syntax, "proto3")) {
      file->syntax = UPB_SYNTAX_PROTO3;
    } else {
      upb_status_seterrf(ctx->status, "Invalid syntax '%s'", syntax);
      return false;
    }
  } else {
    file->syntax = UPB_SYNTAX_PROTO2;
  }

  /* Read options. */
  file_options_proto = google_protobuf_FileDescriptorProto_options(file_proto);
  if (file_options_proto) {
    if (google_protobuf_FileOptions_has_php_class_prefix(file_options_proto)) {
      file->phpprefix = strviewdup(
          ctx,
          google_protobuf_FileOptions_php_class_prefix(file_options_proto));
    }
    if (google_protobuf_FileOptions_has_php_namespace(file_options_proto)) {
      file->phpnamespace = strviewdup(
          ctx, google_protobuf_FileOptions_php_namespace(file_options_proto));
    }
  }

  /* Verify dependencies. */
  strs = google_protobuf_FileDescriptorProto_dependency(file_proto, &n);
  file->deps = upb_malloc(alloc, sizeof(*file->deps) * n) ;
  CHK_OOM(n == 0 || file->deps);

  for (i = 0; i < n; i++) {
    upb_strview dep_name = strs[i];
    upb_value v;
    if (!upb_strtable_lookup2(&ctx->symtab->files, dep_name.data,
                              dep_name.size, &v)) {
      upb_status_seterrf(ctx->status,
                         "Depends on file '" UPB_STRVIEW_FORMAT
                         "', but it has not been loaded",
                         UPB_STRVIEW_ARGS(dep_name));
      return false;
    }
    file->deps[i] = upb_value_getconstptr(v);
  }

  /* Create messages. */
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    CHK(create_msgdef(ctx, file->package, msgs[i]));
  }

  /* Create enums. */
  enums = google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    CHK(create_enumdef(ctx, file->package, enums[i]));
  }

  /* Create extensions. */
  exts = google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->exts = upb_malloc(alloc, sizeof(*file->exts) * n);
  CHK_OOM(n == 0 || file->exts);
  for (i = 0; i < n; i++) {
    CHK(create_fielddef(ctx, file->package, NULL, exts[i]));
  }

  /* Now that all names are in the table, resolve references. */
  for (i = 0; i < file->ext_count; i++) {
    CHK(resolve_fielddef(ctx, file->package, (upb_fielddef*)&file->exts[i]));
  }

  for (i = 0; i < file->msg_count; i++) {
    const upb_msgdef *m = &file->msgs[i];
    int j;
    for (j = 0; j < m->field_count; j++) {
      CHK(resolve_fielddef(ctx, m->full_name, (upb_fielddef*)&m->fields[j]));
    }
  }

  return true;
 }

static bool upb_symtab_addtotabs(upb_symtab *s, symtab_addctx *ctx,
                                 upb_status *status) {
  const upb_filedef *file = ctx->file;
  upb_alloc *alloc = upb_arena_alloc(s->arena);
  upb_strtable_iter iter;

  CHK_OOM(upb_strtable_insert3(&s->files, file->name, strlen(file->name),
                               upb_value_constptr(file), alloc));

  upb_strtable_begin(&iter, ctx->addtab);
  for (; !upb_strtable_done(&iter); upb_strtable_next(&iter)) {
    const char *key = upb_strtable_iter_key(&iter);
    size_t keylen = upb_strtable_iter_keylength(&iter);
    upb_value value = upb_strtable_iter_value(&iter);
    CHK_OOM(upb_strtable_insert3(&s->syms, key, keylen, value, alloc));
  }

  return true;
}

/* upb_filedef ****************************************************************/

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

int upb_filedef_msgcount(const upb_filedef *f) {
  return f->msg_count;
}

int upb_filedef_depcount(const upb_filedef *f) {
  return f->dep_count;
}

int upb_filedef_enumcount(const upb_filedef *f) {
  return f->enum_count;
}

const upb_filedef *upb_filedef_dep(const upb_filedef *f, int i) {
  return i < 0 || i >= f->dep_count ? NULL : f->deps[i];
}

const upb_msgdef *upb_filedef_msg(const upb_filedef *f, int i) {
  return i < 0 || i >= f->msg_count ? NULL : &f->msgs[i];
}

const upb_enumdef *upb_filedef_enum(const upb_filedef *f, int i) {
  return i < 0 || i >= f->enum_count ? NULL : &f->enums[i];
}

void upb_symtab_free(upb_symtab *s) {
  upb_arena_free(s->arena);
  upb_gfree(s);
}

upb_symtab *upb_symtab_new() {
  upb_symtab *s = upb_gmalloc(sizeof(*s));
  upb_alloc *alloc;

  if (!s) {
    return NULL;
  }

  s->arena = upb_arena_new();
  alloc = upb_arena_alloc(s->arena);

  if (!upb_strtable_init2(&s->syms, UPB_CTYPE_CONSTPTR, alloc) ||
      !upb_strtable_init2(&s->files, UPB_CTYPE_CONSTPTR, alloc)) {
    upb_arena_free(s->arena);
    upb_gfree(s);
    s = NULL;
  }
  return s;
}

const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ?
      unpack_def(v, UPB_DEFTYPE_MSG) : NULL;
}

const upb_msgdef *upb_symtab_lookupmsg2(const upb_symtab *s, const char *sym,
                                        size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->syms, sym, len, &v) ?
      unpack_def(v, UPB_DEFTYPE_MSG) : NULL;
}

const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ?
      unpack_def(v, UPB_DEFTYPE_ENUM) : NULL;
}

const upb_filedef *upb_symtab_lookupfile(const upb_symtab *s, const char *name) {
  upb_value v;
  return upb_strtable_lookup(&s->files, name, &v) ? upb_value_getconstptr(v)
                                                  : NULL;
}

const upb_filedef *upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file_proto,
    upb_status *status) {
  upb_arena *tmparena = upb_arena_new();
  upb_strtable addtab;
  upb_alloc *alloc = upb_arena_alloc(s->arena);
  upb_filedef *file = upb_malloc(alloc, sizeof(*file));
  bool ok;
  symtab_addctx ctx;

  ctx.file = file;
  ctx.symtab = s;
  ctx.alloc = alloc;
  ctx.tmp = upb_arena_alloc(tmparena);
  ctx.addtab = &addtab;
  ctx.status = status;

  ok = file &&
      upb_strtable_init2(&addtab, UPB_CTYPE_CONSTPTR, ctx.tmp) &&
      build_filedef(&ctx, file, file_proto) &&
      upb_symtab_addtotabs(s, &ctx, status);

  upb_arena_free(tmparena);
  return ok ? file : NULL;
}

/* Include here since we want most of this file to be stdio-free. */
#include <stdio.h>

bool _upb_symtab_loaddefinit(upb_symtab *s, const upb_def_init *init) {
  /* Since this function should never fail (it would indicate a bug in upb) we
   * print errors to stderr instead of returning error status to the user. */
  upb_def_init **deps = init->deps;
  google_protobuf_FileDescriptorProto *file;
  upb_arena *arena;
  upb_status status;

  upb_status_clear(&status);

  if (upb_strtable_lookup(&s->files, init->filename, NULL)) {
    return true;
  }

  arena = upb_arena_new();

  for (; *deps; deps++) {
    if (!_upb_symtab_loaddefinit(s, *deps)) goto err;
  }

  file = google_protobuf_FileDescriptorProto_parse(
      init->descriptor.data, init->descriptor.size, arena);

  if (!file) {
    upb_status_seterrf(
        &status,
        "Failed to parse compiled-in descriptor for file '%s'. This should "
        "never happen.",
        init->filename);
    goto err;
  }

  if (!upb_symtab_addfile(s, file, &status)) goto err;

  upb_arena_free(arena);
  return true;

err:
  fprintf(stderr, "Error loading compiled-in descriptor: %s\n",
          upb_status_errmsg(&status));
  upb_arena_free(arena);
  return false;
}

#undef CHK
#undef CHK_OOM
