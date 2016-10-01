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

import static com.google.protobuf.Internal.checkNotNull;

import java.lang.reflect.Field;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;

/** Information for a single field in a protobuf message class. */
@ExperimentalApi
public final class FieldInfo implements Comparable<FieldInfo> {
  private final Field field;
  private final FieldType type;
  private final int fieldNumber;
  private final Field presenceField;
  private final int presenceMask;

  /** Constructs a new descriptor for a field. */
  public static FieldInfo forField(Field field, int fieldNumber, FieldType fieldType) {
    return new FieldInfo(field, fieldNumber, fieldType, null, 0);
  }

  /** Constructor for a field that uses a presence bit field (i.e. proto2 only). */
  public static FieldInfo forPresenceCheckedField(
      Field field, int fieldNumber, FieldType fieldType, Field presenceField, int presenceMask) {
    checkNotNull(presenceField, "presenceField");
    return new FieldInfo(field, fieldNumber, fieldType, presenceField, presenceMask);
  }

  private FieldInfo(
      Field field, int fieldNumber, FieldType type, Field presenceField, int presenceMask) {
    if (fieldNumber <= 0) {
      throw new IllegalArgumentException("fieldNumber must be positive: " + fieldNumber);
    }
    if (presenceField != null && !isExactlyOneBitSet(presenceMask)) {
      throw new IllegalArgumentException(
          "presenceMask must have exactly one bit set: " + presenceMask);
    }
    this.field = checkNotNull(field, "field");
    this.type = checkNotNull(type, "type");
    this.fieldNumber = fieldNumber;
    this.presenceField = presenceField;
    this.presenceMask = presenceMask;
  }

  /** Gets the subject {@link Field} of this descriptor. */
  public Field getField() {
    return field;
  }

  /** Gets the type information for the field. */
  public FieldType getType() {
    return type;
  }

  /** Gets the field number for the field. */
  public int getFieldNumber() {
    return fieldNumber;
  }

  @Override
  public int compareTo(FieldInfo o) {
    return fieldNumber - o.fieldNumber;
  }

  /**
   * For list fields, returns the generic argument that represents the type stored in the list. For
   * non-list fields, returns {@code null}.
   */
  public Class<?> getListElementType() {
    if (!type.isList()) {
      return null;
    }

    Type genericType = field.getGenericType();
    if (!(genericType instanceof ParameterizedType)) {
      throw new IllegalStateException(
          "Cannot determine parameterized type for list field " + fieldNumber);
    }

    Type type = ((ParameterizedType) field.getGenericType()).getActualTypeArguments()[0];
    return (Class<?>) type;
  }

  /** Gets the presence bit field. Only valid for unary fields. For lists, returns {@code null}. */
  public Field getPresenceField() {
    return presenceField;
  }

  /**
   * If {@link #getPresenceField()} is non-{@code null}, returns the mask used to identify the
   * presence bit for this field in the message.
   */
  public int getPresenceMask() {
    return presenceMask;
  }

  public static Builder newBuilder() {
    return new Builder();
  }

  /** A builder for {@link FieldInfo} instances. */
  public static final class Builder {
    private Field field;
    private FieldType type;
    private int fieldNumber;
    private Field presenceField;
    private int presenceMask;

    private Builder() {}

    public Builder withField(Field field) {
      this.field = field;
      return this;
    }

    public Builder withType(FieldType type) {
      this.type = type;
      return this;
    }

    public Builder withFieldNumber(int fieldNumber) {
      this.fieldNumber = fieldNumber;
      return this;
    }

    public Builder withPresenceField(Field presenceField) {
      this.presenceField = checkNotNull(presenceField, "presenceField");
      return this;
    }

    public Builder withPresenceMask(int presenceMask) {
      this.presenceMask = presenceMask;
      return this;
    }

    public FieldInfo build() {
      return new FieldInfo(field, fieldNumber, type, presenceField, presenceMask);
    }
  }

  private static boolean isExactlyOneBitSet(int value) {
    return value != 0 && (value & (value - 1)) == 0;
  }
}
