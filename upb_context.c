/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>
#include "descriptor.h"
#include "upb_context.h"
#include "upb_enum.h"
#include "upb_msg.h"

static int memrchr(char *data, char c, size_t len)
{
  int off = len-1;
  while(off > 0 && data[off] != c) --off;
  return off;
}

bool upb_context_init(struct upb_context *c)
{
  upb_strtable_init(&c->symtab, 16, sizeof(struct upb_symtab_entry));
  /* Add all the types in descriptor.proto so we can parse descriptors. */
  if(!upb_context_addfd(c, &google_protobuf_filedescriptor)) {
    assert(false);
    return false;  /* Indicates that upb is buggy or corrupt. */
  }
  c->fd_size = 16;
  c->fd_len = 0;
  c->fd = malloc(sizeof(*c->fd));
  return true;
}

void upb_context_free(struct upb_context *c)
{
  struct upb_symtab_entry *e = upb_strtable_begin(&c->symtab);
  for(; e; e = upb_strtable_next(&c->symtab, &e->e)) {
    switch(e->type) {
      case UPB_SYM_MESSAGE: upb_msg_free(e->ref.msg); break;
      case UPB_SYM_ENUM: upb_enum_free(e->ref._enum); break;
      default: break;  /* TODO */
    }
    free(e->ref.msg);  /* The pointer is the same for all. */
    free(e->e.key.ptr);
  }
  upb_strtable_free(&c->symtab);
  for(size_t i = 0; i < c->fd_len; i++) free(c->fd[i]);
  free(c->fd);
}

struct upb_symtab_entry *upb_context_lookup(struct upb_context *c,
                                            struct upb_string *symbol)
{
  return upb_strtable_lookup(&c->symtab, symbol);
}

static struct upb_symtab_entry *resolve(struct upb_strtable *t,
                                        struct upb_string *base,
                                        struct upb_string *symbol)
{
  if(base->byte_len + symbol->byte_len + 1 >= UPB_SYM_MAX_LENGTH ||
     symbol->byte_len == 0) return NULL;

  if(symbol->ptr[0] == UPB_CONTEXT_SEPARATOR) {
    /* Symbols starting with '.' are absolute, so we do a single lookup. */
    struct upb_string sym_str = {.ptr = symbol->ptr+1,
                                 .byte_len = symbol->byte_len-1};
    return upb_strtable_lookup(t, &sym_str);
  } else {
    /* Remove components from base until we find an entry or run out. */
    char sym[UPB_SYM_MAX_LENGTH+1];
    struct upb_string sym_str = {.ptr = sym};
    int baselen = base->byte_len;
    while(1) {
      /* sym_str = base[0...base_len] + UPB_CONTEXT_SEPARATOR + symbol */
      memcpy(sym, base->ptr, baselen);
      sym[baselen] = UPB_CONTEXT_SEPARATOR;
      memcpy(sym + baselen + 1, symbol->ptr, symbol->byte_len);
      sym_str.byte_len = baselen + symbol->byte_len + 1;

      struct upb_symtab_entry *e = upb_strtable_lookup(t, &sym_str);
      if (e) return e;
      else if(baselen == 0) return NULL;  /* No more scopes to try. */

      baselen = memrchr(base->ptr, UPB_CONTEXT_SEPARATOR, baselen);
    }
  }
}

union upb_symbol_ref resolve2(struct upb_strtable *t1, struct upb_strtable *t2,
                              struct upb_string *base, struct upb_string *sym,
                                 enum upb_symbol_type expected_type) {
  union upb_symbol_ref nullref = {.msg = NULL};
  struct upb_symtab_entry *e = resolve(t1, base, sym);
  if(e == NULL) e = resolve(t2, base, sym);

  if(e && e->type == expected_type) return e->ref;
  else return nullref;
}


struct upb_symtab_entry *upb_context_resolve(struct upb_context *c,
                                             struct upb_string *base,
                                             struct upb_string *symbol) {
  return resolve(&c->symtab, base, symbol);
}

/* join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
 * join("", "Baz") -> "Baz"
 * Caller owns the returned string and must free it. */
static struct upb_string join(struct upb_string *base, struct upb_string *name) {
  size_t len = base->byte_len + name->byte_len;
  if(base->byte_len > 0) len++;  /* For the separator. */
  struct upb_string joined = {.byte_len=len, .ptr=malloc(len)};
  if(base->byte_len > 0) {
    /* nested_base = base + '.' +  d->name */
    memcpy(joined.ptr, base->ptr, base->byte_len);
    joined.ptr[base->byte_len] = UPB_CONTEXT_SEPARATOR;
    memcpy(&joined.ptr[base->byte_len+1], name->ptr, name->byte_len);
  } else {
    memcpy(joined.ptr, name->ptr, name->byte_len);
  }
  return joined;
}

static bool insert_enum(struct upb_strtable *t,
                        google_protobuf_EnumDescriptorProto *ed,
                        struct upb_string *base)
{
  // TODO: re-enable when compiler sets this flag
  //if(!ed->set_flags.has.name) return false;

  /* We own this and must free it on destruct. */
  struct upb_string fqname = join(base, ed->name);

