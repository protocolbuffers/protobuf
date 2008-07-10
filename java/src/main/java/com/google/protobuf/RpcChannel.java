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

/**
 * <p>Abstract interface for an RPC channel.  An {@code RpcChannel} represents a
 * communication line to a {@link Service} which can be used to call that
 * {@link Service}'s methods.  The {@link Service} may be running on another
 * machine.  Normally, you should not call an {@code RpcChannel} directly, but
 * instead construct a stub {@link Service} wrapping it.  Example:
 *
 * <pre>
 *   RpcChannel channel = rpcImpl.newChannel("remotehost.example.com:1234");
 *   RpcController controller = rpcImpl.newController();
 *   MyService service = MyService.newStub(channel);
 *   service.myMethod(controller, request, callback);
 * </pre>
 *
 * @author kenton@google.com Kenton Varda
 */
public interface RpcChannel {
  /**
   * Call the given method of the remote service.  This method is similar to
   * {@code Service.callMethod()} with one important difference:  the caller
   * decides the types of the {@code Message} objects, not the callee.  The
   * request may be of any type as long as
   * {@code request.getDescriptor() == method.getInputType()}.
   * The response passed to the callback will be of the same type as
   * {@code responsePrototype} (which must have
   * {@code getDescriptor() == method.getOutputType()}).
   */
  void callMethod(Descriptors.MethodDescriptor method,
                  RpcController controller,
                  Message request,
                  Message responsePrototype,
                  RpcCallback<Message> done);
}
