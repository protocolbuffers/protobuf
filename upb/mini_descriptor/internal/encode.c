/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/mini_descriptor/internal/encode.h"

#include "upb/base/log2.h"
#include "upb/mini_descriptor/internal/base92.h"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_descriptor/internal/wire_constants.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  uint64_t present_values_mask;
  uint32_t last_written_value;
} upb_MtDataEncoderInternal_EnumState;

typedef struct {
  uint64_t msg_modifiers;
  uint32_t last_field_num;
  enum {
    kUpb_OneofState_NotStarted,
    kUpb_OneofState_StartedOneof,
    kUpb_OneofState_EmittedOneofField,
  } oneof_state;
} upb_MtDataEncoderInternal_MsgState;

typedef struct {
  char* buf_start;  // Only for checking kUpb_MtDataEncoder_MinSize.
  union {
    upb_MtDataEncoderInternal_EnumState enum_state;
    upb_MtDataEncoderInternal_MsgState msg_state;
  } state;
} upb_MtDataEncoderInternal;

static upb_MtDataEncoderInternal* upb_MtDataEncoder_GetInternal(
    upb_MtDataEncoder* e, char* buf_start) {
  UPB_ASSERT(sizeof(upb_MtDataEncoderInternal) <= sizeof(e->internal));
  upb_MtDataEncoderInternal* ret = (upb_MtDataEncoderInternal*)e->internal;
  ret->buf_start = buf_start;
  return ret;
}

static char* upb_MtDataEncoder_PutRaw(upb_MtDataEncoder* e, char* ptr,
                                      char ch) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  UPB_ASSERT(ptr - in->buf_start < kUpb_MtDataEncoder_MinSize);
  if (ptr == e->end) return NULL;
  *ptr++ = ch;
  return ptr;
}

static char* upb_MtDataEncoder_Put(upb_MtDataEncoder* e, char* ptr, char ch) {
  return upb_MtDataEncoder_PutRaw(e, ptr, _upb_ToBase92(ch));
}

static char* upb_MtDataEncoder_PutBase92Varint(upb_MtDataEncoder* e, char* ptr,
                                               uint32_t val, int min, int max) {
  int shift = upb_Log2Ceiling(_upb_FromBase92(max) - _upb_FromBase92(min) + 1);
  UPB_ASSERT(shift <= 6);
  uint32_t mask = (1 << shift) - 1;
  do {
    uint32_t bits = val & mask;
    ptr = upb_MtDataEncoder_Put(e, ptr, bits + _upb_FromBase92(min));
    if (!ptr) return NULL;
    val >>= shift;
  } while (val);
  return ptr;
}

char* upb_MtDataEncoder_PutModifier(upb_MtDataEncoder* e, char* ptr,
                                    uint64_t mod) {
  if (mod) {
    ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, mod,
                                            kUpb_EncodedValue_MinModifier,
                                            kUpb_EncodedValue_MaxModifier);
  }
  return ptr;
}

char* upb_MtDataEncoder_EncodeExtension(upb_MtDataEncoder* e, char* ptr,
                                        upb_FieldType type, uint32_t field_num,
                                        uint64_t field_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.msg_state.msg_modifiers = 0;
  in->state.msg_state.last_field_num = 0;
  in->state.msg_state.oneof_state = kUpb_OneofState_NotStarted;

  ptr = upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_ExtensionV1);
  if (!ptr) return NULL;

  return upb_MtDataEncoder_PutField(e, ptr, type, field_num, field_mod);
}

char* upb_MtDataEncoder_EncodeMap(upb_MtDataEncoder* e, char* ptr,
                                  upb_FieldType key_type,
                                  upb_FieldType value_type, uint64_t key_mod,
                                  uint64_t value_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.msg_state.msg_modifiers = 0;
  in->state.msg_state.last_field_num = 0;
  in->state.msg_state.oneof_state = kUpb_OneofState_NotStarted;

  ptr = upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_MapV1);
  if (!ptr) return NULL;

  ptr = upb_MtDataEncoder_PutField(e, ptr, key_type, 1, key_mod);
  if (!ptr) return NULL;

  return upb_MtDataEncoder_PutField(e, ptr, value_type, 2, value_mod);
}

