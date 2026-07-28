// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_BUILDER_H__
#define GOOGLE_PROTOBUF_DESCRIPTOR_BUILDER_H__

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/functional/function_ref.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/feature_resolver.h"
#include "google/protobuf/internal_feature_helper.h"
#include "google/protobuf/message.h"
#include "google/protobuf/symbol.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

class OptionInterpreter;
class AggregateOptionFinder;

// A path through a FileDescriptorProto to a specific location of source code,
// e.g. a field name. See SourceCodeInfo.Location.path in descriptor.proto for
// full structure of this vector.
using SourceCodePath = std::vector<int>;

// Represents an options message to interpret. Extension names in the option
// name are resolved relative to name_scope. element_name and orig_opt are
// used only for error reporting (since the parser records locations against
// pointers in the original options, not the mutable copy). The Message must be
// one of the Options messages in descriptor.proto.
struct OptionsToInterpret {
  OptionsToInterpret(absl::string_view ns, absl::string_view el,
                     SourceCodePath path, const Message* orig_opt, Message* opt)
      : name_scope(ns),
        element_name(el),
        element_path(path.begin(), path.end()),
        original_options(orig_opt),
        options(opt) {}
  std::string name_scope;
  std::string element_name;
  SourceCodePath element_path;
  const Message* original_options;
  Message* options;
};

// DescriptorBuilder is an internal helper class used by DescriptorPool to
// construct a FileDescriptor from a FileDescriptorProto representation.
//
// The build process is multi-phase and includes:
// 1. Building: Constructing descriptor objects (Descriptor, FieldDescriptor,
//    etc.) and adding them to the pool's symbol table.
// 2. Cross-linking: Resolving references between descriptors (e.g. field types
//    referenced by name).
// 3. Option interpretation: Resolving and parsing custom options.
// 4. Validation: Verifying that the resulting descriptors satisfy all rules and
//    constraints.
//
// This class is short-lived and instantiated for a single BuildFile operation.
class DescriptorBuilder {
 public:
  static std::unique_ptr<DescriptorBuilder> New(
      const DescriptorPool* pool, DescriptorPool::Tables* tables,
      DescriptorPool::DeferredValidation& deferred_validation,
      DescriptorPool::ErrorCollector* error_collector) {
    return std::unique_ptr<DescriptorBuilder>(new DescriptorBuilder(
        pool, tables, deferred_validation, error_collector));
  }

  ~DescriptorBuilder();

  const FileDescriptor* BuildFile(const FileDescriptorProto& proto);

 private:
  static constexpr size_t kMaxNumErrors = 1000;

  DescriptorBuilder(const DescriptorPool* pool, DescriptorPool::Tables* tables,
                    DescriptorPool::DeferredValidation& deferred_validation,
                    DescriptorPool::ErrorCollector* error_collector);

  friend class OptionInterpreter;

  // Non-recursive part of BuildFile functionality.
  FileDescriptor* BuildFileImpl(const FileDescriptorProto& proto,
                                internal::FlatAllocator& alloc);

  bool has_errors() const { return error_count_ != 0; }

  const DescriptorPool* pool_;
  DescriptorPool::Tables* tables_;  // for convenience
  DescriptorPool::DeferredValidation& deferred_validation_;
  DescriptorPool::ErrorCollector* error_collector_;

  absl::optional<FeatureResolver> feature_resolver_ = absl::nullopt;

  // As we build descriptors we store copies of the options messages in
  // them. We put pointers to those copies in this vector, as we build, so we
  // can later (after cross-linking) interpret those options.
  std::vector<OptionsToInterpret> options_to_interpret_;

  size_t error_count_ = 0;
  size_t warning_count_ = 0;
  std::string filename_;
  FileDescriptor* file_;
  FileDescriptorTables* file_tables_ = nullptr;
  absl::flat_hash_set<const FileDescriptor*> dependencies_;
  absl::flat_hash_set<const FileDescriptor*> option_dependencies_;

  struct MessageHints {
    int fields_to_suggest = 0;
    const Message* first_reason = nullptr;
    DescriptorPool::ErrorCollector::ErrorLocation first_reason_location =
        DescriptorPool::ErrorCollector::ErrorLocation::OTHER;

