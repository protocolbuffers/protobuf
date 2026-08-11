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
    /// <summary>
    /// Covers the bulk path used to total the encoded length of a packed varint
    /// field. A wrong total is not a slow serialization but a corrupt one, because
    /// the total becomes the length prefix.
    /// </summary>
    /// <remarks>
    /// <para>
    /// Expectations here are hand-computed wire bytes and hand-counted lengths, not
    /// values derived by running the encoding rules a second time. A test that
    /// recomputes the expectation the same way the code does will agree with the
    /// code whether or not either is right.
    /// </para>
    /// <para>
    /// These use TestPackedTypes rather than TestAllTypes: unittest.proto is proto2,
    /// where a repeated field is not packed unless declared so, and only a packed
    /// field reaches the code under test. Every field used here is in the range 90-95,
    /// so every tag is two bytes: for example packed_int32 is field 90, giving
    /// (90 &lt;&lt; 3) | 2 = 722, which encodes as D2 05.
    /// </para>
    /// </remarks>
    public class PackedVarintSizeTest
    {
        /// <summary>
        /// The declared size must match the bytes actually produced. Serializing to a
        /// stream rather than via ToByteArray is deliberate: ToByteArray allocates its
        /// buffer at CalculateSize(), so comparing against that array's length would
        /// compare a number with itself.
        /// </summary>
        private static void AssertDeclaredSizeMatchesBytesWritten(TestPackedTypes message)
        {
            var stream = new MemoryStream();
            message.WriteTo(stream);

            Assert.AreEqual(stream.Length, message.CalculateSize());
            Assert.AreEqual(message, TestPackedTypes.Parser.ParseFrom(stream.ToArray()));
        }

        private static void AssertWireBytes(TestPackedTypes message, byte[] expected)
        {
            Assert.AreEqual(expected.Length, message.CalculateSize());
            CollectionAssert.AreEqual(expected, message.ToByteArray());
        }

        // ---- Exact wire bytes, hand-encoded ----

        [Test]
        public void Int32WireBytes()
        {
            var message = new TestPackedTypes();
            message.PackedInt32.Add(new[] { 0, 1, 127, 128, 300, -1 });

            // 00           0
            // 01           1
            // 7F           127
            // 80 01        128
            // AC 02        300
            // FF x9 01     -1, sign-extended to 64 bits
            // 17 bytes of payload, so the length prefix is 11.
            AssertWireBytes(message, new byte[]
            {
                0xD2, 0x05, 0x11,
                0x00, 0x01, 0x7F, 0x80, 0x01, 0xAC, 0x02,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01
            });
        }

        [Test]
        public void UInt32WireBytes()
        {
            var message = new TestPackedTypes();
            message.PackedUint32.Add(new uint[] { 0, 127, 128, 16383, 16384, uint.MaxValue });

            // 00           0
            // 7F           127
            // 80 01        128
            // FF 7F        16383
            // 80 80 01     16384
            // FF FF FF FF 0F   uint.MaxValue
            // 14 bytes of payload.
            AssertWireBytes(message, new byte[]
            {
                0xE2, 0x05, 0x0E,
                0x00, 0x7F, 0x80, 0x01, 0xFF, 0x7F, 0x80, 0x80, 0x01,
                0xFF, 0xFF, 0xFF, 0xFF, 0x0F
            });
        }

        [Test]
        public void SInt32WireBytes()
        {
            var message = new TestPackedTypes();
            message.PackedSint32.Add(new[] { 0, -1, 1, -64, 64, int.MinValue });

            // Zigzag maps n to (n << 1) ^ (n >> 31), so:
            // 00           0  -> 0
            // 01           -1 -> 1
            // 02           1  -> 2
            // 7F           -64 -> 127
            // 80 01        64 -> 128
            // FF FF FF FF 0F   int.MinValue -> 4294967295
            // 11 bytes of payload.
            AssertWireBytes(message, new byte[]
            {
                0xF2, 0x05, 0x0B,
                0x00, 0x01, 0x02, 0x7F, 0x80, 0x01,
                0xFF, 0xFF, 0xFF, 0xFF, 0x0F
            });
        }

        [Test]
        public void Int64WireBytes()
        {
            var message = new TestPackedTypes();
            message.PackedInt64.Add(new[] { 0L, 128L, long.MaxValue, -1L });

            // 00                   0
            // 80 01                128
            // FF x8 7F             long.MaxValue, 63 significant bits -> 9 bytes
            // FF x9 01             -1 -> 10 bytes
            // 22 bytes of payload, so the length prefix is 16.
            AssertWireBytes(message, new byte[]
            {
                0xDA, 0x05, 0x16,
                0x00, 0x80, 0x01,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01
            });
        }

        [Test]
        public void UInt64WireBytes()
        {
            var message = new TestPackedTypes();
            message.PackedUint64.Add(new ulong[] { 0, 1, ulong.MaxValue });

            // 00           0
            // 01           1
            // FF x9 01     ulong.MaxValue, 64 significant bits -> 10 bytes
            // 12 bytes of payload.
            AssertWireBytes(message, new byte[]
            {
                0xEA, 0x05, 0x0C,
                0x00, 0x01,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01
            });
        }

        [Test]
        public void SInt64WireBytes()
        {
            var message = new TestPackedTypes();
            message.PackedSint64.Add(new[] { 0L, -1L, long.MinValue });

            // 00           0 -> 0
            // 01           -1 -> 1
            // FF x9 01     long.MinValue -> 18446744073709551615
            // 12 bytes of payload.
            AssertWireBytes(message, new byte[]
            {
                0xFA, 0x05, 0x0C,
                0x00, 0x01,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01
            });
        }

        // ---- Per-value encoded lengths, hand-counted ----
        //
        // A single-element packed field, so the message is a two-byte tag, a one-byte
        // length prefix and the value: 3 + the length below. These straddle every
        // point at which a varint gains a byte.

        [TestCase(0, 1)]
        [TestCase(1, 1)]
        [TestCase(127, 1)]
        [TestCase(128, 2)]
        [TestCase(16383, 2)]
        [TestCase(16384, 3)]
        [TestCase(2097151, 3)]
        [TestCase(2097152, 4)]
        [TestCase(268435455, 4)]
        [TestCase(268435456, 5)]
        [TestCase(int.MaxValue, 5)]
        [TestCase(-1, 10)]
        [TestCase(-2, 10)]
        [TestCase(int.MinValue, 10)]
        public void Int32EncodedLength(int value, int expectedLength)
        {
            var message = new TestPackedTypes();
            message.PackedInt32.Add(value);
            Assert.AreEqual(3 + expectedLength, message.CalculateSize());
        }

        [TestCase(0u, 1)]
        [TestCase(127u, 1)]
        [TestCase(128u, 2)]
        [TestCase(16383u, 2)]
        [TestCase(16384u, 3)]
        [TestCase(2097151u, 3)]
        [TestCase(2097152u, 4)]
        [TestCase(268435455u, 4)]
        [TestCase(268435456u, 5)]
        [TestCase(uint.MaxValue, 5)]
        public void UInt32EncodedLength(uint value, int expectedLength)
        {
            var message = new TestPackedTypes();
            message.PackedUint32.Add(value);
            Assert.AreEqual(3 + expectedLength, message.CalculateSize());
        }

        [TestCase(0, 1)]
        [TestCase(-1, 1)]
        [TestCase(1, 1)]
        [TestCase(63, 1)]
        [TestCase(-64, 1)]
        [TestCase(64, 2)]
        [TestCase(-65, 2)]
        [TestCase(8191, 2)]
        [TestCase(-8192, 2)]
        [TestCase(8192, 3)]
        [TestCase(int.MaxValue, 5)]
        [TestCase(int.MinValue, 5)]
        public void SInt32EncodedLength(int value, int expectedLength)
        {
            var message = new TestPackedTypes();
            message.PackedSint32.Add(value);
            Assert.AreEqual(3 + expectedLength, message.CalculateSize());
        }

        [TestCase(0L, 1)]
        [TestCase(127L, 1)]
        [TestCase(128L, 2)]
        [TestCase(16384L, 3)]
        [TestCase(2097152L, 4)]
        [TestCase(268435456L, 5)]
        [TestCase(34359738367L, 5)]
        [TestCase(34359738368L, 6)]
        [TestCase(4398046511103L, 6)]
        [TestCase(4398046511104L, 7)]
        [TestCase(562949953421311L, 7)]
        [TestCase(562949953421312L, 8)]
        [TestCase(72057594037927935L, 8)]
        [TestCase(72057594037927936L, 9)]
        [TestCase(long.MaxValue, 9)]
        [TestCase(-1L, 10)]
        [TestCase(long.MinValue, 10)]
        public void Int64EncodedLength(long value, int expectedLength)
        {
            var message = new TestPackedTypes();
            message.PackedInt64.Add(value);
            Assert.AreEqual(3 + expectedLength, message.CalculateSize());
        }

        [TestCase(0UL, 1)]
        [TestCase(127UL, 1)]
        [TestCase(128UL, 2)]
        [TestCase(72057594037927935UL, 8)]
        [TestCase(72057594037927936UL, 9)]
        [TestCase(9223372036854775807UL, 9)]
        [TestCase(9223372036854775808UL, 10)]
        [TestCase(ulong.MaxValue, 10)]
        public void UInt64EncodedLength(ulong value, int expectedLength)
        {
            var message = new TestPackedTypes();
            message.PackedUint64.Add(value);
            Assert.AreEqual(3 + expectedLength, message.CalculateSize());
        }

        [TestCase(0L, 1)]
        [TestCase(-1L, 1)]
        [TestCase(1L, 1)]
        [TestCase(63L, 1)]
        [TestCase(-64L, 1)]
        [TestCase(64L, 2)]
        [TestCase(long.MaxValue, 10)]
        [TestCase(long.MinValue, 10)]
        public void SInt64EncodedLength(long value, int expectedLength)
        {
            var message = new TestPackedTypes();
            message.PackedSint64.Add(value);
            Assert.AreEqual(3 + expectedLength, message.CalculateSize());
        }

        // ---- Properties that hold whatever the values are ----

        [Test]
        public void EmptyMessageIsEmpty()
        {
            Assert.AreEqual(0, new TestPackedTypes().CalculateSize());
            Assert.AreEqual(0, new TestPackedTypes().ToByteArray().Length);
        }

        /// <summary>
        /// A run long enough that the size is not something anyone would hand-count,
        /// checked as a property instead: what CalculateSize declares is what gets
        /// written, and the values survive the round trip under that length prefix.
        /// </summary>
        [Test]
        public void LongRunDeclaresTheSizeItWrites()
        {
            var message = new TestPackedTypes();
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

        /// <summary>
        /// Fixed-width and non-varint packed fields must keep sizing the way they did.
        /// fixed32 in particular is also a FieldCodec&lt;uint&gt;, so it is the case a
        /// type-based shortcut would get wrong.
        /// </summary>
        [Test]
        public void NonVarintPackedFieldsAreUnaffected()
        {
            var message = new TestPackedTypes();
            message.PackedFixed32.Add(new uint[] { 0, 1, uint.MaxValue });
            message.PackedFixed64.Add(new ulong[] { 0, 1, ulong.MaxValue });
            message.PackedSfixed32.Add(new[] { 0, -1, int.MinValue });
            message.PackedSfixed64.Add(new[] { 0L, -1L, long.MinValue });
            message.PackedFloat.Add(new[] { 0f, 1f, -1f, float.MaxValue });
            message.PackedDouble.Add(new[] { 0d, 1d, -1d, double.MaxValue });
            message.PackedBool.Add(new[] { true, false, true });
            message.PackedEnum.Add(ForeignEnum.ForeignBar);

            // Three fixed32 is a two-byte tag, a one-byte length and 12 bytes of payload.
            Assert.AreEqual(15, new TestPackedTypes { PackedFixed32 = { 0u, 1u, uint.MaxValue } }.CalculateSize());
            AssertDeclaredSizeMatchesBytesWritten(message);
        }

        /// <summary>
        /// The same values in unpacked fields, which take the per-element path and so
        /// must be unchanged by any of this.
        /// </summary>
        [Test]
        public void UnpackedFieldsAreUnaffected()
        {
            var message = new TestAllTypes();
            message.RepeatedInt32.Add(new[] { 0, 1, 128, -1, int.MinValue });
            message.RepeatedUint32.Add(new uint[] { 0, 128, uint.MaxValue });
            message.RepeatedInt64.Add(new[] { 0L, 128L, -1L });
            message.RepeatedUint64.Add(new ulong[] { 0, 128, ulong.MaxValue });
            message.RepeatedSint32.Add(new[] { 0, -1, int.MinValue });
            message.RepeatedSint64.Add(new[] { 0L, -1L, long.MinValue });

            var stream = new MemoryStream();
            message.WriteTo(stream);

            Assert.AreEqual(stream.Length, message.CalculateSize());
            Assert.AreEqual(message, TestAllTypes.Parser.ParseFrom(stream.ToArray()));
        }
    }
}
