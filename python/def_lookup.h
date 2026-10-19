// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file or at https://developers.google.com/open-source/licenses/bsd

#ifndef PYUPB_DEF_LOOKUP_H_
#define PYUPB_DEF_LOOKUP_H_

#include "upb/reflection/def.h"

typedef enum {
  kPyUpb_LookupMessage,
  kPyUpb_LookupEnum,
  kPyUpb_LookupEnumValue,
  kPyUpb_LookupExtension,
  kPyUpb_LookupService,
} PyUpb_LookupKind;

// Resolve relative names using each published file's immutable symbol index,
// never the mutable DefPool tables. The caller must keep the owning pool alive.
// Dotted descendant names retain the historical upb descriptor-map behavior.
const void* PyUpb_DefLookup_Message(const upb_MessageDef* message,
                                  const char* name, PyUpb_LookupKind kind);
const void* PyUpb_DefLookup_File(const upb_FileDef* file, const char* name,
                               PyUpb_LookupKind kind);

#endif  // PYUPB_DEF_LOOKUP_H_
