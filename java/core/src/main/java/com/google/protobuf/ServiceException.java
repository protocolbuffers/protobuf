// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Thrown by blocking RPC methods when a failure occurs.
 *
 * @author cpovirk@google.com (Chris Povirk)
 */
public class ServiceException extends Exception {
  private static final long serialVersionUID = -1219262335729891920L;

  public ServiceException(final String message) {
    super(message);
  }

  public ServiceException(final Throwable cause) {
    super(cause);
  }

  public ServiceException(final String message, final Throwable cause) {
    super(message, cause);
  }
}
