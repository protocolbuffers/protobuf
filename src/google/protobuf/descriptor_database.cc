// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/descriptor_database.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/wire_format_lite.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

namespace {
void RecordMessageNames(const DescriptorProto& desc_proto,
                        absl::string_view prefix,
                        absl::btree_set<std::string>* output) {
  ABSL_CHECK(desc_proto.has_name());
  std::string full_name = prefix.empty()
                              ? desc_proto.name()
                              : absl::StrCat(prefix, ".", desc_proto.name());
  output->insert(full_name);

  for (const auto& d : desc_proto.nested_type()) {
    RecordMessageNames(d, full_name, output);
  }
}

void RecordMessageNames(const FileDescriptorProto& file_proto,
                        absl::btree_set<std::string>* output) {
  for (const auto& d : file_proto.message_type()) {
    RecordMessageNames(d, file_proto.package(), output);
  }
}

template <typename Fn>
bool ForAllFileProtos(DescriptorDatabase* db, Fn callback,
                      std::vector<std::string>* output) {
  std::vector<std::string> file_names;
  if (!db->FindAllFileNames(&file_names)) {
    return false;
  }
  absl::btree_set<std::string> set;
  FileDescriptorProto file_proto;
  for (const auto& f : file_names) {
    file_proto.Clear();
    if (!db->FindFileByName(f, &file_proto)) {
      ABSL_LOG(ERROR) << "File not found in database (unexpected): " << f;
      return false;
    }
    callback(file_proto, &set);
  }
  output->insert(output->end(), set.begin(), set.end());
  return true;
}
}  // namespace

DescriptorDatabase::~DescriptorDatabase() = default;

bool DescriptorDatabase::FindAllPackageNames(
    std::vector<std::string>* PROTOBUF_NONNULL output) {
  return ForAllFileProtos(
      this,
      [](const FileDescriptorProto& file_proto,
         absl::btree_set<std::string>* set) {
        set->insert(file_proto.package());
      },
      output);
}

bool DescriptorDatabase::FindAllMessageNames(
    std::vector<std::string>* PROTOBUF_NONNULL output) {
  return ForAllFileProtos(
      this,
      [](const FileDescriptorProto& file_proto,
         absl::btree_set<std::string>* set) {
        RecordMessageNames(file_proto, set);
      },
      output);
}

// ===================================================================

SimpleDescriptorDatabase::SimpleDescriptorDatabase() {}
SimpleDescriptorDatabase::~SimpleDescriptorDatabase() {}

template <typename Value>
bool SimpleDescriptorDatabase::DescriptorIndex<Value>::AddFile(
    const FileDescriptorProto& file, Value value) {
  if (!by_name_.emplace(file.name(), value).second) {
    ABSL_LOG(ERROR) << "File already exists in database: " << file.name();
    return false;
  }

  // We must be careful here -- calling file.package() if file.has_package() is
  // false could access an uninitialized static-storage variable if we are being
  // run at startup time.
  std::string path = file.has_package() ? file.package() : std::string();
  if (!path.empty()) path += '.';

  for (int i = 0; i < file.message_type_size(); i++) {
    if (!AddSymbol(path + file.message_type(i).name(), value)) return false;
    if (!AddNestedExtensions(file.name(), file.message_type(i), value))
      return false;
  }
  for (int i = 0; i < file.enum_type_size(); i++) {
    if (!AddSymbol(path + file.enum_type(i).name(), value)) return false;
  }
  for (int i = 0; i < file.extension_size(); i++) {
    if (!AddSymbol(path + file.extension(i).name(), value)) return false;
    if (!AddExtension(file.name(), file.extension(i), value)) return false;
  }
  for (int i = 0; i < file.service_size(); i++) {
    if (!AddSymbol(path + file.service(i).name(), value)) return false;
  }

  return true;
}

namespace {

// Returns true if and only if all characters in the name are alphanumerics,
// underscores, or periods.
bool ValidateSymbolName(absl::string_view name) {
  for (char c : name) {
    // I don't trust ctype.h due to locales.  :(
    if (c != '.' && c != '_' && (c < '0' || c > '9') && (c < 'A' || c > 'Z') &&
        (c < 'a' || c > 'z')) {
      return false;
    }
  }
  return true;
}

// Find the last key in the container which sorts less than or equal to the
// symbol name.  Since upper_bound() returns the *first* key that sorts
// *greater* than the input, we want the element immediately before that.
template <typename Container, typename Key>
typename Container::const_iterator FindLastLessOrEqual(
    const Container* container, const Key& key) {
  auto iter = container->upper_bound(key);
  if (iter != container->begin()) --iter;
  return iter;
}

// As above, but using std::upper_bound instead.
template <typename Container, typename Key, typename Cmp>
typename Container::const_iterator FindLastLessOrEqual(
    const Container* container, const Key& key, const Cmp& cmp) {
  auto iter = std::upper_bound(container->begin(), container->end(), key, cmp);
  if (iter != container->begin()) --iter;
  return iter;
}

// True if either the arguments are equal or super_symbol identifies a
// parent symbol of sub_symbol (e.g. "foo.bar" is a parent of
// "foo.bar.baz", but not a parent of "foo.barbaz").
bool IsSubSymbol(absl::string_view sub_symbol, absl::string_view super_symbol) {
  return sub_symbol == super_symbol ||
         (absl::StartsWith(super_symbol, sub_symbol) &&
          super_symbol[sub_symbol.size()] == '.');
}

}  // namespace

