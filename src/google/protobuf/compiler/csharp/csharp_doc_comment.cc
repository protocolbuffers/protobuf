// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
#include "google/protobuf/compiler/csharp/csharp_doc_comment.h"

#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

// Functions to create C# XML documentation comments.
// Currently this only includes documentation comments containing text specified as comments
// in the .proto file; documentation comments generated just from field/message/enum/proto names
// is inlined in the relevant code. If more control is required, that code can be moved here.

void WriteDocCommentBodyImpl(io::Printer* printer, SourceLocation location) {
    std::string comments = location.leading_comments.empty() ?
        location.trailing_comments : location.leading_comments;
    if (comments.empty()) {
        return;
    }
    // XML escaping... no need for apostrophes etc as the whole text is going to be a child
    // node of a summary element, not part of an attribute.
    comments = absl::StrReplaceAll(comments, {{"&", "&amp;"}, {"<", "&lt;"}});
    std::vector<std::string> lines;
    lines = absl::StrSplit(comments, "\n", absl::AllowEmpty());
    // TODO: We really should work out which part to put in the summary and which to put in the remarks...
    // but that needs to be part of a bigger effort to understand the markdown better anyway.
    printer->Print("/// <summary>\n");
    bool last_was_empty = false;
    // We squash multiple blank lines down to one, and remove any trailing blank lines. We need
    // to preserve the blank lines themselves, as this is relevant in the markdown.
    // Note that we can't remove leading or trailing whitespace as *that's* relevant in markdown too.
    // (We don't skip "just whitespace" lines, either.)
    for (std::vector<std::string>::iterator it = lines.begin();
         it != lines.end(); ++it) {
      std::string line = *it;
      if (line.empty()) {
        last_was_empty = true;
      } else {
        if (last_was_empty) {
          printer->Print("///\n");
        }
        last_was_empty = false;
        printer->Print("///$line$\n", "line", *it);
      }
    }
    printer->Print("/// </summary>\n");
}

template <typename DescriptorType>
static void WriteDocCommentBody(
    io::Printer* printer, const DescriptorType* descriptor) {
    SourceLocation location;
    if (descriptor->GetSourceLocation(&location)) {
        WriteDocCommentBodyImpl(printer, location);
    }
}

void WriteMessageDocComment(io::Printer* printer, const Descriptor* message) {
    WriteDocCommentBody(printer, message);
}

void WritePropertyDocComment(io::Printer* printer, const FieldDescriptor* field) {
    WriteDocCommentBody(printer, field);
}

void WriteEnumDocComment(io::Printer* printer, const EnumDescriptor* enumDescriptor) {
    WriteDocCommentBody(printer, enumDescriptor);
}
void WriteEnumValueDocComment(io::Printer* printer, const EnumValueDescriptor* value) {
    WriteDocCommentBody(printer, value);
}

void WriteMethodDocComment(io::Printer* printer, const MethodDescriptor* method) {
    WriteDocCommentBody(printer, method);
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
