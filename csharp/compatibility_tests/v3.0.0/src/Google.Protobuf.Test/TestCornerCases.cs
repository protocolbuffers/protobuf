#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
    
using UnitTest.Issues.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class TestCornerCases
    {
        [Test]
        public void TestRoundTripNegativeEnums()
        {
            NegativeEnumMessage msg = new NegativeEnumMessage
            {
                Value = NegativeEnum.MinusOne,
                Values = { NegativeEnum.Zero, NegativeEnum.MinusOne, NegativeEnum.FiveBelow },
                PackedValues = { NegativeEnum.Zero, NegativeEnum.MinusOne, NegativeEnum.FiveBelow }
            };

            Assert.AreEqual(58, msg.CalculateSize());

            byte[] bytes = new byte[58];
            CodedOutputStream output = new CodedOutputStream(bytes);

            msg.WriteTo(output);
            Assert.AreEqual(0, output.SpaceLeft);

            NegativeEnumMessage copy = NegativeEnumMessage.Parser.ParseFrom(bytes);
            Assert.AreEqual(msg, copy);
        }
    }
}
