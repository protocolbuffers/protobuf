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

// TODO(kenton):  Use generics?  E.g. Builder<BuilderType extends Builder>, then
//   mergeFrom*() could return BuilderType for better type-safety.

package com.google.protobuf;

import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Map;

/**
 * Abstract interface implemented by Protocol Message objects.
 *
 * @author kenton@google.com Kenton Varda
 */
public interface Message {
  /**
   * Get the message's type's descriptor.  This differs from the
   * {@code getDescriptor()} method of generated message classes in that
   * this method is an abstract method of the {@code Message} interface
   * whereas {@code getDescriptor()} is a static method of a specific class.
   * They return the same thing.
   */
  Descriptors.Descriptor getDescriptorForType();

  /**
   * Get an instance of the type with all fields set to their default values.
   * This may or may not be a singleton.  This differs from the
   * {@code getDefaultInstance()} method of generated message classes in that
   * this method is an abstract method of the {@code Message} interface
   * whereas {@code getDefaultInstance()} is a static method of a specific
   * class.  They return the same thing.
   */
  Message getDefaultInstanceForType();

  /**
   * Returns a collection of all the fields in this message which are set
   * and their corresponding values.  A singular ("required" or "optional")
   * field is set iff hasField() returns true for that field.  A "repeated"
   * field is set iff getRepeatedFieldSize() is greater than zero.  The
   * values are exactly what would be returned by calling
   * {@link #getField(Descriptors.FieldDescriptor)} for each field.  The map
   * is guaranteed to be a sorted map, so iterating over it will return fields
   * in order by field number.
   */
  Map<Descriptors.FieldDescriptor, Object> getAllFields();

  /**
   * Returns true if the given field is set.  This is exactly equivalent to
   * calling the generated "has" accessor method corresponding to the field.
   * @throws IllegalArgumentException The field is a repeated field, or
   *           {@code field.getContainingType() != getDescriptorForType()}.
   */
  boolean hasField(Descriptors.FieldDescriptor field);

  /**
   * Obtains the value of the given field, or the default value if it is
   * not set.  For primitive fields, the boxed primitive value is returned.
   * For enum fields, the EnumValueDescriptor for the value is returend. For
   * embedded message fields, the sub-message is returned.  For repeated
   * fields, a java.util.List is returned.
   */
  Object getField(Descriptors.FieldDescriptor field);

  /**
   * Gets the number of elements of a repeated field.  This is exactly
   * equivalent to calling the generated "Count" accessor method corresponding
   * to the field.
   * @throws IllegalArgumentException The field is not a repeated field, or
   *           {@code field.getContainingType() != getDescriptorForType()}.
   */
  int getRepeatedFieldCount(Descriptors.FieldDescriptor field);

  /**
   * Gets an element of a repeated field.  For primitive fields, the boxed
   * primitive value is returned.  For enum fields, the EnumValueDescriptor
   * for the value is returend. For embedded message fields, the sub-message
   * is returned.
   * @throws IllegalArgumentException The field is not a repeated field, or
   *           {@code field.getContainingType() != getDescriptorForType()}.
   */
  Object getRepeatedField(Descriptors.FieldDescriptor field, int index);

  /** Get the {@link UnknownFieldSet} for this message. */
  UnknownFieldSet getUnknownFields();

  /**
   * Returns true if all required fields in the message and all embedded
   * messages are set, false otherwise.
   */
  boolean isInitialized();

  /**
   * Serializes the message and writes it to {@code output}.  This does not
   * flush or close the stream.
   */
  void writeTo(CodedOutputStream output) throws IOException;

  /**
   * Get the number of bytes required to encode this message.  The result
   * is only computed on the first call and memoized after that.
   */
  int getSerializedSize();

  // -----------------------------------------------------------------
  // Comparison and hashing

  /**
   * Compares the specified object with this message for equality.  Returns
   * <tt>true</tt> if the given object is a message of the same type (as
   * defined by {@code getDescriptorForType()}) and has identical values for
   * all of its fields.
   *
   * @param other object to be compared for equality with this message
   * @return <tt>true</tt> if the specified object is equal to this message
   */
  boolean equals(Object other);

  /**
   * Returns the hash code value for this message.  The hash code of a message
   * is defined to be <tt>getDescriptor().hashCode() ^ map.hashCode()</tt>,
   * where <tt>map</tt> is a map of field numbers to field values.
   *
   * @return the hash code value for this message
   * @see Map#hashCode()
   */
  int hashCode();

  // -----------------------------------------------------------------
  // Convenience methods.

  /**
   * Converts the message to a string in protocol buffer text format. This is
   * just a trivial wrapper around {@link TextFormat#printToString(Message)}.
   */
  String toString();

  /**
   * Serializes the message to a {@code ByteString} and returns it. This is
   * just a trivial wrapper around
   * {@link #writeTo(CodedOutputStream)}.
   */
  ByteString toByteString();

  /**
   * Serializes the message to a {@code byte} array and returns it.  This is
   * just a trivial wrapper around
   * {@link #writeTo(CodedOutputStream)}.
   */
  byte[] toByteArray();

  /**
   * Serializes the message and writes it to {@code output}.  This is just a
   * trivial wrapper around {@link #writeTo(CodedOutputStream)}.  This does
   * not flush or close the stream.
   */
  void writeTo(OutputStream output) throws IOException;

  // =================================================================
  // Builders

  /**
   * Constructs a new builder for a message of the same type as this message.
   */
  Builder newBuilderForType();

