// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
    ORIGINAL,
    ZERO_COPY_HEAP,
    ZERO_COPY_DIRECT
  }

  public enum ByteStringType {
    LITERAL,
    BUFFER
  }

  private class OutputManager {
    public void add(byte[] b, int offset, int length, boolean mustCopy) {
      if (mustCopy) {
        // Copy but do nothing with the result.
        byte[] copy = new byte[length];
        System.arraycopy(b, offset, copy, 0, length);
        b = copy;
        offset = 0;
      }

      doNothing(ByteBuffer.wrap(b, offset, length));
    }

    public void add(ByteBuffer data, boolean mustCopy) {
      if (mustCopy) {
        // Copy but do nothing with the result.
        byte[] copy = new byte[data.remaining()];
        data.get(copy);
        data = ByteBuffer.wrap(copy);
      }
      doNothing(data);
    }

    private void doNothing(ByteBuffer buffer) {
      buffer.clear();
    }
  }

  @Param
  private DataSize dataSize;

  /**
   * Turns out this doesn't make a huge difference in the benchmarks. Leave it out
   * as a parameter.
   */
  private BufferSize bufferSize = BufferSize.MEDIUM;

  @Param
  private ByteStringType byteStringType;

  @Param
  private Type type;

  private Encoder encoder;

  private boolean zeroCopy;

  private OutputManager outputManager;

  private ZeroCopyEncoder.Handler handler = new ZeroCopyEncoder.Handler() {
    @Override
    public void onDataEncoded(byte[] b, int offset, int length, boolean copy) throws IOException {
      outputManager.add(b, offset, length, copy);
    }

    @Override
    public void onDataEncoded(ByteBuffer data, boolean copy) throws IOException {
      outputManager.add(data, copy);
    }
  };

  private OutputStream stream = new OutputStream() {
    private final byte[] oneElementArray = new byte[1];

    @Override
    public void write(byte[] b, int off, int len) {
      outputManager.add(b, off, len, true);
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
    outputManager = new OutputManager();
    switch (type) {
      case ORIGINAL:
        zeroCopy = false;
        encoder = CodedOutputStream.newInstance(stream, bufferSize.size);
        break;
      case ZERO_COPY_HEAP:
        zeroCopy = true;
        encoder = new ZeroCopyEncoder(handler, ByteBuffer.allocate(bufferSize.size));
        break;
      case ZERO_COPY_DIRECT:
        zeroCopy = true;
        encoder = new ZeroCopyEncoder(handler, ByteBuffer.allocateDirect(bufferSize.size));
        break;
    }

    // Create a builder for the test message and put a few simple fields in it.
    msgBuilder = TestAllTypes.newBuilder();
    byte[] simpleStringBytes = SIMPLE_STRING.getBytes(Charset.forName("UTF-8"));
    for (int ix=0; ix < 10; ++ix) {
      msgBuilder.addRepeatedInt32(ix);
      msgBuilder.addRepeatedBytes(newByteString(simpleStringBytes));
      msgBuilder.addRepeatedString(SIMPLE_STRING);
    }

    // Also add a message from a different charset.
    String yiddish = "דעם איז אַ פּראָבע אָנזאָג צו אילוסטרירן פאַרשידענע העלד שטעלט";
    msgBuilder.addRepeatedString(yiddish);

    // Create a random buffer for the data size and add it to the message.
    bytes = new byte[dataSize.size];
    Arrays.fill(bytes, (byte) 1);
  }

  private ByteString newByteString(byte[] data) {
    switch (byteStringType) {
      case LITERAL:
        return zeroCopy ? ByteString.wrap(data) : ByteString.copyFrom(data);
      case BUFFER:
        if (zeroCopy) {
          return ByteString.wrap(ByteBuffer.wrap(data));
        } else{
          ByteBuffer bufCopy = ByteBuffer.allocate(data.length);
          bufCopy.put(data);
          bufCopy.flip();
          return ByteString.wrap(bufCopy);
        }
      default:
        throw new IllegalStateException("Unknown ByteStringType: " + byteStringType);
    }
  }

  @Benchmark
  public void doOutput() throws IOException {
    // Create the ByteString and build the message.
    ByteString byteString = newByteString(bytes);
    msgBuilder.addRepeatedBytes(byteString);

    // Write the message to the encoder.
    msgBuilder.build().writeTo(encoder);
    encoder.flush();

    // Clear data for the next iteration.
    msgBuilder.clearRepeatedBytes();
  }

  public static void main(String[] args) throws IOException {
    EncoderBenchmark benchmark = new EncoderBenchmark();
    benchmark.bufferSize = BufferSize.MEDIUM;
    benchmark.dataSize = DataSize.SMALL;
    benchmark.type = Type.ZERO_COPY_HEAP;
    benchmark.byteStringType = ByteStringType.BUFFER;

    benchmark.setup();
    benchmark.doOutput();
  }
}
