// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.protobuf;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collection;
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
  private final Map<FieldDescriptor, Object> getAllFieldsMutable() {
    TreeMap<FieldDescriptor, Object> result =
      new TreeMap<FieldDescriptor, Object>();
    Descriptor descriptor = internalGetFieldAccessorTable().descriptor;
    for (FieldDescriptor field : descriptor.getFields()) {
      if (field.isRepeated()) {
        List value = (List)getField(field);
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

  public Map<FieldDescriptor, Object> getAllFields() {
    return Collections.unmodifiableMap(getAllFieldsMutable());
  }

  public boolean hasField(Descriptors.FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).has(this);
  }

  public Object getField(FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).get(this);
  }

  public int getRepeatedFieldCount(FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field)
      .getRepeatedCount(this);
  }

  public Object getRepeatedField(FieldDescriptor field, int index) {
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

    public BuilderType mergeFrom(Message other) {
      if (other.getDescriptorForType() !=
          internalGetFieldAccessorTable().descriptor) {
        throw new IllegalArgumentException("Message type mismatch.");
      }

      for (Map.Entry<FieldDescriptor, Object> entry :
           other.getAllFields().entrySet()) {
        FieldDescriptor field = entry.getKey();
        if (field.isRepeated()) {
          // Concatenate repeated fields.
          for (Object element : (List) entry.getValue()) {
            addRepeatedField(field, element);
          }
        } else if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE &&
                   hasField(field)) {
          // Merge singular embedded messages.
          Message oldValue = (Message) getField(field);
          setField(field,
            oldValue.newBuilderForType()
              .mergeFrom(oldValue)
              .mergeFrom((Message) entry.getValue())
              .buildPartial());
        } else {
          // Just overwrite.
          setField(field, entry.getValue());
        }
      }
      return (BuilderType)this;
    }

    public Descriptor getDescriptorForType() {
      return internalGetFieldAccessorTable().descriptor;
    }

    public Map<Descriptors.FieldDescriptor, Object> getAllFields() {
      return internalGetResult().getAllFields();
    }

    public Message.Builder newBuilderForField(
        Descriptors.FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).newBuilder();
    }

    public boolean hasField(Descriptors.FieldDescriptor field) {
      return internalGetResult().hasField(field);
    }

    public Object getField(Descriptors.FieldDescriptor field) {
      if (field.isRepeated()) {
        // The underlying list object is still modifiable at this point.
        // Make sure not to expose the modifiable list to the caller.
        return Collections.unmodifiableList(
          (List)internalGetResult().getField(field));
      } else {
        return internalGetResult().getField(field);
      }
    }

    public BuilderType setField(Descriptors.FieldDescriptor field,
                                Object value) {
      internalGetFieldAccessorTable().getField(field).set(this, value);
      return (BuilderType)this;
    }

    public BuilderType clearField(Descriptors.FieldDescriptor field) {
      internalGetFieldAccessorTable().getField(field).clear(this);
      return (BuilderType)this;
    }

    public int getRepeatedFieldCount(Descriptors.FieldDescriptor field) {
      return internalGetResult().getRepeatedFieldCount(field);
    }

    public Object getRepeatedField(Descriptors.FieldDescriptor field,
                                   int index) {
      return internalGetResult().getRepeatedField(field, index);
    }

    public BuilderType setRepeatedField(Descriptors.FieldDescriptor field,
                                        int index, Object value) {
      internalGetFieldAccessorTable().getField(field)
        .setRepeated(this, index, value);
      return (BuilderType)this;
    }

    public BuilderType addRepeatedField(Descriptors.FieldDescriptor field,
                                        Object value) {
      internalGetFieldAccessorTable().getField(field).addRepeated(this, value);
      return (BuilderType)this;
    }

    public final UnknownFieldSet getUnknownFields() {
      return internalGetResult().unknownFields;
    }

    public final BuilderType setUnknownFields(UnknownFieldSet unknownFields) {
      internalGetResult().unknownFields = unknownFields;
      return (BuilderType)this;
    }

    public final BuilderType mergeUnknownFields(UnknownFieldSet unknownFields) {
      GeneratedMessage result = internalGetResult();
      result.unknownFields =
        UnknownFieldSet.newBuilder(result.unknownFields)
                       .mergeFrom(unknownFields)
                       .build();
      return (BuilderType)this;
    }

    public boolean isInitialized() {
      return internalGetResult().isInitialized();
    }

    /**
     * Called by subclasses to parse an unknown field.
     * @return {@code true} unless the tag is an end-group tag.
     */
    protected boolean parseUnknownField(CodedInputStream input,
                                        UnknownFieldSet.Builder unknownFields,
                                        ExtensionRegistry extensionRegistry,
                                        int tag)
                                 throws IOException {
      return unknownFields.mergeFieldFrom(tag, input);
    }

    protected <T> void addAll(Iterable<T> values, Collection<? super T> list) {
      if (values instanceof Collection) {
        @SuppressWarnings("unsafe")
        Collection<T> collection = (Collection<T>) values;
        list.addAll(collection);
      } else {
        for (T value : values) {
          list.add(value);
        }
      }
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
    private final FieldSet extensions = FieldSet.newFieldSet();

    private final void verifyExtensionContainingType(
        GeneratedExtension<MessageType, ?> extension) {
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
        GeneratedExtension<MessageType, ?> extension) {
      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.getDescriptor());
    }

    /** Get the number of elements in a repeated extension. */
    public final <Type> int getExtensionCount(
        GeneratedExtension<MessageType, List<Type>> extension) {
      verifyExtensionContainingType(extension);
      return extensions.getRepeatedFieldCount(extension.getDescriptor());
    }

    /** Get the value of an extension. */
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        GeneratedExtension<MessageType, Type> extension) {
      verifyExtensionContainingType(extension);
      Object value = extensions.getField(extension.getDescriptor());
      if (value == null) {
        return (Type)extension.getMessageDefaultInstance();
      } else {
        return (Type)extension.fromReflectionType(value);
      }
    }

    /** Get one element of a repeated extension. */
    @SuppressWarnings("unchecked")
    public final <Type> Type getExtension(
        GeneratedExtension<MessageType, List<Type>> extension, int index) {
      verifyExtensionContainingType(extension);
      return (Type)extension.singularFromReflectionType(
        extensions.getRepeatedField(extension.getDescriptor(), index));
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

      final Iterator<Map.Entry<FieldDescriptor, Object>> iter =
        extensions.iterator();
      Map.Entry<FieldDescriptor, Object> next = null;

      private ExtensionWriter() {
        if (iter.hasNext()) {
          next = iter.next();
        }
      }

      public void writeUntil(int end, CodedOutputStream output)
                             throws IOException {
        while (next != null && next.getKey().getNumber() < end) {
          extensions.writeField(next.getKey(), next.getValue(), output);
          if (iter.hasNext()) {
            next = iter.next();
          } else {
            next = null;
          }
        }
      }
    }

    protected ExtensionWriter newExtensionWriter() {
      return new ExtensionWriter();
    }

    /** Called by subclasses to compute the size of extensions. */
    protected int extensionsSerializedSize() {
      return extensions.getSerializedSize();
    }

    // ---------------------------------------------------------------
    // Reflection

    public Map<Descriptors.FieldDescriptor, Object> getAllFields() {
      Map<FieldDescriptor, Object> result = super.getAllFieldsMutable();
      result.putAll(extensions.getAllFields());
      return Collections.unmodifiableMap(result);
    }

    public boolean hasField(FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions.hasField(field);
      } else {
        return super.hasField(field);
      }
    }

    public Object getField(FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        Object value = extensions.getField(field);
        if (value == null) {
          // Lacking an ExtensionRegistry, we have no way to determine the
          // extension's real type, so we return a DynamicMessage.
          return DynamicMessage.getDefaultInstance(field.getMessageType());
        } else {
          return value;
        }
      } else {
        return super.getField(field);
      }
    }

    public int getRepeatedFieldCount(FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions.getRepeatedFieldCount(field);
      } else {
        return super.getRepeatedFieldCount(field);
      }
    }

    public Object getRepeatedField(FieldDescriptor field, int index) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions.getRepeatedField(field, index);
      } else {
        return super.getRepeatedField(field, index);
      }
    }

    private void verifyContainingType(FieldDescriptor field) {
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
      extends GeneratedMessage.Builder<BuilderType> {
    protected ExtendableBuilder() {}
    protected abstract ExtendableMessage<MessageType> internalGetResult();

    /** Check if a singular extension is present. */
    public final boolean hasExtension(
        GeneratedExtension<MessageType, ?> extension) {
      return internalGetResult().hasExtension(extension);
    }

    /** Get the number of elements in a repeated extension. */
    public final <Type> int getExtensionCount(
        GeneratedExtension<MessageType, List<Type>> extension) {
      return internalGetResult().getExtensionCount(extension);
    }

    /** Get the value of an extension. */
    public final <Type> Type getExtension(
        GeneratedExtension<MessageType, Type> extension) {
      return internalGetResult().getExtension(extension);
    }

    /** Get one element of a repeated extension. */
    public final <Type> Type getExtension(
        GeneratedExtension<MessageType, List<Type>> extension, int index) {
      return internalGetResult().getExtension(extension, index);
    }

    /** Set the value of an extension. */
    public final <Type> BuilderType setExtension(
        GeneratedExtension<MessageType, Type> extension, Type value) {
      ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.setField(extension.getDescriptor(),
                                  extension.toReflectionType(value));
      return (BuilderType)this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <Type> BuilderType setExtension(
        GeneratedExtension<MessageType, List<Type>> extension,
        int index, Type value) {
      ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.setRepeatedField(
        extension.getDescriptor(), index,
        extension.singularToReflectionType(value));
      return (BuilderType)this;
    }

    /** Append a value to a repeated extension. */
    public final <Type> BuilderType addExtension(
        GeneratedExtension<MessageType, List<Type>> extension, Type value) {
      ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.addRepeatedField(
        extension.getDescriptor(), extension.singularToReflectionType(value));
      return (BuilderType)this;
    }

    /** Clear an extension. */
    public final <Type> BuilderType clearExtension(
        GeneratedExtension<MessageType, ?> extension) {
      ExtendableMessage<MessageType> message = internalGetResult();
      message.verifyExtensionContainingType(extension);
      message.extensions.clearField(extension.getDescriptor());
      return (BuilderType)this;
    }

    /**
     * Called by subclasses to parse an unknown field or an extension.
     * @return {@code true} unless the tag is an end-group tag.
     */
    protected boolean parseUnknownField(CodedInputStream input,
                                        UnknownFieldSet.Builder unknownFields,
                                        ExtensionRegistry extensionRegistry,
                                        int tag)
                                 throws IOException {
      ExtendableMessage<MessageType> message = internalGetResult();
      return message.extensions.mergeFieldFrom(
        input, unknownFields, extensionRegistry, this, tag);
    }

    // ---------------------------------------------------------------
    // Reflection

    // We don't have to override the get*() methods here because they already
    // just forward to the underlying message.

    public BuilderType setField(FieldDescriptor field, Object value) {
      if (field.isExtension()) {
        ExtendableMessage<MessageType> message = internalGetResult();
        message.verifyContainingType(field);
        message.extensions.setField(field, value);
        return (BuilderType)this;
      } else {
        return super.setField(field, value);
      }
    }

    public BuilderType clearField(Descriptors.FieldDescriptor field) {
      if (field.isExtension()) {
        ExtendableMessage<MessageType> message = internalGetResult();
        message.verifyContainingType(field);
        message.extensions.clearField(field);
        return (BuilderType)this;
      } else {
        return super.clearField(field);
      }
    }

    public BuilderType setRepeatedField(Descriptors.FieldDescriptor field,
                                        int index, Object value) {
      if (field.isExtension()) {
        ExtendableMessage<MessageType> message = internalGetResult();
        message.verifyContainingType(field);
        message.extensions.setRepeatedField(field, index, value);
        return (BuilderType)this;
      } else {
        return super.setRepeatedField(field, index, value);
      }
    }

    public BuilderType addRepeatedField(Descriptors.FieldDescriptor field,
                                        Object value) {
      if (field.isExtension()) {
        ExtendableMessage<MessageType> message = internalGetResult();
        message.verifyContainingType(field);
        message.extensions.addRepeatedField(field, value);
        return (BuilderType)this;
      } else {
        return super.addRepeatedField(field, value);
      }
    }
  }

  // -----------------------------------------------------------------

  /** For use by generated code only. */
  public static <ContainingType extends Message, Type>
      GeneratedExtension<ContainingType, Type>
      newGeneratedExtension(FieldDescriptor descriptor, Class<Type> type) {
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
        FieldDescriptor descriptor, Class<Type> type) {
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

    private GeneratedExtension(FieldDescriptor descriptor, Class type) {
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
            (Message)invokeOrDie(getMethodOrDie(type, "getDefaultInstance"),
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
    private Object fromReflectionType(Object value) {
      if (descriptor.isRepeated()) {
        if (descriptor.getJavaType() == FieldDescriptor.JavaType.MESSAGE ||
            descriptor.getJavaType() == FieldDescriptor.JavaType.ENUM) {
          // Must convert the whole list.
          List result = new ArrayList();
          for (Object element : (List)value) {
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
    private Object singularFromReflectionType(Object value) {
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
                           .mergeFrom((Message)value).build();
          }
        case ENUM:
          return invokeOrDie(enumValueOf, null, (EnumValueDescriptor)value);
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
    private Object toReflectionType(Object value) {
      if (descriptor.isRepeated()) {
        if (descriptor.getJavaType() == FieldDescriptor.JavaType.ENUM) {
          // Must convert the whole list.
          List result = new ArrayList();
          for (Object element : (List)value) {
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
    private Object singularToReflectionType(Object value) {
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
      Class clazz, String name, Class... params) {
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
      Method method, Object object, Object... params) {
    try {
      return method.invoke(object, params);
    } catch (IllegalAccessException e) {
      throw new RuntimeException(
        "Couldn't use Java reflection to implement protocol message " +
        "reflection.", e);
    } catch (java.lang.reflect.InvocationTargetException e) {
      Throwable cause = e.getCause();
      if (cause instanceof RuntimeException) {
        throw (RuntimeException)cause;
      } else if (cause instanceof Error) {
        throw (Error)cause;
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
        Descriptor descriptor,
        String[] camelCaseNames,
        Class<? extends GeneratedMessage> messageClass,
        Class<? extends GeneratedMessage.Builder> builderClass) {
      this.descriptor = descriptor;
      fields = new FieldAccessor[descriptor.getFields().size()];

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
    }

    private final Descriptor descriptor;
    private final FieldAccessor[] fields;

    /** Get the FieldAccessor for a particular field. */
    private FieldAccessor getField(FieldDescriptor field) {
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
    private static interface FieldAccessor {
      Object get(GeneratedMessage message);
      void set(GeneratedMessage.Builder builder, Object value);
      Object getRepeated(GeneratedMessage message, int index);
      void setRepeated(GeneratedMessage.Builder builder,
                       int index, Object value);
      void addRepeated(GeneratedMessage.Builder builder, Object value);
      boolean has(GeneratedMessage message);
      int getRepeatedCount(GeneratedMessage message);
      void clear(GeneratedMessage.Builder builder);
      Message.Builder newBuilder();
    }

    // ---------------------------------------------------------------

    private static class SingularFieldAccessor implements FieldAccessor {
      SingularFieldAccessor(
          FieldDescriptor descriptor, String camelCaseName,
          Class<? extends GeneratedMessage> messageClass,
          Class<? extends GeneratedMessage.Builder> builderClass) {
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
      Class type;
      Method getMethod;
      Method setMethod;
      Method hasMethod;
      Method clearMethod;

      public Object get(GeneratedMessage message) {
        return invokeOrDie(getMethod, message);
      }
      public void set(GeneratedMessage.Builder builder, Object value) {
        invokeOrDie(setMethod, builder, value);
      }
      public Object getRepeated(GeneratedMessage message, int index) {
        throw new UnsupportedOperationException(
          "getRepeatedField() called on a singular field.");
      }
      public void setRepeated(GeneratedMessage.Builder builder,
                              int index, Object value) {
        throw new UnsupportedOperationException(
          "setRepeatedField() called on a singular field.");
      }
      public void addRepeated(GeneratedMessage.Builder builder, Object value) {
        throw new UnsupportedOperationException(
          "addRepeatedField() called on a singular field.");
      }
      public boolean has(GeneratedMessage message) {
        return (Boolean)invokeOrDie(hasMethod, message);
      }
      public int getRepeatedCount(GeneratedMessage message) {
        throw new UnsupportedOperationException(
          "getRepeatedFieldSize() called on a singular field.");
      }
      public void clear(GeneratedMessage.Builder builder) {
        invokeOrDie(clearMethod, builder);
      }
      public Message.Builder newBuilder() {
        throw new UnsupportedOperationException(
          "newBuilderForField() called on a non-Message type.");
      }
    }

    private static class RepeatedFieldAccessor implements FieldAccessor {
      RepeatedFieldAccessor(
          FieldDescriptor descriptor, String camelCaseName,
          Class<? extends GeneratedMessage> messageClass,
          Class<? extends GeneratedMessage.Builder> builderClass) {
        getMethod = getMethodOrDie(messageClass, "get" + camelCaseName + "List");

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

      Class type;
      Method getMethod;
      Method getRepeatedMethod;
      Method setRepeatedMethod;
      Method addRepeatedMethod;
      Method getCountMethod;
      Method clearMethod;

      public Object get(GeneratedMessage message) {
        return invokeOrDie(getMethod, message);
      }
      public void set(GeneratedMessage.Builder builder, Object value) {
        // Add all the elements individually.  This serves two purposes:
        // 1) Verifies that each element has the correct type.
        // 2) Insures that the caller cannot modify the list later on and
        //    have the modifications be reflected in the message.
        clear(builder);
        for (Object element : (List)value) {
          addRepeated(builder, element);
        }
      }
      public Object getRepeated(GeneratedMessage message, int index) {
        return invokeOrDie(getRepeatedMethod, message, index);
      }
      public void setRepeated(GeneratedMessage.Builder builder,
                              int index, Object value) {
        invokeOrDie(setRepeatedMethod, builder, index, value);
      }
      public void addRepeated(GeneratedMessage.Builder builder, Object value) {
        invokeOrDie(addRepeatedMethod, builder, value);
      }
      public boolean has(GeneratedMessage message) {
        throw new UnsupportedOperationException(
          "hasField() called on a singular field.");
      }
      public int getRepeatedCount(GeneratedMessage message) {
        return (Integer)invokeOrDie(getCountMethod, message);
      }
      public void clear(GeneratedMessage.Builder builder) {
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
          FieldDescriptor descriptor, String camelCaseName,
          Class<? extends GeneratedMessage> messageClass,
          Class<? extends GeneratedMessage.Builder> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        valueOfMethod = getMethodOrDie(type, "valueOf",
                                       EnumValueDescriptor.class);
        getValueDescriptorMethod =
          getMethodOrDie(type, "getValueDescriptor");
      }

      private Method valueOfMethod;
      private Method getValueDescriptorMethod;

      public Object get(GeneratedMessage message) {
        return invokeOrDie(getValueDescriptorMethod, super.get(message));
      }
      public void set(GeneratedMessage.Builder builder, Object value) {
        super.set(builder, invokeOrDie(valueOfMethod, null, value));
      }
    }

    private static final class RepeatedEnumFieldAccessor
        extends RepeatedFieldAccessor {
      RepeatedEnumFieldAccessor(
          FieldDescriptor descriptor, String camelCaseName,
          Class<? extends GeneratedMessage> messageClass,
          Class<? extends GeneratedMessage.Builder> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        valueOfMethod = getMethodOrDie(type, "valueOf",
                                       EnumValueDescriptor.class);
        getValueDescriptorMethod =
          getMethodOrDie(type, "getValueDescriptor");
      }

      private Method valueOfMethod;
      private Method getValueDescriptorMethod;

      @SuppressWarnings("unchecked")
      public Object get(GeneratedMessage message) {
        List newList = new ArrayList();
        for (Object element : (List)super.get(message)) {
          newList.add(invokeOrDie(getValueDescriptorMethod, element));
        }
        return Collections.unmodifiableList(newList);
      }
      public Object getRepeated(GeneratedMessage message, int index) {
        return invokeOrDie(getValueDescriptorMethod,
          super.getRepeated(message, index));
      }
      public void setRepeated(GeneratedMessage.Builder builder,
                              int index, Object value) {
        super.setRepeated(builder, index, invokeOrDie(valueOfMethod, null, value));
      }
      public void addRepeated(GeneratedMessage.Builder builder, Object value) {
        super.addRepeated(builder, invokeOrDie(valueOfMethod, null, value));
      }
    }

    // ---------------------------------------------------------------

    private static final class SingularMessageFieldAccessor
        extends SingularFieldAccessor {
      SingularMessageFieldAccessor(
          FieldDescriptor descriptor, String camelCaseName,
          Class<? extends GeneratedMessage> messageClass,
          Class<? extends GeneratedMessage.Builder> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        newBuilderMethod = getMethodOrDie(type, "newBuilder");
      }

      private Method newBuilderMethod;

      private Object coerceType(Object value) {
        if (type.isInstance(value)) {
          return value;
        } else {
          // The value is not the exact right message type.  However, if it
          // is an alternative implementation of the same type -- e.g. a
          // DynamicMessage -- we should accept it.  In this case we can make
          // a copy of the message.
          return ((Message.Builder)invokeOrDie(newBuilderMethod, null))
                  .mergeFrom((Message)value).build();
        }
      }

      public void set(GeneratedMessage.Builder builder, Object value) {
        super.set(builder, coerceType(value));
      }
      public Message.Builder newBuilder() {
        return (Message.Builder)invokeOrDie(newBuilderMethod, null);
      }
    }

    private static final class RepeatedMessageFieldAccessor
        extends RepeatedFieldAccessor {
      RepeatedMessageFieldAccessor(
          FieldDescriptor descriptor, String camelCaseName,
          Class<? extends GeneratedMessage> messageClass,
          Class<? extends GeneratedMessage.Builder> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        newBuilderMethod = getMethodOrDie(type, "newBuilder");
      }

      private Method newBuilderMethod;

      private Object coerceType(Object value) {
        if (type.isInstance(value)) {
          return value;
        } else {
          // The value is not the exact right message type.  However, if it
          // is an alternative implementation of the same type -- e.g. a
          // DynamicMessage -- we should accept it.  In this case we can make
          // a copy of the message.
          return ((Message.Builder)invokeOrDie(newBuilderMethod, null))
                  .mergeFrom((Message)value).build();
        }
      }

      public void setRepeated(GeneratedMessage.Builder builder,
                              int index, Object value) {
        super.setRepeated(builder, index, coerceType(value));
      }
      public void addRepeated(GeneratedMessage.Builder builder, Object value) {
        super.addRepeated(builder, coerceType(value));
      }
      public Message.Builder newBuilder() {
        return (Message.Builder)invokeOrDie(newBuilderMethod, null);
      }
    }
  }
}
