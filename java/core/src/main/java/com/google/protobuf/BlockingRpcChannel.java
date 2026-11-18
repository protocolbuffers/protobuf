// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Abstract interface for a blocking RPC channel. {@code BlockingRpcChannel} is the blocking
 * equivalent to {@link RpcChannel}.
 *
 * @author kenton@google.com Kenton Varda
 * @author cpovirk@google.com Chris Povirk
 */
public interface BlockingRpcChannel {
  /**
   * Call the given method of the remote service and blocks until it returns. {@code
   * callBlockingMethod()} is the blocking equivalent to {@link RpcChannel#callMethod}.
   */
  Message callBlockingMethod(
      Descriptors.MethodDescriptor method,
      RpcController controller,
      Message request,
      Message responsePrototype)
      throws ServiceException;
}
