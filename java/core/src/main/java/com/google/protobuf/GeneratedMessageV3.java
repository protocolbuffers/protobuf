// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.AbstractMessage.BuilderParent;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.GeneratedMessage.Builder.BuilderParentImpl;

import com.google.protobuf.GeneratedMessage.Builder.BuilderParentImpl;

import java.util.List;

/**
 * Stub for GeneratedMessageV3 wrapping GeneratedMessage for compatibility with older gencode.
 * 
 * <p> Extends GeneratedMessage.ExtendableMessage instead of GeneratedMessage to allow "multiple
 * inheritance" for GeneratedMessageV3.ExtendableMessage subclass.
 *
 * @deprecated This class is deprecated, and slated for removal in the next Java breaking change
 *     (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses GeneratedMessage
 *     instead of GeneratedMessageV3.
 */
@Deprecated
public abstract class GeneratedMessageV3
    extends GeneratedMessage.ExtendableMessage<GeneratedMessageV3> {
  private static final long serialVersionUID = 1L;

  @Deprecated
  protected GeneratedMessageV3() {
    super();
  }

  @Deprecated
  protected GeneratedMessageV3(Builder<?> builder) {
    super(builder);
  }

  /* Returns GeneratedMessageV3.FieldAccessorTable instead of GeneratedMessage.FieldAccessorTable.
  * 
  * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
  * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
  * GeneratedMessage.internalGetFieldAccessorTable instead of GeneratedMessageV3.internalGetFieldAccessorTable.
  */
  @Deprecated
  @Override
  protected FieldAccessorTable internalGetFieldAccessorTable() {
    throw new UnsupportedOperationException("Should be overridden in gencode.");
  }

  @Deprecated
  protected interface BuilderParent extends AbstractMessage.BuilderParent {}

  @Deprecated
  protected abstract Message.Builder newBuilderForType(BuilderParent parent);

  // Support old gencode method removed in
  // https://github.com/protocolbuffers/protobuf/commit/787447430fc9a69c071393e85a380b664d261ab4
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

  /**
   * Stub for GeneratedMessageV3.Builder wrapping GeneratedMessage.Builder for compatibility with
   * older gencode.
   *
   * <p>Extends GeneratedMessage.ExtendableBuilder instead of GeneratedMessage.Builder to allow
   * "multiple inheritance" for GeneratedMessageV3.ExtendableBuilder subclass.
   *
   * @deprecated This class is deprecated, and slated for removal in the next Java breaking change
   *     (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
   *     GeneratedMessage.ExtendableBuilder instead of GeneratedMessageV3.ExtendableBuilder.
   */
  @Deprecated
  public abstract static class Builder<
          BuilderT extends Builder<BuilderT>>
      extends GeneratedMessage.ExtendableBuilder<GeneratedMessageV3, BuilderT> {

    private BuilderParentImpl meAsParent;

    @Deprecated
    protected Builder() {
      super(null);
    }

    @Deprecated
    protected Builder(BuilderParent builderParent) {
      super(builderParent);
    }

    /* Returns GeneratedMessageV3.FieldAccessorTable instead of GeneratedMessage.FieldAccessorTable.
     *
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
     * GeneratedMessage.FieldAccessorTable instead of GeneratedMessageV3.FieldAccessorTable.
     */
    @Deprecated
    @Override
    protected FieldAccessorTable internalGetFieldAccessorTable() {
      throw new UnsupportedOperationException("Should be overridden in gencode.");
    }

    private class BuilderParentImpl extends GeneratedMessage.Builder.BuilderParentImpl
        implements BuilderParent {}

    /* Returns GeneratedMessageV3.Builder.BuilderParent instead of GeneratedMessage.ExtendableMessage.Builder.BuilderParent.
     *
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
     * GeneratedMessage.Builder.BuilderParent instead of GeneratedMessageV3.Builder.BuilderParent.
     */
    @Deprecated
    @Override
    protected BuilderParent getParentForChildren() {
      if (meAsParent == null) {
        meAsParent = new BuilderParentImpl();
      }
      return meAsParent;
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT setUnknownFields(final UnknownFieldSet unknownFields) {
      return super.setUnknownFields(unknownFields);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT mergeUnknownFields(final UnknownFieldSet unknownFields) {
      return super.mergeUnknownFields(unknownFields);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT clone() {
      return super.clone();
    }

    /* Returns GeneratedMessageV3.Builder instead of GeneratedMessage.Builder.*/
    @Deprecated
    @Override
    public BuilderT clear() {
      return super.clear();
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT setField(final FieldDescriptor field, final Object value) {
      return super.setField(field, value);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT clearField(final FieldDescriptor field) {
      return super.clearField(field);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT clearOneof(final OneofDescriptor oneof) {
      return super.clearOneof(oneof);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24    @Deprecated
    @Deprecated
    @Override
    public int getRepeatedFieldCount(final FieldDescriptor field) {
      return super.getRepeatedFieldCount(field);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public Object getRepeatedField(final FieldDescriptor field, final int index) {
      return super.getRepeatedField(field, index);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT setRepeatedField(
        final FieldDescriptor field, final int index, final Object value) {
      return super.setRepeatedField(field, index, value);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT addRepeatedField(final FieldDescriptor field, final Object value) {
      return super.addRepeatedField(field, value);
    }
  }

  /**
   * Stub for GeneratedMessageV3.ExtendableMessageOrBuilder wrapping GeneratedMessage.ExtendableMessageOrBuilder for
   * compatibility with older gencode.
   *
   * @deprecated This class is deprecated, and slated for removal in the next Java breaking change
   *     (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
   *     GeneratedMessage.ExtendableMessageOrBuilder instead of GeneratedMessageV3.ExtendableMessageOrBuilder.
   */
  @Deprecated
  public interface ExtendableMessageOrBuilder<MessageT extends ExtendableMessage<MessageT>>
    extends GeneratedMessage.ExtendableMessageOrBuilder<GeneratedMessageV3> {
    
    // Support method removed from GeneratedMessage.ExtendableMessageOrBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    <T> boolean hasExtension(
      GeneratedExtension<MessageT, T> extension);

    // Support method removed from GeneratedMessage.ExtendableMessageOrBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    <T> int getExtensionCount(
      GeneratedExtension<MessageT, List<T>> extension);

    // Support method removed from GeneratedMessage.ExtendableMessageOrBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    <T> T getExtension(
      GeneratedExtension<MessageT, T> extension);
   
    // Support method removed from GeneratedMessage.ExtendableMessageOrBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    <T> T getExtension(
      GeneratedExtension<MessageT, List<T>> extension,
      int index);
  }
  /**
   * Stub for GeneratedMessageV3.ExtendableMessage wrapping GeneratedMessage.ExtendableMessage for
   * compatibility with older gencode.
   *
   * @deprecated This class is deprecated, and slated for removal in the next Java breaking change
   *     (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
   *     GeneratedMessage.FieldAccessorTable instead of GeneratedMessageV3.FieldAccessorTable.
   */
  @Deprecated
  public abstract static class ExtendableMessage<MessageT extends ExtendableMessage<MessageT>>
      extends GeneratedMessageV3  // Extends GeneratedMessage.ExtendableMessage via GeneratedMessageV3
      implements ExtendableMessageOrBuilder<MessageT> {

    @Deprecated
    protected ExtendableMessage() {
      super();
    }

    @Deprecated
    protected ExtendableMessage(ExtendableBuilder<MessageT, ?> builder) {
      super(builder);
    }

    // Support method removed from GeneratedMessage.ExtendableMessage in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    @Override
    public final <T> boolean hasExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return hasExtension((ExtensionLite<MessageT, T>) extension);
    }
    
    // Support method removed from GeneratedMessage.ExtendableMessage in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    @Override
    public final <T> int getExtensionCount(
        final GeneratedExtension<MessageT, List<T>> extension) {
      return getExtensionCount((ExtensionLite<MessageT, List<T>>) extension);
    }

    // Support method removed from GeneratedMessage.ExtendableMessage in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    @Override
    public final <T> T getExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return getExtension((ExtensionLite<MessageT, T>) extension);
    }

    // Support method removed from GeneratedMessage.ExtendableMessage in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    @Override
    public final <T> T getExtension(
        final GeneratedExtension<MessageT, List<T>> extension, final int index) {
      return getExtension((ExtensionLite<MessageT, List<T>>) extension, index);
    }

    /* Returns GeneratedMessageV3.FieldAccessorTable instead of GeneratedMessage.FieldAccessorTable.
    * 
    * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
    * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
    * GeneratedMessage.internalGetFieldAccessorTable instead of GeneratedMessageV3.internalGetFieldAccessorTable.
    */
    @Deprecated
    @Override
    protected FieldAccessorTable internalGetFieldAccessorTable() {
      throw new UnsupportedOperationException("Should be overridden in gencode.");
    }

    /**
     * Stub for GeneratedMessageV3.ExtendableMessage.ExtensionWriter wrapping
     * GeneratedMessage.ExtendableMessage.ExtensionWriter for compatibility with older gencode.
     *
     * @deprecated This class is deprecated, and slated for removal in the next Java breaking change
     *     (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
     *     GeneratedMessage.ExtendableMessage.ExtensionWriter instead of
     *     GeneratedMessageV3.ExtendableMessage.ExtensionWriter.
     */
    @Deprecated
    protected class ExtensionWriter extends GeneratedMessage.ExtendableMessage.ExtensionWriter {
      private ExtensionWriter(final boolean messageSetWireFormat) {
        super(messageSetWireFormat);
      }
    }

    /* Returns GeneratedMessageV3.ExtendableMessage.ExtensionWriter instead of GeneratedMessage.ExtendableMessage.ExtensionWriter.
     * 
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
     * GeneratedMessage.ExtendableMessage.ExtensionWriter instead of GeneratedMessageV3.ExtendableMessage.ExtensionWriter.
     */
    @Deprecated
    @Override
    protected ExtensionWriter newExtensionWriter() {
      return new ExtensionWriter(false);
    }
  }

  /**
   * Stub for GeneratedMessageV3.ExtendableBuilder wrapping GeneratedMessage.ExtendableBuilder for
   * compatibility with older gencode.
   *
   * @deprecated This class is deprecated, and slated for removal in the next Java breaking change
   *     (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
   *     GeneratedMessage.ExtendableBuilder instead of GeneratedMessageV3.ExtendableBuilder.
   */
  @Deprecated
  public abstract static class ExtendableBuilder<
          MessageT extends ExtendableMessage<MessageT>,
          BuilderT extends ExtendableBuilder<MessageT, BuilderT>>
      extends Builder<BuilderT>  // Extends GeneratedMessage.ExtendableBuilder via Builder
      implements ExtendableMessageOrBuilder<MessageT> {
    
    @Deprecated
    protected ExtendableBuilder() {
      super();
    }

    @Deprecated
    protected ExtendableBuilder(BuilderParent parent) {
      super(parent);
    }
    
    // Support method removed from GeneratedMessage.ExtendableBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    @Override
    public final <T> boolean hasExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return hasExtension((ExtensionLite<MessageT, T>) extension);
    }
    
    // Support method removed from GeneratedMessage.ExtendableBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    @Override
    public final <T> int getExtensionCount(
        final GeneratedExtension<MessageT, List<T>> extension) {
      return getExtensionCount((ExtensionLite<MessageT, List<T>>) extension);
    }

    // Support method removed from GeneratedMessage.ExtendableBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    @Override
    public final <T> T getExtension(
        final GeneratedExtension<MessageT, T> extension) {
      return getExtension((ExtensionLite<MessageT, T>) extension);
    }

    // Support method removed from GeneratedMessage.ExtendableBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    @Override
    public final <T> T getExtension(
        final GeneratedExtension<MessageT, List<T>> extension, final int index) {
      return getExtension((ExtensionLite<MessageT, List<T>>) extension, index);
    }

    // Support method removed from GeneratedMessage.ExtendableBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    public <T> BuilderT setExtension(
        final GeneratedMessage.GeneratedExtension extension, final T value) {
      return setExtension((ExtensionLite<GeneratedMessageV3, T>) extension, value);
    }

    // Support method removed from GeneratedMessage.ExtendableBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    public <T> BuilderT setExtension(
        final GeneratedMessage.GeneratedExtension extension,
        final int index,
        final T value) {
      return setExtension((ExtensionLite<GeneratedMessageV3, List<T>>) extension, index, value);
    }

    // Support method removed from GeneratedMessage.ExtendableBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    public <T> BuilderT addExtension(
        final GeneratedMessage.GeneratedExtension extension, final T value) {
      return addExtension((ExtensionLite<GeneratedMessageV3, List<T>>) extension, value);
    }

    // Support method removed from GeneratedMessage.ExtendableBuilder in
    // https://github.com/protocolbuffers/protobuf/commit/94a2a448518403341b8aa71335ab1123fbdcccd8
    @Deprecated
    public <T> BuilderT clearExtension(
        final GeneratedMessage.GeneratedExtension extension) {
      return clearExtension((ExtensionLite<GeneratedMessageV3, T>) extension);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT setField(final FieldDescriptor field, final Object value) {
      return super.setField(field, value);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT clearField(final FieldDescriptor field) {
      return super.clearField(field);
    }

    // // Support old gencode method override removed in
    // // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    // @Deprecated
    // @Override
    // public BuilderT clearOneof(final OneofDescriptor oneof) {
    //   return super.clearOneof(oneof);
    // }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT setRepeatedField(
        final FieldDescriptor field, final int index, final Object value) {
      return super.setRepeatedField(field, index, value);
    }

    // Support old gencode method override removed in
    // https://github.com/protocolbuffers/protobuf/commit/7bff169d32710b143951ec6ce2c4ea9a56e2ad24
    @Deprecated
    @Override
    public BuilderT addRepeatedField(final FieldDescriptor field, final Object value) {
      return super.addRepeatedField(field, value);
    }

    @Override
    @Deprecated
    protected final void mergeExtensionFields(final ExtendableMessage other) {
      super.mergeExtensionFields(other);
    }
  }

    /**
   * Stub for GeneratedMessageV3.FieldAccessorTable wrapping GeneratedMessage.FieldAccessorTable for compatibility with
   * older gencode.
   *
   * @deprecated This class is deprecated, and slated for removal in the next Java breaking change
   *     (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
   *     GeneratedMessage.FieldAccessorTable instead of GeneratedMessageV3.FieldAccessorTable.
   */
  @Deprecated
  public static final class FieldAccessorTable extends GeneratedMessage.FieldAccessorTable {

    @Deprecated
    public FieldAccessorTable(
        final Descriptor descriptor,
        final String[] camelCaseNames,
        final Class<? extends GeneratedMessageV3> messageClass,
        final Class<? extends Builder<?>> builderClass) {
      super(descriptor, camelCaseNames, messageClass, builderClass);
    }

    @Deprecated
    public FieldAccessorTable(final Descriptor descriptor, final String[] camelCaseNames) {
      super(descriptor, camelCaseNames);
    }

    /* Returns GeneratedMessageV3.FieldAccessorTable instead of GeneratedMessage.FieldAccessorTable.
     * 
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking change
     * (5.x in 2025 Q1). Users should update gencode to >= 4.26.x which uses
     * GeneratedMessage.ensureFieldAccessorsInitialized instead of GeneratedMessageV3.ensureFieldAccessorsInitialized.
     */
    @Deprecated
    @Override
    public FieldAccessorTable ensureFieldAccessorsInitialized(
        Class<? extends GeneratedMessage> messageClass,
        Class<? extends GeneratedMessage.Builder<?>> builderClass) {
        return (FieldAccessorTable) super.ensureFieldAccessorsInitialized(messageClass, builderClass);
    }
  }
}
