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

import com.google.protobuf.Internal.EnumVerifier;
import java.lang.reflect.Field;

/** Information for a single field in a protobuf message class. */
@CheckReturnValue
@ExperimentalApi
final class FieldInfo implements Comparable<FieldInfo> {
  private final Field field;
  private final FieldType type;
  private final Class<?> messageClass; // The message type for repeated message fields.
  private final int fieldNumber;
  private final Field presenceField;
  private final int presenceMask;
  private final boolean required;
  private final boolean enforceUtf8;
  private final OneofInfo oneof;
  private final Field cachedSizeField;
  /**
   * The actual type stored in the oneof value for this field. Since the oneof value is an {@link
   * Object}, primitives will store their boxed type. Only valid in conjunction with {@link #oneof}
   * (both must be either null or non-null.
   */
  private final Class<?> oneofStoredType;

  // TODO(liujisi): make map default entry lazy?
  private final Object mapDefaultEntry;

  private final EnumVerifier enumVerifier;

  /** Constructs a new descriptor for a field. */
  public static FieldInfo forField(
      Field field, int fieldNumber, FieldType fieldType, boolean enforceUtf8) {
    checkFieldNumber(fieldNumber);
    checkNotNull(field, "field");
    checkNotNull(fieldType, "fieldType");
    if (fieldType == FieldType.MESSAGE_LIST || fieldType == FieldType.GROUP_LIST) {
      throw new IllegalStateException("Shouldn't be called for repeated message fields.");
    }
    return new FieldInfo(
        field,
        fieldNumber,
        fieldType,
        /* messageClass= */ null,
        /* presenceField= */ null,
        /* presenceMask= */ 0,
        /* required= */ false,
        enforceUtf8,
        /* oneof= */ null,
        /* oneofStoredType= */ null,
        /* mapDefaultEntry= */ null,
        /* enumVerifier= */ null,
        /* cachedSizeField= */ null);
  }

  /** Constructs a new descriptor for a packed field. */
  public static FieldInfo forPackedField(
      Field field, int fieldNumber, FieldType fieldType, Field cachedSizeField) {
    checkFieldNumber(fieldNumber);
    checkNotNull(field, "field");
    checkNotNull(fieldType, "fieldType");
    if (fieldType == FieldType.MESSAGE_LIST || fieldType == FieldType.GROUP_LIST) {
      throw new IllegalStateException("Shouldn't be called for repeated message fields.");
    }
    return new FieldInfo(
        field,
        fieldNumber,
        fieldType,
        /* messageClass= */ null,
        /* presenceField= */ null,
        /* presenceMask= */ 0,
        /* required= */ false,
        /* enforceUtf8= */ false,
        /* oneof= */ null,
        /* oneofStoredType= */ null,
        /* mapDefaultEntry= */ null,
        /* enumVerifier= */ null,
        cachedSizeField);
  }

  /** Constructs a new descriptor for a repeated message field. */
  public static FieldInfo forRepeatedMessageField(
      Field field, int fieldNumber, FieldType fieldType, Class<?> messageClass) {
    checkFieldNumber(fieldNumber);
    checkNotNull(field, "field");
    checkNotNull(fieldType, "fieldType");
    checkNotNull(messageClass, "messageClass");
    return new FieldInfo(
        field,
        fieldNumber,
        fieldType,
        messageClass,
        /* presenceField= */ null,
        /* presenceMask= */ 0,
        /* required= */ false,
        /* enforceUtf8= */ false,
        /* oneof= */ null,
        /* oneofStoredType= */ null,
        /* mapDefaultEntry= */ null,
        /* enumVerifier= */ null,
        /* cachedSizeField= */ null);
  }

  public static FieldInfo forFieldWithEnumVerifier(
      Field field, int fieldNumber, FieldType fieldType, EnumVerifier enumVerifier) {
    checkFieldNumber(fieldNumber);
    checkNotNull(field, "field");
    return new FieldInfo(
        field,
        fieldNumber,
        fieldType,
        /* messageClass= */ null,
        /* presenceField= */ null,
        /* presenceMask= */ 0,
        /* required= */ false,
        /* enforceUtf8= */ false,
        /* oneof= */ null,
        /* oneofStoredType= */ null,
        /* mapDefaultEntry= */ null,
        enumVerifier,
        /* cachedSizeField= */ null);
  }

