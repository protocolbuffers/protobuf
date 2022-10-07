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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/io/printer.h"

#include <stdlib.h>

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "absl/types/variant.h"

namespace google {
namespace protobuf {
namespace io {
namespace {
// Returns the number of spaces of the first non empty line.
size_t RawStringIndentLen(absl::string_view format) {
  // We are processing a call that looks like
  //
  // p->Emit(R"cc(
  //   class Foo {
  //     int x, y, z;
  //   };
  // )cc");
  //
  // or
  //
  // p->Emit(R"cc(
  //
  //   class Foo {
  //     int x, y, z;
  //   };
  // )cc");
  //
  // To compute the indent, we need to discard all leading newlines, then
  // count all spaces until we reach a non-space; this run of spaces is
  // stripped off at the start of each line.
  size_t len = 0;
  while (absl::ConsumePrefix(&format, "\n")) {
  }

  while (absl::ConsumePrefix(&format, " ")) {
    ++len;
  }

  return len;
}

// Returns the amount of additional indenting past `raw_string_indent_len`.
size_t ConsumeIndentForLine(size_t raw_string_indent_len,
                            absl::string_view& format) {
  size_t total_indent = 0;
  while (absl::ConsumePrefix(&format, " ")) {
    ++total_indent;
  }
  if (total_indent < raw_string_indent_len) {
    total_indent = 0;
  } else {
    total_indent -= raw_string_indent_len;
  }
  return total_indent;
}

template <typename T>
absl::optional<T> LookupInFrameStack(
    absl::string_view var,
    absl::Span<std::function<absl::optional<T>(absl::string_view)>> frames) {
  for (size_t i = frames.size(); i >= 1; --i) {
    auto val = frames[i - 1](var);
    if (val.has_value()) {
      return val;
    }
  }
  return absl::nullopt;
}
}  // namespace

constexpr absl::string_view Printer::kProtocCodegenTrace;

Printer::Printer(ZeroCopyOutputStream* output, Options options)
    : sink_(output), options_(options) {
  if (!options_.enable_codegen_trace.has_value()) {
    // Trace-by-default is threaded through via an env var, rather than a
    // global, so that child processes can pick it up as well. The flag
    // --enable_codegen_trace setenv()'s this in protoc's startup code.
    static const bool kEnableCodegenTrace =
        ::getenv(kProtocCodegenTrace.data()) != nullptr;
    options_.enable_codegen_trace = kEnableCodegenTrace;
  }
}

absl::string_view Printer::LookupVar(absl::string_view var) {
  LookupResult result = LookupInFrameStack(var, absl::MakeSpan(var_lookups_));
  GOOGLE_CHECK(result.has_value()) << "could not find " << var;
  auto* view = absl::get_if<absl::string_view>(&*result);
  GOOGLE_CHECK(view != nullptr) << "could not find " << var
                         << "; found callback instead";

  return *view;
}

bool Printer::Validate(bool cond, Printer::PrintOptions opts,
                       absl::FunctionRef<std::string()> message) {
  if (!cond) {
    if (opts.checks_are_debug_only) {
      GOOGLE_LOG(DFATAL) << message();
    } else {
      GOOGLE_LOG(FATAL) << message();
    }
  }
  return cond;
}

bool Printer::Validate(bool cond, Printer::PrintOptions opts,
                       absl::string_view message) {
  return Validate(cond, opts, [=] { return std::string(message); });
}

// This function is outlined to isolate the use of
// GOOGLE_CHECK into the .cc file.
void Printer::Outdent() {
  PrintOptions opts;
  opts.checks_are_debug_only = true;
  if (!Validate(indent_ >= options_.spaces_per_indent, opts,
                "Outdent() without matching Indent()")) {
    return;
  }
  indent_ -= options_.spaces_per_indent;
}

void Printer::Emit(
    std::initializer_list<
        VarDefinition<absl::string_view, /*allow_callbacks=*/true>>
        vars,
    absl::string_view format, SourceLocation loc) {
  PrintOptions opts;
  opts.strip_raw_string_indentation = true;
  opts.loc = loc;

  auto defs = WithDefs(vars);

  PrintImpl(format, {}, opts);
}

absl::optional<std::pair<size_t, size_t>> Printer::GetSubstitutionRange(
    absl::string_view varname, PrintOptions opts) {
  auto it = substitutions_.find(std::string(varname));
  if (!Validate(it != substitutions_.end(), opts, [varname] {
        return absl::StrCat("undefined variable in annotation: ", varname);
      })) {
    return absl::nullopt;
  }

  std::pair<size_t, size_t> range = it->second;
  if (!Validate(range.first <= range.second, opts, [varname] {
        return absl::StrCat(
            "variable used for annotation used multiple times: ", varname);
      })) {
    return absl::nullopt;
  }

  return range;
}

void Printer::Annotate(absl::string_view begin_varname,
                       absl::string_view end_varname,
                       absl::string_view file_path,
                       const std::vector<int>& path) {
  if (options_.annotation_collector == nullptr) {
    return;
  }

  PrintOptions opts;
  opts.checks_are_debug_only = true;
  auto begin = GetSubstitutionRange(begin_varname, opts);
  auto end = GetSubstitutionRange(end_varname, opts);
  if (!begin.has_value() || !end.has_value()) {
    return;
  }
  if (begin->first > end->second) {
    GOOGLE_LOG(DFATAL) << "annotation has negative length from " << begin_varname
                << " to " << end_varname;
    return;
  }
  options_.annotation_collector->AddAnnotation(begin->first, end->second,
                                               std::string(file_path), path);
}

void Printer::WriteRaw(const char* data, size_t size) {
  if (failed_ || size == 0) {
    return;
  }

  if (at_start_of_line_ && data[0] != '\n') {
    // Insert an indent.
    at_start_of_line_ = false;
    for (size_t i = 0; i < indent_; ++i) {
      sink_.Write(" ");
    }

    if (failed_) {
      return;
    }

    // Fix up empty variables (e.g., "{") that should be annotated as
    // coming after the indent.
    for (const std::string& var : line_start_variables_) {
      substitutions_[var].first += indent_;
      substitutions_[var].second += indent_;
    }
  }

  // If we're going to write any data, clear line_start_variables_, since
  // we've either updated them in the block above or they no longer refer to
  // the current line.
  line_start_variables_.clear();

  sink_.Append(data, size);
  failed_ |= sink_.failed();
}

void Printer::IndentIfAtStart() {
  if (!at_start_of_line_) {
    return;
  }

  for (size_t i = 0; i < indent_; ++i) {
    sink_.Write(" ");
  }
  at_start_of_line_ = false;
}

void Printer::PrintCodegenTrace(absl::optional<SourceLocation> loc) {
  if (!options_.enable_codegen_trace.value_or(false) || !loc.has_value()) {
    return;
  }

  if (!at_start_of_line_) {
    at_start_of_line_ = true;
    line_start_variables_.clear();
    sink_.Write("\n");
  }

  PrintRaw(absl::StrFormat("%s @%s:%d\n", options_.comment_start,
                           loc->file_name(), loc->line()));
  at_start_of_line_ = true;
}

bool Printer::ValidateIndexLookupInBounds(size_t index,
                                          size_t current_arg_index,
                                          size_t args_len, PrintOptions opts) {
  if (!Validate(index < args_len, opts, [this, index] {
        return absl::StrFormat("annotation %c{%d%c is out of bounds",
                               options_.variable_delimiter, index + 1,
                               options_.variable_delimiter);
      })) {
    return false;
  }
  if (!Validate(
          index <= current_arg_index, opts, [this, index, current_arg_index] {
            return absl::StrFormat(
                "annotation arg must be in correct order as given; expected "
                "%c{%d%c but got %c{%d%c",
                options_.variable_delimiter, current_arg_index + 1,
                options_.variable_delimiter, options_.variable_delimiter,
                index + 1, options_.variable_delimiter);
          })) {
    return false;
  }
  return true;
}

void Printer::PrintImpl(absl::string_view format,
                        absl::Span<const std::string> args, PrintOptions opts) {
  // Inside of this function, we set indentation as we print new lines from the
  // format string. No matter how we exit this function, we should fix up the
  // indent to what it was before we entered; a cleanup makes it easy to avoid
  // this mistake.
  size_t original_indent = indent_;
  auto unindent =
      absl::MakeCleanup([this, original_indent] { indent_ = original_indent; });

  absl::string_view original = format;

  line_start_variables_.clear();

  if (opts.use_substitution_map) {
    substitutions_.clear();
  }

  size_t raw_string_indent_len =
      opts.strip_raw_string_indentation ? RawStringIndentLen(format) : 0;

  if (opts.strip_raw_string_indentation) {
    // We only want to remove a single newline from the input string to allow
    // extra newlines at the start to go into the generated code.
    absl::ConsumePrefix(&format, "\n");
    while (absl::ConsumePrefix(&format, " ")) {
    }
  }

  PrintCodegenTrace(opts.loc);

  size_t arg_index = 0;
  std::vector<AnnotationCollector::Annotation> annot_stack;
  std::vector<std::pair<absl::string_view, size_t>> annot_records;
  while (!format.empty()) {
    // Skip to the next special character. We do this so that we can delay
    // printing "normal" text until we know what kind of variable substitution
    // we're doing, since that may require trimming whitespace.
    size_t next_special = 0;
    for (; next_special < format.size(); ++next_special) {
      if (format[next_special] == options_.variable_delimiter ||
          format[next_special] == '\n') {
        break;
      }
    }

    absl::string_view next_chunk = format.substr(0, next_special);
    format = format.substr(next_special);

    if (format.empty()) {
      PrintRaw(next_chunk);
      break;
    }

    char c = format.front();
    format = format.substr(1);
    if (c == '\n') {
      PrintRaw(next_chunk);
      at_start_of_line_ = true;
      line_start_variables_.clear();
      sink_.Write("\n");
      indent_ =
          original_indent + ConsumeIndentForLine(raw_string_indent_len, format);
      continue;
    } else if (c != options_.variable_delimiter) {
      PrintRaw(next_chunk);
      continue;
    }

    size_t end = format.find(options_.variable_delimiter);
    if (!Validate(end != absl::string_view::npos, opts, [format] {
          return absl::StrCat("unclosed variable name: \"",
                              absl::CHexEscape(format), "\"");
        })) {
      PrintRaw(next_chunk);
      WriteRaw(&options_.variable_delimiter, 1);
      PrintRaw(format);
      break;
    }

    absl::string_view match = format.substr(0, end);
    absl::string_view var = match;
    format = format.substr(end + 1);

    if (var.empty()) {
      // `$$` is an escape for just `$`.
      PrintRaw(next_chunk);
      WriteRaw(&options_.variable_delimiter, 1);
      continue;
    }

    if (opts.use_curly_brace_substitutions && absl::ConsumePrefix(&var, "{")) {
      PrintRaw(next_chunk);

      if (!Validate(var.size() == 1u, opts, "expected single-digit variable")) {
        continue;
      }

      if (!Validate(absl::ascii_isdigit(var[0]), opts,
                    "expected digit after {")) {
        continue;
      }

      size_t idx = var[0] - '1';
      if (!ValidateIndexLookupInBounds(idx, arg_index, args.size(), opts)) {
        continue;
      }

      if (idx == arg_index) {
        ++arg_index;
      }

      IndentIfAtStart();
      annot_stack.push_back({{sink_.bytes_written(), 0}, args[idx]});
      continue;
    } else if (opts.use_curly_brace_substitutions &&
               absl::ConsumePrefix(&var, "}")) {
      PrintRaw(next_chunk);

      // The rest of var is actually ignored, and this is apparently
      // public API now. Oops?
      if (!Validate(!annot_stack.empty(), opts,
                    "unexpected end of annotation")) {
        continue;
      }

      annot_stack.back().first.second = sink_.bytes_written();
      if (options_.annotation_collector != nullptr) {
        options_.annotation_collector->AddAnnotationNew(annot_stack.back());
      }
      IndentIfAtStart();
      annot_stack.pop_back();
      continue;
    }

    absl::string_view prefix, suffix;
    if (opts.strip_spaces_around_vars) {
      var = absl::StripLeadingAsciiWhitespace(var);
      prefix = match.substr(0, match.size() - var.size());
      var = absl::StripTrailingAsciiWhitespace(var);
      suffix = match.substr(prefix.size() + var.size());
    }

    if (!Validate(!var.empty(), opts, "unexpected empty variable")) {
      PrintRaw(next_chunk);
      continue;
    }

    LookupResult sub;
    absl::optional<AnnotationRecord> same_name_record;
    if (opts.allow_digit_substitions && absl::ascii_isdigit(var[0])) {
      PrintRaw(next_chunk);

      if (!Validate(var.size() == 1u, opts, "expected single-digit variable")) {
        continue;
      }

      size_t idx = var[0] - '1';
      if (!ValidateIndexLookupInBounds(idx, arg_index, args.size(), opts)) {
        continue;
      }
      if (idx == arg_index) {
        ++arg_index;
      }
      sub = args[idx];
    } else if (opts.use_annotation_frames &&
               (var == "_start" || var == "_end")) {
      bool is_start = var == "_start";

      size_t next_delim = format.find('$');
      if (!Validate(next_delim != absl::string_view::npos, opts,
                    "$_start$ must be followed by a name and another $")) {
        PrintRaw(next_chunk);
        continue;
      }

      auto var = format.substr(0, next_delim);
      format = format.substr(next_delim + 1);

      if (is_start) {
        PrintRaw(next_chunk);
        IndentIfAtStart();
        annot_records.push_back({var, sink_.bytes_written()});
        // Skip all whitespace immediately after a _start.
        while (!format.empty() && absl::ascii_isspace(format.front())) {
          format = format.substr(1);
        }
      } else {
        // Skip all whitespace immediately *before* an _end.
        while (!next_chunk.empty() && absl::ascii_isspace(next_chunk.back())) {
          next_chunk = next_chunk.substr(0, next_chunk.size() - 1);
        }
        PrintRaw(next_chunk);

        // If a line consisted *only* of an _end, this will likely result in
        // a blank line if we do not zap the newline after it, and any
        // indentation beyond that.
        if (at_start_of_line_) {
          absl::ConsumePrefix(&format, "\n");
          indent_ = original_indent +
                    ConsumeIndentForLine(raw_string_indent_len, format);
        }

        auto record_var = annot_records.back();
        annot_records.pop_back();

        if (!Validate(record_var.first == var, opts, [record_var, var] {
              return absl::StrFormat(
                  "_start and _end variables must match, but got %s and %s, "
                  "respectively",
                  record_var.first, var);
            })) {
          continue;
        }

        absl::optional<AnnotationRecord> record =
            LookupInFrameStack(var, absl::MakeSpan(annotation_lookups_));

        if (!Validate(record.has_value(), opts, [var] {
              return absl::StrCat("undefined variable: \"",
                                  absl::CHexEscape(var), "\"");
            })) {
          continue;
        }

        if (options_.annotation_collector != nullptr) {
          options_.annotation_collector->AddAnnotation(
              record_var.second, sink_.bytes_written(), record->file_path,
              record->path);
        }
      }

      continue;
    } else {
      PrintRaw(next_chunk);
      sub = LookupInFrameStack(var, absl::MakeSpan(var_lookups_));

      if (opts.use_annotation_frames) {
        same_name_record =
            LookupInFrameStack(var, absl::MakeSpan(annotation_lookups_));
      }
    }

    // By returning here in case of empty we also skip possible spaces inside
    // the $...$, i.e. "void$ dllexpor$ f();" -> "void f();" in the empty case.
    if (!Validate(sub.has_value(), opts, [var] {
          return absl::StrCat("undefined variable: \"", absl::CHexEscape(var),
                              "\"");
        })) {
      continue;
    }

    size_t range_start = sink_.bytes_written();
    size_t range_end = sink_.bytes_written();

    if (auto* str = absl::get_if<absl::string_view>(&*sub)) {
      if (at_start_of_line_ && str->empty()) {
        line_start_variables_.emplace_back(var);
      }

      if (!str->empty()) {
        // If `sub` is empty, we do not print the spaces around it.
        PrintRaw(prefix);
        PrintRaw(*str);
        range_end = sink_.bytes_written();
        range_start = range_end - str->size();
        PrintRaw(suffix);
      }
    } else {
      auto* fnc = absl::get_if<std::function<void()>>(&*sub);
      GOOGLE_CHECK(fnc != nullptr);

      Validate(
          prefix.empty() && suffix.empty(), opts,
          "substitution that resolves to callback cannot contain whitespace");

      range_start = sink_.bytes_written();
      (*fnc)();
      range_end = sink_.bytes_written();

      // If we just evaluated a closure, and we are at the start of a line, that
      // means it finished with a newline. If a newline follows immediately
      // after, we drop it. This helps callback formatting "work as expected"
      // with respect to forms like
      //
      //   class Foo {
      //     $methods$;
      //   };
      //
      // Without this line, this would turn into something like
      //
      //   class Foo {
      //     void Bar() {}
      //
      //   };
      //
      // in many cases. We *also* do this if a ; or , follows the substitution,
      // because this helps clang-format keep its head on in many cases.
      // Users that need to keep the semi can write $foo$/**/;
      if (!absl::ConsumePrefix(&format, ";")) {
        absl::ConsumePrefix(&format, ",");
      }
      absl::ConsumePrefix(&format, "\n");
      indent_ =
          original_indent + ConsumeIndentForLine(raw_string_indent_len, format);
    }

    if (same_name_record.has_value() &&
        options_.annotation_collector != nullptr) {
      options_.annotation_collector->AddAnnotation(range_start, range_end,
                                                   same_name_record->file_path,
                                                   same_name_record->path);
    }

    if (opts.use_substitution_map) {
      auto insertion = substitutions_.emplace(
          std::string(var), std::make_pair(range_start, range_end));

      if (!insertion.second) {
        // This variable was used multiple times.
        // Make its span have negative length so
        // we can detect it if it gets used in an
        // annotation.
        insertion.first->second = {1, 0};
      }
    }
  }

  Validate(arg_index == args.size(), opts,
           [original] { return absl::StrCat("unused args: ", original); });
  Validate(annot_stack.empty(), opts, [this, original] {
    return absl::StrFormat(
        "annotation range was not closed; expected %c}%c: %s",
        options_.variable_delimiter, options_.variable_delimiter, original);
  });
}
}  // namespace io
}  // namespace protobuf
}  // namespace google
