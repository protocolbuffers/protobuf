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

import java.lang.reflect.Array;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;

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
     * Returns an text representation of a MessageNano suitable for debugging.
     *
     * <p>Employs Java reflection on the given object and recursively prints primitive fields,
     * groups, and messages.</p>
     */
    public static <T extends MessageNano> String print(T message) {
        if (message == null) {
            return "null";
        }

        StringBuffer buf = new StringBuffer();
        try {
            print(message.getClass().getSimpleName(), message.getClass(), message,
                    new StringBuffer(), buf);
        } catch (IllegalAccessException e) {
            return "Error printing proto: " + e.getMessage();
        }
        return buf.toString();
    }

    /**
     * Function that will print the given message/class into the StringBuffer.
     * Meant to be called recursively.
     */
    private static void print(String identifier, Class<?> clazz, Object message,
            StringBuffer indentBuf, StringBuffer buf) throws IllegalAccessException {
        if (MessageNano.class.isAssignableFrom(clazz)) {
            // Nano proto message
            buf.append(indentBuf).append(identifier);

            // If null, just print it and return
            if (message == null) {
                buf.append(": ").append(message).append("\n");
                return;
            }

            indentBuf.append(INDENT);
            buf.append(" <\n");
            for (Field field : clazz.getFields()) {
                // Proto fields are public, non-static variables that do not begin or end with '_'
                int modifiers = field.getModifiers();
                String fieldName = field.getName();
                if ((modifiers & Modifier.PUBLIC) != Modifier.PUBLIC
                        || (modifiers & Modifier.STATIC) == Modifier.STATIC
                        || fieldName.startsWith("_") || fieldName.endsWith("_")) {
                    continue;
                }

                Class <?> fieldType = field.getType();
                Object value = field.get(message);

                if (fieldType.isArray()) {
                    Class<?> arrayType = fieldType.getComponentType();

                    // bytes is special since it's not repeated, but is represented by an array
                    if (arrayType == byte.class) {
                        print(fieldName, fieldType, value, indentBuf, buf);
                    } else {
                        int len = Array.getLength(value);
                        for (int i = 0; i < len; i++) {
                            Object elem = Array.get(value, i);
                            print(fieldName, arrayType, elem, indentBuf, buf);
                        }
                    }
                } else {
                    print(fieldName, fieldType, value, indentBuf, buf);
                }
            }
            indentBuf.delete(indentBuf.length() - INDENT.length(), indentBuf.length());
            buf.append(indentBuf).append(">\n");
        } else {
            // Primitive value
            identifier = deCamelCaseify(identifier);
            buf.append(indentBuf).append(identifier).append(": ");
            if (message instanceof String) {
                String stringMessage = sanitizeString((String) message);
                buf.append("\"").append(stringMessage).append("\"");
            } else {
                buf.append(message);
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
}
