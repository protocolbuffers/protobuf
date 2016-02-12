#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
#endregion

using Google.Protobuf.Reflection;

namespace Google.Protobuf.WellKnownTypes
{
    public partial class Any
    {
        // This could be moved to MessageDescriptor if we wanted to, but keeping it here means
        // all the Any-specific code is in the same place.
        private static string GetTypeUrl(MessageDescriptor descriptor)
        {
            return "type.googleapis.com/" + descriptor.FullName;
        }

        /// <summary>
        /// Unpacks the content of this Any message into the target message type,
        /// which must match the type URL within this Any message.
        /// </summary>
        /// <typeparam name="T">The type of message to unpack the content into.</typeparam>
        /// <returns>The unpacked message.</returns>
        /// <exception cref="InvalidProtocolBufferException">The target message type doesn't match the type URL in this message</exception>
        public T Unpack<T>() where T : IMessage, new()
        {
            // Note: this doesn't perform as well is it might. We could take a MessageParser<T> in an alternative overload,
            // which would be expected to perform slightly better... although the difference is likely to be negligible.
            T target = new T();
            string targetTypeUrl = GetTypeUrl(target.Descriptor);
            if (TypeUrl != targetTypeUrl)
            {
                throw new InvalidProtocolBufferException(string.Format("Type url for {0} is {1}; Any message's type url is {2}",
                    target.Descriptor.Name, targetTypeUrl, TypeUrl));
            }
            target.MergeFrom(Value);
            return target;
        }

        /// <summary>
        /// Packs the specified message into an Any message.
        /// </summary>
        /// <param name="message">The message to pack.</param>
        /// <returns>An Any message with the content and type URL of <paramref name="message"/>.</returns>
        public static Any Pack(IMessage message)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            return new Any { TypeUrl = GetTypeUrl(message.Descriptor), Value = message.ToByteString() };
        }
    }
}
