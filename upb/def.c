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

/* The upb core does not generally have a concept of default instances. However
 * for descriptor options we make an exception since the max size is known and
 * modest (<200 bytes). All types can share a default instance since it is
 * initialized to zeroes. */
static const char opt_default[_UPB_MAXOPT_SIZE] = {0};

struct upb_fielddef {
  const google_protobuf_FieldOptions *opts;
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
  uint16_t layout_index;  /* Index into msgdef->layout->fields or file->exts */
  bool has_default;
  bool is_extension_;
  bool packed_;
  bool proto3_optional_;
  bool has_json_name_;
  upb_descriptortype_t type_;
  upb_label_t label_;
};

struct upb_extrange {
  const google_protobuf_ExtensionRangeOptions *opts;
  int32_t start;
  int32_t end;
};

struct upb_msgdef {
  const google_protobuf_MessageOptions *opts;
  const upb_msglayout *layout;
  const upb_filedef *file;
  const upb_msgdef *containing_type;
  const char *full_name;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;
  upb_strtable ntof;

  /* All nested defs.
   * MEM: We could save some space here by putting nested defs in a contigous
   * region and calculating counts from offets or vice-versa. */
  const upb_fielddef *fields;
  const upb_oneofdef *oneofs;
  const upb_extrange *ext_ranges;
  const upb_msgdef *nested_msgs;
  const upb_enumdef *nested_enums;
  const upb_fielddef *nested_exts;
  int field_count;
  int real_oneof_count;
  int oneof_count;
  int ext_range_count;
  int nested_msg_count;
  int nested_enum_count;
  int nested_ext_count;
  bool in_message_set;
  upb_wellknowntype_t well_known_type;
};

struct upb_enumdef {
  const google_protobuf_EnumOptions *opts;
  const upb_enumlayout *layout;  // Only for proto2.
  const upb_filedef *file;
  const upb_msgdef *containing_type;  // Could be merged with "file".
  const char *full_name;
  upb_strtable ntoi;
  upb_inttable iton;
  const upb_enumvaldef *values;
  int value_count;
  int32_t defaultval;
};

struct upb_enumvaldef {
  const google_protobuf_EnumValueOptions *opts;
  const upb_enumdef *enum_;
  const char *full_name;
  int32_t number;
};

struct upb_oneofdef {
  const google_protobuf_OneofOptions *opts;
  const upb_msgdef *parent;
  const char *full_name;
  int field_count;
  bool synthetic;
  const upb_fielddef **fields;
  upb_strtable ntof;
  upb_inttable itof;
};

struct upb_filedef {
  const google_protobuf_FileOptions *opts;
  const char *name;
  const char *package;

  const upb_filedef **deps;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  const upb_msgdef *top_lvl_msgs;
  const upb_enumdef *top_lvl_enums;
  const upb_fielddef *top_lvl_exts;
  const upb_servicedef *services;
  const upb_msglayout_ext **ext_layouts;
  const upb_symtab *symtab;

  int dep_count;
  int public_dep_count;
  int weak_dep_count;
  int top_lvl_msg_count;
  int top_lvl_enum_count;
  int top_lvl_ext_count;
  int service_count;
  int ext_count;  /* All exts in the file. */
  upb_syntax_t syntax;
};

struct upb_methoddef {
  const google_protobuf_MethodOptions *opts;
  upb_servicedef *service;
  const char *full_name;
  const upb_msgdef *input_type;
  const upb_msgdef *output_type;
  bool client_streaming;
  bool server_streaming;
};

struct upb_servicedef {
  const google_protobuf_ServiceOptions *opts;
  const upb_filedef *file;
  const char *full_name;
  upb_methoddef *methods;
  int method_count;
  int index;
};

struct upb_symtab {
  upb_arena *arena;
  upb_strtable syms;  /* full_name -> packed def ptr */
  upb_strtable files;  /* file_name -> upb_filedef* */
  upb_inttable exts;   /* upb_msglayout_ext* -> upb_fielddef* */
  upb_extreg *extreg;
  size_t bytes_loaded;

  // For compatibility with proto2, we have to accept json_names that conflict
  // with field names or other json_names.  This is very ill-advised, so we only
  // allow this when it is needed (and hopefully these cases can be cleaned up
  // and eliminated.  When this is enabled, the results are not well-defined.
  bool allow_name_conflicts;
};

/* Inside a symtab we store tagged pointers to specific def types. */
typedef enum {
  UPB_DEFTYPE_MASK = 7,

  /* Only inside symtab table. */
  UPB_DEFTYPE_EXT = 0,
  UPB_DEFTYPE_MSG = 1,
  UPB_DEFTYPE_ENUM = 2,
  UPB_DEFTYPE_ENUMVAL = 3,
  UPB_DEFTYPE_SERVICE = 4,

  /* Only inside message table. */
  UPB_DEFTYPE_FIELD = 0,
  UPB_DEFTYPE_ONEOF = 1,
  UPB_DEFTYPE_FIELD_JSONNAME = 2,

  /* Only inside file table. */
  UPB_DEFTYPE_FILE = 0,
  UPB_DEFTYPE_LAYOUT = 1
} upb_deftype_t;

#define FIELD_TYPE_UNSPECIFIED 0

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
static bool upb_isbetween(uint8_t c, uint8_t low, uint8_t high) {
  return c >= low && c <= high;
}