    void RequestHintOnFieldNumbers(
        const Message& reason,
        DescriptorPool::ErrorCollector::ErrorLocation reason_location,
        int range_start = 0, int range_end = 1) {
      auto fit = [](int value) {
        return std::min(std::max(value, 0), FieldDescriptor::kMaxNumber);
      };
      fields_to_suggest =
          fit(fields_to_suggest + fit(fit(range_end) - fit(range_start)));
      if (first_reason) return;
      first_reason = &reason;
      first_reason_location = reason_location;
    }
  };

  absl::flat_hash_map<const Descriptor*, MessageHints> message_hints_;

  // unused_dependency_ is used to record the unused imported files.
  // Note: public import is not considered.
  absl::flat_hash_set<const FileDescriptor*> unused_dependency_;

  // If LookupSymbol() finds a symbol that is in a file which is not a declared
  // dependency of this file, it will fail, but will set
  // possible_undeclared_dependency_ to point at that file.  This is only used
  // by AddNotDefinedError() to report a more useful error message.
  // possible_undeclared_dependency_name_ is the name of the symbol that was
  // actually found in possible_undeclared_dependency_, which may be a parent
  // of the symbol actually looked for.
  const FileDescriptor* possible_undeclared_dependency_;
  std::string possible_undeclared_dependency_name_;

  // If LookupSymbol() could resolve a symbol which is not defined,
  // record the resolved name.  This is only used by AddNotDefinedError()
  // to report a more useful error message.
  std::string undefine_resolved_name_;

  // Tracker for current recursion depth to implement recursion protection.
  //
  // Counts down to 0 when there is no depth remaining.
  //
  // Maximum recursion depth corresponds to 32 nested message declarations.
  int recursion_depth_ = internal::cpp::MaxMessageDeclarationNestingDepth();

  // Note: Both AddError and AddWarning functions are extremely sensitive to
  // the *caller* stack space used. We call these functions many times in
  // complex code paths that are hot and likely to be inlined heavily. However,
  // these calls themselves are cold error paths. But stack space used by the
  // code that sets up the call in many cases is paid for even when the call
  // isn't reached. To optimize this, we use `absl::string_view` to reuse
  // string objects where possible for the inputs and for the error message
  // itself we use a closure to build the error message inside these routines.
  // The routines themselves are marked to prevent inlining and this lets us
  // move the large code sometimes required to produce a useful error message
  // entirely into a helper closure rather than the immediate caller.
  //
  // The `const char*` overload should only be used for string literal messages
  // where this is a frustrating amount of overhead and there is no harm in
  // directly using the literal.
  void AddError(absl::string_view element_name, const Message& descriptor,
                DescriptorPool::ErrorCollector::ErrorLocation location,
                absl::FunctionRef<std::string()> make_error);
  void AddError(absl::string_view element_name, const Message& descriptor,
                DescriptorPool::ErrorCollector::ErrorLocation location,
                const char* error);
  void AddRecursiveImportError(const FileDescriptorProto& proto, int from_here);
  void AddTwiceListedError(const FileDescriptorProto& proto,
                           absl::string_view import_name);
  void AddImportError(const FileDescriptorProto& proto,
                      absl::string_view import_name);

  // Adds an error indicating that undefined_symbol was not defined.  Must
  // only be called after LookupSymbol() fails.
  void AddNotDefinedError(
      absl::string_view element_name, const Message& descriptor,
      DescriptorPool::ErrorCollector::ErrorLocation location,
      absl::string_view undefined_symbol);

  void AddWarning(absl::string_view element_name, const Message& descriptor,
                  DescriptorPool::ErrorCollector::ErrorLocation location,
                  absl::FunctionRef<std::string()> make_error);
  void AddWarning(absl::string_view element_name, const Message& descriptor,
                  DescriptorPool::ErrorCollector::ErrorLocation location,
                  const char* error);

  // Silly helper which determines if the given file is in the given package.
  // I.e., either file->package() == package_name or file->package() is a
  // nested package within package_name.
  bool IsInPackage(const FileDescriptor* file, absl::string_view package_name);

