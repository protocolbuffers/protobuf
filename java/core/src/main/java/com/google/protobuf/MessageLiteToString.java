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
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/** Helps generate {@link String} representations of {@link MessageLite} protos. */
final class MessageLiteToString {

  private static final String LIST_SUFFIX = "List";
  private static final String BUILDER_LIST_SUFFIX = "OrBuilderList";
  private static final String MAP_SUFFIX = "Map";
  private static final String BYTES_SUFFIX = "Bytes";

  /**
   * Returns a {@link String} representation of the {@link MessageLite} object. The first line of
   * the {@code String} representation representation includes a comment string to uniquely identify
   * the object instance. This acts as an indicator that this should not be relied on for
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
    // Build a map of method name to method. We're looking for methods like getFoo(), hasFoo(),
    // getFooList() and getFooMap() which might be useful for building an object's string
    // representation.
    Map<String, Method> nameToNoArgMethod = new HashMap<String, Method>();
    Map<String, Method> nameToMethod = new HashMap<String, Method>();
    Set<String> getters = new TreeSet<String>();
    for (Method method : messageLite.getClass().getDeclaredMethods()) {
      nameToMethod.put(method.getName(), method);
      if (method.getParameterTypes().length == 0) {
        nameToNoArgMethod.put(method.getName(), method);

        if (method.getName().startsWith("get")) {
          getters.add(method.getName());
        }
      }
    }

    for (String getter : getters) {
      String suffix = getter.replaceFirst("get", "");
      if (suffix.endsWith(LIST_SUFFIX)
          && !suffix.endsWith(BUILDER_LIST_SUFFIX)
          // Sometimes people have fields named 'list' that aren't repeated.
          && !suffix.equals(LIST_SUFFIX)) {
        String camelCase =
            suffix.substring(0, 1).toLowerCase()
                + suffix.substring(1, suffix.length() - LIST_SUFFIX.length());
        // Try to reflectively get the value and toString() the field as if it were repeated. This
        // only works if the method names have not been proguarded out or renamed.
        Method listMethod = nameToNoArgMethod.get(getter);
        if (listMethod != null && listMethod.getReturnType().equals(List.class)) {
          printField(
              buffer,
              indent,
              camelCaseToSnakeCase(camelCase),
              GeneratedMessageLite.invokeOrDie(listMethod, messageLite));
          continue;
        }
      }
      if (suffix.endsWith(MAP_SUFFIX)
          // Sometimes people have fields named 'map' that aren't maps.
          && !suffix.equals(MAP_SUFFIX)) {
        String camelCase =
            suffix.substring(0, 1).toLowerCase()
                + suffix.substring(1, suffix.length() - MAP_SUFFIX.length());
        // Try to reflectively get the value and toString() the field as if it were a map. This only
        // works if the method names have not been proguarded out or renamed.
        Method mapMethod = nameToNoArgMethod.get(getter);
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
              camelCaseToSnakeCase(camelCase),
              GeneratedMessageLite.invokeOrDie(mapMethod, messageLite));
          continue;
        }
      }

      Method setter = nameToMethod.get("set" + suffix);
      if (setter == null) {
        continue;
      }
      if (suffix.endsWith(BYTES_SUFFIX)
          && nameToNoArgMethod.containsKey(
              "get" + suffix.substring(0, suffix.length() - "Bytes".length()))) {
        // Heuristic to skip bytes based accessors for string fields.
        continue;
      }

      String camelCase = suffix.substring(0, 1).toLowerCase() + suffix.substring(1);

      // Try to reflectively get the value and toString() the field as if it were optional. This
      // only works if the method names have not been proguarded out or renamed.
      Method getMethod = nameToNoArgMethod.get("get" + suffix);
      Method hasMethod = nameToNoArgMethod.get("has" + suffix);
      // TODO(dweis): Fix proto3 semantics.
      if (getMethod != null) {
        Object value = GeneratedMessageLite.invokeOrDie(getMethod, messageLite);
        final boolean hasValue =
            hasMethod == null
                ? !isDefaultValue(value)
                : (Boolean) GeneratedMessageLite.invokeOrDie(hasMethod, messageLite);
        // TODO(dweis): This doesn't stop printing oneof case twice: value and enum style.
        if (hasValue) {
          printField(buffer, indent, camelCaseToSnakeCase(camelCase), value);
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
      return ((Float) o) == 0f;
    }
    if (o instanceof Double) {
      return ((Double) o) == 0d;
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
    if (object instanceof Map<?, ?>) {
      Map<?, ?> map = (Map<?, ?>) object;
      for (Map.Entry<?, ?> entry : map.entrySet()) {
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
      reflectivePrintWithIndent((GeneratedMessageLite<?, ?>) object, buffer, indent + 2);
      buffer.append("\n");
      for (int i = 0; i < indent; i++) {
        buffer.append(' ');
      }
      buffer.append("}");
    } else if (object instanceof Map.Entry<?, ?>) {
      buffer.append(" {");
      Map.Entry<?, ?> entry = (Map.Entry<?, ?>) object;
      printField(buffer, indent + 2, "key", entry.getKey());
      printField(buffer, indent + 2, "value", entry.getValue());
      buffer.append("\n");
      for (int i = 0; i < indent; i++) {
        buffer.append(' ');
      }
      buffer.append("}");
    } else {
      buffer.append(": ").append(object.toString());
    }
  }

  private static final String camelCaseToSnakeCase(String camelCase) {
    StringBuilder builder = new StringBuilder();
    for (int i = 0; i < camelCase.length(); i++) {
      char ch = camelCase.charAt(i);
      if (Character.isUpperCase(ch)) {
        builder.append("_");
      }
      builder.append(Character.toLowerCase(ch));
    }
    return builder.toString();
  }
}
