#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def_pool.h"
#include "upb/reflection/enum_def.h"
#include "upb/reflection/field_def.h"
#include "upb/reflection/file_def.h"
#include "upb/reflection/message_def.h"
#include "upb/reflection/method_def.h"
#include "upb/reflection/service_def.h"
#include "upb/util/def_to_proto.h"
#include "upb/wire/encode.h"

#ifdef GOOGLE3
#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/compiler/perl/plugin.upb.h"
#else
#include "google/protobuf/compiler/plugin.upb.h"
#include "google/protobuf/descriptor.upb.h"

// Alias proto2_ symbols to google_protobuf_ for local builds using vendored upb
#define proto2_DescriptorProto google_protobuf_DescriptorProto
#define proto2_DescriptorProto_enum_type \
  google_protobuf_DescriptorProto_enum_type
#define proto2_DescriptorProto_field google_protobuf_DescriptorProto_field
#define proto2_DescriptorProto_name google_protobuf_DescriptorProto_name
#define proto2_DescriptorProto_nested_type \
  google_protobuf_DescriptorProto_nested_type
#define proto2_EnumDescriptorProto google_protobuf_EnumDescriptorProto
#define proto2_EnumDescriptorProto_name google_protobuf_EnumDescriptorProto_name
#define proto2_EnumDescriptorProto_value \
  google_protobuf_EnumDescriptorProto_value
#define proto2_EnumValueDescriptorProto google_protobuf_EnumValueDescriptorProto
#define proto2_EnumValueDescriptorProto_name \
  google_protobuf_EnumValueDescriptorProto_name
#define proto2_EnumValueDescriptorProto_number \
  google_protobuf_EnumValueDescriptorProto_number
#define proto2_FieldDescriptorProto google_protobuf_FieldDescriptorProto
#define proto2_FieldDescriptorProto_name \
  google_protobuf_FieldDescriptorProto_name
#define proto2_FieldDescriptorProto_type \
  google_protobuf_FieldDescriptorProto_type
#define proto2_FieldDescriptorProto_type_name \
  google_protobuf_FieldDescriptorProto_type_name
#define proto2_FileDescriptorProto google_protobuf_FileDescriptorProto
#define proto2_FileDescriptorProto_dependency \
  google_protobuf_FileDescriptorProto_dependency
#define proto2_FileDescriptorProto_enum_type \
  google_protobuf_FileDescriptorProto_enum_type
#define proto2_FileDescriptorProto_message_type \
  google_protobuf_FileDescriptorProto_message_type
#define proto2_FileDescriptorProto_name google_protobuf_FileDescriptorProto_name
#define proto2_FileDescriptorProto_package \
  google_protobuf_FileDescriptorProto_package
#define proto2_FileDescriptorProto_serialize \
  google_protobuf_FileDescriptorProto_serialize
#define proto2_FileDescriptorProto_service \
  google_protobuf_FileDescriptorProto_service
#define proto2_MethodDescriptorProto google_protobuf_MethodDescriptorProto
#define proto2_MethodDescriptorProto_input_type \
  google_protobuf_MethodDescriptorProto_input_type
#define proto2_MethodDescriptorProto_name \
  google_protobuf_MethodDescriptorProto_name
#define proto2_MethodDescriptorProto_output_type \
  google_protobuf_MethodDescriptorProto_output_type
#define proto2_ServiceDescriptorProto google_protobuf_ServiceDescriptorProto
#define proto2_ServiceDescriptorProto_method \
  google_protobuf_ServiceDescriptorProto_method
#define proto2_ServiceDescriptorProto_name \
  google_protobuf_ServiceDescriptorProto_name
#define proto2_compiler_CodeGeneratorRequest \
  google_protobuf_compiler_CodeGeneratorRequest
#define proto2_compiler_CodeGeneratorRequest_file_to_generate \
  google_protobuf_compiler_CodeGeneratorRequest_file_to_generate
