/*
** Defs are upb's internal representation of the constructs that can appear
** in a .proto file:
**
** - upb::MessageDef (upb_msgdef): describes a "message" construct.
** - upb::FieldDef (upb_fielddef): describes a message field.
** - upb::EnumDef (upb_enumdef): describes an enum.
** - upb::OneofDef (upb_oneofdef): describes a oneof.
** - upb::Def (upb_def): base class of all the others.
**
** TODO: definitions of services.
**
** Like upb_refcounted objects, defs are mutable only until frozen, and are
** only thread-safe once frozen.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_DEF_H_
#define UPB_DEF_H_

#include "upb/refcounted.h"

#ifdef __cplusplus
#include <cstring>
#include <string>
#include <vector>

namespace upb {
class Def;
class EnumDef;
class FieldDef;
class MessageDef;
class OneofDef;
}
#endif

UPB_DECLARE_DERIVED_TYPE(upb::Def, upb::RefCounted, upb_def, upb_refcounted)

/* The maximum message depth that the type graph can have.  This is a resource
 * limit for the C stack since we sometimes need to recursively traverse the
 * graph.  Cycles are ok; the traversal will stop when it detects a cycle, but
 * we must hit the cycle before the maximum depth is reached.
 *
 * If having a single static limit is too inflexible, we can add another variant
 * of Def::Freeze that allows specifying this as a parameter. */
#define UPB_MAX_MESSAGE_DEPTH 64


/* upb::Def: base class for defs  *********************************************/

/* All the different kind of defs we support.  These correspond 1:1 with
 * declarations in a .proto file. */
typedef enum {
  UPB_DEF_MSG,
  UPB_DEF_FIELD,
  UPB_DEF_ENUM,
  UPB_DEF_ONEOF,
  UPB_DEF_SERVICE,   /* Not yet implemented. */
  UPB_DEF_ANY = -1   /* Wildcard for upb_symtab_get*() */
} upb_deftype_t;

#ifdef __cplusplus

/* The base class of all defs.  Its base is upb::RefCounted (use upb::upcast()
 * to convert). */
class upb::Def {
 public:
  typedef upb_deftype_t Type;

  Def* Dup(const void *owner) const;

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  Type def_type() const;

  /* "fullname" is the def's fully-qualified name (eg. foo.bar.Message). */
  const char *full_name() const;

  /* The def must be mutable.  Caller retains ownership of fullname.  Defs are
   * not required to have a name; if a def has no name when it is frozen, it
   * will remain an anonymous def.  On failure, returns false and details in "s"
   * if non-NULL. */
  bool set_full_name(const char* fullname, upb::Status* s);
  bool set_full_name(const std::string &fullname, upb::Status* s);

  /* Freezes the given defs; this validates all constraints and marks the defs
   * as frozen (read-only).  "defs" may not contain any fielddefs, but fields
   * of any msgdefs will be frozen.
   *
   * Symbolic references to sub-types and enum defaults must have already been
   * resolved.  Any mutable defs reachable from any of "defs" must also be in
   * the list; more formally, "defs" must be a transitive closure of mutable
   * defs.
   *
   * After this operation succeeds, the finalized defs must only be accessed
   * through a const pointer! */
  static bool Freeze(Def* const* defs, int n, Status* status);
  static bool Freeze(const std::vector<Def*>& defs, Status* status);

