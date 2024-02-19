// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/doc_comment.h"

#include <stddef.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "absl/strings/str_split.h"
#include "google/protobuf/compiler/java/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

std::string EscapeJavadoc(const std::string& input) {
  std::string result;
  result.reserve(input.size() * 2);

  char prev = '*';

  for (std::string::size_type i = 0; i < input.size(); i++) {
    char c = input[i];
    switch (c) {
      case '*':
        // Avoid "/*".
        if (prev == '/') {
          result.append("&#42;");
        } else {
          result.push_back(c);
        }
        break;
      case '/':
        // Avoid "*/".
        if (prev == '*') {
          result.append("&#47;");
        } else {
          result.push_back(c);
        }
        break;
      case '@':
        // '@' starts javadoc tags including the @deprecated tag, which will
        // cause a compile-time error if inserted before a declaration that
        // does not have a corresponding @Deprecated annotation.
        result.append("&#64;");
        break;
      case '<':
        // Avoid interpretation as HTML.
        result.append("&lt;");
        break;
      case '>':
        // Avoid interpretation as HTML.
        result.append("&gt;");
        break;
      case '&':
        // Avoid interpretation as HTML.
        result.append("&amp;");
        break;
      case '\\':
        // Java interprets Unicode escape sequences anywhere!
        result.append("&#92;");
        break;
      default:
        result.push_back(c);
        break;
    }

    prev = c;
  }

  return result;
}

static std::string EscapeKdoc(const std::string& input) {
  std::string result;
  result.reserve(input.size() * 2);

  char prev = 'a';

  for (char c : input) {
    switch (c) {
      case '*':
        // Avoid "/*".
        if (prev == '/') {
          result.append("&#42;");
        } else {
          result.push_back(c);
        }
        break;
      case '/':
        // Avoid "*/".
        if (prev == '*') {
          result.append("&#47;");
        } else {
          result.push_back(c);
        }
        break;
      default:
        result.push_back(c);
        break;
    }

    prev = c;
  }

  return result;
}

static void WriteDocCommentBodyForLocation(io::Printer* printer,
                                           const SourceLocation& location,
                                           const Options options,
                                           const bool kdoc) {
  if (options.strip_nonfunctional_codegen) {
    // TODO: Remove once prototiller can avoid making
    // extraneous formatting changes to comments.
    return;
  }

  std::string comments = location.leading_comments.empty()
                             ? location.trailing_comments
                             : location.leading_comments;
  if (!comments.empty()) {
    if (kdoc) {
      comments = EscapeKdoc(comments);
    } else {
      comments = EscapeJavadoc(comments);
    }

    std::vector<std::string> lines = absl::StrSplit(comments, "\n");
    while (!lines.empty() && lines.back().empty()) {
      lines.pop_back();
    }

    if (kdoc) {
      printer->Print(" * ```\n");
    } else {
      printer->Print(" * <pre>\n");
    }

    for (size_t i = 0; i < lines.size(); i++) {
      // Lines should start with a single space and any extraneous leading
      // spaces should be stripped. For lines starting with a /, the leading
      // space will prevent putting it right after the leading asterick from
      // closing the comment.
      std::string line = lines[i];
      line.erase(line.begin(),
                 std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                   return !std::isspace(ch);
                 }));
      if (!line.empty()) {
        printer->Print(" * $line$\n", "line", line);
      } else {
        printer->Print(" *\n");
      }
    }

    if (kdoc) {
      printer->Print(" * ```\n");
    } else {
      printer->Print(" * </pre>\n");
    }
    printer->Print(" *\n");
  }
}

template <typename DescriptorType>
static void WriteDocCommentBody(io::Printer* printer,
                                const DescriptorType* descriptor,
                                const Options options, const bool kdoc) {
  SourceLocation location;
  if (descriptor->GetSourceLocation(&location)) {
    WriteDocCommentBodyForLocation(printer, location, options, kdoc);
  }
}

static std::string FirstLineOf(const std::string& value) {
  std::string result = value;

  std::string::size_type pos = result.find_first_of('\n');
  if (pos != std::string::npos) {
    result.erase(pos);
  }

  // If line ends in an opening brace, make it "{ ... }" so it looks nice.
  if (!result.empty() && result[result.size() - 1] == '{') {
    result.append(" ... }");
  }

  return result;
}

static void WriteDebugString(io::Printer* printer, const FieldDescriptor* field,
                             const Options options, const bool kdoc) {
  std::string field_comment = FirstLineOf(field->DebugString());
  if (options.strip_nonfunctional_codegen) {
    field_comment = field->name();
  }
  if (kdoc) {
    printer->Print(" * `$def$`\n", "def", EscapeKdoc(field_comment));
  } else {
    printer->Print(" * <code>$def$</code>\n", "def",
                   EscapeJavadoc(field_comment));
  }
}