  // Helper function which finds all public dependencies of the given file, and
  // stores them in the dependencies_ set in the builder.
  void RecordPublicDependencies(const FileDescriptor* file);

  // Helper function which finds all public option dependencies of the given
  // file, and stores them in the option_dependencies_ set in the builder.
  void RecordPublicOptionDependencies(const FileDescriptor* file);

  // Like tables_->FindSymbol(), but additionally:
  // - Search the pool's underlay if not found in tables_.
  // - Insure that the resulting Symbol is from one of the file's declared
  //   dependencies.
  Symbol FindSymbol(absl::string_view name, bool build_it = true);

  // Like FindSymbol() but does not require that the symbol is in one of the
  // file's declared dependencies.
  Symbol FindSymbolNotEnforcingDeps(absl::string_view name,
                                    bool build_it = true);

  // This implements the body of FindSymbolNotEnforcingDeps().
  Symbol FindSymbolNotEnforcingDepsHelper(const DescriptorPool* pool,
                                          absl::string_view name,
                                          bool build_it = true);

  // Like FindSymbol(), but looks up the name relative to some other symbol
  // name.  This first searches siblings of relative_to, then siblings of its
  // parents, etc.  For example, LookupSymbol("foo.bar", "baz.moo.corge") makes
  // the following calls, returning the first non-null result:
  // FindSymbol("baz.moo.foo.bar"), FindSymbol("baz.foo.bar"),
  // FindSymbol("foo.bar").  If AllowUnknownDependencies() has been called
  // on the DescriptorPool, this will generate a placeholder type if
  // the name is not found (unless the name itself is malformed).  The
  // placeholder_type parameter indicates what kind of placeholder should be
  // constructed in this case.  The resolve_mode parameter determines whether
  // any symbol is returned, or only symbols that are types.  Note, however,
  // that LookupSymbol may still return a non-type symbol in LOOKUP_TYPES mode,
  // if it believes that's all it could refer to.  The caller should always
  // check that it receives the type of symbol it was expecting.
  enum ResolveMode { LOOKUP_ALL, LOOKUP_TYPES };
  Symbol LookupSymbol(absl::string_view name, absl::string_view relative_to,
                      DescriptorPool::PlaceholderType placeholder_type =
                          DescriptorPool::PLACEHOLDER_MESSAGE,
                      ResolveMode resolve_mode = LOOKUP_ALL,
                      bool build_it = true);

  // Like LookupSymbol() but will not return a placeholder even if
  // AllowUnknownDependencies() has been used.
  Symbol LookupSymbolNoPlaceholder(absl::string_view name,
                                   absl::string_view relative_to,
                                   ResolveMode resolve_mode = LOOKUP_ALL,
                                   bool build_it = true);

  // Calls tables_->AddSymbol() and records an error if it fails.  Returns
  // true if successful or false if failed, though most callers can ignore
  // the return value since an error has already been recorded.
  bool AddSymbol(absl::string_view full_name, const void* parent,
                 absl::string_view name, const Message& proto, Symbol symbol);

  // Like AddSymbol(), but succeeds if the symbol is already defined as long
  // as the existing definition is also a package (because it's OK to define
  // the same package in two different files).  Also adds all parents of the
  // package to the symbol table (e.g. AddPackage("foo.bar", ...) will add
  // "foo.bar" and "foo" to the table).
  void AddPackage(absl::string_view name, const Message& proto,
                  FileDescriptor* file, bool toplevel);

  // Checks that the symbol name contains only alphanumeric characters and
  // underscores.  Records an error otherwise.
  void ValidateSymbolName(absl::string_view name, absl::string_view full_name,
                          const Message& proto);

  // Allocates a copy of orig_options in tables_ and stores it in the
  // descriptor. Remembers its uninterpreted options, to be interpreted
  // later. DescriptorT must be one of the Descriptor messages from
  // descriptor.proto.
  template <class DescriptorT>
  void AllocateOptions(const typename DescriptorT::Proto& proto,
                       DescriptorT* descriptor, int options_field_tag,
                       absl::string_view option_name,
                       internal::FlatAllocator& alloc);
  // Specialization for FileOptions.
  void AllocateOptions(const FileDescriptorProto& proto,
                       FileDescriptor* descriptor,
                       internal::FlatAllocator& alloc);

