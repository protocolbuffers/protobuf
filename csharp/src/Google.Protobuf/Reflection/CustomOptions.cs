#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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

using Google.Protobuf.Collections;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Linq;
using System.Reflection;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Container for a set of custom options specified within a message, field etc.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This type is publicly immutable, but internally mutable. It is only populated
    /// by the descriptor parsing code - by the time any user code is able to see an instance,
    /// it will be fully initialized.
    /// </para>
    /// <para>
    /// If an option is requested using the incorrect method, an answer may still be returned: all
    /// of the numeric types are represented internally using 64-bit integers, for example. It is up to
    /// the caller to ensure that they make the appropriate method call for the option they're interested in.
    /// Note that enum options are simply stored as integers, so the value should be fetched using
    /// <see cref="TryGetInt32(int, out int)"/> and then cast appropriately.
    /// </para>
    /// <para>
    /// Repeated options are currently not supported. Asking for a single value of an option
    /// which was actually repeated will return the last value, except for message types where
    /// all the set values are merged together.
    /// </para>
    /// </remarks>
    public sealed class CustomOptions
    {
        private const string UnreferencedCodeMessage = "CustomOptions is incompatible with trimming.";

        private static readonly object[] EmptyParameters = new object[0];
        private readonly IDictionary<int, IExtensionValue> values;

        internal CustomOptions(IDictionary<int, IExtensionValue> values)
        {
            this.values = values;
        }

        /// <summary>
        /// Retrieves a Boolean value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetBool(int field, out bool value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves a signed 32-bit integer value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetInt32(int field, out int value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves a signed 64-bit integer value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetInt64(int field, out long value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves an unsigned 32-bit integer value for the specified option field,
        /// assuming a fixed-length representation.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetFixed32(int field, out uint value) => TryGetUInt32(field, out value);

        /// <summary>
        /// Retrieves an unsigned 64-bit integer value for the specified option field,
        /// assuming a fixed-length representation.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetFixed64(int field, out ulong value) => TryGetUInt64(field, out value);

        /// <summary>
        /// Retrieves a signed 32-bit integer value for the specified option field,
        /// assuming a fixed-length representation.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetSFixed32(int field, out int value) => TryGetInt32(field, out value);

        /// <summary>
        /// Retrieves a signed 64-bit integer value for the specified option field,
        /// assuming a fixed-length representation.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetSFixed64(int field, out long value) => TryGetInt64(field, out value);

        /// <summary>
        /// Retrieves a signed 32-bit integer value for the specified option field,
        /// assuming a zigzag encoding.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetSInt32(int field, out int value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves a signed 64-bit integer value for the specified option field,
        /// assuming a zigzag encoding.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetSInt64(int field, out long value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves an unsigned 32-bit integer value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetUInt32(int field, out uint value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves an unsigned 64-bit integer value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetUInt64(int field, out ulong value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves a 32-bit floating point value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetFloat(int field, out float value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves a 64-bit floating point value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetDouble(int field, out double value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves a string value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetString(int field, out string value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves a bytes value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetBytes(int field, out ByteString value) => TryGetPrimitiveValue(field, out value);

        /// <summary>
        /// Retrieves a message value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        public bool TryGetMessage<T>(int field, out T value) where T : class, IMessage, new()
        {
            if (values == null)
            {
                value = default(T);
                return false;
            }

            IExtensionValue extensionValue;
            if (values.TryGetValue(field, out extensionValue))
            {
                if (extensionValue is ExtensionValue<T>)
                {
                    ExtensionValue<T> single = extensionValue as ExtensionValue<T>;
                    ByteString bytes = single.GetValue().ToByteString();
                    value = new T();
                    value.MergeFrom(bytes);
                    return true;
                }
                else if (extensionValue is RepeatedExtensionValue<T>)
                {
                    RepeatedExtensionValue<T> repeated = extensionValue as RepeatedExtensionValue<T>;
                    value = repeated.GetValue()
                        .Select(v => v.ToByteString())
                        .Aggregate(new T(), (t, b) =>
                        {
                            t.MergeFrom(b);
                            return t;
                        });
                    return true;
                }
            }

            value = null;
            return false;
        }

        [RequiresUnreferencedCode(UnreferencedCodeMessage)]
        private bool TryGetPrimitiveValue<T>(int field, out T value)
        {
            if (values == null)
            {
                value = default(T);
                return false;
            }

            IExtensionValue extensionValue;
            if (values.TryGetValue(field, out extensionValue))
            {
                if (extensionValue is ExtensionValue<T>)
                {
                    ExtensionValue<T> single = extensionValue as ExtensionValue<T>;
                    value = single.GetValue();
                    return true;
                }
                else if (extensionValue is RepeatedExtensionValue<T>)
                {
                    RepeatedExtensionValue<T> repeated = extensionValue as RepeatedExtensionValue<T>;
                    if (repeated.GetValue().Count != 0)
                    {
                        RepeatedField<T> repeatedField = repeated.GetValue();
                        value = repeatedField[repeatedField.Count - 1];
                        return true;
                    }
                }
                else // and here we find explicit enum handling since T : Enum ! x is ExtensionValue<Enum>
                {
                    var type = extensionValue.GetType();
                    if (type.GetGenericTypeDefinition() == typeof(ExtensionValue<>))
                    {
                        var typeInfo = type.GetTypeInfo();
                        var typeArgs = typeInfo.GenericTypeArguments;
                        if (typeArgs.Length == 1 && typeArgs[0].GetTypeInfo().IsEnum)
                        {
                            value = (T)typeInfo.GetDeclaredMethod(nameof(ExtensionValue<T>.GetValue)).Invoke(extensionValue, EmptyParameters);
                            return true;
                        }
                    }
                    else if (type.GetGenericTypeDefinition() == typeof(RepeatedExtensionValue<>))
                    {
                        var typeInfo = type.GetTypeInfo();
                        var typeArgs = typeInfo.GenericTypeArguments;
                        if (typeArgs.Length == 1 && typeArgs[0].GetTypeInfo().IsEnum)
                        {
                            var values = (IList)typeInfo.GetDeclaredMethod(nameof(RepeatedExtensionValue<T>.GetValue)).Invoke(extensionValue, EmptyParameters);
                            if (values.Count != 0)
                            {
                                value = (T)values[values.Count - 1];
                                return true;
                            }
                        }
                    }
                }
            }

            value = default(T);
            return false;
        }
    }
}
