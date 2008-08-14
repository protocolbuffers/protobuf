// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/stubs/hash.h>
#include <map>
#include <set>
#include <algorithm>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/stubs/map-util.h>
#include <google/protobuf/stubs/stl_util-inl.h>

#undef PACKAGE  // autoheader #defines this.  :(

namespace google {
namespace protobuf {

const FieldDescriptor::CppType
FieldDescriptor::kTypeToCppTypeMap[MAX_TYPE + 1] = {
  static_cast<CppType>(0),  // 0 is reserved for errors

  CPPTYPE_DOUBLE,   // TYPE_DOUBLE
  CPPTYPE_FLOAT,    // TYPE_FLOAT
  CPPTYPE_INT64,    // TYPE_INT64
  CPPTYPE_UINT64,   // TYPE_UINT64
  CPPTYPE_INT32,    // TYPE_INT32
  CPPTYPE_UINT64,   // TYPE_FIXED64
  CPPTYPE_UINT32,   // TYPE_FIXED32
  CPPTYPE_BOOL,     // TYPE_BOOL
  CPPTYPE_STRING,   // TYPE_STRING
  CPPTYPE_MESSAGE,  // TYPE_GROUP
  CPPTYPE_MESSAGE,  // TYPE_MESSAGE
  CPPTYPE_STRING,   // TYPE_BYTES
  CPPTYPE_UINT32,   // TYPE_UINT32
  CPPTYPE_ENUM,     // TYPE_ENUM
  CPPTYPE_INT32,    // TYPE_SFIXED32
  CPPTYPE_INT64,    // TYPE_SFIXED64
  CPPTYPE_INT32,    // TYPE_SINT32
  CPPTYPE_INT64,    // TYPE_SINT64
};

const char * const FieldDescriptor::kTypeToName[MAX_TYPE + 1] = {
  "ERROR",     // 0 is reserved for errors

  "double",    // TYPE_DOUBLE
  "float",     // TYPE_FLOAT
  "int64",     // TYPE_INT64
  "uint64",    // TYPE_UINT64
  "int32",     // TYPE_INT32
  "fixed64",   // TYPE_FIXED64
  "fixed32",   // TYPE_FIXED32
  "bool",      // TYPE_BOOL
  "string",    // TYPE_STRING
  "group",     // TYPE_GROUP
  "message",   // TYPE_MESSAGE
  "bytes",     // TYPE_BYTES
  "uint32",    // TYPE_UINT32
  "enum",      // TYPE_ENUM
  "sfixed32",  // TYPE_SFIXED32
  "sfixed64",  // TYPE_SFIXED64
  "sint32",    // TYPE_SINT32
  "sint64",    // TYPE_SINT64
};

const char * const FieldDescriptor::kLabelToName[MAX_LABEL + 1] = {
  "ERROR",     // 0 is reserved for errors

  "optional",  // LABEL_OPTIONAL
  "required",  // LABEL_REQUIRED
  "repeated",  // LABEL_REPEATED
};

#ifndef _MSC_VER  // MSVC doesn't need these and won't even accept them.
const int FieldDescriptor::kMaxNumber;
const int FieldDescriptor::kFirstReservedNumber;
const int FieldDescriptor::kLastReservedNumber;
#endif

namespace {

const string kEmptyString;

// A DescriptorPool contains a bunch of hash_maps to implement the
// various Find*By*() methods.  Since hashtable lookups are O(1), it's
// most efficient to construct a fixed set of large hash_maps used by
// all objects in the pool rather than construct one or more small
// hash_maps for each object.
//
// The keys to these hash_maps are (parent, name) or (parent, number)
// pairs.  Unfortunately STL doesn't provide hash functions for pair<>,
// so we must invent our own.
//
// TODO(kenton):  Use StringPiece rather than const char* in keys?  It would
//   be a lot cleaner but we'd just have to convert it back to const char*
//   for the open source release.

typedef pair<const void*, const char*> PointerStringPair;

// Used by GCC/SGI STL only.  (Why isn't this provided by the standard
// library?  :( )
struct CStringEqual {
  inline bool operator()(const char* a, const char* b) const {
    return strcmp(a, b) == 0;
  }
};

struct PointerStringPairEqual {
  inline bool operator()(const PointerStringPair& a,
                         const PointerStringPair& b) const {
    return a.first == b.first && strcmp(a.second, b.second) == 0;
  }
};

template<typename PairType>
struct PointerIntegerPairHash {
  size_t operator()(const PairType& p) const {
    // FIXME(kenton):  What is the best way to compute this hash?  I have
    // no idea!  This seems a bit better than an XOR.
    return reinterpret_cast<intptr_t>(p.first) * ((1 << 16) - 1) + p.second;
  }

  // Used only by MSVC and platforms where hash_map is not available.
  static const size_t bucket_size = 4;
  static const size_t min_buckets = 8;
  inline bool operator()(const PairType& a, const PairType& b) const {
    return a.first < b.first ||
          (a.first == b.first && a.second < b.second);
  }
};

typedef pair<const Descriptor*, int> DescriptorIntPair;
typedef pair<const EnumDescriptor*, int> EnumIntPair;

struct PointerStringPairHash {
  size_t operator()(const PointerStringPair& p) const {
    // FIXME(kenton):  What is the best way to compute this hash?  I have
    // no idea!  This seems a bit better than an XOR.
    hash<const char*> cstring_hash;
    return reinterpret_cast<intptr_t>(p.first) * ((1 << 16) - 1) +
           cstring_hash(p.second);
  }

  // Used only by MSVC and platforms where hash_map is not available.
  static const size_t bucket_size = 4;
  static const size_t min_buckets = 8;
  inline bool operator()(const PointerStringPair& a,
                         const PointerStringPair& b) const {
    if (a.first < b.first) return true;
    if (a.first > b.first) return false;
    return strcmp(a.second, b.second) < 0;
  }
};


struct Symbol {
  enum Type {
    NULL_SYMBOL, MESSAGE, FIELD, ENUM, ENUM_VALUE, SERVICE, METHOD, PACKAGE
  };
  Type type;
  union {
    const Descriptor* descriptor;
    const FieldDescriptor* field_descriptor;
    const EnumDescriptor* enum_descriptor;
    const EnumValueDescriptor* enum_value_descriptor;
    const ServiceDescriptor* service_descriptor;
    const MethodDescriptor* method_descriptor;
    const FileDescriptor* package_file_descriptor;
  };

  inline Symbol() : type(NULL_SYMBOL) { descriptor = NULL; }
  inline bool IsNull() const { return type == NULL_SYMBOL; }

#define CONSTRUCTOR(TYPE, TYPE_CONSTANT, FIELD)  \
  inline explicit Symbol(const TYPE* value) {    \
    type = TYPE_CONSTANT;                        \
    this->FIELD = value;                         \
  }

  CONSTRUCTOR(Descriptor         , MESSAGE   , descriptor             )
  CONSTRUCTOR(FieldDescriptor    , FIELD     , field_descriptor       )
  CONSTRUCTOR(EnumDescriptor     , ENUM      , enum_descriptor        )
  CONSTRUCTOR(EnumValueDescriptor, ENUM_VALUE, enum_value_descriptor  )
  CONSTRUCTOR(ServiceDescriptor  , SERVICE   , service_descriptor     )
  CONSTRUCTOR(MethodDescriptor   , METHOD    , method_descriptor      )
  CONSTRUCTOR(FileDescriptor     , PACKAGE   , package_file_descriptor)
#undef CONSTRUCTOR

  const FileDescriptor* GetFile() const {
    switch (type) {
      case NULL_SYMBOL: return NULL;
      case MESSAGE    : return descriptor           ->file();
      case FIELD      : return field_descriptor     ->file();
      case ENUM       : return enum_descriptor      ->file();
      case ENUM_VALUE : return enum_value_descriptor->type()->file();
      case SERVICE    : return service_descriptor   ->file();
      case METHOD     : return method_descriptor    ->service()->file();
      case PACKAGE    : return package_file_descriptor;
    }
    return NULL;
  }
};

const Symbol kNullSymbol;

typedef hash_map<const char*, Symbol,
                 hash<const char*>, CStringEqual>
  SymbolsByNameMap;
typedef hash_map<PointerStringPair, Symbol,
                 PointerStringPairHash, PointerStringPairEqual>
  SymbolsByParentMap;
typedef hash_map<const char*, const FileDescriptor*,
                 hash<const char*>, CStringEqual>
  FilesByNameMap;
typedef hash_map<DescriptorIntPair, const FieldDescriptor*,
                 PointerIntegerPairHash<DescriptorIntPair> >
  FieldsByNumberMap;
typedef hash_map<EnumIntPair, const EnumValueDescriptor*,
                 PointerIntegerPairHash<EnumIntPair> >
  EnumValuesByNumberMap;

}  // anonymous namespace

// ===================================================================
// DescriptorPool::Tables

class DescriptorPool::Tables {
 public:
  Tables();
  ~Tables();

  // Checkpoint the state of the tables.  Future calls to Rollback() will
  // return the Tables to this state.  This is used when building files, since
  // some kinds of validation errors cannot be detected until the file's
  // descriptors have already been added to the tables.  BuildFile() calls
  // Checkpoint() before it starts building and Rollback() if it encounters
  // an error.
  void Checkpoint();

  // Roll back the Tables to the state of the last Checkpoint(), removing
  // everything that was added after that point.
  void Rollback();

  // The stack of files which are currently being built.  Used to detect
  // cyclic dependencies when loading files from a DescriptorDatabase.  Not
  // used when fallback_database_ == NULL.
  vector<string> pending_files_;

  // A set of files which we have tried to load from the fallback database
  // and encountered errors.  We will not attempt to load them again.
  // Not used when fallback_database_ == NULL.
  hash_set<string> known_bad_files_;

  // -----------------------------------------------------------------
  // Finding items.

  // Find symbols.  These return a null Symbol (symbol.IsNull() is true)
  // if not found.  FindSymbolOfType() additionally returns null if the
  // symbol is not of the given type.
  inline Symbol FindSymbol(const string& key) const;
  inline Symbol FindSymbolOfType(const string& key,
                                 const Symbol::Type type) const;
  inline Symbol FindNestedSymbol(const void* parent,
                                 const string& name) const;
  inline Symbol FindNestedSymbolOfType(const void* parent,
                                       const string& name,
                                       const Symbol::Type type) const;

  // These return NULL if not found.
  inline const FileDescriptor* FindFile(const string& key) const;
  inline const FieldDescriptor* FindFieldByNumber(
    const Descriptor* parent, int number) const;
  inline const EnumValueDescriptor* FindEnumValueByNumber(
    const EnumDescriptor* parent, int number) const;

  // -----------------------------------------------------------------
  // Adding items.

  // These add items to the corresponding tables.  They return false if
  // the key already exists in the table.  For AddSymbol(), the strings passed
  // in must be ones that were constructed using AllocateString(), as they will
  // be used as keys in the symbols_by_name_ and symbols_by_parent_ maps
  // without copying.  (If parent is NULL, nothing is added to
  // symbols_by_parent_.)
  bool AddSymbol(const string& full_name,
                 const void* parent, const string& name,
                 Symbol symbol);
  bool AddFile(const FileDescriptor* file);
  bool AddFieldByNumber(const FieldDescriptor* field);
  bool AddEnumValueByNumber(const EnumValueDescriptor* value);

  // Like AddSymbol(), but only adds to symbols_by_parent_, not
  // symbols_by_name_.  Used for enum values, which need to be registered
  // under multiple parents (their type and its parent).
  bool AddAliasUnderParent(const void* parent, const string& name,
                           Symbol symbol);

