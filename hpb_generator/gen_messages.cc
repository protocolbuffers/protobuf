// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/hpb/gen_messages.h"

#include <cstddef>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/hpb/gen_accessors.h"
#include "google/protobuf/compiler/hpb/gen_enums.h"
#include "google/protobuf/compiler/hpb/gen_extensions.h"
#include "google/protobuf/compiler/hpb/gen_utils.h"
#include "google/protobuf/compiler/hpb/names.h"
#include "google/protobuf/compiler/hpb/output.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/minitable/names.h"

namespace google::protobuf::hpb_generator {

namespace protobuf = ::proto2;

void WriteModelAccessDeclaration(const protobuf::Descriptor* descriptor,
                                 Output& output);
void WriteModelPublicDeclaration(
    const protobuf::Descriptor* descriptor,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    const std::vector<const protobuf::EnumDescriptor*>& file_enums,
    Output& output);
void WriteExtensionIdentifiersInClassHeader(
    const protobuf::Descriptor* message,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    Output& output);
void WriteModelProxyDeclaration(const protobuf::Descriptor* descriptor,
                                Output& output);
void WriteModelCProxyDeclaration(const protobuf::Descriptor* descriptor,
                                 Output& output);
void WriteInternalForwardDeclarationsInHeader(
    const protobuf::Descriptor* message, Output& output);
void WriteDefaultInstanceHeader(const protobuf::Descriptor* message,
                                Output& output);
void WriteExtensionIdentifiersImplementation(
    const protobuf::Descriptor* message,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    Output& output);
void WriteUsingEnumsInHeader(
    const protobuf::Descriptor* message,
    const std::vector<const protobuf::EnumDescriptor*>& file_enums,
    Output& output);

// Writes message class declarations into .upb.proto.h.
//
// For each proto Foo, FooAccess and FooProxy/FooCProxy are generated
// that are exposed to users as Foo , Ptr<Foo> and Ptr<const Foo>.
void WriteMessageClassDeclarations(
    const protobuf::Descriptor* descriptor,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    const std::vector<const protobuf::EnumDescriptor*>& file_enums,
    Output& output) {
  if (IsMapEntryMessage(descriptor)) {
    // Skip map entry generation. Low level accessors for maps are
    // generated that don't require a separate map type.
    return;
  }

  // Forward declaration of Proto Class for GCC handling of free friend method.
  output("class $0;\n", ClassName(descriptor));
  output("namespace internal {\n\n");
  WriteModelAccessDeclaration(descriptor, output);
  output("\n");
  WriteInternalForwardDeclarationsInHeader(descriptor, output);
  output("\n");
  output("}  // namespace internal\n\n");
  WriteModelPublicDeclaration(descriptor, file_exts, file_enums, output);
  output("namespace internal {\n");
  WriteModelCProxyDeclaration(descriptor, output);
  WriteModelProxyDeclaration(descriptor, output);
  output("}  // namespace internal\n\n");
}

void WriteModelAccessDeclaration(const protobuf::Descriptor* descriptor,
                                 Output& output) {
  output(
      R"cc(
        class $0Access {
         public:
          $0Access() {}
          $0Access($1* msg, upb_Arena* arena) : msg_(msg), arena_(arena) {
            assert(arena != nullptr);
          }  // NOLINT
          $0Access(const $1* msg, upb_Arena* arena)
              : msg_(const_cast<$1*>(msg)), arena_(arena) {
            assert(arena != nullptr);
          }  // NOLINT
          void* GetInternalArena() const { return arena_; }
      )cc",
      ClassName(descriptor), MessageName(descriptor));
  WriteFieldAccessorsInHeader(descriptor, output);
  WriteOneofAccessorsInHeader(descriptor, output);
  output.Indent();
  output(
      R"cc(
        private:
        friend class $2;
        friend class $0Proxy;
        friend class $0CProxy;
        friend struct ::hpb::internal::PrivateAccess;
        $1* msg_;
        upb_Arena* arena_;
      )cc",
      ClassName(descriptor), MessageName(descriptor),
      QualifiedClassName(descriptor));
  output.Outdent();
  output("};\n");
}

