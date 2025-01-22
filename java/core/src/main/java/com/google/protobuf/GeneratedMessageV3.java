// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Internal.BooleanList;
import com.google.protobuf.Internal.DoubleList;
import com.google.protobuf.Internal.FloatList;
import com.google.protobuf.Internal.IntList;
import com.google.protobuf.Internal.LongList;
import com.google.protobuf.Internal.ProtobufList;
// In opensource protobuf, we have versioned this GeneratedMessageV3 class to GeneratedMessageV3 and
// in the future may have GeneratedMessageV4 etc. This allows us to change some aspects of this
// class without breaking binary compatibility with old generated code that still subclasses
// the old GeneratedMessageV3 class. To allow these different GeneratedMessageV? classes to
// interoperate (e.g., a GeneratedMessageV3 object has a message extension field whose class
// type is GeneratedMessageV4), these classes still share a common parent class AbstractMessage
// and are using the same GeneratedMessage.GeneratedExtension class for extension definitions.
// Since this class becomes GeneratedMessageV? in opensource, we have to add an import here
// to be able to use GeneratedMessage.GeneratedExtension. The GeneratedExtension definition in
// this file is also excluded from opensource to avoid conflict.
import com.google.protobuf.GeneratedMessage.GeneratedExtension;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectStreamException;
import java.io.Serializable;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * All generated protocol message classes extend this class. This class implements most of the
 * Message and Builder interfaces using Java reflection. Users can ignore this class and pretend
 * that generated messages implement the Message interface directly.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class GeneratedMessageV3 extends AbstractMessage implements Serializable {
  private static final long serialVersionUID = 1L;

  /**
   * For testing. Allows a test to disable the optimization that avoids using field builders for
   * nested messages until they are requested. By disabling this optimization, existing tests can be
   * reused to test the field builders.
   */
  protected static boolean alwaysUseFieldBuilders = false;

  /**
   * For use by generated code only.
   *
   * <p>TODO: mark this private and final (breaking change)
   */
  protected UnknownFieldSet unknownFields;

  protected GeneratedMessageV3() {
    unknownFields = UnknownFieldSet.getDefaultInstance();
  }

  protected GeneratedMessageV3(Builder<?> builder) {
    unknownFields = builder.getUnknownFields();
  }

  /** TODO: Remove this unnecessary intermediate implementation of this method. */
  @Override
  public Parser<? extends GeneratedMessageV3> getParserForType() {
    throw new UnsupportedOperationException("This is supposed to be overridden by subclasses.");
  }

  /**
   * TODO: Stop using SingleFieldBuilder and remove this setting
   *
   * @see #setAlwaysUseFieldBuildersForTesting(boolean)
   */
  static void enableAlwaysUseFieldBuildersForTesting() {
    setAlwaysUseFieldBuildersForTesting(true);
  }

  /**
   * For testing. Allows a test to disable/re-enable the optimization that avoids using field
   * builders for nested messages until they are requested. By disabling this optimization, existing
   * tests can be reused to test the field builders. See {@link RepeatedFieldBuilder} and {@link
   * SingleFieldBuilder}.
   *
   * <p>TODO: Stop using SingleFieldBuilder and remove this setting
   */
  static void setAlwaysUseFieldBuildersForTesting(boolean useBuilders) {
    alwaysUseFieldBuilders = useBuilders;
  }

  /**
   * Get the FieldAccessorTable for this type. We can't have the message class pass this in to the
   * constructor because of bootstrapping trouble with DescriptorProtos.
   */
  protected abstract FieldAccessorTable internalGetFieldAccessorTable();

  @Override
  public Descriptor getDescriptorForType() {
    return internalGetFieldAccessorTable().descriptor;
  }

  /**
   * TODO: This method should be removed. It enables parsing directly into an
   * "immutable" message. Have to leave it for now to support old gencode.
   *
   * @deprecated use newBuilder().mergeFrom() instead
   */
  @Deprecated
  protected void mergeFromAndMakeImmutableInternal(
      CodedInputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    Schema<GeneratedMessageV3> schema = Protobuf.getInstance().schemaFor(this);
    try {
      schema.mergeFrom(this, CodedInputStreamReader.forCodedInput(input), extensionRegistry);
    } catch (InvalidProtocolBufferException e) {
      throw e.setUnfinishedMessage(this);
    } catch (IOException e) {
      throw new InvalidProtocolBufferException(e).setUnfinishedMessage(this);
    }
    schema.makeImmutable(this);
  }

  /**
   * Internal helper to return a modifiable map containing all the fields. The returned Map is
   * modifiable so that the caller can add additional extension fields to implement {@link
   * #getAllFields()}.
   *
   * @param getBytesForString whether to generate ByteString for string fields
   */
  private Map<FieldDescriptor, Object> getAllFieldsMutable(boolean getBytesForString) {
    final TreeMap<FieldDescriptor, Object> result = new TreeMap<>();
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

  // TODO: compute this at {@code build()} time in the Builder class.
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
          @SuppressWarnings("unchecked")
          final List<Message> messageList = (List<Message>) getField(field);
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
    return Collections.unmodifiableMap(getAllFieldsMutable(/* getBytesForString= */ false));
  }

  /**
   * Returns a collection of all the fields in this message which are set and their corresponding
   * values. A singular ("required" or "optional") field is set iff hasField() returns true for that
   * field. A "repeated" field is set iff getRepeatedFieldCount() is greater than zero. The values
   * are exactly what would be returned by calling {@link #getFieldRaw(Descriptors.FieldDescriptor)}
   * for each field. The map is guaranteed to be a sorted map, so iterating over it will return
   * fields in order by field number.
   */
  Map<FieldDescriptor, Object> getAllFieldsRaw() {
    return Collections.unmodifiableMap(getAllFieldsMutable(/* getBytesForString= */ true));
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
   * Obtains the value of the given field, or the default value if it is not set. For primitive
   * fields, the boxed primitive value is returned. For enum fields, the EnumValueDescriptor for the
   * value is returned. For embedded message fields, the sub-message is returned. For repeated
   * fields, a java.util.List is returned. For present string fields, a ByteString is returned
   * representing the bytes that the field contains.
   */
  Object getFieldRaw(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).getRaw(this);
  }

  @Override
  public int getRepeatedFieldCount(final FieldDescriptor field) {
    return internalGetFieldAccessorTable().getField(field).getRepeatedCount(this);
  }

  @Override
  public Object getRepeatedField(final FieldDescriptor field, final int index) {
    return internalGetFieldAccessorTable().getField(field).getRepeated(this, index);
  }

  // TODO: This method should be final.
  @Override
  public UnknownFieldSet getUnknownFields() {
    return unknownFields;
  }

  // TODO: This should go away when Schema classes cannot modify immutable
  // GeneratedMessageV3 objects anymore.
  void setUnknownFields(UnknownFieldSet unknownFields) {
    this.unknownFields = unknownFields;
  }

  /**
   * Called by subclasses to parse an unknown field.
   *
   * <p>TODO remove this method
   *
   * @return {@code true} unless the tag is an end-group tag.
   */
  protected boolean parseUnknownField(
      CodedInputStream input,
      UnknownFieldSet.Builder unknownFields,
      ExtensionRegistryLite extensionRegistry,
      int tag)
      throws IOException {
    if (input.shouldDiscardUnknownFields()) {
      return input.skipField(tag);
    }
    return unknownFields.mergeFieldFrom(tag, input);
  }

  /**
   * Delegates to parseUnknownField. This method is obsolete, but we must retain it for
   * compatibility with older generated code.
   *
   * <p>TODO remove this method
   */
  protected boolean parseUnknownFieldProto3(
      CodedInputStream input,
      UnknownFieldSet.Builder unknownFields,
      ExtensionRegistryLite extensionRegistry,
      int tag)
      throws IOException {
    return parseUnknownField(input, unknownFields, extensionRegistry, tag);
  }

  /** Used by generated code. */
  @SuppressWarnings("ProtoParseWithRegistry")
  protected static <M extends Message> M parseWithIOException(Parser<M> parser, InputStream input)
      throws IOException {
    try {
      return parser.parseFrom(input);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  /** Used by generated code. */
  protected static <M extends Message> M parseWithIOException(
      Parser<M> parser, InputStream input, ExtensionRegistryLite extensions) throws IOException {
    try {
      return parser.parseFrom(input, extensions);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  /** Used by generated code. */
  @SuppressWarnings("ProtoParseWithRegistry")
  protected static <M extends Message> M parseWithIOException(
      Parser<M> parser, CodedInputStream input) throws IOException {
    try {
      return parser.parseFrom(input);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  /** Used by generated code. */
  protected static <M extends Message> M parseWithIOException(
      Parser<M> parser, CodedInputStream input, ExtensionRegistryLite extensions)
      throws IOException {
    try {
      return parser.parseFrom(input, extensions);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  /** Used by generated code. */
  @SuppressWarnings("ProtoParseWithRegistry")
  protected static <M extends Message> M parseDelimitedWithIOException(
      Parser<M> parser, InputStream input) throws IOException {
    try {
      return parser.parseDelimitedFrom(input);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  /** Used by generated code. */
  protected static <M extends Message> M parseDelimitedWithIOException(
      Parser<M> parser, InputStream input, ExtensionRegistryLite extensions) throws IOException {
    try {
      return parser.parseDelimitedFrom(input, extensions);
    } catch (InvalidProtocolBufferException e) {
      throw e.unwrapIOException();
    }
  }

  protected static boolean canUseUnsafe() {
    return UnsafeUtil.hasUnsafeArrayOperations() && UnsafeUtil.hasUnsafeByteBufferOperations();
  }

  protected static IntList emptyIntList() {
    return IntArrayList.emptyList();
  }

  // TODO: Unused. Remove.
  protected static IntList newIntList() {
    return new IntArrayList();
  }

  // TODO: Redundant with makeMutableCopy(). Remove.
  protected static IntList mutableCopy(IntList list) {
    return makeMutableCopy(list);
  }

  // TODO: Redundant with makeMutableCopy(). Remove.
  protected static LongList mutableCopy(LongList list) {
    return makeMutableCopy(list);
  }

  // TODO: Redundant with makeMutableCopy(). Remove.
  protected static FloatList mutableCopy(FloatList list) {
    return makeMutableCopy(list);
  }

  // TODO: Redundant with makeMutableCopy(). Remove.
  protected static DoubleList mutableCopy(DoubleList list) {
    return makeMutableCopy(list);
  }

  // TODO: Redundant with makeMutableCopy(). Remove.
  protected static BooleanList mutableCopy(BooleanList list) {
    return makeMutableCopy(list);
  }

  protected static LongList emptyLongList() {
    return LongArrayList.emptyList();
  }

  // TODO: Unused. Remove.
  protected static LongList newLongList() {
    return new LongArrayList();
  }

  protected static FloatList emptyFloatList() {
    return FloatArrayList.emptyList();
  }

  // TODO: Unused. Remove.
  protected static FloatList newFloatList() {
    return new FloatArrayList();
  }

  protected static DoubleList emptyDoubleList() {
    return DoubleArrayList.emptyList();
  }

  // TODO: Unused. Remove.
  protected static DoubleList newDoubleList() {
    return new DoubleArrayList();
  }

  protected static BooleanList emptyBooleanList() {
    return BooleanArrayList.emptyList();
  }

  // TODO: Unused. Remove.
  protected static BooleanList newBooleanList() {
    return new BooleanArrayList();
  }

  protected static <ListT extends ProtobufList<?>> ListT makeMutableCopy(ListT list) {
    return makeMutableCopy(list, 0);
  }

  @SuppressWarnings("unchecked") // Guaranteed by proto runtime.
  protected static <ListT extends ProtobufList<?>> ListT makeMutableCopy(
      ListT list, int minCapacity) {
    int size = list.size();
    if (minCapacity <= size) {
      minCapacity = size * 2;
    }
    if (minCapacity <= 0) {
      minCapacity = AbstractProtobufList.DEFAULT_CAPACITY;
    }

    return (ListT) list.mutableCopyWithCapacity(minCapacity);
  }

  @SuppressWarnings("unchecked") // The empty list can be safely cast
  protected static <T> ProtobufList<T> emptyList(Class<T> elementType) {
    return (ProtobufList<T>) ProtobufArrayList.emptyList();
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
   * This class is used to make a generated protected method inaccessible from user's code (e.g.,
   * the {@link #newInstance} method below). When this class is used as a parameter's type in a
   * generated protected method, the method is visible to user's code in the same package, but since
   * the constructor of this class is private to protobuf runtime, user's code can't obtain an
   * instance of this class and as such can't actually make a method call on the protected method.
   */
  protected static final class UnusedPrivateParameter {
    static final UnusedPrivateParameter INSTANCE = new UnusedPrivateParameter();

    private UnusedPrivateParameter() {}
  }

  /** Creates a new instance of this message type. Overridden in the generated code. */
  @SuppressWarnings({"unused"})
  protected Object newInstance(UnusedPrivateParameter unused) {
    throw new UnsupportedOperationException("This method must be overridden by the subclass.");
  }

  /**
   * Used by parsing constructors in generated classes.
   *
   * <p>TODO: remove unused method (extensions should be immutable after build)
   */
  protected void makeExtensionsImmutable() {
    // Noop for messages without extensions.
    GeneratedMessage.warnPre22Gencode();
  }

  /**
   * TODO: remove this after b/29368482 is fixed. We need to move this interface to
   * AbstractMessage in order to versioning GeneratedMessageV3 but this move breaks binary
   * compatibility for AppEngine. After AppEngine is fixed we can exclude this from google3.
   *
   * <p>TODO: Remove at breaking change since b/29368482 was fixed in 2020
   */
  protected interface BuilderParent extends AbstractMessage.BuilderParent {}

  /** TODO: remove this together with GeneratedMessageV3.BuilderParent. */
  protected abstract Message.Builder newBuilderForType(BuilderParent parent);

  /** TODO: generated class should implement this directly */
  @Override
  protected Message.Builder newBuilderForType(final AbstractMessage.BuilderParent parent) {
    return newBuilderForType(
        new BuilderParent() {
          @Override
          public void markDirty() {
            parent.markDirty();
          }
        });
  }

  /** Builder class for {@link GeneratedMessageV3}. */
  @SuppressWarnings("unchecked")
  public abstract static class Builder<BuilderT extends Builder<BuilderT>>
      extends AbstractMessage.Builder<BuilderT> {

    private BuilderParent builderParent;

    private BuilderParentImpl meAsParent;

    // Indicates that we've built a message and so we are now obligated
    // to dispatch dirty invalidations. See GeneratedMessageV3.BuilderListener.
    private boolean isClean;

    /**
     * This field holds either an {@link UnknownFieldSet} or {@link UnknownFieldSet.Builder}.
     *
     * <p>We use an object because it should only be one or the other of those things at a time and
     * Object is the only common base. This also saves space.
     *
     * <p>Conversions are lazy: if {@link #setUnknownFields} is called, this will contain {@link
     * UnknownFieldSet}. If unknown fields are merged into this builder, the current {@link
     * UnknownFieldSet} will be converted to a {@link UnknownFieldSet.Builder} and left that way
     * until either {@link #setUnknownFields} or {@link #buildPartial} or {@link #build} is called.
     */
    private Object unknownFieldsOrBuilder = UnknownFieldSet.getDefaultInstance();

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

    /** Called by the subclass when a message is built. */
    protected void onBuilt() {
      if (builderParent != null) {
        markClean();
      }
    }

    /**
     * Called by the subclass or a builder to notify us that a message was built and may be cached
     * and therefore invalidations are needed.
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
    public BuilderT clone() {
      BuilderT builder = (BuilderT) getDefaultInstanceForType().newBuilderForType();
      builder.mergeFrom(buildPartial());
      return builder;
    }

    /**
     * Called by the initialization and clear code paths to allow subclasses to reset any of their
     * builtin fields back to the initial values.
     */
    @Override
    public BuilderT clear() {
      unknownFieldsOrBuilder = UnknownFieldSet.getDefaultInstance();
      onChanged();
      return (BuilderT) this;
    }

    /**
     * Get the FieldAccessorTable for this type. We can't have the message class pass this in to the
     * constructor because of bootstrapping trouble with DescriptorProtos.
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
      final TreeMap<FieldDescriptor, Object> result = new TreeMap<>();
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
      return internalGetFieldAccessorTable().getField(field).getRepeatedBuilder(this, index);
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
        return Collections.unmodifiableList((List<?>) object);
      } else {
        return object;
      }
    }

    @Override
    public BuilderT setField(final FieldDescriptor field, final Object value) {
      internalGetFieldAccessorTable().getField(field).set(this, value);
      return (BuilderT) this;
    }

    @Override
    public BuilderT clearField(final FieldDescriptor field) {
      internalGetFieldAccessorTable().getField(field).clear(this);
      return (BuilderT) this;
    }

    @Override
    public BuilderT clearOneof(final OneofDescriptor oneof) {
      internalGetFieldAccessorTable().getOneof(oneof).clear(this);
      return (BuilderT) this;
    }

    @Override
    public int getRepeatedFieldCount(final FieldDescriptor field) {
      return internalGetFieldAccessorTable().getField(field).getRepeatedCount(this);
    }

    @Override
    public Object getRepeatedField(final FieldDescriptor field, final int index) {
      return internalGetFieldAccessorTable().getField(field).getRepeated(this, index);
    }

    @Override
    public BuilderT setRepeatedField(
        final FieldDescriptor field, final int index, final Object value) {
      internalGetFieldAccessorTable().getField(field).setRepeated(this, index, value);
      return (BuilderT) this;
    }

    @Override
    public BuilderT addRepeatedField(final FieldDescriptor field, final Object value) {
      internalGetFieldAccessorTable().getField(field).addRepeated(this, value);
      return (BuilderT) this;
    }

    private BuilderT setUnknownFieldsInternal(final UnknownFieldSet unknownFields) {
      unknownFieldsOrBuilder = unknownFields;
      onChanged();
      return (BuilderT) this;
    }

    @Override
    public BuilderT setUnknownFields(final UnknownFieldSet unknownFields) {
      return setUnknownFieldsInternal(unknownFields);
    }

    /**
     * This method is obsolete, but we must retain it for compatibility with older generated code.
     */
    protected BuilderT setUnknownFieldsProto3(final UnknownFieldSet unknownFields) {
      return setUnknownFieldsInternal(unknownFields);
    }

    @Override
    public BuilderT mergeUnknownFields(final UnknownFieldSet unknownFields) {
      if (UnknownFieldSet.getDefaultInstance().equals(unknownFields)) {
        return (BuilderT) this;
      }

      if (UnknownFieldSet.getDefaultInstance().equals(unknownFieldsOrBuilder)) {
        unknownFieldsOrBuilder = unknownFields;
        onChanged();
        return (BuilderT) this;
      }

      getUnknownFieldSetBuilder().mergeFrom(unknownFields);
      onChanged();
      return (BuilderT) this;
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
            @SuppressWarnings("unchecked")
            final List<Message> messageList = (List<Message>) getField(field);
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
    public final UnknownFieldSet getUnknownFields() {
      if (unknownFieldsOrBuilder instanceof UnknownFieldSet) {
        return (UnknownFieldSet) unknownFieldsOrBuilder;
      } else {
        return ((UnknownFieldSet.Builder) unknownFieldsOrBuilder).buildPartial();
      }
    }

    /**
     * Called by generated subclasses to parse an unknown field.
     *
     * @return {@code true} unless the tag is an end-group tag.
     */
    protected boolean parseUnknownField(
        CodedInputStream input, ExtensionRegistryLite extensionRegistry, int tag)
        throws IOException {
      if (input.shouldDiscardUnknownFields()) {
        return input.skipField(tag);
      }
      return getUnknownFieldSetBuilder().mergeFieldFrom(tag, input);
    }

    /** Called by generated subclasses to add to the unknown field set. */
    protected final void mergeUnknownLengthDelimitedField(int number, ByteString bytes) {
      getUnknownFieldSetBuilder().mergeLengthDelimitedField(number, bytes);
    }

    /** Called by generated subclasses to add to the unknown field set. */
    protected final void mergeUnknownVarintField(int number, int value) {
      getUnknownFieldSetBuilder().mergeVarintField(number, value);
    }

    @Override
    protected UnknownFieldSet.Builder getUnknownFieldSetBuilder() {
      if (unknownFieldsOrBuilder instanceof UnknownFieldSet) {
        unknownFieldsOrBuilder = ((UnknownFieldSet) unknownFieldsOrBuilder).toBuilder();
      }
      onChanged();
      return (UnknownFieldSet.Builder) unknownFieldsOrBuilder;
    }

    @Override
    protected void setUnknownFieldSetBuilder(UnknownFieldSet.Builder builder) {
      unknownFieldsOrBuilder = builder;
      onChanged();
    }

    /**
     * Implementation of {@link BuilderParent} for giving to our children. This small inner class
     * makes it so we don't publicly expose the BuilderParent methods.
     */
    private class BuilderParentImpl implements BuilderParent {

      @Override
      public void markDirty() {
        onChanged();
      }
    }

    /**
     * Gets the {@link BuilderParent} for giving to our children.
     *
     * @return The builder parent for our children.
     */
    protected BuilderParent getParentForChildren() {
      if (meAsParent == null) {
        meAsParent = new BuilderParentImpl();
      }
      return meAsParent;
    }

    /**
     * Called when a builder or one of its nested children has changed and any parent should be
     * notified of its invalidation.
     */
    protected final void onChanged() {
      if (isClean && builderParent != null) {
        builderParent.markDirty();

        // Don't keep dispatching invalidations until build is called again.
        isClean = false;
      }
    }

    /**
     * Gets the map field with the given field number. This method should be overridden in the
     * generated message class if the message contains map fields.
     *
     * <p>Unlike other field types, reflection support for map fields can't be implemented based on
     * generated public API because we need to access a map field as a list in reflection API but
     * the generated API only allows us to access it as a map. This method returns the underlying
     * map field directly and thus enables us to access the map field as a list.
     */
    @SuppressWarnings({"unused", "rawtypes"})
    protected MapFieldReflectionAccessor internalGetMapFieldReflection(int fieldNumber) {
      return internalGetMapField(fieldNumber);
    }

    /** TODO: Remove, exists for compatibility with generated code. */
    @Deprecated
    @SuppressWarnings({"unused", "rawtypes"})
    protected MapField internalGetMapField(int fieldNumber) {
      // Note that we can't use descriptor names here because this method will
      // be called when descriptor is being initialized.
      throw new IllegalArgumentException("No map fields found in " + getClass().getName());
    }

    /** Like {@link #internalGetMapFieldReflection} but return a mutable version. */
    @SuppressWarnings({"unused", "rawtypes"})
    protected MapFieldReflectionAccessor internalGetMutableMapFieldReflection(int fieldNumber) {
      return internalGetMutableMapField(fieldNumber);
    }

    /** TODO: Remove, exists for compatibility with generated code. */
    @Deprecated
    @SuppressWarnings({"unused", "rawtypes"})
    protected MapField internalGetMutableMapField(int fieldNumber) {
      // Note that we can't use descriptor names here because this method will
      // be called when descriptor is being initialized.
      throw new IllegalArgumentException("No map fields found in " + getClass().getName());
    }
  }

  // =================================================================
  // Extensions-related stuff

  /** Extends {@link MessageOrBuilder} with extension-related functions. */
  public interface ExtendableMessageOrBuilder<MessageT extends ExtendableMessage<MessageT>>
      extends MessageOrBuilder {
    // Re-define for return type covariance.
    @Override
    Message getDefaultInstanceForType();

    /** Check if a singular extension is present. */
    <T> boolean hasExtension(ExtensionLite<MessageT, T> extension);

    /** Get the number of elements in a repeated extension. */
    <T> int getExtensionCount(ExtensionLite<MessageT, List<T>> extension);

    /** Get the value of an extension. */
    <T> T getExtension(ExtensionLite<MessageT, T> extension);

    /** Get one element of a repeated extension. */
    <T> T getExtension(ExtensionLite<MessageT, List<T>> extension, int index);

    /**
     * Check if a singular extension is present.
     * <p>TODO: handled by ExtensionLite version
     */
    <T> boolean hasExtension(
        Extension<MessageT, T> extension);
    /**
     * Check if a singular extension is present.
     * <p>TODO: handled by ExtensionLite version
     */
    <T> boolean hasExtension(
        GeneratedExtension<MessageT, T> extension);
    /**
     * Get the number of elements in a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    <T> int getExtensionCount(
        Extension<MessageT, List<T>> extension);
    /**
     * Get the number of elements in a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    <T> int getExtensionCount(
        GeneratedExtension<MessageT, List<T>> extension);
    /**
     * Get the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    <T> T getExtension(
        Extension<MessageT, T> extension);
    /**
     * Get the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    <T> T getExtension(
        GeneratedExtension<MessageT, T> extension);
    /**
     * Get one element of a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    <T> T getExtension(
        Extension<MessageT, List<T>> extension,
        int index);
    /**
     * Get one element of a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    <T> T getExtension(
        GeneratedExtension<MessageT, List<T>> extension,
        int index);
  }

  /**
   * Generated message classes for message types that contain extension ranges subclass this.
   *
   * <p>This class implements type-safe accessors for extensions. They implement all the same
   * operations that you can do with normal fields -- e.g. "has", "get", and "getCount" -- but for
   * extensions. The extensions are identified using instances of the class {@link
   * GeneratedExtension}; the protocol compiler generates a static instance of this class for every
   * extension in its input. Through the magic of generics, all is made type-safe.
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
  public abstract static class ExtendableMessage<MessageT extends ExtendableMessage<MessageT>>
      extends GeneratedMessageV3 implements ExtendableMessageOrBuilder<MessageT> {

    private static final long serialVersionUID = 1L;

    private final FieldSet<FieldDescriptor> extensions;

    protected ExtendableMessage() {
      this.extensions = FieldSet.newFieldSet();
    }

    protected ExtendableMessage(ExtendableBuilder<MessageT, ?> builder) {
      super(builder);
      this.extensions = builder.buildExtensions();
    }

    private void verifyExtensionContainingType(final Extension<MessageT, ?> extension) {
      if (extension.getDescriptor().getContainingType() != getDescriptorForType()) {
        // This can only happen if someone uses unchecked operations.
        throw new IllegalArgumentException(
            "Extension is for type \""
                + extension.getDescriptor().getContainingType().getFullName()
                + "\" which does not match message type \""
                + getDescriptorForType().getFullName()
                + "\".");
      }
    }

    /** Check if a singular extension is present. */
    @Override
    public final <T> boolean hasExtension(final ExtensionLite<MessageT, T> extensionLite) {
      Extension<MessageT, T> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      return extensions.hasField(extension.getDescriptor());
    }

    /** Get the number of elements in a repeated extension. */
    @Override
    public final <T> int getExtensionCount(final ExtensionLite<MessageT, List<T>> extensionLite) {
      Extension<MessageT, List<T>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      return extensions.getRepeatedFieldCount(descriptor);
    }

    /** Get the value of an extension. */
    @Override
    @SuppressWarnings("unchecked")
    public final <T> T getExtension(final ExtensionLite<MessageT, T> extensionLite) {
      Extension<MessageT, T> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      final Object value = extensions.getField(descriptor);
      if (value == null) {
        if (descriptor.isRepeated()) {
          return (T) Collections.emptyList();
        } else if (descriptor.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          return (T) extension.getMessageDefaultInstance();
        } else {
          return (T) extension.fromReflectionType(descriptor.getDefaultValue());
        }
      } else {
        return (T) extension.fromReflectionType(value);
      }
    }

    /** Get one element of a repeated extension. */
    @Override
    @SuppressWarnings("unchecked")
    public final <T> T getExtension(
        final ExtensionLite<MessageT, List<T>> extensionLite, final int index) {
      Extension<MessageT, List<T>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      return (T)
          extension.singularFromReflectionType(extensions.getRepeatedField(descriptor, index));
    }

    /**
     * Check if a singular extension is present.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> boolean hasExtension(final Extension<MessageT, T> extension) {
      return hasExtension((ExtensionLite<MessageT, T>) extension);
    }
    /**
     * Check if a singular extension is present.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> boolean hasExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return hasExtension((ExtensionLite<MessageT, T>) extension);
    }
    /**
     * Get the number of elements in a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> int getExtensionCount(
        final Extension<MessageT, List<T>> extension) {
      return getExtensionCount((ExtensionLite<MessageT, List<T>>) extension);
    }
    /**
     * Get the number of elements in a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> int getExtensionCount(
        final GeneratedExtension<MessageT, List<T>> extension) {
      return getExtensionCount((ExtensionLite<MessageT, List<T>>) extension);
    }
    /**
     * Get the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> T getExtension(final Extension<MessageT, T> extension) {
      return getExtension((ExtensionLite<MessageT, T>) extension);
    }
    /**
     * Get the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> T getExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return getExtension((ExtensionLite<MessageT, T>) extension);
    }
    /**
     * Get one element of a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> T getExtension(
        final Extension<MessageT, List<T>> extension, final int index) {
      return getExtension((ExtensionLite<MessageT, List<T>>) extension, index);
    }
    /**
     * Get one element of a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> T getExtension(
        final GeneratedExtension<MessageT, List<T>> extension, final int index) {
      return getExtension((ExtensionLite<MessageT, List<T>>) extension, index);
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions.isInitialized();
    }

    // TODO: compute this in the builder at {@code build()} time.
    @Override
    public boolean isInitialized() {
      return super.isInitialized() && extensionsAreInitialized();
    }

    // TODO: remove mutating method from immutable type
    @Override
    protected boolean parseUnknownField(
        CodedInputStream input,
        UnknownFieldSet.Builder unknownFields,
        ExtensionRegistryLite extensionRegistry,
        int tag)
        throws IOException {
      return MessageReflection.mergeFieldFrom(
          input,
          input.shouldDiscardUnknownFields() ? null : unknownFields,
          extensionRegistry,
          getDescriptorForType(),
          new MessageReflection.ExtensionAdapter(extensions),
          tag);
    }

    /**
     * Delegates to parseUnknownField. This method is obsolete, but we must retain it for
     * compatibility with older generated code.
     *
     * <p>TODO: remove mutating method from immutable type
     */
    @Override
    protected boolean parseUnknownFieldProto3(
        CodedInputStream input,
        UnknownFieldSet.Builder unknownFields,
        ExtensionRegistryLite extensionRegistry,
        int tag)
        throws IOException {
      return parseUnknownField(input, unknownFields, extensionRegistry, tag);
    }

    /**
     * Used by parsing constructors in generated classes.
     *
     * <p>TODO: remove unused method (extensions should be immutable after build)
     */
    @Override
    protected void makeExtensionsImmutable() {
      GeneratedMessage.warnPre22Gencode();
      extensions.makeImmutable();
    }

    /**
     * Used by subclasses to serialize extensions. Extension ranges may be interleaved with field
     * numbers, but we must write them in canonical (sorted by field number) order. ExtensionWriter
     * helps us write individual ranges of extensions at once.
     */
    protected class ExtensionWriter {
      // Imagine how much simpler this code would be if Java iterators had
      // a way to get the next element without advancing the iterator.

      private final Iterator<Map.Entry<FieldDescriptor, Object>> iter = extensions.iterator();
      private Map.Entry<FieldDescriptor, Object> next;
      private final boolean messageSetWireFormat;

      private ExtensionWriter(final boolean messageSetWireFormat) {
        if (iter.hasNext()) {
          next = iter.next();
        }
        this.messageSetWireFormat = messageSetWireFormat;
      }

      public void writeUntil(final int end, final CodedOutputStream output) throws IOException {
        while (next != null && next.getKey().getNumber() < end) {
          FieldDescriptor descriptor = next.getKey();
          if (messageSetWireFormat
              && descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE
              && !descriptor.isRepeated()) {
            if (next instanceof LazyField.LazyEntry<?>) {
              output.writeRawMessageSetExtension(
                  descriptor.getNumber(),
                  ((LazyField.LazyEntry<?>) next).getField().toByteString());
            } else {
              output.writeMessageSetExtension(descriptor.getNumber(), (Message) next.getValue());
            }
          } else {
            // TODO: Taken care of following code, it may cause
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
          super.getAllFieldsMutable(/* getBytesForString= */ false);
      result.putAll(getExtensionFields());
      return Collections.unmodifiableMap(result);
    }

    @Override
    public Map<FieldDescriptor, Object> getAllFieldsRaw() {
      final Map<FieldDescriptor, Object> result =
          super.getAllFieldsMutable(/* getBytesForString= */ false);
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
    public Object getRepeatedField(final FieldDescriptor field, final int index) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions.getRepeatedField(field, index);
      } else {
        return super.getRepeatedField(field, index);
      }
    }

    private void verifyContainingType(final FieldDescriptor field) {
      if (field.getContainingType() != getDescriptorForType()) {
        throw new IllegalArgumentException("FieldDescriptor does not match message type.");
      }
    }
  }

  /**
   * Generated message builders for message types that contain extension ranges subclass this.
   *
   * <p>This class implements type-safe accessors for extensions. They implement all the same
   * operations that you can do with normal fields -- e.g. "get", "set", and "add" -- but for
   * extensions. The extensions are identified using instances of the class {@link
   * GeneratedExtension}; the protocol compiler generates a static instance of this class for every
   * extension in its input. Through the magic of generics, all is made type-safe.
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
          MessageT extends ExtendableMessage<MessageT>,
          BuilderT extends ExtendableBuilder<MessageT, BuilderT>>
      extends Builder<BuilderT> implements ExtendableMessageOrBuilder<MessageT> {

    private FieldSet.Builder<FieldDescriptor> extensions;

    protected ExtendableBuilder() {}

    protected ExtendableBuilder(BuilderParent parent) {
      super(parent);
    }

    // For immutable message conversion.
    void internalSetExtensionSet(FieldSet<FieldDescriptor> extensions) {
      this.extensions = FieldSet.Builder.fromFieldSet(extensions);
    }

    @Override
    public BuilderT clear() {
      extensions = null;
      return super.clear();
    }

    private void ensureExtensionsIsMutable() {
      if (extensions == null) {
        extensions = FieldSet.newBuilder();
      }
    }

    private void verifyExtensionContainingType(final Extension<MessageT, ?> extension) {
      if (extension.getDescriptor().getContainingType() != getDescriptorForType()) {
        // This can only happen if someone uses unchecked operations.
        throw new IllegalArgumentException(
            "Extension is for type \""
                + extension.getDescriptor().getContainingType().getFullName()
                + "\" which does not match message type \""
                + getDescriptorForType().getFullName()
                + "\".");
      }
    }

    /** Check if a singular extension is present. */
    @Override
    public final <T> boolean hasExtension(final ExtensionLite<MessageT, T> extensionLite) {
      Extension<MessageT, T> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      return extensions != null && extensions.hasField(extension.getDescriptor());
    }

    /** Get the number of elements in a repeated extension. */
    @Override
    public final <T> int getExtensionCount(final ExtensionLite<MessageT, List<T>> extensionLite) {
      Extension<MessageT, List<T>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      final FieldDescriptor descriptor = extension.getDescriptor();
      return extensions == null ? 0 : extensions.getRepeatedFieldCount(descriptor);
    }

    /** Get the value of an extension. */
    @Override
    public final <T> T getExtension(final ExtensionLite<MessageT, T> extensionLite) {
      Extension<MessageT, T> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      final Object value = extensions == null ? null : extensions.getField(descriptor);
      if (value == null) {
        if (descriptor.isRepeated()) {
          return (T) Collections.emptyList();
        } else if (descriptor.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          return (T) extension.getMessageDefaultInstance();
        } else {
          return (T) extension.fromReflectionType(descriptor.getDefaultValue());
        }
      } else {
        return (T) extension.fromReflectionType(value);
      }
    }

    /** Get one element of a repeated extension. */
    @Override
    public final <T> T getExtension(
        final ExtensionLite<MessageT, List<T>> extensionLite, final int index) {
      Extension<MessageT, List<T>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      FieldDescriptor descriptor = extension.getDescriptor();
      if (extensions == null) {
        throw new IndexOutOfBoundsException();
      }
      return (T)
          extension.singularFromReflectionType(extensions.getRepeatedField(descriptor, index));
    }

    /** Set the value of an extension. */
    public final <T> BuilderT setExtension(
        final ExtensionLite<MessageT, T> extensionLite, final T value) {
      Extension<MessageT, T> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      final FieldDescriptor descriptor = extension.getDescriptor();
      extensions.setField(descriptor, extension.toReflectionType(value));
      onChanged();
      return (BuilderT) this;
    }

    /** Set the value of one element of a repeated extension. */
    public final <T> BuilderT setExtension(
        final ExtensionLite<MessageT, List<T>> extensionLite, final int index, final T value) {
      Extension<MessageT, List<T>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      final FieldDescriptor descriptor = extension.getDescriptor();
      extensions.setRepeatedField(descriptor, index, extension.singularToReflectionType(value));
      onChanged();
      return (BuilderT) this;
    }

    /** Append a value to a repeated extension. */
    public final <T> BuilderT addExtension(
        final ExtensionLite<MessageT, List<T>> extensionLite, final T value) {
      Extension<MessageT, List<T>> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      final FieldDescriptor descriptor = extension.getDescriptor();
      extensions.addRepeatedField(descriptor, extension.singularToReflectionType(value));
      onChanged();
      return (BuilderT) this;
    }

    /** Clear an extension. */
    public final <T> BuilderT clearExtension(final ExtensionLite<MessageT, T> extensionLite) {
      Extension<MessageT, T> extension = checkNotLite(extensionLite);

      verifyExtensionContainingType(extension);
      ensureExtensionsIsMutable();
      extensions.clearField(extension.getDescriptor());
      onChanged();
      return (BuilderT) this;
    }

    /**
     * Check if a singular extension is present.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> boolean hasExtension(final Extension<MessageT, T> extension) {
      return hasExtension((ExtensionLite<MessageT, T>) extension);
    }
    /**
     * Check if a singular extension is present.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> boolean hasExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return hasExtension((ExtensionLite<MessageT, T>) extension);
    }
    /**
     * Get the number of elements in a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> int getExtensionCount(
        final Extension<MessageT, List<T>> extension) {
      return getExtensionCount((ExtensionLite<MessageT, List<T>>) extension);
    }
    /**
     * Get the number of elements in a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> int getExtensionCount(
        final GeneratedExtension<MessageT, List<T>> extension) {
      return getExtensionCount((ExtensionLite<MessageT, List<T>>) extension);
    }
    /**
     * Get the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> T getExtension(final Extension<MessageT, T> extension) {
      return getExtension((ExtensionLite<MessageT, T>) extension);
    }
    /** Get the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> T getExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return getExtension((ExtensionLite<MessageT, T>) extension);
    }
    /**
     * Get the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> T getExtension(
        final Extension<MessageT, List<T>> extension, final int index) {
      return getExtension((ExtensionLite<MessageT, List<T>>) extension, index);
    }
    /**
     * Get the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    @Override
    public final <T> T getExtension(
        final GeneratedExtension<MessageT, List<T>> extension, final int index) {
      return getExtension((ExtensionLite<MessageT, List<T>>) extension, index);
    }
    /**
     * Set the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    public final <T> BuilderT setExtension(
        final Extension<MessageT, T> extension, final T value) {
      return setExtension((ExtensionLite<MessageT, T>) extension, value);
    }
    /**
     * Set the value of an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    public <T> BuilderT setExtension(
        final GeneratedExtension<MessageT, T> extension, final T value) {
      return setExtension((ExtensionLite<MessageT, T>) extension, value);
    }
    /**
     * Set the value of one element of a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    public final <T> BuilderT setExtension(
        final Extension<MessageT, List<T>> extension,
        final int index, final T value) {
      return setExtension((ExtensionLite<MessageT, List<T>>) extension, index, value);
    }
    /**
     * Set the value of one element of a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    public <T> BuilderT setExtension(
        final GeneratedExtension<MessageT, List<T>> extension,
        final int index, final T value) {
      return setExtension((ExtensionLite<MessageT, List<T>>) extension, index, value);
    }
    /**
     * Append a value to a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    public final <T> BuilderT addExtension(
        final Extension<MessageT, List<T>> extension, final T value) {
      return addExtension((ExtensionLite<MessageT, List<T>>) extension, value);
    }
    /**
     * Append a value to a repeated extension.
     * <p>TODO: handled by ExtensionLite version
     */
    public <T> BuilderT addExtension(
        final GeneratedExtension<MessageT, List<T>> extension, final T value) {
      return addExtension((ExtensionLite<MessageT, List<T>>) extension, value);
    }
    /**
     * Clear an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    public final <T> BuilderT clearExtension(
        final Extension<MessageT, T> extension) {
      return clearExtension((ExtensionLite<MessageT, T>) extension);
    }
    /**
     * Clears an extension.
     * <p>TODO: handled by ExtensionLite version
     */
    public <T> BuilderT clearExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return clearExtension((ExtensionLite<MessageT, T>) extension);
    }

    /** Called by subclasses to check if all extensions are initialized. */
    protected boolean extensionsAreInitialized() {
      return extensions == null || extensions.isInitialized();
    }

    /**
     * Called by the build code path to create a copy of the extensions for building the message.
     */
    private FieldSet<FieldDescriptor> buildExtensions() {
      return extensions == null
          ? (FieldSet<FieldDescriptor>) FieldSet.emptySet()
          : extensions.buildPartial();
    }

    @Override
    public boolean isInitialized() {
      return super.isInitialized() && extensionsAreInitialized();
    }

    // ---------------------------------------------------------------
    // Reflection

    @Override
    public Map<FieldDescriptor, Object> getAllFields() {
      final Map<FieldDescriptor, Object> result = super.getAllFieldsMutable();
      if (extensions != null) {
        result.putAll(extensions.getAllFields());
      }
      return Collections.unmodifiableMap(result);
    }

    @Override
    public Object getField(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        final Object value = extensions == null ? null : extensions.getField(field);
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
    public Message.Builder getFieldBuilder(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
          throw new UnsupportedOperationException(
              "getFieldBuilder() called on a non-Message type.");
        }
        ensureExtensionsIsMutable();
        final Object value = extensions.getFieldAllowBuilders(field);
        if (value == null) {
          Message.Builder builder = DynamicMessage.newBuilder(field.getMessageType());
          extensions.setField(field, builder);
          onChanged();
          return builder;
        } else {
          if (value instanceof Message.Builder) {
            return (Message.Builder) value;
          } else if (value instanceof Message) {
            Message.Builder builder = ((Message) value).toBuilder();
            extensions.setField(field, builder);
            onChanged();
            return builder;
          } else {
            throw new UnsupportedOperationException(
                "getRepeatedFieldBuilder() called on a non-Message type.");
          }
        }
      } else {
        return super.getFieldBuilder(field);
      }
    }

    @Override
    public int getRepeatedFieldCount(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions == null ? 0 : extensions.getRepeatedFieldCount(field);
      } else {
        return super.getRepeatedFieldCount(field);
      }
    }

    @Override
    public Object getRepeatedField(final FieldDescriptor field, final int index) {
      if (field.isExtension()) {
        verifyContainingType(field);
        if (extensions == null) {
          throw new IndexOutOfBoundsException();
        }
        return extensions.getRepeatedField(field, index);
      } else {
        return super.getRepeatedField(field, index);
      }
    }

    @Override
    public Message.Builder getRepeatedFieldBuilder(final FieldDescriptor field, final int index) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
          throw new UnsupportedOperationException(
              "getRepeatedFieldBuilder() called on a non-Message type.");
        }
        final Object value = extensions.getRepeatedFieldAllowBuilders(field, index);
        if (value instanceof Message.Builder) {
          return (Message.Builder) value;
        } else if (value instanceof Message) {
          Message.Builder builder = ((Message) value).toBuilder();
          extensions.setRepeatedField(field, index, builder);
          onChanged();
          return builder;
        } else {
          throw new UnsupportedOperationException(
              "getRepeatedFieldBuilder() called on a non-Message type.");
        }
      } else {
        return super.getRepeatedFieldBuilder(field, index);
      }
    }

    @Override
    public boolean hasField(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        return extensions != null && extensions.hasField(field);
      } else {
        return super.hasField(field);
      }
    }

    @Override
    public BuilderT setField(final FieldDescriptor field, final Object value) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        extensions.setField(field, value);
        onChanged();
        return (BuilderT) this;
      } else {
        return super.setField(field, value);
      }
    }

    @Override
    public BuilderT clearField(final FieldDescriptor field) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        extensions.clearField(field);
        onChanged();
        return (BuilderT) this;
      } else {
        return super.clearField(field);
      }
    }

    @Override
    public BuilderT setRepeatedField(
        final FieldDescriptor field, final int index, final Object value) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        extensions.setRepeatedField(field, index, value);
        onChanged();
        return (BuilderT) this;
      } else {
        return super.setRepeatedField(field, index, value);
      }
    }

    @Override
    public BuilderT addRepeatedField(final FieldDescriptor field, final Object value) {
      if (field.isExtension()) {
        verifyContainingType(field);
        ensureExtensionsIsMutable();
        extensions.addRepeatedField(field, value);
        onChanged();
        return (BuilderT) this;
      } else {
        return super.addRepeatedField(field, value);
      }
    }

    @Override
    public Message.Builder newBuilderForField(final FieldDescriptor field) {
      if (field.isExtension()) {
        return DynamicMessage.newBuilder(field.getMessageType());
      } else {
        return super.newBuilderForField(field);
      }
    }

    protected final void mergeExtensionFields(final ExtendableMessage<?> other) {
      if (other.extensions != null) {
        ensureExtensionsIsMutable();
        extensions.mergeFrom(other.extensions);
        onChanged();
      }
    }

    @Override
    protected boolean parseUnknownField(
        CodedInputStream input, ExtensionRegistryLite extensionRegistry, int tag)
        throws IOException {
      ensureExtensionsIsMutable();
      return MessageReflection.mergeFieldFrom(
          input,
          input.shouldDiscardUnknownFields() ? null : getUnknownFieldSetBuilder(),
          extensionRegistry,
          getDescriptorForType(),
          new MessageReflection.ExtensionBuilderAdapter(extensions),
          tag);
    }

    private void verifyContainingType(final FieldDescriptor field) {
      if (field.getContainingType() != getDescriptorForType()) {
        throw new IllegalArgumentException("FieldDescriptor does not match message type.");
      }
    }
  }

  // -----------------------------------------------------------------

  /**
   * Gets the descriptor for an extension. The implementation depends on whether the extension is
   * scoped in the top level of a file or scoped in a Message.
   */
  interface ExtensionDescriptorRetriever {
    FieldDescriptor getDescriptor();
  }

  // =================================================================

  /** Calls Class.getMethod and throws a RuntimeException if it fails. */
  private static Method getMethodOrDie(
      final Class<?> clazz, final String name, final Class<?>... params) {
    try {
      return clazz.getMethod(name, params);
    } catch (NoSuchMethodException e) {
      throw new IllegalStateException(
          "Generated message class \"" + clazz.getName() + "\" missing method \"" + name + "\".",
          e);
    }
  }

  /** Calls invoke and throws a RuntimeException if it fails. */
  @CanIgnoreReturnValue
  private static Object invokeOrDie(
      final Method method, final Object object, final Object... params) {
    try {
      return method.invoke(object, params);
    } catch (IllegalAccessException e) {
      throw new IllegalStateException(
          "Couldn't use Java reflection to implement protocol message reflection.", e);
    } catch (InvocationTargetException e) {
      final Throwable cause = e.getCause();
      if (cause instanceof RuntimeException) {
        throw (RuntimeException) cause;
      } else if (cause instanceof Error) {
        throw (Error) cause;
      } else {
        throw new IllegalStateException(
            "Unexpected exception thrown by generated accessor method.", cause);
      }
    }
  }

  /**
   * Gets the map field with the given field number. This method should be overridden in the
   * generated message class if the message contains map fields.
   *
   * <p>Unlike other field types, reflection support for map fields can't be implemented based on
   * generated public API because we need to access a map field as a list in reflection API but the
   * generated API only allows us to access it as a map. This method returns the underlying map
   * field directly and thus enables us to access the map field as a list.
   */
  @SuppressWarnings("unused")
  protected MapFieldReflectionAccessor internalGetMapFieldReflection(int fieldNumber) {
    return internalGetMapField(fieldNumber);
  }

  /** TODO: Remove, exists for compatibility with generated code. */
  @Deprecated
  @SuppressWarnings({"rawtypes", "unused"})
  protected MapField internalGetMapField(int fieldNumber) {
    // Note that we can't use descriptor names here because this method will
    // be called when descriptor is being initialized.
    throw new IllegalArgumentException("No map fields found in " + getClass().getName());
  }

  /**
   * Users should ignore this class. This class provides the implementation with access to the
   * fields of a message object using Java reflection.
   */
  public static final class FieldAccessorTable {

    /**
     * Construct a FieldAccessorTable for a particular message class. Only one FieldAccessorTable
     * should ever be constructed per class.
     *
     * @param descriptor The type's descriptor.
     * @param camelCaseNames The camelcase names of all fields in the message. These are used to
     *     derive the accessor method names.
     * @param messageClass The message type.
     * @param builderClass The builder type.
     */
    public FieldAccessorTable(
        final Descriptor descriptor,
        final String[] camelCaseNames,
        final Class<? extends GeneratedMessageV3> messageClass,
        final Class<? extends Builder<?>> builderClass) {
      this(descriptor, camelCaseNames);
      ensureFieldAccessorsInitialized(messageClass, builderClass);
    }

    /**
     * Construct a FieldAccessorTable for a particular message class without initializing
     * FieldAccessors.
     */
    public FieldAccessorTable(final Descriptor descriptor, final String[] camelCaseNames) {
      this.descriptor = descriptor;
      this.camelCaseNames = camelCaseNames;
      fields = new FieldAccessor[descriptor.getFields().size()];
      oneofs = new OneofAccessor[descriptor.getOneofs().size()];
      initialized = false;
    }

    /**
     * Ensures the field accessors are initialized. This method is thread-safe.
     *
     * @param messageClass The message type.
     * @param builderClass The builder type.
     * @return this
     */
    public FieldAccessorTable ensureFieldAccessorsInitialized(
        Class<? extends GeneratedMessageV3> messageClass, Class<? extends Builder<?>> builderClass) {
      if (initialized) {
        return this;
      }
      synchronized (this) {
        if (initialized) {
          return this;
        }
        int fieldsSize = fields.length;
        for (int i = 0; i < fieldsSize; i++) {
          FieldDescriptor field = descriptor.getFields().get(i);
          String containingOneofCamelCaseName = null;
          if (field.getContainingOneof() != null) {
            int index = fieldsSize + field.getContainingOneof().getIndex();
            if (index < camelCaseNames.length) {
              containingOneofCamelCaseName = camelCaseNames[index];
            }
          }
          if (field.isRepeated()) {
            if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
              if (field.isMapField()) {
                fields[i] = new MapFieldAccessor(field, messageClass);
              } else {
                fields[i] =
                    new RepeatedMessageFieldAccessor(
                        field, camelCaseNames[i], messageClass, builderClass);
              }
            } else if (field.getJavaType() == FieldDescriptor.JavaType.ENUM) {
              fields[i] =
                  new RepeatedEnumFieldAccessor(
                      field, camelCaseNames[i], messageClass, builderClass);
            } else {
              fields[i] =
                  new RepeatedFieldAccessor(field, camelCaseNames[i], messageClass, builderClass);
            }
          } else {
            if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
              fields[i] =
                  new SingularMessageFieldAccessor(
                      field,
                      camelCaseNames[i],
                      messageClass,
                      builderClass,
                      containingOneofCamelCaseName);
            } else if (field.getJavaType() == FieldDescriptor.JavaType.ENUM) {
              fields[i] =
                  new SingularEnumFieldAccessor(
                      field,
                      camelCaseNames[i],
                      messageClass,
                      builderClass,
                      containingOneofCamelCaseName);
            } else if (field.getJavaType() == FieldDescriptor.JavaType.STRING) {
              fields[i] =
                  new SingularStringFieldAccessor(
                      field,
                      camelCaseNames[i],
                      messageClass,
                      builderClass,
                      containingOneofCamelCaseName);
            } else {
              fields[i] =
                  new SingularFieldAccessor(
                      field,
                      camelCaseNames[i],
                      messageClass,
                      builderClass,
                      containingOneofCamelCaseName);
            }
          }
        }

        for (int i = 0; i < descriptor.getOneofs().size(); i++) {
          if (i < descriptor.getRealOneofs().size()) {
            oneofs[i] =
                new RealOneofAccessor(
                    descriptor, i, camelCaseNames[i + fieldsSize], messageClass, builderClass);
          } else {
            oneofs[i] = new SyntheticOneofAccessor(descriptor, i);
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
    private final OneofAccessor[] oneofs;
    private volatile boolean initialized;

    /** Get the FieldAccessor for a particular field. */
    private FieldAccessor getField(final FieldDescriptor field) {
      if (field.getContainingType() != descriptor) {
        throw new IllegalArgumentException("FieldDescriptor does not match message type.");
      } else if (field.isExtension()) {
        // If this type had extensions, it would subclass ExtendableMessage,
        // which overrides the reflection interface to handle extensions.
        throw new IllegalArgumentException("This type does not have extensions.");
      }
      return fields[field.getIndex()];
    }

    /** Get the OneofAccessor for a particular oneof. */
    private OneofAccessor getOneof(final OneofDescriptor oneof) {
      if (oneof.getContainingType() != descriptor) {
        throw new IllegalArgumentException("OneofDescriptor does not match message type.");
      }
      return oneofs[oneof.getIndex()];
    }

    /**
     * Abstract interface that provides access to a single field. This is implemented differently
     * depending on the field type and cardinality.
     */
    private interface FieldAccessor {
      Object get(GeneratedMessageV3 message);

      Object get(GeneratedMessageV3.Builder<?> builder);

      Object getRaw(GeneratedMessageV3 message);

      void set(Builder<?> builder, Object value);

      Object getRepeated(GeneratedMessageV3 message, int index);

      Object getRepeated(GeneratedMessageV3.Builder<?> builder, int index);

      void setRepeated(Builder<?> builder, int index, Object value);

      void addRepeated(Builder<?> builder, Object value);

      boolean has(GeneratedMessageV3 message);

      boolean has(GeneratedMessageV3.Builder<?> builder);

      int getRepeatedCount(GeneratedMessageV3 message);

      int getRepeatedCount(GeneratedMessageV3.Builder<?> builder);

      void clear(Builder<?> builder);

      Message.Builder newBuilder();

      Message.Builder getBuilder(GeneratedMessageV3.Builder<?> builder);

      Message.Builder getRepeatedBuilder(GeneratedMessageV3.Builder<?> builder, int index);
    }

    /** OneofAccessor provides access to a single oneof. */
    private static interface OneofAccessor {
      public boolean has(final GeneratedMessageV3 message);

      public boolean has(GeneratedMessageV3.Builder<?> builder);

      public FieldDescriptor get(final GeneratedMessageV3 message);

      public FieldDescriptor get(GeneratedMessageV3.Builder<?> builder);

      public void clear(final Builder<?> builder);
    }

    /** RealOneofAccessor provides access to a single real oneof. */
    private static class RealOneofAccessor implements OneofAccessor {
      RealOneofAccessor(
          final Descriptor descriptor,
          final int oneofIndex,
          final String camelCaseName,
          final Class<? extends GeneratedMessageV3> messageClass,
          final Class<? extends Builder<?>> builderClass) {
        this.descriptor = descriptor;
        caseMethod = getMethodOrDie(messageClass, "get" + camelCaseName + "Case");
        caseMethodBuilder = getMethodOrDie(builderClass, "get" + camelCaseName + "Case");
        clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
      }

      private final Descriptor descriptor;
      private final Method caseMethod;
      private final Method caseMethodBuilder;
      private final Method clearMethod;

      @Override
      public boolean has(final GeneratedMessageV3 message) {
        return ((Internal.EnumLite) invokeOrDie(caseMethod, message)).getNumber() != 0;
      }

      @Override
      public boolean has(GeneratedMessageV3.Builder<?> builder) {
        return ((Internal.EnumLite) invokeOrDie(caseMethodBuilder, builder)).getNumber() != 0;
      }

      @Override
      public FieldDescriptor get(final GeneratedMessageV3 message) {
        int fieldNumber = ((Internal.EnumLite) invokeOrDie(caseMethod, message)).getNumber();
        if (fieldNumber > 0) {
          return descriptor.findFieldByNumber(fieldNumber);
        }
        return null;
      }

      @Override
      public FieldDescriptor get(GeneratedMessageV3.Builder<?> builder) {
        int fieldNumber = ((Internal.EnumLite) invokeOrDie(caseMethodBuilder, builder)).getNumber();
        if (fieldNumber > 0) {
          return descriptor.findFieldByNumber(fieldNumber);
        }
        return null;
      }

      @Override
      public void clear(final Builder<?> builder) {
        // TODO: remove the unused variable
        Object unused = invokeOrDie(clearMethod, builder);
      }
    }

    /** SyntheticOneofAccessor provides access to a single synthetic oneof. */
    private static class SyntheticOneofAccessor implements OneofAccessor {
      SyntheticOneofAccessor(final Descriptor descriptor, final int oneofIndex) {
        OneofDescriptor oneofDescriptor = descriptor.getOneofs().get(oneofIndex);
        fieldDescriptor = oneofDescriptor.getFields().get(0);
      }

      private final FieldDescriptor fieldDescriptor;

      @Override
      public boolean has(final GeneratedMessageV3 message) {
        return message.hasField(fieldDescriptor);
      }

      @Override
      public boolean has(GeneratedMessageV3.Builder<?> builder) {
        return builder.hasField(fieldDescriptor);
      }

      @Override
      public FieldDescriptor get(final GeneratedMessageV3 message) {
        return message.hasField(fieldDescriptor) ? fieldDescriptor : null;
      }

      public FieldDescriptor get(GeneratedMessageV3.Builder<?> builder) {
        return builder.hasField(fieldDescriptor) ? fieldDescriptor : null;
      }

      @Override
      public void clear(final Builder<?> builder) {
        builder.clearField(fieldDescriptor);
      }
    }

    // ---------------------------------------------------------------

    @SuppressWarnings("SameNameButDifferent")
    private static class SingularFieldAccessor implements FieldAccessor {
      private interface MethodInvoker {
        Object get(final GeneratedMessageV3 message);

        Object get(GeneratedMessageV3.Builder<?> builder);

        int getOneofFieldNumber(final GeneratedMessageV3 message);

        int getOneofFieldNumber(final GeneratedMessageV3.Builder<?> builder);

        void set(final GeneratedMessageV3.Builder<?> builder, final Object value);

        boolean has(final GeneratedMessageV3 message);

        boolean has(GeneratedMessageV3.Builder<?> builder);

        void clear(final GeneratedMessageV3.Builder<?> builder);
      }

      private static final class ReflectionInvoker implements MethodInvoker {
        private final Method getMethod;
        private final Method getMethodBuilder;
        private final Method setMethod;
        private final Method hasMethod;
        private final Method hasMethodBuilder;
        private final Method clearMethod;
        private final Method caseMethod;
        private final Method caseMethodBuilder;

        ReflectionInvoker(
            final FieldDescriptor descriptor,
            final String camelCaseName,
            final Class<? extends GeneratedMessageV3> messageClass,
            final Class<? extends Builder<?>> builderClass,
            final String containingOneofCamelCaseName,
            boolean isOneofField,
            boolean hasHasMethod) {
          getMethod = getMethodOrDie(messageClass, "get" + camelCaseName);
          getMethodBuilder = getMethodOrDie(builderClass, "get" + camelCaseName);
          Class<?> type = getMethod.getReturnType();
          setMethod = getMethodOrDie(builderClass, "set" + camelCaseName, type);
          hasMethod = hasHasMethod ? getMethodOrDie(messageClass, "has" + camelCaseName) : null;
          hasMethodBuilder =
              hasHasMethod ? getMethodOrDie(builderClass, "has" + camelCaseName) : null;
          clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
          caseMethod =
              isOneofField
                  ? getMethodOrDie(messageClass, "get" + containingOneofCamelCaseName + "Case")
                  : null;
          caseMethodBuilder =
              isOneofField
                  ? getMethodOrDie(builderClass, "get" + containingOneofCamelCaseName + "Case")
                  : null;
        }

        @Override
        public Object get(final GeneratedMessageV3 message) {
          return invokeOrDie(getMethod, message);
        }

        @Override
        public Object get(GeneratedMessageV3.Builder<?> builder) {
          return invokeOrDie(getMethodBuilder, builder);
        }

        @Override
        public int getOneofFieldNumber(final GeneratedMessageV3 message) {
          return ((Internal.EnumLite) invokeOrDie(caseMethod, message)).getNumber();
        }

        @Override
        public int getOneofFieldNumber(final GeneratedMessageV3.Builder<?> builder) {
          return ((Internal.EnumLite) invokeOrDie(caseMethodBuilder, builder)).getNumber();
        }

        @Override
        public void set(final GeneratedMessageV3.Builder<?> builder, final Object value) {
          // TODO: remove the unused variable
          Object unused = invokeOrDie(setMethod, builder, value);
        }

        @Override
        public boolean has(final GeneratedMessageV3 message) {
          return (Boolean) invokeOrDie(hasMethod, message);
        }

        @Override
        public boolean has(GeneratedMessageV3.Builder<?> builder) {
          return (Boolean) invokeOrDie(hasMethodBuilder, builder);
        }

        @Override
        public void clear(final GeneratedMessageV3.Builder<?> builder) {
          // TODO: remove the unused variable
          Object unused = invokeOrDie(clearMethod, builder);
        }
      }

      SingularFieldAccessor(
          final FieldDescriptor descriptor,
          final String camelCaseName,
          final Class<? extends GeneratedMessageV3> messageClass,
          final Class<? extends Builder<?>> builderClass,
          final String containingOneofCamelCaseName) {
        isOneofField =
            descriptor.getRealContainingOneof() != null;
        hasHasMethod =
            descriptor.getFile().getSyntax() == FileDescriptor.Syntax.EDITIONS && descriptor.hasPresence()
                || descriptor.getFile().getSyntax() == FileDescriptor.Syntax.PROTO2
                || descriptor.hasOptionalKeyword()
                || (!isOneofField && descriptor.getJavaType() == FieldDescriptor.JavaType.MESSAGE);
        ReflectionInvoker reflectionInvoker =
            new ReflectionInvoker(
                descriptor,
                camelCaseName,
                messageClass,
                builderClass,
                containingOneofCamelCaseName,
                isOneofField,
                hasHasMethod);
        field = descriptor;
        type = reflectionInvoker.getMethod.getReturnType();
        invoker = getMethodInvoker(reflectionInvoker);
      }

      static MethodInvoker getMethodInvoker(ReflectionInvoker accessor) {
        return accessor;
      }

      // Note:  We use Java reflection to call public methods rather than
      //   access private fields directly as this avoids runtime security
      //   checks.
      protected final Class<?> type;
      protected final FieldDescriptor field;
      protected final boolean isOneofField;
      protected final boolean hasHasMethod;
      protected final MethodInvoker invoker;

      @Override
      public Object get(final GeneratedMessageV3 message) {
        return invoker.get(message);
      }

      @Override
      public Object get(GeneratedMessageV3.Builder<?> builder) {
        return invoker.get(builder);
      }

      @Override
      public Object getRaw(final GeneratedMessageV3 message) {
        return get(message);
      }

      @Override
      public void set(final Builder<?> builder, final Object value) {
        invoker.set(builder, value);
      }

      @Override
      public Object getRepeated(final GeneratedMessageV3 message, final int index) {
        throw new UnsupportedOperationException("getRepeatedField() called on a singular field.");
      }

      @Override
      public Object getRepeated(GeneratedMessageV3.Builder<?> builder, int index) {
        throw new UnsupportedOperationException("getRepeatedField() called on a singular field.");
      }

      @Override
      public void setRepeated(final Builder<?> builder, final int index, final Object value) {
        throw new UnsupportedOperationException("setRepeatedField() called on a singular field.");
      }

      @Override
      public void addRepeated(final Builder<?> builder, final Object value) {
        throw new UnsupportedOperationException("addRepeatedField() called on a singular field.");
      }

      @Override
      public boolean has(final GeneratedMessageV3 message) {
        if (!hasHasMethod) {
          if (isOneofField) {
            return invoker.getOneofFieldNumber(message) == field.getNumber();
          }
          return !get(message).equals(field.getDefaultValue());
        }
        return invoker.has(message);
      }

      @Override
      public boolean has(GeneratedMessageV3.Builder<?> builder) {
        if (!hasHasMethod) {
          if (isOneofField) {
            return invoker.getOneofFieldNumber(builder) == field.getNumber();
          }
          return !get(builder).equals(field.getDefaultValue());
        }
        return invoker.has(builder);
      }

      @Override
      public int getRepeatedCount(final GeneratedMessageV3 message) {
        throw new UnsupportedOperationException(
            "getRepeatedFieldSize() called on a singular field.");
      }

      @Override
      public int getRepeatedCount(GeneratedMessageV3.Builder<?> builder) {
        throw new UnsupportedOperationException(
            "getRepeatedFieldSize() called on a singular field.");
      }

      @Override
      public void clear(final Builder<?> builder) {
        invoker.clear(builder);
      }

      @Override
      public Message.Builder newBuilder() {
        throw new UnsupportedOperationException(
            "newBuilderForField() called on a non-Message type.");
      }

      @Override
      public Message.Builder getBuilder(GeneratedMessageV3.Builder<?> builder) {
        throw new UnsupportedOperationException("getFieldBuilder() called on a non-Message type.");
      }

      @Override
      public Message.Builder getRepeatedBuilder(GeneratedMessageV3.Builder<?> builder, int index) {
        throw new UnsupportedOperationException(
            "getRepeatedFieldBuilder() called on a non-Message type.");
      }
    }

    @SuppressWarnings("SameNameButDifferent")
    private static class RepeatedFieldAccessor implements FieldAccessor {
      interface MethodInvoker {
        Object get(final GeneratedMessageV3 message);

        Object get(GeneratedMessageV3.Builder<?> builder);

        Object getRepeated(final GeneratedMessageV3 message, final int index);

        Object getRepeated(GeneratedMessageV3.Builder<?> builder, int index);

        void setRepeated(
            final GeneratedMessageV3.Builder<?> builder, final int index, final Object value);

        void addRepeated(final GeneratedMessageV3.Builder<?> builder, final Object value);

        int getRepeatedCount(final GeneratedMessageV3 message);

        int getRepeatedCount(GeneratedMessageV3.Builder<?> builder);

        void clear(final GeneratedMessageV3.Builder<?> builder);
      }

      private static final class ReflectionInvoker implements MethodInvoker {
        private final Method getMethod;
        private final Method getMethodBuilder;
        private final Method getRepeatedMethod;
        private final Method getRepeatedMethodBuilder;
        private final Method setRepeatedMethod;
        private final Method addRepeatedMethod;
        private final Method getCountMethod;
        private final Method getCountMethodBuilder;
        private final Method clearMethod;

        ReflectionInvoker(
            final FieldDescriptor descriptor,
            final String camelCaseName,
            final Class<? extends GeneratedMessageV3> messageClass,
            final Class<? extends Builder<?>> builderClass) {
          getMethod = getMethodOrDie(messageClass, "get" + camelCaseName + "List");
          getMethodBuilder = getMethodOrDie(builderClass, "get" + camelCaseName + "List");
          getRepeatedMethod = getMethodOrDie(messageClass, "get" + camelCaseName, Integer.TYPE);
          getRepeatedMethodBuilder =
              getMethodOrDie(builderClass, "get" + camelCaseName, Integer.TYPE);
          Class<?> type = getRepeatedMethod.getReturnType();
          setRepeatedMethod =
              getMethodOrDie(builderClass, "set" + camelCaseName, Integer.TYPE, type);
          addRepeatedMethod = getMethodOrDie(builderClass, "add" + camelCaseName, type);
          getCountMethod = getMethodOrDie(messageClass, "get" + camelCaseName + "Count");
          getCountMethodBuilder = getMethodOrDie(builderClass, "get" + camelCaseName + "Count");
          clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName);
        }

        @Override
        public Object get(final GeneratedMessageV3 message) {
          return invokeOrDie(getMethod, message);
        }

        @Override
        public Object get(GeneratedMessageV3.Builder<?> builder) {
          return invokeOrDie(getMethodBuilder, builder);
        }

        @Override
        public Object getRepeated(final GeneratedMessageV3 message, final int index) {
          return invokeOrDie(getRepeatedMethod, message, index);
        }

        @Override
        public Object getRepeated(GeneratedMessageV3.Builder<?> builder, int index) {
          return invokeOrDie(getRepeatedMethodBuilder, builder, index);
        }

        @Override
        public void setRepeated(
            final GeneratedMessageV3.Builder<?> builder, final int index, final Object value) {
          // TODO: remove the unused variable
          Object unused = invokeOrDie(setRepeatedMethod, builder, index, value);
        }

        @Override
        public void addRepeated(final GeneratedMessageV3.Builder<?> builder, final Object value) {
          // TODO: remove the unused variable
          Object unused = invokeOrDie(addRepeatedMethod, builder, value);
        }

        @Override
        public int getRepeatedCount(final GeneratedMessageV3 message) {
          return (Integer) invokeOrDie(getCountMethod, message);
        }

        @Override
        public int getRepeatedCount(GeneratedMessageV3.Builder<?> builder) {
          return (Integer) invokeOrDie(getCountMethodBuilder, builder);
        }

        @Override
        public void clear(final GeneratedMessageV3.Builder<?> builder) {
          // TODO: remove the unused variable
          Object unused = invokeOrDie(clearMethod, builder);
        }
      }

      protected final Class<?> type;
      protected final MethodInvoker invoker;

      RepeatedFieldAccessor(
          final FieldDescriptor descriptor,
          final String camelCaseName,
          final Class<? extends GeneratedMessageV3> messageClass,
          final Class<? extends Builder<?>> builderClass) {
        ReflectionInvoker reflectionInvoker =
            new ReflectionInvoker(descriptor, camelCaseName, messageClass, builderClass);
        type = reflectionInvoker.getRepeatedMethod.getReturnType();
        invoker = getMethodInvoker(reflectionInvoker);
      }

      static MethodInvoker getMethodInvoker(ReflectionInvoker accessor) {
        return accessor;
      }

      @Override
      public Object get(final GeneratedMessageV3 message) {
        return invoker.get(message);
      }

      @Override
      public Object get(GeneratedMessageV3.Builder<?> builder) {
        return invoker.get(builder);
      }

      @Override
      public Object getRaw(final GeneratedMessageV3 message) {
        return get(message);
      }

      @Override
      public void set(final Builder<?> builder, final Object value) {
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
      public Object getRepeated(final GeneratedMessageV3 message, final int index) {
        return invoker.getRepeated(message, index);
      }

      @Override
      public Object getRepeated(GeneratedMessageV3.Builder<?> builder, int index) {
        return invoker.getRepeated(builder, index);
      }

      @Override
      public void setRepeated(final Builder<?> builder, final int index, final Object value) {
        invoker.setRepeated(builder, index, value);
      }

      @Override
      public void addRepeated(final Builder<?> builder, final Object value) {
        invoker.addRepeated(builder, value);
      }

      @Override
      public boolean has(final GeneratedMessageV3 message) {
        throw new UnsupportedOperationException("hasField() called on a repeated field.");
      }

      @Override
      public boolean has(GeneratedMessageV3.Builder<?> builder) {
        throw new UnsupportedOperationException("hasField() called on a repeated field.");
      }

      @Override
      public int getRepeatedCount(final GeneratedMessageV3 message) {
        return invoker.getRepeatedCount(message);
      }

      @Override
      public int getRepeatedCount(GeneratedMessageV3.Builder<?> builder) {
        return invoker.getRepeatedCount(builder);
      }

      @Override
      public void clear(final Builder<?> builder) {
        invoker.clear(builder);
      }

      @Override
      public Message.Builder newBuilder() {
        throw new UnsupportedOperationException(
            "newBuilderForField() called on a non-Message type.");
      }

      @Override
      public Message.Builder getBuilder(GeneratedMessageV3.Builder<?> builder) {
        throw new UnsupportedOperationException("getFieldBuilder() called on a non-Message type.");
      }

      @Override
      public Message.Builder getRepeatedBuilder(GeneratedMessageV3.Builder<?> builder, int index) {
        throw new UnsupportedOperationException(
            "getRepeatedFieldBuilder() called on a non-Message type.");
      }
    }

    private static class MapFieldAccessor implements FieldAccessor {
      MapFieldAccessor(
          final FieldDescriptor descriptor, final Class<? extends GeneratedMessageV3> messageClass) {
        field = descriptor;
        Method getDefaultInstanceMethod = getMethodOrDie(messageClass, "getDefaultInstance");
        MapFieldReflectionAccessor defaultMapField =
            getMapField((GeneratedMessageV3) invokeOrDie(getDefaultInstanceMethod, null));
        mapEntryMessageDefaultInstance = defaultMapField.getMapEntryMessageDefaultInstance();
      }

      private final FieldDescriptor field;
      private final Message mapEntryMessageDefaultInstance;

      private MapFieldReflectionAccessor getMapField(GeneratedMessageV3 message) {
        return message.internalGetMapFieldReflection(field.getNumber());
      }

      private MapFieldReflectionAccessor getMapField(GeneratedMessageV3.Builder<?> builder) {
        return builder.internalGetMapFieldReflection(field.getNumber());
      }

      private MapFieldReflectionAccessor getMutableMapField(GeneratedMessageV3.Builder<?> builder) {
        return builder.internalGetMutableMapFieldReflection(field.getNumber());
      }

      private Message coerceType(Message value) {
        if (value == null) {
          return null;
        }
        if (mapEntryMessageDefaultInstance.getClass().isInstance(value)) {
          return value;
        }
        // The value is not the exact right message type.  However, if it
        // is an alternative implementation of the same type -- e.g. a
        // DynamicMessage -- we should accept it.  In this case we can make
        // a copy of the message.
        return mapEntryMessageDefaultInstance.toBuilder().mergeFrom(value).build();
      }

      @Override
      public Object get(GeneratedMessageV3 message) {
        List<Object> result = new ArrayList<>();
        for (int i = 0; i < getRepeatedCount(message); i++) {
          result.add(getRepeated(message, i));
        }
        return Collections.unmodifiableList(result);
      }

      @Override
      public Object get(Builder<?> builder) {
        List<Object> result = new ArrayList<>();
        for (int i = 0; i < getRepeatedCount(builder); i++) {
          result.add(getRepeated(builder, i));
        }
        return Collections.unmodifiableList(result);
      }

      @Override
      public Object getRaw(GeneratedMessageV3 message) {
        return get(message);
      }

      @Override
      public void set(Builder<?> builder, Object value) {
        clear(builder);
        for (Object entry : (List<?>) value) {
          addRepeated(builder, entry);
        }
      }

      @Override
      public Object getRepeated(GeneratedMessageV3 message, int index) {
        return getMapField(message).getList().get(index);
      }

      @Override
      public Object getRepeated(Builder<?> builder, int index) {
        return getMapField(builder).getList().get(index);
      }

      @Override
      public void setRepeated(Builder<?> builder, int index, Object value) {
        getMutableMapField(builder).getMutableList().set(index, coerceType((Message) value));
      }

      @Override
      public void addRepeated(Builder<?> builder, Object value) {
        getMutableMapField(builder).getMutableList().add(coerceType((Message) value));
      }

      @Override
      public boolean has(GeneratedMessageV3 message) {
        throw new UnsupportedOperationException("hasField() is not supported for repeated fields.");
      }

      @Override
      public boolean has(Builder<?> builder) {
        throw new UnsupportedOperationException("hasField() is not supported for repeated fields.");
      }

      @Override
      public int getRepeatedCount(GeneratedMessageV3 message) {
        return getMapField(message).getList().size();
      }

      @Override
      public int getRepeatedCount(Builder<?> builder) {
        return getMapField(builder).getList().size();
      }

      @Override
      public void clear(Builder<?> builder) {
        getMutableMapField(builder).getMutableList().clear();
      }

      @Override
      public Message.Builder newBuilder() {
        return mapEntryMessageDefaultInstance.newBuilderForType();
      }

      @Override
      public Message.Builder getBuilder(Builder<?> builder) {
        throw new UnsupportedOperationException("Nested builder not supported for map fields.");
      }

      @Override
      public Message.Builder getRepeatedBuilder(Builder<?> builder, int index) {
        throw new UnsupportedOperationException("Map fields cannot be repeated");
      }
    }

    // ---------------------------------------------------------------

    private static final class SingularEnumFieldAccessor extends SingularFieldAccessor {
      SingularEnumFieldAccessor(
          final FieldDescriptor descriptor,
          final String camelCaseName,
          final Class<? extends GeneratedMessageV3> messageClass,
          final Class<? extends Builder<?>> builderClass,
          final String containingOneofCamelCaseName) {
        super(descriptor, camelCaseName, messageClass, builderClass, containingOneofCamelCaseName);

        enumDescriptor = descriptor.getEnumType();

        valueOfMethod = getMethodOrDie(type, "valueOf", EnumValueDescriptor.class);
        getValueDescriptorMethod = getMethodOrDie(type, "getValueDescriptor");

        supportUnknownEnumValue = !descriptor.legacyEnumFieldTreatedAsClosed();
        if (supportUnknownEnumValue) {
          getValueMethod = getMethodOrDie(messageClass, "get" + camelCaseName + "Value");
          getValueMethodBuilder = getMethodOrDie(builderClass, "get" + camelCaseName + "Value");
          setValueMethod = getMethodOrDie(builderClass, "set" + camelCaseName + "Value", int.class);
        }
      }

      private final EnumDescriptor enumDescriptor;

      private final Method valueOfMethod;
      private final Method getValueDescriptorMethod;

      private final boolean supportUnknownEnumValue;
      private Method getValueMethod;
      private Method getValueMethodBuilder;
      private Method setValueMethod;

      @Override
      public Object get(final GeneratedMessageV3 message) {
        if (supportUnknownEnumValue) {
          int value = (Integer) invokeOrDie(getValueMethod, message);
          return enumDescriptor.findValueByNumberCreatingIfUnknown(value);
        }
        return invokeOrDie(getValueDescriptorMethod, super.get(message));
      }

      @Override
      public Object get(final GeneratedMessageV3.Builder<?> builder) {
        if (supportUnknownEnumValue) {
          int value = (Integer) invokeOrDie(getValueMethodBuilder, builder);
          return enumDescriptor.findValueByNumberCreatingIfUnknown(value);
        }
        return invokeOrDie(getValueDescriptorMethod, super.get(builder));
      }

      @Override
      public void set(final Builder<?> builder, final Object value) {
        if (supportUnknownEnumValue) {
          // TODO: remove the unused variable
          Object unused =
              invokeOrDie(setValueMethod, builder, ((EnumValueDescriptor) value).getNumber());
          return;
        }
        super.set(builder, invokeOrDie(valueOfMethod, null, value));
      }
    }

    private static final class RepeatedEnumFieldAccessor extends RepeatedFieldAccessor {
      RepeatedEnumFieldAccessor(
          final FieldDescriptor descriptor,
          final String camelCaseName,
          final Class<? extends GeneratedMessageV3> messageClass,
          final Class<? extends Builder<?>> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        enumDescriptor = descriptor.getEnumType();

        valueOfMethod = getMethodOrDie(type, "valueOf", EnumValueDescriptor.class);
        getValueDescriptorMethod = getMethodOrDie(type, "getValueDescriptor");

        supportUnknownEnumValue = !descriptor.legacyEnumFieldTreatedAsClosed();
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

      private final EnumDescriptor enumDescriptor;

      private final Method valueOfMethod;
      private final Method getValueDescriptorMethod;

      private final boolean supportUnknownEnumValue;

      private Method getRepeatedValueMethod;
      private Method getRepeatedValueMethodBuilder;
      private Method setRepeatedValueMethod;
      private Method addRepeatedValueMethod;

      @Override
      public Object get(final GeneratedMessageV3 message) {
        final List<Object> newList = new ArrayList<>();
        final int size = getRepeatedCount(message);
        for (int i = 0; i < size; i++) {
          newList.add(getRepeated(message, i));
        }
        return Collections.unmodifiableList(newList);
      }

      @Override
      public Object get(final GeneratedMessageV3.Builder<?> builder) {
        final List<Object> newList = new ArrayList<>();
        final int size = getRepeatedCount(builder);
        for (int i = 0; i < size; i++) {
          newList.add(getRepeated(builder, i));
        }
        return Collections.unmodifiableList(newList);
      }

      @Override
      public Object getRepeated(final GeneratedMessageV3 message, final int index) {
        if (supportUnknownEnumValue) {
          int value = (Integer) invokeOrDie(getRepeatedValueMethod, message, index);
          return enumDescriptor.findValueByNumberCreatingIfUnknown(value);
        }
        return invokeOrDie(getValueDescriptorMethod, super.getRepeated(message, index));
      }

      @Override
      public Object getRepeated(final GeneratedMessageV3.Builder<?> builder, final int index) {
        if (supportUnknownEnumValue) {
          int value = (Integer) invokeOrDie(getRepeatedValueMethodBuilder, builder, index);
          return enumDescriptor.findValueByNumberCreatingIfUnknown(value);
        }
        return invokeOrDie(getValueDescriptorMethod, super.getRepeated(builder, index));
      }

      @Override
      public void setRepeated(final Builder<?> builder, final int index, final Object value) {
        if (supportUnknownEnumValue) {
          // TODO: remove the unused variable
          Object unused =
              invokeOrDie(
                  setRepeatedValueMethod,
                  builder,
                  index,
                  ((EnumValueDescriptor) value).getNumber());
          return;
        }
        super.setRepeated(builder, index, invokeOrDie(valueOfMethod, null, value));
      }

      @Override
      public void addRepeated(final Builder<?> builder, final Object value) {
        if (supportUnknownEnumValue) {
          // TODO: remove the unused variable
          Object unused =
              invokeOrDie(
                  addRepeatedValueMethod, builder, ((EnumValueDescriptor) value).getNumber());
          return;
        }
        super.addRepeated(builder, invokeOrDie(valueOfMethod, null, value));
      }
    }

    // ---------------------------------------------------------------

    /**
     * Field accessor for string fields.
     *
     * <p>This class makes getFooBytes() and setFooBytes() available for reflection API so that
     * reflection based serialize/parse functions can access the raw bytes of the field to preserve
     * non-UTF8 bytes in the string.
     *
     * <p>This ensures the serialize/parse round-trip safety, which is important for servers which
     * forward messages.
     */
    private static final class SingularStringFieldAccessor extends SingularFieldAccessor {
      SingularStringFieldAccessor(
          final FieldDescriptor descriptor,
          final String camelCaseName,
          final Class<? extends GeneratedMessageV3> messageClass,
          final Class<? extends Builder<?>> builderClass,
          final String containingOneofCamelCaseName) {
        super(descriptor, camelCaseName, messageClass, builderClass, containingOneofCamelCaseName);
        getBytesMethod = getMethodOrDie(messageClass, "get" + camelCaseName + "Bytes");
        setBytesMethodBuilder =
            getMethodOrDie(builderClass, "set" + camelCaseName + "Bytes", ByteString.class);
      }

      private final Method getBytesMethod;
      private final Method setBytesMethodBuilder;

      @Override
      public Object getRaw(final GeneratedMessageV3 message) {
        return invokeOrDie(getBytesMethod, message);
      }

      @Override
      public void set(GeneratedMessageV3.Builder<?> builder, Object value) {
        if (value instanceof ByteString) {
          // TODO: remove the unused variable
          Object unused = invokeOrDie(setBytesMethodBuilder, builder, value);
        } else {
          super.set(builder, value);
        }
      }
    }

    // ---------------------------------------------------------------

    private static final class SingularMessageFieldAccessor extends SingularFieldAccessor {
      SingularMessageFieldAccessor(
          final FieldDescriptor descriptor,
          final String camelCaseName,
          final Class<? extends GeneratedMessageV3> messageClass,
          final Class<? extends Builder<?>> builderClass,
          final String containingOneofCamelCaseName) {
        super(descriptor, camelCaseName, messageClass, builderClass, containingOneofCamelCaseName);

        newBuilderMethod = getMethodOrDie(type, "newBuilder");
        getBuilderMethodBuilder = getMethodOrDie(builderClass, "get" + camelCaseName + "Builder");
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
              .mergeFrom((Message) value)
              .buildPartial();
        }
      }

      @Override
      public void set(final Builder<?> builder, final Object value) {
        super.set(builder, coerceType(value));
      }

      @Override
      public Message.Builder newBuilder() {
        return (Message.Builder) invokeOrDie(newBuilderMethod, null);
      }

      @Override
      public Message.Builder getBuilder(GeneratedMessageV3.Builder<?> builder) {
        return (Message.Builder) invokeOrDie(getBuilderMethodBuilder, builder);
      }
    }

    private static final class RepeatedMessageFieldAccessor extends RepeatedFieldAccessor {
      RepeatedMessageFieldAccessor(
          final FieldDescriptor descriptor,
          final String camelCaseName,
          final Class<? extends GeneratedMessageV3> messageClass,
          final Class<? extends Builder<?>> builderClass) {
        super(descriptor, camelCaseName, messageClass, builderClass);

        newBuilderMethod = getMethodOrDie(type, "newBuilder");
        getBuilderMethodBuilder =
            getMethodOrDie(builderClass, "get" + camelCaseName + "Builder", Integer.TYPE);
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
              .mergeFrom((Message) value)
              .build();
        }
      }

      @Override
      public void setRepeated(final Builder<?> builder, final int index, final Object value) {
        super.setRepeated(builder, index, coerceType(value));
      }

      @Override
      public void addRepeated(final Builder<?> builder, final Object value) {
        super.addRepeated(builder, coerceType(value));
      }

      @Override
      public Message.Builder newBuilder() {
        return (Message.Builder) invokeOrDie(newBuilderMethod, null);
      }

      @Override
      public Message.Builder getRepeatedBuilder(
          final GeneratedMessageV3.Builder<?> builder, final int index) {
        return (Message.Builder) invokeOrDie(getBuilderMethodBuilder, builder, index);
      }
    }
  }

  /**
   * Replaces this object in the output stream with a serialized form. Part of Java's serialization
   * magic. Generated sub-classes must override this method by calling {@code return
   * super.writeReplace();}
   *
   * @return a SerializedForm of this message
   */
  protected Object writeReplace() throws ObjectStreamException {
    return new GeneratedMessageLite.SerializedForm(this);
  }

  /**
   * Checks that the {@link Extension} is non-Lite and returns it as a {@link GeneratedExtension}.
   */
  private static <MessageT extends ExtendableMessage<MessageT>, T>
      Extension<MessageT, T> checkNotLite(ExtensionLite<MessageT, T> extension) {
    if (extension.isLite()) {
      throw new IllegalArgumentException("Expected non-lite extension.");
    }

    return (Extension<MessageT, T>) extension;
  }

  protected static boolean isStringEmpty(final Object value) {
    if (value instanceof String) {
      return ((String) value).isEmpty();
    } else {
      return ((ByteString) value).isEmpty();
    }
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

  protected static void writeStringNoTag(CodedOutputStream output, final Object value)
      throws IOException {
    if (value instanceof String) {
      output.writeStringNoTag((String) value);
    } else {
      output.writeBytesNoTag((ByteString) value);
    }
  }

  protected static <V> void serializeIntegerMapTo(
      CodedOutputStream out,
      MapField<Integer, V> field,
      MapEntry<Integer, V> defaultEntry,
      int fieldNumber)
      throws IOException {
    Map<Integer, V> m = field.getMap();
    if (!out.isSerializationDeterministic()) {
      serializeMapTo(out, m, defaultEntry, fieldNumber);
      return;
    }
    // Sorting the unboxed keys and then look up the values during serialization is 2x faster
    // than sorting map entries with a custom comparator directly.
    int[] keys = new int[m.size()];
    int index = 0;
    for (int k : m.keySet()) {
      keys[index++] = k;
    }
    Arrays.sort(keys);
    for (int key : keys) {
      out.writeMessage(
          fieldNumber, defaultEntry.newBuilderForType().setKey(key).setValue(m.get(key)).build());
    }
  }

  protected static <V> void serializeLongMapTo(
      CodedOutputStream out,
      MapField<Long, V> field,
      MapEntry<Long, V> defaultEntry,
      int fieldNumber)
      throws IOException {
    Map<Long, V> m = field.getMap();
    if (!out.isSerializationDeterministic()) {
      serializeMapTo(out, m, defaultEntry, fieldNumber);
      return;
    }

    long[] keys = new long[m.size()];
    int index = 0;
    for (long k : m.keySet()) {
      keys[index++] = k;
    }
    Arrays.sort(keys);
    for (long key : keys) {
      out.writeMessage(
          fieldNumber, defaultEntry.newBuilderForType().setKey(key).setValue(m.get(key)).build());
    }
  }

  protected static <V> void serializeStringMapTo(
      CodedOutputStream out,
      MapField<String, V> field,
      MapEntry<String, V> defaultEntry,
      int fieldNumber)
      throws IOException {
    Map<String, V> m = field.getMap();
    if (!out.isSerializationDeterministic()) {
      serializeMapTo(out, m, defaultEntry, fieldNumber);
      return;
    }

    // Sorting the String keys and then look up the values during serialization is 25% faster than
    // sorting map entries with a custom comparator directly.
    String[] keys = new String[m.size()];
    keys = m.keySet().toArray(keys);
    Arrays.sort(keys);
    for (String key : keys) {
      out.writeMessage(
          fieldNumber, defaultEntry.newBuilderForType().setKey(key).setValue(m.get(key)).build());
    }
  }

  protected static <V> void serializeBooleanMapTo(
      CodedOutputStream out,
      MapField<Boolean, V> field,
      MapEntry<Boolean, V> defaultEntry,
      int fieldNumber)
      throws IOException {
    Map<Boolean, V> m = field.getMap();
    if (!out.isSerializationDeterministic()) {
      serializeMapTo(out, m, defaultEntry, fieldNumber);
      return;
    }
    maybeSerializeBooleanEntryTo(out, m, defaultEntry, fieldNumber, false);
    maybeSerializeBooleanEntryTo(out, m, defaultEntry, fieldNumber, true);
  }

  private static <V> void maybeSerializeBooleanEntryTo(
      CodedOutputStream out,
      Map<Boolean, V> m,
      MapEntry<Boolean, V> defaultEntry,
      int fieldNumber,
      boolean key)
      throws IOException {
    if (m.containsKey(key)) {
      out.writeMessage(
          fieldNumber, defaultEntry.newBuilderForType().setKey(key).setValue(m.get(key)).build());
    }
  }

  /** Serialize the map using the iteration order. */
  private static <K, V> void serializeMapTo(
      CodedOutputStream out, Map<K, V> m, MapEntry<K, V> defaultEntry, int fieldNumber)
      throws IOException {
    for (Map.Entry<K, V> entry : m.entrySet()) {
      out.writeMessage(
          fieldNumber,
          defaultEntry
              .newBuilderForType()
              .setKey(entry.getKey())
              .setValue(entry.getValue())
              .build());
    }
  }
}
