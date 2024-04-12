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

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;
import static com.google.protobuf.WireFormat.FIXED32_SIZE;
import static com.google.protobuf.WireFormat.FIXED64_SIZE;
import static com.google.protobuf.WireFormat.MAX_VARINT32_SIZE;
import static com.google.protobuf.WireFormat.MAX_VARINT64_SIZE;
import static com.google.protobuf.WireFormat.MESSAGE_SET_ITEM;
import static com.google.protobuf.WireFormat.MESSAGE_SET_MESSAGE;
import static com.google.protobuf.WireFormat.MESSAGE_SET_TYPE_ID;
import static com.google.protobuf.WireFormat.WIRETYPE_END_GROUP;
import static com.google.protobuf.WireFormat.WIRETYPE_FIXED32;
import static com.google.protobuf.WireFormat.WIRETYPE_FIXED64;
import static com.google.protobuf.WireFormat.WIRETYPE_LENGTH_DELIMITED;
import static com.google.protobuf.WireFormat.WIRETYPE_START_GROUP;
import static com.google.protobuf.WireFormat.WIRETYPE_VARINT;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayDeque;
import java.util.List;
import java.util.Map;
import java.util.Queue;

/**
 * A protobuf writer that serializes messages in their binary form. Messages are serialized in
 * reverse in order to avoid calculating the serialized size of each nested message. Since the
 * message size is not known in advance, the writer employs a strategy of chunking and buffer
 * chaining. Buffers are allocated as-needed by a provided {@link BufferAllocator}. Once writing is
 * finished, the application can access the buffers in forward-writing order by calling {@link
 * #complete()}.
 *
 * <p>Once {@link #complete()} has been called, the writer can not be reused for additional writes.
 * The {@link #getTotalBytesWritten()} will continue to reflect the total of the write and will not
 * be reset.
 */
@CheckReturnValue
@ExperimentalApi
abstract class BinaryWriter extends ByteOutput implements Writer {
  public static final int DEFAULT_CHUNK_SIZE = 4096;

  private final BufferAllocator alloc;
  private final int chunkSize;

  final ArrayDeque<AllocatedBuffer> buffers = new ArrayDeque<AllocatedBuffer>(4);
  int totalDoneBytes;

  /**
   * Creates a new {@link BinaryWriter} that will allocate heap buffers of {@link
   * #DEFAULT_CHUNK_SIZE} as necessary.
   */
  public static BinaryWriter newHeapInstance(BufferAllocator alloc) {
    return newHeapInstance(alloc, DEFAULT_CHUNK_SIZE);
  }

  /**
   * Creates a new {@link BinaryWriter} that will allocate heap buffers of {@code chunkSize} as
   * necessary.
   */
  public static BinaryWriter newHeapInstance(BufferAllocator alloc, int chunkSize) {
    return isUnsafeHeapSupported()
        ? newUnsafeHeapInstance(alloc, chunkSize)
        : newSafeHeapInstance(alloc, chunkSize);
  }

  /**
   * Creates a new {@link BinaryWriter} that will allocate direct (i.e. non-heap) buffers of {@link
   * #DEFAULT_CHUNK_SIZE} as necessary.
   */
  public static BinaryWriter newDirectInstance(BufferAllocator alloc) {
    return newDirectInstance(alloc, DEFAULT_CHUNK_SIZE);
  }

  /**
   * Creates a new {@link BinaryWriter} that will allocate direct (i.e. non-heap) buffers of {@code
   * chunkSize} as necessary.
   */
  public static BinaryWriter newDirectInstance(BufferAllocator alloc, int chunkSize) {
    return isUnsafeDirectSupported()
        ? newUnsafeDirectInstance(alloc, chunkSize)
        : newSafeDirectInstance(alloc, chunkSize);
  }

  static boolean isUnsafeHeapSupported() {
    return UnsafeHeapWriter.isSupported();
  }

  static boolean isUnsafeDirectSupported() {
    return UnsafeDirectWriter.isSupported();
  }

  static BinaryWriter newSafeHeapInstance(BufferAllocator alloc, int chunkSize) {
    return new SafeHeapWriter(alloc, chunkSize);
  }

  static BinaryWriter newUnsafeHeapInstance(BufferAllocator alloc, int chunkSize) {
    if (!isUnsafeHeapSupported()) {
      throw new UnsupportedOperationException("Unsafe operations not supported");
    }
    return new UnsafeHeapWriter(alloc, chunkSize);
  }

  static BinaryWriter newSafeDirectInstance(BufferAllocator alloc, int chunkSize) {
    return new SafeDirectWriter(alloc, chunkSize);
  }

  static BinaryWriter newUnsafeDirectInstance(BufferAllocator alloc, int chunkSize) {
    if (!isUnsafeDirectSupported()) {
      throw new UnsupportedOperationException("Unsafe operations not supported");
    }
    return new UnsafeDirectWriter(alloc, chunkSize);
  }

  /** Only allow subclassing for inner classes. */
  private BinaryWriter(BufferAllocator alloc, int chunkSize) {
    if (chunkSize <= 0) {
      throw new IllegalArgumentException("chunkSize must be > 0");
    }
    this.alloc = checkNotNull(alloc, "alloc");
    this.chunkSize = chunkSize;
  }

  @Override
  public final FieldOrder fieldOrder() {
    return FieldOrder.DESCENDING;
  }

  /**
   * Completes the write operation and returns a queue of {@link AllocatedBuffer} objects in
   * forward-writing order. This method should only be called once.
   *
   * <p>After calling this method, the writer can not be reused. Create a new writer for future
   * writes.
   */
  @CanIgnoreReturnValue
  public final Queue<AllocatedBuffer> complete() {
    finishCurrentBuffer();
    return buffers;
  }

  @Override
  public final void writeSFixed32(int fieldNumber, int value) throws IOException {
    writeFixed32(fieldNumber, value);
  }

  @Override
  public final void writeInt64(int fieldNumber, long value) throws IOException {
    writeUInt64(fieldNumber, value);
  }

  @Override
  public final void writeSFixed64(int fieldNumber, long value) throws IOException {
    writeFixed64(fieldNumber, value);
  }

  @Override
  public final void writeFloat(int fieldNumber, float value) throws IOException {
    writeFixed32(fieldNumber, Float.floatToRawIntBits(value));
  }

  @Override
  public final void writeDouble(int fieldNumber, double value) throws IOException {
    writeFixed64(fieldNumber, Double.doubleToRawLongBits(value));
  }

  @Override
  public final void writeEnum(int fieldNumber, int value) throws IOException {
    writeInt32(fieldNumber, value);
  }

  @Override
  public final void writeInt32List(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    if (list instanceof IntArrayList) {
      writeInt32List_Internal(fieldNumber, (IntArrayList) list, packed);
    } else {
      writeInt32List_Internal(fieldNumber, list, packed);
    }
  }

  private void writeInt32List_Internal(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeInt32(list.get(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeInt32(fieldNumber, list.get(i));
      }
    }
  }

  private void writeInt32List_Internal(int fieldNumber, IntArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeInt32(list.getInt(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeInt32(fieldNumber, list.getInt(i));
      }
    }
  }

  @Override
  public final void writeFixed32List(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    if (list instanceof IntArrayList) {
      writeFixed32List_Internal(fieldNumber, (IntArrayList) list, packed);
    } else {
      writeFixed32List_Internal(fieldNumber, list, packed);
    }
  }

  private void writeFixed32List_Internal(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * FIXED32_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed32(list.get(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed32(fieldNumber, list.get(i));
      }
    }
  }

  private void writeFixed32List_Internal(int fieldNumber, IntArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * FIXED32_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed32(list.getInt(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed32(fieldNumber, list.getInt(i));
      }
    }
  }

  @Override
  public final void writeInt64List(int fieldNumber, List<Long> list, boolean packed)
      throws IOException {
    writeUInt64List(fieldNumber, list, packed);
  }

  @Override
  public final void writeUInt64List(int fieldNumber, List<Long> list, boolean packed)
      throws IOException {
    if (list instanceof LongArrayList) {
      writeUInt64List_Internal(fieldNumber, (LongArrayList) list, packed);
    } else {
      writeUInt64List_Internal(fieldNumber, list, packed);
    }
  }

  private void writeUInt64List_Internal(int fieldNumber, List<Long> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeVarint64(list.get(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeUInt64(fieldNumber, list.get(i));
      }
    }
  }

  private void writeUInt64List_Internal(int fieldNumber, LongArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeVarint64(list.getLong(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeUInt64(fieldNumber, list.getLong(i));
      }
    }
  }

  @Override
  public final void writeFixed64List(int fieldNumber, List<Long> list, boolean packed)
      throws IOException {
    if (list instanceof LongArrayList) {
      writeFixed64List_Internal(fieldNumber, (LongArrayList) list, packed);
    } else {
      writeFixed64List_Internal(fieldNumber, list, packed);
    }
  }

  private void writeFixed64List_Internal(int fieldNumber, List<Long> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * FIXED64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed64(list.get(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed64(fieldNumber, list.get(i));
      }
    }
  }

  private void writeFixed64List_Internal(int fieldNumber, LongArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * FIXED64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed64(list.getLong(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed64(fieldNumber, list.getLong(i));
      }
    }
  }

  @Override
  public final void writeFloatList(int fieldNumber, List<Float> list, boolean packed)
      throws IOException {
    if (list instanceof FloatArrayList) {
      writeFloatList_Internal(fieldNumber, (FloatArrayList) list, packed);
    } else {
      writeFloatList_Internal(fieldNumber, list, packed);
    }
  }