 private:
  UPB_DISALLOW_POD_OPS(Def, upb::Def)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_def *upb_def_dup(const upb_def *def, const void *owner);

/* Include upb_refcounted methods like upb_def_ref()/upb_def_unref(). */
UPB_REFCOUNTED_CMETHODS(upb_def, upb_def_upcast)

upb_deftype_t upb_def_type(const upb_def *d);
const char *upb_def_fullname(const upb_def *d);
bool upb_def_setfullname(upb_def *def, const char *fullname, upb_status *s);
bool upb_def_freeze(upb_def *const *defs, int n, upb_status *s);

UPB_END_EXTERN_C


/* upb::Def casts *************************************************************/

#ifdef __cplusplus
#define UPB_CPP_CASTS(cname, cpptype)                                          \
  namespace upb {                                                              \
  template <>                                                                  \
  inline cpptype *down_cast<cpptype *, Def>(Def * def) {                       \
    return upb_downcast_##cname##_mutable(def);                                \
  }                                                                            \
  template <>                                                                  \
  inline cpptype *dyn_cast<cpptype *, Def>(Def * def) {                        \
    return upb_dyncast_##cname##_mutable(def);                                 \
  }                                                                            \
  template <>                                                                  \
  inline const cpptype *down_cast<const cpptype *, const Def>(                 \
      const Def *def) {                                                        \
    return upb_downcast_##cname(def);                                          \
  }                                                                            \
  template <>                                                                  \
  inline const cpptype *dyn_cast<const cpptype *, const Def>(const Def *def) { \
    return upb_dyncast_##cname(def);                                           \
  }                                                                            \
  template <>                                                                  \
  inline const cpptype *down_cast<const cpptype *, Def>(Def * def) {           \
    return upb_downcast_##cname(def);                                          \
  }                                                                            \
  template <>                                                                  \
  inline const cpptype *dyn_cast<const cpptype *, Def>(Def * def) {            \
    return upb_dyncast_##cname(def);                                           \
  }                                                                            \
  }  /* namespace upb */
#else
#define UPB_CPP_CASTS(cname, cpptype)
#endif  /* __cplusplus */

/* Dynamic casts, for determining if a def is of a particular type at runtime.
 * Downcasts, for when some wants to assert that a def is of a particular type.
 * These are only checked if we are building debug. */
#define UPB_DEF_CASTS(lower, upper, cpptype)                               \
  UPB_INLINE const upb_##lower *upb_dyncast_##lower(const upb_def *def) {  \
    if (upb_def_type(def) != UPB_DEF_##upper) return NULL;                 \
    return (upb_##lower *)def;                                             \
  }                                                                        \
  UPB_INLINE const upb_##lower *upb_downcast_##lower(const upb_def *def) { \
    assert(upb_def_type(def) == UPB_DEF_##upper);                          \
    return (const upb_##lower *)def;                                       \
  }                                                                        \
  UPB_INLINE upb_##lower *upb_dyncast_##lower##_mutable(upb_def *def) {    \
    return (upb_##lower *)upb_dyncast_##lower(def);                        \
  }                                                                        \
  UPB_INLINE upb_##lower *upb_downcast_##lower##_mutable(upb_def *def) {   \
    return (upb_##lower *)upb_downcast_##lower(def);                       \
  }                                                                        \
  UPB_CPP_CASTS(lower, cpptype)

#define UPB_DEFINE_DEF(cppname, lower, upper, cppmethods, members)             \
  UPB_DEFINE_CLASS2(cppname, upb::Def, upb::RefCounted, cppmethods,            \
                   members)                                                    \
  UPB_DEF_CASTS(lower, upper, cppname)

#define UPB_DECLARE_DEF_TYPE(cppname, lower, upper) \
  UPB_DECLARE_DERIVED_TYPE2(cppname, upb::Def, upb::RefCounted, \
                            upb_ ## lower, upb_def, upb_refcounted) \
  UPB_DEF_CASTS(lower, upper, cppname)

UPB_DECLARE_DEF_TYPE(upb::FieldDef, fielddef, FIELD)
UPB_DECLARE_DEF_TYPE(upb::MessageDef, msgdef, MSG)
UPB_DECLARE_DEF_TYPE(upb::EnumDef, enumdef, ENUM)
UPB_DECLARE_DEF_TYPE(upb::OneofDef, oneofdef, ONEOF)

#undef UPB_DECLARE_DEF_TYPE
#undef UPB_DEF_CASTS
#undef UPB_CPP_CASTS


/* upb::FieldDef **************************************************************/

/* The types a field can have.  Note that this list is not identical to the
 * types defined in descriptor.proto, which gives INT32 and SINT32 separate
 * types (we distinguish the two with the "integer encoding" enum below). */
typedef enum {
  UPB_TYPE_FLOAT    = 1,
  UPB_TYPE_DOUBLE   = 2,
  UPB_TYPE_BOOL     = 3,
  UPB_TYPE_STRING   = 4,
  UPB_TYPE_BYTES    = 5,
  UPB_TYPE_MESSAGE  = 6,
  UPB_TYPE_ENUM     = 7,  /* Enum values are int32. */
  UPB_TYPE_INT32    = 8,
  UPB_TYPE_UINT32   = 9,
  UPB_TYPE_INT64    = 10,
  UPB_TYPE_UINT64   = 11
} upb_fieldtype_t;

/* The repeated-ness of each field; this matches descriptor.proto. */
typedef enum {
  UPB_LABEL_OPTIONAL = 1,
  UPB_LABEL_REQUIRED = 2,
  UPB_LABEL_REPEATED = 3
} upb_label_t;

/* How integers should be encoded in serializations that offer multiple
 * integer encoding methods. */
typedef enum {
  UPB_INTFMT_VARIABLE = 1,
  UPB_INTFMT_FIXED = 2,
  UPB_INTFMT_ZIGZAG = 3   /* Only for signed types (INT32/INT64). */
} upb_intfmt_t;

/* Descriptor types, as defined in descriptor.proto. */
typedef enum {
  UPB_DESCRIPTOR_TYPE_DOUBLE   = 1,
  UPB_DESCRIPTOR_TYPE_FLOAT    = 2,
  UPB_DESCRIPTOR_TYPE_INT64    = 3,
  UPB_DESCRIPTOR_TYPE_UINT64   = 4,
  UPB_DESCRIPTOR_TYPE_INT32    = 5,
  UPB_DESCRIPTOR_TYPE_FIXED64  = 6,
  UPB_DESCRIPTOR_TYPE_FIXED32  = 7,
  UPB_DESCRIPTOR_TYPE_BOOL     = 8,
  UPB_DESCRIPTOR_TYPE_STRING   = 9,
  UPB_DESCRIPTOR_TYPE_GROUP    = 10,
  UPB_DESCRIPTOR_TYPE_MESSAGE  = 11,
  UPB_DESCRIPTOR_TYPE_BYTES    = 12,
  UPB_DESCRIPTOR_TYPE_UINT32   = 13,
  UPB_DESCRIPTOR_TYPE_ENUM     = 14,
  UPB_DESCRIPTOR_TYPE_SFIXED32 = 15,
  UPB_DESCRIPTOR_TYPE_SFIXED64 = 16,
  UPB_DESCRIPTOR_TYPE_SINT32   = 17,
  UPB_DESCRIPTOR_TYPE_SINT64   = 18
} upb_descriptortype_t;

/* Maximum field number allowed for FieldDefs.  This is an inherent limit of the
 * protobuf wire format. */
#define UPB_MAX_FIELDNUMBER ((1 << 29) - 1)

#ifdef __cplusplus

/* A upb_fielddef describes a single field in a message.  It is most often
 * found as a part of a upb_msgdef, but can also stand alone to represent
 * an extension.
 *
 * Its base class is upb::Def (use upb::upcast() to convert). */
class upb::FieldDef {
 public:
  typedef upb_fieldtype_t Type;
  typedef upb_label_t Label;
  typedef upb_intfmt_t IntegerFormat;
  typedef upb_descriptortype_t DescriptorType;

  /* These return true if the given value is a valid member of the enumeration. */
  static bool CheckType(int32_t val);
  static bool CheckLabel(int32_t val);
  static bool CheckDescriptorType(int32_t val);
  static bool CheckIntegerFormat(int32_t val);

  /* These convert to the given enumeration; they require that the value is
   * valid. */
  static Type ConvertType(int32_t val);
  static Label ConvertLabel(int32_t val);
  static DescriptorType ConvertDescriptorType(int32_t val);
  static IntegerFormat ConvertIntegerFormat(int32_t val);

  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<FieldDef> New();

  /* Duplicates the given field, returning NULL if memory allocation failed.
   * When a fielddef is duplicated, the subdef (if any) is made symbolic if it
   * wasn't already.  If the subdef is set but has no name (which is possible
   * since msgdefs are not required to have a name) the new fielddef's subdef
   * will be unset. */
  FieldDef* Dup(const void* owner) const;

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Functionality from upb::Def. */
  const char* full_name() const;

  bool type_is_set() const;  /* set_[descriptor_]type() has been called? */
  Type type() const;         /* Requires that type_is_set() == true. */
  Label label() const;       /* Defaults to UPB_LABEL_OPTIONAL. */
  const char* name() const;  /* NULL if uninitialized. */
  uint32_t number() const;   /* Returns 0 if uninitialized. */
  bool is_extension() const;

  /* Copies the JSON name for this field into the given buffer.  Returns the
   * actual size of the JSON name, including the NULL terminator.  If the
   * return value is 0, the JSON name is unset.  If the return value is
   * greater than len, the JSON name was truncated.  The buffer is always
   * NULL-terminated if len > 0.
   *
   * The JSON name always defaults to a camelCased version of the regular
   * name.  However if the regular name is unset, the JSON name will be unset
   * also.
   */
  size_t GetJsonName(char* buf, size_t len) const;

  /* Convenience version of the above function which copies the JSON name
   * into the given string, returning false if the name is not set. */
  template <class T>
  bool GetJsonName(T* str) {
    str->resize(GetJsonName(NULL, 0));
    GetJsonName(&(*str)[0], str->size());
    return str->size() > 0;
  }

  /* For UPB_TYPE_MESSAGE fields only where is_tag_delimited() == false,
   * indicates whether this field should have lazy parsing handlers that yield
   * the unparsed string for the submessage.
   *
   * TODO(haberman): I think we want to move this into a FieldOptions container
   * when we add support for custom options (the FieldOptions struct will
   * contain both regular FieldOptions like "lazy" *and* custom options). */
  bool lazy() const;

  /* For non-string, non-submessage fields, this indicates whether binary
   * protobufs are encoded in packed or non-packed format.
   *
   * TODO(haberman): see note above about putting options like this into a
   * FieldOptions container. */
  bool packed() const;

  /* An integer that can be used as an index into an array of fields for
   * whatever message this field belongs to.  Guaranteed to be less than
   * f->containing_type()->field_count().  May only be accessed once the def has
   * been finalized. */
  uint32_t index() const;

  /* The MessageDef to which this field belongs.
   *
   * If this field has been added to a MessageDef, that message can be retrieved
   * directly (this is always the case for frozen FieldDefs).
   *
   * If the field has not yet been added to a MessageDef, you can set the name
   * of the containing type symbolically instead.  This is mostly useful for
   * extensions, where the extension is declared separately from the message. */
  const MessageDef* containing_type() const;
  const char* containing_type_name();

  /* The OneofDef to which this field belongs, or NULL if this field is not part
   * of a oneof. */
  const OneofDef* containing_oneof() const;

  /* The field's type according to the enum in descriptor.proto.  This is not
   * the same as UPB_TYPE_*, because it distinguishes between (for example)
   * INT32 and SINT32, whereas our "type" enum does not.  This return of
   * descriptor_type() is a function of type(), integer_format(), and
   * is_tag_delimited().  Likewise set_descriptor_type() sets all three
   * appropriately. */
  DescriptorType descriptor_type() const;

  /* Convenient field type tests. */
  bool IsSubMessage() const;
  bool IsString() const;
  bool IsSequence() const;
  bool IsPrimitive() const;
  bool IsMap() const;

  /* Whether this field must be able to explicitly represent presence:
   *
   * * This is always false for repeated fields (an empty repeated field is
   *   equivalent to a repeated field with zero entries).
   *
   * * This is always true for submessages.
   *
   * * For other fields, it depends on the message (see
   *   MessageDef::SetPrimitivesHavePresence())
   */
  bool HasPresence() const;

  /* How integers are encoded.  Only meaningful for integer types.
   * Defaults to UPB_INTFMT_VARIABLE, and is reset when "type" changes. */
  IntegerFormat integer_format() const;

  /* Whether a submessage field is tag-delimited or not (if false, then
   * length-delimited).  May only be set when type() == UPB_TYPE_MESSAGE. */
  bool is_tag_delimited() const;

  /* Returns the non-string default value for this fielddef, which may either
   * be something the client set explicitly or the "default default" (0 for
   * numbers, empty for strings).  The field's type indicates the type of the
   * returned value, except for enum fields that are still mutable.
   *
   * Requires that the given function matches the field's current type. */
  int64_t default_int64() const;
  int32_t default_int32() const;
  uint64_t default_uint64() const;
  uint32_t default_uint32() const;
  bool default_bool() const;
  float default_float() const;
  double default_double() const;

  /* The resulting string is always NULL-terminated.  If non-NULL, the length
   * will be stored in *len. */
  const char *default_string(size_t* len) const;

  /* For frozen UPB_TYPE_ENUM fields, enum defaults can always be read as either
   * string or int32, and both of these methods will always return true.
   *
   * For mutable UPB_TYPE_ENUM fields, the story is a bit more complicated.
   * Enum defaults are unusual. They can be specified either as string or int32,
   * but to be valid the enum must have that value as a member.  And if no
   * default is specified, the "default default" comes from the EnumDef.
   *
   * We allow reading the default as either an int32 or a string, but only if
   * we have a meaningful value to report.  We have a meaningful value if it was
   * set explicitly, or if we could get the "default default" from the EnumDef.
   * Also if you explicitly set the name and we find the number in the EnumDef */
  bool EnumHasStringDefault() const;
  bool EnumHasInt32Default() const;

  /* Submessage and enum fields must reference a "subdef", which is the
   * upb::MessageDef or upb::EnumDef that defines their type.  Note that when
   * the FieldDef is mutable it may not have a subdef *yet*, but this function
   * still returns true to indicate that the field's type requires a subdef. */
  bool HasSubDef() const;

  /* Returns the enum or submessage def for this field, if any.  The field's
   * type must match (ie. you may only call enum_subdef() for fields where
   * type() == UPB_TYPE_ENUM).  Returns NULL if the subdef has not been set or
   * is currently set symbolically. */
  const EnumDef* enum_subdef() const;
  const MessageDef* message_subdef() const;

  /* Returns the generic subdef for this field.  Requires that HasSubDef() (ie.
   * only works for UPB_TYPE_ENUM and UPB_TYPE_MESSAGE fields). */
  const Def* subdef() const;

  /* Returns the symbolic name of the subdef.  If the subdef is currently set
   * unresolved (ie. set symbolically) returns the symbolic name.  If it has
   * been resolved to a specific subdef, returns the name from that subdef. */
  const char* subdef_name() const;

  /* Setters (non-const methods), only valid for mutable FieldDefs! ***********/

  bool set_full_name(const char* fullname, upb::Status* s);
  bool set_full_name(const std::string& fullname, upb::Status* s);

  /* This may only be called if containing_type() == NULL (ie. the field has not
   * been added to a message yet). */
  bool set_containing_type_name(const char *name, Status* status);
  bool set_containing_type_name(const std::string& name, Status* status);

  /* Defaults to false.  When we freeze, we ensure that this can only be true
   * for length-delimited message fields.  Prior to freezing this can be true or
   * false with no restrictions. */
  void set_lazy(bool lazy);

  /* Defaults to true.  Sets whether this field is encoded in packed format. */
  void set_packed(bool packed);

  /* "type" or "descriptor_type" MUST be set explicitly before the fielddef is
   * finalized.  These setters require that the enum value is valid; if the
   * value did not come directly from an enum constant, the caller should
   * validate it first with the functions above (CheckFieldType(), etc). */
  void set_type(Type type);
  void set_label(Label label);
  void set_descriptor_type(DescriptorType type);
  void set_is_extension(bool is_extension);

  /* "number" and "name" must be set before the FieldDef is added to a
   * MessageDef, and may not be set after that.
   *
   * "name" is the same as full_name()/set_full_name(), but since fielddefs
   * most often use simple, non-qualified names, we provide this accessor
   * also.  Generally only extensions will want to think of this name as
   * fully-qualified. */
  bool set_number(uint32_t number, upb::Status* s);
  bool set_name(const char* name, upb::Status* s);
  bool set_name(const std::string& name, upb::Status* s);

  /* Sets the JSON name to the given string. */
  /* TODO(haberman): implement.  Right now only default json_name (camelCase)
   * is supported. */
  bool set_json_name(const char* json_name, upb::Status* s);
  bool set_json_name(const std::string& name, upb::Status* s);

  /* Clears the JSON name. This will make it revert to its default, which is
   * a camelCased version of the regular field name. */
  void clear_json_name();

  void set_integer_format(IntegerFormat format);
  bool set_tag_delimited(bool tag_delimited, upb::Status* s);

  /* Sets default value for the field.  The call must exactly match the type
   * of the field.  Enum fields may use either setint32 or setstring to set
   * the default numerically or symbolically, respectively, but symbolic
   * defaults must be resolved before finalizing (see ResolveEnumDefault()).
   *
   * Changing the type of a field will reset its default. */
  void set_default_int64(int64_t val);
  void set_default_int32(int32_t val);
  void set_default_uint64(uint64_t val);
  void set_default_uint32(uint32_t val);
  void set_default_bool(bool val);
  void set_default_float(float val);
  void set_default_double(double val);
  bool set_default_string(const void *str, size_t len, Status *s);
  bool set_default_string(const std::string &str, Status *s);
  void set_default_cstr(const char *str, Status *s);

  /* Before a fielddef is frozen, its subdef may be set either directly (with a
   * upb::Def*) or symbolically.  Symbolic refs must be resolved before the
   * containing msgdef can be frozen (see upb_resolve() above).  upb always
   * guarantees that any def reachable from a live def will also be kept alive.
   *
   * Both methods require that upb_hassubdef(f) (so the type must be set prior
   * to calling these methods).  Returns false if this is not the case, or if
   * the given subdef is not of the correct type.  The subdef is reset if the
   * field's type is changed.  The subdef can be set to NULL to clear it. */
  bool set_subdef(const Def* subdef, Status* s);
  bool set_enum_subdef(const EnumDef* subdef, Status* s);
  bool set_message_subdef(const MessageDef* subdef, Status* s);
  bool set_subdef_name(const char* name, Status* s);
  bool set_subdef_name(const std::string &name, Status* s);

 private:
  UPB_DISALLOW_POD_OPS(FieldDef, upb::FieldDef)
};

# endif  /* defined(__cplusplus) */

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_fielddef *upb_fielddef_new(const void *owner);
upb_fielddef *upb_fielddef_dup(const upb_fielddef *f, const void *owner);

/* Include upb_refcounted methods like upb_fielddef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_fielddef, upb_fielddef_upcast2)

/* Methods from upb_def. */
const char *upb_fielddef_fullname(const upb_fielddef *f);
bool upb_fielddef_setfullname(upb_fielddef *f, const char *fullname,
                              upb_status *s);

bool upb_fielddef_typeisset(const upb_fielddef *f);
upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f);
upb_descriptortype_t upb_fielddef_descriptortype(const upb_fielddef *f);
upb_label_t upb_fielddef_label(const upb_fielddef *f);
uint32_t upb_fielddef_number(const upb_fielddef *f);
const char *upb_fielddef_name(const upb_fielddef *f);
bool upb_fielddef_isextension(const upb_fielddef *f);
bool upb_fielddef_lazy(const upb_fielddef *f);
bool upb_fielddef_packed(const upb_fielddef *f);
size_t upb_fielddef_getjsonname(const upb_fielddef *f, char *buf, size_t len);
const upb_msgdef *upb_fielddef_containingtype(const upb_fielddef *f);
const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f);
upb_msgdef *upb_fielddef_containingtype_mutable(upb_fielddef *f);
const char *upb_fielddef_containingtypename(upb_fielddef *f);
upb_intfmt_t upb_fielddef_intfmt(const upb_fielddef *f);
uint32_t upb_fielddef_index(const upb_fielddef *f);
bool upb_fielddef_istagdelim(const upb_fielddef *f);
bool upb_fielddef_issubmsg(const upb_fielddef *f);
bool upb_fielddef_isstring(const upb_fielddef *f);
bool upb_fielddef_isseq(const upb_fielddef *f);
bool upb_fielddef_isprimitive(const upb_fielddef *f);
bool upb_fielddef_ismap(const upb_fielddef *f);
bool upb_fielddef_haspresence(const upb_fielddef *f);
int64_t upb_fielddef_defaultint64(const upb_fielddef *f);
int32_t upb_fielddef_defaultint32(const upb_fielddef *f);
uint64_t upb_fielddef_defaultuint64(const upb_fielddef *f);
uint32_t upb_fielddef_defaultuint32(const upb_fielddef *f);
bool upb_fielddef_defaultbool(const upb_fielddef *f);
float upb_fielddef_defaultfloat(const upb_fielddef *f);
double upb_fielddef_defaultdouble(const upb_fielddef *f);
const char *upb_fielddef_defaultstr(const upb_fielddef *f, size_t *len);
bool upb_fielddef_enumhasdefaultint32(const upb_fielddef *f);
bool upb_fielddef_enumhasdefaultstr(const upb_fielddef *f);
bool upb_fielddef_hassubdef(const upb_fielddef *f);
const upb_def *upb_fielddef_subdef(const upb_fielddef *f);
const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f);
const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f);
const char *upb_fielddef_subdefname(const upb_fielddef *f);