  public static FieldInfo forPackedFieldWithEnumVerifier(
      Field field,
      int fieldNumber,
      FieldType fieldType,
      EnumVerifier enumVerifier,
      Field cachedSizeField) {
    checkFieldNumber(fieldNumber);
    checkNotNull(field, "field");
    return new FieldInfo(
        field,
        fieldNumber,
        fieldType,
        /* messageClass= */ null,
        /* presenceField= */ null,
        /* presenceMask= */ 0,
        /* required= */ false,
        /* enforceUtf8= */ false,
        /* oneof= */ null,
        /* oneofStoredType= */ null,
        /* mapDefaultEntry= */ null,
        enumVerifier,
        cachedSizeField);
  }

  /** Constructor for a proto2 optional field. */
  public static FieldInfo forProto2OptionalField(
      Field field,
      int fieldNumber,
      FieldType fieldType,
      Field presenceField,
      int presenceMask,
      boolean enforceUtf8,
      EnumVerifier enumVerifier) {
    checkFieldNumber(fieldNumber);
    checkNotNull(field, "field");
    checkNotNull(fieldType, "fieldType");
    checkNotNull(presenceField, "presenceField");
    if (presenceField != null && !isExactlyOneBitSet(presenceMask)) {
      throw new IllegalArgumentException(
          "presenceMask must have exactly one bit set: " + presenceMask);
    }
    return new FieldInfo(
        field,
        fieldNumber,
        fieldType,
        /* messageClass= */ null,
        presenceField,
        presenceMask,
        /* required= */ false,
        enforceUtf8,
        /* oneof= */ null,
        /* oneofStoredType= */ null,
        /* mapDefaultEntry= */ null,
        enumVerifier,
        /* cachedSizeField= */ null);
  }

  /**
   * Constructor for a field that is part of a oneof.
   *
   * @param fieldNumber the unique field number for this field within the message.
   * @param fieldType the type of the field (must be non-null).
   * @param oneof the oneof for which this field is associated (must be non-null).
   * @param oneofStoredType the actual type stored in the oneof value for this field. Since the
   *     oneof value is an {@link Object}, primitives will store their boxed type. Must be non-null.
   * @param enforceUtf8 Only used for string fields. If {@code true}, will enforce UTF-8 on a string
   *     field.
   * @return the {@link FieldInfo} describing this field.
   */
  public static FieldInfo forOneofMemberField(
      int fieldNumber,
      FieldType fieldType,
      OneofInfo oneof,
      Class<?> oneofStoredType,
      boolean enforceUtf8,
      EnumVerifier enumVerifier) {
    checkFieldNumber(fieldNumber);
    checkNotNull(fieldType, "fieldType");
    checkNotNull(oneof, "oneof");
    checkNotNull(oneofStoredType, "oneofStoredType");
    if (!fieldType.isScalar()) {
      throw new IllegalArgumentException(
          "Oneof is only supported for scalar fields. Field "
              + fieldNumber
              + " is of type "
              + fieldType);
    }
    return new FieldInfo(
        /* field= */ null,
        fieldNumber,
        fieldType,
        /* messageClass= */ null,
        /* presenceField= */ null,
        /* presenceMask= */ 0,
        /* required= */ false,
        enforceUtf8,
        oneof,
        oneofStoredType,
        /* mapDefaultEntry= */ null,
        enumVerifier,
        /* cachedSizeField= */ null);
  }

  private static void checkFieldNumber(int fieldNumber) {
    if (fieldNumber <= 0) {
      throw new IllegalArgumentException("fieldNumber must be positive: " + fieldNumber);
    }
  }

