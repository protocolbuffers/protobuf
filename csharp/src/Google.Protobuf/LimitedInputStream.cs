#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.IO;

namespace Google.Protobuf
{
    /// <summary>
    /// Stream implementation which proxies another stream, only allowing a certain amount
    /// of data to be read. Note that this is only used to read delimited streams, so it
    /// doesn't attempt to implement everything.
    /// </summary>
    internal sealed class LimitedInputStream : Stream
    {
        private readonly Stream proxied;
        private int bytesLeft;

        internal LimitedInputStream(Stream proxied, int size)
        {
            this.proxied = proxied;
            bytesLeft = size;
        }

        public override bool CanRead => true;
        public override bool CanSeek => false;
        public override bool CanWrite => false;

        public override void Flush()
        {
        }

        public override long Length => throw new NotSupportedException();

        public override long Position
        {
            get => throw new NotSupportedException();
            set => throw new NotSupportedException();
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            if (bytesLeft > 0)
            {
                int bytesRead = proxied.Read(buffer, offset, Math.Min(bytesLeft, count));
                bytesLeft -= bytesRead;
                return bytesRead;
            }
            return 0;
        }

        public override long Seek(long offset, SeekOrigin origin) => throw new NotSupportedException();

        public override void SetLength(long value) => throw new NotSupportedException();

        public override void Write(byte[] buffer, int offset, int count) => throw new NotSupportedException();
    }
}