void upb_fielddef_settype(upb_fielddef *f, upb_fieldtype_t type);
void upb_fielddef_setdescriptortype(upb_fielddef *f, int type);
void upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label);
bool upb_fielddef_setnumber(upb_fielddef *f, uint32_t number, upb_status *s);
bool upb_fielddef_setname(upb_fielddef *f, const char *name, upb_status *s);
bool upb_fielddef_setjsonname(upb_fielddef *f, const char *name, upb_status *s);
bool upb_fielddef_clearjsonname(upb_fielddef *f);
bool upb_fielddef_setcontainingtypename(upb_fielddef *f, const char *name,
                                        upb_status *s);
void upb_fielddef_setisextension(upb_fielddef *f, bool is_extension);
void upb_fielddef_setlazy(upb_fielddef *f, bool lazy);
void upb_fielddef_setpacked(upb_fielddef *f, bool packed);
void upb_fielddef_setintfmt(upb_fielddef *f, upb_intfmt_t fmt);
void upb_fielddef_settagdelim(upb_fielddef *f, bool tag_delim);
void upb_fielddef_setdefaultint64(upb_fielddef *f, int64_t val);
void upb_fielddef_setdefaultint32(upb_fielddef *f, int32_t val);
void upb_fielddef_setdefaultuint64(upb_fielddef *f, uint64_t val);
void upb_fielddef_setdefaultuint32(upb_fielddef *f, uint32_t val);
void upb_fielddef_setdefaultbool(upb_fielddef *f, bool val);
void upb_fielddef_setdefaultfloat(upb_fielddef *f, float val);
void upb_fielddef_setdefaultdouble(upb_fielddef *f, double val);
bool upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len,
                                upb_status *s);