#define proto2_compiler_CodeGeneratorRequest_parse \
  google_protobuf_compiler_CodeGeneratorRequest_parse
#define proto2_compiler_CodeGeneratorRequest_proto_file \
  google_protobuf_compiler_CodeGeneratorRequest_proto_file
#define proto2_compiler_CodeGeneratorResponse \
  google_protobuf_compiler_CodeGeneratorResponse
#define proto2_compiler_CodeGeneratorResponse_FEATURE_PROTO3_OPTIONAL \
  google_protobuf_compiler_CodeGeneratorResponse_FEATURE_PROTO3_OPTIONAL
#define proto2_compiler_CodeGeneratorResponse_FEATURE_SUPPORTS_EDITIONS \
  google_protobuf_compiler_CodeGeneratorResponse_FEATURE_SUPPORTS_EDITIONS
#define proto2_compiler_CodeGeneratorResponse_File \
  google_protobuf_compiler_CodeGeneratorResponse_File
#define proto2_compiler_CodeGeneratorResponse_File_set_content \
  google_protobuf_compiler_CodeGeneratorResponse_File_set_content
#define proto2_compiler_CodeGeneratorResponse_File_set_name \
  google_protobuf_compiler_CodeGeneratorResponse_File_set_name
#define proto2_compiler_CodeGeneratorResponse_add_file \
  google_protobuf_compiler_CodeGeneratorResponse_add_file
#define proto2_compiler_CodeGeneratorResponse_new \
  google_protobuf_compiler_CodeGeneratorResponse_new
#define proto2_compiler_CodeGeneratorResponse_serialize \
  google_protobuf_compiler_CodeGeneratorResponse_serialize
#define proto2_compiler_CodeGeneratorResponse_set_maximum_edition \
  google_protobuf_compiler_CodeGeneratorResponse_set_maximum_edition
#define proto2_compiler_CodeGeneratorResponse_set_minimum_edition \
  google_protobuf_compiler_CodeGeneratorResponse_set_minimum_edition
#define proto2_compiler_CodeGeneratorResponse_set_supported_features \
  google_protobuf_compiler_CodeGeneratorResponse_set_supported_features
#endif

// -- Proto Utils --

// Converts a string to CamelCase, with custom mappings for known words.
std::string ToCamelCase(absl::string_view s) {
  static constexpr std::pair<absl::string_view, absl::string_view>
      kCustomMappings[] = {{"bigquery", "BigQuery"}, {"pubsub", "PubSub"}};

  std::string res;
  for (absl::string_view segment : absl::StrSplit(s, '_')) {
    if (segment.empty()) continue;

    bool found_custom = false;
    for (const auto& [key, val] : kCustomMappings) {
      if (segment == key) {
        absl::StrAppend(&res, val);
        found_custom = true;
        break;
      }
    }
    if (!found_custom) {
      res += (char)toupper(segment[0]);
      absl::StrAppend(&res, segment.substr(1));
    }
  }
  return res;
}

// Converts a CamelCase string to snake_case.
std::string ToSnakeCase(absl::string_view s) {
  std::string res;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isupper(c)) {
      if (i > 0 && s[i - 1] != '_') {
        res += '_';
      }
      res += (char)tolower(c);
    } else {
      res += c;
    }
  }
  return res;
}

// Extracts the default host for standard Google Cloud APIs based on the package
// prefix.
std::string GetDefaultHost(absl::string_view proto_pkg) {
  absl::string_view sub = proto_pkg;
  if (absl::ConsumePrefix(&sub, "google.cloud.") ||
      absl::ConsumePrefix(&sub, "google.")) {
    size_t end = sub.find('.');
    if (end != absl::string_view::npos) {
      return absl::StrCat(sub.substr(0, end), ".googleapis.com:443");
    }
  }
  return "localhost:443";
}

