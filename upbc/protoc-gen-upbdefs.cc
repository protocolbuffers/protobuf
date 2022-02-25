// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <memory>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "upbc/common.h"

namespace upbc {
namespace {

namespace protoc = ::google::protobuf::compiler;
namespace protobuf = ::google::protobuf;

std::string DefInitSymbol(const protobuf::FileDescriptor* file) {
  return ToCIdent(file->name()) + "_upbdefinit";
}

static std::string DefHeaderFilename(std::string proto_filename) {
  return StripExtension(proto_filename) + ".upbdefs.h";
}

static std::string DefSourceFilename(std::string proto_filename) {
  return StripExtension(proto_filename) + ".upbdefs.c";
}

void GenerateMessageDefAccessor(const protobuf::Descriptor* d, Output& output) {
  output("UPB_INLINE const upb_MessageDef *$0_getmsgdef(upb_DefPool *s) {\n",
         ToCIdent(d->full_name()));
  output("  _upb_DefPool_LoadDefInit(s, &$0);\n", DefInitSymbol(d->file()));
  output("  return upb_DefPool_FindMessageByName(s, \"$0\");\n",
         d->full_name());
  output("}\n");
  output("\n");

  for (int i = 0; i < d->nested_type_count(); i++) {
    GenerateMessageDefAccessor(d->nested_type(i), output);
  }
}

void WriteDefHeader(const protobuf::FileDescriptor* file, Output& output) {
  EmitFileWarning(file, output);

  output(
      "#ifndef $0_UPBDEFS_H_\n"
      "#define $0_UPBDEFS_H_\n\n"
      "#include \"upb/def.h\"\n"
      "#include \"upb/port_def.inc\"\n"
      "#ifdef __cplusplus\n"
      "extern \"C\" {\n"
      "#endif\n\n",
      ToPreproc(file->name()));

  output("#include \"upb/def.h\"\n");
  output("\n");
  output("#include \"upb/port_def.inc\"\n");
  output("\n");

  output("extern _upb_DefPool_Init $0;\n", DefInitSymbol(file));
  output("\n");

  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateMessageDefAccessor(file->message_type(i), output);
  }

  output(
      "#ifdef __cplusplus\n"
      "}  /* extern \"C\" */\n"
      "#endif\n"
      "\n"
      "#include \"upb/port_undef.inc\"\n"
      "\n"
      "#endif  /* $0_UPBDEFS_H_ */\n",
      ToPreproc(file->name()));
}

void WriteDefSource(const protobuf::FileDescriptor* file, Output& output) {
  EmitFileWarning(file, output);

  output("#include \"upb/def.h\"\n");
  output("#include \"$0\"\n", DefHeaderFilename(file->name()));
  output("#include \"$0\"\n", HeaderFilename(file));
  output("\n");

  for (int i = 0; i < file->dependency_count(); i++) {
    output("extern _upb_DefPool_Init $0;\n",
           DefInitSymbol(file->dependency(i)));
  }

  protobuf::FileDescriptorProto file_proto;
  file->CopyTo(&file_proto);
  std::string file_data;
  file_proto.SerializeToString(&file_data);

  output("static const char descriptor[$0] = {", file_data.size());

  // C90 only guarantees that strings can be up to 509 characters, and some
  // implementations have limits here (for example, MSVC only allows 64k:
  // https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/fatal-error-c1091.
  // So we always emit an array instead of a string.
  for (size_t i = 0; i < file_data.size();) {
    for (size_t j = 0; j < 25 && i < file_data.size(); ++i, ++j) {
      output("'$0', ", absl::CEscape(file_data.substr(i, 1)));
    }
    output("\n");
  }
  output("};\n\n");

  output("static _upb_DefPool_Init *deps[$0] = {\n",
         file->dependency_count() + 1);
  for (int i = 0; i < file->dependency_count(); i++) {
    output("  &$0,\n", DefInitSymbol(file->dependency(i)));
  }
  output("  NULL\n");
  output("};\n");
  output("\n");

  output("_upb_DefPool_Init $0 = {\n", DefInitSymbol(file));
  output("  deps,\n");
  output("  &$0,\n", FileLayoutName(file));
  output("  \"$0\",\n", file->name());
  output("  UPB_STRINGVIEW_INIT(descriptor, $0)\n", file_data.size());
  output("};\n");
}

class Generator : public protoc::CodeGenerator {
  ~Generator() override {}
  bool Generate(const protobuf::FileDescriptor* file,
                const std::string& parameter, protoc::GeneratorContext* context,
                std::string* error) const override;
  uint64_t GetSupportedFeatures() const override {
    return FEATURE_PROTO3_OPTIONAL;
  }
};

bool Generator::Generate(const protobuf::FileDescriptor* file,
                         const std::string& parameter,
                         protoc::GeneratorContext* context,
                         std::string* error) const {
  std::vector<std::pair<std::string, std::string>> params;
  google::protobuf::compiler::ParseGeneratorParameter(parameter, &params);

  for (const auto& pair : params) {
    *error = "Unknown parameter: " + pair.first;
    return false;
  }

  std::unique_ptr<protobuf::io::ZeroCopyOutputStream> h_output_stream(
      context->Open(DefHeaderFilename(file->name())));
  Output h_def_output(h_output_stream.get());
  WriteDefHeader(file, h_def_output);

  std::unique_ptr<protobuf::io::ZeroCopyOutputStream> c_output_stream(
      context->Open(DefSourceFilename(file->name())));
  Output c_def_output(c_output_stream.get());
  WriteDefSource(file, c_def_output);

  return true;
}

}  // namespace
}  // namespace upbc

int main(int argc, char** argv) {
  std::unique_ptr<google::protobuf::compiler::CodeGenerator> generator(
      new upbc::Generator());
  return google::protobuf::compiler::PluginMain(argc, argv, generator.get());
}
