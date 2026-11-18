#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf
{
    /// <summary>
    /// Provides a utility routine to copy small arrays much more quickly than Buffer.BlockCopy
    /// </summary>
    internal static class ByteArray
    {
        /// <summary>
        /// The threshold above which you should use Buffer.BlockCopy rather than ByteArray.Copy
        /// </summary>
        private const int CopyThreshold = 12;

        /// <summary>
        /// Determines which copy routine to use based on the number of bytes to be copied.
        /// </summary>
        internal static void Copy(byte[] src, int srcOffset, byte[] dst, int dstOffset, int count)
        {
            if (count > CopyThreshold)
            {
                Buffer.BlockCopy(src, srcOffset, dst, dstOffset, count);
            }
            else
            {
                int stop = srcOffset + count;
                for (int i = srcOffset; i < stop; i++)
                {
                    dst[dstOffset++] = src[i];
                }
            }
        }

        /// <summary>
        /// Reverses the order of bytes in the array
        /// </summary>
        internal static void Reverse(byte[] bytes)
        {
            for (int first = 0, last = bytes.Length - 1; first < last; first++, last--)
            {
                byte temp = bytes[first];
                bytes[first] = bytes[last];
                bytes[last] = temp;
            }
        }
    }
}