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
using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Base interface for protocol-buffer-based RPC services. Services themselves
  /// are abstract classes (implemented either by servers or as stubs) but they
  /// implement this itnerface. The methods of this interface can be used to call
  /// the methods of the service without knowing its exact type at compile time
  /// (analagous to the IMessage interface).
  /// </summary>
  public interface IService {
    /// <summary>
    /// The ServiceDescriptor describing this service and its methods.
    /// </summary>
    ServiceDescriptor DescriptorForType { get; }

    /// <summary>
    /// Call a method of the service specified by MethodDescriptor.  This is
    /// normally implemented as a simple switch that calls the standard
    /// definitions of the service's methods.
    /// <para>
    /// Preconditions
    /// <list>
    /// <item><c>method.Service == DescriptorForType</c></item>
    /// <item>request is of the exact same class as the object returned by GetRequestPrototype(method)</item>
    /// <item>controller is of the correct type for the RPC implementation being used by this service.
    /// For stubs, the "correct type" depends on the IRpcChannel which the stub is using. Server-side
    /// implementations are expected to accept whatever type of IRpcController the server-side RPC implementation
    /// uses.</item>
    /// </list>
    /// </para>
    /// <para>
    /// Postconditions
    /// <list>
    /// <item><paramref name="done" /> will be called when the method is complete.
    /// This may before CallMethod returns or it may be at some point in the future.</item>
    /// <item>The parameter to <paramref name="done"/> is the response. It will be of the
    /// exact same type as would be returned by <see cref="GetResponsePrototype"/>.</item>
    /// <item>If the RPC failed, the parameter to <paramref name="done"/> will be null.
    /// Further details about the failure can be found by querying <paramref name="controller"/>.</item>
    /// </list>
    /// </para>
    /// </summary>
    void CallMethod(MethodDescriptor method, IRpcController controller,
                    IMessage request, Action<IMessage> done);

    /// <summary>
    /// CallMethod requires that the request passed in is of a particular implementation
    /// of IMessage. This method gets the default instance of this type of a given method.
    /// You can then call WeakCreateBuilderForType to create a builder to build an object which
    /// you can then pass to CallMethod.
    /// </summary>
    IMessage GetRequestPrototype(MethodDescriptor method);

    /// <summary>
    /// Like GetRequestPrototype, but returns a prototype of the response message.
    /// This is generally not needed because the IService implementation contructs
    /// the response message itself, but it may be useful in some cases to know ahead
    /// of time what type of object will be returned.
    /// </summary>
    IMessage GetResponsePrototype(MethodDescriptor method);
  }
}
