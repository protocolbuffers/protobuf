// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/** Enum that identifies the Java types required to store protobuf fields. */
@ExperimentalApi
public enum JavaType {
  VOID(Void.class, Void.class, null),
  INT(int.class, Integer.class, 0),
  LONG(long.class, Long.class, 0L),
  FLOAT(float.class, Float.class, 0F),
  DOUBLE(double.class, Double.class, 0D),
  BOOLEAN(boolean.class, Boolean.class, false),
  STRING(String.class, String.class, ""),
  BYTE_STRING(ByteString.class, ByteString.class, ByteString.EMPTY),
  ENUM(int.class, Integer.class, null),
  MESSAGE(Object.class, Object.class, null);

  private final Class<?> type;
  private final Class<?> boxedType;
  private final Object defaultDefault;

  JavaType(Class<?> type, Class<?> boxedType, Object defaultDefault) {
    this.type = type;
    this.boxedType = boxedType;
    this.defaultDefault = defaultDefault;
  }

  /** The default default value for fields of this type, if it's a primitive type. */
  public Object getDefaultDefault() {
    return defaultDefault;
  }

  /** Gets the required type for a field that would hold a value of this type. */
  public Class<?> getType() {
    return type;
  }

  /** @return the boxedType */
  public Class<?> getBoxedType() {
    return boxedType;
  }

  /** Indicates whether or not this {@link JavaType} can be applied to a field of the given type. */
  public boolean isValidType(Class<?> t) {
    return type.isAssignableFrom(t);
  }
}
