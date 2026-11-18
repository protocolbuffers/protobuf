#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Security;
using Google.Protobuf.Collections;

// Note: The choice of the namespace here is important because it's rare that people will directly reference Google.Protobuf.Collections
// in their app as they're typically using collections created by generated code. We are keeping the extension method in the Google.Protobuf
// namespace so that it is discoverable by most users.

namespace Google.Protobuf
{
    /// <summary>
    /// A set of extension methods on <see cref="RepeatedField{T}"/>
    /// </summary>
    public static class RepeatedFieldExtensions
    {
        // Note: This method is an extension method to avoid ambiguous overload conflict with existing AddRange(IEnumerable<T>) when the source is an array.
        /// <summary>Adds the elements of the specified span to the end of the <see cref="RepeatedField{T}"/>.</summary>
        /// <typeparam name="T">The type of elements in the <see cref="RepeatedField{T}"/>.</typeparam>
        /// <param name="repeatedField">The list to which the elements should be added.</param>
        /// <param name="source">The span whose elements should be added to the end of the <see cref="RepeatedField{T}"/>.</param>
        /// <exception cref="ArgumentNullException">The <paramref name="repeatedField"/> is null.</exception>
        [SecuritySafeCritical]
        public static void AddRange<T>(this RepeatedField<T> repeatedField, ReadOnlySpan<T> source)
        {
            ProtoPreconditions.CheckNotNull(repeatedField, nameof(repeatedField));
            repeatedField.AddRangeSpan(source);
        }
    }
}