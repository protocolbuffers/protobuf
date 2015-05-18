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
