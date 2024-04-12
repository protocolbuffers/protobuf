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

import static com.google.protobuf.WireFormat.FIXED32_SIZE;
import static com.google.protobuf.WireFormat.FIXED64_SIZE;
import static com.google.protobuf.WireFormat.WIRETYPE_END_GROUP;
import static com.google.protobuf.WireFormat.WIRETYPE_FIXED32;
import static com.google.protobuf.WireFormat.WIRETYPE_FIXED64;
import static com.google.protobuf.WireFormat.WIRETYPE_LENGTH_DELIMITED;
import static com.google.protobuf.WireFormat.WIRETYPE_START_GROUP;
import static com.google.protobuf.WireFormat.WIRETYPE_VARINT;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;

/**
 * A {@link Reader} that reads from a buffer containing a message serialized with the binary
 * protocol.
 */
@CheckReturnValue
@ExperimentalApi
abstract class BinaryReader implements Reader {
  private static final int FIXED32_MULTIPLE_MASK = FIXED32_SIZE - 1;
  private static final int FIXED64_MULTIPLE_MASK = FIXED64_SIZE - 1;

  /**
   * Creates a new reader using the given {@code buffer} as input.
   *
   * @param buffer the input buffer. The buffer (including position, limit, etc.) will not be
   *     modified. To increment the buffer position after the read completes, use the value returned
   *     by {@link #getTotalBytesRead()}.
   * @param bufferIsImmutable if {@code true} the reader assumes that the content of {@code buffer}
   *     will never change and any allocated {@link ByteString} instances will by directly wrap
   *     slices of {@code buffer}.
   * @return the reader
   */
  public static BinaryReader newInstance(ByteBuffer buffer, boolean bufferIsImmutable) {
    if (buffer.hasArray()) {
      // TODO(nathanmittler): Add support for unsafe operations.
      return new SafeHeapReader(buffer, bufferIsImmutable);
    }
    // TODO(nathanmittler): Add support for direct buffers
    throw new IllegalArgumentException("Direct buffers not yet supported");
  }

  /** Only allow subclassing for inner classes. */
  private BinaryReader() {}

  /** Returns the total number of bytes read so far from the input buffer. */
  public abstract int getTotalBytesRead();

  @Override
  public boolean shouldDiscardUnknownFields() {
    return false;
  }

  /**
   * A {@link BinaryReader} implementation that operates on a heap {@link ByteBuffer}. Uses only
   * safe operations on the underlying array.
   */
  private static final class SafeHeapReader extends BinaryReader {
    private final boolean bufferIsImmutable;
    private final byte[] buffer;
    private int pos;
    private final int initialPos;
    private int limit;
    private int tag;
    private int endGroupTag;

    public SafeHeapReader(ByteBuffer bytebuf, boolean bufferIsImmutable) {
      this.bufferIsImmutable = bufferIsImmutable;
      buffer = bytebuf.array();
      initialPos = pos = bytebuf.arrayOffset() + bytebuf.position();
      limit = bytebuf.arrayOffset() + bytebuf.limit();
    }

    private boolean isAtEnd() {
      return pos == limit;
    }

    @Override
    public int getTotalBytesRead() {
      return pos - initialPos;
    }

    @Override
    public int getFieldNumber() throws IOException {
      if (isAtEnd()) {
        return Reader.READ_DONE;
      }
      tag = readVarint32();
      if (tag == endGroupTag) {
        return Reader.READ_DONE;
      }
      return WireFormat.getTagFieldNumber(tag);
    }

    @Override
    public int getTag() {
      return tag;
    }