template <typename Value>
bool SimpleDescriptorDatabase::DescriptorIndex<Value>::AddSymbol(
    absl::string_view name, Value value) {
  // We need to make sure not to violate our map invariant.

  // If the symbol name is invalid it could break our lookup algorithm (which
  // relies on the fact that '.' sorts before all other characters that are
  // valid in symbol names).
  if (!ValidateSymbolName(name)) {
    ABSL_LOG(ERROR) << "Invalid symbol name: " << name;
    return false;
  }

  // Try to look up the symbol to make sure a super-symbol doesn't already
  // exist.
  auto iter = FindLastLessOrEqual(&by_symbol_, name);

  if (iter == by_symbol_.end()) {
    // Apparently the map is currently empty.  Just insert and be done with it.
    by_symbol_.try_emplace(name, value);
    return true;
  }

  if (IsSubSymbol(iter->first, name)) {
    ABSL_LOG(ERROR) << "Symbol name \"" << name
                    << "\" conflicts with the existing "
                       "symbol \""
                    << iter->first << "\".";
    return false;
  }

  // OK, that worked.  Now we have to make sure that no symbol in the map is
  // a sub-symbol of the one we are inserting.  The only symbol which could
  // be so is the first symbol that is greater than the new symbol.  Since
  // |iter| points at the last symbol that is less than or equal, we just have
  // to increment it.
  ++iter;

  if (iter != by_symbol_.end() && IsSubSymbol(name, iter->first)) {
    ABSL_LOG(ERROR) << "Symbol name \"" << name
                    << "\" conflicts with the existing "
                       "symbol \""
                    << iter->first << "\".";
    return false;
  }

  // OK, no conflicts.

  // Insert the new symbol using the iterator as a hint, the new entry will
  // appear immediately before the one the iterator is pointing at.
  by_symbol_.insert(iter, {std::string(name), value});

  return true;
}

template <typename Value>
bool SimpleDescriptorDatabase::DescriptorIndex<Value>::AddNestedExtensions(
    StringViewArg filename, const DescriptorProto& message_type, Value value) {
  for (int i = 0; i < message_type.nested_type_size(); i++) {
    if (!AddNestedExtensions(filename, message_type.nested_type(i), value))
      return false;
  }
  for (int i = 0; i < message_type.extension_size(); i++) {
    if (!AddExtension(filename, message_type.extension(i), value)) return false;
  }
  return true;
}

template <typename Value>
bool SimpleDescriptorDatabase::DescriptorIndex<Value>::AddExtension(
    StringViewArg filename, const FieldDescriptorProto& field, Value value) {
  if (!field.extendee().empty() && field.extendee()[0] == '.') {
    // The extension is fully-qualified.  We can use it as a lookup key in
    // the by_symbol_ table.
    if (!by_extension_
             .emplace(
                 std::make_pair(field.extendee().substr(1), field.number()),
                 value)
             .second) {
      ABSL_LOG(ERROR)
          << "Extension conflicts with extension already in database: "
             "extend "
          << field.extendee() << " { " << field.name() << " = "
          << field.number() << " } from:" << filename;
      return false;
    }
  } else {
    // Not fully-qualified.  We can't really do anything here, unfortunately.
    // We don't consider this an error, though, because the descriptor is
    // valid.
  }
  return true;
}

template <typename Value>
Value SimpleDescriptorDatabase::DescriptorIndex<Value>::FindFile(
    StringViewArg filename) {
  auto it = by_name_.find(filename);
  if (it == by_name_.end()) return {};
  return it->second;
}

template <typename Value>
Value SimpleDescriptorDatabase::DescriptorIndex<Value>::FindSymbol(
    StringViewArg name) {
  auto iter = FindLastLessOrEqual(&by_symbol_, name);

  return (iter != by_symbol_.end() && IsSubSymbol(iter->first, name))
             ? iter->second
             : Value();
}

