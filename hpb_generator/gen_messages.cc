// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb_generator/gen_messages.h"

#include <cstddef>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/numeric/bits.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "hpb_generator/context.h"
#include "hpb_generator/gen_accessors.h"
#include "hpb_generator/gen_enums.h"
#include "hpb_generator/gen_extensions.h"
#include "hpb_generator/gen_utils.h"
#include "hpb_generator/keywords.h"
#include "hpb_generator/names.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/c/names.h"
#include "upb_generator/minitable/names.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

using Sub = google::protobuf::io::Printer::Sub;

void WriteModelAccessDeclaration(const google::protobuf::Descriptor* descriptor,
                                 Context& ctx);
void WriteModelPublicDeclaration(
    const google::protobuf::Descriptor* descriptor,
    const std::vector<const google::protobuf::FieldDescriptor*>& file_exts,
    const std::vector<const google::protobuf::EnumDescriptor*>& file_enums, Context& ctx);
void WriteExtensionIdentifiersInClassHeader(
    const google::protobuf::Descriptor* message,
    const std::vector<const google::protobuf::FieldDescriptor*>& file_exts, Context& ctx);
void WriteModelProxyDeclaration(const google::protobuf::Descriptor* descriptor,
                                Context& ctx);
void WriteModelCProxyDeclaration(const google::protobuf::Descriptor* descriptor,
                                 Context& ctx);
void WriteDefaultInstanceHeader(const google::protobuf::Descriptor* message,
                                Context& ctx);
void WriteDefaultInstanceDefinitionHeader(const google::protobuf::Descriptor* message,
                                          Context& ctx);
void WriteUsingEnumsInHeader(
    const google::protobuf::Descriptor* message,
    const std::vector<const google::protobuf::EnumDescriptor*>& file_enums, Context& ctx);

// Writes message class declarations into .hpb.h.
//
// For each proto Foo, FooAccess and FooProxy/FooCProxy are generated
// that are exposed to users as Foo , Ptr<Foo> and Ptr<const Foo>.
void WriteMessageClassDeclarations(
    const google::protobuf::Descriptor* descriptor,
    const std::vector<const google::protobuf::FieldDescriptor*>& file_exts,
    const std::vector<const google::protobuf::EnumDescriptor*>& file_enums,
    Context& ctx) {
  if (IsMapEntryMessage(descriptor)) {
    // Skip map entry generation. Low level accessors for maps are
    // generated that don't require a separate map type.
    return;
  }

  // Forward declaration of Proto Class for GCC handling of free friend method.
  ctx.Emit(
      {
          Sub("class_name", ClassName(descriptor)),
          Sub("model_access",
              [&] { WriteModelAccessDeclaration(descriptor, ctx); })
              .WithSuffix(";"),
          Sub("public_decl",
              [&] {
                WriteModelPublicDeclaration(descriptor, file_exts, file_enums,
                                            ctx);
              })
              .WithSuffix(";"),
          Sub("cproxy_decl",
              [&] { WriteModelCProxyDeclaration(descriptor, ctx); })
              .WithSuffix(";"),
          Sub("proxy_decl",
              [&] { WriteModelProxyDeclaration(descriptor, ctx); })
              .WithSuffix(";"),
          Sub("default_instance",
              [&] { WriteDefaultInstanceDefinitionHeader(descriptor, ctx); })
              .WithSuffix(";"),
      },
      R"cc(
        class $class_name$;
        namespace internal {
        $model_access$;
        }  // namespace internal

        $public_decl$;
        namespace internal {
        $cproxy_decl$;
        $proxy_decl$;
        }  // namespace internal
        $default_instance$;
      )cc");
}