  // -----------------------------------------------------------------
  // Allocating memory.

  // Allocate an object which will be reclaimed when the pool is
  // destroyed.  Note that the object's destructor will never be called,
  // so its fields must be plain old data (primitive data types and
  // pointers).  All of the descriptor types are such objects.
  template<typename Type> Type* Allocate();

  // Allocate an array of objects which will be reclaimed when the
  // pool in destroyed.  Again, destructors are never called.
  template<typename Type> Type* AllocateArray(int count);

  // Allocate a string which will be destroyed when the pool is destroyed.
  // The string is initialized to the given value for convenience.
  string* AllocateString(const string& value);

  // Allocate a protocol message object.
  template<typename Type> Type* AllocateMessage();

 private:
  vector<string*> strings_;    // All strings in the pool.
  vector<Message*> messages_;  // All messages in the pool.
  vector<void*> allocations_;  // All other memory allocated in the pool.

  SymbolsByNameMap      symbols_by_name_;
  SymbolsByParentMap    symbols_by_parent_;
  FilesByNameMap        files_by_name_;
  FieldsByNumberMap     fields_by_number_;       // Includes extensions.
  EnumValuesByNumberMap enum_values_by_number_;

  int strings_before_checkpoint_;
  int messages_before_checkpoint_;
  int allocations_before_checkpoint_;
  vector<const char*      > symbols_after_checkpoint_;
  vector<PointerStringPair> symbols_by_parent_after_checkpoint_;
  vector<const char*      > files_after_checkpoint_;
  vector<DescriptorIntPair> field_numbers_after_checkpoint_;
  vector<EnumIntPair      > enum_numbers_after_checkpoint_;

  // Allocate some bytes which will be reclaimed when the pool is
  // destroyed.
  void* AllocateBytes(int size);
};

DescriptorPool::Tables::Tables()
  : strings_before_checkpoint_(0),
    messages_before_checkpoint_(0),
    allocations_before_checkpoint_(0) {}

DescriptorPool::Tables::~Tables() {
  for (int i = 0; i < allocations_.size(); i++) {
    operator delete(allocations_[i]);
  }
  STLDeleteElements(&strings_);
  STLDeleteElements(&messages_);
}

void DescriptorPool::Tables::Checkpoint() {
  strings_before_checkpoint_ = strings_.size();
  messages_before_checkpoint_ = messages_.size();
  allocations_before_checkpoint_ = allocations_.size();

  symbols_after_checkpoint_.clear();
  symbols_by_parent_after_checkpoint_.clear();
  files_after_checkpoint_.clear();
  field_numbers_after_checkpoint_.clear();
  enum_numbers_after_checkpoint_.clear();
}

void DescriptorPool::Tables::Rollback() {
  for (int i = 0; i < symbols_after_checkpoint_.size(); i++) {
    symbols_by_name_.erase(symbols_after_checkpoint_[i]);
  }
  for (int i = 0; i < symbols_by_parent_after_checkpoint_.size(); i++) {
    symbols_by_parent_.erase(symbols_by_parent_after_checkpoint_[i]);
  }
  for (int i = 0; i < files_after_checkpoint_.size(); i++) {
    files_by_name_.erase(files_after_checkpoint_[i]);
  }
  for (int i = 0; i < field_numbers_after_checkpoint_.size(); i++) {
    fields_by_number_.erase(field_numbers_after_checkpoint_[i]);
  }
  for (int i = 0; i < enum_numbers_after_checkpoint_.size(); i++) {
    enum_values_by_number_.erase(enum_numbers_after_checkpoint_[i]);
  }

  symbols_after_checkpoint_.clear();
  symbols_by_parent_after_checkpoint_.clear();
  files_after_checkpoint_.clear();
  field_numbers_after_checkpoint_.clear();
  enum_numbers_after_checkpoint_.clear();

  STLDeleteContainerPointers(
    strings_.begin() + strings_before_checkpoint_, strings_.end());
  STLDeleteContainerPointers(
    messages_.begin() + messages_before_checkpoint_, messages_.end());
  for (int i = allocations_before_checkpoint_; i < allocations_.size(); i++) {
    operator delete(allocations_[i]);
  }

  strings_.resize(strings_before_checkpoint_);
  messages_.resize(messages_before_checkpoint_);
  allocations_.resize(allocations_before_checkpoint_);
}

// -------------------------------------------------------------------

inline Symbol DescriptorPool::Tables::FindSymbol(const string& key) const {
  const Symbol* result = FindOrNull(symbols_by_name_, key.c_str());
  if (result == NULL) {
    return kNullSymbol;
  } else {
    return *result;
  }
}

inline Symbol DescriptorPool::Tables::FindSymbolOfType(
    const string& key, const Symbol::Type type) const {
  Symbol result = FindSymbol(key);
  if (result.type != type) return kNullSymbol;
  return result;
}

inline Symbol DescriptorPool::Tables::FindNestedSymbol(
    const void* parent, const string& name) const {
  const Symbol* result =
    FindOrNull(symbols_by_parent_, PointerStringPair(parent, name.c_str()));
  if (result == NULL) {
    return kNullSymbol;
  } else {
    return *result;
  }
}

inline Symbol DescriptorPool::Tables::FindNestedSymbolOfType(
    const void* parent, const string& name, const Symbol::Type type) const {
  Symbol result = FindNestedSymbol(parent, name);
  if (result.type != type) return kNullSymbol;
  return result;
}

inline const FileDescriptor* DescriptorPool::Tables::FindFile(
    const string& key) const {
  return FindPtrOrNull(files_by_name_, key.c_str());
}

inline const FieldDescriptor* DescriptorPool::Tables::FindFieldByNumber(
    const Descriptor* parent, int number) const {
  return FindPtrOrNull(fields_by_number_, make_pair(parent, number));
}

inline const EnumValueDescriptor* DescriptorPool::Tables::FindEnumValueByNumber(
    const EnumDescriptor* parent, int number) const {
  return FindPtrOrNull(enum_values_by_number_, make_pair(parent, number));
}

// -------------------------------------------------------------------

bool DescriptorPool::Tables::AddSymbol(
    const string& full_name,
    const void* parent, const string& name,
    Symbol symbol) {
  if (InsertIfNotPresent(&symbols_by_name_, full_name.c_str(), symbol)) {
    symbols_after_checkpoint_.push_back(full_name.c_str());

    if (parent != NULL && !AddAliasUnderParent(parent, name, symbol)) {
      GOOGLE_LOG(DFATAL) << "\"" << full_name << "\" not previously defined in "
                     "symbols_by_name_, but was defined in symbols_by_parent_; "
                     "this shouldn't be possible.";
      return false;
    }

    return true;
  } else {
    return false;
  }
}

bool DescriptorPool::Tables::AddAliasUnderParent(
    const void* parent, const string& name, Symbol symbol) {
  PointerStringPair by_parent_key(parent, name.c_str());
  if (InsertIfNotPresent(&symbols_by_parent_, by_parent_key, symbol)) {
    symbols_by_parent_after_checkpoint_.push_back(by_parent_key);
    return true;
  } else {
    return false;
  }
}

bool DescriptorPool::Tables::AddFile(const FileDescriptor* file) {
  if (InsertIfNotPresent(&files_by_name_, file->name().c_str(), file)) {
    files_after_checkpoint_.push_back(file->name().c_str());
    return true;
  } else {
    return false;
  }
}

bool DescriptorPool::Tables::AddFieldByNumber(const FieldDescriptor* field) {
  DescriptorIntPair key(field->containing_type(), field->number());
  if (InsertIfNotPresent(&fields_by_number_, key, field)) {
    field_numbers_after_checkpoint_.push_back(key);
    return true;
  } else {
    return false;
  }
}

bool DescriptorPool::Tables::AddEnumValueByNumber(
    const EnumValueDescriptor* value) {
  EnumIntPair key(value->type(), value->number());
  if (InsertIfNotPresent(&enum_values_by_number_, key, value)) {
    enum_numbers_after_checkpoint_.push_back(key);
    return true;
  } else {
    return false;
  }
}

// -------------------------------------------------------------------

template<typename Type>
Type* DescriptorPool::Tables::Allocate() {
  return reinterpret_cast<Type*>(AllocateBytes(sizeof(Type)));
}

template<typename Type>
Type* DescriptorPool::Tables::AllocateArray(int count) {
  return reinterpret_cast<Type*>(AllocateBytes(sizeof(Type) * count));
}

string* DescriptorPool::Tables::AllocateString(const string& value) {
  string* result = new string(value);
  strings_.push_back(result);
  return result;
}

template<typename Type>
Type* DescriptorPool::Tables::AllocateMessage() {
  Type* result = new Type;
  messages_.push_back(result);
  return result;
}

void* DescriptorPool::Tables::AllocateBytes(int size) {
  // TODO(kenton):  Would it be worthwhile to implement this in some more
  // sophisticated way?  Probably not for the open source release, but for
  // internal use we could easily plug in one of our existing memory pool
  // allocators...
  if (size == 0) return NULL;

  void* result = operator new(size);
  allocations_.push_back(result);
  return result;
}

// ===================================================================
// DescriptorPool

DescriptorPool::ErrorCollector::~ErrorCollector() {}

DescriptorPool::DescriptorPool()
  : mutex_(NULL),
    fallback_database_(NULL),
    default_error_collector_(NULL),
    underlay_(NULL),
    tables_(new Tables),
    enforce_dependencies_(true),
    last_internal_build_generated_file_call_(NULL) {}

DescriptorPool::DescriptorPool(DescriptorDatabase* fallback_database,
                               ErrorCollector* error_collector)
  : mutex_(new Mutex),
    fallback_database_(fallback_database),
    default_error_collector_(error_collector),
    underlay_(NULL),
    tables_(new Tables),
    enforce_dependencies_(true),
    last_internal_build_generated_file_call_(NULL) {
}

DescriptorPool::DescriptorPool(const DescriptorPool* underlay)
  : mutex_(NULL),
    fallback_database_(NULL),
    default_error_collector_(NULL),
    underlay_(underlay),
    tables_(new Tables),
    last_internal_build_generated_file_call_(NULL) {}

DescriptorPool::~DescriptorPool() {
  if (mutex_ != NULL) delete mutex_;
}

// DescriptorPool::BuildFile() defined later.
// DescriptorPool::BuildFileCollectingErrors() defined later.
// DescriptorPool::InternalBuildGeneratedFile() defined later.

const DescriptorPool* DescriptorPool::generated_pool() {
  return internal_generated_pool();
}

DescriptorPool* DescriptorPool::internal_generated_pool() {
  static DescriptorPool singleton;
  return &singleton;
}

void DescriptorPool::InternalDontEnforceDependencies() {
  enforce_dependencies_ = false;
}

// Find*By* methods ==================================================

// TODO(kenton):  There's a lot of repeated code here, but I'm not sure if
//   there's any good way to factor it out.  Think about this some time when
//   there's nothing more important to do (read: never).

const FileDescriptor* DescriptorPool::FindFileByName(const string& name) const {
  MutexLockMaybe lock(mutex_);
  const FileDescriptor* result = tables_->FindFile(name);
  if (result != NULL) return result;
  if (underlay_ != NULL) {
    const FileDescriptor* result = underlay_->FindFileByName(name);
    if (result != NULL) return result;
  }
  if (TryFindFileInFallbackDatabase(name)) {
    const FileDescriptor* result = tables_->FindFile(name);
    if (result != NULL) return result;
  }
  return NULL;
}

const FileDescriptor* DescriptorPool::FindFileContainingSymbol(
    const string& symbol_name) const {
  MutexLockMaybe lock(mutex_);
  Symbol result = tables_->FindSymbol(symbol_name);
  if (!result.IsNull()) return result.GetFile();
  if (underlay_ != NULL) {
    const FileDescriptor* result =
      underlay_->FindFileContainingSymbol(symbol_name);
    if (result != NULL) return result;
  }
  if (TryFindSymbolInFallbackDatabase(symbol_name)) {
    Symbol result = tables_->FindSymbol(symbol_name);
    if (!result.IsNull()) return result.GetFile();
  }
  return NULL;
}

const Descriptor* DescriptorPool::FindMessageTypeByName(
    const string& name) const {
  MutexLockMaybe lock(mutex_);
  Symbol result = tables_->FindSymbolOfType(name, Symbol::MESSAGE);
  if (!result.IsNull()) return result.descriptor;
  if (underlay_ != NULL) {
    const Descriptor* result = underlay_->FindMessageTypeByName(name);
    if (result != NULL) return result;
  }
  if (TryFindSymbolInFallbackDatabase(name)) {
    Symbol result = tables_->FindSymbolOfType(name, Symbol::MESSAGE);
    if (!result.IsNull()) return result.descriptor;
  }
  return NULL;
}

const FieldDescriptor* DescriptorPool::FindFieldByName(
    const string& name) const {
  MutexLockMaybe lock(mutex_);
  Symbol result = tables_->FindSymbolOfType(name, Symbol::FIELD);
  if (!result.IsNull() && !result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  }
  if (underlay_ != NULL) {
    const FieldDescriptor* result = underlay_->FindFieldByName(name);
    if (result != NULL) return result;
  }
  if (TryFindSymbolInFallbackDatabase(name)) {
    Symbol result = tables_->FindSymbolOfType(name, Symbol::FIELD);
    if (!result.IsNull() && !result.field_descriptor->is_extension()) {
      return result.field_descriptor;
    }
  }
  return NULL;
}

const FieldDescriptor* DescriptorPool::FindExtensionByName(
    const string& name) const {
  MutexLockMaybe lock(mutex_);
  Symbol result = tables_->FindSymbolOfType(name, Symbol::FIELD);
  if (!result.IsNull() && result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  }
  if (underlay_ != NULL) {
    const FieldDescriptor* result = underlay_->FindExtensionByName(name);
    if (result != NULL) return result;
  }
  if (TryFindSymbolInFallbackDatabase(name)) {
    Symbol result = tables_->FindSymbolOfType(name, Symbol::FIELD);
    if (!result.IsNull() && result.field_descriptor->is_extension()) {
      return result.field_descriptor;
    }
  }
  return NULL;
}

const EnumDescriptor* DescriptorPool::FindEnumTypeByName(
    const string& name) const {
  MutexLockMaybe lock(mutex_);
  Symbol result = tables_->FindSymbolOfType(name, Symbol::ENUM);
  if (!result.IsNull()) return result.enum_descriptor;
  if (underlay_ != NULL) {
    const EnumDescriptor* result = underlay_->FindEnumTypeByName(name);
    if (result != NULL) return result;
  }
  if (TryFindSymbolInFallbackDatabase(name)) {
    Symbol result = tables_->FindSymbolOfType(name, Symbol::ENUM);
    if (!result.IsNull()) return result.enum_descriptor;
  }
  return NULL;
}

const EnumValueDescriptor* DescriptorPool::FindEnumValueByName(
    const string& name) const {
  MutexLockMaybe lock(mutex_);
  Symbol result = tables_->FindSymbolOfType(name, Symbol::ENUM_VALUE);
  if (!result.IsNull()) return result.enum_value_descriptor;
  if (underlay_ != NULL) {
    const EnumValueDescriptor* result = underlay_->FindEnumValueByName(name);
    if (result != NULL) return result;
  }
  if (TryFindSymbolInFallbackDatabase(name)) {
    Symbol result = tables_->FindSymbolOfType(name, Symbol::ENUM_VALUE);
    if (!result.IsNull()) return result.enum_value_descriptor;
  }
  return NULL;
}

const ServiceDescriptor* DescriptorPool::FindServiceByName(
    const string& name) const {
  MutexLockMaybe lock(mutex_);
  Symbol result = tables_->FindSymbolOfType(name, Symbol::SERVICE);
  if (!result.IsNull()) return result.service_descriptor;
  if (underlay_ != NULL) {
    const ServiceDescriptor* result = underlay_->FindServiceByName(name);
    if (result != NULL) return result;
  }
  if (TryFindSymbolInFallbackDatabase(name)) {
    Symbol result = tables_->FindSymbolOfType(name, Symbol::SERVICE);
    if (!result.IsNull()) return result.service_descriptor;
  }
  return NULL;
}

const MethodDescriptor* DescriptorPool::FindMethodByName(
    const string& name) const {
  MutexLockMaybe lock(mutex_);
  Symbol result = tables_->FindSymbolOfType(name, Symbol::METHOD);
  if (!result.IsNull()) return result.method_descriptor;
  if (underlay_ != NULL) {
    const MethodDescriptor* result = underlay_->FindMethodByName(name);
    if (result != NULL) return result;
  }
  if (TryFindSymbolInFallbackDatabase(name)) {
    Symbol result = tables_->FindSymbolOfType(name, Symbol::METHOD);
    if (!result.IsNull()) return result.method_descriptor;
  }
  return NULL;
}

const FieldDescriptor* DescriptorPool::FindExtensionByNumber(
    const Descriptor* extendee, int number) const {
  MutexLockMaybe lock(mutex_);
  const FieldDescriptor* result = tables_->FindFieldByNumber(extendee, number);
  if (result != NULL && result->is_extension()) {
    return result;
  }
  if (underlay_ != NULL) {
    const FieldDescriptor* result =
      underlay_->FindExtensionByNumber(extendee, number);
    if (result != NULL) return result;
  }
  if (TryFindExtensionInFallbackDatabase(extendee, number)) {
    const FieldDescriptor* result =
      tables_->FindFieldByNumber(extendee, number);
    if (result != NULL && result->is_extension()) {
      return result;
    }
  }
  return NULL;
}

// -------------------------------------------------------------------

const FieldDescriptor*
Descriptor::FindFieldByNumber(int key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  const FieldDescriptor* result =
    file()->pool()->tables_->FindFieldByNumber(this, key);
  if (result == NULL || result->is_extension()) {
    return NULL;
  } else {
    return result;
  }
}

const FieldDescriptor*
Descriptor::FindFieldByName(const string& key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  Symbol result =
    file()->pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::FIELD);
  if (!result.IsNull() && !result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  } else {
    return NULL;
  }
}

const FieldDescriptor*
Descriptor::FindExtensionByName(const string& key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  Symbol result =
    file()->pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::FIELD);
  if (!result.IsNull() && result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  } else {
    return NULL;
  }
}

