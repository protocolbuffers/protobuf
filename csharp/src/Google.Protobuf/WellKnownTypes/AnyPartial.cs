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
        private const string DefaultPrefix = "type.googleapis.com";

        // This could be moved to MessageDescriptor if we wanted to, but keeping it here means
        // all the Any-specific code is in the same place.
        private static string GetTypeUrl(MessageDescriptor descriptor, string prefix) =>
            prefix.EndsWith("/") ? prefix + descriptor.FullName : prefix + "/" + descriptor.FullName;

        /// <summary>
        /// Retrieves the type name for a type URL, matching the <see cref="DescriptorBase.FullName"/>
        /// of the packed message type.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This is always just the last part of the URL, after the final slash. No validation of 
        /// anything before the trailing slash is performed. If the type URL does not include a slash,
        /// an empty string is returned rather than an exception being thrown; this won't match any types,
        /// and the calling code is probably in a better position to give a meaningful error.
        /// </para>
        /// <para>
        /// There is no handling of fragments or queries  at the moment.
        /// </para>
        /// </remarks>
        /// <param name="typeUrl">The URL to extract the type name from</param>
        /// <returns>The type name</returns>
        public static string GetTypeName(string typeUrl)
        {
            ProtoPreconditions.CheckNotNull(typeUrl, nameof(typeUrl));
            int lastSlash = typeUrl.LastIndexOf('/');
            return lastSlash == -1 ? "" : typeUrl.Substring(lastSlash + 1);
        }

        /// <summary>
        /// Returns a bool indictating whether this Any message is of the target message type
        /// </summary>
        /// <param name="descriptor">The descriptor of the message type</param>
        /// <returns><c>true</c> if the type name matches the descriptor's full name or <c>false</c> otherwise</returns>
        public bool Is(MessageDescriptor descriptor)
        {
            ProtoPreconditions.CheckNotNull(descriptor, nameof(descriptor));
            return GetTypeName(TypeUrl) == descriptor.FullName;
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
            if (GetTypeName(TypeUrl) != target.Descriptor.FullName)
            {
                throw new InvalidProtocolBufferException(
                    $"Full type name for {target.Descriptor.Name} is {target.Descriptor.FullName}; Any message's type url is {TypeUrl}");
            }
            target.MergeFrom(Value);
            return target;
        }

        /// <summary>
        /// Attempts to unpack the content of this Any message into the target message type,
        /// if it matches the type URL within this Any message.
        /// </summary>
        /// <typeparam name="T">The type of message to attempt to unpack the content into.</typeparam>
        /// <returns><c>true</c> if the message was successfully unpacked; <c>false</c> if the type name didn't match</returns>
        public bool TryUnpack<T>(out T result) where T : IMessage, new()
        {
            // Note: deliberately avoid writing anything to result until the end, in case it's being
            // monitored by other threads. (That would be a bug in the calling code, but let's not make it worse.)
            T target = new T();
            if (GetTypeName(TypeUrl) != target.Descriptor.FullName)
            {
                result = default(T); // Can't use null as there's no class constraint, but this always *will* be null in real usage.
                return false;
            }
            target.MergeFrom(Value);
            result = target;
            return true;
        }

        /// <summary>
        /// Packs the specified message into an Any message using a type URL prefix of "type.googleapis.com".
        /// </summary>
        /// <param name="message">The message to pack.</param>
        /// <returns>An Any message with the content and type URL of <paramref name="message"/>.</returns>
        public static Any Pack(IMessage message) => Pack(message, DefaultPrefix);

        /// <summary>
        /// Packs the specified message into an Any message using the specified type URL prefix.
        /// </summary>
        /// <param name="message">The message to pack.</param>
        /// <param name="typeUrlPrefix">The prefix for the type URL.</param>
        /// <returns>An Any message with the content and type URL of <paramref name="message"/>.</returns>
        public static Any Pack(IMessage message, string typeUrlPrefix)
        {
            ProtoPreconditions.CheckNotNull(message, nameof(message));
            ProtoPreconditions.CheckNotNull(typeUrlPrefix, nameof(typeUrlPrefix));
            return new Any
            {
                TypeUrl = GetTypeUrl(message.Descriptor, typeUrlPrefix),
                Value = message.ToByteString()
            };
        }
    }
}