std::string UnderscoresToCamelCase(absl::string_view input,
                                   bool cap_next_letter) {
  std::string result;

  for (size_t i = 0; i < input.size(); i++) {
    if (absl::ascii_islower(input[i])) {
      if (cap_next_letter) {
        result += absl::ascii_toupper(input[i]);
      } else {
        result += input[i];
      }
      cap_next_letter = false;
    } else if (absl::ascii_isupper(input[i])) {
      // Capital letters are left as-is.
      result += input[i];
      cap_next_letter = false;
    } else if (absl::ascii_isdigit(input[i])) {
      result += input[i];
      cap_next_letter = true;
    } else {
      cap_next_letter = true;
    }
  }
  return result;
}

std::string FieldConstantName(const protobuf::FieldDescriptor* field) {
  std::string field_name = UnderscoresToCamelCase(field->name(), true);
  std::string result = absl::StrCat("k", field_name, "FieldNumber");

  if (!field->is_extension() &&
      field->containing_type()->FindFieldByCamelcaseName(
          field->camelcase_name()) != field) {
    // This field's camelcase name is not unique, add field number to make it
    // unique.
    absl::StrAppend(&result, "_", field->number());
  }
  return result;
}

void WriteConstFieldNumbers(Output& output,
                            const protobuf::Descriptor* descriptor) {
  for (auto field : FieldRange(descriptor)) {
    output("static constexpr ::uint32_t $0 = $1;\n", FieldConstantName(field),
           field->number());
  }
  output("\n\n");
}

void WriteModelPublicDeclaration(
    const protobuf::Descriptor* descriptor,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    const std::vector<const protobuf::EnumDescriptor*>& file_enums,
    Output& output) {
  output(
      R"cc(
        class $0 final : private internal::$0Access {
         public:
          using Access = internal::$0Access;
          using Proxy = internal::$0Proxy;
          using CProxy = internal::$0CProxy;

          $0();

          $0(const $0& from);
          $0& operator=(const $3& from);
          $0(const CProxy& from);
          $0(const Proxy& from);
          $0& operator=(const CProxy& from);

          $0($0&& m)
              : Access(std::exchange(m.msg_, nullptr),
                       std::exchange(m.arena_, nullptr)),
                owned_arena_(std::move(m.owned_arena_)) {}

          $0& operator=($0&& m) {
            msg_ = std::exchange(m.msg_, nullptr);
            arena_ = std::exchange(m.arena_, nullptr);
            owned_arena_ = std::move(m.owned_arena_);
            return *this;
          }
      )cc",
      ClassName(descriptor),
      ::upb::generator::MiniTableMessageVarName(descriptor->full_name()),
      MessageName(descriptor), QualifiedClassName(descriptor));

  WriteUsingAccessorsInHeader(descriptor, MessageClassType::kMessage, output);
  WriteUsingEnumsInHeader(descriptor, file_enums, output);
  WriteDefaultInstanceHeader(descriptor, output);
  WriteExtensionIdentifiersInClassHeader(descriptor, file_exts, output);
  if (descriptor->extension_range_count()) {
    // for typetrait checking
    output("using ExtendableType = $0;\n", ClassName(descriptor));
  }
  // Note: free function friends that are templates such as ::hpb::Parse
  // require explicit <$2> type parameter in declaration to be able to compile
  // with gcc otherwise the compiler will fail with
  // "has not been declared within namespace" error. Even though there is a
  // namespace qualifier, cross namespace matching fails.
  output.Indent();
  output(
      R"cc(
        static const upb_MiniTable* minitable();
        using $0Access::GetInternalArena;
      )cc",
      ClassName(descriptor));
  output("\n");
  WriteConstFieldNumbers(output, descriptor);
  output(
      R"cc(
        private:
        const upb_Message* msg() const { return UPB_UPCAST(msg_); }
        upb_Message* msg() { return UPB_UPCAST(msg_); }

        $0(upb_Message* msg, upb_Arena* arena) : $0Access() {
          msg_ = ($1*)msg;
          arena_ = owned_arena_.ptr();
          upb_Arena_Fuse(arena_, arena);
        }
        ::hpb::Arena owned_arena_;
        friend struct ::hpb::internal::PrivateAccess;
        friend Proxy;
        friend CProxy;
        friend absl::StatusOr<$2>(::hpb::Parse<$2>(absl::string_view bytes,
                                                   int options));
        friend absl::StatusOr<$2>(::hpb::Parse<$2>(
            absl::string_view bytes,
            const ::hpb::ExtensionRegistry& extension_registry, int options));
        friend upb_Arena* hpb::interop::upb::GetArena<$0>($0* message);
        friend upb_Arena* hpb::interop::upb::GetArena<$0>(::hpb::Ptr<$0> message);
        friend $0(hpb::interop::upb::MoveMessage<$0>(upb_Message* msg,
                                                     upb_Arena* arena));
      )cc",
      ClassName(descriptor), MessageName(descriptor),
      QualifiedClassName(descriptor));
  output.Outdent();
  output("};\n\n");
}

