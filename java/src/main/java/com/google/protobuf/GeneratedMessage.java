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
import java.io.ObjectStreamException;
import java.io.Serializable;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
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
public abstract class GeneratedMessage extends AbstractMessage
    implements Serializable {
  private static final long serialVersionUID = 1L;

  /**
   * For testing. Allows a test to disable the optimization that avoids using
   * field builders for nested messages until they are requested. By disabling
   * this optimization, existing tests can be reused to test the field builders.
   */
  protected static boolean alwaysUseFieldBuilders = false;

  protected GeneratedMessage() {
  }

  protected GeneratedMessage(Builder<?> builder) {
  }

  public Parser<? extends Message> getParserForType() {
    throw new UnsupportedOperationException(
        "This is supposed to be overridden by subclasses.");
  }

 /**
  * For testing. Allows a test to disable the optimization that avoids using
  * field builders for nested messages until they are requested. By disabling
  * this optimization, existing tests can be reused to test the field builders.
  * See {@link RepeatedFieldBuilder} and {@link SingleFieldBuilder}.
  */
  static void enableAlwaysUseFieldBuildersForTesting() {
    alwaysUseFieldBuilders = true;
  }

  /**
   * Get the FieldAccessorTable for this type.  We can't have the message
   * class pass this in to the constructor because of bootstrapping trouble
   * with DescriptorProtos.
   */
  protected abstract FieldAccessorTable internalGetFieldAccessorTable();

  //@Override (Java 1.6 override semantics, but we must support 1.5)
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
        final List<?> value = (List<?>) getField(field);
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

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public Map<FieldDescriptor, Object> getAllFields() {
    return Collections.unmodifiableMap(getAllFieldsMutable());
  }

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public boolean hasField(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).has(this);
  }

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public Object getField(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).get(this);
  }

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public int getRepeatedFieldCount(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field)
      .getRepeatedCount(this);
  }

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public Object getRepeatedField(final FieldDescriptor field, final int index) {
    return internalGetFieldAccessorTable().getField(field)
      .getRepeated(this, index);
  }

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public UnknownFieldSet getUnknownFields() {
    throw new UnsupportedOperationException(
        "This is supposed to be overridden by subclasses.");
  }

  /**
   * Called by subclasses to parse an unknown field.
   * @return {@code true} unless the tag is an end-group tag.
   */
  protected boolean parseUnknownField(
      CodedInputStream input,
      UnknownFieldSet.Builder unknownFields,
      ExtensionRegistryLite extensionRegistry,
      int tag) throws IOException {
    return unknownFields.mergeFieldFrom(tag, input);
  }

  /**
   * Used by parsing constructors in generated classes.
   */
  protected void makeExtensionsImmutable() {
    // Noop for messages without extensions.
  }

  protected abstract Message.Builder newBuilderForType(BuilderParent parent);

  /**
   * Interface for the parent of a Builder that allows the builder to
   * communicate invalidations back to the parent for use when using nested
   * builders.
   */
  protected interface BuilderParent {

    /**
     * A builder becomes dirty whenever a field is modified -- including fields
     * in nested builders -- and becomes clean when build() is called.  Thus,
     * when a builder becomes dirty, all its parents become dirty as well, and
     * when it becomes clean, all its children become clean.  The dirtiness
     * state is used to invalidate certain cached values.
     * <br>
     * To this end, a builder calls markAsDirty() on its parent whenever it
     * transitions from clean to dirty.  The parent must propagate this call to
     * its own parent, unless it was already dirty, in which case the
     * grandparent must necessarily already be dirty as well.  The parent can
     * only transition back to "clean" after calling build() on all children.
     */
    void markDirty();
  }

  @SuppressWarnings("unchecked")
  public abstract static class Builder <BuilderType extends Builder>
      extends AbstractMessage.Builder<BuilderType> {

    private BuilderParent builderParent;

    private BuilderParentImpl meAsParent;

    // Indicates that we've built a message and so we are now obligated
    // to dispatch dirty invalidations. See GeneratedMessage.BuilderListener.
    private boolean isClean;

    private UnknownFieldSet unknownFields =
        UnknownFieldSet.getDefaultInstance();

    protected Builder() {
      this(null);
    }

    protected Builder(BuilderParent builderParent) {
      this.builderParent = builderParent;
    }

    void dispose() {
      builderParent = null;
    }

    /**
     * Called by the subclass when a message is built.
     */
    protected void onBuilt() {
      if (builderParent != null) {
        markClean();
      }
    }

    /**
     * Called by the subclass or a builder to notify us that a message was
     * built and may be cached and therefore invalidations are needed.
     */
    protected void markClean() {
      this.isClean = true;
    }

    /**
     * Gets whether invalidations are needed
     *
     * @return whether invalidations are needed
     */
    protected boolean isClean() {
      return isClean;
    }

    // This is implemented here only to work around an apparent bug in the
    // Java compiler and/or build system.  See bug #1898463.  The mere presence
    // of this dummy clone() implementation makes it go away.
    @Override
    public BuilderType clone() {
      throw new UnsupportedOperationException(
          "This is supposed to be overridden by subclasses.");
    }

    /**
     * Called by the initialization and clear code paths to allow subclasses to
     * reset any of their builtin fields back to the initial values.
     */
    public BuilderType clear() {
      unknownFields = UnknownFieldSet.getDefaultInstance();
      onChanged();
      return (BuilderType) this;
    }

    /**
     * Get the FieldAccessorTable for this type.  We can't have the message
     * class pass this in to the constructor because of bootstrapping trouble
     * with DescriptorProtos.
     */
    protected abstract FieldAccessorTable internalGetFieldAccessorTable();

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public Descriptor getDescriptorForType() {
      return internalGetFieldAccessorTable().descriptor;
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public Map<FieldDescriptor, Object> getAllFields() {
      return Collections.unmodifiableMap(getAllFieldsMutable());
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

    public Message.Builder newBuilderForField(
        final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).newBuilder();
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public Message.Builder getFieldBuilder(final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).getBuilder(this);
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public boolean hasField(final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).has(this);
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public Object getField(final FieldDescriptor field) {
      Object object = internalGetFieldAccessorTable().getField(field).get(this);
      if (field.isRepeated()) {
        // The underlying list object is still modifiable at this point.
        // Make sure not to expose the modifiable list to the caller.
        return Collections.unmodifiableList((List) object);
      } else {
        return object;
      }
    }

    public BuilderType setField(final FieldDescriptor field,
                                final Object value) {
      internalGetFieldAccessorTable().getField(field).set(this, value);
      return (BuilderType) this;
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public BuilderType clearField(final FieldDescriptor field) {
      internalGetFieldAccessorTable().getField(field).clear(this);
      return (BuilderType) this;
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public int getRepeatedFieldCount(final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field)
          .getRepeatedCount(this);
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public Object getRepeatedField(final FieldDescriptor field,
                                   final int index) {
      return internalGetFieldAccessorTable().getField(field)
          .getRepeated(this, index);
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

    public final BuilderType setUnknownFields(
        final UnknownFieldSet unknownFields) {
      this.unknownFields = unknownFields;
      onChanged();
      return (BuilderType) this;
    }

    @Override
    public final BuilderType mergeUnknownFields(
        final UnknownFieldSet unknownFields) {
      this.unknownFields =
        UnknownFieldSet.newBuilder(this.unknownFields)
                       .mergeFrom(unknownFields)
                       .build();
      onChanged();
      return (BuilderType) this;
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
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
            if (hasField(field) &&
                !((Message) getField(field)).isInitialized()) {
              return false;
            }
          }
        }
      }
      return true;
    }

    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final UnknownFieldSet getUnknownFields() {
      return unknownFields;
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

    /**
     * Implementation of {@link BuilderParent} for giving to our children. This
     * small inner class makes it so we don't publicly expose the BuilderParent
     * methods.
     */
    private class BuilderParentImpl implements BuilderParent {

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public void markDirty() {
        onChanged();
      }
    }

    /**
     * Gets the {@link BuilderParent} for giving to our children.
     * @return The builder parent for our children.
     */
    protected BuilderParent getParentForChildren() {
      if (meAsParent == null) {
        meAsParent = new BuilderParentImpl();
      }
      return meAsParent;
    }

    /**
     * Called when a the builder or one of its nested children has changed
     * and any parent should be notified of its invalidation.
     */
    protected final void onChanged() {
      if (isClean && builderParent != null) {
        builderParent.markDirty();

        // Don't keep dispatching invalidations until build is called again.
        isClean = false;
      }
    }
  }

  // =================================================================
  // Extensions-related stuff

  public interface ExtendableMessageOrBuilder<
      MessageType extends ExtendableMessage> extends MessageOrBuilder {

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
      extends GeneratedMessage
      implements ExtendableMessageOrBuilder<MessageType> {

    private final FieldSet<FieldDescriptor> extensions;

    protected ExtendableMessage() {
      this.extensions = FieldSet.newFieldSet();
    }

    protected ExtendableMessage(
        ExtendableBuilder<MessageType, ?> builder) {
      super(builder);
      this.extensions = builder.buildExtensions();
    }

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
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> boolean hasExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.getDescriptor());
    }

    /** Get the number of elements in a repeated extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      return extensions.getRepeatedFieldCount(descriptor);
    }

    /** Get the value of an extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      final Object value = extensions.getField(descriptor);
      if (value == null) {
        if (descriptor.isRepeated()) {
          return (Type) Collections.emptyList();
        } else if (descriptor.getJavaType() ==
                   FieldDescriptor.JavaType.MESSAGE) {
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
    //@Override (Java 1.6 override semantics, but we must support 1.5)
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

    @Override
    protected boolean parseUnknownField(
        CodedInputStream input,
        UnknownFieldSet.Builder unknownFields,
        ExtensionRegistryLite extensionRegistry,
        int tag) throws IOException {
      return AbstractMessage.Builder.mergeFieldFrom(
        input, unknownFields, extensionRegistry, getDescriptorForType(),
        null, extensions, tag);
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
            if (next instanceof LazyField.LazyEntry<?>) {
              output.writeRawMessageSetExtension(descriptor.getNumber(),
                  ((LazyField.LazyEntry<?>) next).getField().toByteString());
            } else {
              output.writeMessageSetExtension(descriptor.getNumber(),
                                              (Message) next.getValue());
            }
          } else {
            // TODO(xiangl): Taken care of following code, it may cause
            // problem when we use LazyField for normal fields/extensions.
            // Due to the optional field can be duplicated at the end of
            // serialized bytes, which will make the serialized size change
            // after lazy field parsed. So when we use LazyField globally,
            // we need to change the following write method to write cached
            // bytes directly rather than write the parsed message.
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

    protected Map<FieldDescriptor, Object> getExtensionFields() {
      return extensions.getAllFields();
    }

    @Override
    public Map<FieldDescriptor, Object> getAllFields() {
      final Map<FieldDescriptor, Object> result = super.getAllFieldsMutable();
      result.putAll(getExtensionFields());
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
      extends Builder<BuilderType>
      implements ExtendableMessageOrBuilder<MessageType> {

    private FieldSet<FieldDescriptor> extensions = FieldSet.emptySet();

    protected ExtendableBuilder() {}

    protected ExtendableBuilder(
        BuilderParent parent) {
      super(parent);
    }

    @Override
    public BuilderType clear() {
      extensions = FieldSet.emptySet();
      return super.clear();
    }

    // This is implemented here only to work around an apparent bug in the
    // Java compiler and/or build system.  See bug #1898463.  The mere presence
    // of this dummy clone() implementation makes it go away.
    @Override
    public BuilderType clone() {
      throw new UnsupportedOperationException(
          "This is supposed to be overridden by subclasses.");
    }

    private void ensureExtensionsIsMutable() {
      if (extensions.isImmutable()) {
        extensions = extensions.clone();
      }
    }

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
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> boolean hasExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.getDescriptor());
    }

    /** Get the number of elements in a repeated extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      return extensions.getRepeatedFieldCount(descriptor);
    }

    /** Get the value of an extension. */
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      final Object value = extensions.getField(descriptor);
      if (value == null) {
        if (descriptor.isRepeated()) {
          return (Type) Collections.emptyList();
        } else if (descriptor.getJavaType() ==
                   FieldDescriptor.JavaType.MESSAGE) {
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
    //@Override (Java 1.6 override semantics, but we must support 1.5)
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index) {
      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      return (Type) extension.singularFromReflectionType(
          extensions.getRepeatedField(descriptor, index));
    }

    /** Set the value of an extension. */
    public final <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, Type> extension,
        final Type value) {
      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      final FieldDescriptor descriptor = extension.getDescriptor();
      extensions.setField(descriptor, extension.toReflectionType(value));
      onChanged();
      return (BuilderType) this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index, final Type value) {
      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      final FieldDescriptor descriptor = extension.getDescriptor();
      extensions.setRepeatedField(
        descriptor, index,
        extension.singularToReflectionType(value));
      onChanged();
      return (BuilderType) this;
    }

    /** Append a value to a repeated extension. */
    public final <Type> BuilderType addExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final Type value) {
      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      final FieldDescriptor descriptor = extension.getDescriptor();
      extensions.addRepeatedField(
          descriptor, extension.singularToReflectionType(value));
      onChanged();
      return (BuilderType) this;
    }

    /** Clear an extension. */
    public final <Type> BuilderType clearExtension(
        final GeneratedExtension<MessageType, ?> extension) {
      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      extensions.clearField(extension.getDescriptor());
      onChanged();
      return (BuilderType) this;
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions.isInitialized();
    }

    /**
     * Called by the build code path to create a copy of the extensions for
     * building the message.
     */
    private FieldSet<FieldDescriptor> buildExtensions() {
      extensions.makeImmutable();
      return extensions;
    }

    @Override
    public boolean isInitialized() {
      return super.isInitialized() && extensionsAreInitialized();
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
      return AbstractMessage.Builder.mergeFieldFrom(
        input, unknownFields, extensionRegistry, getDescriptorForType(),
        this, null, tag);
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
    public BuilderType setField(final FieldDescriptor field,
                                final Object value) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        extensions.setField(field, value);
        onChanged();
        return (BuilderType) this;
      } else {
        return super.setField(field, value);
      }
    }

    @Override
    public BuilderType clearField(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        extensions.clearField(field);
        onChanged();
        return (BuilderType) this;
      } else {
        return super.clearField(field);
      }
    }

    @Override
    public BuilderType setRepeatedField(final FieldDescriptor field,
                                        final int index, final Object value) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        extensions.setRepeatedField(field, index, value);
        onChanged();
        return (BuilderType) this;
      } else {
        return super.setRepeatedField(field, index, value);
      }
    }

    @Override
    public BuilderType addRepeatedField(final FieldDescriptor field,
                                        final Object value) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        extensions.addRepeatedField(field, value);
        onChanged();
        return (BuilderType) this;
      } else {
        return super.addRepeatedField(field, value);
      }
    }

    protected final void mergeExtensionFields(final ExtendableMessage other) {
      ensureExtensionsIsMutable();
      extensions.mergeFrom(other.extensions);
      onChanged();
    }

    private void verifyContainingType(final FieldDescriptor field) {
      if (field.getContainingType() != getDescriptorForType()) {
        throw new IllegalArgumentException(
          "FieldDescriptor does not match message type.");
      }
    }
  }

  // -----------------------------------------------------------------

  /**
   * Gets the descriptor for an extension. The implementation depends on whether
   * the extension is scoped in the top level of a file or scoped in a Message.
   */
  private static interface ExtensionDescriptorRetriever {
    FieldDescriptor getDescriptor();
  }

  /** For use by generated code only. */
  public static <ContainingType extends Message, Type>
      GeneratedExtension<ContainingType, Type>
      newMessageScopedGeneratedExtension(final Message scope,
                                         final int descriptorIndex,
                                         final Class singularType,
                                         final Message defaultInstance) {
    // For extensions scoped within a Message, we use the Message to resolve
    // the outer class's descriptor, from which the extension descriptor is
    // obtained.
    return new GeneratedExtension<ContainingType, Type>(
        new ExtensionDescriptorRetriever() {
          //@Override (Java 1.6 override semantics, but we must support 1.5)
          public FieldDescriptor getDescriptor() {
            return scope.getDescriptorForType().getExtensions()
                .get(descriptorIndex);
          }
        },
        singularType,
        defaultInstance);
  }

  /** For use by generated code only. */
  public static <ContainingType extends Message, Type>
     GeneratedExtension<ContainingType, Type>
     newFileScopedGeneratedExtension(final Class singularType,
                                     final Message defaultInstance) {
    // For extensions scoped within a file, we rely on the outer class's
    // static initializer to call internalInit() on the extension when the
    // descriptor is available.
    return new GeneratedExtension<ContainingType, Type>(
        null,  // ExtensionDescriptorRetriever is initialized in internalInit();
        singularType,
        defaultInstance);
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

    // We can't always initialize the descriptor of a GeneratedExtension when
    // we first construct it due to initialization order difficulties (namely,
    // the descriptor may not have been constructed yet, since it is often
    // constructed by the initializer of a separate module).
    //
    // In the case of nested extensions, we initialize the
    // ExtensionDescriptorRetriever with an instance that uses the scoping
    // Message's default instance to retrieve the extension's descriptor.
    //
    // In the case of non-nested extensions, we initialize the
    // ExtensionDescriptorRetriever to null and rely on the outer class's static
    // initializer to call internalInit() after the descriptor has been parsed.
    private GeneratedExtension(ExtensionDescriptorRetriever descriptorRetriever,
                               Class singularType,
                               Message messageDefaultInstance) {
      if (Message.class.isAssignableFrom(singularType) &&
          !singularType.isInstance(messageDefaultInstance)) {
        throw new IllegalArgumentException(
            "Bad messageDefaultInstance for " + singularType.getName());
      }
      this.descriptorRetriever = descriptorRetriever;
      this.singularType = singularType;
      this.messageDefaultInstance = messageDefaultInstance;

      if (ProtocolMessageEnum.class.isAssignableFrom(singularType)) {
        this.enumValueOf = getMethodOrDie(singularType, "valueOf",
                                          EnumValueDescriptor.class);
        this.enumGetValueDescriptor =
            getMethodOrDie(singularType, "getValueDescriptor");
      } else {
        this.enumValueOf = null;
        this.enumGetValueDescriptor = null;
      }
    }

    /** For use by generated code only. */
    public void internalInit(final FieldDescriptor descriptor) {
      if (descriptorRetriever != null) {
        throw new IllegalStateException("Already initialized.");
      }
      descriptorRetriever = new ExtensionDescriptorRetriever() {
          //@Override (Java 1.6 override semantics, but we must support 1.5)
          public FieldDescriptor getDescriptor() {
            return descriptor;
          }
        };
    }

    private ExtensionDescriptorRetriever descriptorRetriever;
    private final Class singularType;
    private final Message messageDefaultInstance;
    private final Method enumValueOf;
    private final Method enumGetValueDescriptor;

    public FieldDescriptor getDescriptor() {
      if (descriptorRetriever == null) {
        throw new IllegalStateException(
            "getDescriptor() called before internalInit()");
      }
      return descriptorRetriever.getDescriptor();
    }

    /**
     * If the extension is an embedded message or group, returns the default
     * instance of the message.
     */
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
      FieldDescriptor descriptor = getDescriptor();
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
      FieldDescriptor descriptor = getDescriptor();
      switch (descriptor.getJavaType()) {
        case MESSAGE:
          if (singularType.isInstance(value)) {
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
      FieldDescriptor descriptor = getDescriptor();
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
      FieldDescriptor descriptor = getDescriptor();
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
      this(descriptor, camelCaseNames);
      ensureFieldAccessorsInitialized(messageClass, builderClass);
    }

    /**
     * Construct a FieldAccessorTable for a particular message class without
     * initializing FieldAccessors.
     */
    public FieldAccessorTable(
        final Descriptor descriptor,
        final String[] camelCaseNames) {
      this.descriptor = descriptor;
      this.camelCaseNames = camelCaseNames;
      fields = new FieldAccessor[descriptor.getFields().size()];
      initialized = false;
    }

    /**
     * Ensures the field accessors are initialized. This method is thread-safe.
     *
     * @param messageClass   The message type.
     * @param builderClass   The builder type.
     * @return this
     */
    public FieldAccessorTable ensureFieldAccessorsInitialized(
        Class<? extends GeneratedMessage> messageClass,
        Class<? extends Builder> builderClass) {
      if (initialized) { return this; }
      synchronized (this) {
        if (initialized) { return this; }
        for (int i = 0; i < fields.length; i++) {
          FieldDescriptor field = descriptor.getFields().get(i);
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
        initialized = true;
        camelCaseNames = null;
        return this;
      }
    }

    private final Descriptor descriptor;
    private final FieldAccessor[] fields;
    private String[] camelCaseNames;
    private volatile boolean initialized;

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
      Object get(GeneratedMessage.Builder builder);
      void set(Builder builder, Object value);
      Object getRepeated(GeneratedMessage message, int index);
      Object getRepeated(GeneratedMessage.Builder builder, int index);
      void setRepeated(Builder builder,
                       int index, Object value);
      void addRepeated(Builder builder, Object value);
      boolean has(GeneratedMessage message);
      boolean has(GeneratedMessage.Builder builder);
      int getRepeatedCount(GeneratedMessage message);
      int getRepeatedCount(GeneratedMessage.Builder builder);
      void clear(Builder builder);
      Message.Builder newBuilder();
      Message.Builder getBuilder(GeneratedMessage.Builder builder);
    }

    // ---------------------------------------------------------------

    private static class SingularFieldAccessor implements FieldAccessor {
      SingularFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        getMethod = getMethodOrDie(messageClass, "get" + camelCaseName);
        getMethodBuilder = getMethodOrDie(builderClass, "get" + camelCaseName);
        type = getMethod.getReturnType();
        setMethod = getMethodOrDie(builderClass, "set" + camelCaseName, type);
        hasMethod =
            getMethodOrDie(messageClass, "has" + camelCaseName);
        hasMethodBuilder =
            getMethodOrDie(builderClass, "has" + camelCaseName);
        clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
      }

      // Note:  We use Java reflection to call public methods rather than
      //   access private fields directly as this avoids runtime security
      //   checks.
      protected final Class<?> type;
      protected final Method getMethod;
      protected final Method getMethodBuilder;
      protected final Method setMethod;
      protected final Method hasMethod;
      protected final Method hasMethodBuilder;
      protected final Method clearMethod;

      public Object get(final GeneratedMessage message) {
        return invokeOrDie(getMethod, message);
      }
      public Object get(GeneratedMessage.Builder builder) {
        return invokeOrDie(getMethodBuilder, builder);
      }
      public void set(final Builder builder, final Object value) {
        invokeOrDie(setMethod, builder, value);
      }
      public Object getRepeated(final GeneratedMessage message,
                                final int index) {
        throw new UnsupportedOperationException(
          "getRepeatedField() called on a singular field.");
      }
      public Object getRepeated(GeneratedMessage.Builder builder, int index) {
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
      public boolean has(GeneratedMessage.Builder builder) {
        return (Boolean) invokeOrDie(hasMethodBuilder, builder);
      }
      public int getRepeatedCount(final GeneratedMessage message) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldSize() called on a singular field.");
      }
      public int getRepeatedCount(GeneratedMessage.Builder builder) {
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
      public Message.Builder getBuilder(GeneratedMessage.Builder builder) {
        throw new UnsupportedOperationException(
          "getFieldBuilder() called on a non-Message type.");
      }
    }

    private static class RepeatedFieldAccessor implements FieldAccessor {
      protected final Class type;
      protected final Method getMethod;
      protected final Method getMethodBuilder;
      protected final Method getRepeatedMethod;
      protected final Method getRepeatedMethodBuilder;
      protected final Method setRepeatedMethod;
      protected final Method addRepeatedMethod;
      protected final Method getCountMethod;
      protected final Method getCountMethodBuilder;
      protected final Method clearMethod;

      RepeatedFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        getMethod = getMethodOrDie(messageClass,
                                   "get" + camelCaseName + "List");
        getMethodBuilder = getMethodOrDie(builderClass,
                                   "get" + camelCaseName + "List");
        getRepeatedMethod =
            getMethodOrDie(messageClass, "get" + camelCaseName, Integer.TYPE);
        getRepeatedMethodBuilder =
            getMethodOrDie(builderClass, "get" + camelCaseName, Integer.TYPE);
        type = getRepeatedMethod.getReturnType();
        setRepeatedMethod =
            getMethodOrDie(builderClass, "set" + camelCaseName,
                           Integer.TYPE, type);
        addRepeatedMethod =
            getMethodOrDie(builderClass, "add" + camelCaseName, type);
        getCountMethod =
            getMethodOrDie(messageClass, "get" + camelCaseName + "Count");
        getCountMethodBuilder =
            getMethodOrDie(builderClass, "get" + camelCaseName + "Count");

        clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
      }

      public Object get(final GeneratedMessage message) {
        return invokeOrDie(getMethod, message);
      }
      public Object get(GeneratedMessage.Builder builder) {
        return invokeOrDie(getMethodBuilder, builder);
      }
      public void set(final Builder builder, final Object value) {
        // Add all the elements individually.  This serves two purposes:
        // 1) Verifies that each element has the correct type.
        // 2) Insures that the caller cannot modify the list later on and
        //    have the modifications be reflected in the message.
        clear(builder);
        for (final Object element : (List<?>) value) {
          addRepeated(builder, element);
        }
      }
      public Object getRepeated(final GeneratedMessage message,
                                final int index) {
        return invokeOrDie(getRepeatedMethod, message, index);
      }
      public Object getRepeated(GeneratedMessage.Builder builder, int index) {
        return invokeOrDie(getRepeatedMethodBuilder, builder, index);
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
          "hasField() called on a repeated field.");
      }
      public boolean has(GeneratedMessage.Builder builder) {
        throw new UnsupportedOperationException(
          "hasField() called on a repeated field.");
      }
      public int getRepeatedCount(final GeneratedMessage message) {
        return (Integer) invokeOrDie(getCountMethod, message);
      }
      public int getRepeatedCount(GeneratedMessage.Builder builder) {
        return (Integer) invokeOrDie(getCountMethodBuilder, builder);
      }
      public void clear(final Builder builder) {
        invokeOrDie(clearMethod, builder);
      }
      public Message.Builder newBuilder() {
        throw new UnsupportedOperationException(
          "newBuilderForField() called on a non-Message type.");
      }
      public Message.Builder getBuilder(GeneratedMessage.Builder builder) {
        throw new UnsupportedOperationException(
          "getFieldBuilder() called on a non-Message type.");
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
      public Object get(final GeneratedMessage.Builder builder) {
        return invokeOrDie(getValueDescriptorMethod, super.get(builder));
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
      @SuppressWarnings("unchecked")
      public Object get(final GeneratedMessage.Builder builder) {
        final List newList = new ArrayList();
        for (final Object element : (List) super.get(builder)) {
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
      public Object getRepeated(final GeneratedMessage.Builder builder,
                                final int index) {
        return invokeOrDie(getValueDescriptorMethod,
          super.getRepeated(builder, index));
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
        getBuilderMethodBuilder =
            getMethodOrDie(builderClass, "get" + camelCaseName + "Builder");
      }

      private final Method newBuilderMethod;
      private final Method getBuilderMethodBuilder;

      private Object coerceType(final Object value) {
        if (type.isInstance(value)) {
          return value;
        } else {
          // The value is not the exact right message type.  However, if it
          // is an alternative implementation of the same type -- e.g. a
          // DynamicMessage -- we should accept it.  In this case we can make
          // a copy of the message.
          return ((Message.Builder) invokeOrDie(newBuilderMethod, null))
                  .mergeFrom((Message) value).buildPartial();
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
      @Override
      public Message.Builder getBuilder(GeneratedMessage.Builder builder) {
        return (Message.Builder) invokeOrDie(getBuilderMethodBuilder, builder);
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

  /**
   * Replaces this object in the output stream with a serialized form.
   * Part of Java's serialization magic.  Generated sub-classes must override
   * this method by calling {@code return super.writeReplace();}
   * @return a SerializedForm of this message
   */
  protected Object writeReplace() throws ObjectStreamException {
    return new GeneratedMessageLite.SerializedForm(this);
  }
}
