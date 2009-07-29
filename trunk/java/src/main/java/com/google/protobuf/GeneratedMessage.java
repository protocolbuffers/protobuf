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
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;

import java.io.IOException;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * All generated protocol message classes extend this class.  This class
 * implements most of the Message and Builder interfaces using Java reflection.
 * Users can ignore this class and pretend that generated messages implement
 * the Message interface directly.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class GeneratedMessage extends AbstractMessage {
  protected GeneratedMessage() {}

  private UnknownFieldSet unknownFields = UnknownFieldSet.getDefaultInstance();

  /**
   * Get the FieldAccessorTable for this type.  We can't have the message
   * class pass this in to the constructor because of bootstrapping trouble
   * with DescriptorProtos.
   */
  protected abstract FieldAccessorTable internalGetFieldAccessorTable();

  public Descriptor getDescriptorForType() {
    return internalGetFieldAccessorTable().descriptor;
  }

  /** Internal helper which returns a mutable map. */
  private Map<FieldDescriptor, Object> getAllFieldsMutable() {
    final TreeMap<FieldDescriptor, Object> result =
      new TreeMap<FieldDescriptor, Object>();
    final Descriptor descriptor = internalGetFieldAccessorTable().descriptor;
    for (final FieldDescriptor field : descriptor.getFields()) {
      if (field.isRepeated()) {
        final List value = (List) getField(field);
        if (!value.isEmpty()) {
          result.put(field, value);
        }
      } else {
        if (hasField(field)) {
          result.put(field, getField(field));
        }
      }
    }
    return result;
  }

  @Override
  public boolean isInitialized() {
    for (final FieldDescriptor field : getDescriptorForType().getFields()) {
      // Check that all required fields are present.
      if (field.isRequired()) {
        if (!hasField(field)) {
          return false;
        }
      }
      // Check that embedded messages are initialized.
      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        if (field.isRepeated()) {
          @SuppressWarnings("unchecked") final
          List<Message> messageList = (List<Message>) getField(field);
          for (final Message element : messageList) {
            if (!element.isInitialized()) {
              return false;
            }
          }
        } else {
          if (hasField(field) && !((Message) getField(field)).isInitialized()) {
            return false;
          }
        }
      }
    }

    return true;
  }

  public Map<FieldDescriptor, Object> getAllFields() {
    return Collections.unmodifiableMap(getAllFieldsMutable());
  }

  public boolean hasField(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).has(this);
  }

  public Object getField(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).get(this);
  }

  public int getRepeatedFieldCount(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field)
      .getRepeatedCount(this);
  }

  public Object getRepeatedField(final FieldDescriptor field, final int index) {
    return internalGetFieldAccessorTable().getField(field)
      .getRepeated(this, index);
  }

  public final UnknownFieldSet getUnknownFields() {
    return unknownFields;
  }

  @SuppressWarnings("unchecked")
  public abstract static class Builder <BuilderType extends Builder>
      extends AbstractMessage.Builder<BuilderType> {
    protected Builder() {}

    // This is implemented here only to work around an apparent bug in the
    // Java compiler and/or build system.  See bug #1898463.  The mere presence
    // of this dummy clone() implementation makes it go away.
    @Override
    public BuilderType clone() {
      throw new UnsupportedOperationException(
          "This is supposed to be overridden by subclasses.");
    }

    /**
     * Get the message being built.  We don't just pass this to the
     * constructor because it becomes null when build() is called.
     */
    protected abstract GeneratedMessage internalGetResult();

    /**
     * Get the FieldAccessorTable for this type.  We can't have the message
     * class pass this in to the constructor because of bootstrapping trouble
     * with DescriptorProtos.
     */
    private FieldAccessorTable internalGetFieldAccessorTable() {
      return internalGetResult().internalGetFieldAccessorTable();
    }

    public Descriptor getDescriptorForType() {
      return internalGetFieldAccessorTable().descriptor;
    }

    public Map<FieldDescriptor, Object> getAllFields() {
      return internalGetResult().getAllFields();
    }

    public Message.Builder newBuilderForField(
        final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).newBuilder();
    }

    public boolean hasField(final FieldDescriptor field) {
      return internalGetResult().hasField(field);
    }

    public Object getField(final FieldDescriptor field) {
      if (field.isRepeated()) {
        // The underlying list object is still modifiable at this point.
        // Make sure not to expose the modifiable list to the caller.
        return Collections.unmodifiableList(
          (List) internalGetResult().getField(field));
      } else {
        return internalGetResult().getField(field);
      }
    }

    public BuilderType setField(final FieldDescriptor field,
                                final Object value) {
      internalGetFieldAccessorTable().getField(field).set(this, value);
      return (BuilderType) this;
    }

    public BuilderType clearField(final FieldDescriptor field) {
      internalGetFieldAccessorTable().getField(field).clear(this);
      return (BuilderType) this;
    }

    public int getRepeatedFieldCount(final FieldDescriptor field) {
      return internalGetResult().getRepeatedFieldCount(field);
    }

    public Object getRepeatedField(final FieldDescriptor field,
                                   final int index) {
      return internalGetResult().getRepeatedField(field, index);
    }

    public BuilderType setRepeatedField(final FieldDescriptor field,
                                        final int index, final Object value) {
      internalGetFieldAccessorTable().getField(field)
        .setRepeated(this, index, value);
      return (BuilderType) this;
    }

    public BuilderType addRepeatedField(final FieldDescriptor field,
                                        final Object value) {
      internalGetFieldAccessorTable().getField(field).addRepeated(this, value);
      return (BuilderType) this;
    }

    public final UnknownFieldSet getUnknownFields() {
      return internalGetResult().unknownFields;
    }

    public final BuilderType setUnknownFields(
        final UnknownFieldSet unknownFields) {
      internalGetResult().unknownFields = unknownFields;
      return (BuilderType) this;
    }

    @Override
    public final BuilderType mergeUnknownFields(
        final UnknownFieldSet unknownFields) {
      final GeneratedMessage result = internalGetResult();
      result.unknownFields =
        UnknownFieldSet.newBuilder(result.unknownFields)
                       .mergeFrom(unknownFields)
                       .build();
      return (BuilderType) this;
    }

    public boolean isInitialized() {
      return internalGetResult().isInitialized();
    }

    /**
     * Called by subclasses to parse an unknown field.
     * @return {@code true} unless the tag is an end-group tag.
     */
    protected boolean parseUnknownField(
        final CodedInputStream input,
        final UnknownFieldSet.Builder unknownFields,
        final ExtensionRegistryLite extensionRegistry,
        final int tag) throws IOException {
      return unknownFields.mergeFieldFrom(tag, input);
    }
  }

  // =================================================================
  // Extensions-related stuff

  /**
   * Generated message classes for message types that contain extension ranges
   * subclass this.
   *
   * <p>This class implements type-safe accessors for extensions.  They
   * implement all the same operations that you can do with normal fields --
   * e.g. "has", "get", and "getCount" -- but for extensions.  The extensions
   * are identified using instances of the class {@link GeneratedExtension};
   * the protocol compiler generates a static instance of this class for every
   * extension in its input.  Through the magic of generics, all is made
   * type-safe.
   *
   * <p>For example, imagine you have the {@code .proto} file:
   *
   * <pre>
   * option java_class = "MyProto";
   *
   * message Foo {
   *   extensions 1000 to max;
   * }
   *
   * extend Foo {
   *   optional int32 bar;
   * }
   * </pre>
   *
   * <p>Then you might write code like:
   *
   * <pre>
   * MyProto.Foo foo = getFoo();
   * int i = foo.getExtension(MyProto.bar);
   * </pre>
   *
   * <p>See also {@link ExtendableBuilder}.
   */
  public abstract static class ExtendableMessage<
        MessageType extends ExtendableMessage>
      extends GeneratedMessage {
    protected ExtendableMessage() {}
    private final FieldSet<FieldDescriptor> extensions = FieldSet.newFieldSet();

    private void verifyExtensionContainingType(
        final GeneratedExtension<MessageType, ?> extension) {
      if (extension.getDescriptor().getContainingType() !=
          getDescriptorForType()) {
        // This can only happen if someone uses unchecked operations.
        throw new IllegalArgumentException(
          "Extension is for type \"" +
          extension.getDescriptor().getContainingType().getFullName() +
          "\" which does not match message type \"" +
          getDescriptorForType().getFullName() + "\".");
      }
    }

    /** Check if a singular extension is present. */
    public final boolean hasExtension(
        final GeneratedExtension<MessageType, ?> extension) {
      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.getDescriptor());
    }

    /** Get the number of elements in a repeated extension. */
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      return extensions.getRepeatedFieldCount(descriptor);
    }

    /** Get the value of an extension. */
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      final Object value = extensions.getField(descriptor);
      if (value == null) {
        if (descriptor.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          return (Type) extension.getMessageDefaultInstance();
        } else {
          return (Type) extension.fromReflectionType(
              descriptor.getDefaultValue());
        }
      } else {
        return (Type) extension.fromReflectionType(value);
      }
    }

    /** Get one element of a repeated extension. */
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index) {
      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      return (Type) extension.singularFromReflectionType(
          extensions.getRepeatedField(descriptor, index));
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions.isInitialized();
    }

    @Override
    public boolean isInitialized() {
      return super.isInitialized() && extensionsAreInitialized();
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

      private final Iterator<Map.Entry<FieldDescriptor, Object>> iter =
        extensions.iterator();
      private Map.Entry<FieldDescriptor, Object> next;
      private final boolean messageSetWireFormat;

      private ExtensionWriter(final boolean messageSetWireFormat) {
        if (iter.hasNext()) {
          next = iter.next();
        }
        this.messageSetWireFormat = messageSetWireFormat;
      }

      public void writeUntil(final int end, final CodedOutputStream output)
                             throws IOException {
        while (next != null && next.getKey().getNumber() < end) {
          FieldDescriptor descriptor = next.getKey();
          if (messageSetWireFormat && descriptor.getLiteJavaType() ==
                  WireFormat.JavaType.MESSAGE &&
              !descriptor.isRepeated()) {
            output.writeMessageSetExtension(descriptor.getNumber(),
                                            (Message) next.getValue());
          } else {
            FieldSet.writeField(descriptor, next.getValue(), output);
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

    // ---------------------------------------------------------------
    // Reflection

    @Override
    public Map<FieldDescriptor, Object> getAllFields() {
      final Map<FieldDescriptor, Object> result = super.getAllFieldsMutable();
      result.putAll(extensions.getAllFields());
      return Collections.unmodifiableMap(result);
    }

    @Override
    public boolean hasField(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions.hasField(field);
      } else {
        return super.hasField(field);
      }
    }

    @Override
    public Object getField(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        final Object value = extensions.getField(field);
        if (value == null) {
          if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
            // Lacking an ExtensionRegistry, we have no way to determine the
            // extension's real type, so we return a DynamicMessage.
            return DynamicMessage.getDefaultInstance(field.getMessageType());
          } else {
            return field.getDefaultValue();
          }
        } else {
          return value;
        }
      } else {
        return super.getField(field);
      }
    }

    @Override
    public int getRepeatedFieldCount(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions.getRepeatedFieldCount(field);
      } else {
        return super.getRepeatedFieldCount(field);
      }
    }

    @Override
    public Object getRepeatedField(final FieldDescriptor field,
                                   final int index) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions.getRepeatedField(field, index);
      } else {
        return super.getRepeatedField(field, index);
      }
    }

    private void verifyContainingType(final FieldDescriptor field) {
      if (field.getContainingType() != getDescriptorForType()) {
        throw new IllegalArgumentException(
          "FieldDescriptor does not match message type.");
      }
    }
  }

  /**
   * Generated message builders for message types that contain extension ranges
   * subclass this.
   *
   * <p>This class implements type-safe accessors for extensions.  They
   * implement all the same operations that you can do with normal fields --
   * e.g. "get", "set", and "add" -- but for extensions.  The extensions are
   * identified using instances of the class {@link GeneratedExtension}; the
   * protocol compiler generates a static instance of this class for every
   * extension in its input.  Through the magic of generics, all is made
   * type-safe.
   *
   * <p>For example, imagine you have the {@code .proto} file:
   *
   * <pre>
   * option java_class = "MyProto";
   *
   * message Foo {
   *   extensions 1000 to max;
   * }
   *
   * extend Foo {
   *   optional int32 bar;
   * }
   * </pre>
   *
   * <p>Then you might write code like:
   *
   * <pre>
   * MyProto.Foo foo =
   *   MyProto.Foo.newBuilder()
   *     .setExtension(MyProto.bar, 123)
   *     .build();
   * </pre>
   *
   * <p>See also {@link ExtendableMessage}.
   */
  @SuppressWarnings("unchecked")
  public abstract static class ExtendableBuilder<
        MessageType extends ExtendableMessage,
        BuilderType extends ExtendableBuilder>
      extends Builder<BuilderType> {
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
    protected abstract ExtendableMessage<MessageType> internalGetResult();

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
      final FieldDescriptor descriptor = extension.getDescriptor();
      message.extensions.setField(descriptor,
                                  extension.toReflectionType(value));
      return (BuilderType) this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index, final Type value) {
      final ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      message.extensions.setRepeatedField(
        descriptor, index,
        extension.singularToReflectionType(value));
      return (BuilderType) this;
    }

    /** Append a value to a repeated extension. */
    public final <Type> BuilderType addExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final Type value) {
      final ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      message.extensions.addRepeatedField(
          descriptor, extension.singularToReflectionType(value));
      return (BuilderType) this;
    }

    /** Clear an extension. */
    public final <Type> BuilderType clearExtension(
        final GeneratedExtension<MessageType, ?> extension) {
      final ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.clearField(extension.getDescriptor());
      return (BuilderType) this;
    }

    /**
     * Called by subclasses to parse an unknown field or an extension.
     * @return {@code true} unless the tag is an end-group tag.
     */
    @Override
    protected boolean parseUnknownField(
        final CodedInputStream input,
        final UnknownFieldSet.Builder unknownFields,
        final ExtensionRegistryLite extensionRegistry,
        final int tag) throws IOException {
      final ExtendableMessage<MessageType> message = internalGetResult();
      return AbstractMessage.Builder.mergeFieldFrom(
        input, unknownFields, extensionRegistry, this, tag);
    }

    // ---------------------------------------------------------------
    // Reflection

    // We don't have to override the get*() methods here because they already
    // just forward to the underlying message.

    @Override
    public BuilderType setField(final FieldDescriptor field,
                                final Object value) {
      if (field.isExtension()) {
        final ExtendableMessage<MessageType> message = internalGetResult();
        message.verifyContainingType(field);
        message.extensions.setField(field, value);
        return (BuilderType) this;
      } else {
        return super.setField(field, value);
      }
    }

    @Override
    public BuilderType clearField(final FieldDescriptor field) {
      if (field.isExtension()) {
        final ExtendableMessage<MessageType> message = internalGetResult();
        message.verifyContainingType(field);
        message.extensions.clearField(field);
        return (BuilderType) this;
      } else {
        return super.clearField(field);
      }
    }

    @Override
    public BuilderType setRepeatedField(final FieldDescriptor field,
                                        final int index, final Object value) {
      if (field.isExtension()) {
        final ExtendableMessage<MessageType> message = internalGetResult();
        message.verifyContainingType(field);
        message.extensions.setRepeatedField(field, index, value);
        return (BuilderType) this;
      } else {
        return super.setRepeatedField(field, index, value);
      }
    }

    @Override
    public BuilderType addRepeatedField(final FieldDescriptor field,
                                        final Object value) {
      if (field.isExtension()) {
        final ExtendableMessage<MessageType> message = internalGetResult();
        message.verifyContainingType(field);
        message.extensions.addRepeatedField(field, value);
        return (BuilderType) this;
      } else {
        return super.addRepeatedField(field, value);
      }
    }

    protected final void mergeExtensionFields(final ExtendableMessage other) {
      internalGetResult().extensions.mergeFrom(other.extensions);
    }
  }

  // -----------------------------------------------------------------

  /** For use by generated code only. */
  public static <ContainingType extends Message, Type>
      GeneratedExtension<ContainingType, Type>
      newGeneratedExtension(final FieldDescriptor descriptor,
                            final Class<Type> type) {
    if (descriptor.isRepeated()) {
      throw new IllegalArgumentException(
        "Must call newRepeatedGeneratedExtension() for repeated types.");
    }
    return new GeneratedExtension<ContainingType, Type>(descriptor, type);
  }

  /** For use by generated code only. */
  public static <ContainingType extends Message, Type>
      GeneratedExtension<ContainingType, List<Type>>
      newRepeatedGeneratedExtension(
        final FieldDescriptor descriptor, final Class<Type> type) {
    if (!descriptor.isRepeated()) {
      throw new IllegalArgumentException(
        "Must call newGeneratedExtension() for non-repeated types.");
    }
    return new GeneratedExtension<ContainingType, List<Type>>(descriptor, type);
  }

  /**
   * Type used to represent generated extensions.  The protocol compiler
   * generates a static singleton instance of this class for each extension.
   *
   * <p>For example, imagine you have the {@code .proto} file:
   *
   * <pre>
   * option java_class = "MyProto";
   *
   * message Foo {
   *   extensions 1000 to max;
   * }
   *
   * extend Foo {
   *   optional int32 bar;
   * }
   * </pre>
   *
   * <p>Then, {@code MyProto.Foo.bar} has type
   * {@code GeneratedExtension<MyProto.Foo, Integer>}.
   *
   * <p>In general, users should ignore the details of this type, and simply use
   * these static singletons as parameters to the extension accessors defined
   * in {@link ExtendableMessage} and {@link ExtendableBuilder}.
   */
  public static final class GeneratedExtension<
      ContainingType extends Message, Type> {
    // TODO(kenton):  Find ways to avoid using Java reflection within this
    //   class.  Also try to avoid suppressing unchecked warnings.

    private GeneratedExtension(final FieldDescriptor descriptor,
                               final Class type) {
      if (!descriptor.isExtension()) {
        throw new IllegalArgumentException(
          "GeneratedExtension given a regular (non-extension) field.");
      }

      this.descriptor = descriptor;
      this.type = type;

      switch (descriptor.getJavaType()) {
        case MESSAGE:
          enumValueOf = null;
          enumGetValueDescriptor = null;
          messageDefaultInstance =
            (Message) invokeOrDie(getMethodOrDie(type, "getDefaultInstance"),
                                  null);
          break;
        case ENUM:
          enumValueOf = getMethodOrDie(type, "valueOf",
                                       EnumValueDescriptor.class);
          enumGetValueDescriptor = getMethodOrDie(type, "getValueDescriptor");
          messageDefaultInstance = null;
          break;
        default:
          enumValueOf = null;
          enumGetValueDescriptor = null;
          messageDefaultInstance = null;
          break;
      }
    }

    private final FieldDescriptor descriptor;
    private final Class type;
    private final Method enumValueOf;
    private final Method enumGetValueDescriptor;
    private final Message messageDefaultInstance;

    public FieldDescriptor getDescriptor() { return descriptor; }

    /**
     * If the extension is an embedded message or group, returns the default
     * instance of the message.
     */
    @SuppressWarnings("unchecked")
    public Message getMessageDefaultInstance() {
      return messageDefaultInstance;
    }

    /**
     * Convert from the type used by the reflection accessors to the type used
     * by native accessors.  E.g., for enums, the reflection accessors use
     * EnumValueDescriptors but the native accessors use the generated enum
     * type.
     */
    @SuppressWarnings("unchecked")
    private Object fromReflectionType(final Object value) {
      if (descriptor.isRepeated()) {
        if (descriptor.getJavaType() == FieldDescriptor.JavaType.MESSAGE ||
            descriptor.getJavaType() == FieldDescriptor.JavaType.ENUM) {
          // Must convert the whole list.
          final List result = new ArrayList();
          for (final Object element : (List) value) {
            result.add(singularFromReflectionType(element));
          }
          return result;
        } else {
          return value;
        }
      } else {
        return singularFromReflectionType(value);
      }
    }

    /**
     * Like {@link #fromReflectionType(Object)}, but if the type is a repeated
     * type, this converts a single element.
     */
    private Object singularFromReflectionType(final Object value) {
      switch (descriptor.getJavaType()) {
        case MESSAGE:
          if (type.isInstance(value)) {
            return value;
          } else {
            // It seems the copy of the embedded message stored inside the
            // extended message is not of the exact type the user was
            // expecting.  This can happen if a user defines a
            // GeneratedExtension manually and gives it a different type.
            // This should not happen in normal use.  But, to be nice, we'll
            // copy the message to whatever type the caller was expecting.
            return messageDefaultInstance.newBuilderForType()
                           .mergeFrom((Message) value).build();
          }
        case ENUM:
          return invokeOrDie(enumValueOf, null, (EnumValueDescriptor) value);
        default:
          return value;
      }
    }

    /**
     * Convert from the type used by the native accessors to the type used
     * by reflection accessors.  E.g., for enums, the reflection accessors use
     * EnumValueDescriptors but the native accessors use the generated enum
     * type.
     */
    @SuppressWarnings("unchecked")
    private Object toReflectionType(final Object value) {
      if (descriptor.isRepeated()) {
        if (descriptor.getJavaType() == FieldDescriptor.JavaType.ENUM) {
          // Must convert the whole list.
          final List result = new ArrayList();
          for (final Object element : (List) value) {
            result.add(singularToReflectionType(element));
          }
          return result;
        } else {
          return value;
        }
      } else {
        return singularToReflectionType(value);
      }
    }

    /**
     * Like {@link #toReflectionType(Object)}, but if the type is a repeated
     * type, this converts a single element.
     */
    private Object singularToReflectionType(final Object value) {
      switch (descriptor.getJavaType()) {
        case ENUM:
          return invokeOrDie(enumGetValueDescriptor, value);
        default:
          return value;
      }
    }
  }

  // =================================================================

  /** Calls Class.getMethod and throws a RuntimeException if it fails. */
  @SuppressWarnings("unchecked")
  private static Method getMethodOrDie(
      final Class clazz, final String name, final Class... params) {
    try {
      return clazz.getMethod(name, params);
    } catch (NoSuchMethodException e) {
      throw new RuntimeException(
        "Generated message class \"" + clazz.getName() +
        "\" missing method \"" + name + "\".", e);
    }
  }

  /** Calls invoke and throws a RuntimeException if it fails. */
  private static Object invokeOrDie(
      final Method method, final Object object, final Object... params) {
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
   * Users should ignore this class.  This class provides the implementation
   * with access to the fields of a message object using Java reflection.
   */
  public static final class FieldAccessorTable {

    /**
     * Construct a FieldAccessorTable for a particular message class.  Only
     * one FieldAccessorTable should ever be constructed per class.
     *
     * @param descriptor     The type's descriptor.
     * @param camelCaseNames The camelcase names of all fields in the message.
     *                       These are used to derive the accessor method names.
     * @param messageClass   The message type.
     * @param builderClass   The builder type.
     */
    public FieldAccessorTable(
        final Descriptor descriptor,
        final String[] camelCaseNames,
        final Class<? extends GeneratedMessage> messageClass,
        final Class<? extends Builder> builderClass) {
      this.descriptor = descriptor;
      fields = new FieldAccessor[descriptor.getFields().size()];

      for (int i = 0; i < fields.length; i++) {
        final FieldDescriptor field = descriptor.getFields().get(i);
        if (field.isRepeated()) {
          if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
            fields[i] = new RepeatedMessageFieldAccessor(
              field, camelCaseNames[i], messageClass, builderClass);
          } else if (field.getJavaType() == FieldDescriptor.JavaType.ENUM) {
            fields[i] = new RepeatedEnumFieldAccessor(
              field, camelCaseNames[i], messageClass, builderClass);
          } else {
            fields[i] = new RepeatedFieldAccessor(
              field, camelCaseNames[i], messageClass, builderClass);
          }
        } else {
          if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
            fields[i] = new SingularMessageFieldAccessor(
              field, camelCaseNames[i], messageClass, builderClass);
          } else if (field.getJavaType() == FieldDescriptor.JavaType.ENUM) {
            fields[i] = new SingularEnumFieldAccessor(
              field, camelCaseNames[i], messageClass, builderClass);
          } else {
            fields[i] = new SingularFieldAccessor(
              field, camelCaseNames[i], messageClass, builderClass);
          }
        }
      }
    }

    private final Descriptor descriptor;
    private final FieldAccessor[] fields;

    /** Get the FieldAccessor for a particular field. */
    private FieldAccessor getField(final FieldDescriptor field) {
      if (field.getContainingType() != descriptor) {
        throw new IllegalArgumentException(
          "FieldDescriptor does not match message type.");
      } else if (field.isExtension()) {
        // If this type had extensions, it would subclass ExtendableMessage,
        // which overrides the reflection interface to handle extensions.
        throw new IllegalArgumentException(
          "This type does not have extensions.");
      }
      return fields[field.getIndex()];
    }

    /**
     * Abstract interface that provides access to a single field.  This is
     * implemented differently depending on the field type and cardinality.
     */
    private interface FieldAccessor {
      Object get(GeneratedMessage message);
      void set(Builder builder, Object value);
      Object getRepeated(GeneratedMessage message, int index);
      void setRepeated(Builder builder,
                       int index, Object value);
      void addRepeated(Builder builder, Object value);
      boolean has(GeneratedMessage message);
      int getRepeatedCount(GeneratedMessage message);
      void clear(Builder builder);
      Message.Builder newBuilder();
    }

    // ---------------------------------------------------------------

    private static class SingularFieldAccessor implements FieldAccessor {
      SingularFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        getMethod = getMethodOrDie(messageClass, "get" + camelCaseName);
        type = getMethod.getReturnType();
        setMethod = getMethodOrDie(builderClass, "set" + camelCaseName, type);
        hasMethod =
          getMethodOrDie(messageClass, "has" + camelCaseName);
        clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
      }

      // Note:  We use Java reflection to call public methods rather than
      //   access private fields directly as this avoids runtime security
      //   checks.
      protected final Class type;
      protected final Method getMethod;
      protected final Method setMethod;
      protected final Method hasMethod;
      protected final Method clearMethod;

      public Object get(final GeneratedMessage message) {
        return invokeOrDie(getMethod, message);
      }
      public void set(final Builder builder, final Object value) {
        invokeOrDie(setMethod, builder, value);
      }
      public Object getRepeated(final GeneratedMessage message,
                                final int index) {
        throw new UnsupportedOperationException(
          "getRepeatedField() called on a singular field.");
      }
      public void setRepeated(final Builder builder,
                              final int index, final Object value) {
        throw new UnsupportedOperationException(
          "setRepeatedField() called on a singular field.");
      }
      public void addRepeated(final Builder builder, final Object value) {
        throw new UnsupportedOperationException(
          "addRepeatedField() called on a singular field.");
      }
      public boolean has(final GeneratedMessage message) {
        return (Boolean) invokeOrDie(hasMethod, message);
      }
      public int getRepeatedCount(final GeneratedMessage message) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldSize() called on a singular field.");
      }
      public void clear(final Builder builder) {
        invokeOrDie(clearMethod, builder);
      }
      public Message.Builder newBuilder() {
        throw new UnsupportedOperationException(
          "newBuilderForField() called on a non-Message type.");
      }
    }

    private static class RepeatedFieldAccessor implements FieldAccessor {
      RepeatedFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        getMethod = getMethodOrDie(messageClass,
                                   "get" + camelCaseName + "List");

        getRepeatedMethod =
          getMethodOrDie(messageClass, "get" + camelCaseName, Integer.TYPE);
        type = getRepeatedMethod.getReturnType();
        setRepeatedMethod =
          getMethodOrDie(builderClass, "set" + camelCaseName,
                         Integer.TYPE, type);
        addRepeatedMethod =
          getMethodOrDie(builderClass, "add" + camelCaseName, type);
        getCountMethod =
          getMethodOrDie(messageClass, "get" + camelCaseName + "Count");

        clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
      }

      protected final Class type;
      protected final Method getMethod;
      protected final Method getRepeatedMethod;
      protected final Method setRepeatedMethod;
      protected final Method addRepeatedMethod;
      protected final Method getCountMethod;
      protected final Method clearMethod;

      public Object get(final GeneratedMessage message) {
        return invokeOrDie(getMethod, message);
      }
      public void set(final Builder builder, final Object value) {
        // Add all the elements individually.  This serves two purposes:
        // 1) Verifies that each element has the correct type.
        // 2) Insures that the caller cannot modify the list later on and
        //    have the modifications be reflected in the message.
        clear(builder);
        for (final Object element : (List) value) {
          addRepeated(builder, element);
        }
      }
      public Object getRepeated(final GeneratedMessage message,
                                final int index) {
        return invokeOrDie(getRepeatedMethod, message, index);
      }
      public void setRepeated(final Builder builder,
                              final int index, final Object value) {
        invokeOrDie(setRepeatedMethod, builder, index, value);
      }
      public void addRepeated(final Builder builder, final Object value) {
        invokeOrDie(addRepeatedMethod, builder, value);
      }
      public boolean has(final GeneratedMessage message) {
        throw new UnsupportedOperationException(
          "hasField() called on a singular field.");
      }
      public int getRepeatedCount(final GeneratedMessage message) {
        return (Integer) invokeOrDie(getCountMethod, message);
      }
      public void clear(final Builder builder) {
        invokeOrDie(clearMethod, builder);
      }
      public Message.Builder newBuilder() {
        throw new UnsupportedOperationException(
          "newBuilderForField() called on a non-Message type.");
      }
    }

    // ---------------------------------------------------------------

    private static final class SingularEnumFieldAccessor
        extends SingularFieldAccessor {
      SingularEnumFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        valueOfMethod = getMethodOrDie(type, "valueOf",
                                       EnumValueDescriptor.class);
        getValueDescriptorMethod =
          getMethodOrDie(type, "getValueDescriptor");
      }

      private Method valueOfMethod;
      private Method getValueDescriptorMethod;

      @Override
      public Object get(final GeneratedMessage message) {
        return invokeOrDie(getValueDescriptorMethod, super.get(message));
      }
      @Override
      public void set(final Builder builder, final Object value) {
        super.set(builder, invokeOrDie(valueOfMethod, null, value));
      }
    }

    private static final class RepeatedEnumFieldAccessor
        extends RepeatedFieldAccessor {
      RepeatedEnumFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        valueOfMethod = getMethodOrDie(type, "valueOf",
                                       EnumValueDescriptor.class);
        getValueDescriptorMethod =
          getMethodOrDie(type, "getValueDescriptor");
      }

      private final Method valueOfMethod;
      private final Method getValueDescriptorMethod;

      @Override
      @SuppressWarnings("unchecked")
      public Object get(final GeneratedMessage message) {
        final List newList = new ArrayList();
        for (final Object element : (List) super.get(message)) {
          newList.add(invokeOrDie(getValueDescriptorMethod, element));
        }
        return Collections.unmodifiableList(newList);
      }
      @Override
      public Object getRepeated(final GeneratedMessage message,
                                final int index) {
        return invokeOrDie(getValueDescriptorMethod,
          super.getRepeated(message, index));
      }
      @Override
      public void setRepeated(final Builder builder,
                              final int index, final Object value) {
        super.setRepeated(builder, index, invokeOrDie(valueOfMethod, null,
                          value));
      }
      @Override
      public void addRepeated(final Builder builder, final Object value) {
        super.addRepeated(builder, invokeOrDie(valueOfMethod, null, value));
      }
    }

    // ---------------------------------------------------------------

    private static final class SingularMessageFieldAccessor
        extends SingularFieldAccessor {
      SingularMessageFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        newBuilderMethod = getMethodOrDie(type, "newBuilder");
      }

      private final Method newBuilderMethod;

      private Object coerceType(final Object value) {
        if (type.isInstance(value)) {
          return value;
        } else {
          // The value is not the exact right message type.  However, if it
          // is an alternative implementation of the same type -- e.g. a
          // DynamicMessage -- we should accept it.  In this case we can make
          // a copy of the message.
          return ((Message.Builder) invokeOrDie(newBuilderMethod, null))
                  .mergeFrom((Message) value).build();
        }
      }

      @Override
      public void set(final Builder builder, final Object value) {
        super.set(builder, coerceType(value));
      }
      @Override
      public Message.Builder newBuilder() {
        return (Message.Builder) invokeOrDie(newBuilderMethod, null);
      }
    }

    private static final class RepeatedMessageFieldAccessor
        extends RepeatedFieldAccessor {
      RepeatedMessageFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        newBuilderMethod = getMethodOrDie(type, "newBuilder");
      }

      private final Method newBuilderMethod;

      private Object coerceType(final Object value) {
        if (type.isInstance(value)) {
          return value;
        } else {
          // The value is not the exact right message type.  However, if it
          // is an alternative implementation of the same type -- e.g. a
          // DynamicMessage -- we should accept it.  In this case we can make
          // a copy of the message.
          return ((Message.Builder) invokeOrDie(newBuilderMethod, null))
                  .mergeFrom((Message) value).build();
        }
      }

      @Override
      public void setRepeated(final Builder builder,
                              final int index, final Object value) {
        super.setRepeated(builder, index, coerceType(value));
      }
      @Override
      public void addRepeated(final Builder builder, final Object value) {
        super.addRepeated(builder, coerceType(value));
      }
      @Override
      public Message.Builder newBuilder() {
        return (Message.Builder) invokeOrDie(newBuilderMethod, null);
      }
    }
  }
}
