//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// The set of upb::*Def classes and upb::SymbolTable allow for defining and
// manipulating schema information (as defined in .proto files).
//
// Defs go through two distinct phases of life:
//
// 1. MUTABLE: when first created, the properties of the def can be set freely
//    (for example a message's name, its list of fields, the name/number of
//    fields, etc).  During this phase the def is *not* thread-safe, and may
//    not be used for any purpose except to set its properties (it can't be
//    used to parse anything, create any messages in memory, etc).
//
// 2. FINALIZED: the Def::Finzlie() operation finalizes a set of defs,
//    which makes them thread-safe and immutable.  Finalized defs may only be
//    accessed through a CONST POINTER.  If you want to modify an existing
//    immutable def, copy it with Dup() and modify and finalize the copy.
//
// The refcounting of defs works properly no matter what state the def is in.
// Once the def is finalized it is guaranteed that any def reachable from a
// live def is also live (so a ref on the base of a message tree keeps the
// whole tree alive).
//
// You can test for which stage of life a def is in by calling IsMutable().
// This is particularly useful for dynamic language bindings, which must
// properly guarantee that the dynamic language cannot break the rules laid out
// above.
//
// It would be possible to make the defs thread-safe during stage 1 by using
// mutexes internally and changing any methods returning pointers to return
// copies instead.  This could be important if we are integrating with a VM or
// interpreter that does not naturally serialize access to wrapped objects (for
// example, in the case of Python this is not necessary because of the GIL).

#ifndef UPB_DEF_HPP
#define UPB_DEF_HPP

#include <algorithm>
#include <string>
#include <vector>
#include "upb/def.h"
#include "upb/upb.hpp"

namespace upb {

class Def;
class MessageDef;

typedef upb_fieldtype_t FieldType;
typedef upb_label_t Label;

class FieldDef : public upb_fielddef {
 public:
  static FieldDef* Cast(upb_fielddef *f) { return static_cast<FieldDef*>(f); }
  static const FieldDef* Cast(const upb_fielddef *f) {
    return static_cast<const FieldDef*>(f);
  }

  static FieldDef* New(const void *owner) {
    return Cast(upb_fielddef_new(owner));
  }
  FieldDef* Dup(const void *owner) const {
    return Cast(upb_fielddef_dup(this, owner));
  }
  void Ref(const void *owner) { upb_fielddef_ref(this, owner); }
  void Unref(const void *owner) { upb_fielddef_unref(this, owner); }

  bool IsMutable() const { return upb_fielddef_ismutable(this); }
  bool IsFinalized() const { return upb_fielddef_isfinalized(this); }
  bool IsString() const { return upb_isstring(this); }
  bool IsSequence() const { return upb_isseq(this); }
  bool IsSubmessage() const { return upb_issubmsg(this); }

  // Simple accessors. /////////////////////////////////////////////////////////

  FieldType type() const { return upb_fielddef_type(this); }
  Label label() const { return upb_fielddef_label(this); }
  int32_t number() const { return upb_fielddef_number(this); }
  std::string name() const { return std::string(upb_fielddef_name(this)); }
  Value default_() const { return upb_fielddef_default(this); }
  Value bound_value() const { return upb_fielddef_fval(this); }
  uint16_t offset() const { return upb_fielddef_offset(this); }
  int16_t hasbit() const { return upb_fielddef_hasbit(this); }

  bool set_type(FieldType type) { return upb_fielddef_settype(this, type); }
  bool set_label(Label label) { return upb_fielddef_setlabel(this, label); }
  void set_offset(uint16_t offset) { upb_fielddef_setoffset(this, offset); }
  void set_hasbit(int16_t hasbit) { upb_fielddef_sethasbit(this, hasbit); }
  void set_fval(Value fval) { upb_fielddef_setfval(this, fval); }
  void set_accessor(struct _upb_accessor_vtbl* vtbl) {
    upb_fielddef_setaccessor(this, vtbl);
  }
  MessageDef* message();
  const MessageDef* message() const;

  struct _upb_accessor_vtbl *accessor() const {
    return upb_fielddef_accessor(this);
  }

  // "Number" and "name" must be set before the fielddef is added to a msgdef.
  // For the moment we do not allow these to be set once the fielddef is added
  // to a msgdef -- this could be relaxed in the future.
  bool set_number(int32_t number) {
    return upb_fielddef_setnumber(this, number);
  }
  bool set_name(const char *name) { return upb_fielddef_setname(this, name); }
  bool set_name(const std::string& name) { return set_name(name.c_str()); }

