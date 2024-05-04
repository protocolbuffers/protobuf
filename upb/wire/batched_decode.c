// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/internal/arena.h"
#include "upb/wire/decode.h"

#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/message/internal/accessors.h"
#include "upb/base/internal/endian.h"
#include "upb/base/string_view.h"
#include "utf8_range.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/tagged_ptr.h"
#include "upb/message/message.h"
#include "upb/message/tagged_ptr.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/sub.h"
#include "upb/port/atomic.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/constants.h"
#include "upb/wire/reader.h"
#include "upb/wire/batched.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  const upb_ExtensionRegistry* extreg;
  const char* end;
  int depth;                 // Tracks recursion depth to bound stack usage.
  uint16_t options;
  bool missing_required;
  union {
    upb_Arena arena;
    void* foo[UPB_ARENA_SIZE_HACK];
  };
  upb_DecodeStatus status;
  jmp_buf err;
} upb_BatchDecoder;

#define DBG(...) // fprintf(stderr, __VA_ARGS__)

static const char* upb_BatchDecoder_DecodeMessage(upb_BatchDecoder* d,
                                                  const char* ptr,
                                                  upb_Message* msg,
                                                  const upb_MiniTable* m);

UPB_NORETURN static void* _upb_BatchDecoder_ErrorJmp(upb_BatchDecoder* d,
                                                     upb_DecodeStatus status) {
  UPB_ASSERT(status != kUpb_DecodeStatus_Ok);
  __builtin_trap();
  d->status = status;
  UPB_LONGJMP(d->err, 1);
}

static void _upb_BatchDecoder_VerifyUtf8(upb_BatchDecoder* d, const char* buf,
                                         int len) {
  if (!utf8_range_IsValid(buf, len)) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);
  }
}

static bool _upb_BatchDecoder_Reserve(upb_BatchDecoder* d, upb_Array* arr,
                                      size_t elem) {
  bool need_realloc =
      arr->UPB_PRIVATE(capacity) - arr->UPB_PRIVATE(size) < elem;
  if (need_realloc && !UPB_PRIVATE(_upb_Array_Realloc)(
                          arr, arr->UPB_PRIVATE(size) + elem, &d->arena)) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
  return need_realloc;
}

