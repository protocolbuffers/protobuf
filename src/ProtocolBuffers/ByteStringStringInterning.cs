using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// This class tries hard to allow us to generate strings directly from buffer outputs without having to
    /// 
    /// Note, non thread safe
    /// </summary>
    public class ByteStringStringInterning
    {
        private class ByteStringOrByteBuffer : IEquatable<ByteStringOrByteBuffer>
        {
            private readonly ByteString str;
            private readonly ByteBuffer buffer;

            public ByteStringOrByteBuffer(ByteString str)
            {
                this.str = str;
            }

            public ByteStringOrByteBuffer(ByteBuffer buffer)
            {
                this.buffer = buffer;
            }

            public bool Equals(ByteStringOrByteBuffer other)
            {
                if (ReferenceEquals(null, other)) return false;
                if (ReferenceEquals(this, other)) return true;
                if(other.str!=null && str != null)
                    return Equals(other.str, str);
                if (other.buffer != null && buffer != null)
                    return Equals(other.buffer, buffer);
                if (other.str != null && str == null)
                    return StringEqualsToBuffer(other.str, buffer);
                return StringEqualsToBuffer(str, other.buffer);
            }

            private static bool StringEqualsToBuffer(ByteString byteString, ByteBuffer byteBuffer)
            {
                var strLen = byteString.Length;
                if(strLen != byteBuffer.Length)
                    return false;
                for (int i = 0; i < strLen; i++)
                {
                    if(byteString.bytes[i] != byteBuffer.Buffer[byteBuffer.Offset+i])
                        return false;
                }
                return true;
            }

            public override bool Equals(object obj)
            {
                if (ReferenceEquals(null, obj)) return false;
                if (ReferenceEquals(this, obj)) return true;
                return Equals(obj as ByteStringOrByteBuffer);
            }

            public override int GetHashCode()
            {
                return str != null ? str.GetHashCode() : buffer.GetHashCode();
            }
        }

        private readonly int limit;
        private int timestamp;
        private readonly IDictionary<ByteStringOrByteBuffer, Data> strings = new Dictionary<ByteStringOrByteBuffer, Data>();

        public static ByteStringStringInterning CreateInstance()
        {
            return new ByteStringStringInterning(65536);
        }

        [Serializable]
        private class Data
        {
            public string Value;
            public int Timestamp;
        }

        private ByteStringStringInterning(int limit)
        {
            this.limit = limit;
        }

        public void Clear()
        {
           strings.Clear();
        }

        public string Intern(ByteBuffer str)
        {
            Data val;

            int currentTimestamp = Interlocked.Increment(ref timestamp);
            if (strings.TryGetValue(new ByteStringOrByteBuffer(str), out val))
            {
                Interlocked.Exchange(ref val.Timestamp, currentTimestamp);
                return val.Value;
            }

            var byteString = str.ToByteString();
            val = new Data { Timestamp = currentTimestamp, Value = byteString.ToStringUtf8() };

            strings.Add(new ByteStringOrByteBuffer(byteString), val);

            DoCleanupIfNeeded();
            return val.Value;
        }

        private void DoCleanupIfNeeded()
        {
            if (strings.Count <= limit)
                return;

            // to avoid frequent thrashing, we will remove the bottom 10% of the current pool in one go
            // that means that we will hit the limit fairly infrequently
            var list = new List<KeyValuePair<ByteStringOrByteBuffer, Data>>(strings);
            list.Sort((x, y) => x.Value.Timestamp - y.Value.Timestamp);

            for (int i = 0; i < limit/10; i++)
            {
                strings.Remove(list[i].Key);                
            }
        }
    }
}