// Capitalizes each segment of a package name (e.g. foo.bar -> Foo.Bar), with
// custom mappings.
std::string CapitalizePackage(absl::string_view s) {
  static constexpr std::pair<absl::string_view, absl::string_view>
      kCustomMappings[] = {{"bigquery", "BigQuery"}, {"pubsub", "PubSub"}};

  std::string res;
  bool first = true;
  for (absl::string_view segment : absl::StrSplit(s, '.')) {
    if (!first) absl::StrAppend(&res, "::");
    if (!segment.empty()) {
      bool found_custom = false;
      for (const auto& [key, val] : kCustomMappings) {
        if (segment == key) {
          absl::StrAppend(&res, val);
          found_custom = true;
          break;
        }
      }
      if (!found_custom) {
        res += (char)toupper(segment[0]);
        absl::StrAppend(&res, segment.substr(1));
      }
    }
    first = false;
  }
  return res;
}

// -- Generator Class --

class PerlCodeGenerator {
 public:
  PerlCodeGenerator(const proto2_compiler_CodeGeneratorRequest* request,
                    upb_Arena* arena)
      : request_(request), arena_(arena) {
    BuildPackageMap();
    BuildTypeMap();
  }

  proto2_compiler_CodeGeneratorResponse* Generate();

 private:
  void BuildPackageMap();
  void BuildTypeMap();
  void RegisterTypesRecursively(const proto2_DescriptorProto* msg_proto,
                                absl::string_view proto_prefix,
                                absl::string_view perl_prefix);
  std::string ResolvePerlClass(absl::string_view proto_type);
  std::string GetPerlPackage(const proto2_FileDescriptorProto* file_proto);

  std::string GenerateModule(const proto2_FileDescriptorProto* file_proto);
  std::string GenerateTypes(const proto2_FileDescriptorProto* file_proto);

  void PrintTypesRecursively(const proto2_DescriptorProto* msg_proto,
                             absl::string_view current_package,
                             std::stringstream& ss);
  void PrintMessageTypes(const proto2_DescriptorProto* msg_proto,
                         absl::string_view current_package,
                         std::stringstream& ss);
  void PrintEnumTypes(const proto2_EnumDescriptorProto* enum_proto,
                      absl::string_view current_package, std::stringstream& ss);

  upb_StringView ArenaCopy(absl::string_view s) {
    char* buf = (char*)upb_Arena_Malloc(arena_, s.length());
    memcpy(buf, s.data(), s.length());
    return upb_StringView_FromDataAndSize(buf, s.length());
  }

  const proto2_compiler_CodeGeneratorRequest* request_;
  upb_Arena* arena_;
  absl::flat_hash_map<std::string, std::string> file_to_package_;
  absl::flat_hash_map<std::string, std::string> type_to_perl_class_;
  std::string package_name_;
};

void PerlCodeGenerator::BuildPackageMap() {
  size_t file_count;
  const proto2_FileDescriptorProto* const* files =
      proto2_compiler_CodeGeneratorRequest_proto_file(request_, &file_count);

  for (size_t i = 0; i < file_count; i++) {
    upb_StringView name_sv = proto2_FileDescriptorProto_name(files[i]);
    absl::string_view name(name_sv.data, name_sv.size);
    file_to_package_[std::string(name)] = GetPerlPackage(files[i]);
  }
}

void PerlCodeGenerator::RegisterTypesRecursively(
    const proto2_DescriptorProto* msg_proto, absl::string_view proto_prefix,
    absl::string_view perl_prefix) {
  upb_StringView name_sv = proto2_DescriptorProto_name(msg_proto);
  absl::string_view name(name_sv.data, name_sv.size);

  std::string full_proto = absl::StrCat(proto_prefix, ".", name);
  std::string full_perl = absl::StrCat(perl_prefix, "::", name);

  type_to_perl_class_[full_proto] = full_perl;

  size_t nested_count;
  const proto2_DescriptorProto* const* nested =
      proto2_DescriptorProto_nested_type(msg_proto, &nested_count);
  for (size_t i = 0; i < nested_count; i++) {
    RegisterTypesRecursively(nested[i], full_proto, full_perl);
  }
}

