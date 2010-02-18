using System;

namespace Google.ProtocolBuffers
{
    public class ByteBuffer
    {
        public byte[] Buffer;
        public int Offset;
        public int Length;
        private int hash;

        public void ResetHash()
        {
            hash = 23;
            for (var i = Offset; i < Offset + Length; i++)
            {
                hash = (hash * 23) ^ Buffer[i];
            }
        }

        public ByteBuffer(byte[] buffer, int offset, int length)
        {
            Buffer = buffer;
            Offset = offset;
            Length = length;
            ResetHash();
        }

        public ByteString ToByteString()
        {
            return ByteString.CopyFrom(Buffer, Offset, Length);
        }

        public override int GetHashCode()
        {
            return hash;
        }

        public override bool Equals(object obj)
        {
            var other = obj as ByteBuffer;
            if (other == null)
                return false;
            if (other.Offset != Offset)
                return false;
            if (other.Length != Length)
                return false;
            for (int i = Offset; i < Offset + Length; i++)
            {
                if (Buffer[i] != other.Buffer[i])
                    return false;
            }
            return true;
        }
    }
}