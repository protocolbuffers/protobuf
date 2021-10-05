/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/def.h"

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "google/protobuf/descriptor.upb.h"
#include "upb/reflection.h"

/* Must be last. */
#include "upb/port_def.inc"

typedef struct {
  size_t len;
  char str[1];  /* Null-terminated string data follows. */
} str_t;

struct upb_fielddef {
  const upb_filedef *file;
  const upb_msgdef *msgdef;
  const char *full_name;
  const char *json_name;
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
  uint16_t index_;
  uint16_t layout_index;
  bool is_extension_;
  bool lazy_;
  bool packed_;
  bool proto3_optional_;
  upb_descriptortype_t type_;
  upb_label_t label_;
};

struct upb_extrange {
  const google_protobuf_ExtensionRangeOptions *opts;
  int32_t start;
  int32_t end;
};

struct upb_msgdef {
  const upb_msglayout *layout;
  const upb_filedef *file;
  const char *full_name;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;
  upb_strtable ntof;

  const upb_extrange *ext_ranges;
  const upb_fielddef *fields;
  const upb_oneofdef *oneofs;
  int field_count;
  int oneof_count;
  int real_oneof_count;
  int ext_range_count;

  /* Is this a map-entry message? */
  bool map_entry;
  bool is_message_set;
  upb_wellknowntype_t well_known_type;
  const upb_fielddef *message_set_ext;
};

struct upb_enumdef {
  const upb_filedef *file;
  const char *full_name;
  upb_strtable ntoi;
  upb_inttable iton;
  const upb_enumvaldef *values;
  int value_count;
  int32_t defaultval;
};

struct upb_enumvaldef {
  const upb_enumdef *enum_;
  const char *full_name;
  int32_t number;
};

struct upb_oneofdef {
  const upb_msgdef *parent;
  const char *full_name;
  int field_count;
  bool synthetic;
  const upb_fielddef **fields;
  upb_strtable ntof;
  upb_inttable itof;
};

struct upb_filedef {
  const char *name;
  const char *package;
  const char *phpprefix;
  const char *phpnamespace;

  const upb_filedef **deps;
  const upb_msgdef *msgs;
  const upb_enumdef *enums;
  const upb_fielddef *exts;
  const upb_msglayout_ext **ext_layouts;
  const upb_symtab *symtab;

  int dep_count;
  int msg_count;
  int enum_count;
  int ext_count;
  upb_syntax_t syntax;
};

struct upb_symtab {
  upb_arena *arena;
  upb_strtable syms;  /* full_name -> packed def ptr */
  upb_strtable files;  /* file_name -> upb_filedef* */
  upb_inttable exts;   /* upb_msglayout_ext* -> upb_fielddef* */
  upb_extreg *extreg;
  size_t bytes_loaded;
};

/* Inside a symtab we store tagged pointers to specific def types. */
typedef enum {
  UPB_DEFTYPE_MASK = 7,

  UPB_DEFTYPE_FIELD = 0,

  /* Only inside symtab table. */
  UPB_DEFTYPE_MSG = 1,
  UPB_DEFTYPE_ENUM = 2,
  UPB_DEFTYPE_ENUMVAL = 3,

  /* Only inside message table. */
  UPB_DEFTYPE_ONEOF = 1,
  UPB_DEFTYPE_FIELD_JSONNAME = 2
} upb_deftype_t;

static upb_deftype_t deftype(upb_value v) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return num & UPB_DEFTYPE_MASK;
}

static const void *unpack_def(upb_value v, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return (num & UPB_DEFTYPE_MASK) == type
             ? (const void *)(num & ~UPB_DEFTYPE_MASK)
             : NULL;
}

static upb_value pack_def(const void *ptr, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)ptr;
  UPB_ASSERT((num & UPB_DEFTYPE_MASK) == 0);
  num |= type;
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

static void upb_status_setoom(upb_status *status) {
  upb_status_seterrmsg(status, "out of memory");
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
  UPB_ASSERT(upb_enumdef_lookupnum(e, e->defaultval));
  return e->defaultval;
}

const upb_enumvaldef *upb_enumdef_lookupname(const upb_enumdef *def,
                                             const char *name, size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&def->ntoi, name, len, &v)
             ? upb_value_getconstptr(v)
             : NULL;
}

const upb_enumvaldef *upb_enumdef_lookupnum(const upb_enumdef *def, int32_t num) {
  upb_value v;
  return upb_inttable_lookup(&def->iton, num, &v) ? upb_value_getconstptr(v)
                                                  : NULL;
}

const upb_enumvaldef *upb_enumdef_value(const upb_enumdef *e, int i) {
  UPB_ASSERT(0 <= i && i < e->value_count);
  return &e->values[i];
}

// Deprecated functions.

int upb_enumdef_numvals(const upb_enumdef *e) {
  return (int)upb_strtable_count(&e->ntoi);
}

void upb_enum_begin(upb_enum_iter *i, const upb_enumdef *e) {
  /* We iterate over the ntoi table, to account for duplicate numbers. */
  upb_strtable_begin(i, &e->ntoi);
}

void upb_enum_next(upb_enum_iter *iter) { upb_strtable_next(iter); }
bool upb_enum_done(upb_enum_iter *iter) { return upb_strtable_done(iter); }

const char *upb_enum_iter_name(upb_enum_iter *iter) {
  return upb_strtable_iter_key(iter).data;
}

int32_t upb_enum_iter_number(upb_enum_iter *iter) {
  return upb_value_getint32(upb_strtable_iter_value(iter));
}


/* upb_enumvaldef *************************************************************/

const upb_enumdef *upb_enumvaldef_enum(const upb_enumvaldef *ev) {
  return ev->enum_;
}

const char *upb_enumvaldef_fullname(const upb_enumvaldef *ev) {
  return ev->full_name;
}

const char *upb_enumvaldef_name(const upb_enumvaldef *ev) {
  return shortdefname(ev->full_name);
}

int32_t upb_enumvaldef_number(const upb_enumvaldef *ev) {
  return ev->number;
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

const char *upb_fielddef_jsonname(const upb_fielddef *f) {
  return f->json_name;
}

const upb_filedef *upb_fielddef_file(const upb_fielddef *f) {
  return f->file;
}

const upb_msgdef *upb_fielddef_containingtype(const upb_fielddef *f) {
  return f->msgdef;
}

const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f) {
  return f->oneof;
}

const upb_oneofdef *upb_fielddef_realcontainingoneof(const upb_fielddef *f) {
  if (!f->oneof || upb_oneofdef_issynthetic(f->oneof)) return NULL;
  return f->oneof;
}

upb_msgval upb_fielddef_default(const upb_fielddef *f) {
  UPB_ASSERT(!upb_fielddef_issubmsg(f));
  upb_msgval ret;
  if (upb_fielddef_isstring(f)) {
    str_t *str = f->defaultval.str;
    if (str) {
      ret.str_val.data = str->str;
      ret.str_val.size = str->len;
    } else {
      ret.str_val.size = 0;
    }
  } else {
    memcpy(&ret, &f->defaultval, 8);
  }
  return ret;
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
  return (int32_t)f->defaultval.sint;
}

uint64_t upb_fielddef_defaultuint64(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_UINT64);
  return f->defaultval.uint;
}

