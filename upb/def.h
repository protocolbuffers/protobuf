/*
** Defs are upb's internal representation of the constructs that can appear
** in a .proto file:
**
** - upb::MessageDefPtr (upb_msgdef): describes a "message" construct.
** - upb::FieldDefPtr (upb_fielddef): describes a message field.
** - upb::FileDefPtr (upb_filedef): describes a .proto file and its defs.
** - upb::EnumDefPtr (upb_enumdef): describes an enum.
** - upb::OneofDefPtr (upb_oneofdef): describes a oneof.
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
#include <memory>
#include <string>
#include <vector>

namespace upb {
class EnumDefPtr;
class FieldDefPtr;
class FileDefPtr;
class MessageDefPtr;
class OneofDefPtr;
class SymbolTable;
}
#endif

#include "upb/port_def.inc"

struct upb_enumdef;
typedef struct upb_enumdef upb_enumdef;
struct upb_fielddef;
typedef struct upb_fielddef upb_fielddef;
struct upb_filedef;
typedef struct upb_filedef upb_filedef;
struct upb_msgdef;
typedef struct upb_msgdef upb_msgdef;
struct upb_oneofdef;
typedef struct upb_oneofdef upb_oneofdef;
struct upb_symtab;
typedef struct upb_symtab upb_symtab;

typedef enum {
  UPB_SYNTAX_PROTO2 = 2,
  UPB_SYNTAX_PROTO3 = 3
} upb_syntax_t;

/* All the different kind of well known type messages. For simplicity of check,
 * number wrappers and string wrappers are grouped together. Make sure the
 * order and merber of these groups are not changed.
 */
typedef enum {
  UPB_WELLKNOWN_UNSPECIFIED,
  UPB_WELLKNOWN_ANY,
  UPB_WELLKNOWN_FIELDMASK,
  UPB_WELLKNOWN_DURATION,
  UPB_WELLKNOWN_TIMESTAMP,
  /* number wrappers */
  UPB_WELLKNOWN_DOUBLEVALUE,
  UPB_WELLKNOWN_FLOATVALUE,
  UPB_WELLKNOWN_INT64VALUE,
  UPB_WELLKNOWN_UINT64VALUE,
  UPB_WELLKNOWN_INT32VALUE,
  UPB_WELLKNOWN_UINT32VALUE,
  /* string wrappers */
  UPB_WELLKNOWN_STRINGVALUE,
  UPB_WELLKNOWN_BYTESVALUE,
  UPB_WELLKNOWN_BOOLVALUE,
  UPB_WELLKNOWN_VALUE,
  UPB_WELLKNOWN_LISTVALUE,
  UPB_WELLKNOWN_STRUCT
} upb_wellknowntype_t;

/* upb_fielddef ***************************************************************/

/* Maximum field number allowed for FieldDefs.  This is an inherent limit of the
 * protobuf wire format. */
#define UPB_MAX_FIELDNUMBER ((1 << 29) - 1)

#ifdef __cplusplus
extern "C" {
#endif

const char *upb_fielddef_fullname(const upb_fielddef *f);
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
bool upb_fielddef_haspresence(const upb_fielddef *f);
const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f);
const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f);

/* Internal only. */
uint32_t upb_fielddef_selectorbase(const upb_fielddef *f);

#ifdef __cplusplus
}  /* extern "C" */

/* A upb_fielddef describes a single field in a message.  It is most often
 * found as a part of a upb_msgdef, but can also stand alone to represent
 * an extension. */
class upb::FieldDefPtr {
 public:
  FieldDefPtr() : ptr_(nullptr) {}
  explicit FieldDefPtr(const upb_fielddef *ptr) : ptr_(ptr) {}

  const upb_fielddef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  typedef upb_fieldtype_t Type;
  typedef upb_label_t Label;
  typedef upb_descriptortype_t DescriptorType;

  const char* full_name() const { return upb_fielddef_fullname(ptr_); }

  Type type() const { return upb_fielddef_type(ptr_); }
  Label label() const { return upb_fielddef_label(ptr_); }
  const char* name() const { return upb_fielddef_name(ptr_); }
  uint32_t number() const { return upb_fielddef_number(ptr_); }
  bool is_extension() const { return upb_fielddef_isextension(ptr_); }

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
  size_t GetJsonName(char *buf, size_t len) const {
    return upb_fielddef_getjsonname(ptr_, buf, len);
  }

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
  bool lazy() const { return upb_fielddef_lazy(ptr_); }