const Descriptor*
Descriptor::FindNestedTypeByName(const string& key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  Symbol result =
    file()->pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::MESSAGE);
  if (!result.IsNull()) {
    return result.descriptor;
  } else {
    return NULL;
  }
}

const EnumDescriptor*
Descriptor::FindEnumTypeByName(const string& key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  Symbol result =
    file()->pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::ENUM);
  if (!result.IsNull()) {
    return result.enum_descriptor;
  } else {
    return NULL;
  }
}

const EnumValueDescriptor*
Descriptor::FindEnumValueByName(const string& key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  Symbol result =
    file()->pool()->tables_->FindNestedSymbolOfType(
      this, key, Symbol::ENUM_VALUE);
  if (!result.IsNull()) {
    return result.enum_value_descriptor;
  } else {
    return NULL;
  }
}

const EnumValueDescriptor*
EnumDescriptor::FindValueByName(const string& key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  Symbol result =
    file()->pool()->tables_->FindNestedSymbolOfType(
      this, key, Symbol::ENUM_VALUE);
  if (!result.IsNull()) {
    return result.enum_value_descriptor;
  } else {
    return NULL;
  }
}

const EnumValueDescriptor*
EnumDescriptor::FindValueByNumber(int key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  return file()->pool()->tables_->FindEnumValueByNumber(this, key);
}

const MethodDescriptor*
ServiceDescriptor::FindMethodByName(const string& key) const {
  MutexLockMaybe lock(file()->pool()->mutex_);
  Symbol result =
    file()->pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::METHOD);
  if (!result.IsNull()) {
    return result.method_descriptor;
  } else {
    return NULL;
  }
}

const Descriptor*
FileDescriptor::FindMessageTypeByName(const string& key) const {
  MutexLockMaybe lock(pool()->mutex_);
  Symbol result =
    pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::MESSAGE);
  if (!result.IsNull()) {
    return result.descriptor;
  } else {
    return NULL;
  }
}

const EnumDescriptor*
FileDescriptor::FindEnumTypeByName(const string& key) const {
  MutexLockMaybe lock(pool()->mutex_);
  Symbol result =
    pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::ENUM);
  if (!result.IsNull()) {
    return result.enum_descriptor;
  } else {
    return NULL;
  }
}

const EnumValueDescriptor*
FileDescriptor::FindEnumValueByName(const string& key) const {
  MutexLockMaybe lock(pool()->mutex_);
  Symbol result =
    pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::ENUM_VALUE);
  if (!result.IsNull()) {
    return result.enum_value_descriptor;
  } else {
    return NULL;
  }
}

const ServiceDescriptor*
FileDescriptor::FindServiceByName(const string& key) const {
  MutexLockMaybe lock(pool()->mutex_);
  Symbol result =
    pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::SERVICE);
  if (!result.IsNull()) {
    return result.service_descriptor;
  } else {
    return NULL;
  }
}

const FieldDescriptor*
FileDescriptor::FindExtensionByName(const string& key) const {
  MutexLockMaybe lock(pool()->mutex_);
  Symbol result =
    pool()->tables_->FindNestedSymbolOfType(this, key, Symbol::FIELD);
  if (!result.IsNull() && result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  } else {
    return NULL;
  }
}

bool Descriptor::IsExtensionNumber(int number) const {
  // Linear search should be fine because we don't expect a message to have
  // more than a couple extension ranges.
  for (int i = 0; i < extension_range_count(); i++) {
    if (number >= extension_range(i)->start &&
        number <  extension_range(i)->end) {
      return true;
    }
  }
  return false;
}

// -------------------------------------------------------------------

bool DescriptorPool::TryFindFileInFallbackDatabase(const string& name) const {
  if (fallback_database_ == NULL) return false;

  if (tables_->known_bad_files_.count(name) > 0) return false;

  FileDescriptorProto file_proto;
  if (!fallback_database_->FindFileByName(name, &file_proto) ||
      BuildFileFromDatabase(file_proto) == NULL) {
    tables_->known_bad_files_.insert(name);
    return false;
  }

  return true;
}

bool DescriptorPool::TryFindSymbolInFallbackDatabase(const string& name) const {
  if (fallback_database_ == NULL) return false;

  FileDescriptorProto file_proto;
  if (!fallback_database_->FindFileContainingSymbol(name, &file_proto)) {
    return false;
  }

  if (tables_->FindFile(file_proto.name()) != NULL) {
    // We've already loaded this file, and it apparently doesn't contain the
    // symbol we're looking for.  Some DescriptorDatabases return false
    // positives.
    return false;
  }

  if (BuildFileFromDatabase(file_proto) == NULL) {
    return false;
  }

  return true;
}

bool DescriptorPool::TryFindExtensionInFallbackDatabase(
    const Descriptor* containing_type, int field_number) const {
  if (fallback_database_ == NULL) return false;

  FileDescriptorProto file_proto;
  if (!fallback_database_->FindFileContainingExtension(
        containing_type->full_name(), field_number, &file_proto)) {
    return false;
  }

  if (tables_->FindFile(file_proto.name()) != NULL) {
    // We've already loaded this file, and it apparently doesn't contain the
    // extension we're looking for.  Some DescriptorDatabases return false
    // positives.
    return false;
  }

  if (BuildFileFromDatabase(file_proto) == NULL) {
    return false;
  }

  return true;
}

