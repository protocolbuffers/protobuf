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
    /// Interface for an RPC channel. A channel represents a communication line to
    /// a service (IService implementation) which can be used to call that service's
    /// methods. The service may be running on another machine. Normally, you should
    /// not call an IRpcChannel directly, but instead construct a stub wrapping it.
    /// Generated service classes contain a CreateStub method for precisely this purpose.
    /// </summary>
    public interface IRpcChannel
    {
        /// <summary>
        /// Calls the given method of the remote service. This method is similar
        /// to <see cref="IService.CallMethod" /> with one important difference: the
        /// caller decides the types of the IMessage objects, not the implementation.
        /// The request may be of any type as long as <c>request.Descriptor == method.InputType</c>.
        /// The response passed to the callback will be of the same type as
        /// <paramref name="responsePrototype"/> (which must be such that
        /// <c>responsePrototype.Descriptor == method.OutputType</c>).
        /// </summary>
        void CallMethod(MethodDescriptor method, IRpcController controller,
                        IMessage request, IMessage responsePrototype, Action<IMessage> done);
    }
}