static bool upb_isletter(char c) {
  char lower = c | 0x20;  // Per ASCII this will lowercase a letter.
  return upb_isbetween(lower, 'a', 'z') || c == '_';
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

const google_protobuf_EnumOptions *upb_enumdef_options(const upb_enumdef *e) {
  return e->opts;
}

bool upb_enumdef_hasoptions(const upb_enumdef *e) {
  return e->opts != (void*)opt_default;
}

const char *upb_enumdef_fullname(const upb_enumdef *e) {
  return e->full_name;
}

const char *upb_enumdef_name(const upb_enumdef *e) {
  return shortdefname(e->full_name);
}

const upb_filedef *upb_enumdef_file(const upb_enumdef *e) {
  return e->file;
}

const upb_msgdef *upb_enumdef_containingtype(const upb_enumdef *e) {
  return e->containing_type;
}

int32_t upb_enumdef_default(const upb_enumdef *e) {
  UPB_ASSERT(upb_enumdef_lookupnum(e, e->defaultval));
  return e->defaultval;
}

int upb_enumdef_valuecount(const upb_enumdef *e) {
  return e->value_count;
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

bool upb_enumdef_checknum(const upb_enumdef *e, int32_t num) {
  // We could use upb_enumdef_lookupnum(e, num) != NULL, but we expect this to
  // be faster (especially for small numbers).
  return _upb_enumlayout_checkval(e->layout, num);
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

const google_protobuf_EnumValueOptions *upb_enumvaldef_options(
    const upb_enumvaldef *e) {
  return e->opts;
}

bool upb_enumvaldef_hasoptions(const upb_enumvaldef *e) {
  return e->opts != (void*)opt_default;
}

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

/* upb_extrange ***************************************************************/

const google_protobuf_ExtensionRangeOptions *upb_extrange_options(
    const upb_extrange *r) {
  return r->opts;
}

bool upb_extrange_hasoptions(const upb_extrange *r) {
  return r->opts != (void*)opt_default;
}

int32_t upb_extrange_start(const upb_extrange *e) {
  return e->start;
}

int32_t upb_extrange_end(const upb_extrange *e) {
  return e->end;
}

/* upb_fielddef ***************************************************************/

const google_protobuf_FieldOptions *upb_fielddef_options(
    const upb_fielddef *f) {
  return f->opts;
}

bool upb_fielddef_hasoptions(const upb_fielddef *f) {
  return f->opts != (void*)opt_default;
}

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

bool upb_fielddef_packed(const upb_fielddef *f) {
  return f->packed_;
}

const char *upb_fielddef_name(const upb_fielddef *f) {
  return shortdefname(f->full_name);
}

const char *upb_fielddef_jsonname(const upb_fielddef *f) {
  return f->json_name;
}

bool upb_fielddef_hasjsonname(const upb_fielddef *f) {
  return f->has_json_name_;
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

bool _upb_fielddef_proto3optional(const upb_fielddef *f) {
  return f->proto3_optional_;
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

bool upb_fielddef_hasdefault(const upb_fielddef *f) {
  return f->has_default;
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

const google_protobuf_MessageOptions *upb_msgdef_options(const upb_msgdef *m) {
  return m->opts;
}

bool upb_msgdef_hasoptions(const upb_msgdef *m) {
  return m->opts != (void*)opt_default;
}

const char *upb_msgdef_fullname(const upb_msgdef *m) {
  return m->full_name;
}

const upb_filedef *upb_msgdef_file(const upb_msgdef *m) {
  return m->file;
}

const upb_msgdef *upb_msgdef_containingtype(const upb_msgdef *m) {
  return m->containing_type;
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
                           const upb_fielddef **out_f,
                           const upb_oneofdef **out_o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  const upb_fielddef *f = unpack_def(val, UPB_DEFTYPE_FIELD);
  const upb_oneofdef *o = unpack_def(val, UPB_DEFTYPE_ONEOF);
  if (out_f) *out_f = f;
  if (out_o) *out_o = o;
  return f || o;  /* False if this was a JSON name. */
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

int upb_msgdef_nestedmsgcount(const upb_msgdef *m) {
  return m->nested_msg_count;
}

int upb_msgdef_nestedenumcount(const upb_msgdef *m) {
  return m->nested_enum_count;
}

int upb_msgdef_nestedextcount(const upb_msgdef *m) {
  return m->nested_ext_count;
}

int upb_msgdef_realoneofcount(const upb_msgdef *m) {
  return m->real_oneof_count;
}

const upb_msglayout *upb_msgdef_layout(const upb_msgdef *m) {
  return m->layout;
}

const upb_extrange *upb_msgdef_extrange(const upb_msgdef *m, int i) {
  UPB_ASSERT(0 <= i && i < m->ext_range_count);
  return &m->ext_ranges[i];
}

const upb_fielddef *upb_msgdef_field(const upb_msgdef *m, int i) {
  UPB_ASSERT(0 <= i && i < m->field_count);
  return &m->fields[i];
}

const upb_oneofdef *upb_msgdef_oneof(const upb_msgdef *m, int i) {
  UPB_ASSERT(0 <= i && i < m->oneof_count);
  return &m->oneofs[i];
}

const upb_msgdef *upb_msgdef_nestedmsg(const upb_msgdef *m, int i) {
  UPB_ASSERT(0 <= i && i < m->nested_msg_count);
  return &m->nested_msgs[i];
}

const upb_enumdef *upb_msgdef_nestedenum(const upb_msgdef *m, int i) {
  UPB_ASSERT(0 <= i && i < m->nested_enum_count);
  return &m->nested_enums[i];
}

const upb_fielddef *upb_msgdef_nestedext(const upb_msgdef *m, int i) {
  UPB_ASSERT(0 <= i && i < m->nested_ext_count);
  return &m->nested_exts[i];
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

const google_protobuf_OneofOptions *upb_oneofdef_options(const upb_oneofdef *o) {
  return o->opts;
}

bool upb_oneofdef_hasoptions(const upb_oneofdef *o) {
  return o->opts != (void*)opt_default;
}

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

const google_protobuf_FileOptions *upb_filedef_options(const upb_filedef *f) {
  return f->opts;
}

bool upb_filedef_hasoptions(const upb_filedef *f) {
  return f->opts != (void*)opt_default;
}

const char *upb_filedef_name(const upb_filedef *f) {
  return f->name;
}

const char *upb_filedef_package(const upb_filedef *f) {
  return f->package;
}

upb_syntax_t upb_filedef_syntax(const upb_filedef *f) {
  return f->syntax;
}

int upb_filedef_toplvlmsgcount(const upb_filedef *f) {
  return f->top_lvl_msg_count;
}

int upb_filedef_depcount(const upb_filedef *f) {
  return f->dep_count;
}

int upb_filedef_publicdepcount(const upb_filedef *f) {
  return f->public_dep_count;
}

int upb_filedef_weakdepcount(const upb_filedef *f) {
  return f->weak_dep_count;
}

const int32_t *_upb_filedef_publicdepnums(const upb_filedef *f) {
  return f->public_deps;
}

const int32_t *_upb_filedef_weakdepnums(const upb_filedef *f) {
  return f->weak_deps;
}

int upb_filedef_toplvlenumcount(const upb_filedef *f) {
  return f->top_lvl_enum_count;
}

int upb_filedef_toplvlextcount(const upb_filedef *f) {
  return f->top_lvl_ext_count;
}

int upb_filedef_servicecount(const upb_filedef *f) {
  return f->service_count;
}

const upb_filedef *upb_filedef_dep(const upb_filedef *f, int i) {
  UPB_ASSERT(0 <= i && i < f->dep_count);
  return f->deps[i];
}

const upb_filedef *upb_filedef_publicdep(const upb_filedef *f, int i) {
  UPB_ASSERT(0 <= i && i < f->public_dep_count);
  return f->deps[f->public_deps[i]];
}

const upb_filedef *upb_filedef_weakdep(const upb_filedef *f, int i) {
  UPB_ASSERT(0 <= i && i < f->public_dep_count);
  return f->deps[f->weak_deps[i]];
}

const upb_msgdef *upb_filedef_toplvlmsg(const upb_filedef *f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_msg_count);
  return &f->top_lvl_msgs[i];
}

const upb_enumdef *upb_filedef_toplvlenum(const upb_filedef *f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_enum_count);
  return &f->top_lvl_enums[i];
}

const upb_fielddef *upb_filedef_toplvlext(const upb_filedef *f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_ext_count);
  return &f->top_lvl_exts[i];
}

const upb_servicedef *upb_filedef_service(const upb_filedef *f, int i) {
  UPB_ASSERT(0 <= i && i < f->service_count);
  return &f->services[i];
}

const upb_symtab *upb_filedef_symtab(const upb_filedef *f) {
  return f->symtab;
}

/* upb_methoddef **************************************************************/

const google_protobuf_MethodOptions *upb_methoddef_options(
    const upb_methoddef *m) {
  return m->opts;
}

bool upb_methoddef_hasoptions(const upb_methoddef *m) {
  return m->opts != (void*)opt_default;
}

const char *upb_methoddef_fullname(const upb_methoddef *m) {
  return m->full_name;
}

const char *upb_methoddef_name(const upb_methoddef *m) {
  return shortdefname(m->full_name);
}

const upb_servicedef *upb_methoddef_service(const upb_methoddef *m) {
  return m->service;
}

const upb_msgdef *upb_methoddef_inputtype(const upb_methoddef *m) {
  return m->input_type;
}

const upb_msgdef *upb_methoddef_outputtype(const upb_methoddef *m) {
  return m->output_type;
}

bool upb_methoddef_clientstreaming(const upb_methoddef *m) {
  return m->client_streaming;
}

bool upb_methoddef_serverstreaming(const upb_methoddef *m) {
  return m->server_streaming;
}

/* upb_servicedef *************************************************************/

const google_protobuf_ServiceOptions *upb_servicedef_options(
    const upb_servicedef *s) {
  return s->opts;
}

bool upb_servicedef_hasoptions(const upb_servicedef *s) {
  return s->opts != (void*)opt_default;
}

const char *upb_servicedef_fullname(const upb_servicedef *s) {
  return s->full_name;
}

const char *upb_servicedef_name(const upb_servicedef *s) {
  return shortdefname(s->full_name);
}

int upb_servicedef_index(const upb_servicedef *s) {
  return s->index;
}

const upb_filedef *upb_servicedef_file(const upb_servicedef *s) {
  return s->file;
}

int upb_servicedef_methodcount(const upb_servicedef *s) {
  return s->method_count;
}

const upb_methoddef *upb_servicedef_method(const upb_servicedef *s, int i) {
  return i < 0 || i >= s->method_count ? NULL : &s->methods[i];
}

const upb_methoddef *upb_servicedef_lookupmethod(const upb_servicedef *s,
                                                 const char *name) {
  for (int i = 0; i < s->method_count; i++) {
    if (strcmp(name, upb_methoddef_name(&s->methods[i])) == 0) {
      return &s->methods[i];
    }
  }
  return NULL;
}

/* upb_symtab *****************************************************************/

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
  s->allow_name_conflicts = false;

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

void _upb_symtab_allownameconflicts(upb_symtab *s) {
  s->allow_name_conflicts = true;
}

static const void *symtab_lookup(const upb_symtab *s, const char *sym,
                                 upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ? unpack_def(v, type) : NULL;
}

static const void *symtab_lookup2(const upb_symtab *s, const char *sym,
                                  size_t size, upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup2(&s->syms, sym, size, &v) ? unpack_def(v, type)
                                                       : NULL;
}

const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_MSG);
}

const upb_msgdef *upb_symtab_lookupmsg2(const upb_symtab *s, const char *sym,
                                        size_t len) {
  return symtab_lookup2(s, sym, len, UPB_DEFTYPE_MSG);
}

const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_ENUM);
}

const upb_enumvaldef *upb_symtab_lookupenumval(const upb_symtab *s,
                                               const char *sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_ENUMVAL);
}

