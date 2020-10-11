
#ifndef UPB_DEF_HPP_
#define UPB_DEF_HPP_

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "upb/def.h"
#include "upb/upb.hpp"

namespace upb {

class EnumDefPtr;
class MessageDefPtr;
class OneofDefPtr;

// A upb::FieldDefPtr describes a single field in a message.  It is most often
// found as a part of a upb_msgdef, but can also stand alone to represent
// an extension.
class FieldDefPtr {
 public:
  FieldDefPtr() : ptr_(nullptr) {}
  explicit FieldDefPtr(const upb_fielddef* ptr) : ptr_(ptr) {}

  const upb_fielddef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  typedef upb_fieldtype_t Type;
  typedef upb_label_t Label;
  typedef upb_descriptortype_t DescriptorType;

  const char* full_name() const { return upb_fielddef_fullname(ptr_); }

  Type type() const { return upb_fielddef_type(ptr_); }
  Label label() const { return upb_fielddef_label(ptr_); }
  const char* name() const { return upb_fielddef_name(ptr_); }
  const char* json_name() const { return upb_fielddef_jsonname(ptr_); }
  uint32_t number() const { return upb_fielddef_number(ptr_); }
  bool is_extension() const { return upb_fielddef_isextension(ptr_); }

  // For UPB_TYPE_MESSAGE fields only where is_tag_delimited() == false,
  // indicates whether this field should have lazy parsing handlers that yield
  // the unparsed string for the submessage.
  //
  // TODO(haberman): I think we want to move this into a FieldOptions container
  // when we add support for custom options (the FieldOptions struct will
  // contain both regular FieldOptions like "lazy" *and* custom options).
  bool lazy() const { return upb_fielddef_lazy(ptr_); }

  // For non-string, non-submessage fields, this indicates whether binary
  // protobufs are encoded in packed or non-packed format.
  //
  // TODO(haberman): see note above about putting options like this into a
  // FieldOptions container.
  bool packed() const { return upb_fielddef_packed(ptr_); }

  // An integer that can be used as an index into an array of fields for
  // whatever message this field belongs to.  Guaranteed to be less than
  // f->containing_type()->field_count().  May only be accessed once the def has
  // been finalized.
  uint32_t index() const { return upb_fielddef_index(ptr_); }

  // The MessageDef to which this field belongs.
  //
  // If this field has been added to a MessageDef, that message can be retrieved
  // directly (this is always the case for frozen FieldDefs).
  //
  // If the field has not yet been added to a MessageDef, you can set the name
  // of the containing type symbolically instead.  This is mostly useful for
  // extensions, where the extension is declared separately from the message.
  MessageDefPtr containing_type() const;

  // The OneofDef to which this field belongs, or NULL if this field is not part
  // of a oneof.
  OneofDefPtr containing_oneof() const;

  // The field's type according to the enum in descriptor.proto.  This is not
  // the same as UPB_TYPE_*, because it distinguishes between (for example)
  // INT32 and SINT32, whereas our "type" enum does not.  This return of
  // descriptor_type() is a function of type(), integer_format(), and
  // is_tag_delimited().
  DescriptorType descriptor_type() const {
    return upb_fielddef_descriptortype(ptr_);
  }

  // Convenient field type tests.
  bool IsSubMessage() const { return upb_fielddef_issubmsg(ptr_); }
  bool IsString() const { return upb_fielddef_isstring(ptr_); }
  bool IsSequence() const { return upb_fielddef_isseq(ptr_); }
  bool IsPrimitive() const { return upb_fielddef_isprimitive(ptr_); }
  bool IsMap() const { return upb_fielddef_ismap(ptr_); }

  // Returns the non-string default value for this fielddef, which may either
  // be something the client set explicitly or the "default default" (0 for
  // numbers, empty for strings).  The field's type indicates the type of the
  // returned value, except for enum fields that are still mutable.
  //
  // Requires that the given function matches the field's current type.
  int64_t default_int64() const { return upb_fielddef_defaultint64(ptr_); }
  int32_t default_int32() const { return upb_fielddef_defaultint32(ptr_); }
  uint64_t default_uint64() const { return upb_fielddef_defaultuint64(ptr_); }
  uint32_t default_uint32() const { return upb_fielddef_defaultuint32(ptr_); }
  bool default_bool() const { return upb_fielddef_defaultbool(ptr_); }
  float default_float() const { return upb_fielddef_defaultfloat(ptr_); }
  double default_double() const { return upb_fielddef_defaultdouble(ptr_); }