/*
static upb_Message* _upb_BatchDecoder_NewSubMessage(
    upb_BatchDecoder* d, const upb_MiniTableSub* subs,
    const upb_MiniTableField* field, upb_TaggedMessagePtr* target) {
  const upb_MiniTable* subl = _upb_MiniTableSubs_MessageByField(subs, field);
  UPB_ASSERT(subl);
  upb_Message* msg = _upb_Message_New(subl, &d->arena);
  if (!msg) _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);

  // Extensions should not be unlinked. A message extension should not be
  // registered until its sub-message type is available to be linked.
  bool is_empty = UPB_PRIVATE(_upb_MiniTable_IsEmpty)(subl);
  bool is_extension = field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsExtension;
  UPB_ASSERT(!(is_empty && is_extension));

  if (is_empty && !(d->options & kUpb_DecodeOption_ExperimentalAllowUnlinked)) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_UnlinkedSubMessage);
  }

  upb_TaggedMessagePtr tagged =
      UPB_PRIVATE(_upb_TaggedMessagePtr_Pack)(msg, is_empty);
  memcpy(target, &tagged, sizeof(tagged));
  return msg;
}

static upb_Message* _upb_BatchDecoder_ReuseSubMessage(
    upb_BatchDecoder* d, const upb_MiniTableSub* subs,
    const upb_MiniTableField* field, upb_TaggedMessagePtr* target) {
  upb_TaggedMessagePtr tagged = *target;
  const upb_MiniTable* subl = _upb_MiniTableSubs_MessageByField(subs, field);
  UPB_ASSERT(subl);
  if (!upb_TaggedMessagePtr_IsEmpty(tagged) ||
      UPB_PRIVATE(_upb_MiniTable_IsEmpty)(subl)) {
    return UPB_PRIVATE(_upb_TaggedMessagePtr_GetMessage)(tagged);
  }

  // We found an empty message from a previous parse that was performed before
  // this field was linked.  But it is linked now, so we want to allocate a new
  // message of the correct type and promote data into it before continuing.
  upb_Message* existing =
      UPB_PRIVATE(_upb_TaggedMessagePtr_GetEmptyMessage)(tagged);
  upb_Message* promoted =
      _upb_BatchDecoder_NewSubMessage(d, subs, field, target);
  size_t size;
  const char* unknown = upb_Message_GetUnknown(existing, &size);
  upb_DecodeStatus status = upb_Decode(unknown, size, promoted, subl, d->extreg,
                                       d->options, &d->arena);
  if (status != kUpb_DecodeStatus_Ok) _upb_BatchDecoder_ErrorJmp(d, status);
  return promoted;
}

UPB_FORCEINLINE
const char* _upb_BatchDecoder_RecurseSubMessage(upb_BatchDecoder* d,
                                                const char* ptr,
                                                upb_Message* submsg,
                                                const upb_MiniTable* subl,
                                                uint32_t expected_end_group) {
  if (--d->depth < 0) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);
  }
  ptr = _upb_BatchDecoder_DecodeMessage(d, ptr, submsg, subl);
  d->depth++;
  if (d->end_group != expected_end_group) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_BatchDecoder_DecodeSubMessage(
    upb_BatchDecoder* d, const char* ptr, upb_Message* submsg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field, int size) {
  int saved_delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, size);
  const upb_MiniTable* subl = _upb_MiniTableSubs_MessageByField(subs, field);
  UPB_ASSERT(subl);
  ptr =
      _upb_BatchDecoder_RecurseSubMessage(d, ptr, submsg, subl, DECODE_NOGROUP);
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, saved_delta);
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_BatchDecoder_DecodeGroup(upb_BatchDecoder* d, const char* ptr,
                                          upb_Message* submsg,
                                          const upb_MiniTable* subl,
                                          uint32_t number) {
  if (_upb_BatchDecoder_IsDone(d, &ptr)) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  ptr = _upb_BatchDecoder_RecurseSubMessage(d, ptr, submsg, subl, number);
  d->end_group = DECODE_NOGROUP;
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_BatchDecoder_DecodeUnknownGroup(upb_BatchDecoder* d,
                                                 const char* ptr,
                                                 uint32_t number) {
  return _upb_BatchDecoder_DecodeGroup(d, ptr, NULL, NULL, number);
}

UPB_FORCEINLINE
const char* _upb_BatchDecoder_DecodeKnownGroup(
    upb_BatchDecoder* d, const char* ptr, upb_Message* submsg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field) {
  const upb_MiniTable* subl = _upb_MiniTableSubs_MessageByField(subs, field);
  UPB_ASSERT(subl);
  return _upb_BatchDecoder_DecodeGroup(d, ptr, submsg, subl,
                                       field->UPB_PRIVATE(number));
}

static char* upb_BatchDecoder_EncodeVarint32(uint32_t val, char* ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

static void _upb_BatchDecoder_AddUnknownVarints(upb_BatchDecoder* d,
                                                upb_Message* msg, uint32_t val1,
                                                uint32_t val2) {
  char buf[20];
  char* end = buf;
  end = upb_BatchDecoder_EncodeVarint32(val1, end);
  end = upb_BatchDecoder_EncodeVarint32(val2, end);

  if (!UPB_PRIVATE(_upb_Message_AddUnknown)(msg, buf, end - buf, &d->arena)) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

UPB_FORCEINLINE
bool _upb_BatchDecoder_CheckEnum(upb_BatchDecoder* d, const char* ptr,
                                 upb_Message* msg, const upb_MiniTableEnum* e,
                                 const upb_MiniTableField* field,
                                 wireval* val) {
  const uint32_t v = val->uint32_val;

  if (UPB_LIKELY(upb_MiniTableEnum_CheckValue(e, v))) return true;

  // Unrecognized enum goes into unknown fields.
  // For packed fields the tag could be arbitrarily far in the past,
  // so we just re-encode the tag and value here.
  const uint32_t tag =
      ((uint32_t)field->UPB_PRIVATE(number) << 3) | kUpb_WireType_Varint;
  upb_Message* unknown_msg =
      field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsExtension ? d->unknown_msg
                                                             : msg;
  _upb_BatchDecoder_AddUnknownVarints(d, unknown_msg, tag, v);
  return false;
}

UPB_NOINLINE
static const char* _upb_BatchDecoder_DecodeEnumArray(
    upb_BatchDecoder* d, const char* ptr, upb_Message* msg, upb_Array* arr,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field,
    wireval* val) {
  const upb_MiniTableEnum* e = _upb_MiniTableSubs_EnumByField(subs, field);
  if (!_upb_BatchDecoder_CheckEnum(d, ptr, msg, e, field, val)) return ptr;
  void* mem = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) * 4, void);
  arr->UPB_PRIVATE(size)++;
  memcpy(mem, val, 4);
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_BatchDecoder_DecodeFixedPacked(upb_BatchDecoder* d,
                                                const char* ptr, upb_Array* arr,
                                                wireval* val,
                                                const upb_MiniTableField* field,
                                                int lg2) {
  int mask = (1 << lg2) - 1;
  size_t count = val->size >> lg2;
  if ((val->size & mask) != 0) {
    // Length isn't a round multiple of elem size.
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  _upb_BatchDecoder_Reserve(d, arr, count);
  void* mem = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) << lg2, void);
  arr->UPB_PRIVATE(size) += count;
  // Note: if/when the decoder supports multi-buffer input, we will need to
  // handle buffer seams here.
  if (upb_IsLittleEndian()) {
    ptr = upb_EpsCopyInputStream_Copy(&d->input, ptr, mem, val->size);
  } else {
    int delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
    char* dst = mem;
    while (!_upb_BatchDecoder_IsDone(d, &ptr)) {
      if (lg2 == 2) {
        ptr = upb_WireReader_ReadFixed32(ptr, dst);
        dst += 4;
      } else {
        UPB_ASSERT(lg2 == 3);
        ptr = upb_WireReader_ReadFixed64(ptr, dst);
        dst += 8;
      }
    }
    upb_EpsCopyInputStream_PopLimit(&d->input, ptr, delta);
  }

  return ptr;
}

UPB_FORCEINLINE
const char* _upb_BatchDecoder_DecodeVarintPacked(
    upb_BatchDecoder* d, const char* ptr, upb_Array* arr, wireval* val,
    const upb_MiniTableField* field, int lg2) {
  int scale = 1 << lg2;
  int saved_limit = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
  char* out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) << lg2, void);
  while (!_upb_BatchDecoder_IsDone(d, &ptr)) {
    wireval elem;
    ptr = _upb_BatchDecoder_DecodeVarint(d, ptr, &elem.uint64_val);
    _upb_BatchDecoder_Munge(field->UPB_PRIVATE(descriptortype), &elem);
    if (_upb_BatchDecoder_Reserve(d, arr, 1)) {
      out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                       arr->UPB_PRIVATE(size) << lg2, void);
    }
    arr->UPB_PRIVATE(size)++;
    memcpy(out, &elem, scale);
    out += scale;
  }
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, saved_limit);
  return ptr;
}
*/

