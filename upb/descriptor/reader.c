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
#include "upb/def.h"
#include "upb/sink.h"
#include "upb/descriptor/descriptor.upb.h"

static char *upb_strndup(const char *buf, size_t n) {
  char *ret = malloc(n + 1);
  if (!ret) return NULL;
  memcpy(ret, buf, n);
  ret[n] = '\0';
  return ret;
}

// Returns a newly allocated string that joins input strings together, for
// example:
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


/* upb_deflist ****************************************************************/

// upb_deflist is an internal-only dynamic array for storing a growing list of
// upb_defs.
typedef struct {
  upb_pipeline *pipeline;
  upb_def **defs;
  size_t len;
  size_t size;
  bool owned;
} upb_deflist;

void upb_deflist_init(upb_deflist *l, upb_pipeline *pipeline) {
  l->pipeline = pipeline;
  l->size = 0;
  l->defs = NULL;
  l->len = 0;
  l->owned = true;
}

void upb_deflist_uninit(upb_deflist *l) {
  if (l->owned)
    for(size_t i = 0; i < l->len; i++)
      upb_def_unref(l->defs[i], l);
}

bool upb_deflist_push(upb_deflist *l, upb_def *d) {
  if(++l->len >= l->size) {
    size_t new_size = UPB_MAX(l->size, 4);
    new_size *= 2;
    l->defs = upb_pipeline_realloc(
        l->pipeline, l->defs,
        l->size * sizeof(void*), new_size * sizeof(void*));
    if (!l->defs) return false;
    l->size = new_size;
  }
  l->defs[l->len - 1] = d;
  return true;
}

void upb_deflist_donaterefs(upb_deflist *l, void *owner) {
  assert(l->owned);
  for (size_t i = 0; i < l->len; i++)
    upb_def_donateref(l->defs[i], l, owner);
  l->owned = false;
}

static upb_def *upb_deflist_last(upb_deflist *l) {
  return l->defs[l->len-1];
}

// Qualify the defname for all defs starting with offset "start" with "str".
static void upb_deflist_qualify(upb_deflist *l, char *str, int32_t start) {
  for(uint32_t i = start; i < l->len; i++) {
    upb_def *def = l->defs[i];
    char *name = upb_join(str, upb_def_fullname(def));
    upb_def_setfullname(def, name, NULL);
    free(name);
  }
}


/* upb_descreader  ************************************************************/

// We keep a stack of all the messages scopes we are currently in, as well as
// the top-level file scope.  This is necessary to correctly qualify the
// definitions that are contained inside.  "name" tracks the name of the
// message or package (a bare name -- not qualified by any enclosing scopes).
typedef struct {
  char *name;
  // Index of the first def that is under this scope.  For msgdefs, the
  // msgdef itself is at start-1.
  int start;
} upb_descreader_frame;

struct upb_descreader {
  upb_deflist defs;
  upb_descreader_frame stack[UPB_MAX_TYPE_DEPTH];
  int stack_len;

  uint32_t number;
  char *name;
  bool saw_number;
  bool saw_name;

  char *default_string;

  upb_fielddef *f;
};

void upb_descreader_init(void *self, upb_pipeline *p);
void upb_descreader_uninit(void *self);

const upb_frametype upb_descreader_frametype = {
  sizeof(upb_descreader),
  upb_descreader_init,
  upb_descreader_uninit,
  NULL,
};

const upb_frametype *upb_descreader_getframetype() {
  return &upb_descreader_frametype;
}

// Registers handlers that will build the defs.  Pass the descreader as the
// closure.
const upb_handlers *upb_descreader_gethandlers(const void *owner);


/* upb_descreader  ************************************************************/

void upb_descreader_init(void *self, upb_pipeline *pipeline) {
  upb_descreader *r = self;
  upb_deflist_init(&r->defs, pipeline);
  r->stack_len = 0;
  r->name = NULL;
  r->default_string = NULL;
}

void upb_descreader_uninit(void *self) {
  upb_descreader *r = self;
  free(r->name);
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
static bool file_startmsg(void *r, const void *hd) {
  UPB_UNUSED(hd);
  upb_descreader_startcontainer(r);
  return true;
}

static bool file_endmsg(void *closure, const void *hd, upb_status *status) {
  UPB_UNUSED(hd);
  UPB_UNUSED(status);
  upb_descreader *r = closure;
  upb_descreader_endcontainer(r);
  return true;
}

static size_t file_onpackage(void *closure, const void *hd, const char *buf,
                             size_t n) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  // XXX: see comment at the top of the file.
  upb_descreader_setscopename(r, upb_strndup(buf, n));
  return n;
}

// Handlers for google.protobuf.EnumValueDescriptorProto.
static bool enumval_startmsg(void *closure, const void *hd) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  r->saw_number = false;
  r->saw_name = false;
  return true;
}

static size_t enumval_onname(void *closure, const void *hd, const char *buf,
                             size_t n) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  // XXX: see comment at the top of the file.
  free(r->name);
  r->name = upb_strndup(buf, n);
  r->saw_name = true;
  return n;
}

