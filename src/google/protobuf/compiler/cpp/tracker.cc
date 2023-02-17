// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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

#include "google/protobuf/compiler/cpp/tracker.h"

#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using Sub = ::google::protobuf::io::Printer::Sub;

constexpr absl::string_view kTracker = "Impl_::_tracker_";
constexpr absl::string_view kVarPrefix = "annotate_";
constexpr absl::string_view kTypeTraits = "_proto_TypeTraits";

struct Call {
  Call(absl::string_view var, absl::string_view call) : var(var), call(call) {}
  Call(absl::optional<int> field_index, absl::string_view var,
       absl::string_view call)
      : var(var), call(call), field_index(field_index) {}

  Call This(absl::optional<absl::string_view> thiz) && {
    this->thiz = thiz;
    return std::move(*this);
  }

  template <typename... SubArgs>
  Call Arg(absl::string_view format, const SubArgs&... args) && {
    this->args.emplace_back(absl::Substitute(format, args...));
    return std::move(*this);
  }

  Call Suppressed() && {
    suppressed = true;
    return std::move(*this);
  }

  absl::string_view var;
  absl::string_view call;
  absl::optional<int> field_index;
  absl::optional<absl::string_view> thiz = "this";
  std::vector<std::string> args;
  bool suppressed = false;
};

std::vector<Sub> GenerateTrackerCalls(
    const Options& opts, const Descriptor* message,
    absl::optional<std::string> alt_annotation, absl::Span<const Call> calls) {
  bool enable_tracking = HasTracker(message, opts);
  const auto& forbidden =
      opts.field_listener_options.forbidden_field_listener_events;

  std::vector<Sub> subs;
  for (const auto& call : calls) {
    std::string call_str;
    if (enable_tracking && !call.suppressed && !forbidden.contains(call.var)) {
      absl::SubstituteAndAppend(&call_str, "$0.$1", kTracker, call.call);
      if (call.field_index.has_value()) {
        absl::SubstituteAndAppend(&call_str, "<$0>", *call.field_index);
      }
      absl::StrAppend(&call_str, "(");

      absl::string_view arg_sep = "";
      if (call.thiz.has_value()) {
        absl::StrAppend(&call_str, *call.thiz);
        arg_sep = ", ";
      }

      for (const auto& arg : call.args) {
        absl::StrAppend(&call_str, arg_sep, arg);
        arg_sep = ", ";
      }

      absl::StrAppend(&call_str, ");");
    } else if (opts.annotate_accessor && alt_annotation.has_value()) {
      call_str = *alt_annotation;
    }

    if (!call_str.empty()) {
      // TODO(b/245791219): Until we migrate all of the C++ backend to use
      // Emit(), we need to include a newline here so that the line that follows
      // the annotation is on its own line.
      call_str.push_back('\n');
    }

    subs.push_back(
        Sub(absl::StrCat(kVarPrefix, call.var), call_str).WithSuffix(";"));
  }

  return subs;
}
}  // namespace

std::vector<Sub> MakeTrackerCalls(const Descriptor* message,
                                  const Options& opts) {
  absl::string_view extns =
      IsMapEntryMessage(message) ? "_extensions_" : "_impl_._extensions_";

  auto primitive_extn_accessor = [extns](absl::string_view var,
                                         absl::string_view call) {
    return Call(var, call)
        .Arg("id.number()")
        .Arg("$0::GetPtr(id.number(), $1, id.default_value_ref())", kTypeTraits,
             extns);
  };

  auto index_extn_accessor = [extns](absl::string_view var,
                                     absl::string_view call) {
    return Call(var, call)
        .Arg("id.number()")
        .Arg("$0::GetPtr(id.number(), $1, index)", kTypeTraits, extns);
  };

  auto add_extn_accessor = [extns](absl::string_view var,
                                   absl::string_view call) {
    return Call(var, call)
        .Arg("id.number()")
        .Arg("$0::GetPtr(id.number(), $1, $1.ExtensionSize(id.number()) - 1)",
             kTypeTraits, extns);
  };

  auto list_extn_accessor = [extns](absl::string_view var,
                                    absl::string_view call) {
    return Call(var, call)
        .Arg("id.number()")
        .Arg("$0::GetRepeatedPtr(id.number(), $1)", kTypeTraits, extns);
  };

  return GenerateTrackerCalls(
      opts, message, absl::nullopt,
      {
          Call("serialize", "OnSerialize"),
          Call("deserialize", "OnDeserialize"),
          // TODO(danilak): Ideally annotate_reflection should not exist and we
          // need to annotate all reflective calls on our own, however, as this
          // is a cause for side effects, i.e. reading values dynamically, we
          // want the users know that dynamic access can happen.
          Call("reflection", "OnGetMetadata").This(absl::nullopt),
          Call("bytesize", "OnByteSize"),
          Call("mergefrom", "OnMergeFrom").This("_this").Arg("&from"),

          // "Has" is here as users calling "has" on a repeated field is a
          // mistake.
          primitive_extn_accessor("extension_has", "OnHasExtension"),
          primitive_extn_accessor("extension_get", "OnGetExtension"),
          primitive_extn_accessor("extension_mutable", "OnMutableExtension"),
          primitive_extn_accessor("extension_set", "OnSetExtension"),
          primitive_extn_accessor("extension_release", "OnReleaseExtension"),

          index_extn_accessor("repeated_extension_get", "OnGetExtension"),
          index_extn_accessor("repeated_extension_mutable",
                              "OnMutableExtension"),
          index_extn_accessor("repeated_extension_set", "OnSetExtension"),

          add_extn_accessor("repeated_extension_add", "OnAddExtension"),
          add_extn_accessor("repeated_extension_add_mutable",
                            "OnAddMutableExtension"),

          list_extn_accessor("extension_repeated_size", "OnExtensionSize"),
          list_extn_accessor("repeated_extension_list", "OnListExtension"),
          list_extn_accessor("repeated_extension_list_mutable",
                             "OnMutableListExtension"),

          // Generic accessors such as "clear".
          // TODO(b/190614678): Generalize clear from both repeated and non
          // repeated calls, currently their underlying memory interfaces are
          // very different. Or think of removing clear callback as no usages
          // are needed and no memory exist
          Call("extension_clear", "OnClearExtension").Suppressed(),
      });
}

