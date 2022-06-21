// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef UPB_DEF_HPP_
#define UPB_DEF_HPP_

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "upb/def.h"
#include "upb/reflection.h"
#include "upb/upb.hpp"

namespace upb {

typedef upb_MessageValue MessageValue;

class EnumDefPtr;
class FileDefPtr;
class MessageDefPtr;
class OneofDefPtr;

// A upb::FieldDefPtr describes a single field in a message.  It is most often
// found as a part of a upb_MessageDef, but can also stand alone to represent
// an extension.
class FieldDefPtr {
 public:
  FieldDefPtr() : ptr_(nullptr) {}
  explicit FieldDefPtr(const upb_FieldDef* ptr) : ptr_(ptr) {}

  const upb_FieldDef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  typedef upb_CType Type;
  typedef upb_Label Label;
  typedef upb_FieldType DescriptorType;

  const char* full_name() const { return upb_FieldDef_FullName(ptr_); }

  Type type() const { return upb_FieldDef_CType(ptr_); }
  Label label() const { return upb_FieldDef_Label(ptr_); }
  const char* name() const { return upb_FieldDef_Name(ptr_); }
  const char* json_name() const { return upb_FieldDef_JsonName(ptr_); }
  uint32_t number() const { return upb_FieldDef_Number(ptr_); }
  bool is_extension() const { return upb_FieldDef_IsExtension(ptr_); }

  // For non-string, non-submessage fields, this indicates whether binary
  // protobufs are encoded in packed or non-packed format.
  //
  // Note: this accessor reflects the fact that "packed" has different defaults
  // depending on whether the proto is proto2 or proto3.
  bool packed() const { return upb_FieldDef_IsPacked(ptr_); }

  // An integer that can be used as an index into an array of fields for
  // whatever message this field belongs to.  Guaranteed to be less than
  // f->containing_type()->field_count().  May only be accessed once the def has
  // been finalized.
  uint32_t index() const { return upb_FieldDef_Index(ptr_); }

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
  DescriptorType descriptor_type() const { return upb_FieldDef_Type(ptr_); }

  // Convenient field type tests.
  bool IsSubMessage() const { return upb_FieldDef_IsSubMessage(ptr_); }
  bool IsString() const { return upb_FieldDef_IsString(ptr_); }
  bool IsSequence() const { return upb_FieldDef_IsRepeated(ptr_); }
  bool IsPrimitive() const { return upb_FieldDef_IsPrimitive(ptr_); }
  bool IsMap() const { return upb_FieldDef_IsMap(ptr_); }

  MessageValue default_value() const { return upb_FieldDef_Default(ptr_); }

  // Returns the enum or submessage def for this field, if any.  The field's
  // type must match (ie. you may only call enum_subdef() for fields where
  // type() == kUpb_CType_Enum).
  EnumDefPtr enum_subdef() const;
  MessageDefPtr message_subdef() const;

 private:
  const upb_FieldDef* ptr_;
};

// Class that represents a oneof.
class OneofDefPtr {
 public:
  OneofDefPtr() : ptr_(nullptr) {}
  explicit OneofDefPtr(const upb_OneofDef* ptr) : ptr_(ptr) {}

  const upb_OneofDef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  // Returns the MessageDef that contains this OneofDef.
  MessageDefPtr containing_type() const;

  // Returns the name of this oneof.
  const char* name() const { return upb_OneofDef_Name(ptr_); }

  // Returns the number of fields in the oneof.
  int field_count() const { return upb_OneofDef_FieldCount(ptr_); }
  FieldDefPtr field(int i) const {
    return FieldDefPtr(upb_OneofDef_Field(ptr_, i));
  }

  // Looks up by name.
  FieldDefPtr FindFieldByName(const char* name, size_t len) const {
    return FieldDefPtr(upb_OneofDef_LookupNameWithSize(ptr_, name, len));
  }
  FieldDefPtr FindFieldByName(const char* name) const {
    return FieldDefPtr(upb_OneofDef_LookupName(ptr_, name));
  }

  template <class T>
  FieldDefPtr FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  // Looks up by tag number.
  FieldDefPtr FindFieldByNumber(uint32_t num) const {
    return FieldDefPtr(upb_OneofDef_LookupNumber(ptr_, num));
  }

 private:
  const upb_OneofDef* ptr_;
};

// Structure that describes a single .proto message type.
class MessageDefPtr {
 public:
  MessageDefPtr() : ptr_(nullptr) {}
  explicit MessageDefPtr(const upb_MessageDef* ptr) : ptr_(ptr) {}

  const upb_MessageDef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  FileDefPtr file() const;

  const char* full_name() const { return upb_MessageDef_FullName(ptr_); }
  const char* name() const { return upb_MessageDef_Name(ptr_); }

