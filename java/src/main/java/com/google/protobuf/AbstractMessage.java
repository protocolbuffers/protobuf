// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;

import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * A partial implementation of the {@link Message} interface which implements
 * as many methods of that interface as possible in terms of other methods.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class AbstractMessage extends AbstractMessageLite
                                      implements Message {
  @SuppressWarnings("unchecked")
  public boolean isInitialized() {
    // Check that all required fields are present.
    for (final FieldDescriptor field : getDescriptorForType().getFields()) {
      if (field.isRequired()) {
        if (!hasField(field)) {
          return false;
        }
      }
    }

    // Check that embedded messages are initialized.
    for (final Map.Entry<FieldDescriptor, Object> entry :
        getAllFields().entrySet()) {
      final FieldDescriptor field = entry.getKey();
      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        if (field.isRepeated()) {
          for (final Message element : (List<Message>) entry.getValue()) {
            if (!element.isInitialized()) {
              return false;
            }
          }
        } else {
          if (!((Message) entry.getValue()).isInitialized()) {
            return false;
          }
        }
      }
    }

    return true;
  }

  @Override
  public final String toString() {
    return TextFormat.printToString(this);
  }

  public void writeTo(final CodedOutputStream output) throws IOException {
    final boolean isMessageSet =
        getDescriptorForType().getOptions().getMessageSetWireFormat();

    for (final Map.Entry<FieldDescriptor, Object> entry :
        getAllFields().entrySet()) {
      final FieldDescriptor field = entry.getKey();
      final Object value = entry.getValue();
      if (isMessageSet && field.isExtension() &&
          field.getType() == FieldDescriptor.Type.MESSAGE &&
          !field.isRepeated()) {
        output.writeMessageSetExtension(field.getNumber(), (Message) value);
      } else {
        FieldSet.writeField(field, value, output);
      }
    }

    final UnknownFieldSet unknownFields = getUnknownFields();
    if (isMessageSet) {
      unknownFields.writeAsMessageSetTo(output);
    } else {
      unknownFields.writeTo(output);
    }
  }

  private int memoizedSize = -1;

  public int getSerializedSize() {
    int size = memoizedSize;
    if (size != -1) {
      return size;
    }

    size = 0;
    final boolean isMessageSet =
        getDescriptorForType().getOptions().getMessageSetWireFormat();

    for (final Map.Entry<FieldDescriptor, Object> entry :
        getAllFields().entrySet()) {
      final FieldDescriptor field = entry.getKey();
      final Object value = entry.getValue();
      if (isMessageSet && field.isExtension() &&
          field.getType() == FieldDescriptor.Type.MESSAGE &&
          !field.isRepeated()) {
        size += CodedOutputStream.computeMessageSetExtensionSize(
            field.getNumber(), (Message) value);
      } else {
        size += FieldSet.computeFieldSize(field, value);
      }
    }

    final UnknownFieldSet unknownFields = getUnknownFields();
    if (isMessageSet) {
      size += unknownFields.getSerializedSizeAsMessageSet();
    } else {
      size += unknownFields.getSerializedSize();
    }

    memoizedSize = size;
    return size;
  }

  @Override
  public boolean equals(final Object other) {
    if (other == this) {
      return true;
    }
    if (!(other instanceof Message)) {
      return false;
    }
    final Message otherMessage = (Message) other;
    if (getDescriptorForType() != otherMessage.getDescriptorForType()) {
      return false;
    }
    return getAllFields().equals(otherMessage.getAllFields()) &&
        getUnknownFields().equals(otherMessage.getUnknownFields());
  }

  @Override
  public int hashCode() {
    int hash = 41;
    hash = (19 * hash) + getDescriptorForType().hashCode();
    hash = (53 * hash) + getAllFields().hashCode();
    hash = (29 * hash) + getUnknownFields().hashCode();
    return hash;
  }

  // =================================================================

  /**
   * A partial implementation of the {@link Message.Builder} interface which
   * implements as many methods of that interface as possible in terms of
   * other methods.
   */
  @SuppressWarnings("unchecked")
  public static abstract class Builder<BuilderType extends Builder>
      extends AbstractMessageLite.Builder<BuilderType>
      implements Message.Builder {
    // The compiler produces an error if this is not declared explicitly.
    @Override
    public abstract BuilderType clone();

    public BuilderType clear() {
      for (final Map.Entry<FieldDescriptor, Object> entry :
           getAllFields().entrySet()) {
        clearField(entry.getKey());
      }
      return (BuilderType) this;
    }

    public BuilderType mergeFrom(final Message other) {
      if (other.getDescriptorForType() != getDescriptorForType()) {
        throw new IllegalArgumentException(
          "mergeFrom(Message) can only merge messages of the same type.");
      }

      // Note:  We don't attempt to verify that other's fields have valid
      //   types.  Doing so would be a losing battle.  We'd have to verify
      //   all sub-messages as well, and we'd have to make copies of all of
      //   them to insure that they don't change after verification (since
      //   the Message interface itself cannot enforce immutability of
      //   implementations).
      // TODO(kenton):  Provide a function somewhere called makeDeepCopy()
      //   which allows people to make secure deep copies of messages.

      for (final Map.Entry<FieldDescriptor, Object> entry :
           other.getAllFields().entrySet()) {
        final FieldDescriptor field = entry.getKey();
        if (field.isRepeated()) {
          for (final Object element : (List)entry.getValue()) {
            addRepeatedField(field, element);
          }
        } else if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          final Message existingValue = (Message)getField(field);
          if (existingValue == existingValue.getDefaultInstanceForType()) {
            setField(field, entry.getValue());
          } else {
            setField(field,
              existingValue.newBuilderForType()
                .mergeFrom(existingValue)
                .mergeFrom((Message)entry.getValue())
                .build());
          }
        } else {
          setField(field, entry.getValue());
        }
      }

      mergeUnknownFields(other.getUnknownFields());

      return (BuilderType) this;
    }

    @Override
    public BuilderType mergeFrom(final CodedInputStream input)
                                 throws IOException {
      return mergeFrom(input, ExtensionRegistry.getEmptyRegistry());
    }

    @Override
    public BuilderType mergeFrom(
        final CodedInputStream input,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      final UnknownFieldSet.Builder unknownFields =
        UnknownFieldSet.newBuilder(getUnknownFields());
      while (true) {
        final int tag = input.readTag();
        if (tag == 0) {
          break;
        }

        if (!mergeFieldFrom(input, unknownFields, extensionRegistry,
                            this, tag)) {
          // end group tag
          break;
        }
      }
      setUnknownFields(unknownFields.build());
      return (BuilderType) this;
    }

    /**
     * Like {@link #mergeFrom(CodedInputStream, UnknownFieldSet.Builder,
     * ExtensionRegistryLite, Message.Builder)}, but parses a single field.
     * Package-private because it is used by GeneratedMessage.ExtendableMessage.
     * @param tag The tag, which should have already been read.
     * @return {@code true} unless the tag is an end-group tag.
     */
    static boolean mergeFieldFrom(
        final CodedInputStream input,
        final UnknownFieldSet.Builder unknownFields,
        final ExtensionRegistryLite extensionRegistry,
        final Message.Builder builder,
        final int tag) throws IOException {
      final Descriptor type = builder.getDescriptorForType();

      if (type.getOptions().getMessageSetWireFormat() &&
          tag == WireFormat.MESSAGE_SET_ITEM_TAG) {
        mergeMessageSetExtensionFromCodedStream(
          input, unknownFields, extensionRegistry, builder);
        return true;
      }

      final int wireType = WireFormat.getTagWireType(tag);
      final int fieldNumber = WireFormat.getTagFieldNumber(tag);

      final FieldDescriptor field;
      Message defaultInstance = null;

      if (type.isExtensionNumber(fieldNumber)) {
        // extensionRegistry may be either ExtensionRegistry or
        // ExtensionRegistryLite.  Since the type we are parsing is a full
        // message, only a full ExtensionRegistry could possibly contain
        // extensions of it.  Otherwise we will treat the registry as if it
        // were empty.
        if (extensionRegistry instanceof ExtensionRegistry) {
          final ExtensionRegistry.ExtensionInfo extension =
            ((ExtensionRegistry) extensionRegistry)
              .findExtensionByNumber(type, fieldNumber);
          if (extension == null) {
            field = null;
          } else {
            field = extension.descriptor;
            defaultInstance = extension.defaultInstance;
            if (defaultInstance == null &&
                field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
              throw new IllegalStateException(
                  "Message-typed extension lacked default instance: " +
                  field.getFullName());
            }
          }
        } else {
          field = null;
        }
      } else {
        field = type.findFieldByNumber(fieldNumber);
      }

      boolean unknown = false;
      boolean packed = false;
      if (field == null) {
        unknown = true;  // Unknown field.
      } else if (wireType == FieldSet.getWireFormatForFieldType(
                   field.getLiteType(),
                   false  /* isPacked */)) {
        packed = false;
      } else if (field.isPackable() &&
                 wireType == FieldSet.getWireFormatForFieldType(
                   field.getLiteType(),
                   true  /* isPacked */)) {
        packed = true;
      } else {
        unknown = true;  // Unknown wire type.
      }

      if (unknown) {  // Unknown field or wrong wire type.  Skip.
        return unknownFields.mergeFieldFrom(tag, input);
      }

      if (packed) {
        final int length = input.readRawVarint32();
        final int limit = input.pushLimit(length);
        if (field.getLiteType() == WireFormat.FieldType.ENUM) {
          while (input.getBytesUntilLimit() > 0) {
            final int rawValue = input.readEnum();
            final Object value = field.getEnumType().findValueByNumber(rawValue);
            if (value == null) {
              // If the number isn't recognized as a valid value for this
              // enum, drop it (don't even add it to unknownFields).
              return true;
            }
            builder.addRepeatedField(field, value);
          }
        } else {
          while (input.getBytesUntilLimit() > 0) {
            final Object value =
              FieldSet.readPrimitiveField(input, field.getLiteType());
            builder.addRepeatedField(field, value);
          }
        }
        input.popLimit(limit);
      } else {
        final Object value;
        switch (field.getType()) {
          case GROUP: {
            final Message.Builder subBuilder;
            if (defaultInstance != null) {
              subBuilder = defaultInstance.newBuilderForType();
            } else {
              subBuilder = builder.newBuilderForField(field);
            }
            if (!field.isRepeated()) {
              subBuilder.mergeFrom((Message) builder.getField(field));
            }
            input.readGroup(field.getNumber(), subBuilder, extensionRegistry);
            value = subBuilder.build();
            break;
          }
          case MESSAGE: {
            final Message.Builder subBuilder;
            if (defaultInstance != null) {
              subBuilder = defaultInstance.newBuilderForType();
            } else {
              subBuilder = builder.newBuilderForField(field);
            }
            if (!field.isRepeated()) {
              subBuilder.mergeFrom((Message) builder.getField(field));
            }
            input.readMessage(subBuilder, extensionRegistry);
            value = subBuilder.build();
            break;
          }
          case ENUM:
            final int rawValue = input.readEnum();
            value = field.getEnumType().findValueByNumber(rawValue);
            // If the number isn't recognized as a valid value for this enum,
            // drop it.
            if (value == null) {
              unknownFields.mergeVarintField(fieldNumber, rawValue);
              return true;
            }
            break;
          default:
            value = FieldSet.readPrimitiveField(input, field.getLiteType());
            break;
        }

        if (field.isRepeated()) {
          builder.addRepeatedField(field, value);
        } else {
          builder.setField(field, value);
        }
      }

      return true;
    }

    /** Called by {@code #mergeFieldFrom()} to parse a MessageSet extension. */
    private static void mergeMessageSetExtensionFromCodedStream(
        final CodedInputStream input,
        final UnknownFieldSet.Builder unknownFields,
        final ExtensionRegistryLite extensionRegistry,
        final Message.Builder builder) throws IOException {
      final Descriptor type = builder.getDescriptorForType();

      // The wire format for MessageSet is:
      //   message MessageSet {
      //     repeated group Item = 1 {
      //       required int32 typeId = 2;
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
      ByteString rawBytes = null;  // If we encounter "message" before "typeId"
      Message.Builder subBuilder = null;
      FieldDescriptor field = null;

      while (true) {
        final int tag = input.readTag();
        if (tag == 0) {
          break;
        }

        if (tag == WireFormat.MESSAGE_SET_TYPE_ID_TAG) {
          typeId = input.readUInt32();
          // Zero is not a valid type ID.
          if (typeId != 0) {
            final ExtensionRegistry.ExtensionInfo extension;

            // extensionRegistry may be either ExtensionRegistry or
            // ExtensionRegistryLite.  Since the type we are parsing is a full
            // message, only a full ExtensionRegistry could possibly contain
            // extensions of it.  Otherwise we will treat the registry as if it
            // were empty.
            if (extensionRegistry instanceof ExtensionRegistry) {
              extension = ((ExtensionRegistry) extensionRegistry)
                  .findExtensionByNumber(type, typeId);
            } else {
              extension = null;
            }

            if (extension != null) {
              field = extension.descriptor;
              subBuilder = extension.defaultInstance.newBuilderForType();
              final Message originalMessage = (Message)builder.getField(field);
              if (originalMessage != null) {
                subBuilder.mergeFrom(originalMessage);
              }
              if (rawBytes != null) {
                // We already encountered the message.  Parse it now.
                subBuilder.mergeFrom(
                  CodedInputStream.newInstance(rawBytes.newInput()));
                rawBytes = null;
              }
            } else {
              // Unknown extension number.  If we already saw data, put it
              // in rawBytes.
              if (rawBytes != null) {
                unknownFields.mergeField(typeId,
                  UnknownFieldSet.Field.newBuilder()
                    .addLengthDelimited(rawBytes)
                    .build());
                rawBytes = null;
              }
            }
          }
        } else if (tag == WireFormat.MESSAGE_SET_MESSAGE_TAG) {
          if (typeId == 0) {
            // We haven't seen a type ID yet, so we have to store the raw bytes
            // for now.
            rawBytes = input.readBytes();
          } else if (subBuilder == null) {
            // We don't know how to parse this.  Ignore it.
            unknownFields.mergeField(typeId,
              UnknownFieldSet.Field.newBuilder()
                .addLengthDelimited(input.readBytes())
                .build());
          } else {
            // We already know the type, so we can parse directly from the input
            // with no copying.  Hooray!
            input.readMessage(subBuilder, extensionRegistry);
          }
        } else {
          // Unknown tag.  Skip it.
          if (!input.skipField(tag)) {
            break;  // end of group
          }
        }
      }

      input.checkLastTagWas(WireFormat.MESSAGE_SET_ITEM_END_TAG);

      if (subBuilder != null) {
        builder.setField(field, subBuilder.build());
      }
    }

    public BuilderType mergeUnknownFields(final UnknownFieldSet unknownFields) {
      setUnknownFields(
        UnknownFieldSet.newBuilder(getUnknownFields())
                       .mergeFrom(unknownFields)
                       .build());
      return (BuilderType) this;
    }

    /**
     * Construct an UninitializedMessageException reporting missing fields in
     * the given message.
     */
    protected static UninitializedMessageException
        newUninitializedMessageException(Message message) {
      return new UninitializedMessageException(findMissingFields(message));
    }

    /**
     * Populates {@code this.missingFields} with the full "path" of each
     * missing required field in the given message.
     */
    private static List<String> findMissingFields(final Message message) {
      final List<String> results = new ArrayList<String>();
      findMissingFields(message, "", results);
      return results;
    }

    /** Recursive helper implementing {@link #findMissingFields(Message)}. */
    private static void findMissingFields(final Message message,
                                          final String prefix,
                                          final List<String> results) {
      for (final FieldDescriptor field :
          message.getDescriptorForType().getFields()) {
        if (field.isRequired() && !message.hasField(field)) {
          results.add(prefix + field.getName());
        }
      }

      for (final Map.Entry<FieldDescriptor, Object> entry :
           message.getAllFields().entrySet()) {
        final FieldDescriptor field = entry.getKey();
        final Object value = entry.getValue();

        if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          if (field.isRepeated()) {
            int i = 0;
            for (final Object element : (List) value) {
              findMissingFields((Message) element,
                                subMessagePrefix(prefix, field, i++),
                                results);
            }
          } else {
            if (message.hasField(field)) {
              findMissingFields((Message) value,
                                subMessagePrefix(prefix, field, -1),
                                results);
            }
          }
        }
      }
    }

    private static String subMessagePrefix(final String prefix,
                                           final FieldDescriptor field,
                                           final int index) {
      final StringBuilder result = new StringBuilder(prefix);
      if (field.isExtension()) {
        result.append('(')
              .append(field.getFullName())
              .append(')');
      } else {
        result.append(field.getName());
      }
      if (index != -1) {
        result.append('[')
              .append(index)
              .append(']');
      }
      result.append('.');
      return result.toString();
    }

    // ===============================================================
    // The following definitions seem to be required in order to make javac
    // not produce weird errors like:
    //
    // java/com/google/protobuf/DynamicMessage.java:203: types
    //   com.google.protobuf.AbstractMessage.Builder<
    //     com.google.protobuf.DynamicMessage.Builder> and
    //   com.google.protobuf.AbstractMessage.Builder<
    //     com.google.protobuf.DynamicMessage.Builder> are incompatible; both
    //   define mergeFrom(com.google.protobuf.ByteString), but with unrelated
    //   return types.
    //
    // Strangely, these lines are only needed if javac is invoked separately
    // on AbstractMessage.java and AbstractMessageLite.java.  If javac is
    // invoked on both simultaneously, it works.  (Or maybe the important
    // point is whether or not DynamicMessage.java is compiled together with
    // AbstractMessageLite.java -- not sure.)  I suspect this is a compiler
    // bug.

    @Override
    public BuilderType mergeFrom(final ByteString data)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data);
    }

    @Override
    public BuilderType mergeFrom(
        final ByteString data,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(final byte[] data)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data, final int off, final int len)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data, off, len);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data, final int off, final int len,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data, off, len, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(final InputStream input)
        throws IOException {
      return super.mergeFrom(input);
    }

    @Override
    public BuilderType mergeFrom(
        final InputStream input,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      return super.mergeFrom(input, extensionRegistry);
    }

    @Override
    public boolean mergeDelimitedFrom(final InputStream input)
        throws IOException {
      return super.mergeDelimitedFrom(input);
    }

    @Override
    public boolean mergeDelimitedFrom(
        final InputStream input,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      return super.mergeDelimitedFrom(input, extensionRegistry);
    }

  }
}