  /* For non-string, non-submessage fields, this indicates whether binary
   * protobufs are encoded in packed or non-packed format.
   *
   * TODO(haberman): see note above about putting options like this into a
   * FieldOptions container. */
  bool packed() const { return upb_fielddef_packed(ptr_); }

  /* An integer that can be used as an index into an array of fields for
   * whatever message this field belongs to.  Guaranteed to be less than
   * f->containing_type()->field_count().  May only be accessed once the def has
   * been finalized. */
  uint32_t index() const { return upb_fielddef_index(ptr_); }

  /* The MessageDef to which this field belongs.
   *
   * If this field has been added to a MessageDef, that message can be retrieved
   * directly (this is always the case for frozen FieldDefs).
   *
   * If the field has not yet been added to a MessageDef, you can set the name
   * of the containing type symbolically instead.  This is mostly useful for
   * extensions, where the extension is declared separately from the message. */
  MessageDefPtr containing_type() const;

  /* The OneofDef to which this field belongs, or NULL if this field is not part
   * of a oneof. */
  OneofDefPtr containing_oneof() const;

  /* The field's type according to the enum in descriptor.proto.  This is not
   * the same as UPB_TYPE_*, because it distinguishes between (for example)
   * INT32 and SINT32, whereas our "type" enum does not.  This return of
   * descriptor_type() is a function of type(), integer_format(), and
   * is_tag_delimited().  */
  DescriptorType descriptor_type() const {
    return upb_fielddef_descriptortype(ptr_);
  }

  /* Convenient field type tests. */
  bool IsSubMessage() const { return upb_fielddef_issubmsg(ptr_); }
  bool IsString() const { return upb_fielddef_isstring(ptr_); }
  bool IsSequence() const { return upb_fielddef_isseq(ptr_); }
  bool IsPrimitive() const { return upb_fielddef_isprimitive(ptr_); }
  bool IsMap() const { return upb_fielddef_ismap(ptr_); }

  /* Returns the non-string default value for this fielddef, which may either
   * be something the client set explicitly or the "default default" (0 for
   * numbers, empty for strings).  The field's type indicates the type of the
   * returned value, except for enum fields that are still mutable.
   *
   * Requires that the given function matches the field's current type. */
  int64_t default_int64() const { return upb_fielddef_defaultint64(ptr_); }
  int32_t default_int32() const { return upb_fielddef_defaultint32(ptr_); }
  uint64_t default_uint64() const { return upb_fielddef_defaultuint64(ptr_); }
  uint32_t default_uint32() const { return upb_fielddef_defaultuint32(ptr_); }
  bool default_bool() const { return upb_fielddef_defaultbool(ptr_); }
  float default_float() const { return upb_fielddef_defaultfloat(ptr_); }
  double default_double() const { return upb_fielddef_defaultdouble(ptr_); }

  /* The resulting string is always NULL-terminated.  If non-NULL, the length
   * will be stored in *len. */
  const char *default_string(size_t * len) const {
    return upb_fielddef_defaultstr(ptr_, len);
  }

  /* Returns the enum or submessage def for this field, if any.  The field's
   * type must match (ie. you may only call enum_subdef() for fields where
   * type() == UPB_TYPE_ENUM). */
  EnumDefPtr enum_subdef() const;
  MessageDefPtr message_subdef() const;

 private:
  const upb_fielddef *ptr_;
};

#endif  /* __cplusplus */

/* upb_oneofdef ***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef upb_inttable_iter upb_oneof_iter;

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
bool upb_oneof_iter_isequal(const upb_oneof_iter *iter1,
                            const upb_oneof_iter *iter2);

#ifdef __cplusplus
}  /* extern "C" */

/* Class that represents a oneof. */
class upb::OneofDefPtr {
 public:
  OneofDefPtr() : ptr_(nullptr) {}
  explicit OneofDefPtr(const upb_oneofdef *ptr) : ptr_(ptr) {}

  const upb_oneofdef* ptr() const { return ptr_; }
  explicit operator bool() { return ptr_ != nullptr; }

  /* Returns the MessageDef that owns this OneofDef. */
  MessageDefPtr containing_type() const;

  /* Returns the name of this oneof. This is the name used to look up the oneof
   * by name once added to a message def. */
  const char* name() const { return upb_oneofdef_name(ptr_); }

  /* Returns the number of fields currently defined in the oneof. */
  int field_count() const { return upb_oneofdef_numfields(ptr_); }

