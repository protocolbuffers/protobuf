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
using System.Collections.Generic;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// All generated protocol message builder classes extend this class. It implements
    /// most of the IBuilder interface using reflection. Users can ignore this class
    /// as an implementation detail.
    /// </summary>
    public abstract partial class GeneratedBuilderLite<TMessage, TBuilder> : AbstractBuilderLite<TMessage, TBuilder>
        where TMessage : GeneratedMessageLite<TMessage, TBuilder>
        where TBuilder : GeneratedBuilderLite<TMessage, TBuilder>
    {
        /// <summary>
        /// Returns the message being built at the moment.
        /// </summary>
        protected abstract TMessage MessageBeingBuilt { get; }

        public override TBuilder MergeFrom(IMessageLite other)
        {
            //do nothing, Lite runtime does not support cross-message merges
            return ThisBuilder;
        }

        public abstract TBuilder MergeFrom(TMessage other);

        /// <summary>
        /// Called by derived classes to parse an unknown field.
        /// </summary>
        /// <returns>true unless the tag is an end-group tag</returns>
        [CLSCompliant(false)]
        protected virtual bool ParseUnknownField(ICodedInputStream input,
                                                 ExtensionRegistry extensionRegistry, uint tag, string fieldName)
        {
            return input.SkipField();
        }

        /// <summary>
        /// Like Build(), but will wrap UninitializedMessageException in
        /// InvalidProtocolBufferException.
        /// </summary>
        public TMessage BuildParsed()
        {
            if (!IsInitialized)
            {
                throw new UninitializedMessageException(MessageBeingBuilt).AsInvalidProtocolBufferException();
            }
            return BuildPartial();
        }

        /// <summary>
        /// Implementation of <see cref="IBuilder{TMessage, TBuilder}.Build" />.
        /// </summary>
        public override TMessage Build()
        {
            // If the message is null, we'll throw a more appropriate exception in BuildPartial.
            if (!IsInitialized)
            {
                throw new UninitializedMessageException(MessageBeingBuilt);
            }
            return BuildPartial();
        }
    }
}