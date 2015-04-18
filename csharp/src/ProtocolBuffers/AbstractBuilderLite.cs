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
using System.IO;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Implementation of the non-generic IMessage interface as far as possible.
    /// </summary>
    public abstract partial class AbstractBuilderLite<TMessage, TBuilder> : IBuilderLite<TMessage, TBuilder>
        where TMessage : AbstractMessageLite<TMessage, TBuilder>
        where TBuilder : AbstractBuilderLite<TMessage, TBuilder>
    {
        protected abstract TBuilder ThisBuilder { get; }

        public abstract bool IsInitialized { get; }

        public abstract TBuilder Clear();

        public abstract TBuilder Clone();

        public abstract TMessage Build();

        public abstract TMessage BuildPartial();

        public abstract TBuilder MergeFrom(IMessageLite other);

        public abstract TBuilder MergeFrom(ICodedInputStream input, ExtensionRegistry extensionRegistry);

        public abstract TMessage DefaultInstanceForType { get; }

        #region IBuilderLite<TMessage,TBuilder> Members

        public virtual TBuilder MergeFrom(ICodedInputStream input)
        {
            return MergeFrom(input, ExtensionRegistry.CreateInstance());
        }

        public TBuilder MergeDelimitedFrom(Stream input)
        {
            return MergeDelimitedFrom(input, ExtensionRegistry.CreateInstance());
        }

        public TBuilder MergeDelimitedFrom(Stream input, ExtensionRegistry extensionRegistry)
        {
            int size = (int) CodedInputStream.ReadRawVarint32(input);
            Stream limitedStream = new LimitedInputStream(input, size);
            return MergeFrom(limitedStream, extensionRegistry);
        }

        public TBuilder MergeFrom(ByteString data)
        {
            return MergeFrom(data, ExtensionRegistry.CreateInstance());
        }

        public TBuilder MergeFrom(ByteString data, ExtensionRegistry extensionRegistry)
        {
            CodedInputStream input = data.CreateCodedInput();
            MergeFrom(input, extensionRegistry);
            input.CheckLastTagWas(0);
            return ThisBuilder;
        }

        public TBuilder MergeFrom(byte[] data)
        {
            CodedInputStream input = CodedInputStream.CreateInstance(data);
            MergeFrom(input);
            input.CheckLastTagWas(0);
            return ThisBuilder;
        }

        public TBuilder MergeFrom(byte[] data, ExtensionRegistry extensionRegistry)
        {
            CodedInputStream input = CodedInputStream.CreateInstance(data);
            MergeFrom(input, extensionRegistry);
            input.CheckLastTagWas(0);
            return ThisBuilder;
        }

        public TBuilder MergeFrom(Stream input)
        {
            CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
            MergeFrom(codedInput);
            codedInput.CheckLastTagWas(0);
            return ThisBuilder;
        }

        public TBuilder MergeFrom(Stream input, ExtensionRegistry extensionRegistry)
        {
            CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
            MergeFrom(codedInput, extensionRegistry);
            codedInput.CheckLastTagWas(0);
            return ThisBuilder;
        }

        #endregion

        #region Explicit definitions

        IBuilderLite IBuilderLite.WeakClear()
        {
            return Clear();
        }

        IBuilderLite IBuilderLite.WeakMergeFrom(IMessageLite message)
        {
            return MergeFrom(message);
        }

        IBuilderLite IBuilderLite.WeakMergeFrom(ByteString data)
        {
            return MergeFrom(data);
        }

        IBuilderLite IBuilderLite.WeakMergeFrom(ByteString data, ExtensionRegistry registry)
        {
            return MergeFrom(data, registry);
        }

        IBuilderLite IBuilderLite.WeakMergeFrom(ICodedInputStream input)
        {
            return MergeFrom(input);
        }

        IBuilderLite IBuilderLite.WeakMergeFrom(ICodedInputStream input, ExtensionRegistry registry)
        {
            return MergeFrom(input, registry);
        }

        IMessageLite IBuilderLite.WeakBuild()
        {
            return Build();
        }

        IMessageLite IBuilderLite.WeakBuildPartial()
        {
            return BuildPartial();
        }

        IBuilderLite IBuilderLite.WeakClone()
        {
            return Clone();
        }

        IMessageLite IBuilderLite.WeakDefaultInstanceForType
        {
            get { return DefaultInstanceForType; }
        }

        #endregion

        #region LimitedInputStream

        /// <summary>
        /// Stream implementation which proxies another stream, only allowing a certain amount
        /// of data to be read. Note that this is only used to read delimited streams, so it
        /// doesn't attempt to implement everything.
        /// </summary>
        private class LimitedInputStream : Stream
        {
            private readonly Stream proxied;
            private int bytesLeft;

            internal LimitedInputStream(Stream proxied, int size)
            {
                this.proxied = proxied;
                bytesLeft = size;
            }

            public override bool CanRead
            {
                get { return true; }
            }

            public override bool CanSeek
            {
                get { return false; }
            }

            public override bool CanWrite
            {
                get { return false; }
            }

            public override void Flush()
            {
            }

            public override long Length
            {
                get { throw new NotSupportedException(); }
            }

            public override long Position
            {
                get { throw new NotSupportedException(); }
                set { throw new NotSupportedException(); }
            }

            public override int Read(byte[] buffer, int offset, int count)
            {
                if (bytesLeft > 0)
                {
                    int bytesRead = proxied.Read(buffer, offset, Math.Min(bytesLeft, count));
                    bytesLeft -= bytesRead;
                    return bytesRead;
                }
                return 0;
            }

            public override long Seek(long offset, SeekOrigin origin)
            {
                throw new NotSupportedException();
            }

            public override void SetLength(long value)
            {
                throw new NotSupportedException();
            }

            public override void Write(byte[] buffer, int offset, int count)
            {
                throw new NotSupportedException();
            }
        }

        #endregion
    }
}