static bool enumval_onnumber(void *closure, const void *hd, int32_t val) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  r->number = val;
  r->saw_number = true;
  return true;
}

static bool enumval_endmsg(void *closure, const void *hd, upb_status *status) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  if(!r->saw_number || !r->saw_name) {
    upb_status_seterrliteral(status, "Enum value missing name or number.");
    return false;
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
  return true;
}


// Handlers for google.protobuf.EnumDescriptorProto.
static bool enum_startmsg(void *closure, const void *hd) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_deflist_push(&r->defs, upb_upcast(upb_enumdef_new(&r->defs)));
  return true;
}

static bool enum_endmsg(void *closure, const void *hd, upb_status *status) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_enumdef *e = upb_downcast_enumdef_mutable(upb_descreader_last(r));
  if (upb_def_fullname(upb_descreader_last(r)) == NULL) {
    upb_status_seterrliteral(status, "Enum had no name.");
    return false;
  }
  if (upb_enumdef_numvals(e) == 0) {
    upb_status_seterrliteral(status, "Enum had no values.");
    return false;
  }
  return true;
}

static size_t enum_onname(void *closure, const void *hd, const char *buf,
                          size_t n) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  // XXX: see comment at the top of the file.
  char *fullname = upb_strndup(buf, n);
  upb_def_setfullname(upb_descreader_last(r), fullname, NULL);
  free(fullname);
  return n;
}

// Handlers for google.protobuf.FieldDescriptorProto
static bool field_startmsg(void *closure, const void *hd) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
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
      case UPB_TYPE_INT32: upb_value_setint32(d, 0); break;
      case UPB_TYPE_INT64: upb_value_setint64(d, 0); break;
      case UPB_TYPE_UINT32: upb_value_setuint32(d, 0);
      case UPB_TYPE_UINT64: upb_value_setuint64(d, 0); break;
      case UPB_TYPE_FLOAT: upb_value_setfloat(d, 0); break;
      case UPB_TYPE_DOUBLE: upb_value_setdouble(d, 0); break;
      case UPB_TYPE_BOOL: upb_value_setbool(d, false); break;
      default: abort();
    }
  } else {
    char *end;
    switch (type) {
      case UPB_TYPE_INT32: {
        long val = strtol(str, &end, 0);
        if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end)
          success = false;
        else
          upb_value_setint32(d, val);
        break;
      }
      case UPB_TYPE_INT64:
        upb_value_setint64(d, strtoll(str, &end, 0));
        if (errno == ERANGE || *end) success = false;
        break;
      case UPB_TYPE_UINT32: {
        unsigned long val = strtoul(str, &end, 0);
        if (val > UINT32_MAX || errno == ERANGE || *end)
          success = false;
        else
          upb_value_setuint32(d, val);
        break;
      }
      case UPB_TYPE_UINT64:
        upb_value_setuint64(d, strtoull(str, &end, 0));
        if (errno == ERANGE || *end) success = false;
        break;
      case UPB_TYPE_DOUBLE:
        upb_value_setdouble(d, strtod(str, &end));
        if (errno == ERANGE || *end) success = false;
        break;
      case UPB_TYPE_FLOAT:
        upb_value_setfloat(d, strtof(str, &end));
        if (errno == ERANGE || *end) success = false;
        break;
      case UPB_TYPE_BOOL: {
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

static bool field_endmsg(void *closure, const void *hd, upb_status *status) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_fielddef *f = r->f;
  // TODO: verify that all required fields were present.
  assert(upb_fielddef_number(f) != 0 && upb_fielddef_name(f) != NULL);
  assert((upb_fielddef_subdefname(f) != NULL) == upb_fielddef_hassubdef(f));

  if (r->default_string) {
    if (upb_fielddef_issubmsg(f)) {
      upb_status_seterrliteral(status, "Submessages cannot have defaults.");
      return false;
    }
    if (upb_fielddef_isstring(f) || upb_fielddef_type(f) == UPB_TYPE_ENUM) {
      upb_fielddef_setdefaultcstr(f, r->default_string, NULL);
    } else {
      upb_value val;
      upb_value_setptr(&val, NULL);  // Silence inaccurate compiler warnings.
      if (!parse_default(r->default_string, &val, upb_fielddef_type(f))) {
        // We don't worry too much about giving a great error message since the
        // compiler should have ensured this was correct.
        upb_status_seterrliteral(status, "Error converting default value.");
        return false;
      }
      upb_fielddef_setdefault(f, val);
    }
  }
  return true;
}

static bool field_ontype(void *closure, const void *hd, int32_t val) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_fielddef_setdescriptortype(r->f, val);
  return true;
}

static bool field_onlabel(void *closure, const void *hd, int32_t val) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_fielddef_setlabel(r->f, val);
  return true;
}

static bool field_onnumber(void *closure, const void *hd, int32_t val) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_fielddef_setnumber(r->f, val, NULL);
  return true;
}

static size_t field_onname(void *closure, const void *hd, const char *buf,
                           size_t n) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  // XXX: see comment at the top of the file.
  char *name = upb_strndup(buf, n);
  upb_fielddef_setname(r->f, name, NULL);
  free(name);
  return n;
}