  // Default value. ////////////////////////////////////////////////////////////

  // Returns the default value for this fielddef, which may either be something
  // the client set explicitly or the "default default" (0 for numbers, empty
  // for strings).  The field's type indicates the type of the returned value,
  // except for enum fields that are still mutable.
  //
  // For enums the default can be set either numerically or symbolically -- the
  // upb_fielddef_default_is_symbolic() function below will indicate which it
  // is.  For string defaults, the value will be a upb_byteregion which is
  // invalidated by any other non-const call on this object.  Once the fielddef
  // is finalized, symbolic enum defaults are resolved, so finalized enum
  // fielddefs always have a default of type int32.
  Value defaultval() { return upb_fielddef_default(this); }

  // Sets default value for the field.  For numeric types, use
  // upb_fielddef_setdefault(), and "value" must match the type of the field.
  // For string/bytes types, use upb_fielddef_setdefaultstr().  Enum types may
  // use either, since the default may be set either numerically or
  // symbolically.
  //
  // NOTE: May only be called for fields whose type has already been set.
  // Also, will be reset to default if the field's type is set again.
  void set_default(Value value) { upb_fielddef_setdefault(this, value); }
  void set_default(const char *str) { upb_fielddef_setdefaultcstr(this, str); }
  void set_default(const char *str, size_t len) {
    upb_fielddef_setdefaultstr(this, str, len);
  }
  void set_default(const std::string& str) {
    upb_fielddef_setdefaultstr(this, str.c_str(), str.size());
  }

  // The results of this function are only meaningful for mutable enum fields,
  // which can have a default specified either as an integer or as a string.
  // If this returns true, the default returned from upb_fielddef_default() is
  // a string, otherwise it is an integer.
  bool DefaultIsSymbolic() { return upb_fielddef_default_is_symbolic(this); }

  // Subdef. ///////////////////////////////////////////////////////////////////

  // Submessage and enum fields must reference a "subdef", which is the
  // MessageDef or EnumDef that defines their type.  Note that when the
  // FieldDef is mutable it may not have a subdef *yet*, but this still returns
  // true to indicate that the field's type requires a subdef.
  bool HasSubDef() { return upb_hassubdef(this); }

  // Before a FieldDef is finalized, its subdef may be set either directly
  // (with a Def*) or symbolically.  Symbolic refs must be resolved by the
  // client before the containing msgdef can be finalized.
  //
  // Both methods require that HasSubDef() (so the type must be set prior to
  // calling these methods).  Returns false if this is not the case, or if the
  // given subdef is not of the correct type.  The subtype is reset if the
  // field's type is changed.
  bool set_subdef(Def* def);
  bool set_subtype_name(const char *name) {
    return upb_fielddef_setsubtypename(this, name);
  }
  bool set_subtype_name(const std::string& str) {
    return set_subtype_name(str.c_str());
  }

  // Returns the enum or submessage def or symbolic name for this field, if
  // any.  May only be called for fields where HasSubDef() is true.  Returns
  // NULL if the subdef has not been set or if you ask for a subtype name when
  // the subtype is currently set symbolically (or vice-versa).
  //
  // Caller does *not* own a ref on the returned def or string.
  // subtypename_name() is non-const because only mutable defs can have the
  // subtype name set symbolically (symbolic references must be resolved before
  // the MessageDef can be finalized).
  const Def* subdef() const;
  const char *subtype_name() { return upb_fielddef_subtypename(this); }

 private:
  UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(FieldDef);
};

class Def : public upb_def {
 public:
  // Converting from C types to C++ wrapper types.
  static Def* Cast(upb_def *def) { return static_cast<Def*>(def); }
  static const Def* Cast(const upb_def *def) {
    return static_cast<const Def*>(def);
  }

  void Ref(const void *owner) const { upb_def_ref(this, owner); }
  void Unref(const void *owner) const { upb_def_unref(this, owner); }

  void set_full_name(const char *name) { upb_def_setfullname(this, name); }
  void set_full_name(const std::string& name) {
    upb_def_setfullname(this, name.c_str());
  }

  const char *full_name() const { return upb_def_fullname(this); }

