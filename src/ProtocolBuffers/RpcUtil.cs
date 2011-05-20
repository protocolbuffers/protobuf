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
    /// Grab-bag of utility functions useful when dealing with RPCs.
    /// </summary>
    public static class RpcUtil
    {
        /// <summary>
        /// Converts an Action[IMessage] to an Action[T].
        /// </summary>
        public static Action<T> SpecializeCallback<T>(Action<IMessage> action)
            where T : IMessage<T>
        {
            return message => action(message);
        }

        /// <summary>
        /// Converts an Action[T] to an Action[IMessage].
        /// The generalized action will accept any message object which has
        /// the same descriptor, and will convert it to the correct class
        /// before calling the original action. However, if the generalized
        /// callback is given a message with a different descriptor, an
        /// exception will be thrown.
        /// </summary>
        public static Action<IMessage> GeneralizeCallback<TMessage, TBuilder>(Action<TMessage> action,
                                                                              TMessage defaultInstance)
            where TMessage : class, IMessage<TMessage, TBuilder>
            where TBuilder : IBuilder<TMessage, TBuilder>
        {
            return message =>
                       {
                           TMessage castMessage = message as TMessage;
                           if (castMessage == null)
                           {
                               castMessage = defaultInstance.CreateBuilderForType().MergeFrom(message).Build();
                           }
                           action(castMessage);
                       };
        }
    }
}