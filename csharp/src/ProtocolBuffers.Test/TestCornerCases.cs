using UnitTest.Issues.TestProtos;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class TestCornerCases
    {
        [Fact]
        public void TestRoundTripNegativeEnums()
        {
            NegativeEnumMessage msg = NegativeEnumMessage.CreateBuilder()
                .SetValue(NegativeEnum.MinusOne) //11
                .AddValues(NegativeEnum.Zero) //2
                .AddValues(NegativeEnum.MinusOne) //11
                .AddValues(NegativeEnum.FiveBelow) //11
                //2
                .AddPackedValues(NegativeEnum.Zero) //1
                .AddPackedValues(NegativeEnum.MinusOne) //10
                .AddPackedValues(NegativeEnum.FiveBelow) //10
                .Build();

            Assert.Equal(58, msg.SerializedSize);

            byte[] bytes = new byte[58];
            CodedOutputStream output = CodedOutputStream.CreateInstance(bytes);

            msg.WriteTo(output);
            Assert.Equal(0, output.SpaceLeft);

            NegativeEnumMessage copy = NegativeEnumMessage.ParseFrom(bytes);
            Assert.Equal(msg, copy);
        }
    }
}
