#!/usr/bin/env node

/*
 * Protocol Buffers - Google's data interchange format
 * Copyright 2008 Google Inc.  All rights reserved.
 * https://developers.google.com/protocol-buffers/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

var conformance = require('conformance_pb');
var fs = require('fs');

var testCount = 0;

function doTest(request) {
  var testMessage;
  var response = new conformance.ConformanceResponse();

  try {
    switch (request.getPayloadCase()) {
      case conformance.ConformanceRequest.PayloadCase.PROTOBUF_PAYLOAD:
        try {
          testMessage = conformance.TestAllTypes.deserializeBinary(
              request.getProtobufPayload());
        } catch (err) {
          response.setParseError(err.toString());
          return response;
        }

      case conformance.ConformanceRequest.PayloadCase.JSON_PAYLOAD:
        response.setSkipped("JSON not supported.");
        return response;

      case conformance.ConformanceRequest.PayloadCase.PAYLOAD_NOT_SET:
        response.setRuntimeError("Request didn't have payload");
        return response;
    }

    switch (request.getRequestedOutputFormat()) {
      case conformance.UNSPECIFIED:
        response.setRuntimeError("Unspecified output format");
        return response;

      case conformance.PROTOBUF:
        response.setProtobufPayload(testMessage.serializeBinary());

      case conformance.JSON:
        response.setSkipped("JSON not supported.");
        return response;

      default:
        throw "Request didn't have requested output format";
    }
  } catch (err) {
    response.setRuntimeError(err.toString());
  }

  return response
}

function onEof(totalRead) {
  if (totalRead == 0) {
    return undefined;
  } else {
    throw "conformance_nodejs: premature EOF on stdin.";
  }
}

// Utility function to read a buffer of N bytes.
function readBuffer(bytes) {
  var buf = new Buffer(bytes);
  var totalRead = 0;
  while (totalRead < bytes) {
    var read;
    try {
      read = fs.readSync(process.stdin.fd, buf, totalRead, bytes - totalRead);
    } catch (e) {
      if (e.code == 'EOF') {
        return onEof(totalRead)
      } else {
        throw "conformance_nodejs: Error reading from stdin.";
      }
    }

    if (read === 0) {
      return onEof(totalRead);
    }
    totalRead += read;
  }

  return buf;
}

function writeBuffer(buffer) {
  var totalWritten = 0;
  while (totalWritten < buffer.length) {
    totalWritten += fs.writeSync(
        process.stdout.fd, buffer, totalWritten, buffer.length - totalWritten);
  }
}

function uint8ArrayToBuffer(arr) {
  var buffer = new Buffer(arr.length);
  for (var i = 0; i < arr.length; i++) {
    buffer[i] = arr[i];
  }
  return buffer;
}

// Returns true if the test ran successfully, false on legitimate EOF.
// If EOF is encountered in an unexpected place, raises IOError.
function doTestIo() {
  var lengthBuf = readBuffer(4);
  if (!lengthBuf) {
    return false;
  }

  var length = lengthBuf.readInt32LE(0);
  var serializedRequest = readBuffer(length);
  if (!serializedRequest) {
    throw "conformance_nodejs: Failed to read request.";
  }

  var request =
      conformance.ConformanceRequest.deserializeBinary(serializedRequest);
  var response = doTest(request);

  var serializedResponse = response.serializeBinary();

  lengthBuf = new Buffer(4);
  lengthBuf.writeInt32LE(serializedResponse.length, 0);
  writeBuffer(lengthBuf);
  writeBuffer(uint8ArrayToBuffer(serializedResponse));

  testCount += 1

  return true;
}

while (true) {
  if (!doTestIo()) {
    console.error('conformance_ruby: received EOF from test runner ' +
                  "after " + testCount + " tests, exiting")
    break;
  }
}