void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str,
                                 upb_status *s);
bool upb_fielddef_setsubdef(upb_fielddef *f, const upb_def *subdef,
                            upb_status *s);
bool upb_fielddef_setmsgsubdef(upb_fielddef *f, const upb_msgdef *subdef,
                               upb_status *s);
bool upb_fielddef_setenumsubdef(upb_fielddef *f, const upb_enumdef *subdef,
                                upb_status *s);
bool upb_fielddef_setsubdefname(upb_fielddef *f, const char *name,
                                upb_status *s);

bool upb_fielddef_checklabel(int32_t label);
bool upb_fielddef_checktype(int32_t type);
bool upb_fielddef_checkdescriptortype(int32_t type);
bool upb_fielddef_checkintfmt(int32_t fmt);

UPB_END_EXTERN_C


/* upb::MessageDef ************************************************************/

typedef upb_inttable_iter upb_msg_field_iter;
typedef upb_strtable_iter upb_msg_oneof_iter;

#ifdef __cplusplus

/* Structure that describes a single .proto message type.
 *
 * Its base class is upb::Def (use upb::upcast() to convert). */
class upb::MessageDef {
 public:
  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<MessageDef> New();

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Functionality from upb::Def. */
  const char* full_name() const;
  bool set_full_name(const char* fullname, Status* s);
  bool set_full_name(const std::string& fullname, Status* s);

  /* Call to freeze this MessageDef.
   * WARNING: this will fail if this message has any unfrozen submessages!
   * Messages with cycles must be frozen as a batch using upb::Def::Freeze(). */
  bool Freeze(Status* s);

  /* The number of fields that belong to the MessageDef. */
  int field_count() const;

  /* The number of oneofs that belong to the MessageDef. */
  int oneof_count() const;

  /* Adds a field (upb_fielddef object) to a msgdef.  Requires that the msgdef
   * and the fielddefs are mutable.  The fielddef's name and number must be
   * set, and the message may not already contain any field with this name or
   * number, and this fielddef may not be part of another message.  In error
   * cases false is returned and the msgdef is unchanged.
   *
   * If the given field is part of a oneof, this call succeeds if and only if
   * that oneof is already part of this msgdef. (Note that adding a oneof to a
   * msgdef automatically adds all of its fields to the msgdef at the time that
   * the oneof is added, so it is usually more idiomatic to add the oneof's
   * fields first then add the oneof to the msgdef. This case is supported for
   * convenience.)
   *
   * If |f| is already part of this MessageDef, this method performs no action
   * and returns true (success). Thus, this method is idempotent. */
  bool AddField(FieldDef* f, Status* s);
  bool AddField(const reffed_ptr<FieldDef>& f, Status* s);

  /* Adds a oneof (upb_oneofdef object) to a msgdef. Requires that the msgdef,
   * oneof, and any fielddefs are mutable, that the fielddefs contained in the
   * oneof do not have any name or number conflicts with existing fields in the
   * msgdef, and that the oneof's name is unique among all oneofs in the msgdef.
   * If the oneof is added successfully, all of its fields will be added
   * directly to the msgdef as well. In error cases, false is returned and the
   * msgdef is unchanged. */
  bool AddOneof(OneofDef* o, Status* s);
  bool AddOneof(const reffed_ptr<OneofDef>& o, Status* s);

  /* Set this to false to indicate that primitive fields should not have
   * explicit presence information associated with them.  This will affect all
   * fields added to this message.  Defaults to true. */
  void SetPrimitivesHavePresence(bool have_presence);

  /* These return NULL if the field is not found. */
  FieldDef* FindFieldByNumber(uint32_t number);
  FieldDef* FindFieldByName(const char *name, size_t len);
  const FieldDef* FindFieldByNumber(uint32_t number) const;
  const FieldDef* FindFieldByName(const char* name, size_t len) const;


  FieldDef* FindFieldByName(const char *name) {
    return FindFieldByName(name, strlen(name));
  }
  const FieldDef* FindFieldByName(const char *name) const {
    return FindFieldByName(name, strlen(name));
  }

  template <class T>
  FieldDef* FindFieldByName(const T& str) {
    return FindFieldByName(str.c_str(), str.size());
  }
  template <class T>
  const FieldDef* FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  OneofDef* FindOneofByName(const char* name, size_t len);
  const OneofDef* FindOneofByName(const char* name, size_t len) const;

  OneofDef* FindOneofByName(const char* name) {
    return FindOneofByName(name, strlen(name));
  }
  const OneofDef* FindOneofByName(const char* name) const {
    return FindOneofByName(name, strlen(name));
  }

  template<class T>
  OneofDef* FindOneofByName(const T& str) {
    return FindOneofByName(str.c_str(), str.size());
  }
  template<class T>
  const OneofDef* FindOneofByName(const T& str) const {
    return FindOneofByName(str.c_str(), str.size());
  }

  /* Returns a new msgdef that is a copy of the given msgdef (and a copy of all
   * the fields) but with any references to submessages broken and replaced
   * with just the name of the submessage.  Returns NULL if memory allocation
   * failed.
   *
   * TODO(haberman): which is more useful, keeping fields resolved or
   * unresolving them?  If there's no obvious answer, Should this functionality
   * just be moved into symtab.c? */
  MessageDef* Dup(const void* owner) const;

  /* Is this message a map entry? */
  void setmapentry(bool map_entry);
  bool mapentry() const;

  /* Iteration over fields.  The order is undefined. */
  class field_iterator
      : public std::iterator<std::forward_iterator_tag, FieldDef*> {
   public:
    explicit field_iterator(MessageDef* md);
    static field_iterator end(MessageDef* md);

    void operator++();
    FieldDef* operator*() const;
    bool operator!=(const field_iterator& other) const;
    bool operator==(const field_iterator& other) const;

   private:
    upb_msg_field_iter iter_;
  };

  class const_field_iterator
      : public std::iterator<std::forward_iterator_tag, const FieldDef*> {
   public:
    explicit const_field_iterator(const MessageDef* md);
    static const_field_iterator end(const MessageDef* md);

    void operator++();
    const FieldDef* operator*() const;
    bool operator!=(const const_field_iterator& other) const;
    bool operator==(const const_field_iterator& other) const;

   private:
    upb_msg_field_iter iter_;
  };

  /* Iteration over oneofs. The order is undefined. */
  class oneof_iterator
      : public std::iterator<std::forward_iterator_tag, FieldDef*> {
   public:
    explicit oneof_iterator(MessageDef* md);
    static oneof_iterator end(MessageDef* md);

    void operator++();
    OneofDef* operator*() const;
    bool operator!=(const oneof_iterator& other) const;
    bool operator==(const oneof_iterator& other) const;

   private:
    upb_msg_oneof_iter iter_;
  };

  class const_oneof_iterator
      : public std::iterator<std::forward_iterator_tag, const FieldDef*> {
   public:
    explicit const_oneof_iterator(const MessageDef* md);
    static const_oneof_iterator end(const MessageDef* md);

    void operator++();
    const OneofDef* operator*() const;
    bool operator!=(const const_oneof_iterator& other) const;
    bool operator==(const const_oneof_iterator& other) const;

   private:
    upb_msg_oneof_iter iter_;
  };

  class FieldAccessor {
   public:
    explicit FieldAccessor(MessageDef* msg) : msg_(msg) {}
    field_iterator begin() { return msg_->field_begin(); }
    field_iterator end() { return msg_->field_end(); }
   private:
    MessageDef* msg_;
  };

  class ConstFieldAccessor {
   public:
    explicit ConstFieldAccessor(const MessageDef* msg) : msg_(msg) {}
    const_field_iterator begin() { return msg_->field_begin(); }
    const_field_iterator end() { return msg_->field_end(); }
   private:
    const MessageDef* msg_;
  };

  class OneofAccessor {
   public:
    explicit OneofAccessor(MessageDef* msg) : msg_(msg) {}
    oneof_iterator begin() { return msg_->oneof_begin(); }
    oneof_iterator end() { return msg_->oneof_end(); }
   private:
    MessageDef* msg_;
  };

  class ConstOneofAccessor {
   public:
    explicit ConstOneofAccessor(const MessageDef* msg) : msg_(msg) {}
    const_oneof_iterator begin() { return msg_->oneof_begin(); }
    const_oneof_iterator end() { return msg_->oneof_end(); }
   private:
    const MessageDef* msg_;
  };

  field_iterator field_begin();
  field_iterator field_end();
  const_field_iterator field_begin() const;
  const_field_iterator field_end() const;

  oneof_iterator oneof_begin();
  oneof_iterator oneof_end();
  const_oneof_iterator oneof_begin() const;
  const_oneof_iterator oneof_end() const;

  FieldAccessor fields() { return FieldAccessor(this); }
  ConstFieldAccessor fields() const { return ConstFieldAccessor(this); }
  OneofAccessor oneofs() { return OneofAccessor(this); }
  ConstOneofAccessor oneofs() const { return ConstOneofAccessor(this); }