void PerlCodeGenerator::BuildTypeMap() {
  size_t file_count;
  const proto2_FileDescriptorProto* const* files =
      proto2_compiler_CodeGeneratorRequest_proto_file(request_, &file_count);

  for (size_t i = 0; i < file_count; i++) {
    upb_StringView name_sv = proto2_FileDescriptorProto_name(files[i]);
    absl::string_view name(name_sv.data, name_sv.size);

    std::string perl_pkg;
    auto it = file_to_package_.find(name);
    if (it != file_to_package_.end()) {
      perl_pkg = it->second;
    }

    upb_StringView pkg_sv = proto2_FileDescriptorProto_package(files[i]);
    std::string proto_pkg =
        absl::StrCat(".", absl::string_view(pkg_sv.data, pkg_sv.size));

    size_t msg_count;
    const proto2_DescriptorProto* const* messages =
        proto2_FileDescriptorProto_message_type(files[i], &msg_count);
    for (size_t j = 0; j < msg_count; j++) {
      RegisterTypesRecursively(messages[j], proto_pkg, perl_pkg);
    }
  }
}

std::string PerlCodeGenerator::ResolvePerlClass(absl::string_view proto_type) {
  auto it = type_to_perl_class_.find(proto_type);
  if (it != type_to_perl_class_.end()) {
    return it->second;
  }
  absl::string_view s = proto_type;
  absl::ConsumePrefix(&s, ".");
  return CapitalizePackage(s);
}

std::string PerlCodeGenerator::GetPerlPackage(
    const proto2_FileDescriptorProto* file_proto) {
  upb_StringView name_sv = proto2_FileDescriptorProto_name(file_proto);
  absl::string_view name(name_sv.data, name_sv.size);

  upb_StringView pkg_sv = proto2_FileDescriptorProto_package(file_proto);
  absl::string_view pkg(pkg_sv.data, pkg_sv.size);

  absl::string_view file_segment = name;
  size_t last_slash = file_segment.find_last_of('/');
  if (last_slash != absl::string_view::npos) {
    file_segment = file_segment.substr(last_slash + 1);
  }
  size_t dot = file_segment.find_last_of('.');
  if (dot != absl::string_view::npos) {
    file_segment = file_segment.substr(0, dot);
  }
  std::string camel_file_segment = ToCamelCase(file_segment);

  std::string base_package = CapitalizePackage(pkg);
  if (!base_package.empty()) {
    return absl::StrCat(base_package, "::", camel_file_segment);
  } else {
    return camel_file_segment;
  }
}

