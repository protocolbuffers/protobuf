/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Defs are upb's internal representation of the constructs that can appear
 * in a .proto file:
 *
 * - upb_msgdef: describes a "message" construct.
 * - upb_fielddef: describes a message field.
 * - upb_enumdef: describes an enum.
 * (TODO: definitions of services).
 *
 * Like upb_refcounted objects, defs are mutable only until frozen, and are
 * only thread-safe once frozen.
 *
 * This is a mixed C/C++ interface that offers a full API to both languages.
 * See the top-level README for more information.
 */

#ifndef UPB_DEF_H_
#define UPB_DEF_H_

#ifdef __cplusplus
#include <cstring>
#include <string>
#include <vector>

namespace upb {
class Def;
class EnumDef;
class FieldDef;
class MessageDef;
}

typedef upb::Def upb_def;
typedef upb::EnumDef upb_enumdef;
typedef upb::FieldDef upb_fielddef;
typedef upb::MessageDef upb_msgdef;
#else
struct upb_def;
struct upb_enumdef;
struct upb_fielddef;
struct upb_msgdef;

typedef struct upb_def upb_def;
typedef struct upb_enumdef upb_enumdef;
typedef struct upb_fielddef upb_fielddef;
typedef struct upb_msgdef upb_msgdef;
#endif

#include "upb/refcounted.h"


/* upb::Def: base class for defs  *********************************************/

// All the different kind of defs we support.  These correspond 1:1 with
// declarations in a .proto file.
typedef enum {
  UPB_DEF_MSG,
  UPB_DEF_FIELD,
  UPB_DEF_ENUM,
  UPB_DEF_SERVICE,          // Not yet implemented.

  UPB_DEF_ANY = -1,         // Wildcard for upb_symtab_get*()
} upb_deftype_t;

#ifdef __cplusplus

class upb::Def {
 public:
  typedef upb_deftype_t Type;

  Def* Dup(const void *owner) const;

  // Though not declared as such in C++, upb::RefCounted is the base of
  // Def and we can upcast to it.
  RefCounted* Upcast();
  const RefCounted* Upcast() const;

  // Functionality from upb::RefCounted.
  bool IsFrozen() const;
  void Ref(const void* owner) const;
  void Unref(const void* owner) const;
  void DonateRef(const void *from, const void *to) const;
  void CheckRef(const void *owner) const;

  Type def_type() const;

  // "fullname" is the def's fully-qualified name (eg. foo.bar.Message).
  const char *full_name() const;

  // The def must be mutable.  Caller retains ownership of fullname.  Defs are
  // not required to have a name; if a def has no name when it is frozen, it
  // will remain an anonymous def.
  bool set_full_name(const char *fullname);
  bool set_full_name(const std::string& fullname);

  // Freezes the given defs; this validates all constraints and marks the defs
  // as frozen (read-only).  "defs" may not contain any fielddefs, but fields
  // of any msgdefs will be frozen.
  //
  // Symbolic references to sub-types and enum defaults must have already been
  // resolved.  Any mutable defs reachable from any of "defs" must also be in
  // the list; more formally, "defs" must be a transitive closure of mutable
  // defs.
  //
  // After this operation succeeds, the finalized defs must only be accessed
  // through a const pointer!
  static bool Freeze(Def *const*defs, int n, Status *status);
  static bool Freeze(const std::vector<Def*>& defs, Status *status);

