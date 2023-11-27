// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;
import java.util.Map;

@CheckReturnValue
abstract class ExtensionSchema<T extends FieldSet.FieldDescriptorLite<T>> {

  /** Returns true for messages that support extensions. */
  abstract boolean hasExtensions(MessageLite prototype);

  /** Returns the extension {@link FieldSet} for the message instance. */
  abstract FieldSet<T> getExtensions(Object message);

  /** Replaces the extension {@link FieldSet} for the message instance. */
  abstract void setExtensions(Object message, FieldSet<T> extensions);

  /** Returns the extension {@link FieldSet} and ensures it's mutable. */
  abstract FieldSet<T> getMutableExtensions(Object message);

  /** Marks the extension {@link FieldSet} as immutable. */
  abstract void makeImmutable(Object message);

  /**
   * Parses an extension. Returns the passed-in unknownFields parameter if no unknown enum value is
   * found or a modified unknownFields (a new instance if the passed-in unknownFields is null)
   * containing unknown enum values found while parsing.
   *
   * @param <UT> The type used to store unknown fields. It's either UnknownFieldSet in full runtime
   *     or UnknownFieldSetLite in lite runtime.
   */
  abstract <UT, UB> UB parseExtension(
      Object containerMessage,
      Reader reader,
      Object extension,
      ExtensionRegistryLite extensionRegistry,
      FieldSet<T> extensions,
      UB unknownFields,
      UnknownFieldSchema<UT, UB> unknownFieldSchema)
      throws IOException;

  /** Gets the field number of an extension entry. */
  abstract int extensionNumber(Map.Entry<?, ?> extension);

  /** Serializes one extension entry. */
  abstract void serializeExtension(Writer writer, Map.Entry<?, ?> extension) throws IOException;

  /** Finds an extension by field number. */
  abstract Object findExtensionByNumber(
      ExtensionRegistryLite extensionRegistry, MessageLite defaultInstance, int number);

  /** Parses a length-prefixed MessageSet item from the reader. */
  abstract void parseLengthPrefixedMessageSetItem(
      Reader reader,
      Object extension,
      ExtensionRegistryLite extensionRegistry,
      FieldSet<T> extensions)
      throws IOException;

  /**
   * Parses the entire content of a {@link ByteString} as one MessageSet item. Unlike {@link
   * #parseLengthPrefixedMessageSetItem}, there isn't a length-prefix.
   */
  abstract void parseMessageSetItem(
      ByteString data,
      Object extension,
      ExtensionRegistryLite extensionRegistry,
      FieldSet<T> extensions)
      throws IOException;
}
