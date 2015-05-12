using System;
using System.Reflection;
using UnitTest.Issues.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers
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
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("HasPrimitiveValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("PrimitiveValue"));

            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("HasPrimitiveValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("PrimitiveValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("ClearPrimitiveValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("SetPrimitiveValue"));
        }
        [Test]
        public void TestDepreatedPrimitiveArray()
        {
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("PrimitiveArrayList"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("PrimitiveArrayCount"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetMethod("GetPrimitiveArray"));

            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("PrimitiveArrayList"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("PrimitiveArrayCount"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("GetPrimitiveArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("SetPrimitiveArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("AddPrimitiveArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("AddRangePrimitiveArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("ClearPrimitiveArray"));
        }
        [Test]
        public void TestDepreatedMessageValue()
        {
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("HasMessageValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("MessageValue"));

            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("HasMessageValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("MessageValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("MergeMessageValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("ClearMessageValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("SetMessageValue", new[] { typeof(DeprecatedChild) }));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("SetMessageValue", new[] { typeof(DeprecatedChild.Builder) }));
        }
        [Test]
        public void TestDepreatedMessageArray()
        {
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("MessageArrayList"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("MessageArrayCount"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetMethod("GetMessageArray"));

            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("MessageArrayList"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("MessageArrayCount"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("GetMessageArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("SetMessageArray", new[] { typeof(int), typeof(DeprecatedChild) }));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("SetMessageArray", new[] { typeof(int), typeof(DeprecatedChild.Builder) }));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("AddMessageArray", new[] { typeof(DeprecatedChild) }));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("AddMessageArray", new[] { typeof(DeprecatedChild.Builder) }));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("AddRangeMessageArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("ClearMessageArray"));
        }
        [Test]
        public void TestDepreatedEnumValue()
        {
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("HasEnumValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("EnumValue"));

            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("HasEnumValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("EnumValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("ClearEnumValue"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("SetEnumValue"));
        }
        [Test]
        public void TestDepreatedEnumArray()
        {
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("EnumArrayList"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetProperty("EnumArrayCount"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage).GetMethod("GetEnumArray"));

            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("EnumArrayList"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetProperty("EnumArrayCount"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("GetEnumArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("SetEnumArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("AddEnumArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("AddRangeEnumArray"));
            AssertIsDeprecated(typeof(DeprecatedFieldsMessage.Builder).GetMethod("ClearEnumArray"));
        }
    }
}
