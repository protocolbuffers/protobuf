// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/** Enum that identifies the Java types required to store protobuf fields. */
@SuppressWarnings("nullness")
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

  private final Class<?> type_;
  private final Class<?> boxedType_;
  private final Object defaultDefault_;

  JavaType(Class<?> type, Class<?> boxedType, Object defaultDefault) {
    this.type_ = type;
    this.boxedType_ = boxedType;
    this.defaultDefault_ = defaultDefault;
  }

  /** The default default value for fields of this type, if it's a primitive type. */
  public Object getDefaultDefault() {
    return defaultDefault_;
  }

  /** Gets the required type for a field that would hold a value of this type. */
  public Class<?> getType() {
    return type_;
  }

  /**
   * @return the boxedType
   */
  public Class<?> getBoxedType() {
    return boxedType_;
  }
}
