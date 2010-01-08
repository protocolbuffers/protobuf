// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
 * Abstract base interface for protocol-buffer-based RPC services.  Services
 * themselves are abstract classes (implemented either by servers or as
 * stubs), but they subclass this base interface.  The methods of this
 * interface can be used to call the methods of the service without knowing
 * its exact type at compile time (analogous to the Message interface).
 *
 * <p>Starting with version 2.3.0, RPC implementations should not try to build
 * on this, but should instead provide code generator plugins which generate
 * code specific to the particular RPC implementation.  This way the generated
 * code can be more appropriate for the implementation in use and can avoid
 * unnecessary layers of indirection.
 *
 * @author kenton@google.com Kenton Varda
 */
public interface Service {
  /**
   * Get the {@code ServiceDescriptor} describing this service and its methods.
   */
  Descriptors.ServiceDescriptor getDescriptorForType();

  /**
   * <p>Call a method of the service specified by MethodDescriptor.  This is
   * normally implemented as a simple {@code switch()} that calls the standard
   * definitions of the service's methods.
   *
   * <p>Preconditions:
   * <ul>
   *   <li>{@code method.getService() == getDescriptorForType()}
   *   <li>{@code request} is of the exact same class as the object returned by
   *       {@code getRequestPrototype(method)}.
   *   <li>{@code controller} is of the correct type for the RPC implementation
   *       being used by this Service.  For stubs, the "correct type" depends
   *       on the RpcChannel which the stub is using.  Server-side Service
   *       implementations are expected to accept whatever type of
   *       {@code RpcController} the server-side RPC implementation uses.
   * </ul>
   *
   * <p>Postconditions:
   * <ul>
   *   <li>{@code done} will be called when the method is complete.  This may be
   *       before {@code callMethod()} returns or it may be at some point in
   *       the future.
   *   <li>The parameter to {@code done} is the response.  It must be of the
   *       exact same type as would be returned by
   *       {@code getResponsePrototype(method)}.
   *   <li>If the RPC failed, the parameter to {@code done} will be
   *       {@code null}.  Further details about the failure can be found by
   *       querying {@code controller}.
   * </ul>
   */
  void callMethod(Descriptors.MethodDescriptor method,
                  RpcController controller,
                  Message request,
                  RpcCallback<Message> done);

  /**
   * <p>{@code callMethod()} requires that the request passed in is of a
   * particular subclass of {@code Message}.  {@code getRequestPrototype()}
   * gets the default instances of this type for a given method.  You can then
   * call {@code Message.newBuilderForType()} on this instance to
   * construct a builder to build an object which you can then pass to
   * {@code callMethod()}.
   *
   * <p>Example:
   * <pre>
   *   MethodDescriptor method =
   *     service.getDescriptorForType().findMethodByName("Foo");
   *   Message request =
   *     stub.getRequestPrototype(method).newBuilderForType()
   *         .mergeFrom(input).build();
   *   service.callMethod(method, request, callback);
   * </pre>
   */
  Message getRequestPrototype(Descriptors.MethodDescriptor method);

  /**
   * Like {@code getRequestPrototype()}, but gets a prototype of the response
   * message.  {@code getResponsePrototype()} is generally not needed because
   * the {@code Service} implementation constructs the response message itself,
   * but it may be useful in some cases to know ahead of time what type of
   * object will be returned.
   */
  Message getResponsePrototype(Descriptors.MethodDescriptor method);
}
