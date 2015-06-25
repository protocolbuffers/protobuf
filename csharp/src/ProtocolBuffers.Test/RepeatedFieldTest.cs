using System;
using System.Collections.Generic;
using Google.Protobuf.Collections;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class RepeatedFieldTest
    {
        [Test]
        public void NullValuesRejected()
        {
            var list = new RepeatedField<string>();
            Assert.Throws<ArgumentNullException>(() => list.Add((string)null));
            Assert.Throws<ArgumentNullException>(() => list.Add((IEnumerable<string>)null));
            Assert.Throws<ArgumentNullException>(() => list.Add((RepeatedField<string>)null));
            Assert.Throws<ArgumentNullException>(() => list.Contains(null));
            Assert.Throws<ArgumentNullException>(() => list.IndexOf(null));
        }

        [Test]
        public void Add_SingleItem()
        {
            var list = new RepeatedField<string>();
            list.Add("foo");
            Assert.AreEqual(1, list.Count);
            Assert.AreEqual("foo", list[0]);
        }

        [Test]
        public void Add_Sequence()
        {
            var list = new RepeatedField<string>();
            list.Add(new[] { "foo", "bar" });
            Assert.AreEqual(2, list.Count);
            Assert.AreEqual("foo", list[0]);
            Assert.AreEqual("bar", list[1]);
        }

        [Test]
        public void Add_RepeatedField()
        {
            var list = new RepeatedField<string> { "original" };
            list.Add(new RepeatedField<string> { "foo", "bar" });
            Assert.AreEqual(3, list.Count);
            Assert.AreEqual("original", list[0]);
            Assert.AreEqual("foo", list[1]);
            Assert.AreEqual("bar", list[2]);
        }

        [Test]
        public void Freeze_FreezesElements()
        {
            var list = new RepeatedField<TestAllTypes> { new TestAllTypes() };
            Assert.IsFalse(list[0].IsFrozen);
            list.Freeze();
            Assert.IsTrue(list[0].IsFrozen);
        }

        [Test]
        public void Freeze_PreventsMutations()
        {
            var list = new RepeatedField<int> { 0 };
            list.Freeze();
            Assert.Throws<InvalidOperationException>(() => list.Add(1));
            Assert.Throws<InvalidOperationException>(() => list[0] = 1);
            Assert.Throws<InvalidOperationException>(() => list.Clear());
            Assert.Throws<InvalidOperationException>(() => list.RemoveAt(0));
            Assert.Throws<InvalidOperationException>(() => list.Remove(0));
            Assert.Throws<InvalidOperationException>(() => list.Insert(0, 0));
        }

        [Test]
        public void Freeze_ReportsFrozen()
        {
            var list = new RepeatedField<int> { 0 };
            Assert.IsFalse(list.IsFrozen);
            Assert.IsFalse(list.IsReadOnly);
            list.Freeze();
            Assert.IsTrue(list.IsFrozen);
            Assert.IsTrue(list.IsReadOnly);
        }

        [Test]
        public void Clone_ReturnsMutable()
        {
            var list = new RepeatedField<int> { 0 };
            list.Freeze();
            var clone = list.Clone();
            clone[0] = 1;
        }
    }
}