  // Implementation for AllocateOptions(). Don't call this directly.
  template <class DescriptorT>
  const typename DescriptorT::OptionsType* AllocateOptionsImpl(
      absl::string_view name_scope, absl::string_view element_name,
      const typename DescriptorT::Proto& proto, SourceCodePath options_path,
      absl::string_view option_name, internal::FlatAllocator& alloc);

  // Allocates and resolves any feature sets that need to be owned by a given
  // descriptor. This also strips features out of the mutable options message to
  // prevent leaking of unresolved features.
  // Note: This must be used during a pre-order traversal of the
  // descriptor tree, so that each descriptor's parent has a fully resolved
  // feature set already.
  template <class DescriptorT>
  void ResolveFeatures(const typename DescriptorT::Proto& proto,
                       DescriptorT* descriptor,
                       typename DescriptorT::OptionsType* options,
                       internal::FlatAllocator& alloc);
  void ResolveFeatures(const FileDescriptorProto& proto,
                       FileDescriptor* descriptor, FileOptions* options,
                       internal::FlatAllocator& alloc);
  template <class DescriptorT>
  void ResolveFeaturesImpl(
      Edition edition, const typename DescriptorT::Proto& proto,
      DescriptorT* descriptor, typename DescriptorT::OptionsType* options,
      internal::FlatAllocator& alloc,
      DescriptorPool::ErrorCollector::ErrorLocation error_location,
      bool force_merge = false);

  void PostProcessFieldFeatures(FieldDescriptor& field,
                                const FieldDescriptorProto& proto);

  // Allocates an array of two strings, the first one is a copy of
  // `proto_name`, and the second one is the full name. Full proto name is
  // "scope.proto_name" if scope is non-empty and "proto_name" otherwise.
  auto AllocateNameStrings(absl::string_view scope,
                           absl::string_view proto_name, const Message& entity,
                           internal::FlatAllocator& alloc);

  // These methods all have the same signature for the sake of the BUILD_ARRAY
  // macro, below.
  void BuildMessage(const DescriptorProto& proto, const Descriptor* parent,
                    Descriptor* result, internal::FlatAllocator& alloc);
  void BuildFieldOrExtension(const FieldDescriptorProto& proto,
                             Descriptor* parent, FieldDescriptor* result,
                             bool is_extension, internal::FlatAllocator& alloc);
  void BuildField(const FieldDescriptorProto& proto, Descriptor* parent,
                  FieldDescriptor* result, internal::FlatAllocator& alloc) {
    BuildFieldOrExtension(proto, parent, result, false, alloc);
  }
  void BuildExtension(const FieldDescriptorProto& proto, Descriptor* parent,
                      FieldDescriptor* result, internal::FlatAllocator& alloc) {
    BuildFieldOrExtension(proto, parent, result, true, alloc);
  }
  void BuildExtensionRange(const DescriptorProto::ExtensionRange& proto,
                           const Descriptor* parent,
                           Descriptor::ExtensionRange* result,
                           internal::FlatAllocator& alloc);
  void BuildReservedRange(const DescriptorProto::ReservedRange& proto,
                          const Descriptor* parent,
                          Descriptor::ReservedRange* result,
                          internal::FlatAllocator& alloc);
  void BuildReservedRange(const EnumDescriptorProto::EnumReservedRange& proto,
                          const EnumDescriptor* parent,
                          EnumDescriptor::ReservedRange* result,
                          internal::FlatAllocator& alloc);
  void BuildOneof(const OneofDescriptorProto& proto, Descriptor* parent,
                  OneofDescriptor* result, internal::FlatAllocator& alloc);
  void BuildEnum(const EnumDescriptorProto& proto, const Descriptor* parent,
                 EnumDescriptor* result, internal::FlatAllocator& alloc);
  void BuildEnumValue(const EnumValueDescriptorProto& proto,
                      const EnumDescriptor* parent, EnumValueDescriptor* result,
                      internal::FlatAllocator& alloc);
  void BuildService(const ServiceDescriptorProto& proto, const void* dummy,
                    ServiceDescriptor* result, internal::FlatAllocator& alloc);
  void BuildMethod(const MethodDescriptorProto& proto,
                   const ServiceDescriptor* parent, MethodDescriptor* result,
                   internal::FlatAllocator& alloc);