  /* Looks up by name. */
  FieldDefPtr FindFieldByName(const char *name, size_t len) const {
    return FieldDefPtr(upb_oneofdef_ntof(ptr_, name, len));
  }
  FieldDefPtr FindFieldByName(const char* name) const {
    return FieldDefPtr(upb_oneofdef_ntofz(ptr_, name));
  }

  template <class T>
  FieldDefPtr FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  /* Looks up by tag number. */
  FieldDefPtr FindFieldByNumber(uint32_t num) const {
    return FieldDefPtr(upb_oneofdef_itof(ptr_, num));
  }

  class const_iterator
      : public std::iterator<std::forward_iterator_tag, FieldDefPtr> {
   public:
    void operator++() { upb_oneof_next(&iter_); }

    FieldDefPtr operator*() const {
      return FieldDefPtr(upb_oneof_iter_field(&iter_));
    }

    bool operator!=(const const_iterator& other) const {
      return !upb_oneof_iter_isequal(&iter_, &other.iter_);
    }

    bool operator==(const const_iterator& other) const {
      return upb_oneof_iter_isequal(&iter_, &other.iter_);
    }

   private:
    friend class OneofDefPtr;

    const_iterator() {}
    explicit const_iterator(OneofDefPtr o) {
      upb_oneof_begin(&iter_, o.ptr());
    }
    static const_iterator end() {
      const_iterator iter;
      upb_oneof_iter_setdone(&iter.iter_);
      return iter;
    }

    upb_oneof_iter iter_;
  };

  const_iterator begin() const { return const_iterator(*this); }
  const_iterator end() const { return const_iterator::end(); }

 private:
  const upb_oneofdef *ptr_;
};

inline upb::OneofDefPtr upb::FieldDefPtr::containing_oneof() const {
  return OneofDefPtr(upb_fielddef_containingoneof(ptr_));
}

#endif  /* __cplusplus */

/* upb_msgdef *****************************************************************/

typedef upb_inttable_iter upb_msg_field_iter;
typedef upb_strtable_iter upb_msg_oneof_iter;

/* Well-known field tag numbers for map-entry messages. */
#define UPB_MAPENTRY_KEY   1
#define UPB_MAPENTRY_VALUE 2

/* Well-known field tag numbers for Any messages. */
#define UPB_ANY_TYPE 1
#define UPB_ANY_VALUE 2

/* Well-known field tag numbers for timestamp messages. */
#define UPB_DURATION_SECONDS 1
#define UPB_DURATION_NANOS 2

/* Well-known field tag numbers for duration messages. */
#define UPB_TIMESTAMP_SECONDS 1
#define UPB_TIMESTAMP_NANOS 2

#ifdef __cplusplus
extern "C" {
#endif

const char *upb_msgdef_fullname(const upb_msgdef *m);
const upb_filedef *upb_msgdef_file(const upb_msgdef *m);
const char *upb_msgdef_name(const upb_msgdef *m);
int upb_msgdef_numoneofs(const upb_msgdef *m);
upb_syntax_t upb_msgdef_syntax(const upb_msgdef *m);
bool upb_msgdef_mapentry(const upb_msgdef *m);
upb_wellknowntype_t upb_msgdef_wellknowntype(const upb_msgdef *m);
bool upb_msgdef_isnumberwrapper(const upb_msgdef *m);
bool upb_msgdef_setsyntax(upb_msgdef *m, upb_syntax_t syntax);
const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i);
const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name,
                                    size_t len);
const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len);
int upb_msgdef_numfields(const upb_msgdef *m);
int upb_msgdef_numoneofs(const upb_msgdef *m);

UPB_INLINE const upb_oneofdef *upb_msgdef_ntooz(const upb_msgdef *m,
                                               const char *name) {
  return upb_msgdef_ntoo(m, name, strlen(name));
}

UPB_INLINE const upb_fielddef *upb_msgdef_ntofz(const upb_msgdef *m,
                                                const char *name) {
  return upb_msgdef_ntof(m, name, strlen(name));
}

/* Internal-only. */
size_t upb_msgdef_selectorcount(const upb_msgdef *m);
uint32_t upb_msgdef_submsgfieldcount(const upb_msgdef *m);

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
bool upb_msg_field_iter_isequal(const upb_msg_field_iter * iter1,
                                const upb_msg_field_iter * iter2);

/* Similar to above, we also support iterating through the oneofs in a
 * msgdef. */