void WriteModelAccessDeclaration(const google::protobuf::Descriptor* descriptor,
                                 Context& ctx) {
  ctx.Emit({Sub("class_name", ClassName(descriptor)),
            Sub("qualified_class_name", QualifiedClassName(descriptor)),
            Sub("upb_msg_name",
                upb::generator::CApiMessageType(descriptor->full_name())),
            Sub("field_accessors",
                [&] { WriteFieldAccessorsInHeader(descriptor, ctx); })
                .WithSuffix(";"),
            Sub("oneof_accessors",
                [&] { WriteOneofAccessorsInHeader(descriptor, ctx); })
                .WithSuffix(";")},
           R"cc(
             class $class_name$Access {
              public:
               $class_name$Access() {}
               $class_name$Access($upb_msg_name$* msg, upb_Arena* arena)
                   : msg_(msg), arena_(arena) {
                 assert(arena != nullptr);
               }  // NOLINT
               $class_name$Access(const $upb_msg_name$* msg, upb_Arena* arena)
                   : msg_(const_cast<$upb_msg_name$*>(msg)), arena_(arena) {
                 assert(arena != nullptr);
               }  // NOLINT

               $field_accessors$;
               $oneof_accessors$;

              private:
               friend class $qualified_class_name$;
               friend class $class_name$Proxy;
               friend class $class_name$CProxy;
               friend struct ::hpb::internal::PrivateAccess;
               $upb_msg_name$* msg_;
               upb_Arena* arena_;
             };
           )cc");
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

std::string FieldConstantName(const google::protobuf::FieldDescriptor* field) {
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

void WriteConstFieldNumbers(Context& ctx,
                            const google::protobuf::Descriptor* descriptor) {
  for (auto field : internal::FieldRange(descriptor)) {
    ctx.Emit({{"name", FieldConstantName(field)}, {"number", field->number()}},
             "static constexpr ::uint32_t $name$ = $number$;\n");
  }
  ctx.Emit("\n\n");
}

void WriteModelPublicDeclaration(
    const google::protobuf::Descriptor* descriptor,
    const std::vector<const google::protobuf::FieldDescriptor*>& file_exts,
    const std::vector<const google::protobuf::EnumDescriptor*>& file_enums,
    Context& ctx) {
  ctx.Emit({{"class_name", ClassName(descriptor)},
            {"qualified_class_name", QualifiedClassName(descriptor)}},
           R"cc(
             class $class_name$ final : private internal::$class_name$Access {
              public:
               using Access = internal::$class_name$Access;
               using Proxy = internal::$class_name$Proxy;
               using CProxy = internal::$class_name$CProxy;

               $class_name$();

               $class_name$(const $class_name$& from);
               $class_name$& operator=(const $qualified_class_name$& from);
               $class_name$(const CProxy& from);
               $class_name$(const Proxy& from);
               $class_name$& operator=(const CProxy& from);

               $class_name$($class_name$&& m)
                   : Access(std::exchange(m.msg_, nullptr),
                            std::exchange(m.arena_, nullptr)),
                     owned_arena_(std::move(m.owned_arena_)) {}

               $class_name$& operator=($class_name$&& m) {
                 msg_ = std::exchange(m.msg_, nullptr);
                 arena_ = std::exchange(m.arena_, nullptr);
                 owned_arena_ = std::move(m.owned_arena_);
                 return *this;
               }
           )cc");

  WriteUsingAccessorsInHeader(descriptor, MessageClassType::kMessage, ctx);
  WriteUsingEnumsInHeader(descriptor, file_enums, ctx);
  WriteDefaultInstanceHeader(descriptor, ctx);
  WriteExtensionIdentifiersInClassHeader(descriptor, file_exts, ctx);
  if (descriptor->extension_range_count()) {
    // for typetrait checking
    ctx.Emit({{"class_name", ClassName(descriptor)}},
             "using ExtendableType = $class_name$;\n");
  }
  // Note: free function friends that are templates such as ::hpb::Parse
  // require explicit <$2> type parameter in declaration to be able to compile
  // with gcc otherwise the compiler will fail with
  // "has not been declared within namespace" error. Even though there is a
  // namespace qualifier, cross namespace matching fails.
  ctx.Emit(
      R"cc(
        static const upb_MiniTable* minitable();
      )cc");
  ctx.Emit("\n");
  WriteConstFieldNumbers(ctx, descriptor);
  ctx.Emit({{"class_name", ClassName(descriptor)},
            {"c_api_msg_type",
             upb::generator::CApiMessageType(descriptor->full_name())}},
           R"cc(
             private:
             const upb_Message* msg() const { return UPB_UPCAST(msg_); }
             upb_Message* msg() { return UPB_UPCAST(msg_); }

             upb_Arena* arena() const { return arena_; }

             $class_name$(upb_Message* msg, upb_Arena* arena) : $class_name$Access() {
               msg_ = ($c_api_msg_type$*)msg;
               arena_ = ::hpb::interop::upb::UnwrapArena(owned_arena_);
               upb_Arena_Fuse(arena_, arena);
             }
             ::hpb::Arena owned_arena_;
             friend struct ::hpb::internal::PrivateAccess;
             friend Proxy;
             friend CProxy;
           )cc");
  ctx.Emit("};\n\n");
}

void WriteModelProxyDeclaration(const google::protobuf::Descriptor* descriptor,
                                Context& ctx) {
  // Foo::Proxy.
  ctx.Emit({{"class_name", ClassName(descriptor)}},
           R"cc(
             class $class_name$Proxy final
                 : private internal::$class_name$Access {
              public:
               $class_name$Proxy() = delete;
               $class_name$Proxy(const $class_name$Proxy& m)
                   : internal::$class_name$Access() {
                 msg_ = m.msg_;
                 arena_ = m.arena_;
               }
               $class_name$Proxy($class_name$* m) : internal::$class_name$Access() {
                 msg_ = m->msg_;
                 arena_ = m->arena_;
               }
               $class_name$Proxy operator=(const $class_name$Proxy& m) {
                 msg_ = m.msg_;
                 arena_ = m.arena_;
                 return *this;
               }
           )cc");

  WriteUsingAccessorsInHeader(descriptor, MessageClassType::kMessageProxy, ctx);
  ctx.Emit("\n");
  ctx.Emit(
      {{"class_name", ClassName(descriptor)},
       {"c_api_msg_type",
        upb::generator::CApiMessageType(descriptor->full_name())},
       {"qualified_class_name", QualifiedClassName(descriptor)}},
      R"cc(
        private:
        upb_Message* msg() const { return UPB_UPCAST(msg_); }

        upb_Arena* arena() const { return arena_; }

        $class_name$Proxy(upb_Message* msg, upb_Arena* arena)
            : internal::$class_name$Access(($c_api_msg_type$*)msg, arena) {}
        friend $class_name$::Proxy(
            ::hpb::CreateMessage<$class_name$>(::hpb::Arena& arena));
        friend $class_name$::Proxy(hpb::interop::upb::MakeHandle<$class_name$>(
            upb_Message*, upb_Arena*));
        friend struct ::hpb::internal::PrivateAccess;
        friend class RepeatedFieldProxy;
        friend class $class_name$CProxy;
        friend class $class_name$Access;
        friend class ::hpb::Ptr<$class_name$>;
        friend class ::hpb::Ptr<const $class_name$>;
        static const upb_MiniTable* minitable() { return $class_name$::minitable(); }
        friend const upb_MiniTable* ::hpb::interop::upb::GetMiniTable<
            $class_name$Proxy>(const $class_name$Proxy* message);
        friend const upb_MiniTable* ::hpb::interop::upb::GetMiniTable<
            $class_name$Proxy>(::hpb::Ptr<$class_name$Proxy> message);
        friend upb_Arena* hpb::interop::upb::GetArena<$qualified_class_name$>(
            $qualified_class_name$* message);
        friend upb_Arena* hpb::interop::upb::GetArena<$qualified_class_name$>(
            ::hpb::Ptr<$qualified_class_name$> message);
        static void Rebind($class_name$Proxy& lhs, const $class_name$Proxy& rhs) {
          lhs.msg_ = rhs.msg_;
          lhs.arena_ = rhs.arena_;
        }
      )cc");
  ctx.Emit("};\n\n");
}

void WriteModelCProxyDeclaration(const google::protobuf::Descriptor* descriptor,
                                 Context& ctx) {
  // Foo::CProxy.
  ctx.Emit({{"class_name", ClassName(descriptor)}},
           R"cc(
             class $class_name$CProxy final
                 : private internal::$class_name$Access {
              public:
               $class_name$CProxy() = delete;
               $class_name$CProxy(const $class_name$* m)
                   : internal::$class_name$Access(
                         m->msg_, hpb::interop::upb::GetArena(m)) {}
               $class_name$CProxy($class_name$Proxy m);
           )cc");

  WriteUsingAccessorsInHeader(descriptor, MessageClassType::kMessageCProxy,
                              ctx);

  ctx.Emit({{"class_name", ClassName(descriptor)},
            {"c_api_msg_type",
             upb::generator::CApiMessageType(descriptor->full_name())}},
           R"cc(
             private:
             using AsNonConst = $class_name$Proxy;
             const upb_Message* msg() const { return UPB_UPCAST(msg_); }
             upb_Arena* arena() const { return arena_; }

             $class_name$CProxy(const upb_Message* msg, upb_Arena* arena)
                 : internal::$class_name$Access(($c_api_msg_type$*)msg,
                                                arena){};
             friend struct ::hpb::internal::PrivateAccess;
             friend class RepeatedFieldProxy;
             friend class ::hpb::Ptr<$class_name$>;
             friend class ::hpb::Ptr<const $class_name$>;
             static const upb_MiniTable* minitable() { return $class_name$::minitable(); }
             friend const upb_MiniTable* ::hpb::interop::upb::GetMiniTable<
                 $class_name$CProxy>(const $class_name$CProxy* message);
             friend const upb_MiniTable* ::hpb::interop::upb::GetMiniTable<
                 $class_name$CProxy>(::hpb::Ptr<$class_name$CProxy> message);

             static void Rebind($class_name$CProxy& lhs, const $class_name$CProxy& rhs) {
               lhs.msg_ = rhs.msg_;
               lhs.arena_ = rhs.arena_;
             }
           )cc");
  ctx.Emit("};\n\n");
}

void WriteDefaultInstanceHeader(const google::protobuf::Descriptor* message,
                                Context& ctx) {
  if (message->options().map_entry()) {
    return;
  }
  ctx.Emit({{"class_name", ClassName(message)}},
           R"cc(
             static ::hpb::Ptr<const $class_name$> default_instance();
           )cc");
}

void WriteDefaultInstanceDefinitionHeader(const google::protobuf::Descriptor* message,
                                          Context& ctx) {
  if (message->options().map_entry()) {
    return;
  }
  ctx.Emit(
      {{"class_name", ClassName(message)},
       {"size_class",
        // Use log2 size class of message size to reduce the number of default
        // instances created.
        absl::bit_ceil(static_cast<size_t>(ctx.GetLayoutSize(message)))}},
      R"cc(
        inline ::hpb::Ptr<const $class_name$> $class_name$::default_instance() {
          return ::hpb::interop::upb::MakeCHandle<$class_name$>(
              ::hpb::internal::backend::upb::DefaultInstance<
                  $size_class$>::msg(),
              ::hpb::internal::backend::upb::DefaultInstance<
                  $size_class$>::arena());
        }
      )cc");
}

void WriteMessageImplementation(
    const google::protobuf::Descriptor* descriptor,
    const std::vector<const google::protobuf::FieldDescriptor*>& file_exts,
    Context& ctx) {
  bool message_is_map_entry = descriptor->options().map_entry();
  if (!message_is_map_entry) {
    // Constructor.
    ctx.Emit(
        {{"class_name", ClassName(descriptor)},
         {"c_api_msg_type",
          upb::generator::CApiMessageType(descriptor->full_name())},
         {"minitable_var",
          ::upb::generator::MiniTableMessageVarName(descriptor->full_name())},
         {"qualified_class_name", QualifiedClassName(descriptor)}},
        R"cc(
          $class_name$::$class_name$() : $class_name$Access() {
            arena_ = ::hpb::interop::upb::UnwrapArena(owned_arena_);
            msg_ = $c_api_msg_type$_new(arena_);
          }
          $class_name$::$class_name$(const $class_name$& from) : $class_name$Access() {
            arena_ = ::hpb::interop::upb::UnwrapArena(owned_arena_);
            msg_ = ($c_api_msg_type$*)::hpb::internal::DeepClone(
                UPB_UPCAST(from.msg_), &$minitable_var$, arena_);
          }
          $class_name$::$class_name$(const CProxy& from) : $class_name$Access() {
            arena_ = ::hpb::interop::upb::UnwrapArena(owned_arena_);
            msg_ = ($c_api_msg_type$*)::hpb::internal::DeepClone(
                ::hpb::interop::upb::GetMessage(&from), &$minitable_var$,
                arena_);
          }
          $class_name$::$class_name$(const Proxy& from)
              : $class_name$(static_cast<const CProxy&>(from)) {}
          internal::$class_name$CProxy::$class_name$CProxy($class_name$Proxy m)
              : $class_name$Access() {
            arena_ = m.arena_;
            msg_ = ($c_api_msg_type$*)::hpb::interop::upb::GetMessage(&m);
          }
          $class_name$& $class_name$::operator=(const $qualified_class_name$& from) {
            arena_ = ::hpb::interop::upb::UnwrapArena(owned_arena_);
            msg_ = ($c_api_msg_type$*)::hpb::internal::DeepClone(
                UPB_UPCAST(from.msg_), &$minitable_var$, arena_);
            return *this;
          }
          $class_name$& $class_name$::operator=(const CProxy& from) {
            arena_ = ::hpb::interop::upb::UnwrapArena(owned_arena_);
            msg_ = ($c_api_msg_type$*)::hpb::internal::DeepClone(
                ::hpb::interop::upb::GetMessage(&from), &$minitable_var$,
                arena_);
            return *this;
          }
        )cc");
    ctx.Emit("\n");
    // Minitable
    ctx.Emit({{"class_name", ClassName(descriptor)},
              {"minitable_var", ::upb::generator::MiniTableMessageVarName(
                                    descriptor->full_name())}},
             R"cc(
               const upb_MiniTable* $class_name$::minitable() {
                 return &$minitable_var$;
               }
             )cc");
    ctx.Emit("\n");
  }

  WriteAccessorsInSource(descriptor, ctx);
}

void WriteExtensionIdentifiersInClassHeader(
    const google::protobuf::Descriptor* message,
    const std::vector<const google::protobuf::FieldDescriptor*>& file_exts,
    Context& ctx) {
  for (auto* ext : file_exts) {
    if (ext->extension_scope() &&
        ext->extension_scope()->full_name() == message->full_name()) {
      WriteExtensionIdentifierHeader(ext, ctx);
    }
  }
}

void WriteUsingEnumsInHeader(
    const google::protobuf::Descriptor* message,
    const std::vector<const google::protobuf::EnumDescriptor*>& file_enums,
    Context& ctx) {
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
    ctx.Emit({{"enum_name", ResolveKeywordConflict(enum_descriptor->name())}},
             "using $enum_name$");
    if (enum_descriptor->options().deprecated()) {
      ctx.Emit({{"enum_name", ResolveKeywordConflict(enum_descriptor->name())}},
               " ABSL_DEPRECATED(\"Proto enum $enum_name$\")");
    }
    ctx.Emit({{"enum_resolved_type_name", enum_resolved_type_name}},
             " = $enum_resolved_type_name$;\n");
    int value_count = enum_descriptor->value_count();
    for (int i = 0; i < value_count; i++) {
      ctx.Emit({{"enum_name", ResolveKeywordConflict(enum_descriptor->name())},
                {"enum_value_name",
                 ResolveKeywordConflict(enum_descriptor->value(i)->name())}},
               "static constexpr $enum_name$ $enum_value_name$");
      if (enum_descriptor->options().deprecated() ||
          enum_descriptor->value(i)->options().deprecated()) {
        ctx.Emit({{"enum_value_name",
                   ResolveKeywordConflict(enum_descriptor->value(i)->name())}},
                 " ABSL_DEPRECATED(\"Proto enum value $enum_value_name$\") ");
      }
      ctx.Emit({{"enum_value_symbol",
                 EnumValueSymbolInNameSpace(enum_descriptor,
                                            enum_descriptor->value(i))}},
               " = $enum_value_symbol$;\n");
    }
  }
}

}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google
