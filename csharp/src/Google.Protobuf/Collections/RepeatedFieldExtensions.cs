#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// A set of extension methods on RepeatedField
    /// </summary>
    public static class RepeatedFieldExtensions
    {
        /// <summary>Adds the elements of the specified span to the end of the <see cref="RepeatedField{T}"/>.</summary>
        /// <typeparam name="T">The type of elements in the <see cref="RepeatedField{T}"/>.</typeparam>
        /// <param name="repeatedField">The list to which the elements should be added.</param>
        /// <param name="source">The span whose elements should be added to the end of the <see cref="RepeatedField{T}"/>.</param>
        /// <exception cref="ArgumentNullException">The <paramref name="repeatedField"/> is null.</exception>
        public static void AddRange<T>(this RepeatedField<T> repeatedField, ReadOnlySpan<T> source)
        {
            ProtoPreconditions.CheckNotNull(repeatedField, nameof(repeatedField));

            if (source.IsEmpty)
            {
                return;
            }

            // For reference types and nullable value types, we need to check that there are no nulls
            // present. (This isn't a thread-safe approach, but we don't advertise this is thread-safe.)
            // We expect the JITter to optimize this test to true/false, so it's effectively conditional
            // specialization.
            if (default(T) == null)
            {
                for (int i = 0; i < source.Length; i++)
                {
                    if (source[i] == null)
                    {
                        throw new ArgumentException("ReadOnlySpan contained null element", nameof(source));
                    }
                }
            }

            repeatedField.EnsureSize(repeatedField.count + source.Length);
            source.CopyTo(repeatedField.array.AsSpan(repeatedField.count));
            repeatedField.count += source.Length;
        }
    }
}