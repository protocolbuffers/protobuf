// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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

package com.google.protobuf.nano;

import java.lang.reflect.Array;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Map;

/**
 * Static helper methods for printing nano protos.
 *
 * @author flynn@google.com Andrew Flynn
 */
public final class MessageNanoPrinter {
    // Do not allow instantiation
    private MessageNanoPrinter() {}

    private static final String INDENT = "  ";
    private static final int MAX_STRING_LEN = 200;

    /**
     * Returns an text representation of a MessageNano suitable for debugging. The returned string
     * is mostly compatible with Protocol Buffer's TextFormat (as provided by non-nano protocol
     * buffers) -- groups (which are deprecated) are output with an underscore name (e.g. foo_bar
     * instead of FooBar) and will thus not parse.
     *
     * <p>Employs Java reflection on the given object and recursively prints primitive fields,
     * groups, and messages.</p>
     */
    public static <T extends MessageNano> String print(T message) {
        if (message == null) {
            return "";
        }

        StringBuffer buf = new StringBuffer();
        try {
            print(null, message, new StringBuffer(), buf);
        } catch (IllegalAccessException e) {
            return "Error printing proto: " + e.getMessage();
        } catch (InvocationTargetException e) {
            return "Error printing proto: " + e.getMessage();
        }
        return buf.toString();
    }

    /**
     * Function that will print the given message/field into the StringBuffer.
     * Meant to be called recursively.
     *
     * @param identifier the identifier to use, or {@code null} if this is the root message to
     *        print.
     * @param object the value to print. May in fact be a primitive value or byte array and not a
     *        message.
     * @param indentBuf the indentation each line should begin with.
     * @param buf the output buffer.
     */
    private static void print(String identifier, Object object,
            StringBuffer indentBuf, StringBuffer buf) throws IllegalAccessException,
            InvocationTargetException {
        if (object == null) {
            // This can happen if...
            //   - we're about to print a message, String, or byte[], but it not present;
            //   - we're about to print a primitive, but "reftype" optional style is enabled, and
            //     the field is unset.
            // In both cases the appropriate behavior is to output nothing.
        } else if (object instanceof MessageNano) {  // Nano proto message
            int origIndentBufLength = indentBuf.length();
            if (identifier != null) {
                buf.append(indentBuf).append(deCamelCaseify(identifier)).append(" <\n");
                indentBuf.append(INDENT);
            }
            Class<?> clazz = object.getClass();

            // Proto fields follow one of two formats:
            //
            // 1) Public, non-static variables that do not begin or end with '_'
            // Find and print these using declared public fields
            for (Field field : clazz.getFields()) {
                int modifiers = field.getModifiers();
                String fieldName = field.getName();
                if ("cachedSize".equals(fieldName)) {
                    // TODO(bduff): perhaps cachedSize should have a more obscure name.
                    continue;
                }

                if ((modifiers & Modifier.PUBLIC) == Modifier.PUBLIC
                        && (modifiers & Modifier.STATIC) != Modifier.STATIC
                        && !fieldName.startsWith("_")
                        && !fieldName.endsWith("_")) {
                    Class<?> fieldType = field.getType();
                    Object value = field.get(object);

                    if (fieldType.isArray()) {
                        Class<?> arrayType = fieldType.getComponentType();

                        // bytes is special since it's not repeated, but is represented by an array
                        if (arrayType == byte.class) {
                            print(fieldName, value, indentBuf, buf);
                        } else {
                            int len = value == null ? 0 : Array.getLength(value);
                            for (int i = 0; i < len; i++) {
                                Object elem = Array.get(value, i);
                                print(fieldName, elem, indentBuf, buf);
                            }
                        }
                    } else {
                        print(fieldName, value, indentBuf, buf);
                    }
                }
            }

            // 2) Fields that are accessed via getter methods (when accessors
            //    mode is turned on)
            // Find and print these using getter methods.
            for (Method method : clazz.getMethods()) {
                String name = method.getName();
                // Check for the setter accessor method since getters and hazzers both have
                // non-proto-field name collisions (hashCode() and getSerializedSize())
                if (name.startsWith("set")) {
                    String subfieldName = name.substring(3);

                    Method hazzer = null;
                    try {
                        hazzer = clazz.getMethod("has" + subfieldName);
                    } catch (NoSuchMethodException e) {
                        continue;
                    }
                    // If hazzer doesn't exist or returns false, no need to continue
                    if (!(Boolean) hazzer.invoke(object)) {
                        continue;
                    }

                    Method getter = null;
                    try {
                        getter = clazz.getMethod("get" + subfieldName);
                    } catch (NoSuchMethodException e) {
                        continue;
                    }

                    print(subfieldName, getter.invoke(object), indentBuf, buf);
                }
            }
            if (identifier != null) {
                indentBuf.setLength(origIndentBufLength);
                buf.append(indentBuf).append(">\n");
            }
        } else if (object instanceof Map) {
          Map<?,?> map = (Map<?,?>) object;
          identifier = deCamelCaseify(identifier);

          for (Map.Entry<?,?> entry : map.entrySet()) {
            buf.append(indentBuf).append(identifier).append(" <\n");
            int origIndentBufLength = indentBuf.length();
            indentBuf.append(INDENT);
            print("key", entry.getKey(), indentBuf, buf);
            print("value", entry.getValue(), indentBuf, buf);
            indentBuf.setLength(origIndentBufLength);
            buf.append(indentBuf).append(">\n");
          }
        } else {
            // Non-null primitive value
            identifier = deCamelCaseify(identifier);
            buf.append(indentBuf).append(identifier).append(": ");
            if (object instanceof String) {
                String stringMessage = sanitizeString((String) object);
                buf.append("\"").append(stringMessage).append("\"");
            } else if (object instanceof byte[]) {
                appendQuotedBytes((byte[]) object, buf);
            } else {
                buf.append(object);
            }
            buf.append("\n");
        }
    }

