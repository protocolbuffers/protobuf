// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.AbstractMessageLite.Builder.LimitedInputStream;
import com.google.protobuf.Internal.BooleanList;
import com.google.protobuf.Internal.DoubleList;
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

  /* For use by lite runtime only */
  static final int UNINITIALIZED_SERIALIZED_SIZE = 0x7FFFFFFF;
  private static final int MUTABLE_FLAG_MASK = 0x80000000;
  private static final int MEMOIZED_SERIALIZED_SIZE_MASK = 0x7FFFFFFF;

  /**
   * We use the high bit of memoizedSerializedSize as the explicit mutability flag. It didn't make
   * sense to have negative sizes anyway. Messages start as mutable.
   *
   * <p>Adding a standalone boolean would have added 8 bytes to every message instance.
   *
   * <p>We also reserve 0x7FFFFFFF as the "uninitialized" value.
   */
  private int memoizedSerializedSize = MUTABLE_FLAG_MASK | UNINITIALIZED_SERIALIZED_SIZE;

  /* For use by the runtime only */
  static final int UNINITIALIZED_HASH_CODE = 0;

  /** For use by generated code only. Lazily initialized to reduce allocations. */
  protected UnknownFieldSetLite unknownFields = UnknownFieldSetLite.getDefaultInstance();

  boolean isMutable() {
    return (memoizedSerializedSize & MUTABLE_FLAG_MASK) != 0;
  }

  void markImmutable() {
    memoizedSerializedSize &= ~MUTABLE_FLAG_MASK;
  }

  int getMemoizedHashCode() {
    return memoizedHashCode;
  }

  void setMemoizedHashCode(int value) {
    memoizedHashCode = value;
  }

  void clearMemoizedHashCode() {
    memoizedHashCode = UNINITIALIZED_HASH_CODE;
  }

  boolean hashCodeIsNotMemoized() {
    return UNINITIALIZED_HASH_CODE == getMemoizedHashCode();
  }

  @Override
  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final Parser<MessageType> getParserForType() {
    return (Parser<MessageType>) dynamicMethod(MethodToInvoke.GET_PARSER, null, null);
  }

  @Override
  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final MessageType getDefaultInstanceForType() {
    return (MessageType) dynamicMethod(MethodToInvoke.GET_DEFAULT_INSTANCE, null, null);
  }

  @Override
  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  public final BuilderType newBuilderForType() {
    return (BuilderType) dynamicMethod(MethodToInvoke.NEW_BUILDER, null, null);
  }

  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  MessageType newMutableInstance() {
    return (MessageType) dynamicMethod(MethodToInvoke.NEW_MUTABLE_INSTANCE, null, null);
  }

  /**
   * A reflective toString function. This is primarily intended as a developer aid, while keeping
   * binary size down. The first line of the {@code toString()} representation includes a commented
   * version of {@code super.toString()} to act as an indicator that this should not be relied on
   * for comparisons.
   *
   * <p>NOTE: This method relies on the field getter methods not being stripped or renamed by
   * proguard. If they are, the fields will not be included in the returned string representation.
   *
   * <p>NOTE: This implementation is liable to change in the future, and should not be relied on in
   * code.
   */
  @Override
  public String toString() {
    return MessageLiteToString.toString(this, super.toString());
  }

  @Override
  public int hashCode() {
    if (isMutable()) {
      return computeHashCode();
    }

    if (hashCodeIsNotMemoized()) {
      setMemoizedHashCode(computeHashCode());
    }

    return getMemoizedHashCode();
  }

  int computeHashCode() {
    return Protobuf.getInstance().schemaFor(this).hashCode(this);
  }

  @SuppressWarnings("unchecked") // Guaranteed by isInstance + runtime
  @Override
  public boolean equals(
          Object other) {
    if (this == other) {
      return true;
    }

    if (other == null) {
      return false;
    }

    if (this.getClass() != other.getClass()) {
      return false;
    }

    return Protobuf.getInstance().schemaFor(this).equals(this, (MessageType) other);
  }

  // The general strategy for unknown fields is to use an UnknownFieldSetLite that is treated as
  // mutable during the parsing constructor and immutable after. This allows us to avoid
  // any unnecessary intermediary allocations while reducing the generated code size.

  /** Lazily initializes unknown fields. */
  private void ensureUnknownFieldsInitialized() {
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

  /** Called by subclasses to parse an unknown field. For use by generated code only. */
  protected void mergeVarintField(int tag, int value) {
    ensureUnknownFieldsInitialized();
    unknownFields.mergeVarintField(tag, value);
  }

  /** Called by subclasses to parse an unknown field. For use by generated code only. */
  protected void mergeLengthDelimitedField(int fieldNumber, ByteString value) {
    ensureUnknownFieldsInitialized();
    unknownFields.mergeLengthDelimitedField(fieldNumber, value);
  }

  /** Called by subclasses to complete parsing. For use by generated code only. */
  protected void makeImmutable() {
    Protobuf.getInstance().schemaFor(this).makeImmutable(this);
    markImmutable();
  }

  @SuppressWarnings("unchecked") // Guaranteed by runtime.
  protected final <
          MessageType extends GeneratedMessageLite<MessageType, BuilderType>,
          BuilderType extends GeneratedMessageLite.Builder<MessageType, BuilderType>>
      BuilderType createBuilder() {
    return (BuilderType) dynamicMethod(MethodToInvoke.NEW_BUILDER, null, null);
  }

  protected final <
          MessageType extends GeneratedMessageLite<MessageType, BuilderType>,
          BuilderType extends GeneratedMessageLite.Builder<MessageType, BuilderType>>
      BuilderType createBuilder(MessageType prototype) {
    return ((BuilderType) createBuilder()).mergeFrom(prototype);
  }

  @Override
  public final boolean isInitialized() {
    return isInitialized((MessageType) this, /* shouldMemoize= */ true);
  }

  @Override
  @SuppressWarnings("unchecked")
  public final BuilderType toBuilder() {
    BuilderType builder = (BuilderType) dynamicMethod(MethodToInvoke.NEW_BUILDER, null, null);
    return builder.mergeFrom((MessageType) this);
  }

  /**
   * Defines which method path to invoke in {@link GeneratedMessageLite
   * #dynamicMethod(MethodToInvoke, Object...)}.
   *
   * <p>For use by generated code only.
   */
  public static enum MethodToInvoke {
    // Rely on/modify instance state
    GET_MEMOIZED_IS_INITIALIZED,
    SET_MEMOIZED_IS_INITIALIZED,

    // Rely on static state
    BUILD_MESSAGE_INFO,
    NEW_MUTABLE_INSTANCE,
    NEW_BUILDER,
    GET_DEFAULT_INSTANCE,
    GET_PARSER;
  }

  /**
   * A method that implements different types of operations described in {@link MethodToInvoke}.
   * These different kinds of operations are required to implement message-level operations for
   * builders in the runtime. This method bundles those operations to reduce the generated methods
   * count.
   *
   * <ul>
   *   <li>{@code NEW_INSTANCE} returns a new instance of the protocol buffer that has not yet been
   *       made immutable. See {@code MAKE_IMMUTABLE}.
   *   <li>{@code IS_INITIALIZED} returns {@code null} for false and the default instance for true.
   *       It doesn't use or modify any memoized value.
   *   <li>{@code GET_MEMOIZED_IS_INITIALIZED} returns the memoized {@code isInitialized} byte
   *       value.
   *   <li>{@code SET_MEMOIZED_IS_INITIALIZED} sets the memoized {@code isInitialized} byte value to
   *       1 if the first parameter is not null, or to 0 if the first parameter is null.
   *   <li>{@code NEW_BUILDER} returns a {@code BuilderType} instance.
   * </ul>
   *
   * This method, plus the implementation of the Builder, enables the Builder class to be proguarded
   * away entirely on Android.
   *
   * <p>For use by generated code only.
   */
  protected abstract Object dynamicMethod(
      MethodToInvoke method,
          Object arg0,
          Object arg1);

  void clearMemoizedSerializedSize() {
    setMemoizedSerializedSize(UNINITIALIZED_SERIALIZED_SIZE);
  }

  @Override
  int getMemoizedSerializedSize() {
    return memoizedSerializedSize & MEMOIZED_SERIALIZED_SIZE_MASK;
  }

  @Override
  void setMemoizedSerializedSize(int size) {
    if (size < 0) {
      throw new IllegalStateException("serialized size must be non-negative, was " + size);
    }
    memoizedSerializedSize =
        (memoizedSerializedSize & MUTABLE_FLAG_MASK) | (size & MEMOIZED_SERIALIZED_SIZE_MASK);
  }

  @Override
  public void writeTo(CodedOutputStream output) throws IOException {
    Protobuf.getInstance()
        .schemaFor(this)
        .writeTo(this, CodedOutputStreamWriter.forCodedOutput(output));
  }

  @Override
  int getSerializedSize(
          Schema schema) {
    if (isMutable()) {
      // The serialized size should never be memoized for mutable instances.
      int size = computeSerializedSize(schema);
      if (size < 0) {
        throw new IllegalStateException("serialized size must be non-negative, was " + size);
      }
      return size;
    }

    // If memoizedSerializedSize has already been set, return it.
    if (getMemoizedSerializedSize() != UNINITIALIZED_SERIALIZED_SIZE) {
      return getMemoizedSerializedSize();
    }

    // Need to compute and memoize the serialized size.
    int size = computeSerializedSize(schema);
    setMemoizedSerializedSize(size);
    return size;
  }

  @Override
  public int getSerializedSize() {
    // Calling this with 'null' to delay schema lookup in case the serializedSize is already
    // memoized.
    return getSerializedSize(null);
  }

  private int computeSerializedSize(
          Schema<?> nullableSchema) {
    if (nullableSchema == null) {
      return Protobuf.getInstance().schemaFor(this).getSerializedSize(this);
    } else {
      return ((Schema<GeneratedMessageLite<MessageType, BuilderType>>) nullableSchema)
          .getSerializedSize(this);
    }
  }

  /** Constructs a {@link MessageInfo} for this message type. */
  Object buildMessageInfo() throws Exception {
    return dynamicMethod(MethodToInvoke.BUILD_MESSAGE_INFO, null, null);
  }

  private static Map<Class<?>, GeneratedMessageLite<?, ?>> defaultInstanceMap =
      new ConcurrentHashMap<>();

  @SuppressWarnings("unchecked")
  static <T extends GeneratedMessageLite<?, ?>> T getDefaultInstance(Class<T> clazz) {
    T result = (T) defaultInstanceMap.get(clazz);
    if (result == null) {
      // Foo.class does not initialize the class so we need to force the initialization in order to
      // get the default instance registered.
      try {
        Class.forName(clazz.getName(), true, clazz.getClassLoader());
      } catch (ClassNotFoundException e) {
        throw new IllegalStateException("Class initialization cannot fail.", e);
      }
      result = (T) defaultInstanceMap.get(clazz);
    }
    if (result == null) {
      // On some Samsung devices, this still doesn't return a valid value for some reason. We add a
      // reflective fallback to keep the device running. See b/114675342.
      result = (T) UnsafeUtil.allocateInstance(clazz).getDefaultInstanceForType();
      // A sanity check to ensure that <clinit> was actually invoked.
      if (result == null) {
        throw new IllegalStateException();
      }
      defaultInstanceMap.put(clazz, result);
    }
    return result;
  }

  protected static <T extends GeneratedMessageLite<?, ?>> void registerDefaultInstance(
      Class<T> clazz, T defaultInstance) {
    // Default instances must be immutable.
    // Marking immutable here to avoid extra bytecode in every generated message class.
    // Only calling "markImmutable" rather than "makeImmutable" because for Default Instances:
    // 1. All sub-messages are initialized to null / default instances and thus immutable
    // 2. All lists are initialized to default instance empty lists which are also immutable.
    defaultInstance.markImmutable();
    defaultInstanceMap.put(clazz, defaultInstance);
  }

  protected static Object newMessageInfo(
      MessageLite defaultInstance, String info, Object[] objects) {
    return new RawMessageInfo(defaultInstance, info, objects);
  }

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

    protected Builder(MessageType defaultInstance) {
      this.defaultInstance = defaultInstance;
      if (defaultInstance.isMutable()) {
        throw new IllegalArgumentException("Default instance must be immutable.");
      }
      // this.instance should be set to defaultInstance but some tests rely on newBuilder().build()
      // creating unique instances.
      this.instance = newMutableInstance();
    }

    private MessageType newMutableInstance() {
      return defaultInstance.newMutableInstance();
    }

    /**
     * Called before any method that would mutate the builder to ensure that it correctly copies any
     * state before the write happens to preserve immutability guarantees.
     */
    protected final void copyOnWrite() {
      if (!instance.isMutable()) {
        copyOnWriteInternal();
      }
    }

    protected void copyOnWriteInternal() {
      MessageType newInstance = newMutableInstance();
      mergeFromInstance(newInstance, instance);
      instance = newInstance;
    }

    @Override
    public final boolean isInitialized() {
      return GeneratedMessageLite.isInitialized(instance, /* shouldMemoize= */ false);
    }

    @Override
    public final BuilderType clear() {
      // No need to copy on write since we're dropping the instance anyway.
      if (defaultInstance.isMutable()) {
        throw new IllegalArgumentException("Default instance must be immutable.");
      }
      instance = newMutableInstance(); // should be defaultInstance;
      return (BuilderType) this;
    }

    @Override
    public BuilderType clone() {
      BuilderType builder = (BuilderType) getDefaultInstanceForType().newBuilderForType();
      builder.instance = buildPartial();
      return builder;
    }

    @Override
    public MessageType buildPartial() {
      if (!instance.isMutable()) {
        return instance;
      }

      instance.makeImmutable();
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
      if (getDefaultInstanceForType().equals(message)) {
        return (BuilderType) this;
      }
      copyOnWrite();
      mergeFromInstance(instance, message);
      return (BuilderType) this;
    }

    @Override
    public BuilderType mergeFrom(
        byte[] input, int offset, int length, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      copyOnWrite();
      try {
        Protobuf.getInstance()
            .schemaFor(instance)
            .mergeFrom(
                instance,
                input,
                offset,
                offset + length,
                new ArrayDecoders.Registers(extensionRegistry));
      } catch (InvalidProtocolBufferException e) {
        throw e;
      } catch (IndexOutOfBoundsException e) {
        throw InvalidProtocolBufferException.truncatedMessage();
      } catch (IOException e) {
        throw new RuntimeException("Reading from byte array should not throw IOException.", e);
      }
      return (BuilderType) this;
    }

    @Override
    public BuilderType mergeFrom(byte[] input, int offset, int length)
        throws InvalidProtocolBufferException {
      return mergeFrom(input, offset, length, ExtensionRegistryLite.getEmptyRegistry());
    }

    @Override
    public BuilderType mergeFrom(
        com.google.protobuf.CodedInputStream input,
        com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws IOException {
      copyOnWrite();
      try {
        // TODO: Try to make input with type CodedInputStream.ArrayDecoder use
        // fast path.
        Protobuf.getInstance()
            .schemaFor(instance)
            .mergeFrom(instance, CodedInputStreamReader.forCodedInput(input), extensionRegistry);
      } catch (RuntimeException e) {
        if (e.getCause() instanceof IOException) {
          throw (IOException) e.getCause();
        }
        throw e;
      }
      return (BuilderType) this;
    }

    private static <MessageType> void mergeFromInstance(MessageType dest, MessageType src) {
      Protobuf.getInstance().schemaFor(dest).mergeFrom(dest, src);
    }

    @Override
    public MessageType getDefaultInstanceForType() {
      return defaultInstance;
    }
  }

  // =================================================================
  // Extensions-related stuff

  /** Lite equivalent of {@link com.google.protobuf.GeneratedMessage.ExtendableMessageOrBuilder}. */
  public interface ExtendableMessageOrBuilder<
          MessageType extends ExtendableMessage<MessageType, BuilderType>,
          BuilderType extends ExtendableBuilder<MessageType, BuilderType>>
      extends MessageLiteOrBuilder {

    /** Check if a singular extension is present. */
    <Type> boolean hasExtension(ExtensionLite<MessageType, Type> extension);

    /** Get the number of elements in a repeated extension. */
    <Type> int getExtensionCount(ExtensionLite<MessageType, List<Type>> extension);

    /** Get the value of an extension. */
    <Type> Type getExtension(ExtensionLite<MessageType, Type> extension);

    /** Get one element of a repeated extension. */
    <Type> Type getExtension(ExtensionLite<MessageType, List<Type>> extension, int index);
  }

  /** Lite equivalent of {@link GeneratedMessage.ExtendableMessage}. */
  public abstract static class ExtendableMessage<
          MessageType extends ExtendableMessage<MessageType, BuilderType>,
          BuilderType extends ExtendableBuilder<MessageType, BuilderType>>
      extends GeneratedMessageLite<MessageType, BuilderType>
      implements ExtendableMessageOrBuilder<MessageType, BuilderType> {

    /** Represents the set of extensions on this message. For use by generated code only. */
    protected FieldSet<ExtensionDescriptor> extensions = FieldSet.emptySet();

    @SuppressWarnings("unchecked")
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
        int tag)
        throws IOException {
      int fieldNumber = WireFormat.getTagFieldNumber(tag);

      // TODO: How much bytecode would be saved by not requiring the generated code to
      //     provide the default instance?
      GeneratedExtension<MessageType, ?> extension =
          extensionRegistry.findLiteExtensionByNumber(defaultInstance, fieldNumber);

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
        unknown = true; // Unknown field.
      } else if (wireType
          == FieldSet.getWireFormatForFieldType(
              extension.descriptor.getLiteType(), /* isPacked= */ false)) {
        packed = false; // Normal, unpacked value.
      } else if (extension.descriptor.isRepeated
          && extension.descriptor.type.isPackable()
          && wireType
              == FieldSet.getWireFormatForFieldType(
                  extension.descriptor.getLiteType(), /* isPacked= */ true)) {
        packed = true; // Packed value.
      } else {
        unknown = true; // Wrong wire type.
      }

      if (unknown) { // Unknown field or wrong wire type.  Skip.
        return parseUnknownField(tag, input);
      }

      // TODO: remove the unused variable
      FieldSet<ExtensionDescriptor> unused = ensureExtensionsAreMutable();

      if (packed) {
        int length = input.readRawVarint32();
        int limit = input.pushLimit(length);
        if (extension.descriptor.getLiteType() == WireFormat.FieldType.ENUM) {
          while (input.getBytesUntilLimit() > 0) {
            int rawValue = input.readEnum();
            Object value = extension.descriptor.getEnumType().findValueByNumber(rawValue);
            if (value == null) {
              // If the number isn't recognized as a valid value for this
              // enum, drop it (don't even add it to unknownFields).
              return true;
            }
            extensions.addRepeatedField(
                extension.descriptor, extension.singularToFieldSetType(value));
          }
        } else {
          while (input.getBytesUntilLimit() > 0) {
            Object value =
                FieldSet.readPrimitiveField(
                    input, extension.descriptor.getLiteType(), /* checkUtf8= */ false);
            extensions.addRepeatedField(extension.descriptor, value);
          }
        }
        input.popLimit(limit);
      } else {
        Object value;
        switch (extension.descriptor.getLiteJavaType()) {
          case MESSAGE:
            {
              MessageLite.Builder subBuilder = null;
              if (!extension.descriptor.isRepeated()) {
                MessageLite existingValue = (MessageLite) extensions.getField(extension.descriptor);
                if (existingValue != null) {
                  subBuilder = existingValue.toBuilder();
                }
              }
              if (subBuilder == null) {
                subBuilder = extension.getMessageDefaultInstance().newBuilderForType();
              }
              if (extension.descriptor.getLiteType() == WireFormat.FieldType.GROUP) {
                input.readGroup(extension.getNumber(), subBuilder, extensionRegistry);
              } else {
                input.readMessage(subBuilder, extensionRegistry);
              }
              value = subBuilder.build();
              break;
            }
          case ENUM:
            int rawValue = input.readEnum();
            value = extension.descriptor.getEnumType().findValueByNumber(rawValue);
            // If the number isn't recognized as a valid value for this enum,
            // write it to unknown fields object.
            if (value == null) {
              mergeVarintField(fieldNumber, rawValue);
              return true;
            }
            break;
          default:
            value =
                FieldSet.readPrimitiveField(
                    input, extension.descriptor.getLiteType(), /* checkUtf8= */ false);
            break;
        }

        if (extension.descriptor.isRepeated()) {
          extensions.addRepeatedField(
              extension.descriptor, extension.singularToFieldSetType(value));
        } else {
          extensions.setField(extension.descriptor, extension.singularToFieldSetType(value));
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

      // TODO: Do we really want to support non message set wire format in message sets?
      // Full runtime does... So we do for now.
      int wireType = WireFormat.getTagWireType(tag);
      if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
        return parseUnknownField(defaultInstance, input, extensionRegistry, tag);
      } else {
        // TODO: Should we throw on invalid input? Full runtime does not...
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
      // TODO: remove the unused variable
      boolean unused = parseExtension(input, extensionRegistry, extension, tag, fieldNumber);
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
      subBuilder.mergeFrom(rawBytes, extensionRegistry);
      MessageLite value = subBuilder.build();

      ensureExtensionsAreMutable()
          .setField(extension.descriptor, extension.singularToFieldSetType(value));
    }

    @CanIgnoreReturnValue
    FieldSet<ExtensionDescriptor> ensureExtensionsAreMutable() {
      if (extensions.isImmutable()) {
        extensions = extensions.clone();
      }
      return extensions;
    }

    private void verifyExtensionContainingType(final GeneratedExtension<MessageType, ?> extension) {
      if (extension.getContainingTypeDefaultInstance() != getDefaultInstanceForType()) {
        // This can only happen if someone uses unchecked operations.
        throw new IllegalArgumentException(
            "This extension is for a different message type.  Please make "
                + "sure that you are not suppressing any generics type warnings.");
      }
    }

    /** Check if a singular extension is present. */
    @Override
    public final <Type> boolean hasExtension(final ExtensionLite<MessageType, Type> extension) {
      GeneratedExtension<MessageType, Type> extensionLite = checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      return extensions.hasField(extensionLite.descriptor);
    }

    /** Get the number of elements in a repeated extension. */
    @Override
    public final <Type> int getExtensionCount(
        final ExtensionLite<MessageType, List<Type>> extension) {
      GeneratedExtension<MessageType, List<Type>> extensionLite = checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      return extensions.getRepeatedFieldCount(extensionLite.descriptor);
    }

    /** Get the value of an extension. */
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(final ExtensionLite<MessageType, Type> extension) {
      GeneratedExtension<MessageType, Type> extensionLite = checkIsLite(extension);

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
      GeneratedExtension<MessageType, List<Type>> extensionLite = checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      return (Type)
          extensionLite.singularFromFieldSetType(
              extensions.getRepeatedField(extensionLite.descriptor, index));
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions.isInitialized();
    }

    /**
     * Used by subclasses to serialize extensions. Extension ranges may be interleaved with field
     * numbers, but we must write them in canonical (sorted by field number) order. ExtensionWriter
     * helps us write individual ranges of extensions at once.
     */
    protected class ExtensionWriter {
      // Imagine how much simpler this code would be if Java iterators had
      // a way to get the next element without advancing the iterator.

      private final Iterator<Map.Entry<ExtensionDescriptor, Object>> iter = extensions.iterator();
      private Map.Entry<ExtensionDescriptor, Object> next;
      private final boolean messageSetWireFormat;

      private ExtensionWriter(boolean messageSetWireFormat) {
        if (iter.hasNext()) {
          next = iter.next();
        }
        this.messageSetWireFormat = messageSetWireFormat;
      }

      public void writeUntil(final int end, final CodedOutputStream output) throws IOException {
        while (next != null && next.getKey().getNumber() < end) {
          ExtensionDescriptor extension = next.getKey();
          if (messageSetWireFormat
              && extension.getLiteJavaType() == WireFormat.JavaType.MESSAGE
              && !extension.isRepeated()) {
            output.writeMessageSetExtension(extension.getNumber(), (MessageLite) next.getValue());
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

  /** Lite equivalent of {@link GeneratedMessage.ExtendableBuilder}. */
  @SuppressWarnings("unchecked")
  public abstract static class ExtendableBuilder<
          MessageType extends ExtendableMessage<MessageType, BuilderType>,
          BuilderType extends ExtendableBuilder<MessageType, BuilderType>>
      extends Builder<MessageType, BuilderType>
      implements ExtendableMessageOrBuilder<MessageType, BuilderType> {
    protected ExtendableBuilder(MessageType defaultInstance) {
      super(defaultInstance);
    }

    // For immutable message conversion.
    void internalSetExtensionSet(FieldSet<ExtensionDescriptor> extensions) {
      copyOnWrite();
      instance.extensions = extensions;
    }

    @Override
    protected void copyOnWriteInternal() {
      super.copyOnWriteInternal();
      if (instance.extensions != FieldSet.emptySet()) {
        instance.extensions = instance.extensions.clone();
      }
    }

    private FieldSet<ExtensionDescriptor> ensureExtensionsAreMutable() {
      FieldSet<ExtensionDescriptor> extensions = instance.extensions;
      if (extensions.isImmutable()) {
        extensions = extensions.clone();
        instance.extensions = extensions;
      }
      return extensions;
    }

    @Override
    public final MessageType buildPartial() {
      if (!instance.isMutable()) {
        return instance;
      }

      instance.extensions.makeImmutable();
      return super.buildPartial();
    }

    private void verifyExtensionContainingType(final GeneratedExtension<MessageType, ?> extension) {
      if (extension.getContainingTypeDefaultInstance() != getDefaultInstanceForType()) {
        // This can only happen if someone uses unchecked operations.
        throw new IllegalArgumentException(
            "This extension is for a different message type.  Please make "
                + "sure that you are not suppressing any generics type warnings.");
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
    public final <Type> Type getExtension(final ExtensionLite<MessageType, Type> extension) {
      return instance.getExtension(extension);
    }

    /** Get one element of a repeated extension. */
    @Override
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, List<Type>> extension, final int index) {
      return instance.getExtension(extension, index);
    }

    /** Set the value of an extension. */
    public final <Type> BuilderType setExtension(
        final ExtensionLite<MessageType, Type> extension, final Type value) {
      GeneratedExtension<MessageType, Type> extensionLite = checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      copyOnWrite();
      ensureExtensionsAreMutable()
          .setField(extensionLite.descriptor, extensionLite.toFieldSetType(value));
      return (BuilderType) this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        final ExtensionLite<MessageType, List<Type>> extension, final int index, final Type value) {
      GeneratedExtension<MessageType, List<Type>> extensionLite = checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      copyOnWrite();
      ensureExtensionsAreMutable()
          .setRepeatedField(
              extensionLite.descriptor, index, extensionLite.singularToFieldSetType(value));
      return (BuilderType) this;
    }

    /** Append a value to a repeated extension. */
    public final <Type> BuilderType addExtension(
        final ExtensionLite<MessageType, List<Type>> extension, final Type value) {
      GeneratedExtension<MessageType, List<Type>> extensionLite = checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      copyOnWrite();
      ensureExtensionsAreMutable()
          .addRepeatedField(extensionLite.descriptor, extensionLite.singularToFieldSetType(value));
      return (BuilderType) this;
    }

    /** Clear an extension. */
    public final BuilderType clearExtension(final ExtensionLite<MessageType, ?> extension) {
      GeneratedExtension<MessageType, ?> extensionLite = checkIsLite(extension);

      verifyExtensionContainingType(extensionLite);
      copyOnWrite();
      ensureExtensionsAreMutable().clearField(extensionLite.descriptor);
      return (BuilderType) this;
    }
  }

  // -----------------------------------------------------------------

  /** For use by generated code only. */
  public static <ContainingType extends MessageLite, Type>
      GeneratedExtension<ContainingType, Type> newSingularGeneratedExtension(
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
        new ExtensionDescriptor(
            enumTypeMap, number, type, /* isRepeated= */ false, /* isPacked= */ false),
        singularType);
  }

  /** For use by generated code only. */
  public static <ContainingType extends MessageLite, Type>
      GeneratedExtension<ContainingType, Type> newRepeatedGeneratedExtension(
          final ContainingType containingTypeDefaultInstance,
          final MessageLite messageDefaultInstance,
          final Internal.EnumLiteMap<?> enumTypeMap,
          final int number,
          final WireFormat.FieldType type,
          final boolean isPacked,
          final Class singularType) {
    @SuppressWarnings("unchecked") // Subclasses ensure Type is a List
    Type emptyList = (Type) ProtobufArrayList.emptyList();
    return new GeneratedExtension<ContainingType, Type>(
        containingTypeDefaultInstance,
        emptyList,
        messageDefaultInstance,
        new ExtensionDescriptor(enumTypeMap, number, type, /* isRepeated= */ true, isPacked),
        singularType);
  }

  static final class ExtensionDescriptor
      implements FieldSet.FieldDescriptorLite<ExtensionDescriptor> {
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
          "Generated message class \"" + clazz.getName() + "\" missing method \"" + name + "\".",
          e);
    }
  }

  /** Calls invoke and throws a RuntimeException if it fails. */
  static Object invokeOrDie(Method method, Object object, Object... params) {
    try {
      return method.invoke(object, params);
    } catch (IllegalAccessException e) {
      throw new RuntimeException(
          "Couldn't use Java reflection to implement protocol message reflection.", e);
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
   * <p>Users should ignore the contents of this class and only use objects of this type as
   * parameters to extension accessors and ExtensionRegistry.add().
   */
  public static class GeneratedExtension<ContainingType extends MessageLite, Type>
      extends ExtensionLite<ContainingType, Type> {

    /**
     * Create a new instance with the given parameters.
     *
     * <p>The last parameter {@code singularType} is only needed for enum types. We store integer
     * values for enum types in a {@link ExtendableMessage} and use Java reflection to convert an
     * integer value back into a concrete enum object.
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
        throw new IllegalArgumentException("Null containingTypeDefaultInstance");
      }
      if (descriptor.getLiteType() == WireFormat.FieldType.MESSAGE
          && messageDefaultInstance == null) {
        throw new IllegalArgumentException("Null messageDefaultInstance");
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

    /** Default instance of the type being extended, used to identify that type. */
    public ContainingType getContainingTypeDefaultInstance() {
      return containingTypeDefaultInstance;
    }

    /** Get the field number. */
    @Override
    public int getNumber() {
      return descriptor.getNumber();
    }

    /**
     * If the extension is an embedded message or group, returns the default instance of the
     * message.
     */
    @Override
    public MessageLite getMessageDefaultInstance() {
      return messageDefaultInstance;
    }

    Object fromFieldSetType(Object value) {
      if (descriptor.isRepeated()) {
        if (descriptor.getLiteJavaType() == WireFormat.JavaType.ENUM) {
          ProtobufArrayList<Object> result = new ProtobufArrayList<>();
          result.ensureCapacity(((List<?>) value).size());
          for (Object element : (List) value) {
            result.add(singularFromFieldSetType(element));
          }
          result.makeImmutable();
          return result;
        } else {
          return value;
        }
      } else {
        return singularFromFieldSetType(value);
      }
    }

    Object singularFromFieldSetType(Object value) {
      if (descriptor.getLiteJavaType() == WireFormat.JavaType.ENUM) {
        return descriptor.enumTypeMap.findValueByNumber((Integer) value);
      } else {
        return value;
      }
    }

    Object toFieldSetType(Object value) {
      if (descriptor.isRepeated()) {
        if (descriptor.getLiteJavaType() == WireFormat.JavaType.ENUM) {
          List<Object> result = new ArrayList<>();
          for (Object element : (List) value) {
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

    Object singularToFieldSetType(Object value) {
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
   * A serialized (serializable) form of the generated message. Stores the message as a class name
   * and a byte array.
   */
  protected static final class SerializedForm implements Serializable {

    public static SerializedForm of(MessageLite message) {
      return new SerializedForm(message);
    }

    private static final long serialVersionUID = 0L;

    // since v3.6.1
    private final Class<?> messageClass;
    private final String messageClassName;
    private final byte[] asBytes;

    /**
     * Creates the serialized form by calling {@link com.google.protobuf.MessageLite#toByteArray}.
     *
     * @param regularForm the message to serialize
     */
    SerializedForm(MessageLite regularForm) {
      messageClass = regularForm.getClass();
      messageClassName = regularForm.getClass().getName();
      asBytes = regularForm.toByteArray();
    }

    /**
     * When read from an ObjectInputStream, this method converts this object back to the regular
     * form. Part of Java's serialization magic.
     *
     * @return a GeneratedMessage of the type that was serialized
     */
    protected Object readResolve() throws ObjectStreamException {
      try {
        Class<?> messageClass = resolveMessageClass();
        java.lang.reflect.Field defaultInstanceField =
            messageClass.getDeclaredField("DEFAULT_INSTANCE");
        defaultInstanceField.setAccessible(true);
        MessageLite.Builder builder =
            ((MessageLite) defaultInstanceField.get(null)).newBuilderForType();
        builder.mergeFrom(asBytes);
        return builder.buildPartial();
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

    private Class<?> resolveMessageClass() throws ClassNotFoundException {
      return messageClass != null ? messageClass : Class.forName(messageClassName);
    }
  }

  /** Checks that the {@link Extension} is Lite and returns it as a {@link GeneratedExtension}. */
  private static <
          MessageType extends ExtendableMessage<MessageType, BuilderType>,
          BuilderType extends ExtendableBuilder<MessageType, BuilderType>,
          T>
      GeneratedExtension<MessageType, T> checkIsLite(ExtensionLite<MessageType, T> extension) {
    if (!extension.isLite()) {
      throw new IllegalArgumentException("Expected a lite extension.");
    }

    return (GeneratedExtension<MessageType, T>) extension;
  }

  /** A static helper method for checking if a message is initialized, optionally memoizing. */
  private static final <T extends GeneratedMessageLite<T, ?>> boolean isInitialized(
      T message, boolean shouldMemoize) {
    byte memoizedIsInitialized =
        (Byte) message.dynamicMethod(MethodToInvoke.GET_MEMOIZED_IS_INITIALIZED, null, null);
    if (memoizedIsInitialized == 1) {
      return true;
    }
    if (memoizedIsInitialized == 0) {
      return false;
    }
    boolean isInitialized = Protobuf.getInstance().schemaFor(message).isInitialized(message);
    if (shouldMemoize) {
      // TODO: remove the unused variable
      Object unused =
          message.dynamicMethod(
              MethodToInvoke.SET_MEMOIZED_IS_INITIALIZED, isInitialized ? message : null, null);
    }
    return isInitialized;
  }

  protected static IntList emptyIntList() {
    return IntArrayList.emptyList();
  }

  protected static IntList mutableCopy(IntList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(size * 2);
  }

  protected static LongList emptyLongList() {
    return LongArrayList.emptyList();
  }

  protected static LongList mutableCopy(LongList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(size * 2);
  }

  protected static FloatList emptyFloatList() {
    return FloatArrayList.emptyList();
  }

  protected static FloatList mutableCopy(FloatList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(size * 2);
  }

  protected static DoubleList emptyDoubleList() {
    return DoubleArrayList.emptyList();
  }

  protected static DoubleList mutableCopy(DoubleList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(size * 2);
  }

  protected static BooleanList emptyBooleanList() {
    return BooleanArrayList.emptyList();
  }

  protected static BooleanList mutableCopy(BooleanList list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(size * 2);
  }

  protected static <E> ProtobufList<E> emptyProtobufList() {
    return ProtobufArrayList.emptyList();
  }

  protected static <E> ProtobufList<E> mutableCopy(ProtobufList<E> list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(size * 2);
  }

  /**
   * A {@link Parser} implementation that delegates to the default instance.
   *
   * <p>For use by generated code only.
   */
  protected static class DefaultInstanceBasedParser<T extends GeneratedMessageLite<T, ?>>
      extends AbstractParser<T> {

    private final T defaultInstance;

    public DefaultInstanceBasedParser(T defaultInstance) {
      this.defaultInstance = defaultInstance;
    }

    @Override
    public T parsePartialFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return GeneratedMessageLite.parsePartialFrom(defaultInstance, input, extensionRegistry);
    }

    @Override
    public T parsePartialFrom(
        byte[] input, int offset, int length, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return GeneratedMessageLite.parsePartialFrom(
          defaultInstance, input, offset, length, extensionRegistry);
    }
  }

  /**
   * A static helper method for parsing a partial from input using the extension registry and the
   * instance.
   */
  // TODO: Should this verify that the last tag was 0?
  static <T extends GeneratedMessageLite<T, ?>> T parsePartialFrom(
      T instance, CodedInputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    T result = instance.newMutableInstance();
    try {
      // TODO: Try to make input with type CodedInputStream.ArrayDecoder use
      // fast path.
      Schema<T> schema = Protobuf.getInstance().schemaFor(result);
      schema.mergeFrom(result, CodedInputStreamReader.forCodedInput(input), extensionRegistry);
      schema.makeImmutable(result);
    } catch (InvalidProtocolBufferException e) {
      if (e.getThrownFromInputStream()) {
        e = new InvalidProtocolBufferException(e);
      }
      throw e.setUnfinishedMessage(result);
    } catch (UninitializedMessageException e) {
      throw e.asInvalidProtocolBufferException().setUnfinishedMessage(result);
    } catch (IOException e) {
      if (e.getCause() instanceof InvalidProtocolBufferException) {
        throw (InvalidProtocolBufferException) e.getCause();
      }
      throw new InvalidProtocolBufferException(e).setUnfinishedMessage(result);
    } catch (RuntimeException e) {
      if (e.getCause() instanceof InvalidProtocolBufferException) {
        throw (InvalidProtocolBufferException) e.getCause();
      }
      throw e;
    }
    return result;
  }

  /** A static helper method for parsing a partial from byte array. */
  private static <T extends GeneratedMessageLite<T, ?>> T parsePartialFrom(
      T defaultInstance,
      byte[] input,
      int offset,
      int length,
      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    if (length == 0) {
      return defaultInstance;
    }
    T result = defaultInstance.newMutableInstance();
    try {
      Schema<T> schema = Protobuf.getInstance().schemaFor(result);
      schema.mergeFrom(
          result, input, offset, offset + length, new ArrayDecoders.Registers(extensionRegistry));
      schema.makeImmutable(result);
    } catch (InvalidProtocolBufferException e) {
      if (e.getThrownFromInputStream()) {
        e = new InvalidProtocolBufferException(e);
      }
      throw e.setUnfinishedMessage(result);
    } catch (UninitializedMessageException e) {
      throw e.asInvalidProtocolBufferException().setUnfinishedMessage(result);
    } catch (IOException e) {
      if (e.getCause() instanceof InvalidProtocolBufferException) {
        throw (InvalidProtocolBufferException) e.getCause();
      }
      throw new InvalidProtocolBufferException(e).setUnfinishedMessage(result);
    } catch (IndexOutOfBoundsException e) {
      throw InvalidProtocolBufferException.truncatedMessage().setUnfinishedMessage(result);
    }
    return result;
  }

  protected static <T extends GeneratedMessageLite<T, ?>> T parsePartialFrom(
      T defaultInstance, CodedInputStream input) throws InvalidProtocolBufferException {
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
      throw message
          .newUninitializedMessageException()
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
      T defaultInstance, ByteString data) throws InvalidProtocolBufferException {
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
    CodedInputStream input = data.newCodedInput();
    T message = parsePartialFrom(defaultInstance, input, extensionRegistry);
    try {
      input.checkLastTagWas(0);
    } catch (InvalidProtocolBufferException e) {
      throw e.setUnfinishedMessage(message);
    }
    return message;
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, byte[] data) throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialFrom(
            defaultInstance, data, 0, data.length, ExtensionRegistryLite.getEmptyRegistry()));
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, byte[] data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialFrom(defaultInstance, data, 0, data.length, extensionRegistry));
  }

  // Does not validate last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, InputStream input) throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialFrom(
            defaultInstance,
            CodedInputStream.newInstance(input),
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
      T defaultInstance, CodedInputStream input) throws InvalidProtocolBufferException {
    return parseFrom(defaultInstance, input, ExtensionRegistryLite.getEmptyRegistry());
  }

  // Does not validate last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseFrom(
      T defaultInstance, CodedInputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(parsePartialFrom(defaultInstance, input, extensionRegistry));
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseDelimitedFrom(
      T defaultInstance, InputStream input) throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialDelimitedFrom(
            defaultInstance, input, ExtensionRegistryLite.getEmptyRegistry()));
  }

  // Validates last tag.
  protected static <T extends GeneratedMessageLite<T, ?>> T parseDelimitedFrom(
      T defaultInstance, InputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return checkMessageInitialized(
        parsePartialDelimitedFrom(defaultInstance, input, extensionRegistry));
  }

  private static <T extends GeneratedMessageLite<T, ?>> T parsePartialDelimitedFrom(
      T defaultInstance, InputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    int size;
    try {
      int firstByte = input.read();
      if (firstByte == -1) {
        return null;
      }
      size = CodedInputStream.readRawVarint32(firstByte, input);
    } catch (InvalidProtocolBufferException e) {
      if (e.getThrownFromInputStream()) {
        e = new InvalidProtocolBufferException(e);
      }
      throw e;
    } catch (IOException e) {
      throw new InvalidProtocolBufferException(e);
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
}