  /** Constructor for a proto2 required field. */
  public static FieldInfo forProto2RequiredField(
      Field field,
      int fieldNumber,
      FieldType fieldType,
      Field presenceField,
      int presenceMask,
      boolean enforceUtf8,
      EnumVerifier enumVerifier) {
    checkFieldNumber(fieldNumber);
    checkNotNull(field, "field");
    checkNotNull(fieldType, "fieldType");
    checkNotNull(presenceField, "presenceField");
    if (presenceField != null && !isExactlyOneBitSet(presenceMask)) {
      throw new IllegalArgumentException(
          "presenceMask must have exactly one bit set: " + presenceMask);
    }
    return new FieldInfo(
        field,
        fieldNumber,
        fieldType,
        /* messageClass= */ null,
        presenceField,
        presenceMask,
        /* required= */ true,
        enforceUtf8,
        /* oneof= */ null,
        /* oneofStoredType= */ null,
        /* mapDefaultEntry= */ null,
        /* enumVerifier= */ enumVerifier,
        /* cachedSizeField= */ null);
  }

  public static FieldInfo forMapField(
      Field field, int fieldNumber, Object mapDefaultEntry, EnumVerifier enumVerifier) {
    checkNotNull(mapDefaultEntry, "mapDefaultEntry");
    checkFieldNumber(fieldNumber);
    checkNotNull(field, "field");
    return new FieldInfo(
        field,
        fieldNumber,
        FieldType.MAP,
        /* messageClass= */ null,
        /* presenceField= */ null,
        /* presenceMask= */ 0,
        /* required= */ false,
        /* enforceUtf8= */ true,
        /* oneof= */ null,
        /* oneofStoredType= */ null,
        mapDefaultEntry,
        enumVerifier,
        /* cachedSizeField= */ null);
  }

  private FieldInfo(
      Field field,
      int fieldNumber,
      FieldType type,
      Class<?> messageClass,
      Field presenceField,
      int presenceMask,
      boolean required,
      boolean enforceUtf8,
      OneofInfo oneof,
      Class<?> oneofStoredType,
      Object mapDefaultEntry,
      EnumVerifier enumVerifier,
      Field cachedSizeField) {
    this.field = field;
    this.type = type;
    this.messageClass = messageClass;
    this.fieldNumber = fieldNumber;
    this.presenceField = presenceField;
    this.presenceMask = presenceMask;
    this.required = required;
    this.enforceUtf8 = enforceUtf8;
    this.oneof = oneof;
    this.oneofStoredType = oneofStoredType;
    this.mapDefaultEntry = mapDefaultEntry;
    this.enumVerifier = enumVerifier;
    this.cachedSizeField = cachedSizeField;
  }

  /** Gets the field number for the field. */
  public int getFieldNumber() {
    return fieldNumber;
  }

  /** Gets the subject {@link Field} of this descriptor. */
  public Field getField() {
    return field;
  }

  /** Gets the type information for the field. */
  public FieldType getType() {
    return type;
  }

  /** Gets the oneof for which this field is a member, or {@code null} if not part of a oneof. */
  public OneofInfo getOneof() {
    return oneof;
  }

  /**
   * Gets the actual type stored in the oneof value by this field. Since the oneof value is an
   * {@link Object}, primitives will store their boxed type. For non-oneof fields, this will always
   * be {@code null}.
   */
  public Class<?> getOneofStoredType() {
    return oneofStoredType;
  }

  /** Gets the {@code EnumVerifier} if the field is an enum field. */
  public EnumVerifier getEnumVerifier() {
    return enumVerifier;
  }

  @Override
  public int compareTo(FieldInfo o) {
    return fieldNumber - o.fieldNumber;
  }

  /**
   * For repeated message fields, returns the message type of the field. For other fields, returns
   * {@code null}.
   */
  public Class<?> getListElementType() {
    return messageClass;
  }

  /** Gets the presence bit field. Only valid for unary fields. For lists, returns {@code null}. */
  public Field getPresenceField() {
    return presenceField;
  }

  public Object getMapDefaultEntry() {
    return mapDefaultEntry;
  }

  /**
   * If {@link #getPresenceField()} is non-{@code null}, returns the mask used to identify the
   * presence bit for this field in the message.
   */
  public int getPresenceMask() {
    return presenceMask;
  }

  /** Whether this is a required field. */
  public boolean isRequired() {
    return required;
  }

  /**
   * Whether a UTF-8 should be enforced on string fields. Only applies to strings and string lists.
   */
  public boolean isEnforceUtf8() {
    return enforceUtf8;
  }

  public Field getCachedSizeField() {
    return cachedSizeField;
  }

