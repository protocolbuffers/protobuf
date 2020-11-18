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