 private:
  UPB_DISALLOW_POD_OPS(Def);

#else
struct upb_def {
#endif
  upb_refcounted base;
  const char *fullname;
  upb_deftype_t type:8;
  // Used as a flag during the def's mutable stage.  Must be false unless
  // it is currently being used by a function on the stack.  This allows
  // us to easily determine which defs were passed into the function's
  // current invocation.
  bool came_from_user;
};

#define UPB_DEF_INIT(name, type) {UPB_REFCOUNT_INIT, name, type, false}

// Native C API.
#ifdef __cplusplus
extern "C" {
#endif
upb_def *upb_def_dup(const upb_def *def, const void *owner);

// From upb_refcounted.
bool upb_def_isfrozen(const upb_def *def);
void upb_def_ref(const upb_def *def, const void *owner);
void upb_def_unref(const upb_def *def, const void *owner);
void upb_def_donateref(const upb_def *def, const void *from, const void *to);
void upb_def_checkref(const upb_def *def, const void *owner);

upb_deftype_t upb_def_type(const upb_def *d);
const char *upb_def_fullname(const upb_def *d);
bool upb_def_setfullname(upb_def *def, const char *fullname);
bool upb_def_freeze(upb_def *const*defs, int n, upb_status *status);
#ifdef __cplusplus
}  // extern "C"
#endif


/* upb::FieldDef **************************************************************/

// We choose these to match descriptor.proto.  Clients may use UPB_TYPE() and
// UPB_LABEL() instead of referencing these directly.
typedef enum {
  UPB_TYPE_NONE     = -1,  // Internal-only, may be removed.
  UPB_TYPE_DOUBLE   = 1,
  UPB_TYPE_FLOAT    = 2,
  UPB_TYPE_INT64    = 3,
  UPB_TYPE_UINT64   = 4,
  UPB_TYPE_INT32    = 5,
  UPB_TYPE_FIXED64  = 6,
  UPB_TYPE_FIXED32  = 7,
  UPB_TYPE_BOOL     = 8,
  UPB_TYPE_STRING   = 9,
  UPB_TYPE_GROUP    = 10,
  UPB_TYPE_MESSAGE  = 11,
  UPB_TYPE_BYTES    = 12,
  UPB_TYPE_UINT32   = 13,
  UPB_TYPE_ENUM     = 14,
  UPB_TYPE_SFIXED32 = 15,
  UPB_TYPE_SFIXED64 = 16,
  UPB_TYPE_SINT32   = 17,
  UPB_TYPE_SINT64   = 18,
} upb_fieldtype_t;

#define UPB_NUM_TYPES 19

typedef enum {
  UPB_LABEL_OPTIONAL = 1,
  UPB_LABEL_REQUIRED = 2,
  UPB_LABEL_REPEATED = 3,
} upb_label_t;

// These macros are provided for legacy reasons.
#define UPB_TYPE(type) UPB_TYPE_ ## type
#define UPB_LABEL(type) UPB_LABEL_ ## type

// Info for a given field type.
typedef struct {
  uint8_t align;
  uint8_t size;
  uint8_t inmemory_type;    // For example, INT32, SINT32, and SFIXED32 -> INT32
} upb_typeinfo;

extern const upb_typeinfo upb_types[UPB_NUM_TYPES];

#ifdef __cplusplus

// A upb_fielddef describes a single field in a message.  It is most often
// found as a part of a upb_msgdef, but can also stand alone to represent
// an extension.
class upb::FieldDef {
 public:
  typedef upb_fieldtype_t Type;
  typedef upb_label_t Label;

  // Returns NULL if memory allocation failed.
  static FieldDef* New(const void *owner);

  // Duplicates the given field, returning NULL if memory allocation failed.
  // When a fielddef is duplicated, the subdef (if any) is made symbolic if it
  // wasn't already.  If the subdef is set but has no name (which is possible
  // since msgdefs are not required to have a name) the new fielddef's subdef
  // will be unset.
  FieldDef* Dup(const void *owner) const;

  // Though not declared as such in C++, upb::Def is the base of FieldDef and
  // we can upcast to it.
  Def* Upcast();
  const Def* Upcast() const;

  // Functionality from upb::RefCounted.
  bool IsFrozen() const;
  void Ref(const void* owner) const;
  void Unref(const void* owner) const;
  void DonateRef(const void *from, const void *to) const;
  void CheckRef(const void *owner) const;

  // Functionality from upb::Def.
  const char *full_name() const;
  bool set_full_name(const char *fullname);
  bool set_full_name(const std::string& fullname);

  Type type() const;        // Return UPB_TYPE_NONE if uninitialized.
  Label label() const;      // Defaults to UPB_LABEL_OPTIONAL.
  uint32_t number() const;  // Returns 0 if uninitialized.
  const MessageDef* message_def() const;

  // "number" and "name" must be set before the fielddef is added to a msgdef.
  // For the moment we do not allow these to be set once the fielddef is added
  // to a msgdef -- this could be relaxed in the future.
  bool set_number(uint32_t number);
  bool set_type(upb_fieldtype_t type);
  bool set_label(upb_label_t label);

  // These are the same as full_name()/set_full_name(), but since fielddefs
  // most often use simple, non-qualified names, we provide this accessor
  // also.  Generally only extensions will want to think of this name as
  // fully-qualified.
  bool set_name(const char *name);
  bool set_name(const std::string& name);
  const char *name() const;

  bool IsSubMessage() const;
  bool IsString() const;
  bool IsSequence() const;
  bool IsPrimitive() const;

  // Returns the default value for this fielddef, which may either be something
  // the client set explicitly or the "default default" (0 for numbers, empty
  // for strings).  The field's type indicates the type of the returned value,
  // except for enum fields that are still mutable.
  //
  // For enums the default can be set either numerically or symbolically -- the
  // upb_fielddef_default_is_symbolic() function below will indicate which it
  // is.  For string defaults, the value will be a upb_byteregion which is
  // invalidated by any other non-const call on this object.  Once the fielddef
  // is frozen, symbolic enum defaults are resolved, so frozen enum fielddefs
  // always have a default of type int32.
  Value default_value() const;