static upb_Array* _upb_BatchDecoder_CreateArray(
    upb_BatchDecoder* d, const upb_MiniTableField* field) {
  const upb_FieldType field_type = field->UPB_PRIVATE(descriptortype);
  const size_t lg2 = UPB_PRIVATE(_upb_FieldType_SizeLg2)(field_type);
  upb_Array* ret = UPB_PRIVATE(_upb_Array_New)(&d->arena, 4, lg2);
  if (!ret) _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static void upb_BatchDecoder_ReadString(upb_BatchDecoder* d, const char* ptr,
                                        size_t size, upb_StringView* str) {
  if (d->options & kUpb_DecodeOption_AliasString) {
    str->data = ptr;
    str->size = size;
  } else {
    char* data = upb_Arena_Malloc(&d->arena, size);
    if (!data) _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    memcpy(data, ptr, size);
    str->data = data;
    str->size = size;
  }
}

static const char* _upb_BatchDecoder_DecodeToArray(
    upb_BatchDecoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* f, int size) {
  upb_Array** arrp = UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), void);
  upb_Array* arr = *arrp;

  if (!arr) {
    arr = _upb_BatchDecoder_CreateArray(d, f);
    *arrp = arr;
  }

  if (upb_MiniTableField_IsString(f)) {
    _upb_BatchDecoder_Reserve(d, arr, 1);
    upb_StringView* str = upb_Array_MutableDataPtr(arr);
    str += arr->UPB_PRIVATE(size);
    arr->UPB_PRIVATE(size) += 1;
    upb_BatchDecoder_ReadString(d, ptr, size, str);
  } else {
    UPB_ASSERT(false);  // NYI
  }

  return ptr + size;
}