 private:
  UPB_DISALLOW_POD_OPS(MessageDef, upb::MessageDef)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Returns NULL if memory allocation failed. */
upb_msgdef *upb_msgdef_new(const void *owner);

/* Include upb_refcounted methods like upb_msgdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_msgdef, upb_msgdef_upcast2)

bool upb_msgdef_freeze(upb_msgdef *m, upb_status *status);

const char *upb_msgdef_fullname(const upb_msgdef *m);
bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname, upb_status *s);

upb_msgdef *upb_msgdef_dup(const upb_msgdef *m, const void *owner);
bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f, const void *ref_donor,
                         upb_status *s);
bool upb_msgdef_addoneof(upb_msgdef *m, upb_oneofdef *o, const void *ref_donor,
                         upb_status *s);
void upb_msgdef_setprimitiveshavepresence(upb_msgdef *m, bool have_presence);

/* Field lookup in a couple of different variations:
 *   - itof = int to field
 *   - ntof = name to field
 *   - ntofz = name to field, null-terminated string. */
const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i);
const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name,
                                    size_t len);
int upb_msgdef_numfields(const upb_msgdef *m);

UPB_INLINE const upb_fielddef *upb_msgdef_ntofz(const upb_msgdef *m,
                                                const char *name) {
  return upb_msgdef_ntof(m, name, strlen(name));
}

UPB_INLINE upb_fielddef *upb_msgdef_itof_mutable(upb_msgdef *m, uint32_t i) {
  return (upb_fielddef*)upb_msgdef_itof(m, i);
}

UPB_INLINE upb_fielddef *upb_msgdef_ntof_mutable(upb_msgdef *m,
                                                 const char *name, size_t len) {
  return (upb_fielddef *)upb_msgdef_ntof(m, name, len);
}

/* Oneof lookup:
 *   - ntoo = name to oneof
 *   - ntooz = name to oneof, null-terminated string. */
const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len);
int upb_msgdef_numoneofs(const upb_msgdef *m);

UPB_INLINE const upb_oneofdef *upb_msgdef_ntooz(const upb_msgdef *m,
                                               const char *name) {
  return upb_msgdef_ntoo(m, name, strlen(name));
}

UPB_INLINE upb_oneofdef *upb_msgdef_ntoo_mutable(upb_msgdef *m,
                                                 const char *name, size_t len) {
  return (upb_oneofdef *)upb_msgdef_ntoo(m, name, len);
}

void upb_msgdef_setmapentry(upb_msgdef *m, bool map_entry);
bool upb_msgdef_mapentry(const upb_msgdef *m);

/* Well-known field tag numbers for map-entry messages. */
#define UPB_MAPENTRY_KEY   1
#define UPB_MAPENTRY_VALUE 2

const upb_oneofdef *upb_msgdef_findoneof(const upb_msgdef *m,
                                          const char *name);
int upb_msgdef_numoneofs(const upb_msgdef *m);

/* upb_msg_field_iter i;
 * for(upb_msg_field_begin(&i, m);
 *     !upb_msg_field_done(&i);
 *     upb_msg_field_next(&i)) {
 *   upb_fielddef *f = upb_msg_iter_field(&i);
 *   // ...
 * }
 *
 * For C we don't have separate iterators for const and non-const.
 * It is the caller's responsibility to cast the upb_fielddef* to
 * const if the upb_msgdef* is const. */
void upb_msg_field_begin(upb_msg_field_iter *iter, const upb_msgdef *m);
void upb_msg_field_next(upb_msg_field_iter *iter);
bool upb_msg_field_done(const upb_msg_field_iter *iter);
upb_fielddef *upb_msg_iter_field(const upb_msg_field_iter *iter);
void upb_msg_field_iter_setdone(upb_msg_field_iter *iter);

/* Similar to above, we also support iterating through the oneofs in a
 * msgdef. */
void upb_msg_oneof_begin(upb_msg_oneof_iter *iter, const upb_msgdef *m);
void upb_msg_oneof_next(upb_msg_oneof_iter *iter);
bool upb_msg_oneof_done(const upb_msg_oneof_iter *iter);
upb_oneofdef *upb_msg_iter_oneof(const upb_msg_oneof_iter *iter);
void upb_msg_oneof_iter_setdone(upb_msg_oneof_iter *iter);

UPB_END_EXTERN_C


/* upb::EnumDef ***************************************************************/

typedef upb_strtable_iter upb_enum_iter;

#ifdef __cplusplus

/* Class that represents an enum.  Its base class is upb::Def (convert with
 * upb::upcast()). */
class upb::EnumDef {
 public:
  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<EnumDef> New();

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Functionality from upb::Def. */
  const char* full_name() const;
  bool set_full_name(const char* fullname, Status* s);
  bool set_full_name(const std::string& fullname, Status* s);

  /* Call to freeze this EnumDef. */
  bool Freeze(Status* s);

  /* The value that is used as the default when no field default is specified.
   * If not set explicitly, the first value that was added will be used.
   * The default value must be a member of the enum.
   * Requires that value_count() > 0. */
  int32_t default_value() const;

  /* Sets the default value.  If this value is not valid, returns false and an
   * error message in status. */
  bool set_default_value(int32_t val, Status* status);

  /* Returns the number of values currently defined in the enum.  Note that
   * multiple names can refer to the same number, so this may be greater than
   * the total number of unique numbers. */
  int value_count() const;

  /* Adds a single name/number pair to the enum.  Fails if this name has
   * already been used by another value. */
  bool AddValue(const char* name, int32_t num, Status* status);
  bool AddValue(const std::string& name, int32_t num, Status* status);

  /* Lookups from name to integer, returning true if found. */
  bool FindValueByName(const char* name, int32_t* num) const;

  /* Finds the name corresponding to the given number, or NULL if none was
   * found.  If more than one name corresponds to this number, returns the
   * first one that was added. */
  const char* FindValueByNumber(int32_t num) const;

  /* Returns a new EnumDef with all the same values.  The new EnumDef will be
   * owned by the given owner. */
  EnumDef* Dup(const void* owner) const;

  /* Iteration over name/value pairs.  The order is undefined.
   * Adding an enum val invalidates any iterators.
   *
   * TODO: make compatible with range-for, with elements as pairs? */
  class Iterator {
   public:
    explicit Iterator(const EnumDef*);

    int32_t number();
    const char *name();
    bool Done();
    void Next();

   private:
    upb_enum_iter iter_;
  };

 private:
  UPB_DISALLOW_POD_OPS(EnumDef, upb::EnumDef)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_enumdef *upb_enumdef_new(const void *owner);
upb_enumdef *upb_enumdef_dup(const upb_enumdef *e, const void *owner);

/* Include upb_refcounted methods like upb_enumdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_enumdef, upb_enumdef_upcast2)

bool upb_enumdef_freeze(upb_enumdef *e, upb_status *status);

/* From upb_def. */
const char *upb_enumdef_fullname(const upb_enumdef *e);
bool upb_enumdef_setfullname(upb_enumdef *e, const char *fullname,
                             upb_status *s);

int32_t upb_enumdef_default(const upb_enumdef *e);
bool upb_enumdef_setdefault(upb_enumdef *e, int32_t val, upb_status *s);
int upb_enumdef_numvals(const upb_enumdef *e);
bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num,
                        upb_status *status);

/* Enum lookups:
 * - ntoi:  look up a name with specified length.
 * - ntoiz: look up a name provided as a null-terminated string.
 * - iton:  look up an integer, returning the name as a null-terminated
 *          string. */
bool upb_enumdef_ntoi(const upb_enumdef *e, const char *name, size_t len,
                      int32_t *num);
UPB_INLINE bool upb_enumdef_ntoiz(const upb_enumdef *e,
                                  const char *name, int32_t *num) {
  return upb_enumdef_ntoi(e, name, strlen(name), num);
}
const char *upb_enumdef_iton(const upb_enumdef *e, int32_t num);

/*  upb_enum_iter i;
 *  for(upb_enum_begin(&i, e); !upb_enum_done(&i); upb_enum_next(&i)) {
 *    // ...
 *  }
 */
void upb_enum_begin(upb_enum_iter *iter, const upb_enumdef *e);
void upb_enum_next(upb_enum_iter *iter);
bool upb_enum_done(upb_enum_iter *iter);
const char *upb_enum_iter_name(upb_enum_iter *iter);
int32_t upb_enum_iter_number(upb_enum_iter *iter);

UPB_END_EXTERN_C

/* upb::OneofDef **************************************************************/

typedef upb_inttable_iter upb_oneof_iter;

#ifdef __cplusplus

/* Class that represents a oneof.  Its base class is upb::Def (convert with
 * upb::upcast()). */
class upb::OneofDef {
 public:
  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<OneofDef> New();

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Functionality from upb::Def. */
  const char* full_name() const;

  /* Returns the MessageDef that owns this OneofDef. */
  const MessageDef* containing_type() const;

