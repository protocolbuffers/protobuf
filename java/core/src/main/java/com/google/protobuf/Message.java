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

// TODO(kenton):  Use generics?  E.g. Builder<BuilderType extends Builder>, then
//   mergeFrom*() could return BuilderType for better type-safety.

package com.google.protobuf;

import java.io.IOException;
import java.io.InputStream;
import java.util.Map;

/**
 * Abstract interface implemented by Protocol Message objects.
 * <p>
 * See also {@link MessageLite}, which defines most of the methods that typical
 * users care about.  {@link Message} adds to it methods that are not available
 * in the "lite" runtime.  The biggest added features are introspection and
 * reflection -- i.e., getting descriptors for the message type and accessing
 * the field values dynamically.
 *
 * @author kenton@google.com Kenton Varda
 */
public interface Message extends MessageLite, MessageOrBuilder {

  // (From MessageLite, re-declared here only for return type covariance.)
  @Override
  Parser<? extends Message> getParserForType();


  // -----------------------------------------------------------------
  // Comparison and hashing

  /**
   * Compares the specified object with this message for equality.  Returns
   * {@code true} if the given object is a message of the same type (as
   * defined by {@code getDescriptorForType()}) and has identical values for
   * all of its fields.  Subclasses must implement this; inheriting
   * {@code Object.equals()} is incorrect.
   *
   * @param other object to be compared for equality with this message
   * @return {@code true} if the specified object is equal to this message
   */
  @Override
  boolean equals(Object other);

  /**
   * Returns the hash code value for this message.  The hash code of a message
   * should mix the message's type (object identity of the descriptor) with its
   * contents (known and unknown field values).  Subclasses must implement this;
   * inheriting {@code Object.hashCode()} is incorrect.
   *
   * @return the hash code value for this message
   * @see Map#hashCode()
   */
  @Override
  int hashCode();

  // -----------------------------------------------------------------
  // Convenience methods.

  /**
   * Converts the message to a string in protocol buffer text format. This is
   * just a trivial wrapper around {@link
   * TextFormat#printToString(MessageOrBuilder)}.
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

  /**
   * Abstract interface implemented by Protocol Message builders.
   */
  interface Builder extends MessageLite.Builder, MessageOrBuilder {
    // (From MessageLite.Builder, re-declared here only for return type
    // covariance.)
    @Override
    Builder clear();

    /**
     * Merge {@code other} into the message being built.  {@code other} must
     * have the exact same type as {@code this} (i.e.
     * {@code getDescriptorForType() == other.getDescriptorForType()}).
     *
     * Merging occurs as follows.  For each field:<br>
     * * For singular primitive fields, if the field is set in {@code other},
     *   then {@code other}'s value overwrites the value in this message.<br>
     * * For singular message fields, if the field is set in {@code other},
     *   it is merged into the corresponding sub-message of this message
     *   using the same merging rules.<br>
     * * For repeated fields, the elements in {@code other} are concatenated
     *   with the elements in this message.
     * * For oneof groups, if the other message has one of the fields set,
     *   the group of this message is cleared and replaced by the field
     *   of the other message, so that the oneof constraint is preserved.
     *
     * This is equivalent to the {@code Message::MergeFrom} method in C++.
     */
    Builder mergeFrom(Message other);

    // (From MessageLite.Builder, re-declared here only for return type
    // covariance.)
    @Override
    Message build();

    @Override
    Message buildPartial();

    @Override
    Builder clone();

    @Override
    Builder mergeFrom(CodedInputStream input) throws IOException;

    @Override
    Builder mergeFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;

    /**
     * Get the message's type's descriptor.
     * See {@link Message#getDescriptorForType()}.
     */
    @Override
    Descriptors.Descriptor getDescriptorForType();

    /**
     * Create a Builder for messages of the appropriate type for the given
     * field.  Messages built with this can then be passed to setField(),
     * setRepeatedField(), or addRepeatedField().
     */
    Builder newBuilderForField(Descriptors.FieldDescriptor field);