char* upb_MtDataEncoder_EncodeMessageSet(upb_MtDataEncoder* e, char* ptr) {
  (void)upb_MtDataEncoder_GetInternal(e, ptr);
  return upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_MessageSetV1);
}

char* upb_MtDataEncoder_StartMessage(upb_MtDataEncoder* e, char* ptr,
                                     uint64_t msg_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.msg_state.msg_modifiers = msg_mod;
  in->state.msg_state.last_field_num = 0;
  in->state.msg_state.oneof_state = kUpb_OneofState_NotStarted;

  ptr = upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_MessageV1);
  if (!ptr) return NULL;

  return upb_MtDataEncoder_PutModifier(e, ptr, msg_mod);
}

static char* _upb_MtDataEncoder_MaybePutFieldSkip(upb_MtDataEncoder* e,
                                                  char* ptr,
                                                  uint32_t field_num) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  if (field_num <= in->state.msg_state.last_field_num) return NULL;
  if (in->state.msg_state.last_field_num + 1 != field_num) {
    // Put skip.
    UPB_ASSERT(field_num > in->state.msg_state.last_field_num);
    uint32_t skip = field_num - in->state.msg_state.last_field_num;
    ptr = upb_MtDataEncoder_PutBase92Varint(
        e, ptr, skip, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip);
    if (!ptr) return NULL;
  }
  in->state.msg_state.last_field_num = field_num;
  return ptr;
}

static char* _upb_MtDataEncoder_PutFieldType(upb_MtDataEncoder* e, char* ptr,
                                             upb_FieldType type,
                                             uint64_t field_mod) {
  static const char kUpb_TypeToEncoded[] = {
      [kUpb_FieldType_Double] = kUpb_EncodedType_Double,
      [kUpb_FieldType_Float] = kUpb_EncodedType_Float,
      [kUpb_FieldType_Int64] = kUpb_EncodedType_Int64,
      [kUpb_FieldType_UInt64] = kUpb_EncodedType_UInt64,
      [kUpb_FieldType_Int32] = kUpb_EncodedType_Int32,
      [kUpb_FieldType_Fixed64] = kUpb_EncodedType_Fixed64,
      [kUpb_FieldType_Fixed32] = kUpb_EncodedType_Fixed32,
      [kUpb_FieldType_Bool] = kUpb_EncodedType_Bool,
      [kUpb_FieldType_String] = kUpb_EncodedType_String,
      [kUpb_FieldType_Group] = kUpb_EncodedType_Group,
      [kUpb_FieldType_Message] = kUpb_EncodedType_Message,
      [kUpb_FieldType_Bytes] = kUpb_EncodedType_Bytes,
      [kUpb_FieldType_UInt32] = kUpb_EncodedType_UInt32,
      [kUpb_FieldType_Enum] = kUpb_EncodedType_OpenEnum,
      [kUpb_FieldType_SFixed32] = kUpb_EncodedType_SFixed32,
      [kUpb_FieldType_SFixed64] = kUpb_EncodedType_SFixed64,
      [kUpb_FieldType_SInt32] = kUpb_EncodedType_SInt32,
      [kUpb_FieldType_SInt64] = kUpb_EncodedType_SInt64,
  };

  int encoded_type = kUpb_TypeToEncoded[type];

  if (field_mod & kUpb_FieldModifier_IsClosedEnum) {
    UPB_ASSERT(type == kUpb_FieldType_Enum);
    encoded_type = kUpb_EncodedType_ClosedEnum;
  }

  if (field_mod & kUpb_FieldModifier_IsRepeated) {
    // Repeated fields shift the type number up (unlike other modifiers which
    // are bit flags).
    encoded_type += kUpb_EncodedType_RepeatedBase;
  }

  return upb_MtDataEncoder_Put(e, ptr, encoded_type);
}