  /* Returns the name of this oneof. This is the name used to look up the oneof
   * by name once added to a message def. */
  const char* name() const;
  bool set_name(const char* name, Status* s);

  /* Returns the number of fields currently defined in the oneof. */
  int field_count() const;

  /* Adds a field to the oneof. The field must not have been added to any other
   * oneof or msgdef. If the oneof is not yet part of a msgdef, then when the
   * oneof is eventually added to a msgdef, all fields added to the oneof will
   * also be added to the msgdef at that time. If the oneof is already part of a
   * msgdef, the field must either be a part of that msgdef already, or must not
   * be a part of any msgdef; in the latter case, the field is added to the
   * msgdef as a part of this operation.
   *
   * The field may only have an OPTIONAL label, never REQUIRED or REPEATED.
   *
   * If |f| is already part of this MessageDef, this method performs no action
   * and returns true (success). Thus, this method is idempotent. */
  bool AddField(FieldDef* field, Status* s);
  bool AddField(const reffed_ptr<FieldDef>& field, Status* s);

  /* Looks up by name. */
  const FieldDef* FindFieldByName(const char* name, size_t len) const;
  FieldDef* FindFieldByName(const char* name, size_t len);
  const FieldDef* FindFieldByName(const char* name) const {
    return FindFieldByName(name, strlen(name));
  }
  FieldDef* FindFieldByName(const char* name) {
    return FindFieldByName(name, strlen(name));
  }

  template <class T>
  FieldDef* FindFieldByName(const T& str) {
    return FindFieldByName(str.c_str(), str.size());
  }
  template <class T>
  const FieldDef* FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  /* Looks up by tag number. */
  const FieldDef* FindFieldByNumber(uint32_t num) const;

  /* Returns a new OneofDef with all the same fields. The OneofDef will be owned
   * by the given owner. */
  OneofDef* Dup(const void* owner) const;

  /* Iteration over fields.  The order is undefined. */
  class iterator : public std::iterator<std::forward_iterator_tag, FieldDef*> {
   public:
    explicit iterator(OneofDef* md);
    static iterator end(OneofDef* md);

    void operator++();
    FieldDef* operator*() const;
    bool operator!=(const iterator& other) const;
    bool operator==(const iterator& other) const;

   private:
    upb_oneof_iter iter_;
  };

  class const_iterator
      : public std::iterator<std::forward_iterator_tag, const FieldDef*> {
   public:
    explicit const_iterator(const OneofDef* md);
    static const_iterator end(const OneofDef* md);

    void operator++();
    const FieldDef* operator*() const;
    bool operator!=(const const_iterator& other) const;
    bool operator==(const const_iterator& other) const;

   private:
    upb_oneof_iter iter_;
  };

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