    /**
     * Converts an identifier of the format "FieldName" into "field_name".
     */
    private static String deCamelCaseify(String identifier) {
        StringBuffer out = new StringBuffer();
        for (int i = 0; i < identifier.length(); i++) {
            char currentChar = identifier.charAt(i);
            if (i == 0) {
                out.append(Character.toLowerCase(currentChar));
            } else if (Character.isUpperCase(currentChar)) {
                out.append('_').append(Character.toLowerCase(currentChar));
            } else {
                out.append(currentChar);
            }
        }
        return out.toString();
    }

    /**
     * Shortens and escapes the given string.
     */
    private static String sanitizeString(String str) {
        if (!str.startsWith("http") && str.length() > MAX_STRING_LEN) {
            // Trim non-URL strings.
            str = str.substring(0, MAX_STRING_LEN) + "[...]";
        }
        return escapeString(str);
    }

    /**
     * Escape everything except for low ASCII code points.
     */
    private static String escapeString(String str) {
        int strLen = str.length();
        StringBuilder b = new StringBuilder(strLen);
        for (int i = 0; i < strLen; i++) {
            char original = str.charAt(i);
            if (original >= ' ' && original <= '~' && original != '"' && original != '\'') {
                b.append(original);
            } else {
                b.append(String.format("\\u%04x", (int) original));
            }
        }
        return b.toString();
    }

    /**
     * Appends a quoted byte array to the provided {@code StringBuffer}.
     */
    private static void appendQuotedBytes(byte[] bytes, StringBuffer builder) {
        if (bytes == null) {
            builder.append("\"\"");
            return;
        }

        builder.append('"');
        for (int i = 0; i < bytes.length; ++i) {
            int ch = bytes[i] & 0xff;
            if (ch == '\\' || ch == '"') {
                builder.append('\\').append((char) ch);
            } else if (ch >= 32 && ch < 127) {
                builder.append((char) ch);
            } else {
                builder.append(String.format("\\%03o", ch));
            }
        }
        builder.append('"');
    }
}
