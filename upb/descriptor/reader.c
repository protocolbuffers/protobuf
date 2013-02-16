/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * XXX: The routines in this file that consume a string do not currently
 * support having the string span buffers.  In the future, as upb_sink and
 * its buffering/sharing functionality evolve there should be an easy and
 * idiomatic way of correctly handling this case.  For now, we accept this
 * limitation since we currently only parse descriptors from single strings.
 */

#include "upb/descriptor/reader.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "upb/bytestream.h"
#include "upb/def.h"
#include "upb/descriptor/descriptor.upb.h"

static char *upb_strndup(const char *buf, size_t n) {
  char *ret = malloc(n + 1);
  if (!ret) return NULL;
  memcpy(ret, buf, n);
  ret[n] = '\0';
  return ret;
}

// Returns a newly allocated string that joins input strings together, for example:
//   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
//   join("", "Baz") -> "Baz"
// Caller owns a ref on the returned string.
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

void upb_deflist_init(upb_deflist *l) {
  l->size = 8;
  l->defs = malloc(l->size * sizeof(void*));
  l->len = 0;
  l->owned = true;
}

void upb_deflist_uninit(upb_deflist *l) {
  if (l->owned)
    for(size_t i = 0; i < l->len; i++)
      upb_def_unref(l->defs[i], &l->defs);
  free(l->defs);
}

void upb_deflist_push(upb_deflist *l, upb_def *d) {
  if(l->len == l->size) {
    l->size *= 2;
    l->defs = realloc(l->defs, l->size * sizeof(void*));
  }
  l->defs[l->len++] = d;
}

void upb_deflist_donaterefs(upb_deflist *l, void *owner) {
  assert(l->owned);
  for (size_t i = 0; i < l->len; i++)
    upb_def_donateref(l->defs[i], &l->defs, owner);
  l->owned = false;
}


/* upb_descreader  ************************************************************/

static upb_def *upb_deflist_last(upb_deflist *l) {
  return l->defs[l->len-1];
}

// Qualify the defname for all defs starting with offset "start" with "str".
static void upb_deflist_qualify(upb_deflist *l, char *str, int32_t start) {
  for(uint32_t i = start; i < l->len; i++) {
    upb_def *def = l->defs[i];
    char *name = upb_join(str, upb_def_fullname(def));
    upb_def_setfullname(def, name);
    free(name);
  }
}

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

upb_def **upb_descreader_getdefs(upb_descreader *r, void *owner, int *n) {
  *n = r->defs.len;
  upb_deflist_donaterefs(&r->defs, owner);
  return r->defs.defs;
}

static upb_msgdef *upb_descreader_top(upb_descreader *r) {
  if (r->stack_len <= 1) return NULL;
  int index = r->stack[r->stack_len-1].start - 1;
  assert(index >= 0);
  return upb_downcast_msgdef_mutable(r->defs.defs[index]);
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
static bool file_startmsg(void *_r) {
  upb_descreader *r = _r;
  upb_descreader_startcontainer(r);
  return true;
}

static void file_endmsg(void *_r, upb_status *status) {
  UPB_UNUSED(status);
  upb_descreader *r = _r;
  upb_descreader_endcontainer(r);
}

static size_t file_onpackage(void *_r, void *fval, const char *buf, size_t n) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  // XXX: see comment at the top of the file.
  upb_descreader_setscopename(r, upb_strndup(buf, n));
  return n;
}

// Handlers for google.protobuf.EnumValueDescriptorProto.
static bool enumval_startmsg(void *_r) {
  upb_descreader *r = _r;
  r->saw_number = false;
  r->saw_name = false;
  return true;
}

static size_t enumval_onname(void *_r, void *fval, const char *buf, size_t n) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  // XXX: see comment at the top of the file.
  free(r->name);
  r->name = upb_strndup(buf, n);
  r->saw_name = true;
  return n;
}

static bool enumval_onnumber(void *_r, void *fval, int32_t val) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  r->number = val;
  r->saw_number = true;
  return true;
}

static void enumval_endmsg(void *_r, upb_status *status) {
  upb_descreader *r = _r;
  if(!r->saw_number || !r->saw_name) {
    upb_status_seterrliteral(status, "Enum value missing name or number.");
    return;
  }
  upb_enumdef *e = upb_downcast_enumdef_mutable(upb_descreader_last(r));
  if (upb_enumdef_numvals(e) == 0) {
    // The default value of an enum (in the absence of an explicit default) is
    // its first listed value.
    upb_enumdef_setdefault(e, r->number);
  }
  upb_enumdef_addval(e, r->name, r->number, status);
  free(r->name);
  r->name = NULL;
}


