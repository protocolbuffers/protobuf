/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>
#include "descriptor.h"
#include "upb_context.h"
#include "upb_def.h"
#include "upb_mm.h"

struct upb_symtab_entry {
  struct upb_strtable_entry e;
  struct upb_def *def;
};

/* Search for a character in a string, in reverse. */
static int my_memrchr(char *data, char c, size_t len)
{
  int off = len-1;
  while(off > 0 && data[off] != c) --off;
  return off;
}

void addfd(struct upb_strtable *addto, struct upb_strtable *existingdefs,
           google_protobuf_FileDescriptorProto *fd, bool sort,
           struct upb_status *status);

struct upb_context *upb_context_new()
{
  struct upb_context *c = malloc(sizeof(*c));
  upb_atomic_refcount_init(&c->refcount, 1);
  upb_rwlock_init(&c->lock);
  upb_strtable_init(&c->symtab, 16, sizeof(struct upb_symtab_entry));
  upb_strtable_init(&c->psymtab, 16, sizeof(struct upb_symtab_entry));
  /* Add all the types in descriptor.proto so we can parse descriptors. */
  google_protobuf_FileDescriptorProto *fd =
      upb_file_descriptor_set->file->elements[0]; /* We know there is only 1. */
  struct upb_status status = UPB_STATUS_INIT;
  addfd(&c->psymtab, &c->symtab, fd, false, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Failed to initialize upb: %s.\n", status.msg);
    assert(false);
    return NULL;  /* Indicates that upb is buggy or corrupt. */
  }
  struct upb_string name = UPB_STRLIT("google.protobuf.FileDescriptorSet");
  struct upb_symtab_entry *e = upb_strtable_lookup(&c->psymtab, &name);
  assert(e);
  c->fds_msgdef = upb_downcast_msgdef(e->def);
  return c;
}

static void free_symtab(struct upb_strtable *t)
{
  struct upb_symtab_entry *e = upb_strtable_begin(t);
  for(; e; e = upb_strtable_next(t, &e->e)) {
    upb_def_unref(e->def);
    upb_string_unref(e->e.key);
  }
  upb_strtable_free(t);
}

static void free_context(struct upb_context *c)
{
  free_symtab(&c->symtab);
  free_symtab(&c->psymtab);
}

void upb_context_unref(struct upb_context *c)
{
  if(upb_atomic_unref(&c->refcount)) {
    upb_rwlock_wrlock(&c->lock);
    free_context(c);
    upb_rwlock_unlock(&c->lock);
    upb_rwlock_destroy(&c->lock);
    free(c);
  }
}

struct upb_def **upb_context_getandref_defs(struct upb_context *c, int *count)
{
  upb_rwlock_wrlock(&c->lock);
  *count = upb_strtable_count(&c->symtab);
  struct upb_def **defs = malloc(sizeof(*defs) * (*count));
  struct upb_symtab_entry *e = upb_strtable_begin(&c->symtab);
  int i = 0;
  for(; e; e = upb_strtable_next(&c->symtab, &e->e), i++) {
    assert(e->def);
    defs[i] = e->def;
    upb_def_ref(defs[i]);
  }
  assert(*count == i);
  upb_rwlock_unlock(&c->lock);
  return defs;
}

struct upb_def *upb_context_lookup(struct upb_context *c,
                                   struct upb_string *sym)
{
  upb_rwlock_rdlock(&c->lock);
  struct upb_symtab_entry *e = upb_strtable_lookup(&c->symtab, sym);
  upb_rwlock_unlock(&c->lock);
  return e ? e->def : NULL;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static struct upb_symtab_entry *resolve(struct upb_strtable *t,
                                        struct upb_string *base,
                                        struct upb_string *symbol)
{
  if(base->byte_len + symbol->byte_len + 1 >= UPB_SYMBOL_MAXLEN ||
     symbol->byte_len == 0) return NULL;

