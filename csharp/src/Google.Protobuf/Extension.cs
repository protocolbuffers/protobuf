#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

using System;

namespace Google.Protobuf
{
    /// <summary>
    /// Represents a non-generic extension definition. This API is experimental and subject to change.
    /// </summary>
    public abstract class Extension
    {
        internal abstract Type TargetType { get; }

        /// <summary>
        /// Internal use. Creates a new extension with the specified field number.
        /// </summary>
        protected Extension(int fieldNumber)
        {
            FieldNumber = fieldNumber;
        }

        internal abstract IExtensionValue CreateValue();

        /// <summary>
        /// Gets the field number of this extension
        /// </summary>
        public int FieldNumber { get; }

        internal abstract bool IsRepeated { get; }
    }

    /// <summary>
    /// Represents a type-safe extension identifier used for getting and setting single extension values in <see cref="IExtendableMessage{T}"/> instances. 
    /// This API is experimental and subject to change.
    /// </summary>
    /// <typeparam name="TTarget">The message type this field applies to</typeparam>
    /// <typeparam name="TValue">The field value type of this extension</typeparam>
    public sealed class Extension<TTarget, TValue> : Extension where TTarget : IExtendableMessage<TTarget>
    {
        private readonly FieldCodec<TValue> codec;

        /// <summary>
        /// Creates a new extension identifier with the specified field number and codec
        /// </summary>
        public Extension(int fieldNumber, FieldCodec<TValue> codec) : base(fieldNumber)
        {
            this.codec = codec;
        }

        internal TValue DefaultValue => codec != null ? codec.DefaultValue : default(TValue);

        internal override Type TargetType => typeof(TTarget);

        internal override bool IsRepeated => false;

        internal override IExtensionValue CreateValue()
        {
            return new ExtensionValue<TValue>(codec);
        }
    }

    /// <summary>
    /// Represents a type-safe extension identifier used for getting repeated extension values in <see cref="IExtendableMessage{T}"/> instances.
    /// This API is experimental and subject to change.
    /// </summary>
    /// <typeparam name="TTarget">The message type this field applies to</typeparam>
    /// <typeparam name="TValue">The repeated field value type of this extension</typeparam>
    public sealed class RepeatedExtension<TTarget, TValue> : Extension where TTarget : IExtendableMessage<TTarget>
    {
        private readonly FieldCodec<TValue> codec;

        /// <summary>
        /// Creates a new repeated extension identifier with the specified field number and codec
        /// </summary>
        public RepeatedExtension(int fieldNumber, FieldCodec<TValue> codec) : base(fieldNumber)
        {
            this.codec = codec;
        }

        internal override Type TargetType => typeof(TTarget);

        internal override bool IsRepeated => true;

        internal override IExtensionValue CreateValue()
        {
            return new RepeatedExtensionValue<TValue>(codec);
        }
    }
}