// Handlers for google.protobuf.EnumDescriptorProto.
static bool enum_startmsg(void *_r) {
  upb_descreader *r = _r;
  upb_deflist_push(&r->defs, upb_upcast(upb_enumdef_new(&r->defs)));
  return true;
}

static void enum_endmsg(void *_r, upb_status *status) {
  upb_descreader *r = _r;
  upb_enumdef *e = upb_downcast_enumdef_mutable(upb_descreader_last(r));
  if (upb_def_fullname(upb_descreader_last((upb_descreader*)_r)) == NULL) {
    upb_status_seterrliteral(status, "Enum had no name.");
    return;
  }
  if (upb_enumdef_numvals(e) == 0) {
    upb_status_seterrliteral(status, "Enum had no values.");
    return;
  }
}

static size_t enum_onname(void *_r, void *fval, const char *buf, size_t n) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  // XXX: see comment at the top of the file.
  char *fullname = upb_strndup(buf, n);
  upb_def_setfullname(upb_descreader_last(r), fullname);
  free(fullname);
  return n;
}

// Handlers for google.protobuf.FieldDescriptorProto
static bool field_startmsg(void *_r) {
  upb_descreader *r = _r;
  r->f = upb_fielddef_new(&r->defs);
  free(r->default_string);
  r->default_string = NULL;
  return true;
}

// Converts the default value in string "str" into "d".  Passes a ref on str.
// Returns true on success.
static bool parse_default(char *str, upb_value *d, int type) {
  bool success = true;
  if (str) {
    switch(type) {
      case UPB_TYPE(INT32):
      case UPB_TYPE(SINT32):
      case UPB_TYPE(SFIXED32): upb_value_setint32(d, 0); break;
      case UPB_TYPE(INT64):
      case UPB_TYPE(SINT64):
      case UPB_TYPE(SFIXED64): upb_value_setint64(d, 0); break;
      case UPB_TYPE(UINT32):
      case UPB_TYPE(FIXED32): upb_value_setuint32(d, 0);
      case UPB_TYPE(UINT64):
      case UPB_TYPE(FIXED64): upb_value_setuint64(d, 0); break;
      case UPB_TYPE(DOUBLE): upb_value_setdouble(d, 0); break;
      case UPB_TYPE(FLOAT): upb_value_setfloat(d, 0); break;
      case UPB_TYPE(BOOL): upb_value_setbool(d, false); break;
      default: abort();
    }
  } else {
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
      case UPB_TYPE(BOOL): {
        if (strcmp(str, "false") == 0)
          upb_value_setbool(d, false);
        else if (strcmp(str, "true") == 0)
          upb_value_setbool(d, true);
        else
          success = false;
      }
      default: abort();
    }
  }
  return success;
}

static void field_endmsg(void *_r, upb_status *status) {
  upb_descreader *r = _r;
  upb_fielddef *f = r->f;
  // TODO: verify that all required fields were present.
  assert(upb_fielddef_number(f) != 0 && upb_fielddef_name(f) != NULL);
  assert((upb_fielddef_subdefname(f) != NULL) == upb_fielddef_hassubdef(f));

  if (r->default_string) {
    if (upb_fielddef_issubmsg(f)) {
      upb_status_seterrliteral(status, "Submessages cannot have defaults.");
      return;
    }
    if (upb_fielddef_isstring(f) || upb_fielddef_type(f) == UPB_TYPE(ENUM)) {
      upb_fielddef_setdefaultcstr(f, r->default_string);
    } else {
      upb_value val;
      upb_value_setptr(&val, NULL);  // Silence inaccurate compiler warnings.
      if (!parse_default(r->default_string, &val, upb_fielddef_type(f))) {
        // We don't worry too much about giving a great error message since the
        // compiler should have ensured this was correct.
        upb_status_seterrliteral(status, "Error converting default value.");
        return;
      }
      upb_fielddef_setdefault(f, val);
    }
  }
}

static bool field_ontype(void *_r, void *fval, int32_t val) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  upb_fielddef_settype(r->f, val);
  return true;
}

static bool field_onlabel(void *_r, void *fval, int32_t val) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  upb_fielddef_setlabel(r->f, val);
  return true;
}

static bool field_onnumber(void *_r, void *fval, int32_t val) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  upb_fielddef_setnumber(r->f, val);
  return true;
}

static size_t field_onname(void *_r, void *fval, const char *buf, size_t n) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  // XXX: see comment at the top of the file.
  char *name = upb_strndup(buf, n);
  upb_fielddef_setname(r->f, name);
  free(name);
  return n;
}

