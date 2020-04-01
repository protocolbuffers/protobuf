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

import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import java.io.IOException;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

@SuppressWarnings("unchecked")
final class ExtensionSchemaFull extends ExtensionSchema<FieldDescriptor> {

  private static final long EXTENSION_FIELD_OFFSET = getExtensionsFieldOffset();

  private static <T> long getExtensionsFieldOffset() {
    try {
      Field field = GeneratedMessageV3.ExtendableMessage.class.getDeclaredField("extensions");
      return UnsafeUtil.objectFieldOffset(field);
    } catch (Throwable e) {
      throw new IllegalStateException("Unable to lookup extension field offset");
    }
  }

  @Override
  boolean hasExtensions(MessageLite prototype) {
    return prototype instanceof GeneratedMessageV3.ExtendableMessage;
  }

  @Override
  public FieldSet<FieldDescriptor> getExtensions(Object message) {
    return (FieldSet<FieldDescriptor>) UnsafeUtil.getObject(message, EXTENSION_FIELD_OFFSET);
  }

  @Override
  void setExtensions(Object message, FieldSet<FieldDescriptor> extensions) {
    UnsafeUtil.putObject(message, EXTENSION_FIELD_OFFSET, extensions);
  }

  @Override
  FieldSet<FieldDescriptor> getMutableExtensions(Object message) {
    FieldSet<FieldDescriptor> extensions = getExtensions(message);
    if (extensions.isImmutable()) {
      extensions = extensions.clone();
      setExtensions(message, extensions);
    }
    return extensions;
  }

  @Override
  void makeImmutable(Object message) {
    getExtensions(message).makeImmutable();
  }

  @Override
  <UT, UB> UB parseExtension(
      Reader reader,
      Object extensionObject,
      ExtensionRegistryLite extensionRegistry,
      FieldSet<FieldDescriptor> extensions,
      UB unknownFields,
      UnknownFieldSchema<UT, UB> unknownFieldSchema)
      throws IOException {
    ExtensionRegistry.ExtensionInfo extension = (ExtensionRegistry.ExtensionInfo) extensionObject;
    int fieldNumber = extension.descriptor.getNumber();

    if (extension.descriptor.isRepeated() && extension.descriptor.isPacked()) {
      Object value = null;
      switch (extension.descriptor.getLiteType()) {
        case DOUBLE:
          {
            List<Double> list = new ArrayList<Double>();
            reader.readDoubleList(list);
            value = list;
            break;
          }
        case FLOAT:
          {
            List<Float> list = new ArrayList<Float>();
            reader.readFloatList(list);
            value = list;
            break;
          }
        case INT64:
          {
            List<Long> list = new ArrayList<Long>();
            reader.readInt64List(list);
            value = list;
            break;
          }
        case UINT64:
          {
            List<Long> list = new ArrayList<Long>();
            reader.readUInt64List(list);
            value = list;
            break;
          }
        case INT32:
          {
            List<Integer> list = new ArrayList<Integer>();
            reader.readInt32List(list);
            value = list;
            break;
          }
        case FIXED64:
          {
            List<Long> list = new ArrayList<Long>();
            reader.readFixed64List(list);
            value = list;
            break;
          }
        case FIXED32:
          {
            List<Integer> list = new ArrayList<Integer>();
            reader.readFixed32List(list);
            value = list;
            break;
          }
        case BOOL:
          {
            List<Boolean> list = new ArrayList<Boolean>();
            reader.readBoolList(list);
            value = list;
            break;
          }
        case UINT32:
          {
            List<Integer> list = new ArrayList<Integer>();
            reader.readUInt32List(list);
            value = list;
            break;
          }
        case SFIXED32:
          {
            List<Integer> list = new ArrayList<Integer>();
            reader.readSFixed32List(list);
            value = list;
            break;
          }
        case SFIXED64:
          {
            List<Long> list = new ArrayList<Long>();
            reader.readSFixed64List(list);
            value = list;
            break;
          }
        case SINT32:
          {
            List<Integer> list = new ArrayList<Integer>();
            reader.readSInt32List(list);
            value = list;
            break;
          }
        case SINT64:
          {
            List<Long> list = new ArrayList<Long>();
            reader.readSInt64List(list);
            value = list;
            break;
          }
        case ENUM:
          {
            List<Integer> list = new ArrayList<Integer>();
            reader.readEnumList(list);
            List<EnumValueDescriptor> enumList = new ArrayList<EnumValueDescriptor>();
            for (int number : list) {
              EnumValueDescriptor enumDescriptor =
                  extension.descriptor.getEnumType().findValueByNumber(number);
              if (enumDescriptor != null) {
                enumList.add(enumDescriptor);
              } else {
                unknownFields =
                    SchemaUtil.storeUnknownEnum(
                        fieldNumber, number, unknownFields, unknownFieldSchema);
              }
            }
            value = enumList;
            break;
          }
        default:
          throw new IllegalStateException(
              "Type cannot be packed: " + extension.descriptor.getLiteType());
      }
      extensions.setField(extension.descriptor, value);
    } else {
      Object value = null;
      // Enum is a special case because unknown enum values will be put into UnknownFieldSetLite.
      if (extension.descriptor.getLiteType() == WireFormat.FieldType.ENUM) {
        int number = reader.readInt32();
        Object enumValue = extension.descriptor.getEnumType().findValueByNumber(number);
        if (enumValue == null) {
          return SchemaUtil.storeUnknownEnum(
              fieldNumber, number, unknownFields, unknownFieldSchema);
        }
        value = enumValue;
      } else {
        switch (extension.descriptor.getLiteType()) {
          case DOUBLE:
            value = reader.readDouble();
            break;
          case FLOAT:
            value = reader.readFloat();
            break;
          case INT64:
            value = reader.readInt64();
            break;
          case UINT64:
            value = reader.readUInt64();
            break;
          case INT32:
            value = reader.readInt32();
            break;
          case FIXED64:
            value = reader.readFixed64();
            break;
          case FIXED32:
            value = reader.readFixed32();
            break;
          case BOOL:
            value = reader.readBool();
            break;
          case BYTES:
            value = reader.readBytes();
            break;
          case UINT32:
            value = reader.readUInt32();
            break;
          case SFIXED32:
            value = reader.readSFixed32();
            break;
          case SFIXED64:
            value = reader.readSFixed64();
            break;
          case SINT32:
            value = reader.readSInt32();
            break;
          case SINT64:
            value = reader.readSInt64();
            break;

          case STRING:
            value = reader.readString();
            break;
          case GROUP:
            value = reader.readGroup(extension.defaultInstance.getClass(), extensionRegistry);
            break;

          case MESSAGE:
            value = reader.readMessage(extension.defaultInstance.getClass(), extensionRegistry);
            break;

          case ENUM:
            throw new IllegalStateException("Shouldn't reach here.");
        }
      }
      if (extension.descriptor.isRepeated()) {
        extensions.addRepeatedField(extension.descriptor, value);
      } else {
        switch (extension.descriptor.getLiteType()) {
          case MESSAGE:
          case GROUP:
            Object oldValue = extensions.getField(extension.descriptor);
            if (oldValue != null) {
              value = Internal.mergeMessage(oldValue, value);
            }
            break;
          default:
            break;
        }
        extensions.setField(extension.descriptor, value);
      }
    }
    return unknownFields;
  }