void upb_msg_oneof_begin(upb_msg_oneof_iter * iter, const upb_msgdef *m);
void upb_msg_oneof_next(upb_msg_oneof_iter * iter);
bool upb_msg_oneof_done(const upb_msg_oneof_iter *iter);
const upb_oneofdef *upb_msg_iter_oneof(const upb_msg_oneof_iter *iter);
void upb_msg_oneof_iter_setdone(upb_msg_oneof_iter * iter);
bool upb_msg_oneof_iter_isequal(const upb_msg_oneof_iter *iter1,
                                const upb_msg_oneof_iter *iter2);

#ifdef __cplusplus
}  /* extern "C" */

/* Structure that describes a single .proto message type. */
class upb::MessageDefPtr {
 public:
  MessageDefPtr() : ptr_(nullptr) {}
  explicit MessageDefPtr(const upb_msgdef *ptr) : ptr_(ptr) {}

  const upb_msgdef *ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  const char* full_name() const { return upb_msgdef_fullname(ptr_); }
  const char* name() const { return upb_msgdef_name(ptr_); }

  /* The number of fields that belong to the MessageDef. */
  int field_count() const { return upb_msgdef_numfields(ptr_); }

  /* The number of oneofs that belong to the MessageDef. */
  int oneof_count() const { return upb_msgdef_numoneofs(ptr_); }

  upb_syntax_t syntax() const { return upb_msgdef_syntax(ptr_); }

  /* These return null pointers if the field is not found. */
  FieldDefPtr FindFieldByNumber(uint32_t number) const {
    return FieldDefPtr(upb_msgdef_itof(ptr_, number));
  }
  FieldDefPtr FindFieldByName(const char* name, size_t len) const {
    return FieldDefPtr(upb_msgdef_ntof(ptr_, name, len));
  }
  FieldDefPtr FindFieldByName(const char *name) const {
    return FieldDefPtr(upb_msgdef_ntofz(ptr_, name));
  }

  template <class T>
  FieldDefPtr FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  OneofDefPtr FindOneofByName(const char* name, size_t len) const {
    return OneofDefPtr(upb_msgdef_ntoo(ptr_, name, len));
  }

  OneofDefPtr FindOneofByName(const char *name) const {
    return OneofDefPtr(upb_msgdef_ntooz(ptr_, name));
  }

  template <class T>
  OneofDefPtr FindOneofByName(const T &str) const {
    return FindOneofByName(str.c_str(), str.size());
  }

  /* Is this message a map entry? */
  bool mapentry() const { return upb_msgdef_mapentry(ptr_); }

  /* Return the type of well known type message. UPB_WELLKNOWN_UNSPECIFIED for
   * non-well-known message. */
  upb_wellknowntype_t wellknowntype() const {
    return upb_msgdef_wellknowntype(ptr_);
  }

  /* Whether is a number wrapper. */
  bool isnumberwrapper() const { return upb_msgdef_isnumberwrapper(ptr_); }

  /* Iteration over fields.  The order is undefined. */
  class const_field_iterator
      : public std::iterator<std::forward_iterator_tag, FieldDefPtr> {
   public:
    void operator++() { upb_msg_field_next(&iter_); }

    FieldDefPtr operator*() const {
      return FieldDefPtr(upb_msg_iter_field(&iter_));
    }

    bool operator!=(const const_field_iterator &other) const {
      return !upb_msg_field_iter_isequal(&iter_, &other.iter_);
    }

    bool operator==(const const_field_iterator &other) const {
      return upb_msg_field_iter_isequal(&iter_, &other.iter_);
    }

   private:
    friend class MessageDefPtr;

    explicit const_field_iterator() {}

    explicit const_field_iterator(MessageDefPtr msg) {
      upb_msg_field_begin(&iter_, msg.ptr());
    }

    static const_field_iterator end() {
      const_field_iterator iter;
      upb_msg_field_iter_setdone(&iter.iter_);
      return iter;
    }

    upb_msg_field_iter iter_;
  };

  /* Iteration over oneofs. The order is undefined. */
  class const_oneof_iterator
      : public std::iterator<std::forward_iterator_tag, OneofDefPtr> {
   public:

    void operator++() { upb_msg_oneof_next(&iter_); }

    OneofDefPtr operator*() const {
      return OneofDefPtr(upb_msg_iter_oneof(&iter_));
    }

    bool operator!=(const const_oneof_iterator& other) const {
      return !upb_msg_oneof_iter_isequal(&iter_, &other.iter_);
    }

    bool operator==(const const_oneof_iterator &other) const {
      return upb_msg_oneof_iter_isequal(&iter_, &other.iter_);
    }

   private:
    friend class MessageDefPtr;

    const_oneof_iterator() {}

    explicit const_oneof_iterator(MessageDefPtr msg) {
      upb_msg_oneof_begin(&iter_, msg.ptr());
    }

    static const_oneof_iterator end() {
      const_oneof_iterator iter;
      upb_msg_oneof_iter_setdone(&iter.iter_);
      return iter;
    }

    upb_msg_oneof_iter iter_;
  };

