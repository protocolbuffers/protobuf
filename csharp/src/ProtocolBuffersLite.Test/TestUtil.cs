using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Google.ProtocolBuffers
{
    class TestUtil
    {
        internal static void AssertBytesEqual(byte[] a, byte[] b)
        {
            if (a == null || b == null)
            {
                Assert.AreEqual<object>(a, b);
            }
            else
            {
                Assert.AreEqual(a.Length, b.Length, "The byte[] is not of the expected length.");

                for (int i = 0; i < a.Length; i++)
                {
                    if (a[i] != b[i])
                    {
                        Assert.AreEqual(a[i], b[i], "Byte[] differs at index " + i);
                    }
                }
            }
        }

    }
}