template <typename Value>
Value SimpleDescriptorDatabase::DescriptorIndex<Value>::FindExtension(
    StringViewArg containing_type, int field_number) {
  auto it = by_extension_.find(
      std::make_pair(std::string(containing_type), field_number));
  if (it == by_extension_.end()) return {};
  return it->second;
}

template <typename Value>
bool SimpleDescriptorDatabase::DescriptorIndex<Value>::FindAllExtensionNumbers(
    StringViewArg containing_type, std::vector<int>* PROTOBUF_NONNULL output) {
  auto it = by_extension_.lower_bound(
      std::make_pair(std::string(containing_type), 0));
  bool success = false;

  for (; it != by_extension_.end() && it->first.first == containing_type;
       ++it) {
    output->push_back(it->first.second);
    success = true;
  }

  return success;
}

template <typename Value>
void SimpleDescriptorDatabase::DescriptorIndex<Value>::FindAllFileNames(
    std::vector<std::string>* PROTOBUF_NONNULL output) {
  output->resize(by_name_.size());
  int i = 0;
  for (const auto& kv : by_name_) {
    (*output)[i] = kv.first;
    i++;
  }
}

// -------------------------------------------------------------------

bool SimpleDescriptorDatabase::Add(const FileDescriptorProto& file) {
  FileDescriptorProto* new_file = new FileDescriptorProto;
  new_file->CopyFrom(file);
  return AddAndOwn(new_file);
}

bool SimpleDescriptorDatabase::AddAndOwn(
    const FileDescriptorProto* PROTOBUF_NONNULL file) {
  files_to_delete_.emplace_back(file);
  return index_.AddFile(*file, file);
}

bool SimpleDescriptorDatabase::AddUnowned(
    const FileDescriptorProto* PROTOBUF_NONNULL file) {
  return index_.AddFile(*file, file);
}

bool SimpleDescriptorDatabase::FindFileByName(
    StringViewArg filename, FileDescriptorProto* PROTOBUF_NONNULL output) {
  return MaybeCopy(index_.FindFile(filename), output);
}

bool SimpleDescriptorDatabase::FindFileContainingSymbol(
    StringViewArg symbol_name, FileDescriptorProto* PROTOBUF_NONNULL output) {
  return MaybeCopy(index_.FindSymbol(symbol_name), output);
}

bool SimpleDescriptorDatabase::FindFileContainingExtension(
    StringViewArg containing_type, int field_number,
    FileDescriptorProto* PROTOBUF_NONNULL output) {
  return MaybeCopy(index_.FindExtension(containing_type, field_number), output);
}

bool SimpleDescriptorDatabase::FindAllExtensionNumbers(
    StringViewArg extendee_type, std::vector<int>* output) {
  return index_.FindAllExtensionNumbers(extendee_type, output);
}


bool SimpleDescriptorDatabase::FindAllFileNames(
    std::vector<std::string>* PROTOBUF_NONNULL output) {
  index_.FindAllFileNames(output);
  return true;
}

bool SimpleDescriptorDatabase::MaybeCopy(
    const FileDescriptorProto* PROTOBUF_NULLABLE file,
    FileDescriptorProto* PROTOBUF_NONNULL output) {
  if (file == nullptr) return false;
  output->CopyFrom(*file);
  return true;
}

// -------------------------------------------------------------------

class EncodedDescriptorDatabase::DescriptorIndex {
 public:
  using Value = std::pair<const void*, int>;
  // Helpers to recursively add particular descriptors and all their contents
  // to the index.
  template <typename FileProto>
  bool AddFile(const FileProto& file, Value value);

  Value FindFile(absl::string_view filename);
  Value FindSymbol(absl::string_view name);
  Value FindSymbolOnlyFlat(absl::string_view name) const;
  Value FindExtension(absl::string_view containing_type, int field_number);
  bool FindAllExtensionNumbers(absl::string_view containing_type,
                               std::vector<int>* output);
  void FindAllFileNames(
      std::vector<std::string>* PROTOBUF_NONNULL output) const;

 private:
  friend class EncodedDescriptorDatabase;

  bool AddSymbol(absl::string_view symbol);

  template <typename DescProto>
  bool AddNestedExtensions(absl::string_view filename,
                           const DescProto& message_type);
  template <typename FieldProto>
  bool AddExtension(absl::string_view filename, const FieldProto& field);

  // All the maps below have two representations:
  //  - a absl::btree_set<> where we insert initially.
  //  - a std::vector<> where we flatten the structure on demand.
  // The initial tree helps avoid O(N) behavior of inserting into a sorted
  // vector, while the vector reduces the heap requirements of the data
  // structure.

  void EnsureFlat();

  using String = std::string;

  String EncodeString(absl::string_view str) const { return String(str); }
  absl::string_view DecodeString(const String& str, int) const { return str; }

