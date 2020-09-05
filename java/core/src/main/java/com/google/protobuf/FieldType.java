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

import java.lang.reflect.Field;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.util.List;

/** Enumeration identifying all relevant type information for a protobuf field. */
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

  private final JavaType javaType;
  private final int id;
  private final Collection collection;
  private final Class<?> elementType;
  private final boolean primitiveScalar;

  FieldType(int id, Collection collection, JavaType javaType) {
    this.id = id;
    this.collection = collection;
    this.javaType = javaType;

    switch (collection) {
      case MAP:
        elementType = javaType.getBoxedType();
        break;
      case VECTOR:
        elementType = javaType.getBoxedType();
        break;
      case SCALAR:
      default:
        elementType = null;
        break;
    }

    boolean primitiveScalar = false;
    if (collection == Collection.SCALAR) {
      switch (javaType) {
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
    return javaType;
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

  /** Indicates whether or not this {@link FieldType} can be applied to the given {@link Field}. */
  public boolean isValidForField(Field field) {
    if (Collection.VECTOR.equals(collection)) {
      return isValidForList(field);
    } else {
      return javaType.getType().isAssignableFrom(field.getType());
    }
  }

  private boolean isValidForList(Field field) {
    Class<?> clazz = field.getType();
    if (!javaType.getType().isAssignableFrom(clazz)) {
      // The field isn't a List type.
      return false;
    }
    Type[] types = EMPTY_TYPES;
    Type genericType = field.getGenericType();
    if (genericType instanceof ParameterizedType) {
      types = ((ParameterizedType) field.getGenericType()).getActualTypeArguments();
    }
    Type listParameter = getListParameter(clazz, types);
    if (!(listParameter instanceof Class)) {
      // It's a wildcard, we should allow anything in the list.
      return true;
    }
    return elementType.isAssignableFrom((Class<?>) listParameter);
  }

  /**
   * Looks up the appropriate {@link FieldType} by it's identifier.
   *
   * @return the {@link FieldType} or {@code null} if not found.
   */
  /* @Nullable */
  public static FieldType forId(int id) {
    if (id < 0 || id >= VALUES.length) {
      return null;
    }
    return VALUES[id];
  }

  private static final FieldType[] VALUES;
  private static final Type[] EMPTY_TYPES = new Type[0];

  static {
    FieldType[] values = values();
    VALUES = new FieldType[values.length];
    for (FieldType type : values) {
      VALUES[type.id] = type;
    }
  }

  /**
   * Given a class, finds a generic super class or interface that extends {@link List}.
   *
   * @return the generic super class/interface, or {@code null} if not found.
   */
  /* @Nullable */
  private static Type getGenericSuperList(Class<?> clazz) {
    // First look at interfaces.
    Type[] genericInterfaces = clazz.getGenericInterfaces();
    for (Type genericInterface : genericInterfaces) {
      if (genericInterface instanceof ParameterizedType) {
        ParameterizedType parameterizedType = (ParameterizedType) genericInterface;
        Class<?> rawType = (Class<?>) parameterizedType.getRawType();
        if (List.class.isAssignableFrom(rawType)) {
          return genericInterface;
        }
      }
    }

    // Try the subclass
    Type type = clazz.getGenericSuperclass();
    if (type instanceof ParameterizedType) {
      ParameterizedType parameterizedType = (ParameterizedType) type;
      Class<?> rawType = (Class<?>) parameterizedType.getRawType();
      if (List.class.isAssignableFrom(rawType)) {
        return type;
      }
    }

    // No super class/interface extends List.
    return null;
  }

  /**
   * Inspects the inheritance hierarchy for the given class and finds the generic type parameter for
   * {@link List}.
   *
   * @param clazz the class to begin the search.
   * @param realTypes the array of actual type parameters for {@code clazz}. These will be used to
   *     substitute generic parameters up the inheritance hierarchy. If {@code clazz} does not have
   *     any generic parameters, this list should be empty.
   * @return the {@link List} parameter.
   */
  private static Type getListParameter(Class<?> clazz, Type[] realTypes) {
    top:
    while (clazz != List.class) {
      // First look at generic subclass and interfaces.
      Type genericType = getGenericSuperList(clazz);
      if (genericType instanceof ParameterizedType) {
        // Replace any generic parameters with the real values.
        ParameterizedType parameterizedType = (ParameterizedType) genericType;
        Type[] superArgs = parameterizedType.getActualTypeArguments();
        for (int i = 0; i < superArgs.length; ++i) {
          Type superArg = superArgs[i];
          if (superArg instanceof TypeVariable) {
            // Get the type variables for this class so that we can match them to the variables
            // used on the super class.
            TypeVariable<?>[] clazzParams = clazz.getTypeParameters();
            if (realTypes.length != clazzParams.length) {
              throw new RuntimeException("Type array mismatch");
            }

            // Replace the variable parameter with the real type.
            boolean foundReplacement = false;
            for (int j = 0; j < clazzParams.length; ++j) {
              if (superArg == clazzParams[j]) {
                Type realType = realTypes[j];
                superArgs[i] = realType;
                foundReplacement = true;
                break;
              }
            }
            if (!foundReplacement) {
              throw new RuntimeException("Unable to find replacement for " + superArg);
            }
          }
        }

        Class<?> parent = (Class<?>) parameterizedType.getRawType();

        realTypes = superArgs;
        clazz = parent;
        continue;
      }

      // None of the parameterized types inherit List. Just continue up the inheritance hierarchy
      // toward the List interface until we can identify the parameters.
      realTypes = EMPTY_TYPES;
      for (Class<?> iface : clazz.getInterfaces()) {
        if (List.class.isAssignableFrom(iface)) {
          clazz = iface;
          continue top;
        }
      }
      clazz = clazz.getSuperclass();
    }

    if (realTypes.length != 1) {
      throw new RuntimeException("Unable to identify parameter type for List<T>");
    }
    return realTypes[0];
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

    /** @return the isList */
    public boolean isList() {
      return isList;
    }
  }
}