  if(symbol->ptr[0] == UPB_SYMBOL_SEPARATOR) {
    /* Symbols starting with '.' are absolute, so we do a single lookup. */
    struct upb_string sym_str = {.ptr = symbol->ptr+1,
                                 .byte_len = symbol->byte_len-1};
    return upb_strtable_lookup(t, &sym_str);
  } else {
    /* Remove components from base until we find an entry or run out. */
    char sym[UPB_SYMBOL_MAXLEN+1];
    struct upb_string sym_str = {.ptr = sym};
    int baselen = base->byte_len;
    while(1) {
      /* sym_str = base[0...base_len] + UPB_SYMBOL_SEPARATOR + symbol */
      memcpy(sym, base->ptr, baselen);
      sym[baselen] = UPB_SYMBOL_SEPARATOR;
      memcpy(sym + baselen + 1, symbol->ptr, symbol->byte_len);
      sym_str.byte_len = baselen + symbol->byte_len + 1;

      struct upb_symtab_entry *e = upb_strtable_lookup(t, &sym_str);
      if (e) return e;
      else if(baselen == 0) return NULL;  /* No more scopes to try. */

      baselen = my_memrchr(base->ptr, UPB_SYMBOL_SEPARATOR, baselen);
    }
  }
}

/* Tries to resolve a symbol in two different tables. */
struct upb_def *resolve2(struct upb_strtable *t1, struct upb_strtable *t2,
                         struct upb_string *base, struct upb_string *sym,
                         enum upb_def_type expected_type) {
  struct upb_symtab_entry *e = resolve(t1, base, sym);
  if(e == NULL) e = resolve(t2, base, sym);
  if(e && e->def->type == expected_type) return e->def;
  return NULL;
}


struct upb_def *upb_context_resolve(struct upb_context *c,
                                    struct upb_string *base,
                                    struct upb_string *symbol) {
  upb_rwlock_rdlock(&c->lock);
  struct upb_symtab_entry *e = resolve(&c->symtab, base, symbol);
  upb_rwlock_unlock(&c->lock);
  return e ? e->def : NULL;
}

/* Joins strings together, for example:
 *   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
 *   join("", "Baz") -> "Baz"
 * Caller owns the returned string and must free it. */
static struct upb_string *join(struct upb_string *base, struct upb_string *name) {
  size_t len = base->byte_len + name->byte_len;
  if(base->byte_len > 0) len++;  /* For the separator. */
  struct upb_string *joined = upb_string_new();
  upb_string_resize(joined, len);
  if(base->byte_len > 0) {
    /* nested_base = base + '.' +  d->name */
    memcpy(joined->ptr, base->ptr, base->byte_len);
    joined->ptr[base->byte_len] = UPB_SYMBOL_SEPARATOR;
    memcpy(&joined->ptr[base->byte_len+1], name->ptr, name->byte_len);
  } else {
    memcpy(joined->ptr, name->ptr, name->byte_len);
  }
  return joined;
}

static void insert_enum(struct upb_strtable *t,
                        google_protobuf_EnumDescriptorProto *ed,
                        struct upb_string *base,
                        struct upb_status *status)
{
  if(!ed->set_flags.has.name) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "enum in context '" UPB_STRFMT "' does not have a name",
               UPB_STRARG(base));
    return;
  }

  struct upb_string *fqname = join(base, ed->name);
  if(upb_strtable_lookup(t, fqname)) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "attempted to redefine symbol '" UPB_STRFMT "'",
               UPB_STRARG(fqname));
    upb_string_unref(fqname);
    return;
  }

  struct upb_symtab_entry e;
  e.e.key = fqname;  // Donating our ref to the table.
  e.def = (struct upb_def*)upb_enumdef_new(ed, fqname);
  upb_strtable_insert(t, &e.e);
}

static void insert_message(struct upb_strtable *t,
                           google_protobuf_DescriptorProto *d,
                           struct upb_string *base, bool sort,
                           struct upb_status *status)
{
  if(!d->set_flags.has.name) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "message in context '" UPB_STRFMT "' does not have a name",
               UPB_STRARG(base));
    return;
  }

  /* We own this and must free it on destruct. */
  struct upb_string *fqname = join(base, d->name);

  if(upb_strtable_lookup(t, fqname)) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "attempted to redefine symbol '" UPB_STRFMT "'",
               UPB_STRARG(fqname));
    upb_string_unref(fqname);
    return;
  }

  struct upb_symtab_entry e;
  e.e.key = fqname;  // Donating our ref to the table.
  struct upb_fielddef *fielddefs = malloc(sizeof(*fielddefs) * d->field->len);
  for (unsigned int i = 0; i < d->field->len; i++) {
    google_protobuf_FieldDescriptorProto *fd = d->field->elements[i];
    upb_fielddef_init(&fielddefs[i], fd);
  }
  if(sort) upb_fielddef_sort(fielddefs, d->field->len);
  e.def = (struct upb_def*)upb_msgdef_new(fielddefs, d->field->len, fqname);
  upb_strtable_insert(t, &e.e);

  /* Add nested messages and enums. */
  if(d->set_flags.has.nested_type)
    for(unsigned int i = 0; i < d->nested_type->len; i++)
      insert_message(t, d->nested_type->elements[i], fqname, sort, status);

  if(d->set_flags.has.enum_type)
    for(unsigned int i = 0; i < d->enum_type->len; i++)
      insert_enum(t, d->enum_type->elements[i], fqname, status);
}

