// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Base interface for methods common to {@link MessageLite} and {@link MessageLite.Builder} to
 * provide type equivalency.
 *
 * @author jonp@google.com (Jon Perlow)
 */
@CheckReturnValue
public interface MessageLiteOrBuilder {
  /**
   * Get an instance of the type with no fields set. Because no fields are set, all getters for
   * singular fields will return default values and repeated fields will appear empty. This may or
   * may not be a singleton. This differs from the {@code getDefaultInstance()} method of generated
   * message classes in that this method is an abstract method of the {@code MessageLite} interface
   * whereas {@code getDefaultInstance()} is a static method of a specific class. They return the
   * same thing.
   */
  MessageLite getDefaultInstanceForType();

  /**
   * Returns true if all required fields in the message and all embedded messages are set, false
   * otherwise.
   *
   * <p>See also: {@link MessageOrBuilder#getInitializationErrorString()}
   */
  boolean isInitialized();
}
