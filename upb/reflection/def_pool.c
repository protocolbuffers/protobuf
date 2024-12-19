// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/def_pool.h"

#include "upb/base/status.h"
#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/file_def.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/internal/enum_def.h"
#include "upb/reflection/internal/enum_value_def.h"
#include "upb/reflection/internal/field_def.h"
#include "upb/reflection/internal/file_def.h"
#include "upb/reflection/internal/message_def.h"
#include "upb/reflection/internal/service_def.h"
#include "upb/reflection/internal/upb_edition_defaults.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_DefPool {
  upb_Arena* arena;
  upb_strtable syms;   // full_name -> packed def ptr
  upb_strtable files;  // file_name -> (upb_FileDef*)
  upb_inttable exts;   // (upb_MiniTableExtension*) -> (upb_FieldDef*)
  upb_ExtensionRegistry* extreg;
  const UPB_DESC(FeatureSetDefaults) * feature_set_defaults;
  upb_MiniTablePlatform platform;
  void* scratch_data;
  size_t scratch_size;
  size_t bytes_loaded;
};

void upb_DefPool_Free(upb_DefPool* s) {
  upb_Arena_Free(s->arena);
  upb_gfree(s->scratch_data);
  upb_gfree(s);
}

static const char serialized_defaults[] = UPB_INTERNAL_UPB_EDITION_DEFAULTS;

upb_DefPool* upb_DefPool_New(void) {
  upb_DefPool* s = upb_gmalloc(sizeof(*s));
  if (!s) return NULL;

  s->arena = upb_Arena_New();
  s->bytes_loaded = 0;

  s->scratch_size = 240;
  s->scratch_data = upb_gmalloc(s->scratch_size);
  if (!s->scratch_data) goto err;

  if (!upb_strtable_init(&s->syms, 32, s->arena)) goto err;
  if (!upb_strtable_init(&s->files, 4, s->arena)) goto err;
  if (!upb_inttable_init(&s->exts, s->arena)) goto err;

  s->extreg = upb_ExtensionRegistry_New(s->arena);
  if (!s->extreg) goto err;

  s->platform = kUpb_MiniTablePlatform_Native;

  upb_Status status;
  if (!upb_DefPool_SetFeatureSetDefaults(
          s, serialized_defaults, sizeof(serialized_defaults) - 1, &status)) {
    goto err;
  }

  if (!s->feature_set_defaults) goto err;

  return s;

err:
  upb_DefPool_Free(s);
  return NULL;
}

const UPB_DESC(FeatureSetDefaults) *
    upb_DefPool_FeatureSetDefaults(const upb_DefPool* s) {
  return s->feature_set_defaults;
}

bool upb_DefPool_SetFeatureSetDefaults(upb_DefPool* s,
                                       const char* serialized_defaults,
                                       size_t serialized_len,
                                       upb_Status* status) {
  const UPB_DESC(FeatureSetDefaults)* defaults = UPB_DESC(
      FeatureSetDefaults_parse)(serialized_defaults, serialized_len, s->arena);
  if (!defaults) {
    upb_Status_SetErrorFormat(status, "Failed to parse defaults");
    return false;
  }
  if (upb_strtable_count(&s->files) > 0) {
    upb_Status_SetErrorFormat(status,
                              "Feature set defaults can't be changed once the "
                              "pool has started building");
    return false;
  }
  int min_edition = UPB_DESC(FeatureSetDefaults_minimum_edition(defaults));
  int max_edition = UPB_DESC(FeatureSetDefaults_maximum_edition(defaults));
  if (min_edition > max_edition) {
    upb_Status_SetErrorFormat(status, "Invalid edition range %s to %s",
                              upb_FileDef_EditionName(min_edition),
                              upb_FileDef_EditionName(max_edition));
    return false;
  }
  size_t size;
  const UPB_DESC(
      FeatureSetDefaults_FeatureSetEditionDefault)* const* default_list =
      UPB_DESC(FeatureSetDefaults_defaults(defaults, &size));
  int prev_edition = UPB_DESC(EDITION_UNKNOWN);
  for (size_t i = 0; i < size; ++i) {
    int edition = UPB_DESC(
        FeatureSetDefaults_FeatureSetEditionDefault_edition(default_list[i]));
    if (edition == UPB_DESC(EDITION_UNKNOWN)) {
      upb_Status_SetErrorFormat(status, "Invalid edition UNKNOWN specified");
      return false;
    }
    if (edition <= prev_edition) {
      upb_Status_SetErrorFormat(status,
                                "Feature set defaults are not strictly "
                                "increasing, %s is greater than or equal to %s",
                                upb_FileDef_EditionName(prev_edition),
                                upb_FileDef_EditionName(edition));
      return false;
    }
    prev_edition = edition;
  }

  // Copy the defaults into the pool.
  s->feature_set_defaults = defaults;
  return true;
}