  private void writeFloatList_Internal(int fieldNumber, List<Float> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * FIXED32_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed32(Float.floatToRawIntBits(list.get(i)));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFloat(fieldNumber, list.get(i));
      }
    }
  }

  private void writeFloatList_Internal(int fieldNumber, FloatArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * FIXED32_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed32(Float.floatToRawIntBits(list.getFloat(i)));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFloat(fieldNumber, list.getFloat(i));
      }
    }
  }

  @Override
  public final void writeDoubleList(int fieldNumber, List<Double> list, boolean packed)
      throws IOException {
    if (list instanceof DoubleArrayList) {
      writeDoubleList_Internal(fieldNumber, (DoubleArrayList) list, packed);
    } else {
      writeDoubleList_Internal(fieldNumber, list, packed);
    }
  }

  private void writeDoubleList_Internal(int fieldNumber, List<Double> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * FIXED64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed64(Double.doubleToRawLongBits(list.get(i)));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeDouble(fieldNumber, list.get(i));
      }
    }
  }

  private void writeDoubleList_Internal(int fieldNumber, DoubleArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * FIXED64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeFixed64(Double.doubleToRawLongBits(list.getDouble(i)));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeDouble(fieldNumber, list.getDouble(i));
      }
    }
  }

  @Override
  public final void writeEnumList(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    writeInt32List(fieldNumber, list, packed);
  }

  @Override
  public final void writeBoolList(int fieldNumber, List<Boolean> list, boolean packed)
      throws IOException {
    if (list instanceof BooleanArrayList) {
      writeBoolList_Internal(fieldNumber, (BooleanArrayList) list, packed);
    } else {
      writeBoolList_Internal(fieldNumber, list, packed);
    }
  }

  private void writeBoolList_Internal(int fieldNumber, List<Boolean> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + list.size());
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeBool(list.get(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeBool(fieldNumber, list.get(i));
      }
    }
  }

  private void writeBoolList_Internal(int fieldNumber, BooleanArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + list.size());
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeBool(list.getBoolean(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeBool(fieldNumber, list.getBoolean(i));
      }
    }
  }

  @Override
  public final void writeStringList(int fieldNumber, List<String> list) throws IOException {
    if (list instanceof LazyStringList) {
      final LazyStringList lazyList = (LazyStringList) list;
      for (int i = list.size() - 1; i >= 0; i--) {
        writeLazyString(fieldNumber, lazyList.getRaw(i));
      }
    } else {
      for (int i = list.size() - 1; i >= 0; i--) {
        writeString(fieldNumber, list.get(i));
      }
    }
  }

  private void writeLazyString(int fieldNumber, Object value) throws IOException {
    if (value instanceof String) {
      writeString(fieldNumber, (String) value);
    } else {
      writeBytes(fieldNumber, (ByteString) value);
    }
  }

  @Override
  public final void writeBytesList(int fieldNumber, List<ByteString> list) throws IOException {
    for (int i = list.size() - 1; i >= 0; i--) {
      writeBytes(fieldNumber, list.get(i));
    }
  }

  @Override
  public final void writeUInt32List(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    if (list instanceof IntArrayList) {
      writeUInt32List_Internal(fieldNumber, (IntArrayList) list, packed);
    } else {
      writeUInt32List_Internal(fieldNumber, list, packed);
    }
  }

  private void writeUInt32List_Internal(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT32_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeVarint32(list.get(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeUInt32(fieldNumber, list.get(i));
      }
    }
  }

  private void writeUInt32List_Internal(int fieldNumber, IntArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT32_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeVarint32(list.getInt(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeUInt32(fieldNumber, list.getInt(i));
      }
    }
  }

  @Override
  public final void writeSFixed32List(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    writeFixed32List(fieldNumber, list, packed);
  }

  @Override
  public final void writeSFixed64List(int fieldNumber, List<Long> list, boolean packed)
      throws IOException {
    writeFixed64List(fieldNumber, list, packed);
  }

  @Override
  public final void writeSInt32List(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    if (list instanceof IntArrayList) {
      writeSInt32List_Internal(fieldNumber, (IntArrayList) list, packed);
    } else {
      writeSInt32List_Internal(fieldNumber, list, packed);
    }
  }

  private void writeSInt32List_Internal(int fieldNumber, List<Integer> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT32_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeSInt32(list.get(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeSInt32(fieldNumber, list.get(i));
      }
    }
  }

  private void writeSInt32List_Internal(int fieldNumber, IntArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT32_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeSInt32(list.getInt(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeSInt32(fieldNumber, list.getInt(i));
      }
    }
  }

  @Override
  public final void writeSInt64List(int fieldNumber, List<Long> list, boolean packed)
      throws IOException {
    if (list instanceof LongArrayList) {
      writeSInt64List_Internal(fieldNumber, (LongArrayList) list, packed);
    } else {
      writeSInt64List_Internal(fieldNumber, list, packed);
    }
  }

  private static final int MAP_KEY_NUMBER = 1;
  private static final int MAP_VALUE_NUMBER = 2;

  @Override
  public <K, V> void writeMap(int fieldNumber, MapEntryLite.Metadata<K, V> metadata, Map<K, V> map)
      throws IOException {
    // TODO(liujisi): Reverse write those entries.
    for (Map.Entry<K, V> entry : map.entrySet()) {
      int prevBytes = getTotalBytesWritten();
      writeMapEntryField(this, MAP_VALUE_NUMBER, metadata.valueType, entry.getValue());
      writeMapEntryField(this, MAP_KEY_NUMBER, metadata.keyType, entry.getKey());
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }
  }

  static final void writeMapEntryField(
      Writer writer, int fieldNumber, WireFormat.FieldType fieldType, Object object)
      throws IOException {
    switch (fieldType) {
      case BOOL:
        writer.writeBool(fieldNumber, (Boolean) object);
        break;
      case FIXED32:
        writer.writeFixed32(fieldNumber, (Integer) object);
        break;
      case FIXED64:
        writer.writeFixed64(fieldNumber, (Long) object);
        break;
      case INT32:
        writer.writeInt32(fieldNumber, (Integer) object);
        break;
      case INT64:
        writer.writeInt64(fieldNumber, (Long) object);
        break;
      case SFIXED32:
        writer.writeSFixed32(fieldNumber, (Integer) object);
        break;
      case SFIXED64:
        writer.writeSFixed64(fieldNumber, (Long) object);
        break;
      case SINT32:
        writer.writeSInt32(fieldNumber, (Integer) object);
        break;
      case SINT64:
        writer.writeSInt64(fieldNumber, (Long) object);
        break;
      case STRING:
        writer.writeString(fieldNumber, (String) object);
        break;
      case UINT32:
        writer.writeUInt32(fieldNumber, (Integer) object);
        break;
      case UINT64:
        writer.writeUInt64(fieldNumber, (Long) object);
        break;
      case FLOAT:
        writer.writeFloat(fieldNumber, (Float) object);
        break;
      case DOUBLE:
        writer.writeDouble(fieldNumber, (Double) object);
        break;
      case MESSAGE:
        writer.writeMessage(fieldNumber, object);
        break;
      case BYTES:
        writer.writeBytes(fieldNumber, (ByteString) object);
        break;
      case ENUM:
        if (object instanceof Internal.EnumLite) {
          writer.writeEnum(fieldNumber, ((Internal.EnumLite) object).getNumber());
        } else if (object instanceof Integer) {
          writer.writeEnum(fieldNumber, (Integer) object);
        } else {
          throw new IllegalArgumentException("Unexpected type for enum in map.");
        }
        break;
      default:
        throw new IllegalArgumentException("Unsupported map value type for: " + fieldType);
    }
  }

  private void writeSInt64List_Internal(int fieldNumber, List<Long> list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeSInt64(list.get(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeSInt64(fieldNumber, list.get(i));
      }
    }
  }

  private void writeSInt64List_Internal(int fieldNumber, LongArrayList list, boolean packed)
      throws IOException {
    if (packed) {
      requireSpace((MAX_VARINT32_SIZE * 2) + (list.size() * MAX_VARINT64_SIZE));
      int prevBytes = getTotalBytesWritten();
      for (int i = list.size() - 1; i >= 0; --i) {
        writeSInt64(list.getLong(i));
      }
      int length = getTotalBytesWritten() - prevBytes;
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    } else {
      for (int i = list.size() - 1; i >= 0; --i) {
        writeSInt64(fieldNumber, list.getLong(i));
      }
    }
  }

  @Override
  public final void writeMessageList(int fieldNumber, List<?> list) throws IOException {
    for (int i = list.size() - 1; i >= 0; i--) {
      writeMessage(fieldNumber, list.get(i));
    }
  }

  @Override
  public final void writeMessageList(int fieldNumber, List<?> list, Schema schema)
      throws IOException {
    for (int i = list.size() - 1; i >= 0; i--) {
      writeMessage(fieldNumber, list.get(i), schema);
    }
  }

  @Deprecated
  @Override
  public final void writeGroupList(int fieldNumber, List<?> list) throws IOException {
    for (int i = list.size() - 1; i >= 0; i--) {
      writeGroup(fieldNumber, list.get(i));
    }
  }

  @Deprecated
  @Override
  public final void writeGroupList(int fieldNumber, List<?> list, Schema schema)
      throws IOException {
    for (int i = list.size() - 1; i >= 0; i--) {
      writeGroup(fieldNumber, list.get(i), schema);
    }
  }

  @Override
  public final void writeMessageSetItem(int fieldNumber, Object value) throws IOException {
    writeTag(MESSAGE_SET_ITEM, WIRETYPE_END_GROUP);
    if (value instanceof ByteString) {
      writeBytes(MESSAGE_SET_MESSAGE, (ByteString) value);
    } else {
      writeMessage(MESSAGE_SET_MESSAGE, value);
    }
    writeUInt32(MESSAGE_SET_TYPE_ID, fieldNumber);
    writeTag(MESSAGE_SET_ITEM, WIRETYPE_START_GROUP);
  }

  final AllocatedBuffer newHeapBuffer() {
    return alloc.allocateHeapBuffer(chunkSize);
  }

  final AllocatedBuffer newHeapBuffer(int capacity) {
    return alloc.allocateHeapBuffer(Math.max(capacity, chunkSize));
  }

  final AllocatedBuffer newDirectBuffer() {
    return alloc.allocateDirectBuffer(chunkSize);
  }

  final AllocatedBuffer newDirectBuffer(int capacity) {
    return alloc.allocateDirectBuffer(Math.max(capacity, chunkSize));
  }

  /**
   * Gets the total number of bytes that have been written. This will not be reset by a call to
   * {@link #complete()}.
   */
  public abstract int getTotalBytesWritten();

  abstract void requireSpace(int size);

  abstract void finishCurrentBuffer();

  abstract void writeTag(int fieldNumber, int wireType);

  abstract void writeVarint32(int value);

  abstract void writeInt32(int value);

  abstract void writeSInt32(int value);

  abstract void writeFixed32(int value);

  abstract void writeVarint64(long value);

  abstract void writeSInt64(long value);

  abstract void writeFixed64(long value);

  abstract void writeBool(boolean value);

  abstract void writeString(String in);

  /**
   * Not using the version in CodedOutputStream due to the fact that benchmarks have shown a
   * performance improvement when returning a byte (rather than an int).
   */
  private static byte computeUInt64SizeNoTag(long value) {
    // handle two popular special cases up front ...
    if ((value & (~0L << 7)) == 0L) {
      // Byte 1
      return 1;
    }
    if (value < 0L) {
      // Byte 10
      return 10;
    }
    // ... leaving us with 8 remaining, which we can divide and conquer
    byte n = 2;
    if ((value & (~0L << 35)) != 0L) {
      // Byte 6-9
      n += 4; // + (value >>> 63);
      value >>>= 28;
    }
    if ((value & (~0L << 21)) != 0L) {
      // Byte 4-5 or 8-9
      n += 2;
      value >>>= 14;
    }
    if ((value & (~0L << 14)) != 0L) {
      // Byte 3 or 7
      n += 1;
    }
    return n;
  }

  /** Writer that uses safe operations on target array. */
  private static final class SafeHeapWriter extends BinaryWriter {
    private AllocatedBuffer allocatedBuffer;
    private byte[] buffer;
    private int offset;
    private int limit;
    private int offsetMinusOne;
    private int limitMinusOne;
    private int pos;

    SafeHeapWriter(BufferAllocator alloc, int chunkSize) {
      super(alloc, chunkSize);
      nextBuffer();
    }

    @Override
    void finishCurrentBuffer() {
      if (allocatedBuffer != null) {
        totalDoneBytes += bytesWrittenToCurrentBuffer();
        allocatedBuffer.position((pos - allocatedBuffer.arrayOffset()) + 1);
        allocatedBuffer = null;
        pos = 0;
        limitMinusOne = 0;
      }
    }

    private void nextBuffer() {
      nextBuffer(newHeapBuffer());
    }

    private void nextBuffer(int capacity) {
      nextBuffer(newHeapBuffer(capacity));
    }

    private void nextBuffer(AllocatedBuffer allocatedBuffer) {
      if (!allocatedBuffer.hasArray()) {
        throw new RuntimeException("Allocator returned non-heap buffer");
      }

      finishCurrentBuffer();

      buffers.addFirst(allocatedBuffer);

      this.allocatedBuffer = allocatedBuffer;
      this.buffer = allocatedBuffer.array();
      int arrayOffset = allocatedBuffer.arrayOffset();
      this.limit = arrayOffset + allocatedBuffer.limit();
      this.offset = arrayOffset + allocatedBuffer.position();
      this.offsetMinusOne = offset - 1;
      this.limitMinusOne = limit - 1;
      this.pos = limitMinusOne;
    }

    @Override
    public int getTotalBytesWritten() {
      return totalDoneBytes + bytesWrittenToCurrentBuffer();
    }

    int bytesWrittenToCurrentBuffer() {
      return limitMinusOne - pos;
    }

    int spaceLeft() {
      return pos - offsetMinusOne;
    }

    @Override
    public void writeUInt32(int fieldNumber, int value) throws IOException {
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeInt32(int fieldNumber, int value) throws IOException {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeInt32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeSInt32(int fieldNumber, int value) throws IOException {
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeSInt32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeFixed32(int fieldNumber, int value) throws IOException {
      requireSpace(MAX_VARINT32_SIZE + FIXED32_SIZE);
      writeFixed32(value);
      writeTag(fieldNumber, WIRETYPE_FIXED32);
    }

    @Override
    public void writeUInt64(int fieldNumber, long value) throws IOException {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeVarint64(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeSInt64(int fieldNumber, long value) throws IOException {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeSInt64(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeFixed64(int fieldNumber, long value) throws IOException {
      requireSpace(MAX_VARINT32_SIZE + FIXED64_SIZE);
      writeFixed64(value);
      writeTag(fieldNumber, WIRETYPE_FIXED64);
    }

    @Override
    public void writeBool(int fieldNumber, boolean value) throws IOException {
      requireSpace(MAX_VARINT32_SIZE + 1);
      write((byte) (value ? 1 : 0));
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeString(int fieldNumber, String value) throws IOException {
      int prevBytes = getTotalBytesWritten();
      writeString(value);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(2 * MAX_VARINT32_SIZE);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeBytes(int fieldNumber, ByteString value) throws IOException {
      try {
        value.writeToReverse(this);
      } catch (IOException e) {
        // Should never happen since the writer does not throw.
        throw new RuntimeException(e);
      }

      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(value.size());
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeMessage(int fieldNumber, Object value) throws IOException {
      int prevBytes = getTotalBytesWritten();
      Protobuf.getInstance().writeTo(value, this);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeMessage(int fieldNumber, Object value, Schema schema) throws IOException {
      int prevBytes = getTotalBytesWritten();
      schema.writeTo(value, this);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Deprecated
    @Override
    public void writeGroup(int fieldNumber, Object value) throws IOException {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
      Protobuf.getInstance().writeTo(value, this);
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Override
    public void writeGroup(int fieldNumber, Object value, Schema schema) throws IOException {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
      schema.writeTo(value, this);
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Override
    public void writeStartGroup(int fieldNumber) {
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Override
    public void writeEndGroup(int fieldNumber) {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
    }

    @Override
    void writeInt32(int value) {
      if (value >= 0) {
        writeVarint32(value);
      } else {
        writeVarint64(value);
      }
    }

    @Override
    void writeSInt32(int value) {
      writeVarint32(CodedOutputStream.encodeZigZag32(value));
    }

    @Override
    void writeSInt64(long value) {
      writeVarint64(CodedOutputStream.encodeZigZag64(value));
    }

    @Override
    void writeBool(boolean value) {
      write((byte) (value ? 1 : 0));
    }

    @Override
    void writeTag(int fieldNumber, int wireType) {
      writeVarint32(WireFormat.makeTag(fieldNumber, wireType));
    }

    @Override
    void writeVarint32(int value) {
      if ((value & (~0 << 7)) == 0) {
        writeVarint32OneByte(value);
      } else if ((value & (~0 << 14)) == 0) {
        writeVarint32TwoBytes(value);
      } else if ((value & (~0 << 21)) == 0) {
        writeVarint32ThreeBytes(value);
      } else if ((value & (~0 << 28)) == 0) {
        writeVarint32FourBytes(value);
      } else {
        writeVarint32FiveBytes(value);
      }
    }

    private void writeVarint32OneByte(int value) {
      buffer[pos--] = (byte) value;
    }

    private void writeVarint32TwoBytes(int value) {
      buffer[pos--] = (byte) (value >>> 7);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint32ThreeBytes(int value) {
      buffer[pos--] = (byte) (value >>> 14);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint32FourBytes(int value) {
      buffer[pos--] = (byte) (value >>> 21);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint32FiveBytes(int value) {
      buffer[pos--] = (byte) (value >>> 28);
      buffer[pos--] = (byte) (((value >>> 21) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    @Override
    void writeVarint64(long value) {
      switch (computeUInt64SizeNoTag(value)) {
        case 1:
          writeVarint64OneByte(value);
          break;
        case 2:
          writeVarint64TwoBytes(value);
          break;
        case 3:
          writeVarint64ThreeBytes(value);
          break;
        case 4:
          writeVarint64FourBytes(value);
          break;
        case 5:
          writeVarint64FiveBytes(value);
          break;
        case 6:
          writeVarint64SixBytes(value);
          break;
        case 7:
          writeVarint64SevenBytes(value);
          break;
        case 8:
          writeVarint64EightBytes(value);
          break;
        case 9:
          writeVarint64NineBytes(value);
          break;
        case 10:
          writeVarint64TenBytes(value);
          break;
      }
    }

    private void writeVarint64OneByte(long value) {
      buffer[pos--] = (byte) value;
    }

    private void writeVarint64TwoBytes(long value) {
      buffer[pos--] = (byte) (value >>> 7);
      buffer[pos--] = (byte) (((int) value & 0x7F) | 0x80);
    }

    private void writeVarint64ThreeBytes(long value) {
      buffer[pos--] = (byte) (((int) value) >>> 14);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint64FourBytes(long value) {
      buffer[pos--] = (byte) (value >>> 21);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint64FiveBytes(long value) {
      buffer[pos--] = (byte) (value >>> 28);
      buffer[pos--] = (byte) (((value >>> 21) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint64SixBytes(long value) {
      buffer[pos--] = (byte) (value >>> 35);
      buffer[pos--] = (byte) (((value >>> 28) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 21) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint64SevenBytes(long value) {
      buffer[pos--] = (byte) (value >>> 42);
      buffer[pos--] = (byte) (((value >>> 35) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 28) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 21) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint64EightBytes(long value) {
      buffer[pos--] = (byte) (value >>> 49);
      buffer[pos--] = (byte) (((value >>> 42) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 35) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 28) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 21) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint64NineBytes(long value) {
      buffer[pos--] = (byte) (value >>> 56);
      buffer[pos--] = (byte) (((value >>> 49) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 42) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 35) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 28) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 21) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    private void writeVarint64TenBytes(long value) {
      buffer[pos--] = (byte) (value >>> 63);
      buffer[pos--] = (byte) (((value >>> 56) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 49) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 42) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 35) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 28) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 21) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 14) & 0x7F) | 0x80);
      buffer[pos--] = (byte) (((value >>> 7) & 0x7F) | 0x80);
      buffer[pos--] = (byte) ((value & 0x7F) | 0x80);
    }

    @Override
    void writeFixed32(int value) {
      buffer[pos--] = (byte) ((value >> 24) & 0xFF);
      buffer[pos--] = (byte) ((value >> 16) & 0xFF);
      buffer[pos--] = (byte) ((value >> 8) & 0xFF);
      buffer[pos--] = (byte) (value & 0xFF);
    }

    @Override
    void writeFixed64(long value) {
      buffer[pos--] = (byte) ((int) (value >> 56) & 0xFF);
      buffer[pos--] = (byte) ((int) (value >> 48) & 0xFF);
      buffer[pos--] = (byte) ((int) (value >> 40) & 0xFF);
      buffer[pos--] = (byte) ((int) (value >> 32) & 0xFF);
      buffer[pos--] = (byte) ((int) (value >> 24) & 0xFF);
      buffer[pos--] = (byte) ((int) (value >> 16) & 0xFF);
      buffer[pos--] = (byte) ((int) (value >> 8) & 0xFF);
      buffer[pos--] = (byte) ((int) (value) & 0xFF);
    }

    @Override
    void writeString(String in) {
      // Request enough space to write the ASCII string.
      requireSpace(in.length());

      // We know the buffer is big enough...
      int i = in.length() - 1;
      // Set pos to the start of the ASCII string.
      pos -= i;
      // Designed to take advantage of
      // https://wiki.openjdk.java.net/display/HotSpotInternals/RangeCheckElimination
      for (char c; i >= 0 && (c = in.charAt(i)) < 0x80; i--) {
        buffer[pos + i] = (byte) c;
      }
      if (i == -1) {
        // Move pos past the String.
        pos -= 1;
        return;
      }
      pos += i;
      for (char c; i >= 0; i--) {
        c = in.charAt(i);
        if (c < 0x80 && pos > offsetMinusOne) {
          buffer[pos--] = (byte) c;
        } else if (c < 0x800 && pos > offset) { // 11 bits, two UTF-8 bytes
          buffer[pos--] = (byte) (0x80 | (0x3F & c));
          buffer[pos--] = (byte) ((0xF << 6) | (c >>> 6));
        } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c)
            && pos > (offset + 1)) {
          // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
          buffer[pos--] = (byte) (0x80 | (0x3F & c));
          buffer[pos--] = (byte) (0x80 | (0x3F & (c >>> 6)));
          buffer[pos--] = (byte) ((0xF << 5) | (c >>> 12));
        } else if (pos > (offset + 2)) {
          // Minimum code point represented by a surrogate pair is 0x10000, 17 bits,
          // four UTF-8 bytes
          char high = 0;
          if (i == 0 || !Character.isSurrogatePair(high = in.charAt(i - 1), c)) {
            throw new Utf8.UnpairedSurrogateException(i - 1, i);
          }
          i--;
          int codePoint = Character.toCodePoint(high, c);
          buffer[pos--] = (byte) (0x80 | (0x3F & codePoint));
          buffer[pos--] = (byte) (0x80 | (0x3F & (codePoint >>> 6)));
          buffer[pos--] = (byte) (0x80 | (0x3F & (codePoint >>> 12)));
          buffer[pos--] = (byte) ((0xF << 4) | (codePoint >>> 18));
        } else {
          // Buffer is full - allocate a new one and revisit the current character.
          requireSpace(i);
          i++;
        }
      }
    }

    @Override
    public void write(byte value) {
      buffer[pos--] = value;
    }

    @Override
    public void write(byte[] value, int offset, int length) {
      if (spaceLeft() < length) {
        nextBuffer(length);
      }

      pos -= length;
      System.arraycopy(value, offset, buffer, pos + 1, length);
    }

    @Override
    public void writeLazy(byte[] value, int offset, int length) {
      if (spaceLeft() < length) {
        // We consider the value to be immutable (likely the internals of a ByteString). Just
        // wrap it in a Netty buffer and add it to the output buffer.
        totalDoneBytes += length;
        buffers.addFirst(AllocatedBuffer.wrap(value, offset, length));

        // Advance the writer to the next buffer.
        // TODO(nathanmittler): Consider slicing if space available above some threshold.
        nextBuffer();
        return;
      }

      pos -= length;
      System.arraycopy(value, offset, buffer, pos + 1, length);
    }

    @Override
    public void write(ByteBuffer value) {
      int length = value.remaining();
      if (spaceLeft() < length) {
        nextBuffer(length);
      }

      pos -= length;
      value.get(buffer, pos + 1, length);
    }

    @Override
    public void writeLazy(ByteBuffer value) {
      int length = value.remaining();
      if (spaceLeft() < length) {
        // We consider the value to be immutable (likely the internals of a ByteString). Just
        // wrap it in a Netty buffer and add it to the output buffer.
        totalDoneBytes += length;
        buffers.addFirst(AllocatedBuffer.wrap(value));

        // Advance the writer to the next buffer.
        // TODO(nathanmittler): Consider slicing if space available above some threshold.
        nextBuffer();
      }

      pos -= length;
      value.get(buffer, pos + 1, length);
    }

    @Override
    void requireSpace(int size) {
      if (spaceLeft() < size) {
        nextBuffer(size);
      }
    }
  }

  /** Writer that uses unsafe operations on a target array. */
  private static final class UnsafeHeapWriter extends BinaryWriter {
    private AllocatedBuffer allocatedBuffer;
    private byte[] buffer;
    private long offset;
    private long limit;
    private long offsetMinusOne;
    private long limitMinusOne;
    private long pos;

    UnsafeHeapWriter(BufferAllocator alloc, int chunkSize) {
      super(alloc, chunkSize);
      nextBuffer();
    }

    /** Indicates whether the required unsafe operations are supported on this platform. */
    static boolean isSupported() {
      return UnsafeUtil.hasUnsafeArrayOperations();
    }

    @Override
    void finishCurrentBuffer() {
      if (allocatedBuffer != null) {
        totalDoneBytes += bytesWrittenToCurrentBuffer();
        allocatedBuffer.position((arrayPos() - allocatedBuffer.arrayOffset()) + 1);
        allocatedBuffer = null;
        pos = 0;
        limitMinusOne = 0;
      }
    }

    private int arrayPos() {
      return (int) pos;
    }

    private void nextBuffer() {
      nextBuffer(newHeapBuffer());
    }

    private void nextBuffer(int capacity) {
      nextBuffer(newHeapBuffer(capacity));
    }

    private void nextBuffer(AllocatedBuffer allocatedBuffer) {
      if (!allocatedBuffer.hasArray()) {
        throw new RuntimeException("Allocator returned non-heap buffer");
      }

      finishCurrentBuffer();
      buffers.addFirst(allocatedBuffer);

      this.allocatedBuffer = allocatedBuffer;
      this.buffer = allocatedBuffer.array();
      int arrayOffset = allocatedBuffer.arrayOffset();
      this.limit = (long) arrayOffset + allocatedBuffer.limit();
      this.offset = (long) arrayOffset + allocatedBuffer.position();
      this.offsetMinusOne = offset - 1;
      this.limitMinusOne = limit - 1;
      this.pos = limitMinusOne;
    }

    @Override
    public int getTotalBytesWritten() {
      return totalDoneBytes + bytesWrittenToCurrentBuffer();
    }

    int bytesWrittenToCurrentBuffer() {
      return (int) (limitMinusOne - pos);
    }

    int spaceLeft() {
      return (int) (pos - offsetMinusOne);
    }

    @Override
    public void writeUInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeInt32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeSInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeSInt32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeFixed32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE + FIXED32_SIZE);
      writeFixed32(value);
      writeTag(fieldNumber, WIRETYPE_FIXED32);
    }

    @Override
    public void writeUInt64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeVarint64(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeSInt64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeSInt64(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeFixed64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + FIXED64_SIZE);
      writeFixed64(value);
      writeTag(fieldNumber, WIRETYPE_FIXED64);
    }

    @Override
    public void writeBool(int fieldNumber, boolean value) {
      requireSpace(MAX_VARINT32_SIZE + 1);
      write((byte) (value ? 1 : 0));
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeString(int fieldNumber, String value) {
      int prevBytes = getTotalBytesWritten();
      writeString(value);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(2 * MAX_VARINT32_SIZE);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeBytes(int fieldNumber, ByteString value) {
      try {
        value.writeToReverse(this);
      } catch (IOException e) {
        // Should never happen since the writer does not throw.
        throw new RuntimeException(e);
      }

      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(value.size());
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeMessage(int fieldNumber, Object value) throws IOException {
      int prevBytes = getTotalBytesWritten();
      Protobuf.getInstance().writeTo(value, this);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeMessage(int fieldNumber, Object value, Schema schema) throws IOException {
      int prevBytes = getTotalBytesWritten();
      schema.writeTo(value, this);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeGroup(int fieldNumber, Object value) throws IOException {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
      Protobuf.getInstance().writeTo(value, this);
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Override
    public void writeGroup(int fieldNumber, Object value, Schema schema) throws IOException {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
      schema.writeTo(value, this);
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Override
    public void writeStartGroup(int fieldNumber) {
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Override
    public void writeEndGroup(int fieldNumber) {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
    }

    @Override
    void writeInt32(int value) {
      if (value >= 0) {
        writeVarint32(value);
      } else {
        writeVarint64(value);
      }
    }

    @Override
    void writeSInt32(int value) {
      writeVarint32(CodedOutputStream.encodeZigZag32(value));
    }

    @Override
    void writeSInt64(long value) {
      writeVarint64(CodedOutputStream.encodeZigZag64(value));
    }

    @Override
    void writeBool(boolean value) {
      write((byte) (value ? 1 : 0));
    }

    @Override
    void writeTag(int fieldNumber, int wireType) {
      writeVarint32(WireFormat.makeTag(fieldNumber, wireType));
    }

    @Override
    void writeVarint32(int value) {
      if ((value & (~0 << 7)) == 0) {
        writeVarint32OneByte(value);
      } else if ((value & (~0 << 14)) == 0) {
        writeVarint32TwoBytes(value);
      } else if ((value & (~0 << 21)) == 0) {
        writeVarint32ThreeBytes(value);
      } else if ((value & (~0 << 28)) == 0) {
        writeVarint32FourBytes(value);
      } else {
        writeVarint32FiveBytes(value);
      }
    }

    private void writeVarint32OneByte(int value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) value);
    }

    private void writeVarint32TwoBytes(int value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 7));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint32ThreeBytes(int value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 14));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint32FourBytes(int value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 21));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint32FiveBytes(int value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 28));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    @Override
    void writeVarint64(long value) {
      switch (computeUInt64SizeNoTag(value)) {
        case 1:
          writeVarint64OneByte(value);
          break;
        case 2:
          writeVarint64TwoBytes(value);
          break;
        case 3:
          writeVarint64ThreeBytes(value);
          break;
        case 4:
          writeVarint64FourBytes(value);
          break;
        case 5:
          writeVarint64FiveBytes(value);
          break;
        case 6:
          writeVarint64SixBytes(value);
          break;
        case 7:
          writeVarint64SevenBytes(value);
          break;
        case 8:
          writeVarint64EightBytes(value);
          break;
        case 9:
          writeVarint64NineBytes(value);
          break;
        case 10:
          writeVarint64TenBytes(value);
          break;
      }
    }

    private void writeVarint64OneByte(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) value);
    }

    private void writeVarint64TwoBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 7));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((int) value & 0x7F) | 0x80));
    }

    private void writeVarint64ThreeBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (((int) value) >>> 14));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64FourBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 21));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64FiveBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 28));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64SixBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 35));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64SevenBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 42));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 35) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64EightBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 49));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 42) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 35) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64NineBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 56));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 49) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 42) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 35) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64TenBytes(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) (value >>> 63));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 56) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 49) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 42) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 35) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value & 0x7F) | 0x80));
    }

    @Override
    void writeFixed32(int value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value >> 24) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value >> 16) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((value >> 8) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) (value & 0xFF));
    }

    @Override
    void writeFixed64(long value) {
      UnsafeUtil.putByte(buffer, pos--, (byte) ((int) (value >> 56) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((int) (value >> 48) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((int) (value >> 40) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((int) (value >> 32) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((int) (value >> 24) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((int) (value >> 16) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((int) (value >> 8) & 0xFF));
      UnsafeUtil.putByte(buffer, pos--, (byte) ((int) (value) & 0xFF));
    }

    @Override
    void writeString(String in) {
      // Request enough space to write the ASCII string.
      requireSpace(in.length());

      // We know the buffer is big enough...
      int i = in.length() - 1;
      // Set pos to the start of the ASCII string.
      // pos -= i;
      // Designed to take advantage of
      // https://wiki.openjdk.java.net/display/HotSpotInternals/RangeCheckElimination
      for (char c; i >= 0 && (c = in.charAt(i)) < 0x80; i--) {
        UnsafeUtil.putByte(buffer, pos--, (byte) c);
      }
      if (i == -1) {
        // Move pos past the String.
        return;
      }
      for (char c; i >= 0; i--) {
        c = in.charAt(i);
        if (c < 0x80 && pos > offsetMinusOne) {
          UnsafeUtil.putByte(buffer, pos--, (byte) c);
        } else if (c < 0x800 && pos > offset) { // 11 bits, two UTF-8 bytes
          UnsafeUtil.putByte(buffer, pos--, (byte) (0x80 | (0x3F & c)));
          UnsafeUtil.putByte(buffer, pos--, (byte) ((0xF << 6) | (c >>> 6)));
        } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c)
            && pos > offset + 1) {
          // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
          UnsafeUtil.putByte(buffer, pos--, (byte) (0x80 | (0x3F & c)));
          UnsafeUtil.putByte(buffer, pos--, (byte) (0x80 | (0x3F & (c >>> 6))));
          UnsafeUtil.putByte(buffer, pos--, (byte) ((0xF << 5) | (c >>> 12)));
        } else if (pos > offset + 2) {
          // Minimum code point represented by a surrogate pair is 0x10000, 17 bits,
          // four UTF-8 bytes
          final char high;
          if (i == 0 || !Character.isSurrogatePair(high = in.charAt(i - 1), c)) {
            throw new Utf8.UnpairedSurrogateException(i - 1, i);
          }
          i--;
          int codePoint = Character.toCodePoint(high, c);
          UnsafeUtil.putByte(buffer, pos--, (byte) (0x80 | (0x3F & codePoint)));
          UnsafeUtil.putByte(buffer, pos--, (byte) (0x80 | (0x3F & (codePoint >>> 6))));
          UnsafeUtil.putByte(buffer, pos--, (byte) (0x80 | (0x3F & (codePoint >>> 12))));
          UnsafeUtil.putByte(buffer, pos--, (byte) ((0xF << 4) | (codePoint >>> 18)));
        } else {
          // Buffer is full - allocate a new one and revisit the current character.
          requireSpace(i);
          i++;
        }
      }
    }

    @Override
    public void write(byte value) {
      UnsafeUtil.putByte(buffer, pos--, value);
    }

    @Override
    public void write(byte[] value, int offset, int length) {
      if (offset < 0 || offset + length > value.length) {
        throw new ArrayIndexOutOfBoundsException(
            String.format("value.length=%d, offset=%d, length=%d", value.length, offset, length));
      }
      requireSpace(length);

      pos -= length;
      System.arraycopy(value, offset, buffer, arrayPos() + 1, length);
    }

    @Override
    public void writeLazy(byte[] value, int offset, int length) {
      if (offset < 0 || offset + length > value.length) {
        throw new ArrayIndexOutOfBoundsException(
            String.format("value.length=%d, offset=%d, length=%d", value.length, offset, length));
      }
      if (spaceLeft() < length) {
        // We consider the value to be immutable (likely the internals of a ByteString). Just
        // wrap it in a Netty buffer and add it to the output buffer.
        totalDoneBytes += length;
        buffers.addFirst(AllocatedBuffer.wrap(value, offset, length));

        // Advance the writer to the next buffer.
        // TODO(nathanmittler): Consider slicing if space available above some threshold.
        nextBuffer();
        return;
      }

      pos -= length;
      System.arraycopy(value, offset, buffer, arrayPos() + 1, length);
    }

    @Override
    public void write(ByteBuffer value) {
      int length = value.remaining();
      requireSpace(length);

      pos -= length;
      value.get(buffer, arrayPos() + 1, length);
    }

    @Override
    public void writeLazy(ByteBuffer value) {
      int length = value.remaining();
      if (spaceLeft() < length) {
        // We consider the value to be immutable (likely the internals of a ByteString). Just
        // wrap it in a Netty buffer and add it to the output buffer.
        totalDoneBytes += length;
        buffers.addFirst(AllocatedBuffer.wrap(value));

        // Advance the writer to the next buffer.
        // TODO(nathanmittler): Consider slicing if space available above some threshold.
        nextBuffer();
      }

      pos -= length;
      value.get(buffer, arrayPos() + 1, length);
    }

    @Override
    void requireSpace(int size) {
      if (spaceLeft() < size) {
        nextBuffer(size);
      }
    }
  }

  /** Writer that uses safe operations on a target {@link ByteBuffer}. */
  private static final class SafeDirectWriter extends BinaryWriter {
    private ByteBuffer buffer;
    private int limitMinusOne;
    private int pos;

    SafeDirectWriter(BufferAllocator alloc, int chunkSize) {
      super(alloc, chunkSize);
      nextBuffer();
    }

    private void nextBuffer() {
      nextBuffer(newDirectBuffer());
    }

    private void nextBuffer(int capacity) {
      nextBuffer(newDirectBuffer(capacity));
    }

    private void nextBuffer(AllocatedBuffer allocatedBuffer) {
      if (!allocatedBuffer.hasNioBuffer()) {
        throw new RuntimeException("Allocated buffer does not have NIO buffer");
      }
      ByteBuffer nioBuffer = allocatedBuffer.nioBuffer();
      if (!nioBuffer.isDirect()) {
        throw new RuntimeException("Allocator returned non-direct buffer");
      }

      finishCurrentBuffer();
      buffers.addFirst(allocatedBuffer);

      buffer = nioBuffer;
      Java8Compatibility.limit(buffer, buffer.capacity());
      Java8Compatibility.position(buffer, 0);
      // Set byte order to little endian for fast writing of fixed 32/64.
      buffer.order(ByteOrder.LITTLE_ENDIAN);

      limitMinusOne = buffer.limit() - 1;
      pos = limitMinusOne;
    }

    @Override
    public int getTotalBytesWritten() {
      return totalDoneBytes + bytesWrittenToCurrentBuffer();
    }

    private int bytesWrittenToCurrentBuffer() {
      return limitMinusOne - pos;
    }

    private int spaceLeft() {
      return pos + 1;
    }

    @Override
    void finishCurrentBuffer() {
      if (buffer != null) {
        totalDoneBytes += bytesWrittenToCurrentBuffer();
        // Update the indices on the netty buffer.
        Java8Compatibility.position(buffer, pos + 1);
        buffer = null;
        pos = 0;
        limitMinusOne = 0;
      }
    }

    @Override
    public void writeUInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeInt32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeSInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeSInt32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeFixed32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE + FIXED32_SIZE);
      writeFixed32(value);
      writeTag(fieldNumber, WIRETYPE_FIXED32);
    }

    @Override
    public void writeUInt64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeVarint64(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeSInt64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeSInt64(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeFixed64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + FIXED64_SIZE);
      writeFixed64(value);
      writeTag(fieldNumber, WIRETYPE_FIXED64);
    }

    @Override
    public void writeBool(int fieldNumber, boolean value) {
      requireSpace(MAX_VARINT32_SIZE + 1);
      write((byte) (value ? 1 : 0));
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeString(int fieldNumber, String value) {
      int prevBytes = getTotalBytesWritten();
      writeString(value);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(2 * MAX_VARINT32_SIZE);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeBytes(int fieldNumber, ByteString value) {
      try {
        value.writeToReverse(this);
      } catch (IOException e) {
        // Should never happen since the writer does not throw.
        throw new RuntimeException(e);
      }

      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(value.size());
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeMessage(int fieldNumber, Object value) throws IOException {
      int prevBytes = getTotalBytesWritten();
      Protobuf.getInstance().writeTo(value, this);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeMessage(int fieldNumber, Object value, Schema schema) throws IOException {
      int prevBytes = getTotalBytesWritten();
      schema.writeTo(value, this);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Deprecated
    @Override
    public void writeGroup(int fieldNumber, Object value) throws IOException {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
      Protobuf.getInstance().writeTo(value, this);
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Override
    public void writeGroup(int fieldNumber, Object value, Schema schema) throws IOException {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
      schema.writeTo(value, this);
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Deprecated
    @Override
    public void writeStartGroup(int fieldNumber) {
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Deprecated
    @Override
    public void writeEndGroup(int fieldNumber) {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
    }

    @Override
    void writeInt32(int value) {
      if (value >= 0) {
        writeVarint32(value);
      } else {
        writeVarint64(value);
      }
    }

    @Override
    void writeSInt32(int value) {
      writeVarint32(CodedOutputStream.encodeZigZag32(value));
    }

    @Override
    void writeSInt64(long value) {
      writeVarint64(CodedOutputStream.encodeZigZag64(value));
    }

    @Override
    void writeBool(boolean value) {
      write((byte) (value ? 1 : 0));
    }

    @Override
    void writeTag(int fieldNumber, int wireType) {
      writeVarint32(WireFormat.makeTag(fieldNumber, wireType));
    }

    @Override
    void writeVarint32(int value) {
      if ((value & (~0 << 7)) == 0) {
        writeVarint32OneByte(value);
      } else if ((value & (~0 << 14)) == 0) {
        writeVarint32TwoBytes(value);
      } else if ((value & (~0 << 21)) == 0) {
        writeVarint32ThreeBytes(value);
      } else if ((value & (~0 << 28)) == 0) {
        writeVarint32FourBytes(value);
      } else {
        writeVarint32FiveBytes(value);
      }
    }

    private void writeVarint32OneByte(int value) {
      buffer.put(pos--, (byte) value);
    }

    private void writeVarint32TwoBytes(int value) {
      // Byte order is little-endian.
      pos -= 2;
      buffer.putShort(pos + 1, (short) (((value & (0x7F << 7)) << 1) | ((value & 0x7F) | 0x80)));
    }

    private void writeVarint32ThreeBytes(int value) {
      // Byte order is little-endian.
      pos -= 3;
      buffer.putInt(
          pos,
          ((value & (0x7F << 14)) << 10)
              | (((value & (0x7F << 7)) | (0x80 << 7)) << 9)
              | ((value & 0x7F) | 0x80) << 8);
    }

    private void writeVarint32FourBytes(int value) {
      // Byte order is little-endian.
      pos -= 4;
      buffer.putInt(
          pos + 1,
          ((value & (0x7F << 21)) << 3)
              | (((value & (0x7F << 14)) | (0x80 << 14)) << 2)
              | (((value & (0x7F << 7)) | (0x80 << 7)) << 1)
              | ((value & 0x7F) | 0x80));
    }

    private void writeVarint32FiveBytes(int value) {
      // Byte order is little-endian.
      buffer.put(pos--, (byte) (value >>> 28));
      pos -= 4;
      buffer.putInt(
          pos + 1,
          ((((value >>> 21) & 0x7F) | 0x80) << 24)
              | ((((value >>> 14) & 0x7F) | 0x80) << 16)
              | ((((value >>> 7) & 0x7F) | 0x80) << 8)
              | ((value & 0x7F) | 0x80));
    }

    @Override
    void writeVarint64(long value) {
      switch (computeUInt64SizeNoTag(value)) {
        case 1:
          writeVarint64OneByte(value);
          break;
        case 2:
          writeVarint64TwoBytes(value);
          break;
        case 3:
          writeVarint64ThreeBytes(value);
          break;
        case 4:
          writeVarint64FourBytes(value);
          break;
        case 5:
          writeVarint64FiveBytes(value);
          break;
        case 6:
          writeVarint64SixBytes(value);
          break;
        case 7:
          writeVarint64SevenBytes(value);
          break;
        case 8:
          writeVarint64EightBytes(value);
          break;
        case 9:
          writeVarint64NineBytes(value);
          break;
        case 10:
          writeVarint64TenBytes(value);
          break;
      }
    }

    private void writeVarint64OneByte(long value) {
      writeVarint32OneByte((int) value);
    }

    private void writeVarint64TwoBytes(long value) {
      writeVarint32TwoBytes((int) value);
    }

    private void writeVarint64ThreeBytes(long value) {
      writeVarint32ThreeBytes((int) value);
    }

    private void writeVarint64FourBytes(long value) {
      writeVarint32FourBytes((int) value);
    }

    private void writeVarint64FiveBytes(long value) {
      // Byte order is little-endian.
      pos -= 5;
      buffer.putLong(
          pos - 2,
          ((value & (0x7FL << 28)) << 28)
              | (((value & (0x7F << 21)) | (0x80 << 21)) << 27)
              | (((value & (0x7F << 14)) | (0x80 << 14)) << 26)
              | (((value & (0x7F << 7)) | (0x80 << 7)) << 25)
              | (((value & 0x7F) | 0x80)) << 24);
    }

    private void writeVarint64SixBytes(long value) {
      // Byte order is little-endian.
      pos -= 6;
      buffer.putLong(
          pos - 1,
          ((value & (0x7FL << 35)) << 21)
              | (((value & (0x7FL << 28)) | (0x80L << 28)) << 20)
              | (((value & (0x7F << 21)) | (0x80 << 21)) << 19)
              | (((value & (0x7F << 14)) | (0x80 << 14)) << 18)
              | (((value & (0x7F << 7)) | (0x80 << 7)) << 17)
              | (((value & 0x7F) | 0x80)) << 16);
    }

    private void writeVarint64SevenBytes(long value) {
      // Byte order is little-endian.
      pos -= 7;
      buffer.putLong(
          pos,
          ((value & (0x7FL << 42)) << 14)
              | (((value & (0x7FL << 35)) | (0x80L << 35)) << 13)
              | (((value & (0x7FL << 28)) | (0x80L << 28)) << 12)
              | (((value & (0x7F << 21)) | (0x80 << 21)) << 11)
              | (((value & (0x7F << 14)) | (0x80 << 14)) << 10)
              | (((value & (0x7F << 7)) | (0x80 << 7)) << 9)
              | (((value & 0x7F) | 0x80)) << 8);
    }

    private void writeVarint64EightBytes(long value) {
      // Byte order is little-endian.
      pos -= 8;
      buffer.putLong(
          pos + 1,
          ((value & (0x7FL << 49)) << 7)
              | (((value & (0x7FL << 42)) | (0x80L << 42)) << 6)
              | (((value & (0x7FL << 35)) | (0x80L << 35)) << 5)
              | (((value & (0x7FL << 28)) | (0x80L << 28)) << 4)
              | (((value & (0x7F << 21)) | (0x80 << 21)) << 3)
              | (((value & (0x7F << 14)) | (0x80 << 14)) << 2)
              | (((value & (0x7F << 7)) | (0x80 << 7)) << 1)
              | ((value & 0x7F) | 0x80));
    }

    private void writeVarint64EightBytesWithSign(long value) {
      // Byte order is little-endian.
      pos -= 8;
      buffer.putLong(
          pos + 1,
          (((value & (0x7FL << 49)) | (0x80L << 49)) << 7)
              | (((value & (0x7FL << 42)) | (0x80L << 42)) << 6)
              | (((value & (0x7FL << 35)) | (0x80L << 35)) << 5)
              | (((value & (0x7FL << 28)) | (0x80L << 28)) << 4)
              | (((value & (0x7F << 21)) | (0x80 << 21)) << 3)
              | (((value & (0x7F << 14)) | (0x80 << 14)) << 2)
              | (((value & (0x7F << 7)) | (0x80 << 7)) << 1)
              | ((value & 0x7F) | 0x80));
    }

    private void writeVarint64NineBytes(long value) {
      buffer.put(pos--, (byte) (value >>> 56));
      writeVarint64EightBytesWithSign(value & 0xFFFFFFFFFFFFFFL);
    }

    private void writeVarint64TenBytes(long value) {
      buffer.put(pos--, (byte) (value >>> 63));
      buffer.put(pos--, (byte) (((value >>> 56) & 0x7F) | 0x80));
      writeVarint64EightBytesWithSign(value & 0xFFFFFFFFFFFFFFL);
    }

    @Override
    void writeFixed32(int value) {
      pos -= 4;
      buffer.putInt(pos + 1, value);
    }

    @Override
    void writeFixed64(long value) {
      pos -= 8;
      buffer.putLong(pos + 1, value);
    }

    @Override
    void writeString(String in) {
      // Request enough space to write the ASCII string.
      requireSpace(in.length());

      // We know the buffer is big enough...
      int i = in.length() - 1;
      pos -= i;
      // Designed to take advantage of
      // https://wiki.openjdk.java.net/display/HotSpotInternals/RangeCheckElimination
      for (char c; i >= 0 && (c = in.charAt(i)) < 0x80; i--) {
        buffer.put(pos + i, (byte) c);
      }
      if (i == -1) {
        // Move the position past the ASCII string.
        pos -= 1;
        return;
      }
      pos += i;
      for (char c; i >= 0; i--) {
        c = in.charAt(i);
        if (c < 0x80 && pos >= 0) {
          buffer.put(pos--, (byte) c);
        } else if (c < 0x800 && pos > 0) { // 11 bits, two UTF-8 bytes
          buffer.put(pos--, (byte) (0x80 | (0x3F & c)));
          buffer.put(pos--, (byte) ((0xF << 6) | (c >>> 6)));
        } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c) && pos > 1) {
          // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
          buffer.put(pos--, (byte) (0x80 | (0x3F & c)));
          buffer.put(pos--, (byte) (0x80 | (0x3F & (c >>> 6))));
          buffer.put(pos--, (byte) ((0xF << 5) | (c >>> 12)));
        } else if (pos > 2) {
          // Minimum code point represented by a surrogate pair is 0x10000, 17 bits,
          // four UTF-8 bytes
          char high = 0;
          if (i == 0 || !Character.isSurrogatePair(high = in.charAt(i - 1), c)) {
            throw new Utf8.UnpairedSurrogateException(i - 1, i);
          }
          i--;
          int codePoint = Character.toCodePoint(high, c);
          buffer.put(pos--, (byte) (0x80 | (0x3F & codePoint)));
          buffer.put(pos--, (byte) (0x80 | (0x3F & (codePoint >>> 6))));
          buffer.put(pos--, (byte) (0x80 | (0x3F & (codePoint >>> 12))));
          buffer.put(pos--, (byte) ((0xF << 4) | (codePoint >>> 18)));
        } else {
          // Buffer is full - allocate a new one and revisit the current character.
          requireSpace(i);
          i++;
        }
      }
    }

    @Override
    public void write(byte value) {
      buffer.put(pos--, value);
    }

    @Override
    public void write(byte[] value, int offset, int length) {
      if (spaceLeft() < length) {
        nextBuffer(length);
      }

      pos -= length;
      Java8Compatibility.position(buffer, pos + 1);
      buffer.put(value, offset, length);
    }

    @Override
    public void writeLazy(byte[] value, int offset, int length) {
      if (spaceLeft() < length) {
        // We consider the value to be immutable (likely the internals of a ByteString). Just
        // wrap it in a Netty buffer and add it to the output buffer.
        totalDoneBytes += length;
        buffers.addFirst(AllocatedBuffer.wrap(value, offset, length));

        // Advance the writer to the next buffer.
        // TODO(nathanmittler): Consider slicing if space available above some threshold.
        nextBuffer();
        return;
      }

      pos -= length;
      Java8Compatibility.position(buffer, pos + 1);
      buffer.put(value, offset, length);
    }

    @Override
    public void write(ByteBuffer value) {
      int length = value.remaining();
      if (spaceLeft() < length) {
        nextBuffer(length);
      }

      pos -= length;
      Java8Compatibility.position(buffer, pos + 1);
      buffer.put(value);
    }

    @Override
    public void writeLazy(ByteBuffer value) {
      int length = value.remaining();
      if (spaceLeft() < length) {
        // We consider the value to be immutable (likely the internals of a ByteString). Just
        // wrap it in a Netty buffer and add it to the output buffer.
        totalDoneBytes += length;
        buffers.addFirst(AllocatedBuffer.wrap(value));

        // Advance the writer to the next buffer.
        // TODO(nathanmittler): Consider slicing if space available above some threshold.
        nextBuffer();
        return;
      }

      pos -= length;
      Java8Compatibility.position(buffer, pos + 1);
      buffer.put(value);
    }

    @Override
    void requireSpace(int size) {
      if (spaceLeft() < size) {
        nextBuffer(size);
      }
    }
  }

  /** Writer that uses unsafe operations on a target {@link ByteBuffer}. */
  private static final class UnsafeDirectWriter extends BinaryWriter {
    private ByteBuffer buffer;
    private long bufferOffset;
    private long limitMinusOne;
    private long pos;

    UnsafeDirectWriter(BufferAllocator alloc, int chunkSize) {
      super(alloc, chunkSize);
      nextBuffer();
    }

    /** Indicates whether the required unsafe operations are supported on this platform. */
    private static boolean isSupported() {
      return UnsafeUtil.hasUnsafeByteBufferOperations();
    }

    private void nextBuffer() {
      nextBuffer(newDirectBuffer());
    }

    private void nextBuffer(int capacity) {
      nextBuffer(newDirectBuffer(capacity));
    }

    private void nextBuffer(AllocatedBuffer allocatedBuffer) {
      if (!allocatedBuffer.hasNioBuffer()) {
        throw new RuntimeException("Allocated buffer does not have NIO buffer");
      }
      ByteBuffer nioBuffer = allocatedBuffer.nioBuffer();
      if (!nioBuffer.isDirect()) {
        throw new RuntimeException("Allocator returned non-direct buffer");
      }

      finishCurrentBuffer();
      buffers.addFirst(allocatedBuffer);

      buffer = nioBuffer;
      Java8Compatibility.limit(buffer, buffer.capacity());
      Java8Compatibility.position(buffer, 0);

      bufferOffset = UnsafeUtil.addressOffset(buffer);
      limitMinusOne = bufferOffset + (buffer.limit() - 1);
      pos = limitMinusOne;
    }

    @Override
    public int getTotalBytesWritten() {
      return totalDoneBytes + bytesWrittenToCurrentBuffer();
    }

    private int bytesWrittenToCurrentBuffer() {
      return (int) (limitMinusOne - pos);
    }

    private int spaceLeft() {
      return bufferPos() + 1;
    }

    @Override
    void finishCurrentBuffer() {
      if (buffer != null) {
        totalDoneBytes += bytesWrittenToCurrentBuffer();
        // Update the indices on the netty buffer.
        Java8Compatibility.position(buffer, bufferPos() + 1);
        buffer = null;
        pos = 0;
        limitMinusOne = 0;
      }
    }

    private int bufferPos() {
      return (int) (pos - bufferOffset);
    }

    @Override
    public void writeUInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeInt32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeSInt32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeSInt32(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeFixed32(int fieldNumber, int value) {
      requireSpace(MAX_VARINT32_SIZE + FIXED32_SIZE);
      writeFixed32(value);
      writeTag(fieldNumber, WIRETYPE_FIXED32);
    }

    @Override
    public void writeUInt64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeVarint64(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeSInt64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + MAX_VARINT64_SIZE);
      writeSInt64(value);
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeFixed64(int fieldNumber, long value) {
      requireSpace(MAX_VARINT32_SIZE + FIXED64_SIZE);
      writeFixed64(value);
      writeTag(fieldNumber, WIRETYPE_FIXED64);
    }

    @Override
    public void writeBool(int fieldNumber, boolean value) {
      requireSpace(MAX_VARINT32_SIZE + 1);
      write((byte) (value ? 1 : 0));
      writeTag(fieldNumber, WIRETYPE_VARINT);
    }

    @Override
    public void writeString(int fieldNumber, String value) {
      int prevBytes = getTotalBytesWritten();
      writeString(value);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(2 * MAX_VARINT32_SIZE);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeBytes(int fieldNumber, ByteString value) {
      try {
        value.writeToReverse(this);
      } catch (IOException e) {
        // Should never happen since the writer does not throw.
        throw new RuntimeException(e);
      }

      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(value.size());
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeMessage(int fieldNumber, Object value) throws IOException {
      int prevBytes = getTotalBytesWritten();
      Protobuf.getInstance().writeTo(value, this);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeMessage(int fieldNumber, Object value, Schema schema) throws IOException {
      int prevBytes = getTotalBytesWritten();
      schema.writeTo(value, this);
      int length = getTotalBytesWritten() - prevBytes;
      requireSpace(MAX_VARINT32_SIZE * 2);
      writeVarint32(length);
      writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    }

    @Override
    public void writeGroup(int fieldNumber, Object value) throws IOException {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
      Protobuf.getInstance().writeTo(value, this);
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Override
    public void writeGroup(int fieldNumber, Object value, Schema schema) throws IOException {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
      schema.writeTo(value, this);
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Deprecated
    @Override
    public void writeStartGroup(int fieldNumber) {
      writeTag(fieldNumber, WIRETYPE_START_GROUP);
    }

    @Deprecated
    @Override
    public void writeEndGroup(int fieldNumber) {
      writeTag(fieldNumber, WIRETYPE_END_GROUP);
    }

    @Override
    void writeInt32(int value) {
      if (value >= 0) {
        writeVarint32(value);
      } else {
        writeVarint64(value);
      }
    }

    @Override
    void writeSInt32(int value) {
      writeVarint32(CodedOutputStream.encodeZigZag32(value));
    }

    @Override
    void writeSInt64(long value) {
      writeVarint64(CodedOutputStream.encodeZigZag64(value));
    }

    @Override
    void writeBool(boolean value) {
      write((byte) (value ? 1 : 0));
    }

    @Override
    void writeTag(int fieldNumber, int wireType) {
      writeVarint32(WireFormat.makeTag(fieldNumber, wireType));
    }

    @Override
    void writeVarint32(int value) {
      if ((value & (~0 << 7)) == 0) {
        writeVarint32OneByte(value);
      } else if ((value & (~0 << 14)) == 0) {
        writeVarint32TwoBytes(value);
      } else if ((value & (~0 << 21)) == 0) {
        writeVarint32ThreeBytes(value);
      } else if ((value & (~0 << 28)) == 0) {
        writeVarint32FourBytes(value);
      } else {
        writeVarint32FiveBytes(value);
      }
    }

    private void writeVarint32OneByte(int value) {
      UnsafeUtil.putByte(pos--, (byte) value);
    }

    private void writeVarint32TwoBytes(int value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 7));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint32ThreeBytes(int value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 14));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint32FourBytes(int value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 21));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint32FiveBytes(int value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 28));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    @Override
    void writeVarint64(long value) {
      switch (computeUInt64SizeNoTag(value)) {
        case 1:
          writeVarint64OneByte(value);
          break;
        case 2:
          writeVarint64TwoBytes(value);
          break;
        case 3:
          writeVarint64ThreeBytes(value);
          break;
        case 4:
          writeVarint64FourBytes(value);
          break;
        case 5:
          writeVarint64FiveBytes(value);
          break;
        case 6:
          writeVarint64SixBytes(value);
          break;
        case 7:
          writeVarint64SevenBytes(value);
          break;
        case 8:
          writeVarint64EightBytes(value);
          break;
        case 9:
          writeVarint64NineBytes(value);
          break;
        case 10:
          writeVarint64TenBytes(value);
          break;
      }
    }

    private void writeVarint64OneByte(long value) {
      UnsafeUtil.putByte(pos--, (byte) value);
    }

    private void writeVarint64TwoBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 7));
      UnsafeUtil.putByte(pos--, (byte) (((int) value & 0x7F) | 0x80));
    }

    private void writeVarint64ThreeBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (((int) value) >>> 14));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64FourBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 21));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64FiveBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 28));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64SixBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 35));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64SevenBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 42));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 35) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64EightBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 49));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 42) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 35) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64NineBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 56));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 49) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 42) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 35) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    private void writeVarint64TenBytes(long value) {
      UnsafeUtil.putByte(pos--, (byte) (value >>> 63));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 56) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 49) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 42) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 35) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 28) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 21) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 14) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) (((value >>> 7) & 0x7F) | 0x80));
      UnsafeUtil.putByte(pos--, (byte) ((value & 0x7F) | 0x80));
    }

    @Override
    void writeFixed32(int value) {
      UnsafeUtil.putByte(pos--, (byte) ((value >> 24) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((value >> 16) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((value >> 8) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) (value & 0xFF));
    }

    @Override
    void writeFixed64(long value) {
      UnsafeUtil.putByte(pos--, (byte) ((int) (value >> 56) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((int) (value >> 48) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((int) (value >> 40) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((int) (value >> 32) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((int) (value >> 24) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((int) (value >> 16) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((int) (value >> 8) & 0xFF));
      UnsafeUtil.putByte(pos--, (byte) ((int) (value) & 0xFF));
    }

    @Override
    void writeString(String in) {
      // Request enough space to write the ASCII string.
      requireSpace(in.length());

      // We know the buffer is big enough...
      int i = in.length() - 1;
      // Designed to take advantage of
      // https://wiki.openjdk.java.net/display/HotSpotInternals/RangeCheckElimination
      for (char c; i >= 0 && (c = in.charAt(i)) < 0x80; i--) {
        UnsafeUtil.putByte(pos--, (byte) c);
      }
      if (i == -1) {
        // ASCII.
        return;
      }
      for (char c; i >= 0; i--) {
        c = in.charAt(i);
        if (c < 0x80 && pos >= bufferOffset) {
          UnsafeUtil.putByte(pos--, (byte) c);
        } else if (c < 0x800 && pos > bufferOffset) { // 11 bits, two UTF-8 bytes
          UnsafeUtil.putByte(pos--, (byte) (0x80 | (0x3F & c)));
          UnsafeUtil.putByte(pos--, (byte) ((0xF << 6) | (c >>> 6)));
        } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c)
            && pos > bufferOffset + 1) {
          // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
          UnsafeUtil.putByte(pos--, (byte) (0x80 | (0x3F & c)));
          UnsafeUtil.putByte(pos--, (byte) (0x80 | (0x3F & (c >>> 6))));
          UnsafeUtil.putByte(pos--, (byte) ((0xF << 5) | (c >>> 12)));
        } else if (pos > bufferOffset + 2) {
          // Minimum code point represented by a surrogate pair is 0x10000, 17 bits,
          // four UTF-8 bytes
          final char high;
          if (i == 0 || !Character.isSurrogatePair(high = in.charAt(i - 1), c)) {
            throw new Utf8.UnpairedSurrogateException(i - 1, i);
          }
          i--;
          int codePoint = Character.toCodePoint(high, c);
          UnsafeUtil.putByte(pos--, (byte) (0x80 | (0x3F & codePoint)));
          UnsafeUtil.putByte(pos--, (byte) (0x80 | (0x3F & (codePoint >>> 6))));
          UnsafeUtil.putByte(pos--, (byte) (0x80 | (0x3F & (codePoint >>> 12))));
          UnsafeUtil.putByte(pos--, (byte) ((0xF << 4) | (codePoint >>> 18)));
        } else {
          // Buffer is full - allocate a new one and revisit the current character.
          requireSpace(i);
          i++;
        }
      }
    }

    @Override
    public void write(byte value) {
      UnsafeUtil.putByte(pos--, value);
    }

    @Override
    public void write(byte[] value, int offset, int length) {
      if (spaceLeft() < length) {
        nextBuffer(length);
      }

      pos -= length;
      Java8Compatibility.position(buffer, bufferPos() + 1);
      buffer.put(value, offset, length);
    }

    @Override
    public void writeLazy(byte[] value, int offset, int length) {
      if (spaceLeft() < length) {
        // We consider the value to be immutable (likely the internals of a ByteString). Just
        // wrap it in a Netty buffer and add it to the output buffer.
        totalDoneBytes += length;
        buffers.addFirst(AllocatedBuffer.wrap(value, offset, length));

        // Advance the writer to the next buffer.
        // TODO(nathanmittler): Consider slicing if space available above some threshold.
        nextBuffer();
        return;
      }

      pos -= length;
      Java8Compatibility.position(buffer, bufferPos() + 1);
      buffer.put(value, offset, length);
    }

    @Override
    public void write(ByteBuffer value) {
      int length = value.remaining();
      if (spaceLeft() < length) {
        nextBuffer(length);
      }

      pos -= length;
      Java8Compatibility.position(buffer, bufferPos() + 1);
      buffer.put(value);
    }

    @Override
    public void writeLazy(ByteBuffer value) {
      int length = value.remaining();
      if (spaceLeft() < length) {
        // We consider the value to be immutable (likely the internals of a ByteString). Just
        // wrap it in a Netty buffer and add it to the output buffer.
        totalDoneBytes += length;
        buffers.addFirst(AllocatedBuffer.wrap(value));

        // Advance the writer to the next buffer.
        // TODO(nathanmittler): Consider slicing if space available above some threshold.
        nextBuffer();
        return;
      }

      pos -= length;
      Java8Compatibility.position(buffer, bufferPos() + 1);
      buffer.put(value);
    }

    @Override
    void requireSpace(int size) {
      if (spaceLeft() < size) {
        nextBuffer(size);
      }
    }
  }
}