  struct EncodedEntry {
    // Do not use `Value` here to avoid the padding of that object.
    const void* data;
    int size;
    // Keep the package here instead of each SymbolEntry to save space.
    String encoded_package;

    Value value() const { return {data, size}; }
  };
  std::vector<EncodedEntry> all_values_;

  struct FileEntry {
    int data_offset;
    String encoded_name;

    absl::string_view name(const DescriptorIndex& index) const {
      return index.DecodeString(encoded_name, data_offset);
    }
  };
  struct FileCompare {
    const DescriptorIndex& index;

    bool operator()(const FileEntry& a, const FileEntry& b) const {
      return a.name(index) < b.name(index);
    }
    bool operator()(const FileEntry& a, absl::string_view b) const {
      return a.name(index) < b;
    }
    bool operator()(absl::string_view a, const FileEntry& b) const {
      return a < b.name(index);
    }
  };
  absl::btree_set<FileEntry, FileCompare> by_name_{FileCompare{*this}};
  std::vector<FileEntry> by_name_flat_;

  struct SymbolEntry {
    int data_offset;
    String encoded_symbol;

    absl::string_view package(const DescriptorIndex& index) const {
      return index.DecodeString(index.all_values_[data_offset].encoded_package,
                                data_offset);
    }
    absl::string_view symbol(const DescriptorIndex& index) const {
      return index.DecodeString(encoded_symbol, data_offset);
    }

    std::string AsString(const DescriptorIndex& index) const {
      auto p = package(index);
      return absl::StrCat(p, p.empty() ? "" : ".", symbol(index));
    }

    bool IsSubSymbolOf(const DescriptorIndex& index,
                       absl::string_view super_symbol) const {
      const auto consume_part = [&](absl::string_view part) {
        if (!absl::ConsumePrefix(&super_symbol, part)) return false;
        return super_symbol.empty() || absl::ConsumePrefix(&super_symbol, ".");
      };
      if (auto p = package(index); !p.empty()) {
        if (!consume_part(p)) return false;
      }
      return consume_part(symbol(index));
    }
  };

  struct SymbolCompare {
    const DescriptorIndex& index;

    std::string AsString(const SymbolEntry& entry) const {
      return entry.AsString(index);
    }

    std::pair<absl::string_view, absl::string_view> GetParts(
        const SymbolEntry& entry) const {
      auto package = entry.package(index);
      if (package.empty()) return {entry.symbol(index), absl::string_view{}};
      return {package, entry.symbol(index)};
    }

    bool operator()(const SymbolEntry& lhs, const SymbolEntry& rhs) const {
      auto lhs_parts = GetParts(lhs);
      auto rhs_parts = GetParts(rhs);

      // Fast path to avoid making the whole string for common cases.
      if (int res =
              lhs_parts.first.substr(0, rhs_parts.first.size())
                  .compare(rhs_parts.first.substr(0, lhs_parts.first.size()))) {
        // If the packages already differ, exit early.
        return res < 0;
      } else if (lhs_parts.first.size() == rhs_parts.first.size()) {
        return lhs_parts.second < rhs_parts.second;
      }
      return AsString(lhs) < AsString(rhs);
    }

    bool operator()(absl::string_view lhs, const SymbolEntry& rhs) const {
      auto p = rhs.package(index);
      if (!p.empty()) {
        absl::string_view lhs_part = lhs.substr(0, p.size());
        lhs.remove_prefix(lhs_part.size());
        if (int res = lhs_part.compare(p); res != 0) return res < 0;
        // If compare returned 0 is because we consumed all of `p` and it
        // matched.

        // Compare the implicit `.`
        if (lhs.empty() || lhs[0] < '.') return true;
        if (lhs[0] > '.') return false;
        lhs.remove_prefix(1);
      }
      return lhs < rhs.symbol(index);
    }
  };
  absl::btree_set<SymbolEntry, SymbolCompare> by_symbol_{SymbolCompare{*this}};
  std::vector<SymbolEntry> by_symbol_flat_;

  struct ExtensionEntry {
    int data_offset;
    String encoded_extendee;
    absl::string_view extendee(const DescriptorIndex& index) const {
      return index.DecodeString(encoded_extendee, data_offset).substr(1);
    }
    int extension_number;
  };
  struct ExtensionCompare {
    const DescriptorIndex& index;