const upb_filedef *upb_symtab_lookupfile(const upb_symtab *s,
                                         const char *name) {
  upb_value v;
  return upb_strtable_lookup(&s->files, name, &v)
             ? unpack_def(v, UPB_DEFTYPE_FILE)
             : NULL;
}

const upb_filedef *upb_symtab_lookupfile2(const upb_symtab *s, const char *name,
                                          size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->files, name, len, &v)
             ? unpack_def(v, UPB_DEFTYPE_FILE)
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
      return m->in_message_set ? &m->nested_exts[0] : NULL;
    }
    default:
      break;
  }

  return NULL;
}

const upb_fielddef *upb_symtab_lookupext(const upb_symtab *s, const char *sym) {
  return upb_symtab_lookupext2(s, sym, strlen(sym));
}

const upb_servicedef *upb_symtab_lookupservice(const upb_symtab *s,
                                               const char *name) {
  return symtab_lookup(s, name, UPB_DEFTYPE_SERVICE);
}

const upb_filedef *upb_symtab_lookupfileforsym(const upb_symtab *s,
                                               const char *name) {
  upb_value v;
  // TODO(haberman): non-extension fields and oneofs.
  if (upb_strtable_lookup(&s->syms, name, &v)) {
    switch (deftype(v)) {
      case UPB_DEFTYPE_EXT: {
        const upb_fielddef *f = unpack_def(v, UPB_DEFTYPE_EXT);
        return upb_fielddef_file(f);
      }
      case UPB_DEFTYPE_MSG: {
        const upb_msgdef *m = unpack_def(v, UPB_DEFTYPE_MSG);
        return upb_msgdef_file(m);
      }
      case UPB_DEFTYPE_ENUM: {
        const upb_enumdef *e = unpack_def(v, UPB_DEFTYPE_ENUM);
        return upb_enumdef_file(e);
      }
      case UPB_DEFTYPE_ENUMVAL: {
        const upb_enumvaldef *ev = unpack_def(v, UPB_DEFTYPE_ENUMVAL);
        return upb_enumdef_file(upb_enumvaldef_enum(ev));
      }
      case UPB_DEFTYPE_SERVICE: {
        const upb_servicedef *service = unpack_def(v, UPB_DEFTYPE_SERVICE);
        return upb_servicedef_file(service);
      }
      default:
        UPB_UNREACHABLE();
    }
  }

  const char *last_dot = strrchr(name, '.');
  if (last_dot) {
    const upb_msgdef *parent = upb_symtab_lookupmsg2(s, name, last_dot - name);
    if (parent) {
      const char *shortname = last_dot + 1;
      if (upb_msgdef_lookupname(parent, shortname, strlen(shortname), NULL,
                                NULL)) {
        return upb_msgdef_file(parent);
      }
    }
  }

  return NULL;
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
  upb_arena *tmp_arena;                 /* For temporary allocations. */
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
  if (bytes == 0) return NULL;
  void *ret = upb_arena_malloc(ctx->arena, bytes);
  if (!ret) symtab_oomerr(ctx);
  return ret;
}

