using System;
using System.Reflection;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class DeprecatedMemberTest
    {
        private static void AssertIsDeprecated(MemberInfo member)
        {
            Assert.NotNull(member);
            Assert.IsTrue(member.IsDefined(typeof(ObsoleteAttribute), false), "Member not obsolete: " + member);
        }

        [Test]
        public void TestDepreatedPrimitiveValue()
        {
            AssertIsDeprecated(typeof(TestDeprecatedFields).GetProperty("DeprecatedInt32"));
        }

    }
}
