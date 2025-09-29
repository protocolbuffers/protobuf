// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

import com.google.protobuf.AbstractMessage;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Parser;
import com.google.protobuf.TextFormat;
import com.google.protobuf.conformance.Conformance;
import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.JsonFormat.TypeRegistry;
import com.google.protobuf_test_messages.edition2023.TestAllTypesEdition2023;
import com.google.protobuf_test_messages.edition2023.TestMessagesEdition2023;
import com.google.protobuf_test_messages.editions.proto2.TestMessagesProto2Editions;
import com.google.protobuf_test_messages.editions.proto3.TestMessagesProto3Editions;
import com.google.protobuf_test_messages.proto2.TestMessagesProto2;
import com.google.protobuf_test_messages.proto2.TestMessagesProto2.TestAllTypesProto2;
import com.google.protobuf_test_messages.proto3.TestMessagesProto3;
import com.google.protobuf_test_messages.proto3.TestMessagesProto3.TestAllTypesProto3;
import java.nio.ByteBuffer;
import java.util.ArrayList;

class ConformanceJava {
  private int testCount = 0;
  private TypeRegistry typeRegistry;

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
    BYTE_STRING_DECODER,
    BYTE_ARRAY_DECODER,
    ARRAY_BYTE_BUFFER_DECODER,
    READONLY_ARRAY_BYTE_BUFFER_DECODER,
    DIRECT_BYTE_BUFFER_DECODER,
    READONLY_DIRECT_BYTE_BUFFER_DECODER,
    INPUT_STREAM_DECODER;
  }

  private static class BinaryDecoder<T extends AbstractMessage> {
    public T decode(
        ByteString bytes, BinaryDecoderType type, Parser<T> parser, ExtensionRegistry extensions)
        throws InvalidProtocolBufferException {
      switch (type) {
        case BYTE_STRING_DECODER:
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

  private <T extends AbstractMessage> T parseBinary(
      ByteString bytes, Parser<T> parser, ExtensionRegistry extensions)
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
        // = BinaryDecoderType.values()[i].parseProto3(bytes);
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

  private Class<? extends AbstractMessage> createTestMessage(String messageType) {
    switch (messageType) {
      case "protobuf_test_messages.proto3.TestAllTypesProto3":
        return TestAllTypesProto3.class;
      case "protobuf_test_messages.proto2.TestAllTypesProto2":
        return TestAllTypesProto2.class;
      case "protobuf_test_messages.editions.TestAllTypesEdition2023":
        return TestAllTypesEdition2023.class;
      case "protobuf_test_messages.editions.proto3.TestAllTypesProto3":
        return TestMessagesProto3Editions.TestAllTypesProto3.class;
      case "protobuf_test_messages.editions.proto2.TestAllTypesProto2":
        return TestMessagesProto2Editions.TestAllTypesProto2.class;
      default:
        throw new IllegalArgumentException(
            "Protobuf request has unexpected payload type: " + messageType);
    }
  }

  private Class<?> createTestFile(String messageType) {
    switch (messageType) {
      case "protobuf_test_messages.proto3.TestAllTypesProto3":
        return TestMessagesProto3.class;
      case "protobuf_test_messages.proto2.TestAllTypesProto2":
        return TestMessagesProto2.class;
      case "protobuf_test_messages.editions.TestAllTypesEdition2023":
        return TestMessagesEdition2023.class;
      case "protobuf_test_messages.editions.proto3.TestAllTypesProto3":
        return TestMessagesProto3Editions.class;
      case "protobuf_test_messages.editions.proto2.TestAllTypesProto2":
        return TestMessagesProto2Editions.class;
      default:
        throw new IllegalArgumentException(
            "Protobuf request has unexpected payload type: " + messageType);
    }
  }

  @SuppressWarnings("unchecked")
  private Conformance.ConformanceResponse doTest(Conformance.ConformanceRequest request) {
    AbstractMessage testMessage;
    String messageType = request.getMessageType();

    ExtensionRegistry extensions = ExtensionRegistry.newInstance();
    try {
      createTestFile(messageType)
          .getMethod("registerAllExtensions", ExtensionRegistry.class)
          .invoke(null, extensions);
    } catch (Exception e) {
      throw new RuntimeException(e);
    }

    switch (request.getPayloadCase()) {
      case PROTOBUF_PAYLOAD:
        {
          try {
            testMessage =
                parseBinary(
                    request.getProtobufPayload(),
                    (Parser<AbstractMessage>)
                        createTestMessage(messageType).getMethod("parser").invoke(null),
                    extensions);
          } catch (InvalidProtocolBufferException e) {
            return Conformance.ConformanceResponse.newBuilder()
                .setParseError(e.getMessage())
                .build();
          } catch (Exception e) {
            throw new RuntimeException(e);
          }
          break;
        }
      case JSON_PAYLOAD:
        {
          try {
            JsonFormat.Parser parser = JsonFormat.parser().usingTypeRegistry(typeRegistry);
            if (request.getTestCategory()
                == Conformance.TestCategory.JSON_IGNORE_UNKNOWN_PARSING_TEST) {
              parser = parser.ignoringUnknownFields();
            }
            AbstractMessage.Builder<?> builder =
                (AbstractMessage.Builder<?>)
                    createTestMessage(messageType).getMethod("newBuilder").invoke(null);
            parser.merge(request.getJsonPayload(), builder);
            testMessage = (AbstractMessage) builder.build();
          } catch (InvalidProtocolBufferException e) {
            return Conformance.ConformanceResponse.newBuilder()
                .setParseError(e.getMessage())
                .build();
          } catch (Exception e) {
            throw new RuntimeException(e);
          }
          break;
        }
      case TEXT_PAYLOAD:
        {
          try {
            AbstractMessage.Builder<?> builder =
                (AbstractMessage.Builder<?>)
                    createTestMessage(messageType).getMethod("newBuilder").invoke(null);
            TextFormat.merge(request.getTextPayload(), extensions, builder);
            testMessage = (AbstractMessage) builder.build();
            } catch (TextFormat.ParseException e) {
              return Conformance.ConformanceResponse.newBuilder()
                  .setParseError(e.getMessage())
                  .build();
          } catch (Exception e) {
            throw new RuntimeException(e);
          }
          break;
        }
      case PAYLOAD_NOT_SET:
        {
          throw new IllegalArgumentException("Request didn't have payload.");
        }

      default:
        {
          throw new IllegalArgumentException("Unexpected payload case.");
        }
    }

    switch (request.getRequestedOutputFormat()) {
      case UNSPECIFIED:
        throw new IllegalArgumentException("Unspecified output format.");

      case PROTOBUF:
        {
          ByteString messageString = testMessage.toByteString();
          return Conformance.ConformanceResponse.newBuilder()
              .setProtobufPayload(messageString)
              .build();
        }

      case JSON:
        try {
          return Conformance.ConformanceResponse.newBuilder()
              .setJsonPayload(
                  JsonFormat.printer().usingTypeRegistry(typeRegistry).print(testMessage))
              .build();
        } catch (InvalidProtocolBufferException | IllegalArgumentException e) {
          return Conformance.ConformanceResponse.newBuilder()
              .setSerializeError(e.getMessage())
              .build();
        }

      case TEXT_FORMAT:
        return Conformance.ConformanceResponse.newBuilder()
            .setTextPayload(TextFormat.printer().printToString(testMessage))
            .build();

      default:
        {
          throw new IllegalArgumentException("Unexpected request output.");
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
    typeRegistry =
        TypeRegistry.newBuilder()
            .add(TestMessagesProto3.TestAllTypesProto3.getDescriptor())
            .add(
                com.google.protobuf_test_messages.editions.proto3.TestMessagesProto3Editions
                    .TestAllTypesProto3.getDescriptor())
            .build();
    while (doTestIo()) {
      this.testCount++;
    }

    System.err.println(
        "ConformanceJava: received EOF from test runner after " + this.testCount + " tests");
  }

  public static void main(String[] args) throws Exception {
    new ConformanceJava().run();
  }
}