  // The number of fields that belong to the MessageDef.
  int field_count() const { return upb_MessageDef_FieldCount(ptr_); }
  FieldDefPtr field(int i) const {
    return FieldDefPtr(upb_MessageDef_Field(ptr_, i));
  }

  // The number of oneofs that belong to the MessageDef.
  int oneof_count() const { return upb_MessageDef_OneofCount(ptr_); }
  OneofDefPtr oneof(int i) const {
    return OneofDefPtr(upb_MessageDef_Oneof(ptr_, i));
  }

  upb_Syntax syntax() const { return upb_MessageDef_Syntax(ptr_); }

  // These return null pointers if the field is not found.
  FieldDefPtr FindFieldByNumber(uint32_t number) const {
    return FieldDefPtr(upb_MessageDef_FindFieldByNumber(ptr_, number));
  }
  FieldDefPtr FindFieldByName(const char* name, size_t len) const {
    return FieldDefPtr(upb_MessageDef_FindFieldByNameWithSize(ptr_, name, len));
  }
  FieldDefPtr FindFieldByName(const char* name) const {
    return FieldDefPtr(upb_MessageDef_FindFieldByName(ptr_, name));
  }

  template <class T>
  FieldDefPtr FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  OneofDefPtr FindOneofByName(const char* name, size_t len) const {
    return OneofDefPtr(upb_MessageDef_FindOneofByNameWithSize(ptr_, name, len));
  }

  OneofDefPtr FindOneofByName(const char* name) const {
    return OneofDefPtr(upb_MessageDef_FindOneofByName(ptr_, name));
  }

  template <class T>
  OneofDefPtr FindOneofByName(const T& str) const {
    return FindOneofByName(str.c_str(), str.size());
  }

  // Is this message a map entry?
  bool mapentry() const { return upb_MessageDef_IsMapEntry(ptr_); }

  // Return the type of well known type message. kUpb_WellKnown_Unspecified for
  // non-well-known message.
  upb_WellKnown wellknowntype() const {
    return upb_MessageDef_WellKnownType(ptr_);
  }

 private:
  class FieldIter {
   public:
    explicit FieldIter(const upb_MessageDef* m, int i) : m_(m), i_(i) {}
    void operator++() { i_++; }

    FieldDefPtr operator*() {
      return FieldDefPtr(upb_MessageDef_Field(m_, i_));
    }
    bool operator!=(const FieldIter& other) { return i_ != other.i_; }
    bool operator==(const FieldIter& other) { return i_ == other.i_; }

   private:
    const upb_MessageDef* m_;
    int i_;
  };

  class FieldAccessor {
   public:
    explicit FieldAccessor(const upb_MessageDef* md) : md_(md) {}
    FieldIter begin() { return FieldIter(md_, 0); }
    FieldIter end() { return FieldIter(md_, upb_MessageDef_FieldCount(md_)); }

   private:
    const upb_MessageDef* md_;
  };

  class OneofIter {
   public:
    explicit OneofIter(const upb_MessageDef* m, int i) : m_(m), i_(i) {}
    void operator++() { i_++; }

    OneofDefPtr operator*() {
      return OneofDefPtr(upb_MessageDef_Oneof(m_, i_));
    }
    bool operator!=(const OneofIter& other) { return i_ != other.i_; }
    bool operator==(const OneofIter& other) { return i_ == other.i_; }

   private:
    const upb_MessageDef* m_;
    int i_;
  };

  class OneofAccessor {
   public:
    explicit OneofAccessor(const upb_MessageDef* md) : md_(md) {}
    OneofIter begin() { return OneofIter(md_, 0); }
    OneofIter end() { return OneofIter(md_, upb_MessageDef_OneofCount(md_)); }

   private:
    const upb_MessageDef* md_;
  };

 public:
  FieldAccessor fields() const { return FieldAccessor(ptr()); }
  OneofAccessor oneofs() const { return OneofAccessor(ptr()); }

 private:
  const upb_MessageDef* ptr_;
};

class EnumValDefPtr {
 public:
  EnumValDefPtr() : ptr_(nullptr) {}
  explicit EnumValDefPtr(const upb_EnumValueDef* ptr) : ptr_(ptr) {}

  int32_t number() const { return upb_EnumValueDef_Number(ptr_); }
  const char* full_name() const { return upb_EnumValueDef_FullName(ptr_); }
  const char* name() const { return upb_EnumValueDef_Name(ptr_); }

 private:
  const upb_EnumValueDef* ptr_;
};

class EnumDefPtr {
 public:
  EnumDefPtr() : ptr_(nullptr) {}
  explicit EnumDefPtr(const upb_EnumDef* ptr) : ptr_(ptr) {}

  const upb_EnumDef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  const char* full_name() const { return upb_EnumDef_FullName(ptr_); }
  const char* name() const { return upb_EnumDef_Name(ptr_); }

