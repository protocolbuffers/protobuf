namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Provides a utility routine to copy small arrays much more quickly than Buffer.BlockCopy
    /// </summary>
    static class ByteArray
    {
        /// <summary>
        /// The threshold above which you should use Buffer.BlockCopy rather than ByteArray.Copy
        /// </summary>
        const int CopyThreshold = 12;
        /// <summary>
        /// Determines which copy routine to use based on the number of bytes to be copied.
        /// </summary>
        public static void Copy(byte[] src, int srcOffset, byte[] dst, int dstOffset, int count)
        {
            if (count > CopyThreshold)
                global::System.Buffer.BlockCopy(src, srcOffset, dst, dstOffset, count);
            else
                ByteCopy(src, srcOffset, dst, dstOffset, count);
        }
        /// <summary>
        /// Copy the bytes provided with a for loop, faster when there are only a few bytes to copy
        /// </summary>
        public static void ByteCopy(byte[] src, int srcOffset, byte[] dst, int dstOffset, int count)
        {
            int stop = srcOffset + count;
            for (int i = srcOffset; i < stop; i++)
                dst[dstOffset++] = src[i];
        }
        /// <summary>
        /// Reverses the order of bytes in the array
        /// </summary>
        public static void Reverse(byte[] bytes)
        {
            byte temp;
            for (int first = 0, last = bytes.Length - 1; first < last; first++, last--)
            {
                temp = bytes[first];
                bytes[first] = bytes[last];
                bytes[last] = temp;
            }
        }
    }
}