bool _upb_DefPool_InsertExt(upb_DefPool* s, const upb_MiniTableExtension* ext,
                            const upb_FieldDef* f) {
  return upb_inttable_insert(&s->exts, (uintptr_t)ext, upb_value_constptr(f),
                             s->arena);
}

bool _upb_DefPool_InsertSym(upb_DefPool* s, upb_StringView sym, upb_value v,
                            upb_Status* status) {
  // TODO: table should support an operation "tryinsert" to avoid the double
  // lookup.
  if (upb_strtable_lookup2(&s->syms, sym.data, sym.size, NULL)) {
    upb_Status_SetErrorFormat(status, "duplicate symbol '%s'", sym.data);
    return false;
  }
  if (!upb_strtable_insert(&s->syms, sym.data, sym.size, v, s->arena)) {
    upb_Status_SetErrorMessage(status, "out of memory");
    return false;
  }
  return true;
}

static const void* _upb_DefPool_Unpack(const upb_DefPool* s, const char* sym,
                                       size_t size, upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup2(&s->syms, sym, size, &v)
             ? _upb_DefType_Unpack(v, type)
             : NULL;
}

bool _upb_DefPool_LookupSym(const upb_DefPool* s, const char* sym, size_t size,
                            upb_value* v) {
  return upb_strtable_lookup2(&s->syms, sym, size, v);
}

upb_ExtensionRegistry* _upb_DefPool_ExtReg(const upb_DefPool* s) {
  return s->extreg;
}

void** _upb_DefPool_ScratchData(const upb_DefPool* s) {
  return (void**)&s->scratch_data;
}

size_t* _upb_DefPool_ScratchSize(const upb_DefPool* s) {
  return (size_t*)&s->scratch_size;
}

void _upb_DefPool_SetPlatform(upb_DefPool* s, upb_MiniTablePlatform platform) {
  assert(upb_strtable_count(&s->files) == 0);
  s->platform = platform;
}

const upb_MessageDef* upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym) {
  return _upb_DefPool_Unpack(s, sym, strlen(sym), UPB_DEFTYPE_MSG);
}

const upb_MessageDef* upb_DefPool_FindMessageByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len) {
  return _upb_DefPool_Unpack(s, sym, len, UPB_DEFTYPE_MSG);
}

const upb_EnumDef* upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym) {
  return _upb_DefPool_Unpack(s, sym, strlen(sym), UPB_DEFTYPE_ENUM);
}

const upb_EnumValueDef* upb_DefPool_FindEnumByNameval(const upb_DefPool* s,
                                                      const char* sym) {
  return _upb_DefPool_Unpack(s, sym, strlen(sym), UPB_DEFTYPE_ENUMVAL);
}

const upb_FileDef* upb_DefPool_FindFileByName(const upb_DefPool* s,
                                              const char* name) {
  upb_value v;
  return upb_strtable_lookup(&s->files, name, &v) ? upb_value_getconstptr(v)
                                                  : NULL;
}

const upb_FileDef* upb_DefPool_FindFileByNameWithSize(const upb_DefPool* s,
                                                      const char* name,
                                                      size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->files, name, len, &v)
             ? upb_value_getconstptr(v)
             : NULL;
}

