/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include <errno.h>
#include "upb/def.h"
#include "upb/descriptor.h"

// Returns a newly allocated string that joins input strings together, for example:
//   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
//   join("", "Baz") -> "Baz"
// Caller owns a ref on the returned string. */
static char *upb_join(char *base, char *name) {
  if (!base || strlen(base) == 0) {
    return strdup(name);
  } else {
    char *ret = malloc(strlen(base) + strlen(name) + 2);
    ret[0] = '\0';
    strcat(ret, base);
    strcat(ret, ".");
    strcat(ret, name);
    return ret;
  }
}

/* upb_descreader  ************************************************************/

// A upb_descreader builds a list of defs by handling a parse of a protobuf in
// the format defined in descriptor.proto.  The output of a upb_descreader is
// a upb_symtabtxn.

static upb_def *upb_deflist_last(upb_deflist *l) {
  return l->defs[l->len-1];
}

// Qualify the defname for all defs starting with offset "start" with "str".
static void upb_deflist_qualify(upb_deflist *l, char *str, int32_t start) {
  for(uint32_t i = start; i < l->len; i++) {
    upb_def *def = l->defs[i];
    char *name = def->fqname;
    def->fqname = upb_join(str, name);
    free(name);
  }
}

// Forward declares for top-level file descriptors.
static upb_mhandlers *upb_msgdef_register_DescriptorProto(upb_handlers *h);
static upb_mhandlers * upb_enumdef_register_EnumDescriptorProto(upb_handlers *h);

void upb_descreader_init(upb_descreader *r) {
  upb_deflist_init(&r->defs);
  upb_status_init(&r->status);
  r->stack_len = 0;
  r->name = NULL;
  r->default_string = NULL;
}

void upb_descreader_uninit(upb_descreader *r) {
  free(r->name);
  upb_status_uninit(&r->status);
  upb_deflist_uninit(&r->defs);
  free(r->default_string);
  while (r->stack_len > 0) {
    upb_descreader_frame *f = &r->stack[--r->stack_len];
    free(f->name);
  }
}

upb_def **upb_descreader_getdefs(upb_descreader *r, int *n) {
  *n = r->defs.len;
  r->defs.len = 0;
  return r->defs.defs;
}

static upb_msgdef *upb_descreader_top(upb_descreader *r) {
  if (r->stack_len <= 1) return NULL;
  int index = r->stack[r->stack_len-1].start - 1;
  assert(index >= 0);
  return upb_downcast_msgdef(r->defs.defs[index]);
}

static upb_def *upb_descreader_last(upb_descreader *r) {
  return upb_deflist_last(&r->defs);
}

// Start/end handlers for FileDescriptorProto and DescriptorProto (the two
// entities that have names and can contain sub-definitions.
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

// Handlers for google.protobuf.FileDescriptorProto.
static upb_flow_t upb_descreader_FileDescriptorProto_startmsg(void *_r) {
  upb_descreader *r = _r;
  upb_descreader_startcontainer(r);
  return UPB_CONTINUE;
}

static void upb_descreader_FileDescriptorProto_endmsg(void *_r,
                                                      upb_status *status) {
  (void)status;
  upb_descreader *r = _r;
  upb_descreader_endcontainer(r);
}

static upb_flow_t upb_descreader_FileDescriptorProto_package(void *_r,
                                                             upb_value fval,
                                                             upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  upb_descreader_setscopename(r, upb_strref_dup(upb_value_getstrref(val)));
  return UPB_CONTINUE;
}

static upb_mhandlers *upb_descreader_register_FileDescriptorProto(
    upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_descreader_FileDescriptorProto_startmsg);
  upb_mhandlers_setendmsg(m, &upb_descreader_FileDescriptorProto_endmsg);

#define FNUM(field) GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ ## field ## __FIELDNUM
#define FTYPE(field) GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ ## field ## __FIELDTYPE
  upb_fhandlers *f =
      upb_mhandlers_newfhandlers(m, FNUM(PACKAGE), FTYPE(PACKAGE), false);
  upb_fhandlers_setvalue(f, &upb_descreader_FileDescriptorProto_package);

  upb_mhandlers_newfhandlers_subm(m, FNUM(MESSAGE_TYPE), FTYPE(MESSAGE_TYPE), true,
                                  upb_msgdef_register_DescriptorProto(h));
  upb_mhandlers_newfhandlers_subm(m, FNUM(ENUM_TYPE), FTYPE(ENUM_TYPE), true,
                                  upb_enumdef_register_EnumDescriptorProto(h));
  // TODO: services, extensions
  return m;
}
#undef FNUM
#undef FTYPE