 private:
  UPB_DISALLOW_POD_OPS(OneofDef, upb::OneofDef)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_oneofdef *upb_oneofdef_new(const void *owner);
upb_oneofdef *upb_oneofdef_dup(const upb_oneofdef *o, const void *owner);

/* Include upb_refcounted methods like upb_oneofdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_oneofdef, upb_oneofdef_upcast2)

const char *upb_oneofdef_name(const upb_oneofdef *o);
bool upb_oneofdef_setname(upb_oneofdef *o, const char *name, upb_status *s);

const upb_msgdef *upb_oneofdef_containingtype(const upb_oneofdef *o);
int upb_oneofdef_numfields(const upb_oneofdef *o);
bool upb_oneofdef_addfield(upb_oneofdef *o, upb_fielddef *f,
                           const void *ref_donor,
                           upb_status *s);

/* Oneof lookups:
 * - ntof:  look up a field by name.
 * - ntofz: look up a field by name (as a null-terminated string).
 * - itof:  look up a field by number. */
const upb_fielddef *upb_oneofdef_ntof(const upb_oneofdef *o,
                                      const char *name, size_t length);
UPB_INLINE const upb_fielddef *upb_oneofdef_ntofz(const upb_oneofdef *o,
                                                  const char *name) {
  return upb_oneofdef_ntof(o, name, strlen(name));
}
const upb_fielddef *upb_oneofdef_itof(const upb_oneofdef *o, uint32_t num);

/*  upb_oneof_iter i;
 *  for(upb_oneof_begin(&i, e); !upb_oneof_done(&i); upb_oneof_next(&i)) {
 *    // ...
 *  }
 */
void upb_oneof_begin(upb_oneof_iter *iter, const upb_oneofdef *o);
void upb_oneof_next(upb_oneof_iter *iter);
bool upb_oneof_done(upb_oneof_iter *iter);
upb_fielddef *upb_oneof_iter_field(const upb_oneof_iter *iter);
void upb_oneof_iter_setdone(upb_oneof_iter *iter);

UPB_END_EXTERN_C

#ifdef __cplusplus

UPB_INLINE const char* upb_safecstr(const std::string& str) {
  assert(str.size() == std::strlen(str.c_str()));
  return str.c_str();
}

/* Inline C++ wrappers. */
namespace upb {

inline Def* Def::Dup(const void* owner) const {
  return upb_def_dup(this, owner);
}
inline Def::Type Def::def_type() const { return upb_def_type(this); }
inline const char* Def::full_name() const { return upb_def_fullname(this); }
inline bool Def::set_full_name(const char* fullname, Status* s) {
  return upb_def_setfullname(this, fullname, s);
}
inline bool Def::set_full_name(const std::string& fullname, Status* s) {
  return upb_def_setfullname(this, upb_safecstr(fullname), s);
}
inline bool Def::Freeze(Def* const* defs, int n, Status* status) {
  return upb_def_freeze(defs, n, status);
}
inline bool Def::Freeze(const std::vector<Def*>& defs, Status* status) {
  return upb_def_freeze((Def* const*)&defs[0], defs.size(), status);
}

inline bool FieldDef::CheckType(int32_t val) {
  return upb_fielddef_checktype(val);
}
inline bool FieldDef::CheckLabel(int32_t val) {
  return upb_fielddef_checklabel(val);
}
inline bool FieldDef::CheckDescriptorType(int32_t val) {
  return upb_fielddef_checkdescriptortype(val);
}
inline bool FieldDef::CheckIntegerFormat(int32_t val) {
  return upb_fielddef_checkintfmt(val);
}
inline FieldDef::Type FieldDef::ConvertType(int32_t val) {
  assert(CheckType(val));
  return static_cast<FieldDef::Type>(val);
}
inline FieldDef::Label FieldDef::ConvertLabel(int32_t val) {
  assert(CheckLabel(val));
  return static_cast<FieldDef::Label>(val);
}
inline FieldDef::DescriptorType FieldDef::ConvertDescriptorType(int32_t val) {
  assert(CheckDescriptorType(val));
  return static_cast<FieldDef::DescriptorType>(val);
}
inline FieldDef::IntegerFormat FieldDef::ConvertIntegerFormat(int32_t val) {
  assert(CheckIntegerFormat(val));
  return static_cast<FieldDef::IntegerFormat>(val);
}

inline reffed_ptr<FieldDef> FieldDef::New() {
  upb_fielddef *f = upb_fielddef_new(&f);
  return reffed_ptr<FieldDef>(f, &f);
}
inline FieldDef* FieldDef::Dup(const void* owner) const {
  return upb_fielddef_dup(this, owner);
}
inline const char* FieldDef::full_name() const {
  return upb_fielddef_fullname(this);
}
inline bool FieldDef::set_full_name(const char* fullname, Status* s) {
  return upb_fielddef_setfullname(this, fullname, s);
}
inline bool FieldDef::set_full_name(const std::string& fullname, Status* s) {
  return upb_fielddef_setfullname(this, upb_safecstr(fullname), s);
}
inline bool FieldDef::type_is_set() const {
  return upb_fielddef_typeisset(this);
}
inline FieldDef::Type FieldDef::type() const { return upb_fielddef_type(this); }
inline FieldDef::DescriptorType FieldDef::descriptor_type() const {
  return upb_fielddef_descriptortype(this);
}
inline FieldDef::Label FieldDef::label() const {
  return upb_fielddef_label(this);
}
inline uint32_t FieldDef::number() const { return upb_fielddef_number(this); }
inline const char* FieldDef::name() const { return upb_fielddef_name(this); }
inline bool FieldDef::is_extension() const {
  return upb_fielddef_isextension(this);
}
inline size_t FieldDef::GetJsonName(char* buf, size_t len) const {
  return upb_fielddef_getjsonname(this, buf, len);
}
inline bool FieldDef::lazy() const {
  return upb_fielddef_lazy(this);
}
inline void FieldDef::set_lazy(bool lazy) {
  upb_fielddef_setlazy(this, lazy);
}
inline bool FieldDef::packed() const {
  return upb_fielddef_packed(this);
}
inline uint32_t FieldDef::index() const {
  return upb_fielddef_index(this);
}
inline void FieldDef::set_packed(bool packed) {
  upb_fielddef_setpacked(this, packed);
}
inline const MessageDef* FieldDef::containing_type() const {
  return upb_fielddef_containingtype(this);
}
inline const OneofDef* FieldDef::containing_oneof() const {
  return upb_fielddef_containingoneof(this);
}
inline const char* FieldDef::containing_type_name() {
  return upb_fielddef_containingtypename(this);
}
inline bool FieldDef::set_number(uint32_t number, Status* s) {
  return upb_fielddef_setnumber(this, number, s);
}
inline bool FieldDef::set_name(const char *name, Status* s) {
  return upb_fielddef_setname(this, name, s);
}
inline bool FieldDef::set_name(const std::string& name, Status* s) {
  return upb_fielddef_setname(this, upb_safecstr(name), s);
}
inline bool FieldDef::set_json_name(const char *name, Status* s) {
  return upb_fielddef_setjsonname(this, name, s);
}
inline bool FieldDef::set_json_name(const std::string& name, Status* s) {
  return upb_fielddef_setjsonname(this, upb_safecstr(name), s);
}
inline void FieldDef::clear_json_name() {
  upb_fielddef_clearjsonname(this);
}
inline bool FieldDef::set_containing_type_name(const char *name, Status* s) {
  return upb_fielddef_setcontainingtypename(this, name, s);
}
inline bool FieldDef::set_containing_type_name(const std::string &name,
                                               Status *s) {
  return upb_fielddef_setcontainingtypename(this, upb_safecstr(name), s);
}
inline void FieldDef::set_type(upb_fieldtype_t type) {
  upb_fielddef_settype(this, type);
}
inline void FieldDef::set_is_extension(bool is_extension) {
  upb_fielddef_setisextension(this, is_extension);
}
inline void FieldDef::set_descriptor_type(FieldDef::DescriptorType type) {
  upb_fielddef_setdescriptortype(this, type);
}
inline void FieldDef::set_label(upb_label_t label) {
  upb_fielddef_setlabel(this, label);
}
inline bool FieldDef::IsSubMessage() const {
  return upb_fielddef_issubmsg(this);
}
inline bool FieldDef::IsString() const { return upb_fielddef_isstring(this); }
inline bool FieldDef::IsSequence() const { return upb_fielddef_isseq(this); }
inline bool FieldDef::IsMap() const { return upb_fielddef_ismap(this); }
inline int64_t FieldDef::default_int64() const {
  return upb_fielddef_defaultint64(this);
}
inline int32_t FieldDef::default_int32() const {
  return upb_fielddef_defaultint32(this);
}
inline uint64_t FieldDef::default_uint64() const {
  return upb_fielddef_defaultuint64(this);
}
inline uint32_t FieldDef::default_uint32() const {
  return upb_fielddef_defaultuint32(this);
}
inline bool FieldDef::default_bool() const {
  return upb_fielddef_defaultbool(this);
}
inline float FieldDef::default_float() const {
  return upb_fielddef_defaultfloat(this);
}
inline double FieldDef::default_double() const {
  return upb_fielddef_defaultdouble(this);
}
inline const char* FieldDef::default_string(size_t* len) const {
  return upb_fielddef_defaultstr(this, len);
}
inline void FieldDef::set_default_int64(int64_t value) {
  upb_fielddef_setdefaultint64(this, value);
}
inline void FieldDef::set_default_int32(int32_t value) {
  upb_fielddef_setdefaultint32(this, value);
}
inline void FieldDef::set_default_uint64(uint64_t value) {
  upb_fielddef_setdefaultuint64(this, value);
}
inline void FieldDef::set_default_uint32(uint32_t value) {
  upb_fielddef_setdefaultuint32(this, value);
}
inline void FieldDef::set_default_bool(bool value) {
  upb_fielddef_setdefaultbool(this, value);
}
inline void FieldDef::set_default_float(float value) {
  upb_fielddef_setdefaultfloat(this, value);
}
inline void FieldDef::set_default_double(double value) {
  upb_fielddef_setdefaultdouble(this, value);
}
inline bool FieldDef::set_default_string(const void *str, size_t len,
                                         Status *s) {
  return upb_fielddef_setdefaultstr(this, str, len, s);
}
inline bool FieldDef::set_default_string(const std::string& str, Status* s) {
  return upb_fielddef_setdefaultstr(this, str.c_str(), str.size(), s);
}
inline void FieldDef::set_default_cstr(const char* str, Status* s) {
  return upb_fielddef_setdefaultcstr(this, str, s);
}
inline bool FieldDef::HasSubDef() const { return upb_fielddef_hassubdef(this); }
inline const Def* FieldDef::subdef() const { return upb_fielddef_subdef(this); }
inline const MessageDef *FieldDef::message_subdef() const {
  return upb_fielddef_msgsubdef(this);
}
inline const EnumDef *FieldDef::enum_subdef() const {
  return upb_fielddef_enumsubdef(this);
}
inline const char* FieldDef::subdef_name() const {
  return upb_fielddef_subdefname(this);
}
inline bool FieldDef::set_subdef(const Def* subdef, Status* s) {
  return upb_fielddef_setsubdef(this, subdef, s);
}
inline bool FieldDef::set_enum_subdef(const EnumDef* subdef, Status* s) {
  return upb_fielddef_setenumsubdef(this, subdef, s);
}
inline bool FieldDef::set_message_subdef(const MessageDef* subdef, Status* s) {
  return upb_fielddef_setmsgsubdef(this, subdef, s);
}
inline bool FieldDef::set_subdef_name(const char* name, Status* s) {
  return upb_fielddef_setsubdefname(this, name, s);
}
inline bool FieldDef::set_subdef_name(const std::string& name, Status* s) {
  return upb_fielddef_setsubdefname(this, upb_safecstr(name), s);
}

inline reffed_ptr<MessageDef> MessageDef::New() {
  upb_msgdef *m = upb_msgdef_new(&m);
  return reffed_ptr<MessageDef>(m, &m);
}
inline const char *MessageDef::full_name() const {
  return upb_msgdef_fullname(this);
}
inline bool MessageDef::set_full_name(const char* fullname, Status* s) {
  return upb_msgdef_setfullname(this, fullname, s);
}
inline bool MessageDef::set_full_name(const std::string& fullname, Status* s) {
  return upb_msgdef_setfullname(this, upb_safecstr(fullname), s);
}
inline bool MessageDef::Freeze(Status* status) {
  return upb_msgdef_freeze(this, status);
}
inline int MessageDef::field_count() const {
  return upb_msgdef_numfields(this);
}
inline int MessageDef::oneof_count() const {
  return upb_msgdef_numoneofs(this);
}
inline bool MessageDef::AddField(upb_fielddef* f, Status* s) {
  return upb_msgdef_addfield(this, f, NULL, s);
}
inline bool MessageDef::AddField(const reffed_ptr<FieldDef>& f, Status* s) {
  return upb_msgdef_addfield(this, f.get(), NULL, s);
}
inline bool MessageDef::AddOneof(upb_oneofdef* o, Status* s) {
  return upb_msgdef_addoneof(this, o, NULL, s);
}
inline bool MessageDef::AddOneof(const reffed_ptr<OneofDef>& o, Status* s) {
  return upb_msgdef_addoneof(this, o.get(), NULL, s);
}
inline FieldDef* MessageDef::FindFieldByNumber(uint32_t number) {
  return upb_msgdef_itof_mutable(this, number);
}
inline FieldDef* MessageDef::FindFieldByName(const char* name, size_t len) {
  return upb_msgdef_ntof_mutable(this, name, len);
}
inline const FieldDef* MessageDef::FindFieldByNumber(uint32_t number) const {
  return upb_msgdef_itof(this, number);
}
inline const FieldDef *MessageDef::FindFieldByName(const char *name,
                                                   size_t len) const {
  return upb_msgdef_ntof(this, name, len);
}
inline OneofDef* MessageDef::FindOneofByName(const char* name, size_t len) {
  return upb_msgdef_ntoo_mutable(this, name, len);
}
inline const OneofDef* MessageDef::FindOneofByName(const char* name,
                                                   size_t len) const {
  return upb_msgdef_ntoo(this, name, len);
}
inline MessageDef* MessageDef::Dup(const void *owner) const {
  return upb_msgdef_dup(this, owner);
}
inline void MessageDef::setmapentry(bool map_entry) {
  upb_msgdef_setmapentry(this, map_entry);
}
inline bool MessageDef::mapentry() const {
  return upb_msgdef_mapentry(this);
}
inline MessageDef::field_iterator MessageDef::field_begin() {
  return field_iterator(this);
}
inline MessageDef::field_iterator MessageDef::field_end() {
  return field_iterator::end(this);
}
inline MessageDef::const_field_iterator MessageDef::field_begin() const {
  return const_field_iterator(this);
}
inline MessageDef::const_field_iterator MessageDef::field_end() const {
  return const_field_iterator::end(this);
}

inline MessageDef::oneof_iterator MessageDef::oneof_begin() {
  return oneof_iterator(this);
}
inline MessageDef::oneof_iterator MessageDef::oneof_end() {
  return oneof_iterator::end(this);
}
inline MessageDef::const_oneof_iterator MessageDef::oneof_begin() const {
  return const_oneof_iterator(this);
}
inline MessageDef::const_oneof_iterator MessageDef::oneof_end() const {
  return const_oneof_iterator::end(this);
}

inline MessageDef::field_iterator::field_iterator(MessageDef* md) {
  upb_msg_field_begin(&iter_, md);
}
inline MessageDef::field_iterator MessageDef::field_iterator::end(
    MessageDef* md) {
  MessageDef::field_iterator iter(md);
  upb_msg_field_iter_setdone(&iter.iter_);
  return iter;
}
inline FieldDef* MessageDef::field_iterator::operator*() const {
  return upb_msg_iter_field(&iter_);
}
inline void MessageDef::field_iterator::operator++() {
  return upb_msg_field_next(&iter_);
}
inline bool MessageDef::field_iterator::operator==(
    const field_iterator &other) const {
  return upb_inttable_iter_isequal(&iter_, &other.iter_);
}
inline bool MessageDef::field_iterator::operator!=(
    const field_iterator &other) const {
  return !(*this == other);
}

inline MessageDef::const_field_iterator::const_field_iterator(
    const MessageDef* md) {
  upb_msg_field_begin(&iter_, md);
}
inline MessageDef::const_field_iterator MessageDef::const_field_iterator::end(
    const MessageDef *md) {
  MessageDef::const_field_iterator iter(md);
  upb_msg_field_iter_setdone(&iter.iter_);
  return iter;
}
inline const FieldDef* MessageDef::const_field_iterator::operator*() const {
  return upb_msg_iter_field(&iter_);
}
inline void MessageDef::const_field_iterator::operator++() {
  return upb_msg_field_next(&iter_);
}
inline bool MessageDef::const_field_iterator::operator==(
    const const_field_iterator &other) const {
  return upb_inttable_iter_isequal(&iter_, &other.iter_);
}
inline bool MessageDef::const_field_iterator::operator!=(
    const const_field_iterator &other) const {
  return !(*this == other);
}

inline MessageDef::oneof_iterator::oneof_iterator(MessageDef* md) {
  upb_msg_oneof_begin(&iter_, md);
}
inline MessageDef::oneof_iterator MessageDef::oneof_iterator::end(
    MessageDef* md) {
  MessageDef::oneof_iterator iter(md);
  upb_msg_oneof_iter_setdone(&iter.iter_);
  return iter;
}
inline OneofDef* MessageDef::oneof_iterator::operator*() const {
  return upb_msg_iter_oneof(&iter_);
}
inline void MessageDef::oneof_iterator::operator++() {
  return upb_msg_oneof_next(&iter_);
}
inline bool MessageDef::oneof_iterator::operator==(
    const oneof_iterator &other) const {
  return upb_strtable_iter_isequal(&iter_, &other.iter_);
}
inline bool MessageDef::oneof_iterator::operator!=(
    const oneof_iterator &other) const {
  return !(*this == other);
}

inline MessageDef::const_oneof_iterator::const_oneof_iterator(
    const MessageDef* md) {
  upb_msg_oneof_begin(&iter_, md);
}
inline MessageDef::const_oneof_iterator MessageDef::const_oneof_iterator::end(
    const MessageDef *md) {
  MessageDef::const_oneof_iterator iter(md);
  upb_msg_oneof_iter_setdone(&iter.iter_);
  return iter;
}
inline const OneofDef* MessageDef::const_oneof_iterator::operator*() const {
  return upb_msg_iter_oneof(&iter_);
}
inline void MessageDef::const_oneof_iterator::operator++() {
  return upb_msg_oneof_next(&iter_);
}
inline bool MessageDef::const_oneof_iterator::operator==(
    const const_oneof_iterator &other) const {
  return upb_strtable_iter_isequal(&iter_, &other.iter_);
}
inline bool MessageDef::const_oneof_iterator::operator!=(
    const const_oneof_iterator &other) const {
  return !(*this == other);
}

inline reffed_ptr<EnumDef> EnumDef::New() {
  upb_enumdef *e = upb_enumdef_new(&e);
  return reffed_ptr<EnumDef>(e, &e);
}
inline const char* EnumDef::full_name() const {
  return upb_enumdef_fullname(this);
}
inline bool EnumDef::set_full_name(const char* fullname, Status* s) {
  return upb_enumdef_setfullname(this, fullname, s);
}
inline bool EnumDef::set_full_name(const std::string& fullname, Status* s) {
  return upb_enumdef_setfullname(this, upb_safecstr(fullname), s);
}
inline bool EnumDef::Freeze(Status* status) {
  return upb_enumdef_freeze(this, status);
}
inline int32_t EnumDef::default_value() const {
  return upb_enumdef_default(this);
}
inline bool EnumDef::set_default_value(int32_t val, Status* status) {
  return upb_enumdef_setdefault(this, val, status);
}
inline int EnumDef::value_count() const { return upb_enumdef_numvals(this); }
inline bool EnumDef::AddValue(const char* name, int32_t num, Status* status) {
  return upb_enumdef_addval(this, name, num, status);
}
inline bool EnumDef::AddValue(const std::string& name, int32_t num,
                              Status* status) {
  return upb_enumdef_addval(this, upb_safecstr(name), num, status);
}
inline bool EnumDef::FindValueByName(const char* name, int32_t *num) const {
  return upb_enumdef_ntoiz(this, name, num);
}
inline const char* EnumDef::FindValueByNumber(int32_t num) const {
  return upb_enumdef_iton(this, num);
}
inline EnumDef* EnumDef::Dup(const void* owner) const {
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
inline bool EnumDef::Iterator::Done() { return upb_enum_done(&iter_); }
inline void EnumDef::Iterator::Next() { return upb_enum_next(&iter_); }

inline reffed_ptr<OneofDef> OneofDef::New() {
  upb_oneofdef *o = upb_oneofdef_new(&o);
  return reffed_ptr<OneofDef>(o, &o);
}
inline const char* OneofDef::full_name() const {
  return upb_oneofdef_name(this);
}

inline const MessageDef* OneofDef::containing_type() const {
  return upb_oneofdef_containingtype(this);
}
inline const char* OneofDef::name() const {
  return upb_oneofdef_name(this);
}
inline bool OneofDef::set_name(const char* name, Status* s) {
  return upb_oneofdef_setname(this, name, s);
}
inline int OneofDef::field_count() const {
  return upb_oneofdef_numfields(this);
}
inline bool OneofDef::AddField(FieldDef* field, Status* s) {
  return upb_oneofdef_addfield(this, field, NULL, s);
}
inline bool OneofDef::AddField(const reffed_ptr<FieldDef>& field, Status* s) {
  return upb_oneofdef_addfield(this, field.get(), NULL, s);
}
inline const FieldDef* OneofDef::FindFieldByName(const char* name,
                                                 size_t len) const {
  return upb_oneofdef_ntof(this, name, len);
}
inline const FieldDef* OneofDef::FindFieldByNumber(uint32_t num) const {
  return upb_oneofdef_itof(this, num);
}
inline OneofDef::iterator OneofDef::begin() { return iterator(this); }
inline OneofDef::iterator OneofDef::end() { return iterator::end(this); }
inline OneofDef::const_iterator OneofDef::begin() const {
  return const_iterator(this);
}
inline OneofDef::const_iterator OneofDef::end() const {
  return const_iterator::end(this);
}

inline OneofDef::iterator::iterator(OneofDef* o) {
  upb_oneof_begin(&iter_, o);
}
inline OneofDef::iterator OneofDef::iterator::end(OneofDef* o) {
  OneofDef::iterator iter(o);
  upb_oneof_iter_setdone(&iter.iter_);
  return iter;
}
inline FieldDef* OneofDef::iterator::operator*() const {
  return upb_oneof_iter_field(&iter_);
}
inline void OneofDef::iterator::operator++() { return upb_oneof_next(&iter_); }
inline bool OneofDef::iterator::operator==(const iterator &other) const {
  return upb_inttable_iter_isequal(&iter_, &other.iter_);
}
inline bool OneofDef::iterator::operator!=(const iterator &other) const {
  return !(*this == other);
}

inline OneofDef::const_iterator::const_iterator(const OneofDef* md) {
  upb_oneof_begin(&iter_, md);
}
inline OneofDef::const_iterator OneofDef::const_iterator::end(
    const OneofDef *md) {
  OneofDef::const_iterator iter(md);
  upb_oneof_iter_setdone(&iter.iter_);
  return iter;
}
inline const FieldDef* OneofDef::const_iterator::operator*() const {
  return upb_msg_iter_field(&iter_);
}
inline void OneofDef::const_iterator::operator++() {
  return upb_oneof_next(&iter_);
}
inline bool OneofDef::const_iterator::operator==(
    const const_iterator &other) const {
  return upb_inttable_iter_isequal(&iter_, &other.iter_);
}
inline bool OneofDef::const_iterator::operator!=(
    const const_iterator &other) const {
  return !(*this == other);
}

}  /* namespace upb */
#endif

#endif /* UPB_DEF_H_ */
