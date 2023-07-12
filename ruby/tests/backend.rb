require 'google/protobuf'
require 'test/unit'

# Verifies that the implementation of Protobuf is the expected (preferred) one.
# See protobuf.rb for the logic that defines PREFER_FFI.
class BackendTest < Test::Unit::TestCase
  EXPECTED_IMPLEMENTATION = Google::Protobuf::PREFER_FFI ? :FFI : :NATIVE
  def test_ffi_implementation
    assert_equal EXPECTED_IMPLEMENTATION, Google::Protobuf::IMPLEMENTATION
  end
end
