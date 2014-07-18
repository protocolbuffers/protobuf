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
import java.io.ObjectStreamException;
import java.io.Serializable;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Lite version of {@link GeneratedMessage}.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class GeneratedMessageLite extends AbstractMessageLite
    implements Serializable {
  private static final long serialVersionUID = 1L;

  protected GeneratedMessageLite() {
  }

  protected GeneratedMessageLite(Builder builder) {
  }

  public Parser<? extends MessageLite> getParserForType() {
    throw new UnsupportedOperationException(
        "This is supposed to be overridden by subclasses.");
  }

  /**
   * Called by subclasses to parse an unknown field.
   * @return {@code true} unless the tag is an end-group tag.
   */
  protected boolean parseUnknownField(
      CodedInputStream input,
      CodedOutputStream unknownFieldsCodedOutput,
      ExtensionRegistryLite extensionRegistry,
      int tag) throws IOException {
    return input.skipField(tag, unknownFieldsCodedOutput);
  }

  /**
   * Used by parsing constructors in generated classes.
   */
  protected void makeExtensionsImmutable() {
    // Noop for messages without extensions.
  }

  @SuppressWarnings("unchecked")
  public abstract static class Builder<MessageType extends GeneratedMessageLite,
                                       BuilderType extends Builder>
      extends AbstractMessageLite.Builder<BuilderType> {
    protected Builder() {}

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public BuilderType clear() {
      unknownFields = ByteString.EMPTY;
      return (BuilderType) this;
    }

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
     * Called by subclasses to parse an unknown field.
     * @return {@code true} unless the tag is an end-group tag.
     */
    protected boolean parseUnknownField(
        CodedInputStream input,
        CodedOutputStream unknownFieldsCodedOutput,
        ExtensionRegistryLite extensionRegistry,
        int tag) throws IOException {
      return input.skipField(tag, unknownFieldsCodedOutput);
    }

    public final ByteString getUnknownFields() {
      return unknownFields;
    }

    public final BuilderType setUnknownFields(final ByteString unknownFields) {
      this.unknownFields = unknownFields;
      return (BuilderType) this;
    }

    private ByteString unknownFields = ByteString.EMPTY;
  }


  // =================================================================
  // Extensions-related stuff

  /**
   * Lite equivalent of {@link com.google.protobuf.GeneratedMessage.ExtendableMessageOrBuilder}.
   */
  public interface ExtendableMessageOrBuilder<
      MessageType extends ExtendableMessage> extends MessageLiteOrBuilder {

    /** Check if a singular extension is present. */
    <Type> boolean hasExtension(
        GeneratedExtension<MessageType, Type> extension);

    /** Get the number of elements in a repeated extension. */
    <Type> int getExtensionCount(
        GeneratedExtension<MessageType, List<Type>> extension);

    /** Get the value of an extension. */
    <Type> Type getExtension(GeneratedExtension<MessageType, Type> extension);

    /** Get one element of a repeated extension. */
    <Type> Type getExtension(
        GeneratedExtension<MessageType, List<Type>> extension,
        int index);
  }

  /**
   * Lite equivalent of {@link GeneratedMessage.ExtendableMessage}.
   */
  public abstract static class ExtendableMessage<
        MessageType extends ExtendableMessage<MessageType>>
      extends GeneratedMessageLite
      implements ExtendableMessageOrBuilder<MessageType> {

    private final FieldSet<ExtensionDescriptor> extensions;

    protected ExtendableMessage() {
      this.extensions = FieldSet.newFieldSet();
    }

    protected ExtendableMessage(ExtendableBuilder<MessageType, ?> builder) {
      this.extensions = builder.buildExtensions();
    }

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
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> boolean hasExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.descriptor);
    }

    /** Get the number of elements in a repeated extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      verifyExtensionContainingType(extension);
      return extensions.getRepeatedFieldCount(extension.descriptor);
    }

    /** Get the value of an extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      final Object value = extensions.getField(extension.descriptor);
      if (value == null) {
        return extension.defaultValue;
      } else {
        return (Type) extension.fromFieldSetType(value);
      }
    }

    /** Get one element of a repeated extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index) {
      verifyExtensionContainingType(extension);
      return (Type) extension.singularFromFieldSetType(
          extensions.getRepeatedField(extension.descriptor, index));
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions.isInitialized();
    }

    /**
     * Called by subclasses to parse an unknown field or an extension.
     * @return {@code true} unless the tag is an end-group tag.
     */
    @Override
    protected boolean parseUnknownField(
        CodedInputStream input,
        CodedOutputStream unknownFieldsCodedOutput,
        ExtensionRegistryLite extensionRegistry,
        int tag) throws IOException {
      return GeneratedMessageLite.parseUnknownField(
          extensions,
          getDefaultInstanceForType(),
          input,
          unknownFieldsCodedOutput,
          extensionRegistry,
          tag);
    }


    /**
     * Used by parsing constructors in generated classes.
     */
    @Override
    protected void makeExtensionsImmutable() {
      extensions.makeImmutable();
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
      extends Builder<MessageType, BuilderType>
      implements ExtendableMessageOrBuilder<MessageType> {
    protected ExtendableBuilder() {}

    private FieldSet<ExtensionDescriptor> extensions = FieldSet.emptySet();
    private boolean extensionsIsMutable;

    // For immutable message conversion.
    void internalSetExtensionSet(FieldSet<ExtensionDescriptor> extensions) {
      this.extensions = extensions;
    }

    @Override
    public BuilderType clear() {
      extensions.clear();
      extensionsIsMutable = false;
      return super.clear();
    }

    private void ensureExtensionsIsMutable() {
      if (!extensionsIsMutable) {
        extensions = extensions.clone();
        extensionsIsMutable = true;
      }
    }

    /**
     * Called by the build code path to create a copy of the extensions for
     * building the message.
     */
    private FieldSet<ExtensionDescriptor> buildExtensions() {
      extensions.makeImmutable();
      extensionsIsMutable = false;
      return extensions;
    }

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
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> boolean hasExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.descriptor);
    }

    /** Get the number of elements in a repeated extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      verifyExtensionContainingType(extension);
      return extensions.getRepeatedFieldCount(extension.descriptor);
    }

    /** Get the value of an extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      final Object value = extensions.getField(extension.descriptor);
      if (value == null) {
        return extension.defaultValue;
      } else {
        return (Type) extension.fromFieldSetType(value);
      }
    }

    /** Get one element of a repeated extension. */
    @SuppressWarnings("unchecked")
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index) {
      verifyExtensionContainingType(extension);
      return (Type) extension.singularFromFieldSetType(
          extensions.getRepeatedField(extension.descriptor, index));
    }

    // This is implemented here only to work around an apparent bug in the
    // Java compiler and/or build system.  See bug #1898463.  The mere presence
    // of this dummy clone() implementation makes it go away.
    @Override
    public BuilderType clone() {
      throw new UnsupportedOperationException(
          "This is supposed to be overridden by subclasses.");
    }

    /** Set the value of an extension. */
    public final <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, Type> extension,
        final Type value) {
      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      extensions.setField(extension.descriptor,
                          extension.toFieldSetType(value));
      return (BuilderType) this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index, final Type value) {
      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      extensions.setRepeatedField(extension.descriptor, index,
                                  extension.singularToFieldSetType(value));
      return (BuilderType) this;
    }

    /** Append a value to a repeated extension. */
    public final <Type> BuilderType addExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final Type value) {
      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      extensions.addRepeatedField(extension.descriptor,
                                  extension.singularToFieldSetType(value));
      return (BuilderType) this;
    }

    /** Clear an extension. */
    public final <Type> BuilderType clearExtension(
        final GeneratedExtension<MessageType, ?> extension) {
      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      extensions.clearField(extension.descriptor);
      return (BuilderType) this;
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions.isInitialized();
    }

    /**
     * Called by subclasses to parse an unknown field or an extension.
     * @return {@code true} unless the tag is an end-group tag.
     */
    @Override
    protected boolean parseUnknownField(
        CodedInputStream input,
        CodedOutputStream unknownFieldsCodedOutput,
        ExtensionRegistryLite extensionRegistry,
        int tag) throws IOException {
      ensureExtensionsIsMutable();
      return GeneratedMessageLite.parseUnknownField(
          extensions,
          getDefaultInstanceForType(),
          input,
          unknownFieldsCodedOutput,
          extensionRegistry,
          tag);
    }

    protected final void mergeExtensionFields(final MessageType other) {
      ensureExtensionsIsMutable();
      extensions.mergeFrom(((ExtendableMessage) other).extensions);
    }
  }

  // -----------------------------------------------------------------

  /**
   * Parse an unknown field or an extension.
   * @return {@code true} unless the tag is an end-group tag.
   */
  private static <MessageType extends MessageLite>
      boolean parseUnknownField(
          FieldSet<ExtensionDescriptor> extensions,
          MessageType defaultInstance,
          CodedInputStream input,
          CodedOutputStream unknownFieldsCodedOutput,
          ExtensionRegistryLite extensionRegistry,
          int tag) throws IOException {
    int wireType = WireFormat.getTagWireType(tag);
    int fieldNumber = WireFormat.getTagFieldNumber(tag);

    GeneratedExtension<MessageType, ?> extension =
      extensionRegistry.findLiteExtensionByNumber(
          defaultInstance, fieldNumber);

    boolean unknown = false;
    boolean packed = false;
    if (extension == null) {
      unknown = true;  // Unknown field.
    } else if (wireType == FieldSet.getWireFormatForFieldType(
                 extension.descriptor.getLiteType(),
                 false  /* isPacked */)) {
      packed = false;  // Normal, unpacked value.
    } else if (extension.descriptor.isRepeated &&
               extension.descriptor.type.isPackable() &&
               wireType == FieldSet.getWireFormatForFieldType(
                 extension.descriptor.getLiteType(),
                 true  /* isPacked */)) {
      packed = true;  // Packed value.
    } else {
      unknown = true;  // Wrong wire type.
    }

    if (unknown) {  // Unknown field or wrong wire type.  Skip.
      return input.skipField(tag, unknownFieldsCodedOutput);
    }

    if (packed) {
      int length = input.readRawVarint32();
      int limit = input.pushLimit(length);
      if (extension.descriptor.getLiteType() == WireFormat.FieldType.ENUM) {
        while (input.getBytesUntilLimit() > 0) {
          int rawValue = input.readEnum();
          Object value =
              extension.descriptor.getEnumType().findValueByNumber(rawValue);
          if (value == null) {
            // If the number isn't recognized as a valid value for this
            // enum, drop it (don't even add it to unknownFields).
            return true;
          }
          extensions.addRepeatedField(extension.descriptor,
                                      extension.singularToFieldSetType(value));
        }
      } else {
        while (input.getBytesUntilLimit() > 0) {
          Object value =
              FieldSet.readPrimitiveField(input,
                                          extension.descriptor.getLiteType(),
                                          /*checkUtf8=*/ false);
          extensions.addRepeatedField(extension.descriptor, value);
        }
      }
      input.popLimit(limit);
    } else {
      Object value;
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
            subBuilder = extension.getMessageDefaultInstance()
                .newBuilderForType();
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
          int rawValue = input.readEnum();
          value = extension.descriptor.getEnumType()
                           .findValueByNumber(rawValue);
          // If the number isn't recognized as a valid value for this enum,
          // write it to unknown fields object.
          if (value == null) {
            unknownFieldsCodedOutput.writeRawVarint32(tag);
            unknownFieldsCodedOutput.writeUInt32NoTag(rawValue);
            return true;
          }
          break;
        default:
          value = FieldSet.readPrimitiveField(input,
              extension.descriptor.getLiteType(),
              /*checkUtf8=*/ false);
          break;
      }

      if (extension.descriptor.isRepeated()) {
        extensions.addRepeatedField(extension.descriptor,
                                    extension.singularToFieldSetType(value));
      } else {
        extensions.setField(extension.descriptor,
                            extension.singularToFieldSetType(value));
      }
    }

    return true;
  }

  // -----------------------------------------------------------------

  /** For use by generated code only. */
  public static <ContainingType extends MessageLite, Type>
      GeneratedExtension<ContainingType, Type>
          newSingularGeneratedExtension(
              final ContainingType containingTypeDefaultInstance,
              final Type defaultValue,
              final MessageLite messageDefaultInstance,
              final Internal.EnumLiteMap<?> enumTypeMap,
              final int number,
              final WireFormat.FieldType type,
              final Class singularType) {
    return new GeneratedExtension<ContainingType, Type>(
        containingTypeDefaultInstance,
        defaultValue,
        messageDefaultInstance,
        new ExtensionDescriptor(enumTypeMap, number, type,
                                false /* isRepeated */,
                                false /* isPacked */),
        singularType);
  }

  /** For use by generated code only. */
  public static <ContainingType extends MessageLite, Type>
      GeneratedExtension<ContainingType, Type>
          newRepeatedGeneratedExtension(
              final ContainingType containingTypeDefaultInstance,
              final MessageLite messageDefaultInstance,
              final Internal.EnumLiteMap<?> enumTypeMap,
              final int number,
              final WireFormat.FieldType type,
              final boolean isPacked,
              final Class singularType) {
    @SuppressWarnings("unchecked")  // Subclasses ensure Type is a List
    Type emptyList = (Type) Collections.emptyList();
    return new GeneratedExtension<ContainingType, Type>(
        containingTypeDefaultInstance,
        emptyList,
        messageDefaultInstance,
        new ExtensionDescriptor(
            enumTypeMap, number, type, true /* isRepeated */, isPacked),
        singularType);
  }

  static final class ExtensionDescriptor
      implements FieldSet.FieldDescriptorLite<
        ExtensionDescriptor> {
    ExtensionDescriptor(
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

    final Internal.EnumLiteMap<?> enumTypeMap;
    final int number;
    final WireFormat.FieldType type;
    final boolean isRepeated;
    final boolean isPacked;

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

  // =================================================================

  /** Calls Class.getMethod and throws a RuntimeException if it fails. */
  @SuppressWarnings("unchecked")
  static Method getMethodOrDie(Class clazz, String name, Class... params) {
    try {
      return clazz.getMethod(name, params);
    } catch (NoSuchMethodException e) {
      throw new RuntimeException(
        "Generated message class \"" + clazz.getName() +
        "\" missing method \"" + name + "\".", e);
    }
  }

  /** Calls invoke and throws a RuntimeException if it fails. */
  static Object invokeOrDie(Method method, Object object, Object... params) {
    try {
      return method.invoke(object, params);
    } catch (IllegalAccessException e) {
      throw new RuntimeException(
        "Couldn't use Java reflection to implement protocol message " +
        "reflection.", e);
    } catch (InvocationTargetException e) {
      final Throwable cause = e.getCause();
      if (cause instanceof RuntimeException) {
        throw (RuntimeException) cause;
      } else if (cause instanceof Error) {
        throw (Error) cause;
      } else {
        throw new RuntimeException(
          "Unexpected exception thrown by generated accessor method.", cause);
      }
    }
  }

  /**
   * Lite equivalent to {@link GeneratedMessage.GeneratedExtension}.
   *
   * Users should ignore the contents of this class and only use objects of
   * this type as parameters to extension accessors and ExtensionRegistry.add().
   */
  public static class GeneratedExtension<
      ContainingType extends MessageLite, Type> {

    /**
     * Create a new isntance with the given parameters.
     *
     * The last parameter {@code singularType} is only needed for enum types.
     * We store integer values for enum types in a {@link ExtendableMessage}
     * and use Java reflection to convert an integer value back into a concrete
     * enum object.
     */
    GeneratedExtension(
        final ContainingType containingTypeDefaultInstance,
        final Type defaultValue,
        final MessageLite messageDefaultInstance,
        final ExtensionDescriptor descriptor,
        Class singularType) {
      // Defensive checks to verify the correct initialization order of
      // GeneratedExtensions and their related GeneratedMessages.
      if (containingTypeDefaultInstance == null) {
        throw new IllegalArgumentException(
            "Null containingTypeDefaultInstance");
      }
      if (descriptor.getLiteType() == WireFormat.FieldType.MESSAGE &&
          messageDefaultInstance == null) {
        throw new IllegalArgumentException(
            "Null messageDefaultInstance");
      }
      this.containingTypeDefaultInstance = containingTypeDefaultInstance;
      this.defaultValue = defaultValue;
      this.messageDefaultInstance = messageDefaultInstance;
      this.descriptor = descriptor;

      // Use Java reflection to invoke the static method {@code valueOf} of
      // enum types in order to convert integers to concrete enum objects.
      this.singularType = singularType;
      if (Internal.EnumLite.class.isAssignableFrom(singularType)) {
        this.enumValueOf = getMethodOrDie(
            singularType, "valueOf", int.class);
      } else {
        this.enumValueOf = null;
      }
    }

    final ContainingType containingTypeDefaultInstance;
    final Type defaultValue;
    final MessageLite messageDefaultInstance;
    final ExtensionDescriptor descriptor;
    final Class singularType;
    final Method enumValueOf;

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
     * If the extension is an embedded message or group, returns the default
     * instance of the message.
     */
    public MessageLite getMessageDefaultInstance() {
      return messageDefaultInstance;
    }

    @SuppressWarnings("unchecked")
    Object fromFieldSetType(final Object value) {
      if (descriptor.isRepeated()) {
        if (descriptor.getLiteJavaType() == WireFormat.JavaType.ENUM) {
          final List result = new ArrayList();
          for (final Object element : (List) value) {
            result.add(singularFromFieldSetType(element));
          }
          return result;
        } else {
          return value;
        }
      } else {
        return singularFromFieldSetType(value);
      }
    }

    Object singularFromFieldSetType(final Object value) {
      if (descriptor.getLiteJavaType() == WireFormat.JavaType.ENUM) {
        return invokeOrDie(enumValueOf, null, (Integer) value);
      } else {
        return value;
      }
    }

    @SuppressWarnings("unchecked")
    Object toFieldSetType(final Object value) {
      if (descriptor.isRepeated()) {
        if (descriptor.getLiteJavaType() == WireFormat.JavaType.ENUM) {
          final List result = new ArrayList();
          for (final Object element : (List) value) {
            result.add(singularToFieldSetType(element));
          }
          return result;
        } else {
          return value;
        }
      } else {
        return singularToFieldSetType(value);
      }
    }

    Object singularToFieldSetType(final Object value) {
      if (descriptor.getLiteJavaType() == WireFormat.JavaType.ENUM) {
        return ((Internal.EnumLite) value).getNumber();
      } else {
        return value;
      }
    }
  }

  /**
   * A serialized (serializable) form of the generated message.  Stores the
   * message as a class name and a byte array.
   */
  static final class SerializedForm implements Serializable {
    private static final long serialVersionUID = 0L;

    private String messageClassName;
    private byte[] asBytes;

    /**
     * Creates the serialized form by calling {@link com.google.protobuf.MessageLite#toByteArray}.
     * @param regularForm the message to serialize
     */
    SerializedForm(MessageLite regularForm) {
      messageClassName = regularForm.getClass().getName();
      asBytes = regularForm.toByteArray();
    }

    /**
     * When read from an ObjectInputStream, this method converts this object
     * back to the regular form.  Part of Java's serialization magic.
     * @return a GeneratedMessage of the type that was serialized
     */
    @SuppressWarnings("unchecked")
    protected Object readResolve() throws ObjectStreamException {
      try {
        Class messageClass = Class.forName(messageClassName);
        Method newBuilder = messageClass.getMethod("newBuilder");
        MessageLite.Builder builder =
            (MessageLite.Builder) newBuilder.invoke(null);
        builder.mergeFrom(asBytes);
        return builder.buildPartial();
      } catch (ClassNotFoundException e) {
        throw new RuntimeException("Unable to find proto buffer class", e);
      } catch (NoSuchMethodException e) {
        throw new RuntimeException("Unable to find newBuilder method", e);
      } catch (IllegalAccessException e) {
        throw new RuntimeException("Unable to call newBuilder method", e);
      } catch (InvocationTargetException e) {
        throw new RuntimeException("Error calling newBuilder", e.getCause());
      } catch (InvalidProtocolBufferException e) {
        throw new RuntimeException("Unable to understand proto buffer", e);
      }
    }
  }

  /**
   * Replaces this object in the output stream with a serialized form.
   * Part of Java's serialization magic.  Generated sub-classes must override
   * this method by calling {@code return super.writeReplace();}
   * @return a SerializedForm of this message
   */
  protected Object writeReplace() throws ObjectStreamException {
    return new SerializedForm(this);
  }
}
