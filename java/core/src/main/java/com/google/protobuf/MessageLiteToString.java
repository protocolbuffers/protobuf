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

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.TreeMap;

/** Helps generate {@link String} representations of {@link MessageLite} protos. */
final class MessageLiteToString {

  private static final String LIST_SUFFIX = "List";
  private static final String BUILDER_LIST_SUFFIX = "OrBuilderList";
  private static final String MAP_SUFFIX = "Map";
  private static final String BYTES_SUFFIX = "Bytes";
  private static final char[] INDENT_BUFFER = new char[80];

  static {
    Arrays.fill(INDENT_BUFFER, ' ');
  }

  private MessageLiteToString() {
    // Classes which are not intended to be instantiated should be made non-instantiable with a
    // private constructor. This includes utility classes (classes with only static members).
  }

  /**
   * Returns a {@link String} representation of the {@link MessageLite} object. The first line of
   * the {@code String} representation includes a comment string to uniquely identify
   * the object instance. This acts as an indicator that this should not be relied on for
   * comparisons.
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
    // Build a map of method name to method. We're looking for methods like getFoo(), hasFoo(),
    // getFooList() and getFooMap() which might be useful for building an object's string
    // representation.
    Set<String> setters = new HashSet<>();
    Map<String, Method> hazzers = new HashMap<>();
    Map<String, Method> getters = new TreeMap<>();
    for (Method method : messageLite.getClass().getDeclaredMethods()) {
      if (Modifier.isStatic(method.getModifiers())) {
        continue;
      }
      if (method.getName().length() < 3) {
        continue;
      }

      if (method.getName().startsWith("set")) {
        setters.add(method.getName());
        continue;
      }

      if (!Modifier.isPublic(method.getModifiers())) {
        continue;
      }

      if (method.getParameterTypes().length != 0) {
        continue;
      }

      if (method.getName().startsWith("has")) {
        hazzers.put(method.getName(), method);
      } else if (method.getName().startsWith("get")) {
        getters.put(method.getName(), method);
      }
    }

    for (Entry<String, Method> getter : getters.entrySet()) {
      String suffix = getter.getKey().substring(3);
      if (suffix.endsWith(LIST_SUFFIX)
          && !suffix.endsWith(BUILDER_LIST_SUFFIX)
          // Sometimes people have fields named 'list' that aren't repeated.
          && !suffix.equals(LIST_SUFFIX)) {
        // Try to reflectively get the value and toString() the field as if it were repeated. This
        // only works if the method names have not been proguarded out or renamed.
        Method listMethod = getter.getValue();
        if (listMethod != null && listMethod.getReturnType().equals(List.class)) {
          printField(
              buffer,
              indent,
              suffix.substring(0, suffix.length() - LIST_SUFFIX.length()),
              GeneratedMessageLite.invokeOrDie(listMethod, messageLite));
          continue;
        }
      }
      if (suffix.endsWith(MAP_SUFFIX)
          // Sometimes people have fields named 'map' that aren't maps.
          && !suffix.equals(MAP_SUFFIX)) {
        // Try to reflectively get the value and toString() the field as if it were a map. This only
        // works if the method names have not been proguarded out or renamed.
        Method mapMethod = getter.getValue();
        if (mapMethod != null
            && mapMethod.getReturnType().equals(Map.class)
            // Skip the deprecated getter method with no prefix "Map" when the field name ends with
            // "map".
            && !mapMethod.isAnnotationPresent(Deprecated.class)
            // Skip the internal mutable getter method.
            && Modifier.isPublic(mapMethod.getModifiers())) {
          printField(
              buffer,
              indent,
              suffix.substring(0, suffix.length() - MAP_SUFFIX.length()),
              GeneratedMessageLite.invokeOrDie(mapMethod, messageLite));
          continue;
        }
      }

      if (!setters.contains("set" + suffix)) {
        continue;
      }
      if (suffix.endsWith(BYTES_SUFFIX)
          && getters.containsKey("get" + suffix.substring(0, suffix.length() - "Bytes".length()))) {
        // Heuristic to skip bytes based accessors for string fields.
        continue;
      }

      // Try to reflectively get the value and toString() the field as if it were optional. This
      // only works if the method names have not been proguarded out or renamed.
      Method getMethod = getter.getValue();
      Method hasMethod = hazzers.get("has" + suffix);
      // TODO(dweis): Fix proto3 semantics.
      if (getMethod != null) {
        Object value = GeneratedMessageLite.invokeOrDie(getMethod, messageLite);
        final boolean hasValue =
            hasMethod == null
                ? !isDefaultValue(value)
                : (Boolean) GeneratedMessageLite.invokeOrDie(hasMethod, messageLite);
        // TODO(dweis): This doesn't stop printing oneof case twice: value and enum style.
        if (hasValue) {
          printField(buffer, indent, suffix, value);
        }
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

    if (((GeneratedMessageLite<?, ?>) messageLite).unknownFields != null) {
      ((GeneratedMessageLite<?, ?>) messageLite).unknownFields.printWithIndent(buffer, indent);
    }
  }

  private static boolean isDefaultValue(Object o) {
    if (o instanceof Boolean) {
      return !((Boolean) o);
    }
    if (o instanceof Integer) {
      return ((Integer) o) == 0;
    }
    if (o instanceof Float) {
      return Float.floatToRawIntBits((Float) o) == 0;
    }
    if (o instanceof Double) {
      return Double.doubleToRawLongBits((Double) o) == 0;
    }
    if (o instanceof String) {
      return o.equals("");
    }
    if (o instanceof ByteString) {
      return o.equals(ByteString.EMPTY);
    }
    if (o instanceof MessageLite) { // Can happen in oneofs.
      return o == ((MessageLite) o).getDefaultInstanceForType();
    }
    if (o instanceof java.lang.Enum<?>) { // Catches oneof enums.
      return ((java.lang.Enum<?>) o).ordinal() == 0;
    }

    return false;
  }

  /**
   * Formats a text proto field.
   *
   * <p>For use by generated code only.
   *
   * @param buffer the buffer to write to
   * @param indent the number of spaces the proto should be indented by
   * @param name the field name (in PascalCase)
   * @param object the object value of the field
   */
  static void printField(StringBuilder buffer, int indent, String name, Object object) {
    if (object instanceof List<?>) {
      List<?> list = (List<?>) object;
      for (Object entry : list) {
        printField(buffer, indent, name, entry);
      }
      return;
    }
    if (object instanceof Map<?, ?>) {
      Map<?, ?> map = (Map<?, ?>) object;
      for (Map.Entry<?, ?> entry : map.entrySet()) {
        printField(buffer, indent, name, entry);
      }
      return;
    }

    buffer.append('\n');
    indent(indent, buffer);
    buffer.append(pascalCaseToSnakeCase(name));

    if (object instanceof String) {
      buffer.append(": \"").append(TextFormatEscaper.escapeText((String) object)).append('"');
    } else if (object instanceof ByteString) {
      buffer.append(": \"").append(TextFormatEscaper.escapeBytes((ByteString) object)).append('"');
    } else if (object instanceof GeneratedMessageLite) {
      buffer.append(" {");
      reflectivePrintWithIndent((GeneratedMessageLite<?, ?>) object, buffer, indent + 2);
      buffer.append("\n");
      indent(indent, buffer);
      buffer.append("}");
    } else if (object instanceof Map.Entry<?, ?>) {
      buffer.append(" {");
      Map.Entry<?, ?> entry = (Map.Entry<?, ?>) object;
      printField(buffer, indent + 2, "key", entry.getKey());
      printField(buffer, indent + 2, "value", entry.getValue());
      buffer.append("\n");
      indent(indent, buffer);
      buffer.append("}");
    } else {
      buffer.append(": ").append(object);
    }
  }

  private static void indent(int indent, StringBuilder buffer) {
    while (indent > 0) {
      int partialIndent = indent;
      if (partialIndent > INDENT_BUFFER.length) {
        partialIndent = INDENT_BUFFER.length;
      }
      buffer.append(INDENT_BUFFER, 0, partialIndent);
      indent -= partialIndent;
    }
  }

  private static String pascalCaseToSnakeCase(String pascalCase) {
    if (pascalCase.isEmpty()) {
      return pascalCase;
    }

    StringBuilder builder = new StringBuilder();
    builder.append(Character.toLowerCase(pascalCase.charAt(0)));
    for (int i = 1; i < pascalCase.length(); i++) {
      char ch = pascalCase.charAt(i);
      if (Character.isUpperCase(ch)) {
        builder.append("_");
      }
      builder.append(Character.toLowerCase(ch));
    }
    return builder.toString();
  }
}