uint32_t upb_fielddef_defaultuint32(const upb_fielddef *f) {
  chkdefaulttype(f, UPB_TYPE_UINT32);
  return (uint32_t)f->defaultval.uint;
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
  return upb_fielddef_type(f) == UPB_TYPE_MESSAGE ? f->sub.msgdef : NULL;
}

const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_ENUM ? f->sub.enumdef : NULL;
}

const upb_msglayout_field *upb_fielddef_layout(const upb_fielddef *f) {
  UPB_ASSERT(!upb_fielddef_isextension(f));
  return &f->msgdef->layout->fields[f->layout_index];
}

const upb_msglayout_ext *_upb_fielddef_extlayout(const upb_fielddef *f) {
  UPB_ASSERT(upb_fielddef_isextension(f));
  return f->file->ext_layouts[f->layout_index];
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
  return upb_fielddef_issubmsg(f) || upb_fielddef_containingoneof(f) ||
         f->file->syntax == UPB_SYNTAX_PROTO2;
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

const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i) {
  upb_value val;
  return upb_inttable_lookup(&m->itof, i, &val) ? upb_value_getconstptr(val)
                                                : NULL;
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
  return *o || *f;  /* False if this was a JSON name. */
}

const upb_fielddef *upb_msgdef_lookupjsonname(const upb_msgdef *m,
                                              const char *name, size_t len) {
  upb_value val;
  const upb_fielddef* f;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  f = unpack_def(val, UPB_DEFTYPE_FIELD);
  if (!f) f = unpack_def(val, UPB_DEFTYPE_FIELD_JSONNAME);

  return f;
}

int upb_msgdef_numfields(const upb_msgdef *m) {
  return m->field_count;
}

int upb_msgdef_numoneofs(const upb_msgdef *m) {
  return m->oneof_count;
}

int upb_msgdef_numrealoneofs(const upb_msgdef *m) {
  return m->real_oneof_count;
}

int upb_msgdef_extrangecount(const upb_msgdef *m) {
  return m->ext_range_count;
}

int upb_msgdef_fieldcount(const upb_msgdef *m) {
  return m->field_count;
}

int upb_msgdef_oneofcount(const upb_msgdef *m) {
  return m->oneof_count;
}

int upb_msgdef_realoneofcount(const upb_msgdef *m) {
  return m->real_oneof_count;
}

const upb_msglayout *upb_msgdef_layout(const upb_msgdef *m) {
  return m->layout;
}

const upb_extrange *upb_msgdef_extrange(const upb_msgdef *m, int i) {
  UPB_ASSERT(i >= 0 && i < m->ext_range_count);
  return &m->ext_ranges[i];
}

const upb_fielddef *upb_msgdef_field(const upb_msgdef *m, int i) {
  UPB_ASSERT(i >= 0 && i < m->field_count);
  return &m->fields[i];
}

const upb_oneofdef *upb_msgdef_oneof(const upb_msgdef *m, int i) {
  UPB_ASSERT(i >= 0 && i < m->oneof_count);
  return &m->oneofs[i];
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

bool upb_msgdef_iswrapper(const upb_msgdef *m) {
  upb_wellknowntype_t type = upb_msgdef_wellknowntype(m);
  return type >= UPB_WELLKNOWN_DOUBLEVALUE &&
         type <= UPB_WELLKNOWN_BOOLVALUE;
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

int upb_oneofdef_fieldcount(const upb_oneofdef *o) {
  return o->field_count;
}

const upb_fielddef *upb_oneofdef_field(const upb_oneofdef *o, int i) {
  UPB_ASSERT(i < o->field_count);
  return o->fields[i];
}

int upb_oneofdef_numfields(const upb_oneofdef *o) {
  return o->field_count;
}

uint32_t upb_oneofdef_index(const upb_oneofdef *o) {
  return o - o->parent->oneofs;
}

bool upb_oneofdef_issynthetic(const upb_oneofdef *o) {
  return o->synthetic;
}

const upb_fielddef *upb_oneofdef_ntof(const upb_oneofdef *o,
                                      const char *name, size_t length) {
  upb_value val;
  return upb_strtable_lookup2(&o->ntof, name, length, &val) ?
      upb_value_getptr(val) : NULL;
}

const upb_fielddef *upb_oneofdef_itof(const upb_oneofdef *o, uint32_t num) {
  upb_value val;
  return upb_inttable_lookup(&o->itof, num, &val) ? upb_value_getptr(val)
                                                  : NULL;
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

const upb_symtab *upb_filedef_symtab(const upb_filedef *f) {
  return f->symtab;
}

void upb_symtab_free(upb_symtab *s) {
  upb_arena_free(s->arena);
  upb_gfree(s);
}

upb_symtab *upb_symtab_new(void) {
  upb_symtab *s = upb_gmalloc(sizeof(*s));

  if (!s) {
    return NULL;
  }

  s->arena = upb_arena_new();
  s->bytes_loaded = 0;

  if (!upb_strtable_init(&s->syms, 32, s->arena) ||
      !upb_strtable_init(&s->files, 4, s->arena) ||
      !upb_inttable_init(&s->exts, s->arena)) {
    goto err;
  }

  s->extreg = upb_extreg_new(s->arena);
  if (!s->extreg) goto err;
  return s;

err:
  upb_arena_free(s->arena);
  upb_gfree(s);
  return NULL;
}

const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ?
      unpack_def(v, UPB_DEFTYPE_MSG) : NULL;
}

static const void *symtab_lookup2(const upb_symtab *s, const char *sym,
                                  size_t size, upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup2(&s->syms, sym, size, &v) ? unpack_def(v, type)
                                                       : NULL;
}

const upb_msgdef *upb_symtab_lookupmsg2(const upb_symtab *s, const char *sym,
                                        size_t len) {
  return symtab_lookup2(s, sym, len, UPB_DEFTYPE_MSG);
}

const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ?
      unpack_def(v, UPB_DEFTYPE_ENUM) : NULL;
}

const upb_enumvaldef *upb_symtab_lookupenumval(const upb_symtab *s,
                                               const char *sym) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v)
             ? unpack_def(v, UPB_DEFTYPE_ENUMVAL)
             : NULL;
}

const upb_fielddef *upb_symtab_lookupext2(const upb_symtab *s, const char *name,
                                          size_t size) {
  upb_value v;
  if (!upb_strtable_lookup2(&s->syms, name, size, &v)) return NULL;

  switch (deftype(v)) {
    case UPB_DEFTYPE_FIELD:
      return unpack_def(v, UPB_DEFTYPE_FIELD);
    case UPB_DEFTYPE_MSG: {
      const upb_msgdef *m = unpack_def(v, UPB_DEFTYPE_MSG);
      return m->message_set_ext;  /* May be NULL if not in MessageeSet. */
    }
    default:
      break;
  }

  return NULL;
}

const upb_fielddef *upb_symtab_lookupext(const upb_symtab *s, const char *sym) {
  return upb_symtab_lookupext2(s, sym, strlen(sym));
}

const upb_filedef *upb_symtab_lookupfile(const upb_symtab *s, const char *name) {
  upb_value v;
  return upb_strtable_lookup(&s->files, name, &v) ? upb_value_getconstptr(v)
                                                  : NULL;
}

const upb_filedef *upb_symtab_lookupfile2(
    const upb_symtab *s, const char *name, size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->files, name, len, &v) ?
      upb_value_getconstptr(v) : NULL;
}