void WriteModelProxyDeclaration(const protobuf::Descriptor* descriptor,
                                Output& output) {
  // Foo::Proxy.
  output(
      R"cc(
        class $0Proxy final : private internal::$0Access {
         public:
          $0Proxy() = delete;
          $0Proxy(const $0Proxy& m) : internal::$0Access() {
            msg_ = m.msg_;
            arena_ = m.arena_;
          }
          $0Proxy($0* m) : internal::$0Access() {
            msg_ = m->msg_;
            arena_ = m->arena_;
          }
          $0Proxy operator=(const $0Proxy& m) {
            msg_ = m.msg_;
            arena_ = m.arena_;
            return *this;
          }
          using $0Access::GetInternalArena;
      )cc",
      ClassName(descriptor));

  WriteUsingAccessorsInHeader(descriptor, MessageClassType::kMessageProxy,
                              output);
  output("\n");
  output.Indent(1);
  output(
      R"cc(
        private:
        upb_Message* msg() const { return UPB_UPCAST(msg_); }

        $0Proxy(upb_Message* msg, upb_Arena* arena)
            : internal::$0Access(($1*)msg, arena) {}
        friend $0::Proxy(::hpb::CreateMessage<$0>(::hpb::Arena& arena));
        friend $0::Proxy(hpb::interop::upb::MakeHandle<$0>(upb_Message*, upb_Arena*));
        friend struct ::hpb::internal::PrivateAccess;
        friend class RepeatedFieldProxy;
        friend class $0CProxy;
        friend class $0Access;
        friend class ::hpb::Ptr<$0>;
        friend class ::hpb::Ptr<const $0>;
        static const upb_MiniTable* minitable() { return $0::minitable(); }
        friend const upb_MiniTable* ::hpb::interop::upb::GetMiniTable<$0Proxy>(
            const $0Proxy* message);
        friend const upb_MiniTable* ::hpb::interop::upb::GetMiniTable<$0Proxy>(
            ::hpb::Ptr<$0Proxy> message);
        friend upb_Arena* hpb::interop::upb::GetArena<$2>($2* message);
        friend upb_Arena* hpb::interop::upb::GetArena<$2>(::hpb::Ptr<$2> message);
        static void Rebind($0Proxy& lhs, const $0Proxy& rhs) {
          lhs.msg_ = rhs.msg_;
          lhs.arena_ = rhs.arena_;
        }
      )cc",
      ClassName(descriptor), MessageName(descriptor),
      QualifiedClassName(descriptor));
  output.Outdent(1);
  output("};\n\n");
}

