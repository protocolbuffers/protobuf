/*
** XXX: The routines in this file that consume a string do not currently
** support having the string span buffers.  In the future, as upb_sink and
** its buffering/sharing functionality evolve there should be an easy and
** idiomatic way of correctly handling this case.  For now, we accept this
** limitation since we currently only parse descriptors from single strings.
*/

#include "upb/descriptor/reader.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "upb/def.h"
#include "upb/sink.h"
#include "upb/descriptor/descriptor.upbdefs.h"

/* Compares a NULL-terminated string with a non-NULL-terminated string. */
static bool upb_streq(const char *str, const char *buf, size_t n) {
  return strlen(str) == n && memcmp(str, buf, n) == 0;
}

/* We keep a stack of all the messages scopes we are currently in, as well as
 * the top-level file scope.  This is necessary to correctly qualify the
 * definitions that are contained inside.  "name" tracks the name of the
 * message or package (a bare name -- not qualified by any enclosing scopes). */
typedef struct {
  char *name;
  /* Index of the first def that is under this scope.  For msgdefs, the
   * msgdef itself is at start-1. */
  int start;
  uint32_t oneof_start;
  uint32_t oneof_index;
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
  upb_inttable files;
  upb_strtable files_by_name;
  upb_filedef *file;  /* The last file in files. */
  upb_descreader_frame stack[UPB_MAX_MESSAGE_NESTING];
  int stack_len;
  upb_inttable oneofs;

  uint32_t number;
  char *name;
  bool saw_number;
  bool saw_name;

  char *default_string;

  upb_fielddef *f;
};

static char *upb_gstrndup(const char *buf, size_t n) {
  char *ret = upb_gmalloc(n + 1);
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
    return upb_gstrdup(name);
  } else {
    char *ret = upb_gmalloc(strlen(base) + strlen(name) + 2);
    if (!ret) {
      return NULL;
    }
    ret[0] = '\0';
    strcat(ret, base);
    strcat(ret, ".");
    strcat(ret, name);
    return ret;
  }
}

/* Qualify the defname for all defs starting with offset "start" with "str". */
static bool upb_descreader_qualify(upb_filedef *f, char *str, int32_t start) {
  size_t i;
  for (i = start; i < upb_filedef_defcount(f); i++) {
    upb_def *def = upb_filedef_mutabledef(f, i);
    char *name = upb_join(str, upb_def_fullname(def));
    if (!name) {
      /* Need better logic here; at this point we've qualified some names but
       * not others. */
      return false;
    }
    upb_def_setfullname(def, name, NULL);
    upb_gfree(name);
  }
  return true;
}


/* upb_descreader  ************************************************************/

static upb_msgdef *upb_descreader_top(upb_descreader *r) {
  int index;
  UPB_ASSERT(r->stack_len > 1);
  index = r->stack[r->stack_len-1].start - 1;
  UPB_ASSERT(index >= 0);
  return upb_downcast_msgdef_mutable(upb_filedef_mutabledef(r->file, index));
}

static upb_def *upb_descreader_last(upb_descreader *r) {
  return upb_filedef_mutabledef(r->file, upb_filedef_defcount(r->file) - 1);
}

/* Start/end handlers for FileDescriptorProto and DescriptorProto (the two
 * entities that have names and can contain sub-definitions. */
void upb_descreader_startcontainer(upb_descreader *r) {
  upb_descreader_frame *f = &r->stack[r->stack_len++];
  f->start = upb_filedef_defcount(r->file);
  f->oneof_start = upb_inttable_count(&r->oneofs);
  f->oneof_index = 0;
  f->name = NULL;
}

bool upb_descreader_endcontainer(upb_descreader *r) {
  upb_descreader_frame *f = &r->stack[r->stack_len - 1];

  while (upb_inttable_count(&r->oneofs) > f->oneof_start) {
    upb_oneofdef *o = upb_value_getptr(upb_inttable_pop(&r->oneofs));
    bool ok = upb_msgdef_addoneof(upb_descreader_top(r), o, &r->oneofs, NULL);
    UPB_ASSERT(ok);
  }

  if (!upb_descreader_qualify(r->file, f->name, f->start)) {
    return false;
  }
  upb_gfree(f->name);
  f->name = NULL;

  r->stack_len--;
  return true;
}