/*
static upb_Map* _upb_BatchDecoder_CreateMap(upb_BatchDecoder* d,
                                       const upb_MiniTable* entry) {
  // Maps descriptor type -> upb map size
  static const uint8_t kSizeInMap[] = {
      [0] = -1,  // invalid descriptor type
      [kUpb_FieldType_Double] = 8,
      [kUpb_FieldType_Float] = 4,
      [kUpb_FieldType_Int64] = 8,
      [kUpb_FieldType_UInt64] = 8,
      [kUpb_FieldType_Int32] = 4,
      [kUpb_FieldType_Fixed64] = 8,
      [kUpb_FieldType_Fixed32] = 4,
      [kUpb_FieldType_Bool] = 1,
      [kUpb_FieldType_String] = UPB_MAPTYPE_STRING,
      [kUpb_FieldType_Group] = sizeof(void*),
      [kUpb_FieldType_Message] = sizeof(void*),
      [kUpb_FieldType_Bytes] = UPB_MAPTYPE_STRING,
      [kUpb_FieldType_UInt32] = 4,
      [kUpb_FieldType_Enum] = 4,
      [kUpb_FieldType_SFixed32] = 4,
      [kUpb_FieldType_SFixed64] = 8,
      [kUpb_FieldType_SInt32] = 4,
      [kUpb_FieldType_SInt64] = 8,
  };

  const upb_MiniTableField* key_field = &entry->UPB_PRIVATE(fields)[0];
  const upb_MiniTableField* val_field = &entry->UPB_PRIVATE(fields)[1];
  char key_size = kSizeInMap[key_field->UPB_PRIVATE(descriptortype)];
  char val_size = kSizeInMap[val_field->UPB_PRIVATE(descriptortype)];
  UPB_ASSERT(key_field->UPB_PRIVATE(offset) == offsetof(upb_MapEntry, k));
  UPB_ASSERT(val_field->UPB_PRIVATE(offset) == offsetof(upb_MapEntry, v));
  upb_Map* ret = _upb_Map_New(&d->arena, key_size, val_size);
  if (!ret) _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static const char* _upb_BatchDecoder_DecodeToMap(upb_BatchDecoder* d, const
char* ptr, upb_Message* msg, const upb_MiniTableSub* subs, const
upb_MiniTableField* field, wireval* val) { upb_Map** map_p = UPB_PTR_AT(msg,
field->UPB_PRIVATE(offset), upb_Map*); upb_Map* map = *map_p; upb_MapEntry ent;
  UPB_ASSERT(upb_MiniTableField_Type(field) == kUpb_FieldType_Message);
  const upb_MiniTable* entry = _upb_MiniTableSubs_MessageByField(subs, field);

  UPB_ASSERT(entry);
  UPB_ASSERT(entry->UPB_PRIVATE(field_count) == 2);
  UPB_ASSERT(upb_MiniTableField_IsScalar(&entry->UPB_PRIVATE(fields)[0]));
  UPB_ASSERT(upb_MiniTableField_IsScalar(&entry->UPB_PRIVATE(fields)[1]));

  if (!map) {
    map = _upb_BatchDecoder_CreateMap(d, entry);
    *map_p = map;
  }

  // Parse map entry.
  memset(&ent, 0, sizeof(ent));

  if (entry->UPB_PRIVATE(fields)[1].UPB_PRIVATE(descriptortype) ==
          kUpb_FieldType_Message ||
      entry->UPB_PRIVATE(fields)[1].UPB_PRIVATE(descriptortype) ==
          kUpb_FieldType_Group) {
    // Create proactively to handle the case where it doesn't appear.
    upb_TaggedMessagePtr msg;
    _upb_BatchDecoder_NewSubMessage(d, entry->UPB_PRIVATE(subs),
                               &entry->UPB_PRIVATE(fields)[1], &msg);
    ent.v.val = upb_value_uintptr(msg);
  }

  ptr = _upb_BatchDecoder_DecodeSubMessage(d, ptr, &ent.message, subs, field,
                                      val->size);
  // check if ent had any unknown fields
  size_t size;
  upb_Message_GetUnknown(&ent.message, &size);
  if (size != 0) {
    char* buf;
    size_t size;
    uint32_t tag =
        ((uint32_t)field->UPB_PRIVATE(number) << 3) | kUpb_WireType_Delimited;
    upb_EncodeStatus status =
        upb_Encode(&ent.message, entry, 0, &d->arena, &buf, &size);
    if (status != kUpb_EncodeStatus_Ok) {
      _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
    _upb_BatchDecoder_AddUnknownVarints(d, msg, tag, size);
    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(msg, buf, size, &d->arena)) {
      _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else {
    if (_upb_Map_Insert(map, &ent.k, map->key_size, &ent.v, map->val_size,
                        &d->arena) == kUpb_MapInsertStatus_OutOfMemory) {
      _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  }
  return ptr;
}

  // Store into message.
  switch (op) {
    case kUpb_DecodeOp_SubMessage: {
      upb_TaggedMessagePtr* submsgp = mem;
      upb_Message* submsg;
      if (*submsgp) {
        submsg = _upb_BatchDecoder_ReuseSubMessage(d, subs, field, submsgp);
      } else {
        submsg = _upb_BatchDecoder_NewSubMessage(d, subs, field, submsgp);
      }
      if (UPB_UNLIKELY(type == kUpb_FieldType_Group)) {
        ptr = _upb_BatchDecoder_DecodeKnownGroup(d, ptr, submsg, subs, field);
      } else {
        ptr = _upb_BatchDecoder_DecodeSubMessage(d, ptr, submsg, subs, field,
                                            val->size);
      }
      break;
    }

*/

