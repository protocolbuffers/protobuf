#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#endregion

using System;
using System.Security;
using Google.Protobuf.Collections;

namespace Google.Protobuf;

/// <summary>
/// An unsafe class that provides a set of methods to access the underlying data representations of
/// collections.
/// </summary>
[SecuritySafeCritical]
public static class UnsafeCollectionOperations
{
    /// <summary>
    /// <para>
    /// Returns a <see cref="Span{T}"/> that wraps the current backing array of the given
    /// <see cref="RepeatedField{T}"/>.
    /// </para>
    /// <para>
    /// Values in the <see cref="Span{T}"/> should not be set to null. Use
    /// <see cref="RepeatedField{T}.Remove(T)"/> or <see cref="RepeatedField{T}.RemoveAt(int)"/> to
    /// remove items instead.
    /// </para>
    /// <para>
    /// The returned <see cref="Span{T}"/> is only valid until the size of the
    /// <see cref="RepeatedField{T}"/> is modified, after which its state becomes undefined.
    /// Modifying existing elements without changing the size is safe as long as the modifications
    /// do not set null values.
    /// </para>
    /// </summary>
    /// <typeparam name="T">
    /// The type of elements in the <see cref="RepeatedField{T}"/>.
    /// </typeparam>
    /// <param name="field">
    /// The <see cref="RepeatedField{T}"/> for which to wrap the current backing array. Must not be
    /// null.
    /// </param>
    /// <returns>
    /// A <see cref="Span{T}"/> that wraps the current backing array of the
    /// <see cref="RepeatedField{T}"/>.
    /// </returns>
    /// <exception cref="ArgumentNullException">
    /// Thrown if <paramref name="field"/> is <see langword="null"/>.
    /// </exception>
    public static Span<T> AsSpan<T>(RepeatedField<T> field)
    {
        ProtoPreconditions.CheckNotNull(field, nameof(field));
        return field.AsSpan();
    }

    /// <summary>
    /// <para>
    /// Sets the count of the specified <see cref="RepeatedField{T}"/> to the given value.
    /// </para>
    /// <para>
    /// This method should only be called if the subsequent code guarantees to populate
    /// the field with the specified number of items.
    /// </para>
    /// <para>
    /// If count is less than <see cref="RepeatedField{T}.Count"/>, the collection is effectively
    /// trimmed down to the first count elements. <see cref="RepeatedField{T}.Capacity"/>
    /// is unchanged, meaning the underlying array remains allocated.
    /// </para>
    /// </summary>
    /// <typeparam name="T">
    /// The type of elements in the <see cref="RepeatedField{T}"/>.
    /// </typeparam>
    /// <param name="field">
    /// The field to set the count of. Must not be null.
    /// </param>
    /// <param name="count">
    /// The value to set the field's count to. Must be non-negative.
    /// </param>
    /// <exception cref="ArgumentNullException">
    /// Thrown if <paramref name="field"/> is <see langword="null"/>.
    /// </exception>
    /// <exception cref="ArgumentOutOfRangeException">
    /// Thrown if <paramref name="count"/> is negative.
    /// </exception>
    public static void SetCount<T>(RepeatedField<T> field, int count)
    {
        ProtoPreconditions.CheckNotNull(field, nameof(field));
        field.SetCount(count);
    }
}