int upb_symtab_filecount(const upb_symtab *s) {
  return (int)upb_strtable_count(&s->files);
}

/* Code to build defs from descriptor protos. *********************************/

/* There is a question of how much validation to do here.  It will be difficult
 * to perfectly match the amount of validation performed by proto2.  But since
 * this code is used to directly build defs from Ruby (for example) we do need
 * to validate important constraints like uniqueness of names and numbers. */

#define CHK_OOM(x) if (!(x)) { symtab_oomerr(ctx); }

typedef struct {
  upb_symtab *symtab;
  upb_filedef *file;              /* File we are building. */
  upb_arena *arena;               /* Allocate defs here. */
  const upb_msglayout_file *layout;  /* NULL if we should build layouts. */
  int enum_count;                 /* Count of enums built so far. */
  int msg_count;                  /* Count of messages built so far. */
  int ext_count;                  /* Count of extensions built so far. */
  upb_status *status;             /* Record errors here. */
  jmp_buf err;                    /* longjmp() on error. */
} symtab_addctx;

UPB_NORETURN UPB_NOINLINE UPB_PRINTF(2, 3)
static void symtab_errf(symtab_addctx *ctx, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_status_vseterrf(ctx->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(ctx->err, 1);
}

UPB_NORETURN UPB_NOINLINE
static void symtab_oomerr(symtab_addctx *ctx) {
  upb_status_setoom(ctx->status);
  UPB_LONGJMP(ctx->err, 1);
}

void *symtab_alloc(symtab_addctx *ctx, size_t bytes) {
  void *ret = upb_arena_malloc(ctx->arena, bytes);
  if (!ret) symtab_oomerr(ctx);
  return ret;
}

static void check_ident(symtab_addctx *ctx, upb_strview name, bool full) {
  const char *str = name.data;
  size_t len = name.size;
  bool start = true;
  size_t i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if (c == '.') {
      if (start || !full) {
        symtab_errf(ctx, "invalid name: unexpected '.' (%.*s)", (int)len, str);
      }
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) {
        symtab_errf(
            ctx,
            "invalid name: path components must start with a letter (%.*s)",
            (int)len, str);
      }
      start = false;
    } else {
      if (!upb_isalphanum(c)) {
        symtab_errf(ctx, "invalid name: non-alphanumeric character (%.*s)",
                    (int)len, str);
      }
    }
  }
  if (start) {
    symtab_errf(ctx, "invalid name: empty part (%.*s)", (int)len, str);
  }
}

static size_t div_round_up(size_t n, size_t d) {
  return (n + d - 1) / d;
}

static size_t upb_msgval_sizeof(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      return 8;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_FLOAT:
      return 4;
    case UPB_TYPE_BOOL:
      return 1;
    case UPB_TYPE_MESSAGE:
      return sizeof(void*);
    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING:
      return sizeof(upb_strview);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fielddefsize(const upb_fielddef *f) {
  if (upb_msgdef_mapentry(upb_fielddef_containingtype(f))) {
    upb_map_entry ent;
    UPB_ASSERT(sizeof(ent.k) == sizeof(ent.v));
    return sizeof(ent.k);
  } else if (upb_fielddef_isseq(f)) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof(upb_fielddef_type(f));
  }
}

static uint32_t upb_msglayout_place(upb_msglayout *l, size_t size) {
  uint32_t ret;

  l->size = UPB_ALIGN_UP(l->size, size);
  ret = l->size;
  l->size += size;
  return ret;
}

static int field_number_cmp(const void *p1, const void *p2) {
  const upb_msglayout_field *f1 = p1;
  const upb_msglayout_field *f2 = p2;
  return f1->number - f2->number;
}

static void assign_layout_indices(const upb_msgdef *m, upb_msglayout *l,
                                  upb_msglayout_field *fields) {
  int i;
  int n = upb_msgdef_numfields(m);
  int dense_below = 0;
  for (i = 0; i < n; i++) {
    upb_fielddef *f = (upb_fielddef*)upb_msgdef_itof(m, fields[i].number);
    UPB_ASSERT(f);
    f->layout_index = i;
    if (i < UINT8_MAX && fields[i].number == i + 1 &&
        (i == 0 || fields[i-1].number == i)) {
      dense_below = i + 1;
    }
  }
  l->dense_below = dense_below;
}

static void fill_fieldlayout(upb_msglayout_field *field, const upb_fielddef *f) {
  field->number = upb_fielddef_number(f);
  field->descriptortype = upb_fielddef_descriptortype(f);

  if (field->descriptortype == UPB_DTYPE_STRING &&
      f->file->syntax == UPB_SYNTAX_PROTO2) {
    /* See TableDescriptorType() in upbc/generator.cc for details and
     * rationale. */
    field->descriptortype = UPB_DTYPE_BYTES;
  }

  if (upb_fielddef_ismap(f)) {
    field->mode = _UPB_MODE_MAP | (_UPB_REP_PTR << _UPB_REP_SHIFT);
  } else if (upb_fielddef_isseq(f)) {
    field->mode = _UPB_MODE_ARRAY | (_UPB_REP_PTR << _UPB_REP_SHIFT);
  } else {
    /* Maps descriptor type -> elem_size_lg2.  */
    static const uint8_t sizes[] = {
        -1,               /* invalid descriptor type */
        _UPB_REP_8BYTE,  /* DOUBLE */
        _UPB_REP_4BYTE,   /* FLOAT */
        _UPB_REP_8BYTE,   /* INT64 */
        _UPB_REP_8BYTE,  /* UINT64 */
        _UPB_REP_4BYTE,   /* INT32 */
        _UPB_REP_8BYTE,  /* FIXED64 */
        _UPB_REP_4BYTE,  /* FIXED32 */
        _UPB_REP_1BYTE,    /* BOOL */
        _UPB_REP_STRVIEW,  /* STRING */
        _UPB_REP_PTR,  /* GROUP */
        _UPB_REP_PTR,  /* MESSAGE */
        _UPB_REP_STRVIEW,  /* BYTES */
        _UPB_REP_4BYTE,  /* UINT32 */
        _UPB_REP_4BYTE,    /* ENUM */
        _UPB_REP_4BYTE,   /* SFIXED32 */
        _UPB_REP_8BYTE,   /* SFIXED64 */
        _UPB_REP_4BYTE,   /* SINT32 */
        _UPB_REP_8BYTE,   /* SINT64 */
    };
    field->mode =
        _UPB_MODE_SCALAR | (sizes[field->descriptortype] << _UPB_REP_SHIFT);
  }

  if (upb_fielddef_packed(f)) {
    field->mode |= _UPB_MODE_IS_PACKED;
  }

  if (upb_fielddef_isextension(f)) {
    field->mode |= _UPB_MODE_IS_EXTENSION;
  }
}

/* This function is the dynamic equivalent of message_layout.{cc,h} in upbc.
 * It computes a dynamic layout for all of the fields in |m|. */
