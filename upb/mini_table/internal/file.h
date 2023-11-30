// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_FILE_H_
#define UPB_MINI_TABLE_INTERNAL_FILE_H_

#include "upb/mini_table/types.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MiniTableFile {
  const upb_MiniTable** UPB_PRIVATE(msgs);
  const upb_MiniTableEnum** UPB_PRIVATE(enums);
  const upb_MiniTableExtension** UPB_PRIVATE(exts);
  int UPB_PRIVATE(msg_count);
  int UPB_PRIVATE(enum_count);
  int UPB_PRIVATE(ext_count);
};

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE int UPB_PRIVATE(_upb_MiniTableFile_EnumCount)(
    const upb_MiniTableFile* f) {
  return f->UPB_PRIVATE(enum_count);
}

UPB_INLINE int UPB_PRIVATE(_upb_MiniTableFile_ExtensionCount)(
    const upb_MiniTableFile* f) {
  return f->UPB_PRIVATE(ext_count);
}

UPB_INLINE int UPB_PRIVATE(_upb_MiniTableFile_MessageCount)(
    const upb_MiniTableFile* f) {
  return f->UPB_PRIVATE(msg_count);
}

UPB_INLINE const upb_MiniTableEnum* UPB_PRIVATE(_upb_MiniTableFile_Enum)(
    const upb_MiniTableFile* f, int i) {
  UPB_ASSERT(i < f->UPB_PRIVATE(enum_count));
  return f->UPB_PRIVATE(enums)[i];
}

UPB_INLINE const upb_MiniTableExtension* UPB_PRIVATE(
    _upb_MiniTableFile_Extension)(const upb_MiniTableFile* f, int i) {
  UPB_ASSERT(i < f->UPB_PRIVATE(ext_count));
  return f->UPB_PRIVATE(exts)[i];
}

UPB_INLINE const upb_MiniTable* UPB_PRIVATE(_upb_MiniTableFile_Message)(
    const upb_MiniTableFile* f, int i) {
  UPB_ASSERT(i < f->UPB_PRIVATE(msg_count));
  return f->UPB_PRIVATE(msgs)[i];
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_FILE_H_ */