  void CheckFieldJsonNameUniqueness(const DescriptorProto& proto,
                                    const Descriptor* result);
  void CheckFieldJsonNameUniqueness(absl::string_view message_name,
                                    const DescriptorProto& message,
                                    const Descriptor* descriptor,
                                    bool use_custom_names);
  void CheckEnumValueUniqueness(const EnumDescriptorProto& proto,
                                const EnumDescriptor* result);

  void CheckEnumCustomStringUniqueness(const EnumDescriptorProto& proto,
                                       const EnumDescriptor* result);

  void LogUnusedDependency(const FileDescriptorProto& proto,
                           const FileDescriptor* result);

  // Must be run only after building.
  //
  // NOTE: Options will not be available during cross-linking, as they
  // have not yet been interpreted. Defer any handling of options to the
  // Validate*Options methods.
  void CrossLinkFile(FileDescriptor* file, const FileDescriptorProto& proto);
  void CrossLinkMessage(Descriptor* message, const DescriptorProto& proto);
  void CrossLinkField(FieldDescriptor* field,
                      const FieldDescriptorProto& proto);
  void CrossLinkService(ServiceDescriptor* service,
                        const ServiceDescriptorProto& proto);
  void CrossLinkMethod(MethodDescriptor* method,
                       const MethodDescriptorProto& proto);
  void SuggestFieldNumbers(FileDescriptor* file,
                           const FileDescriptorProto& proto);


  // Checks that the extension field matches what is declared.
  void CheckExtensionDeclaration(const FieldDescriptor& field,
                                 const FieldDescriptorProto& proto,
                                 absl::string_view declared_full_name,
                                 absl::string_view declared_type_name,
                                 bool is_repeated);
  // Checks that the extension field type matches the declared type. It also
  // handles message types that look like non-message types such as "fixed64" vs
  // ".fixed64".
  void CheckExtensionDeclarationFieldType(const FieldDescriptor& field,
                                          const FieldDescriptorProto& proto,
                                          absl::string_view type);

  // Work-around for broken compilers:  According to the C++ standard,
  // OptionInterpreter should have access to the private members of any class
  // which has declared DescriptorBuilder as a friend.  Unfortunately some old
  // versions of GCC and other compilers do not implement this correctly.  So,
  // we have to have these intermediate methods to provide access.  We also
  // redundantly declare OptionInterpreter a friend just to make things extra
  // clear for these bad compilers.
  friend class OptionInterpreter;
  friend class AggregateOptionFinder;

  static bool get_allow_unknown(const DescriptorPool* pool) {
    return pool->allow_unknown_;
  }
  static bool get_enforce_weak(const DescriptorPool* pool) {
    return pool->enforce_weak_;
  }
  static bool get_is_placeholder(const Descriptor* descriptor) {
    return descriptor != nullptr && descriptor->is_placeholder_;
  }
  static void assert_mutex_held(const DescriptorPool* pool) {
    if (pool->mutex_ != nullptr) {
      pool->mutex_->AssertHeld();
    }
  }