void upb_descreader_setscopename(upb_descreader *r, char *str) {
  upb_descreader_frame *f = &r->stack[r->stack_len-1];
  upb_gfree(f->name);
  f->name = str;
}

static upb_oneofdef *upb_descreader_getoneof(upb_descreader *r,
                                             uint32_t index) {
  bool found;
  upb_value val;
  upb_descreader_frame *f = &r->stack[r->stack_len-1];

  /* DescriptorProto messages can be nested, so we will see the nested messages
   * between when we see the FieldDescriptorProto and the OneofDescriptorProto.
   * We need to preserve the oneofs in between these two things. */
  index += f->oneof_start;

  while (upb_inttable_count(&r->oneofs) <= index) {
    upb_inttable_push(&r->oneofs, upb_value_ptr(upb_oneofdef_new(&r->oneofs)));
  }

  found = upb_inttable_lookup(&r->oneofs, index, &val);
  UPB_ASSERT(found);
  return upb_value_getptr(val);
}

/** Handlers for google.protobuf.FileDescriptorSet. ***************************/

static void *fileset_startfile(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  r->file = upb_filedef_new(&r->files);
  upb_inttable_push(&r->files, upb_value_ptr(r->file));
  return r;
}

/** Handlers for google.protobuf.FileDescriptorProto. *************************/

static bool file_start(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  upb_descreader_startcontainer(r);
  return true;
}

static bool file_end(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_UNUSED(status);
  return upb_descreader_endcontainer(r);
}

static size_t file_onname(void *closure, const void *hd, const char *buf,
                          size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  name = upb_gstrndup(buf, n);
  upb_strtable_insert(&r->files_by_name, name, upb_value_ptr(r->file));
  /* XXX: see comment at the top of the file. */
  ok = upb_filedef_setname(r->file, name, NULL);
  upb_gfree(name);
  UPB_ASSERT(ok);
  return n;
}

static size_t file_onpackage(void *closure, const void *hd, const char *buf,
                             size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *package;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  package = upb_gstrndup(buf, n);
  /* XXX: see comment at the top of the file. */
  upb_descreader_setscopename(r, package);
  ok = upb_filedef_setpackage(r->file, package, NULL);
  UPB_ASSERT(ok);
  return n;
}

static void *file_startphpnamespace(void *closure, const void *hd,
                                    size_t size_hint) {
  upb_descreader *r = closure;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(size_hint);

  ok = upb_filedef_setphpnamespace(r->file, "", NULL);
  UPB_ASSERT(ok);
  return closure;
}

static size_t file_onphpnamespace(void *closure, const void *hd,
                                  const char *buf, size_t n,
                                  const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *php_namespace;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  php_namespace = upb_gstrndup(buf, n);
  ok = upb_filedef_setphpnamespace(r->file, php_namespace, NULL);
  upb_gfree(php_namespace);
  UPB_ASSERT(ok);
  return n;
}

static size_t file_onphpprefix(void *closure, const void *hd, const char *buf,
                             size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *prefix;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  prefix = upb_gstrndup(buf, n);
  ok = upb_filedef_setphpprefix(r->file, prefix, NULL);
  upb_gfree(prefix);
  UPB_ASSERT(ok);
  return n;
}

static size_t file_onsyntax(void *closure, const void *hd, const char *buf,
                            size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  bool ok;
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  /* XXX: see comment at the top of the file. */
  if (upb_streq("proto2", buf, n)) {
    ok = upb_filedef_setsyntax(r->file, UPB_SYNTAX_PROTO2, NULL);
  } else if (upb_streq("proto3", buf, n)) {
    ok = upb_filedef_setsyntax(r->file, UPB_SYNTAX_PROTO3, NULL);
  } else {
    ok = false;
  }

  UPB_ASSERT(ok);
  return n;
}