static void make_layout(symtab_addctx *ctx, const upb_msgdef *m) {
  upb_msglayout *l = (upb_msglayout*)m->layout;
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  size_t hasbit;
  size_t field_count = upb_msgdef_numfields(m);
  size_t sublayout_count = 0;
  upb_msglayout_sub *subs;
  upb_msglayout_field *fields;

  memset(l, 0, sizeof(*l) + sizeof(_upb_fasttable_entry));

  /* Count sub-messages. */
  for (size_t i = 0; i < field_count; i++) {
    if (upb_fielddef_issubmsg(&m->fields[i])) {
      sublayout_count++;
    }
  }

  fields = symtab_alloc(ctx, field_count * sizeof(*fields));
  subs = symtab_alloc(ctx, sublayout_count * sizeof(*subs));

  l->field_count = upb_msgdef_numfields(m);
  l->fields = fields;
  l->subs = subs;
  l->table_mask = 0;

  if (upb_msgdef_extrangecount(m) > 0) {
    if (m->is_message_set) {
      l->ext = _UPB_MSGEXT_MSGSET;
    } else {
      l->ext = _UPB_MSGEXT_EXTENDABLE;
    }
  } else {
    l->ext = _UPB_MSGEXT_NONE;
  }

  /* TODO(haberman): initialize fast tables so that reflection-based parsing
   * can get the same speeds as linked-in types. */
  l->fasttable[0].field_parser = &fastdecode_generic;
  l->fasttable[0].field_data = 0;

  if (upb_msgdef_mapentry(m)) {
    /* TODO(haberman): refactor this method so this special case is more
     * elegant. */
    const upb_fielddef *key = upb_msgdef_itof(m, 1);
    const upb_fielddef *val = upb_msgdef_itof(m, 2);
    fields[0].number = 1;
    fields[1].number = 2;
    fields[0].mode = _UPB_MODE_SCALAR;
    fields[1].mode = _UPB_MODE_SCALAR;
    fields[0].presence = 0;
    fields[1].presence = 0;
    fields[0].descriptortype = upb_fielddef_descriptortype(key);
    fields[1].descriptortype = upb_fielddef_descriptortype(val);
    fields[0].offset = 0;
    fields[1].offset = sizeof(upb_strview);
    fields[1].submsg_index = 0;

    if (upb_fielddef_type(val) == UPB_TYPE_MESSAGE) {
      subs[0].submsg = upb_fielddef_msgsubdef(val)->layout;
    }

    l->field_count = 2;
    l->size = 2 * sizeof(upb_strview);
    l->size = UPB_ALIGN_UP(l->size, 8);
    return;
  }

  /* Allocate data offsets in three stages:
   *
   * 1. hasbits.
   * 2. regular fields.
   * 3. oneof fields.
   *
   * OPT: There is a lot of room for optimization here to minimize the size.
   */

  /* Allocate hasbits and set basic field attributes. */
  sublayout_count = 0;
  for (upb_msg_field_begin(&it, m), hasbit = 0;
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    upb_fielddef* f = upb_msg_iter_field(&it);
    upb_msglayout_field *field = &fields[upb_fielddef_index(f)];

    fill_fieldlayout(field, f);

    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subm = upb_fielddef_msgsubdef(f);
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].submsg = subm->layout;
    }

    if (upb_fielddef_haspresence(f) && !upb_fielddef_realcontainingoneof(f)) {
      /* We don't use hasbit 0, so that 0 can indicate "no presence" in the
       * table. This wastes one hasbit, but we don't worry about it for now. */
      field->presence = ++hasbit;
    } else {
      field->presence = 0;
    }
  }

  /* Account for space used by hasbits. */
  l->size = div_round_up(hasbit, 8);

  /* Allocate non-oneof fields. */
  for (upb_msg_field_begin(&it, m); !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    size_t field_size = upb_msg_fielddefsize(f);
    size_t index = upb_fielddef_index(f);

    if (upb_fielddef_realcontainingoneof(f)) {
      /* Oneofs are handled separately below. */
      continue;
    }

    fields[index].offset = upb_msglayout_place(l, field_size);
  }

  /* Allocate oneof fields.  Each oneof field consists of a uint32 for the case
   * and space for the actual data. */
  for (upb_msg_oneof_begin(&oit, m); !upb_msg_oneof_done(&oit);
       upb_msg_oneof_next(&oit)) {
    const upb_oneofdef* o = upb_msg_iter_oneof(&oit);
    upb_oneof_iter fit;

    size_t case_size = sizeof(uint32_t);  /* Could potentially optimize this. */
    size_t field_size = 0;
    uint32_t case_offset;
    uint32_t data_offset;

    if (upb_oneofdef_issynthetic(o)) continue;

    /* Calculate field size: the max of all field sizes. */
    for (upb_oneof_begin(&fit, o);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* f = upb_oneof_iter_field(&fit);
      field_size = UPB_MAX(field_size, upb_msg_fielddefsize(f));
    }

    /* Align and allocate case offset. */
    case_offset = upb_msglayout_place(l, case_size);
    data_offset = upb_msglayout_place(l, field_size);

    for (upb_oneof_begin(&fit, o);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* f = upb_oneof_iter_field(&fit);
      fields[upb_fielddef_index(f)].offset = data_offset;
      fields[upb_fielddef_index(f)].presence = ~case_offset;
    }
  }

  /* Size of the entire structure should be a multiple of its greatest
   * alignment.  TODO: track overall alignment for real? */
  l->size = UPB_ALIGN_UP(l->size, 8);

  /* Sort fields by number. */
  qsort(fields, upb_msgdef_numfields(m), sizeof(*fields), field_number_cmp);
  assign_layout_indices(m, l, fields);
}

static char *strviewdup(symtab_addctx *ctx, upb_strview view) {
  char *ret = upb_strdup2(view.data, view.size, ctx->arena);
  CHK_OOM(ret);
  return ret;
}

static bool streql2(const char *a, size_t n, const char *b) {
  return n == strlen(b) && memcmp(a, b, n) == 0;
}

static bool streql_view(upb_strview view, const char *b) {
  return streql2(view.data, view.size, b);
}

static const char *makefullname(symtab_addctx *ctx, const char *prefix,
                                upb_strview name) {
  if (prefix) {
    /* ret = prefix + '.' + name; */
    size_t n = strlen(prefix);
    char *ret = symtab_alloc(ctx, n + name.size + 2);
    strcpy(ret, prefix);
    ret[n] = '.';
    memcpy(&ret[n + 1], name.data, name.size);
    ret[n + 1 + name.size] = '\0';
    return ret;
  } else {
    return strviewdup(ctx, name);
  }
}

static void finalize_oneofs(symtab_addctx *ctx, upb_msgdef *m) {
  int i;
  int synthetic_count = 0;
  upb_oneofdef *mutable_oneofs = (upb_oneofdef*)m->oneofs;

  for (i = 0; i < m->oneof_count; i++) {
    upb_oneofdef *o = &mutable_oneofs[i];

    if (o->synthetic && o->field_count != 1) {
      symtab_errf(ctx, "Synthetic oneofs must have one field, not %d: %s",
                  o->field_count, upb_oneofdef_name(o));
    }

    if (o->synthetic) {
      synthetic_count++;
    } else if (synthetic_count != 0) {
      symtab_errf(ctx, "Synthetic oneofs must be after all other oneofs: %s",
                  upb_oneofdef_name(o));
    }

    o->fields = symtab_alloc(ctx, sizeof(upb_fielddef *) * o->field_count);
    o->field_count = 0;
  }

  for (i = 0; i < m->field_count; i++) {
    const upb_fielddef *f = &m->fields[i];
    upb_oneofdef *o = (upb_oneofdef*)f->oneof;
    if (o) {
      o->fields[o->field_count++] = f;
    }
  }

  m->real_oneof_count = m->oneof_count - synthetic_count;
}

