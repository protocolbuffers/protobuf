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
import java.util.List;
import java.util.Map;

/** An adapter between the {@link Reader} interface and {@link CodedInputStream}. */
@CheckReturnValue
@ExperimentalApi
final class CodedInputStreamReader implements Reader {
  private static final int FIXED32_MULTIPLE_MASK = FIXED32_SIZE - 1;
  private static final int FIXED64_MULTIPLE_MASK = FIXED64_SIZE - 1;
  private static final int NEXT_TAG_UNSET = 0;

  private final CodedInputStream input;
  private int tag;
  private int endGroupTag;
  private int nextTag = NEXT_TAG_UNSET;

  public static CodedInputStreamReader forCodedInput(CodedInputStream input) {
    if (input.wrapper != null) {
      return input.wrapper;
    }
    return new CodedInputStreamReader(input);
  }

  private CodedInputStreamReader(CodedInputStream input) {
    this.input = Internal.checkNotNull(input, "input");
    this.input.wrapper = this;
  }

  @Override
  public boolean shouldDiscardUnknownFields() {
    return input.shouldDiscardUnknownFields();
  }

  @Override
  public int getFieldNumber() throws IOException {
    if (nextTag != NEXT_TAG_UNSET) {
      tag = nextTag;
      nextTag = NEXT_TAG_UNSET;
    } else {
      tag = input.readTag();
    }
    if (tag == 0 || tag == endGroupTag) {
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
    if (input.isAtEnd() || tag == endGroupTag) {
      return false;
    }
    return input.skipField(tag);
  }

  private void requireWireType(int requiredWireType) throws IOException {
    if (WireFormat.getTagWireType(tag) != requiredWireType) {
      throw InvalidProtocolBufferException.invalidWireType();
    }
  }

  @Override
  public double readDouble() throws IOException {
    requireWireType(WIRETYPE_FIXED64);
    return input.readDouble();
  }

  @Override
  public float readFloat() throws IOException {
    requireWireType(WIRETYPE_FIXED32);
    return input.readFloat();
  }

  @Override
  public long readUInt64() throws IOException {
    requireWireType(WIRETYPE_VARINT);
    return input.readUInt64();
  }

  @Override
  public long readInt64() throws IOException {
    requireWireType(WIRETYPE_VARINT);
    return input.readInt64();
  }

  @Override
  public int readInt32() throws IOException {
    requireWireType(WIRETYPE_VARINT);
    return input.readInt32();
  }

  @Override
  public long readFixed64() throws IOException {
    requireWireType(WIRETYPE_FIXED64);
    return input.readFixed64();
  }

  @Override
  public int readFixed32() throws IOException {
    requireWireType(WIRETYPE_FIXED32);
    return input.readFixed32();
  }

  @Override
  public boolean readBool() throws IOException {
    requireWireType(WIRETYPE_VARINT);
    return input.readBool();
  }

  @Override
  public String readString() throws IOException {
    requireWireType(WIRETYPE_LENGTH_DELIMITED);
    return input.readString();
  }

  @Override
  public String readStringRequireUtf8() throws IOException {
    requireWireType(WIRETYPE_LENGTH_DELIMITED);
    return input.readStringRequireUtf8();
  }

  @Override
  public <T> T readMessage(Class<T> clazz, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    requireWireType(WIRETYPE_LENGTH_DELIMITED);
    return readMessage(Protobuf.getInstance().schemaFor(clazz), extensionRegistry);
  }

  @SuppressWarnings("unchecked")
  @Override
  public <T> T readMessageBySchemaWithCheck(
      Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException {
    requireWireType(WIRETYPE_LENGTH_DELIMITED);
    return readMessage(schema, extensionRegistry);
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
  public <T> T readGroupBySchemaWithCheck(Schema<T> schema, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    requireWireType(WIRETYPE_START_GROUP);
    return readGroup(schema, extensionRegistry);
  }

  @Override
  public <T> void mergeMessageField(
      T target, Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException {
    requireWireType(WIRETYPE_LENGTH_DELIMITED);
    mergeMessageFieldInternal(target, schema, extensionRegistry);
  }

  private <T> void mergeMessageFieldInternal(
      T target, Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException {
    int size = input.readUInt32();
    if (input.recursionDepth >= input.recursionLimit) {
      throw InvalidProtocolBufferException.recursionLimitExceeded();
    }

    // Push the new limit.
    final int prevLimit = input.pushLimit(size);
    ++input.recursionDepth;
    schema.mergeFrom(target, this, extensionRegistry);
    input.checkLastTagWas(0);
    --input.recursionDepth;
    // Restore the previous limit.
    input.popLimit(prevLimit);
  }

  // Should have the same semantics of CodedInputStream#readMessage()
  private <T> T readMessage(Schema<T> schema, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    T newInstance = schema.newInstance();
    mergeMessageFieldInternal(newInstance, schema, extensionRegistry);
    schema.makeImmutable(newInstance);
    return newInstance;
  }

  @Override
  public <T> void mergeGroupField(
      T target, Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException {
    requireWireType(WIRETYPE_START_GROUP);
    mergeGroupFieldInternal(target, schema, extensionRegistry);
  }

  private <T> void mergeGroupFieldInternal(
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

  private <T> T readGroup(Schema<T> schema, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    T newInstance = schema.newInstance();
    mergeGroupFieldInternal(newInstance, schema, extensionRegistry);
    schema.makeImmutable(newInstance);
    return newInstance;
  }

  @Override
  public ByteString readBytes() throws IOException {
    requireWireType(WIRETYPE_LENGTH_DELIMITED);
    return input.readBytes();
  }

  @Override
  public int readUInt32() throws IOException {
    requireWireType(WIRETYPE_VARINT);
    return input.readUInt32();
  }

  @Override
  public int readEnum() throws IOException {
    requireWireType(WIRETYPE_VARINT);
    return input.readEnum();
  }

  @Override
  public int readSFixed32() throws IOException {
    requireWireType(WIRETYPE_FIXED32);
    return input.readSFixed32();
  }

  @Override
  public long readSFixed64() throws IOException {
    requireWireType(WIRETYPE_FIXED64);
    return input.readSFixed64();
  }

  @Override
  public int readSInt32() throws IOException {
    requireWireType(WIRETYPE_VARINT);
    return input.readSInt32();
  }

  @Override
  public long readSInt64() throws IOException {
    requireWireType(WIRETYPE_VARINT);
    return input.readSInt64();
  }

  @Override
  public void readDoubleList(List<Double> target) throws IOException {
    if (target instanceof DoubleArrayList) {
      DoubleArrayList plist = (DoubleArrayList) target;
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          verifyPackedFixed64Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addDouble(input.readDouble());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED64:
          while (true) {
            plist.addDouble(input.readDouble());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          verifyPackedFixed64Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readDouble());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED64:
          while (true) {
            target.add(input.readDouble());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          verifyPackedFixed32Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addFloat(input.readFloat());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED32:
          while (true) {
            plist.addFloat(input.readFloat());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          verifyPackedFixed32Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readFloat());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED32:
          while (true) {
            target.add(input.readFloat());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addLong(input.readUInt64());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            plist.addLong(input.readUInt64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readUInt64());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            target.add(input.readUInt64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addLong(input.readInt64());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            plist.addLong(input.readInt64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readInt64());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            target.add(input.readInt64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addInt(input.readInt32());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            plist.addInt(input.readInt32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readInt32());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            target.add(input.readInt32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          verifyPackedFixed64Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addLong(input.readFixed64());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED64:
          while (true) {
            plist.addLong(input.readFixed64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          verifyPackedFixed64Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readFixed64());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED64:
          while (true) {
            target.add(input.readFixed64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          verifyPackedFixed32Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addInt(input.readFixed32());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED32:
          while (true) {
            plist.addInt(input.readFixed32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          verifyPackedFixed32Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readFixed32());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED32:
          while (true) {
            target.add(input.readFixed32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addBoolean(input.readBool());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            plist.addBoolean(input.readBool());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readBool());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            target.add(input.readBool());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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

  public void readStringListInternal(List<String> target, boolean requireUtf8) throws IOException {
    if (WireFormat.getTagWireType(tag) != WIRETYPE_LENGTH_DELIMITED) {
      throw InvalidProtocolBufferException.invalidWireType();
    }

    if (target instanceof LazyStringList && !requireUtf8) {
      LazyStringList lazyList = (LazyStringList) target;
      while (true) {
        lazyList.add(readBytes());
        if (input.isAtEnd()) {
          return;
        }
        int nextTag = input.readTag();
        if (nextTag != tag) {
          // We've reached the end of the repeated field. Save the next tag value.
          this.nextTag = nextTag;
          return;
        }
      }
    } else {
      while (true) {
        target.add(requireUtf8 ? readStringRequireUtf8() : readString());
        if (input.isAtEnd()) {
          return;
        }
        int nextTag = input.readTag();
        if (nextTag != tag) {
          // We've reached the end of the repeated field. Save the next tag value.
          this.nextTag = nextTag;
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
      if (input.isAtEnd() || nextTag != NEXT_TAG_UNSET) {
        return;
      }
      int nextTag = input.readTag();
      if (nextTag != listTag) {
        // We've reached the end of the repeated field. Save the next tag value.
        this.nextTag = nextTag;
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
      if (input.isAtEnd() || nextTag != NEXT_TAG_UNSET) {
        return;
      }
      int nextTag = input.readTag();
      if (nextTag != listTag) {
        // We've reached the end of the repeated field. Save the next tag value.
        this.nextTag = nextTag;
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
      if (input.isAtEnd()) {
        return;
      }
      int nextTag = input.readTag();
      if (nextTag != tag) {
        // We've reached the end of the repeated field. Save the next tag value.
        this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addInt(input.readUInt32());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            plist.addInt(input.readUInt32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readUInt32());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            target.add(input.readUInt32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addInt(input.readEnum());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            plist.addInt(input.readEnum());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readEnum());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            target.add(input.readEnum());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          verifyPackedFixed32Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addInt(input.readSFixed32());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED32:
          while (true) {
            plist.addInt(input.readSFixed32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          verifyPackedFixed32Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readSFixed32());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED32:
          while (true) {
            target.add(input.readSFixed32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          verifyPackedFixed64Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addLong(input.readSFixed64());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED64:
          while (true) {
            plist.addLong(input.readSFixed64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          verifyPackedFixed64Length(bytes);
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readSFixed64());
          } while (input.getTotalBytesRead() < endPos);
          break;
        case WIRETYPE_FIXED64:
          while (true) {
            target.add(input.readSFixed64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addInt(input.readSInt32());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            plist.addInt(input.readSInt32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readSInt32());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            target.add(input.readSInt32());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
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
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            plist.addLong(input.readSInt64());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            plist.addLong(input.readSInt64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    } else {
      switch (WireFormat.getTagWireType(tag)) {
        case WIRETYPE_LENGTH_DELIMITED:
          final int bytes = input.readUInt32();
          int endPos = input.getTotalBytesRead() + bytes;
          do {
            target.add(input.readSInt64());
          } while (input.getTotalBytesRead() < endPos);
          requirePosition(endPos);
          break;
        case WIRETYPE_VARINT:
          while (true) {
            target.add(input.readSInt64());
            if (input.isAtEnd()) {
              return;
            }
            int nextTag = input.readTag();
            if (nextTag != tag) {
              // We've reached the end of the repeated field. Save the next tag value.
              this.nextTag = nextTag;
              return;
            }
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }
  }

  private void verifyPackedFixed64Length(int bytes) throws IOException {
    if ((bytes & FIXED64_MULTIPLE_MASK) != 0) {
      // Require that the number of bytes be a multiple of 8.
      throw InvalidProtocolBufferException.parseFailure();
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
    int size = input.readUInt32();
    final int prevLimit = input.pushLimit(size);
    K key = metadata.defaultKey;
    V value = metadata.defaultValue;
    try {
      while (true) {
        int number = getFieldNumber();
        if (number == READ_DONE || input.isAtEnd()) {
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
                          metadata.valueType, metadata.defaultValue.getClass(), extensionRegistry);
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
      // Restore the previous limit.
      input.popLimit(prevLimit);
    }
  }

  private Object readField(
      WireFormat.FieldType fieldType, Class<?> messageType, ExtensionRegistryLite extensionRegistry)
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
        throw new IllegalArgumentException("unsupported field type.");
    }
  }

  private void verifyPackedFixed32Length(int bytes) throws IOException {
    if ((bytes & FIXED32_MULTIPLE_MASK) != 0) {
      // Require that the number of bytes be a multiple of 4.
      throw InvalidProtocolBufferException.parseFailure();
    }
  }

  private void requirePosition(int expectedPosition) throws IOException {
    if (input.getTotalBytesRead() != expectedPosition) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
  }
}
