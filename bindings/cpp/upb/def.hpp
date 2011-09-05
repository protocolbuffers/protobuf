/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * The set of upb::*Def classes and upb::SymbolTable allow for defining and
 * manipulating schema information (as defined in .proto files).
 */

#ifndef UPB_DEF_HPP
#define UPB_DEF_HPP

#include "upb/def.h"

namespace upb {

class MessageDef : public upb_msgdef {
 public:
  // Converting from C types to C++ wrapper types.
  static MessageDef* Cast(upb_msgdef *md) { return (MessageDef*)md; }
  static const MessageDef* Cast(const upb_msgdef *md) {
    return (const MessageDef*)md;
  }

  void Ref() const { upb_msgdef_ref(this); }
  void Unref() const { upb_msgdef_unref(this); }

 private:
  MessageDef();
  ~MessageDef();
};

class SymbolTable : public upb_symtab {
 public:
  // Converting from C types to C++ wrapper types.
  static SymbolTable* Cast(upb_symtab *s) { return (SymbolTable*)s; }
  static const SymbolTable* Cast(const upb_symtab *s) {
    return (SymbolTable*)s;
  }

  static SymbolTable* New() { return Cast(upb_symtab_new()); }

  void Ref() const { upb_symtab_unref(this); }
  void Unref() const { upb_symtab_unref(this); }

  // If the given name refers to a message in this symbol table, returns a new
  // ref to that MessageDef object, otherwise returns NULL.
  const MessageDef* LookupMessage(const char *name) const {
    return MessageDef::Cast(upb_symtab_lookupmsg(this, name));
  }

 private:
  SymbolTable();
  ~SymbolTable();
};

}  // namespace upb

#endif
