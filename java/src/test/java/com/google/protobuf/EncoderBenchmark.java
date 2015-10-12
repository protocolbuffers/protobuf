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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

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
    ORIGINAL,
    ZERO_COPY
  }

  public enum ByteStringType {
    LITERAL,
    BUFFER
  }

  private class OutputManager {
    private List<ByteBuffer> buffers = new ArrayList<ByteBuffer>();

    public void clear() {
      buffers.clear();
    }

    public void addCopy(byte[] b, int offset, int length) {
      // Do nothing.
      ByteBuffer buf = ByteBuffer.allocate(length);
      buf.put(b, offset, length);
      buf.flip();
      buffers.add(buf);
    }

    public void add(byte[] b, int offset, int length) {
      buffers.add(ByteBuffer.wrap(b, offset, length));
    }

    public void add(ByteBuffer data) {
      buffers.add(data);
    }
  }

  @Param
  private DataSize dataSize;

  @Param("MEDIUM")
  private BufferSize bufferSize;

  @Param
  private ByteStringType byteStringType;

  @Param
  private Type type;

  private Encoder encoder;

  private OutputManager outputManager = new OutputManager();

  private OutputStream stream = new OutputStream() {
    private final byte[] oneElementArray = new byte[1];

    @Override
    public void write(byte[] b, int off, int len) {
      outputManager.addCopy(b, off, len);
    }

    @Override
    public void write(byte[] b) {
      write(b, 0, b.length);
    }

    @Override
    public void write(int b) {
      oneElementArray[0] = (byte) b;
      write(oneElementArray, 0, 1);
    }
  };

  private TestAllTypes.Builder msgBuilder;
  private byte[] bytes;

  @Setup(Level.Trial)
  public void setup() {
    switch (type) {
      case ORIGINAL:
        encoder = CodedOutputStream.newInstance(stream, bufferSize.size);
        break;
      case ZERO_COPY:
        encoder = new ZeroCopyEncoder(new ZeroCopyEncoder.Handler() {
          @Override
          public void copyEncodedData(byte[] b, int offset, int length) throws IOException {
            outputManager.addCopy(b, offset, length);
          }

          @Override
          public void onDataEncoded(byte[] b, int offset, int length) throws IOException {
            outputManager.add(b, offset, length);
          }

          @Override
          public void onDataEncoded(ByteBuffer data) throws IOException {
            outputManager.add(data);
          }
        }, bufferSize.size);
        break;
    }

    // Create a builder for the test message and put a few simple fields in it.
    msgBuilder = TestAllTypes.newBuilder();
    byte[] simpleStringBytes = SIMPLE_STRING.getBytes(Charset.forName("UTF-8"));
    for (int ix=0; ix < 10; ++ix) {
      msgBuilder.addRepeatedInt32(ix);
      msgBuilder.addRepeatedBytes(newByteString(simpleStringBytes, false));
      msgBuilder.addRepeatedString(SIMPLE_STRING);
    }

    // Create a random buffer for the data size and add it to the message.
    bytes = new byte[dataSize.size];
    Arrays.fill(bytes, (byte) 1);
  }

  private ByteString newByteString(byte[] data, boolean copy) {
    switch (byteStringType) {
      case LITERAL:
        return copy ? ByteString.copyFrom(data) : ByteString.wrap(data);
      case BUFFER:
        if (copy) {
          ByteBuffer bufCopy = ByteBuffer.allocate(data.length);
          bufCopy.put(data);
          bufCopy.flip();
          return ByteString.wrap(bufCopy);
        } else {
          return ByteString.wrap(ByteBuffer.wrap(data));
        }
      default:
        throw new IllegalStateException("Unknown ByteStringType: " + byteStringType);
    }
  }
  @Benchmark
  public void doOutput() throws IOException {
    // Create the ByteString and build the message.
    ByteString byteString = newByteString(bytes, type != Type.ZERO_COPY);
    msgBuilder.addRepeatedBytes(byteString);

    // Write the message to the encoder.
    msgBuilder.build().writeTo(encoder);
    encoder.flush();

    // Clear data for the next iteration.
    msgBuilder.clearRepeatedBytes();
    outputManager.clear();
  }

  public static void main(String[] args) throws IOException {
    EncoderBenchmark benchmark = new EncoderBenchmark();
    benchmark.bufferSize = BufferSize.MEDIUM;
    benchmark.dataSize = DataSize.SMALL;
    benchmark.type = Type.ORIGINAL;
    benchmark.byteStringType = ByteStringType.BUFFER;

    benchmark.setup();
    //while(true) {
      benchmark.doOutput();
    //}
  }
}