  class ConstFieldAccessor {
   public:
    explicit ConstFieldAccessor(const upb_msgdef* md) : md_(md) {}
    const_field_iterator begin() { return MessageDefPtr(md_).field_begin(); }
    const_field_iterator end() { return MessageDefPtr(md_).field_end(); }
   private:
    const upb_msgdef* md_;
  };

  class ConstOneofAccessor {
   public:
    explicit ConstOneofAccessor(const upb_msgdef* md) : md_(md) {}
    const_oneof_iterator begin() { return MessageDefPtr(md_).oneof_begin(); }
    const_oneof_iterator end() { return MessageDefPtr(md_).oneof_end(); }
   private:
    const upb_msgdef* md_;
  };

  const_field_iterator field_begin() const {
    return const_field_iterator(*this);
  }

  const_field_iterator field_end() const { return const_field_iterator::end(); }

  const_oneof_iterator oneof_begin() const {
    return const_oneof_iterator(*this);
  }

  const_oneof_iterator oneof_end() const { return const_oneof_iterator::end(); }

  ConstFieldAccessor fields() const { return ConstFieldAccessor(ptr()); }
  ConstOneofAccessor oneofs() const { return ConstOneofAccessor(ptr()); }

 private:
  const upb_msgdef* ptr_;
};

inline upb::MessageDefPtr upb::FieldDefPtr::message_subdef() const {
  return MessageDefPtr(upb_fielddef_msgsubdef(ptr_));
}

inline upb::MessageDefPtr upb::FieldDefPtr::containing_type() const {
  return MessageDefPtr(upb_fielddef_containingtype(ptr_));
}

inline upb::MessageDefPtr upb::OneofDefPtr::containing_type() const {
  return MessageDefPtr(upb_oneofdef_containingtype(ptr_));
}

#endif  /* __cplusplus */

/* upb_enumdef ****************************************************************/

typedef upb_strtable_iter upb_enum_iter;

const char *upb_enumdef_fullname(const upb_enumdef *e);
const char *upb_enumdef_name(const upb_enumdef *e);
const upb_filedef *upb_enumdef_file(const upb_enumdef *e);
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

#ifdef __cplusplus

class upb::EnumDefPtr {
 public:
  EnumDefPtr() : ptr_(nullptr) {}
  explicit EnumDefPtr(const upb_enumdef* ptr) : ptr_(ptr) {}

  const upb_enumdef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  const char* full_name() const { return upb_enumdef_fullname(ptr_); }
  const char* name() const { return upb_enumdef_name(ptr_); }

  /* The value that is used as the default when no field default is specified.
   * If not set explicitly, the first value that was added will be used.
   * The default value must be a member of the enum.
   * Requires that value_count() > 0. */
  int32_t default_value() const { return upb_enumdef_default(ptr_); }

  /* Returns the number of values currently defined in the enum.  Note that
   * multiple names can refer to the same number, so this may be greater than
   * the total number of unique numbers. */
  int value_count() const { return upb_enumdef_numvals(ptr_); }

  /* Lookups from name to integer, returning true if found. */
  bool FindValueByName(const char *name, int32_t *num) const {
    return upb_enumdef_ntoiz(ptr_, name, num);
  }

  /* Finds the name corresponding to the given number, or NULL if none was
   * found.  If more than one name corresponds to this number, returns the
   * first one that was added. */
  const char *FindValueByNumber(int32_t num) const {
    return upb_enumdef_iton(ptr_, num);
  }

  /* Iteration over name/value pairs.  The order is undefined.
   * Adding an enum val invalidates any iterators.
   *
   * TODO: make compatible with range-for, with elements as pairs? */
  class Iterator {
   public:
    explicit Iterator(EnumDefPtr e) { upb_enum_begin(&iter_, e.ptr()); }

    int32_t number() { return upb_enum_iter_number(&iter_); }
    const char *name() { return upb_enum_iter_name(&iter_); }
    bool Done() { return upb_enum_done(&iter_); }
    void Next() { return upb_enum_next(&iter_); }

