/*
** Defs are upb's internal representation of the constructs that can appear
** in a .proto file:
**
** - upb::MessageDef (upb_msgdef): describes a "message" construct.
** - upb::FieldDef (upb_fielddef): describes a message field.
** - upb::FileDef (upb_filedef): describes a .proto file and its defs.
** - upb::EnumDef (upb_enumdef): describes an enum.
** - upb::OneofDef (upb_oneofdef): describes a oneof.
**
** TODO: definitions of services.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_DEF_H_
#define UPB_DEF_H_

#include "upb/upb.h"
#include "upb/table.int.h"
#include "google/protobuf/descriptor.upb.h"

#ifdef __cplusplus
#include <cstring>
#include <string>
#include <vector>

namespace upb {
class EnumDef;
class FieldDef;
class FileDef;
class MessageDef;
class OneofDef;
class SymbolTable;
}
#endif

UPB_DECLARE_TYPE(upb::EnumDef, upb_enumdef)
UPB_DECLARE_TYPE(upb::FieldDef, upb_fielddef)
UPB_DECLARE_TYPE(upb::FileDef, upb_filedef)
UPB_DECLARE_TYPE(upb::MessageDef, upb_msgdef)
UPB_DECLARE_TYPE(upb::OneofDef, upb_oneofdef)
UPB_DECLARE_TYPE(upb::SymbolTable, upb_symtab)


/* upb::FieldDef **************************************************************/

/* Maximum field number allowed for FieldDefs.  This is an inherent limit of the
 * protobuf wire format. */
#define UPB_MAX_FIELDNUMBER ((1 << 29) - 1)

#ifdef __cplusplus

/* A upb_fielddef describes a single field in a message.  It is most often
 * found as a part of a upb_msgdef, but can also stand alone to represent
 * an extension. */
class upb::FieldDef {
 public:
  typedef upb_fieldtype_t Type;
  typedef upb_label_t Label;
  typedef upb_descriptortype_t DescriptorType;

  const char* full_name() const;

  Type type() const;
  Label label() const;
  const char* name() const;
  uint32_t number() const;
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

  /* The OneofDef to which this field belongs, or NULL if this field is not part
   * of a oneof. */
  const OneofDef* containing_oneof() const;

  /* The field's type according to the enum in descriptor.proto.  This is not
   * the same as UPB_TYPE_*, because it distinguishes between (for example)
   * INT32 and SINT32, whereas our "type" enum does not.  This return of
   * descriptor_type() is a function of type(), integer_format(), and
   * is_tag_delimited().  */
  DescriptorType descriptor_type() const;

  /* Convenient field type tests. */
  bool IsSubMessage() const;
  bool IsString() const;
  bool IsSequence() const;
  bool IsPrimitive() const;
  bool IsMap() const;

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

  /* Returns the enum or submessage def for this field, if any.  The field's
   * type must match (ie. you may only call enum_subdef() for fields where
   * type() == UPB_TYPE_ENUM). */
  const EnumDef* enum_subdef() const;
  const MessageDef* message_subdef() const;

 private:
  UPB_DISALLOW_POD_OPS(FieldDef, upb::FieldDef)
};

# endif  /* defined(__cplusplus) */

UPB_BEGIN_EXTERN_C

/* Native C API. */
const char *upb_fielddef_fullname(const upb_fielddef *f);
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
uint32_t upb_fielddef_index(const upb_fielddef *f);
bool upb_fielddef_issubmsg(const upb_fielddef *f);
bool upb_fielddef_isstring(const upb_fielddef *f);
bool upb_fielddef_isseq(const upb_fielddef *f);
bool upb_fielddef_isprimitive(const upb_fielddef *f);
bool upb_fielddef_ismap(const upb_fielddef *f);
int64_t upb_fielddef_defaultint64(const upb_fielddef *f);
int32_t upb_fielddef_defaultint32(const upb_fielddef *f);
uint64_t upb_fielddef_defaultuint64(const upb_fielddef *f);
uint32_t upb_fielddef_defaultuint32(const upb_fielddef *f);
bool upb_fielddef_defaultbool(const upb_fielddef *f);
float upb_fielddef_defaultfloat(const upb_fielddef *f);
double upb_fielddef_defaultdouble(const upb_fielddef *f);
const char *upb_fielddef_defaultstr(const upb_fielddef *f, size_t *len);
bool upb_fielddef_hassubdef(const upb_fielddef *f);
const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f);
const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f);