  /**
   * For singular or repeated message fields, returns the message type. For other fields, returns
   * {@code null}.
   */
  public Class<?> getMessageFieldClass() {
    switch (type) {
      case MESSAGE:
      case GROUP:
        return field != null ? field.getType() : oneofStoredType;
      case MESSAGE_LIST:
      case GROUP_LIST:
        return messageClass;
      default:
        return null;
    }
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
    private boolean required;
    private boolean enforceUtf8;
    private OneofInfo oneof;
    private Class<?> oneofStoredType;
    private Object mapDefaultEntry;
    private EnumVerifier enumVerifier;
    private Field cachedSizeField;

    private Builder() {}

    /**
     * Specifies the actual field on the message represented by this field. This should not be
     * called for oneof member fields.
     */
    public Builder withField(Field field) {
      if (oneof != null) {
        throw new IllegalStateException("Cannot set field when building a oneof.");
      }
      this.field = field;
      return this;
    }

    /** Specifies the type of this field. */
    public Builder withType(FieldType type) {
      this.type = type;
      return this;
    }

    /** Specifies the unique field number for this field within the message. */
    public Builder withFieldNumber(int fieldNumber) {
      this.fieldNumber = fieldNumber;
      return this;
    }

    /** Specifies proto2 presence information. This should not be called for oneof fields. */
    public Builder withPresence(Field presenceField, int presenceMask) {
      this.presenceField = checkNotNull(presenceField, "presenceField");
      this.presenceMask = presenceMask;
      return this;
    }

    /**
     * Sets the information for building a oneof member field. This is incompatible with {@link
     * #withField(Field)} and {@link #withPresence(Field, int)}.
     *
     * @param oneof the oneof for which this field is associated.
     * @param oneofStoredType the actual type stored in the oneof value for this field. Since the
     *     oneof value is an {@link Object}, primitives will store their boxed type.
     */
    public Builder withOneof(OneofInfo oneof, Class<?> oneofStoredType) {
      if (field != null || presenceField != null) {
        throw new IllegalStateException(
            "Cannot set oneof when field or presenceField have been provided");
      }
      this.oneof = oneof;
      this.oneofStoredType = oneofStoredType;
      return this;
    }

    public Builder withRequired(boolean required) {
      this.required = required;
      return this;
    }

    public Builder withMapDefaultEntry(Object mapDefaultEntry) {
      this.mapDefaultEntry = mapDefaultEntry;
      return this;
    }

    public Builder withEnforceUtf8(boolean enforceUtf8) {
      this.enforceUtf8 = enforceUtf8;
      return this;
    }

    public Builder withEnumVerifier(EnumVerifier enumVerifier) {
      this.enumVerifier = enumVerifier;
      return this;
    }

    public Builder withCachedSizeField(Field cachedSizeField) {
      this.cachedSizeField = cachedSizeField;
      return this;
    }

    public FieldInfo build() {
      if (oneof != null) {
        return forOneofMemberField(
            fieldNumber, type, oneof, oneofStoredType, enforceUtf8, enumVerifier);
      }
      if (mapDefaultEntry != null) {
        return forMapField(field, fieldNumber, mapDefaultEntry, enumVerifier);
      }
      if (presenceField != null) {
        if (required) {
          return forProto2RequiredField(
              field, fieldNumber, type, presenceField, presenceMask, enforceUtf8, enumVerifier);
        } else {
          return forProto2OptionalField(
              field, fieldNumber, type, presenceField, presenceMask, enforceUtf8, enumVerifier);
        }
      }
      if (enumVerifier != null) {
        if (cachedSizeField == null) {
          return forFieldWithEnumVerifier(field, fieldNumber, type, enumVerifier);
        } else {
          return forPackedFieldWithEnumVerifier(
              field, fieldNumber, type, enumVerifier, cachedSizeField);
        }
      } else {
        if (cachedSizeField == null) {
          return forField(field, fieldNumber, type, enforceUtf8);
        } else {
          return forPackedField(field, fieldNumber, type, cachedSizeField);
        }
      }
    }
  }

  private static boolean isExactlyOneBitSet(int value) {
    return value != 0 && (value & (value - 1)) == 0;
  }
}
