import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.conformance.Conformance;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf_test_messages.proto3.TestMessagesProto3;
import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.JsonFormat.TypeRegistry;
import java.io.IOException;
import java.nio.ByteBuffer;

class ConformanceJava {
  private int testCount = 0;
  private TypeRegistry typeRegistry;

  private boolean readFromStdin(byte[] buf, int len) throws Exception {
    int ofs = 0;
    while (len > 0) {
      int read = System.in.read(buf, ofs, len);
      if (read == -1) {
        return false;  // EOF
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
    buf[0] = (byte)val;
    buf[1] = (byte)(val >> 8);
    buf[2] = (byte)(val >> 16);
    buf[3] = (byte)(val >> 24);
    writeToStdout(buf);
  }

  private enum BinaryDecoder {
    BYTE_STRING_DECODER() {
      @Override
      public TestMessagesProto3.TestAllTypes parse(ByteString bytes)
          throws InvalidProtocolBufferException {
        return TestMessagesProto3.TestAllTypes.parseFrom(bytes);
      }
    },
    BYTE_ARRAY_DECODER() {
      @Override
      public TestMessagesProto3.TestAllTypes parse(ByteString bytes)
          throws InvalidProtocolBufferException {
        return TestMessagesProto3.TestAllTypes.parseFrom(bytes.toByteArray());
      }
    },
    ARRAY_BYTE_BUFFER_DECODER() {
      @Override
      public TestMessagesProto3.TestAllTypes parse(ByteString bytes)
          throws InvalidProtocolBufferException {
        ByteBuffer buffer = ByteBuffer.allocate(bytes.size());
        bytes.copyTo(buffer);
        buffer.flip();
        try {
          return TestMessagesProto3.TestAllTypes.parseFrom(CodedInputStream.newInstance(buffer));
        } catch (InvalidProtocolBufferException e) {
          throw e;
        } catch (IOException e) {
          throw new RuntimeException(
              "ByteString based ByteBuffer should not throw IOException.", e);
        }
      }
    },
    READONLY_ARRAY_BYTE_BUFFER_DECODER() {
      @Override
      public TestMessagesProto3.TestAllTypes parse(ByteString bytes)
          throws InvalidProtocolBufferException {
        try {
          return TestMessagesProto3.TestAllTypes.parseFrom(
              CodedInputStream.newInstance(bytes.asReadOnlyByteBuffer()));
        } catch (InvalidProtocolBufferException e) {
          throw e;
        } catch (IOException e) {
          throw new RuntimeException(
              "ByteString based ByteBuffer should not throw IOException.", e);
        }
      }
    },
    DIRECT_BYTE_BUFFER_DECODER() {
      @Override
      public TestMessagesProto3.TestAllTypes parse(ByteString bytes)
          throws InvalidProtocolBufferException {
        ByteBuffer buffer = ByteBuffer.allocateDirect(bytes.size());
        bytes.copyTo(buffer);
        buffer.flip();
        try {
          return TestMessagesProto3.TestAllTypes.parseFrom(CodedInputStream.newInstance(buffer));
        } catch (InvalidProtocolBufferException e) {
          throw e;
        } catch (IOException e) {
          throw new RuntimeException(
              "ByteString based ByteBuffer should not throw IOException.", e);
        }
      }
    },
    READONLY_DIRECT_BYTE_BUFFER_DECODER() {
      @Override
      public TestMessagesProto3.TestAllTypes parse(ByteString bytes)
          throws InvalidProtocolBufferException {
        ByteBuffer buffer = ByteBuffer.allocateDirect(bytes.size());
        bytes.copyTo(buffer);
        buffer.flip();
        try {
          return TestMessagesProto3.TestAllTypes.parseFrom(
              CodedInputStream.newInstance(buffer.asReadOnlyBuffer()));
        } catch (InvalidProtocolBufferException e) {
          throw e;
        } catch (IOException e) {
          throw new RuntimeException(
              "ByteString based ByteBuffer should not throw IOException.", e);
        }
      }
    },
    INPUT_STREAM_DECODER() {
      @Override
      public TestMessagesProto3.TestAllTypes parse(ByteString bytes)
          throws InvalidProtocolBufferException {
        try {
          return TestMessagesProto3.TestAllTypes.parseFrom(bytes.newInput());
        } catch (InvalidProtocolBufferException e) {
          throw e;
        } catch (IOException e) {
          throw new RuntimeException(
              "ByteString based InputStream should not throw IOException.", e);
        }
      }
    };

    public abstract TestMessagesProto3.TestAllTypes parse(ByteString bytes)
        throws InvalidProtocolBufferException;
  }

  private TestMessagesProto3.TestAllTypes parseBinary(ByteString bytes)
      throws InvalidProtocolBufferException {
    TestMessagesProto3.TestAllTypes[] messages =
        new TestMessagesProto3.TestAllTypes[BinaryDecoder.values().length];
    InvalidProtocolBufferException[] exceptions =
        new InvalidProtocolBufferException[BinaryDecoder.values().length];

    boolean hasMessage = false;
    boolean hasException = false;
    for (int i = 0; i < BinaryDecoder.values().length; ++i) {
      try {
        messages[i] = BinaryDecoder.values()[i].parse(bytes);
        hasMessage = true;
      } catch (InvalidProtocolBufferException e) {
        exceptions[i] = e;
        hasException = true;
      }
    }

    if (hasMessage && hasException) {
      StringBuilder sb =
          new StringBuilder("Binary decoders disagreed on whether the payload was valid.\n");
      for (int i = 0; i < BinaryDecoder.values().length; ++i) {
        sb.append(BinaryDecoder.values()[i].name());
        if (messages[i] != null) {
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
      throw exceptions[0];
    }

    // Fast path comparing all the messages with the first message, assuming equality being
    // symmetric and transitive.
    boolean allEqual = true;
    for (int i = 1; i < messages.length; ++i) {
      if (!messages[0].equals(messages[i])) {
        allEqual = false;
        break;
      }
    }

    // Slow path: compare and find out all unequal pairs.
    if (!allEqual) {
      StringBuilder sb = new StringBuilder();
      for (int i = 0; i < messages.length - 1; ++i) {
        for (int j = i + 1; j < messages.length; ++j) {
          if (!messages[i].equals(messages[j])) {
            sb.append(BinaryDecoder.values()[i].name())
                .append(" and ")
                .append(BinaryDecoder.values()[j].name())
                .append(" parsed the payload differently.\n");
          }
        }
      }
      throw new RuntimeException(sb.toString());
    }

    return messages[0];
  }

  private Conformance.ConformanceResponse doTest(Conformance.ConformanceRequest request) {
    TestMessagesProto3.TestAllTypes testMessage;

    switch (request.getPayloadCase()) {
      case PROTOBUF_PAYLOAD: {
        try {
          testMessage = parseBinary(request.getProtobufPayload());
        } catch (InvalidProtocolBufferException e) {
          return Conformance.ConformanceResponse.newBuilder().setParseError(e.getMessage()).build();
        }
        break;
      }
      case JSON_PAYLOAD: {
        try {
          TestMessagesProto3.TestAllTypes.Builder builder = TestMessagesProto3.TestAllTypes.newBuilder();
          JsonFormat.parser().usingTypeRegistry(typeRegistry)
              .merge(request.getJsonPayload(), builder);
          testMessage = builder.build();
        } catch (InvalidProtocolBufferException e) {
          return Conformance.ConformanceResponse.newBuilder().setParseError(e.getMessage()).build();
        }
        break;
      }
      case PAYLOAD_NOT_SET: {
        throw new RuntimeException("Request didn't have payload.");
      }

      default: {
        throw new RuntimeException("Unexpected payload case.");
      }
    }

    switch (request.getRequestedOutputFormat()) {
      case UNSPECIFIED:
        throw new RuntimeException("Unspecified output format.");

      case PROTOBUF:
        return Conformance.ConformanceResponse.newBuilder().setProtobufPayload(testMessage.toByteString()).build();

      case JSON:
        try {
          return Conformance.ConformanceResponse.newBuilder().setJsonPayload(
              JsonFormat.printer().usingTypeRegistry(typeRegistry).print(testMessage)).build();
        } catch (InvalidProtocolBufferException | IllegalArgumentException e) {
          return Conformance.ConformanceResponse.newBuilder().setSerializeError(
              e.getMessage()).build();
        }

      default: {
        throw new RuntimeException("Unexpected request output.");
      }
    }
  }

  private boolean doTestIo() throws Exception {
    int bytes = readLittleEndianIntFromStdin();

    if (bytes == -1) {
      return false;  // EOF
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
    typeRegistry = TypeRegistry.newBuilder().add(
        TestMessagesProto3.TestAllTypes.getDescriptor()).build();
    while (doTestIo()) {
      this.testCount++;
    }

    System.err.println("ConformanceJava: received EOF from test runner after " +
        this.testCount + " tests");
  }

  public static void main(String[] args) throws Exception {
    new ConformanceJava().run();
  }
}