/* Internal only. */
uint32_t upb_fielddef_selectorbase(const upb_fielddef *f);

UPB_END_EXTERN_C


/* upb::MessageDef ************************************************************/

typedef upb_inttable_iter upb_msg_field_iter;
typedef upb_strtable_iter upb_msg_oneof_iter;

/* Well-known field tag numbers for map-entry messages. */
#define UPB_MAPENTRY_KEY   1
#define UPB_MAPENTRY_VALUE 2

/* Well-known field tag numbers for timestamp messages. */
#define UPB_DURATION_SECONDS 1
#define UPB_DURATION_NANOS 2

/* Well-known field tag numbers for duration messages. */
#define UPB_TIMESTAMP_SECONDS 1
#define UPB_TIMESTAMP_NANOS 2

#ifdef __cplusplus

/* Structure that describes a single .proto message type. */
class upb::MessageDef {
 public:
  const char* full_name() const;
  const char* name() const;

  /* The number of fields that belong to the MessageDef. */
  int field_count() const;

  /* The number of oneofs that belong to the MessageDef. */
  int oneof_count() const;

  upb_syntax_t syntax() const;

  /* These return NULL if the field is not found. */
  const FieldDef* FindFieldByNumber(uint32_t number) const;
  const FieldDef* FindFieldByName(const char* name, size_t len) const;


  const FieldDef* FindFieldByName(const char *name) const {
    return FindFieldByName(name, strlen(name));
  }

  template <class T>
  const FieldDef* FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  OneofDef* FindOneofByName(const char* name, size_t len);
  const OneofDef* FindOneofByName(const char* name, size_t len) const;

  const OneofDef* FindOneofByName(const char* name) const {
    return FindOneofByName(name, strlen(name));
  }

  template<class T>
  const OneofDef* FindOneofByName(const T& str) const {
    return FindOneofByName(str.c_str(), str.size());
  }

  /* Is this message a map entry? */
  void setmapentry(bool map_entry);
  bool mapentry() const;

  /* Return the type of well known type message. UPB_WELLKNOWN_UNSPECIFIED for
   * non-well-known message. */
  upb_wellknowntype_t wellknowntype() const;

  /* Whether is a number wrapper. */
  bool isnumberwrapper() const;

  /* Iteration over fields.  The order is undefined. */
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

  class ConstFieldAccessor {
   public:
    explicit ConstFieldAccessor(const MessageDef* msg) : msg_(msg) {}
    const_field_iterator begin() { return msg_->field_begin(); }
    const_field_iterator end() { return msg_->field_end(); }
   private:
    const MessageDef* msg_;
  };

  class ConstOneofAccessor {
   public:
    explicit ConstOneofAccessor(const MessageDef* msg) : msg_(msg) {}
    const_oneof_iterator begin() { return msg_->oneof_begin(); }
    const_oneof_iterator end() { return msg_->oneof_end(); }
   private:
    const MessageDef* msg_;
  };

  const_field_iterator field_begin() const;
  const_field_iterator field_end() const;

  const_oneof_iterator oneof_begin() const;
  const_oneof_iterator oneof_end() const;

  ConstFieldAccessor fields() const { return ConstFieldAccessor(this); }
  ConstOneofAccessor oneofs() const { return ConstOneofAccessor(this); }