static void upb_BatchDecoder_BoundsCheck(upb_BatchDecoder* d, const char* ptr,
                                        size_t size) {
  if (UPB_UNLIKELY(d->end - ptr < size)) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
}

static const char* upb_BatchDecoder_ReadUintNoBoundsCheck(upb_BatchDecoder* d,
                                                          const char* ptr,
                                                          void* val,
                                                          size_t size) {
  memcpy(val, ptr, size);
  return ptr + size;
}

static const char* upb_BatchDecoder_ReadUint(upb_BatchDecoder* d,
                                             const char* ptr, void* val,
                                             size_t size) {
  upb_BatchDecoder_BoundsCheck(d, ptr, size);
  return upb_BatchDecoder_ReadUintNoBoundsCheck(d, ptr, val, size);
}

static void upb_BranchlessStoreUint(void* to, const void* from, size_t size) {
  UPB_ASSERT(size == 1 || size == 4 || size == 8);
  uint64_t dummy;
  memcpy(size == 1 ? to : &dummy, from, 1);
  memcpy(size == 4 ? to : &dummy, from, 4);
  memcpy(size == 8 ? to : &dummy, from, 8);
}

static const char* _upb_BatchDecoder_DecodeToMessage(
    upb_BatchDecoder* d, const char* ptr, upb_Message* restrict msg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* restrict f, int size) {
  UPB_PRIVATE(_upb_Message_SetPresence)(msg, f);

  void* mem = UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), void);
  int dtype = f->UPB_PRIVATE(descriptortype);

  if (dtype == kUpb_FieldType_String || dtype == kUpb_FieldType_Bytes) {
    DBG("- Decoding scalar string field, number=%d, value=%.*s\n",
        (int)upb_MiniTableField_Number(f), (int)size, ptr);
    if (dtype == kUpb_FieldType_String) {
      _upb_BatchDecoder_VerifyUtf8(d, ptr, size);
    }
    upb_StringView* str = (upb_StringView*)mem;
    if (d->options & kUpb_DecodeOption_AliasString) {
      str->data = ptr;
      str->size = size;
    } else {
      char* data = upb_Arena_Malloc(&d->arena, size);
      if (!data) _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
      memcpy(data, ptr, size);
      str->data = data;
      str->size = size;
    }
  } else {
    uint64_t data = 0;
    int field_size = upb_BatchEncoder_PrimitiveFieldSize(f);

    if (UPB_UNLIKELY(d->end - ptr < 8)) {
      upb_BatchDecoder_ReadUintNoBoundsCheck(d, ptr, &data, size);
    } else {
      // Branchless fast path.
      memcpy(&data, ptr, 8);
      data &= (1ULL << (size * 8)) - 1;
    }

    DBG("- Decoding scalar primitive field, number=%d, value=%ld, size=%d\n",
        (int)upb_MiniTableField_Number(f), (long)data, size);

    if (upb_MiniTableField_IsClosedEnum(f)) {
      const upb_MiniTableEnum* e =
          subs[f->UPB_PRIVATE(submsg_index)].UPB_PRIVATE(subenum);
      DBG("Checking enum value...");
      if (!upb_MiniTableEnum_CheckValue(e, data)) {
        // We don't handle this case yet.
        _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
      }
      DBG("ok\n");
    }

    upb_BranchlessStoreUint(mem, &data, field_size);
  }

  return ptr + size;
}

