// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/file_def.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/file.h"
#include "upb/reflection/def.h"
#include "upb/reflection/descriptor_bootstrap.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/internal/def_pool.h"
#include "upb/reflection/internal/enum_def.h"
#include "upb/reflection/internal/field_def.h"
#include "upb/reflection/internal/message_def.h"
#include "upb/reflection/internal/service_def.h"
#include "upb/reflection/internal/strdup2.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_FileDef {
  const google_protobuf_FileOptions* opts;
  const google_protobuf_FeatureSet* resolved_features;
  const char* name;
  const char* package;
  google_protobuf_Edition edition;

  const upb_FileDef** deps;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  const upb_MessageDef* top_lvl_msgs;
  const upb_EnumDef* top_lvl_enums;
  const upb_FieldDef* top_lvl_exts;
  const upb_ServiceDef* services;
  const upb_MiniTableExtension** ext_layouts;
  const upb_DefPool* symtab;

  int dep_count;
  int public_dep_count;
  int weak_dep_count;
  int top_lvl_msg_count;
  int top_lvl_enum_count;
  int top_lvl_ext_count;
  int service_count;
  int ext_count;  // All exts in the file.
};

UPB_API const char* upb_FileDef_EditionName(int edition) {
  // TODO Synchronize this with descriptor.proto better.
  switch (edition) {
    case google_protobuf_EDITION_PROTO2:
      return "PROTO2";
    case google_protobuf_EDITION_PROTO3:
      return "PROTO3";
    case google_protobuf_EDITION_2023:
      return "2023";
    default:
      return "UNKNOWN";
  }
}

const google_protobuf_FileOptions* upb_FileDef_Options(const upb_FileDef* f) {
  return f->opts;
}

const google_protobuf_FeatureSet* upb_FileDef_ResolvedFeatures(const upb_FileDef* f) {
  return f->resolved_features;
}

bool upb_FileDef_HasOptions(const upb_FileDef* f) {
  return f->opts != (void*)kUpbDefOptDefault;
}

const char* upb_FileDef_Name(const upb_FileDef* f) { return f->name; }

const char* upb_FileDef_Package(const upb_FileDef* f) {
  return f->package ? f->package : "";
}

google_protobuf_Edition upb_FileDef_Edition(const upb_FileDef* f) { return f->edition; }

const char* _upb_FileDef_RawPackage(const upb_FileDef* f) { return f->package; }

int upb_FileDef_TopLevelMessageCount(const upb_FileDef* f) {
  return f->top_lvl_msg_count;
}

int upb_FileDef_DependencyCount(const upb_FileDef* f) { return f->dep_count; }

int upb_FileDef_PublicDependencyCount(const upb_FileDef* f) {
  return f->public_dep_count;
}

int upb_FileDef_WeakDependencyCount(const upb_FileDef* f) {
  return f->weak_dep_count;
}

const int32_t* _upb_FileDef_PublicDependencyIndexes(const upb_FileDef* f) {
  return f->public_deps;
}

const int32_t* _upb_FileDef_WeakDependencyIndexes(const upb_FileDef* f) {
  return f->weak_deps;
}

bool _upb_FileDef_ClosedEnumCheckingDisabled(const upb_FileDef* f) {
  return upb_DefPool_ClosedEnumCheckingDisabled(f->symtab);
}

bool _upb_FileDef_ImplicitFieldPresenceDisabled(const upb_FileDef* f) {
  return upb_DefPool_ImplicitFieldPresenceDisabled(f->symtab);
}

int upb_FileDef_TopLevelEnumCount(const upb_FileDef* f) {
  return f->top_lvl_enum_count;
}

int upb_FileDef_TopLevelExtensionCount(const upb_FileDef* f) {
  return f->top_lvl_ext_count;
}

int upb_FileDef_ServiceCount(const upb_FileDef* f) { return f->service_count; }

const upb_FileDef* upb_FileDef_Dependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->dep_count);
  return f->deps[i];
}

const upb_FileDef* upb_FileDef_PublicDependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->public_dep_count);
  return f->deps[f->public_deps[i]];
}

const upb_FileDef* upb_FileDef_WeakDependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->public_dep_count);
  return f->deps[f->weak_deps[i]];
}

const upb_MessageDef* upb_FileDef_TopLevelMessage(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_msg_count);
  return _upb_MessageDef_At(f->top_lvl_msgs, i);
}

const upb_EnumDef* upb_FileDef_TopLevelEnum(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_enum_count);
  return _upb_EnumDef_At(f->top_lvl_enums, i);
}