  // The value that is used as the default when no field default is specified.
  // If not set explicitly, the first value that was added will be used.
  // The default value must be a member of the enum.
  // Requires that value_count() > 0.
  int32_t default_value() const { return upb_EnumDef_Default(ptr_); }

  // Returns the number of values currently defined in the enum.  Note that
  // multiple names can refer to the same number, so this may be greater than
  // the total number of unique numbers.
  int value_count() const { return upb_EnumDef_ValueCount(ptr_); }

  // Lookups from name to integer, returning true if found.
  EnumValDefPtr FindValueByName(const char* name) const {
    return EnumValDefPtr(upb_EnumDef_FindValueByName(ptr_, name));
  }

  // Finds the name corresponding to the given number, or NULL if none was
  // found.  If more than one name corresponds to this number, returns the
  // first one that was added.
  EnumValDefPtr FindValueByNumber(int32_t num) const {
    return EnumValDefPtr(upb_EnumDef_FindValueByNumber(ptr_, num));
  }

 private:
  const upb_EnumDef* ptr_;
};

// Class that represents a .proto file with some things defined in it.
//
// Many users won't care about FileDefs, but they are necessary if you want to
// read the values of file-level options.
class FileDefPtr {
 public:
  explicit FileDefPtr(const upb_FileDef* ptr) : ptr_(ptr) {}

  const upb_FileDef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  // Get/set name of the file (eg. "foo/bar.proto").
  const char* name() const { return upb_FileDef_Name(ptr_); }

  // Package name for definitions inside the file (eg. "foo.bar").
  const char* package() const { return upb_FileDef_Package(ptr_); }

  // Syntax for the file.  Defaults to proto2.
  upb_Syntax syntax() const { return upb_FileDef_Syntax(ptr_); }

  // Get the list of dependencies from the file.  These are returned in the
  // order that they were added to the FileDefPtr.
  int dependency_count() const { return upb_FileDef_DependencyCount(ptr_); }
  const FileDefPtr dependency(int index) const {
    return FileDefPtr(upb_FileDef_Dependency(ptr_, index));
  }

 private:
  const upb_FileDef* ptr_;
};

// Non-const methods in upb::DefPool are NOT thread-safe.
class DefPool {
 public:
  DefPool() : ptr_(upb_DefPool_New(), upb_DefPool_Free) {}
  explicit DefPool(upb_DefPool* s) : ptr_(s, upb_DefPool_Free) {}

  const upb_DefPool* ptr() const { return ptr_.get(); }
  upb_DefPool* ptr() { return ptr_.get(); }

  // Finds an entry in the symbol table with this exact name.  If not found,
  // returns NULL.
  MessageDefPtr FindMessageByName(const char* sym) const {
    return MessageDefPtr(upb_DefPool_FindMessageByName(ptr_.get(), sym));
  }

  EnumDefPtr FindEnumByName(const char* sym) const {
    return EnumDefPtr(upb_DefPool_FindEnumByName(ptr_.get(), sym));
  }

  FileDefPtr FindFileByName(const char* name) const {
    return FileDefPtr(upb_DefPool_FindFileByName(ptr_.get(), name));
  }

  // TODO: iteration?

  // Adds the given serialized FileDescriptorProto to the pool.
  FileDefPtr AddFile(const google_protobuf_FileDescriptorProto* file_proto,
                     Status* status) {
    return FileDefPtr(
        upb_DefPool_AddFile(ptr_.get(), file_proto, status->ptr()));
  }

 private:
  std::unique_ptr<upb_DefPool, decltype(&upb_DefPool_Free)> ptr_;
};

// TODO(b/236632406): This typedef is deprecated. Delete it.
using SymbolTable = DefPool;

inline FileDefPtr MessageDefPtr::file() const {
  return FileDefPtr(upb_MessageDef_File(ptr_));
}

inline MessageDefPtr FieldDefPtr::message_subdef() const {
  return MessageDefPtr(upb_FieldDef_MessageSubDef(ptr_));
}

inline MessageDefPtr FieldDefPtr::containing_type() const {
  return MessageDefPtr(upb_FieldDef_ContainingType(ptr_));
}

inline MessageDefPtr OneofDefPtr::containing_type() const {
  return MessageDefPtr(upb_OneofDef_ContainingType(ptr_));
}

inline OneofDefPtr FieldDefPtr::containing_oneof() const {
  return OneofDefPtr(upb_FieldDef_ContainingOneof(ptr_));
}

inline EnumDefPtr FieldDefPtr::enum_subdef() const {
  return EnumDefPtr(upb_FieldDef_EnumSubDef(ptr_));
}

}  // namespace upb

#endif  // UPB_DEF_HPP_