  // The resulting string is always NULL-terminated.  If non-NULL, the length
  // will be stored in *len.
  const char* default_string(size_t* len) const {
    return upb_fielddef_defaultstr(ptr_, len);
  }

  // Returns the enum or submessage def for this field, if any.  The field's
  // type must match (ie. you may only call enum_subdef() for fields where
  // type() == UPB_TYPE_ENUM).
  EnumDefPtr enum_subdef() const;
  MessageDefPtr message_subdef() const;

 private:
  const upb_fielddef* ptr_;
};

// Class that represents a oneof.
class OneofDefPtr {
 public:
  OneofDefPtr() : ptr_(nullptr) {}
  explicit OneofDefPtr(const upb_oneofdef* ptr) : ptr_(ptr) {}

  const upb_oneofdef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  // Returns the MessageDef that contains this OneofDef.
  MessageDefPtr containing_type() const;

  // Returns the name of this oneof.
  const char* name() const { return upb_oneofdef_name(ptr_); }

  // Returns the number of fields in the oneof.
  int field_count() const { return upb_oneofdef_numfields(ptr_); }
  FieldDefPtr field(int i) const { return FieldDefPtr(upb_oneofdef_field(ptr_, i)); }

  // Looks up by name.
  FieldDefPtr FindFieldByName(const char* name, size_t len) const {
    return FieldDefPtr(upb_oneofdef_ntof(ptr_, name, len));
  }
  FieldDefPtr FindFieldByName(const char* name) const {
    return FieldDefPtr(upb_oneofdef_ntofz(ptr_, name));
  }

  template <class T>
  FieldDefPtr FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  // Looks up by tag number.
  FieldDefPtr FindFieldByNumber(uint32_t num) const {
    return FieldDefPtr(upb_oneofdef_itof(ptr_, num));
  }

 private:
  const upb_oneofdef* ptr_;
};

// Structure that describes a single .proto message type.
class MessageDefPtr {
 public:
  MessageDefPtr() : ptr_(nullptr) {}
  explicit MessageDefPtr(const upb_msgdef* ptr) : ptr_(ptr) {}

  const upb_msgdef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  const char* full_name() const { return upb_msgdef_fullname(ptr_); }
  const char* name() const { return upb_msgdef_name(ptr_); }

  // The number of fields that belong to the MessageDef.
  int field_count() const { return upb_msgdef_numfields(ptr_); }
  FieldDefPtr field(int i) const { return FieldDefPtr(upb_msgdef_field(ptr_, i)); }

  // The number of oneofs that belong to the MessageDef.
  int oneof_count() const { return upb_msgdef_numoneofs(ptr_); }
  OneofDefPtr oneof(int i) const { return OneofDefPtr(upb_msgdef_oneof(ptr_, i)); }

  upb_syntax_t syntax() const { return upb_msgdef_syntax(ptr_); }

  // These return null pointers if the field is not found.
  FieldDefPtr FindFieldByNumber(uint32_t number) const {
    return FieldDefPtr(upb_msgdef_itof(ptr_, number));
  }
  FieldDefPtr FindFieldByName(const char* name, size_t len) const {
    return FieldDefPtr(upb_msgdef_ntof(ptr_, name, len));
  }
  FieldDefPtr FindFieldByName(const char* name) const {
    return FieldDefPtr(upb_msgdef_ntofz(ptr_, name));
  }

  template <class T>
  FieldDefPtr FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  OneofDefPtr FindOneofByName(const char* name, size_t len) const {
    return OneofDefPtr(upb_msgdef_ntoo(ptr_, name, len));
  }

  OneofDefPtr FindOneofByName(const char* name) const {
    return OneofDefPtr(upb_msgdef_ntooz(ptr_, name));
  }

