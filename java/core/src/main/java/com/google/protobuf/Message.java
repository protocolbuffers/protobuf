// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;
import java.io.InputStream;
import java.util.Map;

/**
 * Abstract interface implemented by Protocol Message objects.
 *
 * <p>See also {@link MessageLite}, which defines most of the methods that typical users care about.
 * {@link Message} adds methods that are not available in the "lite" runtime. The biggest added
 * features are introspection and reflection; that is, getting descriptors for the message type and
 * accessing the field values dynamically.
 *
 * @author kenton@google.com Kenton Varda
 */
@CheckReturnValue
public interface Message extends MessageLite, MessageOrBuilder {

  // (From MessageLite, re-declared here only for return type covariance.)
  @Override
  Parser<? extends Message> getParserForType();

  // -----------------------------------------------------------------
  // Comparison and hashing

  /**
   * Compares the specified object with this message for equality. Returns {@code true} if the given
   * object is a message of the same type (as defined by {@code getDescriptorForType()}) and has
   * identical values for all of its fields. Subclasses must implement this; inheriting {@code
   * Object.equals()} is incorrect.
   *
   * @param other object to be compared for equality with this message
   * @return {@code true} if the specified object is equal to this message
   */
  @Override
  boolean equals(
          Object other);

  /**
   * Returns the hash code value for this message. The hash code of a message should mix the
   * message's type (object identity of the descriptor) with its contents (known and unknown field
   * values). Subclasses must implement this; inheriting {@code Object.hashCode()} is incorrect.
   *
   * @return the hash code value for this message
   * @see Map#hashCode()
   */
  @Override
  int hashCode();

  // -----------------------------------------------------------------
  // Convenience methods.

  /**
   * Converts the message to a string in protocol buffer text format. This is just a trivial wrapper
   * around {@link TextFormat.Printer#printToString(MessageOrBuilder)}.
   */
  @Override
  String toString();

  // =================================================================
  // Builders

  // (From MessageLite, re-declared here only for return type covariance.)
  @Override
  Builder newBuilderForType();

  @Override
  Builder toBuilder();

  /** Abstract interface implemented by Protocol Message builders. */
  interface Builder extends MessageLite.Builder, MessageOrBuilder {
    // (From MessageLite.Builder, re-declared here only for return type
    // covariance.)
    @Override
    @CanIgnoreReturnValue
    Builder clear();

    /**
     * Merge {@code other} into the message being built. {@code other} must have the exact same type
     * as {@code this} (i.e. {@code getDescriptorForType() == other.getDescriptorForType()}).
     *
     * <p>Merging occurs as follows. For each field:<br>
     * * For singular primitive fields, if the field is set in {@code other}, then {@code other}'s
     * value overwrites the value in this message.<br>
     * * For singular message fields, if the field is set in {@code other}, it is merged into the
     * corresponding sub-message of this message using the same merging rules.<br>
     * * For repeated fields, the elements in {@code other} are concatenated with the elements in
     * this message.<br>
     * * For oneof groups, if the other message has one of the fields set, the group of this message
     * is cleared and replaced by the field of the other message, so that the oneof constraint is
     * preserved.
     *
     * <p>This is equivalent to the {@code Message::MergeFrom} method in C++.
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(Message other);

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(CodedInputStream input) throws IOException;

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;

    // ---------------------------------------------------------------
    // Convenience methods.

    // (From MessageLite.Builder, re-declared here only for return type
    // covariance.)
    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(ByteString data) throws InvalidProtocolBufferException;

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(ByteString data, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(byte[] data) throws InvalidProtocolBufferException;

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(byte[] data, int off, int len) throws InvalidProtocolBufferException;

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(byte[] data, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(byte[] data, int off, int len, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(InputStream input) throws IOException;

    @Override
    @CanIgnoreReturnValue
    Builder mergeFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;

    // (From MessageLite.Builder, re-declared here only for return type
    // covariance.)
    @Override
    Message build();

    @Override
    Message buildPartial();

    @Override
    Builder clone();

    /** Get the message's type's descriptor. See {@link Message#getDescriptorForType()}. */
    @Override
    Descriptors.Descriptor getDescriptorForType();

