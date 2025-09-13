#include "google/protobuf/compiler/cpp/enum_strategy_unscoped.h"

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/cpp/enum_strategy.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google::protobuf::compiler::cpp {

using Sub = ::google::protobuf::io::Printer::Sub;

void UnscopedEnumStrategy::GenerateEnumDefinitionBlock(
    io::Printer* p, const EnumStrategyContext& ctx) const {
  auto v1 = p->WithVars(ctx.enum_vars);

  auto v2 = p->WithVars({
      Sub("Msg_Enum_Enum_MIN",
          absl::StrCat(p->LookupVar("Msg_Enum_"), ctx.enum_->name(), "_MIN"))
          .AnnotatedAs(ctx.enum_),
      Sub("Msg_Enum_Enum_MAX",
          absl::StrCat(p->LookupVar("Msg_Enum_"), ctx.enum_->name(), "_MAX"))
          .AnnotatedAs(ctx.enum_),
  });
  p->Emit(
      {
          {"values",
           [&] {
             for (int i = 0; i < ctx.enum_->value_count(); ++i) {
               const auto* value = ctx.enum_->value(i);
               p->Emit(
                   {
                       Sub("Msg_Enum_VALUE",
                           absl::StrCat(p->LookupVar("Msg_Enum_"),
                                        EnumValueName(value)))
                           .AnnotatedAs(value),
                       {"kNumber", Int32ToString(value->number())},
                       {"DEPRECATED",
                        value->options().deprecated() ? "[[deprecated]]" : ""},
                   },
                   R"cc(
                     $Msg_Enum_VALUE$$ DEPRECATED$ = $kNumber$,
                   )cc");
             }
           }},
          // Only emit annotations for the $Msg_Enum$ used in the `enum`
          // definition.
          Sub("Msg_Enum_annotated", p->LookupVar("Msg_Enum"))
              .AnnotatedAs(ctx.enum_),
          {"open_enum_sentinels",
           [&] {
             if (ctx.enum_->is_closed()) {
               return;
             }

             // For open enum semantics: generate min and max sentinel values
             // equal to INT32_MIN and INT32_MAX
             p->Emit({{"Msg_Enum_Msg_Enum_",
                       absl::StrCat(p->LookupVar("Msg_Enum"), "_",
                                    p->LookupVar("Msg_Enum_"))}},
                     R"cc(
                       $Msg_Enum_Msg_Enum_$INT_MIN_SENTINEL_DO_NOT_USE_ =
                           ::std::numeric_limits<::int32_t>::min(),
                       $Msg_Enum_Msg_Enum_$INT_MAX_SENTINEL_DO_NOT_USE_ =
                           ::std::numeric_limits<::int32_t>::max(),
                     )cc");
           }},
      },
      R"cc(
        enum $Msg_Enum_annotated$ : int {
          $values$,
          $open_enum_sentinels$,
        };
      )cc");
}

void UnscopedEnumStrategy::GenerateSymbolImports(
    io::Printer* p, const EnumStrategyContext& ctx) const {
  auto v = p->WithVars(ctx.enum_vars);

  p->Emit({Sub("Enum_", p->LookupVar("Enum_")).AnnotatedAs(ctx.enum_)}, R"cc(
    using $Enum_$ = $Msg_Enum$;
  )cc");

  for (int j = 0; j < ctx.enum_->value_count(); ++j) {
    const auto* value = ctx.enum_->value(j);
    p->Emit(
        {
            Sub("VALUE", EnumValueName(ctx.enum_->value(j))).AnnotatedAs(value),
            {"DEPRECATED",
             value->options().deprecated() ? "[[deprecated]]" : ""},
        },
        R"cc(
          $DEPRECATED $static constexpr $Enum_$ $VALUE$ = $Msg_Enum$_$VALUE$;
        )cc");
  }

  p->Emit(
      {
          Sub("Enum_MIN", absl::StrCat(ctx.enum_->name(), "_MIN"))
              .AnnotatedAs(ctx.enum_),
          Sub("Enum_MAX", absl::StrCat(ctx.enum_->name(), "_MAX"))
              .AnnotatedAs(ctx.enum_),
      },
      R"cc(
        static inline bool $Enum$_IsValid(int value) {
          return $Msg_Enum$_IsValid(value);
        }
        static constexpr $Enum_$ $Enum_MIN$ = $Msg_Enum$_$Enum$_MIN;
        static constexpr $Enum_$ $Enum_MAX$ = $Msg_Enum$_$Enum$_MAX;
      )cc");

  if (ctx.generate_array_size) {
    p->Emit(
        {
            Sub("Enum_ARRAYSIZE", absl::StrCat(ctx.enum_->name(), "_ARRAYSIZE"))
                .AnnotatedAs(ctx.enum_),
        },
        R"cc(
          static constexpr int $Enum_ARRAYSIZE$ = $Msg_Enum$_$Enum$_ARRAYSIZE;
        )cc");
  }

  if (ctx.has_reflection) {
    p->Emit(R"(
      static inline const $pb$::EnumDescriptor* $nonnull$ $Enum$_descriptor() {
        return $Msg_Enum$_descriptor();
      }
    )");
  }

  p->Emit(R"cc(
    template <typename T>
    static inline $return_type$ $Enum$_Name(T value) {
      return $Msg_Enum$_Name(value);
    }
    static inline bool $Enum$_Parse(
        //~
        ::absl::string_view name, $Enum_$* $nonnull$ value) {
      return $Msg_Enum$_Parse(name, value);
    }
  )cc");
}

}  // namespace protobuf
}  // namespace google::compiler::cpp
