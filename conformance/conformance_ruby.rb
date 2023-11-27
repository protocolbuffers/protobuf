#!/usr/bin/env ruby
#
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

require 'conformance/conformance_pb'
require 'google/protobuf/test_messages_proto3_pb'
require 'google/protobuf/test_messages_proto2_pb'

$test_count = 0
$verbose = false

def do_test(request)
  test_message = ProtobufTestMessages::Proto3::TestAllTypesProto3.new
  response = Conformance::ConformanceResponse.new
  descriptor = Google::Protobuf::DescriptorPool.generated_pool.lookup(request.message_type)

  unless descriptor
    response.skipped = "Unknown message type: " + request.message_type
  end

  begin
    case request.payload
    when :protobuf_payload
      begin
        test_message = descriptor.msgclass.decode(request.protobuf_payload)
      rescue Google::Protobuf::ParseError => err
        response.parse_error = err.message.encode('utf-8')
        return response
      end

    when :json_payload
      begin
        options = {}
        if request.test_category == :JSON_IGNORE_UNKNOWN_PARSING_TEST
          options[:ignore_unknown_fields] = true
        end
        test_message = descriptor.msgclass.decode_json(request.json_payload, options)
      rescue Google::Protobuf::ParseError => err
        response.parse_error = err.message.encode('utf-8')
        return response
      end

    when :text_payload
      begin
        response.skipped = "Ruby doesn't support text format"
        return response
      end

    when nil
      fail "Request didn't have payload"
    end

    case request.requested_output_format
    when :UNSPECIFIED
      fail 'Unspecified output format'

    when :PROTOBUF
      begin
        response.protobuf_payload = test_message.to_proto
      rescue Google::Protobuf::ParseError => err
        response.serialize_error = err.message.encode('utf-8')
      end

    when :JSON
      begin
        response.json_payload = test_message.to_json
      rescue Google::Protobuf::ParseError => err
        response.serialize_error = err.message.encode('utf-8')
      end

    when nil
      fail "Request didn't have requested output format"
    end
  rescue StandardError => err
    response.runtime_error = err.message.encode('utf-8')
  end

  response
end

# Returns true if the test ran successfully, false on legitimate EOF.
# If EOF is encountered in an unexpected place, raises IOError.
def do_test_io
  length_bytes = STDIN.read(4)
  return false if length_bytes.nil?

  length = length_bytes.unpack('V').first
  serialized_request = STDIN.read(length)
  if serialized_request.nil? || serialized_request.length != length
    fail IOError
  end

  request = Conformance::ConformanceRequest.decode(serialized_request)

  response = do_test(request)

  serialized_response = Conformance::ConformanceResponse.encode(response)
  STDOUT.write([serialized_response.length].pack('V'))
  STDOUT.write(serialized_response)
  STDOUT.flush

  if $verbose
    STDERR.puts("conformance_ruby: request=#{request.to_json}, " \
                                 "response=#{response.to_json}\n")
  end

  $test_count += 1

  true
end

loop do
  unless do_test_io
    STDERR.puts('conformance_ruby: received EOF from test runner ' \
                "after #{$test_count} tests, exiting")
    break
  end
end
