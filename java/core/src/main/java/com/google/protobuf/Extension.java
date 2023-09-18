// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

// TODO: Change ContainingType to extend Message
/**
 * Interface that generated extensions implement.
 *
 * @author liujisi@google.com (Jisi Liu)
 */
public abstract class Extension<ContainingType extends MessageLite, Type>
    extends ExtensionLite<ContainingType, Type> {
  // TODO: Add package-private constructor.

  /** {@inheritDoc} Overridden to return {@link Message} instead of {@link MessageLite}. */
  @Override
  public abstract Message getMessageDefaultInstance();

  /** Returns the descriptor of the extension. */
  public abstract Descriptors.FieldDescriptor getDescriptor();

  /** Returns whether or not this extension is a Lite Extension. */
  @Override
  final boolean isLite() {
    return false;
  }

  // All the methods below are extension implementation details.

  /** The API type that the extension is used for. */
  protected enum ExtensionType {
    IMMUTABLE,
    MUTABLE,
    PROTO1,
  }

  protected abstract ExtensionType getExtensionType();

  /** Type of a message extension. */
  public enum MessageType {
    PROTO1,
    PROTO2,
  }

  /**
   * If the extension is a message extension (i.e., getLiteType() == MESSAGE), returns the type of
   * the message, otherwise undefined.
   */
  public MessageType getMessageType() {
    return MessageType.PROTO2;
  }

  protected abstract Object fromReflectionType(Object value);

  protected abstract Object singularFromReflectionType(Object value);

  protected abstract Object toReflectionType(Object value);

  protected abstract Object singularToReflectionType(Object value);
}