// ===================================================================

string FieldDescriptor::DefaultValueAsString(bool quote_string_type) const {
  GOOGLE_CHECK(has_default_value()) << "No default value";
  switch (cpp_type()) {
    case CPPTYPE_INT32:
      return SimpleItoa(default_value_int32());
      break;
    case CPPTYPE_INT64:
      return SimpleItoa(default_value_int64());
      break;
    case CPPTYPE_UINT32:
      return SimpleItoa(default_value_uint32());
      break;
    case CPPTYPE_UINT64:
      return SimpleItoa(default_value_uint64());
      break;
    case CPPTYPE_FLOAT:
      return SimpleFtoa(default_value_float());
      break;
    case CPPTYPE_DOUBLE:
      return SimpleDtoa(default_value_double());
      break;
    case CPPTYPE_BOOL:
      return default_value_bool() ? "true" : "false";
      break;
    case CPPTYPE_STRING:
      if (quote_string_type) {
        return "\"" + CEscape(default_value_string()) + "\"";
      } else {
        if (type() == TYPE_BYTES) {
          return CEscape(default_value_string());
        } else {
          return default_value_string();
        }
      }
      break;
    case CPPTYPE_ENUM:
      return default_value_enum()->name();
      break;
    case CPPTYPE_MESSAGE:
      GOOGLE_LOG(DFATAL) << "Messages can't have default values!";
      break;
  }
  GOOGLE_LOG(FATAL) << "Can't get here: failed to get default value as string";
  return "";
}

// CopyTo methods ====================================================

void FileDescriptor::CopyTo(FileDescriptorProto* proto) const {
  proto->set_name(name());
  if (!package().empty()) proto->set_package(package());

  for (int i = 0; i < dependency_count(); i++) {
    proto->add_dependency(dependency(i)->name());
  }

  for (int i = 0; i < message_type_count(); i++) {
    message_type(i)->CopyTo(proto->add_message_type());
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->CopyTo(proto->add_enum_type());
  }
  for (int i = 0; i < service_count(); i++) {
    service(i)->CopyTo(proto->add_service());
  }
  for (int i = 0; i < extension_count(); i++) {
    extension(i)->CopyTo(proto->add_extension());
  }

  if (&options() != &FileOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void Descriptor::CopyTo(DescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < field_count(); i++) {
    field(i)->CopyTo(proto->add_field());
  }
  for (int i = 0; i < nested_type_count(); i++) {
    nested_type(i)->CopyTo(proto->add_nested_type());
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->CopyTo(proto->add_enum_type());
  }
  for (int i = 0; i < extension_range_count(); i++) {
    DescriptorProto::ExtensionRange* range = proto->add_extension_range();
    range->set_start(extension_range(i)->start);
    range->set_end(extension_range(i)->end);
  }
  for (int i = 0; i < extension_count(); i++) {
    extension(i)->CopyTo(proto->add_extension());
  }

  if (&options() != &MessageOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void FieldDescriptor::CopyTo(FieldDescriptorProto* proto) const {
  proto->set_name(name());
  proto->set_number(number());
  proto->set_label(static_cast<FieldDescriptorProto::Label>(label()));
  proto->set_type(static_cast<FieldDescriptorProto::Type>(type()));

  if (is_extension()) {
    proto->set_extendee(".");
    proto->mutable_extendee()->append(containing_type()->full_name());
  }

  if (cpp_type() == CPPTYPE_MESSAGE) {
    proto->set_type_name(".");
    proto->mutable_type_name()->append(message_type()->full_name());
  } else if (cpp_type() == CPPTYPE_ENUM) {
    proto->set_type_name(".");
    proto->mutable_type_name()->append(enum_type()->full_name());
  }

  if (has_default_value()) {
    proto->set_default_value(DefaultValueAsString(false));
  }

  if (&options() != &FieldOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void EnumDescriptor::CopyTo(EnumDescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < value_count(); i++) {
    value(i)->CopyTo(proto->add_value());
  }

  if (&options() != &EnumOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void EnumValueDescriptor::CopyTo(EnumValueDescriptorProto* proto) const {
  proto->set_name(name());
  proto->set_number(number());

  if (&options() != &EnumValueOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void ServiceDescriptor::CopyTo(ServiceDescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < method_count(); i++) {
    method(i)->CopyTo(proto->add_method());
  }

  if (&options() != &ServiceOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void MethodDescriptor::CopyTo(MethodDescriptorProto* proto) const {
  proto->set_name(name());

  proto->set_input_type(".");
  proto->mutable_input_type()->append(input_type()->full_name());
  proto->set_output_type(".");
  proto->mutable_output_type()->append(output_type()->full_name());

  if (&options() != &MethodOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

// DebugString methods ===============================================

namespace {

// Used by each of the option formatters.
bool RetrieveOptions(const Message &options, vector<string> *option_entries) {
  option_entries->clear();
  const Reflection* reflection = options.GetReflection();
  vector<const FieldDescriptor*> fields;
  reflection->ListFields(options, &fields);
  for (int i = 0; i < fields.size(); i++) {
    // Doesn't make sense to have message type fields here
    if (fields[i]->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      continue;
    }
    int count = 1;
    bool repeated = false;
    if (fields[i]->is_repeated()) {
      count = reflection->FieldSize(options, fields[i]);
      repeated = true;
    }
    for (int j = 0; j < count; j++) {
      string fieldval;
      TextFormat::PrintFieldValueToString(options, fields[i],
                                          repeated ? count : -1, &fieldval);
      option_entries->push_back(fields[i]->name() + " = " + fieldval);
    }
  }
  return !option_entries->empty();
}

// Formats options that all appear together in brackets. Does not include
// brackets.
bool FormatBracketedOptions(const Message &options, string *output) {
  vector<string> all_options;
  if (RetrieveOptions(options, &all_options)) {
    output->append(JoinStrings(all_options, ", "));
  }
  return !all_options.empty();
}

// Formats options one per line
bool FormatLineOptions(int depth, const Message &options, string *output) {
  string prefix(depth * 2, ' ');
  vector<string> all_options;
  if (RetrieveOptions(options, &all_options)) {
    for (int i = 0; i < all_options.size(); i++) {
      strings::SubstituteAndAppend(output, "$0option $1;\n",
                                   prefix, all_options[i]);
    }
  }
  return !all_options.empty();
}

}  // anonymous namespace

string FileDescriptor::DebugString() const {
  string contents = "syntax = \"proto2\";\n\n";

  for (int i = 0; i < dependency_count(); i++) {
    strings::SubstituteAndAppend(&contents, "import \"$0\";\n",
                                 dependency(i)->name());
  }

  if (!package().empty()) {
    strings::SubstituteAndAppend(&contents, "package $0;\n\n", package());
  }

  if (FormatLineOptions(0, options(), &contents)) {
    contents.append("\n");  // add some space if we had options
  }

  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->DebugString(0, &contents);
    contents.append("\n");
  }

  // Find all the 'group' type extensions; we will not output their nested
  // definitions (those will be done with their group field descriptor).
  set<const Descriptor*> groups;
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->type() == FieldDescriptor::TYPE_GROUP) {
      groups.insert(extension(i)->message_type());
    }
  }

  for (int i = 0; i < message_type_count(); i++) {
    if (groups.count(message_type(i)) == 0) {
      strings::SubstituteAndAppend(&contents, "message $0",
                                   message_type(i)->name());
      message_type(i)->DebugString(0, &contents);
      contents.append("\n");
    }
  }

  for (int i = 0; i < service_count(); i++) {
    service(i)->DebugString(&contents);
    contents.append("\n");
  }

  const Descriptor* containing_type = NULL;
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->containing_type() != containing_type) {
      if (i > 0) contents.append("}\n\n");
      containing_type = extension(i)->containing_type();
      strings::SubstituteAndAppend(&contents, "extend .$0 {\n",
                                   containing_type->full_name());
    }
    extension(i)->DebugString(1, &contents);
  }
  if (extension_count() > 0) contents.append("}\n\n");

  return contents;
}

string Descriptor::DebugString() const {
  string contents;
  strings::SubstituteAndAppend(&contents, "message $0", name());
  DebugString(0, &contents);
  return contents;
}

void Descriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  ++depth;
  contents->append(" {\n");

  FormatLineOptions(depth, options(), contents);

  // Find all the 'group' types for fields and extensions; we will not output
  // their nested definitions (those will be done with their group field
  // descriptor).
  set<const Descriptor*> groups;
  for (int i = 0; i < field_count(); i++) {
    if (field(i)->type() == FieldDescriptor::TYPE_GROUP) {
      groups.insert(field(i)->message_type());
    }
  }
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->type() == FieldDescriptor::TYPE_GROUP) {
      groups.insert(extension(i)->message_type());
    }
  }

  for (int i = 0; i < nested_type_count(); i++) {
    if (groups.count(nested_type(i)) == 0) {
      strings::SubstituteAndAppend(contents, "$0  message $1",
                                   prefix, nested_type(i)->name());
      nested_type(i)->DebugString(depth, contents);
    }
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->DebugString(depth, contents);
  }
  for (int i = 0; i < field_count(); i++) {
    field(i)->DebugString(depth, contents);
  }

  for (int i = 0; i < extension_range_count(); i++) {
    strings::SubstituteAndAppend(contents, "$0  extensions $1 to $2;\n",
                                 prefix,
                                 extension_range(i)->start,
                                 extension_range(i)->end - 1);
  }

  // Group extensions by what they extend, so they can be printed out together.
  const Descriptor* containing_type = NULL;
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->containing_type() != containing_type) {
      if (i > 0) strings::SubstituteAndAppend(contents, "$0  }\n", prefix);
      containing_type = extension(i)->containing_type();
      strings::SubstituteAndAppend(contents, "$0  extend .$1 {\n",
                                   prefix, containing_type->full_name());
    }
    extension(i)->DebugString(depth + 1, contents);
  }
  if (extension_count() > 0)
    strings::SubstituteAndAppend(contents, "$0  }\n", prefix);

  strings::SubstituteAndAppend(contents, "$0}\n", prefix);
}

string FieldDescriptor::DebugString() const {
  string contents;
  int depth = 0;
  if (is_extension()) {
    strings::SubstituteAndAppend(&contents, "extend .$0 {\n",
                                 containing_type()->full_name());
    depth = 1;
  }
  DebugString(depth, &contents);
  if (is_extension()) {
     contents.append("}\n");
  }
  return contents;
}

void FieldDescriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  string field_type;
  switch (type()) {
    case TYPE_MESSAGE:
      field_type = "." + message_type()->full_name();
      break;
    case TYPE_ENUM:
      field_type = "." + enum_type()->full_name();
      break;
    default:
      field_type = kTypeToName[type()];
  }

  strings::SubstituteAndAppend(contents, "$0$1 $2 $3 = $4",
                               prefix,
                               kLabelToName[label()],
                               field_type,
                               type() == TYPE_GROUP ? message_type()->name() :
                                                      name(),
                               number());

  bool bracketed = false;
  if (has_default_value()) {
    bracketed = true;
    strings::SubstituteAndAppend(contents, " [default = $0",
                                 DefaultValueAsString(true));
  }

  string formatted_options;
  if (FormatBracketedOptions(options(), &formatted_options)) {
    contents->append(bracketed ? ", " : " [");
    bracketed = true;
    contents->append(formatted_options);
  }

  if (bracketed) {
    contents->append("]");
  }

  if (type() == TYPE_GROUP) {
    message_type()->DebugString(depth, contents);
  } else {
    contents->append(";\n");
  }
}