   private:
    upb_enum_iter iter_;
  };

 private:
  const upb_enumdef *ptr_;
};

inline upb::EnumDefPtr upb::FieldDefPtr::enum_subdef() const {
  return EnumDefPtr(upb_fielddef_enumsubdef(ptr_));
}

#endif  /* __cplusplus */

/* upb_filedef ****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}  /* extern "C" */

/* Class that represents a .proto file with some things defined in it.
 *
 * Many users won't care about FileDefs, but they are necessary if you want to
 * read the values of file-level options. */
class upb::FileDefPtr {
 public:
  explicit FileDefPtr(const upb_filedef *ptr) : ptr_(ptr) {}

  const upb_filedef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  /* Get/set name of the file (eg. "foo/bar.proto"). */
  const char* name() const { return upb_filedef_name(ptr_); }

  /* Package name for definitions inside the file (eg. "foo.bar"). */
  const char* package() const { return upb_filedef_package(ptr_); }

  /* Sets the php class prefix which is prepended to all php generated classes
   * from this .proto. Default is empty. */
  const char* phpprefix() const { return upb_filedef_phpprefix(ptr_); }

  /* Use this option to change the namespace of php generated classes. Default
   * is empty. When this option is empty, the package name will be used for
   * determining the namespace. */
  const char* phpnamespace() const { return upb_filedef_phpnamespace(ptr_); }

  /* Syntax for the file.  Defaults to proto2. */
  upb_syntax_t syntax() const { return upb_filedef_syntax(ptr_); }

  /* Get the list of dependencies from the file.  These are returned in the
   * order that they were added to the FileDefPtr. */
  int dependency_count() const { return upb_filedef_depcount(ptr_); }
  const FileDefPtr dependency(int index) const {
    return FileDefPtr(upb_filedef_dep(ptr_, index));
  }

 private:
  const upb_filedef* ptr_;
};

#endif  /* __cplusplus */

/* upb_symtab *****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

upb_symtab *upb_symtab_new(void);
void upb_symtab_free(upb_symtab* s);
const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym);
const upb_msgdef *upb_symtab_lookupmsg2(
    const upb_symtab *s, const char *sym, size_t len);
const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym);
const upb_filedef *upb_symtab_lookupfile(const upb_symtab *s, const char *name);
int upb_symtab_filecount(const upb_symtab *s);
const upb_filedef *upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file,
    upb_status *status);

/* For generated code only: loads a generated descriptor. */
typedef struct upb_def_init {
  struct upb_def_init **deps;
  const char *filename;
  upb_strview descriptor;
} upb_def_init;

bool _upb_symtab_loaddefinit(upb_symtab *s, const upb_def_init *init);

#ifdef __cplusplus
}  /* extern "C" */

/* Non-const methods in upb::SymbolTable are NOT thread-safe. */
class upb::SymbolTable {
 public:
  SymbolTable() : ptr_(upb_symtab_new(), upb_symtab_free) {}
  explicit SymbolTable(upb_symtab* s) : ptr_(s, upb_symtab_free) {}

  const upb_symtab* ptr() const { return ptr_.get(); }
  upb_symtab* ptr() { return ptr_.get(); }

  /* Finds an entry in the symbol table with this exact name.  If not found,
   * returns NULL. */
  MessageDefPtr LookupMessage(const char *sym) const {
    return MessageDefPtr(upb_symtab_lookupmsg(ptr_.get(), sym));
  }

  EnumDefPtr LookupEnum(const char *sym) const {
    return EnumDefPtr(upb_symtab_lookupenum(ptr_.get(), sym));
  }

  FileDefPtr LookupFile(const char *name) const {
    return FileDefPtr(upb_symtab_lookupfile(ptr_.get(), name));
  }

  /* TODO: iteration? */

  /* Adds the given serialized FileDescriptorProto to the pool. */
  FileDefPtr AddFile(const google_protobuf_FileDescriptorProto *file_proto,
                     Status *status) {
    return FileDefPtr(
        upb_symtab_addfile(ptr_.get(), file_proto, status->ptr()));
  }

 private:
  std::unique_ptr<upb_symtab, decltype(&upb_symtab_free)> ptr_;
};

UPB_INLINE const char* upb_safecstr(const std::string& str) {
  UPB_ASSERT(str.size() == std::strlen(str.c_str()));
  return str.c_str();
}

#endif  /* __cplusplus */

#include "upb/port_undef.inc"

#endif /* UPB_DEF_H_ */
