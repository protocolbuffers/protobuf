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

import java.io.IOException;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Lite version of {@link GeneratedMessage}.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class GeneratedMessageLite extends AbstractMessageLite {
  protected GeneratedMessageLite() {}

  @SuppressWarnings("unchecked")
  public abstract static class Builder<MessageType extends GeneratedMessageLite,
                                       BuilderType extends Builder>
      extends AbstractMessageLite.Builder<BuilderType> {
    protected Builder() {}

    // This is implemented here only to work around an apparent bug in the
    // Java compiler and/or build system.  See bug #1898463.  The mere presence
    // of this dummy clone() implementation makes it go away.
    @Override
    public BuilderType clone() {
      throw new UnsupportedOperationException(
          "This is supposed to be overridden by subclasses.");
    }

    /** All subclasses implement this. */
    public abstract BuilderType mergeFrom(MessageType message);

    // Defined here for return type covariance.
    public abstract MessageType getDefaultInstanceForType();

    /**
     * Get the message being built.  We don't just pass this to the
     * constructor because it becomes null when build() is called.
     */
    protected abstract MessageType internalGetResult();

    /**
     * Called by subclasses to parse an unknown field.
     * @return {@code true} unless the tag is an end-group tag.
     */
    protected boolean parseUnknownField(
        final CodedInputStream input,
        final ExtensionRegistryLite extensionRegistry,
        final int tag) throws IOException {
      return input.skipField(tag);
    }
  }

  // =================================================================
  // Extensions-related stuff

  /**
   * Lite equivalent of {@link GeneratedMessage.ExtendableMessage}.
   */
  public abstract static class ExtendableMessage<
        MessageType extends ExtendableMessage<MessageType>>
      extends GeneratedMessageLite {
    protected ExtendableMessage() {}
    private final FieldSet<ExtensionDescriptor> extensions =
        FieldSet.newFieldSet();

    private void verifyExtensionContainingType(
        final GeneratedExtension<MessageType, ?> extension) {
      if (extension.getContainingTypeDefaultInstance() !=
          getDefaultInstanceForType()) {
        // This can only happen if someone uses unchecked operations.
        throw new IllegalArgumentException(
          "This extension is for a different message type.  Please make " +
          "sure that you are not suppressing any generics type warnings.");
      }
    }

    /** Check if a singular extension is present. */
    public final boolean hasExtension(
        final GeneratedExtension<MessageType, ?> extension) {
      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.descriptor);
    }

    /** Get the number of elements in a repeated extension. */
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      verifyExtensionContainingType(extension);
      return extensions.getRepeatedFieldCount(extension.descriptor);
    }

    /** Get the value of an extension. */
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      final Object value = extensions.getField(extension.descriptor);
      if (value == null) {
        return extension.defaultValue;
      } else {
        return (Type) value;
      }
    }

    /** Get one element of a repeated extension. */
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index) {
      verifyExtensionContainingType(extension);
      return (Type) extensions.getRepeatedField(extension.descriptor, index);
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions.isInitialized();
    }

    /**
     * Used by subclasses to serialize extensions.  Extension ranges may be
     * interleaved with field numbers, but we must write them in canonical
     * (sorted by field number) order.  ExtensionWriter helps us write
     * individual ranges of extensions at once.
     */
    protected class ExtensionWriter {
      // Imagine how much simpler this code would be if Java iterators had
      // a way to get the next element without advancing the iterator.

      private final Iterator<Map.Entry<ExtensionDescriptor, Object>> iter =
            extensions.iterator();
      private Map.Entry<ExtensionDescriptor, Object> next;
      private final boolean messageSetWireFormat;

      private ExtensionWriter(boolean messageSetWireFormat) {
        if (iter.hasNext()) {
          next = iter.next();
        }
        this.messageSetWireFormat = messageSetWireFormat;
      }

      public void writeUntil(final int end, final CodedOutputStream output)
                             throws IOException {
        while (next != null && next.getKey().getNumber() < end) {
          ExtensionDescriptor extension = next.getKey();
          if (messageSetWireFormat && extension.getLiteJavaType() ==
                  WireFormat.JavaType.MESSAGE &&
              !extension.isRepeated()) {
            output.writeMessageSetExtension(extension.getNumber(),
                                            (MessageLite) next.getValue());
          } else {
            FieldSet.writeField(extension, next.getValue(), output);
          }
          if (iter.hasNext()) {
            next = iter.next();
          } else {
            next = null;
          }
        }
      }
    }

    protected ExtensionWriter newExtensionWriter() {
      return new ExtensionWriter(false);
    }
    protected ExtensionWriter newMessageSetExtensionWriter() {
      return new ExtensionWriter(true);
    }

    /** Called by subclasses to compute the size of extensions. */
    protected int extensionsSerializedSize() {
      return extensions.getSerializedSize();
    }
    protected int extensionsSerializedSizeAsMessageSet() {
      return extensions.getMessageSetSerializedSize();
    }
  }

  /**
   * Lite equivalent of {@link GeneratedMessage.ExtendableBuilder}.
   */
  @SuppressWarnings("unchecked")
  public abstract static class ExtendableBuilder<
        MessageType extends ExtendableMessage<MessageType>,
        BuilderType extends ExtendableBuilder<MessageType, BuilderType>>
      extends Builder<MessageType, BuilderType> {
    protected ExtendableBuilder() {}

    // This is implemented here only to work around an apparent bug in the
    // Java compiler and/or build system.  See bug #1898463.  The mere presence
    // of this dummy clone() implementation makes it go away.
    @Override
    public BuilderType clone() {
      throw new UnsupportedOperationException(
          "This is supposed to be overridden by subclasses.");
    }

    @Override
    protected abstract MessageType internalGetResult();

    /** Check if a singular extension is present. */
    public final boolean hasExtension(
        final GeneratedExtension<MessageType, ?> extension) {
      return internalGetResult().hasExtension(extension);
    }

    /** Get the number of elements in a repeated extension. */
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      return internalGetResult().getExtensionCount(extension);
    }

    /** Get the value of an extension. */
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      return internalGetResult().getExtension(extension);
    }

    /** Get one element of a repeated extension. */
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index) {
      return internalGetResult().getExtension(extension, index);
    }

    /** Set the value of an extension. */
    public final <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, Type> extension,
        final Type value) {
      final ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.setField(extension.descriptor, value);
      return (BuilderType) this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index, final Type value) {
      final ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.setRepeatedField(extension.descriptor, index, value);
      return (BuilderType) this;
    }

    /** Append a value to a repeated extension. */
    public final <Type> BuilderType addExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final Type value) {
      final ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.addRepeatedField(extension.descriptor, value);
      return (BuilderType) this;
    }

    /** Clear an extension. */
    public final <Type> BuilderType clearExtension(
        final GeneratedExtension<MessageType, ?> extension) {
      final ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.clearField(extension.descriptor);
      return (BuilderType) this;
    }

    /**
     * Called by subclasses to parse an unknown field or an extension.
     * @return {@code true} unless the tag is an end-group tag.
     */
    @Override
    protected boolean parseUnknownField(
        final CodedInputStream input,
        final ExtensionRegistryLite extensionRegistry,
        final int tag) throws IOException {
      final FieldSet<ExtensionDescriptor> extensions =
          internalGetResult().extensions;

      final int wireType = WireFormat.getTagWireType(tag);
      final int fieldNumber = WireFormat.getTagFieldNumber(tag);

      final GeneratedExtension<MessageType, ?> extension =
        extensionRegistry.findLiteExtensionByNumber(
            getDefaultInstanceForType(), fieldNumber);

      if (extension == null || wireType !=
            FieldSet.getWireFormatForFieldType(
                extension.descriptor.getLiteType(),
                extension.descriptor.isPacked())) {
        // Unknown field or wrong wire type.  Skip.
        return input.skipField(tag);
      }

      if (extension.descriptor.isPacked()) {
        final int length = input.readRawVarint32();
        final int limit = input.pushLimit(length);
        if (extension.descriptor.getLiteType() == WireFormat.FieldType.ENUM) {
          while (input.getBytesUntilLimit() > 0) {
            final int rawValue = input.readEnum();
            final Object value =
                extension.descriptor.getEnumType().findValueByNumber(rawValue);
            if (value == null) {
              // If the number isn't recognized as a valid value for this
              // enum, drop it (don't even add it to unknownFields).
              return true;
            }
            extensions.addRepeatedField(extension.descriptor, value);
          }
        } else {
          while (input.getBytesUntilLimit() > 0) {
            final Object value =
              FieldSet.readPrimitiveField(input,
                                          extension.descriptor.getLiteType());
            extensions.addRepeatedField(extension.descriptor, value);
          }
        }
        input.popLimit(limit);
      } else {
        final Object value;
        switch (extension.descriptor.getLiteJavaType()) {
          case MESSAGE: {
            MessageLite.Builder subBuilder = null;
            if (!extension.descriptor.isRepeated()) {
              MessageLite existingValue =
                  (MessageLite) extensions.getField(extension.descriptor);
              if (existingValue != null) {
                subBuilder = existingValue.toBuilder();
              }
            }
            if (subBuilder == null) {
              subBuilder = extension.messageDefaultInstance.newBuilderForType();
            }
            if (extension.descriptor.getLiteType() ==
                WireFormat.FieldType.GROUP) {
              input.readGroup(extension.getNumber(),
                              subBuilder, extensionRegistry);
            } else {
              input.readMessage(subBuilder, extensionRegistry);
            }
            value = subBuilder.build();
            break;
          }
          case ENUM:
            final int rawValue = input.readEnum();
            value = extension.descriptor.getEnumType()
                             .findValueByNumber(rawValue);
            // If the number isn't recognized as a valid value for this enum,
            // drop it.
            if (value == null) {
              return true;
            }
            break;
          default:
            value = FieldSet.readPrimitiveField(input,
                extension.descriptor.getLiteType());
            break;
        }

        if (extension.descriptor.isRepeated()) {
          extensions.addRepeatedField(extension.descriptor, value);
        } else {
          extensions.setField(extension.descriptor, value);
        }
      }

      return true;
    }

    protected final void mergeExtensionFields(final MessageType other) {
      internalGetResult().extensions.mergeFrom(other.extensions);
    }
  }

  // -----------------------------------------------------------------

  /** For use by generated code only. */
  public static <ContainingType extends MessageLite, Type>
      GeneratedExtension<ContainingType, Type>
      newGeneratedExtension(
        final ContainingType containingTypeDefaultInstance,
        final Type defaultValue,
        final MessageLite messageDefaultInstance,
        final Internal.EnumLiteMap<?> enumTypeMap,
        final int number,
        final WireFormat.FieldType type) {
    return new GeneratedExtension<ContainingType, Type>(
      containingTypeDefaultInstance, defaultValue, messageDefaultInstance,
      new ExtensionDescriptor(enumTypeMap, number, type,
        false /* isRepeated */, false /* isPacked */));
  }

  /** For use by generated code only. */
  public static <ContainingType extends MessageLite, Type>
      GeneratedExtension<ContainingType, List<Type>>
      newRepeatedGeneratedExtension(
        final ContainingType containingTypeDefaultInstance,
        final MessageLite messageDefaultInstance,
        final Internal.EnumLiteMap<?> enumTypeMap,
        final int number,
        final WireFormat.FieldType type,
        final boolean isPacked) {
    return new GeneratedExtension<ContainingType, List<Type>>(
      containingTypeDefaultInstance, Collections.<Type>emptyList(),
      messageDefaultInstance,
      new ExtensionDescriptor(
        enumTypeMap, number, type, true /* isRepeated */, isPacked));
  }

  private static final class ExtensionDescriptor
      implements FieldSet.FieldDescriptorLite<
        ExtensionDescriptor> {
    private ExtensionDescriptor(
        final Internal.EnumLiteMap<?> enumTypeMap,
        final int number,
        final WireFormat.FieldType type,
        final boolean isRepeated,
        final boolean isPacked) {
      this.enumTypeMap = enumTypeMap;
      this.number = number;
      this.type = type;
      this.isRepeated = isRepeated;
      this.isPacked = isPacked;
    }

    private final Internal.EnumLiteMap<?> enumTypeMap;
    private final int number;
    private final WireFormat.FieldType type;
    private final boolean isRepeated;
    private final boolean isPacked;

    public int getNumber() {
      return number;
    }

    public WireFormat.FieldType getLiteType() {
      return type;
    }

    public WireFormat.JavaType getLiteJavaType() {
      return type.getJavaType();
    }

    public boolean isRepeated() {
      return isRepeated;
    }

    public boolean isPacked() {
      return isPacked;
    }

    public Internal.EnumLiteMap<?> getEnumType() {
      return enumTypeMap;
    }

    @SuppressWarnings("unchecked")
    public MessageLite.Builder internalMergeFrom(
        MessageLite.Builder to, MessageLite from) {
      return ((Builder) to).mergeFrom((GeneratedMessageLite) from);
    }

    public int compareTo(ExtensionDescriptor other) {
      return number - other.number;
    }
  }

  /**
   * Lite equivalent to {@link GeneratedMessage.GeneratedExtension}.
   *
   * Users should ignore the contents of this class and only use objects of
   * this type as parameters to extension accessors and ExtensionRegistry.add().
   */
  public static final class GeneratedExtension<
      ContainingType extends MessageLite, Type> {
    private GeneratedExtension(
        final ContainingType containingTypeDefaultInstance,
        final Type defaultValue,
        final MessageLite messageDefaultInstance,
        final ExtensionDescriptor descriptor) {
      this.containingTypeDefaultInstance = containingTypeDefaultInstance;
      this.defaultValue = defaultValue;
      this.messageDefaultInstance = messageDefaultInstance;
      this.descriptor = descriptor;
    }

    private final ContainingType containingTypeDefaultInstance;
    private final Type defaultValue;
    private final MessageLite messageDefaultInstance;
    private final ExtensionDescriptor descriptor;

    /**
     * Default instance of the type being extended, used to identify that type.
     */
    public ContainingType getContainingTypeDefaultInstance() {
      return containingTypeDefaultInstance;
    }

    /** Get the field number. */
    public int getNumber() {
      return descriptor.getNumber();
    }

    /**
     * If the extension is an embedded message, this is the default instance of
     * that type.
     */
    public MessageLite getMessageDefaultInstance() {
      return messageDefaultInstance;
    }
  }
}