  @Override
  int extensionNumber(Map.Entry<?, ?> extension) {
    FieldDescriptor descriptor = (FieldDescriptor) extension.getKey();
    return descriptor.getNumber();
  }

  @Override
  void serializeExtension(Writer writer, Map.Entry<?, ?> extension) throws IOException {
    FieldDescriptor descriptor = (FieldDescriptor) extension.getKey();
    if (descriptor.isRepeated()) {
      switch (descriptor.getLiteType()) {
        case DOUBLE:
          SchemaUtil.writeDoubleList(
              descriptor.getNumber(),
              (List<Double>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case FLOAT:
          SchemaUtil.writeFloatList(
              descriptor.getNumber(),
              (List<Float>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case INT64:
          SchemaUtil.writeInt64List(
              descriptor.getNumber(),
              (List<Long>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case UINT64:
          SchemaUtil.writeUInt64List(
              descriptor.getNumber(),
              (List<Long>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case INT32:
          SchemaUtil.writeInt32List(
              descriptor.getNumber(),
              (List<Integer>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case FIXED64:
          SchemaUtil.writeFixed64List(
              descriptor.getNumber(),
              (List<Long>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case FIXED32:
          SchemaUtil.writeFixed32List(
              descriptor.getNumber(),
              (List<Integer>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case BOOL:
          SchemaUtil.writeBoolList(
              descriptor.getNumber(),
              (List<Boolean>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case BYTES:
          SchemaUtil.writeBytesList(
              descriptor.getNumber(), (List<ByteString>) extension.getValue(), writer);
          break;
        case UINT32:
          SchemaUtil.writeUInt32List(
              descriptor.getNumber(),
              (List<Integer>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case SFIXED32:
          SchemaUtil.writeSFixed32List(
              descriptor.getNumber(),
              (List<Integer>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case SFIXED64:
          SchemaUtil.writeSFixed64List(
              descriptor.getNumber(),
              (List<Long>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case SINT32:
          SchemaUtil.writeSInt32List(
              descriptor.getNumber(),
              (List<Integer>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case SINT64:
          SchemaUtil.writeSInt64List(
              descriptor.getNumber(),
              (List<Long>) extension.getValue(),
              writer,
              descriptor.isPacked());
          break;
        case ENUM:
          {
            List<EnumValueDescriptor> enumList = (List<EnumValueDescriptor>) extension.getValue();
            List<Integer> list = new ArrayList<Integer>();
            for (EnumValueDescriptor d : enumList) {
              list.add(d.getNumber());
            }
            SchemaUtil.writeInt32List(descriptor.getNumber(), list, writer, descriptor.isPacked());
            break;
          }
        case STRING:
          SchemaUtil.writeStringList(
              descriptor.getNumber(), (List<String>) extension.getValue(), writer);
          break;
        case GROUP:
          SchemaUtil.writeGroupList(descriptor.getNumber(), (List<?>) extension.getValue(), writer);
          break;
        case MESSAGE:
          SchemaUtil.writeMessageList(
              descriptor.getNumber(), (List<?>) extension.getValue(), writer);
          break;
      }
    } else {
      switch (descriptor.getLiteType()) {
        case DOUBLE:
          writer.writeDouble(descriptor.getNumber(), (Double) extension.getValue());
          break;
        case FLOAT:
          writer.writeFloat(descriptor.getNumber(), (Float) extension.getValue());
          break;
        case INT64:
          writer.writeInt64(descriptor.getNumber(), (Long) extension.getValue());
          break;
        case UINT64:
          writer.writeUInt64(descriptor.getNumber(), (Long) extension.getValue());
          break;
        case INT32:
          writer.writeInt32(descriptor.getNumber(), (Integer) extension.getValue());
          break;
        case FIXED64:
          writer.writeFixed64(descriptor.getNumber(), (Long) extension.getValue());
          break;
        case FIXED32:
          writer.writeFixed32(descriptor.getNumber(), (Integer) extension.getValue());
          break;
        case BOOL:
          writer.writeBool(descriptor.getNumber(), (Boolean) extension.getValue());
          break;
        case BYTES:
          writer.writeBytes(descriptor.getNumber(), (ByteString) extension.getValue());
          break;
        case UINT32:
          writer.writeUInt32(descriptor.getNumber(), (Integer) extension.getValue());
          break;
        case SFIXED32:
          writer.writeSFixed32(descriptor.getNumber(), (Integer) extension.getValue());
          break;
        case SFIXED64:
          writer.writeSFixed64(descriptor.getNumber(), (Long) extension.getValue());
          break;
        case SINT32:
          writer.writeSInt32(descriptor.getNumber(), (Integer) extension.getValue());
          break;
        case SINT64:
          writer.writeSInt64(descriptor.getNumber(), (Long) extension.getValue());
          break;
        case ENUM:
          writer.writeInt32(
              descriptor.getNumber(), ((EnumValueDescriptor) extension.getValue()).getNumber());
          break;
        case STRING:
          writer.writeString(descriptor.getNumber(), (String) extension.getValue());
          break;
        case GROUP:
          writer.writeGroup(descriptor.getNumber(), extension.getValue());
          break;
        case MESSAGE:
          writer.writeMessage(descriptor.getNumber(), extension.getValue());
          break;
      }
    }
  }

  @Override
  Object findExtensionByNumber(
      ExtensionRegistryLite extensionRegistry, MessageLite defaultInstance, int number) {
    return ((ExtensionRegistry) extensionRegistry)
        .findExtensionByNumber(((Message) defaultInstance).getDescriptorForType(), number);
  }

  @Override
  void parseLengthPrefixedMessageSetItem(
      Reader reader,
      Object extension,
      ExtensionRegistryLite extensionRegistry,
      FieldSet<FieldDescriptor> extensions)
      throws IOException {
    ExtensionRegistry.ExtensionInfo extensionInfo = (ExtensionRegistry.ExtensionInfo) extension;

    if (ExtensionRegistryLite.isEagerlyParseMessageSets()) {
      Object value =
          reader.readMessage(extensionInfo.defaultInstance.getClass(), extensionRegistry);
      extensions.setField(extensionInfo.descriptor, value);
    } else {
      extensions.setField(
          extensionInfo.descriptor,
          new LazyField(extensionInfo.defaultInstance, extensionRegistry, reader.readBytes()));
    }
  }

  @Override
  void parseMessageSetItem(
      ByteString data,
      Object extension,
      ExtensionRegistryLite extensionRegistry,
      FieldSet<FieldDescriptor> extensions)
      throws IOException {
    ExtensionRegistry.ExtensionInfo extensionInfo = (ExtensionRegistry.ExtensionInfo) extension;
    Object value = extensionInfo.defaultInstance.newBuilderForType().buildPartial();

    if (ExtensionRegistryLite.isEagerlyParseMessageSets()) {
      Reader reader = BinaryReader.newInstance(ByteBuffer.wrap(data.toByteArray()), true);
      Protobuf.getInstance().mergeFrom(value, reader, extensionRegistry);
      extensions.setField(extensionInfo.descriptor, value);

      if (reader.getFieldNumber() != Reader.READ_DONE) {
        throw InvalidProtocolBufferException.invalidEndTag();
      }
    } else {
      extensions.setField(
          extensionInfo.descriptor,
          new LazyField(extensionInfo.defaultInstance, extensionRegistry, data));
    }
  }
}