size_t getjsonname(const char *name, char *buf, size_t len) {
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

static char* makejsonname(symtab_addctx *ctx, const char* name) {
  size_t size = getjsonname(name, NULL, 0);
  char* json_name = symtab_alloc(ctx, size);
  getjsonname(name, json_name, size);
  return json_name;
}

static void symtab_add(symtab_addctx *ctx, const char *name, upb_value v) {
  // TODO: table should support an operation "tryinsert" to avoid the double
  // lookup.
  if (upb_strtable_lookup(&ctx->symtab->syms, name, NULL)) {
    symtab_errf(ctx, "duplicate symbol '%s'", name);
  }
  size_t len = strlen(name);
  CHK_OOM(upb_strtable_insert(&ctx->symtab->syms, name, len, v,
                              ctx->symtab->arena));
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static const void *symtab_resolve(symtab_addctx *ctx, const upb_fielddef *f,
                                  const char *base, upb_strview sym,
                                  upb_deftype_t type) {
  const upb_strtable *t = &ctx->symtab->syms;
  if(sym.size == 0) goto notfound;
  if(sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    upb_value v;
    if (!upb_strtable_lookup2(t, sym.data + 1, sym.size - 1, &v)) {
      goto notfound;
    }

    const void *ret = unpack_def(v, type);
    if (!ret) {
      symtab_errf(ctx, "type mismatch when resolving field %s, name %s",
                  f->full_name, sym.data);
    }
    return ret;
  } else {
    /* Remove components from base until we find an entry or run out.
     * TODO: This branch is totally broken, but currently not used. */
    (void)base;
    UPB_ASSERT(false);
    goto notfound;
  }

notfound:
  symtab_errf(ctx, "couldn't resolve name '" UPB_STRVIEW_FORMAT "'",
              UPB_STRVIEW_ARGS(sym));
}

static void create_oneofdef(
    symtab_addctx *ctx, upb_msgdef *m,
    const google_protobuf_OneofDescriptorProto *oneof_proto) {
  upb_oneofdef *o;
  upb_strview name = google_protobuf_OneofDescriptorProto_name(oneof_proto);
  upb_value v;

  o = (upb_oneofdef*)&m->oneofs[m->oneof_count++];
  o->parent = m;
  o->full_name = makefullname(ctx, m->full_name, name);
  o->field_count = 0;
  o->synthetic = false;

  v = pack_def(o, UPB_DEFTYPE_ONEOF);
  symtab_add(ctx, o->full_name, v);
  CHK_OOM(upb_strtable_insert(&m->ntof, name.data, name.size, v, ctx->arena));

  CHK_OOM(upb_inttable_init(&o->itof, ctx->arena));
  CHK_OOM(upb_strtable_init(&o->ntof, 4, ctx->arena));
}

static str_t *newstr(symtab_addctx *ctx, const char *data, size_t len) {
  str_t *ret = symtab_alloc(ctx, sizeof(*ret) + len);
  if (!ret) return NULL;
  ret->len = len;
  if (len) memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static void parse_default(symtab_addctx *ctx, const char *str, size_t len,
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
        symtab_errf(ctx, "Default too long: %.*s", (int)len, str);
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
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_ENUM: {
      const upb_enumdef *e = f->sub.enumdef;
      const upb_enumvaldef *ev = upb_enumdef_lookupname(e, str, len);
      if (!ev) {
        goto invalid;
      }
      f->defaultval.sint = ev->number;
      break;
    }
    case UPB_TYPE_INT64: {
      long long val = strtoll(str, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(str, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.uint = val;
      break;
    }
    case UPB_TYPE_UINT64: {
      unsigned long long val = strtoull(str, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.uint = val;
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(str, &end);
      if (errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.dbl = val;
      break;
    }
    case UPB_TYPE_FLOAT: {
      float val = strtof(str, &end);
      if (errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.flt = val;
      break;
    }
    case UPB_TYPE_BOOL: {
      if (streql2(str, len, "false")) {
        f->defaultval.boolean = false;
      } else if (streql2(str, len, "true")) {
        f->defaultval.boolean = true;
      } else {
      }
      break;
    }
    case UPB_TYPE_STRING:
      f->defaultval.str = newstr(ctx, str, len);
      break;
    case UPB_TYPE_BYTES:
      /* XXX: need to interpret the C-escaped value. */
      f->defaultval.str = newstr(ctx, str, len);
      break;
    case UPB_TYPE_MESSAGE:
      /* Should not have a default value. */
      symtab_errf(ctx, "Message should not have a default (%s)",
                  upb_fielddef_fullname(f));
  }

  return;

invalid:
  symtab_errf(ctx, "Invalid default '%.*s' for field %s", (int)len, str,
              upb_fielddef_fullname(f));
}

static void set_default_default(symtab_addctx *ctx, upb_fielddef *f) {
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
      f->defaultval.str = newstr(ctx, NULL, 0);
      break;
    case UPB_TYPE_BOOL:
      f->defaultval.boolean = false;
      break;
    case UPB_TYPE_MESSAGE:
      break;
  }
}

static void create_fielddef(
    symtab_addctx *ctx, const char *prefix, upb_msgdef *m,
    const google_protobuf_FieldDescriptorProto *field_proto) {
  upb_fielddef *f;
  const google_protobuf_FieldOptions *options;
  upb_strview name;
  const char *full_name;
  const char *json_name;
  const char *shortname;
  uint32_t field_number;

  if (!google_protobuf_FieldDescriptorProto_has_name(field_proto)) {
    symtab_errf(ctx, "field has no name (%s)", upb_msgdef_fullname(m));
  }

  name = google_protobuf_FieldDescriptorProto_name(field_proto);
  check_ident(ctx, name, false);
  full_name = makefullname(ctx, prefix, name);
  shortname = shortdefname(full_name);

  if (google_protobuf_FieldDescriptorProto_has_json_name(field_proto)) {
    json_name = strviewdup(
        ctx, google_protobuf_FieldDescriptorProto_json_name(field_proto));
  } else {
    json_name = makejsonname(ctx, shortname);
  }

  field_number = google_protobuf_FieldDescriptorProto_number(field_proto);

  if (field_number == 0 || field_number > UPB_MAX_FIELDNUMBER) {
    symtab_errf(ctx, "invalid field number (%u)", field_number);
  }

  if (m) {
    /* direct message field. */
    upb_value v, field_v, json_v;
    size_t json_size;

    f = (upb_fielddef*)&m->fields[m->field_count];
    f->index_ = m->field_count++;
    f->msgdef = m;
    f->is_extension_ = false;

    if (upb_strtable_lookup(&m->ntof, shortname, NULL)) {
      symtab_errf(ctx, "duplicate field name (%s)", shortname);
    }

    if (upb_strtable_lookup(&m->ntof, json_name, NULL)) {
      symtab_errf(ctx, "duplicate json_name (%s)", json_name);
    }

    if (upb_inttable_lookup(&m->itof, field_number, NULL)) {
      symtab_errf(ctx, "duplicate field number (%u)", field_number);
    }

    field_v = pack_def(f, UPB_DEFTYPE_FIELD);
    json_v = pack_def(f, UPB_DEFTYPE_FIELD_JSONNAME);
    v = upb_value_constptr(f);
    json_size = strlen(json_name);

    CHK_OOM(upb_strtable_insert(&m->ntof, name.data, name.size, field_v,
                                ctx->arena));
    CHK_OOM(upb_inttable_insert(&m->itof, field_number, v, ctx->arena));

    if (strcmp(shortname, json_name) != 0) {
      upb_strtable_insert(&m->ntof, json_name, json_size, json_v, ctx->arena);
    }

    if (ctx->layout) {
      const upb_msglayout_field *fields = m->layout->fields;
      int count = m->layout->field_count;
      bool found = false;
      for (int i = 0; i < count; i++) {
        if (fields[i].number == field_number) {
          f->layout_index = i;
          found = true;
          break;
        }
      }
      UPB_ASSERT(found);
    }
  } else {
    /* extension field. */
    uint16_t layout_index = ctx->ext_count++;
    f = (upb_fielddef*)&ctx->file->exts[layout_index];
    f->layout_index = layout_index;
    f->is_extension_ = true;
    symtab_add(ctx, full_name, pack_def(f, UPB_DEFTYPE_FIELD));
    if (ctx->layout) {
      UPB_ASSERT(ctx->file->ext_layouts[f->layout_index]->field.number ==
                 field_number);
    }
  }

  f->full_name = full_name;
  f->json_name = json_name;
  f->file = ctx->file;
  f->type_ = (int)google_protobuf_FieldDescriptorProto_type(field_proto);
  f->label_ = (int)google_protobuf_FieldDescriptorProto_label(field_proto);
  f->number_ = field_number;
  f->oneof = NULL;
  f->proto3_optional_ =
      google_protobuf_FieldDescriptorProto_proto3_optional(field_proto);

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (f->label_ == UPB_LABEL_REQUIRED && f->file->syntax == UPB_SYNTAX_PROTO3) {
    symtab_errf(ctx, "proto3 fields cannot be required (%s)", f->full_name);
  }

  if (google_protobuf_FieldDescriptorProto_has_oneof_index(field_proto)) {
    int oneof_index =
        google_protobuf_FieldDescriptorProto_oneof_index(field_proto);
    upb_oneofdef *oneof;
    upb_value v = upb_value_constptr(f);

    if (upb_fielddef_label(f) != UPB_LABEL_OPTIONAL) {
      symtab_errf(ctx, "fields in oneof must have OPTIONAL label (%s)",
                  f->full_name);
    }

    if (!m) {
      symtab_errf(ctx, "oneof_index provided for extension field (%s)",
                  f->full_name);
    }

    if (oneof_index >= m->oneof_count) {
      symtab_errf(ctx, "oneof_index out of range (%s)", f->full_name);
    }

    oneof = (upb_oneofdef *)&m->oneofs[oneof_index];
    f->oneof = oneof;

    oneof->field_count++;
    if (f->proto3_optional_) {
      oneof->synthetic = true;
    }
    CHK_OOM(upb_inttable_insert(&oneof->itof, f->number_, v, ctx->arena));
    CHK_OOM(
        upb_strtable_insert(&oneof->ntof, name.data, name.size, v, ctx->arena));
  } else {
    f->oneof = NULL;
    if (f->proto3_optional_) {
      symtab_errf(ctx, "field with proto3_optional was not in a oneof (%s)",
                  f->full_name);
    }
  }

  options = google_protobuf_FieldDescriptorProto_has_options(field_proto) ?
    google_protobuf_FieldDescriptorProto_options(field_proto) : NULL;

  if (options && google_protobuf_FieldOptions_has_packed(options)) {
    f->packed_ = google_protobuf_FieldOptions_packed(options);
  } else {
    /* Repeated fields default to packed for proto3 only. */
    f->packed_ = upb_fielddef_isprimitive(f) &&
        f->label_ == UPB_LABEL_REPEATED && f->file->syntax == UPB_SYNTAX_PROTO3;
  }

  if (options) {
    f->lazy_ = google_protobuf_FieldOptions_lazy(options);
  } else {
    f->lazy_ = false;
  }
}

static void create_enumdef(
    symtab_addctx *ctx, const char *prefix,
    const google_protobuf_EnumDescriptorProto *enum_proto) {
  upb_enumdef *e;
  const google_protobuf_EnumValueDescriptorProto *const *values;
  upb_strview name;
  size_t i, n;

  name = google_protobuf_EnumDescriptorProto_name(enum_proto);
  check_ident(ctx, name, false);

  e = (upb_enumdef*)&ctx->file->enums[ctx->enum_count++];
  e->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, e->full_name, pack_def(e, UPB_DEFTYPE_ENUM));

  values = google_protobuf_EnumDescriptorProto_value(enum_proto, &n);
  CHK_OOM(upb_strtable_init(&e->ntoi, n, ctx->arena));
  CHK_OOM(upb_inttable_init(&e->iton, ctx->arena));

  e->file = ctx->file;
  e->defaultval = 0;
  e->value_count = n;
  e->values = symtab_alloc(ctx, sizeof(*e->values) * n);

  if (n == 0) {
    symtab_errf(ctx, "enums must contain at least one value (%s)",
                e->full_name);
  }

  for (i = 0; i < n; i++) {
    const google_protobuf_EnumValueDescriptorProto *val_proto = values[i];
    upb_enumvaldef *val = (upb_enumvaldef*)&e->values[i];
    upb_strview name = google_protobuf_EnumValueDescriptorProto_name(val_proto);
    upb_value v = upb_value_constptr(val);

    val->enum_ = e;
    val->full_name = makefullname(ctx, prefix, name);
    val->number = google_protobuf_EnumValueDescriptorProto_number(val_proto);
    symtab_add(ctx, val->full_name, pack_def(val, UPB_DEFTYPE_ENUMVAL));

    if (i == 0 && e->file->syntax == UPB_SYNTAX_PROTO3 && val->number != 0) {
      symtab_errf(ctx, "for proto3, the first enum value must be zero (%s)",
                  e->full_name);
    }

    CHK_OOM(upb_strtable_insert(&e->ntoi, name.data, name.size, v, ctx->arena));

    // Multiple enumerators can have the same number, first one wins.
    if (!upb_inttable_lookup(&e->iton, val->number, NULL)) {
      CHK_OOM(upb_inttable_insert(&e->iton, val->number, v, ctx->arena));
    }
  }

  upb_inttable_compact(&e->iton, ctx->arena);
}

static void create_msgdef(symtab_addctx *ctx, const char *prefix,
                          const google_protobuf_DescriptorProto *msg_proto) {
  upb_msgdef *m;
  const google_protobuf_MessageOptions *options;
  const google_protobuf_OneofDescriptorProto *const *oneofs;
  const google_protobuf_FieldDescriptorProto *const *fields;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_DescriptorProto *const *msgs;
  const google_protobuf_DescriptorProto_ExtensionRange *const *ext_ranges;
  size_t i, n_oneof, n_field, n_ext_range, n;
  upb_strview name;

  name = google_protobuf_DescriptorProto_name(msg_proto);
  check_ident(ctx, name, false);

  int msg_index = ctx->msg_count;
  m = (upb_msgdef*)&ctx->file->msgs[msg_index];
  m->full_name = makefullname(ctx, prefix, name);
  ctx->msg_count++;
  symtab_add(ctx, m->full_name, pack_def(m, UPB_DEFTYPE_MSG));

  oneofs = google_protobuf_DescriptorProto_oneof_decl(msg_proto, &n_oneof);
  fields = google_protobuf_DescriptorProto_field(msg_proto, &n_field);
  ext_ranges =
      google_protobuf_DescriptorProto_extension_range(msg_proto, &n_ext_range);

  CHK_OOM(upb_inttable_init(&m->itof, ctx->arena));
  CHK_OOM(upb_strtable_init(&m->ntof, n_oneof + n_field, ctx->arena));

  m->file = ctx->file;
  m->map_entry = false;
  m->is_message_set = false;
  m->message_set_ext = NULL;

  options = google_protobuf_DescriptorProto_options(msg_proto);

  if (options) {
    m->map_entry = google_protobuf_MessageOptions_map_entry(options);
    m->is_message_set =
        google_protobuf_MessageOptions_message_set_wire_format(options);
  }

  if (ctx->layout) {
    /* create_fielddef() below depends on this being set. */
    m->layout = ctx->layout->msgs[msg_index];
    UPB_ASSERT(n_field == m->layout->field_count);
  } else {
    /* Allocate now (to allow cross-linking), populate later. */
    m->layout = symtab_alloc(
        ctx, sizeof(*m->layout) + sizeof(_upb_fasttable_entry));
  }

  m->oneof_count = 0;
  m->oneofs = symtab_alloc(ctx, sizeof(*m->oneofs) * n_oneof);
  for (i = 0; i < n_oneof; i++) {
    create_oneofdef(ctx, m, oneofs[i]);
  }

  m->field_count = 0;
  m->fields = symtab_alloc(ctx, sizeof(*m->fields) * n_field);
  for (i = 0; i < n_field; i++) {
    create_fielddef(ctx, m->full_name, m, fields[i]);
  }

  m->ext_range_count = n_ext_range;
  m->ext_ranges = symtab_alloc(ctx, sizeof(*m->ext_ranges) * n_ext_range);
  for (i = 0; i < n_ext_range; i++) {
    const google_protobuf_DescriptorProto_ExtensionRange *r = ext_ranges[i];
    upb_extrange *r_def = (upb_extrange*)&m->ext_ranges[i];
    r_def->start = google_protobuf_DescriptorProto_ExtensionRange_start(r);
    r_def->end = google_protobuf_DescriptorProto_ExtensionRange_end(r);
  }

  finalize_oneofs(ctx, m);
  assign_msg_wellknowntype(m);
  upb_inttable_compact(&m->itof, ctx->arena);

  /* This message is built.  Now build nested messages and enums. */

  enums = google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    create_enumdef(ctx, m->full_name, enums[i]);
  }

  fields = google_protobuf_DescriptorProto_extension(msg_proto, &n);
  for (i = 0; i < n; i++) {
    create_fielddef(ctx, m->full_name, NULL, fields[i]);
  }

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    create_msgdef(ctx, m->full_name, msgs[i]);
  }
}

static void count_types_in_msg(const google_protobuf_DescriptorProto *msg_proto,
                               upb_filedef *file) {
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;

  file->msg_count++;

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (i = 0; i < n; i++) {
    count_types_in_msg(msgs[i], file);
  }

  google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  file->enum_count += n;

  google_protobuf_DescriptorProto_extension(msg_proto, &n);
  file->ext_count += n;
}

static void count_types_in_file(
    const google_protobuf_FileDescriptorProto *file_proto,
    upb_filedef *file) {
  const google_protobuf_DescriptorProto *const *msgs;
  size_t i, n;

  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    count_types_in_msg(msgs[i], file);
  }

  google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  file->enum_count += n;

  google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->ext_count += n;
}

static void resolve_fielddef(symtab_addctx *ctx, const char *prefix,
                             upb_fielddef *f) {
  upb_strview name;
  const google_protobuf_FieldDescriptorProto *field_proto = f->sub.unresolved;

  if (f->is_extension_) {
    if (!google_protobuf_FieldDescriptorProto_has_extendee(field_proto)) {
      symtab_errf(ctx, "extension for field '%s' had no extendee",
                  f->full_name);
    }

    name = google_protobuf_FieldDescriptorProto_extendee(field_proto);
    f->msgdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_MSG);

    const upb_msglayout_ext *ext = ctx->file->ext_layouts[f->layout_index];
    if (ctx->layout) {
      UPB_ASSERT(upb_fielddef_number(f) == ext->field.number);
    } else {
      upb_msglayout_ext *mut_ext = (upb_msglayout_ext*)ext;
      fill_fieldlayout(&mut_ext->field, f);
      mut_ext->field.presence = 0;
      mut_ext->field.offset = 0;
      mut_ext->field.submsg_index = 0;
      mut_ext->extendee = f->msgdef->layout;
      mut_ext->sub.submsg = f->sub.msgdef->layout;
    }

    CHK_OOM(upb_inttable_insert(&ctx->symtab->exts, (uintptr_t)ext,
                                upb_value_constptr(f), ctx->arena));
  }

  if ((upb_fielddef_issubmsg(f) || f->type_ == UPB_DESCRIPTOR_TYPE_ENUM) &&
      !google_protobuf_FieldDescriptorProto_has_type_name(field_proto)) {
    symtab_errf(ctx, "field '%s' is missing type name", f->full_name);
  }

  name = google_protobuf_FieldDescriptorProto_type_name(field_proto);

  if (upb_fielddef_issubmsg(f)) {
    f->sub.msgdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_MSG);
    if (f->is_extension_ && f->msgdef->is_message_set &&
        f->file == f->msgdef->file) {
      // TODO: When defs are restructured to follow message nesting, we can make
      // this check more robust.  The actual rules for what make something
      // qualify as a MessageSet item are more strict.
      ((upb_msgdef*)f->sub.msgdef)->message_set_ext = f;
    }
  } else if (f->type_ == UPB_DESCRIPTOR_TYPE_ENUM) {
    f->sub.enumdef = symtab_resolve(ctx, f, prefix, name, UPB_DEFTYPE_ENUM);
  }

  /* Have to delay resolving of the default value until now because of the enum
   * case, since enum defaults are specified with a label. */
  if (google_protobuf_FieldDescriptorProto_has_default_value(field_proto)) {
    upb_strview defaultval =
        google_protobuf_FieldDescriptorProto_default_value(field_proto);

    if (f->file->syntax == UPB_SYNTAX_PROTO3) {
      symtab_errf(ctx, "proto3 fields cannot have explicit defaults (%s)",
                  f->full_name);
    }

    if (upb_fielddef_issubmsg(f)) {
      symtab_errf(ctx, "message fields cannot have explicit defaults (%s)",
                  f->full_name);
    }

    parse_default(ctx, defaultval.data, defaultval.size, f);
  } else {
    set_default_default(ctx, f);
  }
}