 private:
  UPB_DISALLOW_POD_OPS(MessageDef, upb::MessageDef)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

const char *upb_msgdef_fullname(const upb_msgdef *m);
const char *upb_msgdef_name(const upb_msgdef *m);
int upb_msgdef_numoneofs(const upb_msgdef *m);
upb_syntax_t upb_msgdef_syntax(const upb_msgdef *m);
bool upb_msgdef_mapentry(const upb_msgdef *m);
upb_wellknowntype_t upb_msgdef_wellknowntype(const upb_msgdef *m);
bool upb_msgdef_isnumberwrapper(const upb_msgdef *m);
bool upb_msgdef_setsyntax(upb_msgdef *m, upb_syntax_t syntax);

/* Internal-only. */
size_t upb_msgdef_selectorcount(const upb_msgdef *m);
uint32_t upb_msgdef_submsgfieldcount(const upb_msgdef *m);

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

/* Lookup of either field or oneof by name.  Returns whether either was found.
 * If the return is true, then the found def will be set, and the non-found
 * one set to NULL. */
bool upb_msgdef_lookupname(const upb_msgdef *m, const char *name, size_t len,
                           const upb_fielddef **f, const upb_oneofdef **o);

UPB_INLINE bool upb_msgdef_lookupnamez(const upb_msgdef *m, const char *name,
                                       const upb_fielddef **f,
                                       const upb_oneofdef **o) {
  return upb_msgdef_lookupname(m, name, strlen(name), f, o);
}

/* Iteration over fields and oneofs.  For example:
 *
 * upb_msg_field_iter i;
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

class upb::EnumDef {
 public:
  const char* full_name() const;
  const char* name() const;
  /* The value that is used as the default when no field default is specified.
   * If not set explicitly, the first value that was added will be used.
   * The default value must be a member of the enum.
   * Requires that value_count() > 0. */
  int32_t default_value() const;

  /* Returns the number of values currently defined in the enum.  Note that
   * multiple names can refer to the same number, so this may be greater than
   * the total number of unique numbers. */
  int value_count() const;

  /* Lookups from name to integer, returning true if found. */
  bool FindValueByName(const char* name, int32_t* num) const;

  /* Finds the name corresponding to the given number, or NULL if none was
   * found.  If more than one name corresponds to this number, returns the
   * first one that was added. */
  const char* FindValueByNumber(int32_t num) const;

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

const char *upb_enumdef_fullname(const upb_enumdef *e);
const char *upb_enumdef_name(const upb_enumdef *e);
int32_t upb_enumdef_default(const upb_enumdef *e);
int upb_enumdef_numvals(const upb_enumdef *e);

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

/* Class that represents a oneof. */
class upb::OneofDef {
 public:
  /* Returns the MessageDef that owns this OneofDef. */
  const MessageDef* containing_type() const;

  /* Returns the name of this oneof. This is the name used to look up the oneof
   * by name once added to a message def. */
  const char* name() const;

  /* Returns the number of fields currently defined in the oneof. */
  int field_count() const;

  /* Looks up by name. */
  const FieldDef* FindFieldByName(const char* name, size_t len) const;
  FieldDef* FindFieldByName(const char* name, size_t len);
  const FieldDef* FindFieldByName(const char* name) const {
    return FindFieldByName(name, strlen(name));
  }

  template <class T>
  const FieldDef* FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  /* Looks up by tag number. */
  const FieldDef* FindFieldByNumber(uint32_t num) const;

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

  const_iterator begin() const;
  const_iterator end() const;

 private:
  UPB_DISALLOW_POD_OPS(OneofDef, upb::OneofDef)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

const char *upb_oneofdef_name(const upb_oneofdef *o);
const upb_msgdef *upb_oneofdef_containingtype(const upb_oneofdef *o);
int upb_oneofdef_numfields(const upb_oneofdef *o);
uint32_t upb_oneofdef_index(const upb_oneofdef *o);

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


/* upb::FileDef ***************************************************************/

#ifdef __cplusplus

/* Class that represents a .proto file with some things defined in it.
 *
 * Many users won't care about FileDefs, but they are necessary if you want to
 * read the values of file-level options. */
class upb::FileDef {
 public:
  /* Get/set name of the file (eg. "foo/bar.proto"). */
  const char* name() const;

  /* Package name for definitions inside the file (eg. "foo.bar"). */
  const char* package() const;

  /* Sets the php class prefix which is prepended to all php generated classes
   * from this .proto. Default is empty. */
  const char* phpprefix() const;

  /* Use this option to change the namespace of php generated classes. Default
   * is empty. When this option is empty, the package name will be used for
   * determining the namespace. */
  const char* phpnamespace() const;