    bool operator()(const ExtensionEntry& a, const ExtensionEntry& b) const {
      return std::make_tuple(a.extendee(index), a.extension_number) <
             std::make_tuple(b.extendee(index), b.extension_number);
    }
    bool operator()(const ExtensionEntry& a,
                    std::tuple<absl::string_view, int> b) const {
      return std::make_tuple(a.extendee(index), a.extension_number) < b;
    }
    bool operator()(std::tuple<absl::string_view, int> a,
                    const ExtensionEntry& b) const {
      return a < std::make_tuple(b.extendee(index), b.extension_number);
    }
  };
  absl::btree_set<ExtensionEntry, ExtensionCompare> by_extension_{
      ExtensionCompare{*this}};
  std::vector<ExtensionEntry> by_extension_flat_;
};

bool EncodedDescriptorDatabase::Add(
    const void* PROTOBUF_NONNULL encoded_file_descriptor, int size) {
  FileDescriptorProto file;
  if (file.ParseFromArray(encoded_file_descriptor, size)) {
    return index_->AddFile(file, std::make_pair(encoded_file_descriptor, size));
  } else {
    ABSL_LOG(ERROR) << "Invalid file descriptor data passed to "
                       "EncodedDescriptorDatabase::Add().";
    return false;
  }
}

bool EncodedDescriptorDatabase::AddCopy(
    const void* PROTOBUF_NONNULL encoded_file_descriptor, int size) {
  void* copy = operator new(size);
  memcpy(copy, encoded_file_descriptor, size);
  files_to_delete_.push_back(copy);
  return Add(copy, size);
}

bool EncodedDescriptorDatabase::FindFileByName(
    StringViewArg filename, FileDescriptorProto* PROTOBUF_NONNULL output) {
  return MaybeParse(index_->FindFile(filename), output);
}

bool EncodedDescriptorDatabase::FindFileContainingSymbol(
    StringViewArg symbol_name, FileDescriptorProto* PROTOBUF_NONNULL output) {
  return MaybeParse(index_->FindSymbol(symbol_name), output);
}

bool EncodedDescriptorDatabase::FindNameOfFileContainingSymbol(
    StringViewArg symbol_name, std::string* PROTOBUF_NONNULL output) {
  auto encoded_file = index_->FindSymbol(symbol_name);
  if (encoded_file.first == nullptr) return false;

  // Optimization:  The name should be the first field in the encoded message.
  //   Try to just read it directly.
  io::CodedInputStream input(static_cast<const uint8_t*>(encoded_file.first),
                             encoded_file.second);

  const uint32_t kNameTag = internal::WireFormatLite::MakeTag(
      FileDescriptorProto::kNameFieldNumber,
      internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED);

  if (input.ReadTagNoLastTag() == kNameTag) {
    // Success!
    return internal::WireFormatLite::ReadString(&input, output);
  } else {
    // Slow path.  Parse whole message.
    FileDescriptorProto file_proto;
    if (!file_proto.ParseFromArray(encoded_file.first, encoded_file.second)) {
      return false;
    }
    *output = file_proto.name();
    return true;
  }
}

bool EncodedDescriptorDatabase::FindFileContainingExtension(
    StringViewArg containing_type, int field_number,
    FileDescriptorProto* PROTOBUF_NONNULL output) {
  return MaybeParse(index_->FindExtension(containing_type, field_number),
                    output);
}

bool EncodedDescriptorDatabase::FindAllExtensionNumbers(
    StringViewArg extendee_type, std::vector<int>* output) {
  return index_->FindAllExtensionNumbers(extendee_type, output);
}

template <typename FileProto>
bool EncodedDescriptorDatabase::DescriptorIndex::AddFile(const FileProto& file,
                                                         Value value) {
  // We push `value` into the array first. This is important because the AddXXX
  // functions below will expect it to be there.
  all_values_.push_back({value.first, value.second, {}});

  if (!ValidateSymbolName(file.package())) {
    ABSL_LOG(ERROR) << "Invalid package name: " << file.package();
    return false;
  }
  all_values_.back().encoded_package = EncodeString(file.package());

  if (!by_name_
           .insert({static_cast<int>(all_values_.size() - 1),
                    EncodeString(file.name())})
           .second ||
      std::binary_search(by_name_flat_.begin(), by_name_flat_.end(),
                         file.name(), by_name_.key_comp())) {
    ABSL_LOG(ERROR) << "File already exists in database: " << file.name();
    return false;
  }

  for (const auto& message_type : file.message_type()) {
    if (!AddSymbol(message_type.name())) return false;
    if (!AddNestedExtensions(file.name(), message_type)) return false;
  }
  for (const auto& enum_type : file.enum_type()) {
    if (!AddSymbol(enum_type.name())) return false;
  }
  for (const auto& extension : file.extension()) {
    if (!AddSymbol(extension.name())) return false;
    if (!AddExtension(file.name(), extension)) return false;
  }
  for (const auto& service : file.service()) {
    if (!AddSymbol(service.name())) return false;
  }

  return true;
}