static void *file_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_msgdef_new(&m);
  bool ok = upb_filedef_addmsg(r->file, m, &m, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static void *file_startenum(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_enumdef *e = upb_enumdef_new(&e);
  bool ok = upb_filedef_addenum(r->file, e, &e, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static void *file_startext(void *closure, const void *hd) {
  upb_descreader *r = closure;
  bool ok;
  r->f = upb_fielddef_new(r);
  ok = upb_filedef_addext(r->file, r->f, r, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static size_t file_ondep(void *closure, const void *hd, const char *buf,
                         size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  upb_value val;
  if (upb_strtable_lookup2(&r->files_by_name, buf, n, &val)) {
    upb_filedef_adddep(r->file, upb_value_getptr(val));
  }
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  return n;
}

/** Handlers for google.protobuf.EnumValueDescriptorProto. *********************/

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
  upb_gfree(r->name);
  r->name = upb_gstrndup(buf, n);
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
  upb_gfree(r->name);
  r->name = NULL;
  return true;
}

/** Handlers for google.protobuf.EnumDescriptorProto. *************************/

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
  char *fullname = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  /* XXX: see comment at the top of the file. */
  upb_def_setfullname(upb_descreader_last(r), fullname, NULL);
  upb_gfree(fullname);
  return n;
}

/** Handlers for google.protobuf.FieldDescriptorProto *************************/

static bool field_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);
  UPB_ASSERT(r->f);
  upb_gfree(r->default_string);
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
  UPB_ASSERT(upb_fielddef_number(f) != 0);
  UPB_ASSERT(upb_fielddef_name(f) != NULL);
  UPB_ASSERT((upb_fielddef_subdefname(f) != NULL) == upb_fielddef_hassubdef(f));

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
  bool ok;
  UPB_UNUSED(hd);

  ok = upb_fielddef_setnumber(r->f, val, NULL);
  UPB_ASSERT(ok);
  return true;
}

static size_t field_onname(void *closure, const void *hd, const char *buf,
                           size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setname(r->f, name, NULL);
  upb_gfree(name);
  return n;
}

static size_t field_ontypename(void *closure, const void *hd, const char *buf,
                               size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setsubdefname(r->f, name, NULL);
  upb_gfree(name);
  return n;
}

static size_t field_onextendee(void *closure, const void *hd, const char *buf,
                               size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  char *name = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  /* XXX: see comment at the top of the file. */
  upb_fielddef_setcontainingtypename(r->f, name, NULL);
  upb_gfree(name);
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
  upb_gfree(r->default_string);
  r->default_string = upb_gstrndup(buf, n);
  return n;
}

static bool field_ononeofindex(void *closure, const void *hd, int32_t index) {
  upb_descreader *r = closure;
  upb_oneofdef *o = upb_descreader_getoneof(r, index);
  bool ok = upb_oneofdef_addfield(o, r->f, &r->f, NULL);
  UPB_UNUSED(hd);

  UPB_ASSERT(ok);
  return true;
}

/** Handlers for google.protobuf.OneofDescriptorProto. ************************/

static size_t oneof_name(void *closure, const void *hd, const char *buf,
                         size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  upb_descreader_frame *f = &r->stack[r->stack_len-1];
  upb_oneofdef *o = upb_descreader_getoneof(r, f->oneof_index++);
  char *name_null_terminated = upb_gstrndup(buf, n);
  bool ok = upb_oneofdef_setname(o, name_null_terminated, NULL);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  UPB_ASSERT(ok);
  free(name_null_terminated);
  return n;
}

/** Handlers for google.protobuf.DescriptorProto ******************************/

static bool msg_start(void *closure, const void *hd) {
  upb_descreader *r = closure;
  UPB_UNUSED(hd);

  upb_descreader_startcontainer(r);
  return true;
}

static bool msg_end(void *closure, const void *hd, upb_status *status) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  UPB_UNUSED(hd);

  if(!upb_def_fullname(upb_msgdef_upcast_mutable(m))) {
    upb_status_seterrmsg(status, "Encountered message with no name.");
    return false;
  }
  return upb_descreader_endcontainer(r);
}

static size_t msg_name(void *closure, const void *hd, const char *buf,
                       size_t n, const upb_bufhandle *handle) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  /* XXX: see comment at the top of the file. */
  char *name = upb_gstrndup(buf, n);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  upb_def_setfullname(upb_msgdef_upcast_mutable(m), name, NULL);
  upb_descreader_setscopename(r, name);  /* Passes ownership of name. */

  return n;
}

