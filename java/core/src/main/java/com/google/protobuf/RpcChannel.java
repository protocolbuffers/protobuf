// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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