static void build_filedef(
    symtab_addctx *ctx, upb_filedef *file,
    const google_protobuf_FileDescriptorProto *file_proto) {
  const google_protobuf_FileOptions *file_options_proto;
  const google_protobuf_DescriptorProto *const *msgs;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_FieldDescriptorProto *const *exts;
  const upb_strview* strs;
  size_t i, n;

  file->symtab = ctx->symtab;

  /* One pass to count and allocate. */
  file->msg_count = 0;
  file->enum_count = 0;
  file->ext_count = 0;
  count_types_in_file(file_proto, file);
  file->msgs = symtab_alloc(ctx, sizeof(*file->msgs) * file->msg_count);
  file->enums = symtab_alloc(ctx, sizeof(*file->enums) * file->enum_count);
  file->exts = symtab_alloc(ctx, sizeof(*file->exts) * file->ext_count);

  ctx->msg_count = 0;
  ctx->enum_count = 0;
  ctx->ext_count = 0;

  if (ctx->layout) {
    /* We are using the ext layouts that were passed in. */
    file->ext_layouts = ctx->layout->exts;
    if (ctx->layout->ext_count != file->ext_count) {
      symtab_errf(ctx, "Extension count did not match layout (%d vs %d)",
                  ctx->layout->ext_count, file->ext_count);
    }
  } else {
    /* We are building ext layouts from scratch. */
    file->ext_layouts =
        symtab_alloc(ctx, sizeof(*file->ext_layouts) * file->ext_count);
    upb_msglayout_ext *ext = symtab_alloc(ctx, sizeof(*ext) * file->ext_count);
    for (int i = 0; i < file->ext_count; i++) {
      file->ext_layouts[i] = &ext[i];
    }
  }

  if (!google_protobuf_FileDescriptorProto_has_name(file_proto)) {
    symtab_errf(ctx, "File has no name");
  }

  file->name =
      strviewdup(ctx, google_protobuf_FileDescriptorProto_name(file_proto));
  file->phpprefix = NULL;
  file->phpnamespace = NULL;

  if (google_protobuf_FileDescriptorProto_has_package(file_proto)) {
    upb_strview package =
        google_protobuf_FileDescriptorProto_package(file_proto);
    check_ident(ctx, package, true);
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
      symtab_errf(ctx, "Invalid syntax '" UPB_STRVIEW_FORMAT "'",
                  UPB_STRVIEW_ARGS(syntax));
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
  file->deps = symtab_alloc(ctx, sizeof(*file->deps) * n);

  for (i = 0; i < n; i++) {
    upb_strview dep_name = strs[i];
    upb_value v;
    if (!upb_strtable_lookup2(&ctx->symtab->files, dep_name.data,
                              dep_name.size, &v)) {
      symtab_errf(ctx,
                  "Depends on file '" UPB_STRVIEW_FORMAT
                  "', but it has not been loaded",
                  UPB_STRVIEW_ARGS(dep_name));
    }
    file->deps[i] = upb_value_getconstptr(v);
  }

  /* Create messages. */
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    create_msgdef(ctx, file->package, msgs[i]);
  }

  /* Create enums. */
  enums = google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  for (i = 0; i < n; i++) {
    create_enumdef(ctx, file->package, enums[i]);
  }

  /* Create extensions. */
  exts = google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  for (i = 0; i < n; i++) {
    create_fielddef(ctx, file->package, NULL, exts[i]);
  }

  UPB_ASSERT(ctx->ext_count == file->ext_count);

  /* Now that all names are in the table, build layouts and resolve refs. */
  for (i = 0; i < (size_t)file->ext_count; i++) {
    resolve_fielddef(ctx, file->package, (upb_fielddef*)&file->exts[i]);
  }

  for (i = 0; i < (size_t)file->msg_count; i++) {
    const upb_msgdef *m = &file->msgs[i];
    int j;
    for (j = 0; j < m->field_count; j++) {
      resolve_fielddef(ctx, m->full_name, (upb_fielddef*)&m->fields[j]);
    }
  }

  if (!ctx->layout) {
    for (i = 0; i < (size_t)file->msg_count; i++) {
      const upb_msgdef *m = &file->msgs[i];
      make_layout(ctx, m);
    }
  }

  CHK_OOM(
      _upb_extreg_add(ctx->symtab->extreg, file->ext_layouts, file->ext_count));
}

