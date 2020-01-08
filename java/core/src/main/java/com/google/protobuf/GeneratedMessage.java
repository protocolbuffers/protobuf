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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;

import java.io.IOException;
import java.io.InputStream;
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

  /** For use by generated code only.  */
  protected UnknownFieldSet unknownFields;

  protected GeneratedMessage() {
    unknownFields = UnknownFieldSet.getDefaultInstance();
  }

  protected GeneratedMessage(Builder<?> builder) {
    unknownFields = builder.getUnknownFields();
  }

  @Override
  public Parser<? extends GeneratedMessage> getParserForType() {
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

  @Override
  public Descriptor getDescriptorForType() {
    return internalGetFieldAccessorTable().descriptor;
  }

  /**
   * Internal helper to return a modifiable map containing all the fields.
   * The returned Map is modifialbe so that the caller can add additional
   * extension fields to implement {@link #getAllFields()}.
   *
   * @param getBytesForString whether to generate ByteString for string fields
   */
  private Map<FieldDescriptor, Object> getAllFieldsMutable(
      boolean getBytesForString) {
    final TreeMap<FieldDescriptor, Object> result =
      new TreeMap<FieldDescriptor, Object>();
    final Descriptor descriptor = internalGetFieldAccessorTable().descriptor;
    final List<FieldDescriptor> fields = descriptor.getFields();

    for (int i = 0; i < fields.size(); i++) {
      FieldDescriptor field = fields.get(i);
      final OneofDescriptor oneofDescriptor = field.getContainingOneof();

      /*
       * If the field is part of a Oneof, then at maximum one field in the Oneof is set
       * and it is not repeated. There is no need to iterate through the others.
       */
      if (oneofDescriptor != null) {
        // Skip other fields in the Oneof we know are not set
        i += oneofDescriptor.getFieldCount() - 1;
        if (!hasOneof(oneofDescriptor)) {
          // If no field is set in the Oneof, skip all the fields in the Oneof
          continue;
        }
        // Get the pointer to the only field which is set in the Oneof
        field = getOneofFieldDescriptor(oneofDescriptor);
      } else {
        // If we are not in a Oneof, we need to check if the field is set and if it is repeated
        if (field.isRepeated()) {
          final List<?> value = (List<?>) getField(field);
          if (!value.isEmpty()) {
            result.put(field, value);
          }
          continue;
        }
        if (!hasField(field)) {
          continue;
        }
      }
      // Add the field to the map
      if (getBytesForString && field.getJavaType() == FieldDescriptor.JavaType.STRING) {
        result.put(field, getFieldRaw(field));
      } else {
        result.put(field, getField(field));
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

  @Override
  public Map<FieldDescriptor, Object> getAllFields() {
    return Collections.unmodifiableMap(
        getAllFieldsMutable(/* getBytesForString = */ false));
  }

  /**
   * Returns a collection of all the fields in this message which are set
   * and their corresponding values.  A singular ("required" or "optional")
   * field is set iff hasField() returns true for that field.  A "repeated"
   * field is set iff getRepeatedFieldCount() is greater than zero.  The
   * values are exactly what would be returned by calling
   * {@link #getFieldRaw(Descriptors.FieldDescriptor)} for each field.  The map
   * is guaranteed to be a sorted map, so iterating over it will return fields
   * in order by field number.
   */
  Map<FieldDescriptor, Object> getAllFieldsRaw() {
    return Collections.unmodifiableMap(
        getAllFieldsMutable(/* getBytesForString = */ true));
  }

  @Override
  public boolean hasOneof(final OneofDescriptor oneof) {
    return internalGetFieldAccessorTable().getOneof(oneof).has(this);
  }

  @Override
  public FieldDescriptor getOneofFieldDescriptor(final OneofDescriptor oneof) {
    return internalGetFieldAccessorTable().getOneof(oneof).get(this);
  }

  @Override
  public boolean hasField(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).has(this);
  }

  @Override
  public Object getField(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).get(this);
  }

  /**
   * Obtains the value of the given field, or the default value if it is
   * not set.  For primitive fields, the boxed primitive value is returned.
   * For enum fields, the EnumValueDescriptor for the value is returned. For
   * embedded message fields, the sub-message is returned.  For repeated
   * fields, a java.util.List is returned. For present string fields, a
   * ByteString is returned representing the bytes that the field contains.
   */
  Object getFieldRaw(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).getRaw(this);
  }

  @Override
  public int getRepeatedFieldCount(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field)
      .getRepeatedCount(this);
  }

  @Override
  public Object getRepeatedField(final FieldDescriptor field, final int index) {
    return internalGetFieldAccessorTable().getField(field)
      .getRepeated(this, index);
  }

  @Override
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

  protected static <M extends Message> M parseWithIOException(Parser<M> parser, InputStream input)
      throws IOException {
    try {
      return parser.parseFrom(input);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  protected static <M extends Message> M parseWithIOException(Parser<M> parser, InputStream input,
      ExtensionRegistryLite extensions) throws IOException {
    try {
      return parser.parseFrom(input, extensions);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  protected static <M extends Message> M parseWithIOException(Parser<M> parser,
      CodedInputStream input) throws IOException {
    try {
      return parser.parseFrom(input);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  protected static <M extends Message> M parseWithIOException(Parser<M> parser,
      CodedInputStream input, ExtensionRegistryLite extensions) throws IOException {
    try {
      return parser.parseFrom(input, extensions);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  protected static <M extends Message> M parseDelimitedWithIOException(Parser<M> parser,
      InputStream input) throws IOException {
    try {
      return parser.parseDelimitedFrom(input);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  protected static <M extends Message> M parseDelimitedWithIOException(Parser<M> parser,
      InputStream input, ExtensionRegistryLite extensions) throws IOException {
    try {
      return parser.parseDelimitedFrom(input, extensions);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  @Override
  public void writeTo(final CodedOutputStream output) throws IOException {
    MessageReflection.writeMessageTo(this, getAllFieldsRaw(), output, false);
  }

  @Override
  public int getSerializedSize() {
    int size = memoizedSize;
    if (size != -1) {
      return size;
    }

    memoizedSize = MessageReflection.getSerializedSize(
        this, getAllFieldsRaw());
    return memoizedSize;
  }



  /**
   * Used by parsing constructors in generated classes.
   */
  protected void makeExtensionsImmutable() {
    // Noop for messages without extensions.
  }

  /**
   * TODO(xiaofeng): remove this after b/29368482 is fixed. We need to move this
   * interface to AbstractMessage in order to versioning GeneratedMessage but
   * this move breaks binary compatibility for AppEngine. After AppEngine is
   * fixed we can exclude this from google3.
   */
  protected interface BuilderParent extends AbstractMessage.BuilderParent {}

  /**
   * TODO(xiaofeng): remove this together with GeneratedMessage.BuilderParent.
   */
  protected abstract Message.Builder newBuilderForType(BuilderParent parent);

  @Override
  protected Message.Builder newBuilderForType(final AbstractMessage.BuilderParent parent) {
    return newBuilderForType(new BuilderParent() {
      @Override
      public void markDirty() {
        parent.markDirty();
      }
    });
  }


  @SuppressWarnings("unchecked")
  public abstract static class Builder <BuilderType extends Builder<BuilderType>>
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

    @Override
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
    @Override
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

    @Override
    public BuilderType clone() {
      BuilderType builder =
          (BuilderType) getDefaultInstanceForType().newBuilderForType();
      builder.mergeFrom(buildPartial());
      return builder;
    }

    /**
     * Called by the initialization and clear code paths to allow subclasses to
     * reset any of their builtin fields back to the initial values.
     */
    @Override
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

    @Override
    public Descriptor getDescriptorForType() {
      return internalGetFieldAccessorTable().descriptor;
    }

    @Override
    public Map<FieldDescriptor, Object> getAllFields() {
      return Collections.unmodifiableMap(getAllFieldsMutable());
    }

    /** Internal helper which returns a mutable map. */
    private Map<FieldDescriptor, Object> getAllFieldsMutable() {
      final TreeMap<FieldDescriptor, Object> result =
        new TreeMap<FieldDescriptor, Object>();
      final Descriptor descriptor = internalGetFieldAccessorTable().descriptor;
      final List<FieldDescriptor> fields = descriptor.getFields();

      for (int i = 0; i < fields.size(); i++) {
        FieldDescriptor field = fields.get(i);
        final OneofDescriptor oneofDescriptor = field.getContainingOneof();

        /*
         * If the field is part of a Oneof, then at maximum one field in the Oneof is set
         * and it is not repeated. There is no need to iterate through the others.
         */
        if (oneofDescriptor != null) {
          // Skip other fields in the Oneof we know are not set
          i += oneofDescriptor.getFieldCount() - 1;
          if (!hasOneof(oneofDescriptor)) {
            // If no field is set in the Oneof, skip all the fields in the Oneof
            continue;
          }
          // Get the pointer to the only field which is set in the Oneof
          field = getOneofFieldDescriptor(oneofDescriptor);
        } else {
          // If we are not in a Oneof, we need to check if the field is set and if it is repeated
          if (field.isRepeated()) {
            final List<?> value = (List<?>) getField(field);
            if (!value.isEmpty()) {
              result.put(field, value);
            }
            continue;
          }
          if (!hasField(field)) {
            continue;
          }
        }
        // Add the field to the map
        result.put(field, getField(field));
      }
      return result;
    }

    @Override
    public Message.Builder newBuilderForField(final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).newBuilder();
    }

    @Override
    public Message.Builder getFieldBuilder(final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).getBuilder(this);
    }

    @Override
    public Message.Builder getRepeatedFieldBuilder(final FieldDescriptor field, int index) {
      return internalGetFieldAccessorTable().getField(field).getRepeatedBuilder(
          this, index);
    }

    @Override
    public boolean hasOneof(final OneofDescriptor oneof) {
      return internalGetFieldAccessorTable().getOneof(oneof).has(this);
    }

    @Override
    public FieldDescriptor getOneofFieldDescriptor(final OneofDescriptor oneof) {
      return internalGetFieldAccessorTable().getOneof(oneof).get(this);
    }

    @Override
    public boolean hasField(final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).has(this);
    }

    @Override
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

    @Override
    public BuilderType setField(final FieldDescriptor field, final Object value) {
      internalGetFieldAccessorTable().getField(field).set(this, value);
      return (BuilderType) this;
    }

    @Override
    public BuilderType clearField(final FieldDescriptor field) {
      internalGetFieldAccessorTable().getField(field).clear(this);
      return (BuilderType) this;
    }

    @Override
    public BuilderType clearOneof(final OneofDescriptor oneof) {
      internalGetFieldAccessorTable().getOneof(oneof).clear(this);
      return (BuilderType) this;
    }

    @Override
    public int getRepeatedFieldCount(final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field)
          .getRepeatedCount(this);
    }

    @Override
    public Object getRepeatedField(final FieldDescriptor field, final int index) {
      return internalGetFieldAccessorTable().getField(field)
          .getRepeated(this, index);
    }

    @Override
    public BuilderType setRepeatedField(
        final FieldDescriptor field, final int index, final Object value) {
      internalGetFieldAccessorTable().getField(field)
        .setRepeated(this, index, value);
      return (BuilderType) this;
    }

    @Override
    public BuilderType addRepeatedField(final FieldDescriptor field, final Object value) {
      internalGetFieldAccessorTable().getField(field).addRepeated(this, value);
      return (BuilderType) this;
    }

    @Override
    public BuilderType setUnknownFields(final UnknownFieldSet unknownFields) {
      this.unknownFields = unknownFields;
      onChanged();
      return (BuilderType) this;
    }

    @Override
    public BuilderType mergeUnknownFields(
        final UnknownFieldSet unknownFields) {
      this.unknownFields =
        UnknownFieldSet.newBuilder(this.unknownFields)
                       .mergeFrom(unknownFields)
                       .build();
      onChanged();
      return (BuilderType) this;
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
            if (hasField(field) &&
                !((Message) getField(field)).isInitialized()) {
              return false;
            }
          }
        }
      }
      return true;
    }

    @Override
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

      @Override
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

    /**
     * Gets the map field with the given field number. This method should be
     * overridden in the generated message class if the message contains map
     * fields.
     *
     * Unlike other field types, reflection support for map fields can't be
     * implemented based on generated public API because we need to access a
     * map field as a list in reflection API but the generated API only allows
     * us to access it as a map. This method returns the underlying map field
     * directly and thus enables us to access the map field as a list.
     */
    @SuppressWarnings({"unused", "rawtypes"})
    protected MapField internalGetMapField(int fieldNumber) {
      // Note that we can't use descriptor names here because this method will
      // be called when descriptor is being initialized.
      throw new RuntimeException(
          "No map fields found in " + getClass().getName());
    }

    /** Like {@link #internalGetMapField} but return a mutable version. */
    @SuppressWarnings({"unused", "rawtypes"})
    protected MapField internalGetMutableMapField(int fieldNumber) {
      // Note that we can't use descriptor names here because this method will
      // be called when descriptor is being initialized.
      throw new RuntimeException(
          "No map fields found in " + getClass().getName());
    }
  }

  // =================================================================
  // Extensions-related stuff

  public interface ExtendableMessageOrBuilder<
      MessageType extends ExtendableMessage> extends MessageOrBuilder {
    // Re-define for return type covariance.
    @Override
    Message getDefaultInstanceForType();

    /** Check if a singular extension is present. */
    <Type> boolean hasExtension(
        ExtensionLite<MessageType, Type> extension);

    /** Get the number of elements in a repeated extension. */
    <Type> int getExtensionCount(
        ExtensionLite<MessageType, List<Type>> extension);

    /** Get the value of an extension. */
    <Type> Type getExtension(
        ExtensionLite<MessageType, Type> extension);

    /** Get one element of a repeated extension. */
    <Type> Type getExtension(
        ExtensionLite<MessageType, List<Type>> extension,
        int index);

    /** Check if a singular extension is present. */
    <Type> boolean hasExtension(
        Extension<MessageType, Type> extension);
    /** Check if a singular extension is present. */
    <Type> boolean hasExtension(
        GeneratedExtension<MessageType, Type> extension);
    /** Get the number of elements in a repeated extension. */
    <Type> int getExtensionCount(
        Extension<MessageType, List<Type>> extension);
    /** Get the number of elements in a repeated extension. */
    <Type> int getExtensionCount(
        GeneratedExtension<MessageType, List<Type>> extension);
    /** Get the value of an extension. */
    <Type> Type getExtension(
        Extension<MessageType, Type> extension);
    /** Get the value of an extension. */
    <Type> Type getExtension(
        GeneratedExtension<MessageType, Type> extension);
    /** Get one element of a repeated extension. */
    <Type> Type getExtension(
        Extension<MessageType, List<Type>> extension,
        int index);
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

    private static final long serialVersionUID = 1L;

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
        final Extension<MessageType, ?> extension) {
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
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> boolean hasExtension(final ExtensionLite<MessageType, Type> extensionLite) {
      Extension<MessageType, Type> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.getDescriptor());
    }

    /** Get the number of elements in a repeated extension. */
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> int getExtensionCount(
        final ExtensionLite<MessageType, List<Type>> extensionLite) {
      Extension<MessageType, List<Type>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      return extensions.getRepeatedFieldCount(descriptor);
    }

    /** Get the value of an extension. */
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(final ExtensionLite<MessageType, Type> extensionLite) {
      Extension<MessageType, Type> extension = checkNotLite(extensionLite);

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
    @Override
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, List<Type>> extensionLite, final int index) {
      Extension<MessageType, List<Type>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      return (Type) extension.singularFromReflectionType(
          extensions.getRepeatedField(descriptor, index));
    }

    /** Check if a singular extension is present. */
    @Override
    public final <Type> boolean hasExtension(final Extension<MessageType, Type> extension) {
      return hasExtension((ExtensionLite<MessageType, Type>) extension);
    }
    /** Check if a singular extension is present. */
    @Override
    public final <Type> boolean hasExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      return hasExtension((ExtensionLite<MessageType, Type>) extension);
    }
    /** Get the number of elements in a repeated extension. */
    @Override
    public final <Type> int getExtensionCount(
        final Extension<MessageType, List<Type>> extension) {
      return getExtensionCount((ExtensionLite<MessageType, List<Type>>) extension);
    }
    /** Get the number of elements in a repeated extension. */
    @Override
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      return getExtensionCount((ExtensionLite<MessageType, List<Type>>) extension);
    }
    /** Get the value of an extension. */
    @Override
    public final <Type> Type getExtension(final Extension<MessageType, Type> extension) {
      return getExtension((ExtensionLite<MessageType, Type>) extension);
    }
    /** Get the value of an extension. */
    @Override
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      return getExtension((ExtensionLite<MessageType, Type>) extension);
    }
    /** Get one element of a repeated extension. */
    @Override
    public final <Type> Type getExtension(
        final Extension<MessageType, List<Type>> extension, final int index) {
      return getExtension((ExtensionLite<MessageType, List<Type>>) extension, index);
    }
    /** Get one element of a repeated extension. */
    @Override
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, List<Type>> extension, final int index) {
      return getExtension((ExtensionLite<MessageType, List<Type>>) extension, index);
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
      return MessageReflection.mergeFieldFrom(
          input, unknownFields, extensionRegistry, getDescriptorForType(),
          new MessageReflection.ExtensionAdapter(extensions), tag);
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
      final Map<FieldDescriptor, Object> result =
          super.getAllFieldsMutable(/* getBytesForString = */ false);
      result.putAll(getExtensionFields());
      return Collections.unmodifiableMap(result);
    }

    @Override
    public Map<FieldDescriptor, Object> getAllFieldsRaw() {
      final Map<FieldDescriptor, Object> result =
          super.getAllFieldsMutable(/* getBytesForString = */ false);
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
          if (field.isRepeated()) {
            return Collections.emptyList();
          } else if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
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
        BuilderType extends ExtendableBuilder<MessageType, BuilderType>>
      extends Builder<BuilderType>
      implements ExtendableMessageOrBuilder<MessageType> {

    private FieldSet<FieldDescriptor> extensions = FieldSet.emptySet();

    protected ExtendableBuilder() {}

    protected ExtendableBuilder(
        BuilderParent parent) {
      super(parent);
    }

    // For immutable message conversion.
    void internalSetExtensionSet(FieldSet<FieldDescriptor> extensions) {
      this.extensions = extensions;
    }

    @Override
    public BuilderType clear() {
      extensions = FieldSet.emptySet();
      return super.clear();
    }

    // This is implemented here only to work around an apparent bug in the
    // Java compiler and/or build system.  See bug #1898463.  The mere presence
    // of this clone() implementation makes it go away.
    @Override
    public BuilderType clone() {
      return super.clone();
    }

    private void ensureExtensionsIsMutable() {
      if (extensions.isImmutable()) {
        extensions = extensions.clone();
      }
    }

    private void verifyExtensionContainingType(
        final Extension<MessageType, ?> extension) {
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
    @Override
    public final <Type> boolean hasExtension(final ExtensionLite<MessageType, Type> extensionLite) {
      Extension<MessageType, Type> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.getDescriptor());
    }

    /** Get the number of elements in a repeated extension. */
    @Override
    public final <Type> int getExtensionCount(
        final ExtensionLite<MessageType, List<Type>> extensionLite) {
      Extension<MessageType, List<Type>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      return extensions.getRepeatedFieldCount(descriptor);
    }

    /** Get the value of an extension. */
    @Override
    public final <Type> Type getExtension(final ExtensionLite<MessageType, Type> extensionLite) {
      Extension<MessageType, Type> extension = checkNotLite(extensionLite);

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
    @Override
    public final <Type> Type getExtension(
        final ExtensionLite<MessageType, List<Type>> extensionLite, final int index) {
      Extension<MessageType, List<Type>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      return (Type) extension.singularFromReflectionType(
          extensions.getRepeatedField(descriptor, index));
    }

    /** Set the value of an extension. */
    public final <Type> BuilderType setExtension(
        final ExtensionLite<MessageType, Type> extensionLite,
        final Type value) {
      Extension<MessageType, Type> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      final FieldDescriptor descriptor = extension.getDescriptor();
      extensions.setField(descriptor, extension.toReflectionType(value));
      onChanged();
      return (BuilderType) this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        final ExtensionLite<MessageType, List<Type>> extensionLite,
        final int index, final Type value) {
      Extension<MessageType, List<Type>> extension = checkNotLite(extensionLite);

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
        final ExtensionLite<MessageType, List<Type>> extensionLite,
        final Type value) {
      Extension<MessageType, List<Type>> extension = checkNotLite(extensionLite);

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
        final ExtensionLite<MessageType, ?> extensionLite) {
      Extension<MessageType, ?> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      extensions.clearField(extension.getDescriptor());
      onChanged();
      return (BuilderType) this;
    }

    /** Check if a singular extension is present. */
    @Override
    public final <Type> boolean hasExtension(final Extension<MessageType, Type> extension) {
      return hasExtension((ExtensionLite<MessageType, Type>) extension);
    }
    /** Check if a singular extension is present. */
    @Override
    public final <Type> boolean hasExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      return hasExtension((ExtensionLite<MessageType, Type>) extension);
    }
    /** Get the number of elements in a repeated extension. */
    @Override
    public final <Type> int getExtensionCount(
        final Extension<MessageType, List<Type>> extension) {
      return getExtensionCount((ExtensionLite<MessageType, List<Type>>) extension);
    }
    /** Get the number of elements in a repeated extension. */
    @Override
    public final <Type> int getExtensionCount(
        final GeneratedExtension<MessageType, List<Type>> extension) {
      return getExtensionCount((ExtensionLite<MessageType, List<Type>>) extension);
    }
    /** Get the value of an extension. */
    @Override
    public final <Type> Type getExtension(final Extension<MessageType, Type> extension) {
      return getExtension((ExtensionLite<MessageType, Type>) extension);
    }
    /** Get the value of an extension. */
    @Override
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, Type> extension) {
      return getExtension((ExtensionLite<MessageType, Type>) extension);
    }
    /** Get the value of an extension. */
    @Override
    public final <Type> Type getExtension(
        final Extension<MessageType, List<Type>> extension, final int index) {
      return getExtension((ExtensionLite<MessageType, List<Type>>) extension, index);
    }
    /** Get the value of an extension. */
    @Override
    public final <Type> Type getExtension(
        final GeneratedExtension<MessageType, List<Type>> extension, final int index) {
      return getExtension((ExtensionLite<MessageType, List<Type>>) extension, index);
    }
    /** Set the value of an extension. */
    public final <Type> BuilderType setExtension(
        final Extension<MessageType, Type> extension, final Type value) {
      return setExtension((ExtensionLite<MessageType, Type>) extension, value);
    }
    /** Set the value of an extension. */
    public <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, Type> extension, final Type value) {
      return setExtension((ExtensionLite<MessageType, Type>) extension, value);
    }
    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        final Extension<MessageType, List<Type>> extension,
        final int index, final Type value) {
      return setExtension((ExtensionLite<MessageType, List<Type>>) extension, index, value);
    }
    /** Set the value of one element of a repeated extension. */
    public <Type> BuilderType setExtension(
        final GeneratedExtension<MessageType, List<Type>> extension,
        final int index, final Type value) {
      return setExtension((ExtensionLite<MessageType, List<Type>>) extension, index, value);
    }
    /** Append a value to a repeated extension. */
    public final <Type> BuilderType addExtension(
        final Extension<MessageType, List<Type>> extension, final Type value) {
      return addExtension((ExtensionLite<MessageType, List<Type>>) extension, value);
    }
    /** Append a value to a repeated extension. */
    public <Type> BuilderType addExtension(
        final GeneratedExtension<MessageType, List<Type>> extension, final Type value) {
      return addExtension((ExtensionLite<MessageType, List<Type>>) extension, value);
    }
    /** Clear an extension. */
    public final <Type> BuilderType clearExtension(
        final Extension<MessageType, ?> extension) {
      return clearExtension((ExtensionLite<MessageType, ?>) extension);
    }
    /** Clear an extension. */
    public <Type> BuilderType clearExtension(
        final GeneratedExtension<MessageType, ?> extension) {
      return clearExtension((ExtensionLite<MessageType, ?>) extension);
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
      return MessageReflection.mergeFieldFrom(
          input, unknownFields, extensionRegistry, getDescriptorForType(),
          new MessageReflection.BuilderAdapter(this), tag);
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
  static interface ExtensionDescriptorRetriever {
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
        new CachedDescriptorRetriever() {
          @Override
          public FieldDescriptor loadDescriptor() {
            return scope.getDescriptorForType().getExtensions().get(descriptorIndex);
          }
        },
        singularType,
        defaultInstance,
        Extension.ExtensionType.IMMUTABLE);
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
        defaultInstance,
        Extension.ExtensionType.IMMUTABLE);
  }

  private abstract static class CachedDescriptorRetriever
      implements ExtensionDescriptorRetriever {
    private volatile FieldDescriptor descriptor;
    protected abstract FieldDescriptor loadDescriptor();

    @Override
    public FieldDescriptor getDescriptor() {
      if (descriptor == null) {
        synchronized (this) {
          if (descriptor == null) {
            descriptor = loadDescriptor();
          }
        }
      }
      return descriptor;
    }
  }

  /**
   * Used in proto1 generated code only.
   *
   * After enabling bridge, we can define proto2 extensions (the extended type
   * is a proto2 mutable message) in a proto1 .proto file. For these extensions
   * we should generate proto2 GeneratedExtensions.
   */
  public static <ContainingType extends Message, Type>
      GeneratedExtension<ContainingType, Type>
      newMessageScopedGeneratedExtension(
          final Message scope, final String name,
          final Class singularType, final Message defaultInstance) {
    // For extensions scoped within a Message, we use the Message to resolve
    // the outer class's descriptor, from which the extension descriptor is
    // obtained.
    return new GeneratedExtension<ContainingType, Type>(
        new CachedDescriptorRetriever() {
          @Override
          protected FieldDescriptor loadDescriptor() {
            return scope.getDescriptorForType().findFieldByName(name);
          }
        },
        singularType,
        defaultInstance,
        Extension.ExtensionType.MUTABLE);
  }

  /**
   * Used in proto1 generated code only.
   *
   * After enabling bridge, we can define proto2 extensions (the extended type
   * is a proto2 mutable message) in a proto1 .proto file. For these extensions
   * we should generate proto2 GeneratedExtensions.
   */
  public static <ContainingType extends Message, Type>
     GeneratedExtension<ContainingType, Type>
     newFileScopedGeneratedExtension(
         final Class singularType, final Message defaultInstance,
         final String descriptorOuterClass, final String extensionName) {
    // For extensions scoped within a file, we load the descriptor outer
    // class and rely on it to get the FileDescriptor which then can be
    // used to obtain the extension's FieldDescriptor.
    return new GeneratedExtension<ContainingType, Type>(
        new CachedDescriptorRetriever() {
          @Override
          protected FieldDescriptor loadDescriptor() {
            try {
              Class clazz = singularType.getClassLoader().loadClass(descriptorOuterClass);
              FileDescriptor file = (FileDescriptor) clazz.getField("descriptor").get(null);
              return file.findExtensionByName(extensionName);
            } catch (Exception e) {
              throw new RuntimeException(
                  "Cannot load descriptors: "
                      + descriptorOuterClass
                      + " is not a valid descriptor class name",
                  e);
            }
          }
        },
        singularType,
        defaultInstance,
        Extension.ExtensionType.MUTABLE);
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
  public static class GeneratedExtension<
      ContainingType extends Message, Type> extends
      Extension<ContainingType, Type> {
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
    GeneratedExtension(ExtensionDescriptorRetriever descriptorRetriever,
        Class singularType,
        Message messageDefaultInstance,
        ExtensionType extensionType) {
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
      this.extensionType = extensionType;
    }

    /** For use by generated code only. */
    public void internalInit(final FieldDescriptor descriptor) {
      if (descriptorRetriever != null) {
        throw new IllegalStateException("Already initialized.");
      }
      descriptorRetriever =
          new ExtensionDescriptorRetriever() {
            @Override
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
    private final ExtensionType extensionType;

    @Override
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
    @Override
    public Message getMessageDefaultInstance() {
      return messageDefaultInstance;
    }

    @Override
    protected ExtensionType getExtensionType() {
      return extensionType;
    }

    /**
     * Convert from the type used by the reflection accessors to the type used
     * by native accessors.  E.g., for enums, the reflection accessors use
     * EnumValueDescriptors but the native accessors use the generated enum
     * type.
     */
    @Override
    @SuppressWarnings("unchecked")
    protected Object fromReflectionType(final Object value) {
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
    @Override
    protected Object singularFromReflectionType(final Object value) {
      FieldDescriptor descriptor = getDescriptor();
      switch (descriptor.getJavaType()) {
        case MESSAGE:
          if (singularType.isInstance(value)) {
            return value;
          } else {
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
    @Override
    @SuppressWarnings("unchecked")
    protected Object toReflectionType(final Object value) {
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
    @Override
    protected Object singularToReflectionType(final Object value) {
      FieldDescriptor descriptor = getDescriptor();
      switch (descriptor.getJavaType()) {
        case ENUM:
          return invokeOrDie(enumGetValueDescriptor, value);
        default:
          return value;
      }
    }

    @Override
    public int getNumber() {
      return getDescriptor().getNumber();
    }

    @Override
    public WireFormat.FieldType getLiteType() {
      return getDescriptor().getLiteType();
    }

    @Override
    public boolean isRepeated() {
      return getDescriptor().isRepeated();
    }

    @Override
    @SuppressWarnings("unchecked")
    public Type getDefaultValue() {
      if (isRepeated()) {
        return (Type) Collections.emptyList();
      }
      if (getDescriptor().getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        return (Type) messageDefaultInstance;
      }
      return (Type) singularFromReflectionType(
          getDescriptor().getDefaultValue());
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
   * Gets the map field with the given field number. This method should be
   * overridden in the generated message class if the message contains map
   * fields.
   *
   * Unlike other field types, reflection support for map fields can't be
   * implemented based on generated public API because we need to access a
   * map field as a list in reflection API but the generated API only allows
   * us to access it as a map. This method returns the underlying map field
   * directly and thus enables us to access the map field as a list.
   */
  @SuppressWarnings({"rawtypes", "unused"})
  protected MapField internalGetMapField(int fieldNumber) {
    // Note that we can't use descriptor names here because this method will
    // be called when descriptor is being initialized.
    throw new RuntimeException(
        "No map fields found in " + getClass().getName());
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
      oneofs = new OneofAccessor[descriptor.getOneofs().size()];
      initialized = false;
    }

    private boolean isMapFieldEnabled(FieldDescriptor field) {
      boolean result = true;
      return result;
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
        int fieldsSize = fields.length;
        for (int i = 0; i < fieldsSize; i++) {
          FieldDescriptor field = descriptor.getFields().get(i);
          String containingOneofCamelCaseName = null;
          if (field.getContainingOneof() != null) {
            containingOneofCamelCaseName =
                camelCaseNames[fieldsSize + field.getContainingOneof().getIndex()];
          }
          if (field.isRepeated()) {
            if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
              if (field.isMapField() && isMapFieldEnabled(field)) {
                fields[i] = new MapFieldAccessor(
                    field, camelCaseNames[i], messageClass, builderClass);
              } else {
                fields[i] = new RepeatedMessageFieldAccessor(
                    field, camelCaseNames[i], messageClass, builderClass);
              }
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
                  field, camelCaseNames[i], messageClass, builderClass,
                  containingOneofCamelCaseName);
            } else if (field.getJavaType() == FieldDescriptor.JavaType.ENUM) {
              fields[i] = new SingularEnumFieldAccessor(
                  field, camelCaseNames[i], messageClass, builderClass,
                  containingOneofCamelCaseName);
            } else if (field.getJavaType() == FieldDescriptor.JavaType.STRING) {
              fields[i] = new SingularStringFieldAccessor(
                  field, camelCaseNames[i], messageClass, builderClass,
                  containingOneofCamelCaseName);
            } else {
              fields[i] = new SingularFieldAccessor(
                  field, camelCaseNames[i], messageClass, builderClass,
                  containingOneofCamelCaseName);
            }
          }
        }

        int oneofsSize = oneofs.length;
        for (int i = 0; i < oneofsSize; i++) {
          oneofs[i] = new OneofAccessor(
              descriptor, camelCaseNames[i + fieldsSize],
              messageClass, builderClass);
        }
        initialized = true;
        camelCaseNames = null;
        return this;
      }
    }

    private final Descriptor descriptor;
    private final FieldAccessor[] fields;
    private String[] camelCaseNames;
    private final OneofAccessor[] oneofs;
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

    /** Get the OneofAccessor for a particular oneof. */
    private OneofAccessor getOneof(final OneofDescriptor oneof) {
      if (oneof.getContainingType() != descriptor) {
        throw new IllegalArgumentException(
          "OneofDescriptor does not match message type.");
      }
      return oneofs[oneof.getIndex()];
    }

    /**
     * Abstract interface that provides access to a single field.  This is
     * implemented differently depending on the field type and cardinality.
     */
    private interface FieldAccessor {
      Object get(GeneratedMessage message);
      Object get(GeneratedMessage.Builder builder);
      Object getRaw(GeneratedMessage message);
      Object getRaw(GeneratedMessage.Builder builder);
      void set(Builder builder, Object value);
      Object getRepeated(GeneratedMessage message, int index);
      Object getRepeated(GeneratedMessage.Builder builder, int index);
      Object getRepeatedRaw(GeneratedMessage message, int index);
      Object getRepeatedRaw(GeneratedMessage.Builder builder, int index);
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
      Message.Builder getRepeatedBuilder(GeneratedMessage.Builder builder,
                                         int index);
    }

    /** OneofAccessor provides access to a single oneof. */
    private static class OneofAccessor {
      OneofAccessor(
          final Descriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        this.descriptor = descriptor;
        caseMethod =
            getMethodOrDie(messageClass, "get" + camelCaseName + "Case");
        caseMethodBuilder =
            getMethodOrDie(builderClass, "get" + camelCaseName + "Case");
        clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
      }

      private final Descriptor descriptor;
      private final Method caseMethod;
      private final Method caseMethodBuilder;
      private final Method clearMethod;

      public boolean has(final GeneratedMessage message) {
        if (((Internal.EnumLite) invokeOrDie(caseMethod, message)).getNumber() == 0) {
          return false;
        }
        return true;
      }

      public boolean has(GeneratedMessage.Builder builder) {
        if (((Internal.EnumLite) invokeOrDie(caseMethodBuilder, builder)).getNumber() == 0) {
          return false;
        }
        return true;
      }

      public FieldDescriptor get(final GeneratedMessage message) {
        int fieldNumber = ((Internal.EnumLite) invokeOrDie(caseMethod, message)).getNumber();
        if (fieldNumber > 0) {
          return descriptor.findFieldByNumber(fieldNumber);
        }
        return null;
      }

      public FieldDescriptor get(GeneratedMessage.Builder builder) {
        int fieldNumber = ((Internal.EnumLite) invokeOrDie(caseMethodBuilder, builder)).getNumber();
        if (fieldNumber > 0) {
          return descriptor.findFieldByNumber(fieldNumber);
        }
        return null;
      }

      public void clear(final Builder builder) {
        invokeOrDie(clearMethod, builder);
      }
    }

    private static boolean supportFieldPresence(FileDescriptor file) {
      return file.getSyntax() == FileDescriptor.Syntax.PROTO2;
    }

    // ---------------------------------------------------------------

    private static class SingularFieldAccessor implements FieldAccessor {
      SingularFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass,
          final String containingOneofCamelCaseName) {
        field = descriptor;
        isOneofField = descriptor.getContainingOneof() != null;
        hasHasMethod = supportFieldPresence(descriptor.getFile())
            || (!isOneofField && descriptor.getJavaType() == FieldDescriptor.JavaType.MESSAGE);
        getMethod = getMethodOrDie(messageClass, "get" + camelCaseName);
        getMethodBuilder = getMethodOrDie(builderClass, "get" + camelCaseName);
        type = getMethod.getReturnType();
        setMethod = getMethodOrDie(builderClass, "set" + camelCaseName, type);
        hasMethod =
            hasHasMethod ? getMethodOrDie(messageClass, "has" + camelCaseName) : null;
        hasMethodBuilder =
            hasHasMethod ? getMethodOrDie(builderClass, "has" + camelCaseName) : null;
        clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
        caseMethod = isOneofField ? getMethodOrDie(
            messageClass, "get" + containingOneofCamelCaseName + "Case") : null;
        caseMethodBuilder = isOneofField ? getMethodOrDie(
            builderClass, "get" + containingOneofCamelCaseName + "Case") : null;
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
      protected final Method caseMethod;
      protected final Method caseMethodBuilder;
      protected final FieldDescriptor field;
      protected final boolean isOneofField;
      protected final boolean hasHasMethod;

      private int getOneofFieldNumber(final GeneratedMessage message) {
        return ((Internal.EnumLite) invokeOrDie(caseMethod, message)).getNumber();
      }

      private int getOneofFieldNumber(final GeneratedMessage.Builder builder) {
        return ((Internal.EnumLite) invokeOrDie(caseMethodBuilder, builder)).getNumber();
      }

      @Override
      public Object get(final GeneratedMessage message) {
        return invokeOrDie(getMethod, message);
      }
      @Override
      public Object get(GeneratedMessage.Builder builder) {
        return invokeOrDie(getMethodBuilder, builder);
      }
      @Override
      public Object getRaw(final GeneratedMessage message) {
        return get(message);
      }
      @Override
      public Object getRaw(GeneratedMessage.Builder builder) {
        return get(builder);
      }
      @Override
      public void set(final Builder builder, final Object value) {
        invokeOrDie(setMethod, builder, value);
      }
      @Override
      public Object getRepeated(final GeneratedMessage message, final int index) {
        throw new UnsupportedOperationException(
          "getRepeatedField() called on a singular field.");
      }
      @Override
      public Object getRepeatedRaw(final GeneratedMessage message, final int index) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldRaw() called on a singular field.");
      }
      @Override
      public Object getRepeated(GeneratedMessage.Builder builder, int index) {
        throw new UnsupportedOperationException(
          "getRepeatedField() called on a singular field.");
      }
      @Override
      public Object getRepeatedRaw(GeneratedMessage.Builder builder, int index) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldRaw() called on a singular field.");
      }
      @Override
      public void setRepeated(final Builder builder, final int index, final Object value) {
        throw new UnsupportedOperationException(
          "setRepeatedField() called on a singular field.");
      }
      @Override
      public void addRepeated(final Builder builder, final Object value) {
        throw new UnsupportedOperationException(
          "addRepeatedField() called on a singular field.");
      }
      @Override
      public boolean has(final GeneratedMessage message) {
        if (!hasHasMethod) {
          if (isOneofField) {
            return getOneofFieldNumber(message) == field.getNumber();
          }
          return !get(message).equals(field.getDefaultValue());
        }
        return (Boolean) invokeOrDie(hasMethod, message);
      }
      @Override
      public boolean has(GeneratedMessage.Builder builder) {
        if (!hasHasMethod) {
          if (isOneofField) {
            return getOneofFieldNumber(builder) == field.getNumber();
          }
          return !get(builder).equals(field.getDefaultValue());
        }
        return (Boolean) invokeOrDie(hasMethodBuilder, builder);
      }
      @Override
      public int getRepeatedCount(final GeneratedMessage message) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldSize() called on a singular field.");
      }
      @Override
      public int getRepeatedCount(GeneratedMessage.Builder builder) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldSize() called on a singular field.");
      }
      @Override
      public void clear(final Builder builder) {
        invokeOrDie(clearMethod, builder);
      }
      @Override
      public Message.Builder newBuilder() {
        throw new UnsupportedOperationException(
          "newBuilderForField() called on a non-Message type.");
      }
      @Override
      public Message.Builder getBuilder(GeneratedMessage.Builder builder) {
        throw new UnsupportedOperationException(
          "getFieldBuilder() called on a non-Message type.");
      }
      @Override
      public Message.Builder getRepeatedBuilder(GeneratedMessage.Builder builder, int index) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldBuilder() called on a non-Message type.");
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

      @Override
      public Object get(final GeneratedMessage message) {
        return invokeOrDie(getMethod, message);
      }
      @Override
      public Object get(GeneratedMessage.Builder builder) {
        return invokeOrDie(getMethodBuilder, builder);
      }
      @Override
      public Object getRaw(final GeneratedMessage message) {
        return get(message);
      }
      @Override
      public Object getRaw(GeneratedMessage.Builder builder) {
        return get(builder);
      }
      @Override
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
      @Override
      public Object getRepeated(final GeneratedMessage message, final int index) {
        return invokeOrDie(getRepeatedMethod, message, index);
      }
      @Override
      public Object getRepeated(GeneratedMessage.Builder builder, int index) {
        return invokeOrDie(getRepeatedMethodBuilder, builder, index);
      }
      @Override
      public Object getRepeatedRaw(GeneratedMessage message, int index) {
        return getRepeated(message, index);
      }
      @Override
      public Object getRepeatedRaw(GeneratedMessage.Builder builder, int index) {
        return getRepeated(builder, index);
      }
      @Override
      public void setRepeated(final Builder builder, final int index, final Object value) {
        invokeOrDie(setRepeatedMethod, builder, index, value);
      }
      @Override
      public void addRepeated(final Builder builder, final Object value) {
        invokeOrDie(addRepeatedMethod, builder, value);
      }
      @Override
      public boolean has(final GeneratedMessage message) {
        throw new UnsupportedOperationException(
          "hasField() called on a repeated field.");
      }
      @Override
      public boolean has(GeneratedMessage.Builder builder) {
        throw new UnsupportedOperationException(
          "hasField() called on a repeated field.");
      }
      @Override
      public int getRepeatedCount(final GeneratedMessage message) {
        return (Integer) invokeOrDie(getCountMethod, message);
      }
      @Override
      public int getRepeatedCount(GeneratedMessage.Builder builder) {
        return (Integer) invokeOrDie(getCountMethodBuilder, builder);
      }
      @Override
      public void clear(final Builder builder) {
        invokeOrDie(clearMethod, builder);
      }
      @Override
      public Message.Builder newBuilder() {
        throw new UnsupportedOperationException(
          "newBuilderForField() called on a non-Message type.");
      }
      @Override
      public Message.Builder getBuilder(GeneratedMessage.Builder builder) {
        throw new UnsupportedOperationException(
          "getFieldBuilder() called on a non-Message type.");
      }
      @Override
      public Message.Builder getRepeatedBuilder(GeneratedMessage.Builder builder, int index) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldBuilder() called on a non-Message type.");
      }
    }

    private static class MapFieldAccessor implements FieldAccessor {
      MapFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass) {
        field = descriptor;
        Method getDefaultInstanceMethod =
            getMethodOrDie(messageClass, "getDefaultInstance");
        MapField defaultMapField = getMapField(
            (GeneratedMessage) invokeOrDie(getDefaultInstanceMethod, null));
        mapEntryMessageDefaultInstance =
            defaultMapField.getMapEntryMessageDefaultInstance();
      }

      private final FieldDescriptor field;
      private final Message mapEntryMessageDefaultInstance;

      private MapField<?, ?> getMapField(GeneratedMessage message) {
        return (MapField<?, ?>) message.internalGetMapField(field.getNumber());
      }

      private MapField<?, ?> getMapField(GeneratedMessage.Builder builder) {
        return (MapField<?, ?>) builder.internalGetMapField(field.getNumber());
      }

      private MapField<?, ?> getMutableMapField(
          GeneratedMessage.Builder builder) {
        return (MapField<?, ?>) builder.internalGetMutableMapField(
            field.getNumber());
      }

      @Override
      @SuppressWarnings("unchecked")
      public Object get(GeneratedMessage message) {
        List result = new ArrayList();
        for (int i = 0; i < getRepeatedCount(message); i++) {
          result.add(getRepeated(message, i));
        }
        return Collections.unmodifiableList(result);
      }

      @Override
      @SuppressWarnings("unchecked")
      public Object get(Builder builder) {
        List result = new ArrayList();
        for (int i = 0; i < getRepeatedCount(builder); i++) {
          result.add(getRepeated(builder, i));
        }
        return Collections.unmodifiableList(result);
      }

      @Override
      public Object getRaw(GeneratedMessage message) {
        return get(message);
      }

      @Override
      public Object getRaw(GeneratedMessage.Builder builder) {
        return get(builder);
      }

      @Override
      public void set(Builder builder, Object value) {
        clear(builder);
        for (Object entry : (List) value) {
          addRepeated(builder, entry);
        }
      }

      @Override
      public Object getRepeated(GeneratedMessage message, int index) {
        return getMapField(message).getList().get(index);
      }

      @Override
      public Object getRepeated(Builder builder, int index) {
        return getMapField(builder).getList().get(index);
      }

      @Override
      public Object getRepeatedRaw(GeneratedMessage message, int index) {
        return getRepeated(message, index);
      }

      @Override
      public Object getRepeatedRaw(Builder builder, int index) {
        return getRepeated(builder, index);
      }

      @Override
      public void setRepeated(Builder builder, int index, Object value) {
        getMutableMapField(builder).getMutableList().set(index, (Message) value);
      }

      @Override
      public void addRepeated(Builder builder, Object value) {
        getMutableMapField(builder).getMutableList().add((Message) value);
      }

      @Override
      public boolean has(GeneratedMessage message) {
        throw new UnsupportedOperationException(
            "hasField() is not supported for repeated fields.");
      }

      @Override
      public boolean has(Builder builder) {
        throw new UnsupportedOperationException(
            "hasField() is not supported for repeated fields.");
      }

      @Override
      public int getRepeatedCount(GeneratedMessage message) {
        return getMapField(message).getList().size();
      }

      @Override
      public int getRepeatedCount(Builder builder) {
        return getMapField(builder).getList().size();
      }

      @Override
      public void clear(Builder builder) {
        getMutableMapField(builder).getMutableList().clear();
      }

      @Override
      public com.google.protobuf.Message.Builder newBuilder() {
        return mapEntryMessageDefaultInstance.newBuilderForType();
      }

      @Override
      public com.google.protobuf.Message.Builder getBuilder(Builder builder) {
        throw new UnsupportedOperationException(
            "Nested builder not supported for map fields.");
      }

      @Override
      public com.google.protobuf.Message.Builder getRepeatedBuilder(Builder builder, int index) {
        throw new UnsupportedOperationException(
            "Nested builder not supported for map fields.");
      }
    }

    // ---------------------------------------------------------------

    private static final class SingularEnumFieldAccessor
        extends SingularFieldAccessor {
      SingularEnumFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass,
          final String containingOneofCamelCaseName) {
        super(descriptor, camelCaseName, messageClass, builderClass, containingOneofCamelCaseName);

        enumDescriptor = descriptor.getEnumType();

        valueOfMethod = getMethodOrDie(type, "valueOf",
                                       EnumValueDescriptor.class);
        getValueDescriptorMethod =
          getMethodOrDie(type, "getValueDescriptor");

        supportUnknownEnumValue = descriptor.getFile().supportsUnknownEnumValue();
        if (supportUnknownEnumValue) {
          getValueMethod =
              getMethodOrDie(messageClass, "get" + camelCaseName + "Value");
          getValueMethodBuilder =
              getMethodOrDie(builderClass, "get" + camelCaseName + "Value");
          setValueMethod =
              getMethodOrDie(builderClass, "set" + camelCaseName + "Value", int.class);
        }
      }

      private EnumDescriptor enumDescriptor;

      private Method valueOfMethod;
      private Method getValueDescriptorMethod;

      private boolean supportUnknownEnumValue;
      private Method getValueMethod;
      private Method getValueMethodBuilder;
      private Method setValueMethod;

      @Override
      public Object get(final GeneratedMessage message) {
        if (supportUnknownEnumValue) {
          int value = (Integer) invokeOrDie(getValueMethod, message);
          return enumDescriptor.findValueByNumberCreatingIfUnknown(value);
        }
        return invokeOrDie(getValueDescriptorMethod, super.get(message));
      }

      @Override
      public Object get(final GeneratedMessage.Builder builder) {
        if (supportUnknownEnumValue) {
          int value = (Integer) invokeOrDie(getValueMethodBuilder, builder);
          return enumDescriptor.findValueByNumberCreatingIfUnknown(value);
        }
        return invokeOrDie(getValueDescriptorMethod, super.get(builder));
      }

      @Override
      public void set(final Builder builder, final Object value) {
        if (supportUnknownEnumValue) {
          invokeOrDie(setValueMethod, builder,
              ((EnumValueDescriptor) value).getNumber());
          return;
        }
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

        enumDescriptor = descriptor.getEnumType();

        valueOfMethod = getMethodOrDie(type, "valueOf",
                                       EnumValueDescriptor.class);
        getValueDescriptorMethod =
          getMethodOrDie(type, "getValueDescriptor");

        supportUnknownEnumValue = descriptor.getFile().supportsUnknownEnumValue();
        if (supportUnknownEnumValue) {
          getRepeatedValueMethod =
              getMethodOrDie(messageClass, "get" + camelCaseName + "Value", int.class);
          getRepeatedValueMethodBuilder =
              getMethodOrDie(builderClass, "get" + camelCaseName + "Value", int.class);
          setRepeatedValueMethod =
              getMethodOrDie(builderClass, "set" + camelCaseName + "Value", int.class, int.class);
          addRepeatedValueMethod =
              getMethodOrDie(builderClass, "add" + camelCaseName + "Value", int.class);
        }
      }
      private EnumDescriptor enumDescriptor;

      private final Method valueOfMethod;
      private final Method getValueDescriptorMethod;

      private boolean supportUnknownEnumValue;
      private Method getRepeatedValueMethod;
      private Method getRepeatedValueMethodBuilder;
      private Method setRepeatedValueMethod;
      private Method addRepeatedValueMethod;

      @Override
      @SuppressWarnings("unchecked")
      public Object get(final GeneratedMessage message) {
        final List newList = new ArrayList();
        final int size = getRepeatedCount(message);
        for (int i = 0; i < size; i++) {
          newList.add(getRepeated(message, i));
        }
        return Collections.unmodifiableList(newList);
      }

      @Override
      @SuppressWarnings("unchecked")
      public Object get(final GeneratedMessage.Builder builder) {
        final List newList = new ArrayList();
        final int size = getRepeatedCount(builder);
        for (int i = 0; i < size; i++) {
          newList.add(getRepeated(builder, i));
        }
        return Collections.unmodifiableList(newList);
      }

      @Override
      public Object getRepeated(final GeneratedMessage message,
                                final int index) {
        if (supportUnknownEnumValue) {
          int value = (Integer) invokeOrDie(getRepeatedValueMethod, message, index);
          return enumDescriptor.findValueByNumberCreatingIfUnknown(value);
        }
        return invokeOrDie(getValueDescriptorMethod,
          super.getRepeated(message, index));
      }
      @Override
      public Object getRepeated(final GeneratedMessage.Builder builder,
                                final int index) {
        if (supportUnknownEnumValue) {
          int value = (Integer) invokeOrDie(getRepeatedValueMethodBuilder, builder, index);
          return enumDescriptor.findValueByNumberCreatingIfUnknown(value);
        }
        return invokeOrDie(getValueDescriptorMethod,
          super.getRepeated(builder, index));
      }
      @Override
      public void setRepeated(final Builder builder,
                              final int index, final Object value) {
        if (supportUnknownEnumValue) {
          invokeOrDie(setRepeatedValueMethod, builder, index,
              ((EnumValueDescriptor) value).getNumber());
          return;
        }
        super.setRepeated(builder, index, invokeOrDie(valueOfMethod, null,
                          value));
      }
      @Override
      public void addRepeated(final Builder builder, final Object value) {
        if (supportUnknownEnumValue) {
          invokeOrDie(addRepeatedValueMethod, builder,
              ((EnumValueDescriptor) value).getNumber());
          return;
        }
        super.addRepeated(builder, invokeOrDie(valueOfMethod, null, value));
      }
    }

    // ---------------------------------------------------------------

    /**
     * Field accessor for string fields.
     *
     * <p>This class makes getFooBytes() and setFooBytes() available for
     * reflection API so that reflection based serialize/parse functions can
     * access the raw bytes of the field to preserve non-UTF8 bytes in the
     * string.
     *
     * <p>This ensures the serialize/parse round-trip safety, which is important
     * for servers which forward messages.
     */
    private static final class SingularStringFieldAccessor
        extends SingularFieldAccessor {
      SingularStringFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass,
          final String containingOneofCamelCaseName) {
        super(descriptor, camelCaseName, messageClass, builderClass,
            containingOneofCamelCaseName);
        getBytesMethod = getMethodOrDie(messageClass,
            "get" + camelCaseName + "Bytes");
        getBytesMethodBuilder = getMethodOrDie(builderClass,
            "get" + camelCaseName + "Bytes");
        setBytesMethodBuilder = getMethodOrDie(builderClass,
            "set" + camelCaseName + "Bytes", ByteString.class);
      }

      private final Method getBytesMethod;
      private final Method getBytesMethodBuilder;
      private final Method setBytesMethodBuilder;

      @Override
      public Object getRaw(final GeneratedMessage message) {
        return invokeOrDie(getBytesMethod, message);
      }

      @Override
      public Object getRaw(GeneratedMessage.Builder builder) {
        return invokeOrDie(getBytesMethodBuilder, builder);
      }

      @Override
      public void set(GeneratedMessage.Builder builder, Object value) {
        if (value instanceof ByteString) {
          invokeOrDie(setBytesMethodBuilder, builder, value);
        } else {
          super.set(builder, value);
        }
      }
    }

    // ---------------------------------------------------------------

    private static final class SingularMessageFieldAccessor
        extends SingularFieldAccessor {
      SingularMessageFieldAccessor(
          final FieldDescriptor descriptor, final String camelCaseName,
          final Class<? extends GeneratedMessage> messageClass,
          final Class<? extends Builder> builderClass,
          final String containingOneofCamelCaseName) {
        super(descriptor, camelCaseName, messageClass, builderClass,
            containingOneofCamelCaseName);

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
        getBuilderMethodBuilder = getMethodOrDie(builderClass,
            "get" + camelCaseName + "Builder", Integer.TYPE);
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
      @Override
      public Message.Builder getRepeatedBuilder(
          final GeneratedMessage.Builder builder, final int index) {
        return (Message.Builder) invokeOrDie(
            getBuilderMethodBuilder, builder, index);
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

  /**
   * Checks that the {@link Extension} is non-Lite and returns it as a
   * {@link GeneratedExtension}.
   */
  private static <MessageType extends ExtendableMessage<MessageType>, T>
    Extension<MessageType, T> checkNotLite(
        ExtensionLite<MessageType, T> extension) {
    if (extension.isLite()) {
      throw new IllegalArgumentException("Expected non-lite extension.");
    }

    return (Extension<MessageType, T>) extension;
  }

  protected static int computeStringSize(final int fieldNumber, final Object value) {
    if (value instanceof String) {
      return CodedOutputStream.computeStringSize(fieldNumber, (String) value);
    } else {
      return CodedOutputStream.computeBytesSize(fieldNumber, (ByteString) value);
    }
  }

  protected static int computeStringSizeNoTag(final Object value) {
    if (value instanceof String) {
      return CodedOutputStream.computeStringSizeNoTag((String) value);
    } else {
      return CodedOutputStream.computeBytesSizeNoTag((ByteString) value);
    }
  }

  protected static void writeString(
      CodedOutputStream output, final int fieldNumber, final Object value) throws IOException {
    if (value instanceof String) {
      output.writeString(fieldNumber, (String) value);
    } else {
      output.writeBytes(fieldNumber, (ByteString) value);
    }
  }

  protected static void writeStringNoTag(
      CodedOutputStream output, final Object value) throws IOException {
    if (value instanceof String) {
      output.writeStringNoTag((String) value);
    } else {
      output.writeBytesNoTag((ByteString) value);
    }
  }
}