proto2_compiler_CodeGeneratorResponse* PerlCodeGenerator::Generate() {
  proto2_compiler_CodeGeneratorResponse* response =
      proto2_compiler_CodeGeneratorResponse_new(arena_);
  proto2_compiler_CodeGeneratorResponse_set_supported_features(
      response,
      proto2_compiler_CodeGeneratorResponse_FEATURE_PROTO3_OPTIONAL |
          proto2_compiler_CodeGeneratorResponse_FEATURE_SUPPORTS_EDITIONS);
  proto2_compiler_CodeGeneratorResponse_set_minimum_edition(response, 1000);
  proto2_compiler_CodeGeneratorResponse_set_maximum_edition(response, 1001);

  size_t gen_file_count;
  const upb_StringView* gen_files =
      proto2_compiler_CodeGeneratorRequest_file_to_generate(request_,
                                                            &gen_file_count);

  size_t all_file_count;
  const proto2_FileDescriptorProto* const* all_files =
      proto2_compiler_CodeGeneratorRequest_proto_file(request_,
                                                      &all_file_count);

  for (size_t i = 0; i < gen_file_count; i++) {
    absl::string_view name(gen_files[i].data, gen_files[i].size);

    const proto2_FileDescriptorProto* file_proto = nullptr;
    for (size_t j = 0; j < all_file_count; j++) {
      upb_StringView cur_name_sv =
          proto2_FileDescriptorProto_name(all_files[j]);
      if (name == absl::string_view(cur_name_sv.data, cur_name_sv.size)) {
        file_proto = all_files[j];
        break;
      }
    }
    if (!file_proto) continue;

    auto it = file_to_package_.find(name);
    if (it != file_to_package_.end()) {
      package_name_ = it->second;
    }

    // 1. Generate Main Module (.pm)
    std::string module_content = GenerateModule(file_proto);
    proto2_compiler_CodeGeneratorResponse_File* file =
        proto2_compiler_CodeGeneratorResponse_add_file(response, arena_);

    std::string p = package_name_;
    std::replace(p.begin(), p.end(), ':', '/');
    size_t double_slash;
    while ((double_slash = p.find("//")) != std::string::npos) {
      p.replace(double_slash, 2, "/");
    }

    std::string final_path = absl::StrCat(p, ".pm");
    proto2_compiler_CodeGeneratorResponse_File_set_name(file,
                                                        ArenaCopy(final_path));
    proto2_compiler_CodeGeneratorResponse_File_set_content(
        file, ArenaCopy(module_content));

    // 2. Generate Types Module (::Types)
    std::string types_content = GenerateTypes(file_proto);
    proto2_compiler_CodeGeneratorResponse_File* types_file =
        proto2_compiler_CodeGeneratorResponse_add_file(response, arena_);
    std::string types_path = absl::StrCat(p, "/Types.pm");
    proto2_compiler_CodeGeneratorResponse_File_set_name(types_file,
                                                        ArenaCopy(types_path));
    proto2_compiler_CodeGeneratorResponse_File_set_content(
        types_file, ArenaCopy(types_content));
  }

  return response;
}