const upb_FieldDef* upb_FileDef_TopLevelExtension(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_ext_count);
  return _upb_FieldDef_At(f->top_lvl_exts, i);
}

const upb_ServiceDef* upb_FileDef_Service(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->service_count);
  return _upb_ServiceDef_At(f->services, i);
}

const upb_DefPool* upb_FileDef_Pool(const upb_FileDef* f) { return f->symtab; }

const upb_MiniTableExtension* _upb_FileDef_ExtensionMiniTable(
    const upb_FileDef* f, int i) {
  return f->ext_layouts[i];
}

// Note: Import cycles are not allowed so this will terminate.
bool upb_FileDef_Resolves(const upb_FileDef* f, const char* path) {
  if (!strcmp(f->name, path)) return true;

  for (int i = 0; i < upb_FileDef_PublicDependencyCount(f); i++) {
    const upb_FileDef* dep = upb_FileDef_PublicDependency(f, i);
    if (upb_FileDef_Resolves(dep, path)) return true;
  }
  return false;
}

static char* _strviewdup(upb_DefBuilder* ctx, upb_StringView view) {
  char* ret = upb_strdup2(view.data, view.size, _upb_DefBuilder_Arena(ctx));
  if (!ret) _upb_DefBuilder_OomErr(ctx);
  return ret;
}

static bool streql_view(upb_StringView view, const char* b) {
  return view.size == strlen(b) && memcmp(view.data, b, view.size) == 0;
}

static int count_exts_in_msg(const google_protobuf_DescriptorProto* msg_proto) {
  size_t n;
  google_protobuf_DescriptorProto_extension(msg_proto, &n);
  int ext_count = n;

  const google_protobuf_DescriptorProto* const* nested_msgs =
      google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (size_t i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(nested_msgs[i]);
  }

  return ext_count;
}

const google_protobuf_FeatureSet* _upb_FileDef_FindEdition(upb_DefBuilder* ctx,
                                                  int edition) {
  const google_protobuf_FeatureSetDefaults* defaults =
      upb_DefPool_FeatureSetDefaults(ctx->symtab);

  int min = google_protobuf_FeatureSetDefaults_minimum_edition(defaults);
  int max = google_protobuf_FeatureSetDefaults_maximum_edition(defaults);
  if (edition < min) {
    _upb_DefBuilder_Errf(ctx,
                         "Edition %s is earlier than the minimum edition %s "
                         "given in the defaults",
                         upb_FileDef_EditionName(edition),
                         upb_FileDef_EditionName(min));
    return NULL;
  }
  if (edition > max && edition != google_protobuf_EDITION_UNSTABLE) {
    _upb_DefBuilder_Errf(ctx,
                         "Edition %s is later than the maximum edition %s "
                         "given in the defaults",
                         upb_FileDef_EditionName(edition),
                         upb_FileDef_EditionName(max));
    return NULL;
  }

  size_t n;
  const google_protobuf_FeatureSetDefaults_FeatureSetEditionDefault* const* d =
      google_protobuf_FeatureSetDefaults_defaults(defaults, &n);
  const google_protobuf_FeatureSetDefaults_FeatureSetEditionDefault* result = NULL;
  for (size_t i = 0; i < n; i++) {
    if (google_protobuf_FeatureSetDefaults_FeatureSetEditionDefault_edition(d[i]) >
        edition) {
      break;
    }
    result = d[i];
  }
  if (result == NULL) {
    _upb_DefBuilder_Errf(ctx, "No valid default found for edition %s",
                         upb_FileDef_EditionName(edition));
    return NULL;
  }

  // Merge the fixed and overridable features to get the edition's default
  // feature set.
  const google_protobuf_FeatureSet* fixed =
      google_protobuf_FeatureSetDefaults_FeatureSetEditionDefault_fixed_features(result);
  const google_protobuf_FeatureSet* overridable =
      google_protobuf_FeatureSetDefaults_FeatureSetEditionDefault_overridable_features(
          result);
  if (!fixed && !overridable) {
    _upb_DefBuilder_Errf(ctx, "No valid default found for edition %s",
                         upb_FileDef_EditionName(edition));
    return NULL;
  } else if (!fixed) {
    return overridable;
  }
  return _upb_DefBuilder_DoResolveFeatures(ctx, fixed, overridable,
                                           /*is_implicit=*/true);
}