    @Override
    public boolean skipField() throws IOException {
      if (isAtEnd() || tag == endGroupTag) {
        return false;
      }

      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_VARINT:
          skipVarint();
          return true;
        case WIRETYPE_FIXED64:
          skipBytes(FIXED64_SIZE);
          return true;
        case WIRETYPE_LENGTH_DELIMITED:
          skipBytes(readVarint32());
          return true;
        case WIRETYPE_FIXED32:
          skipBytes(FIXED32_SIZE);
          return true;
        case WIRETYPE_START_GROUP:
          skipGroup();
          return true;
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public double readDouble() throws IOException {
      requireWireType(WIRETYPE_FIXED64);
      return Double.longBitsToDouble(readLittleEndian64());
    }

    @Override
    public float readFloat() throws IOException {
      requireWireType(WIRETYPE_FIXED32);
      return Float.intBitsToFloat(readLittleEndian32());
    }

    @Override
    public long readUInt64() throws IOException {
      requireWireType(WIRETYPE_VARINT);
      return readVarint64();
    }

    @Override
    public long readInt64() throws IOException {
      requireWireType(WIRETYPE_VARINT);
      return readVarint64();
    }

    @Override
    public int readInt32() throws IOException {
      requireWireType(WIRETYPE_VARINT);
      return readVarint32();
    }

    @Override
    public long readFixed64() throws IOException {
      requireWireType(WIRETYPE_FIXED64);
      return readLittleEndian64();
    }

    @Override
    public int readFixed32() throws IOException {
      requireWireType(WIRETYPE_FIXED32);
      return readLittleEndian32();
    }

    @Override
    public boolean readBool() throws IOException {
      requireWireType(WIRETYPE_VARINT);
      return readVarint32() != 0;
    }

    @Override
    public String readString() throws IOException {
      return readStringInternal(false);
    }

    @Override
    public String readStringRequireUtf8() throws IOException {
      return readStringInternal(true);
    }

    public String readStringInternal(boolean requireUtf8) throws IOException {
      requireWireType(WIRETYPE_LENGTH_DELIMITED);
      final int size = readVarint32();
      if (size == 0) {
        return "";
      }

      requireBytes(size);
      if (requireUtf8 && !Utf8.isValidUtf8(buffer, pos, pos + size)) {
        throw InvalidProtocolBufferException.invalidUtf8();
      }
      String result = new String(buffer, pos, size, Internal.UTF_8);
      pos += size;
      return result;
    }

    @Override
    public <T> T readMessage(Class<T> clazz, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      requireWireType(WIRETYPE_LENGTH_DELIMITED);
      return readMessage(Protobuf.getInstance().schemaFor(clazz), extensionRegistry);
    }

    @Override
    public <T> T readMessageBySchemaWithCheck(
        Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException {
      requireWireType(WIRETYPE_LENGTH_DELIMITED);
      return readMessage(schema, extensionRegistry);
    }

    private <T> T readMessage(Schema<T> schema, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      T newInstance = schema.newInstance();
      mergeMessageField(newInstance, schema, extensionRegistry);
      schema.makeImmutable(newInstance);
      return newInstance;
    }

    @Override
    public <T> void mergeMessageField(
        T target, Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException {
      int size = readVarint32();
      requireBytes(size);

      // Update the limit.
      int prevLimit = limit;
      int newLimit = pos + size;
      limit = newLimit;

      try {
        schema.mergeFrom(target, this, extensionRegistry);
        if (pos != newLimit) {
          throw InvalidProtocolBufferException.parseFailure();
        }
      } finally {
        // Restore the limit.
        limit = prevLimit;
      }
    }

    @Deprecated
    @Override
    public <T> T readGroup(Class<T> clazz, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      requireWireType(WIRETYPE_START_GROUP);
      return readGroup(Protobuf.getInstance().schemaFor(clazz), extensionRegistry);
    }

    @Deprecated
    @Override
    public <T> T readGroupBySchemaWithCheck(
        Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException {
      requireWireType(WIRETYPE_START_GROUP);
      return readGroup(schema, extensionRegistry);
    }

    private <T> T readGroup(Schema<T> schema, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      T newInstance = schema.newInstance();
      mergeGroupField(newInstance, schema, extensionRegistry);
      schema.makeImmutable(newInstance);
      return newInstance;
    }

    @Override
    public <T> void mergeGroupField(
        T target, Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException {
      int prevEndGroupTag = endGroupTag;
      endGroupTag = WireFormat.makeTag(WireFormat.getTagFieldNumber(tag), WIRETYPE_END_GROUP);

      try {
        schema.mergeFrom(target, this, extensionRegistry);
        if (tag != endGroupTag) {
          throw InvalidProtocolBufferException.parseFailure();
        }
      } finally {
        // Restore the old end group tag.
        endGroupTag = prevEndGroupTag;
      }
    }

    @Override
    public ByteString readBytes() throws IOException {
      requireWireType(WIRETYPE_LENGTH_DELIMITED);
      int size = readVarint32();
      if (size == 0) {
        return ByteString.EMPTY;
      }

      requireBytes(size);
      ByteString bytes =
          bufferIsImmutable
              ? ByteString.wrap(buffer, pos, size)
              : ByteString.copyFrom(buffer, pos, size);
      pos += size;
      return bytes;
    }

    @Override
    public int readUInt32() throws IOException {
      requireWireType(WIRETYPE_VARINT);
      return readVarint32();
    }

    @Override
    public int readEnum() throws IOException {
      requireWireType(WIRETYPE_VARINT);
      return readVarint32();
    }

    @Override
    public int readSFixed32() throws IOException {
      requireWireType(WIRETYPE_FIXED32);
      return readLittleEndian32();
    }

    @Override
    public long readSFixed64() throws IOException {
      requireWireType(WIRETYPE_FIXED64);
      return readLittleEndian64();
    }

    @Override
    public int readSInt32() throws IOException {
      requireWireType(WIRETYPE_VARINT);
      return CodedInputStream.decodeZigZag32(readVarint32());
    }

    @Override
    public long readSInt64() throws IOException {
      requireWireType(WIRETYPE_VARINT);
      return CodedInputStream.decodeZigZag64(readVarint64());
    }

    @Override
    public void readDoubleList(List<Double> target) throws IOException {
      if (target instanceof DoubleArrayList) {
        DoubleArrayList plist = (DoubleArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed64Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addDouble(Double.longBitsToDouble(readLittleEndian64_NoCheck()));
            }
            break;
          case WIRETYPE_FIXED64:
            while (true) {
              plist.addDouble(readDouble());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed64Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(Double.longBitsToDouble(readLittleEndian64_NoCheck()));
            }
            break;
          case WIRETYPE_FIXED64:
            while (true) {
              target.add(readDouble());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readFloatList(List<Float> target) throws IOException {
      if (target instanceof FloatArrayList) {
        FloatArrayList plist = (FloatArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed32Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addFloat(Float.intBitsToFloat(readLittleEndian32_NoCheck()));
            }
            break;
          case WIRETYPE_FIXED32:
            while (true) {
              plist.addFloat(readFloat());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed32Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(Float.intBitsToFloat(readLittleEndian32_NoCheck()));
            }
            break;
          case WIRETYPE_FIXED32:
            while (true) {
              target.add(readFloat());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readUInt64List(List<Long> target) throws IOException {
      if (target instanceof LongArrayList) {
        LongArrayList plist = (LongArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addLong(readVarint64());
            }
            requirePosition(fieldEndPos);
            break;
          case WIRETYPE_VARINT:
            while (true) {
              plist.addLong(readUInt64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readVarint64());
            }
            requirePosition(fieldEndPos);
            break;
          case WIRETYPE_VARINT:
            while (true) {
              target.add(readUInt64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readInt64List(List<Long> target) throws IOException {
      if (target instanceof LongArrayList) {
        LongArrayList plist = (LongArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addLong(readVarint64());
            }
            requirePosition(fieldEndPos);
            break;
          case WIRETYPE_VARINT:
            while (true) {
              plist.addLong(readInt64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readVarint64());
            }
            requirePosition(fieldEndPos);
            break;
          case WIRETYPE_VARINT:
            while (true) {
              target.add(readInt64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readInt32List(List<Integer> target) throws IOException {
      if (target instanceof IntArrayList) {
        IntArrayList plist = (IntArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addInt(readVarint32());
            }
            requirePosition(fieldEndPos);
            break;
          case WIRETYPE_VARINT:
            while (true) {
              plist.addInt(readInt32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readVarint32());
            }
            requirePosition(fieldEndPos);
            break;
          case WIRETYPE_VARINT:
            while (true) {
              target.add(readInt32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readFixed64List(List<Long> target) throws IOException {
      if (target instanceof LongArrayList) {
        LongArrayList plist = (LongArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed64Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addLong(readLittleEndian64_NoCheck());
            }
            break;
          case WIRETYPE_FIXED64:
            while (true) {
              plist.addLong(readFixed64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed64Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readLittleEndian64_NoCheck());
            }
            break;
          case WIRETYPE_FIXED64:
            while (true) {
              target.add(readFixed64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readFixed32List(List<Integer> target) throws IOException {
      if (target instanceof IntArrayList) {
        IntArrayList plist = (IntArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed32Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addInt(readLittleEndian32_NoCheck());
            }
            break;
          case WIRETYPE_FIXED32:
            while (true) {
              plist.addInt(readFixed32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed32Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readLittleEndian32_NoCheck());
            }
            break;
          case WIRETYPE_FIXED32:
            while (true) {
              target.add(readFixed32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readBoolList(List<Boolean> target) throws IOException {
      if (target instanceof BooleanArrayList) {
        BooleanArrayList plist = (BooleanArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addBoolean(readVarint32() != 0);
            }
            requirePosition(fieldEndPos);
            break;
          case WIRETYPE_VARINT:
            while (true) {
              plist.addBoolean(readBool());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readVarint32() != 0);
            }
            requirePosition(fieldEndPos);
            break;
          case WIRETYPE_VARINT:
            while (true) {
              target.add(readBool());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readStringList(List<String> target) throws IOException {
      readStringListInternal(target, false);
    }

    @Override
    public void readStringListRequireUtf8(List<String> target) throws IOException {
      readStringListInternal(target, true);
    }

    public void readStringListInternal(List<String> target, boolean requireUtf8)
        throws IOException {
      if (WireFormat.getTagWireType(tag) != WIRETYPE_LENGTH_DELIMITED) {
        throw InvalidProtocolBufferException.invalidWireType();
      }

      if (target instanceof LazyStringList && !requireUtf8) {
        LazyStringList lazyList = (LazyStringList) target;
        while (true) {
          lazyList.add(readBytes());

          if (isAtEnd()) {
            return;
          }
          int prevPos = pos;
          int nextTag = readVarint32();
          if (nextTag != tag) {
            // We've reached the end of the repeated field. Rewind the buffer position to before
            // the new tag.
            pos = prevPos;
            return;
          }
        }
      } else {
        while (true) {
          target.add(readStringInternal(requireUtf8));

          if (isAtEnd()) {
            return;
          }
          int prevPos = pos;
          int nextTag = readVarint32();
          if (nextTag != tag) {
            // We've reached the end of the repeated field. Rewind the buffer position to before
            // the new tag.
            pos = prevPos;
            return;
          }
        }
      }
    }

    @Override
    public <T> void readMessageList(
        List<T> target, Class<T> targetType, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      final Schema<T> schema = Protobuf.getInstance().schemaFor(targetType);
      readMessageList(target, schema, extensionRegistry);
    }

    @Override
    public <T> void readMessageList(
        List<T> target, Schema<T> schema, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      if (WireFormat.getTagWireType(tag) != WIRETYPE_LENGTH_DELIMITED) {
        throw InvalidProtocolBufferException.invalidWireType();
      }
      final int listTag = tag;
      while (true) {
        target.add(readMessage(schema, extensionRegistry));

        if (isAtEnd()) {
          return;
        }
        int prevPos = pos;
        int nextTag = readVarint32();
        if (nextTag != listTag) {
          // We've reached the end of the repeated field. Rewind the buffer position to before
          // the new tag.
          pos = prevPos;
          return;
        }
      }
    }

    @Deprecated
    @Override
    public <T> void readGroupList(
        List<T> target, Class<T> targetType, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      final Schema<T> schema = Protobuf.getInstance().schemaFor(targetType);
      readGroupList(target, schema, extensionRegistry);
    }

    @Deprecated
    @Override
    public <T> void readGroupList(
        List<T> target, Schema<T> schema, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      if (WireFormat.getTagWireType(tag) != WIRETYPE_START_GROUP) {
        throw InvalidProtocolBufferException.invalidWireType();
      }
      final int listTag = tag;
      while (true) {
        target.add(readGroup(schema, extensionRegistry));

        if (isAtEnd()) {
          return;
        }
        int prevPos = pos;
        int nextTag = readVarint32();
        if (nextTag != listTag) {
          // We've reached the end of the repeated field. Rewind the buffer position to before
          // the new tag.
          pos = prevPos;
          return;
        }
      }
    }

    @Override
    public void readBytesList(List<ByteString> target) throws IOException {
      if (WireFormat.getTagWireType(tag) != WIRETYPE_LENGTH_DELIMITED) {
        throw InvalidProtocolBufferException.invalidWireType();
      }

      while (true) {
        target.add(readBytes());

        if (isAtEnd()) {
          return;
        }
        int prevPos = pos;
        int nextTag = readVarint32();
        if (nextTag != tag) {
          // We've reached the end of the repeated field. Rewind the buffer position to before
          // the new tag.
          pos = prevPos;
          return;
        }
      }
    }

    @Override
    public void readUInt32List(List<Integer> target) throws IOException {
      if (target instanceof IntArrayList) {
        IntArrayList plist = (IntArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addInt(readVarint32());
            }
            break;
          case WIRETYPE_VARINT:
            while (true) {
              plist.addInt(readUInt32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readVarint32());
            }
            break;
          case WIRETYPE_VARINT:
            while (true) {
              target.add(readUInt32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readEnumList(List<Integer> target) throws IOException {
      if (target instanceof IntArrayList) {
        IntArrayList plist = (IntArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addInt(readVarint32());
            }
            break;
          case WIRETYPE_VARINT:
            while (true) {
              plist.addInt(readEnum());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readVarint32());
            }
            break;
          case WIRETYPE_VARINT:
            while (true) {
              target.add(readEnum());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readSFixed32List(List<Integer> target) throws IOException {
      if (target instanceof IntArrayList) {
        IntArrayList plist = (IntArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed32Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addInt(readLittleEndian32_NoCheck());
            }
            break;
          case WIRETYPE_FIXED32:
            while (true) {
              plist.addInt(readSFixed32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed32Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readLittleEndian32_NoCheck());
            }
            break;
          case WIRETYPE_FIXED32:
            while (true) {
              target.add(readSFixed32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readSFixed64List(List<Long> target) throws IOException {
      if (target instanceof LongArrayList) {
        LongArrayList plist = (LongArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed64Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addLong(readLittleEndian64_NoCheck());
            }
            break;
          case WIRETYPE_FIXED64:
            while (true) {
              plist.addLong(readSFixed64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            verifyPackedFixed64Length(bytes);
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(readLittleEndian64_NoCheck());
            }
            break;
          case WIRETYPE_FIXED64:
            while (true) {
              target.add(readSFixed64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readSInt32List(List<Integer> target) throws IOException {
      if (target instanceof IntArrayList) {
        IntArrayList plist = (IntArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addInt(CodedInputStream.decodeZigZag32(readVarint32()));
            }
            break;
          case WIRETYPE_VARINT:
            while (true) {
              plist.addInt(readSInt32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(CodedInputStream.decodeZigZag32(readVarint32()));
            }
            break;
          case WIRETYPE_VARINT:
            while (true) {
              target.add(readSInt32());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @Override
    public void readSInt64List(List<Long> target) throws IOException {
      if (target instanceof LongArrayList) {
        LongArrayList plist = (LongArrayList) target;
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              plist.addLong(CodedInputStream.decodeZigZag64(readVarint64()));
            }
            break;
          case WIRETYPE_VARINT:
            while (true) {
              plist.addLong(readSInt64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      } else {
        switch (WireFormat.getTagWireType(tag)) {
          case WIRETYPE_LENGTH_DELIMITED:
            final int bytes = readVarint32();
            final int fieldEndPos = pos + bytes;
            while (pos < fieldEndPos) {
              target.add(CodedInputStream.decodeZigZag64(readVarint64()));
            }
            break;
          case WIRETYPE_VARINT:
            while (true) {
              target.add(readSInt64());

              if (isAtEnd()) {
                return;
              }
              int prevPos = pos;
              int nextTag = readVarint32();
              if (nextTag != tag) {
                // We've reached the end of the repeated field. Rewind the buffer position to before
                // the new tag.
                pos = prevPos;
                return;
              }
            }
          default:
            throw InvalidProtocolBufferException.invalidWireType();
        }
      }
    }

    @SuppressWarnings("unchecked")
    @Override
    public <K, V> void readMap(
        Map<K, V> target,
        MapEntryLite.Metadata<K, V> metadata,
        ExtensionRegistryLite extensionRegistry)
        throws IOException {
      requireWireType(WIRETYPE_LENGTH_DELIMITED);
      int size = readVarint32();
      requireBytes(size);

      // Update the limit.
      int prevLimit = limit;
      int newLimit = pos + size;
      limit = newLimit;

      try {
        K key = metadata.defaultKey;
        V value = metadata.defaultValue;
        while (true) {
          int number = getFieldNumber();
          if (number == READ_DONE) {
            break;
          }
          try {
            switch (number) {
              case 1:
                key = (K) readField(metadata.keyType, null, null);
                break;
              case 2:
                value =
                    (V)
                        readField(
                            metadata.valueType,
                            metadata.defaultValue.getClass(),
                            extensionRegistry);
                break;
              default:
                if (!skipField()) {
                  throw new InvalidProtocolBufferException("Unable to parse map entry.");
                }
                break;
            }
          } catch (InvalidProtocolBufferException.InvalidWireTypeException ignore) {
            // the type doesn't match, skip the field.
            if (!skipField()) {
              throw new InvalidProtocolBufferException("Unable to parse map entry.");
            }
          }
        }
        target.put(key, value);
      } finally {
        // Restore the limit.
        limit = prevLimit;
      }
    }

    private Object readField(
        WireFormat.FieldType fieldType,
        Class<?> messageType,
        ExtensionRegistryLite extensionRegistry)
        throws IOException {
      switch (fieldType) {
        case BOOL:
          return readBool();
        case BYTES:
          return readBytes();
        case DOUBLE:
          return readDouble();
        case ENUM:
          return readEnum();
        case FIXED32:
          return readFixed32();
        case FIXED64:
          return readFixed64();
        case FLOAT:
          return readFloat();
        case INT32:
          return readInt32();
        case INT64:
          return readInt64();
        case MESSAGE:
          return readMessage(messageType, extensionRegistry);
        case SFIXED32:
          return readSFixed32();
        case SFIXED64:
          return readSFixed64();
        case SINT32:
          return readSInt32();
        case SINT64:
          return readSInt64();
        case STRING:
          return readStringRequireUtf8();
        case UINT32:
          return readUInt32();
        case UINT64:
          return readUInt64();
        default:
          throw new RuntimeException("unsupported field type.");
      }
    }

    /** Read a raw Varint from the stream. If larger than 32 bits, discard the upper bits. */
    private int readVarint32() throws IOException {
      // See implementation notes for readRawVarint64
      int i = pos;

      if (limit == pos) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }

      int x;
      if ((x = buffer[i++]) >= 0) {
        pos = i;
        return x;
      } else if (limit - i < 9) {
        return (int) readVarint64SlowPath();
      } else if ((x ^= (buffer[i++] << 7)) < 0) {
        x ^= (~0 << 7);
      } else if ((x ^= (buffer[i++] << 14)) >= 0) {
        x ^= (~0 << 7) ^ (~0 << 14);
      } else if ((x ^= (buffer[i++] << 21)) < 0) {
        x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21);
      } else {
        int y = buffer[i++];
        x ^= y << 28;
        x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21) ^ (~0 << 28);
        if (y < 0
            && buffer[i++] < 0
            && buffer[i++] < 0
            && buffer[i++] < 0
            && buffer[i++] < 0
            && buffer[i++] < 0) {
          throw InvalidProtocolBufferException.malformedVarint();
        }
      }
      pos = i;
      return x;
    }

    public long readVarint64() throws IOException {
      // Implementation notes:
      //
      // Optimized for one-byte values, expected to be common.
      // The particular code below was selected from various candidates
      // empirically, by winning VarintBenchmark.
      //
      // Sign extension of (signed) Java bytes is usually a nuisance, but
      // we exploit it here to more easily obtain the sign of bytes read.
      // Instead of cleaning up the sign extension bits by masking eagerly,
      // we delay until we find the final (positive) byte, when we clear all
      // accumulated bits with one xor.  We depend on javac to constant fold.
      int i = pos;

      if (limit == i) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }

      final byte[] buffer = this.buffer;
      long x;
      int y;
      if ((y = buffer[i++]) >= 0) {
        pos = i;
        return y;
      } else if (limit - i < 9) {
        return readVarint64SlowPath();
      } else if ((y ^= (buffer[i++] << 7)) < 0) {
        x = y ^ (~0 << 7);
      } else if ((y ^= (buffer[i++] << 14)) >= 0) {
        x = y ^ ((~0 << 7) ^ (~0 << 14));
      } else if ((y ^= (buffer[i++] << 21)) < 0) {
        x = y ^ ((~0 << 7) ^ (~0 << 14) ^ (~0 << 21));
      } else if ((x = y ^ ((long) buffer[i++] << 28)) >= 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28);
      } else if ((x ^= ((long) buffer[i++] << 35)) < 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35);
      } else if ((x ^= ((long) buffer[i++] << 42)) >= 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35) ^ (~0L << 42);
      } else if ((x ^= ((long) buffer[i++] << 49)) < 0L) {
        x ^=
            (~0L << 7)
                ^ (~0L << 14)
                ^ (~0L << 21)
                ^ (~0L << 28)
                ^ (~0L << 35)
                ^ (~0L << 42)
                ^ (~0L << 49);
      } else {
        x ^= ((long) buffer[i++] << 56);
        x ^=
            (~0L << 7)
                ^ (~0L << 14)
                ^ (~0L << 21)
                ^ (~0L << 28)
                ^ (~0L << 35)
                ^ (~0L << 42)
                ^ (~0L << 49)
                ^ (~0L << 56);
        if (x < 0L) {
          if (buffer[i++] < 0L) {
            throw InvalidProtocolBufferException.malformedVarint();
          }
        }
      }
      pos = i;
      return x;
    }

    private long readVarint64SlowPath() throws IOException {
      long result = 0;
      for (int shift = 0; shift < 64; shift += 7) {
        final byte b = readByte();
        result |= (long) (b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
          return result;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    private byte readByte() throws IOException {
      if (pos == limit) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      return buffer[pos++];
    }

    private int readLittleEndian32() throws IOException {
      requireBytes(FIXED32_SIZE);
      return readLittleEndian32_NoCheck();
    }

    private long readLittleEndian64() throws IOException {
      requireBytes(FIXED64_SIZE);
      return readLittleEndian64_NoCheck();
    }

    private int readLittleEndian32_NoCheck() {
      int p = pos;
      final byte[] buffer = this.buffer;
      pos = p + FIXED32_SIZE;
      return (((buffer[p] & 0xff))
          | ((buffer[p + 1] & 0xff) << 8)
          | ((buffer[p + 2] & 0xff) << 16)
          | ((buffer[p + 3] & 0xff) << 24));
    }

    private long readLittleEndian64_NoCheck() {
      int p = pos;
      final byte[] buffer = this.buffer;
      pos = p + FIXED64_SIZE;
      return (((buffer[p] & 0xffL))
          | ((buffer[p + 1] & 0xffL) << 8)
          | ((buffer[p + 2] & 0xffL) << 16)
          | ((buffer[p + 3] & 0xffL) << 24)
          | ((buffer[p + 4] & 0xffL) << 32)
          | ((buffer[p + 5] & 0xffL) << 40)
          | ((buffer[p + 6] & 0xffL) << 48)
          | ((buffer[p + 7] & 0xffL) << 56));
    }

    private void skipVarint() throws IOException {
      if (limit - pos >= 10) {
        final byte[] buffer = this.buffer;
        int p = pos;
        for (int i = 0; i < 10; i++) {
          if (buffer[p++] >= 0) {
            pos = p;
            return;
          }
        }
      }
      skipVarintSlowPath();
    }

    private void skipVarintSlowPath() throws IOException {
      for (int i = 0; i < 10; i++) {
        if (readByte() >= 0) {
          return;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    private void skipBytes(final int size) throws IOException {
      requireBytes(size);

      pos += size;
    }

    private void skipGroup() throws IOException {
      int prevEndGroupTag = endGroupTag;
      endGroupTag = WireFormat.makeTag(WireFormat.getTagFieldNumber(tag), WIRETYPE_END_GROUP);
      while (true) {
        if (getFieldNumber() == READ_DONE || !skipField()) {
          break;
        }
      }
      if (tag != endGroupTag) {
        throw InvalidProtocolBufferException.parseFailure();
      }
      endGroupTag = prevEndGroupTag;
    }

    private void requireBytes(int size) throws IOException {
      if (size < 0 || size > (limit - pos)) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
    }

    private void requireWireType(int requiredWireType) throws IOException {
      if (WireFormat.getTagWireType(tag) != requiredWireType) {
        throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    private void verifyPackedFixed64Length(int bytes) throws IOException {
      requireBytes(bytes);
      if ((bytes & FIXED64_MULTIPLE_MASK) != 0) {
        // Require that the number of bytes be a multiple of 8.
        throw InvalidProtocolBufferException.parseFailure();
      }
    }

    private void verifyPackedFixed32Length(int bytes) throws IOException {
      requireBytes(bytes);
      if ((bytes & FIXED32_MULTIPLE_MASK) != 0) {
        // Require that the number of bytes be a multiple of 4.
        throw InvalidProtocolBufferException.parseFailure();
      }
    }

    private void requirePosition(int expectedPosition) throws IOException {
      if (pos != expectedPosition) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
    }
  }
}