  /**
   * Abstract interface implemented by Protocol Message builders.
   */
  public static interface Builder extends Cloneable {
    /** Resets all fields to their default values. */
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
     *
     * This is equivalent to the {@code Message::MergeFrom} method in C++.
     */
    Builder mergeFrom(Message other);

    /**
     * Construct the final message.  Once this is called, the Builder is no
     * longer valid, and calling any other method may throw a
     * NullPointerException.  If you need to continue working with the builder
     * after calling {@code build()}, {@code clone()} it first.
     * @throws UninitializedMessageException The message is missing one or more
     *         required fields (i.e. {@link #isInitialized()} returns false).
     *         Use {@link #buildPartial()} to bypass this check.
     */
    Message build();

    /**
     * Like {@link #build()}, but does not throw an exception if the message
     * is missing required fields.  Instead, a partial message is returned.
     */
    Message buildPartial();

    /**
     * Clones the Builder.
     * @see Object#clone()
     */
    Builder clone();

    /**
     * Returns true if all required fields in the message and all embedded
     * messages are set, false otherwise.
     */
    boolean isInitialized();

    /**
     * Parses a message of this type from the input and merges it with this
     * message, as if using {@link Builder#mergeFrom(Message)}.
     *
     * <p>Warning:  This does not verify that all required fields are present in
     * the input message.  If you call {@link #build()} without setting all
     * required fields, it will throw an {@link UninitializedMessageException},
     * which is a {@code RuntimeException} and thus might not be caught.  There
     * are a few good ways to deal with this:
     * <ul>
     *   <li>Call {@link #isInitialized()} to verify that all required fields
     *       are set before building.
     *   <li>Parse the message separately using one of the static
     *       {@code parseFrom} methods, then use {@link #mergeFrom(Message)}
     *       to merge it with this one.  {@code parseFrom} will throw an
     *       {@link InvalidProtocolBufferException} (an {@code IOException})
     *       if some required fields are missing.
     *   <li>Use {@code buildPartial()} to build, which ignores missing
     *       required fields.
     * </ul>
     *
     * <p>Note:  The caller should call
     * {@link CodedInputStream#checkLastTagWas(int)} after calling this to
     * verify that the last tag seen was the appropriate end-group tag,
     * or zero for EOF.
     */
    Builder mergeFrom(CodedInputStream input) throws IOException;

    /**
     * Like {@link Builder#mergeFrom(CodedInputStream)}, but also
     * parses extensions.  The extensions that you want to be able to parse
     * must be registered in {@code extensionRegistry}.  Extensions not in
     * the registry will be treated as unknown fields.
     */
    Builder mergeFrom(CodedInputStream input,
                      ExtensionRegistry extensionRegistry)
                      throws IOException;

    /**
     * Get the message's type's descriptor.
     * See {@link Message#getDescriptorForType()}.
     */
    Descriptors.Descriptor getDescriptorForType();

    /**
     * Get the message's type's default instance.
     * See {@link Message#getDefaultInstanceForType()}.
     */
    Message getDefaultInstanceForType();

    /**
     * Like {@link Message#getAllFields()}.  The returned map may or may not
     * reflect future changes to the builder.  Either way, the returned map is
     * itself unmodifiable.
     */
    Map<Descriptors.FieldDescriptor, Object> getAllFields();

    /**
     * Create a Builder for messages of the appropriate type for the given
     * field.  Messages built with this can then be passed to setField(),
     * setRepeatedField(), or addRepeatedField().
     */
    Builder newBuilderForField(Descriptors.FieldDescriptor field);

    /** Like {@link Message#hasField(Descriptors.FieldDescriptor)} */
    boolean hasField(Descriptors.FieldDescriptor field);

    /** Like {@link Message#getField(Descriptors.FieldDescriptor)} */
    Object getField(Descriptors.FieldDescriptor field);

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
     * Like {@link Message#getRepeatedFieldCount(Descriptors.FieldDescriptor)}
     */
    int getRepeatedFieldCount(Descriptors.FieldDescriptor field);

    /**
     * Like {@link Message#getRepeatedField(Descriptors.FieldDescriptor,int)}
     */
    Object getRepeatedField(Descriptors.FieldDescriptor field, int index);

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

    /** Get the {@link UnknownFieldSet} for this message. */
    UnknownFieldSet getUnknownFields();

    /** Set the {@link UnknownFieldSet} for this message. */
    Builder setUnknownFields(UnknownFieldSet unknownFields);

    /**
     * Merge some unknown fields into the {@link UnknownFieldSet} for this
     * message.
     */
    Builder mergeUnknownFields(UnknownFieldSet unknownFields);

    // ---------------------------------------------------------------
    // Convenience methods.

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.
     */
    Builder mergeFrom(ByteString data) throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream,ExtensionRegistry)}.
     */
    Builder mergeFrom(ByteString data,
                      ExtensionRegistry extensionRegistry)
                      throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.
     */
    public Builder mergeFrom(byte[] data) throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream,ExtensionRegistry)}.
     */
    Builder mergeFrom(byte[] data,
                      ExtensionRegistry extensionRegistry)
                      throws InvalidProtocolBufferException;

    /**
     * Parse a message of this type from {@code input} and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.  Note that this method always
     * reads the <i>entire</i> input (unless it throws an exception).  If you
     * want it to stop earlier, you will need to wrap your input in some
     * wrapper stream that limits reading.  Despite usually reading the entire
     * input, this does not close the stream.
     */
    Builder mergeFrom(InputStream input) throws IOException;

    /**
     * Parse a message of this type from {@code input} and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream,ExtensionRegistry)}.
     */
    Builder mergeFrom(InputStream input,
                      ExtensionRegistry extensionRegistry)
                      throws IOException;
  }
}
