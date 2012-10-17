using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Google.ProtocolBuffers
{
    [TestClass]
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

        [TestMethod]
        public void DoesNotEnumerateTwiceForMessageList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedForeignMessage(
                new OneTimeEnumerator<ForeignMessage>(
                    ForeignMessage.DefaultInstance));
        }
        [TestMethod]
        public void DoesNotEnumerateTwiceForPrimitiveList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedInt32(new OneTimeEnumerator<int>(1));
        }
        [TestMethod]
        public void DoesNotEnumerateTwiceForStringList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedString(new OneTimeEnumerator<string>("test"));
        }
        [TestMethod]
        public void DoesNotEnumerateTwiceForEnumList()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRangeRepeatedForeignEnum(new OneTimeEnumerator<ForeignEnum>(ForeignEnum.FOREIGN_BAR));
        }

        private delegate void TestMethod();

        private static void AssertThrows<T>(TestMethod method) where T : Exception
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

        [TestMethod]
        public void DoesNotAddNullToMessageListByAddRange()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            AssertThrows<ArgumentNullException>(
                () => b.AddRangeRepeatedForeignMessage(new ForeignMessage[] { null })
                    );
        }
        [TestMethod]
        public void DoesNotAddNullToMessageListByAdd()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            AssertThrows<ArgumentNullException>(
                () => b.AddRepeatedForeignMessage((ForeignMessage)null)
                    );
        }
        [TestMethod]
        public void DoesNotAddNullToMessageListBySet()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            b.AddRepeatedForeignMessage(ForeignMessage.DefaultInstance);
            AssertThrows<ArgumentNullException>(
                () => b.SetRepeatedForeignMessage(0, (ForeignMessage)null)
                    );
        }
        [TestMethod]
        public void DoesNotAddNullToStringListByAddRange()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            AssertThrows<ArgumentNullException>(
                () => b.AddRangeRepeatedString(new String[] { null })
                    );
        }
        [TestMethod]
        public void DoesNotAddNullToStringListByAdd()
        {
            TestAllTypes.Builder b = new TestAllTypes.Builder();
            AssertThrows<ArgumentNullException>(
                () => b.AddRepeatedString(null)
                    );
        }
        [TestMethod]
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
