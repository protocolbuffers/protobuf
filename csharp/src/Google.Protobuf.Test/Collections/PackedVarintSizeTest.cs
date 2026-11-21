#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.IO;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf.Collections
{
    public class PackedVarintSizeTest
    {
        // Writes to a stream because ToByteArray sizes its buffer from CalculateSize.
        private static void AssertDeclaredSizeMatchesBytesWritten(TestPackedTypes message)
        {
            var stream = new MemoryStream();
            message.WriteTo(stream);

            Assert.AreEqual(stream.Length, message.CalculateSize());
            Assert.AreEqual(message, TestPackedTypes.Parser.ParseFrom(stream.ToArray()));
        }

        [Test]
        public void PackedVarintSizeMatchesBytesWritten()
        {
            var message = new TestPackedTypes();
            message.PackedInt32.Add(new[] { -1, int.MinValue, int.MaxValue });
            message.PackedInt64.Add(new[] { -1L, long.MinValue, long.MaxValue });
            for (int i = 0; i < 1000; i++)
            {
                message.PackedInt32.Add(i * 7919);
                message.PackedInt64.Add((long) i * 7919 * 65599);
                message.PackedUint32.Add((uint) (i * 65599));
                message.PackedUint64.Add((ulong) i * 7919 * 65599 * 104729);
                message.PackedSint32.Add(i % 2 == 0 ? i * 7919 : -i * 7919);
                message.PackedSint64.Add(i % 2 == 0 ? (long) i * 7919 * 65599 : -(long) i * 7919 * 65599);
            }
            AssertDeclaredSizeMatchesBytesWritten(message);
        }

        // fixed32 shares FieldCodec<uint> with uint32 but must keep its fixed-size path.
        [Test]
        public void PackedFixed32SizeIsUnaffected()
        {
            // Two-byte tag, one-byte length, 12 bytes of payload.
            var message = new TestPackedTypes { PackedFixed32 = { 0u, 1u, uint.MaxValue } };
            Assert.AreEqual(15, message.CalculateSize());
            AssertDeclaredSizeMatchesBytesWritten(message);
        }
    }
}
