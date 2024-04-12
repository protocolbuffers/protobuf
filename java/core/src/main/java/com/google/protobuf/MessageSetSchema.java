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

import java.io.IOException;
import java.util.Iterator;
import java.util.Map.Entry;

/** Schema used for proto2 messages using message_set_wireformat. */
@CheckReturnValue
final class MessageSetSchema<T> implements Schema<T> {
  private final MessageLite defaultInstance;
  private final UnknownFieldSchema<?, ?> unknownFieldSchema;
  private final boolean hasExtensions;
  private final ExtensionSchema<?> extensionSchema;

  private MessageSetSchema(
      UnknownFieldSchema<?, ?> unknownFieldSchema,
      ExtensionSchema<?> extensionSchema,
      MessageLite defaultInstance) {
    this.unknownFieldSchema = unknownFieldSchema;
    this.hasExtensions = extensionSchema.hasExtensions(defaultInstance);
    this.extensionSchema = extensionSchema;
    this.defaultInstance = defaultInstance;
  }

  static <T> MessageSetSchema<T> newSchema(
      UnknownFieldSchema<?, ?> unknownFieldSchema,
      ExtensionSchema<?> extensionSchema,
      MessageLite defaultInstance) {
    return new MessageSetSchema<T>(unknownFieldSchema, extensionSchema, defaultInstance);
  }

  @SuppressWarnings("unchecked")
  @Override
  public T newInstance() {
    // TODO(b/248560713) decide if we're keeping support for Full in schema classes and handle this
    // better.
    if (defaultInstance instanceof GeneratedMessageLite) {
      return (T) ((GeneratedMessageLite<?, ?>) defaultInstance).newMutableInstance();
    } else {
      return (T) defaultInstance.newBuilderForType().buildPartial();
    }
  }

  @Override
  public boolean equals(T message, T other) {
    Object messageUnknown = unknownFieldSchema.getFromMessage(message);
    Object otherUnknown = unknownFieldSchema.getFromMessage(other);
    if (!messageUnknown.equals(otherUnknown)) {
      return false;
    }
    if (hasExtensions) {
      FieldSet<?> messageExtensions = extensionSchema.getExtensions(message);
      FieldSet<?> otherExtensions = extensionSchema.getExtensions(other);
      return messageExtensions.equals(otherExtensions);
    }
    return true;
  }

  @Override
  public int hashCode(T message) {
    int hashCode = unknownFieldSchema.getFromMessage(message).hashCode();
    if (hasExtensions) {
      FieldSet<?> extensions = extensionSchema.getExtensions(message);
      hashCode = (hashCode * 53) + extensions.hashCode();
    }
    return hashCode;
  }

  @Override
  public void mergeFrom(T message, T other) {
    SchemaUtil.mergeUnknownFields(unknownFieldSchema, message, other);
    if (hasExtensions) {
      SchemaUtil.mergeExtensions(extensionSchema, message, other);
    }
  }

  @SuppressWarnings("unchecked")
  @Override
  public void writeTo(T message, Writer writer) throws IOException {
    FieldSet<?> extensions = extensionSchema.getExtensions(message);
    Iterator<?> iterator = extensions.iterator();
    while (iterator.hasNext()) {
      Entry<?, ?> extension = (Entry<?, ?>) iterator.next();
      FieldSet.FieldDescriptorLite<?> fd = (FieldSet.FieldDescriptorLite<?>) extension.getKey();
      if (fd.getLiteJavaType() != WireFormat.JavaType.MESSAGE || fd.isRepeated() || fd.isPacked()) {
        throw new IllegalStateException("Found invalid MessageSet item.");
      }
      if (extension instanceof LazyField.LazyEntry) {
        writer.writeMessageSetItem(
            fd.getNumber(), ((LazyField.LazyEntry) extension).getField().toByteString());
      } else {
        writer.writeMessageSetItem(fd.getNumber(), extension.getValue());
      }
    }
    writeUnknownFieldsHelper(unknownFieldSchema, message, writer);
  }

  /**
   * A helper method for wildcard capture of {@code unknownFieldSchema}. See:
   * https://docs.oracle.com/javase/tutorial/java/generics/capture.html
   */
  private <UT, UB> void writeUnknownFieldsHelper(
      UnknownFieldSchema<UT, UB> unknownFieldSchema, T message, Writer writer) throws IOException {
    unknownFieldSchema.writeAsMessageSetTo(unknownFieldSchema.getFromMessage(message), writer);
  }