std::string PerlCodeGenerator::GenerateModule(
    const proto2_FileDescriptorProto* file_proto) {
  std::stringstream ss;
  ss << "package " << package_name_ << ";\n\n";
  ss << "use strict;\n";
  ss << "use warnings;\n\n";
  ss << "our $VERSION = '0.01';\n\n";

  ss << "use Protobuf::Message;\n";
  ss << "use Protobuf::DescriptorPool;\n";
  ss << "use Protobuf::Internal qw(:all);\n";
  ss << "use Const::Fast;\n";
  ss << "use MIME::Base64;\n\n";

  ss << "BEGIN {\n";

  // Add requires for dependencies INSIDE BEGIN block
  size_t dep_count;
  const upb_StringView* deps =
      proto2_FileDescriptorProto_dependency(file_proto, &dep_count);
  for (size_t i = 0; i < dep_count; i++) {
    absl::string_view dep(deps[i].data, deps[i].size);
    auto it = file_to_package_.find(dep);
    if (it != file_to_package_.end()) {
      ss << "    eval { require " << it->second << " };\n";
    }
  }

  ss << "    my $descriptor_b64 = <<'EOF';\n";

  size_t size;
  char* buf = proto2_FileDescriptorProto_serialize(file_proto, arena_, &size);

  static const char b64_table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (size_t i = 0; i < size; i += 3) {
    uint32_t val = (uint8_t)buf[i] << 16;
    if (i + 1 < size) val |= (uint8_t)buf[i + 1] << 8;
    if (i + 2 < size) val |= (uint8_t)buf[i + 2];

    ss << b64_table[(val >> 18) & 0x3F];
    ss << b64_table[(val >> 12) & 0x3F];
    ss << (i + 1 < size ? b64_table[(val >> 6) & 0x3F] : '=');
    ss << (i + 2 < size ? b64_table[val & 0x3F] : '=');

    if ((i / 3 + 1) % 18 == 0) ss << "\n";
  }
  ss << "\nEOF\n";
  ss << "    "
        "Protobuf::DescriptorPool->generated_pool->add_serialized_file(MIME::"
        "Base64::decode_base64($descriptor_b64));\n";
  ss << "}\n\n";

  ss << "# Message definitions\n\n";

  size_t message_count;
  const proto2_DescriptorProto* const* messages =
      proto2_FileDescriptorProto_message_type(file_proto, &message_count);

  for (size_t i = 0; i < message_count; i++) {
    const proto2_DescriptorProto* msg = messages[i];
    upb_StringView msg_name_sv = proto2_DescriptorProto_name(msg);
    absl::string_view msg_name(msg_name_sv.data, msg_name_sv.size);

    ss << "# === Message: " << package_name_ << "::" << msg_name << " ===\n";
    ss << "    # Fields for " << msg_name << "\n";

    size_t field_count;
    const proto2_FieldDescriptorProto* const* fields =
        proto2_DescriptorProto_field(msg, &field_count);

    for (size_t j = 0; j < field_count; j++) {
      const proto2_FieldDescriptorProto* f = fields[j];
      upb_StringView f_name_sv = proto2_FieldDescriptorProto_name(f);
      upb_StringView f_type_name_sv = proto2_FieldDescriptorProto_type_name(f);
      int f_type = proto2_FieldDescriptorProto_type(f);

      ss << "    # Field: "
         << absl::string_view(f_name_sv.data, f_name_sv.size);
      ss << " Type: " << f_type << " ("
         << absl::string_view(f_type_name_sv.data, f_type_name_sv.size)
         << ")\n";
    }
    ss << "\n";

    size_t enum_count;
    const proto2_EnumDescriptorProto* const* enums =
        proto2_DescriptorProto_enum_type(msg, &enum_count);
    for (size_t j = 0; j < enum_count; j++) {
      const proto2_EnumDescriptorProto* e = enums[j];
      upb_StringView e_name_sv = proto2_EnumDescriptorProto_name(e);
      absl::string_view e_name(e_name_sv.data, e_name_sv.size);

      ss << "# Enum: " << msg_name << "::" << e_name << "\n";
      size_t val_count;
      const proto2_EnumValueDescriptorProto* const* values =
          proto2_EnumDescriptorProto_value(e, &val_count);
      for (size_t k = 0; k < val_count; k++) {
        const proto2_EnumValueDescriptorProto* ev = values[k];
        upb_StringView ev_name_sv = proto2_EnumValueDescriptorProto_name(ev);
        int ev_num = proto2_EnumValueDescriptorProto_number(ev);
        ss << "const my $" << msg_name << "_"
           << absl::string_view(ev_name_sv.data, ev_name_sv.size) << " => "
           << ev_num << ";\n";
      }
      ss << "\n";
    }
  }

  // Service definitions
  size_t service_count;
  const proto2_ServiceDescriptorProto* const* services =
      proto2_FileDescriptorProto_service(file_proto, &service_count);

  upb_StringView pkg_sv = proto2_FileDescriptorProto_package(file_proto);
  absl::string_view proto_pkg(pkg_sv.data, pkg_sv.size);
  std::string default_host = GetDefaultHost(proto_pkg);

  for (size_t i = 0; i < service_count; i++) {
    const proto2_ServiceDescriptorProto* srv = services[i];
    upb_StringView srv_name_sv = proto2_ServiceDescriptorProto_name(srv);
    absl::string_view srv_name(srv_name_sv.data, srv_name_sv.size);

    ss << "# === Service Client: " << package_name_ << "::" << srv_name
       << "Client ===\n";
    ss << "package " << package_name_ << "::" << srv_name << "Client;\n\n";

    ss << "=pod\n\n";
    ss << "=head1 NAME\n\n";
    ss << package_name_ << "::" << srv_name
       << "Client - Client stub representing the remote " << srv_name
       << " service\n\n";
    ss << "=head1 DESCRIPTION\n\n";
    ss << "This class acts as a local client stub for the remote gRPC "
          "service.\n";
    ss << "It delegates call dispatching to an underlying "
          "L<Google::gRPC::Client>\n";
    ss << "instance, ensuring type-safe request parsing and response "
          "mapping.\n\n";
    ss << "=head1 CONFIGURATION AND ENVIRONMENT\n\n";
    ss << "=head2 target\n\n";
    ss << "The endpoint target address. Defaults to C<" << default_host
       << ">.\n\n";
    ss << "=head2 credentials\n\n";
    ss << "The authentication credentials provider. Defaults to application "
          "default credentials via L<Google::Auth>.\n\n";
    ss << "=cut\n\n";

    ss << "use Moo;\n";
    ss << "use Google::Auth;\n";
    ss << "use Google::gRPC::Client;\n\n";

    ss << "has credentials => ( is => 'ro', default => sub { "
          "Google::Auth->default() } );\n";
    ss << "has target      => ( is => 'ro', default => '" << default_host
       << "' );\n\n";

    ss << "has _grpc_client => (\n";
    ss << "    is => 'ro',\n";
    ss << "    lazy => 1,\n";
    ss << "    builder => sub {\n";
    ss << "        my $self = shift;\n";
    ss << "        return Google::gRPC::Client->new(\n";
    ss << "            target     => $self->target,\n";
    ss << "            auth_token => $self->credentials->get_token(),\n";
    ss << "        );\n";
    ss << "    }\n";
    ss << ");\n\n";

    size_t method_count;
    const proto2_MethodDescriptorProto* const* methods =
        proto2_ServiceDescriptorProto_method(srv, &method_count);
    for (size_t j = 0; j < method_count; j++) {
      const proto2_MethodDescriptorProto* m = methods[j];
      upb_StringView m_name_sv = proto2_MethodDescriptorProto_name(m);
      absl::string_view m_name(m_name_sv.data, m_name_sv.size);

      upb_StringView in_type_sv = proto2_MethodDescriptorProto_input_type(m);
      absl::string_view in_type(in_type_sv.data, in_type_sv.size);
      std::string req_class = ResolvePerlClass(in_type);

      upb_StringView out_type_sv = proto2_MethodDescriptorProto_output_type(m);
      absl::string_view out_type(out_type_sv.data, out_type_sv.size);
      std::string res_class = ResolvePerlClass(out_type);

      std::string snake_method = ToSnakeCase(m_name);

      ss << "sub " << snake_method << " {\n";
      ss << "    my ($self, $args) = @_;\n";
      ss << "    my $req = ref($args) eq 'HASH'\n";
      ss << "        ? " << req_class << "->new($args)\n";
      ss << "        : $args;\n";
      ss << "    return $self->_grpc_client->call({\n";
      ss << "        service        => '" << proto_pkg << "." << srv_name
         << "',\n";
      ss << "        method         => '" << m_name << "',\n";
      ss << "        request        => $req,\n";
      ss << "        response_class => '" << res_class << "',\n";
      ss << "    });\n";
      ss << "}\n\n";
    }
  }

  ss << "1;\n";
  return ss.str();
}