void WriteMessageDocComment(io::Printer* printer, const Descriptor* message,
                            const Options options, const bool kdoc) {
  printer->Print("/**\n");
  WriteDocCommentBody(printer, message, options, kdoc);
  if (kdoc) {
    printer->Print(
        " * Protobuf type `$fullname$`\n"
        " */\n",
        "fullname", EscapeKdoc(message->full_name()));
  } else {
    printer->Print(
        " * Protobuf type {@code $fullname$}\n"
        " */\n",
        "fullname", EscapeJavadoc(message->full_name()));
  }
}

void WriteFieldDocComment(io::Printer* printer, const FieldDescriptor* field,
                          const Options options, const bool kdoc) {
  // We start the comment with the main body based on the comments from the
  // .proto file (if present). We then continue with the field declaration,
  // e.g.:
  //   optional string foo = 5;
  // And then we end with the javadoc tags if applicable.
  // If the field is a group, the debug string might end with {.
  printer->Print("/**\n");
  WriteDocCommentBody(printer, field, options, kdoc);
  WriteDebugString(printer, field, options, kdoc);
  printer->Print(" */\n");
}

void WriteDeprecatedJavadoc(io::Printer* printer, const FieldDescriptor* field,
                            const FieldAccessorType type,
                            const Options options) {
  if (!field->options().deprecated()) {
    return;
  }

  // Lite codegen does not annotate set & clear methods with @Deprecated.
  if (field->file()->options().optimize_for() == FileOptions::LITE_RUNTIME &&
      (type == SETTER || type == CLEARER)) {
    return;
  }

  std::string startLine = "0";
  SourceLocation location;
  if (field->GetSourceLocation(&location)) {
    startLine = std::to_string(location.start_line);
  }

  printer->Print(" * @deprecated $name$ is deprecated.\n", "name",
                 field->full_name());
  if (!options.strip_nonfunctional_codegen) {
    printer->Print(" *     See $file$;l=$line$\n", "file",
                   field->file()->name(), "line", startLine);
  }
}

void WriteFieldAccessorDocComment(io::Printer* printer,
                                  const FieldDescriptor* field,
                                  const FieldAccessorType type,
                                  const Options options, const bool builder,
                                  const bool kdoc) {
  printer->Print("/**\n");
  WriteDocCommentBody(printer, field, options, kdoc);
  WriteDebugString(printer, field, options, kdoc);
  if (!kdoc) WriteDeprecatedJavadoc(printer, field, type, options);
  switch (type) {
    case HAZZER:
      printer->Print(" * @return Whether the $name$ field is set.\n", "name",
                     field->camelcase_name());
      break;
    case GETTER:
      printer->Print(" * @return The $name$.\n", "name",
                     field->camelcase_name());
      break;
    case SETTER:
      printer->Print(" * @param value The $name$ to set.\n", "name",
                     field->camelcase_name());
      break;
    case CLEARER:
      // Print nothing
      break;
    // Repeated
    case LIST_COUNT:
      printer->Print(" * @return The count of $name$.\n", "name",
                     field->camelcase_name());
      break;
    case LIST_GETTER:
      printer->Print(" * @return A list containing the $name$.\n", "name",
                     field->camelcase_name());
      break;
    case LIST_INDEXED_GETTER:
      printer->Print(" * @param index The index of the element to return.\n");
      printer->Print(" * @return The $name$ at the given index.\n", "name",
                     field->camelcase_name());
      break;
    case LIST_INDEXED_SETTER:
      printer->Print(" * @param index The index to set the value at.\n");
      printer->Print(" * @param value The $name$ to set.\n", "name",
                     field->camelcase_name());
      break;
    case LIST_ADDER:
      printer->Print(" * @param value The $name$ to add.\n", "name",
                     field->camelcase_name());
      break;
    case LIST_MULTI_ADDER:
      printer->Print(" * @param values The $name$ to add.\n", "name",
                     field->camelcase_name());
      break;
  }
  if (builder) {
    printer->Print(" * @return This builder for chaining.\n");
  }
  printer->Print(" */\n");
}

