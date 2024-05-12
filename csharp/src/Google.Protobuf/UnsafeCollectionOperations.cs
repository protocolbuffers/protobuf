#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#endregion

using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Security;
using Google.Protobuf.Collections;

namespace Google.Protobuf
{
    /// <summary>
    /// Provides unsafe operations for working with <see cref="RepeatedField{T}"/> instances. These
    /// methods are referred to as "unsafe" due to the fact that they potentially expose the backing
    /// buffer of a <see cref="RepeatedField{T}"/> to the application.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The methods in this class should only be called if it is guaranteed that the
    /// <see cref="RepeatedField{T}"/> will not be modified while the returned <see cref="Span{T}"/>
    /// is in use. Modifying a <see cref="RepeatedField{T}"/> while a <see cref="Span{T}"/> is in
    /// use can lead to unexpected and undesirable consequences in your application, and will likely
    /// be difficult to debug. Proceed with caution!
    /// </para>
    /// <para>
    /// Modifying a <see cref="RepeatedField{T}"/> while a <see cref="Span{T}"/> is in use can have
    /// a number of significant side effects:
    /// </para>
    /// <list type="bullet">
    /// <item>
    /// <description>
    /// The <see cref="Span{T}"/> may or may not reflect the changes made to the
    /// <see cref="RepeatedField{T}"/>.
    /// </description>
    /// </item>
    /// <item>
    /// <description>
    /// If the backing array of the <see cref="RepeatedField{T}"/> is reallocated, further updates
    /// to the <see cref="Span{T}"/> will not be visible.
    /// </description>
    /// </item>
    /// <item>
    /// <description>
    /// The state of the <see cref="Span{T}"/> is undefined after any modifications to the <see
    /// cref="RepeatedField{T}"/>.
    /// </description>
    /// </item>
    /// </list>
    /// </remarks>
    [SecuritySafeCritical]
    public static class UnsafeCollectionOperations
    {
        /// <summary>
        /// Returns a <see cref="Span{T}"/> that wraps the backing array of the specified
        /// <see cref="RepeatedField{T}"/>. The returned <see cref="Span{T}"/> should not be used
        /// after any modifications are made to the <see cref="RepeatedField{T}"/>.
        /// </summary>
        /// <typeparam name="T">
        /// The element type of the <see cref="RepeatedField{T}"/>.
        /// </typeparam>
        /// <param name="field">
        /// The <see cref="RepeatedField{T}"/> to wrap.
        /// </param>
        /// <returns>
        /// A <see cref="Span{T}"/> that wraps the backing array of the
        /// <see cref="RepeatedField{T}"/>.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Span<T> AsSpan<T>(RepeatedField<T> field)
        {
            Span<T> span = default;
            if (field is not null)
            {
                var size = field.count;
                var items = field.array;

                Debug.Assert(items is not null, "Implementation depends on RepeatedField<T> always having an array.");

                if ((uint) size > (uint) items.Length)
                {
                    // RepeatedField<T> was erroneously mutated concurrently with this call, leading
                    // to a count larger than its array.
                    throw new InvalidOperationException("Operations that change non-concurrent collections must have exclusive access. A concurrent update was performed on this collection and corrupted its state. The collection's state is no longer correct.");
                }

                Debug.Assert(typeof(T[]) == field.array.GetType(), "Implementation depends on RepeatedField<T> always using a T[] and not U[] where U : T.");

                span = items.AsSpan(0, size);
            }

            return span;
        }

        /// <summary>
        /// Sets the count of the <see cref="RepeatedField{T}"/> to the specified value.
        /// </summary>
        /// <param name="field">The field to set the count of.</param>
        /// <param name="count">The value to set the field's count to.</param>
        /// <typeparam name="T">The type of the elements in the field.</typeparam>
        /// <exception cref="NullReferenceException">
        /// <paramref name="field"/> is <see langword="null"/>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="count"/> is negative.
        /// </exception>
        /// <remarks>
        /// When increasing the count, uninitialized data is being exposed.
        /// </remarks>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void SetCount<T>(RepeatedField<T> field, int count)
        {
            if (count < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(count), count, "Non-negative number required.");
            }

            if (count > field.Capacity)
            {
                field.EnsureSize(count);
            }
#if NET5_0_OR_GREATER
            else if (count < field.count && RuntimeHelpers.IsReferenceOrContainsReferences<T>())
            {
                // Only reference types need to be cleared to allow GC to collect them.
                Array.Clear(field.array, count, field.count - count);
            }
#else
            else if (count < field.count)
            {
                Array.Clear(field.array, count, field.count - count);
            }
#endif

            field.count = count;
        }
    }
}