string EnumDescriptor::DebugString() const {
  string contents;
  DebugString(0, &contents);
  return contents;
}

void EnumDescriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  ++depth;
  strings::SubstituteAndAppend(contents, "$0enum $1 {\n",
                               prefix, name());

  FormatLineOptions(depth, options(), contents);

  for (int i = 0; i < value_count(); i++) {
    value(i)->DebugString(depth, contents);
  }
  strings::SubstituteAndAppend(contents, "$0}\n", prefix);
}

string EnumValueDescriptor::DebugString() const {
  string contents;
  DebugString(0, &contents);
  return contents;
}

void EnumValueDescriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  strings::SubstituteAndAppend(contents, "$0$1 = $2",
                               prefix, name(), number());

  string formatted_options;
  if (FormatBracketedOptions(options(), &formatted_options)) {
    strings::SubstituteAndAppend(contents, " [$0]", formatted_options);
  }
  contents->append(";\n");
}

string ServiceDescriptor::DebugString() const {
  string contents;
  DebugString(&contents);
  return contents;
}

void ServiceDescriptor::DebugString(string *contents) const {
  strings::SubstituteAndAppend(contents, "service $0 {\n", name());

  FormatLineOptions(1, options(), contents);

  for (int i = 0; i < method_count(); i++) {
    method(i)->DebugString(1, contents);
  }

  contents->append("}\n");
}

string MethodDescriptor::DebugString() const {
  string contents;
  DebugString(0, &contents);
  return contents;
}

void MethodDescriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  ++depth;
  strings::SubstituteAndAppend(contents, "$0rpc $1(.$2) returns (.$3)",
                               prefix, name(),
                               input_type()->full_name(),
                               output_type()->full_name());

  string formatted_options;
  if (FormatLineOptions(depth, options(), &formatted_options)) {
    strings::SubstituteAndAppend(contents, " {\n$0$1}\n",
                                 formatted_options, prefix);
  } else {
    contents->append(";\n");
  }
}
// ===================================================================

class DescriptorBuilder {
 public:
  DescriptorBuilder(const DescriptorPool* pool,
                    DescriptorPool::Tables* tables,
                    DescriptorPool::ErrorCollector* error_collector);
  ~DescriptorBuilder();

  const FileDescriptor* BuildFile(const FileDescriptorProto& proto);

 private:
  const DescriptorPool* pool_;
  DescriptorPool::Tables* tables_;  // for convenience
  DescriptorPool::ErrorCollector* error_collector_;
  bool had_errors_;
  string filename_;
  FileDescriptor* file_;

  // If LookupSymbol() finds a symbol that is in a file which is not a declared
  // dependency of this file, it will fail, but will set
  // possible_undeclared_dependency_ to point at that file.  This is only used
  // by AddNotDefinedError() to report a more useful error message.
  const FileDescriptor* possible_undeclared_dependency_;

  void AddError(const string& element_name,
                const Message& descriptor,
                DescriptorPool::ErrorCollector::ErrorLocation location,
                const string& error);

  // Adds an error indicating that undefined_symbol was not defined.  Must
  // only be called after LookupSymbol() fails.
  void AddNotDefinedError(
    const string& element_name,
    const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const string& undefined_symbol);

  // Silly helper which determines if the given file is in the given package.
  // I.e., either file->package() == package_name or file->package() is a
  // nested package within package_name.
  bool IsInPackage(const FileDescriptor* file, const string& package_name);

  // Like tables_->FindSymbol(), but additionally:
  // - Search the pool's underlay if not found in tables_.
  // - Insure that the resulting Symbol is from one of the file's declared
  //   dependencies.
  Symbol FindSymbol(const string& name);

  // Like FindSymbol(), but looks up the name relative to some other symbol
  // name.  This first searches syblings of relative_to, then siblings of its
  // parents, etc.  For example, LookupSymbol("foo.bar", "baz.qux.corge") makes
  // the following calls, returning the first non-null result:
  // FindSymbol("baz.qux.foo.bar"), FindSymbol("baz.foo.bar"),
  // FindSymbol("foo.bar").
  Symbol LookupSymbol(const string& name, const string& relative_to);

  // Calls tables_->AddSymbol() and records an error if it fails.  Returns
  // true if successful or false if failed, though most callers can ignore
  // the return value since an error has already been recorded.
  bool AddSymbol(const string& full_name,
                 const void* parent, const string& name,
                 const Message& proto, Symbol symbol);

  // Like AddSymbol(), but succeeds if the symbol is already defined as long
  // as the existing definition is also a package (because it's OK to define
  // the same package in two different files).  Also adds all parents of the
  // packgae to the symbol table (e.g. AddPackage("foo.bar", ...) will add
  // "foo.bar" and "foo" to the table).
  void AddPackage(const string& name, const Message& proto,
                  const FileDescriptor* file);

  // Checks that the symbol name contains only alphanumeric characters and
  // underscores.  Records an error otherwise.
  void ValidateSymbolName(const string& name, const string& full_name,
                          const Message& proto);

  // Used by BUILD_ARRAY macro (below) to avoid having to have the type
  // specified as a macro parameter.
  template <typename Type>
  inline void AllocateArray(int size, Type** output) {
    *output = tables_->AllocateArray<Type>(size);
  }

  // These methods all have the same signature for the sake of the BUILD_ARRAY
  // macro, below.
  void BuildMessage(const DescriptorProto& proto,
                    const Descriptor* parent,
                    Descriptor* result);
  void BuildFieldOrExtension(const FieldDescriptorProto& proto,
                             const Descriptor* parent,
                             FieldDescriptor* result,
                             bool is_extension);
  void BuildField(const FieldDescriptorProto& proto,
                  const Descriptor* parent,
                  FieldDescriptor* result) {
    BuildFieldOrExtension(proto, parent, result, false);
  }
  void BuildExtension(const FieldDescriptorProto& proto,
                      const Descriptor* parent,
                      FieldDescriptor* result) {
    BuildFieldOrExtension(proto, parent, result, true);
  }
  void BuildExtensionRange(const DescriptorProto::ExtensionRange& proto,
                           const Descriptor* parent,
                           Descriptor::ExtensionRange* result);
  void BuildEnum(const EnumDescriptorProto& proto,
                 const Descriptor* parent,
                 EnumDescriptor* result);
  void BuildEnumValue(const EnumValueDescriptorProto& proto,
                      const EnumDescriptor* parent,
                      EnumValueDescriptor* result);
  void BuildService(const ServiceDescriptorProto& proto,
                    const void* dummy,
                    ServiceDescriptor* result);
  void BuildMethod(const MethodDescriptorProto& proto,
                   const ServiceDescriptor* parent,
                   MethodDescriptor* result);

  void CrossLinkFile(FileDescriptor* file, const FileDescriptorProto& proto);
  void CrossLinkMessage(Descriptor* message, const DescriptorProto& proto);
  void CrossLinkField(FieldDescriptor* field,
                      const FieldDescriptorProto& proto);
  void CrossLinkService(ServiceDescriptor* service,
                        const ServiceDescriptorProto& proto);
  void CrossLinkMethod(MethodDescriptor* method,
                       const MethodDescriptorProto& proto);
  void CrossLinkMapKey(FieldDescriptor* field,
                       const FieldDescriptorProto& proto);
};

const FileDescriptor* DescriptorPool::BuildFile(
    const FileDescriptorProto& proto) {
  GOOGLE_CHECK(fallback_database_ == NULL)
    << "Cannot call BuildFile on a DescriptorPool that uses a "
       "DescriptorDatabase.  You must instead find a way to get your file "
       "into the underlying database.";
  GOOGLE_CHECK(mutex_ == NULL);   // Implied by the above GOOGLE_CHECK.
  return DescriptorBuilder(this, tables_.get(), NULL).BuildFile(proto);
}

const FileDescriptor* DescriptorPool::BuildFileCollectingErrors(
    const FileDescriptorProto& proto,
    ErrorCollector* error_collector) {
  GOOGLE_CHECK(fallback_database_ == NULL)
    << "Cannot call BuildFile on a DescriptorPool that uses a "
       "DescriptorDatabase.  You must instead find a way to get your file "
       "into the underlying database.";
  GOOGLE_CHECK(mutex_ == NULL);   // Implied by the above GOOGLE_CHECK.
  return DescriptorBuilder(this, tables_.get(),
                           error_collector).BuildFile(proto);
}

const FileDescriptor* DescriptorPool::BuildFileFromDatabase(
    const FileDescriptorProto& proto) const {
  mutex_->AssertHeld();
  return DescriptorBuilder(this, tables_.get(),
                           default_error_collector_).BuildFile(proto);
}

const FileDescriptor* DescriptorPool::InternalBuildGeneratedFile(
    const void* data, int size) {
  // So, this function is called in the process of initializing the
  // descriptors for generated proto classes.  Each generated .pb.cc file
  // has an internal procedure called BuildDescriptors() which is called the
  // first time one of its descriptors is accessed, and that function calls
  // this one in order to parse the raw bytes of the FileDescriptorProto
  // representing the file.
  //
  // Note, though, that FileDescriptorProto is itself a generated protocol
  // message.  So, when we attempt to construct one below, it will attempt
  // to initialize its own descriptors via its own BuildDescriptors() method.
  // This will in turn cause InternalBuildGeneratedFile() to build
  // descriptor.proto's descriptors.
  //
  // We are saved from an infinite loop by the fact that BuildDescriptors()
  // only does anything the first time it is called.  That is, the first few
  // lines of any BuildDescriptors() procedure look like this:
  //   void BuildDescriptors() {
  //     static bool already_here = false;
  //     if (already_here) return;
  //     already_here = true;
  //     ...
  // So, when descriptor.pb.cc's BuildDescriptors() is called recursively, it
  // will end up just returning without doing anything.  The result is that
  // all of the descriptors for FileDescriptorProto and friends will just be
  // NULL.
  //
  // Luckily, it turns out that our limited use of FileDescriptorProto within
  // InternalBuildGeneratedFile() does not require that its descriptors be
  // initialized.  So, this ends up working.  As soon as
  // InternalBuildGeneratedFile() returns, the descriptors will be initialized
  // by the original call to BuildDescriptors(), and everything will be happy
  // again.
  //
  // If this turns out to be too fragile a hack, there are other ways we
  // can accomplish bootstrapping here (like building the descriptor for
  // descriptor.proto manually), but if this works then it's a lot easier.
  //
  // Note that because this is only triggered at static initialization time,
  // there are no thread-safety concerns here.
  FileDescriptorProto proto;
  GOOGLE_CHECK(proto.ParseFromArray(data, size));
  const FileDescriptor* result = BuildFile(proto);
  GOOGLE_CHECK(result != NULL);

  return result;
}

DescriptorBuilder::DescriptorBuilder(
    const DescriptorPool* pool,
    DescriptorPool::Tables* tables,
    DescriptorPool::ErrorCollector* error_collector)
  : pool_(pool),
    tables_(tables),
    error_collector_(error_collector),
    had_errors_(false),
    possible_undeclared_dependency_(NULL) {}

DescriptorBuilder::~DescriptorBuilder() {}

void DescriptorBuilder::AddError(
    const string& element_name,
    const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const string& error) {
  if (error_collector_ == NULL) {
    if (!had_errors_) {
      GOOGLE_LOG(ERROR) << "Invalid proto descriptor for file \"" << filename_
                 << "\":";
    }
    GOOGLE_LOG(ERROR) << "  " << element_name << ": " << error;
  } else {
    error_collector_->AddError(filename_, element_name,
                               &descriptor, location, error);
  }
  had_errors_ = true;
}

