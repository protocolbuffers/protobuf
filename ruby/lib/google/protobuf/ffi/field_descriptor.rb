# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    class FieldDescriptor
      attr :field_def, :descriptor_pool

      include Google::Protobuf::Internal::Convert

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety
        include Google::Protobuf::Internal::PointerHelper

        # @param value [FieldDescriptor] FieldDescriptor to convert to an FFI native type
        # @param _ [Object] Unused
        def to_native(value, _)
          field_def_ptr = value.instance_variable_get(:@field_def)
          warn "Underlying field_def was nil!" if field_def_ptr.nil?
          raise "Underlying field_def was null!" if !field_def_ptr.nil? and field_def_ptr.null?
          field_def_ptr
        end

        ##
        # @param field_def [::FFI::Pointer] FieldDef pointer to be wrapped
        # @param _ [Object] Unused
        def from_native(field_def, _ = nil)
          return nil if field_def.nil? or field_def.null?
          file_def = Google::Protobuf::FFI.file_def_by_raw_field_def(field_def)
          descriptor_from_file_def(file_def, field_def)
        end
      end

      def self.new(*arguments, &block)
        raise "Descriptor objects may not be created from Ruby."
      end

      def to_s
        inspect
      end

      def inspect
        "#{self.class.name}: #{name}"
      end

      def name
        @name ||= Google::Protobuf::FFI.get_full_name(self)
      end

      def json_name
        @json_name ||= Google::Protobuf::FFI.get_json_name(self)
      end

      def number
        @number ||= Google::Protobuf::FFI.get_number(self)
      end

      def type
        @type ||= Google::Protobuf::FFI.get_type(self)
      end

      def label
        @label ||= Google::Protobuf::FFI::Label[Google::Protobuf::FFI.get_label(self)]
      end

      def default
        return nil if Google::Protobuf::FFI.is_sub_message(self)
        if Google::Protobuf::FFI.is_repeated(self)
          message_value = Google::Protobuf::FFI::MessageValue.new
        else
          message_value = Google::Protobuf::FFI.get_default(self)
        end
        enum_def = Google::Protobuf::FFI.get_subtype_as_enum(self)
        if enum_def.null?
          convert_upb_to_ruby message_value, c_type
        else
          convert_upb_to_ruby message_value, c_type, enum_def
        end
      end

      def submsg_name
        if defined? @submsg_name
          @submsg_name
        else
          @submsg_name = case c_type
            when :enum
              Google::Protobuf::FFI.get_enum_fullname Google::Protobuf::FFI.get_subtype_as_enum self
            when :message
              Google::Protobuf::FFI.get_message_fullname Google::Protobuf::FFI.get_subtype_as_message self
            else
              nil
            end
        end
      end

      ##
      # Tests if this field has been set on the argument message.
      #
      # @param msg [Google::Protobuf::Message]
      # @return [Object] Value of the field on this message.
      # @raise [TypeError] If the field is not defined on this message.
      def get(msg)
        if msg.class.descriptor == Google::Protobuf::FFI.get_containing_message_def(self)
          msg.send :get_field, self
        else
          raise TypeError.new "get method called on wrong message type"
        end
      end

      def subtype
        if defined? @subtype
          @subtype
        else
          @subtype = case c_type
            when :enum
              Google::Protobuf::FFI.get_subtype_as_enum(self)
            when :message
              Google::Protobuf::FFI.get_subtype_as_message(self)
            else
              nil
            end
        end
      end

      ##
      # Tests if this field has been set on the argument message.
      #
      # @param msg [Google::Protobuf::Message]
      # @return [Boolean] True iff message has this field set
      # @raise [TypeError] If this field does not exist on the message
      # @raise [ArgumentError] If this field does not track presence
      def has?(msg)
        if msg.class.descriptor != Google::Protobuf::FFI.get_containing_message_def(self)
          raise TypeError.new "has method called on wrong message type"
        end
        unless has_presence?
          raise ArgumentError.new "does not track presence"
        end

        Google::Protobuf::FFI.get_message_has msg.instance_variable_get(:@msg), self
      end

      ##
      # Tests if this field tracks presence.
      #
      # @return [Boolean] True iff this field tracks presence
      def has_presence?
        @has_presence ||= Google::Protobuf::FFI.get_has_presence(self)
      end

      # @param msg [Google::Protobuf::Message]
      def clear(msg)
        if msg.class.descriptor != Google::Protobuf::FFI.get_containing_message_def(self)
          raise TypeError.new "clear method called on wrong message type"
        end
        Google::Protobuf::FFI.clear_message_field msg.instance_variable_get(:@msg), self
        nil
      end

      ##
      # call-seq:
      #     FieldDescriptor.set(message, value)
      #
      # Sets the value corresponding to this field to the given value on the given
      # message. Raises an exception if message is of the wrong type. Performs the
      # ordinary type-checks for field setting.
      #
      # @param msg [Google::Protobuf::Message]
      # @param value [Object]
      def set(msg, value)
        if msg.class.descriptor != Google::Protobuf::FFI.get_containing_message_def(self)
          raise TypeError.new "set method called on wrong message type"
        end
        unless set_value_on_message value, msg.instance_variable_get(:@msg), msg.instance_variable_get(:@arena)
          raise RuntimeError.new "allocation failed"
        end
        nil
      end

      def map?
        @map ||= Google::Protobuf::FFI.is_map self
      end

      def repeated?
        @repeated ||= Google::Protobuf::FFI.is_repeated self
      end

      def sub_message?
        @sub_message ||= Google::Protobuf::FFI.is_sub_message self
      end

      def wrapper?
        if defined? @wrapper
          @wrapper
        else
          message_descriptor = Google::Protobuf::FFI.get_subtype_as_message(self)
          @wrapper = message_descriptor.nil? ? false : message_descriptor.send(:wrapper?)
        end
      end

      def options
        @options ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.field_options(self, size_ptr, temporary_arena)
          Google::Protobuf::FieldOptions.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze).send(:internal_deep_freeze)
        end
      end

      private

      def initialize(field_def, descriptor_pool)
        @field_def = field_def
        @descriptor_pool = descriptor_pool
      end

      def self.private_constructor(field_def, descriptor_pool)
        instance = allocate
        instance.send(:initialize, field_def, descriptor_pool)
        instance
      end

      # TODO Can this be added to the public API?
      def real_containing_oneof
        @real_containing_oneof ||= Google::Protobuf::FFI.real_containing_oneof self
      end

      # Implementation details below are subject to breaking changes without
      # warning and are intended for use only within the gem.

      ##
      # Sets the field this FieldDescriptor represents to the given value on the given message.
      # @param value [Object] Value to be set
      # @param msg [::FFI::Pointer] Pointer the the upb_Message
      # @param arena [Arena] Arena of the message that owns msg
      def set_value_on_message(value, msg, arena, wrap: false)
        message_to_alter = msg
        field_def_to_set = self
        if map?
          raise TypeError.new "Expected map" unless value.is_a? Google::Protobuf::Map
          message_descriptor = subtype

          key_field_def = Google::Protobuf::FFI.get_field_by_number(message_descriptor, 1)
          key_field_type = Google::Protobuf::FFI.get_type(key_field_def)
          raise TypeError.new "Map key type does not match field's key type" unless key_field_type == value.send(:key_type)

          value_field_def = Google::Protobuf::FFI.get_field_by_number(message_descriptor, 2)
          value_field_type = Google::Protobuf::FFI.get_type(value_field_def)
          raise TypeError.new "Map value type does not match field's value type" unless value_field_type == value.send(:value_type)

          raise TypeError.new "Map value type has wrong message/enum class" unless value_field_def.subtype == value.send(:descriptor)

          arena.fuse(value.send(:arena))
          message_value = Google::Protobuf::FFI::MessageValue.new
          message_value[:map_val] = value.send(:map_ptr)
        elsif repeated?
          raise TypeError.new "Expected repeated field array" unless value.is_a? RepeatedField
          raise TypeError.new "Repeated field array has wrong message/enum class" unless value.send(:type) == type
          arena.fuse(value.send(:arena))
          message_value = Google::Protobuf::FFI::MessageValue.new
          message_value[:array_val] = value.send(:array)
        else
          if value.nil? and (sub_message? or !real_containing_oneof.nil?)
            Google::Protobuf::FFI.clear_message_field message_to_alter, field_def_to_set
            return true
          end
          if wrap
            value_field_def = Google::Protobuf::FFI.get_field_by_number subtype, 1
            type_for_conversion = Google::Protobuf::FFI.get_c_type(value_field_def)
            raise RuntimeError.new "Not expecting to get a msg or enum when unwrapping" if [:enum, :message].include? type_for_conversion
            message_value = convert_ruby_to_upb(value, arena, type_for_conversion, nil)
            message_to_alter = Google::Protobuf::FFI.get_mutable_message(msg, self, arena)[:msg]
            field_def_to_set = value_field_def
          else
            message_value = convert_ruby_to_upb(value, arena, c_type, subtype)
          end
        end
        Google::Protobuf::FFI.set_message_field message_to_alter, field_def_to_set, message_value, arena
      end

      def c_type
        @c_type ||= Google::Protobuf::FFI.get_c_type(self)
      end
    end

    class FFI
      # MessageDef
      attach_function :get_field_by_index,   :upb_MessageDef_Field,                  [Descriptor, :int], FieldDescriptor
      attach_function :get_field_by_name,    :upb_MessageDef_FindFieldByNameWithSize,[Descriptor, :string, :size_t], FieldDescriptor
      attach_function :get_field_by_number,  :upb_MessageDef_FindFieldByNumber,      [Descriptor, :uint32_t], FieldDescriptor

      # FieldDescriptor
      attach_function :field_options,              :FieldDescriptor_serialized_options, [FieldDescriptor, :pointer, Internal::Arena], :pointer
      attach_function :get_containing_message_def, :upb_FieldDef_ContainingType,        [FieldDescriptor], Descriptor
      attach_function :get_c_type,                 :upb_FieldDef_CType,                 [FieldDescriptor], CType
      attach_function :get_default,                :upb_FieldDef_Default,               [FieldDescriptor], MessageValue.by_value
      attach_function :get_subtype_as_enum,        :upb_FieldDef_EnumSubDef,            [FieldDescriptor], EnumDescriptor
      attach_function :get_has_presence,           :upb_FieldDef_HasPresence,           [FieldDescriptor], :bool
      attach_function :is_map,                     :upb_FieldDef_IsMap,                 [FieldDescriptor], :bool
      attach_function :is_repeated,                :upb_FieldDef_IsRepeated,            [FieldDescriptor], :bool
      attach_function :is_sub_message,             :upb_FieldDef_IsSubMessage,          [FieldDescriptor], :bool
      attach_function :get_json_name,              :upb_FieldDef_JsonName,              [FieldDescriptor], :string
      attach_function :get_label,                  :upb_FieldDef_Label,                 [FieldDescriptor], Label
      attach_function :get_subtype_as_message,     :upb_FieldDef_MessageSubDef,         [FieldDescriptor], Descriptor
      attach_function :get_full_name,              :upb_FieldDef_Name,                  [FieldDescriptor], :string
      attach_function :get_number,                 :upb_FieldDef_Number,                [FieldDescriptor], :uint32_t
      attach_function :get_type,                   :upb_FieldDef_Type,                  [FieldDescriptor], FieldType
      attach_function :file_def_by_raw_field_def,  :upb_FieldDef_File,                  [:pointer], :FileDef
    end
  end
end
