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
using System.Runtime.Serialization;

/*
 * This entire source file is not supported on the Silverlight platform
 */
#if !SILVERLIGHT
namespace Google.ProtocolBuffers
{
    /* 
     * Specialized handing of *all* message types.  Messages are serialized into a byte[] and stored
     * into the SerializationInfo, and are then reconstituted by an IObjectReference class after
     * deserialization.  IDeserializationCallback is supported on both the Builder and Message.
     */
    [Serializable]
    partial class AbstractMessageLite<TMessage, TBuilder> : ISerializable
    {
        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.SetType(typeof(SerializationSurrogate));
            info.AddValue("message", ToByteArray());
            info.AddValue("initialized", IsInitialized);
        }

        [Serializable]
        private sealed class SerializationSurrogate : IObjectReference, ISerializable
        {
            static readonly TBuilder TemplateInstance = (TBuilder)Activator.CreateInstance(typeof(TBuilder));
            private readonly byte[] _message;
            private readonly bool _initialized;

            private SerializationSurrogate(SerializationInfo info, StreamingContext context)
            {
                _message = (byte[])info.GetValue("message", typeof(byte[]));
                _initialized = info.GetBoolean("initialized");
            }

            object IObjectReference.GetRealObject(StreamingContext context)
            {
                ExtensionRegistry registry = context.Context as ExtensionRegistry;
                TBuilder builder = TemplateInstance.DefaultInstanceForType.CreateBuilderForType();
                builder.MergeFrom(_message, registry ?? ExtensionRegistry.Empty);

                IDeserializationCallback callback = builder as IDeserializationCallback;
                if(callback != null)
                {
                    callback.OnDeserialization(context);
                }

                TMessage message = _initialized ? builder.Build() : builder.BuildPartial();
                callback = message as IDeserializationCallback;
                if (callback != null)
                {
                    callback.OnDeserialization(context);
                }

                return message;
            }

            void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
            {
                info.AddValue("message", _message);
            }
        }
    }

    [Serializable]
    partial class AbstractBuilderLite<TMessage, TBuilder> : ISerializable
    {
        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.SetType(typeof(SerializationSurrogate));
            info.AddValue("message", Clone().BuildPartial().ToByteArray());
        }

        [Serializable]
        private sealed class SerializationSurrogate : IObjectReference, ISerializable
        {
            static readonly TBuilder TemplateInstance = (TBuilder)Activator.CreateInstance(typeof(TBuilder));
            private readonly byte[] _message;

            private SerializationSurrogate(SerializationInfo info, StreamingContext context)
            {
                _message = (byte[])info.GetValue("message", typeof(byte[]));
            }

            object IObjectReference.GetRealObject(StreamingContext context)
            {
                ExtensionRegistry registry = context.Context as ExtensionRegistry;
                TBuilder builder = TemplateInstance.DefaultInstanceForType.CreateBuilderForType();
                builder.MergeFrom(_message, registry ?? ExtensionRegistry.Empty);

                IDeserializationCallback callback = builder as IDeserializationCallback;
                if(callback != null)
                {
                    callback.OnDeserialization(context);
                }

                return builder;
            }

            void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
            {
                info.AddValue("message", _message);
            }
        }
    }

    /*
     * Spread some attribute love around, keeping this all here so we don't use conditional compliation 
     * in every one of these classes.  If we introduce a new platform that also does not support this
     * we can control it all from this source file.
     */

    [Serializable]
    partial class GeneratedMessageLite<TMessage, TBuilder> { }

    [Serializable]
    partial class ExtendableMessageLite<TMessage, TBuilder> { }

    [Serializable]
    partial class AbstractMessage<TMessage, TBuilder> { }

    [Serializable]
    partial class GeneratedMessage<TMessage, TBuilder> { }

    [Serializable]
    partial class ExtendableMessage<TMessage, TBuilder> { }

    [Serializable]
    partial class GeneratedBuilderLite<TMessage, TBuilder> { }

    [Serializable]
    partial class ExtendableBuilderLite<TMessage, TBuilder> { }

    [Serializable]
    partial class AbstractBuilder<TMessage, TBuilder> { }

    [Serializable]
    partial class GeneratedBuilder<TMessage, TBuilder> { }

    [Serializable]
    partial class ExtendableBuilder<TMessage, TBuilder> { }

    [Serializable]
    partial class DynamicMessage 
    {
        [Serializable]
        partial class Builder { }
    }
}
#endif