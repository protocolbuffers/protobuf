#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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