static upb_mhandlers *upb_descreader_register_FileDescriptorSet(upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);

#define FNUM(field) GOOGLE_PROTOBUF_FILEDESCRIPTORSET_ ## field ## __FIELDNUM
#define FTYPE(field) GOOGLE_PROTOBUF_FILEDESCRIPTORSET_ ## field ## __FIELDTYPE
  upb_mhandlers_newfhandlers_subm(m, FNUM(FILE), FTYPE(FILE), true,
                                   upb_descreader_register_FileDescriptorProto(h));
  return m;
}
#undef FNUM
#undef FTYPE

upb_mhandlers *upb_descreader_reghandlers(upb_handlers *h) {
  h->should_jit = false;
  return upb_descreader_register_FileDescriptorSet(h);
}

// google.protobuf.EnumValueDescriptorProto.
static upb_flow_t upb_enumdef_EnumValueDescriptorProto_startmsg(void *_r) {
  upb_descreader *r = _r;
  r->saw_number = false;
  r->saw_name = false;
  return UPB_CONTINUE;
}

static upb_flow_t upb_enumdef_EnumValueDescriptorProto_name(void *_r,
                                                            upb_value fval,
                                                            upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  free(r->name);
  r->name = upb_strref_dup(upb_value_getstrref(val));
  r->saw_name = true;
  return UPB_CONTINUE;
}

static upb_flow_t upb_enumdef_EnumValueDescriptorProto_number(void *_r,
                                                              upb_value fval,
                                                              upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  r->number = upb_value_getint32(val);
  r->saw_number = true;
  return UPB_CONTINUE;
}

static void upb_enumdef_EnumValueDescriptorProto_endmsg(void *_r,
                                                        upb_status *status) {
  upb_descreader *r = _r;
  if(!r->saw_number || !r->saw_name) {
    upb_status_setf(status, UPB_ERROR, "Enum value missing name or number.");
    return;
  }
  upb_enumdef *e = upb_downcast_enumdef(upb_descreader_last(r));
  if (upb_inttable_count(&e->iton) == 0) {
    // The default value of an enum (in the absence of an explicit default) is
    // its first listed value.
    upb_enumdef_setdefault(e, r->number);
  }
  upb_enumdef_addval(e, r->name, r->number);
  free(r->name);
  r->name = NULL;
}

static upb_mhandlers *upb_enumdef_register_EnumValueDescriptorProto(
    upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_enumdef_EnumValueDescriptorProto_startmsg);
  upb_mhandlers_setendmsg(m, &upb_enumdef_EnumValueDescriptorProto_endmsg);

#define FNUM(f) GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_ ## f ## __FIELDNUM
#define FTYPE(f) GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_ ## f ## __FIELDTYPE
  upb_fhandlers *f;
  f = upb_mhandlers_newfhandlers(m, FNUM(NAME), FTYPE(NAME), false);
  upb_fhandlers_setvalue(f, &upb_enumdef_EnumValueDescriptorProto_name);

  f = upb_mhandlers_newfhandlers(m, FNUM(NUMBER), FTYPE(NUMBER), false);
  upb_fhandlers_setvalue(f, &upb_enumdef_EnumValueDescriptorProto_number);
  return m;
}
#undef FNUM
#undef FTYPE

// google.protobuf.EnumDescriptorProto.
static upb_flow_t upb_enumdef_EnumDescriptorProto_startmsg(void *_r) {
  upb_descreader *r = _r;
  upb_deflist_push(&r->defs, UPB_UPCAST(upb_enumdef_new()));
  return UPB_CONTINUE;
}

static void upb_enumdef_EnumDescriptorProto_endmsg(void *_r, upb_status *status) {
  upb_descreader *r = _r;
  upb_enumdef *e = upb_downcast_enumdef(upb_descreader_last(r));
  if (upb_descreader_last((upb_descreader*)_r)->fqname == NULL) {
    upb_status_setf(status, UPB_ERROR, "Enum had no name.");
    return;
  }
  if (upb_inttable_count(&e->iton) == 0) {
    upb_status_setf(status, UPB_ERROR, "Enum had no values.");
    return;
  }
}

static upb_flow_t upb_enumdef_EnumDescriptorProto_name(void *_r,
                                                       upb_value fval,
                                                       upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  upb_enumdef *e = upb_downcast_enumdef(upb_descreader_last(r));
  free(e->base.fqname);
  e->base.fqname = upb_strref_dup(upb_value_getstrref(val));
  return UPB_CONTINUE;
}