  // Sets default value for the field.  For numeric types, use
  // upb_fielddef_setdefault(), and "value" must match the type of the field.
  // For string/bytes types, use upb_fielddef_setdefaultstr().  Enum types may
  // use either, since the default may be set either numerically or
  // symbolically.
  //
  // NOTE: May only be called for fields whose type has already been set.
  // Also, will be reset to default if the field's type is set again.
  void set_default_value(Value value);
  bool set_default_string(const void *str, size_t len);
  bool set_default_string(const std::string& str);
  void set_default_cstr(const char *str);

  // The results of this function are only meaningful for mutable enum fields,
  // which can have a default specified either as an integer or as a string.
  // If this returns true, the default returned from upb_fielddef_default() is
  // a string, otherwise it is an integer.
  bool IsDefaultSymbolic() const;

  // If this is an enum field with a symbolic default, resolves the default and
  // returns true if resolution was successful or if this field didn't need to
  // be resolved (because it is not an enum with a symbolic default).
  bool ResolveDefault();

  // Submessage and enum fields must reference a "subdef", which is the
  // upb_msgdef or upb_enumdef that defines their type.  Note that when the
  // fielddef is mutable it may not have a subdef *yet*, but this function
  // still returns true to indicate that the field's type requires a subdef.
  bool HasSubDef() const;

  // Returns the enum or submessage def or symbolic name for this field, if
  // any.  Requires that upb_hassubdef(f).  Returns NULL if the subdef has not
  // been set or if you ask for a subdef when the subdef is currently set
  // symbolically (or vice-versa).  To access the subdef's name for a linked
  // fielddef, use upb_def_fullname(upb_fielddef_subdef(f)).
  //
  // Caller does *not* own a ref on the returned def or string.
  // upb_fielddef_subdefename() is non-const because frozen defs will never
  // have a symbolic reference (they must be resolved before the msgdef can be
  // frozen).
  const Def* subdef() const;
  const char* subdef_name() const;

  // Before a fielddef is frozen, its subdef may be set either directly (with a
  // upb::Def*) or symbolically.  Symbolic refs must be resolved before the
  // containing msgdef can be frozen (see upb_resolve() above).  The client is
  // responsible for making sure that "subdef" lives until this fielddef is
  // frozen or deleted.
  //
  // Both methods require that upb_hassubdef(f) (so the type must be set prior
  // to calling these methods).  Returns false if this is not the case, or if
  // the given subdef is not of the correct type.  The subdef is reset if the
  // field's type is changed.  The subdef can be set to NULL to clear it.
  bool set_subdef(const Def* subdef);
  bool set_subdef_name(const char* name);
  bool set_subdef_name(const std::string& name);

 private:
  UPB_DISALLOW_POD_OPS(FieldDef);

#else
struct upb_fielddef {
#endif
  upb_def base;
  const upb_msgdef *msgdef;
  union {
    const upb_def *def;  // If !subdef_is_symbolic.
    char *name;    // If subdef_is_symbolic.
  } sub;  // The msgdef or enumdef for this field, if upb_hassubdef(f).
  bool subdef_is_symbolic;
  bool default_is_string;
  bool subdef_is_owned;
  upb_fieldtype_t type_;
  upb_label_t label_;
  uint32_t number_;
  upb_value defaultval;  // Only for non-repeated scalars and strings.
  uint32_t selector_base;  // Used to index into a upb::Handlers table.
};

// This will only work for static initialization because of the subdef_is_owned
// initialization.  Theoretically the other _INIT() macros could possible work
// for non-static initialization, but this has not been tested.
#define UPB_FIELDDEF_INIT(label, type, name, num, msgdef, subdef, \
                          selector_base, defaultval) \
  {UPB_DEF_INIT(name, UPB_DEF_FIELD), msgdef, {subdef}, false, \
   type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES, \
   false, /* subdef_is_owned: not used since fielddef is not freed. */ \
   type, label, num, defaultval, selector_base}

// Native C API.
#ifdef __cplusplus
extern "C" {
#endif
upb_fielddef *upb_fielddef_new(const void *owner);
upb_fielddef *upb_fielddef_dup(const upb_fielddef *f, const void *owner);

// From upb_refcounted.
bool upb_fielddef_isfrozen(const upb_fielddef *f);
void upb_fielddef_ref(const upb_fielddef *f, const void *owner);
void upb_fielddef_unref(const upb_fielddef *f, const void *owner);
void upb_fielddef_donateref(
    const upb_fielddef *f, const void *from, const void *to);
void upb_fielddef_checkref(const upb_fielddef *f, const void *owner);

// From upb_def.
const char *upb_fielddef_fullname(const upb_fielddef *f);
bool upb_fielddef_setfullname(upb_fielddef *f, const char *fullname);

upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f);
upb_label_t upb_fielddef_label(const upb_fielddef *f);
uint32_t upb_fielddef_number(const upb_fielddef *f);
const char *upb_fielddef_name(const upb_fielddef *f);
const upb_msgdef *upb_fielddef_msgdef(const upb_fielddef *f);
upb_msgdef *upb_fielddef_msgdef_mutable(upb_fielddef *f);
bool upb_fielddef_settype(upb_fielddef *f, upb_fieldtype_t type);
bool upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label);
bool upb_fielddef_setnumber(upb_fielddef *f, uint32_t number);
bool upb_fielddef_setname(upb_fielddef *f, const char *name);
bool upb_fielddef_issubmsg(const upb_fielddef *f);
bool upb_fielddef_isstring(const upb_fielddef *f);
bool upb_fielddef_isseq(const upb_fielddef *f);
bool upb_fielddef_isprimitive(const upb_fielddef *f);
upb_value upb_fielddef_default(const upb_fielddef *f);
void upb_fielddef_setdefault(upb_fielddef *f, upb_value value);
bool upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len);
void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str);
bool upb_fielddef_default_is_symbolic(const upb_fielddef *f);
bool upb_fielddef_resolvedefault(upb_fielddef *f);
bool upb_fielddef_hassubdef(const upb_fielddef *f);
bool upb_fielddef_setsubdef(upb_fielddef *f, const upb_def *subdef);
bool upb_fielddef_setsubdefname(upb_fielddef *f, const char *name);
const upb_def *upb_fielddef_subdef(const upb_fielddef *f);
const char *upb_fielddef_subdefname(const upb_fielddef *f);
#ifdef __cplusplus
}  // extern "C"
#endif


