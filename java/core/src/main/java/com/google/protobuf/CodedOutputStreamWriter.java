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
import static com.google.protobuf.WireFormat.WIRETYPE_LENGTH_DELIMITED;

import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

/** An adapter between the {@link Writer} interface and {@link CodedOutputStream}. */
@CheckReturnValue
@ExperimentalApi
final class CodedOutputStreamWriter implements Writer {
  private final CodedOutputStream output;

  public static CodedOutputStreamWriter forCodedOutput(CodedOutputStream output) {
    if (output.wrapper != null) {
      return output.wrapper;
    }
    return new CodedOutputStreamWriter(output);
  }

  private CodedOutputStreamWriter(CodedOutputStream output) {
    this.output = checkNotNull(output, "output");
    this.output.wrapper = this;
  }

  @Override
  public FieldOrder fieldOrder() {
    return FieldOrder.ASCENDING;
  }

  public int getTotalBytesWritten() {
    return output.getTotalBytesWritten();
  }

  @Override
  public void writeSFixed32(int fieldNumber, int value) throws IOException {
    output.writeSFixed32(fieldNumber, value);
  }

  @Override
  public void writeInt64(int fieldNumber, long value) throws IOException {
    output.writeInt64(fieldNumber, value);
  }

  @Override
  public void writeSFixed64(int fieldNumber, long value) throws IOException {
    output.writeSFixed64(fieldNumber, value);
  }

  @Override
  public void writeFloat(int fieldNumber, float value) throws IOException {
    output.writeFloat(fieldNumber, value);
  }

  @Override
  public void writeDouble(int fieldNumber, double value) throws IOException {
    output.writeDouble(fieldNumber, value);
  }

  @Override
  public void writeEnum(int fieldNumber, int value) throws IOException {
    output.writeEnum(fieldNumber, value);
  }

  @Override
  public void writeUInt64(int fieldNumber, long value) throws IOException {
    output.writeUInt64(fieldNumber, value);
  }

  @Override
  public void writeInt32(int fieldNumber, int value) throws IOException {
    output.writeInt32(fieldNumber, value);
  }

  @Override
  public void writeFixed64(int fieldNumber, long value) throws IOException {
    output.writeFixed64(fieldNumber, value);
  }

  @Override
  public void writeFixed32(int fieldNumber, int value) throws IOException {
    output.writeFixed32(fieldNumber, value);
  }

  @Override
  public void writeBool(int fieldNumber, boolean value) throws IOException {
    output.writeBool(fieldNumber, value);
  }

  @Override
  public void writeString(int fieldNumber, String value) throws IOException {
    output.writeString(fieldNumber, value);
  }

  @Override
  public void writeBytes(int fieldNumber, ByteString value) throws IOException {
    output.writeBytes(fieldNumber, value);
  }

  @Override
  public void writeUInt32(int fieldNumber, int value) throws IOException {
    output.writeUInt32(fieldNumber, value);
  }

  @Override
  public void writeSInt32(int fieldNumber, int value) throws IOException {
    output.writeSInt32(fieldNumber, value);
  }

  @Override
  public void writeSInt64(int fieldNumber, long value) throws IOException {
    output.writeSInt64(fieldNumber, value);
  }

  @Override
  public void writeMessage(int fieldNumber, Object value) throws IOException {
    output.writeMessage(fieldNumber, (MessageLite) value);
  }

  @Override
  public void writeMessage(int fieldNumber, Object value, Schema schema) throws IOException {
    output.writeMessage(fieldNumber, (MessageLite) value, schema);
  }

  @Deprecated
  @Override
  public void writeGroup(int fieldNumber, Object value) throws IOException {
    output.writeGroup(fieldNumber, (MessageLite) value);
  }

  @Override
  public void writeGroup(int fieldNumber, Object value, Schema schema) throws IOException {
    output.writeGroup(fieldNumber, (MessageLite) value, schema);
  }

  @Deprecated
  @Override
  public void writeStartGroup(int fieldNumber) throws IOException {
    output.writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
  }

  @Deprecated
  @Override
  public void writeEndGroup(int fieldNumber) throws IOException {
    output.writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
  }