std::string PerlCodeGenerator::GenerateTypes(
    const proto2_FileDescriptorProto* file_proto) {
  std::stringstream ss;
  ss << "package " << package_name_ << "::Types;\n\n";
  ss << "use strict;\n";
  ss << "use warnings;\n\n";
  ss << "use Type::Library -base;\n";
  ss << "use Type::Utils -all;\n";
  ss << "use Types::Standard -types;\n\n";

  size_t enum_count;
  const proto2_EnumDescriptorProto* const* enums =
      proto2_FileDescriptorProto_enum_type(file_proto, &enum_count);
  for (size_t j = 0; j < enum_count; ++j) {
    PrintEnumTypes(enums[j], package_name_, ss);
  }

  size_t message_count;
  const proto2_DescriptorProto* const* messages =
      proto2_FileDescriptorProto_message_type(file_proto, &message_count);
  for (size_t j = 0; j < message_count; ++j) {
    PrintTypesRecursively(messages[j], package_name_, ss);
  }

  ss << "1;\n";
  return ss.str();
}

void PerlCodeGenerator::PrintTypesRecursively(
    const proto2_DescriptorProto* msg_proto, absl::string_view current_package,
    std::stringstream& ss) {
  upb_StringView msg_name_sv = proto2_DescriptorProto_name(msg_proto);
  absl::string_view msg_name(msg_name_sv.data, msg_name_sv.size);
  std::string full_msg_name = absl::StrCat(current_package, "::", msg_name);

  PrintMessageTypes(msg_proto, current_package, ss);

  size_t nested_enum_count;
  const proto2_EnumDescriptorProto* const* nested_enums =
      proto2_DescriptorProto_enum_type(msg_proto, &nested_enum_count);
  for (size_t k = 0; k < nested_enum_count; ++k) {
    PrintEnumTypes(nested_enums[k], full_msg_name, ss);
  }

  size_t nested_message_count;
  const proto2_DescriptorProto* const* nested_messages =
      proto2_DescriptorProto_nested_type(msg_proto, &nested_message_count);
  for (size_t k = 0; k < nested_message_count; ++k) {
    PrintTypesRecursively(nested_messages[k], full_msg_name, ss);
  }
}