/* upb::MessageDef ************************************************************/

typedef upb_inttable_iter upb_msg_iter;

#ifdef __cplusplus

// Structure that describes a single .proto message type.
class upb::MessageDef {
 public:
  // Returns NULL if memory allocation failed.
  static MessageDef* New(const void *owner);

  // Though not declared as such in C++, upb::Def is the base of MessageDef and
  // we can upcast to it.
  Def* Upcast();
  const Def* Upcast() const;

  // Functionality from upb::RefCounted.
  bool IsFrozen() const;
  void Ref(const void* owner) const;
  void Unref(const void* owner) const;
  void DonateRef(const void *from, const void *to) const;
  void CheckRef(const void *owner) const;

  // Functionality from upb::Def.
  const char *full_name() const;
  bool set_full_name(const char *fullname);
  bool set_full_name(const std::string& fullname);

  // The number of fields that belong to the MessageDef.
  int field_count() const;

  // Adds a set of fields (upb_fielddef objects) to a msgdef.  Requires that
  // the msgdef and all the fielddefs are mutable.  The fielddef's name and
  // number must be set, and the message may not already contain any field with
  // this name or number, and this fielddef may not be part of another message.
  // In error cases false is returned and the msgdef is unchanged.  On success,
  // the caller donates a ref from ref_donor (if non-NULL).
  bool AddField(upb_fielddef *f, const void *ref_donor);

  // These return NULL if the field is not found.
  FieldDef* FindFieldByNumber(uint32_t number);
  FieldDef* FieldFieldByName(const char *name);
  const FieldDef* FindFieldByNumber(uint32_t number) const;
  const FieldDef* FieldFieldByName(const char *name) const;

  // Returns a new msgdef that is a copy of the given msgdef (and a copy of all
  // the fields) but with any references to submessages broken and replaced
  // with just the name of the submessage.  Returns NULL if memory allocation
  // failed.
  //
  // TODO(haberman): which is more useful, keeping fields resolved or
  // unresolving them?  If there's no obvious answer, Should this functionality
  // just be moved into symtab.c?
  MessageDef* Dup(const void *owner) const;

  // Iteration over fields.  The order is undefined.
  class Iterator {
   public:
    explicit Iterator(MessageDef* md);

    FieldDef* field();
    bool Done();
    void Next();

   private:
    upb_msg_iter iter_;
  };

  // For iterating over the fields of a const MessageDef.
  class ConstIterator {
   public:
    explicit ConstIterator(const MessageDef* md);

    const FieldDef* field();
    bool Done();
    void Next();

   private:
    upb_msg_iter iter_;
  };

 private:
  UPB_DISALLOW_POD_OPS(MessageDef);

#else
struct upb_msgdef {
#endif
  upb_def base;
  size_t selector_count;

  // Tables for looking up fields by number and name.
  upb_inttable itof;  // int to field
  upb_strtable ntof;  // name to field