  /* Syntax for the file.  Defaults to proto2. */
  upb_syntax_t syntax() const;

  /* Get the list of dependencies from the file.  These are returned in the
   * order that they were added to the FileDef. */
  int dependency_count() const;
  const FileDef* dependency(int index) const;

 private:
  UPB_DISALLOW_POD_OPS(FileDef, upb::FileDef)
};

#endif

UPB_BEGIN_EXTERN_C

const char *upb_filedef_name(const upb_filedef *f);
const char *upb_filedef_package(const upb_filedef *f);
const char *upb_filedef_phpprefix(const upb_filedef *f);
const char *upb_filedef_phpnamespace(const upb_filedef *f);
upb_syntax_t upb_filedef_syntax(const upb_filedef *f);
int upb_filedef_depcount(const upb_filedef *f);
int upb_filedef_msgcount(const upb_filedef *f);
int upb_filedef_enumcount(const upb_filedef *f);
const upb_filedef *upb_filedef_dep(const upb_filedef *f, int i);
const upb_msgdef *upb_filedef_msg(const upb_filedef *f, int i);
const upb_enumdef *upb_filedef_enum(const upb_filedef *f, int i);

UPB_END_EXTERN_C

#ifdef __cplusplus

/* Non-const methods in upb::SymbolTable are NOT thread-safe. */
class upb::SymbolTable {
 public:
  /* Returns a new symbol table with a single ref owned by "owner."
   * Returns NULL if memory allocation failed. */
  static SymbolTable* New();
  static void Free(upb::SymbolTable* table);

  /* Finds an entry in the symbol table with this exact name.  If not found,
   * returns NULL. */
  const MessageDef* LookupMessage(const char *sym) const;
  const EnumDef* LookupEnum(const char *sym) const;

  /* TODO: iteration? */

  /* Adds the given serialized FileDescriptorProto to the pool. */
  bool AddFile(const google_protobuf_FileDescriptorProto *file_proto,
               Status *status);

  /* Adds the given serialized FileDescriptorSet to the pool. */
  bool AddSet(const char *set, size_t len, Status *status);