template <typename Iter, typename Iter2, typename Index>
static bool CheckForMutualSubsymbols(absl::string_view symbol_name, Iter* iter,
                                     Iter2 end, const Index& index) {
  if (*iter != end) {
    if (IsSubSymbol((*iter)->AsString(index), symbol_name)) {
      ABSL_LOG(ERROR) << "Symbol name \"" << symbol_name
                      << "\" conflicts with the existing symbol \""
                      << (*iter)->AsString(index) << "\".";
      return false;
    }

    // OK, that worked.  Now we have to make sure that no symbol in the map is
    // a sub-symbol of the one we are inserting.  The only symbol which could
    // be so is the first symbol that is greater than the new symbol.  Since
    // |iter| points at the last symbol that is less than or equal, we just have
    // to increment it.
    ++*iter;

    if (*iter != end && IsSubSymbol(symbol_name, (*iter)->AsString(index))) {
      ABSL_LOG(ERROR) << "Symbol name \"" << symbol_name
                      << "\" conflicts with the existing symbol \""
                      << (*iter)->AsString(index) << "\".";
      return false;
    }
  }
  return true;
}

bool EncodedDescriptorDatabase::DescriptorIndex::AddSymbol(
    absl::string_view symbol) {
  SymbolEntry entry = {static_cast<int>(all_values_.size() - 1),
                       EncodeString(symbol)};
  std::string entry_as_string = entry.AsString(*this);

  // We need to make sure not to violate our map invariant.

  // If the symbol name is invalid it could break our lookup algorithm (which
  // relies on the fact that '.' sorts before all other characters that are
  // valid in symbol names).
  if (!ValidateSymbolName(symbol)) {
    ABSL_LOG(ERROR) << "Invalid symbol name: " << entry_as_string;
    return false;
  }

  auto iter = FindLastLessOrEqual(&by_symbol_, entry);
  if (!CheckForMutualSubsymbols(entry_as_string, &iter, by_symbol_.end(),
                                *this)) {
    return false;
  }

  // Same, but on by_symbol_flat_
  auto flat_iter =
      FindLastLessOrEqual(&by_symbol_flat_, entry, by_symbol_.key_comp());
  if (!CheckForMutualSubsymbols(entry_as_string, &flat_iter,
                                by_symbol_flat_.end(), *this)) {
    return false;
  }

  // OK, no conflicts.

  // Insert the new symbol using the iterator as a hint, the new entry will
  // appear immediately before the one the iterator is pointing at.
  by_symbol_.insert(iter, entry);

  return true;
}

template <typename DescProto>
bool EncodedDescriptorDatabase::DescriptorIndex::AddNestedExtensions(
    absl::string_view filename, const DescProto& message_type) {
  for (const auto& nested_type : message_type.nested_type()) {
    if (!AddNestedExtensions(filename, nested_type)) return false;
  }
  for (const auto& extension : message_type.extension()) {
    if (!AddExtension(filename, extension)) return false;
  }
  return true;
}

template <typename FieldProto>
bool EncodedDescriptorDatabase::DescriptorIndex::AddExtension(
    absl::string_view filename, const FieldProto& field) {
  if (!field.extendee().empty() && field.extendee()[0] == '.') {
    // The extension is fully-qualified.  We can use it as a lookup key in
    // the by_symbol_ table.
    if (!by_extension_
             .insert({static_cast<int>(all_values_.size() - 1),
                      EncodeString(field.extendee()), field.number()})
             .second ||
        std::binary_search(
            by_extension_flat_.begin(), by_extension_flat_.end(),
            std::make_pair(field.extendee().substr(1), field.number()),
            by_extension_.key_comp())) {
      ABSL_LOG(ERROR)
          << "Extension conflicts with extension already in database: "
             "extend "
          << field.extendee() << " { " << field.name() << " = "
          << field.number() << " } from:" << filename;
      return false;
    }
  } else {
    // Not fully-qualified.  We can't really do anything here, unfortunately.
    // We don't consider this an error, though, because the descriptor is
    // valid.
  }
  return true;
}

std::pair<const void*, int>
EncodedDescriptorDatabase::DescriptorIndex::FindSymbol(absl::string_view name) {
  EnsureFlat();
  return FindSymbolOnlyFlat(name);
}

std::pair<const void*, int>
EncodedDescriptorDatabase::DescriptorIndex::FindSymbolOnlyFlat(
    absl::string_view name) const {
  auto iter =
      FindLastLessOrEqual(&by_symbol_flat_, name, by_symbol_.key_comp());

  return iter != by_symbol_flat_.end() && iter->IsSubSymbolOf(*this, name)
             ? all_values_[iter->data_offset].value()
             : Value();
}

