using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.TestProtos;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class GeneratedBuilderTest
    {
        class OneTimeEnumerator<T> : IEnumerable<T>
        {
            readonly T _item;
            bool _enumerated;
            public OneTimeEnumerator(T item)
            {
                _item = item;
            }
            public IEnumerator<T> GetEnumerator()
            {
                Assert.False(_enumerated);
                _enumerated = true;
                yield return _item;
            }
            System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }
        }

        [Fact]
        public void DoesNotEnumerateTwiceForMessageList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedForeignMessage(new OneTimeEnumerator<ForeignMessage>(ForeignMessage.DefaultInstance));
        }

        [Fact]
        public void DoesNotEnumerateTwiceForPrimitiveList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedInt32(new OneTimeEnumerator<int>(1));
        }

        [Fact]
        public void DoesNotEnumerateTwiceForStringList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedString(new OneTimeEnumerator<string>("test"));
        }

        [Fact]
        public void DoesNotEnumerateTwiceForEnumList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedForeignEnum(new OneTimeEnumerator<ForeignEnum>(ForeignEnum.FOREIGN_BAR));
        }
        
        [Fact]
        public void DoesNotAddNullToMessageListByAddRange()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            Assert.Throws<ArgumentNullException>(() => b.AddRangeRepeatedForeignMessage(new ForeignMessage[] { null }));
        }

        [Fact]
        public void DoesNotAddNullToMessageListByAdd()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            Assert.Throws<ArgumentNullException>(() => b.AddRepeatedForeignMessage((ForeignMessage)null));
        }

        [Fact]
        public void DoesNotAddNullToMessageListBySet()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRepeatedForeignMessage(ForeignMessage.DefaultInstance);
            Assert.Throws<ArgumentNullException>(() => b.SetRepeatedForeignMessage(0, (ForeignMessage)null));
        }

        [Fact]
        public void DoesNotAddNullToStringListByAddRange()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            Assert.Throws<ArgumentNullException>(() => b.AddRangeRepeatedString(new String[] { null }));
        }

        [Fact]
        public void DoesNotAddNullToStringListByAdd()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            Assert.Throws<ArgumentNullException>(() => b.AddRepeatedString(null));
        }

        [Fact]
        public void DoesNotAddNullToStringListBySet()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRepeatedString("one");
            Assert.Throws<ArgumentNullException>(() => b.SetRepeatedString(0, null));
        }
    }
}