  // TODO(haberman): proper extension ranges (there can be multiple).
};

#define UPB_MSGDEF_INIT(name, itof, ntof, selector_count) \
  {UPB_DEF_INIT(name, UPB_DEF_MSG), selector_count, itof, ntof}

#ifdef __cplusplus
extern "C" {
#endif
// Returns NULL if memory allocation failed.
upb_msgdef *upb_msgdef_new(const void *owner);

// From upb_refcounted.
bool upb_msgdef_isfrozen(const upb_msgdef *m);
void upb_msgdef_ref(const upb_msgdef *m, const void *owner);
void upb_msgdef_unref(const upb_msgdef *m, const void *owner);
void upb_msgdef_donateref(
    const upb_msgdef *m, const void *from, const void *to);
void upb_msgdef_checkref(const upb_msgdef *m, const void *owner);

// From upb_def.
const char *upb_msgdef_fullname(const upb_msgdef *m);
bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname);

upb_msgdef *upb_msgdef_dup(const upb_msgdef *m, const void *owner);
bool upb_msgdef_addfields(
    upb_msgdef *m, upb_fielddef *const *f, int n, const void *ref_donor);
bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f, const void *ref_donor);
const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i);
const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name);
upb_fielddef *upb_msgdef_itof_mutable(upb_msgdef *m, uint32_t i);
upb_fielddef *upb_msgdef_ntof_mutable(upb_msgdef *m, const char *name);
int upb_msgdef_numfields(const upb_msgdef *m);

// upb_msg_iter i;
// for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
//   upb_fielddef *f = upb_msg_iter_field(&i);
//   // ...
// }
void upb_msg_begin(upb_msg_iter *iter, const upb_msgdef *m);
void upb_msg_next(upb_msg_iter *iter);
bool upb_msg_done(upb_msg_iter *iter);
upb_fielddef *upb_msg_iter_field(upb_msg_iter *iter);
#ifdef __cplusplus
}  // extern "C
#endif


/* upb::EnumDef ***************************************************************/

typedef upb_strtable_iter upb_enum_iter;

#ifdef __cplusplus

class upb::EnumDef {
 public:
  // Returns NULL if memory allocation failed.
  static EnumDef* New(const void *owner);

  // Though not declared as such in C++, upb::Def is the base of EnumDef and we
  // can upcast to it.
  Def* Upcast();
  const Def* Upcast() const;

  // Functionality from upb::RefCounted.
  bool IsFrozen() const;
  void Ref(const void* owner) const;
  void Unref(const void* owner) const;
  void DonateRef(const void *from, const void *to) const;
  void CheckRef(const void *owner) const;

  // Functionality from upb::Def.
  const char *full_name() const;
  bool set_full_name(const char *fullname);
  bool set_full_name(const std::string& fullname);

  // The value that is used as the default when no field default is specified.
  int32_t default_value() const;
  void set_default_value(int32_t val);

  // Returns the number of values currently defined in the enum.  Note that
  // multiple names can refer to the same number, so this may be greater than
  // the total number of unique numbers.
  int value_count() const;

  // Adds a single name/number pair to the enum.  Fails if this name has
  // already been used by another value.
  bool AddValue(const char* name, int32_t num, Status* status);
  bool AddValue(const std::string& name, int32_t num, Status* status);

  // Lookups from name to integer, returning true if found.
  bool FindValueByName(const char* name, int32_t* num) const;

  // Finds the name corresponding to the given number, or NULL if none was
  // found.  If more than one name corresponds to this number, returns the
  // first one that was added.
  const char* FindValueByNumber(int32_t num) const;

  // Returns a new EnumDef with all the same values.  The new EnumDef will be
  // owned by the given owner.
  EnumDef* Dup(const void *owner) const;

  // Iteration over name/value pairs.  The order is undefined.
  // Adding an enum val invalidates any iterators.
  class Iterator {
   public:
    explicit Iterator(const EnumDef*);

    int32_t number();
    const char* name();
    bool Done();
    void Next();

   private:
    upb_enum_iter iter_;
  };