static upb_mhandlers *upb_enumdef_register_EnumDescriptorProto(upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_enumdef_EnumDescriptorProto_startmsg);
  upb_mhandlers_setendmsg(m, &upb_enumdef_EnumDescriptorProto_endmsg);

#define FNUM(f) GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_ ## f ## __FIELDNUM
#define FTYPE(f) GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_ ## f ## __FIELDTYPE
  upb_fhandlers *f =
      upb_mhandlers_newfhandlers(m, FNUM(NAME), FTYPE(NAME), false);
  upb_fhandlers_setvalue(f, &upb_enumdef_EnumDescriptorProto_name);

  upb_mhandlers_newfhandlers_subm(m, FNUM(VALUE), FTYPE(VALUE), true,
                               upb_enumdef_register_EnumValueDescriptorProto(h));
  return m;
}
#undef FNUM
#undef FTYPE

static upb_flow_t upb_fielddef_startmsg(void *_r) {
  upb_descreader *r = _r;
  r->f = upb_fielddef_new();
  return UPB_CONTINUE;
}

// Converts the default value in string "str" into "d".  Passes a ref on str.
// Returns true on success.
static bool upb_fielddef_parsedefault(char *str, upb_value *d, int type) {
  bool success = true;
  if (type == UPB_TYPE(STRING) || type == UPB_TYPE(BYTES) || type == UPB_TYPE(ENUM)) {
    // We'll keep the ref we had on it.  We include enums in this case because
    // we need the enumdef to resolve the name, but we may not have it yet.
    // We'll resolve it later.
    if (!str) str = strdup("");
    upb_value_setptr(d, str);
  } else if (type == UPB_TYPE(MESSAGE) || type == UPB_TYPE(GROUP)) {
    // We don't expect to get a default value.
    free(str);
    upb_value_setptr(d, NULL);
    if (str != NULL) success = false;
  } else if (type == UPB_TYPE(BOOL)) {
    if (!str || strcmp(str, "false") == 0)
      upb_value_setbool(d, false);
    else if (strcmp(str, "true") == 0)
      upb_value_setbool(d, true);
    else
      success = false;
    free(str);
  } else {
    // The strto* functions need the string to be NULL-terminated.
    if (!str) str = strdup("0");
    char *end;
    switch (type) {
      case UPB_TYPE(INT32):
      case UPB_TYPE(SINT32):
      case UPB_TYPE(SFIXED32): {
        long val = strtol(str, &end, 0);
        if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end)
          success = false;
        else
          upb_value_setint32(d, val);
        break;
      }
      case UPB_TYPE(INT64):
      case UPB_TYPE(SINT64):
      case UPB_TYPE(SFIXED64):
        upb_value_setint64(d, strtoll(str, &end, 0));
        if (errno == ERANGE || *end) success = false;
        break;
      case UPB_TYPE(UINT32):
      case UPB_TYPE(FIXED32): {
        unsigned long val = strtoul(str, &end, 0);
        if (val > UINT32_MAX || errno == ERANGE || *end)
          success = false;
        else
          upb_value_setuint32(d, val);
        break;
      }
      case UPB_TYPE(UINT64):
      case UPB_TYPE(FIXED64):
        upb_value_setuint64(d, strtoull(str, &end, 0));
        if (errno == ERANGE || *end) success = false;
        break;
      case UPB_TYPE(DOUBLE):
        upb_value_setdouble(d, strtod(str, &end));
        if (errno == ERANGE || *end) success = false;
        break;
      case UPB_TYPE(FLOAT):
        upb_value_setfloat(d, strtof(str, &end));
        if (errno == ERANGE || *end) success = false;
        break;
      default: abort();
    }
    free(str);
  }
  return success;
}

static void upb_fielddef_endmsg(void *_r, upb_status *status) {
  upb_descreader *r = _r;
  upb_fielddef *f = r->f;
  // TODO: verify that all required fields were present.
  assert(f->number != -1 && f->name != NULL);
  assert((f->def != NULL) == upb_hassubdef(f));

  // Field was successfully read, add it as a field of the msgdef.
  upb_msgdef *m = upb_descreader_top(r);
  upb_msgdef_addfield(m, f);
  upb_fielddef_unref(f);
  r->f = NULL;
  char *dstr = r->default_string;
  r->default_string = NULL;
  upb_value val;
  upb_value_setptr(&val, NULL);  // Silence inaccurate compiler warnings.
  if (!upb_fielddef_parsedefault(dstr, &val, f->type)) {
    // We don't worry too much about giving a great error message since the
    // compiler should have ensured this was correct.
    upb_status_setf(status, UPB_ERROR, "Error converting default value.");
    return;
  }
  upb_fielddef_setdefault(f, val);
}

