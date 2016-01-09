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

import com.google.protobuf.Internal.BooleanList;
import com.google.protobuf.Internal.DoubleList;
import com.google.protobuf.Internal.FloatList;
import com.google.protobuf.Internal.IntList;
import com.google.protobuf.Internal.LongList;
import com.google.protobuf.Internal.ProtobufList;
import com.google.protobuf.WireFormat.FieldType;

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
public abstract class GeneratedMessageLite<
    MessageType extends GeneratedMessageLite<MessageType, BuilderType>,
    BuilderType extends GeneratedMessageLite.Builder<MessageType, BuilderType>> 
        extends AbstractMessageLite
        implements Serializable {

  private static final long serialVersionUID = 1L;

  /** For use by generated code only. Lazily initialized to reduce allocations. */
  protected UnknownFieldSetLite unknownFields = null;
  
  /** For use by generated code only.  */
  protected int memoizedSerializedSize = -1;
  
  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final Parser<MessageType> getParserForType() {
    return (Parser<MessageType>) dynamicMethod(MethodToInvoke.GET_PARSER);
  }

  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final MessageType getDefaultInstanceForType() {
    return (MessageType) dynamicMethod(MethodToInvoke.GET_DEFAULT_INSTANCE);
  }

  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final BuilderType newBuilderForType() {
    return (BuilderType) dynamicMethod(MethodToInvoke.NEW_BUILDER);
  }

  // The general strategy for unknown fields is to use an UnknownFieldSetLite that is treated as
  // mutable during the parsing constructor and immutable after. This allows us to avoid
  // any unnecessary intermediary allocations while reducing the generated code size.

  /**
   * Lazily initializes unknown fields.
   */
  private final void ensureUnknownFieldsInitialized() {
    if (unknownFields == null) {
      unknownFields = UnknownFieldSetLite.newInstance();
    }
  }
  
  /**
   * Called by subclasses to parse an unknown field. For use by generated code only.
   * 
   * @return {@code true} unless the tag is an end-group tag.
   */
  protected boolean parseUnknownField(int tag, CodedInputStream input) throws IOException {
    // This will avoid the allocation of unknown fields when a group tag is encountered.
    if (WireFormat.getTagWireType(tag) == WireFormat.WIRETYPE_END_GROUP) {
      return false;
    }
    
    ensureUnknownFieldsInitialized();
    return unknownFields.mergeFieldFrom(tag, input);
  }

  /**
   * Called by subclasses to parse an unknown field. For use by generated code only.
   */
  protected void mergeVarintField(int tag, int value) {
    ensureUnknownFieldsInitialized();
    unknownFields.mergeVarintField(tag, value);
  }
  
  /**
   * Called by subclasses to parse an unknown field. For use by generated code only.
   */
  protected void mergeLengthDelimitedField(int fieldNumber, ByteString value) {
    ensureUnknownFieldsInitialized();
    unknownFields.mergeLengthDelimitedField(fieldNumber, value);
  }
  
  /**
   * Called by subclasses to complete parsing. For use by generated code only.
   */
  protected void doneParsing() {
    if (unknownFields == null) {
      unknownFields = UnknownFieldSetLite.getDefaultInstance();
    } else {
      unknownFields.makeImmutable();
    }
  }

  public final boolean isInitialized() {
    return dynamicMethod(MethodToInvoke.IS_INITIALIZED, Boolean.TRUE) != null;
  }

  public final BuilderType toBuilder() {
    BuilderType builder = (BuilderType) dynamicMethod(MethodToInvoke.NEW_BUILDER);
    builder.mergeFrom((MessageType) this);
    return builder;
  }

  /**
   * Defines which method path to invoke in {@link GeneratedMessageLite
   * #dynamicMethod(MethodToInvoke, Object...)}.
   * <p>
   * For use by generated code only.
   */
  public static enum MethodToInvoke {
    IS_INITIALIZED,
    PARSE_PARTIAL_FROM,
    MERGE_FROM,
    MAKE_IMMUTABLE,
    NEW_INSTANCE,
    NEW_BUILDER,
    GET_DEFAULT_INSTANCE,
    GET_PARSER;
  }

  /**
   * A method that implements different types of operations described in {@link MethodToInvoke}.
   * Theses different kinds of operations are required to implement message-level operations for
   * builders in the runtime. This method bundles those operations to reduce the generated methods
   * count.
   * <ul>
   * <li>{@code PARSE_PARTIAL_FROM} is parameterized with an {@link CodedInputStream} and
   * {@link ExtensionRegistryLite}. It consumes the input stream, parsing the contents into the
   * returned protocol buffer. If parsing throws an {@link InvalidProtocolBufferException}, the
   * implementation wraps it in a RuntimeException
   * <li>{@code NEW_INSTANCE} returns a new instance of the protocol buffer
   * <li>{@code IS_INITIALIZED} is parameterized with a {@code Boolean} detailing whether to
   * memoize. It returns {@code null} for false and the default instance for true. We optionally
   * memoize to support the Builder case, where memoization is not desired.
   * <li>{@code NEW_BUILDER} returns a {@code BuilderType} instance.
   * <li>{@code MERGE_FROM} is parameterized with a {@code MessageType} and merges the fields from
   * that instance into this instance.
   * <li>{@code MAKE_IMMUTABLE} sets all internal fields to an immutable state.
   * </ul>
   * This method, plus the implementation of the Builder, enables the Builder class to be proguarded
   * away entirely on Android.
   * <p>
   * For use by generated code only.
   */
  protected abstract Object dynamicMethod(MethodToInvoke method, Object arg0, Object arg1);

  /**
   * Same as {@link #dynamicMethod(MethodToInvoke, Object, Object)} with {@code null} padding.
   */
  protected Object dynamicMethod(MethodToInvoke method, Object arg0) {
    return dynamicMethod(method, arg0, null);
  }

  /**
   * Same as {@link #dynamicMethod(MethodToInvoke, Object, Object)} with {@code null} padding.
   */
  protected Object dynamicMethod(MethodToInvoke method) {
    return dynamicMethod(method, null, null);
  }

  /**
   * Merge some unknown fields into the {@link UnknownFieldSetLite} for this
   * message.
   *
   * <p>For use by generated code only.
   */
  protected final void mergeUnknownFields(UnknownFieldSetLite unknownFields) {
    this.unknownFields = UnknownFieldSetLite.mutableCopyOf(this.unknownFields, unknownFields);
  }

  @SuppressWarnings("unchecked")
  public abstract static class Builder<
      MessageType extends GeneratedMessageLite<MessageType, BuilderType>,
      BuilderType extends Builder<MessageType, BuilderType>>
          extends AbstractMessageLite.Builder<BuilderType> {

    private final MessageType defaultInstance;
    protected MessageType instance;
    protected boolean isBuilt;

    protected Builder(MessageType defaultInstance) {
      this.defaultInstance = defaultInstance;
      this.instance = (MessageType) defaultInstance.dynamicMethod(MethodToInvoke.NEW_INSTANCE);
      isBuilt = false;
    }

    /**
     * Called before any method that would mutate the builder to ensure that it correctly copies
     * any state before the write happens to preserve immutability guarantees.
     */
    protected void copyOnWrite() {
      if (isBuilt) {
        MessageType newInstance = (MessageType) instance.dynamicMethod(MethodToInvoke.NEW_INSTANCE);
        newInstance.dynamicMethod(MethodToInvoke.MERGE_FROM, instance);
        instance = newInstance;
        isBuilt = false;
      }
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final boolean isInitialized() {
      return GeneratedMessageLite.isInitialized(instance, false /* shouldMemoize */);
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final BuilderType clear() {
      // No need to copy on write since we're dropping the instance anyways.
      instance = (MessageType) instance.dynamicMethod(MethodToInvoke.NEW_INSTANCE);
      return (BuilderType) this;
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public BuilderType clone() {
      BuilderType builder =
          (BuilderType) getDefaultInstanceForType().newBuilderForType();
      builder.mergeFrom(buildPartial());
      return builder;
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public MessageType buildPartial() {
      if (isBuilt) {
        return instance;
      }
      
      instance.dynamicMethod(MethodToInvoke.MAKE_IMMUTABLE);
      instance.unknownFields.makeImmutable();
      
      isBuilt = true;
      return instance;
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final MessageType build() {
      MessageType result = buildPartial();
      if (!result.isInitialized()) {
        throw newUninitializedMessageException(result);
      }
      return result;
    }
    
    /** All subclasses implement this. */
    public BuilderType mergeFrom(MessageType message) {
      copyOnWrite();
      instance.dynamicMethod(MethodToInvoke.MERGE_FROM, message);
      return (BuilderType) this;
    }
    
    public MessageType getDefaultInstanceForType() {
      return defaultInstance;
    }
    
    public BuilderType mergeFrom(
        com.google.protobuf.CodedInputStream input,
        com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws java.io.IOException {
      MessageType parsedMessage = null;
      try {
        parsedMessage =
            (MessageType) getDefaultInstanceForType().getParserForType().parsePartialFrom(
                input, extensionRegistry);
      } catch (com.google.protobuf.InvalidProtocolBufferException e) {
        parsedMessage = (MessageType) e.getUnfinishedMessage();
        throw e;
      } finally {
        if (parsedMessage != null) {
          mergeFrom(parsedMessage);
        }
      }
      return (BuilderType) this;
    }
  }


  // =================================================================
  // Extensions-related stuff

  /**
   * Lite equivalent of {@link com.google.protobuf.GeneratedMessage.ExtendableMessageOrBuilder}.
   */
  public interface ExtendableMessageOrBuilder<
      MessageType extends ExtendableMessage<MessageType, BuilderType>,
      BuilderType extends ExtendableBuilder<MessageType, BuilderType>>
          extends MessageLiteOrBuilder {

    /** Check if a singular extension is present. */
    <Type> boolean hasExtension(
        ExtensionLite<MessageType, Type> extension);

    /** Get the number of elements in a repeated extension. */
    <Type> int getExtensionCount(
        ExtensionLite<MessageType, List<Type>> extension);

    /** Get the value of an extension. */
    <Type> Type getExtension(ExtensionLite<MessageType, Type> extension);

    /** Get one element of a repeated extension. */
    <Type> Type getExtension(
        ExtensionLite<MessageType, List<Type>> extension,
        int index);
  }

  /**
   * Lite equivalent of {@link GeneratedMessage.ExtendableMessage}.
   */
  public abstract static class ExtendableMessage<
        MessageType extends ExtendableMessage<MessageType, BuilderType>,
        BuilderType extends ExtendableBuilder<MessageType, BuilderType>>
            extends GeneratedMessageLite<MessageType, BuilderType>
            implements ExtendableMessageOrBuilder<MessageType, BuilderType> {

    /**
     * Represents the set of extensions on this message. For use by generated
     * code only.
     */
    protected FieldSet<ExtensionDescriptor> extensions = FieldSet.newFieldSet();

    protected final void mergeExtensionFields(final MessageType other) {
      if (extensions.isImmutable()) {
        extensions = extensions.clone();
      }
      extensions.mergeFrom(((ExtendableMessage) other).extensions);
    }
    
    /**
     * Parse an unknown field or an extension. For use by generated code only.
     * 
     * <p>For use by generated code only.
     * 
     * @return {@code true} unless the tag is an end-group tag.
     */
    protected <MessageType extends MessageLite> boolean parseUnknownField(
        MessageType defaultInstance,
        CodedInputStream input,
        ExtensionRegistryLite extensionRegistry,
        int tag) throws IOException {
      int wireType = WireFormat.getTagWireType(tag);
      int fieldNumber = WireFormat.getTagFieldNumber(tag);

      // TODO(dweis): How much bytecode would be saved by not requiring the generated code to
      //     provide the default instance?
      GeneratedExtension<MessageType, ?> extension = extensionRegistry.findLiteExtensionByNumber(
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
        return parseUnknownField(tag, input);
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
              mergeVarintField(fieldNumber, rawValue);
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
        final ExtensionLite<MessageType, Type> extension) {
      GeneratedExtension<MessageType, Type> extensionLite =
          checkIsLite(extension);
      
      verifyExtensionContainingType(extensionLite);
      return extensions.hasField(extensionLite.descriptor);
    }

    /** Get the number of elements in a repeated extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> int getExtensionCount(
        final ExtensionLite<MessageType, List<Type>> extension) {
      GeneratedExtension<MessageType, List<Type>> extensionLite =
          checkIsLite(extension);
      
      verifyExtensionContainingType(extensionLite);
      return extensions.getRepeatedFieldCount(extensionLite.descriptor);
    }

    /** Get the value of an extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, Type> extension) {
      GeneratedExtension<MessageType, Type> extensionLite =
          checkIsLite(extension);
      
      verifyExtensionContainingType(extensionLite);
      final Object value = extensions.getField(extensionLite.descriptor);
      if (value == null) {
        return extensionLite.defaultValue;
      } else {
        return (Type) extensionLite.fromFieldSetType(value);
      }
    }

    /** Get one element of a repeated extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, List<Type>> extension,
        final int index) {
      GeneratedExtension<MessageType, List<Type>> extensionLite =
          checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      return (Type) extensionLite.singularFromFieldSetType(
          extensions.getRepeatedField(extensionLite.descriptor, index));
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions.isInitialized();
    }


    @Override
    protected final void doneParsing() {
      super.doneParsing();
      
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
        MessageType extends ExtendableMessage<MessageType, BuilderType>,
        BuilderType extends ExtendableBuilder<MessageType, BuilderType>>
      extends Builder<MessageType, BuilderType>
      implements ExtendableMessageOrBuilder<MessageType, BuilderType> {
    protected ExtendableBuilder(MessageType defaultInstance) {
      super(defaultInstance);
      
      // TODO(dweis): This is kind of an unnecessary clone since we construct a
      //     new instance in the parent constructor which makes the extensions
      //     immutable. This extra allocation shouldn't matter in practice
      //     though.
      instance.extensions = instance.extensions.clone();
    }

    // For immutable message conversion.
    void internalSetExtensionSet(FieldSet<ExtensionDescriptor> extensions) {
      copyOnWrite();
      instance.extensions = extensions;
    }

    // @Override (Java 1.6 override semantics, but we must support 1.5)
    protected void copyOnWrite() {
      if (!isBuilt) {
        return;
      }
      
      super.copyOnWrite();
      instance.extensions = instance.extensions.clone();
    }

    // @Override (Java 1.6 override semantics, but we must support 1.5)
    public final MessageType buildPartial() {
      if (isBuilt) {
        return instance;
      }

      instance.extensions.makeImmutable();
      return super.buildPartial();
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
        final ExtensionLite<MessageType, Type> extension) {
      return instance.hasExtension(extension);
    }

    /** Get the number of elements in a repeated extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> int getExtensionCount(
        final ExtensionLite<MessageType, List<Type>> extension) {
      return instance.getExtensionCount(extension);
    }

    /** Get the value of an extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, Type> extension) {
      return instance.getExtension(extension);
    }

    /** Get one element of a repeated extension. */
    @SuppressWarnings("unchecked")
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, List<Type>> extension,
        final int index) {
      return instance.getExtension(extension, index);
    }

    // This is implemented here only to work around an apparent bug in the
    // Java compiler and/or build system.  See bug #1898463.  The mere presence
    // of this dummy clone() implementation makes it go away.
    @Override
    public BuilderType clone() {
      return super.clone();
    }
    
    /** Set the value of an extension. */
    public final <Type> BuilderType setExtension(
        final ExtensionLite<MessageType, Type> extension,
        final Type value) {
      GeneratedExtension<MessageType, Type> extensionLite =
          checkIsLite(extension);
      
      verifyExtensionContainingType(extensionLite);
      copyOnWrite();
      instance.extensions.setField(extensionLite.descriptor, extensionLite.toFieldSetType(value));
      return (BuilderType) this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        final ExtensionLite<MessageType, List<Type>> extension,
        final int index, final Type value) {
      GeneratedExtension<MessageType, List<Type>> extensionLite =
          checkIsLite(extension);
      
      verifyExtensionContainingType(extensionLite);
      copyOnWrite();
      instance.extensions.setRepeatedField(
          extensionLite.descriptor, index, extensionLite.singularToFieldSetType(value));
      return (BuilderType) this;
    }

    /** Append a value to a repeated extension. */
    public final <Type> BuilderType addExtension(
        final ExtensionLite<MessageType, List<Type>> extension,
        final Type value) {
      GeneratedExtension<MessageType, List<Type>> extensionLite =
          checkIsLite(extension);
      
      verifyExtensionContainingType(extensionLite);
      copyOnWrite();
      instance.extensions.addRepeatedField(
          extensionLite.descriptor, extensionLite.singularToFieldSetType(value));
      return (BuilderType) this;
    }

    /** Clear an extension. */
    public final <Type> BuilderType clearExtension(
        final ExtensionLite<MessageType, ?> extension) {
      GeneratedExtension<MessageType, ?> extensionLite = checkIsLite(extension);
      
      verifyExtensionContainingType(extensionLite);
      copyOnWrite();
      instance.extensions.clearField(extensionLite.descriptor);
      return (BuilderType) this;
    }
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
      ContainingType extends MessageLite, Type>
          extends ExtensionLite<ContainingType, Type> {

    /**
     * Create a new instance with the given parameters.
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
        final Class singularType) {
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
    }

    final ContainingType containingTypeDefaultInstance;
    final Type defaultValue;
    final MessageLite messageDefaultInstance;
    final ExtensionDescriptor descriptor;

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
        return descriptor.enumTypeMap.findValueByNumber((Integer) value);
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

    public FieldType getLiteType() {
      return descriptor.getLiteType();
    }

    public boolean isRepeated() {
      return descriptor.isRepeated;
    }

    public Type getDefaultValue() {
      return defaultValue;
    }
  }

  /**
   * A serialized (serializable) form of the generated message.  Stores the
   * message as a class name and a byte array.
   */
  static final class SerializedForm implements Serializable {
    private static final long serialVersionUID = 0L;

    private final String messageClassName;
    private final byte[] asBytes;

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
        Class<?> messageClass = Class.forName(messageClassName);
        java.lang.reflect.Field defaultInstanceField =
            messageClass.getDeclaredField("DEFAULT_INSTANCE");
        defaultInstanceField.setAccessible(true);
        MessageLite defaultInstance = (MessageLite) defaultInstanceField.get(null);
        return defaultInstance.newBuilderForType()
            .mergeFrom(asBytes)
            .buildPartial();
      } catch (ClassNotFoundException e) {
        throw new RuntimeException("Unable to find proto buffer class: " + messageClassName, e);
      } catch (NoSuchFieldException e) {
        throw new RuntimeException("Unable to find DEFAULT_INSTANCE in " + messageClassName, e);
      } catch (SecurityException e) {
        throw new RuntimeException("Unable to call DEFAULT_INSTANCE in " + messageClassName, e);
      } catch (IllegalAccessException e) {
        throw new RuntimeException("Unable to call parsePartialFrom", e);
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
  
  /**
   * Checks that the {@link Extension} is Lite and returns it as a
   * {@link GeneratedExtension}.
   */
  private static <
      MessageType extends ExtendableMessage<MessageType, BuilderType>,
      BuilderType extends ExtendableBuilder<MessageType, BuilderType>,
      T>
    GeneratedExtension<MessageType, T> checkIsLite(
        ExtensionLite<MessageType, T> extension) {
    if (!extension.isLite()) {
      throw new IllegalArgumentException("Expected a lite extension.");
    }
    
    return (GeneratedExtension<MessageType, T>) extension;
  }

  /**
   * A static helper method for checking if a message is initialized, optionally memoizing.
   * <p>
   * For use by generated code only.
   */
  protected static final <T extends GeneratedMessageLite<T, ?>> boolean isInitialized(
      T message, boolean shouldMemoize) {
    return message.dynamicMethod(MethodToInvoke.IS_INITIALIZED, shouldMemoize) != null;
  }
  
  protected static final <T extends GeneratedMessageLite<T, ?>> void makeImmutable(T message) {
    message.dynamicMethod(MethodToInvoke.MAKE_IMMUTABLE);
  }
  
  /**
   * A static helper method for parsing a partial from input using the extension registry and the
   * instance.
   */
  static <T extends GeneratedMessageLite<T, ?>> T parsePartialFrom(
      T instance, CodedInputStream input, ExtensionRegistryLite extensionRegistry)
          throws InvalidProtocolBufferException {
    try {
      return (T) instance.dynamicMethod(
          MethodToInvoke.PARSE_PARTIAL_FROM, input, extensionRegistry);
    } catch (RuntimeException e) {
      if (e.getCause() instanceof InvalidProtocolBufferException) {
        throw (InvalidProtocolBufferException) e.getCause();
      }
      throw e;
    }
  }
  
  /**
   * A {@link Parser} implementation that delegates to the default instance.
   * <p>
   * For use by generated code only.
   */
  protected static class DefaultInstanceBasedParser<T extends GeneratedMessageLite<T, ?>>
      extends AbstractParser<T> {
    
    private T defaultInstance;
    
    public DefaultInstanceBasedParser(T defaultInstance) {
      this.defaultInstance = defaultInstance;
    }
    
    @Override
    public T parsePartialFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return GeneratedMessageLite.parsePartialFrom(defaultInstance, input, extensionRegistry);
    }
  }
  
  protected static IntList newIntList() {
    return new IntArrayList();
  }
  
  protected static IntList newIntListWithCapacity(int capacity) {
    return new IntArrayList(capacity);
  }
  
  protected static IntList newIntList(List<Integer> toCopy) {
    return new IntArrayList(toCopy);
  }
  
  protected static IntList emptyIntList() {
    return IntArrayList.emptyList();
  }

  protected static LongList newLongList() {
    return new LongArrayList();
  }

  protected static LongList newLongListWithCapacity(int capacity) {
    return new LongArrayList(capacity);
  }
  
  protected static LongList newLongList(List<Long> toCopy) {
    return new LongArrayList(toCopy);
  }
  
  protected static LongList emptyLongList() {
    return LongArrayList.emptyList();
  }
  
  protected static FloatList newFloatList() {
    return new FloatArrayList();
  }
  
  protected static FloatList newFloatListWithCapacity(int capacity) {
    return new FloatArrayList(capacity);
  }
  
  protected static FloatList newFloatList(List<Float> toCopy) {
    return new FloatArrayList(toCopy);
  }
  
  protected static FloatList emptyFloatList() {
    return FloatArrayList.emptyList();
  }
  
  protected static DoubleList newDoubleList() {
    return new DoubleArrayList();
  }
  
  protected static DoubleList newDoubleListWithCapacity(int capacity) {
    return new DoubleArrayList(capacity);
  }
  
  protected static DoubleList newDoubleList(List<Double> toCopy) {
    return new DoubleArrayList(toCopy);
  }
  
  protected static DoubleList emptyDoubleList() {
    return DoubleArrayList.emptyList();
  }
  
  protected static BooleanList newBooleanList() {
    return new BooleanArrayList();
  }
  
  protected static BooleanList newBooleanListWithCapacity(int capacity) {
    return new BooleanArrayList(capacity);
  }
  
  protected static BooleanList newBooleanList(List<Boolean> toCopy) {
    return new BooleanArrayList(toCopy);
  }
  
  protected static BooleanList emptyBooleanList() {
    return BooleanArrayList.emptyList();
  }
  
  protected static <E> ProtobufList<E> newProtobufList() {
    return new ProtobufArrayList<E>();
  }
  
  protected static <E> ProtobufList<E> newProtobufList(List<E> toCopy) {
    return new ProtobufArrayList<E>(toCopy);
  }
  
  protected static <E> ProtobufList<E> newProtobufListWithCapacity(int capacity) {
    return new ProtobufArrayList<E>(capacity);
  }
  
  protected static <E> ProtobufList<E> emptyProtobufList() {
    return ProtobufArrayList.emptyList();
  }
  
  protected static LazyStringArrayList emptyLazyStringArrayList() {
    return LazyStringArrayList.emptyList();
  }
}
