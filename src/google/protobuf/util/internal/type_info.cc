// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/util/internal/type_info.h"

#include <map>
#include <set>

#include "google/protobuf/type.pb.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/util/internal/utility.h"

namespace google {
namespace protobuf {
namespace util {
namespace converter {

namespace {
// A TypeInfo that looks up information provided by a TypeResolver.
class TypeInfoForTypeResolver : public TypeInfo {
 public:
  explicit TypeInfoForTypeResolver(TypeResolver* type_resolver)
      : type_resolver_(type_resolver) {}

  ~TypeInfoForTypeResolver() override {
    DeleteCachedTypes(&cached_types_);
    DeleteCachedTypes(&cached_enums_);
  }

  absl::StatusOr<const google::protobuf::Type*> ResolveTypeUrl(
      absl::string_view type_url) const override {
    auto it = cached_types_.find(type_url);
    if (it != cached_types_.end()) {
      return it->second;
    }
    // Stores the string value so it can be referenced using absl::string_view
    // in the cached_types_ map.
    const std::string& string_type_url =
        *string_storage_.insert(std::string(type_url)).first;
    std::unique_ptr<google::protobuf::Type> type(new google::protobuf::Type());
    absl::Status status =
        type_resolver_->ResolveMessageType(string_type_url, type.get());
    StatusOrType result =
        status.ok() ? StatusOrType(type.release()) : StatusOrType(status);
    cached_types_[string_type_url] = result;
    return result;
  }

  const google::protobuf::Type* GetTypeByTypeUrl(
      absl::string_view type_url) const override {
    StatusOrType result = ResolveTypeUrl(type_url);
    return result.ok() ? result.value() : NULL;
  }

  const google::protobuf::Enum* GetEnumByTypeUrl(
      absl::string_view type_url) const override {
    std::map<absl::string_view, StatusOrEnum>::iterator it =
        cached_enums_.find(type_url);
    if (it != cached_enums_.end()) {
      return it->second.ok() ? it->second.value() : NULL;
    }
    // Stores the string value so it can be referenced using absl::string_view
    // in the cached_enums_ map.
    const std::string& string_type_url =
        *string_storage_.insert(std::string(type_url)).first;
    std::unique_ptr<google::protobuf::Enum> enum_type(
        new google::protobuf::Enum());
    absl::Status status =
        type_resolver_->ResolveEnumType(string_type_url, enum_type.get());
    StatusOrEnum result =
        status.ok() ? StatusOrEnum(enum_type.release()) : StatusOrEnum(status);
    cached_enums_[string_type_url] = result;
    return result.ok() ? result.value() : NULL;
  }

  const google::protobuf::Field* FindField(
      const google::protobuf::Type* type,
      absl::string_view camel_case_name) const override {
    auto it = indexed_types_.find(type);
    const CamelCaseNameTable& camel_case_name_table =
        it == indexed_types_.end()
            ? PopulateNameLookupTable(type, &indexed_types_[type])
            : it->second;

    absl::string_view name = camel_case_name;
    auto cc_it = camel_case_name_table.find(name);
    if (cc_it != camel_case_name_table.end()) {
      name = cc_it->second;
    }
    return FindFieldInTypeOrNull(type, name);
  }

 private:
  typedef absl::StatusOr<const google::protobuf::Type*> StatusOrType;
  typedef absl::StatusOr<const google::protobuf::Enum*> StatusOrEnum;
  typedef std::map<absl::string_view, absl::string_view> CamelCaseNameTable;

  template <typename T>
  static void DeleteCachedTypes(std::map<absl::string_view, T>* cached_types) {
    for (typename std::map<absl::string_view, T>::iterator it =
             cached_types->begin();
         it != cached_types->end(); ++it) {
      if (it->second.ok()) {
        delete it->second.value();
      }
    }
  }

  const CamelCaseNameTable& PopulateNameLookupTable(
      const google::protobuf::Type* type,
      CamelCaseNameTable* camel_case_name_table) const {
    for (int i = 0; i < type->fields_size(); ++i) {
      const google::protobuf::Field& field = type->fields(i);
      absl::string_view existing =
          camel_case_name_table->insert({field.json_name(), field.name()})
              .first->second;
      if (existing != field.name()) {
        GOOGLE_LOG(WARNING) << "Field '" << field.name() << "' and '" << existing
                     << "' map to the same camel case name '"
                     << field.json_name() << "'.";
      }
    }
    return *camel_case_name_table;
  }

  TypeResolver* type_resolver_;

  // Stores string values that will be referenced by StringPieces in
  // cached_types_, cached_enums_.
  mutable std::set<std::string> string_storage_;

  mutable std::map<absl::string_view, StatusOrType> cached_types_;
  mutable std::map<absl::string_view, StatusOrEnum> cached_enums_;

  mutable std::map<const google::protobuf::Type*, CamelCaseNameTable>
      indexed_types_;
};
}  // namespace

TypeInfo* TypeInfo::NewTypeInfo(TypeResolver* type_resolver) {
  return new TypeInfoForTypeResolver(type_resolver);
}

}  // namespace converter
}  // namespace util
}  // namespace protobuf
}  // namespace google
