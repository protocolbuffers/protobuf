using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

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
                Assert.IsFalse(_enumerated);
                _enumerated = true;
                yield return _item;
            }
            System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }
        }

        [Test]
        public void DoesNotEnumerateTwiceForMessageList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedForeignMessage(new OneTimeEnumerator<ForeignMessage>(ForeignMessage.DefaultInstance));
        }

        [Test]
        public void DoesNotEnumerateTwiceForPrimitiveList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedInt32(new OneTimeEnumerator<int>(1));
        }

        [Test]
        public void DoesNotEnumerateTwiceForStringList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedString(new OneTimeEnumerator<string>("test"));
        }

        [Test]
        public void DoesNotEnumerateTwiceForEnumList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedForeignEnum(new OneTimeEnumerator<ForeignEnum>(ForeignEnum.FOREIGN_BAR));
        }
        
        [Test]
        public void DoesNotAddNullToMessageListByAddRange()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            Assert.Throws<ArgumentNullException>(() => b.AddRangeRepeatedForeignMessage(new ForeignMessage[] { null }));
        }

        [Test]
        public void DoesNotAddNullToMessageListByAdd()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            Assert.Throws<ArgumentNullException>(() => b.AddRepeatedForeignMessage((ForeignMessage)null));
        }

        [Test]
        public void DoesNotAddNullToMessageListBySet()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRepeatedForeignMessage(ForeignMessage.DefaultInstance);
            Assert.Throws<ArgumentNullException>(() => b.SetRepeatedForeignMessage(0, (ForeignMessage)null));
        }

        [Test]
        public void DoesNotAddNullToStringListByAddRange()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            Assert.Throws<ArgumentNullException>(() => b.AddRangeRepeatedString(new String[] { null }));
        }

        [Test]
        public void DoesNotAddNullToStringListByAdd()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            Assert.Throws<ArgumentNullException>(() => b.AddRepeatedString(null));
        }

        [Test]
        public void DoesNotAddNullToStringListBySet()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRepeatedString("one");
            Assert.Throws<ArgumentNullException>(() => b.SetRepeatedString(0, null));
        }
    }
}
