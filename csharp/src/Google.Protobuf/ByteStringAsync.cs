#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Protobuf
{
    /// <summary>
    /// SecuritySafeCritical attribute can not be placed on types with async methods.
    /// This class has ByteString's async methods so it can be marked with SecuritySafeCritical.
    /// </summary>
    internal static class ByteStringAsync
    {
        internal static async Task<ByteString> FromStreamAsyncCore(Stream stream, CancellationToken cancellationToken)
        {
            int capacity = stream.CanSeek ? checked((int)(stream.Length - stream.Position)) : 0;
            var memoryStream = new MemoryStream(capacity);
            // We have to specify the buffer size here, as there's no overload accepting the cancellation token
            // alone. But it's documented to use 81920 by default if not specified.
            await stream.CopyToAsync(memoryStream, 81920, cancellationToken);
#if NETSTANDARD1_1
            byte[] bytes = memoryStream.ToArray();
#else
            // Avoid an extra copy if we can.
            byte[] bytes = memoryStream.Length == memoryStream.Capacity ? memoryStream.GetBuffer() : memoryStream.ToArray();
#endif
            return ByteString.AttachBytes(bytes);
        }
    }
}