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
import com.google.protobuf.GeneratedMessage.ExtendableBuilder;
import com.google.protobuf.Internal.EnumLite;

import java.io.IOException;
import java.io.InputStream;
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

  public List<String> findInitializationErrors() {
    return Builder.findMissingFields(this);
  }

  public String getInitializationErrorString() {
    return delimitWithCommas(findInitializationErrors());
  }

  private static String delimitWithCommas(List<String> parts) {
    StringBuilder result = new StringBuilder();
    for (String part : parts) {
      if (result.length() > 0) {
        result.append(", ");
      }
      result.append(part);
    }
    return result.toString();
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
    hash = hashFields(hash, getAllFields());
    hash = (29 * hash) + getUnknownFields().hashCode();
    return hash;
  }

  /** Get a hash code for given fields and values, using the given seed. */
  @SuppressWarnings("unchecked")
  protected int hashFields(int hash, Map<FieldDescriptor, Object> map) {
    for (Map.Entry<FieldDescriptor, Object> entry : map.entrySet()) {
      FieldDescriptor field = entry.getKey();
      Object value = entry.getValue();
      hash = (37 * hash) + field.getNumber();
      if (field.getType() != FieldDescriptor.Type.ENUM){
        hash = (53 * hash) + value.hashCode();
      } else if (field.isRepeated()) {
        List<? extends EnumLite> list = (List<? extends EnumLite>) value;
        hash = (53 * hash) + hashEnumList(list);
      } else {
        hash = (53 * hash) + hashEnum((EnumLite) value);
      }
    }
    return hash;
  }

  /**
   * Helper method for implementing {@link Message#hashCode()}.
   * @see Boolean#hashCode()
   */
  protected static int hashLong(long n) {
    return (int) (n ^ (n >>> 32));
  }

  /**
   * Helper method for implementing {@link Message#hashCode()}.
   * @see Boolean#hashCode()
   */
  protected static int hashBoolean(boolean b) {
    return b ? 1231 : 1237;
  }

  /**
   * Package private helper method for AbstractParser to create
   * UninitializedMessageException with missing field information.
   */
  @Override
  UninitializedMessageException newUninitializedMessageException() {
    return Builder.newUninitializedMessageException(this);
  }

  /**
   * Helper method for implementing {@link Message#hashCode()}.
   * <p>
   * This is needed because {@link java.lang.Enum#hashCode()} is final, but we
   * need to use the field number as the hash code to ensure compatibility
   * between statically and dynamically generated enum objects.
   */
  protected static int hashEnum(EnumLite e) {
    return e.getNumber();
  }

  /** Helper method for implementing {@link Message#hashCode()}. */
  protected static int hashEnumList(List<? extends EnumLite> list) {
    int hash = 1;
    for (EnumLite e : list) {
      hash = 31 * hash + hashEnum(e);
    }
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

    public List<String> findInitializationErrors() {
      return findMissingFields(this);
    }

    public String getInitializationErrorString() {
      return delimitWithCommas(findInitializationErrors());
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
                            getDescriptorForType(), this, null, tag)) {
          // end group tag
          break;
        }
      }
      setUnknownFields(unknownFields.build());
      return (BuilderType) this;
    }

    /** helper method to handle {@code builder} and {@code extensions}. */
    private static void addRepeatedField(
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions,
        FieldDescriptor field,
        Object value) {
      if (builder != null) {
        builder.addRepeatedField(field, value);
      } else {
        extensions.addRepeatedField(field, value);
      }
    }

    /** helper method to handle {@code builder} and {@code extensions}. */
    private static void setField(
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions,
        FieldDescriptor field,
        Object value) {
      if (builder != null) {
        builder.setField(field, value);
      } else {
        extensions.setField(field, value);
      }
    }

    /** helper method to handle {@code builder} and {@code extensions}. */
    private static boolean hasOriginalMessage(
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions,
        FieldDescriptor field) {
      if (builder != null) {
        return builder.hasField(field);
      } else {
        return extensions.hasField(field);
      }
    }

    /** helper method to handle {@code builder} and {@code extensions}. */
    private static Message getOriginalMessage(
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions,
        FieldDescriptor field) {
      if (builder != null) {
        return (Message) builder.getField(field);
      } else {
        return (Message) extensions.getField(field);
      }
    }

    /** helper method to handle {@code builder} and {@code extensions}. */
    private static void mergeOriginalMessage(
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions,
        FieldDescriptor field,
        Message.Builder subBuilder) {
      Message originalMessage = getOriginalMessage(builder, extensions, field);
      if (originalMessage != null) {
        subBuilder.mergeFrom(originalMessage);
      }
    }

    /**
     * Like {@link #mergeFrom(CodedInputStream, ExtensionRegistryLite)}, but
     * parses a single field.
     *
     * When {@code builder} is not null, the method will parse and merge the
     * field into {@code builder}. Otherwise, it will try to parse the field
     * into {@code extensions}, when it's called by the parsing constructor in
     * generated classes.
     *
     * Package-private because it is used by GeneratedMessage.ExtendableMessage.
     * @param tag The tag, which should have already been read.
     * @return {@code true} unless the tag is an end-group tag.
     */
    static boolean mergeFieldFrom(
        CodedInputStream input,
        UnknownFieldSet.Builder unknownFields,
        ExtensionRegistryLite extensionRegistry,
        Descriptor type,
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions,
        int tag) throws IOException {
      if (type.getOptions().getMessageSetWireFormat() &&
          tag == WireFormat.MESSAGE_SET_ITEM_TAG) {
        mergeMessageSetExtensionFromCodedStream(
            input, unknownFields, extensionRegistry, type, builder, extensions);
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
      } else if (builder != null) {
        field = type.findFieldByNumber(fieldNumber);
      } else {
        field = null;
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
            addRepeatedField(builder, extensions, field, value);
          }
        } else {
          while (input.getBytesUntilLimit() > 0) {
            final Object value =
              FieldSet.readPrimitiveField(input, field.getLiteType());
            addRepeatedField(builder, extensions, field, value);
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
              mergeOriginalMessage(builder, extensions, field, subBuilder);
            }
            input.readGroup(field.getNumber(), subBuilder, extensionRegistry);
            value = subBuilder.buildPartial();
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
              mergeOriginalMessage(builder, extensions, field, subBuilder);
            }
            input.readMessage(subBuilder, extensionRegistry);
            value = subBuilder.buildPartial();
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
          addRepeatedField(builder, extensions, field, value);
        } else {
          setField(builder, extensions, field, value);
        }
      }

      return true;
    }

    /**
     * Called by {@code #mergeFieldFrom()} to parse a MessageSet extension.
     * If {@code builder} is not null, this method will merge MessageSet into
     * the builder.  Otherwise, it will merge the MessageSet into {@code
     * extensions}.
     */
    private static void mergeMessageSetExtensionFromCodedStream(
        CodedInputStream input,
        UnknownFieldSet.Builder unknownFields,
        ExtensionRegistryLite extensionRegistry,
        Descriptor type,
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions) throws IOException {

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
      ByteString rawBytes = null; // If we encounter "message" before "typeId"
      ExtensionRegistry.ExtensionInfo extension = null;

      // Read bytes from input, if we get it's type first then parse it eagerly,
      // otherwise we store the raw bytes in a local variable.
      while (true) {
        final int tag = input.readTag();
        if (tag == 0) {
          break;
        }

        if (tag == WireFormat.MESSAGE_SET_TYPE_ID_TAG) {
          typeId = input.readUInt32();
          if (typeId != 0) {
            // extensionRegistry may be either ExtensionRegistry or
            // ExtensionRegistryLite. Since the type we are parsing is a full
            // message, only a full ExtensionRegistry could possibly contain
            // extensions of it. Otherwise we will treat the registry as if it
            // were empty.
            if (extensionRegistry instanceof ExtensionRegistry) {
              extension = ((ExtensionRegistry) extensionRegistry)
                  .findExtensionByNumber(type, typeId);
            }
          }

        } else if (tag == WireFormat.MESSAGE_SET_MESSAGE_TAG) {
          if (typeId != 0) {
            if (extension != null && ExtensionRegistryLite.isEagerlyParseMessageSets()) {
              // We already know the type, so we can parse directly from the
              // input with no copying.  Hooray!
              eagerlyMergeMessageSetExtension(
                  input, extension, extensionRegistry, builder, extensions);
              rawBytes = null;
              continue;
            }
          }
          // We haven't seen a type ID yet or we want parse message lazily.
          rawBytes = input.readBytes();

        } else { // Unknown tag. Skip it.
          if (!input.skipField(tag)) {
            break; // End of group
          }
        }
      }
      input.checkLastTagWas(WireFormat.MESSAGE_SET_ITEM_END_TAG);

      // Process the raw bytes.
      if (rawBytes != null && typeId != 0) { // Zero is not a valid type ID.
        if (extension != null) { // We known the type
          mergeMessageSetExtensionFromBytes(
              rawBytes, extension, extensionRegistry, builder, extensions);
        } else { // We don't know how to parse this. Ignore it.
          if (rawBytes != null) {
            unknownFields.mergeField(typeId, UnknownFieldSet.Field.newBuilder()
                .addLengthDelimited(rawBytes).build());
          }
        }
      }
    }

    private static void eagerlyMergeMessageSetExtension(
        CodedInputStream input,
        ExtensionRegistry.ExtensionInfo extension,
        ExtensionRegistryLite extensionRegistry,
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions) throws IOException {

      FieldDescriptor field = extension.descriptor;
      Message value = null;
      if (hasOriginalMessage(builder, extensions, field)) {
        Message originalMessage =
            getOriginalMessage(builder, extensions, field);
        Message.Builder subBuilder = originalMessage.toBuilder();
        input.readMessage(subBuilder, extensionRegistry);
        value = subBuilder.buildPartial();
      } else {
        value = input.readMessage(extension.defaultInstance.getParserForType(),
          extensionRegistry);
      }

      if (builder != null) {
        builder.setField(field, value);
      } else {
        extensions.setField(field, value);
      }
    }

    private static void mergeMessageSetExtensionFromBytes(
        ByteString rawBytes,
        ExtensionRegistry.ExtensionInfo extension,
        ExtensionRegistryLite extensionRegistry,
        Message.Builder builder,
        FieldSet<FieldDescriptor> extensions) throws IOException {

      FieldDescriptor field = extension.descriptor;
      boolean hasOriginalValue = hasOriginalMessage(builder, extensions, field);

      if (hasOriginalValue || ExtensionRegistryLite.isEagerlyParseMessageSets()) {
        // If the field already exists, we just parse the field.
        Message value = null;
        if (hasOriginalValue) {
          Message originalMessage =
              getOriginalMessage(builder, extensions, field);
          Message.Builder subBuilder= originalMessage.toBuilder();
          subBuilder.mergeFrom(rawBytes, extensionRegistry);
          value = subBuilder.buildPartial();
        } else {
          value = extension.defaultInstance.getParserForType()
              .parsePartialFrom(rawBytes, extensionRegistry);
        }
        setField(builder, extensions, field, value);
      } else {
        // Use LazyField to load MessageSet lazily.
        LazyField lazyField = new LazyField(
            extension.defaultInstance, extensionRegistry, rawBytes);
        if (builder != null) {
          // TODO(xiangl): it looks like this method can only be invoked by
          // ExtendableBuilder, but I'm not sure. So I double check the type of
          // builder here. It may be useless and need more investigation.
          if (builder instanceof ExtendableBuilder) {
            builder.setField(field, lazyField);
          } else {
            builder.setField(field, lazyField.getValue());
          }
        } else {
          extensions.setField(field, lazyField);
        }
      }
    }

    public BuilderType mergeUnknownFields(final UnknownFieldSet unknownFields) {
      setUnknownFields(
        UnknownFieldSet.newBuilder(getUnknownFields())
                       .mergeFrom(unknownFields)
                       .build());
      return (BuilderType) this;
    }

    public Message.Builder getFieldBuilder(final FieldDescriptor field) {
      throw new UnsupportedOperationException(
          "getFieldBuilder() called on an unsupported message type.");
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
    private static List<String> findMissingFields(
        final MessageOrBuilder message) {
      final List<String> results = new ArrayList<String>();
      findMissingFields(message, "", results);
      return results;
    }

    /** Recursive helper implementing {@link #findMissingFields(Message)}. */
    private static void findMissingFields(final MessageOrBuilder message,
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
              findMissingFields((MessageOrBuilder) element,
                                subMessagePrefix(prefix, field, i++),
                                results);
            }
          } else {
            if (message.hasField(field)) {
              findMissingFields((MessageOrBuilder) value,
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