std::pair<const void*, int>
EncodedDescriptorDatabase::DescriptorIndex::FindExtension(
    absl::string_view containing_type, int field_number) {
  EnsureFlat();

  auto it = std::lower_bound(
      by_extension_flat_.begin(), by_extension_flat_.end(),
      std::make_tuple(containing_type, field_number), by_extension_.key_comp());
  return it == by_extension_flat_.end() ||
                 it->extendee(*this) != containing_type ||
                 it->extension_number != field_number
             ? std::make_pair(nullptr, 0)
             : all_values_[it->data_offset].value();
}

template <typename T, typename Less>
static void MergeIntoFlat(absl::btree_set<T, Less>* s, std::vector<T>* flat) {
  if (s->empty()) return;
  std::vector<T> new_flat(s->size() + flat->size());
  std::merge(s->begin(), s->end(), flat->begin(), flat->end(), &new_flat[0],
             s->key_comp());
  *flat = std::move(new_flat);
  s->clear();
}

void EncodedDescriptorDatabase::DescriptorIndex::EnsureFlat() {
  all_values_.shrink_to_fit();
  // Merge each of the sets into their flat counterpart.
  MergeIntoFlat(&by_name_, &by_name_flat_);
  MergeIntoFlat(&by_symbol_, &by_symbol_flat_);
  MergeIntoFlat(&by_extension_, &by_extension_flat_);
}

bool EncodedDescriptorDatabase::DescriptorIndex::FindAllExtensionNumbers(
    absl::string_view containing_type, std::vector<int>* output) {
  EnsureFlat();

  bool success = false;
  auto it = std::lower_bound(
      by_extension_flat_.begin(), by_extension_flat_.end(),
      std::make_tuple(containing_type, 0), by_extension_.key_comp());
  for (;
       it != by_extension_flat_.end() && it->extendee(*this) == containing_type;
       ++it) {
    output->push_back(it->extension_number);
    success = true;
  }

  return success;
}

void EncodedDescriptorDatabase::DescriptorIndex::FindAllFileNames(
    std::vector<std::string>* PROTOBUF_NONNULL output) const {
  output->resize(by_name_.size() + by_name_flat_.size());
  int i = 0;
  for (const auto& entry : by_name_) {
    (*output)[i] = std::string(entry.name(*this));
    i++;
  }
  for (const auto& entry : by_name_flat_) {
    (*output)[i] = std::string(entry.name(*this));
    i++;
  }
}

std::pair<const void*, int>
EncodedDescriptorDatabase::DescriptorIndex::FindFile(
    absl::string_view filename) {
  EnsureFlat();

  auto it = std::lower_bound(by_name_flat_.begin(), by_name_flat_.end(),
                             filename, by_name_.key_comp());
  return it == by_name_flat_.end() || it->name(*this) != filename
             ? std::make_pair(nullptr, 0)
             : all_values_[it->data_offset].value();
}


bool EncodedDescriptorDatabase::FindAllFileNames(
    std::vector<std::string>* PROTOBUF_NONNULL output) {
  index_->FindAllFileNames(output);
  return true;
}

bool EncodedDescriptorDatabase::MaybeParse(
    std::pair<const void * PROTOBUF_NULLABLE, int> encoded_file,
    FileDescriptorProto* PROTOBUF_NONNULL output) {
  if (encoded_file.first == nullptr) return false;
  absl::string_view source(static_cast<const char*>(encoded_file.first),
                           encoded_file.second);
  return internal::ParseNoReflection(source, *output);
}

EncodedDescriptorDatabase::EncodedDescriptorDatabase()
    : index_(new DescriptorIndex()) {}

EncodedDescriptorDatabase::~EncodedDescriptorDatabase() {
  for (void* p : files_to_delete_) {
    operator delete(p);
  }
}

// ===================================================================

DescriptorPoolDatabase::DescriptorPoolDatabase(
    const DescriptorPool& pool, DescriptorPoolDatabaseOptions options)
    : pool_(pool), options_(std::move(options)) {}
DescriptorPoolDatabase::~DescriptorPoolDatabase() {}

bool DescriptorPoolDatabase::FindFileByName(
    StringViewArg filename, FileDescriptorProto* PROTOBUF_NONNULL output) {
  const FileDescriptor* file = pool_.FindFileByName(filename);
  if (file == nullptr) return false;
  output->Clear();
  file->CopyTo(output);
  if (options_.preserve_source_code_info) {
    file->CopySourceCodeInfoTo(output);
  }
  return true;
}

bool DescriptorPoolDatabase::FindFileContainingSymbol(
    StringViewArg symbol_name, FileDescriptorProto* PROTOBUF_NONNULL output) {
  const FileDescriptor* file = pool_.FindFileContainingSymbol(symbol_name);
  if (file == nullptr) return false;
  output->Clear();
  file->CopyTo(output);
  if (options_.preserve_source_code_info) {
    file->CopySourceCodeInfoTo(output);
  }
  return true;
}

