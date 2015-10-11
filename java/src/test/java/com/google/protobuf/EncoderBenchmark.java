package com.google.protobuf;

import com.google.protobuf.FieldPresenceTestProto.TestAllTypes;

import org.openjdk.jmh.annotations.Benchmark;
import org.openjdk.jmh.annotations.Fork;
import org.openjdk.jmh.annotations.Level;
import org.openjdk.jmh.annotations.Param;
import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.Setup;
import org.openjdk.jmh.annotations.State;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Arrays;

/**
 * Microbenchmarks for writing out protobufs.
 */
@State(Scope.Benchmark)
@Fork(1)
public class EncoderBenchmark {
  private static final String SIMPLE_STRING = "This is a test string!";

  public enum DataSize {
    SMALL(10),
    MEDIUM(1024),
    LARGE(1024 * 100),
    JUMBO(1024 * 10000);

    final int size;

    DataSize(int size) {
      this.size = size;
    }
  }

  public enum BufferSize {
    SMALL(1024),
    MEDIUM(1024 * 4),
    LARGE(1024 * 8);

    final int size;

    BufferSize(int size) {
      this.size = size;
    }
  }

  public enum Type {
    OLD,
    NEW
  }

  public enum ByteStringType {
    LITERAL,
    UNSAFE
  }

  @Param
  private DataSize dataSize;

  @Param("MEDIUM")
  private BufferSize bufferSize;

  @Param("UNSAFE")
  private ByteStringType byteStringType;

  @Param
  private Type type;

  private Encoder outputter;

  private OutputStream stream = new OutputStream() {
    @Override
    public void write(byte[] b, int off, int len) {
      // Do nothing.
    }

    @Override
    public void write(byte[] b) {
      // Do nothing.
    }

    @Override
    public void write(int b) {
      // Do nothing.
    }
  };

  private MessageLite msg;

  @Setup(Level.Trial)
  public void setup() {
    switch (type) {
      case OLD:
        outputter = CodedOutputStream.newInstance(stream, bufferSize.size);
        break;
      case NEW:
        outputter = new ZeroCopyEncoder(new ZeroCopyEncoder.Handler() {
          @Override
          public void copyEncodedData(byte[] b, int offset, int length) throws IOException {
            // Do nothing.
          }

          @Override
          public void onDataEncoded(byte[] b, int offset, int length) throws IOException {
            // Do nothing.
          }

          @Override
          public void onDataEncoded(ByteBuffer data) throws IOException {
            // Do nothing.
          }
        }, bufferSize.size);
        break;
    }

    // Create a builder for the test message and put a few simple fields in it.
    TestAllTypes.Builder msgBuilder = TestAllTypes.newBuilder();
    byte[] simpleStringBytes = SIMPLE_STRING.getBytes(Charset.forName("UTF-8"));
    for (int ix=0; ix < 10; ++ix) {
      msgBuilder.addRepeatedInt32(ix);
      msgBuilder.addRepeatedBytes(newByteString(simpleStringBytes));
      msgBuilder.addRepeatedString(SIMPLE_STRING);
    }

    // Create a random buffer for the data size and add it to the message.
    byte[] bytes = new byte[dataSize.size];
    Arrays.fill(bytes, (byte) 1);
    msgBuilder.addRepeatedBytes(newByteString(bytes));
    msg = msgBuilder.build();
  }

  private ByteString newByteString(byte[] data) {
    switch (byteStringType) {
      case LITERAL:
        return ByteString.copyFrom(data);
      case UNSAFE:
        return ByteString.unsafeWrapper(ByteBuffer.wrap(data));
      default:
        throw new IllegalStateException("Unknown ByteStringType: " + byteStringType);
    }
  }
  @Benchmark
  public void doOutput() throws IOException {
    msg.writeTo(outputter);
    outputter.flush();
  }

  public static void main(String[] args) throws IOException {
    EncoderBenchmark benchmark = new EncoderBenchmark();
    benchmark.bufferSize = BufferSize.MEDIUM;
    benchmark.dataSize = DataSize.SMALL;
    benchmark.type = Type.OLD;
    benchmark.byteStringType = ByteStringType.UNSAFE;

    benchmark.setup();
    //while(true) {
      benchmark.doOutput();
    //}
  }
}
