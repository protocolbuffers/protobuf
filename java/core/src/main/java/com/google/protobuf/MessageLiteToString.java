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
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Helps generate {@link String} representations of {@link MessageLite} protos.
 */
final class MessageLiteToString {
  /**
   * Suffix for *_FIELD_NUMBER fields. This is used to reflectively detect proto fields that should
   * be toString()ed.
   */
  private static final String FIELD_NUMBER_NAME_SUFFIX = "_FIELD_NUMBER";

  /**
   * Returns a {@link String} representation of the {@link MessageLite} object.  The first line of
   * the {@code String} representation representation includes a comment string to uniquely identify
   * the objcet instance. This acts as an indicator that this should not be relied on for
   * comparisons.
   *
   * <p>For use by generated code only.
   */
  static String toString(MessageLite messageLite, String commentString) {
    StringBuilder buffer = new StringBuilder();
    buffer.append("# ").append(commentString);
    reflectivePrintWithIndent(messageLite, buffer, 0);
    return buffer.toString();
  }

  /**
   * Reflectively prints the {@link MessageLite} to the buffer at given {@code indent} level.
   *
   * @param buffer the buffer to write to
   * @param indent the number of spaces to indent the proto by
   */
  private static void reflectivePrintWithIndent(
      MessageLite messageLite, StringBuilder buffer, int indent) {
    // Build a map of method name to method. We're looking for methods like getFoo(), hasFoo(), and
    // getFooList() which might be useful for building an object's string representation.
    Map<String, Method> nameToNoArgMethod = new HashMap<String, Method>();
    for (Method method : messageLite.getClass().getDeclaredMethods()) {
      if (method.getParameterTypes().length == 0) {
        nameToNoArgMethod.put(method.getName(), method);
      }
    }

    for (Field field : messageLite.getClass().getDeclaredFields()) {
      String fieldName = field.getName();
      // Skip all fields that aren't in a format like "FOO_BAR_FIELD_NUMBER"
      if (!fieldName.endsWith(FIELD_NUMBER_NAME_SUFFIX)) {
        continue;
      }

      // For "FOO_BAR_FIELD_NUMBER" his would be "FOO_BAR"
      String upperUnderscore =
          fieldName.substring(0, fieldName.length() - FIELD_NUMBER_NAME_SUFFIX.length());

      // For "FOO_BAR_FIELD_NUMBER" his would be "FooBar"
      String upperCamelCaseName = upperUnderscoreToUpperCamel(upperUnderscore);

      // Try to reflectively get the value and toString() the field as if it were optional. This
      // only works if the method names have not be proguarded out or renamed.
      Method getMethod = nameToNoArgMethod.get("get" + upperCamelCaseName);
      Method hasMethod = nameToNoArgMethod.get("has" + upperCamelCaseName);
      if (getMethod != null && hasMethod != null) {
        if ((Boolean) GeneratedMessageLite.invokeOrDie(hasMethod, messageLite)) {
          printField(
              buffer,
              indent,
              upperUnderscore.toLowerCase(),
              GeneratedMessageLite.invokeOrDie(getMethod, messageLite));
        }
        continue;
      }

      // Try to reflectively get the value and toString() the field as if it were repeated. This
      // only works if the method names have not be proguarded out or renamed.
      Method listMethod = nameToNoArgMethod.get("get" + upperCamelCaseName + "List");
      if (listMethod != null) {
        printField(
            buffer,
            indent,
            upperUnderscore.toLowerCase(),
            GeneratedMessageLite.invokeOrDie(listMethod, messageLite));
        continue;
      }
    }

    if (messageLite instanceof GeneratedMessageLite.ExtendableMessage) {
      Iterator<Map.Entry<GeneratedMessageLite.ExtensionDescriptor, Object>> iter =
          ((GeneratedMessageLite.ExtendableMessage<?, ?>) messageLite).extensions.iterator();
      while (iter.hasNext()) {
        Map.Entry<GeneratedMessageLite.ExtensionDescriptor, Object> entry = iter.next();
        printField(buffer, indent, "[" + entry.getKey().getNumber() + "]", entry.getValue());
      }
    }

    if (((GeneratedMessageLite) messageLite).unknownFields != null) {
      ((GeneratedMessageLite) messageLite).unknownFields.printWithIndent(buffer, indent);
    }
  }

  /**
   * Formats a text proto field.
   *
   * <p>For use by generated code only.
   *
   * @param buffer the buffer to write to
   * @param indent the number of spaces the proto should be indented by
   * @param name the field name (in lower underscore case)
   * @param object the object value of the field
   */
  static final void printField(StringBuilder buffer, int indent, String name, Object object) {
    if (object instanceof List<?>) {
      List<?> list = (List<?>) object;
      for (Object entry : list) {
        printField(buffer, indent, name, entry);
      }
      return;
    }

    buffer.append('\n');
    for (int i = 0; i < indent; i++) {
      buffer.append(' ');
    }
    buffer.append(name);

    if (object instanceof String) {
      buffer.append(": \"").append(TextFormatEscaper.escapeText((String) object)).append('"');
    } else if (object instanceof ByteString) {
      buffer.append(": \"").append(TextFormatEscaper.escapeBytes((ByteString) object)).append('"');
    } else if (object instanceof GeneratedMessageLite) {
      buffer.append(" {");
      reflectivePrintWithIndent((GeneratedMessageLite) object, buffer, indent + 2);
      buffer.append("\n");
      for (int i = 0; i < indent; i++) {
        buffer.append(' ');
      }
      buffer.append("}");
    } else {
      buffer.append(": ").append(object.toString());
    }
  }

  /**
   * A Guava-less implementation of:
   * {@code CaseFormat.UPPER_UNDERSCORE.to(CaseFormat.UPPER_CAMEL, upperUnderscore)}
   */
  private static String upperUnderscoreToUpperCamel(String upperUnderscore) {
    String upperCamelCaseName = "";
    boolean nextCharacterShouldBeUpper = true;
    for (int i = 0; i < upperUnderscore.length(); i++) {
      char ch = upperUnderscore.charAt(i);
      if (ch == '_') {
        nextCharacterShouldBeUpper = true;
      } else if (nextCharacterShouldBeUpper){
        upperCamelCaseName += Character.toUpperCase(ch);
        nextCharacterShouldBeUpper = false;
      } else {
        upperCamelCaseName += Character.toLowerCase(ch);
      }
    }
    return upperCamelCaseName;
  }
}
