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

        public override bool CanRead
        {
            get { return true; }
        }

        public override bool CanSeek
        {
            get { return false; }
        }

        public override bool CanWrite
        {
            get { return false; }
        }

        public override void Flush()
        {
        }

        public override long Length
        {
            get { throw new NotSupportedException(); }
        }

        public override long Position
        {
            get { throw new NotSupportedException(); }
            set { throw new NotSupportedException(); }
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

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotSupportedException();
        }

        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotSupportedException();
        }
    }
}