  @SuppressWarnings("ReferenceEquality")
  @Override
  public void mergeFrom(
      T message, byte[] data, int position, int limit, ArrayDecoders.Registers registers)
      throws IOException {
    // TODO(b/248560713) decide if we're keeping support for Full in schema classes and handle this
    // better.
    UnknownFieldSetLite unknownFields = ((GeneratedMessageLite) message).unknownFields;
    if (unknownFields == UnknownFieldSetLite.getDefaultInstance()) {
      unknownFields = UnknownFieldSetLite.newInstance();
      ((GeneratedMessageLite) message).unknownFields = unknownFields;
    }
    final FieldSet<GeneratedMessageLite.ExtensionDescriptor> extensions =
        ((GeneratedMessageLite.ExtendableMessage<?, ?>) message).ensureExtensionsAreMutable();
    GeneratedMessageLite.GeneratedExtension<?, ?> extension = null;
    while (position < limit) {
      position = ArrayDecoders.decodeVarint32(data, position, registers);
      final int startTag = registers.int1;
      if (startTag != WireFormat.MESSAGE_SET_ITEM_TAG) {
        if (WireFormat.getTagWireType(startTag) == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          extension =
              (GeneratedMessageLite.GeneratedExtension<?, ?>) extensionSchema.findExtensionByNumber(
                  registers.extensionRegistry, defaultInstance,
                  WireFormat.getTagFieldNumber(startTag));
          if (extension != null) {
            position =
                ArrayDecoders.decodeMessageField(
                    Protobuf.getInstance().schemaFor(
                        extension.getMessageDefaultInstance().getClass()),
                    data, position, limit, registers);
            extensions.setField(extension.descriptor, registers.object1);
          } else {
            position =
                ArrayDecoders.decodeUnknownField(
                    startTag, data, position, limit, unknownFields, registers);
          }
        } else {
          position = ArrayDecoders.skipField(startTag, data, position, limit, registers);
        }
        continue;
      }

      int typeId = 0;
      ByteString rawBytes = null;

      while (position < limit) {
        position = ArrayDecoders.decodeVarint32(data, position, registers);
        final int tag = registers.int1;
        final int number = WireFormat.getTagFieldNumber(tag);
        final int wireType = WireFormat.getTagWireType(tag);
        switch (number) {
          case WireFormat.MESSAGE_SET_TYPE_ID:
            if (wireType == WireFormat.WIRETYPE_VARINT) {
              position = ArrayDecoders.decodeVarint32(data, position, registers);
              typeId = registers.int1;
              // TODO(b/248560713) decide if we're keeping support for Full in schema classes and
              // handle this better.
              extension =
                  (GeneratedMessageLite.GeneratedExtension<?, ?>)
                      extensionSchema.findExtensionByNumber(
                          registers.extensionRegistry, defaultInstance, typeId);
              continue;
            }
            break;
          case WireFormat.MESSAGE_SET_MESSAGE:
            if (extension != null) {
              position = ArrayDecoders.decodeMessageField(
                  Protobuf.getInstance().schemaFor(
                      extension.getMessageDefaultInstance().getClass()),
                  data, position, limit, registers);
              extensions.setField(extension.descriptor, registers.object1);
              continue;
            } else {
              if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
                position = ArrayDecoders.decodeBytes(data, position, registers);
                rawBytes = (ByteString) registers.object1;
                continue;
              }
              break;
            }
          default:
            break;
        }
        if (tag == WireFormat.MESSAGE_SET_ITEM_END_TAG) {
          break;
        }
        position = ArrayDecoders.skipField(tag, data, position, limit, registers);
      }

      if (rawBytes != null) {
        unknownFields.storeField(
            WireFormat.makeTag(typeId, WireFormat.WIRETYPE_LENGTH_DELIMITED), rawBytes);
      }
    }
    if (position != limit) {
      throw InvalidProtocolBufferException.parseFailure();
    }
  }

  @Override
  public void mergeFrom(T message, Reader reader, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    mergeFromHelper(unknownFieldSchema, extensionSchema, message, reader, extensionRegistry);
  }

  /**
   * A helper method for wildcard capture of {@code unknownFieldSchema}. See:
   * https://docs.oracle.com/javase/tutorial/java/generics/capture.html
   */
  private <UT, UB, ET extends FieldSet.FieldDescriptorLite<ET>> void mergeFromHelper(
      UnknownFieldSchema<UT, UB> unknownFieldSchema,
      ExtensionSchema<ET> extensionSchema,
      T message,
      Reader reader,
      ExtensionRegistryLite extensionRegistry)
      throws IOException {
    UB unknownFields = unknownFieldSchema.getBuilderFromMessage(message);
    FieldSet<ET> extensions = extensionSchema.getMutableExtensions(message);
    try {
      while (true) {
        final int number = reader.getFieldNumber();
        if (number == Reader.READ_DONE) {
          return;
        }
        if (parseMessageSetItemOrUnknownField(
            reader,
            extensionRegistry,
            extensionSchema,
            extensions,
            unknownFieldSchema,
            unknownFields)) {
          continue;
        }
        // Done reading.
        return;
      }
    } finally {
      unknownFieldSchema.setBuilderToMessage(message, unknownFields);
    }
  }

  @Override
  public void makeImmutable(T message) {
    unknownFieldSchema.makeImmutable(message);
    extensionSchema.makeImmutable(message);
  }

  private <UT, UB, ET extends FieldSet.FieldDescriptorLite<ET>>
      boolean parseMessageSetItemOrUnknownField(
          Reader reader,
          ExtensionRegistryLite extensionRegistry,
          ExtensionSchema<ET> extensionSchema,
          FieldSet<ET> extensions,
          UnknownFieldSchema<UT, UB> unknownFieldSchema,
          UB unknownFields)
          throws IOException {
    int startTag = reader.getTag();
    if (startTag != WireFormat.MESSAGE_SET_ITEM_TAG) {
      if (WireFormat.getTagWireType(startTag) == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
        Object extension =
            extensionSchema.findExtensionByNumber(
                extensionRegistry, defaultInstance, WireFormat.getTagFieldNumber(startTag));
        if (extension != null) {
          extensionSchema.parseLengthPrefixedMessageSetItem(
              reader, extension, extensionRegistry, extensions);
          return true;
        } else {
          return unknownFieldSchema.mergeOneFieldFrom(unknownFields, reader);
        }
      } else {
        return reader.skipField();
      }
    }

    // The wire format for MessageSet is:
    //   message MessageSet {
    //     repeated group Item = 1 {
    //       required uint32 typeId = 2;
    //       required bytes message = 3;
    //     }
    //   }
    // "typeId" is the extension's field number.  The extension can only be
    // a message type, where "message" contains the encoded bytes of that
    // message.
    //
    // In practice, we will probably never see a MessageSet item in which
    // the message appears before the type ID, or where either field does not
    // appear exactly once.  However, in theory such cases are valid, so we
    // should be prepared to accept them.

    int typeId = 0;
    ByteString rawBytes = null; // If we encounter "message" before "typeId"
    Object extension = null;

    // Read bytes from input, if we get it's type first then parse it eagerly,
    // otherwise we store the raw bytes in a local variable.
    loop:
    while (true) {
      final int number = reader.getFieldNumber();
      if (number == Reader.READ_DONE) {
        break;
      }

      final int tag = reader.getTag();
      if (tag == WireFormat.MESSAGE_SET_TYPE_ID_TAG) {
        typeId = reader.readUInt32();
        extension =
            extensionSchema.findExtensionByNumber(extensionRegistry, defaultInstance, typeId);
        continue;
      } else if (tag == WireFormat.MESSAGE_SET_MESSAGE_TAG) {
        if (extension != null) {
          extensionSchema.parseLengthPrefixedMessageSetItem(
              reader, extension, extensionRegistry, extensions);
          continue;
        }
        // We haven't seen a type ID yet or we want parse message lazily.
        rawBytes = reader.readBytes();
        continue;
      } else {
        if (!reader.skipField()) {
          break loop; // End of group
        }
      }
    }

    if (reader.getTag() != WireFormat.MESSAGE_SET_ITEM_END_TAG) {
      throw InvalidProtocolBufferException.invalidEndTag();
    }

    // If there are any rawBytes left, it means the message content appears before the type ID.
    if (rawBytes != null) {
      if (extension != null) { // We known the type
        // TODO(xiaofeng): Instead of reading into a temporary ByteString, maybe there is a way
        // to read directly from Reader to the submessage?
        extensionSchema.parseMessageSetItem(rawBytes, extension, extensionRegistry, extensions);
      } else {
        unknownFieldSchema.addLengthDelimited(unknownFields, typeId, rawBytes);
      }
    }
    return true;
  }

  @Override
  public final boolean isInitialized(T message) {
    FieldSet<?> extensions = extensionSchema.getExtensions(message);
    return extensions.isInitialized();
  }

  @Override
  public int getSerializedSize(T message) {
    int size = 0;

    size += getUnknownFieldsSerializedSize(unknownFieldSchema, message);

    if (hasExtensions) {
      size += extensionSchema.getExtensions(message).getMessageSetSerializedSize();
    }

    return size;
  }

  private <UT, UB> int getUnknownFieldsSerializedSize(
      UnknownFieldSchema<UT, UB> schema, T message) {
    UT unknowns = schema.getFromMessage(message);
    return schema.getSerializedSizeAsMessageSet(unknowns);
  }
}
