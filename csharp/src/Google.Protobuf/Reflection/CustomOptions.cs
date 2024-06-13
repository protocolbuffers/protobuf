#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Collections;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
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
    [DebuggerDisplay("Count = {DebugCount}")]
    [DebuggerTypeProxy(typeof(CustomOptionsDebugView))]
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
                value = default;
                return false;
            }

            if (values.TryGetValue(field, out IExtensionValue extensionValue))
            {
                if (extensionValue is ExtensionValue<T> single)
                {
                    ByteString bytes = single.GetValue().ToByteString();
                    value = new T();
                    value.MergeFrom(bytes);
                    return true;
                }
                else if (extensionValue is RepeatedExtensionValue<T> repeated)
                {
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
                value = default;
                return false;
            }

            if (values.TryGetValue(field, out IExtensionValue extensionValue))
            {
                if (extensionValue is ExtensionValue<T> single)
                {
                    value = single.GetValue();
                    return true;
                }
                else if (extensionValue is RepeatedExtensionValue<T> repeated)
                {
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

            value = default;
            return false;
        }

        private int DebugCount => values?.Count ?? 0;

        private sealed class CustomOptionsDebugView
        {
            private readonly CustomOptions customOptions;

            public CustomOptionsDebugView(CustomOptions customOptions)
            {
                this.customOptions = customOptions;
            }

            [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
            public KeyValuePair<int, IExtensionValue>[] Items => customOptions.values?.ToArray() ?? new KeyValuePair<int, IExtensionValue>[0];
        }
    }
}