bool DescriptorPoolDatabase::FindFileContainingExtension(
    StringViewArg containing_type, int field_number,
    FileDescriptorProto* PROTOBUF_NONNULL output) {
  const Descriptor* extendee = pool_.FindMessageTypeByName(containing_type);
  if (extendee == nullptr) return false;

  const FieldDescriptor* extension =
      pool_.FindExtensionByNumber(extendee, field_number);
  if (extension == nullptr) return false;

  output->Clear();
  extension->file()->CopyTo(output);
  if (options_.preserve_source_code_info) {
    extension->file()->CopySourceCodeInfoTo(output);
  }
  return true;
}

bool DescriptorPoolDatabase::FindAllExtensionNumbers(
    StringViewArg extendee_type, std::vector<int>* output) {
  const Descriptor* extendee = pool_.FindMessageTypeByName(extendee_type);
  if (extendee == nullptr) return false;

  std::vector<const FieldDescriptor*> extensions;
  pool_.FindAllExtensions(extendee, &extensions);

  for (const FieldDescriptor* extension : extensions) {
    output->push_back(extension->number());
  }

  return true;
}

// ===================================================================

MergedDescriptorDatabase::MergedDescriptorDatabase(
    DescriptorDatabase* PROTOBUF_NONNULL source1,
    DescriptorDatabase* PROTOBUF_NONNULL source2) {
  sources_.push_back(source1);
  sources_.push_back(source2);
}
MergedDescriptorDatabase::MergedDescriptorDatabase(
    const std::vector<DescriptorDatabase*>& sources)
    : sources_(sources) {}
MergedDescriptorDatabase::~MergedDescriptorDatabase() {}

bool MergedDescriptorDatabase::FindFileByName(
    StringViewArg filename, FileDescriptorProto* PROTOBUF_NONNULL output) {
  for (DescriptorDatabase* source : sources_) {
    if (source->FindFileByName(filename, output)) {
      return true;
    }
  }
  return false;
}

bool MergedDescriptorDatabase::FindFileContainingSymbol(
    StringViewArg symbol_name, FileDescriptorProto* PROTOBUF_NONNULL output) {
  for (size_t i = 0; i < sources_.size(); i++) {
    if (sources_[i]->FindFileContainingSymbol(symbol_name, output)) {
      // The symbol was found in source i.  However, if one of the previous
      // sources defines a file with the same name (which presumably doesn't
      // contain the symbol, since it wasn't found in that source), then we
      // must hide it from the caller.
      FileDescriptorProto temp;
      for (size_t j = 0; j < i; j++) {
        if (sources_[j]->FindFileByName(output->name(), &temp)) {
          // Found conflicting file in a previous source.
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

bool MergedDescriptorDatabase::FindFileContainingExtension(
    StringViewArg containing_type, int field_number,
    FileDescriptorProto* PROTOBUF_NONNULL output) {
  for (size_t i = 0; i < sources_.size(); i++) {
    if (sources_[i]->FindFileContainingExtension(containing_type, field_number,
                                                 output)) {
      // The symbol was found in source i.  However, if one of the previous
      // sources defines a file with the same name (which presumably doesn't
      // contain the symbol, since it wasn't found in that source), then we
      // must hide it from the caller.
      FileDescriptorProto temp;
      for (size_t j = 0; j < i; j++) {
        if (sources_[j]->FindFileByName(output->name(), &temp)) {
          // Found conflicting file in a previous source.
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

bool MergedDescriptorDatabase::FindAllExtensionNumbers(
    StringViewArg extendee_type, std::vector<int>* output) {
  // NOLINTNEXTLINE(google3-runtime-rename-unnecessary-ordering)
  absl::btree_set<int> merged_results;
  std::vector<int> results;
  bool success = false;
  for (DescriptorDatabase* source : sources_) {
    if (source->FindAllExtensionNumbers(extendee_type, &results)) {
      for (int r : results) merged_results.insert(r);
      success = true;
    }
    results.clear();
  }
  for (int r : merged_results) output->push_back(r);
  return success;
}


bool MergedDescriptorDatabase::FindAllFileNames(
    std::vector<std::string>* PROTOBUF_NONNULL output) {
  bool implemented = false;
  for (DescriptorDatabase* source : sources_) {
    std::vector<std::string> source_output;
    if (source->FindAllFileNames(&source_output)) {
      output->reserve(output->size() + source_output.size());
      for (auto& source_out : source_output) {
        output->push_back(std::move(source_out));
      }
      implemented = true;
    }
  }
  return implemented;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
