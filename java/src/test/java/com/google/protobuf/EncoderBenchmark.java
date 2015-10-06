package com.google.protobuf;

import com.google.protobuf.FieldPresenceTestProto.TestAllTypes;

import org.openjdk.jmh.annotations.Benchmark;
import org.openjdk.jmh.annotations.Fork;
import org.openjdk.jmh.annotations.Level;
import org.openjdk.jmh.annotations.Param;
import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.Setup;
import org.openjdk.jmh.annotations.State;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.nio.channels.WritableByteChannel;
import java.nio.charset.Charset;
import java.util.Random;

/**
 * Microbenchmarks for writing out protobufs.
 */
@State(Scope.Benchmark)
@Fork(1)
public class EncoderBenchmark {

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

  @Param
  private DataSize dataSize;

  @Param
  private BufferSize bufferSize;

  @Param
  private Type type;

  private Encoder outputter;

  private ByteArrayOutputStream stream = new ByteArrayOutputStream(BufferSize.LARGE.size * 2);

  private MessageLite msg;

  @Setup(Level.Trial)
  public void setup() throws Exception {
    switch (type) {
      case OLD:
        outputter = CodedOutputStream.newInstance(stream, bufferSize.size);
        break;
      case NEW:
        outputter = new ZeroCopyEncoder(new ZeroCopyEncoder.Handler() {
          private final WritableByteChannel channel = Channels.newChannel(stream);
          @Override
          public void onDataEncoded(byte[] b, int offset, int length) {
            stream.write(b, offset, length);
          }

          @Override
          public void onDataEncoded(ByteBuffer data) throws IOException {
            channel.write(data);
          }
        }, bufferSize.size);
        break;
    }

    // Create a builder for the test message and put a few simple fields in it.
    TestAllTypes.Builder msgBuilder = TestAllTypes.newBuilder();
    ByteString smallBytes = ByteString.unsafeWrapper(
            ByteBuffer.wrap("This is a test string!".getBytes(Charset.forName("UTF-8"))));
    for (int ix=0; ix < 10; ++ix) {
      msgBuilder.addRepeatedInt32(ix);
      msgBuilder.addRepeatedBytes(smallBytes);
      msgBuilder.addRepeatedString("This is a test string!");
    }

    // Create a random buffer for the data size and add it to the message.
    byte[] bytes = new byte[dataSize.size];
    new Random().nextBytes(bytes);
    msgBuilder.addRepeatedBytes(ByteString.unsafeWrapper(ByteBuffer.wrap(bytes)));
    msg = msgBuilder.build();
  }

  @Benchmark
  public void doOutput() throws IOException {
    stream.reset();
    msg.writeTo(outputter);
    outputter.flush();
  }
}
