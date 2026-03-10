require 'google/protobuf'
require 'test/unit'

class BackendTest < Test::Unit::TestCase
  # Verifies the implementation of Protobuf is the preferred one.
  # See protobuf.rb for the logic that defines PREFER_FFI.
  def test_prefer_ffi_aligns_with_implementation
    expected = Google::Protobuf::PREFER_FFI ? :FFI : :NATIVE
    assert_equal expected, Google::Protobuf::IMPLEMENTATION
  end

  def test_prefer_ffi
    unless ENV['PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION'] =~ /ffi/i
      omit"FFI implementation requires environment variable PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION=FFI to activate."
    end
    assert_equal true, Google::Protobuf::PREFER_FFI
  end
  def test_ffi_implementation
    unless ENV['PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION'] =~ /ffi/i
      omit "FFI implementation requires environment variable PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION=FFI to activate."
    end
    assert_equal :FFI, Google::Protobuf::IMPLEMENTATION
  end

  def test_prefer_native
    if ENV.include?('PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION') and ENV['PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION'] !~ /native/i
      omit"Native implementation requires omitting environment variable PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION or setting it to `NATIVE` to activate."
    end
    assert_equal false, Google::Protobuf::PREFER_FFI
  end
  def test_native_implementation
    if ENV.include?('PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION') and ENV['PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION'] !~ /native/i
      omit"Native implementation requires omitting environment variable PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION or setting it to `NATIVE` to activate."
    end
    assert_equal :NATIVE, Google::Protobuf::IMPLEMENTATION
  end
end
