// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Blocking equivalent to {@link Service}.
 *
 * @author kenton@google.com Kenton Varda
 * @author cpovirk@google.com Chris Povirk
 */
public interface BlockingService {
  /** Equivalent to {@link Service#getDescriptorForType}. */
  Descriptors.ServiceDescriptor getDescriptorForType();

  /**
   * Equivalent to {@link Service#callMethod}, except that {@code callBlockingMethod()} returns the
   * result of the RPC or throws a {@link ServiceException} if there is a failure, rather than
   * passing the information to a callback.
   */
  Message callBlockingMethod(
      Descriptors.MethodDescriptor method, RpcController controller, Message request)
      throws ServiceException;

  /** Equivalent to {@link Service#getRequestPrototype}. */
  Message getRequestPrototype(Descriptors.MethodDescriptor method);

  /** Equivalent to {@link Service#getResponsePrototype}. */
  Message getResponsePrototype(Descriptors.MethodDescriptor method);
}