namespace {
struct Getters {
  std::string base = "nullptr";
  std::string for_last = "nullptr";
  std::string for_flat = "nullptr";
};

Getters RepeatedFieldGetters(const FieldDescriptor* field,
                             const Options& opts) {
  std::string member = FieldMemberName(field, ShouldSplit(field, opts));

  Getters getters;
  if (!field->is_map() &&
      field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    getters.base = absl::Substitute("&$0.Get(index)", member);
    getters.for_last = absl::Substitute("&$0.Get($0.size() - 1)", member);
    getters.for_flat = absl::StrCat("&", member);
  }

  return getters;
}

Getters StringFieldGetters(const FieldDescriptor* field, const Options& opts) {
  std::string member = FieldMemberName(field, ShouldSplit(field, opts));
  bool is_std_string = field->options().ctype() == FieldOptions::STRING;

  Getters getters;
  if (is_std_string && !field->default_value_string().empty()) {
    getters.base =
        absl::Substitute("$0.IsDefault() ? &$1.get() : $0.UnsafeGetPointer()",
                         member, MakeDefaultFieldName(field));
  } else {
    getters.base = absl::StrCat("&", member);
  }

  getters.for_flat = getters.base;
  return getters;
}

Getters StringOneofGetters(const FieldDescriptor* field,
                           const OneofDescriptor* oneof, const Options& opts) {
  ABSL_CHECK(oneof != nullptr);

  std::string member = FieldMemberName(field, ShouldSplit(field, opts));
  bool is_std_string = field->options().ctype() == FieldOptions::STRING;

  std::string field_ptr = member;
  if (is_std_string) {
    field_ptr = absl::Substitute("$0.UnsafeGetPointer()", member);
  }

  std::string has =
      absl::Substitute("$0_case() == k$1", oneof->name(),
                       UnderscoresToCamelCase(field->name(), true));

  std::string default_field = MakeDefaultFieldName(field);
  if (is_std_string) {
    absl::StrAppend(&default_field, ".get()");
  }

  Getters getters;
  if (field->default_value_string().empty() ||
      field->options().ctype() == FieldOptions::STRING_PIECE) {
    getters.base = absl::Substitute("$0 ? $1 : nullptr", has, field_ptr);
  } else {
    getters.base =
        absl::Substitute("$0 ? $1 : &$2", has, field_ptr, default_field);
  }

  getters.for_flat = getters.base;
  return getters;
}

Getters SingularFieldGetters(const FieldDescriptor* field,
                             const Options& opts) {
  std::string member = FieldMemberName(field, ShouldSplit(field, opts));

  Getters getters;
  getters.base = absl::StrCat("&", member);
  if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    getters.for_flat = absl::StrCat("&", member);
  }
  return getters;
}
}  // namespace

std::vector<Sub> MakeTrackerCalls(const FieldDescriptor* field,
                                  const Options& opts) {
  Getters getters;
  if (field->is_repeated()) {
    getters = RepeatedFieldGetters(field, opts);
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
    const auto* oneof = field->real_containing_oneof();
    if (oneof != nullptr) {
      getters = StringOneofGetters(field, oneof, opts);
    } else {
      getters = StringFieldGetters(field, opts);
    }
  } else if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE ||
             IsExplicitLazy(field)) {
    getters = SingularFieldGetters(field, opts);
  }

  auto index = field->index();
  return GenerateTrackerCalls(
      opts, field->containing_type(),
      absl::Substitute("$0_AccessedNoStrip = true;", FieldName(field)),
      {
          Call(index, "get", "OnGet").Arg(getters.base),
          Call(index, "set", "OnSet").Arg(getters.base),
          Call(index, "has", "OnHas").Arg(getters.base),
          Call(index, "mutable", "OnMutable").Arg(getters.base),
          Call(index, "release", "OnRelease").Arg(getters.base),
          Call(index, "clear", "OnClear").Arg(getters.for_flat),
          Call(index, "size", "OnSize").Arg(getters.for_flat),
          Call(index, "list", "OnList").Arg(getters.for_flat),
          Call(index, "mutable_list", "OnMutableList").Arg(getters.for_flat),
          Call(index, "add", "OnAdd").Arg(getters.for_last),
          Call(index, "add_mutable", "OnAddMutable").Arg(getters.for_last),
      });
}
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