const upb_FieldDef* upb_DefPool_FindExtensionByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size) {
  upb_value v;
  if (!upb_strtable_lookup2(&s->syms, name, size, &v)) return NULL;

  switch (_upb_DefType_Type(v)) {
    case UPB_DEFTYPE_FIELD:
      return _upb_DefType_Unpack(v, UPB_DEFTYPE_FIELD);
    case UPB_DEFTYPE_MSG: {
      const upb_MessageDef* m = _upb_DefType_Unpack(v, UPB_DEFTYPE_MSG);
      if (_upb_MessageDef_InMessageSet(m)) {
        for (int i = 0; i < upb_MessageDef_NestedExtensionCount(m); i++) {
          const upb_FieldDef* ext = upb_MessageDef_NestedExtension(m, i);
          if (upb_FieldDef_MessageSubDef(ext) == m) {
            return ext;
          }
        }
      }
      return NULL;
    }
    default:
      break;
  }

  return NULL;
}

const upb_FieldDef* upb_DefPool_FindExtensionByName(const upb_DefPool* s,
                                                    const char* sym) {
  return upb_DefPool_FindExtensionByNameWithSize(s, sym, strlen(sym));
}

const upb_ServiceDef* upb_DefPool_FindServiceByName(const upb_DefPool* s,
                                                    const char* name) {
  return _upb_DefPool_Unpack(s, name, strlen(name), UPB_DEFTYPE_SERVICE);
}

const upb_ServiceDef* upb_DefPool_FindServiceByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size) {
  return _upb_DefPool_Unpack(s, name, size, UPB_DEFTYPE_SERVICE);
}

const upb_FileDef* upb_DefPool_FindFileContainingSymbol(const upb_DefPool* s,
                                                        const char* name) {
  upb_value v;
  // TODO: non-extension fields and oneofs.
  if (upb_strtable_lookup(&s->syms, name, &v)) {
    switch (_upb_DefType_Type(v)) {
      case UPB_DEFTYPE_EXT: {
        const upb_FieldDef* f = _upb_DefType_Unpack(v, UPB_DEFTYPE_EXT);
        return upb_FieldDef_File(f);
      }
      case UPB_DEFTYPE_MSG: {
        const upb_MessageDef* m = _upb_DefType_Unpack(v, UPB_DEFTYPE_MSG);
        return upb_MessageDef_File(m);
      }
      case UPB_DEFTYPE_ENUM: {
        const upb_EnumDef* e = _upb_DefType_Unpack(v, UPB_DEFTYPE_ENUM);
        return upb_EnumDef_File(e);
      }
      case UPB_DEFTYPE_ENUMVAL: {
        const upb_EnumValueDef* ev =
            _upb_DefType_Unpack(v, UPB_DEFTYPE_ENUMVAL);
        return upb_EnumDef_File(upb_EnumValueDef_Enum(ev));
      }
      case UPB_DEFTYPE_SERVICE: {
        const upb_ServiceDef* service =
            _upb_DefType_Unpack(v, UPB_DEFTYPE_SERVICE);
        return upb_ServiceDef_File(service);
      }
      default:
        UPB_UNREACHABLE();
    }
  }

  const char* last_dot = strrchr(name, '.');
  if (last_dot) {
    const upb_MessageDef* parent =
        upb_DefPool_FindMessageByNameWithSize(s, name, last_dot - name);
    if (parent) {
      const char* shortname = last_dot + 1;
      if (upb_MessageDef_FindByNameWithSize(parent, shortname,
                                            strlen(shortname), NULL, NULL)) {
        return upb_MessageDef_File(parent);
      }
    }
  }

  return NULL;
}