void DescriptorBuilder::AddNotDefinedError(
    const string& element_name,
    const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const string& undefined_symbol) {
  if (possible_undeclared_dependency_ == NULL) {
    AddError(element_name, descriptor, location,
             "\"" + undefined_symbol + "\" is not defined.");
  } else {
    AddError(element_name, descriptor, location,
             "\"" + undefined_symbol + "\" seems to be defined in \""
             + possible_undeclared_dependency_->name() + "\", which is not "
             "imported by \"" + filename_ + "\".  To use it here, please "
             "add the necessary import.");
  }
}

bool DescriptorBuilder::IsInPackage(const FileDescriptor* file,
                                    const string& package_name) {
  return HasPrefixString(file->package(), package_name) &&
           (file->package().size() == package_name.size() ||
            file->package()[package_name.size()] == '.');
}

Symbol DescriptorBuilder::FindSymbol(const string& name) {
  Symbol result;

  // We need to search our pool and all its underlays.
  const DescriptorPool* pool = pool_;
  while (true) {
    // If we are looking at an underlay, we must lock its mutex_, since we are
    // accessing the underlay's tables_ dircetly.
    MutexLockMaybe lock((pool == pool_) ? NULL : pool->mutex_);

    // Note that we don't have to check fallback_database_ here because the
    // symbol has to be in one of its file's direct dependencies, and we have
    // already loaded those by the time we get here.
    result = pool->tables_->FindSymbol(name);
    if (!result.IsNull()) break;
    if (pool->underlay_ == NULL) return kNullSymbol;
    pool = pool->underlay_;
  }

  if (!pool_->enforce_dependencies_) {
    // Hack for CompilerUpgrader.
    return result;
  }

  // Only find symbols which were defined in this file or one of its
  // dependencies.
  const FileDescriptor* file = result.GetFile();
  if (file == file_) return result;
  for (int i = 0; i < file_->dependency_count(); i++) {
    if (file == file_->dependency(i)) return result;
  }

  if (result.type == Symbol::PACKAGE) {
    // Arg, this is overcomplicated.  The symbol is a package name.  It could
    // be that the package was defined in multiple files.  result.GetFile()
    // returns the first file we saw that used this package.  We've determined
    // that that file is not a direct dependency of the file we are currently
    // building, but it could be that some other file which *is* a direct
    // dependency also defines the same package.  We can't really rule out this
    // symbol unless none of the dependencies define it.
    if (IsInPackage(file_, name)) return result;
    for (int i = 0; i < file_->dependency_count(); i++) {
      if (IsInPackage(file_->dependency(i), name)) return result;
    }
  }

  possible_undeclared_dependency_ = file;
  return kNullSymbol;
}

Symbol DescriptorBuilder::LookupSymbol(
    const string& name, const string& relative_to) {
  possible_undeclared_dependency_ = NULL;

  if (name.size() > 0 && name[0] == '.') {
    // Fully-qualified name.
    return FindSymbol(name.substr(1));
  }

  // If name is something like "Foo.Bar.baz", and symbols named "Foo" are
  // defined in multiple parent scopes, we only want to find "Bar.baz" in the
  // innermost one.  E.g., the following should produce an error:
  //   message Bar { message Baz {} }
  //   message Foo {
  //     message Bar {
  //     }
  //     optional Bar.Baz baz = 1;
  //   }
  // So, we look for just "Foo" first, then look for "Bar.baz" within it if
  // found.
  int name_dot_pos = name.find_first_of('.');
  string first_part_of_name;
  if (name_dot_pos == string::npos) {
    first_part_of_name = name;
  } else {
    first_part_of_name = name.substr(0, name_dot_pos);
  }

  string scope_to_try(relative_to);

  while (true) {
    // Chop off the last component of the scope.
    string::size_type dot_pos = scope_to_try.find_last_of('.');
    if (dot_pos == string::npos) {
      return FindSymbol(name);
    } else {
      scope_to_try.erase(dot_pos);
    }

    // Append ".first_part_of_name" and try to find.
    string::size_type old_size = scope_to_try.size();
    scope_to_try.append(1, '.');
    scope_to_try.append(first_part_of_name);
    Symbol result = FindSymbol(scope_to_try);
    if (!result.IsNull()) {
      if (first_part_of_name.size() < name.size()) {
        // name is a compound symbol, of which we only found the first part.
        // Now try to look up the rest of it.
        scope_to_try.append(name, first_part_of_name.size(),
                            name.size() - first_part_of_name.size());
        result = FindSymbol(scope_to_try);
      }
      return result;
    }

    // Not found.  Remove the name so we can try again.
    scope_to_try.erase(old_size);
  }
}

bool DescriptorBuilder::AddSymbol(
    const string& full_name, const void* parent, const string& name,
    const Message& proto, Symbol symbol) {
  // If the caller passed NULL for the parent, the symbol is at file scope.
  // Use its file as the parent instead.
  if (parent == NULL) parent = file_;

  if (tables_->AddSymbol(full_name, parent, name, symbol)) {
    return true;
  } else {
    const FileDescriptor* other_file = tables_->FindSymbol(full_name).GetFile();
    if (other_file == file_) {
      string::size_type dot_pos = full_name.find_last_of('.');
      if (dot_pos == string::npos) {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                 "\"" + full_name + "\" is already defined.");
      } else {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                 "\"" + full_name.substr(dot_pos + 1) +
                 "\" is already defined in \"" +
                 full_name.substr(0, dot_pos) + "\".");
      }
    } else {
      // Symbol seems to have been defined in a different file.
      AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
               "\"" + full_name + "\" is already defined in file \"" +
               other_file->name() + "\".");
    }
    return false;
  }
}

void DescriptorBuilder::AddPackage(
    const string& name, const Message& proto, const FileDescriptor* file) {
  if (tables_->AddSymbol(name, NULL, name, Symbol(file))) {
    // Success.  Also add parent package, if any.
    string::size_type dot_pos = name.find_last_of('.');
    if (dot_pos == string::npos) {
      // No parents.
      ValidateSymbolName(name, name, proto);
    } else {
      // Has parent.
      string* parent_name = tables_->AllocateString(name.substr(0, dot_pos));
      AddPackage(*parent_name, proto, file);
      ValidateSymbolName(name.substr(dot_pos + 1), name, proto);
    }
  } else {
    Symbol existing_symbol = tables_->FindSymbol(name);
    // It's OK to redefine a package.
    if (existing_symbol.type != Symbol::PACKAGE) {
      // Symbol seems to have been defined in a different file.
      AddError(name, proto, DescriptorPool::ErrorCollector::NAME,
               "\"" + name + "\" is already defined (as something other than "
               "a package) in file \"" + existing_symbol.GetFile()->name() +
               "\".");
    }
  }
}

void DescriptorBuilder::ValidateSymbolName(
    const string& name, const string& full_name, const Message& proto) {
  if (name.empty()) {
    AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
             "Missing name.");
  } else {
    for (int i = 0; i < name.size(); i++) {
      // I don't trust isalnum() due to locales.  :(
      if ((name[i] < 'a' || 'z' < name[i]) &&
          (name[i] < 'A' || 'Z' < name[i]) &&
          (name[i] < '0' || '9' < name[i]) &&
          (name[i] != '_')) {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                 "\"" + name + "\" is not a valid identifier.");
      }
    }
  }
}

// -------------------------------------------------------------------

// A common pattern:  We want to convert a repeated field in the descriptor
// to an array of values, calling some method to build each value.
#define BUILD_ARRAY(INPUT, OUTPUT, NAME, METHOD, PARENT)             \
  OUTPUT->NAME##_count_ = INPUT.NAME##_size();                       \
  AllocateArray(INPUT.NAME##_size(), &OUTPUT->NAME##s_);             \
  for (int i = 0; i < INPUT.NAME##_size(); i++) {                    \
    METHOD(INPUT.NAME(i), PARENT, OUTPUT->NAME##s_ + i);             \
  }

const FileDescriptor* DescriptorBuilder::BuildFile(
    const FileDescriptorProto& proto) {
  filename_ = proto.name();

  // Check to see if this file is already on the pending files list.
  // TODO(kenton):  Allow recursive imports?  It may not work with some
  //   (most?) programming languages.  E.g., in C++, a forward declaration
  //   of a type is not sufficient to allow it to be used even in a
  //   generated header file due to inlining.  This could perhaps be
  //   worked around using tricks involving inserting #include statements
  //   mid-file, but that's pretty ugly, and I'm pretty sure there are
  //   some languages out there that do not allow recursive dependencies
  //   at all.
  for (int i = 0; i < tables_->pending_files_.size(); i++) {
    if (tables_->pending_files_[i] == proto.name()) {
      string error_message("File recursively imports itself: ");
      for (; i < tables_->pending_files_.size(); i++) {
        error_message.append(tables_->pending_files_[i]);
        error_message.append(" -> ");
      }
      error_message.append(proto.name());

      AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
               error_message);
      return NULL;
    }
  }

  // If we have a fallback_database_, attempt to load all dependencies now,
  // before checkpointing tables_.  This avoids confusion with recursive
  // checkpoints.
  if (pool_->fallback_database_ != NULL) {
    tables_->pending_files_.push_back(proto.name());
    for (int i = 0; i < proto.dependency_size(); i++) {
      if (tables_->FindFile(proto.dependency(i)) == NULL &&
          (pool_->underlay_ == NULL ||
           pool_->underlay_->FindFileByName(proto.dependency(i)) == NULL)) {
        // We don't care what this returns since we'll find out below anyway.
        pool_->TryFindFileInFallbackDatabase(proto.dependency(i));
      }
    }
    tables_->pending_files_.pop_back();
  }

  // Checkpoint the tables so that we can roll back if something goes wrong.
  tables_->Checkpoint();

  FileDescriptor* result = tables_->Allocate<FileDescriptor>();
  file_ = result;

  if (!proto.has_name()) {
    AddError("", proto, DescriptorPool::ErrorCollector::OTHER,
             "Missing field: FileDescriptorProto.name.");
  }

  result->name_ = tables_->AllocateString(proto.name());
  if (proto.has_package()) {
    result->package_ = tables_->AllocateString(proto.package());
  } else {
    // We cannot rely on proto.package() returning a valid string if
    // proto.has_package() is false, because we might be running at static
    // initialization time, in which case default values have not yet been
    // initialized.
    result->package_ = tables_->AllocateString("");
  }
  result->pool_ = pool_;

  // Add to tables.
  if (!tables_->AddFile(result)) {
    AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
             "A file with this name is already in the pool.");
    // Bail out early so that if this is actually the exact same file, we
    // don't end up reporting that every single symbol is already defined.
    tables_->Rollback();
    return NULL;
  }
  if (!result->package().empty()) {
    AddPackage(result->package(), proto, result);
  }

  // Make sure all dependencies are loaded.
  set<string> seen_dependencies;
  result->dependency_count_ = proto.dependency_size();
  result->dependencies_ =
    tables_->AllocateArray<const FileDescriptor*>(proto.dependency_size());
  for (int i = 0; i < proto.dependency_size(); i++) {
    if (!seen_dependencies.insert(proto.dependency(i)).second) {
      AddError(proto.name(), proto,
               DescriptorPool::ErrorCollector::OTHER,
               "Import \"" + proto.dependency(i) + "\" was listed twice.");
    }

    const FileDescriptor* dependency = tables_->FindFile(proto.dependency(i));
    if (dependency == NULL && pool_->underlay_ != NULL) {
      dependency = pool_->underlay_->FindFileByName(proto.dependency(i));
    }

    if (dependency == NULL) {
      string message;
      if (pool_->fallback_database_ == NULL) {
        message = "Import \"" + proto.dependency(i) +
                  "\" has not been loaded.";
      } else {
        message = "Import \"" + proto.dependency(i) +
                  "\" was not found or had errors.";
      }
      AddError(proto.name(), proto,
               DescriptorPool::ErrorCollector::OTHER,
               message);
    }

    result->dependencies_[i] = dependency;
  }

  // Convert children.
  BUILD_ARRAY(proto, result, message_type, BuildMessage  , NULL);
  BUILD_ARRAY(proto, result, enum_type   , BuildEnum     , NULL);
  BUILD_ARRAY(proto, result, service     , BuildService  , NULL);
  BUILD_ARRAY(proto, result, extension   , BuildExtension, NULL);

  // Copy options.
  if (!proto.has_options()) {
    result->options_ = &FileOptions::default_instance();
  } else {
    FileOptions* options = tables_->AllocateMessage<FileOptions>();
    options->CopyFrom(proto.options());
    result->options_ = options;
  }

  // Cross-link.
  CrossLinkFile(result, proto);

  if (had_errors_) {
    tables_->Rollback();
    return NULL;
  } else {
    tables_->Checkpoint();
    return result;
  }
}