 private:
  UPB_DISALLOW_POD_OPS(SymbolTable, upb::SymbolTable)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */

upb_symtab *upb_symtab_new();
void upb_symtab_free(upb_symtab* s);
const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym);
const upb_msgdef *upb_symtab_lookupmsg2(
    const upb_symtab *s, const char *sym, size_t len);
const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym);
int upb_symtab_filecount(const upb_symtab *s);
bool upb_symtab_addfile(upb_symtab *s,
                        const google_protobuf_FileDescriptorProto* file,
                        upb_status *status);

/* For generated code only: loads a generated descriptor. */
typedef struct upb_def_init {
  struct upb_def_init **deps;
  const char *filename;
  upb_stringview descriptor;
} upb_def_init;

bool _upb_symtab_loaddefinit(upb_symtab *s, const upb_def_init *init);

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ inline wrappers. */
namespace upb {
inline SymbolTable* SymbolTable::New() {
  return upb_symtab_new();
}
inline void SymbolTable::Free(SymbolTable* s) {
  upb_symtab_free(s);
}
inline const MessageDef *SymbolTable::LookupMessage(const char *sym) const {
  return upb_symtab_lookupmsg(this, sym);
}
inline bool SymbolTable::AddFile(
    const google_protobuf_FileDescriptorProto *file_proto, Status *status) {
  return upb_symtab_addfile(this, file_proto, status);
}
}  /* namespace upb */
#endif

#ifdef __cplusplus

UPB_INLINE const char* upb_safecstr(const std::string& str) {
  UPB_ASSERT(str.size() == std::strlen(str.c_str()));
  return str.c_str();
}

/* Inline C++ wrappers. */
namespace upb {

inline const char* FieldDef::full_name() const {
  return upb_fielddef_fullname(this);
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
inline bool FieldDef::packed() const {
  return upb_fielddef_packed(this);
}
inline uint32_t FieldDef::index() const {
  return upb_fielddef_index(this);
}
inline const MessageDef* FieldDef::containing_type() const {
  return upb_fielddef_containingtype(this);
}
inline const OneofDef* FieldDef::containing_oneof() const {
  return upb_fielddef_containingoneof(this);
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
inline const MessageDef *FieldDef::message_subdef() const {
  return upb_fielddef_msgsubdef(this);
}
inline const EnumDef *FieldDef::enum_subdef() const {
  return upb_fielddef_enumsubdef(this);
}

inline const char *MessageDef::full_name() const {
  return upb_msgdef_fullname(this);
}
inline const char *MessageDef::name() const {
  return upb_msgdef_name(this);
}
inline upb_syntax_t MessageDef::syntax() const {
  return upb_msgdef_syntax(this);
}
inline int MessageDef::field_count() const {
  return upb_msgdef_numfields(this);
}
inline int MessageDef::oneof_count() const {
  return upb_msgdef_numoneofs(this);
}
inline const FieldDef* MessageDef::FindFieldByNumber(uint32_t number) const {
  return upb_msgdef_itof(this, number);
}
inline const FieldDef *MessageDef::FindFieldByName(const char *name,
                                                   size_t len) const {
  return upb_msgdef_ntof(this, name, len);
}
inline const OneofDef* MessageDef::FindOneofByName(const char* name,
                                                   size_t len) const {
  return upb_msgdef_ntoo(this, name, len);
}
inline bool MessageDef::mapentry() const {
  return upb_msgdef_mapentry(this);
}
inline upb_wellknowntype_t MessageDef::wellknowntype() const {
  return upb_msgdef_wellknowntype(this);
}
inline bool MessageDef::isnumberwrapper() const {
  return upb_msgdef_isnumberwrapper(this);
}
inline MessageDef::const_field_iterator MessageDef::field_begin() const {
  return const_field_iterator(this);
}
inline MessageDef::const_field_iterator MessageDef::field_end() const {
  return const_field_iterator::end(this);
}

inline MessageDef::const_oneof_iterator MessageDef::oneof_begin() const {
  return const_oneof_iterator(this);
}
inline MessageDef::const_oneof_iterator MessageDef::oneof_end() const {
  return const_oneof_iterator::end(this);
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

inline const char* EnumDef::full_name() const {
  return upb_enumdef_fullname(this);
}
inline const char* EnumDef::name() const {
  return upb_enumdef_name(this);
}
inline int32_t EnumDef::default_value() const {
  return upb_enumdef_default(this);
}
inline int EnumDef::value_count() const { return upb_enumdef_numvals(this); }
inline bool EnumDef::FindValueByName(const char* name, int32_t *num) const {
  return upb_enumdef_ntoiz(this, name, num);
}
inline const char* EnumDef::FindValueByNumber(int32_t num) const {
  return upb_enumdef_iton(this, num);
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

inline const MessageDef* OneofDef::containing_type() const {
  return upb_oneofdef_containingtype(this);
}
inline const char* OneofDef::name() const {
  return upb_oneofdef_name(this);
}
inline int OneofDef::field_count() const {
  return upb_oneofdef_numfields(this);
}
inline const FieldDef* OneofDef::FindFieldByName(const char* name,
                                                 size_t len) const {
  return upb_oneofdef_ntof(this, name, len);
}
inline const FieldDef* OneofDef::FindFieldByNumber(uint32_t num) const {
  return upb_oneofdef_itof(this, num);
}
inline OneofDef::const_iterator OneofDef::begin() const {
  return const_iterator(this);
}
inline OneofDef::const_iterator OneofDef::end() const {
  return const_iterator::end(this);
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

inline const char* FileDef::name() const {
  return upb_filedef_name(this);
}
inline const char* FileDef::package() const {
  return upb_filedef_package(this);
}
inline const char* FileDef::phpprefix() const {
  return upb_filedef_phpprefix(this);
}
inline const char* FileDef::phpnamespace() const {
  return upb_filedef_phpnamespace(this);
}
inline int FileDef::dependency_count() const {
  return upb_filedef_depcount(this);
}
inline const FileDef* FileDef::dependency(int index) const {
  return upb_filedef_dep(this, index);
}

}  /* namespace upb */
#endif

#endif /* UPB_DEF_H_ */
