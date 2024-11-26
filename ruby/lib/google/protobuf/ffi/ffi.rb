# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

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
      Upb_Status_MaxMessage = 511
      Upb_Encode_Deterministic = 1
      Upb_Encode_SkipUnknown = 2

      ## JSON Encoding options
      # When set, emits 0/default values.  TODO: proto3 only?
      Upb_JsonEncode_EmitDefaults = 1
      # When set, use normal (snake_case) field names instead of JSON (camelCase) names.
      Upb_JsonEncode_UseProtoNames = 2
      # When set, emits enums as their integer values instead of as their names.
      Upb_JsonEncode_FormatEnumsAsIntegers = 4

      ## JSON Decoding options
      Upb_JsonDecode_IgnoreUnknown = 1

      ## JSON Decoding results
      Upb_JsonDecodeResult_Ok = 0
      Upb_JsonDecodeResult_OkWithEmptyStringNumerics = 1
      Upb_JsonDecodeResult_Error = 2

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

      Upb_Message_Begin = -1

      class MutableMessageValue < ::FFI::Union
        layout :map, :Map,
               :msg, :Message,
               :array, :Array
      end

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
