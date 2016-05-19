#!/usr/bin/ruby

require 'google/protobuf'
require 'test/unit'

class WellKnownProtobufRequireTest < Test::Unit::TestCase
  def test_any_require
    assert_nothing_raised LoadError do
      require "google/protobuf/any"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::Any
    end
  end

  def test_api_require
    assert_nothing_raised LoadError do
      require "google/protobuf/api"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::Api
      Google::Protobuf::Method
      Google::Protobuf::Mixin
    end
  end

  def test_duration_require
    assert_nothing_raised LoadError do
      require "google/protobuf/duration"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::Duration
    end
  end

  def test_empty_require
    assert_nothing_raised LoadError do
      require "google/protobuf/empty"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::Empty
    end
  end

  def test_field_mask_require
    assert_nothing_raised LoadError do
      require "google/protobuf/field_mask"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::FieldMask
    end
  end

  def test_source_context_require
    assert_nothing_raised LoadError do
      require "google/protobuf/source_context"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::SourceContext
    end
  end

  def test_struct_require
    assert_nothing_raised LoadError do
      require "google/protobuf/struct"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::Struct
      Google::Protobuf::Value
      Google::Protobuf::ListValue
      Google::Protobuf::NullValue
    end
  end

  def test_timestamp_require
    assert_nothing_raised LoadError do
      require "google/protobuf/timestamp"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::Timestamp
    end
  end

  def test_type_require
    assert_nothing_raised LoadError do
      require "google/protobuf/type"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::Type
      Google::Protobuf::Field
      Google::Protobuf::Field::Kind
      Google::Protobuf::Field::Cardinality
      Google::Protobuf::Enum
      Google::Protobuf::EnumValue
      Google::Protobuf::Option
      Google::Protobuf::Syntax
    end
  end

  def test_wrappers_require
    assert_nothing_raised LoadError do
      require "google/protobuf/wrappers"
    end
    assert_nothing_raised NameError do
      Google::Protobuf::DoubleValue
      Google::Protobuf::FloatValue
      Google::Protobuf::Int64Value
      Google::Protobuf::UInt64Value
      Google::Protobuf::Int32Value
      Google::Protobuf::UInt32Value
      Google::Protobuf::BoolValue
      Google::Protobuf::StringValue
      Google::Protobuf::BytesValue
    end
  end

  # These files should be included, so any proto that includes them will still
  # work after generating the ruby code.
  # Disabled for now since these files are not currently included. See #1481.
  # def test_descriptor_require
  #   assert_nothing_raised LoadError do
  #     require "google/protobuf/descriptor"
  #   end
  # end
  #
  # def test_compiler_plugin_require
  #   assert_nothing_raised LoadError do
  #     require "google/protobuf/compiler/plugin"
  #   end
  # end
end