    /**
     * Create a builder for messages of the appropriate type for the given field. The builder is NOT
     * nested in the current builder. However, messages built with the builder can then be passed to
     * the {@link #setField(Descriptors.FieldDescriptor, Object)}, {@link
     * #setRepeatedField(Descriptors.FieldDescriptor, int, Object)}, or {@link
     * #addRepeatedField(Descriptors.FieldDescriptor, Object)} method of the current builder.
     *
     * <p>To obtain a builder nested in the current builder, use {@link
     * #getFieldBuilder(Descriptors.FieldDescriptor)} instead.
     */
    Builder newBuilderForField(Descriptors.FieldDescriptor field);

    /**
     * Get a nested builder instance for the given field.
     *
     * <p>Normally, we hold a reference to the immutable message object for the message type field.
     * Some implementations (the generated message builders) can also hold a reference to the
     * builder object (a nested builder) for the field.
     *
     * <p>If the field is already backed up by a nested builder, the nested builder is returned.
     * Otherwise, a new field builder is created and returned. The original message field (if one
     * exists) is merged into the field builder, which is then nested into its parent builder.
     */
    Builder getFieldBuilder(Descriptors.FieldDescriptor field);

    /**
     * Get a nested builder instance for the given repeated field instance.
     *
     * <p>Normally, we hold a reference to the immutable message object for the message type field.
     * Some implementations (the generated message builders) can also hold a reference to the
     * builder object (a nested builder) for the field.
     *
     * <p>If the field is already backed up by a nested builder, the nested builder is returned.
     * Otherwise, a new field builder is created and returned. The original message field (if one
     * exists) is merged into the field builder, which is then nested into its parent builder.
     */
    Builder getRepeatedFieldBuilder(Descriptors.FieldDescriptor field, int index);

    /**
     * Sets a field to the given value. The value must be of the correct type for this field, that
     * is, the same type that {@link Message#getField(Descriptors.FieldDescriptor)} returns.
     */
    @CanIgnoreReturnValue
    Builder setField(Descriptors.FieldDescriptor field, Object value);

    /**
     * Clears the field. This is exactly equivalent to calling the generated "clear" accessor method
     * corresponding to the field.
     */
    @CanIgnoreReturnValue
    Builder clearField(Descriptors.FieldDescriptor field);

    /**
     * Clears the oneof. This is exactly equivalent to calling the generated "clear" accessor method
     * corresponding to the oneof.
     */
    @CanIgnoreReturnValue
    Builder clearOneof(Descriptors.OneofDescriptor oneof);

    /**
     * Sets an element of a repeated field to the given value. The value must be of the correct type
     * for this field; that is, the same type that {@link
     * Message#getRepeatedField(Descriptors.FieldDescriptor,int)} returns.
     *
     * @throws IllegalArgumentException if the field is not a repeated field, or {@code
     *     field.getContainingType() != getDescriptorForType()}.
     */
    @CanIgnoreReturnValue
    Builder setRepeatedField(Descriptors.FieldDescriptor field, int index, Object value);

    /**
     * Like {@code setRepeatedField}, but appends the value as a new element.
     *
     * @throws IllegalArgumentException if the field is not a repeated field, or {@code
     *     field.getContainingType() != getDescriptorForType()}
     */
    @CanIgnoreReturnValue
    Builder addRepeatedField(Descriptors.FieldDescriptor field, Object value);

    /** Set the {@link UnknownFieldSet} for this message. */
    @CanIgnoreReturnValue
    Builder setUnknownFields(UnknownFieldSet unknownFields);

    /** Merge some unknown fields into the {@link UnknownFieldSet} for this message. */
    @CanIgnoreReturnValue
    Builder mergeUnknownFields(UnknownFieldSet unknownFields);

    // ---------------------------------------------------------------
    // Convenience methods.

    // (From MessageLite.Builder, re-declared here only for return type
    // covariance.)
    @Override
    boolean mergeDelimitedFrom(InputStream input) throws IOException;

    @Override
    boolean mergeDelimitedFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;
  }
}