  @Override
  public final void writeMessageSetItem(int fieldNumber, Object value) throws IOException {
    if (value instanceof ByteString) {
      output.writeRawMessageSetExtension(fieldNumber, (ByteString) value);
    } else {
      output.writeMessageSetExtension(fieldNumber, (MessageLite) value);
    }
  }

  @Override
  public void writeInt32List(int fieldNumber, List<Integer> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeInt32SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeInt32NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeInt32(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeFixed32List(int fieldNumber, List<Integer> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeFixed32SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeFixed32NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeFixed32(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeInt64List(int fieldNumber, List<Long> value, boolean packed) throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeInt64SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeInt64NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeInt64(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeUInt64List(int fieldNumber, List<Long> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeUInt64SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeUInt64NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeUInt64(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeFixed64List(int fieldNumber, List<Long> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeFixed64SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeFixed64NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeFixed64(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeFloatList(int fieldNumber, List<Float> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeFloatSizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeFloatNoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeFloat(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeDoubleList(int fieldNumber, List<Double> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeDoubleSizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeDoubleNoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeDouble(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeEnumList(int fieldNumber, List<Integer> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeEnumSizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeEnumNoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeEnum(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeBoolList(int fieldNumber, List<Boolean> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeBoolSizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeBoolNoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeBool(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeStringList(int fieldNumber, List<String> value) throws IOException {
    if (value instanceof LazyStringList) {
      final LazyStringList lazyList = (LazyStringList) value;
      for (int i = 0; i < value.size(); ++i) {
        writeLazyString(fieldNumber, lazyList.getRaw(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeString(fieldNumber, value.get(i));
      }
    }
  }

  private void writeLazyString(int fieldNumber, Object value) throws IOException {
    if (value instanceof String) {
      output.writeString(fieldNumber, (String) value);
    } else {
      output.writeBytes(fieldNumber, (ByteString) value);
    }
  }

  @Override
  public void writeBytesList(int fieldNumber, List<ByteString> value) throws IOException {
    for (int i = 0; i < value.size(); ++i) {
      output.writeBytes(fieldNumber, value.get(i));
    }
  }

  @Override
  public void writeUInt32List(int fieldNumber, List<Integer> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeUInt32SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeUInt32NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeUInt32(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeSFixed32List(int fieldNumber, List<Integer> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeSFixed32SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeSFixed32NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeSFixed32(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeSFixed64List(int fieldNumber, List<Long> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeSFixed64SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeSFixed64NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeSFixed64(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeSInt32List(int fieldNumber, List<Integer> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeSInt32SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeSInt32NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeSInt32(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeSInt64List(int fieldNumber, List<Long> value, boolean packed)
      throws IOException {
    if (packed) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);

      // Compute and write the length of the data.
      int dataSize = 0;
      for (int i = 0; i < value.size(); ++i) {
        dataSize += CodedOutputStream.computeSInt64SizeNoTag(value.get(i));
      }
      output.writeUInt32NoTag(dataSize);

      // Write the data itself, without any tags.
      for (int i = 0; i < value.size(); ++i) {
        output.writeSInt64NoTag(value.get(i));
      }
    } else {
      for (int i = 0; i < value.size(); ++i) {
        output.writeSInt64(fieldNumber, value.get(i));
      }
    }
  }

  @Override
  public void writeMessageList(int fieldNumber, List<?> value) throws IOException {
    for (int i = 0; i < value.size(); ++i) {
      writeMessage(fieldNumber, value.get(i));
    }
  }

  @Override
  public void writeMessageList(int fieldNumber, List<?> value, Schema schema) throws IOException {
    for (int i = 0; i < value.size(); ++i) {
      writeMessage(fieldNumber, value.get(i), schema);
    }
  }

  @Deprecated
  @Override
  public void writeGroupList(int fieldNumber, List<?> value) throws IOException {
    for (int i = 0; i < value.size(); ++i) {
      writeGroup(fieldNumber, value.get(i));
    }
  }

  @Override
  public void writeGroupList(int fieldNumber, List<?> value, Schema schema) throws IOException {
    for (int i = 0; i < value.size(); ++i) {
      writeGroup(fieldNumber, value.get(i), schema);
    }
  }

  @Override
  public <K, V> void writeMap(int fieldNumber, MapEntryLite.Metadata<K, V> metadata, Map<K, V> map)
      throws IOException {
    if (output.isSerializationDeterministic()) {
      writeDeterministicMap(fieldNumber, metadata, map);
      return;
    }
    for (Map.Entry<K, V> entry : map.entrySet()) {
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
      output.writeUInt32NoTag(
          MapEntryLite.computeSerializedSize(metadata, entry.getKey(), entry.getValue()));
      MapEntryLite.writeTo(output, metadata, entry.getKey(), entry.getValue());
    }
  }

  @SuppressWarnings("unchecked")
  private <K, V> void writeDeterministicMap(
      int fieldNumber, MapEntryLite.Metadata<K, V> metadata, Map<K, V> map) throws IOException {
    switch (metadata.keyType) {
      case BOOL:
        V value;
        if ((value = map.get(Boolean.FALSE)) != null) {
          writeDeterministicBooleanMapEntry(
              fieldNumber, /* key= */ false, value, (MapEntryLite.Metadata<Boolean, V>) metadata);
        }
        if ((value = map.get(Boolean.TRUE)) != null) {
          writeDeterministicBooleanMapEntry(
              fieldNumber, /* key= */ true, value, (MapEntryLite.Metadata<Boolean, V>) metadata);
        }
        break;
      case FIXED32:
      case INT32:
      case SFIXED32:
      case SINT32:
      case UINT32:
        writeDeterministicIntegerMap(
            fieldNumber, (MapEntryLite.Metadata<Integer, V>) metadata, (Map<Integer, V>) map);
        break;
      case FIXED64:
      case INT64:
      case SFIXED64:
      case SINT64:
      case UINT64:
        writeDeterministicLongMap(
            fieldNumber, (MapEntryLite.Metadata<Long, V>) metadata, (Map<Long, V>) map);
        break;
      case STRING:
        writeDeterministicStringMap(
            fieldNumber, (MapEntryLite.Metadata<String, V>) metadata, (Map<String, V>) map);
        break;
      default:
        throw new IllegalArgumentException("does not support key type: " + metadata.keyType);
    }
  }

  private <V> void writeDeterministicBooleanMapEntry(
      int fieldNumber, boolean key, V value, MapEntryLite.Metadata<Boolean, V> metadata)
      throws IOException {
    output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
    output.writeUInt32NoTag(MapEntryLite.computeSerializedSize(metadata, key, value));
    MapEntryLite.writeTo(output, metadata, key, value);
  }

  private <V> void writeDeterministicIntegerMap(
      int fieldNumber, MapEntryLite.Metadata<Integer, V> metadata, Map<Integer, V> map)
      throws IOException {
    int[] keys = new int[map.size()];
    int index = 0;
    for (int k : map.keySet()) {
      keys[index++] = k;
    }
    Arrays.sort(keys);
    for (int key : keys) {
      V value = map.get(key);
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
      output.writeUInt32NoTag(MapEntryLite.computeSerializedSize(metadata, key, value));
      MapEntryLite.writeTo(output, metadata, key, value);
    }
  }

  private <V> void writeDeterministicLongMap(
      int fieldNumber, MapEntryLite.Metadata<Long, V> metadata, Map<Long, V> map)
      throws IOException {
    long[] keys = new long[map.size()];
    int index = 0;
    for (long k : map.keySet()) {
      keys[index++] = k;
    }
    Arrays.sort(keys);
    for (long key : keys) {
      V value = map.get(key);
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
      output.writeUInt32NoTag(MapEntryLite.computeSerializedSize(metadata, key, value));
      MapEntryLite.writeTo(output, metadata, key, value);
    }
  }

  private <V> void writeDeterministicStringMap(
      int fieldNumber, MapEntryLite.Metadata<String, V> metadata, Map<String, V> map)
      throws IOException {
    String[] keys = new String[map.size()];
    int index = 0;
    for (String k : map.keySet()) {
      keys[index++] = k;
    }
    Arrays.sort(keys);
    for (String key : keys) {
      V value = map.get(key);
      output.writeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
      output.writeUInt32NoTag(MapEntryLite.computeSerializedSize(metadata, key, value));
      MapEntryLite.writeTo(output, metadata, key, value);
    }
  }
}
