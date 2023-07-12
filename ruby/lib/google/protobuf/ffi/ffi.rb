# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

module Google
  module Protobuf
    class FFI
      extend ::FFI::Library
      # Workaround for Bazel's use of symlinks + JRuby's __FILE__ and `caller`
      # that resolves them.
      if ENV['BAZEL'] == 'true'
        ffi_lib ::FFI::Compiler::Loader.find 'protobuf_c_ffi', ENV['PWD']
      else
        ffi_lib ::FFI::Compiler::Loader.find 'protobuf_c_ffi'
      end

      ## Map
      Upb_Map_Begin = -1

      ## Encoding Status
      Upb_Status_MaxMessage = 127
      Upb_Encode_Deterministic = 1
      Upb_Encode_SkipUnknown = 2

      ## JSON Encoding options
      # When set, emits 0/default values.  TODO(haberman): proto3 only?
      Upb_JsonEncode_EmitDefaults = 1
      # When set, use normal (snake_case) field names instead of JSON (camelCase) names.
      Upb_JsonEncode_UseProtoNames = 2
      # When set, emits enums as their integer values instead of as their names.
      Upb_JsonEncode_FormatEnumsAsIntegers = 4

      ## JSON Decoding options
      Upb_JsonDecode_IgnoreUnknown = 1

      Arena = Google::Protobuf::Internal::Arena
      MessageDef = Google::Protobuf::Descriptor
      EnumDef = Google::Protobuf::EnumDescriptor
      FieldDef = Google::Protobuf::FieldDescriptor
      OneofDef = Google::Protobuf::OneofDescriptor

      typedef :pointer, :Array
      typedef :pointer, :DefPool
      typedef :pointer, :EnumValueDef
      typedef :pointer, :ExtensionRegistry
      typedef :pointer, :FieldDefPointer
      typedef :pointer, :FileDef
      typedef :pointer, :FileDescriptorProto
      typedef :pointer, :Map
      typedef :pointer, :Message    # Instances of a message
      typedef :pointer, :OneofDefPointer
      typedef :pointer, :binary_string
      if ::FFI::Platform::ARCH == "aarch64"
        typedef :u_int8_t, :uint8_t
        typedef :u_int16_t, :uint16_t
        typedef :u_int32_t, :uint32_t
        typedef :u_int64_t, :uint64_t
      end

      FieldType = enum(
        :double, 1,
        :float,
        :int64,
        :uint64,
        :int32,
        :fixed64,
        :fixed32,
        :bool,
        :string,
        :group,
        :message,
        :bytes,
        :uint32,
        :enum,
        :sfixed32,
        :sfixed64,
        :sint32,
        :sint64
      )

      CType = enum(
        :bool, 1,
        :float,
        :int32,
        :uint32,
        :enum,
        :message,
        :double,
        :int64,
        :uint64,
        :string,
        :bytes
      )

      Label = enum(
        :optional, 1,
        :required,
        :repeated
      )

      Syntax = enum(
        :Proto2, 2,
        :Proto3
      )

      # All the different kind of well known type messages. For simplicity of check,
      # number wrappers and string wrappers are grouped together. Make sure the
      # order and merber of these groups are not changed.

      WellKnown = enum(
        :Unspecified,
        :Any,
        :FieldMask,
        :Duration,
        :Timestamp,
        # number wrappers
        :DoubleValue,
        :FloatValue,
        :Int64Value,
        :UInt64Value,
        :Int32Value,
        :UInt32Value,
        # string wrappers
        :StringValue,
        :BytesValue,
        :BoolValue,
        :Value,
        :ListValue,
        :Struct
      )

      DecodeStatus = enum(
        :Ok,
        :Malformed,         # Wire format was corrupt
        :OutOfMemory,       # Arena alloc failed
        :BadUtf8,           # String field had bad UTF-8
        :MaxDepthExceeded,  # Exceeded UPB_DECODE_MAXDEPTH

        # CheckRequired failed, but the parse otherwise succeeded.
        :MissingRequired,
      )

      EncodeStatus = enum(
        :Ok,
        :OutOfMemory,       # Arena alloc failed
        :MaxDepthExceeded,  # Exceeded UPB_DECODE_MAXDEPTH

        # CheckRequired failed, but the parse otherwise succeeded.
        :MissingRequired,
      )

      class StringView < ::FFI::Struct
        layout :data, :pointer,
               :size, :size_t
      end

      class MiniTable < ::FFI::Struct
        layout :subs, :pointer,
               :fields, :pointer,
               :size, :uint16_t,
               :field_count, :uint16_t,
               :ext, :uint8_t,  # upb_ExtMode, declared as uint8_t so sizeof(ext) == 1
               :dense_below, :uint8_t,
               :table_mask, :uint8_t,
               :required_count, :uint8_t  # Required fields have the lowest hasbits.
        # To statically initialize the tables of variable length, we need a flexible
        # array member, and we need to compile in gnu99 mode (constant initialization
        # of flexible array members is a GNU extension, not in C99 unfortunately. */
        #       _upb_FastTable_Entry fasttable[];
      end

      class Status < ::FFI::Struct
        layout :ok, :bool,
               :msg, [:char, Upb_Status_MaxMessage]

        def initialize
          super
          FFI.clear self
        end
      end

      class MessageValue < ::FFI::Union
        layout :bool_val, :bool,
               :float_val, :float,
               :double_val, :double,
               :int32_val, :int32_t,
               :int64_val, :int64_t,
               :uint32_val, :uint32_t,
               :uint64_val,:uint64_t,
               :map_val, :pointer,
               :msg_val, :pointer,
               :array_val,:pointer,
               :str_val, StringView
      end

      class MutableMessageValue < ::FFI::Union
        layout :map, :Map,
               :msg, :Message,
               :array, :Array
      end

      # Arena
      attach_function :create_arena, :Arena_create,         [], Arena
      attach_function :fuse_arena,   :upb_Arena_Fuse,       [Arena, Arena], :bool
      # Argument takes a :pointer rather than a typed Arena here due to
      # implementation details of FFI::AutoPointer.
      attach_function :free_arena,   :upb_Arena_Free,       [:pointer], :void
      attach_function :arena_malloc, :upb_Arena_Malloc,     [Arena, :size_t], :pointer

      # Array
      attach_function :append_array, :upb_Array_Append,        [:Array, MessageValue.by_value, Arena], :bool
      attach_function :get_msgval_at,:upb_Array_Get,           [:Array, :size_t], MessageValue.by_value
      attach_function :create_array, :upb_Array_New,           [Arena, CType], :Array
      attach_function :array_resize, :upb_Array_Resize,        [:Array, :size_t, Arena], :bool
      attach_function :array_set,    :upb_Array_Set,           [:Array, :size_t, MessageValue.by_value], :void
      attach_function :array_size,   :upb_Array_Size,          [:Array], :size_t

      # DefPool
      attach_function :add_serialized_file,   :API_PENDING_upb_DefPool_AddFile,           [:DefPool, :FileDescriptorProto, Status.by_ref], :FileDef
      attach_function :free_descriptor_pool,  :API_PENDING_upb_DefPool_Free,              [:DefPool], :void
      attach_function :create_descriptor_pool,:API_PENDING_upb_DefPool_New,               [], :DefPool
      attach_function :lookup_enum,           :API_PENDING_upb_DefPool_FindEnumByName,    [:DefPool, :string], EnumDef
      attach_function :lookup_msg,            :API_PENDING_upb_DefPool_FindMessageByName, [:DefPool, :string], MessageDef

      # EnumDescriptor
      attach_function :get_enum_file_descriptor,  :API_PENDING_upb_EnumDef_File,                   [EnumDef], :FileDef
      attach_function :enum_value_by_name,        :API_PENDING_upb_EnumDef_FindValueByNameWithSize,[EnumDef, :string, :size_t], :EnumValueDef
      attach_function :enum_value_by_number,      :API_PENDING_upb_EnumDef_FindValueByNumber,      [EnumDef, :int], :EnumValueDef
      attach_function :get_enum_fullname,         :API_PENDING_upb_EnumDef_FullName,               [EnumDef], :string
      attach_function :enum_value_by_index,       :API_PENDING_upb_EnumDef_Value,                  [EnumDef, :int], :EnumValueDef
      attach_function :enum_value_count,          :API_PENDING_upb_EnumDef_ValueCount,             [EnumDef], :int
      attach_function :enum_name,                 :API_PENDING_upb_EnumValueDef_Name,              [:EnumValueDef], :string
      attach_function :enum_number,               :API_PENDING_upb_EnumValueDef_Number,            [:EnumValueDef], :int

      # FileDescriptor
      attach_function :file_def_name,   :API_PENDING_upb_FileDef_Name,   [:FileDef], :string
      attach_function :file_def_syntax, :API_PENDING_upb_FileDef_Syntax, [:FileDef], Syntax
      attach_function :file_def_pool,   :API_PENDING_upb_FileDef_Pool,   [:FileDef], :DefPool

      # FileDescriptorProto
      attach_function :parse,                 :FileDescriptorProto_parse, [:binary_string, :size_t], :FileDescriptorProto

      # FieldDescriptor
      attach_function :get_containing_message_def, :API_PENDING_upb_FieldDef_ContainingType,     [FieldDescriptor], MessageDef
      attach_function :get_c_type,                 :API_PENDING_upb_FieldDef_CType,              [FieldDescriptor], CType
      attach_function :get_default,                :API_PENDING_upb_FieldDef_Default,            [FieldDescriptor], MessageValue.by_value
      attach_function :get_subtype_as_enum,        :API_PENDING_upb_FieldDef_EnumSubDef,         [FieldDescriptor], EnumDef
      attach_function :get_has_presence,           :API_PENDING_upb_FieldDef_HasPresence,        [FieldDescriptor], :bool
      attach_function :is_map,                     :API_PENDING_upb_FieldDef_IsMap,              [FieldDescriptor], :bool
      attach_function :is_repeated,                :API_PENDING_upb_FieldDef_IsRepeated,         [FieldDescriptor], :bool
      attach_function :is_sub_message,             :API_PENDING_upb_FieldDef_IsSubMessage,       [FieldDescriptor], :bool
      attach_function :get_json_name,              :API_PENDING_upb_FieldDef_JsonName,           [FieldDescriptor], :string
      attach_function :get_label,                  :API_PENDING_upb_FieldDef_Label,              [FieldDescriptor], Label
      attach_function :get_subtype_as_message,     :API_PENDING_upb_FieldDef_MessageSubDef,      [FieldDescriptor], MessageDef
      attach_function :get_full_name,              :API_PENDING_upb_FieldDef_Name,               [FieldDescriptor], :string
      attach_function :get_number,                 :API_PENDING_upb_FieldDef_Number,             [FieldDescriptor], :uint32_t
      attach_function :real_containing_oneof,      :API_PENDING_upb_FieldDef_RealContainingOneof,[FieldDescriptor], OneofDef
      attach_function :get_type,                   :API_PENDING_upb_FieldDef_Type,               [FieldDescriptor], FieldType
      attach_function :file_def_by_raw_field_def,  :API_PENDING_upb_FieldDef_File,               [:pointer], :FileDef
      # Map
      attach_function :map_clear,  :upb_Map_Clear,                    [:Map], :void
      attach_function :map_delete, :upb_Map_Delete,                   [:Map, MessageValue.by_value, MessageValue.by_ref], :bool
      attach_function :map_get,    :upb_Map_Get,                      [:Map, MessageValue.by_value, MessageValue.by_ref], :bool
      attach_function :create_map, :upb_Map_New,                      [Arena, CType, CType], :Map
      attach_function :map_size,   :upb_Map_Size,                     [:Map], :size_t
      attach_function :map_set,    :upb_Map_Set,                      [:Map, MessageValue.by_value, MessageValue.by_value, Arena], :bool

      # MapIterator
      attach_function :map_next,   :API_PENDING_upb_MapIterator_Next,             [:Map, :pointer], :bool
      attach_function :map_done,   :API_PENDING_upb_MapIterator_Done,             [:Map, :size_t], :bool
      attach_function :map_key,    :API_PENDING_upb_MapIterator_Key,              [:Map, :size_t], MessageValue.by_value
      attach_function :map_value,  :API_PENDING_upb_MapIterator_Value,            [:Map, :size_t], MessageValue.by_value

      # MessageDef
      attach_function :new_message_from_def, :upb_Message_New,                        [MessageDef, Arena], :Message
      attach_function :get_field_by_index,   :API_PENDING_upb_MessageDef_Field,                   [MessageDef, :int], FieldDescriptor
      attach_function :field_count,          :API_PENDING_upb_MessageDef_FieldCount,              [MessageDef], :int
      attach_function :get_message_file_def, :API_PENDING_upb_MessageDef_File,                    [:pointer], :FileDef
      attach_function :get_field_by_name,    :API_PENDING_upb_MessageDef_FindFieldByNameWithSize, [MessageDef, :string, :size_t], FieldDescriptor
      attach_function :get_field_by_number,  :API_PENDING_upb_MessageDef_FindFieldByNumber,       [MessageDef, :uint32_t], FieldDescriptor
      attach_function :get_oneof_by_name,    :API_PENDING_upb_MessageDef_FindOneofByNameWithSize, [MessageDef, :string, :size_t], OneofDef
      attach_function :get_message_fullname, :API_PENDING_upb_MessageDef_FullName,                [MessageDef], :string
      attach_function :get_mini_table,       :API_PENDING_upb_MessageDef_MiniTable,               [MessageDef], MiniTable.ptr
      attach_function :get_oneof_by_index,   :API_PENDING_upb_MessageDef_Oneof,                   [MessageDef, :int], OneofDef
      attach_function :oneof_count,          :API_PENDING_upb_MessageDef_OneofCount,              [MessageDef], :int
      attach_function :get_well_known_type,  :API_PENDING_upb_MessageDef_WellKnownType,           [MessageDef], WellKnown
      attach_function :message_def_syntax,   :API_PENDING_upb_MessageDef_Syntax,                  [MessageDef], Syntax
      attach_function :find_msg_def_by_name, :API_PENDING_upb_MessageDef_FindByNameWithSize,      [MessageDef, :string, :size_t, :FieldDefPointer, :OneofDefPointer], :bool

      # Message
      attach_function :clear_message_field,     :API_PENDING_upb_Message_ClearFieldByDef, [:Message, FieldDescriptor], :void
      attach_function :get_message_value,       :API_PENDING_upb_Message_GetFieldByDef,   [:Message, FieldDescriptor], MessageValue.by_value
      attach_function :get_message_has,         :API_PENDING_upb_Message_HasFieldByDef,   [:Message, FieldDescriptor], :bool
      attach_function :set_message_field,       :API_PENDING_upb_Message_SetFieldByDef,   [:Message, FieldDescriptor, MessageValue.by_value, Arena], :bool
      attach_function :encode_message,          :API_PENDING_upb_Encode,                  [:Message, MiniTable.by_ref, :size_t, Arena, :pointer, :pointer], EncodeStatus
      attach_function :json_decode_message,     :API_PENDING_upb_JsonDecode,              [:binary_string, :size_t, :Message, MessageDef, :DefPool, :int, Arena, Status.by_ref], :bool
      attach_function :json_encode_message,     :API_PENDING_upb_JsonEncode,              [:Message, MessageDef, :DefPool, :int, :binary_string, :size_t, Status.by_ref], :size_t
      attach_function :decode_message,          :upb_Decode,                  [:binary_string, :size_t, :Message, MiniTable.by_ref, :ExtensionRegistry, :int, Arena], DecodeStatus
      attach_function :get_mutable_message,     :API_PENDING_upb_Message_Mutable,         [:Message, FieldDescriptor, Arena], MutableMessageValue.by_value
      attach_function :get_message_which_oneof, :API_PENDING_upb_Message_WhichOneof,      [:Message, OneofDef], FieldDescriptor
      attach_function :message_discard_unknown, :API_PENDING_upb_Message_DiscardUnknown,  [:Message, MessageDef, :int], :bool

      # MessageValue
      attach_function :message_value_equal,     :shared_Msgval_IsEqual,       [MessageValue.by_value, MessageValue.by_value, CType, MessageDef], :bool
      attach_function :message_value_hash,      :shared_Msgval_GetHash,       [MessageValue.by_value, CType, MessageDef, :uint64_t], :uint64_t

      # OneofDescriptor
      attach_function :get_oneof_name,           :API_PENDING_upb_OneofDef_Name,          [OneofDef], :string
      attach_function :get_oneof_field_count,    :API_PENDING_upb_OneofDef_FieldCount,    [OneofDef], :int
      attach_function :get_oneof_field_by_index, :API_PENDING_upb_OneofDef_Field,         [OneofDef, :int], FieldDescriptor
      attach_function :get_oneof_containing_type,:API_PENDING_upb_OneofDef_ContainingType,[:pointer], MessageDef

      # RepeatableField

      # Status
      attach_function :clear,                 :upb_Status_Clear, [Status.by_ref], :void
      attach_function :error_message,         :upb_Status_ErrorMessage, [Status.by_ref], :string

      # Generic
      attach_function :memcmp, [:pointer, :pointer, :size_t], :int
      attach_function :memcpy, [:pointer, :pointer, :size_t], :int

      # Alternatives to pre-processor macros
      def self.decode_max_depth(i)
        i << 16
      end
    end
  end
end
