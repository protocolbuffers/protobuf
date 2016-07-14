#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#endregion

using System.Collections.Generic;
using System.IO;
using System.Reflection;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class FieldCodecTest
    {
#pragma warning disable 0414 // Used by tests via reflection - do not remove!
        private static readonly List<ICodecTestData> Codecs = new List<ICodecTestData>
        {
            new FieldCodecTestData<bool>(FieldCodec.ForBool(100), true, "Bool"),
            new FieldCodecTestData<string>(FieldCodec.ForString(100), "sample", "String"),
            new FieldCodecTestData<ByteString>(FieldCodec.ForBytes(100), ByteString.CopyFrom(1, 2, 3), "Bytes"),
            new FieldCodecTestData<int>(FieldCodec.ForInt32(100), -1000, "Int32"),
            new FieldCodecTestData<int>(FieldCodec.ForSInt32(100), -1000, "SInt32"),
            new FieldCodecTestData<int>(FieldCodec.ForSFixed32(100), -1000, "SFixed32"),
            new FieldCodecTestData<uint>(FieldCodec.ForUInt32(100), 1234, "UInt32"),
            new FieldCodecTestData<uint>(FieldCodec.ForFixed32(100), 1234, "Fixed32"),
            new FieldCodecTestData<long>(FieldCodec.ForInt64(100), -1000, "Int64"),
            new FieldCodecTestData<long>(FieldCodec.ForSInt64(100), -1000, "SInt64"),
            new FieldCodecTestData<long>(FieldCodec.ForSFixed64(100), -1000, "SFixed64"),
            new FieldCodecTestData<ulong>(FieldCodec.ForUInt64(100), 1234, "UInt64"),
            new FieldCodecTestData<ulong>(FieldCodec.ForFixed64(100), 1234, "Fixed64"),
            new FieldCodecTestData<float>(FieldCodec.ForFloat(100), 1234.5f, "Float"),
            new FieldCodecTestData<double>(FieldCodec.ForDouble(100), 1234567890.5d, "Double"),
            new FieldCodecTestData<ForeignEnum>(
                FieldCodec.ForEnum(100, t => (int) t, t => (ForeignEnum) t), ForeignEnum.ForeignBaz, "Enum"),
            new FieldCodecTestData<ForeignMessage>(
                FieldCodec.ForMessage(100, ForeignMessage.Parser), new ForeignMessage { C = 10 }, "Message"),
        };
#pragma warning restore 0414

        [Test, TestCaseSource("Codecs")]
        public void RoundTripWithTag(ICodecTestData codec)
        {
            codec.TestRoundTripWithTag();
        }

        [Test, TestCaseSource("Codecs")]
        public void RoundTripRaw(ICodecTestData codec)
        {
            codec.TestRoundTripRaw();
        }

        [Test, TestCaseSource("Codecs")]
        public void CalculateSize(ICodecTestData codec)
        {
            codec.TestCalculateSizeWithTag();
        }

        [Test, TestCaseSource("Codecs")]
        public void DefaultValue(ICodecTestData codec)
        {
            codec.TestDefaultValue();
        }

        [Test, TestCaseSource("Codecs")]
        public void FixedSize(ICodecTestData codec)
        {
            codec.TestFixedSize();
        }

        // This is ugly, but it means we can have a non-generic interface.
        // It feels like NUnit should support this better, but I don't know
        // of any better ways right now.
        public interface ICodecTestData
        {
            void TestRoundTripRaw();
            void TestRoundTripWithTag();
            void TestCalculateSizeWithTag();
            void TestDefaultValue();
            void TestFixedSize();
        }

        public class FieldCodecTestData<T> : ICodecTestData
        {
            private readonly FieldCodec<T> codec;
            private readonly T sampleValue;
            private readonly string name;

            public FieldCodecTestData(FieldCodec<T> codec, T sampleValue, string name)
            {
                this.codec = codec;
                this.sampleValue = sampleValue;
                this.name = name;
            }

            public void TestRoundTripRaw()
            {
                var stream = new MemoryStream();
                var codedOutput = new CodedOutputStream(stream);
                codec.ValueWriter(codedOutput, sampleValue);
                codedOutput.Flush();
                stream.Position = 0;
                var codedInput = new CodedInputStream(stream);
                Assert.AreEqual(sampleValue, codec.ValueReader(codedInput));
                Assert.IsTrue(codedInput.IsAtEnd);
            }

            public void TestRoundTripWithTag()
            {
                var stream = new MemoryStream();
                var codedOutput = new CodedOutputStream(stream);
                codec.WriteTagAndValue(codedOutput, sampleValue);
                codedOutput.Flush();
                stream.Position = 0;
                var codedInput = new CodedInputStream(stream);
                codedInput.AssertNextTag(codec.Tag);
                Assert.AreEqual(sampleValue, codec.Read(codedInput));
                Assert.IsTrue(codedInput.IsAtEnd);
            }

            public void TestCalculateSizeWithTag()
            {
                var stream = new MemoryStream();
                var codedOutput = new CodedOutputStream(stream);
                codec.WriteTagAndValue(codedOutput, sampleValue);
                codedOutput.Flush();
                Assert.AreEqual(stream.Position, codec.CalculateSizeWithTag(sampleValue));
            }

            public void TestDefaultValue()
            {
                // WriteTagAndValue ignores default values
                var stream = new MemoryStream();
                var codedOutput = new CodedOutputStream(stream);
                codec.WriteTagAndValue(codedOutput, codec.DefaultValue);
                codedOutput.Flush();
                Assert.AreEqual(0, stream.Position);
                Assert.AreEqual(0, codec.CalculateSizeWithTag(codec.DefaultValue));
                if (typeof(T).GetTypeInfo().IsValueType)
                {
                    Assert.AreEqual(default(T), codec.DefaultValue);
                }

                // The plain ValueWriter/ValueReader delegates don't.
                if (codec.DefaultValue != null) // This part isn't appropriate for message types.
                {
                    codedOutput = new CodedOutputStream(stream);
                    codec.ValueWriter(codedOutput, codec.DefaultValue);
                    codedOutput.Flush();
                    Assert.AreNotEqual(0, stream.Position);
                    Assert.AreEqual(stream.Position, codec.ValueSizeCalculator(codec.DefaultValue));
                    stream.Position = 0;
                    var codedInput = new CodedInputStream(stream);
                    Assert.AreEqual(codec.DefaultValue, codec.ValueReader(codedInput));
                }
            }

            public void TestFixedSize()
            {
                Assert.AreEqual(name.Contains("Fixed"), codec.FixedSize != 0);
            }

            public override string ToString()
            {
                return name;
            }
        }
    }
}
