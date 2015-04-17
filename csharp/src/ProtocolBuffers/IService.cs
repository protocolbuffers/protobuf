#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
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

#endregion

using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Base interface for protocol-buffer-based RPC services. Services themselves
    /// are abstract classes (implemented either by servers or as stubs) but they
    /// implement this itnerface. The methods of this interface can be used to call
    /// the methods of the service without knowing its exact type at compile time
    /// (analagous to the IMessage interface).
    /// </summary>
    public interface IService
    {
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