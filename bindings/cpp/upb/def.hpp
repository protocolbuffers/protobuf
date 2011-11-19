/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * The set of upb::*Def classes and upb::SymbolTable allow for defining and
 * manipulating schema information (as defined in .proto files).
 *
 * Defs go through two distinct phases of life:
 *
 * 1. MUTABLE: when first created, the properties of the def can be set freely
 *    (for example a message's name, its list of fields, the name/number of
 *    fields, etc).  During this phase the def is *not* thread-safe, and may
 *    not be used for any purpose except to set its properties (it can't be
 *    used to parse anything, create any messages in memory, etc).
 *
 * 2. FINALIZED: after being added to a symtab (which links the defs together)
 *    the defs become finalized (thread-safe and immutable).  Programs may only
 *    access defs through a CONST POINTER during this stage -- upb_symtab will
 *    help you out with this requirement by only vending const pointers, but
 *    you need to make sure not to use any non-const pointers you still have
 *    sitting around.  In practice this means that you may not call any setters
 *    on the defs (or functions that themselves call the setters).  If you want
 *    to modify an existing immutable def, copy it with upb_*_dup(), modify the
 *    copy, and add the modified def to the symtab (replacing the existing
 *    def).
 *
 * You can test for which stage of life a def is in by calling
 * upb::Def::IsMutable().  This is particularly useful for dynamic language
 * bindings, which must properly guarantee that the dynamic language cannot
 * break the rules laid out above.
 *
 * It would be possible to make the defs thread-safe during stage 1 by using
 * mutexes internally and changing any methods returning pointers to return
 * copies instead.  This could be important if we are integrating with a VM or
 * interpreter that does not naturally serialize access to wrapped objects (for
 * example, in the case of Python this is not necessary because of the GIL).
 */

#ifndef UPB_DEF_HPP
#define UPB_DEF_HPP

#include <algorithm>
#include <string>
#include <vector>
#include "upb/def.h"
#include "upb/upb.hpp"

namespace upb {

class MessageDef;

class FieldDef : public upb_fielddef {
 public:
  static FieldDef* Cast(upb_fielddef *f) { return (FieldDef*)f; }
  static const FieldDef* Cast(const upb_fielddef *f) { return (FieldDef*)f; }

  static FieldDef* New() { return Cast(upb_fielddef_new()); }
  FieldDef* Dup() { return Cast(upb_fielddef_dup(this)); }

  // Read accessors -- may be called at any time.
  uint8_t type() const { return upb_fielddef_type(this); }
  uint8_t label() const { return upb_fielddef_label(this); }
  int32_t number() const { return upb_fielddef_number(this); }
  std::string name() const { return std::string(upb_fielddef_name(this)); }
  Value default_() const { return upb_fielddef_default(this); }
  Value bound_value() const { return upb_fielddef_fval(this); }

  MessageDef* message() { return (MessageDef*)upb_fielddef_msgdef(this); }
  const MessageDef* message() const { return (MessageDef*)upb_fielddef_msgdef(this); }

  // Will be added once upb::Def is defined:
  // Def* subdef() { return upb_fielddef_subdef(this); }
  // const Def* subdef() { return upb_fielddef_subdef(this); }

  // Returns true if this FieldDef is finalized
  bool IsFinalized() const { return upb_fielddef_finalized(this); }
  struct _upb_accessor_vtbl *accessor() const {
    return upb_fielddef_accessor(this);
  }
  std::string type_name() const {
    return std::string(upb_fielddef_typename(this));
  }

  // Write accessors -- may not be called once the FieldDef is finalized.

 private:
  FieldDef();
  ~FieldDef();
};

class MessageDef : public upb_msgdef {
 public:
  // Converting from C types to C++ wrapper types.
  static MessageDef* Cast(upb_msgdef *md) { return (MessageDef*)md; }
  static const MessageDef* Cast(const upb_msgdef *md) {
    return (MessageDef*)md;
  }

  static MessageDef* New() { return Cast(upb_msgdef_new()); }
  MessageDef* Dup() { return Cast(upb_msgdef_dup(this)); }

  void Ref() const { upb_msgdef_ref(this); }
  void Unref() const { upb_msgdef_unref(this); }

  // Read accessors -- may be called at any time.

  // The total size of in-memory messages created with this MessageDef.
  uint16_t instance_size() const { return upb_msgdef_size(this); }

  // The number of "hasbit" bytes in a message instance.
  uint8_t hasbit_bytes() const { return upb_msgdef_hasbit_bytes(this); }

  uint32_t extension_start() const { return upb_msgdef_extstart(this); }
  uint32_t extension_end() const { return upb_msgdef_extend(this); }

  // Write accessors.  May only be called before the msgdef is in a symtab.

  void set_instance_size(uint16_t size) { upb_msgdef_setsize(this, size); }
  void set_hasbit_bytes(uint16_t size) { upb_msgdef_setsize(this, size); }
  bool SetExtensionRange(uint32_t start, uint32_t end) {
    return upb_msgdef_setextrange(this, start, end);
  }

  // Adds a set of fields (upb_fielddef objects) to a msgdef.  Caller retains
  // its ref on the fielddef.  May only be done before the msgdef is in a
  // symtab (requires upb_def_ismutable(m) for the msgdef).  The fielddef's
  // name and number must be set, and the message may not already contain any
  // field with this name or number, and this fielddef may not be part of
  // another message, otherwise false is returned and no action is performed.
  bool AddFields(FieldDef*const * f, int n) {
    return upb_msgdef_addfields(this, (upb_fielddef**)f, n);
  }
  bool AddFields(const std::vector<FieldDef*>& fields) {
    return AddFields(&fields[0], fields.size());
  }

  // Lookup fields by name or number, returning NULL if no such field exists.
  FieldDef* FindFieldByName(const char *name) {
    return FieldDef::Cast(upb_msgdef_ntof(this, name));
  }
  FieldDef* FindFieldByName(const std::string& name) {
    return FieldDef::Cast(upb_msgdef_ntof(this, name.c_str()));
  }
  FieldDef* FindFieldByNumber(uint32_t num) {
    return FieldDef::Cast(upb_msgdef_itof(this, num));
  }

  const FieldDef* FindFieldByName(const char *name) const {
    return FindFieldByName(name);
  }
  const FieldDef* FindFieldByName(const std::string& name) const {
    return FindFieldByName(name);
  }
  const FieldDef* FindFieldByNumber(uint32_t num) const {
    return FindFieldByNumber(num);
  }

  // TODO: iteration over fields.

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