static size_t field_ontypename(void *closure, const void *hd, const char *buf,
                               size_t n) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  // XXX: see comment at the top of the file.
  char *name = upb_strndup(buf, n);
  upb_fielddef_setsubdefname(r->f, name, NULL);
  free(name);
  return n;
}

static size_t field_ondefaultval(void *closure, const void *hd,
                                 const char *buf, size_t n) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  // Have to convert from string to the correct type, but we might not know the
  // type yet, so we save it as a string until the end of the field.
  // XXX: see comment at the top of the file.
  free(r->default_string);
  r->default_string = upb_strndup(buf, n);
  return n;
}

// Handlers for google.protobuf.DescriptorProto (representing a message).
static bool msg_startmsg(void *closure, const void *hd) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_deflist_push(&r->defs, upb_upcast(upb_msgdef_new(&r->defs)));
  upb_descreader_startcontainer(r);
  return true;
}

static bool msg_endmsg(void *closure, const void *hd, upb_status *status) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  if(!upb_def_fullname(upb_upcast(m))) {
    upb_status_seterrliteral(status, "Encountered message with no name.");
    return false;
  }
  upb_descreader_endcontainer(r);
  return true;
}

static size_t msg_onname(void *closure, const void *hd, const char *buf,
                         size_t n) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  // XXX: see comment at the top of the file.
  char *name = upb_strndup(buf, n);
  upb_def_setfullname(upb_upcast(m), name, NULL);
  upb_descreader_setscopename(r, name);  // Passes ownership of name.
  return n;
}

static bool msg_onendfield(void *closure, const void *hd) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  upb_msgdef_addfield(m, r->f, &r->defs, NULL);
  r->f = NULL;
  return true;
}

static bool discardfield(void *closure, const void *hd) {
  UPB_UNUSED(hd);
  upb_descreader *r = closure;
  // Discard extension field so we don't leak it.
  upb_fielddef_unref(r->f, &r->defs);
  r->f = NULL;
  return true;
}

static const upb_fielddef *f(const upb_handlers *h, const char *name) {
  const upb_fielddef *ret = upb_msgdef_ntof(upb_handlers_msgdef(h), name);
  assert(ret);
  return ret;
}

static void reghandlers(void *closure, upb_handlers *h) {
  UPB_UNUSED(closure);
  const upb_msgdef *m = upb_handlers_msgdef(h);

  if (m == GOOGLE_PROTOBUF_DESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &msg_startmsg, NULL, NULL);
    upb_handlers_setendmsg(h, &msg_endmsg, NULL, NULL);
    upb_handlers_setstring(h,    f(h, "name"),  &msg_onname, NULL, NULL);
    upb_handlers_setendsubmsg(h, f(h, "field"), &msg_onendfield, NULL, NULL);
    // TODO: support extensions
    upb_handlers_setendsubmsg(h, f(h, "extension"), &discardfield, NULL, NULL);
  } else if (m == GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &file_startmsg, NULL, NULL);
    upb_handlers_setendmsg(h, &file_endmsg, NULL, NULL);
    upb_handlers_setstring(h, f(h, "package"), &file_onpackage, NULL, NULL);
    // TODO: support extensions
    upb_handlers_setendsubmsg(h, f(h, "extension"), &discardfield, NULL, NULL);
  } else if (m == GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &enumval_startmsg, NULL, NULL);
    upb_handlers_setendmsg(h, &enumval_endmsg, NULL, NULL);
    upb_handlers_setstring(h, f(h, "name"),   &enumval_onname, NULL, NULL);
    upb_handlers_setint32(h,  f(h, "number"), &enumval_onnumber, NULL, NULL);
  } else if (m == GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &enum_startmsg, NULL, NULL);
    upb_handlers_setendmsg(h, &enum_endmsg, NULL, NULL);
    upb_handlers_setstring(h, f(h, "name"), &enum_onname, NULL, NULL);
  } else if (m == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO) {
    upb_handlers_setstartmsg(h, &field_startmsg, NULL, NULL);
    upb_handlers_setendmsg(h, &field_endmsg, NULL, NULL);
    upb_handlers_setint32 (h, f(h, "type"),      &field_ontype, NULL, NULL);
    upb_handlers_setint32 (h, f(h, "label"),     &field_onlabel, NULL, NULL);
    upb_handlers_setint32 (h, f(h, "number"),    &field_onnumber, NULL, NULL);
    upb_handlers_setstring(h, f(h, "name"),      &field_onname, NULL, NULL);
    upb_handlers_setstring(h, f(h, "type_name"), &field_ontypename, NULL, NULL);
    upb_handlers_setstring(h, f(h, "default_value"), &field_ondefaultval, NULL,
                           NULL);
  }
}

const upb_handlers *upb_descreader_gethandlers(const void *owner) {
  return upb_handlers_newfrozen(
      GOOGLE_PROTOBUF_FILEDESCRIPTORSET, &upb_descreader_frametype,
      owner, reghandlers, NULL);
}
