// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_SYMBOL_H__
#define GOOGLE_PROTOBUF_SYMBOL_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace internal {

// Symbol is a variant-like wrapper class that holds a pointer to any type of
// Protobuf entity during the descriptor building process. It allows the
// DescriptorBuilder and DescriptorPool to store and look up different
// descriptor types (e.g., Message, Field, Enum, Service) uniformly in their
// internal hash tables. It relies on the `SymbolBase` classes to
// store the type enum.
class Symbol {
 public:
  enum Type {
    NULL_SYMBOL,
    MESSAGE,
    FIELD,
    ONEOF,
    ENUM,
    ENUM_VALUE,
    ENUM_VALUE_OTHER_PARENT,
    SERVICE,
    METHOD,
    FULL_PACKAGE,
    SUB_PACKAGE,
  };

  Symbol() {
    static constexpr SymbolBase null_symbol{};
    static_assert(null_symbol.symbol_type_ == NULL_SYMBOL, "");
    // Initialize with a sentinel to make sure `ptr_` is never null.
    ptr_ = &null_symbol;
  }

  explicit Symbol(const SymbolBase* ptr) : ptr_(ptr) {}

  // Every object we store derives from SymbolBase, where we store the
  // symbol type enum.
  // Storing in the object can be done without using more space in most cases,
  // while storing it in the Symbol type would require 8 bytes.
#define DEFINE_MEMBERS(TYPE, TYPE_CONSTANT, FIELD)                             \
  explicit Symbol(TYPE* value) : ptr_(value) {                                 \
    value->symbol_type_ = TYPE_CONSTANT;                                       \
  }                                                                            \
  const TYPE* FIELD() const {                                                  \
    return type() == TYPE_CONSTANT ? static_cast<const TYPE*>(ptr_) : nullptr; \
  }

  DEFINE_MEMBERS(Descriptor, MESSAGE, descriptor)
  DEFINE_MEMBERS(FieldDescriptor, FIELD, field_descriptor)
  DEFINE_MEMBERS(OneofDescriptor, ONEOF, oneof_descriptor)
  DEFINE_MEMBERS(EnumDescriptor, ENUM, enum_descriptor)
  DEFINE_MEMBERS(ServiceDescriptor, SERVICE, service_descriptor)
  DEFINE_MEMBERS(MethodDescriptor, METHOD, method_descriptor)
  DEFINE_MEMBERS(FileDescriptor, FULL_PACKAGE, file_descriptor)

  // We use a special node for subpackage FileDescriptor.
  // It is potentially added to the table with multiple different names, so we
  // need a separate place to put the name.
  struct Subpackage : SymbolBase {
    int name_size;
    const FileDescriptor* file;
  };
  DEFINE_MEMBERS(Subpackage, SUB_PACKAGE, sub_package_file_descriptor)

  // Enum values have two different parents.
  // We use two different identities for the same object to determine the two
  // different insertions in the map.
  static Symbol EnumValue(EnumValueDescriptor* value, int n) {
    Symbol s;
    SymbolBase* ptr;
    if (n == 0) {
      ptr = static_cast<SymbolBaseN<0>*>(value);
      ptr->symbol_type_ = ENUM_VALUE;
    } else {
      ptr = static_cast<SymbolBaseN<1>*>(value);
      ptr->symbol_type_ = ENUM_VALUE_OTHER_PARENT;
    }
    s.ptr_ = ptr;
    return s;
  }

  const EnumValueDescriptor* enum_value_descriptor() const {
    return type() == ENUM_VALUE ? static_cast<const EnumValueDescriptor*>(
                                      static_cast<const SymbolBaseN<0>*>(ptr_))
           : type() == ENUM_VALUE_OTHER_PARENT
               ? static_cast<const EnumValueDescriptor*>(
                     static_cast<const SymbolBaseN<1>*>(ptr_))
               : nullptr;
  }

#undef DEFINE_MEMBERS

  // Returns the type of the underlying descriptor.
  Type type() const { return static_cast<Type>(ptr_->symbol_type_); }

  // Returns true if this Symbol does not point to any valid descriptor.
  bool IsNull() const { return type() == NULL_SYMBOL; }

  // Returns true if this Symbol represents a type definition (Message or Enum).
  bool IsType() const { return type() == MESSAGE || type() == ENUM; }

  // Returns true if this Symbol represents an entity that can contain other
  // entities (Message, Enum, Package, or Service).
  bool IsAggregate() const {
    return IsType() || IsPackage() || type() == SERVICE;
  }

  // Returns true if this Symbol represents a package (full or sub-package).
  bool IsPackage() const {
    return type() == FULL_PACKAGE || type() == SUB_PACKAGE;
  }

  // Returns the FileDescriptor where this symbol is defined, or nullptr if
  // it is not associated with a specific file (e.g., NULL_SYMBOL).
  const FileDescriptor* GetFile() const;

  // Returns the fully qualified name of the symbol (e.g., "foo.bar.MyMessage").
  absl::string_view full_name() const;

  // Returns a unique key used for hashing symbols by their parent entity
  // and their local name.
  std::pair<const void*, absl::string_view> parent_name_key() const;

  // Returns the resolved FeatureSet applied to this symbol.
  const FeatureSet& features() const;

  // Returns true if this symbol is a placeholder (an unresolved dependency).
  bool is_placeholder() const;

  // Returns the explicit visibility keyword applied to this symbol, if any.
  SymbolVisibility visibility_keyword() const;

  // Returns true if this symbol is defined within another type (e.g., a nested
  // message or enum).
  bool IsNestedDefinition() const;

  // Calculates and returns the effective visibility of this symbol, taking
  // into account its explicit keyword, its features, and its nesting level.
  SymbolVisibility GetEffectiveVisibility() const;

  // Determines whether this symbol can be referenced from the given file,
  // respecting protobuf visibility rules.
  bool IsVisibleFrom(FileDescriptor* other) const;

  // Returns a detailed error message explaining why this symbol is not
  // visible from the given file.
  std::string GetVisibilityError(FileDescriptor* other,
                                 absl::string_view usage = "") const;

  const SymbolBase* ptr() const { return ptr_; }

 private:
  const SymbolBase* ptr_;
};

// A DescriptorPool contains a bunch of hash-maps to implement the
// various Find*By*() methods.  Since hashtable lookups are O(1), it's
// most efficient to construct a fixed set of large hash-maps used by
// all objects in the pool rather than construct one or more small
// hash-maps for each object.
//
// The keys to these hash-maps are (parent, name) or (parent, number) pairs.

// A query object used to find a Symbol given its fully qualified name.
struct FullNameQuery {
  absl::string_view query;
  absl::string_view full_name() const { return query; }
};

// Hash functor for Symbol, based on its fully qualified name.
struct SymbolByFullNameHash {
  using is_transparent = void;

  template <typename T>
  size_t operator()(const T& s) const {
    return absl::HashOf(s.full_name());
  }
};

// Equality functor for Symbol, based on its fully qualified name.
struct SymbolByFullNameEq {
  using is_transparent = void;

  template <typename T, typename U>
  bool operator()(const T& a, const U& b) const {
    return a.full_name() == b.full_name();
  }
};

// A set of Symbols, hashed and compared by their fully qualified name.
using SymbolsByNameSet =
    absl::flat_hash_set<Symbol, SymbolByFullNameHash, SymbolByFullNameEq>;

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_SYMBOL_H__