UPB_NOINLINE
const char* _upb_BatchDecoder_CheckRequired(upb_BatchDecoder* d,
                                            const char* ptr,
                                            const upb_Message* msg,
                                            const upb_MiniTable* m) {
  UPB_ASSERT(m->UPB_PRIVATE(required_count));
  if (UPB_UNLIKELY(d->options & kUpb_DecodeOption_CheckRequired)) {
    d->missing_required =
        !UPB_PRIVATE(_upb_Message_IsInitializedShallow)(msg, m);
  }
  return ptr;
}

enum {
  kUpb_FakeFieldType_FieldNotFound = 0,
};

static const upb_MiniTableField* _upb_BatchDecoder_FindField(
    upb_BatchDecoder* d, const upb_MiniTable* t, uint32_t field_number,
    upb_Message* msg, upb_Message** to_msg, const upb_MiniTableSub** subs,
    int* last_field_index) {
  size_t idx = ((size_t)field_number) - 1;  // 0 wraps to SIZE_MAX
  if (UPB_LIKELY(idx < t->UPB_PRIVATE(dense_below))) {
    // Fastest case: index into dense fields.
    *last_field_index = idx;
    *to_msg = msg;
    *subs = t->UPB_PRIVATE(subs);
    return &t->UPB_PRIVATE(fields)[idx];
  }

  if (t->UPB_PRIVATE(dense_below) < t->UPB_PRIVATE(field_count)) {
    // Linear search non-dense fields. Resume scanning from last_field_index
    // since fields are usually in order.
    size_t last = *last_field_index;
    for (idx = last; idx < t->UPB_PRIVATE(field_count); idx++) {
      if (t->UPB_PRIVATE(fields)[idx].UPB_PRIVATE(number) == field_number) {
        goto found;
      }
    }

    for (idx = t->UPB_PRIVATE(dense_below); idx < last; idx++) {
      if (t->UPB_PRIVATE(fields)[idx].UPB_PRIVATE(number) == field_number) {
        goto found;
      }
    }
  }

  if (d->extreg && t->UPB_PRIVATE(ext) == kUpb_ExtMode_Extendable) {
    const upb_MiniTableExtension* ext_mt =
        upb_ExtensionRegistry_Lookup(d->extreg, t, field_number);
    if (ext_mt) {
      upb_Extension* ext = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
          msg, ext_mt, &d->arena);
      if (UPB_UNLIKELY(!ext)) {
        _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
      }
      *to_msg = (upb_Message*)&ext->data;
      *subs = &ext_mt->UPB_PRIVATE(sub);
      return &ext_mt->UPB_PRIVATE(field);
    }
  }

  return NULL;  // Unknown field.

found:
  UPB_ASSERT(t->UPB_PRIVATE(fields)[idx].UPB_PRIVATE(number) == field_number);
  *last_field_index = idx;
  *to_msg = msg;
  *subs = t->UPB_PRIVATE(subs);
  return &t->UPB_PRIVATE(fields)[idx];
}

/*
UPB_FORCEINLINE
void _upb_BatchDecoder_MaybeVerifyUtf8(upb_BatchDecoder* d,
                                       const upb_MiniTableField* field,
                                       int* op) {
  if ((field->UPB_ONLYBITS(mode) & kUpb_LabelFlags_IsAlternate) &&
      UPB_UNLIKELY(d->options & kUpb_DecodeOption_AlwaysValidateUtf8))
    *op = kUpb_DecodeOp_String;
}
*/