static void remove_filedef(upb_DefPool* s, upb_FileDef* file) {
  intptr_t iter = UPB_INTTABLE_BEGIN;
  upb_StringView key;
  upb_value val;
  while (upb_strtable_next2(&s->syms, &key, &val, &iter)) {
    const upb_FileDef* f;
    switch (_upb_DefType_Type(val)) {
      case UPB_DEFTYPE_EXT:
        f = upb_FieldDef_File(_upb_DefType_Unpack(val, UPB_DEFTYPE_EXT));
        break;
      case UPB_DEFTYPE_MSG:
        f = upb_MessageDef_File(_upb_DefType_Unpack(val, UPB_DEFTYPE_MSG));
        break;
      case UPB_DEFTYPE_ENUM:
        f = upb_EnumDef_File(_upb_DefType_Unpack(val, UPB_DEFTYPE_ENUM));
        break;
      case UPB_DEFTYPE_ENUMVAL:
        f = upb_EnumDef_File(upb_EnumValueDef_Enum(
            _upb_DefType_Unpack(val, UPB_DEFTYPE_ENUMVAL)));
        break;
      case UPB_DEFTYPE_SERVICE:
        f = upb_ServiceDef_File(_upb_DefType_Unpack(val, UPB_DEFTYPE_SERVICE));
        break;
      default:
        UPB_UNREACHABLE();
    }

    if (f == file) upb_strtable_removeiter(&s->syms, &iter);
  }
}

static const upb_FileDef* upb_DefBuilder_AddFileToPool(
    upb_DefBuilder* const builder, upb_DefPool* const s,
    const UPB_DESC(FileDescriptorProto) * const file_proto,
    const upb_StringView name, upb_Status* const status) {
  if (UPB_SETJMP(builder->err) != 0) {
    UPB_ASSERT(!upb_Status_IsOk(status));
    if (builder->file) {
      remove_filedef(s, builder->file);
      builder->file = NULL;
    }
  } else if (!builder->arena || !builder->tmp_arena ||
             !upb_strtable_init(&builder->feature_cache, 16,
                                builder->tmp_arena) ||
             !(builder->legacy_features =
                   UPB_DESC(FeatureSet_new)(builder->tmp_arena))) {
    _upb_DefBuilder_OomErr(builder);
  } else {
    _upb_FileDef_Create(builder, file_proto);
    upb_strtable_insert(&s->files, name.data, name.size,
                        upb_value_constptr(builder->file), builder->arena);
    UPB_ASSERT(upb_Status_IsOk(status));
    upb_Arena_Fuse(s->arena, builder->arena);
  }

  if (builder->arena) upb_Arena_Free(builder->arena);
  if (builder->tmp_arena) upb_Arena_Free(builder->tmp_arena);
  return builder->file;
}

static const upb_FileDef* _upb_DefPool_AddFile(
    upb_DefPool* s, const UPB_DESC(FileDescriptorProto) * file_proto,
    const upb_MiniTableFile* layout, upb_Status* status) {
  const upb_StringView name = UPB_DESC(FileDescriptorProto_name)(file_proto);

  // Determine whether we already know about this file.
  {
    upb_value v;
    if (upb_strtable_lookup2(&s->files, name.data, name.size, &v)) {
      upb_Status_SetErrorFormat(status,
                                "duplicate file name " UPB_STRINGVIEW_FORMAT,
                                UPB_STRINGVIEW_ARGS(name));
      return NULL;
    }
  }

  upb_DefBuilder ctx = {
      .symtab = s,
      .tmp_buf = NULL,
      .tmp_buf_size = 0,
      .layout = layout,
      .platform = s->platform,
      .msg_count = 0,
      .enum_count = 0,
      .ext_count = 0,
      .status = status,
      .file = NULL,
      .arena = upb_Arena_New(),
      .tmp_arena = upb_Arena_New(),
  };

  return upb_DefBuilder_AddFileToPool(&ctx, s, file_proto, name, status);
}

const upb_FileDef* upb_DefPool_AddFile(upb_DefPool* s,
                                       const UPB_DESC(FileDescriptorProto) *
                                           file_proto,
                                       upb_Status* status) {
  return _upb_DefPool_AddFile(s, file_proto, NULL, status);
}

bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable) {
  /* Since this function should never fail (it would indicate a bug in upb) we
   * print errors to stderr instead of returning error status to the user. */
  _upb_DefPool_Init** deps = init->deps;
  UPB_DESC(FileDescriptorProto) * file;
  upb_Arena* arena;
  upb_Status status;

  upb_Status_Clear(&status);

  if (upb_DefPool_FindFileByName(s, init->filename)) {
    return true;
  }

  arena = upb_Arena_New();

  for (; *deps; deps++) {
    if (!_upb_DefPool_LoadDefInitEx(s, *deps, rebuild_minitable)) goto err;
  }

  file = UPB_DESC(FileDescriptorProto_parse_ex)(
      init->descriptor.data, init->descriptor.size, NULL,
      kUpb_DecodeOption_AliasString, arena);
  s->bytes_loaded += init->descriptor.size;

  if (!file) {
    upb_Status_SetErrorFormat(
        &status,
        "Failed to parse compiled-in descriptor for file '%s'. This should "
        "never happen.",
        init->filename);
    goto err;
  }

  const upb_MiniTableFile* mt = rebuild_minitable ? NULL : init->layout;
  if (!_upb_DefPool_AddFile(s, file, mt, &status)) {
    goto err;
  }

  upb_Arena_Free(arena);
  return true;

err:
  fprintf(stderr,
          "Error loading compiled-in descriptor for file '%s' (this should "
          "never happen): %s\n",
          init->filename, upb_Status_ErrorMessage(&status));
  upb_Arena_Free(arena);
  return false;
}

size_t _upb_DefPool_BytesLoaded(const upb_DefPool* s) {
  return s->bytes_loaded;
}

upb_Arena* _upb_DefPool_Arena(const upb_DefPool* s) { return s->arena; }

const upb_FieldDef* upb_DefPool_FindExtensionByMiniTable(
    const upb_DefPool* s, const upb_MiniTableExtension* ext) {
  upb_value v;
  bool ok = upb_inttable_lookup(&s->exts, (uintptr_t)ext, &v);
  UPB_ASSERT(ok);
  return upb_value_getconstptr(v);
}

const upb_FieldDef* upb_DefPool_FindExtensionByNumber(const upb_DefPool* s,
                                                      const upb_MessageDef* m,
                                                      int32_t fieldnum) {
  const upb_MiniTable* t = upb_MessageDef_MiniTable(m);
  const upb_MiniTableExtension* ext =
      upb_ExtensionRegistry_Lookup(s->extreg, t, fieldnum);
  return ext ? upb_DefPool_FindExtensionByMiniTable(s, ext) : NULL;
}

const upb_ExtensionRegistry* upb_DefPool_ExtensionRegistry(
    const upb_DefPool* s) {
  return s->extreg;
}

const upb_FieldDef** upb_DefPool_GetAllExtensions(const upb_DefPool* s,
                                                  const upb_MessageDef* m,
                                                  size_t* count) {
  size_t n = 0;
  intptr_t iter = UPB_INTTABLE_BEGIN;
  uintptr_t key;
  upb_value val;
  // This is O(all exts) instead of O(exts for m).  If we need this to be
  // efficient we may need to make extreg into a two-level table, or have a
  // second per-message index.
  while (upb_inttable_next(&s->exts, &key, &val, &iter)) {
    const upb_FieldDef* f = upb_value_getconstptr(val);
    if (upb_FieldDef_ContainingType(f) == m) n++;
  }
  const upb_FieldDef** exts = upb_gmalloc(n * sizeof(*exts));
  iter = UPB_INTTABLE_BEGIN;
  size_t i = 0;
  while (upb_inttable_next(&s->exts, &key, &val, &iter)) {
    const upb_FieldDef* f = upb_value_getconstptr(val);
    if (upb_FieldDef_ContainingType(f) == m) exts[i++] = f;
  }
  *count = n;
  return exts;
}

bool _upb_DefPool_LoadDefInit(upb_DefPool* s, const _upb_DefPool_Init* init) {
  return _upb_DefPool_LoadDefInitEx(s, init, false);
}
