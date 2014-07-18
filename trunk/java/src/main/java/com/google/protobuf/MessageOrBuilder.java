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

import java.util.List;
import java.util.Map;

/**
 * Base interface for methods common to {@link Message} and
 * {@link Message.Builder} to provide type equivalency.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public interface MessageOrBuilder extends MessageLiteOrBuilder {

  // (From MessageLite, re-declared here only for return type covariance.)
  //@Override (Java 1.6 override semantics, but we must support 1.5)
  Message getDefaultInstanceForType();

  /**
   * Returns a list of field paths (e.g. "foo.bar.baz") of required fields
   * which are not set in this message.  You should call
   * {@link MessageLiteOrBuilder#isInitialized()} first to check if there
   * are any missing fields, as that method is likely to be much faster
   * than this one even when the message is fully-initialized.
   */
  List<String> findInitializationErrors();

  /**
   * Returns a comma-delimited list of required fields which are not set
   * in this message object.  You should call
   * {@link MessageLiteOrBuilder#isInitialized()} first to check if there
   * are any missing fields, as that method is likely to be much faster
   * than this one even when the message is fully-initialized.
   */
  String getInitializationErrorString();

  /**
   * Get the message's type's descriptor.  This differs from the
   * {@code getDescriptor()} method of generated message classes in that
   * this method is an abstract method of the {@code Message} interface
   * whereas {@code getDescriptor()} is a static method of a specific class.
   * They return the same thing.
   */
  Descriptors.Descriptor getDescriptorForType();

  /**
   * Returns a collection of all the fields in this message which are set
   * and their corresponding values.  A singular ("required" or "optional")
   * field is set iff hasField() returns true for that field.  A "repeated"
   * field is set iff getRepeatedFieldCount() is greater than zero.  The
   * values are exactly what would be returned by calling
   * {@link #getField(Descriptors.FieldDescriptor)} for each field.  The map
   * is guaranteed to be a sorted map, so iterating over it will return fields
   * in order by field number.
   * <br>
   * If this is for a builder, the returned map may or may not reflect future
   * changes to the builder.  Either way, the returned map is itself
   * unmodifiable.
   */
  Map<Descriptors.FieldDescriptor, Object> getAllFields();

  /**
   * Returns true if the given oneof is set.
   * @throws IllegalArgumentException if
   *           {@code oneof.getContainingType() != getDescriptorForType()}.
   */
  boolean hasOneof(Descriptors.OneofDescriptor oneof);

  /**
   * Obtains the FieldDescriptor if the given oneof is set. Returns null
   * if no field is set.
   */
  Descriptors.FieldDescriptor getOneofFieldDescriptor(
      Descriptors.OneofDescriptor oneof);

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
   * For enum fields, the EnumValueDescriptor for the value is returned. For
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
   * for the value is returned. For embedded message fields, the sub-message
   * is returned.
   * @throws IllegalArgumentException The field is not a repeated field, or
   *           {@code field.getContainingType() != getDescriptorForType()}.
   */
  Object getRepeatedField(Descriptors.FieldDescriptor field, int index);

  /** Get the {@link UnknownFieldSet} for this message. */
  UnknownFieldSet getUnknownFields();
}
