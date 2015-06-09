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
                Values = { NegativeEnum.NEGATIVE_ENUM_ZERO, NegativeEnum.MinusOne, NegativeEnum.FiveBelow },
                PackedValues = { NegativeEnum.NEGATIVE_ENUM_ZERO, NegativeEnum.MinusOne, NegativeEnum.FiveBelow }
            };

            Assert.AreEqual(58, msg.CalculateSize());

            byte[] bytes = new byte[58];
            CodedOutputStream output = CodedOutputStream.CreateInstance(bytes);

            msg.WriteTo(output);
            Assert.AreEqual(0, output.SpaceLeft);

            NegativeEnumMessage copy = NegativeEnumMessage.Parser.ParseFrom(bytes);
            Assert.AreEqual(msg, copy);
        }
    }
}