  template <class T>
  OneofDefPtr FindOneofByName(const T& str) const {
    return FindOneofByName(str.c_str(), str.size());
  }

  // Is this message a map entry?
  bool mapentry() const { return upb_msgdef_mapentry(ptr_); }

  // Return the type of well known type message. UPB_WELLKNOWN_UNSPECIFIED for
  // non-well-known message.
  upb_wellknowntype_t wellknowntype() const {
    return upb_msgdef_wellknowntype(ptr_);
  }

  // Whether is a number wrapper.
  bool isnumberwrapper() const { return upb_msgdef_isnumberwrapper(ptr_); }

 private:
  class FieldIter {
   public:
    explicit FieldIter(const upb_msgdef *m, int i) : m_(m), i_(i) {}
    void operator++() { i_++; }

    FieldDefPtr operator*() { return FieldDefPtr(upb_msgdef_field(m_, i_)); }
    bool operator!=(const FieldIter& other) { return i_ != other.i_; }
    bool operator==(const FieldIter& other) { return i_ == other.i_; }

   private:
    const upb_msgdef *m_;
    int i_;
  };

  class FieldAccessor {
   public:
    explicit FieldAccessor(const upb_msgdef* md) : md_(md) {}
    FieldIter begin() { return FieldIter(md_, 0); }
    FieldIter end() { return FieldIter(md_, upb_msgdef_fieldcount(md_)); }

   private:
    const upb_msgdef* md_;
  };

  class OneofIter {
   public:
    explicit OneofIter(const upb_msgdef *m, int i) : m_(m), i_(i) {}
    void operator++() { i_++; }

    OneofDefPtr operator*() { return OneofDefPtr(upb_msgdef_oneof(m_, i_)); }
    bool operator!=(const OneofIter& other) { return i_ != other.i_; }
    bool operator==(const OneofIter& other) { return i_ == other.i_; }

   private:
    const upb_msgdef *m_;
    int i_;
  };

  class OneofAccessor {
   public:
    explicit OneofAccessor(const upb_msgdef* md) : md_(md) {}
    OneofIter begin() { return OneofIter(md_, 0); }
    OneofIter end() { return OneofIter(md_, upb_msgdef_oneofcount(md_)); }

   private:
    const upb_msgdef* md_;
  };

 public:
  FieldAccessor fields() const { return FieldAccessor(ptr()); }
  OneofAccessor oneofs() const { return OneofAccessor(ptr()); }

 private:
  const upb_msgdef* ptr_;
};

class EnumDefPtr {
 public:
  EnumDefPtr() : ptr_(nullptr) {}
  explicit EnumDefPtr(const upb_enumdef* ptr) : ptr_(ptr) {}

  const upb_enumdef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  const char* full_name() const { return upb_enumdef_fullname(ptr_); }
  const char* name() const { return upb_enumdef_name(ptr_); }

  // The value that is used as the default when no field default is specified.
  // If not set explicitly, the first value that was added will be used.
  // The default value must be a member of the enum.
  // Requires that value_count() > 0.
  int32_t default_value() const { return upb_enumdef_default(ptr_); }

  // Returns the number of values currently defined in the enum.  Note that
  // multiple names can refer to the same number, so this may be greater than
  // the total number of unique numbers.
  int value_count() const { return upb_enumdef_numvals(ptr_); }

  // Lookups from name to integer, returning true if found.
  bool FindValueByName(const char* name, int32_t* num) const {
    return upb_enumdef_ntoiz(ptr_, name, num);
  }

  // Finds the name corresponding to the given number, or NULL if none was
  // found.  If more than one name corresponds to this number, returns the
  // first one that was added.
  const char* FindValueByNumber(int32_t num) const {
    return upb_enumdef_iton(ptr_, num);
  }

  // Iteration over name/value pairs.  The order is undefined.
  // Adding an enum val invalidates any iterators.
  //
  // TODO: make compatible with range-for, with elements as pairs?
  class Iterator {
   public:
    explicit Iterator(EnumDefPtr e) { upb_enum_begin(&iter_, e.ptr()); }

