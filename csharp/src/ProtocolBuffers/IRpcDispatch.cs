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

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Provides an entry-point for transport listeners to call a specified method on a service
    /// </summary>
    public interface IRpcServerStub : IDisposable
    {
        /// <summary>
        /// Calls the method identified by methodName and returns the message
        /// </summary>
        /// <param name="methodName">The method name on the service descriptor (case-sensitive)</param>
        /// <param name="input">The ICodedInputStream to deserialize the call parameter from</param>
        /// <param name="registry">The extension registry to use when deserializing the call parameter</param>
        /// <returns>The message that was returned from the service's method</returns>
        IMessageLite CallMethod(string methodName, ICodedInputStream input, ExtensionRegistry registry);
    }

    /// <summary>
    /// Used to forward an invocation of a service method to a transport sender implementation
    /// </summary>
    public interface IRpcDispatch
    {
        /// <summary>
        /// Calls the service member on the endpoint connected.  This is generally done by serializing
        /// the message, sending the bytes over a transport, and then deserializing the call parameter
        /// to invoke the service's actual implementation via IRpcServerStub.  Once the call has
        /// completed the result message is serialized and returned to the originating endpoint.
        /// </summary>
        /// <typeparam name="TMessage">The type of the response message</typeparam>
        /// <typeparam name="TBuilder">The type of of the response builder</typeparam>
        /// <param name="method">The name of the method on the service</param>
        /// <param name="request">The message instance provided to the service call</param>
        /// <param name="response">The builder used to deserialize the response</param>
        /// <returns>The resulting message of the service call</returns>
        TMessage CallMethod<TMessage, TBuilder>(string method, IMessageLite request,
                                                IBuilderLite<TMessage, TBuilder> response)
            where TMessage : IMessageLite<TMessage, TBuilder>
            where TBuilder : IBuilderLite<TMessage, TBuilder>;
    }
}