  // Finalizes the given list of defs (as well as the fielddefs for the given
  // msgdefs).  All defs reachable from any def in this list must either be
  // already finalized or elsewhere in the list.  Any symbolic references to
  // enums or submessages must already have been resolved.  Returns true on
  // success, otherwise false is returned and status contains details.  In the
  // error case the input defs are unmodified.  See the comment at the top of
  // this file for the semantics of finalized defs.
  //
  // n is currently limited to 64k defs, if more are required break them into
  // batches of 64k (or we could raise this limit, at the cost of a bigger
  // upb_def structure or complexity in upb_def_finalize()).
  static bool Finalize(Def*const* defs, int n, Status* status) {
    return upb_finalize(reinterpret_cast<upb_def*const*>(defs), n, status);
  }
  static bool Finalize(const std::vector<Def*>& defs, Status* status) {
    return Finalize(&defs[0], defs.size(), status);
  }
};

class MessageDef : public upb_msgdef {
 public:
  // Converting from C types to C++ wrapper types.
  static MessageDef* Cast(upb_msgdef *md) {
    return static_cast<MessageDef*>(md);
  }
  static const MessageDef* Cast(const upb_msgdef *md) {
    return static_cast<const MessageDef*>(md);
  }
  static MessageDef* DynamicCast(Def* def) {
    return Cast(upb_dyncast_msgdef(def));
  }
  static const MessageDef* DynamicCast(const Def* def) {
    return Cast(upb_dyncast_msgdef_const(def));
  }

  Def* AsDef() { return Def::Cast(UPB_UPCAST(this)); }
  const Def* AsDef() const { return Def::Cast(UPB_UPCAST(this)); }

  static MessageDef* New(void *owner) { return Cast(upb_msgdef_new(owner)); }
  MessageDef* Dup(void *owner) const {
    return Cast(upb_msgdef_dup(this, owner));
  }

  void Ref(const void *owner) const { upb_msgdef_ref(this, owner); }
  void Unref(const void *owner) const { upb_msgdef_unref(this, owner); }

  // Read accessors -- may be called at any time.

  const char *full_name() const { return AsDef()->full_name(); }

  // The total size of in-memory messages created with this MessageDef.
  uint16_t instance_size() const { return upb_msgdef_size(this); }

  // The number of "hasbit" bytes in a message instance.
  uint8_t hasbit_bytes() const { return upb_msgdef_hasbit_bytes(this); }

  uint32_t extension_start() const { return upb_msgdef_extstart(this); }
  uint32_t extension_end() const { return upb_msgdef_extend(this); }

  // Write accessors.  May only be called before the msgdef is in a symtab.

  void set_full_name(const char *name) { AsDef()->set_full_name(name); }
  void set_full_name(const std::string& name) { AsDef()->set_full_name(name); }

  void set_instance_size(uint16_t size) { upb_msgdef_setsize(this, size); }
  void set_hasbit_bytes(uint16_t size) { upb_msgdef_setsize(this, size); }
  bool SetExtensionRange(uint32_t start, uint32_t end) {
    return upb_msgdef_setextrange(this, start, end);
  }

  // Adds a set of fields (FieldDef objects) to a MessageDef.  Caller passes a
  // ref on the FieldDef to the MessageDef in both success and failure cases.
  // May only be done before the MessageDef is in a SymbolTable (requires
  // m->IsMutable() for the MessageDef).  The FieldDef's name and number must
  // be set, and the message may not already contain any field with this name
  // or number, and this FieldDef may not be part of another message, otherwise
  // false is returned and the MessageDef is unchanged.
  bool AddField(FieldDef* f, const void *owner) {
    return AddFields(&f, 1, owner);
  }
  bool AddFields(FieldDef*const * f, int n, const void *owner) {
    return upb_msgdef_addfields(this, (upb_fielddef*const*)f, n, owner);
  }
  bool AddFields(const std::vector<FieldDef*>& fields, const void *owner) {
    return AddFields(&fields[0], fields.size(), owner);
  }

  int field_count() const { return upb_msgdef_numfields(this); }

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

  class Iterator : public upb_msg_iter {
   public:
    explicit Iterator(MessageDef* md) { upb_msg_begin(this, md); }
    Iterator() {}

    FieldDef* field() { return FieldDef::Cast(upb_msg_iter_field(this)); }
    bool Done() { return upb_msg_done(this); }
    void Next() { return upb_msg_next(this); }
  };

  class ConstIterator : public upb_msg_iter {
   public:
    explicit ConstIterator(const MessageDef* md) { upb_msg_begin(this, md); }
    ConstIterator() {}

    const FieldDef* field() { return FieldDef::Cast(upb_msg_iter_field(this)); }
    bool Done() { return upb_msg_done(this); }
    void Next() { return upb_msg_next(this); }
  };

