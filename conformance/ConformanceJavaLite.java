// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.ExtensionRegistryLite;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.MessageLite;
import com.google.protobuf.Parser;
import com.google.protobuf.conformance.Conformance;
import com.google.protobuf_test_messages.proto2.TestMessagesProto2;
import com.google.protobuf_test_messages.proto2.TestMessagesProto2.TestAllTypesProto2;
import com.google.protobuf_test_messages.proto3.TestMessagesProto3;
import com.google.protobuf_test_messages.proto3.TestMessagesProto3.TestAllTypesProto3;
import java.nio.ByteBuffer;
import java.util.ArrayList;

class ConformanceJavaLite {
  private int testCount = 0;

  private boolean readFromStdin(byte[] buf, int len) throws Exception {
    int ofs = 0;
    while (len > 0) {
      int read = System.in.read(buf, ofs, len);
      if (read == -1) {
        return false; // EOF
      }
      ofs += read;
      len -= read;
    }

    return true;
  }

  private void writeToStdout(byte[] buf) throws Exception {
    System.out.write(buf);
  }

  // Returns -1 on EOF (the actual values will always be positive).
  private int readLittleEndianIntFromStdin() throws Exception {
    byte[] buf = new byte[4];
    if (!readFromStdin(buf, 4)) {
      return -1;
    }
    return (buf[0] & 0xff)
        | ((buf[1] & 0xff) << 8)
        | ((buf[2] & 0xff) << 16)
        | ((buf[3] & 0xff) << 24);
  }

  private void writeLittleEndianIntToStdout(int val) throws Exception {
    byte[] buf = new byte[4];
    buf[0] = (byte) val;
    buf[1] = (byte) (val >> 8);
    buf[2] = (byte) (val >> 16);
    buf[3] = (byte) (val >> 24);
    writeToStdout(buf);
  }

  private enum BinaryDecoderType {
    BTYE_STRING_DECODER,
    BYTE_ARRAY_DECODER,
    ARRAY_BYTE_BUFFER_DECODER,
    READONLY_ARRAY_BYTE_BUFFER_DECODER,
    DIRECT_BYTE_BUFFER_DECODER,
    READONLY_DIRECT_BYTE_BUFFER_DECODER,
    INPUT_STREAM_DECODER;
  }

  private static class BinaryDecoder<T extends MessageLite> {
    public T decode(
        ByteString bytes,
        BinaryDecoderType type,
        Parser<T> parser,
        ExtensionRegistryLite extensions)
        throws InvalidProtocolBufferException {
      switch (type) {
        case BTYE_STRING_DECODER:
        case BYTE_ARRAY_DECODER:
          return parser.parseFrom(bytes, extensions);
        case ARRAY_BYTE_BUFFER_DECODER:
          {
            ByteBuffer buffer = ByteBuffer.allocate(bytes.size());
            bytes.copyTo(buffer);
            buffer.flip();
            return parser.parseFrom(CodedInputStream.newInstance(buffer), extensions);
          }
        case READONLY_ARRAY_BYTE_BUFFER_DECODER:
          {
            return parser.parseFrom(
                CodedInputStream.newInstance(bytes.asReadOnlyByteBuffer()), extensions);
          }
        case DIRECT_BYTE_BUFFER_DECODER:
          {
            ByteBuffer buffer = ByteBuffer.allocateDirect(bytes.size());
            bytes.copyTo(buffer);
            buffer.flip();
            return parser.parseFrom(CodedInputStream.newInstance(buffer), extensions);
          }
        case READONLY_DIRECT_BYTE_BUFFER_DECODER:
          {
            ByteBuffer buffer = ByteBuffer.allocateDirect(bytes.size());
            bytes.copyTo(buffer);
            buffer.flip();
            return parser.parseFrom(
                CodedInputStream.newInstance(buffer.asReadOnlyBuffer()), extensions);
          }
        case INPUT_STREAM_DECODER:
          {
            return parser.parseFrom(bytes.newInput(), extensions);
          }
      }
      return null;
    }
  }

