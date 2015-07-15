#!/usr/bin/env ruby
#
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
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

require 'conformance'

test_count = 0;
verbose = false;

def do_test(request)
  test_message = Conformance::TestAllTypes.new
  response = Conformance::ConformanceResponse.new

  begin
    case request.payload
    when :protobuf_payload
      begin
        test_message = Conformance::TestAllTypes.decode(request.protobuf_payload)
      rescue Google::Protobuf::ParseError => err
        response.parse_error = err.message.encode("utf-8")
        return response
      end

    when :json_payload
      test_message = Conformance::TestAllTypes.decode_json(request.json_payload)

    when nil
      raise "Request didn't have payload.";
    end

    case request.requested_output_format
    when :UNSPECIFIED
      raise "Unspecified output format"

    when :PROTOBUF
      response.protobuf_payload = Conformance::TestAllTypes.encode(test_message)

    when :JSON
      response.json_payload = Conformance::TestAllTypes.encode_json(test_message)
    end
  rescue Exception => err
    response.runtime_error = err.message.encode("utf-8") + err.backtrace.join("\n")
  end

  return response
end

def do_test_io
  length_bytes = STDIN.read(4)
  return false if length_bytes.nil?

  length = length_bytes.unpack("V").first
  serialized_request = STDIN.read(length)
  if serialized_request.nil? or serialized_request.length != length
    raise "I/O error"
  end

  request = Conformance::ConformanceRequest.decode(serialized_request)

  response = do_test(request)

  serialized_response = Conformance::ConformanceResponse.encode(response)
  STDOUT.write([serialized_response.length].pack("V"))
  STDOUT.write(serialized_response)
  STDOUT.flush

  #if verbose
  #  fprintf(stderr, "conformance-cpp: request=%s, response=%s\n",
  #          request.ShortDebugString().c_str(),
  #          response.ShortDebugString().c_str());

  #test_count++;

  return true;
end

while true
  if not do_test_io()
    STDERR.puts("conformance-cpp: received EOF from test runner " +
                "after #{test_count} tests, exiting")
    exit 0
  end
end