 private:
  UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(MessageDef);
};

class EnumDef : public upb_enumdef {
 public:
  // Converting from C types to C++ wrapper types.
  static EnumDef* Cast(upb_enumdef *e) { return static_cast<EnumDef*>(e); }
  static const EnumDef* Cast(const upb_enumdef *e) {
    return static_cast<const EnumDef*>(e);
  }

  static EnumDef* New(const void *owner) { return Cast(upb_enumdef_new(owner)); }

  void Ref(const void *owner) { upb_enumdef_ref(this, owner); }
  void Unref(const void *owner) { upb_enumdef_unref(this, owner); }
  EnumDef* Dup(const void *owner) const {
    return Cast(upb_enumdef_dup(this, owner));
  }

  Def* AsDef() { return Def::Cast(UPB_UPCAST(this)); }
  const Def* AsDef() const { return Def::Cast(UPB_UPCAST(this)); }

  int32_t default_value() const { return upb_enumdef_default(this); }

  // May only be set if IsMutable().
  void set_full_name(const char *name) { AsDef()->set_full_name(name); }
  void set_full_name(const std::string& name) { AsDef()->set_full_name(name); }
  void set_default_value(int32_t val) {
    return upb_enumdef_setdefault(this, val);
  }

  // Adds a value to the enumdef.  Requires that no existing val has this
  // name or number (returns false and does not add if there is).  May only
  // be called if IsMutable().
  bool AddValue(char *name, int32_t num) {
    return upb_enumdef_addval(this, name, num);
  }
  bool AddValue(const std::string& name, int32_t num) {
    return upb_enumdef_addval(this, name.c_str(), num);
  }

  // Lookups from name to integer and vice-versa.
  bool LookupName(const char *name, int32_t* num) const {
    return upb_enumdef_ntoi(this, name, num);
  }

  // Lookup from integer to name, returns a NULL-terminated string which
  // the caller does not own, or NULL if not found.
  const char *LookupNumber(int32_t num) const {
    return upb_enumdef_iton(this, num);
  }

 private:
  UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(EnumDef);
};

class SymbolTable : public upb_symtab {
 public:
  // Converting from C types to C++ wrapper types.
  static SymbolTable* Cast(upb_symtab *s) {
    return static_cast<SymbolTable*>(s);
  }
  static const SymbolTable* Cast(const upb_symtab *s) {
    return static_cast<const SymbolTable*>(s);
  }

  static SymbolTable* New(const void *owner) {
    return Cast(upb_symtab_new(owner));
  }

  void Ref(const void *owner) const { upb_symtab_unref(this, owner); }
  void Unref(const void *owner) const { upb_symtab_unref(this, owner); }
  void DonateRef(const void *from, const void *to) const {
    upb_symtab_donateref(this, from, to);
  }

  // Adds the given defs to the symtab, resolving all symbols.  Only one def
  // per name may be in the list, but defs can replace existing defs in the
  // symtab.  The entire operation either succeeds or fails.  If the operation
  // fails, the symtab is unchanged, false is returned, and status indicates
  // the error.  The caller passes a ref on the defs in all cases.
  bool Add(Def *const *defs, int n, void *owner, Status* status) {
    return upb_symtab_add(this, (upb_def*const*)defs, n, owner, status);
  }
  bool Add(const std::vector<Def*>& defs, void *owner, Status* status) {
    return Add(&defs[0], defs.size(), owner, status);
  }

  // If the given name refers to a message in this symbol table, returns a new
  // ref to that MessageDef object, otherwise returns NULL.
  const MessageDef* LookupMessage(const char *name, void *owner) const {
    return MessageDef::Cast(upb_symtab_lookupmsg(this, name, owner));
  }

 private:
  UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(SymbolTable);
};

template <> inline const FieldDef* GetValue<const FieldDef*>(Value v) {
  return static_cast<const FieldDef*>(upb_value_getfielddef(v));
}

template <> inline Value MakeValue<FieldDef*>(FieldDef* v) {
  return upb_value_fielddef(v);
}

inline MessageDef* FieldDef::message() {
  return MessageDef::Cast(upb_fielddef_msgdef(this));
}
inline const MessageDef* FieldDef::message() const {
  return MessageDef::Cast(upb_fielddef_msgdef(this));
}

inline const Def* FieldDef::subdef() const {
  return Def::Cast(upb_fielddef_subdef(this));
}
inline bool FieldDef::set_subdef(Def* def) {
  return upb_fielddef_setsubdef(this, def);
}

}  // namespace upb

#endif
