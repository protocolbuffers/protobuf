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

namespace Google.Protobuf
{
    /// <summary>
    /// Provides a number of unsafe byte operations to be used by advanced applications with high performance
    /// requirements. These methods are referred to as "unsafe" due to the fact that they potentially expose
    /// the backing buffer of a <see cref="ByteString"/> to the application.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The methods in this class should only be called if it is guaranteed that the buffer backing the
    /// <see cref="ByteString"/> will never change! Mutation of a <see cref="ByteString"/> can lead to unexpected
    /// and undesirable consequences in your application, and will likely be difficult to debug. Proceed with caution!
    /// </para>
    /// <para>
    /// This can have a number of significant side affects that have spooky-action-at-a-distance-like behavior. In
    /// particular, if the bytes value changes out from under a Protocol Buffer:
    /// </para>
    /// <list type="bullet">
    /// <item>
    /// <description>serialization may throw</description>
    /// </item>
    /// <item>
    /// <description>serialization may succeed but the wrong bytes may be written out</description>
    /// </item>
    /// <item>
    /// <description>objects that are normally immutable (such as ByteString) are no longer immutable</description>
    /// </item>
    /// <item>
    /// <description>hashCode may be incorrect</description>
    /// </item>
    /// </list>
    /// </remarks>
    [SecuritySafeCritical]
    public static class UnsafeByteOperations
    {
        /// <summary>
        /// Constructs a new <see cref="ByteString" /> from the given bytes. The bytes are not copied,
        /// and must not be modified while the <see cref="ByteString" /> is in use.
        /// This API is experimental and subject to change.
        /// </summary>
        public static ByteString UnsafeWrap(ReadOnlyMemory<byte> bytes)
        {
            return ByteString.AttachBytes(bytes);
        }
    }
}
