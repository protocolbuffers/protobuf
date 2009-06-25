/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "descriptor.h"
#include "upb_context.h"

int memrchr(char *data, char c, size_t len)
{
  int off = len-1;
  while(off > 0 && data[off] != c) --off;
  return off;
}

void upb_context_init(struct upb_context *c)
{
  upb_strtable_init(&c->symtab, 16, sizeof(struct upb_symtab_entry));
  /* Add all the types in descriptor.proto so we can parse descriptors. */
  if(!upb_context_addfd(c, &google_protobuf_filedescriptor, UPB_ONREDEF_ERROR))
    assert(false);
}

void upb_context_free(struct upb_context *c)
{
  upb_strtable_free(&c->symtab);
}

struct upb_symtab_entry *upb_context_lookup(struct upb_context *c,
                                            struct upb_string *symbol)
{
  return upb_strtable_lookup(&c->symtab, symbol);
}

struct upb_symtab_entry *upb_context_resolve(struct upb_context *c,
                                             struct upb_string *base,
                                             struct upb_string *symbol)
{
  if(base->byte_len + symbol->byte_len + 1 >= UPB_SYM_MAX_LENGTH ||
     symbol->byte_len == 0) return NULL;

  if(symbol->data[0] == UPB_CONTEXT_SEPARATOR) {
    /* Symbols starting with '.' are absolute, so we do a single lookup. */
    struct upb_string sym_str = {.data = symbol->data+1,
                                 .byte_len = symbol->byte_len-1};
    return upb_context_lookup(c, &sym_str);
  } else {
    /* Remove components from base until we find an entry or run out. */
    char sym[UPB_SYM_MAX_LENGTH+1];
    struct upb_string sym_str = {.data = sym};
    int baselen = base->byte_len;
    while(1) {
      /* sym_str = base[0...base_len] + UPB_CONTEXT_SEPARATOR + symbol */
      memcpy(sym, base->data, baselen);
      sym[baselen] = UPB_CONTEXT_SEPARATOR;
      memcpy(sym + baselen + 1, symbol->data, symbol->byte_len);
      sym_str.byte_len = baselen + symbol->byte_len + 1;

      struct upb_symtab_entry *e = upb_context_lookup(c, &sym_str);
      if (e) return e;
      else if(baselen == 0) return NULL;  /* No more scopes to try. */

      baselen = memrchr(base->data, UPB_CONTEXT_SEPARATOR, baselen);
    }
  }
}

bool upb_context_addfd(struct upb_context *c,
                       google_protobuf_FileDescriptorProto *fd,
                       int onredef)
{
  /* TODO: properly handle redefinitions and unresolvable symbols. */
  if(fd->set_flags.has.message_type) {
    for(unsigned int i = 0; i < fd->message_type->len; i++) {
      struct google_protobuf_DescriptorProto *d = fd->message_type->elements[i];
      if(!d->set_flags.has.name) return false;
      struct upb_symtab_entry e;
      e.e.key = *d->name;
      e.type = UPB_SYM_MESSAGE;
      e.p.msg = malloc(sizeof(*e.p.msg));
      upb_msg_init(e.p.msg, d);
      upb_strtable_insert(&c->symtab, &e.e);
    }
  }
  /* TODO: handle enums, extensions, and services. */
  return true;
}
