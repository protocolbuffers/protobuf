// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.Collections;
import java.util.List;

/**
 * Thrown when attempting to build a protocol message that is missing required fields. This is a
 * {@code RuntimeException} because it normally represents a programming error: it happens when some
 * code which constructs a message fails to set all the fields. {@code parseFrom()} methods <b>do
 * not</b> throw this; they throw an {@link InvalidProtocolBufferException} if required fields are
 * missing, because it is not a programming error to receive an incomplete message. In other words,
 * {@code UninitializedMessageException} should never be thrown by correct code, but {@code
 * InvalidProtocolBufferException} might be.
 *
 * @author kenton@google.com Kenton Varda
 */
public class UninitializedMessageException extends RuntimeException {
  private static final long serialVersionUID = -7466929953374883507L;

  public UninitializedMessageException(final MessageLite message) {
    super(
        "Message was missing required fields.  (Lite runtime could not "
            + "determine which fields were missing).");
    missingFields = null;
  }

  public UninitializedMessageException(final List<String> missingFields) {
    super(buildDescription(missingFields));
    this.missingFields = missingFields;
  }

  private final List<String> missingFields;

  /**
   * Get a list of human-readable names of required fields missing from this message. Each name is a
   * full path to a field, e.g. "foo.bar[5].baz". Returns null if the lite runtime was used, since
   * it lacks the ability to find missing fields.
   */
  public List<String> getMissingFields() {
    return Collections.unmodifiableList(missingFields);
  }

  /**
   * Converts this exception to an {@link InvalidProtocolBufferException}. When a parsed message is
   * missing required fields, this should be thrown instead of {@code
   * UninitializedMessageException}.
   */
  public InvalidProtocolBufferException asInvalidProtocolBufferException() {
    return new InvalidProtocolBufferException(getMessage());
  }

  /** Construct the description string for this exception. */
  private static String buildDescription(final List<String> missingFields) {
    final StringBuilder description = new StringBuilder("Message missing required fields: ");
    boolean first = true;
    for (final String field : missingFields) {
      if (first) {
        first = false;
      } else {
        description.append(", ");
      }
      description.append(field);
    }
    return description.toString();
  }
}
