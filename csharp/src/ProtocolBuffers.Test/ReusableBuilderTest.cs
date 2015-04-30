using System;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.TestProtos;
using UnitTest.Issues.TestProtos;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class ReusableBuilderTest
    {
        //Issue 28: Circular message dependencies result in null defaults for DefaultInstance
        [Fact]
        public void EnsureStaticCicularReference()
        {
            MyMessageAReferenceB ab = MyMessageAReferenceB.DefaultInstance;
            Assert.NotNull(ab);
            Assert.NotNull(ab.Value);
            MyMessageBReferenceA ba = MyMessageBReferenceA.DefaultInstance;
            Assert.NotNull(ba);
            Assert.NotNull(ba.Value);
        }

        [Fact]
        public void TestModifyDefaultInstance()
        {
            //verify that the default instance has correctly been marked as read-only
            Assert.Equal(typeof(PopsicleList<bool>), TestAllTypes.DefaultInstance.RepeatedBoolList.GetType());
            PopsicleList<bool> list = (PopsicleList<bool>)TestAllTypes.DefaultInstance.RepeatedBoolList;
            Assert.True(list.IsReadOnly);
        }

        [Fact]
        public void TestUnmodifiedDefaultInstance()
        {
            //Simply calling ToBuilder().Build() no longer creates a copy of the message
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void BuildMultipleWithoutChange()
        {
            //Calling Build() or BuildPartial() does not require a copy of the message
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            builder.SetDefaultBool(true);

            TestAllTypes first = builder.BuildPartial();
            //Still the same instance?
            Assert.True(ReferenceEquals(first, builder.Build()));
            //Still the same instance?
            Assert.True(ReferenceEquals(first, builder.BuildPartial().ToBuilder().Build()));
        }

        [Fact]
        public void MergeFromDefaultInstance()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            builder.MergeFrom(TestAllTypes.DefaultInstance);
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void BuildNewBuilderIsDefaultInstance()
        {
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, new TestAllTypes.Builder().Build()));
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, TestAllTypes.CreateBuilder().Build()));
            //last test, if you clear a builder it reverts to default instance
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance,
                TestAllTypes.CreateBuilder().SetOptionalBool(true).Build().ToBuilder().Clear().Build()));
        }

        [Fact]
        public void BuildModifyAndRebuild()
        {
            TestAllTypes.Builder b1 = new TestAllTypes.Builder();
            b1.SetDefaultInt32(1);
            b1.AddRepeatedInt32(2);
            b1.SetOptionalForeignMessage(ForeignMessage.DefaultInstance);

            TestAllTypes m1 = b1.Build();

            b1.SetDefaultInt32(5);
            b1.AddRepeatedInt32(6);
            b1.SetOptionalForeignMessage(b1.OptionalForeignMessage.ToBuilder().SetC(7));

            TestAllTypes m2 = b1.Build();
            
            Assert.Equal("{\"optional_foreign_message\":{},\"repeated_int32\":[2],\"default_int32\":1}", Extensions.ToJson(m1));
            Assert.Equal("{\"optional_foreign_message\":{\"c\":7},\"repeated_int32\":[2,6],\"default_int32\":5}", Extensions.ToJson(m2));
        }

        [Fact]
        public void CloneOnChangePrimitive()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            builder.SetDefaultBool(true);
            Assert.False(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void CloneOnAddRepeatedBool()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            builder.AddRepeatedBool(true);
            Assert.False(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void CloneOnGetRepeatedBoolList()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            GC.KeepAlive(builder.RepeatedBoolList);
            Assert.False(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void CloneOnChangeMessage()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            builder.SetOptionalForeignMessage(new ForeignMessage.Builder());
            Assert.False(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void CloneOnClearMessage()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            builder.ClearOptionalForeignMessage();
            Assert.False(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void CloneOnGetRepeatedForeignMessageList()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            GC.KeepAlive(builder.RepeatedForeignMessageList);
            Assert.False(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void CloneOnChangeEnumValue()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            builder.SetOptionalForeignEnum(ForeignEnum.FOREIGN_BAR);
            Assert.False(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

        [Fact]
        public void CloneOnGetRepeatedForeignEnumList()
        {
            TestAllTypes.Builder builder = TestAllTypes.DefaultInstance.ToBuilder();
            Assert.True(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
            GC.KeepAlive(builder.RepeatedForeignEnumList);
            Assert.False(ReferenceEquals(TestAllTypes.DefaultInstance, builder.Build()));
        }

    }
}