// Allocate and initialize one file def, and add it to the context object.
void _upb_FileDef_Create(upb_DefBuilder* ctx,
                         const google_protobuf_FileDescriptorProto* file_proto) {
  upb_FileDef* file = _upb_DefBuilder_Alloc(ctx, sizeof(upb_FileDef));
  ctx->file = file;

  const google_protobuf_DescriptorProto* const* msgs;
  const google_protobuf_EnumDescriptorProto* const* enums;
  const google_protobuf_FieldDescriptorProto* const* exts;
  const google_protobuf_ServiceDescriptorProto* const* services;
  const upb_StringView* strs;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  size_t n;

  file->symtab = ctx->symtab;

  // Count all extensions in the file, to build a flat array of layouts.
  google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  int ext_count = n;
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (size_t i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(msgs[i]);
  }
  file->ext_count = ext_count;

  if (ctx->layout) {
    // We are using the ext layouts that were passed in.
    file->ext_layouts = ctx->layout->UPB_PRIVATE(exts);
    const int mt_ext_count = upb_MiniTableFile_ExtensionCount(ctx->layout);
    if (mt_ext_count != file->ext_count) {
      _upb_DefBuilder_Errf(ctx,
                           "Extension count did not match layout (%d vs %d)",
                           mt_ext_count, file->ext_count);
    }
  } else {
    // We are building ext layouts from scratch.
    file->ext_layouts = _upb_DefBuilder_Alloc(
        ctx, sizeof(*file->ext_layouts) * file->ext_count);
    upb_MiniTableExtension* ext =
        UPB_DEFBUILDER_ALLOCARRAY(ctx, upb_MiniTableExtension, file->ext_count);
    for (int i = 0; i < file->ext_count; i++) {
      file->ext_layouts[i] = &ext[i];
    }
  }

  upb_StringView name = google_protobuf_FileDescriptorProto_name(file_proto);
  file->name = _strviewdup(ctx, name);
  if (strlen(file->name) != name.size) {
    _upb_DefBuilder_Errf(ctx, "File name contained embedded NULL");
  }

  upb_StringView package = google_protobuf_FileDescriptorProto_package(file_proto);

  if (package.size) {
    _upb_DefBuilder_CheckIdentFull(ctx, package);
    file->package = _strviewdup(ctx, package);
  } else {
    file->package = NULL;
  }

  upb_StringView syntax = google_protobuf_FileDescriptorProto_syntax(file_proto);

  if (google_protobuf_FileDescriptorProto_has_edition(file_proto)) {
    if (!streql_view(syntax, "editions")) {
      _upb_DefBuilder_Errf(ctx,
                           "Setting edition requires that syntax=\"editions\", "
                           "but syntax is \"" UPB_STRINGVIEW_FORMAT "\"",
                           UPB_STRINGVIEW_ARGS(syntax));
    }
    file->edition = google_protobuf_FileDescriptorProto_edition(file_proto);
  } else if (google_protobuf_FileDescriptorProto_has_syntax(file_proto)) {
    if (streql_view(syntax, "proto2")) {
      file->edition = google_protobuf_EDITION_PROTO2;
    } else if (streql_view(syntax, "proto3")) {
      file->edition = google_protobuf_EDITION_PROTO3;
    } else if (streql_view(syntax, "editions")) {
      _upb_DefBuilder_Errf(
          ctx, "File has syntax=\"editions\", but no edition is specified");
    } else {
      _upb_DefBuilder_Errf(ctx, "Invalid syntax '" UPB_STRINGVIEW_FORMAT "'",
                           UPB_STRINGVIEW_ARGS(syntax));
    }
  } else {
    // The legacy default when no edition or syntax is specified is proto2.
    file->edition = google_protobuf_EDITION_PROTO2;
  }

  // Read options.
  UPB_DEF_SET_OPTIONS(file->opts, FileDescriptorProto, FileOptions, file_proto);

  // Resolve features.
  const google_protobuf_FeatureSet* edition_defaults =
      _upb_FileDef_FindEdition(ctx, file->edition);
  const google_protobuf_FeatureSet* unresolved = google_protobuf_FileOptions_features(file->opts);
  file->resolved_features =
      _upb_DefBuilder_ResolveFeatures(ctx, edition_defaults, unresolved);

  // Verify dependencies.
  strs = google_protobuf_FileDescriptorProto_dependency(file_proto, &n);
  file->dep_count = n;
  file->deps = UPB_DEFBUILDER_ALLOCARRAY(ctx, const upb_FileDef*, n);

  for (size_t i = 0; i < n; i++) {
    upb_StringView str = strs[i];
    file->deps[i] =
        upb_DefPool_FindFileByNameWithSize(ctx->symtab, str.data, str.size);
    if (!file->deps[i]) {
      _upb_DefBuilder_Errf(ctx,
                           "Depends on file '" UPB_STRINGVIEW_FORMAT
                           "', but it has not been loaded",
                           UPB_STRINGVIEW_ARGS(str));
    }
  }

  public_deps = google_protobuf_FileDescriptorProto_public_dependency(file_proto, &n);
  file->public_dep_count = n;
  file->public_deps = UPB_DEFBUILDER_ALLOCARRAY(ctx, int32_t, n);
  int32_t* mutable_public_deps = (int32_t*)file->public_deps;
  for (size_t i = 0; i < n; i++) {
    if (public_deps[i] >= file->dep_count) {
      _upb_DefBuilder_Errf(ctx, "public_dep %d is out of range",
                           (int)public_deps[i]);
    }
    mutable_public_deps[i] = public_deps[i];
  }

  weak_deps = google_protobuf_FileDescriptorProto_weak_dependency(file_proto, &n);
  file->weak_dep_count = n;
  file->weak_deps = UPB_DEFBUILDER_ALLOCARRAY(ctx, const int32_t, n);
  int32_t* mutable_weak_deps = (int32_t*)file->weak_deps;
  for (size_t i = 0; i < n; i++) {
    if (weak_deps[i] >= file->dep_count) {
      _upb_DefBuilder_Errf(ctx, "weak_dep %d is out of range",
                           (int)weak_deps[i]);
    }
    mutable_weak_deps[i] = weak_deps[i];
  }

  // Create enums.
  enums = google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  file->top_lvl_enum_count = n;
  file->top_lvl_enums =
      _upb_EnumDefs_New(ctx, n, enums, file->resolved_features, NULL);

  // Create extensions.
  exts = google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->top_lvl_ext_count = n;
  file->top_lvl_exts = _upb_Extensions_New(
      ctx, n, exts, file->resolved_features, file->package, NULL);

  // Create messages.
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  file->top_lvl_msg_count = n;
  file->top_lvl_msgs =
      _upb_MessageDefs_New(ctx, n, msgs, file->resolved_features, NULL);

  // Create services.
  services = google_protobuf_FileDescriptorProto_service(file_proto, &n);
  file->service_count = n;
  file->services =
      _upb_ServiceDefs_New(ctx, n, services, file->resolved_features);

  // Now that all names are in the table, build layouts and resolve refs.

  for (int i = 0; i < file->top_lvl_msg_count; i++) {
    upb_MessageDef* m = (upb_MessageDef*)upb_FileDef_TopLevelMessage(file, i);
    _upb_MessageDef_Resolve(ctx, m);
  }

  for (int i = 0; i < file->top_lvl_ext_count; i++) {
    upb_FieldDef* f = (upb_FieldDef*)upb_FileDef_TopLevelExtension(file, i);
    _upb_FieldDef_Resolve(ctx, file->package, f);
  }

  for (int i = 0; i < file->top_lvl_msg_count; i++) {
    upb_MessageDef* m = (upb_MessageDef*)upb_FileDef_TopLevelMessage(file, i);
    _upb_MessageDef_CreateMiniTable(ctx, (upb_MessageDef*)m);
  }

  for (int i = 0; i < file->top_lvl_ext_count; i++) {
    upb_FieldDef* f = (upb_FieldDef*)upb_FileDef_TopLevelExtension(file, i);
    _upb_FieldDef_BuildMiniTableExtension(ctx, f);
  }

  for (int i = 0; i < file->top_lvl_msg_count; i++) {
    upb_MessageDef* m = (upb_MessageDef*)upb_FileDef_TopLevelMessage(file, i);
    _upb_MessageDef_LinkMiniTable(ctx, m);
  }

  if (file->ext_count) {
    upb_ExtensionRegistryStatus status = upb_ExtensionRegistry_AddArray(
        _upb_DefPool_ExtReg(ctx->symtab), file->ext_layouts, file->ext_count);
    if (status != kUpb_ExtensionRegistryStatus_Ok) {
      if (status == kUpb_ExtensionRegistryStatus_OutOfMemory) {
        _upb_DefBuilder_OomErr(ctx);
      }

      UPB_ASSERT(status == kUpb_ExtensionRegistryStatus_DuplicateEntry);
      _upb_DefBuilder_Errf(ctx, "duplicate extension entry");
    }
  }
}