  private <T extends MessageLite> T parseBinary(
      ByteString bytes, Parser<T> parser, ExtensionRegistryLite extensions)
      throws InvalidProtocolBufferException {
    ArrayList<T> messages = new ArrayList<>();
    ArrayList<InvalidProtocolBufferException> exceptions = new ArrayList<>();

    for (int i = 0; i < BinaryDecoderType.values().length; i++) {
      messages.add(null);
      exceptions.add(null);
    }
    if (messages.isEmpty()) {
      throw new RuntimeException("binary decoder types missing");
    }

    BinaryDecoder<T> decoder = new BinaryDecoder<>();

    boolean hasMessage = false;
    boolean hasException = false;
    for (int i = 0; i < BinaryDecoderType.values().length; ++i) {
      try {
        messages.set(i, decoder.decode(bytes, BinaryDecoderType.values()[i], parser, extensions));
        hasMessage = true;
      } catch (InvalidProtocolBufferException e) {
        exceptions.set(i, e);
        hasException = true;
      }
    }

    if (hasMessage && hasException) {
      StringBuilder sb =
          new StringBuilder("Binary decoders disagreed on whether the payload was valid.\n");
      for (int i = 0; i < BinaryDecoderType.values().length; ++i) {
        sb.append(BinaryDecoderType.values()[i].name());
        if (messages.get(i) != null) {
          sb.append(" accepted the payload.\n");
        } else {
          sb.append(" rejected the payload.\n");
        }
      }
      throw new RuntimeException(sb.toString());
    }

    if (hasException) {
      // We do not check if exceptions are equal. Different implementations may return different
      // exception messages. Throw an arbitrary one out instead.
      InvalidProtocolBufferException exception = null;
      for (InvalidProtocolBufferException e : exceptions) {
        if (exception != null) {
          exception.addSuppressed(e);
        } else {
          exception = e;
        }
      }
      throw exception;
    }

    // Fast path comparing all the messages with the first message, assuming equality being
    // symmetric and transitive.
    boolean allEqual = true;
    for (int i = 1; i < messages.size(); ++i) {
      if (!messages.get(0).equals(messages.get(i))) {
        allEqual = false;
        break;
      }
    }

    // Slow path: compare and find out all unequal pairs.
    if (!allEqual) {
      StringBuilder sb = new StringBuilder();
      for (int i = 0; i < messages.size() - 1; ++i) {
        for (int j = i + 1; j < messages.size(); ++j) {
          if (!messages.get(i).equals(messages.get(j))) {
            sb.append(BinaryDecoderType.values()[i].name())
                .append(" and ")
                .append(BinaryDecoderType.values()[j].name())
                .append(" parsed the payload differently.\n");
          }
        }
      }
      throw new RuntimeException(sb.toString());
    }

    return messages.get(0);
  }

  private Conformance.ConformanceResponse doTest(Conformance.ConformanceRequest request) {
    com.google.protobuf.MessageLite testMessage;
    boolean isProto3 =
        request.getMessageType().equals("protobuf_test_messages.proto3.TestAllTypesProto3");
    boolean isProto2 =
        request.getMessageType().equals("protobuf_test_messages.proto2.TestAllTypesProto2");

    switch (request.getPayloadCase()) {
      case PROTOBUF_PAYLOAD:
        {
          if (isProto3) {
            try {
              ExtensionRegistryLite extensions = ExtensionRegistryLite.newInstance();
              TestMessagesProto3.registerAllExtensions(extensions);
              testMessage =
                  parseBinary(
                      request.getProtobufPayload(), TestAllTypesProto3.parser(), extensions);
            } catch (InvalidProtocolBufferException e) {
              return Conformance.ConformanceResponse.newBuilder()
                  .setParseError(e.getMessage())
                  .build();
            }
          } else if (isProto2) {
            try {
              ExtensionRegistryLite extensions = ExtensionRegistryLite.newInstance();
              TestMessagesProto2.registerAllExtensions(extensions);
              testMessage =
                  parseBinary(
                      request.getProtobufPayload(), TestAllTypesProto2.parser(), extensions);
            } catch (InvalidProtocolBufferException e) {
              return Conformance.ConformanceResponse.newBuilder()
                  .setParseError(e.getMessage())
                  .build();
            }
          } else {
            throw new RuntimeException("Protobuf request doesn't have specific payload type.");
          }
          break;
        }
      case JSON_PAYLOAD:
        {
          return Conformance.ConformanceResponse.newBuilder()
              .setSkipped("Lite runtime does not support JSON format.")
              .build();
        }
      case TEXT_PAYLOAD:
        {
          return Conformance.ConformanceResponse.newBuilder()
              .setSkipped("Lite runtime does not support Text format.")
              .build();
        }
      case PAYLOAD_NOT_SET:
        {
          throw new RuntimeException("Request didn't have payload.");
        }
      default:
        {
          throw new RuntimeException("Unexpected payload case.");
        }
    }

    switch (request.getRequestedOutputFormat()) {
      case UNSPECIFIED:
        throw new RuntimeException("Unspecified output format.");

      case PROTOBUF:
        return Conformance.ConformanceResponse.newBuilder()
            .setProtobufPayload(testMessage.toByteString())
            .build();

      case JSON:
        return Conformance.ConformanceResponse.newBuilder()
            .setSkipped("Lite runtime does not support JSON format.")
            .build();

      case TEXT_FORMAT:
        return Conformance.ConformanceResponse.newBuilder()
            .setSkipped("Lite runtime does not support Text format.")
            .build();
      default:
        {
          throw new RuntimeException("Unexpected request output.");
        }
    }
  }

  private boolean doTestIo() throws Exception {
    int bytes = readLittleEndianIntFromStdin();

    if (bytes == -1) {
      return false; // EOF
    }

    byte[] serializedInput = new byte[bytes];

    if (!readFromStdin(serializedInput, bytes)) {
      throw new RuntimeException("Unexpected EOF from test program.");
    }

    Conformance.ConformanceRequest request =
        Conformance.ConformanceRequest.parseFrom(serializedInput);
    Conformance.ConformanceResponse response = doTest(request);
    byte[] serializedOutput = response.toByteArray();

    writeLittleEndianIntToStdout(serializedOutput.length);
    writeToStdout(serializedOutput);

    return true;
  }

  public void run() throws Exception {
    while (doTestIo()) {
      this.testCount++;
    }

    System.err.println(
        "ConformanceJavaLite: received EOF from test runner after " + this.testCount + " tests");
  }

  public static void main(String[] args) throws Exception {
    new ConformanceJavaLite().run();
  }
}
