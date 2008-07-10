// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
