#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf
{
    /// <summary>
    /// Generic interface for a deeply cloneable type.
    /// </summary>
    /// <remarks>
    /// <para>
    /// All generated messages implement this interface, but so do some non-message types.
    /// Additionally, due to the type constraint on <c>T</c> in <see cref="IMessage{T}"/>,
    /// it is simpler to keep this as a separate interface.
    /// </para>
    /// </remarks>
    /// <typeparam name="T">The type itself, returned by the <see cref="Clone"/> method.</typeparam>
    public interface IDeepCloneable<T>
    {
        /// <summary>
        /// Creates a deep clone of this object.
        /// </summary>
        /// <returns>A deep clone of this object.</returns>
        T Clone();
    }
}