 private:
  UPB_DISALLOW_POD_OPS(EnumDef);

#else
struct upb_enumdef {
#endif
  upb_def base;
  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
};

#define UPB_ENUMDEF_INIT(name, ntoi, iton, defaultval) \
  {UPB_DEF_INIT(name, UPB_DEF_ENUM), ntoi, iton, defaultval}

// Native C API.
#ifdef __cplusplus
extern "C" {
#endif
upb_enumdef *upb_enumdef_new(const void *owner);
upb_enumdef *upb_enumdef_dup(const upb_enumdef *e, const void *owner);

// From upb_refcounted.
void upb_enumdef_unref(const upb_enumdef *e, const void *owner);
bool upb_enumdef_isfrozen(const upb_enumdef *e);
void upb_enumdef_ref(const upb_enumdef *e, const void *owner);
void upb_enumdef_donateref(
    const upb_enumdef *m, const void *from, const void *to);
void upb_enumdef_checkref(const upb_enumdef *e, const void *owner);

// From upb_def.
const char *upb_enumdef_fullname(const upb_enumdef *e);
bool upb_enumdef_setfullname(upb_enumdef *e, const char *fullname);

int32_t upb_enumdef_default(const upb_enumdef *e);
void upb_enumdef_setdefault(upb_enumdef *e, int32_t val);
int upb_enumdef_numvals(const upb_enumdef *e);
bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num,
                        upb_status *status);
bool upb_enumdef_ntoi(const upb_enumdef *e, const char *name, int32_t *num);
const char *upb_enumdef_iton(const upb_enumdef *e, int32_t num);

//  upb_enum_iter i;
//  for(upb_enum_begin(&i, e); !upb_enum_done(&i); upb_enum_next(&i)) {
//    // ...
//  }
void upb_enum_begin(upb_enum_iter *iter, const upb_enumdef *e);
void upb_enum_next(upb_enum_iter *iter);
bool upb_enum_done(upb_enum_iter *iter);
const char *upb_enum_iter_name(upb_enum_iter *iter);
int32_t upb_enum_iter_number(upb_enum_iter *iter);
#ifdef __cplusplus
}  // extern "C"
#endif


/* upb_def casts **************************************************************/

