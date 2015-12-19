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

import org.openjdk.jmh.annotations.Benchmark;
import org.openjdk.jmh.annotations.Fork;
import org.openjdk.jmh.annotations.Level;
import org.openjdk.jmh.annotations.Param;
import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.Setup;
import org.openjdk.jmh.annotations.State;

import java.nio.ByteBuffer;

/**
 * Microbenchmarks for UTF-8 encoding.
 */
@State(Scope.Benchmark)
@Fork(1)
public class Utf8EncodingBenchmark {

  public enum EncoderType {
    ORIGINAL,
    NEW_HEAP,
    NEW_DIRECT,
    NEW_DIRECT_UNSAFE,
  }

  public enum CharSize {
    ONE_BYTE(0x80),
    ONE_OR_TWO_BYTE(0x90),
    TWO_BYTE(0x800),
    THREE_BYTE(0x10000),
    FOUR_BYTE(0x10FFFF);

    final int maxCodePoint;

    CharSize(int maxCodePoint) {
      this.maxCodePoint = maxCodePoint;
    }
  }


  @Param
  private CharSize charSize = CharSize.ONE_BYTE;

  @Param({"1024" /*, "16384"*/})
  private int length = 1024;

  @Param
  private EncoderType encoderType = EncoderType.NEW_HEAP;

  private String input;

  private Encoder encoder;

  private interface Encoder {
    void encode(String input);
  }

  private final class OldEncoder implements Encoder {
    private final byte[] buffer;

    OldEncoder() {
      buffer = new byte[length * Utf8.MAX_BYTES_PER_CHAR];
    }

    @Override
    public void encode(String input) {
      Utf8.encode(input, buffer, 0, buffer.length);
    }
  }

  private final class NewHeapEncoder implements Encoder {
    private final ByteBuffer buffer;

    NewHeapEncoder() {
      buffer = ByteBuffer.allocate(length * Utf8.MAX_BYTES_PER_CHAR);
    }

    @Override
    public void encode(String input) {
      buffer.clear();
      PlatformDependent.encodeToHeap(input, buffer);
    }
  }

  private final class NewDirectEncoder implements Encoder {
    private final ByteBuffer buffer;

    NewDirectEncoder() {
      buffer = ByteBuffer.allocateDirect(length * Utf8.MAX_BYTES_PER_CHAR);
    }

    @Override
    public void encode(String input) {
      buffer.clear();
      PlatformDependent.encodeToDirect(input, buffer);
    }
  }

  private final class NewUnsafeDirectEncoder implements Encoder {
    private final ByteBuffer buffer;

    NewUnsafeDirectEncoder() {
      buffer = ByteBuffer.allocateDirect(length * Utf8.MAX_BYTES_PER_CHAR);
    }

    @Override
    public void encode(String input) {
      buffer.clear();
      PlatformDependent.unsafeEncodeToDirect(input, buffer);
    }
  }

  @Setup(Level.Trial)
  public void setUp() {
    StringBuilder sb = new StringBuilder();

    // Evenly distribute the string across the range of characters.
    int codePointIncrement = Math.max(charSize.maxCodePoint / length, 1);
    for (int j = 0; j < length; j++) {
      int codePoint;
      do {
        codePoint = (codePointIncrement * j) % length;
      } while (isSurrogate(codePoint));
      sb.appendCodePoint(codePoint);
    }
    input = sb.toString();

    switch(encoderType) {
      case ORIGINAL:
        encoder = new OldEncoder();
        break;
      case NEW_HEAP:
        encoder = new NewHeapEncoder();
        break;
      case NEW_DIRECT:
        encoder = new NewDirectEncoder();
        break;
      case NEW_DIRECT_UNSAFE:
        encoder = new NewUnsafeDirectEncoder();
        break;
    }
  }

  @Benchmark
  public void encodeUtf8() {
    encoder.encode(input);
  }

  /** Character.isSurrogate was added in Java SE 7. */
  private static boolean isSurrogate(int c) {
    return Character.MIN_HIGH_SURROGATE <= c && c <= Character.MAX_LOW_SURROGATE;
  }

  public static void main(String[] args) {
    Utf8EncodingBenchmark bm = new Utf8EncodingBenchmark();
    bm.setUp();
    bm.encodeUtf8();
  }
}