static char* _upb_MtDataEncoder_MaybePutModifiers(upb_MtDataEncoder* e,
                                                  char* ptr, upb_FieldType type,
                                                  uint64_t field_mod) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  uint32_t encoded_modifiers = 0;
  if ((field_mod & kUpb_FieldModifier_IsRepeated) &&
      upb_FieldType_IsPackable(type)) {
    bool field_is_packed = field_mod & kUpb_FieldModifier_IsPacked;
    bool default_is_packed = in->state.msg_state.msg_modifiers &
                             kUpb_MessageModifier_DefaultIsPacked;
    if (field_is_packed != default_is_packed) {
      encoded_modifiers |= kUpb_EncodedFieldModifier_FlipPacked;
    }
  }

  if (field_mod & kUpb_FieldModifier_IsProto3Singular) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsProto3Singular;
  }

  if (field_mod & kUpb_FieldModifier_IsRequired) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsRequired;
  }

  return upb_MtDataEncoder_PutModifier(e, ptr, encoded_modifiers);
}

char* upb_MtDataEncoder_PutField(upb_MtDataEncoder* e, char* ptr,
                                 upb_FieldType type, uint32_t field_num,
                                 uint64_t field_mod) {
  upb_MtDataEncoder_GetInternal(e, ptr);

  ptr = _upb_MtDataEncoder_MaybePutFieldSkip(e, ptr, field_num);
  if (!ptr) return NULL;

  ptr = _upb_MtDataEncoder_PutFieldType(e, ptr, type, field_mod);
  if (!ptr) return NULL;

  return _upb_MtDataEncoder_MaybePutModifiers(e, ptr, type, field_mod);
}

char* upb_MtDataEncoder_StartOneof(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->state.msg_state.oneof_state == kUpb_OneofState_NotStarted) {
    ptr = upb_MtDataEncoder_Put(e, ptr, _upb_FromBase92(kUpb_EncodedValue_End));
  } else {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, _upb_FromBase92(kUpb_EncodedValue_OneofSeparator));
  }
  in->state.msg_state.oneof_state = kUpb_OneofState_StartedOneof;
  return ptr;
}

char* upb_MtDataEncoder_PutOneofField(upb_MtDataEncoder* e, char* ptr,
                                      uint32_t field_num) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->state.msg_state.oneof_state == kUpb_OneofState_EmittedOneofField) {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, _upb_FromBase92(kUpb_EncodedValue_FieldSeparator));
    if (!ptr) return NULL;
  }
  ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, field_num, _upb_ToBase92(0),
                                          _upb_ToBase92(63));
  in->state.msg_state.oneof_state = kUpb_OneofState_EmittedOneofField;
  return ptr;
}

char* upb_MtDataEncoder_StartEnum(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.enum_state.present_values_mask = 0;
  in->state.enum_state.last_written_value = 0;

  return upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_EnumV1);
}

static char* upb_MtDataEncoder_FlushDenseEnumMask(upb_MtDataEncoder* e,
                                                  char* ptr) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  ptr = upb_MtDataEncoder_Put(e, ptr, in->state.enum_state.present_values_mask);
  in->state.enum_state.present_values_mask = 0;
  in->state.enum_state.last_written_value += 5;
  return ptr;
}

char* upb_MtDataEncoder_PutEnumValue(upb_MtDataEncoder* e, char* ptr,
                                     uint32_t val) {
  // TODO(b/229641772): optimize this encoding.
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  UPB_ASSERT(val >= in->state.enum_state.last_written_value);
  uint32_t delta = val - in->state.enum_state.last_written_value;
  if (delta >= 5 && in->state.enum_state.present_values_mask) {
    ptr = upb_MtDataEncoder_FlushDenseEnumMask(e, ptr);
    if (!ptr) {
      return NULL;
    }
    delta -= 5;
  }

  if (delta >= 5) {
    ptr = upb_MtDataEncoder_PutBase92Varint(
        e, ptr, delta, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip);
    in->state.enum_state.last_written_value += delta;
    delta = 0;
  }

  UPB_ASSERT((in->state.enum_state.present_values_mask >> delta) == 0);
  in->state.enum_state.present_values_mask |= 1ULL << delta;
  return ptr;
}

char* upb_MtDataEncoder_EndEnum(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (!in->state.enum_state.present_values_mask) return ptr;
  return upb_MtDataEncoder_FlushDenseEnumMask(e, ptr);
}
