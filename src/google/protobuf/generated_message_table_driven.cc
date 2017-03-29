// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <google/protobuf/generated_message_table_driven.h>

#include <google/protobuf/stubs/type_traits.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/wire_format_lite_inl.h>


namespace google {
namespace protobuf {
namespace internal {


namespace {

enum StringType {
  StringType_STRING = 0,
  StringType_CORD = 1,
  StringType_STRING_PIECE = 2
};

template <typename Type>
inline Type* Raw(MessageLite* msg, int64 offset) {
  return reinterpret_cast<Type*>(reinterpret_cast<uint8*>(msg) + offset);
}

template <typename Type>
inline const Type* Raw(const MessageLite* msg, int64 offset) {
  return reinterpret_cast<const Type*>(reinterpret_cast<const uint8*>(msg) +
                                       offset);
}

inline Arena* GetArena(MessageLite* msg, int64 arena_offset) {
  if (GOOGLE_PREDICT_FALSE(arena_offset == -1)) {
    return NULL;
  }

  return Raw<InternalMetadataWithArenaLite>(msg, arena_offset)->arena();
}

template <typename Type>
inline Type* AddField(MessageLite* msg, int64 offset) {
#if LANG_CXX11
  static_assert(std::is_trivially_copy_assignable<Type>::value,
                "Do not assign");
#endif

  google::protobuf::RepeatedField<Type>* repeated =
      Raw<google::protobuf::RepeatedField<Type> >(msg, offset);
  return repeated->Add();
}

template <>
inline string* AddField<string>(MessageLite* msg, int64 offset) {
  google::protobuf::RepeatedPtrField<string>* repeated =
      Raw<google::protobuf::RepeatedPtrField<string> >(msg, offset);
  return repeated->Add();
}


template <typename Type>
inline void AddField(MessageLite* msg, int64 offset, Type value) {
#if LANG_CXX11
  static_assert(std::is_trivially_copy_assignable<Type>::value,
                "Do not assign");
#endif
  *AddField<Type>(msg, offset) = value;
}

inline void SetBit(uint32* has_bits, uint32 has_bit_index) {
  GOOGLE_DCHECK(has_bits != NULL);

  uint32 mask = static_cast<uint32>(1u) << (has_bit_index % 32);
  has_bits[has_bit_index / 32u] |= mask;
}

template <typename Type>
inline Type* MutableField(MessageLite* msg, uint32* has_bits,
                          uint32 has_bit_index, int64 offset) {
  SetBit(has_bits, has_bit_index);
  return Raw<Type>(msg, offset);
}

template <typename Type>
inline void SetField(MessageLite* msg, uint32* has_bits, uint32 has_bit_index,
                     int64 offset, Type value) {
#if LANG_CXX11
  static_assert(std::is_trivially_copy_assignable<Type>::value,
                "Do not assign");
#endif
  *MutableField<Type>(msg, has_bits, has_bit_index, offset) = value;
}

template <bool repeated, bool validate, StringType ctype>
static inline bool HandleString(io::CodedInputStream* input, MessageLite* msg,
                                Arena* arena, uint32* has_bits,
                                uint32 has_bit_index, int64 offset,
                                const void* default_ptr, bool strict_utf8,
                                const char* field_name) {
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
  const char* sdata;
  size_t size;
#endif

    string* value;
    if (repeated) {
      value = AddField<string>(msg, offset);
      GOOGLE_DCHECK(value != NULL);
    } else {
      // TODO(ckennelly): Is this optimal?
      value = MutableField<ArenaStringPtr>(msg, has_bits, has_bit_index, offset)
                  ->Mutable(static_cast<const string*>(default_ptr), arena);
      GOOGLE_DCHECK(value != NULL);
    }

    if (GOOGLE_PREDICT_FALSE(!WireFormatLite::ReadString(input, value))) {
      return false;
    }

#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    sdata = value->data();
    size = value->size();
#endif

#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
  if (validate) {
    if (strict_utf8) {
      if (GOOGLE_PREDICT_FALSE(!WireFormatLite::VerifyUtf8String(
              sdata, size, WireFormatLite::PARSE, field_name))) {
        return false;
      }
    } else {
      WireFormatLite::VerifyUTF8String(
          sdata, size, WireFormat::PARSE, field_name);
    }
  }
#endif

  return true;
}

string* MutableUnknownFields(MessageLite* msg, int64 arena_offset) {
  return Raw<InternalMetadataWithArenaLite>(msg, arena_offset)
      ->mutable_unknown_fields();
}

// RepeatedMessageTypeHandler allows us to operate on RepeatedPtrField fields
// without instantiating the specific template.
class RepeatedMessageTypeHandler {
 public:
  typedef MessageLite Type;
  static Arena* GetArena(Type* t) { return t->GetArena(); }
  static void* GetMaybeArenaPointer(Type* t) {
    return t->GetMaybeArenaPointer();
  }
  static inline Type* NewFromPrototype(const Type* prototype,
                                       Arena* arena = NULL) {
    return prototype->New(arena);
  }
  static void Delete(Type* t, Arena* arena = NULL) {
    if (arena == NULL) {
      delete t;
    }
  }
};

inline bool ReadGroup(int field_number, io::CodedInputStream* input,
                      MessageLite* value, const ParseTable& table) {
  if (GOOGLE_PREDICT_FALSE(!input->IncrementRecursionDepth())) {
    return false;
  }

  if (GOOGLE_PREDICT_FALSE(!MergePartialFromCodedStream(value, table, input))) {
    return false;
  }

  input->DecrementRecursionDepth();
  // Make sure the last thing read was an end tag for this group.
  if (GOOGLE_PREDICT_FALSE(!input->LastTagWas(WireFormatLite::MakeTag(
          field_number, WireFormatLite::WIRETYPE_END_GROUP)))) {
    return false;
  }

  return true;
}

inline bool ReadMessage(io::CodedInputStream* input, MessageLite* value,
                        const ParseTable& table) {
  int length;
  if (GOOGLE_PREDICT_FALSE(!input->ReadVarintSizeAsInt(&length))) {
    return false;
  }

  std::pair<io::CodedInputStream::Limit, int> p =
      input->IncrementRecursionDepthAndPushLimit(length);
  if (GOOGLE_PREDICT_FALSE(p.second < 0 ||
                    !MergePartialFromCodedStream(value, table, input))) {
    return false;
  }

  // Make sure that parsing stopped when the limit was hit, not at an endgroup
  // tag.
  return input->DecrementRecursionDepthAndPopLimit(p.first);
}

}  // namespace

class MergePartialFromCodedStreamHelper {
 public:
  static MessageLite* Add(RepeatedPtrFieldBase* field,
                          const MessageLite* prototype) {
    return field->Add<RepeatedMessageTypeHandler>(
        const_cast<MessageLite*>(prototype));
  }
};

bool MergePartialFromCodedStream(MessageLite* msg, const ParseTable& table,
                                 io::CodedInputStream* input) {
  // We require that has_bits are present, as to avoid having to check for them
  // for every field.
  //
  // TODO(ckennelly):  Make this a compile-time parameter with templates.
  GOOGLE_DCHECK_GE(table.has_bits_offset, 0);
  uint32* has_bits = Raw<uint32>(msg, table.has_bits_offset);
  GOOGLE_DCHECK(has_bits != NULL);

  while (true) {
    uint32 tag = input->ReadTag();

    const WireFormatLite::WireType wire_type =
        WireFormatLite::GetTagWireType(tag);
    const int field_number = WireFormatLite::GetTagFieldNumber(tag);

    if (GOOGLE_PREDICT_FALSE(field_number > table.max_field_number)) {
      GOOGLE_DCHECK(!table.unknown_field_set);
      ::google::protobuf::io::StringOutputStream unknown_fields_string(
          MutableUnknownFields(msg, table.arena_offset));
      ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
          &unknown_fields_string, false);

      if (!::google::protobuf::internal::WireFormatLite::SkipField(
          input, tag, &unknown_fields_stream)) {
        return false;
      }

      continue;
    }

    // We implicitly verify that data points to a valid field as we check the
    // wire types.  Entries in table.fields[i] that do not correspond to valid
    // field numbers have their normal_wiretype and packed_wiretype fields set
    // with the kInvalidMask value.  As wire_type cannot take on that value, we
    // will never match.
    const ParseTableField* data = table.fields + field_number;

    // TODO(ckennelly): Avoid sign extension
    const int64 has_bit_index = data->has_bit_index;
    const int64 offset = data->offset;
    const unsigned char processing_type = data->processing_type;

    if (data->normal_wiretype == static_cast<unsigned char>(wire_type)) {
      // TODO(ckennelly): Use a computed goto on GCC/LLVM or otherwise eliminate
      // the bounds check on processing_type.

      switch (processing_type) {
#define STR(S) #S
#define HANDLE_TYPE(TYPE, CPPTYPE)                                        \
  case (WireFormatLite::TYPE_##TYPE): {                                   \
    CPPTYPE value;                                                        \
    if (GOOGLE_PREDICT_FALSE(                                                    \
            (!WireFormatLite::ReadPrimitive<                              \
                CPPTYPE, WireFormatLite::TYPE_##TYPE>(input, &value)))) { \
      return false;                                                       \
    }                                                                     \
    SetField(msg, has_bits, has_bit_index, offset, value);                \
    break;                                                                \
  }                                                                       \
  case (WireFormatLite::TYPE_##TYPE) | kRepeatedMask: {                   \
    google::protobuf::RepeatedField<CPPTYPE>* values =                              \
        Raw<google::protobuf::RepeatedField<CPPTYPE> >(msg, offset);                \
    if (GOOGLE_PREDICT_FALSE((!WireFormatLite::ReadRepeatedPrimitive<            \
                       CPPTYPE, WireFormatLite::TYPE_##TYPE>(             \
            data->tag_size, tag, input, values)))) {                      \
      return false;                                                       \
    }                                                                     \
    break;                                                                \
  }

        HANDLE_TYPE(INT32, int32)
        HANDLE_TYPE(INT64, int64)
        HANDLE_TYPE(SINT32, int32)
        HANDLE_TYPE(SINT64, int64)
        HANDLE_TYPE(UINT32, uint32)
        HANDLE_TYPE(UINT64, uint64)

        HANDLE_TYPE(FIXED32, uint32)
        HANDLE_TYPE(FIXED64, uint64)
        HANDLE_TYPE(SFIXED32, int32)
        HANDLE_TYPE(SFIXED64, int64)

        HANDLE_TYPE(FLOAT, float)
        HANDLE_TYPE(DOUBLE, double)

        HANDLE_TYPE(BOOL, bool)
#undef HANDLE_TYPE
#undef STR
        case WireFormatLite::TYPE_BYTES:
#ifndef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
        case WireFormatLite::TYPE_STRING:
#endif
        {
          GOOGLE_DCHECK(!table.unknown_field_set);
          Arena* const arena = GetArena(msg, table.arena_offset);
          const void* default_ptr = table.aux[field_number].strings.default_ptr;

          if (GOOGLE_PREDICT_FALSE((!HandleString<false, false, StringType_STRING>(
                  input, msg, arena, has_bits, has_bit_index, offset,
                  default_ptr, false, NULL)))) {
            return false;
          }
          break;
        }
        case (WireFormatLite::TYPE_BYTES) | kRepeatedMask:
#ifndef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
        case (WireFormatLite::TYPE_STRING) | kRepeatedMask:
#endif
        {
          GOOGLE_DCHECK(!table.unknown_field_set);
          Arena* const arena = GetArena(msg, table.arena_offset);
          const void* default_ptr =
              table.aux[field_number].strings.default_ptr;

          if (GOOGLE_PREDICT_FALSE((!HandleString<true, false, StringType_STRING>(
                  input, msg, arena, has_bits, has_bit_index, offset,
                  default_ptr, false, NULL)))) {
            return false;
          }
          break;
        }
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
        case (WireFormatLite::TYPE_STRING): {
          GOOGLE_DCHECK(!table.unknown_field_set);
          Arena* const arena = GetArena(msg, table.arena_offset);
          const void* default_ptr = table.aux[field_number].strings.default_ptr;
          const char* field_name = table.aux[field_number].strings.field_name;
          const bool strict_utf8 = table.aux[field_number].strings.strict_utf8;

          if (GOOGLE_PREDICT_FALSE((!HandleString<false, true, StringType_STRING>(
                  input, msg, arena, has_bits, has_bit_index, offset,
                  default_ptr, strict_utf8, field_name)))) {
            return false;
          }
          break;
        }
        case (WireFormatLite::TYPE_STRING) | kRepeatedMask: {
          GOOGLE_DCHECK(!table.unknown_field_set);
          Arena* const arena = GetArena(msg, table.arena_offset);
          const void* default_ptr = table.aux[field_number].strings.default_ptr;
          const char* field_name = table.aux[field_number].strings.field_name;
          const bool strict_utf8 = table.aux[field_number].strings.strict_utf8;

          if (GOOGLE_PREDICT_FALSE((!HandleString<true, true, StringType_STRING>(
                  input, msg, arena, has_bits, has_bit_index, offset,
                  default_ptr, strict_utf8, field_name)))) {
            return false;
          }
          break;
        }
#endif
        case WireFormatLite::TYPE_ENUM: {
          int value;
          if (GOOGLE_PREDICT_FALSE((!WireFormatLite::ReadPrimitive<
                             int, WireFormatLite::TYPE_ENUM>(input, &value)))) {
            return false;
          }

          AuxillaryParseTableField::EnumValidator validator =
              table.aux[field_number].enums.validator;
          if (validator(value)) {
            SetField(msg, has_bits, has_bit_index, offset, value);
          } else {
            GOOGLE_DCHECK(!table.unknown_field_set);

            ::google::protobuf::io::StringOutputStream unknown_fields_string(
                MutableUnknownFields(msg, table.arena_offset));
            ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
                &unknown_fields_string, false);
            unknown_fields_stream.WriteVarint32(tag);
            unknown_fields_stream.WriteVarint32(value);
          }
          break;
        }
        case WireFormatLite::TYPE_ENUM | kRepeatedMask: {
          int value;
          if (GOOGLE_PREDICT_FALSE((!WireFormatLite::ReadPrimitive<
                             int, WireFormatLite::TYPE_ENUM>(input, &value)))) {
            return false;
          }

          AuxillaryParseTableField::EnumValidator validator =
              table.aux[field_number].enums.validator;
          if (validator(value)) {
            AddField(msg, offset, value);
          } else {
            GOOGLE_DCHECK(!table.unknown_field_set);

            ::google::protobuf::io::StringOutputStream unknown_fields_string(
                MutableUnknownFields(msg, table.arena_offset));
            ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
                &unknown_fields_string, false);
            unknown_fields_stream.WriteVarint32(tag);
            unknown_fields_stream.WriteVarint32(value);
          }

          break;
        }
        case WireFormatLite::TYPE_GROUP: {
          MessageLite** submsg_holder =
              MutableField<MessageLite*>(msg, has_bits, has_bit_index, offset);
          MessageLite* submsg = *submsg_holder;

          if (submsg == NULL) {
            GOOGLE_DCHECK(!table.unknown_field_set);
            Arena* const arena = GetArena(msg, table.arena_offset);
            const MessageLite* prototype =
                table.aux[field_number].messages.default_message();
            submsg = prototype->New(arena);
            *submsg_holder = submsg;
          }

          const ParseTable* ptable =
              table.aux[field_number].messages.parse_table;

          if (ptable) {
            if (GOOGLE_PREDICT_FALSE(
                    !ReadGroup(field_number, input, submsg, *ptable))) {
              return false;
            }
          } else if (!WireFormatLite::ReadGroup(field_number, input, submsg)) {
            return false;
          }

          break;
        }
        case WireFormatLite::TYPE_GROUP | kRepeatedMask: {
          RepeatedPtrFieldBase* field = Raw<RepeatedPtrFieldBase>(msg, offset);
          const MessageLite* prototype =
              table.aux[field_number].messages.default_message();
          GOOGLE_DCHECK(prototype != NULL);

          MessageLite* submsg =
              MergePartialFromCodedStreamHelper::Add(field, prototype);
          const ParseTable* ptable =
              table.aux[field_number].messages.parse_table;

          if (ptable) {
            if (GOOGLE_PREDICT_FALSE(
                    !ReadGroup(field_number, input, submsg, *ptable))) {
              return false;
            }
          } else if (!WireFormatLite::ReadGroup(field_number, input, submsg)) {
            return false;
          }

          break;
        }
        case WireFormatLite::TYPE_MESSAGE: {
          MessageLite** submsg_holder =
              MutableField<MessageLite*>(msg, has_bits, has_bit_index, offset);
          MessageLite* submsg = *submsg_holder;

          if (submsg == NULL) {
            GOOGLE_DCHECK(!table.unknown_field_set);
            Arena* const arena = GetArena(msg, table.arena_offset);
            const MessageLite* prototype =
                table.aux[field_number].messages.default_message();
            submsg = prototype->New(arena);
            *submsg_holder = submsg;
          }

          const ParseTable* ptable =
              table.aux[field_number].messages.parse_table;

          if (ptable) {
            if (GOOGLE_PREDICT_FALSE(!ReadMessage(input, submsg, *ptable))) {
              return false;
            }
          } else if (!WireFormatLite::ReadMessage(input, submsg)) {
            return false;
          }

          break;
        }
        // TODO(ckennelly):  Adapt ReadMessageNoVirtualNoRecursionDepth and
        // manage input->IncrementRecursionDepth() here.
        case WireFormatLite::TYPE_MESSAGE | kRepeatedMask: {
          RepeatedPtrFieldBase* field = Raw<RepeatedPtrFieldBase>(msg, offset);
          const MessageLite* prototype =
              table.aux[field_number].messages.default_message();
          GOOGLE_DCHECK(prototype != NULL);

          MessageLite* submsg =
              MergePartialFromCodedStreamHelper::Add(field, prototype);
          const ParseTable* ptable =
              table.aux[field_number].messages.parse_table;

          if (ptable) {
            if (GOOGLE_PREDICT_FALSE(!ReadMessage(input, submsg, *ptable))) {
              return false;
            }
          } else if (!WireFormatLite::ReadMessage(input, submsg)) {
            return false;
          }

          break;
        }
        case 0: {
          // Done.
          return true;
        }
        default:
          break;
      }
    } else if (data->packed_wiretype == static_cast<unsigned char>(wire_type)) {
      // Non-packable fields have their packed_wiretype masked with
      // kNotPackedMask, which is impossible to match here.
      GOOGLE_DCHECK(processing_type & kRepeatedMask);
      GOOGLE_DCHECK_NE(processing_type, kRepeatedMask);



      // TODO(ckennelly): Use a computed goto on GCC/LLVM.
      //
      // Mask out kRepeatedMask bit, allowing the jump table to be smaller.
      switch (static_cast<WireFormatLite::FieldType>(
          processing_type ^ kRepeatedMask)) {
#define HANDLE_PACKED_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)                 \
  case WireFormatLite::TYPE_##TYPE: {                                     \
    google::protobuf::RepeatedField<CPPTYPE>* values =                              \
        Raw<google::protobuf::RepeatedField<CPPTYPE> >(msg, offset);                \
    if (GOOGLE_PREDICT_FALSE(                                                    \
            (!WireFormatLite::ReadPackedPrimitive<                        \
                CPPTYPE, WireFormatLite::TYPE_##TYPE>(input, values)))) { \
      return false;                                                       \
    }                                                                     \
    break;                                                                \
  }

        HANDLE_PACKED_TYPE(INT32, int32, Int32)
        HANDLE_PACKED_TYPE(INT64, int64, Int64)
        HANDLE_PACKED_TYPE(SINT32, int32, Int32)
        HANDLE_PACKED_TYPE(SINT64, int64, Int64)
        HANDLE_PACKED_TYPE(UINT32, uint32, UInt32)
        HANDLE_PACKED_TYPE(UINT64, uint64, UInt64)

        HANDLE_PACKED_TYPE(FIXED32, uint32, UInt32)
        HANDLE_PACKED_TYPE(FIXED64, uint64, UInt64)
        HANDLE_PACKED_TYPE(SFIXED32, int32, Int32)
        HANDLE_PACKED_TYPE(SFIXED64, int64, Int64)

        HANDLE_PACKED_TYPE(FLOAT, float, Float)
        HANDLE_PACKED_TYPE(DOUBLE, double, Double)

        HANDLE_PACKED_TYPE(BOOL, bool, Bool)
#undef HANDLE_PACKED_TYPE
        case WireFormatLite::TYPE_ENUM: {
          // To avoid unnecessarily calling MutableUnknownFields (which mutates
          // InternalMetadataWithArena) when all inputs in the repeated series
          // are valid, we implement our own parser rather than call
          // WireFormat::ReadPackedEnumPreserveUnknowns.
          uint32 length;
          if (GOOGLE_PREDICT_FALSE(!input->ReadVarint32(&length))) {
            return false;
          }

          AuxillaryParseTableField::EnumValidator validator =
              table.aux[field_number].enums.validator;
          google::protobuf::RepeatedField<int>* values =
              Raw<google::protobuf::RepeatedField<int> >(msg, offset);
          string* unknown_fields = NULL;

          io::CodedInputStream::Limit limit = input->PushLimit(length);
          while (input->BytesUntilLimit() > 0) {
            int value;
            if (GOOGLE_PREDICT_FALSE(
                    (!google::protobuf::internal::WireFormatLite::ReadPrimitive<
                        int, WireFormatLite::TYPE_ENUM>(input, &value)))) {
              return false;
            }

            if (validator(value)) {
              values->Add(value);
            } else {
              if (GOOGLE_PREDICT_FALSE(unknown_fields == NULL)) {
                GOOGLE_DCHECK(!table.unknown_field_set);
                unknown_fields = MutableUnknownFields(msg, table.arena_offset);
              }

              ::google::protobuf::io::StringOutputStream unknown_fields_string(
                  unknown_fields);
              ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
                  &unknown_fields_string, false);
              unknown_fields_stream.WriteVarint32(tag);
              unknown_fields_stream.WriteVarint32(value);
            }
          }
          input->PopLimit(limit);

          break;
        }
        case WireFormatLite::TYPE_STRING:
        case WireFormatLite::TYPE_GROUP:
        case WireFormatLite::TYPE_MESSAGE:
        case WireFormatLite::TYPE_BYTES:
          GOOGLE_DCHECK(false);
          return false;
        default:
          break;
      }
    } else {
      if (wire_type == WireFormatLite::WIRETYPE_END_GROUP) {
        // Must be the end of the message.
        return true;
      }

      // process unknown field.
      GOOGLE_DCHECK(!table.unknown_field_set);
      ::google::protobuf::io::StringOutputStream unknown_fields_string(
          MutableUnknownFields(msg, table.arena_offset));
      ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
          &unknown_fields_string, false);

      if (!::google::protobuf::internal::WireFormatLite::SkipField(
          input, tag, &unknown_fields_stream)) {
        return false;
      }
    }
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
