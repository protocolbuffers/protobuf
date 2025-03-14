#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf
{
    /// <summary>
    /// Represents a non-generic extension definition.
    /// </summary>
    /// <remarks>
    /// Most users will not use this abstract class directly, instead using the generated instances
    /// of the concrete subclasses.
    /// </remarks>
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

        internal TValue DefaultValue => codec != null ? codec.DefaultValue : default;

        internal override Type TargetType => typeof(TTarget);

        internal override bool IsRepeated => false;

        internal override IExtensionValue CreateValue()
        {
            return new ExtensionValue<TValue>(codec);
        }
    }

    /// <summary>
    /// Represents a type-safe extension identifier used for getting repeated extension values in <see cref="IExtendableMessage{T}"/> instances.
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