void DescriptorBuilder::BuildMessage(const DescriptorProto& proto,
                                     const Descriptor* parent,
                                     Descriptor* result) {
  const string& scope = (parent == NULL) ?
    file_->package() : parent->full_name();
  string* full_name = tables_->AllocateString(scope);
  if (!full_name->empty()) full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_            = tables_->AllocateString(proto.name());
  result->full_name_       = full_name;
  result->file_            = file_;
  result->containing_type_ = parent;

  BUILD_ARRAY(proto, result, field          , BuildField         , result);
  BUILD_ARRAY(proto, result, nested_type    , BuildMessage       , result);
  BUILD_ARRAY(proto, result, enum_type      , BuildEnum          , result);
  BUILD_ARRAY(proto, result, extension_range, BuildExtensionRange, result);
  BUILD_ARRAY(proto, result, extension      , BuildExtension     , result);

  // Copy options.
  if (!proto.has_options()) {
    result->options_ = &MessageOptions::default_instance();
  } else {
    MessageOptions* options = tables_->AllocateMessage<MessageOptions>();
    options->CopyFrom(proto.options());
    result->options_ = options;
  }

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));

  // Check that no fields have numbers in extension ranges.
  for (int i = 0; i < result->field_count(); i++) {
    const FieldDescriptor* field = result->field(i);
    for (int j = 0; j < result->extension_range_count(); j++) {
      const Descriptor::ExtensionRange* range = result->extension_range(j);
      if (range->start <= field->number() && field->number() < range->end) {
        AddError(field->full_name(), proto.extension_range(j),
                 DescriptorPool::ErrorCollector::NUMBER,
                 strings::Substitute(
                   "Extension range $0 to $1 includes field \"$2\" ($3).",
                   range->start, range->end - 1,
                   field->name(), field->number()));
      }
    }
  }

  // Check that extension ranges don't overlap.
  for (int i = 0; i < result->extension_range_count(); i++) {
    const Descriptor::ExtensionRange* range1 = result->extension_range(i);
    for (int j = i + 1; j < result->extension_range_count(); j++) {
      const Descriptor::ExtensionRange* range2 = result->extension_range(j);
      if (range1->end > range2->start && range2->end > range1->start) {
        AddError(result->full_name(), proto.extension_range(j),
                 DescriptorPool::ErrorCollector::NUMBER,
                 strings::Substitute("Extension range $0 to $1 overlaps with "
                                     "already-defined range $2 to $3.",
                                     range2->start, range2->end - 1,
                                     range1->start, range1->end - 1));
      }
    }
  }
}

void DescriptorBuilder::BuildFieldOrExtension(const FieldDescriptorProto& proto,
                                              const Descriptor* parent,
                                              FieldDescriptor* result,
                                              bool is_extension) {
  const string& scope = (parent == NULL) ?
    file_->package() : parent->full_name();
  string* full_name = tables_->AllocateString(scope);
  if (!full_name->empty()) full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_         = tables_->AllocateString(proto.name());
  result->full_name_    = full_name;
  result->file_         = file_;
  result->number_       = proto.number();
  result->type_         = static_cast<FieldDescriptor::Type>(proto.type());
  result->label_        = static_cast<FieldDescriptor::Label>(proto.label());
  result->is_extension_ = is_extension;

  // Some of these may be filled in when cross-linking.
  result->containing_type_ = NULL;
  result->extension_scope_ = NULL;
  result->experimental_map_key_ = NULL;
  result->message_type_ = NULL;
  result->enum_type_ = NULL;

  result->has_default_value_ = proto.has_default_value();
  if (proto.has_type()) {
    if (proto.has_default_value()) {
      char* end_pos = NULL;
      switch (result->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
          result->default_value_int32_ =
            strtol(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_INT64:
          result->default_value_int64_ =
            strto64(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_UINT32:
          result->default_value_uint32_ =
            strtoul(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_UINT64:
          result->default_value_uint64_ =
            strtou64(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_FLOAT:
          result->default_value_float_ =
            NoLocaleStrtod(proto.default_value().c_str(), &end_pos);
          break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
          result->default_value_double_ =
            NoLocaleStrtod(proto.default_value().c_str(), &end_pos);
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          if (proto.default_value() == "true") {
            result->default_value_bool_ = true;
          } else if (proto.default_value() == "false") {
            result->default_value_bool_ = false;
          } else {
            AddError(result->full_name(), proto,
                     DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                     "Boolean default must be true or false.");
          }
          break;
        case FieldDescriptor::CPPTYPE_ENUM:
          // This will be filled in when cross-linking.
          result->default_value_enum_ = NULL;
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          if (result->type() == FieldDescriptor::TYPE_BYTES) {
            result->default_value_string_ = tables_->AllocateString(
              UnescapeCEscapeString(proto.default_value()));
          } else {
            result->default_value_string_ =
              tables_->AllocateString(proto.default_value());
          }
          break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
          AddError(result->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                   "Messages can't have default values.");
          result->has_default_value_ = false;
          break;
      }

      if (end_pos != NULL) {
        // end_pos is only set non-NULL by the parsers for numeric types, above.
        // This checks that the default was non-empty and had no extra junk
        // after the end of the number.
        if (proto.default_value().empty() || *end_pos != '\0') {
          AddError(result->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                   "Couldn't parse default value.");
        }
      }
    } else {
      // No explicit default value
      switch (result->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
          result->default_value_int32_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_INT64:
          result->default_value_int64_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_UINT32:
          result->default_value_uint32_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_UINT64:
          result->default_value_uint64_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_FLOAT:
          result->default_value_float_ = 0.0f;
          break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
          result->default_value_double_ = 0.0;
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          result->default_value_bool_ = false;
          break;
        case FieldDescriptor::CPPTYPE_ENUM:
          // This will be filled in when cross-linking.
          result->default_value_enum_ = NULL;
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          result->default_value_string_ = &kEmptyString;
          break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
          break;
      }
    }
  }

  if (result->number() <= 0) {
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             "Field numbers must be positive integers.");
  } else if (result->number() > FieldDescriptor::kMaxNumber) {
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             strings::Substitute("Field numbers cannot be greater than $0.",
                                 FieldDescriptor::kMaxNumber));
  } else if (result->number() >= FieldDescriptor::kFirstReservedNumber &&
             result->number() <= FieldDescriptor::kLastReservedNumber) {
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             strings::Substitute(
               "Field numbers $0 through $1 are reserved for the protocol "
               "buffer library implementation.",
               FieldDescriptor::kFirstReservedNumber,
               FieldDescriptor::kLastReservedNumber));
  }

  if (is_extension) {
    if (!proto.has_extendee()) {
      AddError(result->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE,
               "FieldDescriptorProto.extendee not set for extension field.");
    }

    result->extension_scope_ = parent;
  } else {
    if (proto.has_extendee()) {
      AddError(result->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE,
               "FieldDescriptorProto.extendee set for non-extension field.");
    }

    result->containing_type_ = parent;
  }

  // Copy options.
  if (!proto.has_options()) {
    result->options_ = &FieldOptions::default_instance();
  } else {
    FieldOptions* options = tables_->AllocateMessage<FieldOptions>();
    options->CopyFrom(proto.options());
    result->options_ = options;
  }

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));
}

void DescriptorBuilder::BuildExtensionRange(
    const DescriptorProto::ExtensionRange& proto,
    const Descriptor* parent,
    Descriptor::ExtensionRange* result) {
  result->start = proto.start();
  result->end = proto.end();
  if (result->start <= 0) {
    AddError(parent->full_name(), proto,
             DescriptorPool::ErrorCollector::NUMBER,
             "Extension numbers must be positive integers.");
  }

  if (result->end > FieldDescriptor::kMaxNumber + 1) {
    AddError(parent->full_name(), proto,
             DescriptorPool::ErrorCollector::NUMBER,
             strings::Substitute("Extension numbers cannot be greater than $0.",
                                 FieldDescriptor::kMaxNumber));
  }

  if (result->start >= result->end) {
    AddError(parent->full_name(), proto,
             DescriptorPool::ErrorCollector::NUMBER,
             "Extension range end number must be greater than start number.");
  }
}

void DescriptorBuilder::BuildEnum(const EnumDescriptorProto& proto,
                                  const Descriptor* parent,
                                  EnumDescriptor* result) {
  const string& scope = (parent == NULL) ?
    file_->package() : parent->full_name();
  string* full_name = tables_->AllocateString(scope);
  if (!full_name->empty()) full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_            = tables_->AllocateString(proto.name());
  result->full_name_       = full_name;
  result->file_            = file_;
  result->containing_type_ = parent;

  if (proto.value_size() == 0) {
    // We cannot allow enums with no values because this would mean there
    // would be no valid default value for fields of this type.
    AddError(result->full_name(), proto,
             DescriptorPool::ErrorCollector::NAME,
             "Enums must contain at least one value.");
  }

  BUILD_ARRAY(proto, result, value, BuildEnumValue, result);

  // Copy options.
  if (!proto.has_options()) {
    result->options_ = &EnumOptions::default_instance();
  } else {
    EnumOptions* options = tables_->AllocateMessage<EnumOptions>();
    options->CopyFrom(proto.options());
    result->options_ = options;
  }

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));
}