void WriteFieldEnumValueAccessorDocComment(io::Printer* printer,
                                           const FieldDescriptor* field,
                                           const FieldAccessorType type,
                                           const Options options,
                                           const bool builder,
                                           const bool kdoc) {
  printer->Print("/**\n");
  WriteDocCommentBody(printer, field, options, kdoc);
  WriteDebugString(printer, field, options, kdoc);
  if (!kdoc) WriteDeprecatedJavadoc(printer, field, type, options);
  switch (type) {
    case HAZZER:
      // Should never happen
      break;
    case GETTER:
      printer->Print(
          " * @return The enum numeric value on the wire for $name$.\n", "name",
          field->camelcase_name());
      break;
    case SETTER:
      printer->Print(
          " * @param value The enum numeric value on the wire for $name$ to "
          "set.\n",
          "name", field->camelcase_name());
      break;
    case CLEARER:
      // Print nothing
      break;
    // Repeated
    case LIST_COUNT:
      // Should never happen
      break;
    case LIST_GETTER:
      printer->Print(
          " * @return A list containing the enum numeric values on the wire "
          "for $name$.\n",
          "name", field->camelcase_name());
      break;
    case LIST_INDEXED_GETTER:
      printer->Print(" * @param index The index of the value to return.\n");
      printer->Print(
          " * @return The enum numeric value on the wire of $name$ at the "
          "given index.\n",
          "name", field->camelcase_name());
      break;
    case LIST_INDEXED_SETTER:
      printer->Print(" * @param index The index to set the value at.\n");
      printer->Print(
          " * @param value The enum numeric value on the wire for $name$ to "
          "set.\n",
          "name", field->camelcase_name());
      break;
    case LIST_ADDER:
      printer->Print(
          " * @param value The enum numeric value on the wire for $name$ to "
          "add.\n",
          "name", field->camelcase_name());
      break;
    case LIST_MULTI_ADDER:
      printer->Print(
          " * @param values The enum numeric values on the wire for $name$ to "
          "add.\n",
          "name", field->camelcase_name());
      break;
  }
  if (builder) {
    printer->Print(" * @return This builder for chaining.\n");
  }
  printer->Print(" */\n");
}

void WriteFieldStringBytesAccessorDocComment(io::Printer* printer,
                                             const FieldDescriptor* field,
                                             const FieldAccessorType type,
                                             const Options options,
                                             const bool builder,
                                             const bool kdoc) {
  printer->Print("/**\n");
  WriteDocCommentBody(printer, field, options, kdoc);
  WriteDebugString(printer, field, options, kdoc);
  if (!kdoc) WriteDeprecatedJavadoc(printer, field, type, options);
  switch (type) {
    case HAZZER:
      // Should never happen
      break;
    case GETTER:
      printer->Print(" * @return The bytes for $name$.\n", "name",
                     field->camelcase_name());
      break;
    case SETTER:
      printer->Print(" * @param value The bytes for $name$ to set.\n", "name",
                     field->camelcase_name());
      break;
    case CLEARER:
      // Print nothing
      break;
    // Repeated
    case LIST_COUNT:
      // Should never happen
      break;
    case LIST_GETTER:
      printer->Print(" * @return A list containing the bytes for $name$.\n",
                     "name", field->camelcase_name());
      break;
    case LIST_INDEXED_GETTER:
      printer->Print(" * @param index The index of the value to return.\n");
      printer->Print(" * @return The bytes of the $name$ at the given index.\n",
                     "name", field->camelcase_name());
      break;
    case LIST_INDEXED_SETTER:
      printer->Print(" * @param index The index to set the value at.\n");
      printer->Print(" * @param value The bytes of the $name$ to set.\n",
                     "name", field->camelcase_name());
      break;
    case LIST_ADDER:
      printer->Print(" * @param value The bytes of the $name$ to add.\n",
                     "name", field->camelcase_name());
      break;
    case LIST_MULTI_ADDER:
      printer->Print(" * @param values The bytes of the $name$ to add.\n",
                     "name", field->camelcase_name());
      break;
  }
  if (builder) {
    printer->Print(" * @return This builder for chaining.\n");
  }
  printer->Print(" */\n");
}

// Enum

void WriteEnumDocComment(io::Printer* printer, const EnumDescriptor* enum_,
                         const Options options, const bool kdoc) {
  printer->Print("/**\n");
  WriteDocCommentBody(printer, enum_, options, kdoc);
  if (kdoc) {
    printer->Print(
        " * Protobuf enum `$fullname$`\n"
        " */\n",
        "fullname", EscapeKdoc(enum_->full_name()));
  } else {
    printer->Print(
        " * Protobuf enum {@code $fullname$}\n"
        " */\n",
        "fullname", EscapeJavadoc(enum_->full_name()));
  }
}

void WriteEnumValueDocComment(io::Printer* printer,
                              const EnumValueDescriptor* value,
                              const Options options) {
  printer->Print("/**\n");
  WriteDocCommentBody(printer, value, options, /* kdoc */ false);

  printer->Print(
      " * <code>$def$</code>\n"
      " */\n",
      "def", EscapeJavadoc(FirstLineOf(value->DebugString())));
}

void WriteServiceDocComment(io::Printer* printer,
                            const ServiceDescriptor* service,
                            const Options options) {
  printer->Print("/**\n");
  WriteDocCommentBody(printer, service, options, /* kdoc */ false);
  printer->Print(
      " * Protobuf service {@code $fullname$}\n"
      " */\n",
      "fullname", EscapeJavadoc(service->full_name()));
}

void WriteMethodDocComment(io::Printer* printer, const MethodDescriptor* method,
                           const Options options) {
  printer->Print("/**\n");
  WriteDocCommentBody(printer, method, options, /* kdoc */ false);
  printer->Print(
      " * <code>$def$</code>\n"
      " */\n",
      "def", EscapeJavadoc(FirstLineOf(method->DebugString())));
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