    /**
     * Get a nested builder instance for the given field.
     * <p>
     * Normally, we hold a reference to the immutable message object for the
     * message type field. Some implementations(the generated message builders),
     * however, can also hold a reference to the builder object (a nested
     * builder) for the field.
     * <p>
     * If the field is already backed up by a nested builder, the nested builder
     * will be returned. Otherwise, a new field builder will be created and
     * returned. The original message field (if exist) will be merged into the
     * field builder, which will then be nested into its parent builder.
     * <p>
     * NOTE: implementations that do not support nested builders will throw
     * <code>UnsupportedOperationException</code>.
     */
    Builder getFieldBuilder(Descriptors.FieldDescriptor field);

    /**
     * Get a nested builder instance for the given repeated field instance.
     * <p>
     * Normally, we hold a reference to the immutable message object for the
     * message type field. Some implementations(the generated message builders),
     * however, can also hold a reference to the builder object (a nested
     * builder) for the field.
     * <p>
     * If the field is already backed up by a nested builder, the nested builder
     * will be returned. Otherwise, a new field builder will be created and
     * returned. The original message field (if exist) will be merged into the
     * field builder, which will then be nested into its parent builder.
     * <p>
     * NOTE: implementations that do not support nested builders will throw
     * <code>UnsupportedOperationException</code>.
     */
    Builder getRepeatedFieldBuilder(Descriptors.FieldDescriptor field,
                                    int index);

    /**
     * Sets a field to the given value.  The value must be of the correct type
     * for this field, i.e. the same type that
     * {@link Message#getField(Descriptors.FieldDescriptor)} would return.
     */
    Builder setField(Descriptors.FieldDescriptor field, Object value);

    /**
     * Clears the field.  This is exactly equivalent to calling the generated
     * "clear" accessor method corresponding to the field.
     */
    Builder clearField(Descriptors.FieldDescriptor field);

    /**
     * Clears the oneof.  This is exactly equivalent to calling the generated
     * "clear" accessor method corresponding to the oneof.
     */
    Builder clearOneof(Descriptors.OneofDescriptor oneof);

    /**
     * Sets an element of a repeated field to the given value.  The value must
     * be of the correct type for this field, i.e. the same type that
     * {@link Message#getRepeatedField(Descriptors.FieldDescriptor,int)} would
     * return.
     * @throws IllegalArgumentException The field is not a repeated field, or
     *           {@code field.getContainingType() != getDescriptorForType()}.
     */
    Builder setRepeatedField(Descriptors.FieldDescriptor field,
                             int index, Object value);

    /**
     * Like {@code setRepeatedField}, but appends the value as a new element.
     * @throws IllegalArgumentException The field is not a repeated field, or
     *           {@code field.getContainingType() != getDescriptorForType()}.
     */
    Builder addRepeatedField(Descriptors.FieldDescriptor field, Object value);

    /** Set the {@link UnknownFieldSet} for this message. */
    Builder setUnknownFields(UnknownFieldSet unknownFields);

    /**
     * Merge some unknown fields into the {@link UnknownFieldSet} for this
     * message.
     */
    Builder mergeUnknownFields(UnknownFieldSet unknownFields);

    // ---------------------------------------------------------------
    // Convenience methods.

    // (From MessageLite.Builder, re-declared here only for return type
    // covariance.)
    @Override
    Builder mergeFrom(ByteString data) throws InvalidProtocolBufferException;

    @Override
    Builder mergeFrom(ByteString data, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    @Override
    Builder mergeFrom(byte[] data) throws InvalidProtocolBufferException;

    @Override
    Builder mergeFrom(byte[] data, int off, int len) throws InvalidProtocolBufferException;

    @Override
    Builder mergeFrom(byte[] data, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    @Override
    Builder mergeFrom(byte[] data, int off, int len, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    @Override
    Builder mergeFrom(InputStream input) throws IOException;

    @Override
    Builder mergeFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;

    @Override
    boolean mergeDelimitedFrom(InputStream input) throws IOException;

    @Override
    boolean mergeDelimitedFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;
  }
}
