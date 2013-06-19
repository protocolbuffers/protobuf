// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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

package com.google.protobuf.nano;

import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.List;

/**
 * Represents an extension.
 *
 * @author bduff@google.com (Brian Duff)
 * @param <T> the type of the extension.
 */
public class Extension<T> {
  public final int fieldNumber;
  public boolean isRepeatedField;
  public Class<T> fieldType;
  public Class<T> listType;

  private Extension(int fieldNumber, TypeLiteral<T> type) {
    this.fieldNumber = fieldNumber;
    isRepeatedField = type.isList();
    fieldType = type.getTargetClass();
    listType = isRepeatedField ? type.getListType() : null;
  }

  /**
   * Creates a new instance of {@code Extension} for the specified {@code fieldNumber} and
   * {@code type}.
   */
  public static <T> Extension<T> create(int fieldNumber, TypeLiteral<T> type) {
    return new Extension<T>(fieldNumber, type);
  }

  /**
   * Creates a new instance of {@code Extension} for the specified {@code fieldNumber} and
   * {@code type}. This version is used for repeated fields.
   */
  public static <T> Extension<List<T>> createRepeated(int fieldNumber, TypeLiteral<List<T>> type) {
    return new Extension<List<T>>(fieldNumber, type);
  }

  /**
   * Represents a generic type literal. We can't typesafely reference a
   * Class&lt;List&lt;Foo>>.class in Java, so we use this instead.
   * See: http://gafter.blogspot.com/2006/12/super-type-tokens.html
   *
   * <p>Somewhat specialized because we only ever have a Foo or a List&lt;Foo>.
   */
  public static abstract class TypeLiteral<T> {
    private final Type type;

    protected TypeLiteral() {
      Type superclass = getClass().getGenericSuperclass();
      if (superclass instanceof Class) {
        throw new RuntimeException("Missing type parameter");
      }
      this.type = ((ParameterizedType) superclass).getActualTypeArguments()[0];
    }

    /**
     * If the generic type is a list, returns {@code true}.
     */
    private boolean isList() {
      return type instanceof ParameterizedType;
    }

    @SuppressWarnings("unchecked")
    private Class<T> getListType() {
      return (Class<T>) ((ParameterizedType) type).getRawType();
    }

    /**
     * If the generic type is a list, returns the type of element in the list. Otherwise,
     * returns the actual type.
     */
    @SuppressWarnings("unchecked")
    private Class<T> getTargetClass() {
      if (isList()) {
        return (Class<T>) ((ParameterizedType) type).getActualTypeArguments()[0];
      }
      return (Class<T>) type;
    }
  }
}
