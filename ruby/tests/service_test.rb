#!/usr/bin/ruby

require 'google/protobuf'
require 'service_test_pb'
require 'test/unit'
require 'json'

class ServiceTest < Test::Unit::TestCase
  def setup
    @test_service = Google::Protobuf::DescriptorPool.generated_pool.lookup('service_test_protos.TestService')
    @deprecated_service = Google::Protobuf::DescriptorPool.generated_pool.lookup('service_test_protos.DeprecatedService')
    @test_service_methods = @test_service.to_h { |method| [method.name, method] }
  end

  def test_lookup_service_descriptor
    assert_kind_of Google::Protobuf::ServiceDescriptor, @test_service
    assert_equal 'service_test_protos.TestService', @test_service.name
  end

  def test_file_descriptor
    assert_kind_of Google::Protobuf::FileDescriptor, @test_service.file_descriptor
    assert_equal 'service_test.proto', @test_service.file_descriptor.name
  end

  def test_method_iteration
    @test_service.each do |method|
      assert_kind_of Google::Protobuf::MethodDescriptor, method
      assert_instance_of Google::Protobuf::MethodDescriptorProto, method.to_proto
    end
    assert_equal %w(UnaryOne UnaryTwo), @test_service.map { |method| method.name }.first(2)
  end

  def test_service_options
    assert @deprecated_service.options.deprecated
    refute @test_service.options.deprecated
  end

  def test_service_options_extensions
    omit "JRuby and FFI do not support service options extensions" if defined?(JRUBY_VERSION) || Google::Protobuf::IMPLEMENTATION != :NATIVE
    extension_field = Google::Protobuf::DescriptorPool.generated_pool.lookup('service_test_protos.test_options')
    assert_equal 8325, extension_field.get(@test_service.options).int_option_value
  end

  def test_service_to_proto
    assert_instance_of Google::Protobuf::ServiceDescriptorProto, @test_service.to_proto
  end

  def test_method_options
    assert_equal :IDEMPOTENT, @test_service_methods['IdempotentMethod'].options.idempotency_level
    assert_equal :NO_SIDE_EFFECTS, @test_service_methods['PureMethod'].options.idempotency_level
  end

  def test_method_input_type
    unary_request_type = Google::Protobuf::DescriptorPool.generated_pool.lookup('service_test_protos.UnaryRequestType')
    assert_same unary_request_type, @test_service_methods['UnaryOne'].input_type
  end

  def test_method_output_type
    unary_response_type = Google::Protobuf::DescriptorPool.generated_pool.lookup('service_test_protos.UnaryResponseType')
    assert_same unary_response_type, @test_service_methods['UnaryOne'].output_type
  end

  def test_method_streaming_flags
    refute @test_service_methods['UnaryOne'].client_streaming
    refute @test_service_methods['UnaryOne'].server_streaming
    assert @test_service_methods['StreamingMethod'].client_streaming
    assert @test_service_methods['StreamingMethod'].server_streaming
  end
end
