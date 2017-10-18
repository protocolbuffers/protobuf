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

import com.google.protobuf.AbstractMessageLite.Builder.LimitedInputStream;
import com.google.protobuf.Internal.BooleanList;
import com.google.protobuf.Internal.DoubleList;
import com.google.protobuf.Internal.EnumLiteMap;
import com.google.protobuf.Internal.FloatList;
import com.google.protobuf.Internal.IntList;
import com.google.protobuf.Internal.LongList;
import com.google.protobuf.Internal.ProtobufList;
import com.google.protobuf.WireFormat.FieldType;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectStreamException;
import java.io.Serializable;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Lite version of {@link GeneratedMessage}.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class GeneratedMessageLite<
    MessageType extends GeneratedMessageLite<MessageType, BuilderType>,
    BuilderType extends GeneratedMessageLite.Builder<MessageType, BuilderType>>
        extends AbstractMessageLite<MessageType, BuilderType> {
  // BEGIN REGULAR
  static final boolean ENABLE_EXPERIMENTAL_RUNTIME_AT_BUILD_TIME = false;
  // END REGULAR
  // BEGIN EXPERIMENTAL
  // static final boolean ENABLE_EXPERIMENTAL_RUNTIME_AT_BUILD_TIME = true;
  // END EXPERIMENTAL

  /** For use by generated code only. Lazily initialized to reduce allocations. */
  protected UnknownFieldSetLite unknownFields = UnknownFieldSetLite.getDefaultInstance();

  /** For use by generated code only.  */
  protected int memoizedSerializedSize = -1;

  @Override
  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final Parser<MessageType> getParserForType() {
    return (Parser<MessageType>) dynamicMethod(MethodToInvoke.GET_PARSER);
  }

  @Override
  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final MessageType getDefaultInstanceForType() {
    return (MessageType) dynamicMethod(MethodToInvoke.GET_DEFAULT_INSTANCE);
  }

  @Override
  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final BuilderType newBuilderForType() {
    return (BuilderType) dynamicMethod(MethodToInvoke.NEW_BUILDER);
  }

  /**
   * A reflective toString function. This is primarily intended as a developer aid, while keeping
   * binary size down. The first line of the {@code toString()} representation includes a commented
   * version of {@code super.toString()} to act as an indicator that this should not be relied on
   * for comparisons.
   * <p>
   * NOTE: This method relies on the field getter methods not being stripped or renamed by proguard.
   * If they are, the fields will not be included in the returned string representation.
   * <p>
   * NOTE: This implementation is liable to change in the future, and should not be relied on in
   * code.
   */
  @Override
  public String toString() {
    return MessageLiteToString.toString(this, super.toString());
  }

  @SuppressWarnings("unchecked") // Guaranteed by runtime
  @Override
  public int hashCode() {
    if (memoizedHashCode != 0) {
      return memoizedHashCode;
    }
    // BEGIN EXPERIMENTAL
    // memoizedHashCode = Protobuf.getInstance().schemaFor(this).hashCode(this);
    // return memoizedHashCode;
    // END EXPERIMENTAL
    // BEGIN REGULAR
    HashCodeVisitor visitor = new HashCodeVisitor();
    visit(visitor, (MessageType) this);
    memoizedHashCode = visitor.hashCode;
    return memoizedHashCode;
    // END REGULAR
  }

  // BEGIN REGULAR
  @SuppressWarnings("unchecked") // Guaranteed by runtime
  int hashCode(HashCodeVisitor visitor) {
    if (memoizedHashCode == 0) {
      int inProgressHashCode = visitor.hashCode;
      visitor.hashCode = 0;
      visit(visitor, (MessageType) this);
      memoizedHashCode = visitor.hashCode;
      visitor.hashCode = inProgressHashCode;
    }
    return memoizedHashCode;
  }
  // END REGULAR

  @SuppressWarnings("unchecked") // Guaranteed by isInstance + runtime
  @Override
  public boolean equals(Object other) {
    if (this == other) {
      return true;
    }

    if (!getDefaultInstanceForType().getClass().isInstance(other)) {
      return false;
    }

    // BEGIN EXPERIMENTAL
    // return Protobuf.getInstance().schemaFor(this).equals(this, (MessageType) other);
    // END EXPERIMENTAL
    // BEGIN REGULAR

    try {
      visit(EqualsVisitor.INSTANCE, (MessageType) other);
    } catch (EqualsVisitor.NotEqualsException e) {
      return false;
    }
    return true;
    // END REGULAR
  }

  // BEGIN REGULAR
  /** Same as {@link #equals(Object)} but throws {@code NotEqualsException}. */
  @SuppressWarnings("unchecked") // Guaranteed by isInstance + runtime
  boolean equals(EqualsVisitor visitor, MessageLite other) {
    if (this == other) {
      return true;
    }

    if (!getDefaultInstanceForType().getClass().isInstance(other)) {
      return false;
    }

    visit(visitor, (MessageType) other);
    return true;
  }
  // END REGULAR

  // The general strategy for unknown fields is to use an UnknownFieldSetLite that is treated as
  // mutable during the parsing constructor and immutable after. This allows us to avoid
  // any unnecessary intermediary allocations while reducing the generated code size.

  /** Lazily initializes unknown fields. */
  private final void ensureUnknownFieldsInitialized() {
    if (unknownFields == UnknownFieldSetLite.getDefaultInstance()) {
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
  protected void makeImmutable() {
    dynamicMethod(MethodToInvoke.MAKE_IMMUTABLE);

    unknownFields.makeImmutable();
  }

  protected final <
    MessageType extends GeneratedMessageLite<MessageType, BuilderType>,
    BuilderType extends GeneratedMessageLite.Builder<MessageType, BuilderType>>
        BuilderType createBuilder() {
    return (BuilderType) dynamicMethod(MethodToInvoke.NEW_BUILDER);
  }

  protected final <
    MessageType extends GeneratedMessageLite<MessageType, BuilderType>,
    BuilderType extends GeneratedMessageLite.Builder<MessageType, BuilderType>>
        BuilderType createBuilder(MessageType prototype) {
    return ((BuilderType) createBuilder()).mergeFrom(prototype);
  }

  @Override
  public final boolean isInitialized() {
    return isInitialized((MessageType) this, Boolean.TRUE);
  }

  @Override
  @SuppressWarnings("unchecked")
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
    // BEGIN REGULAR
    VISIT,
    // END REGULAR
    // Rely on/modify instance state
    GET_MEMOIZED_IS_INITIALIZED,
    SET_MEMOIZED_IS_INITIALIZED,
    MERGE_FROM_STREAM,
    MAKE_IMMUTABLE,

    // Rely on static state
    NEW_MUTABLE_INSTANCE,
    NEW_BUILDER,
    GET_DEFAULT_INSTANCE,
    GET_PARSER;
  }

  /**
   * A method that implements different types of operations described in {@link MethodToInvoke}.
   * Theses different kinds of operations are required to implement message-level operations for
   * builders in the runtime. This method bundles those operations to reduce the generated methods
   * count.
   *
   * <ul>
   *   <li>{@code MERGE_FROM_STREAM} is parameterized with an {@link CodedInputStream} and {@link
   *       ExtensionRegistryLite}. It consumes the input stream, parsing the contents into the
   *       returned protocol buffer. If parsing throws an {@link InvalidProtocolBufferException},
   *       the implementation wraps it in a RuntimeException.
   *   <li>{@code NEW_INSTANCE} returns a new instance of the protocol buffer that has not yet been
   *       made immutable. See {@code MAKE_IMMUTABLE}.
   *   <li>{@code IS_INITIALIZED} returns {@code null} for false and the default instance for true.
   *       It doesn't use or modify any memoized value.
   *   <li>{@code GET_MEMOIZED_IS_INITIALIZED} returns the memoized {@code isInitialized} byte
   *       value.
   *   <li>{@code SET_MEMOIZED_IS_INITIALIZED} sets the memoized {@code isInitilaized} byte value to
   *       1 if the first parameter is not null, or to 0 if the first parameter is null.
   *   <li>{@code NEW_BUILDER} returns a {@code BuilderType} instance.
   *   <li>{@code VISIT} is parameterized with a {@code Visitor} and a {@code MessageType} and
   *       recursively iterates through the fields side by side between this and the instance.
   *   <li>{@code MAKE_IMMUTABLE} sets all internal fields to an immutable state.
   * </ul>
   *
   * This method, plus the implementation of the Builder, enables the Builder class to be proguarded
   * away entirely on Android.
   *
   * <p>For use by generated code only.
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

  // BEGIN REGULAR
  void visit(Visitor visitor, MessageType other) {
    dynamicMethod(MethodToInvoke.VISIT, visitor, other);
    unknownFields = visitor.visitUnknownFields(unknownFields, other.unknownFields);
  }
  // END REGULAR



  /**
   * Merge some unknown fields into the {@link UnknownFieldSetLite} for this message.
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
          extends AbstractMessageLite.Builder<MessageType, BuilderType> {

    private final MessageType defaultInstance;
    protected MessageType instance;
    protected boolean isBuilt;

    protected Builder(MessageType defaultInstance) {
      this.defaultInstance = defaultInstance;
      this.instance =
          (MessageType) defaultInstance.dynamicMethod(MethodToInvoke.NEW_MUTABLE_INSTANCE);
      isBuilt = false;
    }

    /**
     * Called before any method that would mutate the builder to ensure that it correctly copies
     * any state before the write happens to preserve immutability guarantees.
     */
    protected void copyOnWrite() {
      if (isBuilt) {
        MessageType newInstance =
            (MessageType) instance.dynamicMethod(MethodToInvoke.NEW_MUTABLE_INSTANCE);
        mergeFromInstance(newInstance, instance);
        instance = newInstance;
        isBuilt = false;
      }
    }

    @Override
    public final boolean isInitialized() {
      return GeneratedMessageLite.isInitialized(instance, false /* shouldMemoize */);
    }

    @Override
    public final BuilderType clear() {
      // No need to copy on write since we're dropping the instance anyways.
      instance = (MessageType) instance.dynamicMethod(MethodToInvoke.NEW_MUTABLE_INSTANCE);
      return (BuilderType) this;
    }

    @Override
    public BuilderType clone() {
      BuilderType builder =
          (BuilderType) getDefaultInstanceForType().newBuilderForType();
      builder.mergeFrom(buildPartial());
      return builder;
    }

    @Override
    public MessageType buildPartial() {
      if (isBuilt) {
        return instance;
      }

      instance.makeImmutable();

      isBuilt = true;
      return instance;
    }

    @Override
    public final MessageType build() {
      MessageType result = buildPartial();
      if (!result.isInitialized()) {
        throw newUninitializedMessageException(result);
      }
      return result;
    }

    @Override
    protected BuilderType internalMergeFrom(MessageType message) {
      return mergeFrom(message);
    }

    /** All subclasses implement this. */
    public BuilderType mergeFrom(MessageType message) {
      copyOnWrite();
      mergeFromInstance(instance, message);
      return (BuilderType) this;
    }

    private void mergeFromInstance(MessageType dest, MessageType src) {
      // BEGIN EXPERIMENTAL
      // Protobuf.getInstance().schemaFor(dest).mergeFrom(dest, src);
      // END EXPERIMENTAL
      // BEGIN REGULAR
      dest.visit(MergeFromVisitor.INSTANCE, src);
      // END REGULAR
    }

    @Override
    public MessageType getDefaultInstanceForType() {
      return defaultInstance;
    }

    @Override
    public BuilderType mergeFrom(
        com.google.protobuf.CodedInputStream input,
        com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws IOException {
      copyOnWrite();
      try {
        instance.dynamicMethod(MethodToInvoke.MERGE_FROM_STREAM, input, extensionRegistry);
      } catch (RuntimeException e) {
        if (e.getCause() instanceof IOException) {
          throw (IOException) e.getCause();
        }
        throw e;
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

    @SuppressWarnings("unchecked")
    protected final void mergeExtensionFields(final MessageType other) {
      if (extensions.isImmutable()) {
        extensions = extensions.clone();
      }
      extensions.mergeFrom(((ExtendableMessage) other).extensions);
    }

    // BEGIN REGULAR
    @Override
    final void visit(Visitor visitor, MessageType other) {
      super.visit(visitor, other);
      extensions = visitor.visitExtensions(extensions, other.extensions);
    }
    // END REGULAR

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
        int tag)
        throws IOException {
      int fieldNumber = WireFormat.getTagFieldNumber(tag);

      // TODO(dweis): How much bytecode would be saved by not requiring the generated code to
      //     provide the default instance?
      GeneratedExtension<MessageType, ?> extension = extensionRegistry.findLiteExtensionByNumber(
          defaultInstance, fieldNumber);

      return parseExtension(input, extensionRegistry, extension, tag, fieldNumber);
    }

    private boolean parseExtension(
        CodedInputStream input,
        ExtensionRegistryLite extensionRegistry,
        GeneratedExtension<?, ?> extension,
        int tag,
        int fieldNumber)
        throws IOException {
      int wireType = WireFormat.getTagWireType(tag);
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
    
    /**
     * Parse an unknown field or an extension. For use by generated code only.
     *
     * <p>For use by generated code only.
     *
     * @return {@code true} unless the tag is an end-group tag.
     */
    protected <MessageType extends MessageLite> boolean parseUnknownFieldAsMessageSet(
        MessageType defaultInstance,
        CodedInputStream input,
        ExtensionRegistryLite extensionRegistry,
        int tag)
        throws IOException {

      if (tag == WireFormat.MESSAGE_SET_ITEM_TAG) {
        mergeMessageSetExtensionFromCodedStream(defaultInstance, input, extensionRegistry);
        return true;
      }

      // TODO(dweis): Do we really want to support non message set wire format in message sets?
      // Full runtime does... So we do for now.
      int wireType = WireFormat.getTagWireType(tag);
      if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
        return parseUnknownField(defaultInstance, input, extensionRegistry, tag);
      } else {
        // TODO(dweis): Should we throw on invalid input? Full runtime does not...
        return input.skipField(tag);
      }
    }

    /**
     * Merges the message set from the input stream; requires message set wire format.
     * 
     * @param defaultInstance the default instance of the containing message we are parsing in
     * @param input the stream to parse from
     * @param extensionRegistry the registry to use when parsing
     */
    private <MessageType extends MessageLite> void mergeMessageSetExtensionFromCodedStream(
        MessageType defaultInstance,
        CodedInputStream input,
        ExtensionRegistryLite extensionRegistry)
        throws IOException {
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
      GeneratedExtension<?, ?> extension = null;

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
            extension = extensionRegistry.findLiteExtensionByNumber(defaultInstance, typeId);
          }

        } else if (tag == WireFormat.MESSAGE_SET_MESSAGE_TAG) {
          if (typeId != 0) {
            if (extension != null) {
              // We already know the type, so we can parse directly from the
              // input with no copying.  Hooray!
              eagerlyMergeMessageSetExtension(input, extension, extensionRegistry, typeId);
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
          mergeMessageSetExtensionFromBytes(rawBytes, extensionRegistry, extension);
        } else { // We don't know how to parse this. Ignore it.
          if (rawBytes != null) {
            mergeLengthDelimitedField(typeId, rawBytes);
          }
        }
      }
    }

    private void eagerlyMergeMessageSetExtension(
        CodedInputStream input,
        GeneratedExtension<?, ?> extension,
        ExtensionRegistryLite extensionRegistry,
        int typeId)
        throws IOException {
      int fieldNumber = typeId;
      int tag = WireFormat.makeTag(typeId, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      parseExtension(input, extensionRegistry, extension, tag, fieldNumber);
    }

    private void mergeMessageSetExtensionFromBytes(
        ByteString rawBytes,
        ExtensionRegistryLite extensionRegistry,
        GeneratedExtension<?, ?> extension)
        throws IOException {
      MessageLite.Builder subBuilder = null;
      MessageLite existingValue = (MessageLite) extensions.getField(extension.descriptor);
      if (existingValue != null) {
        subBuilder = existingValue.toBuilder();
      }
      if (subBuilder == null) {
        subBuilder = extension.getMessageDefaultInstance().newBuilderForType();
      }
      rawBytes.newCodedInput().readMessage(subBuilder, extensionRegistry);
      MessageLite value = subBuilder.build();

      extensions.setField(extension.descriptor, extension.singularToFieldSetType(value));
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
    @Override
    public final <Type> boolean hasExtension(final ExtensionLite<MessageType, Type> extension) {
      GeneratedExtension<MessageType, Type> extensionLite =
          checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      return extensions.hasField(extensionLite.descriptor);
    }

    /** Get the number of elements in a repeated extension. */
    @Override
    public final <Type> int getExtensionCount(
        final ExtensionLite<MessageType, List<Type>> extension) {
      GeneratedExtension<MessageType, List<Type>> extensionLite =
          checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      return extensions.getRepeatedFieldCount(extensionLite.descriptor);
    }

    /** Get the value of an extension. */
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(final ExtensionLite<MessageType, Type> extension) {
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
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, List<Type>> extension, final int index) {
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
    protected final void makeImmutable() {
      super.makeImmutable();

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

    @Override
    protected void copyOnWrite() {
      if (!isBuilt) {
        return;
      }

      super.copyOnWrite();
      instance.extensions = instance.extensions.clone();
    }

    @Override
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
    @Override
    public final <Type> boolean hasExtension(final ExtensionLite<MessageType, Type> extension) {
      return instance.hasExtension(extension);
    }

    /** Get the number of elements in a repeated extension. */
    @Override
    public final <Type> int getExtensionCount(
        final ExtensionLite<MessageType, List<Type>> extension) {
      return instance.getExtensionCount(extension);
    }

    /** Get the value of an extension. */
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(final ExtensionLite<MessageType, Type> extension) {
      return instance.getExtension(extension);
    }

    /** Get one element of a repeated extension. */
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, List<Type>> extension, final int index) {
      return instance.getExtension(extension, index);
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

    @Override
    public int getNumber() {
      return number;
    }

    @Override
    public WireFormat.FieldType getLiteType() {
      return type;
    }

    @Override
    public WireFormat.JavaType getLiteJavaType() {
      return type.getJavaType();
    }

    @Override
    public boolean isRepeated() {
      return isRepeated;
    }

    @Override
    public boolean isPacked() {
      return isPacked;
    }

    @Override
    public Internal.EnumLiteMap<?> getEnumType() {
      return enumTypeMap;
    }

    @Override
    @SuppressWarnings("unchecked")
    public MessageLite.Builder internalMergeFrom(MessageLite.Builder to, MessageLite from) {
      return ((Builder) to).mergeFrom((GeneratedMessageLite) from);
    }


    @Override
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
    @Override
    public int getNumber() {
      return descriptor.getNumber();
    }


    /**
     * If the extension is an embedded message or group, returns the default
     * instance of the message.
     */
    @Override
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

    @Override
    public FieldType getLiteType() {
      return descriptor.getLiteType();
    }

    @Override
    public boolean isRepeated() {
      return descriptor.isRepeated;
    }

    @Override
    public Type getDefaultValue() {
      return defaultValue;
    }
  }

  /**
   * A serialized (serializable) form of the generated message.  Stores the
   * message as a class name and a byte array.
   */
  protected static final class SerializedForm implements Serializable {

    public static SerializedForm of(MessageLite message) {
      return new SerializedForm(message);
    }

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
        return readResolveFallback();
      } catch (SecurityException e) {
        throw new RuntimeException("Unable to call DEFAULT_INSTANCE in " + messageClassName, e);
      } catch (IllegalAccessException e) {
        throw new RuntimeException("Unable to call parsePartialFrom", e);
      } catch (InvalidProtocolBufferException e) {
        throw new RuntimeException("Unable to understand proto buffer", e);
      }
    }

    /**
     * @deprecated from v3.0.0-beta-3+, for compatibility with v2.5.0 and v2.6.1 generated code.
     */
    @Deprecated
    private Object readResolveFallback() throws ObjectStreamException {
      try {
        Class<?> messageClass = Class.forName(messageClassName);
        java.lang.reflect.Field defaultInstanceField =
            messageClass.getDeclaredField("defaultInstance");
        defaultInstanceField.setAccessible(true);
        MessageLite defaultInstance = (MessageLite) defaultInstanceField.get(null);
        return defaultInstance.newBuilderForType()
            .mergeFrom(asBytes)
            .buildPartial();
      } catch (ClassNotFoundException e) {
        throw new RuntimeException("Unable to find proto buffer class: " + messageClassName, e);
      } catch (NoSuchFieldException e) {
        throw new RuntimeException("Unable to find defaultInstance in " + messageClassName, e);
      } catch (SecurityException e) {
        throw new RuntimeException("Unable to call defaultInstance in " + messageClassName, e);
      } catch (IllegalAccessException e) {
        throw new RuntimeException("Unable to call parsePartialFrom", e);
      } catch (InvalidProtocolBufferException e) {
        throw new RuntimeException("Unable to understand proto buffer", e);
      }
    }
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
    byte memoizedIsInitialized =
        (Byte) message.dynamicMethod(MethodToInvoke.GET_MEMOIZED_IS_INITIALIZED);
    if (memoizedIsInitialized == 1) {
      return true;
    }
    if (memoizedIsInitialized == 0) {
      return false;
    }
    boolean isInitialized =
        message.dynamicMethod(MethodToInvoke.IS_INITIALIZED, Boolean.FALSE) != null;
    if (shouldMemoize) {
      message.dynamicMethod(
          MethodToInvoke.SET_MEMOIZED_IS_INITIALIZED, isInitialized ? message : null);
    }
    return isInitialized;
  }

  protected static final <T extends GeneratedMessageLite<T, ?>> void makeImmutable(T message) {
    message.dynamicMethod(MethodToInvoke.MAKE_IMMUTABLE);
  }

  protected static IntList emptyIntList() {
    return IntArrayList.emptyList();
  }

  protected static IntList mutableCopy(IntList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(
        size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
  }

  protected static LongList emptyLongList() {
    return LongArrayList.emptyList();
  }

  protected static LongList mutableCopy(LongList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(
        size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
  }

  protected static FloatList emptyFloatList() {
    return FloatArrayList.emptyList();
  }

  protected static FloatList mutableCopy(FloatList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(
        size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
  }

  protected static DoubleList emptyDoubleList() {
    return DoubleArrayList.emptyList();
  }

  protected static DoubleList mutableCopy(DoubleList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(
        size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
  }

  protected static BooleanList emptyBooleanList() {
    return BooleanArrayList.emptyList();
  }

  protected static BooleanList mutableCopy(BooleanList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(
        size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
  }

  protected static <E> ProtobufList<E> emptyProtobufList() {
    return ProtobufArrayList.emptyList();
  }

  protected static <E> ProtobufList<E> mutableCopy(ProtobufList<E> list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(
        size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
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

  /**
   * A static helper method for parsing a partial from input using the extension registry and the
   * instance.
   */
  // TODO(dweis): Should this verify that the last tag was 0?
  static <T extends GeneratedMessageLite<T, ?>> T parsePartialFrom(
      T instance, CodedInputStream input, ExtensionRegistryLite extensionRegistry)
          throws InvalidProtocolBufferException {
    @SuppressWarnings("unchecked") // Guaranteed by protoc
    T result = (T) instance.dynamicMethod(MethodToInvoke.NEW_MUTABLE_INSTANCE);
    try {
      result.dynamicMethod(MethodToInvoke.MERGE_FROM_STREAM, input, extensionRegistry);
      result.makeImmutable();
    } catch (RuntimeException e) {
      if (e.getCause() instanceof InvalidProtocolBufferException) {
        throw (InvalidProtocolBufferException) e.getCause();
      }
      throw e;
    }
    return result;
  }

  protected static <T extends GeneratedMessageLite<T, ?>> T parsePartialFrom(
      T defaultInstance,
      CodedInputStream input)
      throws InvalidProtocolBufferException {
    return parsePartialFrom(defaultInstance, input, ExtensionRegistryLite.getEmptyRegistry());
  }

  /**
   * Helper method to check if message is initialized.
   *
   * @throws InvalidProtocolBufferException if it is not initialized.
   * @return The message to check.
   */
  private static <T extends GeneratedMessageLite<T, ?>> T checkMessageInitialized(T message)
      throws InvalidProtocolBufferException {
    if (message != null && !message.isInitialized()) {
      throw message.newUninitializedMessageException()
          .asInvalidProtocolBufferException()
          .setUnfinishedMessage(message);
    }
    return message;
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, ByteBuffer data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parseFrom(defaultInstance, CodedInputStream.newInstance(data), extensionRegistry));
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, ByteBuffer data) throws InvalidProtocolBufferException {
    return parseFrom(defaultInstance, data, ExtensionRegistryLite.getEmptyRegistry());
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, ByteString data)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parseFrom(defaultInstance, data, ExtensionRegistryLite.getEmptyRegistry()));
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, ByteString data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(parsePartialFrom(defaultInstance, data, extensionRegistry));
  }

  // This is a special case since we want to verify that the last tag is 0. We assume we exhaust the
  // ByteString.
  private static <T extends GeneratedMessageLite<T, ?>> T parsePartialFrom(
      T defaultInstance, ByteString data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    T message;
    try {
      CodedInputStream input = data.newCodedInput();
      message = parsePartialFrom(defaultInstance, input, extensionRegistry);
      try {
        input.checkLastTagWas(0);
      } catch (InvalidProtocolBufferException e) {
        throw e.setUnfinishedMessage(message);
      }
      return message;
    } catch (InvalidProtocolBufferException e) {
      throw e;
    }
  }

  // This is a special case since we want to verify that the last tag is 0. We assume we exhaust the
  // ByteString.
  private static <T extends GeneratedMessageLite<T, ?>> T parsePartialFrom(
      T defaultInstance, byte[] data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    T message;
    try {
      CodedInputStream input = CodedInputStream.newInstance(data);
      message = parsePartialFrom(defaultInstance, input, extensionRegistry);
      try {
        input.checkLastTagWas(0);
      } catch (InvalidProtocolBufferException e) {
        throw e.setUnfinishedMessage(message);
      }
      return message;
    } catch (InvalidProtocolBufferException e) {
      throw e;
    }
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, byte[] data)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialFrom(defaultInstance, data, ExtensionRegistryLite.getEmptyRegistry()));
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, byte[] data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(parsePartialFrom(defaultInstance, data, extensionRegistry));
  }

  // Does not validate last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, InputStream input)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialFrom(defaultInstance, CodedInputStream.newInstance(input),
            ExtensionRegistryLite.getEmptyRegistry()));
  }

  // Does not validate last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, InputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialFrom(defaultInstance, CodedInputStream.newInstance(input), extensionRegistry));
  }

  // Does not validate last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, CodedInputStream input)
      throws InvalidProtocolBufferException {
    return parseFrom(defaultInstance, input, ExtensionRegistryLite.getEmptyRegistry());
  }

  // Does not validate last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, CodedInputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialFrom(defaultInstance, input, extensionRegistry));
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseDelimitedFrom(
      T defaultInstance, InputStream input)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialDelimitedFrom(defaultInstance, input,
            ExtensionRegistryLite.getEmptyRegistry()));
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseDelimitedFrom(
      T defaultInstance, InputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialDelimitedFrom(defaultInstance, input, extensionRegistry));
  }

  private static <T extends GeneratedMessageLite<T, ?>> T parsePartialDelimitedFrom(
      T defaultInstance,
      InputStream input,
      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    int size;
    try {
      int firstByte = input.read();
      if (firstByte == -1) {
        return null;
      }
      size = CodedInputStream.readRawVarint32(firstByte, input);
    } catch (IOException e) {
      throw new InvalidProtocolBufferException(e.getMessage());
    }
    InputStream limitedInput = new LimitedInputStream(input, size);
    CodedInputStream codedInput = CodedInputStream.newInstance(limitedInput);
    T message = parsePartialFrom(defaultInstance, codedInput, extensionRegistry);
    try {
      codedInput.checkLastTagWas(0);
    } catch (InvalidProtocolBufferException e) {
      throw e.setUnfinishedMessage(message);
    }
    return message;
  }

  // BEGIN REGULAR
  /**
   * An abstract visitor that the generated code calls into that we use to implement various
   * features. Fields that are not members of oneofs are always visited. Members of a oneof are only
   * visited when they are the set oneof case value on the "other" proto. The visitOneofNotSet
   * method is invoked if other's oneof case is not set.
   */
  protected interface Visitor {
    boolean visitBoolean(boolean minePresent, boolean mine, boolean otherPresent, boolean other);
    int visitInt(boolean minePresent, int mine, boolean otherPresent, int other);
    double visitDouble(boolean minePresent, double mine, boolean otherPresent, double other);
    float visitFloat(boolean minePresent, float mine, boolean otherPresent, float other);
    long visitLong(boolean minePresent, long mine, boolean otherPresent, long other);
    String visitString(boolean minePresent, String mine, boolean otherPresent, String other);
    ByteString visitByteString(
        boolean minePresent, ByteString mine, boolean otherPresent, ByteString other);

    Object visitOneofBoolean(boolean minePresent, Object mine, Object other);
    Object visitOneofInt(boolean minePresent, Object mine, Object other);
    Object visitOneofDouble(boolean minePresent, Object mine, Object other);
    Object visitOneofFloat(boolean minePresent, Object mine, Object other);
    Object visitOneofLong(boolean minePresent, Object mine, Object other);
    Object visitOneofString(boolean minePresent, Object mine, Object other);
    Object visitOneofByteString(boolean minePresent, Object mine, Object other);
    Object visitOneofMessage(boolean minePresent, Object mine, Object other);
    void visitOneofNotSet(boolean minePresent);

    /**
     * Message fields use null sentinals.
     */
    <T extends MessageLite> T visitMessage(T mine, T other);

    <T> ProtobufList<T> visitList(ProtobufList<T> mine, ProtobufList<T> other);
    BooleanList visitBooleanList(BooleanList mine, BooleanList other);
    IntList visitIntList(IntList mine, IntList other);
    DoubleList visitDoubleList(DoubleList mine, DoubleList other);
    FloatList visitFloatList(FloatList mine, FloatList other);
    LongList visitLongList(LongList mine, LongList other);
    FieldSet<ExtensionDescriptor> visitExtensions(
        FieldSet<ExtensionDescriptor> mine, FieldSet<ExtensionDescriptor> other);
    UnknownFieldSetLite visitUnknownFields(UnknownFieldSetLite mine, UnknownFieldSetLite other);
    <K, V> MapFieldLite<K, V> visitMap(MapFieldLite<K, V> mine, MapFieldLite<K, V> other);
  }

  /**
   * Implements equals. Throws a {@link NotEqualsException} when not equal.
   */
  static class EqualsVisitor implements Visitor {

    static final class NotEqualsException extends RuntimeException {}

    static final EqualsVisitor INSTANCE = new EqualsVisitor();

    static final NotEqualsException NOT_EQUALS = new NotEqualsException();

    private EqualsVisitor() {}

    @Override
    public boolean visitBoolean(
        boolean minePresent, boolean mine, boolean otherPresent, boolean other) {
      if (minePresent != otherPresent || mine != other) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public int visitInt(boolean minePresent, int mine, boolean otherPresent, int other) {
      if (minePresent != otherPresent || mine != other) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public double visitDouble(
        boolean minePresent, double mine, boolean otherPresent, double other) {
      if (minePresent != otherPresent || mine != other) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public float visitFloat(boolean minePresent, float mine, boolean otherPresent, float other) {
      if (minePresent != otherPresent || mine != other) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public long visitLong(boolean minePresent, long mine, boolean otherPresent, long other) {
      if (minePresent != otherPresent || mine != other) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public String visitString(
        boolean minePresent, String mine, boolean otherPresent, String other) {
      if (minePresent != otherPresent || !mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public ByteString visitByteString(
        boolean minePresent, ByteString mine, boolean otherPresent, ByteString other) {
      if (minePresent != otherPresent || !mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public Object visitOneofBoolean(boolean minePresent, Object mine, Object other) {
      if (minePresent && mine.equals(other)) {
        return mine;
      }
      throw NOT_EQUALS;
    }

    @Override
    public Object visitOneofInt(boolean minePresent, Object mine, Object other) {
      if (minePresent && mine.equals(other)) {
        return mine;
      }
      throw NOT_EQUALS;
    }

    @Override
    public Object visitOneofDouble(boolean minePresent, Object mine, Object other) {
      if (minePresent && mine.equals(other)) {
        return mine;
      }
      throw NOT_EQUALS;
    }

    @Override
    public Object visitOneofFloat(boolean minePresent, Object mine, Object other) {
      if (minePresent && mine.equals(other)) {
        return mine;
      }
      throw NOT_EQUALS;
    }

    @Override
    public Object visitOneofLong(boolean minePresent, Object mine, Object other) {
      if (minePresent && mine.equals(other)) {
        return mine;
      }
      throw NOT_EQUALS;
    }

    @Override
    public Object visitOneofString(boolean minePresent, Object mine, Object other) {
      if (minePresent && mine.equals(other)) {
        return mine;
      }
      throw NOT_EQUALS;
    }

    @Override
    public Object visitOneofByteString(boolean minePresent, Object mine, Object other) {
      if (minePresent && mine.equals(other)) {
        return mine;
      }
      throw NOT_EQUALS;
    }

    @Override
    public Object visitOneofMessage(boolean minePresent, Object mine, Object other) {
      if (minePresent && ((GeneratedMessageLite<?, ?>) mine).equals(this, (MessageLite) other)) {
        return mine;
      }
      throw NOT_EQUALS;
    }

    @Override
    public void visitOneofNotSet(boolean minePresent) {
      if (minePresent) {
        throw NOT_EQUALS;
      }
    }

    @Override
    public <T extends MessageLite> T visitMessage(T mine, T other) {
      if (mine == null && other == null) {
        return null;
      }

      if (mine == null || other == null) {
        throw NOT_EQUALS;
      }

      ((GeneratedMessageLite<?, ?>) mine).equals(this, other);

      return mine;
    }

    @Override
    public <T> ProtobufList<T> visitList(ProtobufList<T> mine, ProtobufList<T> other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public BooleanList visitBooleanList(BooleanList mine, BooleanList other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public IntList visitIntList(IntList mine, IntList other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public DoubleList visitDoubleList(DoubleList mine, DoubleList other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public FloatList visitFloatList(FloatList mine, FloatList other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public LongList visitLongList(LongList mine, LongList other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public FieldSet<ExtensionDescriptor> visitExtensions(
        FieldSet<ExtensionDescriptor> mine,
        FieldSet<ExtensionDescriptor> other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public UnknownFieldSetLite visitUnknownFields(
        UnknownFieldSetLite mine,
        UnknownFieldSetLite other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }

    @Override
    public <K, V> MapFieldLite<K, V> visitMap(MapFieldLite<K, V> mine, MapFieldLite<K, V> other) {
      if (!mine.equals(other)) {
        throw NOT_EQUALS;
      }
      return mine;
    }
  }

  /**
   * Implements hashCode by accumulating state.
   */
  static class HashCodeVisitor implements Visitor {

    // The caller must ensure that the visitor is invoked parameterized with this and this such that
    // other is this. This is required due to how oneof cases are handled. See the class comment
    // on Visitor for more information.

    int hashCode = 0;

    @Override
    public boolean visitBoolean(
        boolean minePresent, boolean mine, boolean otherPresent, boolean other) {
      hashCode = (53 * hashCode) + Internal.hashBoolean(mine);
      return mine;
    }

    @Override
    public int visitInt(boolean minePresent, int mine, boolean otherPresent, int other) {
      hashCode = (53 * hashCode) + mine;
      return mine;
    }

    @Override
    public double visitDouble(
        boolean minePresent, double mine, boolean otherPresent, double other) {
      hashCode = (53 * hashCode) + Internal.hashLong(Double.doubleToLongBits(mine));
      return mine;
    }

    @Override
    public float visitFloat(boolean minePresent, float mine, boolean otherPresent, float other) {
      hashCode = (53 * hashCode) + Float.floatToIntBits(mine);
      return mine;
    }

    @Override
    public long visitLong(boolean minePresent, long mine, boolean otherPresent, long other) {
      hashCode = (53 * hashCode) + Internal.hashLong(mine);
      return mine;
    }

    @Override
    public String visitString(
        boolean minePresent, String mine, boolean otherPresent, String other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public ByteString visitByteString(
        boolean minePresent, ByteString mine, boolean otherPresent, ByteString other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public Object visitOneofBoolean(boolean minePresent, Object mine, Object other) {
      hashCode = (53 * hashCode) + Internal.hashBoolean(((Boolean) mine));
      return mine;
    }

    @Override
    public Object visitOneofInt(boolean minePresent, Object mine, Object other) {
      hashCode = (53 * hashCode) + (Integer) mine;
      return mine;
    }

    @Override
    public Object visitOneofDouble(boolean minePresent, Object mine, Object other) {
      hashCode = (53 * hashCode) + Internal.hashLong(Double.doubleToLongBits((Double) mine));
      return mine;
    }

    @Override
    public Object visitOneofFloat(boolean minePresent, Object mine, Object other) {
      hashCode = (53 * hashCode) + Float.floatToIntBits((Float) mine);
      return mine;
    }

    @Override
    public Object visitOneofLong(boolean minePresent, Object mine, Object other) {
      hashCode = (53 * hashCode) + Internal.hashLong((Long) mine);
      return mine;
    }

    @Override
    public Object visitOneofString(boolean minePresent, Object mine, Object other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public Object visitOneofByteString(boolean minePresent, Object mine, Object other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public Object visitOneofMessage(boolean minePresent, Object mine, Object other) {
      return visitMessage((MessageLite) mine, (MessageLite) other);
    }

    @Override
    public void visitOneofNotSet(boolean minePresent) {
      if (minePresent) {
        throw new IllegalStateException(); // Can't happen if other == this.
      }
    }

    @Override
    public <T extends MessageLite> T visitMessage(T mine, T other) {
      final int protoHash;
      if (mine != null) {
        if (mine instanceof GeneratedMessageLite) {
          protoHash = ((GeneratedMessageLite) mine).hashCode(this);
        } else {
          protoHash = mine.hashCode();
        }
      } else {
        protoHash = 37;
      }
      hashCode = (53 * hashCode) + protoHash;
      return mine;
    }

    @Override
    public <T> ProtobufList<T> visitList(ProtobufList<T> mine, ProtobufList<T> other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public BooleanList visitBooleanList(BooleanList mine, BooleanList other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public IntList visitIntList(IntList mine, IntList other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public DoubleList visitDoubleList(DoubleList mine, DoubleList other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public FloatList visitFloatList(FloatList mine, FloatList other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public LongList visitLongList(LongList mine, LongList other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public FieldSet<ExtensionDescriptor> visitExtensions(
        FieldSet<ExtensionDescriptor> mine,
        FieldSet<ExtensionDescriptor> other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public UnknownFieldSetLite visitUnknownFields(
        UnknownFieldSetLite mine,
        UnknownFieldSetLite other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }

    @Override
    public <K, V> MapFieldLite<K, V> visitMap(MapFieldLite<K, V> mine, MapFieldLite<K, V> other) {
      hashCode = (53 * hashCode) + mine.hashCode();
      return mine;
    }
  }

  /**
   * Implements field merging semantics over the visitor interface.
   */
  protected static class MergeFromVisitor implements Visitor {

    public static final MergeFromVisitor INSTANCE = new MergeFromVisitor();

    private MergeFromVisitor() {}

    @Override
    public boolean visitBoolean(
        boolean minePresent, boolean mine, boolean otherPresent, boolean other) {
      return otherPresent ? other : mine;
    }

    @Override
    public int visitInt(boolean minePresent, int mine, boolean otherPresent, int other) {
      return otherPresent ? other : mine;
    }

    @Override
    public double visitDouble(
        boolean minePresent, double mine, boolean otherPresent, double other) {
      return otherPresent ? other : mine;
    }

    @Override
    public float visitFloat(boolean minePresent, float mine, boolean otherPresent, float other) {
      return otherPresent ? other : mine;
    }

    @Override
    public long visitLong(boolean minePresent, long mine, boolean otherPresent, long other) {
      return otherPresent ? other : mine;
    }

    @Override
    public String visitString(
        boolean minePresent, String mine, boolean otherPresent, String other) {
      return otherPresent ? other : mine;
    }

    @Override
    public ByteString visitByteString(
        boolean minePresent, ByteString mine, boolean otherPresent, ByteString other) {
      return otherPresent ? other : mine;
    }

    @Override
    public Object visitOneofBoolean(boolean minePresent, Object mine, Object other) {
      return other;
    }

    @Override
    public Object visitOneofInt(boolean minePresent, Object mine, Object other) {
      return other;
    }

    @Override
    public Object visitOneofDouble(boolean minePresent, Object mine, Object other) {
      return other;
    }

    @Override
    public Object visitOneofFloat(boolean minePresent, Object mine, Object other) {
      return other;
    }

    @Override
    public Object visitOneofLong(boolean minePresent, Object mine, Object other) {
      return other;
    }

    @Override
    public Object visitOneofString(boolean minePresent, Object mine, Object other) {
      return other;
    }

    @Override
    public Object visitOneofByteString(boolean minePresent, Object mine, Object other) {
      return other;
    }

    @Override
    public Object visitOneofMessage(boolean minePresent, Object mine, Object other) {
      if (minePresent) {
        return visitMessage((MessageLite) mine, (MessageLite) other);
      }
      return other;
    }

    @Override
    public void visitOneofNotSet(boolean minePresent) {
      return;
    }

    @SuppressWarnings("unchecked") // Guaranteed by runtime.
    @Override
    public <T extends MessageLite> T visitMessage(T mine, T other) {
      if (mine != null && other != null) {
        return (T) mine.toBuilder().mergeFrom(other).build();
      }

      return mine != null ? mine : other;
    }

    @Override
    public <T> ProtobufList<T> visitList(ProtobufList<T> mine, ProtobufList<T> other) {
      int size = mine.size();
      int otherSize = other.size();
      if (size > 0 && otherSize > 0) {
        if (!mine.isModifiable()) {
          mine = mine.mutableCopyWithCapacity(size + otherSize);
        }
        mine.addAll(other);
      }

      return size > 0 ? mine : other;
    }

    @Override
    public BooleanList visitBooleanList(BooleanList mine, BooleanList other) {
      int size = mine.size();
      int otherSize = other.size();
      if (size > 0 && otherSize > 0) {
        if (!mine.isModifiable()) {
          mine = mine.mutableCopyWithCapacity(size + otherSize);
        }
        mine.addAll(other);
      }

      return size > 0 ? mine : other;
    }

    @Override
    public IntList visitIntList(IntList mine, IntList other) {
      int size = mine.size();
      int otherSize = other.size();
      if (size > 0 && otherSize > 0) {
        if (!mine.isModifiable()) {
          mine = mine.mutableCopyWithCapacity(size + otherSize);
        }
        mine.addAll(other);
      }

      return size > 0 ? mine : other;
    }

    @Override
    public DoubleList visitDoubleList(DoubleList mine, DoubleList other) {
      int size = mine.size();
      int otherSize = other.size();
      if (size > 0 && otherSize > 0) {
        if (!mine.isModifiable()) {
          mine = mine.mutableCopyWithCapacity(size + otherSize);
        }
        mine.addAll(other);
      }

      return size > 0 ? mine : other;
    }

    @Override
    public FloatList visitFloatList(FloatList mine, FloatList other) {
      int size = mine.size();
      int otherSize = other.size();
      if (size > 0 && otherSize > 0) {
        if (!mine.isModifiable()) {
          mine = mine.mutableCopyWithCapacity(size + otherSize);
        }
        mine.addAll(other);
      }

      return size > 0 ? mine : other;
    }

    @Override
    public LongList visitLongList(LongList mine, LongList other) {
      int size = mine.size();
      int otherSize = other.size();
      if (size > 0 && otherSize > 0) {
        if (!mine.isModifiable()) {
          mine = mine.mutableCopyWithCapacity(size + otherSize);
        }
        mine.addAll(other);
      }

      return size > 0 ? mine : other;
    }

    @Override
    public FieldSet<ExtensionDescriptor> visitExtensions(
        FieldSet<ExtensionDescriptor> mine,
        FieldSet<ExtensionDescriptor> other) {
      if (mine.isImmutable()) {
        mine = mine.clone();
      }
      mine.mergeFrom(other);
      return mine;
    }

    @Override
    public UnknownFieldSetLite visitUnknownFields(
        UnknownFieldSetLite mine,
        UnknownFieldSetLite other) {
      return other == UnknownFieldSetLite.getDefaultInstance()
          ? mine : UnknownFieldSetLite.mutableCopyOf(mine, other);
    }

    @Override
    public <K, V> MapFieldLite<K, V> visitMap(MapFieldLite<K, V> mine, MapFieldLite<K, V> other) {
      if (!other.isEmpty()) {
        if (!mine.isMutable()) {
          mine = mine.mutableCopy();
        }
        mine.mergeFrom(other);
      }
      return mine;
    }
  }
  // END REGULAR
}
