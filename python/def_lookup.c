// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file or at https://developers.google.com/open-source/licenses/bsd

#include "python/def_lookup.h"

#include <stdint.h>
#include <string.h>

#include "upb/mem/alloc.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/internal/file_def.h"

static upb_deftype_t PyUpb_DefLookup_Type(PyUpb_LookupKind kind) {
  switch (kind) {
    case kPyUpb_LookupMessage: return UPB_DEFTYPE_MSG;
    case kPyUpb_LookupEnum: return UPB_DEFTYPE_ENUM;
    case kPyUpb_LookupEnumValue: return UPB_DEFTYPE_ENUMVAL;
    case kPyUpb_LookupExtension: return UPB_DEFTYPE_EXT;
    case kPyUpb_LookupService: return UPB_DEFTYPE_SERVICE;
  }
  return UPB_DEFTYPE_MSG;  // All callers supply a valid kind.
}

static const void* PyUpb_DefLookup_Qualified(
    const upb_FileDef* file, const char* scope, const char* name,
    PyUpb_LookupKind kind) {
  size_t scope_size = strlen(scope);
  size_t name_size = strlen(name);
  if (name_size > SIZE_MAX - 2 || scope_size > SIZE_MAX - name_size - 2) {
    return NULL;
  }
  size_t size = scope_size + (scope_size != 0) + name_size;
  char buffer[256];
  char* qualified = size < sizeof(buffer) ? buffer : upb_gmalloc(size + 1);
  if (!qualified) return NULL;
  memcpy(qualified, scope, scope_size);
  if (scope_size) qualified[scope_size] = '.';
  memcpy(qualified + scope_size + (scope_size != 0), name, name_size + 1);
  const void* result = _upb_FileDef_FindSymbol(
      file, qualified, size, PyUpb_DefLookup_Type(kind));
  if (qualified != buffer) upb_gfree(qualified);
  return result;
}

const void* PyUpb_DefLookup_Message(const upb_MessageDef* message,
                                  const char* name, PyUpb_LookupKind kind) {
  return PyUpb_DefLookup_Qualified(upb_MessageDef_File(message),
                                 upb_MessageDef_FullName(message), name, kind);
}

const void* PyUpb_DefLookup_File(const upb_FileDef* file, const char* name,
                               PyUpb_LookupKind kind) {
  return PyUpb_DefLookup_Qualified(file, upb_FileDef_Package(file), name, kind);
}
