// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

package com.google.protobuf;

import com.google.protobuf.Descriptors.FieldDescriptor;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

/**
 * Thrown when attempting to build a protocol message that is missing required
 * fields.  This is a {@code RuntimeException} because it normally represents
 * a programming error:  it happens when some code which constructs a message
 * fails to set all the fields.  {@code parseFrom()} methods <b>do not</b>
 * throw this; they throw an {@link InvalidProtocolBufferException} if
 * required fields are missing, because it is not a programming error to
 * receive an incomplete message.  In other words,
 * {@code UninitializedMessageException} should never be thrown by correct
 * code, but {@code InvalidProtocolBufferException} might be.
 *
 * @author kenton@google.com Kenton Varda
 */
public class UninitializedMessageException extends RuntimeException {
  public UninitializedMessageException(Message message) {
    this(findMissingFields(message));
  }

  private UninitializedMessageException(List<String> missingFields) {
    super(buildDescription(missingFields));
    this.missingFields = missingFields;
  }

  private final List<String> missingFields;

  /**
   * Get a list of human-readable names of required fields missing from this
   * message.  Each name is a full path to a field, e.g. "foo.bar[5].baz".
   */
  public List<String> getMissingFields() {
    return Collections.unmodifiableList(missingFields);
  }

  /**
   * Converts this exception to an {@link InvalidProtocolBufferException}.
   * When a parsed message is missing required fields, this should be thrown
   * instead of {@code UninitializedMessageException}.
   */
  public InvalidProtocolBufferException asInvalidProtocolBufferException() {
    return new InvalidProtocolBufferException(getMessage());
  }

  /** Construct the description string for this exception. */
  private static String buildDescription(List<String> missingFields) {
    StringBuilder description =
      new StringBuilder("Message missing required fields: ");
    boolean first = true;
    for (String field : missingFields) {
      if (first) {
        first = false;
      } else {
        description.append(", ");
      }
      description.append(field);
    }
    return description.toString();
  }

  /**
   * Populates {@code this.missingFields} with the full "path" of each
   * missing required field in the given message.
   */
  private static List<String> findMissingFields(Message message) {
    List<String> results = new ArrayList<String>();
    findMissingFields(message, "", results);
    return results;
  }

  /** Recursive helper implementing {@link #findMissingFields(Message)}. */
  private static void findMissingFields(Message message, String prefix,
                                        List<String> results) {
    for (FieldDescriptor field : message.getDescriptorForType().getFields()) {
      if (field.isRequired() && !message.hasField(field)) {
        results.add(prefix + field.getName());
      }
    }

    for (Map.Entry<FieldDescriptor, Object> entry :
         message.getAllFields().entrySet()) {
      FieldDescriptor field = entry.getKey();
      Object value = entry.getValue();

      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        if (field.isRepeated()) {
          int i = 0;
          for (Object element : (List) value) {
            findMissingFields((Message) element,
                              subMessagePrefix(prefix, field, i++),
                              results);
          }
        } else {
          if (message.hasField(field)) {
            findMissingFields((Message) value,
                              subMessagePrefix(prefix, field, -1),
                              results);
          }
        }
      }
    }
  }

  private static String subMessagePrefix(String prefix,
                                         FieldDescriptor field,
                                         int index) {
    StringBuilder result = new StringBuilder(prefix);
    if (field.isExtension()) {
      result.append('(')
            .append(field.getFullName())
            .append(')');
    } else {
      result.append(field.getName());
    }
    if (index != -1) {
      result.append('[')
            .append(index)
            .append(']');
    }
    result.append('.');
    return result.toString();
  }
}