UPB_FORCEINLINE
const char* _upb_BatchDecoder_DecodeKnownField(
    upb_BatchDecoder* d, const char* data, upb_Message* msg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field, int size) {
  uint8_t mode = field->UPB_PRIVATE(mode);

  switch (mode & kUpb_FieldMode_Mask) {
    case kUpb_FieldMode_Array:
      return _upb_BatchDecoder_DecodeToArray(d, data, msg, subs, field, size);
    case kUpb_FieldMode_Scalar:
      return _upb_BatchDecoder_DecodeToMessage(d, data, msg, subs, field, size);
    default:
      UPB_UNREACHABLE();
  }
}

static const char* upb_BatchDecoder_DecodeBatch(upb_BatchDecoder* d,
                                                const char* ptr,
                                                upb_Message* msg,
                                                const upb_MiniTable* m,
                                                int batch_size) {
  size_t tag_bytes = batch_size * sizeof(uint16_t);
  upb_BatchDecoder_BoundsCheck(d, ptr, tag_bytes);

  const char* tag_ptr = ptr;
  const char* data = ptr + tag_bytes;
  int last_field_index = 0;

  for (int i = 0; i < batch_size; i++) {
    uint16_t tag;
    memcpy(&tag, tag_ptr, 2);
    tag_ptr += 2;
    int field_number = tag >> kUpb_FieldNumberShift;
    int field_size = tag & kUpb_LongField;

    if (field_number == kUpb_BigFieldNumber) {
      data = upb_BatchDecoder_ReadUint(d, data, &field_number, 4);
    }

    if (field_size == kUpb_LongField) {
      data = upb_BatchDecoder_ReadUint(d, data, &field_size, 4);
    }

    upb_BatchDecoder_BoundsCheck(d, data, field_size);

    upb_Message* to_msg;
    const upb_MiniTableSub* subs;

    const upb_MiniTableField* f = _upb_BatchDecoder_FindField(
        d, m, field_number, msg, &to_msg, &subs, &last_field_index);

    if (!f) {
      // We don't support unknown fields yet.
      _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
    }

    data = _upb_BatchDecoder_DecodeKnownField(d, data, to_msg, subs, f,
                                              field_size);
  }

  return data;
}

static const char* upb_BatchDecoder_DecodeSubMessages(upb_BatchDecoder* d,
                                                      const char* ptr,
                                                      upb_Message* msg,
                                                      const upb_MiniTable* m,
                                                      int batch_size) {
  int field_number;
  int last_field_index = 0;
  ptr = upb_BatchDecoder_ReadUint(d, ptr, &field_number, 4);
  upb_Message* to_msg;
  const upb_MiniTableSub* subs;
  const upb_MiniTableField* f = _upb_BatchDecoder_FindField(
      d, m, field_number, msg, &to_msg, &subs, &last_field_index);
  if (!upb_MiniTableField_IsSubMessage(f)) {
    _upb_BatchDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }

  const upb_MiniTable* subl = upb_MiniTable_SubMessage(m, f);
  void* mem = UPB_PTR_AT(to_msg, f->UPB_PRIVATE(offset), void);

  if (upb_MiniTableField_IsArray(f)) {
    upb_Array** arrp = mem;
    upb_Array* arr = *arrp;
    if (!arr) {
      arr = _upb_BatchDecoder_CreateArray(d, f);
      *arrp = arr;
    }
    _upb_BatchDecoder_Reserve(d, arr, batch_size);
    upb_Message** elems = upb_Array_MutableDataPtr(arr);
    size_t message_size = subl->UPB_PRIVATE(size);
    size_t message_bytes = batch_size * message_size;
    char* messages = upb_Arena_Malloc(&d->arena, message_bytes);
    memset(messages, 0, message_bytes);
    size_t old_size = arr->UPB_PRIVATE(size);
    elems += old_size;
    // size_t new_size = old_size + batch_size;
    for (int i = 0; i < batch_size; i++, messages += message_size) {
      elems[i] = (upb_Message*)messages;
      ptr = upb_BatchDecoder_DecodeMessage(d, ptr, elems[i], subl);
    }
    arr->UPB_PRIVATE(size) += batch_size;
    DBG("YO: Adding %d elements to array\n", batch_size);
  } else {
    DBG("YO: Parsing batch size %d of non-repeated message, field number=%d\n", batch_size, (int)field_number);
    upb_Message** submsgp = mem;
    upb_Message* submsg = *submsgp;
    if (submsg == NULL) {
      submsg = upb_Message_New(subl, &d->arena);
      *submsgp = submsg;
    }
    for (int i = 0; i < batch_size; i++) {
      ptr = upb_BatchDecoder_DecodeMessage(d, ptr, submsg, subl);
    }
    UPB_PRIVATE(_upb_Message_SetPresence)(to_msg, f);
  }

  return ptr;
}

