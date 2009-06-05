using System;
using System.Collections.Generic;
using System.Text;
using NUnit.Framework;

namespace Google.ProtocolBuffers.Collections {
  [TestFixture]
  public class PopsicleListTest {

    [Test]
    public void MutatingOperationsOnFrozenList() {
      PopsicleList<string> list = new PopsicleList<string>();
      list.MakeReadOnly();
      AssertNotSupported(() => list.Add(""));
      AssertNotSupported(() => list.Clear());
      AssertNotSupported(() => list.Insert(0, ""));
      AssertNotSupported(() => list.Remove(""));
      AssertNotSupported(() => list.RemoveAt(0));
      AssertNotSupported(() => list.Add(new[] {"", ""}));
    }

    [Test]
    public void NonMutatingOperationsOnFrozenList() {
      PopsicleList<string> list = new PopsicleList<string>();
      list.MakeReadOnly();
      Assert.IsFalse(list.Contains(""));
      Assert.AreEqual(0, list.Count);
      list.CopyTo(new string[5], 0);
      list.GetEnumerator();
      Assert.AreEqual(-1, list.IndexOf(""));
      Assert.IsTrue(list.IsReadOnly);
    }

    [Test]
    public void MutatingOperationsOnFluidList() {
      PopsicleList<string> list = new PopsicleList<string>();
      list.Add("");
      list.Clear();
      list.Insert(0, "");
      list.Remove("");
      list.Add("x"); // Just to make the next call valid
      list.RemoveAt(0);
    }

    [Test]
    public void NonMutatingOperationsOnFluidList() {
      PopsicleList<string> list = new PopsicleList<string>();
      Assert.IsFalse(list.Contains(""));
      Assert.AreEqual(0, list.Count);
      list.CopyTo(new string[5], 0);
      list.GetEnumerator();
      Assert.AreEqual(-1, list.IndexOf(""));
      Assert.IsFalse(list.IsReadOnly);
    }

    private static void AssertNotSupported(Action action) {
      try {
        action();
        Assert.Fail("Expected NotSupportedException");
      } catch (NotSupportedException) {
        // Expected
      }
    }
  }
}