    int32_t number() { return upb_enum_iter_number(&iter_); }
    const char* name() { return upb_enum_iter_name(&iter_); }
    bool Done() { return upb_enum_done(&iter_); }
    void Next() { return upb_enum_next(&iter_); }

   private:
    upb_enum_iter iter_;
  };

 private:
  const upb_enumdef* ptr_;
};

// Class that represents a .proto file with some things defined in it.
//
// Many users won't care about FileDefs, but they are necessary if you want to
// read the values of file-level options.
class FileDefPtr {
 public:
  explicit FileDefPtr(const upb_filedef* ptr) : ptr_(ptr) {}

  const upb_filedef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  // Get/set name of the file (eg. "foo/bar.proto").
  const char* name() const { return upb_filedef_name(ptr_); }

  // Package name for definitions inside the file (eg. "foo.bar").
  const char* package() const { return upb_filedef_package(ptr_); }

  // Sets the php class prefix which is prepended to all php generated classes
  // from this .proto. Default is empty.
  const char* phpprefix() const { return upb_filedef_phpprefix(ptr_); }

  // Use this option to change the namespace of php generated classes. Default
  // is empty. When this option is empty, the package name will be used for
  // determining the namespace.
  const char* phpnamespace() const { return upb_filedef_phpnamespace(ptr_); }

  // Syntax for the file.  Defaults to proto2.
  upb_syntax_t syntax() const { return upb_filedef_syntax(ptr_); }

  // Get the list of dependencies from the file.  These are returned in the
  // order that they were added to the FileDefPtr.
  int dependency_count() const { return upb_filedef_depcount(ptr_); }
  const FileDefPtr dependency(int index) const {
    return FileDefPtr(upb_filedef_dep(ptr_, index));
  }

 private:
  const upb_filedef* ptr_;
};

// Non-const methods in upb::SymbolTable are NOT thread-safe.
class SymbolTable {
 public:
  SymbolTable() : ptr_(upb_symtab_new(), upb_symtab_free) {}
  explicit SymbolTable(upb_symtab* s) : ptr_(s, upb_symtab_free) {}

  const upb_symtab* ptr() const { return ptr_.get(); }
  upb_symtab* ptr() { return ptr_.get(); }

  // Finds an entry in the symbol table with this exact name.  If not found,
  // returns NULL.
  MessageDefPtr LookupMessage(const char* sym) const {
    return MessageDefPtr(upb_symtab_lookupmsg(ptr_.get(), sym));
  }

  EnumDefPtr LookupEnum(const char* sym) const {
    return EnumDefPtr(upb_symtab_lookupenum(ptr_.get(), sym));
  }

  FileDefPtr LookupFile(const char* name) const {
    return FileDefPtr(upb_symtab_lookupfile(ptr_.get(), name));
  }

  // TODO: iteration?

  // Adds the given serialized FileDescriptorProto to the pool.
  FileDefPtr AddFile(const google_protobuf_FileDescriptorProto* file_proto,
                     Status* status) {
    return FileDefPtr(
        upb_symtab_addfile(ptr_.get(), file_proto, status->ptr()));
  }

 private:
  std::unique_ptr<upb_symtab, decltype(&upb_symtab_free)> ptr_;
};

inline MessageDefPtr FieldDefPtr::message_subdef() const {
  return MessageDefPtr(upb_fielddef_msgsubdef(ptr_));
}

inline MessageDefPtr FieldDefPtr::containing_type() const {
  return MessageDefPtr(upb_fielddef_containingtype(ptr_));
}

inline MessageDefPtr OneofDefPtr::containing_type() const {
  return MessageDefPtr(upb_oneofdef_containingtype(ptr_));
}

inline OneofDefPtr FieldDefPtr::containing_oneof() const {
  return OneofDefPtr(upb_fielddef_containingoneof(ptr_));
}

inline EnumDefPtr FieldDefPtr::enum_subdef() const {
  return EnumDefPtr(upb_fielddef_enumsubdef(ptr_));
}

}  // namespace upb

#endif  // UPB_DEF_HPP_