static void *msg_startmsg(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_msgdef_new(&m);
  bool ok = upb_filedef_addmsg(r->file, m, &m, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static void *msg_startext(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_fielddef *f = upb_fielddef_new(&f);
  bool ok = upb_filedef_addext(r->file, f, &f, NULL);
  UPB_UNUSED(hd);
  UPB_ASSERT(ok);
  return r;
}

static void *msg_startfield(void *closure, const void *hd) {
  upb_descreader *r = closure;
  r->f = upb_fielddef_new(&r->f);
  /* We can't add the new field to the message until its name/number are
   * filled in. */
  UPB_UNUSED(hd);
  return r;
}

static bool msg_endfield(void *closure, const void *hd) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  bool ok;
  UPB_UNUSED(hd);

  /* Oneof fields are added to the msgdef through their oneof, so don't need to
   * be added here. */
  if (upb_fielddef_containingoneof(r->f) == NULL) {
    ok = upb_msgdef_addfield(m, r->f, &r->f, NULL);
    UPB_ASSERT(ok);
  }
  r->f = NULL;
  return true;
}

static bool msg_onmapentry(void *closure, const void *hd, bool mapentry) {
  upb_descreader *r = closure;
  upb_msgdef *m = upb_descreader_top(r);
  UPB_UNUSED(hd);

  upb_msgdef_setmapentry(m, mapentry);
  r->f = NULL;
  return true;
}



/** Code to register handlers *************************************************/

#define F(msg, field) upbdefs_google_protobuf_ ## msg ## _f_ ## field(m)

static void reghandlers(const void *closure, upb_handlers *h) {
  const upb_msgdef *m = upb_handlers_msgdef(h);
  UPB_UNUSED(closure);

  if (upbdefs_google_protobuf_FileDescriptorSet_is(m)) {
    upb_handlers_setstartsubmsg(h, F(FileDescriptorSet, file),
                                &fileset_startfile, NULL);
  } else if (upbdefs_google_protobuf_DescriptorProto_is(m)) {
    upb_handlers_setstartmsg(h, &msg_start, NULL);
    upb_handlers_setendmsg(h, &msg_end, NULL);
    upb_handlers_setstring(h, F(DescriptorProto, name), &msg_name, NULL);
    upb_handlers_setstartsubmsg(h, F(DescriptorProto, extension), &msg_startext,
                                NULL);
    upb_handlers_setstartsubmsg(h, F(DescriptorProto, nested_type),
                                &msg_startmsg, NULL);
    upb_handlers_setstartsubmsg(h, F(DescriptorProto, field),
                                &msg_startfield, NULL);
    upb_handlers_setendsubmsg(h, F(DescriptorProto, field),
                              &msg_endfield, NULL);
    upb_handlers_setstartsubmsg(h, F(DescriptorProto, enum_type),
                                &file_startenum, NULL);
  } else if (upbdefs_google_protobuf_FileDescriptorProto_is(m)) {
    upb_handlers_setstartmsg(h, &file_start, NULL);
    upb_handlers_setendmsg(h, &file_end, NULL);
    upb_handlers_setstring(h, F(FileDescriptorProto, name), &file_onname,
                           NULL);
    upb_handlers_setstring(h, F(FileDescriptorProto, package), &file_onpackage,
                           NULL);
    upb_handlers_setstring(h, F(FileDescriptorProto, syntax), &file_onsyntax,
                           NULL);
    upb_handlers_setstartsubmsg(h, F(FileDescriptorProto, message_type),
                                &file_startmsg, NULL);
    upb_handlers_setstartsubmsg(h, F(FileDescriptorProto, enum_type),
                                &file_startenum, NULL);
    upb_handlers_setstartsubmsg(h, F(FileDescriptorProto, extension),
                                &file_startext, NULL);
    upb_handlers_setstring(h, F(FileDescriptorProto, dependency),
                           &file_ondep, NULL);
  } else if (upbdefs_google_protobuf_EnumValueDescriptorProto_is(m)) {
    upb_handlers_setstartmsg(h, &enumval_startmsg, NULL);
    upb_handlers_setendmsg(h, &enumval_endmsg, NULL);
    upb_handlers_setstring(h, F(EnumValueDescriptorProto, name), &enumval_onname, NULL);
    upb_handlers_setint32(h, F(EnumValueDescriptorProto, number), &enumval_onnumber,
                          NULL);
  } else if (upbdefs_google_protobuf_EnumDescriptorProto_is(m)) {
    upb_handlers_setendmsg(h, &enum_endmsg, NULL);
    upb_handlers_setstring(h, F(EnumDescriptorProto, name), &enum_onname, NULL);
  } else if (upbdefs_google_protobuf_FieldDescriptorProto_is(m)) {
    upb_handlers_setstartmsg(h, &field_startmsg, NULL);
    upb_handlers_setendmsg(h, &field_endmsg, NULL);
    upb_handlers_setint32(h, F(FieldDescriptorProto, type), &field_ontype,
                          NULL);
    upb_handlers_setint32(h, F(FieldDescriptorProto, label), &field_onlabel,
                          NULL);
    upb_handlers_setint32(h, F(FieldDescriptorProto, number), &field_onnumber,
                          NULL);
    upb_handlers_setstring(h, F(FieldDescriptorProto, name), &field_onname,
                           NULL);
    upb_handlers_setstring(h, F(FieldDescriptorProto, type_name),
                           &field_ontypename, NULL);
    upb_handlers_setstring(h, F(FieldDescriptorProto, extendee),
                           &field_onextendee, NULL);
    upb_handlers_setstring(h, F(FieldDescriptorProto, default_value),
                           &field_ondefaultval, NULL);
    upb_handlers_setint32(h, F(FieldDescriptorProto, oneof_index),
                          &field_ononeofindex, NULL);
  } else if (upbdefs_google_protobuf_OneofDescriptorProto_is(m)) {
    upb_handlers_setstring(h, F(OneofDescriptorProto, name), &oneof_name, NULL);
  } else if (upbdefs_google_protobuf_FieldOptions_is(m)) {
    upb_handlers_setbool(h, F(FieldOptions, lazy), &field_onlazy, NULL);
    upb_handlers_setbool(h, F(FieldOptions, packed), &field_onpacked, NULL);
  } else if (upbdefs_google_protobuf_MessageOptions_is(m)) {
    upb_handlers_setbool(h, F(MessageOptions, map_entry), &msg_onmapentry, NULL);
  } else if (upbdefs_google_protobuf_FileOptions_is(m)) {
    upb_handlers_setstring(h, F(FileOptions, php_class_prefix),
                           &file_onphpprefix, NULL);
    upb_handlers_setstartstr(h, F(FileOptions, php_namespace),
                             &file_startphpnamespace, NULL);
    upb_handlers_setstring(h, F(FileOptions, php_namespace),
                           &file_onphpnamespace, NULL);
  }

  UPB_ASSERT(upb_ok(upb_handlers_status(h)));
}

#undef F

void descreader_cleanup(void *_r) {
  upb_descreader *r = _r;
  size_t i;

  for (i = 0; i < upb_descreader_filecount(r); i++) {
    upb_filedef_unref(upb_descreader_file(r, i), &r->files);
  }

  upb_gfree(r->name);
  upb_inttable_uninit(&r->files);
  upb_strtable_uninit(&r->files_by_name);
  upb_inttable_uninit(&r->oneofs);
  upb_gfree(r->default_string);
  while (r->stack_len > 0) {
    upb_descreader_frame *f = &r->stack[--r->stack_len];
    upb_gfree(f->name);
  }
}


/* Public API  ****************************************************************/

upb_descreader *upb_descreader_create(upb_env *e, const upb_handlers *h) {
  upb_descreader *r = upb_env_malloc(e, sizeof(upb_descreader));
  if (!r || !upb_env_addcleanup(e, descreader_cleanup, r)) {
    return NULL;
  }

  upb_inttable_init(&r->files, UPB_CTYPE_PTR);
  upb_strtable_init(&r->files_by_name, UPB_CTYPE_PTR);
  upb_inttable_init(&r->oneofs, UPB_CTYPE_PTR);
  upb_sink_reset(upb_descreader_input(r), h, r);
  r->stack_len = 0;
  r->name = NULL;
  r->default_string = NULL;

  return r;
}

size_t upb_descreader_filecount(const upb_descreader *r) {
  return upb_inttable_count(&r->files);
}

upb_filedef *upb_descreader_file(const upb_descreader *r, size_t i) {
  upb_value v;
  if (upb_inttable_lookup(&r->files, i, &v)) {
    return upb_value_getptr(v);
  } else {
    return NULL;
  }
}

upb_sink *upb_descreader_input(upb_descreader *r) {
  return &r->sink;
}

const upb_handlers *upb_descreader_newhandlers(const void *owner) {
  const upb_msgdef *m = upbdefs_google_protobuf_FileDescriptorSet_get(&m);
  const upb_handlers *h = upb_handlers_newfrozen(m, owner, reghandlers, NULL);
  upb_msgdef_unref(m, &m);
  return h;
}