// We want to copy the options verbatim into the destination options proto.
// We use serialize+parse as our deep copy.
#define SET_OPTIONS(target, desc_type, options_type, proto)                   \
  if (google_protobuf_##desc_type##_has_options(proto)) {                     \
    size_t size;                                                              \
    char *pb = google_protobuf_##options_type##_serialize(                    \
        google_protobuf_##desc_type##_options(proto), ctx->tmp_arena, &size); \
    CHK_OOM(pb);                                                              \
    target = google_protobuf_##options_type##_parse(pb, size, ctx->arena);    \
    CHK_OOM(target);                                                          \
  } else {                                                                    \
    target = (const google_protobuf_##options_type *)&opt_default;            \
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

static uint32_t upb_msglayout_place(symtab_addctx *ctx, upb_msglayout *l,
                                    size_t size, const upb_msgdef *m) {
  size_t ofs = UPB_ALIGN_UP(l->size, size);
  size_t next = ofs + size;

  if (next > UINT16_MAX) {
    symtab_errf(ctx, "size of message %s exceeded max size of %zu bytes",
                upb_msgdef_fullname(m), (size_t)UINT16_MAX);
  }

  l->size = next;
  return ofs;
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

static uint8_t map_descriptortype(const upb_fielddef *f) {
  uint8_t type = upb_fielddef_descriptortype(f);
  /* See TableDescriptorType() in upbc/generator.cc for details and
   * rationale of these exceptions. */
  if (type == UPB_DTYPE_STRING && f->file->syntax == UPB_SYNTAX_PROTO2) {
    return UPB_DTYPE_BYTES;
  } else if (type == UPB_DTYPE_ENUM &&
             f->sub.enumdef->file->syntax == UPB_SYNTAX_PROTO3) {
    return UPB_DTYPE_INT32;
  }
  return type;
}

static void fill_fieldlayout(upb_msglayout_field *field, const upb_fielddef *f) {
  field->number = upb_fielddef_number(f);
  field->descriptortype = map_descriptortype(f);

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
  size_t field_count = upb_msgdef_numfields(m);
  size_t sublayout_count = 0;
  upb_msglayout_sub *subs;
  upb_msglayout_field *fields;

  memset(l, 0, sizeof(*l) + sizeof(_upb_fasttable_entry));

  /* Count sub-messages. */
  for (size_t i = 0; i < field_count; i++) {
    const upb_fielddef *f = &m->fields[i];
    if (upb_fielddef_issubmsg(f)) {
      sublayout_count++;
    }
    if (upb_fielddef_type(f) == UPB_TYPE_ENUM &&
        f->sub.enumdef->file->syntax == UPB_SYNTAX_PROTO2) {
      sublayout_count++;
    }
  }

  fields = symtab_alloc(ctx, field_count * sizeof(*fields));
  subs = symtab_alloc(ctx, sublayout_count * sizeof(*subs));

  l->field_count = upb_msgdef_numfields(m);
  l->fields = fields;
  l->subs = subs;
  l->table_mask = 0;
  l->required_count = 0;

  if (upb_msgdef_extrangecount(m) > 0) {
    if (google_protobuf_MessageOptions_message_set_wire_format(m->opts)) {
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
    fields[0].descriptortype = map_descriptortype(key);
    fields[1].descriptortype = map_descriptortype(val);
    fields[0].offset = 0;
    fields[1].offset = sizeof(upb_strview);
    fields[1].submsg_index = 0;

    if (upb_fielddef_type(val) == UPB_TYPE_MESSAGE) {
      subs[0].submsg = upb_fielddef_msgsubdef(val)->layout;
    }

    upb_fielddef *fielddefs = (upb_fielddef*)&m->fields[0];
    UPB_ASSERT(fielddefs[0].number_ == 1);
    UPB_ASSERT(fielddefs[1].number_ == 2);
    fielddefs[0].layout_index = 0;
    fielddefs[1].layout_index = 1;

    l->field_count = 2;
    l->size = 2 * sizeof(upb_strview);
    l->size = UPB_ALIGN_UP(l->size, 8);
    l->dense_below = 2;
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

  /* Assign hasbits for required fields first. */
  size_t hasbit = 0;

  for (int i = 0; i < m->field_count; i++) {
    const upb_fielddef* f = &m->fields[i];
    upb_msglayout_field *field = &fields[upb_fielddef_index(f)];
    if (upb_fielddef_label(f) == UPB_LABEL_REQUIRED) {
      field->presence = ++hasbit;
      if (hasbit >= 63) {
        symtab_errf(ctx, "Message with >=63 required fields: %s",
                    upb_msgdef_fullname(m));
      }
      l->required_count++;
    }
  }

  /* Allocate hasbits and set basic field attributes. */
  sublayout_count = 0;
  for (int i = 0; i < m->field_count; i++) {
    const upb_fielddef* f = &m->fields[i];
    upb_msglayout_field *field = &fields[upb_fielddef_index(f)];

    fill_fieldlayout(field, f);

    if (upb_fielddef_issubmsg(f)) {
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].submsg = upb_fielddef_msgsubdef(f)->layout;
    } else if (upb_fielddef_type(f) == UPB_TYPE_ENUM &&
               f->file->syntax == UPB_SYNTAX_PROTO2) {
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].subenum = upb_fielddef_enumsubdef(f)->layout;
      UPB_ASSERT(subs[field->submsg_index].subenum);
    }

    if (upb_fielddef_label(f) == UPB_LABEL_REQUIRED) {
      /* Hasbit was already assigned. */
    } else if (upb_fielddef_haspresence(f) && !upb_fielddef_realcontainingoneof(f)) {
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
  for (int i = 0; i < m->field_count; i++) {
    const upb_fielddef* f = &m->fields[i];
    size_t field_size = upb_msg_fielddefsize(f);
    size_t index = upb_fielddef_index(f);

    if (upb_fielddef_realcontainingoneof(f)) {
      /* Oneofs are handled separately below. */
      continue;
    }

    fields[index].offset = upb_msglayout_place(ctx, l, field_size, m);
  }

  /* Allocate oneof fields.  Each oneof field consists of a uint32 for the case
   * and space for the actual data. */
  for (int i = 0; i < m->oneof_count; i++) {
    const upb_oneofdef* o = &m->oneofs[i];
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
    case_offset = upb_msglayout_place(ctx, l, case_size, m);
    data_offset = upb_msglayout_place(ctx, l, field_size, m);

    for (int i = 0; i < o->field_count; i++) {
      const upb_fielddef* f = o->fields[i];
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

/* Adds a symbol |v| to the symtab, which must be a def pointer previously packed
 * with pack_def().  The def's pointer to upb_filedef* must be set before adding,
 * so we know which entries to remove if building this file fails. */
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

static bool remove_component(char *base, size_t *len) {
  if (*len == 0) return false;

  for (size_t i = *len - 1; i > 0; i--) {
    if (base[i] == '.') {
      *len = i;
      return true;
    }
  }

  *len = 0;
  return true;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static const void *symtab_resolveany(symtab_addctx *ctx,
                                     const char *from_name_dbg,
                                     const char *base, upb_strview sym,
                                     upb_deftype_t *type) {
  const upb_strtable *t = &ctx->symtab->syms;
  if(sym.size == 0) goto notfound;
  upb_value v;
  if(sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    if (!upb_strtable_lookup2(t, sym.data + 1, sym.size - 1, &v)) {
      goto notfound;
    }
  } else {
    /* Remove components from base until we find an entry or run out. */
    size_t baselen = strlen(base);
    char *tmp = malloc(sym.size + strlen(base) + 1);
    while (1) {
      char *p = tmp;
      if (baselen) {
        memcpy(p, base, baselen);
        p[baselen] = '.';
        p += baselen + 1;
      }
      memcpy(p, sym.data, sym.size);
      p += sym.size;
      if (upb_strtable_lookup2(t, tmp, p - tmp, &v)) {
        break;
      }
      if (!remove_component(tmp, &baselen)) {
        free(tmp);
        goto notfound;
      }
    }
    free(tmp);
  }

  *type = deftype(v);
  return unpack_def(v, *type);

notfound:
  symtab_errf(ctx, "couldn't resolve name '" UPB_STRVIEW_FORMAT "'",
              UPB_STRVIEW_ARGS(sym));
}

static const void *symtab_resolve(symtab_addctx *ctx, const char *from_name_dbg,
                                  const char *base, upb_strview sym,
                                  upb_deftype_t type) {
  upb_deftype_t found_type;
  const void *ret =
      symtab_resolveany(ctx, from_name_dbg, base, sym, &found_type);
  if (ret && found_type != type) {
    symtab_errf(
        ctx,
        "type mismatch when resolving %s: couldn't find name %s with type=%d",
        from_name_dbg, sym.data, (int)type);
  }
  return ret;
}

static void create_oneofdef(
    symtab_addctx *ctx, upb_msgdef *m,
    const google_protobuf_OneofDescriptorProto *oneof_proto,
    const upb_oneofdef *_o) {
  upb_oneofdef *o = (upb_oneofdef *)_o;
  upb_strview name = google_protobuf_OneofDescriptorProto_name(oneof_proto);
  upb_value v;

  o->parent = m;
  o->full_name = makefullname(ctx, m->full_name, name);
  o->field_count = 0;
  o->synthetic = false;

  SET_OPTIONS(o->opts, OneofDescriptorProto, OneofOptions, oneof_proto);

  v = pack_def(o, UPB_DEFTYPE_ONEOF);
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
        goto invalid;
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
  symtab_errf(ctx, "Invalid default '%.*s' for field %s of type %d", (int)len, str,
              upb_fielddef_fullname(f), (int)upb_fielddef_descriptortype(f));
}

static void set_default_default(symtab_addctx *ctx, upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
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
    case UPB_TYPE_ENUM:
      f->defaultval.sint = f->sub.enumdef->values[0].number;
    case UPB_TYPE_MESSAGE:
      break;
  }
}

static void create_fielddef(
    symtab_addctx *ctx, const char *prefix, upb_msgdef *m,
    const google_protobuf_FieldDescriptorProto *field_proto,
    const upb_fielddef *_f) {
  upb_fielddef *f = (upb_fielddef*)_f;
  upb_strview name;
  const char *full_name;
  const char *json_name;
  const char *shortname;
  uint32_t field_number;

  f->file = ctx->file;  /* Must happen prior to symtab_add(). */

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
    f->has_json_name_ = true;
  } else {
    json_name = makejsonname(ctx, shortname);
    f->has_json_name_ = false;
  }

  field_number = google_protobuf_FieldDescriptorProto_number(field_proto);

  if (field_number == 0 || field_number > UPB_MAX_FIELDNUMBER) {
    symtab_errf(ctx, "invalid field number (%u)", field_number);
  }

  f->full_name = full_name;
  f->json_name = json_name;
  f->label_ = (int)google_protobuf_FieldDescriptorProto_label(field_proto);
  f->number_ = field_number;
  f->oneof = NULL;
  f->proto3_optional_ =
      google_protobuf_FieldDescriptorProto_proto3_optional(field_proto);

  bool has_type = google_protobuf_FieldDescriptorProto_has_type(field_proto);
  bool has_type_name =
      google_protobuf_FieldDescriptorProto_has_type_name(field_proto);

  f->type_ = (int)google_protobuf_FieldDescriptorProto_type(field_proto);

  if (has_type) {
    switch (f->type_) {
      case UPB_DTYPE_MESSAGE:
      case UPB_DTYPE_GROUP:
      case UPB_DTYPE_ENUM:
        if (!has_type_name) {
          symtab_errf(ctx, "field of type %d requires type name (%s)",
                      (int)f->type_, full_name);
        }
        break;
      default:
        if (has_type_name) {
          symtab_errf(ctx, "invalid type for field with type_name set (%s, %d)",
                      full_name, (int)f->type_);
        }
    }
  } else if (has_type_name) {
    f->type_ = FIELD_TYPE_UNSPECIFIED;  // We'll fill this in in resolve_fielddef().
  }

  if (m) {
    /* direct message field. */
    upb_value v, field_v, json_v, existing_v;
    size_t json_size;

    f->index_ = f - m->fields;
    f->msgdef = m;
    f->is_extension_ = false;

    field_v = pack_def(f, UPB_DEFTYPE_FIELD);
    json_v = pack_def(f, UPB_DEFTYPE_FIELD_JSONNAME);
    v = upb_value_constptr(f);
    json_size = strlen(json_name);

    if (upb_strtable_lookup(&m->ntof, shortname, &existing_v)) {
      if (ctx->symtab->allow_name_conflicts &&
          deftype(existing_v) == UPB_DEFTYPE_FIELD_JSONNAME) {
        // Field name takes precedence over json name.
        upb_strtable_remove(&m->ntof, shortname, NULL);
      } else {
        symtab_errf(ctx, "duplicate field name (%s)", shortname);
      }
    }

    CHK_OOM(upb_strtable_insert(&m->ntof, name.data, name.size, field_v,
                                ctx->arena));

    if (strcmp(shortname, json_name) != 0) {
      if (upb_strtable_lookup(&m->ntof, json_name, &v)) {
        if (!ctx->symtab->allow_name_conflicts) {
          symtab_errf(ctx, "duplicate json_name (%s)", json_name);
        }
      } else {
        CHK_OOM(upb_strtable_insert(&m->ntof, json_name, json_size, json_v,
                                    ctx->arena));
      }
    }

    if (upb_inttable_lookup(&m->itof, field_number, NULL)) {
      symtab_errf(ctx, "duplicate field number (%u)", field_number);
    }

    CHK_OOM(upb_inttable_insert(&m->itof, field_number, v, ctx->arena));

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
    f->is_extension_ = true;
    symtab_add(ctx, full_name, pack_def(f, UPB_DEFTYPE_EXT));
    f->layout_index = ctx->ext_count++;
    if (ctx->layout) {
      UPB_ASSERT(ctx->file->ext_layouts[f->layout_index]->field.number ==
                 field_number);
    }
  }

  if (f->type_ < UPB_DTYPE_DOUBLE || f->type_ > UPB_DTYPE_SINT64) {
    symtab_errf(ctx, "invalid type for field %s (%d)", f->full_name, f->type_);
  }

  if (f->label_ < UPB_LABEL_OPTIONAL || f->label_ > UPB_LABEL_REPEATED) {
    symtab_errf(ctx, "invalid label for field %s (%d)", f->full_name,
                f->label_);
  }

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

  SET_OPTIONS(f->opts, FieldDescriptorProto, FieldOptions, field_proto);

  if (google_protobuf_FieldOptions_has_packed(f->opts)) {
    f->packed_ = google_protobuf_FieldOptions_packed(f->opts);
  } else {
    /* Repeated fields default to packed for proto3 only. */
    f->packed_ = upb_fielddef_isprimitive(f) &&
        f->label_ == UPB_LABEL_REPEATED && f->file->syntax == UPB_SYNTAX_PROTO3;
  }
}

static void create_service(
    symtab_addctx *ctx, const google_protobuf_ServiceDescriptorProto *svc_proto,
    const upb_servicedef *_s) {
  upb_servicedef *s = (upb_servicedef*)_s;
  upb_strview name;
  const google_protobuf_MethodDescriptorProto *const *methods;
  size_t i, n;

  s->file = ctx->file;  /* Must happen prior to symtab_add. */

  name = google_protobuf_ServiceDescriptorProto_name(svc_proto);
  check_ident(ctx, name, false);
  s->full_name = makefullname(ctx, ctx->file->package, name);
  symtab_add(ctx, s->full_name, pack_def(s, UPB_DEFTYPE_SERVICE));

  methods = google_protobuf_ServiceDescriptorProto_method(svc_proto, &n);

  s->method_count = n;
  s->methods = symtab_alloc(ctx, sizeof(*s->methods) * n);

  SET_OPTIONS(s->opts, ServiceDescriptorProto, ServiceOptions, svc_proto);

  for (i = 0; i < n; i++) {
    const google_protobuf_MethodDescriptorProto *method_proto = methods[i];
    upb_methoddef *m = (upb_methoddef*)&s->methods[i];
    upb_strview name = google_protobuf_MethodDescriptorProto_name(method_proto);

    m->service = s;
    m->full_name = makefullname(ctx, s->full_name, name);
    m->client_streaming =
        google_protobuf_MethodDescriptorProto_client_streaming(method_proto);
    m->server_streaming =
        google_protobuf_MethodDescriptorProto_server_streaming(method_proto);
    m->input_type = symtab_resolve(
        ctx, m->full_name, m->full_name,
        google_protobuf_MethodDescriptorProto_input_type(method_proto),
        UPB_DEFTYPE_MSG);
    m->output_type = symtab_resolve(
        ctx, m->full_name, m->full_name,
        google_protobuf_MethodDescriptorProto_output_type(method_proto),
        UPB_DEFTYPE_MSG);

    SET_OPTIONS(m->opts, MethodDescriptorProto, MethodOptions, method_proto);
  }
}

static int count_bits_debug(uint64_t x) {
  // For assertions only, speed does not matter.
  int n = 0;
  while (x) {
    if (x & 1) n++;
    x >>= 1;
  }
  return n;
}

upb_enumlayout *create_enumlayout(symtab_addctx *ctx, const upb_enumdef *e) {
  int n = 0;
  uint64_t mask = 0;

  for (int i = 0; i < e->value_count; i++) {
    uint32_t val = (uint32_t)e->values[i].number;
    if (val < 64) {
      mask |= 1 << val;
    } else {
      n++;
    }
  }

  int32_t *values = symtab_alloc(ctx, sizeof(*values) * n);

  if (n) {
    int32_t *p = values;

    // Add values outside the bitmask range to the list, as described in the
    // comments for upb_enumlayout.
    for (int i = 0; i < e->value_count; i++) {
      int32_t val = e->values[i].number;
      if ((uint32_t)val >= 64) {
        *p++ = val;
      }
    }
    UPB_ASSERT(p == values + n);
  }

  UPB_ASSERT(upb_inttable_count(&e->iton) == n + count_bits_debug(mask));

  upb_enumlayout *layout = symtab_alloc(ctx, sizeof(*layout));
  layout->value_count = n;
  layout->mask = mask;
  layout->values = values;

  return layout;
}

static void create_enumvaldef(
    symtab_addctx *ctx, const char *prefix,
    const google_protobuf_EnumValueDescriptorProto *val_proto, upb_enumdef *e,
    int i) {
  upb_enumvaldef *val = (upb_enumvaldef *)&e->values[i];
  upb_strview name = google_protobuf_EnumValueDescriptorProto_name(val_proto);
  upb_value v = upb_value_constptr(val);

  val->enum_ = e;  /* Must happen prior to symtab_add(). */
  val->full_name = makefullname(ctx, prefix, name);
  val->number = google_protobuf_EnumValueDescriptorProto_number(val_proto);
  symtab_add(ctx, val->full_name, pack_def(val, UPB_DEFTYPE_ENUMVAL));

  SET_OPTIONS(val->opts, EnumValueDescriptorProto, EnumValueOptions, val_proto);

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

static void create_enumdef(
    symtab_addctx *ctx, const char *prefix,
    const google_protobuf_EnumDescriptorProto *enum_proto,
    const upb_msgdef *containing_type,
    const upb_enumdef *_e) {
  upb_enumdef *e = (upb_enumdef*)_e;;
  const google_protobuf_EnumValueDescriptorProto *const *values;
  upb_strview name;
  size_t i, n;

  e->file = ctx->file;  /* Must happen prior to symtab_add() */
  e->containing_type = containing_type;

  name = google_protobuf_EnumDescriptorProto_name(enum_proto);
  check_ident(ctx, name, false);

  e->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, e->full_name, pack_def(e, UPB_DEFTYPE_ENUM));

  values = google_protobuf_EnumDescriptorProto_value(enum_proto, &n);
  CHK_OOM(upb_strtable_init(&e->ntoi, n, ctx->arena));
  CHK_OOM(upb_inttable_init(&e->iton, ctx->arena));

  e->defaultval = 0;
  e->value_count = n;
  e->values = symtab_alloc(ctx, sizeof(*e->values) * n);

  if (n == 0) {
    symtab_errf(ctx, "enums must contain at least one value (%s)",
                e->full_name);
  }

  SET_OPTIONS(e->opts, EnumDescriptorProto, EnumOptions, enum_proto);

  for (i = 0; i < n; i++) {
    create_enumvaldef(ctx, prefix, values[i], e, i);
  }

  upb_inttable_compact(&e->iton, ctx->arena);

  if (e->file->syntax == UPB_SYNTAX_PROTO2) {
    if (ctx->layout) {
      UPB_ASSERT(ctx->enum_count < ctx->layout->enum_count);
      e->layout = ctx->layout->enums[ctx->enum_count++];
      UPB_ASSERT(n == e->layout->value_count + count_bits_debug(e->layout->mask));
    } else {
      e->layout = create_enumlayout(ctx, e);
    }
  } else {
    e->layout = NULL;
  }
}

static void create_msgdef(symtab_addctx *ctx, const char *prefix,
                          const google_protobuf_DescriptorProto *msg_proto,
                          const upb_msgdef *containing_type,
                          const upb_msgdef *_m) {
  upb_msgdef *m = (upb_msgdef*)_m;
  const google_protobuf_OneofDescriptorProto *const *oneofs;
  const google_protobuf_FieldDescriptorProto *const *fields;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_DescriptorProto *const *msgs;
  const google_protobuf_DescriptorProto_ExtensionRange *const *ext_ranges;
  size_t i, n_oneof, n_field, n_ext_range, n;
  upb_strview name;

  m->file = ctx->file;  /* Must happen prior to symtab_add(). */
  m->containing_type = containing_type;

  name = google_protobuf_DescriptorProto_name(msg_proto);
  check_ident(ctx, name, false);

  m->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, m->full_name, pack_def(m, UPB_DEFTYPE_MSG));

  oneofs = google_protobuf_DescriptorProto_oneof_decl(msg_proto, &n_oneof);
  fields = google_protobuf_DescriptorProto_field(msg_proto, &n_field);
  ext_ranges =
      google_protobuf_DescriptorProto_extension_range(msg_proto, &n_ext_range);

  CHK_OOM(upb_inttable_init(&m->itof, ctx->arena));
  CHK_OOM(upb_strtable_init(&m->ntof, n_oneof + n_field, ctx->arena));

  if (ctx->layout) {
    /* create_fielddef() below depends on this being set. */
    UPB_ASSERT(ctx->msg_count < ctx->layout->msg_count);
    m->layout = ctx->layout->msgs[ctx->msg_count++];
    UPB_ASSERT(n_field == m->layout->field_count);
  } else {
    /* Allocate now (to allow cross-linking), populate later. */
    m->layout = symtab_alloc(
        ctx, sizeof(*m->layout) + sizeof(_upb_fasttable_entry));
  }

  SET_OPTIONS(m->opts, DescriptorProto, MessageOptions, msg_proto);

  m->oneof_count = n_oneof;
  m->oneofs = symtab_alloc(ctx, sizeof(*m->oneofs) * n_oneof);
  for (i = 0; i < n_oneof; i++) {
    create_oneofdef(ctx, m, oneofs[i], &m->oneofs[i]);
  }

  m->field_count = n_field;
  m->fields = symtab_alloc(ctx, sizeof(*m->fields) * n_field);
  for (i = 0; i < n_field; i++) {
    create_fielddef(ctx, m->full_name, m, fields[i], &m->fields[i]);
  }

  m->ext_range_count = n_ext_range;
  m->ext_ranges = symtab_alloc(ctx, sizeof(*m->ext_ranges) * n_ext_range);
  for (i = 0; i < n_ext_range; i++) {
    const google_protobuf_DescriptorProto_ExtensionRange *r = ext_ranges[i];
    upb_extrange *r_def = (upb_extrange*)&m->ext_ranges[i];
    r_def->start = google_protobuf_DescriptorProto_ExtensionRange_start(r);
    r_def->end = google_protobuf_DescriptorProto_ExtensionRange_end(r);
    SET_OPTIONS(r_def->opts, DescriptorProto_ExtensionRange,
                ExtensionRangeOptions, r);
  }

  finalize_oneofs(ctx, m);
  assign_msg_wellknowntype(m);
  upb_inttable_compact(&m->itof, ctx->arena);

  /* This message is built.  Now build nested entities. */

  enums = google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  m->nested_enum_count = n;
  m->nested_enums = symtab_alloc(ctx, sizeof(*m->nested_enums) * n);
  for (i = 0; i < n; i++) {
    m->nested_enum_count = i + 1;
    create_enumdef(ctx, m->full_name, enums[i], m, &m->nested_enums[i]);
  }

  fields = google_protobuf_DescriptorProto_extension(msg_proto, &n);
  m->nested_ext_count = n;
  m->nested_exts = symtab_alloc(ctx, sizeof(*m->nested_exts) * n);
  for (i = 0; i < n; i++) {
    create_fielddef(ctx, m->full_name, NULL, fields[i], &m->nested_exts[i]);
    ((upb_fielddef*)&m->nested_exts[i])->index_ = i;
  }

  msgs = google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  m->nested_msg_count = n;
  m->nested_msgs = symtab_alloc(ctx, sizeof(*m->nested_msgs) * n);
  for (i = 0; i < n; i++) {
    create_msgdef(ctx, m->full_name, msgs[i], m, &m->nested_msgs[i]);
  }
}

static void resolve_fielddef(symtab_addctx *ctx, const char *prefix,
                             upb_fielddef *f) {
  const google_protobuf_FieldDescriptorProto *field_proto = f->sub.unresolved;
  upb_strview name =
      google_protobuf_FieldDescriptorProto_type_name(field_proto);
  bool has_name =
      google_protobuf_FieldDescriptorProto_has_type_name(field_proto);

  // Resolve subdef by type name, if necessary.
  switch ((int)f->type_) {
    case FIELD_TYPE_UNSPECIFIED: {
      // Type was not specified and must be inferred.
      UPB_ASSERT(has_name);
      upb_deftype_t type;
      const void *def = 
          symtab_resolveany(ctx, f->full_name, prefix, name, &type);
      switch (type) {
        case UPB_DEFTYPE_ENUM:
          f->sub.enumdef = def;
          f->type_ = UPB_DTYPE_ENUM;
          break;
        case UPB_DEFTYPE_MSG:
          f->sub.msgdef = def;
          f->type_ = UPB_DTYPE_MESSAGE;  // It appears there is no way of this
                                         // being a group.
          break;
        default:
          symtab_errf(ctx, "Couldn't resolve type name for field %s",
                      f->full_name);
      }
    }
    case UPB_DTYPE_MESSAGE:
    case UPB_DTYPE_GROUP:
      UPB_ASSERT(has_name);
      f->sub.msgdef =
          symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);
      break;
    case UPB_DTYPE_ENUM:
      UPB_ASSERT(has_name);
      f->sub.enumdef =
          symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_ENUM);
      break;
    default:
      // No resolution necessary.
      break;
  }

  if (f->is_extension_) {
    if (!google_protobuf_FieldDescriptorProto_has_extendee(field_proto)) {
      symtab_errf(ctx, "extension for field '%s' had no extendee",
                  f->full_name);
    }

    name = google_protobuf_FieldDescriptorProto_extendee(field_proto);
    f->msgdef =
        symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);

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
    f->has_default = true;
  } else {
    set_default_default(ctx, f);
    f->has_default = false;
  }
}

static void resolve_msgdef(symtab_addctx *ctx, upb_msgdef *m) {
  for (int i = 0; i < m->field_count; i++) {
    resolve_fielddef(ctx, m->full_name, (upb_fielddef *)&m->fields[i]);
  }

  for (int i = 0; i < m->nested_ext_count; i++) {
    resolve_fielddef(ctx, m->full_name, (upb_fielddef *)&m->nested_exts[i]);
  }

  if (!ctx->layout) make_layout(ctx, m);

  m->in_message_set = false;
  if (m->nested_ext_count == 1) {
    const upb_fielddef *ext = &m->nested_exts[0];
    if (ext->type_ == UPB_DTYPE_MESSAGE && ext->label_ == UPB_LABEL_OPTIONAL &&
        ext->sub.msgdef == m &&
        google_protobuf_MessageOptions_message_set_wire_format(
            ext->msgdef->opts)) {
      m->in_message_set = true;
    }
  }

  for (int i = 0; i < m->nested_msg_count; i++) {
    resolve_msgdef(ctx, (upb_msgdef*)&m->nested_msgs[i]);
  }
}

static int count_exts_in_msg(const google_protobuf_DescriptorProto *msg_proto) {
  size_t n;
  google_protobuf_DescriptorProto_extension(msg_proto, &n);
  int ext_count = n;

  const google_protobuf_DescriptorProto *const *nested_msgs =
      google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (size_t i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(nested_msgs[i]);
  }

  return ext_count;
}

static void build_filedef(
    symtab_addctx *ctx, upb_filedef *file,
    const google_protobuf_FileDescriptorProto *file_proto) {
  const google_protobuf_DescriptorProto *const *msgs;
  const google_protobuf_EnumDescriptorProto *const *enums;
  const google_protobuf_FieldDescriptorProto *const *exts;
  const google_protobuf_ServiceDescriptorProto *const *services;
  const upb_strview *strs;
  const int32_t *public_deps;
  const int32_t *weak_deps;
  size_t i, n;

  file->symtab = ctx->symtab;

  /* Count all extensions in the file, to build a flat array of layouts. */
  google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  int ext_count = n;
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (int i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(msgs[i]);
  }
  file->ext_count = ext_count;

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

  if (google_protobuf_FileDescriptorProto_has_package(file_proto)) {
    upb_strview package =
        google_protobuf_FileDescriptorProto_package(file_proto);
    check_ident(ctx, package, true);
    file->package = strviewdup(ctx, package);
  } else {
    file->package = NULL;
  }

  if (google_protobuf_FileDescriptorProto_has_syntax(file_proto)) {
    upb_strview syntax = google_protobuf_FileDescriptorProto_syntax(file_proto);

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
  SET_OPTIONS(file->opts, FileDescriptorProto, FileOptions, file_proto);

  /* Verify dependencies. */
  strs = google_protobuf_FileDescriptorProto_dependency(file_proto, &n);
  file->dep_count = n;
  file->deps = symtab_alloc(ctx, sizeof(*file->deps) * n);

  for (i = 0; i < n; i++) {
    upb_strview str = strs[i];
    file->deps[i] = upb_symtab_lookupfile2(ctx->symtab, str.data, str.size);
    if (!file->deps[i]) {
      symtab_errf(ctx,
                  "Depends on file '" UPB_STRVIEW_FORMAT
                  "', but it has not been loaded",
                  UPB_STRVIEW_ARGS(str));
    }
  }

  public_deps =
      google_protobuf_FileDescriptorProto_public_dependency(file_proto, &n);
  file->public_dep_count = n;
  file->public_deps = symtab_alloc(ctx, sizeof(*file->public_deps) * n);
  int32_t *mutable_public_deps = (int32_t*)file->public_deps;
  for (i = 0; i < n; i++) {
    if (public_deps[i] >= file->dep_count) {
      symtab_errf(ctx, "public_dep %d is out of range", (int)public_deps[i]);
    }
    mutable_public_deps[i] = public_deps[i];
  }

  weak_deps =
      google_protobuf_FileDescriptorProto_weak_dependency(file_proto, &n);
  file->weak_dep_count = n;
  file->weak_deps = symtab_alloc(ctx, sizeof(*file->weak_deps) * n);
  int32_t *mutable_weak_deps = (int32_t*)file->weak_deps;
  for (i = 0; i < n; i++) {
    if (weak_deps[i] >= file->dep_count) {
      symtab_errf(ctx, "public_dep %d is out of range", (int)public_deps[i]);
    }
    mutable_weak_deps[i] = weak_deps[i];
  }

  /* Create enums. */
  enums = google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  file->top_lvl_enum_count = n;
  file->top_lvl_enums = symtab_alloc(ctx, sizeof(*file->top_lvl_enums) * n);
  for (i = 0; i < n; i++) {
    create_enumdef(ctx, file->package, enums[i], NULL, &file->top_lvl_enums[i]);
  }

  /* Create extensions. */
  exts = google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->top_lvl_ext_count = n;
  file->top_lvl_exts = symtab_alloc(ctx, sizeof(*file->top_lvl_exts) * n);
  for (i = 0; i < n; i++) {
    create_fielddef(ctx, file->package, NULL, exts[i], &file->top_lvl_exts[i]);
    ((upb_fielddef*)&file->top_lvl_exts[i])->index_ = i;
  }

  /* Create messages. */
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  file->top_lvl_msg_count = n;
  file->top_lvl_msgs = symtab_alloc(ctx, sizeof(*file->top_lvl_msgs) * n);
  for (i = 0; i < n; i++) {
    create_msgdef(ctx, file->package, msgs[i], NULL, &file->top_lvl_msgs[i]);
  }

  /* Create services. */
  services = google_protobuf_FileDescriptorProto_service(file_proto, &n);
  file->service_count = n;
  file->services = symtab_alloc(ctx, sizeof(*file->services) * n);
  for (i = 0; i < n; i++) {
    create_service(ctx, services[i], &file->services[i]);
    ((upb_servicedef*)&file->services[i])->index = i;
  }

  /* Now that all names are in the table, build layouts and resolve refs. */
  for (i = 0; i < (size_t)file->top_lvl_ext_count; i++) {
    resolve_fielddef(ctx, file->package, (upb_fielddef*)&file->top_lvl_exts[i]);
  }

  for (i = 0; i < (size_t)file->top_lvl_msg_count; i++) {
    resolve_msgdef(ctx, (upb_msgdef*)&file->top_lvl_msgs[i]);
  }

  CHK_OOM(
      _upb_extreg_add(ctx->symtab->extreg, file->ext_layouts, file->ext_count));
}

static void remove_filedef(upb_symtab *s, upb_filedef *file) {
  intptr_t iter = UPB_INTTABLE_BEGIN;
  upb_strview key;
  upb_value val;
  while (upb_strtable_next2(&s->syms, &key, &val, &iter)) {
    const upb_filedef *f;
    switch (deftype(val)) {
      case UPB_DEFTYPE_EXT:
        f = upb_fielddef_file(unpack_def(val, UPB_DEFTYPE_EXT));
        break;
      case UPB_DEFTYPE_MSG:
        f = upb_msgdef_file(unpack_def(val, UPB_DEFTYPE_MSG));
        break;
      case UPB_DEFTYPE_ENUM:
        f = upb_enumdef_file(unpack_def(val, UPB_DEFTYPE_ENUM));
        break;
      case UPB_DEFTYPE_ENUMVAL:
        f = upb_enumdef_file(
            upb_enumvaldef_enum(unpack_def(val, UPB_DEFTYPE_ENUMVAL)));
        break;
      case UPB_DEFTYPE_SERVICE:
        f = upb_servicedef_file(unpack_def(val, UPB_DEFTYPE_SERVICE));
        break;
      default:
        UPB_UNREACHABLE();
    }

    if (f == file) upb_strtable_removeiter(&s->syms, &iter);
  }
}

static const upb_filedef *_upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file_proto,
    const upb_msglayout_file *layout, upb_status *status) {
  symtab_addctx ctx;
  upb_strview name = google_protobuf_FileDescriptorProto_name(file_proto);
  upb_value v;

  if (upb_strtable_lookup2(&s->files, name.data, name.size, &v)) {
    if (unpack_def(v, UPB_DEFTYPE_FILE)) {
      upb_status_seterrf(status, "duplicate file name (%.*s)",
                        UPB_STRVIEW_ARGS(name));
      return NULL;
    }
    const upb_msglayout_file *registered = unpack_def(v, UPB_DEFTYPE_LAYOUT);
    UPB_ASSERT(registered);
    if (layout && layout != registered) {
      upb_status_seterrf(
          status, "tried to build with a different layout (filename=%.*s)",
          UPB_STRVIEW_ARGS(name));
      return NULL;
    }
    layout = registered;
  }

  ctx.symtab = s;
  ctx.layout = layout;
  ctx.msg_count = 0;
  ctx.enum_count = 0;
  ctx.ext_count = 0;
  ctx.status = status;
  ctx.file = NULL;
  ctx.arena = upb_arena_new();
  ctx.tmp_arena = upb_arena_new();

  if (!ctx.arena || !ctx.tmp_arena) {
    if (ctx.arena) upb_arena_free(ctx.arena);
    if (ctx.tmp_arena) upb_arena_free(ctx.tmp_arena);
    upb_status_setoom(status);
    return NULL;
  }

  if (UPB_UNLIKELY(UPB_SETJMP(ctx.err))) {
    UPB_ASSERT(!upb_ok(status));
    if (ctx.file) {
      remove_filedef(s, ctx.file);
      ctx.file = NULL;
    }
  } else {
    ctx.file = symtab_alloc(&ctx, sizeof(*ctx.file));
    build_filedef(&ctx, ctx.file, file_proto);
    upb_strtable_insert(&s->files, name.data, name.size,
                        pack_def(ctx.file, UPB_DEFTYPE_FILE), ctx.arena);
    UPB_ASSERT(upb_ok(status));
    upb_arena_fuse(s->arena, ctx.arena);
  }

  upb_arena_free(ctx.arena);
  upb_arena_free(ctx.tmp_arena);
  return ctx.file;
}

  const upb_filedef *upb_symtab_addfile(
      upb_symtab * s, const google_protobuf_FileDescriptorProto *file_proto,
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

  if (upb_symtab_lookupfile(s, init->filename)) {
    return true;
  }

  arena = upb_arena_new();

  for (; *deps; deps++) {
    if (!_upb_symtab_loaddefinit(s, *deps)) goto err;
  }

  file = google_protobuf_FileDescriptorProto_parse_ex(
      init->descriptor.data, init->descriptor.size, NULL, kUpb_DecodeOption_AliasString,
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

  if (!_upb_symtab_addfile(s, file, init->layout, &status)) {
    goto err;
  }

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

const upb_fielddef *upb_symtab_lookupextbynum(const upb_symtab *s,
                                              const upb_msgdef *m,
                                              int32_t fieldnum) {
  const upb_msglayout *l = upb_msgdef_layout(m);
  const upb_msglayout_ext *ext = _upb_extreg_get(s->extreg, l, fieldnum);
  return ext ? _upb_symtab_lookupextfield(s, ext) : NULL;
}

bool _upb_symtab_registerlayout(upb_symtab *s, const char *filename,
                                const upb_msglayout_file *file) {
  if (upb_symtab_lookupfile(s, filename)) return false;
  upb_value v = pack_def(file, UPB_DEFTYPE_LAYOUT);
  return upb_strtable_insert(&s->files, filename, strlen(filename), v,
                             s->arena);
}

const upb_extreg *upb_symtab_extreg(const upb_symtab *s) {
  return s->extreg;
}

const upb_fielddef **upb_symtab_getallexts(const upb_symtab *s,
                                           const upb_msgdef *m, size_t *count) {
  size_t n = 0;
  intptr_t iter = UPB_INTTABLE_BEGIN;
  uintptr_t key;
  upb_value val;
  // This is O(all exts) instead of O(exts for m).  If we need this to be
  // efficient we may need to make extreg into a two-level table, or have a
  // second per-message index.
  while (upb_inttable_next2(&s->exts, &key, &val, &iter)) {
    const upb_fielddef *f = upb_value_getconstptr(val);
    if (upb_fielddef_containingtype(f) == m) n++;
  }
  const upb_fielddef **exts = malloc(n * sizeof(*exts));
  iter = UPB_INTTABLE_BEGIN;
  size_t i = 0;
  while (upb_inttable_next2(&s->exts, &key, &val, &iter)) {
    const upb_fielddef *f = upb_value_getconstptr(val);
    if (upb_fielddef_containingtype(f) == m) exts[i++] = f;
  }
  *count = n;
  return exts;
}

#undef CHK_OOM