void DescriptorBuilder::BuildEnumValue(const EnumValueDescriptorProto& proto,
                                       const EnumDescriptor* parent,
                                       EnumValueDescriptor* result) {
  result->name_   = tables_->AllocateString(proto.name());
  result->number_ = proto.number();
  result->type_   = parent;

  // Note:  full_name for enum values is a sibling to the parent's name, not a
  //   child of it.
  string* full_name = tables_->AllocateString(*parent->full_name_);
  full_name->resize(full_name->size() - parent->name_->size());
  full_name->append(*result->name_);
  result->full_name_ = full_name;

  ValidateSymbolName(proto.name(), *full_name, proto);

  // Copy options.
  if (!proto.has_options()) {
    result->options_ = &EnumValueOptions::default_instance();
  } else {
    EnumValueOptions* options = tables_->AllocateMessage<EnumValueOptions>();
    options->CopyFrom(proto.options());
    result->options_ = options;
  }

  // Again, enum values are weird because we makes them appear as siblings
  // of the enum type instead of children of it.  So, we use
  // parent->containing_type() as the value's parent.
  bool added_to_outer_scope =
    AddSymbol(result->full_name(), parent->containing_type(), result->name(),
              proto, Symbol(result));

  // However, we also want to be able to search for values within a single
  // enum type, so we add it as a child of the enum type itself, too.
  // Note:  This could fail, but if it does, the error has already been
  //   reported by the above AddSymbol() call, so we ignore the return code.
  bool added_to_inner_scope =
    tables_->AddAliasUnderParent(parent, result->name(), Symbol(result));

  if (added_to_inner_scope && !added_to_outer_scope) {
    // This value did not conflict with any values defined in the same enum,
    // but it did conflict with some other symbol defined in the enum type's
    // scope.  Let's print an additional error to explain this.
    string outer_scope;
    if (parent->containing_type() == NULL) {
      outer_scope = file_->package();
    } else {
      outer_scope = parent->containing_type()->full_name();
    }

    if (outer_scope.empty()) {
      outer_scope = "the global scope";
    } else {
      outer_scope = "\"" + outer_scope + "\"";
    }

    AddError(result->full_name(), proto,
             DescriptorPool::ErrorCollector::NAME,
             "Note that enum values use C++ scoping rules, meaning that "
             "enum values are siblings of their type, not children of it.  "
             "Therefore, \"" + result->name() + "\" must be unique within "
             + outer_scope + ", not just within \"" + parent->name() + "\".");
  }

  // An enum is allowed to define two numbers that refer to the same value.
  // FindValueByNumber() should return the first such value, so we simply
  // ignore AddEnumValueByNumber()'s return code.
  tables_->AddEnumValueByNumber(result);
}

void DescriptorBuilder::BuildService(const ServiceDescriptorProto& proto,
                                     const void* dummy,
                                     ServiceDescriptor* result) {
  string* full_name = tables_->AllocateString(file_->package());
  if (!full_name->empty()) full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_      = tables_->AllocateString(proto.name());
  result->full_name_ = full_name;
  result->file_      = file_;

  BUILD_ARRAY(proto, result, method, BuildMethod, result);

  // Copy options.
  if (!proto.has_options()) {
    result->options_ = &ServiceOptions::default_instance();
  } else {
    ServiceOptions* options = tables_->AllocateMessage<ServiceOptions>();
    options->CopyFrom(proto.options());
    result->options_ = options;
  }

  AddSymbol(result->full_name(), NULL, result->name(),
            proto, Symbol(result));
}

void DescriptorBuilder::BuildMethod(const MethodDescriptorProto& proto,
                                    const ServiceDescriptor* parent,
                                    MethodDescriptor* result) {
  result->name_    = tables_->AllocateString(proto.name());
  result->service_ = parent;

  string* full_name = tables_->AllocateString(parent->full_name());
  full_name->append(1, '.');
  full_name->append(*result->name_);
  result->full_name_ = full_name;

  ValidateSymbolName(proto.name(), *full_name, proto);

  // These will be filled in when cross-linking.
  result->input_type_ = NULL;
  result->output_type_ = NULL;

  // Copy options.
  if (!proto.has_options()) {
    result->options_ = &MethodOptions::default_instance();
  } else {
    MethodOptions* options = tables_->AllocateMessage<MethodOptions>();
    options->CopyFrom(proto.options());
    result->options_ = options;
  }

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));
}

#undef BUILD_ARRAY

// -------------------------------------------------------------------

void DescriptorBuilder::CrossLinkFile(
    FileDescriptor* file, const FileDescriptorProto& proto) {
  for (int i = 0; i < file->message_type_count(); i++) {
    CrossLinkMessage(&file->message_types_[i], proto.message_type(i));
  }

  for (int i = 0; i < file->extension_count(); i++) {
    CrossLinkField(&file->extensions_[i], proto.extension(i));
  }

  for (int i = 0; i < file->service_count(); i++) {
    CrossLinkService(&file->services_[i], proto.service(i));
  }
}

void DescriptorBuilder::CrossLinkMessage(
    Descriptor* message, const DescriptorProto& proto) {
  for (int i = 0; i < message->nested_type_count(); i++) {
    CrossLinkMessage(&message->nested_types_[i], proto.nested_type(i));
  }

  for (int i = 0; i < message->field_count(); i++) {
    CrossLinkField(&message->fields_[i], proto.field(i));
  }

  for (int i = 0; i < message->extension_count(); i++) {
    CrossLinkField(&message->extensions_[i], proto.extension(i));
  }
}

void DescriptorBuilder::CrossLinkField(
    FieldDescriptor* field, const FieldDescriptorProto& proto) {
  if (proto.has_extendee()) {
    Symbol extendee = LookupSymbol(proto.extendee(), field->full_name());
    if (extendee.IsNull()) {
      AddNotDefinedError(field->full_name(), proto,
                         DescriptorPool::ErrorCollector::EXTENDEE,
                         proto.extendee());
      return;
    } else if (extendee.type != Symbol::MESSAGE) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE,
               "\"" + proto.extendee() + "\" is not a message type.");
      return;
    }
    field->containing_type_ = extendee.descriptor;

    if (!field->containing_type()->IsExtensionNumber(field->number())) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER,
               strings::Substitute("\"$0\" does not declare $1 as an "
                                   "extension number.",
                                   field->containing_type()->full_name(),
                                   field->number()));
    }
  }

  if (proto.has_type_name()) {
    Symbol type = LookupSymbol(proto.type_name(), field->full_name());
    if (type.IsNull()) {
      AddNotDefinedError(field->full_name(), proto,
                         DescriptorPool::ErrorCollector::TYPE,
                         proto.type_name());
      return;
    }

    if (!proto.has_type()) {
      // Choose field type based on symbol.
      if (type.type == Symbol::MESSAGE) {
        field->type_ = FieldDescriptor::TYPE_MESSAGE;
      } else if (type.type == Symbol::ENUM) {
        field->type_ = FieldDescriptor::TYPE_ENUM;
      } else {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "\"" + proto.type_name() + "\" is not a type.");
        return;
      }
    }

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (type.type != Symbol::MESSAGE) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "\"" + proto.type_name() + "\" is not a message type.");
        return;
      }
      field->message_type_ = type.descriptor;

      if (field->has_default_value()) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                 "Messages can't have default values.");
      }
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      if (type.type != Symbol::ENUM) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "\"" + proto.type_name() + "\" is not an enum type.");
        return;
      }
      field->enum_type_ = type.enum_descriptor;

      if (field->has_default_value()) {
        // We can't just use field->enum_type()->FindValueByName() here
        // because that locks the pool's mutex, which we have already locked
        // at this point.
        Symbol default_value =
          LookupSymbol(proto.default_value(), field->enum_type()->full_name());

        if (default_value.type == Symbol::ENUM_VALUE &&
            default_value.enum_value_descriptor->type() == field->enum_type()) {
          field->default_value_enum_ = default_value.enum_value_descriptor;
        } else {
          AddError(field->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                   "Enum type \"" + field->enum_type()->full_name() +
                   "\" has no value named \"" + proto.default_value() + "\".");
        }
      } else if (field->enum_type()->value_count() > 0) {
        // All enums must have at least one value, or we would have reported
        // an error elsewhere.  We use the first defined value as the default
        // if a default is not explicitly defined.
        field->default_value_enum_ = field->enum_type()->value(0);
      }
    } else {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "Field with primitive type has type_name.");
    }
  } else {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
        field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "Field with message or enum type missing type_name.");
    }
  }

  if (proto.has_options() && proto.options().has_experimental_map_key()) {
    CrossLinkMapKey(field, proto);
  }

  // Add the field to the fields-by-number table.
  // Note:  We have to do this *after* cross-linking because extensions do not
  //   know their containing type until now.
  if (!tables_->AddFieldByNumber(field)) {
    const FieldDescriptor* conflicting_field =
      tables_->FindFieldByNumber(field->containing_type(), field->number());
    if (field->is_extension()) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER,
               strings::Substitute("Extension number $0 has already been used "
                                   "in \"$1\" by extension \"$2\".",
                                   field->number(),
                                   field->containing_type()->full_name(),
                                   conflicting_field->full_name()));
    } else {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER,
               strings::Substitute("Field number $0 has already been used in "
                                   "\"$1\" by field \"$2\".",
                                   field->number(),
                                   field->containing_type()->full_name(),
                                   conflicting_field->name()));
    }
  }

  // Note:  Default instance may not yet be initialized here, so we have to
  //   avoid reading from it.
  if (field->containing_type_ != NULL &&
      &field->containing_type()->options() !=
        &MessageOptions::default_instance() &&
      field->containing_type()->options().message_set_wire_format()) {
    if (field->is_extension()) {
      if (!field->is_optional() ||
          field->type() != FieldDescriptor::TYPE_MESSAGE) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "Extensions of MessageSets must be optional messages.");
      }
    } else {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NAME,
               "MessageSets cannot have fields, only extensions.");
    }
  }
}

void DescriptorBuilder::CrossLinkService(
    ServiceDescriptor* service, const ServiceDescriptorProto& proto) {
  for (int i = 0; i < service->method_count(); i++) {
    CrossLinkMethod(&service->methods_[i], proto.method(i));
  }
}

void DescriptorBuilder::CrossLinkMethod(
    MethodDescriptor* method, const MethodDescriptorProto& proto) {
  Symbol input_type = LookupSymbol(proto.input_type(), method->full_name());
  if (input_type.IsNull()) {
    AddNotDefinedError(method->full_name(), proto,
                       DescriptorPool::ErrorCollector::INPUT_TYPE,
                       proto.input_type());
  } else if (input_type.type != Symbol::MESSAGE) {
    AddError(method->full_name(), proto,
             DescriptorPool::ErrorCollector::INPUT_TYPE,
             "\"" + proto.input_type() + "\" is not a message type.");
  } else {
    method->input_type_ = input_type.descriptor;
  }

  Symbol output_type = LookupSymbol(proto.output_type(), method->full_name());
  if (output_type.IsNull()) {
    AddNotDefinedError(method->full_name(), proto,
                       DescriptorPool::ErrorCollector::OUTPUT_TYPE,
                       proto.output_type());
  } else if (output_type.type != Symbol::MESSAGE) {
    AddError(method->full_name(), proto,
             DescriptorPool::ErrorCollector::OUTPUT_TYPE,
             "\"" + proto.output_type() + "\" is not a message type.");
  } else {
    method->output_type_ = output_type.descriptor;
  }
}

void DescriptorBuilder::CrossLinkMapKey(
    FieldDescriptor* field,
    const FieldDescriptorProto& proto) {
  if (!field->is_repeated()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "map type is only allowed for repeated fields.");
    return;
  }

  if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "map type is only allowed for fields with a message type.");
    return;
  }

  const Descriptor* item_type = field->message_type();
  if (item_type == NULL) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "Could not find field type.");
    return;
  }

  // Find the field in item_type named by "experimental_map_key"
  const string& key_name = proto.options().experimental_map_key();
  const Symbol key_symbol = LookupSymbol(
      key_name,
      // We append ".key_name" to the containing type's name since
      // LookupSymbol() searches for peers of the supplied name, not
      // children of the supplied name.
      item_type->full_name() + "." + key_name);

  if (key_symbol.IsNull() || key_symbol.field_descriptor->is_extension()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "Could not find field named \"" + key_name + "\" in type \"" +
             item_type->full_name() + "\".");
    return;
  }
  const FieldDescriptor* key_field = key_symbol.field_descriptor;

  if (key_field->is_repeated()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "map_key must not name a repeated field.");
    return;
  }

  if (key_field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "map key must name a scalar or string field.");
    return;
  }

  field->experimental_map_key_ = key_field;
}

}  // namespace protobuf
}  // namespace google