  /* Redefinition within a FileDescriptorProto is not allowed. */
  if(upb_strtable_lookup(t, &fqname)) {
    free(fqname.ptr);
    return false;
  }

  struct upb_symtab_entry e;
  e.e.key = fqname;
  e.type = UPB_SYM_ENUM;
  e.ref._enum = malloc(sizeof(*e.ref._enum));
  upb_enum_init(e.ref._enum, ed);
  upb_strtable_insert(t, &e.e);

  return true;
}

static bool insert_message(struct upb_strtable *t,
                           google_protobuf_DescriptorProto *d,
                           struct upb_string *base)
{
  /* TODO: re-enable when compiler sets this flag. */
  //if(!d->set_flags.has.name) return false;

  /* We own this and must free it on destruct. */
  struct upb_string fqname = join(base, d->name);

  /* Redefinition within a FileDescriptorProto is not allowed. */
  if(upb_strtable_lookup(t, d->name)) {
    free(fqname.ptr);
    return false;
  }

  struct upb_symtab_entry e;
  e.e.key = fqname;
  e.type = UPB_SYM_MESSAGE;
  e.ref.msg = malloc(sizeof(*e.ref.msg));
  if(!upb_msg_init(e.ref.msg, d)) {
    free(fqname.ptr);
    return false;
  }
  upb_strtable_insert(t, &e.e);

  /* Add nested messages and enums. */
  //if(d->set_flags.has.nested_type)
  if(d->nested_type)
    for(unsigned int i = 0; i < d->nested_type->len; i++)
      if(!insert_message(t, d->nested_type->elements[i], &fqname))
        return false;

  //if(d->set_flags.has.enum_type)
  if(d->enum_type)
    for(unsigned int i = 0; i < d->enum_type->len; i++)
      if(!insert_enum(t, d->enum_type->elements[i], &fqname))
        return false;

  return true;
}

bool upb_context_addfd(struct upb_context *c,
                       google_protobuf_FileDescriptorProto *fd)
{
  struct upb_string package = {.byte_len=0};
  if(fd->set_flags.has.package) package = *fd->package;

  /* We want the entire add operation to be atomic, so we initially insert into
   * this temporary map of symbols.  Once we have verified that there are no
   * errors (all symbols can be resolved and no illegal redefinitions occurred)
   * only then do we insert into the context's table. */
  struct upb_strtable tmp;
  int symcount = (fd->set_flags.has.message_type ? fd->message_type->len : 0) +
                 (fd->set_flags.has.enum_type ? fd->enum_type->len : 0) +
                 (fd->set_flags.has.service ? fd->service->len : 0);
  upb_strtable_init(&tmp, symcount, sizeof(struct upb_symtab_entry));

  if(fd->set_flags.has.message_type)
    for(unsigned int i = 0; i < fd->message_type->len; i++)
      if(!insert_message(&tmp, fd->message_type->elements[i], &package))
        goto error;

  if(fd->set_flags.has.enum_type)
    for(unsigned int i = 0; i < fd->enum_type->len; i++)
      if(!insert_enum(&tmp, fd->enum_type->elements[i], &package))
        goto error;

  /* TODO: handle extensions and services. */

  /* Attempt to resolve all references. */
  struct upb_symtab_entry *e;
  for(e = upb_strtable_begin(&tmp); e; e = upb_strtable_next(&tmp, &e->e)) {
    if(upb_strtable_lookup(&c->symtab, &e->e.key))
      goto error;  /* Redefinition prohibited. */
    if(e->type == UPB_SYM_MESSAGE) {
      struct upb_msg *m = e->ref.msg;
      for(unsigned int i = 0; i < m->num_fields; i++) {
        struct upb_msg_field *f = &m->fields[i];
        google_protobuf_FieldDescriptorProto *fd = m->field_descriptors[i];
        union upb_symbol_ref ref;
        if(fd->type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE)
          ref = resolve2(&c->symtab, &tmp, &e->e.key, fd->type_name, UPB_SYM_MESSAGE);
        else if(fd->type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM)
          ref = resolve2(&c->symtab, &tmp, &e->e.key, fd->type_name, UPB_SYM_ENUM);
        else
          continue;  /* No resolving necessary. */
        if(!ref.msg) goto error;  /* Ref. to undefined symbol. */
        upb_msg_ref(m, f, ref);
      }
    }
  }

  /* All references were successfully resolved -- add to the symbol table. */
  for(e = upb_strtable_begin(&tmp); e; e = upb_strtable_next(&tmp, &e->e))
    upb_strtable_insert(&c->symtab, &e->e);

  upb_strtable_free(&tmp);
  return true;

error:
  /* TODO */
  return false;
}

bool upb_context_parsefd(struct upb_context *c, struct upb_string *fd_str) {
  google_protobuf_FileDescriptorProto *fd =
      upb_alloc_and_parse(c->fd_msg, fd_str, true);
  if(!fd) return false;
  if(!upb_context_addfd(c, fd)) return false;
  if(c->fd_size == c->fd_len) {
    c->fd_size *= 2;
    c->fd = realloc(c->fd, c->fd_size);
  }
  c->fd[c->fd_len++] = fd;  /* Need to keep a ref since we own it. */
  return true;
}
