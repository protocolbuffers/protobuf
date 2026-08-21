// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Abstract interface for an RPC channel. An {@code RpcChannel} represents a communication line to a
 * {@link Service} which can be used to call that {@link Service}'s methods. The {@link Service} may
 * be running on another machine. Normally, you should not call an {@code RpcChannel} directly, but
 * instead construct a stub {@link Service} wrapping it. Example:
 *
 * <pre>
 *   RpcChannel channel = rpcImpl.newChannel("remotehost.example.com:1234");
 *   RpcController controller = rpcImpl.newController();
 *   MyService service = MyService.newStub(channel);
 *   service.myMethod(controller, request, callback);
 * </pre>
 *
 * <p>Starting with version 2.3.0, RPC implementations should not try to build on this, but should
 * instead provide code generator plugins which generate code specific to the particular RPC
 * implementation. This way the generated code can be more appropriate for the implementation in use
 * and can avoid unnecessary layers of indirection.
 *
 * @author kenton@google.com Kenton Varda
 */
public interface RpcChannel {
  /**
   * Call the given method of the remote service. This method is similar to {@code
   * Service.callMethod()} with one important difference: the caller decides the types of the {@code
   * Message} objects, not the callee. The request may be of any type as long as {@code
   * request.getDescriptor() == method.getInputType()}. The response passed to the callback will be
   * of the same type as {@code responsePrototype} (which must have {@code getDescriptor() ==
   * method.getOutputType()}).
   */
  void callMethod(
      Descriptors.MethodDescriptor method,
      RpcController controller,
      Message request,
      Message responsePrototype,
      RpcCallback<Message> done);
}