void PerlCodeGenerator::PrintMessageTypes(
    const proto2_DescriptorProto* msg_proto, absl::string_view current_package,
    std::stringstream& ss) {
  upb_StringView msg_name_sv = proto2_DescriptorProto_name(msg_proto);
  absl::string_view msg_name(msg_name_sv.data, msg_name_sv.size);
  std::string full_perl_class = absl::StrCat(current_package, "::", msg_name);

  ss << "declare '" << msg_name << "',\n";
  ss << "    as InstanceOf['" << full_perl_class << "'];\n\n";

  ss << "coerce '" << msg_name << "',\n";
  ss << "    from HashRef, via { '" << full_perl_class << "'->new($_) };\n\n";

  // Add Repeated and Map variants
  ss << "declare 'Repeated" << msg_name << "',\n";
  ss << "    as ArrayRef[" << msg_name << "()];\n\n";

  ss << "coerce 'Repeated" << msg_name << "',\n";
  ss << "    from ArrayRef[HashRef], via { [ map { '" << full_perl_class
     << "'->new($_) } @$_ ] };\n\n";

  ss << "declare 'MapString" << msg_name << "',\n";
  ss << "    as HashRef[" << msg_name << "()];\n\n";
}

void PerlCodeGenerator::PrintEnumTypes(
    const proto2_EnumDescriptorProto* enum_proto,
    absl::string_view current_package, std::stringstream& ss) {
  upb_StringView enum_name_sv = proto2_EnumDescriptorProto_name(enum_proto);
  absl::string_view enum_name(enum_name_sv.data, enum_name_sv.size);

  ss << "declare '" << enum_name << "',\n";
  ss << "    as (Int | Str);\n\n";
}

// -- main --

int main(int argc, char** argv) {
  upb_Arena* arena = upb_Arena_New();

  std::string input;
  char buffer[4096];
  while (std::cin.read(buffer, sizeof(buffer))) {
    input.append(buffer, std::cin.gcount());
  }
  input.append(buffer, std::cin.gcount());

  proto2_compiler_CodeGeneratorRequest* request =
      proto2_compiler_CodeGeneratorRequest_parse(input.data(), input.length(),
                                                 arena);

  if (!request) {
    std::cerr << "Failed to parse CodeGeneratorRequest\n";
    upb_Arena_Free(arena);
    return 1;
  }

  PerlCodeGenerator generator(request, arena);
  proto2_compiler_CodeGeneratorResponse* response = generator.Generate();

  size_t size;
  char* buf =
      proto2_compiler_CodeGeneratorResponse_serialize(response, arena, &size);
  std::cout.write(buf, size);

  upb_Arena_Free(arena);
  return 0;
}