UPB_NOINLINE
static const char* upb_BatchDecoder_DecodeMessage(upb_BatchDecoder* d,
                                                  const char* ptr,
                                                  upb_Message* msg,
                                                  const upb_MiniTable* m) {
  while (ptr != d->end) {
    uint8_t batch;
    ptr = upb_BatchDecoder_ReadUint(d, ptr, &batch, 1);

    if (batch == 0) {
      DBG("YO: Batch 0\n");
      break;
    } else if (batch <= kUpb_MaxBatch) {
      DBG("YO: Decoding primitive batch of size %d, remaining buffer=%d bytes\n", batch, (int)(d->end - ptr));
      ptr = upb_BatchDecoder_DecodeBatch(d, ptr, msg, m, batch);
      DBG("YO: Now remaining buffer=%d bytes\n", (int)(d->end - ptr));
    } else {
      batch -= kUpb_MaxBatch;
      DBG("YO: Decoding message batch of size %d\n", batch);
      ptr = upb_BatchDecoder_DecodeSubMessages(d, ptr, msg, m, batch);
    }
  }

  DBG("YO: Finished decoding message\n");

  return UPB_UNLIKELY(m->UPB_PRIVATE(required_count))
             ? _upb_BatchDecoder_CheckRequired(d, ptr, msg, m)
             : ptr;
}

static upb_DecodeStatus _upb_BatchDecoder_DecodeTop(upb_BatchDecoder* d,
                                                    const char* buf,
                                                    upb_Message* msg,
                                                    const upb_MiniTable* m) {
  upb_BatchDecoder_DecodeMessage(d, buf, msg, m);
  if (d->missing_required) return kUpb_DecodeStatus_MissingRequired;
  return kUpb_DecodeStatus_Ok;
}

static upb_DecodeStatus upb_BatchDecoder_Decode(upb_BatchDecoder* const decoder,
                                                const char* const buf,
                                                upb_Message* const msg,
                                                const upb_MiniTable* const m,
                                                upb_Arena* const arena) {
  if (UPB_SETJMP(decoder->err) == 0) {
    decoder->status = _upb_BatchDecoder_DecodeTop(decoder, buf, msg, m);
  } else {
    UPB_ASSERT(decoder->status != kUpb_DecodeStatus_Ok);
  }

  UPB_PRIVATE(_upb_Arena_SwapOut)(arena, &decoder->arena);

  return decoder->status;
}

upb_DecodeStatus upb_BatchedDecode(const char* buf, size_t size,
                                   upb_Message* msg, const upb_MiniTable* mt,
                                   const upb_ExtensionRegistry* extreg,
                                   int options, upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_BatchDecoder decoder;
  unsigned depth = (unsigned)options >> 16;

  decoder.extreg = extreg;
  decoder.end = buf + size;
  decoder.depth = depth ? depth : kUpb_WireFormat_DefaultDepthLimit;
  decoder.options = (uint16_t)options;
  decoder.missing_required = false;
  decoder.status = kUpb_DecodeStatus_Ok;

  // Violating the encapsulation of the arena for performance reasons.
  // This is a temporary arena that we swap into and swap out of when we are
  // done.  The temporary arena only needs to be able to handle allocation,
  // not fuse or free, so it does not need many of the members to be initialized
  // (particularly parent_or_count).
  UPB_PRIVATE(_upb_Arena_SwapIn)(&decoder.arena, arena);

  return upb_BatchDecoder_Decode(&decoder, buf, msg, mt, arena);
}
