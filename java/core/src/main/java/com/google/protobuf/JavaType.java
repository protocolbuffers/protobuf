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