// Dynamic casts, for determining if a def is of a particular type at runtime.
// Downcasts, for when some wants to assert that a def is of a particular type.
// These are only checked if we are building debug.
#define UPB_DEF_CASTS(lower, upper) \
  INLINE const upb_ ## lower *upb_dyncast_ ## lower(const upb_def *def) { \
    if (upb_def_type(def) != UPB_DEF_ ## upper) return NULL; \
    return (upb_ ## lower*)def; \
  } \
  INLINE const upb_ ## lower *upb_downcast_ ## lower(const upb_def *def) { \
    assert(upb_def_type(def) == UPB_DEF_ ## upper); \
    return (const upb_ ## lower*)def; \
  } \
  INLINE upb_ ## lower *upb_dyncast_ ## lower ## _mutable(upb_def *def) { \
    return (upb_ ## lower*)upb_dyncast_ ## lower(def); \
  } \
  INLINE upb_ ## lower *upb_downcast_ ## lower ## _mutable(upb_def *def) { \
    return (upb_ ## lower*)upb_downcast_ ## lower(def); \
  }
UPB_DEF_CASTS(msgdef, MSG);
UPB_DEF_CASTS(fielddef, FIELD);
UPB_DEF_CASTS(enumdef, ENUM);
#undef UPB_DEF_CASTS

#ifdef __cplusplus

INLINE const char *upb_safecstr(const std::string& str) {
  assert(str.size() == std::strlen(str.c_str()));
  return str.c_str();
}

// Inline C++ wrappers.
namespace upb {

inline Def* Def::Dup(const void *owner) const {
  return upb_def_dup(this, owner);
}
inline RefCounted* Def::Upcast() {
  return upb_upcast(this);
}
inline const RefCounted* Def::Upcast() const {
  return upb_upcast(this);
}
inline bool Def::IsFrozen() const {
  return upb_def_isfrozen(this);
}
inline void Def::Ref(const void* owner) const {
  upb_def_ref(this, owner);
}
inline void Def::Unref(const void* owner) const {
  upb_def_unref(this, owner);
}
inline void Def::DonateRef(const void *from, const void *to) const {
  upb_def_donateref(this, from, to);
}
inline void Def::CheckRef(const void *owner) const {
  upb_def_checkref(this, owner);
}
inline Def::Type Def::def_type() const {
  return upb_def_type(this);
}
inline const char *Def::full_name() const {
  return upb_def_fullname(this);
}
inline bool Def::set_full_name(const char *fullname) {
  return upb_def_setfullname(this, fullname);
}
inline bool Def::set_full_name(const std::string& fullname) {
  return upb_def_setfullname(this, upb_safecstr(fullname));
}
inline bool Def::Freeze(Def *const*defs, int n, Status *status) {
  return upb_def_freeze(defs, n, status);
}
inline bool Def::Freeze(const std::vector<Def*>& defs, Status *status) {
  return upb_def_freeze((Def*const*)&defs[0], defs.size(), status);
}

inline FieldDef* FieldDef::New(const void *owner) {
  return upb_fielddef_new(owner);
}
inline FieldDef* FieldDef::Dup(const void *owner) const {
  return upb_fielddef_dup(this, owner);
}
inline Def* FieldDef::Upcast() {
  return upb_upcast(this);
}
inline const Def* FieldDef::Upcast() const {
  return upb_upcast(this);
}
inline bool FieldDef::IsFrozen() const {
  return upb_fielddef_isfrozen(this);
}
inline void FieldDef::Ref(const void* owner) const {
  upb_fielddef_ref(this, owner);
}
inline void FieldDef::Unref(const void* owner) const {
  upb_fielddef_unref(this, owner);
}
inline void FieldDef::DonateRef(const void *from, const void *to) const {
  upb_fielddef_donateref(this, from, to);
}
inline void FieldDef::CheckRef(const void *owner) const {
  upb_fielddef_checkref(this, owner);
}
inline const char *FieldDef::full_name() const {
  return upb_fielddef_fullname(this);
}
inline bool FieldDef::set_full_name(const char *fullname) {
  return upb_fielddef_setfullname(this, fullname);
}
inline bool FieldDef::set_full_name(const std::string& fullname) {
  return upb_fielddef_setfullname(this, upb_safecstr(fullname));
}
inline FieldDef::Type FieldDef::type() const {
  return upb_fielddef_type(this);
}
inline FieldDef::Label FieldDef::label() const {
  return upb_fielddef_label(this);
}
inline uint32_t FieldDef::number() const {
  return upb_fielddef_number(this);
}
inline const char *FieldDef::name() const {
  return upb_fielddef_name(this);
}
inline const MessageDef* FieldDef::message_def() const {
  return upb_fielddef_msgdef(this);
}
inline bool FieldDef::set_number(uint32_t number) {
  return upb_fielddef_setnumber(this, number);
}
inline bool FieldDef::set_name(const char *name) {
  return upb_fielddef_setname(this, name);
}
inline bool FieldDef::set_name(const std::string& name) {
  return upb_fielddef_setname(this, upb_safecstr(name));
}
inline bool FieldDef::set_type(upb_fieldtype_t type) {
  return upb_fielddef_settype(this, type);
}
inline bool FieldDef::set_label(upb_label_t label) {
  return upb_fielddef_setlabel(this, label);
}
inline bool FieldDef::IsSubMessage() const {
  return upb_fielddef_issubmsg(this);
}
inline bool FieldDef::IsString() const {
  return upb_fielddef_isstring(this);
}
inline bool FieldDef::IsSequence() const {
  return upb_fielddef_isseq(this);
}
inline Value FieldDef::default_value() const {
  return upb_fielddef_default(this);
}
inline void FieldDef::set_default_value(Value value) {
  upb_fielddef_setdefault(this, value);
}
inline bool FieldDef::set_default_string(const void *str, size_t len) {
  return upb_fielddef_setdefaultstr(this, str, len);
}
inline bool FieldDef::set_default_string(const std::string& str) {
  return upb_fielddef_setdefaultstr(this, str.c_str(), str.size());
}
inline void FieldDef::set_default_cstr(const char *str) {
  return upb_fielddef_setdefaultcstr(this, str);
}
inline bool FieldDef::IsDefaultSymbolic() const {
  return upb_fielddef_default_is_symbolic(this);
}
inline bool FieldDef::ResolveDefault() {
  return upb_fielddef_resolvedefault(this);
}
inline bool FieldDef::HasSubDef() const {
  return upb_fielddef_hassubdef(this);
}
inline const Def* FieldDef::subdef() const {
  return upb_fielddef_subdef(this);
}
inline const char* FieldDef::subdef_name() const {
  return upb_fielddef_subdefname(this);
}
inline bool FieldDef::set_subdef(const Def* subdef) {
  return upb_fielddef_setsubdef(this, subdef);
}
inline bool FieldDef::set_subdef_name(const char* name) {
  return upb_fielddef_setsubdefname(this, name);
}
inline bool FieldDef::set_subdef_name(const std::string& name) {
  return upb_fielddef_setsubdefname(this, upb_safecstr(name));
}

inline MessageDef* MessageDef::New(const void *owner) {
  return upb_msgdef_new(owner);
}
inline Def* MessageDef::Upcast() {
  return upb_upcast(this);
}
inline const Def* MessageDef::Upcast() const {
  return upb_upcast(this);
}
inline bool MessageDef::IsFrozen() const {
  return upb_msgdef_isfrozen(this);
}
inline void MessageDef::Ref(const void* owner) const {
  return upb_msgdef_ref(this, owner);
}
inline void MessageDef::Unref(const void* owner) const {
  return upb_msgdef_unref(this, owner);
}
inline void MessageDef::DonateRef(const void *from, const void *to) const {
  return upb_msgdef_donateref(this, from, to);
}
inline void MessageDef::CheckRef(const void *owner) const {
  return upb_msgdef_checkref(this, owner);
}
inline const char *MessageDef::full_name() const {
  return upb_msgdef_fullname(this);
}
inline bool MessageDef::set_full_name(const char *fullname) {
  return upb_msgdef_setfullname(this, fullname);
}
inline bool MessageDef::set_full_name(const std::string& fullname) {
  return upb_msgdef_setfullname(this, upb_safecstr(fullname));
}
inline int MessageDef::field_count() const {
  return upb_msgdef_numfields(this);
}
inline bool MessageDef::AddField(upb_fielddef *f, const void *ref_donor) {
  return upb_msgdef_addfield(this, f, ref_donor);
}
inline FieldDef* MessageDef::FindFieldByNumber(uint32_t number) {
  return upb_msgdef_itof_mutable(this, number);
}
inline FieldDef* MessageDef::FieldFieldByName(const char *name) {
  return upb_msgdef_ntof_mutable(this, name);
}
inline const FieldDef* MessageDef::FindFieldByNumber(uint32_t number) const {
  return upb_msgdef_itof(this, number);
}
inline const FieldDef* MessageDef::FieldFieldByName(const char *name) const {
  return upb_msgdef_ntof(this, name);
}
inline MessageDef* MessageDef::Dup(const void *owner) const {
  return upb_msgdef_dup(this, owner);
}

inline MessageDef::Iterator::Iterator(MessageDef* md) {
  upb_msg_begin(&iter_, md);
}
inline FieldDef* MessageDef::Iterator::field() {
  return upb_msg_iter_field(&iter_);
}
inline bool MessageDef::Iterator::Done() {
  return upb_msg_done(&iter_);
}
inline void MessageDef::Iterator::Next() {
  return upb_msg_next(&iter_);
}

inline MessageDef::ConstIterator::ConstIterator(const MessageDef* md) {
  upb_msg_begin(&iter_, md);
}
inline const FieldDef* MessageDef::ConstIterator::field() {
  return upb_msg_iter_field(&iter_);
}
inline bool MessageDef::ConstIterator::Done() {
  return upb_msg_done(&iter_);
}
inline void MessageDef::ConstIterator::Next() {
  return upb_msg_next(&iter_);
}

inline EnumDef* EnumDef::New(const void *owner) {
  return upb_enumdef_new(owner);
}
inline Def* EnumDef::Upcast() {
  return upb_upcast(this);
}
inline const Def* EnumDef::Upcast() const {
  return upb_upcast(this);
}
inline bool EnumDef::IsFrozen() const {
  return upb_enumdef_isfrozen(this);
}
inline void EnumDef::Ref(const void* owner) const {
  return upb_enumdef_ref(this, owner);
}
inline void EnumDef::Unref(const void* owner) const {
  return upb_enumdef_unref(this, owner);
}
inline void EnumDef::DonateRef(const void *from, const void *to) const {
  return upb_enumdef_donateref(this, from, to);
}
inline void EnumDef::CheckRef(const void *owner) const {
  return upb_enumdef_checkref(this, owner);
}
inline const char *EnumDef::full_name() const {
  return upb_enumdef_fullname(this);
}
inline bool EnumDef::set_full_name(const char *fullname) {
  return upb_enumdef_setfullname(this, fullname);
}
inline bool EnumDef::set_full_name(const std::string& fullname) {
  return upb_enumdef_setfullname(this, upb_safecstr(fullname));
}
inline int32_t EnumDef::default_value() const {
  return upb_enumdef_default(this);
}
inline void EnumDef::set_default_value(int32_t val) {
  upb_enumdef_setdefault(this, val);
}
inline int EnumDef::value_count() const {
  return upb_enumdef_numvals(this);
}
inline bool EnumDef::AddValue(const char* name, int32_t num, Status* status) {
  return upb_enumdef_addval(this, name, num, status);
}
inline bool EnumDef::AddValue(
    const std::string& name, int32_t num, Status* status) {
  return upb_enumdef_addval(this, upb_safecstr(name), num, status);
}
inline bool EnumDef::FindValueByName(const char* name, int32_t* num) const {
  return upb_enumdef_ntoi(this, name, num);
}
inline const char* EnumDef::FindValueByNumber(int32_t num) const {
  return upb_enumdef_iton(this, num);
}
inline EnumDef* EnumDef::Dup(const void *owner) const {
  return upb_enumdef_dup(this, owner);
}

inline EnumDef::Iterator::Iterator(const EnumDef* e) {
  upb_enum_begin(&iter_, e);
}
inline int32_t EnumDef::Iterator::number() {
  return upb_enum_iter_number(&iter_);
}
inline const char* EnumDef::Iterator::name() {
  return upb_enum_iter_name(&iter_);
}
inline bool EnumDef::Iterator::Done() {
  return upb_enum_done(&iter_);
}
inline void EnumDef::Iterator::Next() {
  return upb_enum_next(&iter_);
}
}  // namespace upb
#endif

#endif  /* UPB_DEF_H_ */