void WriteModelCProxyDeclaration(const protobuf::Descriptor* descriptor,
                                 Output& output) {
  // Foo::CProxy.
  output(
      R"cc(
        class $0CProxy final : private internal::$0Access {
         public:
          $0CProxy() = delete;
          $0CProxy(const $0* m)
              : internal::$0Access(m->msg_, hpb::interop::upb::GetArena(m)) {}
          $0CProxy($0Proxy m);
          using $0Access::GetInternalArena;
      )cc",
      ClassName(descriptor), MessageName(descriptor));

  WriteUsingAccessorsInHeader(descriptor, MessageClassType::kMessageCProxy,
                              output);

  output.Indent(1);
  output(
      R"cc(
        private:
        using AsNonConst = $0Proxy;
        const upb_Message* msg() const { return UPB_UPCAST(msg_); }

        $0CProxy(const upb_Message* msg, upb_Arena* arena)
            : internal::$0Access(($1*)msg, arena){};
        friend struct ::hpb::internal::PrivateAccess;
        friend class RepeatedFieldProxy;
        friend class ::hpb::Ptr<$0>;
        friend class ::hpb::Ptr<const $0>;
        static const upb_MiniTable* minitable() { return $0::minitable(); }
        friend const upb_MiniTable* ::hpb::interop::upb::GetMiniTable<$0CProxy>(
            const $0CProxy* message);
        friend const upb_MiniTable* ::hpb::interop::upb::GetMiniTable<$0CProxy>(
            ::hpb::Ptr<$0CProxy> message);

        static void Rebind($0CProxy& lhs, const $0CProxy& rhs) {
          lhs.msg_ = rhs.msg_;
          lhs.arena_ = rhs.arena_;
        }
      )cc",
      ClassName(descriptor), MessageName(descriptor));
  output.Outdent(1);
  output("};\n\n");
}

void WriteDefaultInstanceHeader(const protobuf::Descriptor* message,
                                Output& output) {
  output("  static ::hpb::Ptr<const $0> default_instance();\n",
         ClassName(message));
}

void WriteMessageImplementation(
    const protobuf::Descriptor* descriptor,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    Output& output) {
  bool message_is_map_entry = descriptor->options().map_entry();
  if (!message_is_map_entry) {
    // Constructor.
    output(
        R"cc(
          $0::$0() : $0Access() {
            arena_ = owned_arena_.ptr();
            msg_ = $1_new(arena_);
          }
          $0::$0(const $0& from) : $0Access() {
            arena_ = owned_arena_.ptr();
            msg_ = ($1*)::hpb::internal::DeepClone(UPB_UPCAST(from.msg_), &$2, arena_);
          }
          $0::$0(const CProxy& from) : $0Access() {
            arena_ = owned_arena_.ptr();
            msg_ = ($1*)::hpb::internal::DeepClone(
                ::hpb::interop::upb::GetMessage(&from), &$2, arena_);
          }
          $0::$0(const Proxy& from) : $0(static_cast<const CProxy&>(from)) {}
          internal::$0CProxy::$0CProxy($0Proxy m) : $0Access() {
            arena_ = m.arena_;
            msg_ = ($1*)::hpb::interop::upb::GetMessage(&m);
          }
          $0& $0::operator=(const $3& from) {
            arena_ = owned_arena_.ptr();
            msg_ = ($1*)::hpb::internal::DeepClone(UPB_UPCAST(from.msg_), &$2, arena_);
            return *this;
          }
          $0& $0::operator=(const CProxy& from) {
            arena_ = owned_arena_.ptr();
            msg_ = ($1*)::hpb::internal::DeepClone(
                ::hpb::interop::upb::GetMessage(&from), &$2, arena_);
            return *this;
          }
        )cc",
        ClassName(descriptor), MessageName(descriptor),
        ::upb::generator::MiniTableMessageVarName(descriptor->full_name()),
        QualifiedClassName(descriptor));
    output("\n");
    // Minitable
    output(
        R"cc(
          const upb_MiniTable* $0::minitable() { return &$1; }
        )cc",
        ClassName(descriptor),
        ::upb::generator::MiniTableMessageVarName(descriptor->full_name()));
    output("\n");
  }

  WriteAccessorsInSource(descriptor, output);

  if (!message_is_map_entry) {
    output(
        R"cc(
          struct $0DefaultTypeInternal {
            $1* msg;
            upb_Arena* arena;
          };
          static $0DefaultTypeInternal _$0DefaultTypeBuilder() {
            upb_Arena* arena = upb_Arena_New();
            return $0DefaultTypeInternal{$1_new(arena), arena};
          }
          $0DefaultTypeInternal _$0_default_instance_ = _$0DefaultTypeBuilder();
        )cc",
        ClassName(descriptor), MessageName(descriptor));

    output(
        R"cc(
          ::hpb::Ptr<const $0> $0::default_instance() {
            return ::hpb::interop::upb::MakeCHandle<$0>(
                (upb_Message *)_$0_default_instance_.msg,
                _$0_default_instance_.arena);
          }
        )cc",
        ClassName(descriptor));

    WriteExtensionIdentifiersImplementation(descriptor, file_exts, output);
  }
}

