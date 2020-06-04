
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/strings/str_replace.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/io/printer.h>

namespace protoc = ::google::protobuf::compiler;
namespace protobuf = ::google::protobuf;

class LuaGenerator : public protoc::CodeGenerator {
  bool Generate(const protobuf::FileDescriptor* file,
                const std::string& parameter, protoc::GeneratorContext* context,
                std::string* error) const override;

};

static std::string StripExtension(absl::string_view fname) {
  size_t lastdot = fname.find_last_of(".");
  if (lastdot == std::string::npos) {
    return std::string(fname);
  }
  return std::string(fname.substr(0, lastdot));
}

static std::string Filename(const protobuf::FileDescriptor* file) {
  return StripExtension(file->name()) + "_pb.lua";
}

static std::string ModuleName(const protobuf::FileDescriptor* file) {
  std::string ret = StripExtension(file->name()) + "_pb";
  return absl::StrReplaceAll(ret, {{"/", "."}});
}

static void PrintHexDigit(char digit, protobuf::io::Printer* printer) {
  char text;
  if (digit < 10) {
    text = '0' + digit;
  } else {
    text = 'A' + (digit - 10);
  }
  printer->WriteRaw(&text, 1);
}

static void PrintString(int max_cols, absl::string_view* str,
                        protobuf::io::Printer* printer) {
  printer->Print("\'");
  while (max_cols > 0 && !str->empty()) {
    char ch = (*str)[0];
    if (ch == '\\') {
      printer->PrintRaw("\\\\");
      max_cols--;
    } else if (ch == '\'') {
      printer->PrintRaw("\\'");
      max_cols--;
    } else if (isprint(ch)) {
      printer->WriteRaw(&ch, 1);
      max_cols--;
    } else {
      unsigned char byte = ch;
      printer->PrintRaw("\\x");
      PrintHexDigit(byte >> 4, printer);
      PrintHexDigit(byte & 15, printer);
      max_cols -= 4;
    }
    str->remove_prefix(1);
  }
  printer->Print("\'");
}

bool LuaGenerator::Generate(
    const protobuf::FileDescriptor* file,
    const std::string& /* parameter */,
    protoc::GeneratorContext* context,
    std::string* /* error */) const {
  std::string filename = Filename(file);
  protobuf::io::ZeroCopyOutputStream* out = context->Open(filename);
  protobuf::io::Printer printer(out, '$');

  for (int i = 0; i < file->dependency_count(); i++) {
    const protobuf::FileDescriptor* dep = file->dependency(i);
    printer.Print("require('$name$')\n", "name", ModuleName(dep));
  }

  printer.Print("local upb = require('upb')\n");

  protobuf::FileDescriptorProto file_proto;
  file->CopyTo(&file_proto);
  std::string file_data;
  file_proto.SerializeToString(&file_data);

  printer.Print("local descriptor = table.concat({\n");
  absl::string_view data(file_data);
  while (!data.empty()) {
    printer.Print("  ");
    PrintString(72, &data, &printer);
    printer.Print(",\n");
  }
  printer.Print("})\n");

  printer.Print("return upb._generated_module(descriptor)\n");

  return true;
}

int main(int argc, char** argv) {
  LuaGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