void addfd(struct upb_strtable *addto, struct upb_strtable *existingdefs,
           google_protobuf_FileDescriptorProto *fd, bool sort,
           struct upb_status *status)
{
  struct upb_string pkg = {.byte_len=0};
  if(fd->set_flags.has.package) pkg = *fd->package;

  if(fd->set_flags.has.message_type)
    for(unsigned int i = 0; i < fd->message_type->len; i++)
      insert_message(addto, fd->message_type->elements[i], &pkg, sort, status);

  if(fd->set_flags.has.enum_type)
    for(unsigned int i = 0; i < fd->enum_type->len; i++)
      insert_enum(addto, fd->enum_type->elements[i], &pkg, status);

  if(!upb_ok(status)) return;

  /* TODO: handle extensions and services. */

  /* Attempt to resolve all references. */
  struct upb_symtab_entry *e;
  for(e = upb_strtable_begin(addto); e; e = upb_strtable_next(addto, &e->e)) {
    if(upb_strtable_lookup(existingdefs, e->e.key)) {
      upb_seterr(status, UPB_STATUS_ERROR,
                 "attempted to redefine symbol '" UPB_STRFMT "'",
                 UPB_STRARG(e->e.key));
      return;
    }
    if(e->def->type == UPB_DEF_MESSAGE) {
      struct upb_msgdef *m = upb_downcast_msgdef(e->def);
      for(unsigned int i = 0; i < m->num_fields; i++) {
        struct upb_fielddef *f = &m->fields[i];
        if(!upb_issubmsg(f) && f->type != UPB_TYPENUM(ENUM)) {
          // No resolving necessary.
          continue;
        }
        struct upb_def *def;
        struct upb_string *name = upb_downcast_unresolveddef(f->def)->name;
        if(upb_issubmsg(f))
          def = resolve2(existingdefs, addto, e->e.key, name, UPB_DEF_MESSAGE);
        else if(f->type == UPB_TYPENUM(ENUM))
          def = resolve2(existingdefs, addto, e->e.key, name, UPB_DEF_ENUM);
        if(!def) {
          upb_seterr(status, UPB_STATUS_ERROR,
                     "could not resolve symbol '" UPB_STRFMT "'"
                     " in context '" UPB_STRFMT "'",
                     UPB_STRARG(name), UPB_STRARG(e->e.key));
          return;
        }
        upb_msgdef_resolve(m, f, def);
      }
    }
  }
}

void upb_context_addfds(struct upb_context *c,
                        google_protobuf_FileDescriptorSet *fds,
                        struct upb_status *status)
{
  if(fds->set_flags.has.file) {
    /* Insert new symbols into a temporary table until we have verified that
     * the descriptor is valid. */
    struct upb_strtable tmp;
    upb_strtable_init(&tmp, 0, sizeof(struct upb_symtab_entry));
    upb_rwlock_rdlock(&c->lock);
    for(uint32_t i = 0; i < fds->file->len; i++) {
      addfd(&tmp, &c->symtab, fds->file->elements[i], true, status);
      if(!upb_ok(status)) {
        free_symtab(&tmp);
        upb_rwlock_unlock(&c->lock);
        return;
      }
    }
    upb_rwlock_unlock(&c->lock);

    /* Everything was successfully added, copy from the tmp symtable. */
    struct upb_symtab_entry *e;
    {
      upb_rwlock_wrlock(&c->lock);
      for(e = upb_strtable_begin(&tmp); e; e = upb_strtable_next(&tmp, &e->e))
        upb_strtable_insert(&c->symtab, &e->e);
      upb_rwlock_unlock(&c->lock);
    }
    upb_strtable_free(&tmp);
  }
  return;
}

void upb_context_parsefds(struct upb_context *c, struct upb_string *fds_str,
                          struct upb_status *status)
{
  struct upb_msg *fds = upb_msg_new(c->fds_msgdef);
  upb_msg_parsestr(fds, fds_str->ptr, fds_str->byte_len, status);
  if(!upb_ok(status)) return;
  upb_context_addfds(c, (google_protobuf_FileDescriptorSet*)fds, status);
  return;
}