void WriteInternalForwardDeclarationsInHeader(
    const protobuf::Descriptor* message, Output& output) {
  // Write declaration for internal re-usable default_instance without
  // leaking implementation.
  output(
      R"cc(
        struct $0DefaultTypeInternal;
        extern $0DefaultTypeInternal _$0_default_instance_;
      )cc",
      ClassName(message));
}

void WriteExtensionIdentifiersInClassHeader(
    const protobuf::Descriptor* message,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    Output& output) {
  for (auto* ext : file_exts) {
    if (ext->extension_scope() &&
        ext->extension_scope()->full_name() == message->full_name()) {
      WriteExtensionIdentifierHeader(ext, output);
    }
  }
}

void WriteExtensionIdentifiersImplementation(
    const protobuf::Descriptor* message,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    Output& output) {
  for (auto* ext : file_exts) {
    if (ext->extension_scope() &&
        ext->extension_scope()->full_name() == message->full_name()) {
      WriteExtensionIdentifier(ext, output);
    }
  }
}

void WriteUsingEnumsInHeader(
    const protobuf::Descriptor* message,
    const std::vector<const protobuf::EnumDescriptor*>& file_enums,
    Output& output) {
  for (auto* enum_descriptor : file_enums) {
    std::string enum_type_name = EnumTypeName(enum_descriptor);
    std::string enum_resolved_type_name =
        enum_descriptor->file()->package().empty() &&
                enum_descriptor->containing_type() == nullptr
            ? absl::StrCat(kNoPackageNamePrefix,
                           ToCIdent(enum_descriptor->name()))
            : enum_type_name;
    if (enum_descriptor->containing_type() == nullptr ||
        enum_descriptor->containing_type()->full_name() !=
            message->full_name()) {
      continue;
    }
    output("using $0", enum_descriptor->name());
    if (enum_descriptor->options().deprecated()) {
      output(" ABSL_DEPRECATED(\"Proto enum $0\")", enum_descriptor->name());
    }
    output(" = $0;", enum_resolved_type_name);
    output("\n");
    int value_count = enum_descriptor->value_count();
    for (int i = 0; i < value_count; i++) {
      output("static constexpr $0 $1", enum_descriptor->name(),
             enum_descriptor->value(i)->name());
      if (enum_descriptor->options().deprecated() ||
          enum_descriptor->value(i)->options().deprecated()) {
        output(" ABSL_DEPRECATED(\"Proto enum value $0\") ",
               enum_descriptor->value(i)->name());
      }
      output(" = $0;\n", EnumValueSymbolInNameSpace(enum_descriptor,
                                                    enum_descriptor->value(i)));
    }
  }
}

}  // namespace protobuf
}  // namespace google::hpb_generator