static void remove_filedef(symtab_addctx *ctx, upb_symtab *s, upb_filedef *file) {
  int i;
  for (i = 0; i < ctx->msg_count; i++) {
    const char *name = file->msgs[i].full_name;
    upb_strtable_remove(&s->syms, name, strlen(name), NULL);
  }
  for (i = 0; i < ctx->enum_count; i++) {
    const char *name = file->enums[i].full_name;
    upb_strtable_remove(&s->syms, name, strlen(name), NULL);
  }
  for (i = 0; i < ctx->ext_count; i++) {
    const char *name = file->exts[i].full_name;
    upb_strtable_remove(&s->syms, name, strlen(name), NULL);
  }
}

static const upb_filedef *_upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file_proto,
    const upb_msglayout_file *layout, upb_status *status) {
  symtab_addctx ctx;
  upb_strview name = google_protobuf_FileDescriptorProto_name(file_proto);

  if (upb_strtable_lookup2(&s->files, name.data, name.size, NULL)) {
    upb_status_seterrf(status, "duplicate file name (%.*s)",
                       UPB_STRVIEW_ARGS(name));
    return NULL;
  }

  ctx.symtab = s;
  ctx.layout = layout;
  ctx.status = status;
  ctx.file = NULL;
  ctx.arena = upb_arena_new();

  if (!ctx.arena) {
    upb_status_setoom(status);
    return NULL;
  }

  if (UPB_UNLIKELY(UPB_SETJMP(ctx.err))) {
    UPB_ASSERT(!upb_ok(status));
    if (ctx.file) {
      remove_filedef(&ctx, s, ctx.file);
      ctx.file = NULL;
    }
  } else {
    ctx.file = symtab_alloc(&ctx, sizeof(*ctx.file));
    build_filedef(&ctx, ctx.file, file_proto);
    upb_strtable_insert(&s->files, name.data, name.size,
                        upb_value_constptr(ctx.file), ctx.arena);
    UPB_ASSERT(upb_ok(status));
    upb_arena_fuse(s->arena, ctx.arena);
  }

  upb_arena_free(ctx.arena);
  return ctx.file;
}

