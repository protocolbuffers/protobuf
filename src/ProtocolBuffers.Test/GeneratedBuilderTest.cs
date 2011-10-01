using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers
{
    [TestFixture]
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
                Assert.IsFalse(_enumerated, "The collection {0} has already been enumerated", GetType());
                _enumerated = true;
                yield return _item;
            }
            System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
            { return GetEnumerator(); }
        }

        [Test]
        public void DoesNotEnumerateTwiceForMessageList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedForeignMessage(
                new OneTimeEnumerator<ForeignMessage>(
                    ForeignMessage.DefaultInstance));
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

        private static void AssertThrows<T>(System.Threading.ThreadStart method) where T : Exception
        {
            try
            {
                method();
            }
            catch (Exception error)
            {
                if (error is T)
                    return;
                throw;
            }
            Assert.Fail("Expected exception of type " + typeof(T));
        }

        [Test]
        public void DoesNotAddNullToMessageListByAddRange()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            AssertThrows<ArgumentNullException>(
                () => b.AddRangeRepeatedForeignMessage(new ForeignMessage[] { null })
                    );
        }
        [Test]
        public void DoesNotAddNullToMessageListByAdd()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            AssertThrows<ArgumentNullException>(
                () => b.AddRepeatedForeignMessage((ForeignMessage)null)
                    );
        }
        [Test]
        public void DoesNotAddNullToMessageListBySet()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRepeatedForeignMessage(ForeignMessage.DefaultInstance);
            AssertThrows<ArgumentNullException>(
                () => b.SetRepeatedForeignMessage(0, (ForeignMessage)null)
                    );
        }
        [Test]
        public void DoesNotAddNullToStringListByAddRange()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            AssertThrows<ArgumentNullException>(
                () => b.AddRangeRepeatedString(new String[] { null })
                    );
        }
        [Test]
        public void DoesNotAddNullToStringListByAdd()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            AssertThrows<ArgumentNullException>(
                () => b.AddRepeatedString(null)
                    );
        }
        [Test]
        public void DoesNotAddNullToStringListBySet()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRepeatedString("one");
            AssertThrows<ArgumentNullException>(
                () => b.SetRepeatedString(0, null)
                    );
        }
    }
}