static size_t field_ontypename(void *_r, void *fval, const char *buf,
                               size_t n) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  // XXX: see comment at the top of the file.
  char *name = upb_strndup(buf, n);
  upb_fielddef_setsubdefname(r->f, name);
  free(name);
  return n;
}

static size_t field_ondefaultval(void *_r, void *fval, const char *buf,
                                 size_t n) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  // Have to convert from string to the correct type, but we might not know the
  // type yet, so we save it as a string until the end of the field.
  // XXX: see comment at the top of the file.
  free(r->default_string);
  r->default_string = upb_strndup(buf, n);
  return n;
}

// Handlers for google.protobuf.DescriptorProto (representing a message).
static bool msg_startmsg(void *_r) {
  upb_descreader *r = _r;
  upb_deflist_push(&r->defs, upb_upcast(upb_msgdef_new(&r->defs)));
  upb_descreader_startcontainer(r);
  return true;
}

static void msg_endmsg(void *_r, upb_status *status) {
  upb_descreader *r = _r;
  upb_msgdef *m = upb_descreader_top(r);
  if(!upb_def_fullname(upb_upcast(m))) {
    upb_status_seterrliteral(status, "Encountered message with no name.");
    return;
  }
  upb_descreader_endcontainer(r);
}

static size_t msg_onname(void *_r, void *fval, const char *buf, size_t n) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  upb_msgdef *m = upb_descreader_top(r);
  // XXX: see comment at the top of the file.
  char *name = upb_strndup(buf, n);
  upb_def_setfullname(upb_upcast(m), name);
  upb_descreader_setscopename(r, name);  // Passes ownership of name.
  return n;
}

static bool msg_onendfield(void *_r, void *fval) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  upb_msgdef *m = upb_descreader_top(r);
  upb_msgdef_addfield(m, r->f, &r->defs);
  r->f = NULL;
  return true;
}

static bool discardfield(void *_r, void *fval) {
  UPB_UNUSED(fval);
  upb_descreader *r = _r;
  // Discard extension field so we don't leak it.
  upb_fielddef_unref(r->f, &r->defs);
  r->f = NULL;
  return true;
}

static void reghandlers(void *closure, upb_handlers *h) {
  UPB_UNUSED(closure);
  const upb_msgdef *m = upb_handlers_msgdef(h);

  if (m == GOOGLE_PROTOBUF_DESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &msg_startmsg);
    upb_handlers_setendmsg(h, &msg_endmsg);
    upb_handlers_setstring_n(h, "name",  &msg_onname, NULL, NULL);
    upb_handlers_setendsubmsg_n(h,   "field", &msg_onendfield, NULL, NULL);
    // TODO: support extensions
    upb_handlers_setendsubmsg_n(h, "extension", &discardfield, NULL, NULL);
  } else if (m == GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &file_startmsg);
    upb_handlers_setendmsg(h, &file_endmsg);
    upb_handlers_setstring_n(h, "package", &file_onpackage, NULL, NULL);
    // TODO: support extensions
    upb_handlers_setendsubmsg_n(h, "extension", &discardfield, NULL, NULL);
  } else if (m == GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &enumval_startmsg);
    upb_handlers_setendmsg(h, &enumval_endmsg);
    upb_handlers_setstring_n(h, "name",   &enumval_onname, NULL, NULL);
    upb_handlers_setint32_n(h,  "number", &enumval_onnumber, NULL, NULL);
  } else if (m == GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &enum_startmsg);
    upb_handlers_setendmsg(h, &enum_endmsg);
    upb_handlers_setstring_n(h, "name", &enum_onname, NULL, NULL);
  } else if (m == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &field_startmsg);
    upb_handlers_setendmsg(h, &field_endmsg);
    upb_handlers_setint32_n (h, "type",      &field_ontype, NULL, NULL);
    upb_handlers_setint32_n (h, "label",     &field_onlabel, NULL, NULL);
    upb_handlers_setint32_n (h, "number",    &field_onnumber, NULL, NULL);
    upb_handlers_setstring_n(h, "name",      &field_onname, NULL, NULL);
    upb_handlers_setstring_n(h, "type_name", &field_ontypename, NULL, NULL);
    upb_handlers_setstring_n(
        h, "default_value", &field_ondefaultval, NULL, NULL);
  }
}

const upb_handlers *upb_descreader_newhandlers(const void *owner) {
  return upb_handlers_newfrozen(
      GOOGLE_PROTOBUF_FILEDESCRIPTORSET, owner, reghandlers, NULL);
}
