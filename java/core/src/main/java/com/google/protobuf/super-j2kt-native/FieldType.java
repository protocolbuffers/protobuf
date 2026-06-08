// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;


/** Enumeration identifying all relevant type information for a protobuf field. */
@SuppressWarnings("nullness")
@ExperimentalApi
public enum FieldType {
  DOUBLE(0, Collection.SCALAR, JavaType.DOUBLE),
  FLOAT(1, Collection.SCALAR, JavaType.FLOAT),
  INT64(2, Collection.SCALAR, JavaType.LONG),
  UINT64(3, Collection.SCALAR, JavaType.LONG),
  INT32(4, Collection.SCALAR, JavaType.INT),
  FIXED64(5, Collection.SCALAR, JavaType.LONG),
  FIXED32(6, Collection.SCALAR, JavaType.INT),
  BOOL(7, Collection.SCALAR, JavaType.BOOLEAN),
  STRING(8, Collection.SCALAR, JavaType.STRING),
  MESSAGE(9, Collection.SCALAR, JavaType.MESSAGE),
  BYTES(10, Collection.SCALAR, JavaType.BYTE_STRING),
  UINT32(11, Collection.SCALAR, JavaType.INT),
  ENUM(12, Collection.SCALAR, JavaType.ENUM),
  SFIXED32(13, Collection.SCALAR, JavaType.INT),
  SFIXED64(14, Collection.SCALAR, JavaType.LONG),
  SINT32(15, Collection.SCALAR, JavaType.INT),
  SINT64(16, Collection.SCALAR, JavaType.LONG),
  GROUP(17, Collection.SCALAR, JavaType.MESSAGE),
  DOUBLE_LIST(18, Collection.VECTOR, JavaType.DOUBLE),
  FLOAT_LIST(19, Collection.VECTOR, JavaType.FLOAT),
  INT64_LIST(20, Collection.VECTOR, JavaType.LONG),
  UINT64_LIST(21, Collection.VECTOR, JavaType.LONG),
  INT32_LIST(22, Collection.VECTOR, JavaType.INT),
  FIXED64_LIST(23, Collection.VECTOR, JavaType.LONG),
  FIXED32_LIST(24, Collection.VECTOR, JavaType.INT),
  BOOL_LIST(25, Collection.VECTOR, JavaType.BOOLEAN),
  STRING_LIST(26, Collection.VECTOR, JavaType.STRING),
  MESSAGE_LIST(27, Collection.VECTOR, JavaType.MESSAGE),
  BYTES_LIST(28, Collection.VECTOR, JavaType.BYTE_STRING),
  UINT32_LIST(29, Collection.VECTOR, JavaType.INT),
  ENUM_LIST(30, Collection.VECTOR, JavaType.ENUM),
  SFIXED32_LIST(31, Collection.VECTOR, JavaType.INT),
  SFIXED64_LIST(32, Collection.VECTOR, JavaType.LONG),
  SINT32_LIST(33, Collection.VECTOR, JavaType.INT),
  SINT64_LIST(34, Collection.VECTOR, JavaType.LONG),
  DOUBLE_LIST_PACKED(35, Collection.PACKED_VECTOR, JavaType.DOUBLE),
  FLOAT_LIST_PACKED(36, Collection.PACKED_VECTOR, JavaType.FLOAT),
  INT64_LIST_PACKED(37, Collection.PACKED_VECTOR, JavaType.LONG),
  UINT64_LIST_PACKED(38, Collection.PACKED_VECTOR, JavaType.LONG),
  INT32_LIST_PACKED(39, Collection.PACKED_VECTOR, JavaType.INT),
  FIXED64_LIST_PACKED(40, Collection.PACKED_VECTOR, JavaType.LONG),
  FIXED32_LIST_PACKED(41, Collection.PACKED_VECTOR, JavaType.INT),
  BOOL_LIST_PACKED(42, Collection.PACKED_VECTOR, JavaType.BOOLEAN),
  UINT32_LIST_PACKED(43, Collection.PACKED_VECTOR, JavaType.INT),
  ENUM_LIST_PACKED(44, Collection.PACKED_VECTOR, JavaType.ENUM),
  SFIXED32_LIST_PACKED(45, Collection.PACKED_VECTOR, JavaType.INT),
  SFIXED64_LIST_PACKED(46, Collection.PACKED_VECTOR, JavaType.LONG),
  SINT32_LIST_PACKED(47, Collection.PACKED_VECTOR, JavaType.INT),
  SINT64_LIST_PACKED(48, Collection.PACKED_VECTOR, JavaType.LONG),
  GROUP_LIST(49, Collection.VECTOR, JavaType.MESSAGE),
  MAP(50, Collection.MAP, JavaType.VOID);

  private final JavaType javaType_;
  private final int id;
  private final Collection collection;
  private final Class<?> elementType;
  private final boolean primitiveScalar;

  FieldType(int id, Collection collection, JavaType javaType) {
    this.id = id;
    this.collection = collection;
    this.javaType_ = javaType;

    switch (collection) {
      case MAP:
        elementType = javaType_.getBoxedType();
        break;
      case VECTOR:
        elementType = javaType_.getBoxedType();
        break;
      case SCALAR:
      default:
        elementType = null;
        break;
    }

    boolean primitiveScalar = false;
    if (collection == Collection.SCALAR) {
      switch (javaType_) {
        case BYTE_STRING:
        case MESSAGE:
        case STRING:
          break;
        default:
          primitiveScalar = true;
          break;
      }
    }
    this.primitiveScalar = primitiveScalar;
  }

  /** A reliable unique identifier for this type. */
  public int id() {
    return id;
  }

  /**
   * Gets the {@link JavaType} for this field. For lists, this identifies the type of the elements
   * contained within the list.
   */
  public JavaType getJavaType() {
    return javaType_;
  }

  /** Indicates whether a list field should be represented on the wire in packed form. */
  public boolean isPacked() {
    return Collection.PACKED_VECTOR.equals(collection);
  }

  /**
   * Indicates whether this field type represents a primitive scalar value. If this is {@code true},
   * then {@link #isScalar()} will also be {@code true}.
   */
  public boolean isPrimitiveScalar() {
    return primitiveScalar;
  }

  /** Indicates whether this field type represents a scalar value. */
  public boolean isScalar() {
    return collection == Collection.SCALAR;
  }

  /** Indicates whether this field represents a list of values. */
  public boolean isList() {
    return collection.isList();
  }

  /** Indicates whether this field represents a map. */
  public boolean isMap() {
    return collection == Collection.MAP;
  }

  /**
   * Looks up the appropriate {@link FieldType} by it's identifier.
   *
   * @return the {@link FieldType} or {@code null} if not found.
   */
  public static FieldType forId(int id) {
    if (id < 0 || id >= VALUES.length) {
      return null;
    }
    return VALUES[id];
  }

  private static final FieldType[] VALUES;

  static {
    FieldType[] values = values();
    VALUES = new FieldType[values.length];
    for (FieldType type : values) {
      VALUES[type.id] = type;
    }
  }

  enum Collection {
    SCALAR(false),
    VECTOR(true),
    PACKED_VECTOR(true),
    MAP(false);

    private final boolean isList;

    Collection(boolean isList) {
      this.isList = isList;
    }

    /**
     * @return the isList
     */
    public boolean isList() {
      return isList;
    }
  }
}
