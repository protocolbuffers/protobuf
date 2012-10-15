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
using System.Collections;
using System.Collections.Generic;
using System.IO;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Iterates over data created using a <see cref="MessageStreamWriter{T}" />.
    /// Unlike MessageStreamWriter, this class is not usually constructed directly with
    /// a stream; instead it is provided with a way of opening a stream when iteration
    /// is started. The stream is closed when the iteration is completed or the enumerator
    /// is disposed. (This occurs naturally when using <c>foreach</c>.)
    /// </summary>
    public class MessageStreamIterator<TMessage> : IEnumerable<TMessage>
        where TMessage : IMessage<TMessage>
    {
        private readonly StreamProvider streamProvider;
        private readonly ExtensionRegistry extensionRegistry;
        private readonly int sizeLimit;

        /// <summary>
        /// The default instance of TMessage type used to construct builders while reading
        /// </summary>
        private static readonly TMessage defaultMessageInstance = CreateDefaultInstance();
        /// <summary>
        /// Any exception (within reason) thrown in type ctor is caught and rethrown in the constructor.
        /// This makes life a lot simpler for the caller.
        /// </summary>
        private static Exception typeInitializationException;


        /// <summary>
        /// Vastly simplified the reflection to simply obtain the default instance and use it to construct
        /// the weak builder while simply casting the result.  Ideally this class should have required a 
        /// TBuilder type argument with a new() constraint to construct the initial instance thereby the
        /// reflection could be eliminated.
        /// </summary>
        private static TMessage CreateDefaultInstance()
        {
            try
            {
                return (TMessage)typeof(TMessage)
                                      .GetProperty("DefaultInstance", typeof(TMessage), new Type[0])
                                      .GetValue(null, null);
            }
            catch (Exception e)
            {
                typeInitializationException = e;
                return default(TMessage);
            }
        }

        private static readonly uint ExpectedTag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);

        private MessageStreamIterator(StreamProvider streamProvider, ExtensionRegistry extensionRegistry, int sizeLimit)
        {
            if (ReferenceEquals(defaultMessageInstance, null))
            {
                throw new System.Reflection.TargetInvocationException(typeInitializationException);
            }
            this.streamProvider = streamProvider;
            this.extensionRegistry = extensionRegistry;
            this.sizeLimit = sizeLimit;
        }

        private MessageStreamIterator(StreamProvider streamProvider, ExtensionRegistry extensionRegistry)
            : this(streamProvider, extensionRegistry, CodedInputStream.DefaultSizeLimit)
        {
        }

        /// <summary>
        /// Creates a new instance which uses the same stream provider as this one,
        /// but the specified extension registry.
        /// </summary>
        public MessageStreamIterator<TMessage> WithExtensionRegistry(ExtensionRegistry newRegistry)
        {
            return new MessageStreamIterator<TMessage>(streamProvider, newRegistry, sizeLimit);
        }

        /// <summary>
        /// Creates a new instance which uses the same stream provider and extension registry as this one,
        /// but with the specified size limit. Note that this must be big enough for the largest message
        /// and the tag and size preceding it.
        /// </summary>
        public MessageStreamIterator<TMessage> WithSizeLimit(int newSizeLimit)
        {
            return new MessageStreamIterator<TMessage>(streamProvider, extensionRegistry, newSizeLimit);
        }

#if CLIENTPROFILE
        public static MessageStreamIterator<TMessage> FromFile(string file)
        {
            return new MessageStreamIterator<TMessage>(() => File.OpenRead(file), ExtensionRegistry.Empty);
        }
#endif

        public static MessageStreamIterator<TMessage> FromStreamProvider(StreamProvider streamProvider)
        {
            return new MessageStreamIterator<TMessage>(streamProvider, ExtensionRegistry.Empty);
        }

        public IEnumerator<TMessage> GetEnumerator()
        {
            using (Stream stream = streamProvider())
            {
                CodedInputStream input = CodedInputStream.CreateInstance(stream);
                input.SetSizeLimit(sizeLimit);
                uint tag;
                string name;
                while (input.ReadTag(out tag, out name))
                {
                    if ((tag == 0 && name == "item") || (tag == ExpectedTag))
                    {
                        IBuilder builder = defaultMessageInstance.WeakCreateBuilderForType();
                        input.ReadMessage(builder, extensionRegistry);
                        yield return (TMessage)builder.WeakBuild();
                    }
                    else
                    {
                        throw InvalidProtocolBufferException.InvalidMessageStreamTag();
                    }

                    input.ResetSizeCounter();
                }
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}