  // Must be run only after options have been interpreted.
  //
  // NOTE: Validation code must only reference the options in the mutable
  // descriptors, which are the ones that have been interpreted. The const
  // proto references are passed in only so they can be provided to calls to
  // AddError(). Do not look at their options, which have not been interpreted.
  void ValidateOptions(const FileDescriptor* file,
                       const FileDescriptorProto& proto);
  void ValidateFileFeatures(const FileDescriptor* file,
                            const FileDescriptorProto& proto);
  void ValidateOptions(const Descriptor* message, const DescriptorProto& proto);
  void ValidateOptions(const OneofDescriptor* oneof,
                       const OneofDescriptorProto& proto);
  void ValidateOptions(const FieldDescriptor* field,
                       const FieldDescriptorProto& proto);
  void ValidateFieldFeatures(const FieldDescriptor* field,
                             const FieldDescriptorProto& proto);
  void ValidateOptions(const EnumDescriptor* enm,
                       const EnumDescriptorProto& proto);
  void ValidateOptions(const EnumValueDescriptor* enum_value,
                       const EnumValueDescriptorProto& proto);
  void ValidateOptions(const Descriptor::ExtensionRange* range,
                       const DescriptorProto::ExtensionRange& proto) {}
  void ValidateExtensionRangeOptions(const DescriptorProto& proto,
                                     const Descriptor& message);
  void MaybeAddError(const absl::Status& status, absl::string_view full_name,
                     const Message& descriptor,
                     DescriptorPool::ErrorCollector::ErrorLocation location);
  void ValidateExtensionDeclaration(
      absl::string_view full_name,
      const RepeatedPtrField<ExtensionRangeOptions_Declaration>& declarations,
      const DescriptorProto_ExtensionRange& proto,
      absl::flat_hash_set<absl::string_view>& full_name_set);
  void ValidateOptions(const ServiceDescriptor* service,
                       const ServiceDescriptorProto& proto);
  void ValidateOptions(const MethodDescriptor* method,
                       const MethodDescriptorProto& proto);
  void ValidateProto3(const FileDescriptor* file,
                      const FileDescriptorProto& proto);
  void ValidateProto3Message(const Descriptor* message,
                             const DescriptorProto& proto);
  void ValidateProto3Field(const FieldDescriptor* field,
                           const FieldDescriptorProto& proto);

  // Returns true if the map entry message is compatible with the
  // auto-generated entry message from map fields syntax.
  bool ValidateMapEntry(const FieldDescriptor* field,
                        const FieldDescriptorProto& proto);

  // Recursively detects naming conflicts with map entry types for a
  // better error message.
  void DetectMapConflicts(const Descriptor* message,
                          const DescriptorProto& proto);

  void ValidateJSType(const FieldDescriptor* field,
                      const FieldDescriptorProto& proto);

  template <typename DescriptorT, typename DescriptorProtoT>
  void ValidateNamingStyle(const DescriptorT*, const DescriptorProtoT&);

  template <typename DescriptorT>
  bool IsStyleOrGreater(const DescriptorT* descriptor,
                        FeatureSet::EnforceNamingStyle style) {
    return internal::InternalFeatureHelper::GetFeatures(*descriptor)
                   .enforce_naming_style() >= style &&
           // Required because STYLE_LEGACY comes after STYLE2024 in enum
           // definition.
           internal::InternalFeatureHelper::GetFeatures(*descriptor)
                   .enforce_naming_style() != FeatureSet::STYLE_LEGACY;
  }

  // Nothing to validate for extension ranges. This overload only exists
  // so that VisitDescriptors can be exhaustive.
  void ValidateNamingStyle(const Descriptor::ExtensionRange* ext_range,
                           const DescriptorProto::ExtensionRange& proto) {}

  // When called, check the listed descriptor against protobuf limits, such as
  // max number of fields per message, max number of fields in a oneof, or max
  // number of values in an enum. This is a feature introduced in Edition 2026.
  void ValidateProtoLimits(const Descriptor* message,
                           const DescriptorProto& proto);
  void ValidateProtoLimits(const OneofDescriptor* oneof,
                           const OneofDescriptorProto& proto);
  void ValidateProtoLimits(const EnumDescriptor* enum_descriptor,
                           const EnumDescriptorProto& proto);

  // Overloads with nothing to validate. These overload only exist
  // so that VisitDescriptors can be exhaustive.
  void ValidateProtoLimits(const FileDescriptor* file,
                           const FileDescriptorProto& proto) {}
  void ValidateProtoLimits(const FieldDescriptor* field,
                           const FieldDescriptorProto& proto) {}
  void ValidateProtoLimits(const EnumValueDescriptor* file,
                           const EnumValueDescriptorProto& proto) {}
  void ValidateProtoLimits(const ServiceDescriptor* file,
                           const ServiceDescriptorProto& proto) {}
  void ValidateProtoLimits(const MethodDescriptor* file,
                           const MethodDescriptorProto& proto) {}
  void ValidateProtoLimits(const Descriptor::ExtensionRange* ext_range,
                           const DescriptorProto::ExtensionRange& proto) {}
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_DESCRIPTOR_BUILDER_H__