const upb_filedef *upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file_proto,
    upb_status *status) {
  return _upb_symtab_addfile(s, file_proto, NULL, status);
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

  file = google_protobuf_FileDescriptorProto_parse_ex(
      init->descriptor.data, init->descriptor.size, NULL, UPB_DECODE_ALIAS,
      arena);
  s->bytes_loaded += init->descriptor.size;

  if (!file) {
    upb_status_seterrf(
        &status,
        "Failed to parse compiled-in descriptor for file '%s'. This should "
        "never happen.",
        init->filename);
    goto err;
  }

  if (!_upb_symtab_addfile(s, file, init->layout, &status)) goto err;

  upb_arena_free(arena);
  return true;

err:
  fprintf(stderr,
          "Error loading compiled-in descriptor for file '%s' (this should "
          "never happen): %s\n",
          init->filename, upb_status_errmsg(&status));
  upb_arena_free(arena);
  return false;
}

size_t _upb_symtab_bytesloaded(const upb_symtab *s) {
  return s->bytes_loaded;
}

upb_arena *_upb_symtab_arena(const upb_symtab *s) {
  return s->arena;
}

const upb_fielddef *_upb_symtab_lookupextfield(const upb_symtab *s,
                                               const upb_msglayout_ext *ext) {
  upb_value v;
  bool ok = upb_inttable_lookup(&s->exts, (uintptr_t)ext, &v);
  UPB_ASSERT(ok);
  return upb_value_getconstptr(v);
}

const upb_extreg *upb_symtab_extreg(const upb_symtab *s) {
  return s->extreg;
}

#undef CHK_OOM