static upb_flow_t upb_fielddef_ontype(void *_r, upb_value fval, upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  upb_fielddef_settype(r->f, upb_value_getint32(val));
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_onlabel(void *_r, upb_value fval, upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  upb_fielddef_setlabel(r->f, upb_value_getint32(val));
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_onnumber(void *_r, upb_value fval, upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  upb_fielddef_setnumber(r->f, upb_value_getint32(val));
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_onname(void *_r, upb_value fval, upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  char *name = upb_strref_dup(upb_value_getstrref(val));
  upb_fielddef_setname(r->f, name);
  free(name);
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_ontypename(void *_r, upb_value fval,
                                          upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  char *name = upb_strref_dup(upb_value_getstrref(val));
  upb_fielddef_settypename(r->f, name);
  free(name);
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_ondefaultval(void *_r, upb_value fval,
                                            upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  // Have to convert from string to the correct type, but we might not know the
  // type yet.
  free(r->default_string);
  r->default_string = upb_strref_dup(upb_value_getstrref(val));
  return UPB_CONTINUE;
}

static upb_mhandlers *upb_fielddef_register_FieldDescriptorProto(
    upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_fielddef_startmsg);
  upb_mhandlers_setendmsg(m, &upb_fielddef_endmsg);

#define FIELD(name, handler) \
  upb_fhandlers_setvalue( \
      upb_mhandlers_newfhandlers(m, \
          GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_ ## name ## __FIELDNUM, \
          GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_ ## name ## __FIELDTYPE, \
          false), \
      handler);
  FIELD(TYPE, &upb_fielddef_ontype);
  FIELD(LABEL, &upb_fielddef_onlabel);
  FIELD(NUMBER, &upb_fielddef_onnumber);
  FIELD(NAME, &upb_fielddef_onname);
  FIELD(TYPE_NAME, &upb_fielddef_ontypename);
  FIELD(DEFAULT_VALUE, &upb_fielddef_ondefaultval);
  return m;
}
#undef FNUM
#undef FTYPE


// google.protobuf.DescriptorProto.
static upb_flow_t upb_msgdef_startmsg(void *_r) {
  upb_descreader *r = _r;
  upb_deflist_push(&r->defs, UPB_UPCAST(upb_msgdef_new()));
  upb_descreader_startcontainer(r);
  return UPB_CONTINUE;
}

static void upb_msgdef_endmsg(void *_r, upb_status *status) {
  upb_descreader *r = _r;
  upb_msgdef *m = upb_descreader_top(r);
  if(!m->base.fqname) {
    upb_status_setf(status, UPB_ERROR, "Encountered message with no name.");
    return;
  }

  upb_msgdef_layout(m);
  upb_descreader_endcontainer(r);
}

static upb_flow_t upb_msgdef_onname(void *_r, upb_value fval, upb_value val) {
  (void)fval;
  upb_descreader *r = _r;
  assert(val.type == UPB_TYPE(STRING));
  upb_msgdef *m = upb_descreader_top(r);
  free(m->base.fqname);
  m->base.fqname = upb_strref_dup(upb_value_getstrref(val));
  upb_descreader_setscopename(r, strdup(m->base.fqname));
  return UPB_CONTINUE;
}

static upb_mhandlers *upb_msgdef_register_DescriptorProto(upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_msgdef_startmsg);
  upb_mhandlers_setendmsg(m, &upb_msgdef_endmsg);

#define FNUM(f) GOOGLE_PROTOBUF_DESCRIPTORPROTO_ ## f ## __FIELDNUM
#define FTYPE(f) GOOGLE_PROTOBUF_DESCRIPTORPROTO_ ## f ## __FIELDTYPE
  upb_fhandlers *f =
      upb_mhandlers_newfhandlers(m, FNUM(NAME), FTYPE(NAME), false);
  upb_fhandlers_setvalue(f, &upb_msgdef_onname);

  upb_mhandlers_newfhandlers_subm(m, FNUM(FIELD), FTYPE(FIELD), true,
                                  upb_fielddef_register_FieldDescriptorProto(h));
  upb_mhandlers_newfhandlers_subm(m, FNUM(ENUM_TYPE), FTYPE(ENUM_TYPE), true,
                                  upb_enumdef_register_EnumDescriptorProto(h));

  // DescriptorProto is self-recursive, so we must link the definition.
  upb_mhandlers_newfhandlers_subm(
      m, FNUM(NESTED_TYPE), FTYPE(NESTED_TYPE), true, m);

  // TODO: extensions.
  return m;
}
#undef FNUM
#undef FTYPE

