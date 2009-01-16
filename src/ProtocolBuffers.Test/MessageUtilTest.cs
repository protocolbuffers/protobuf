using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class MessageUtilTest {

    [Test]
    [ExpectedException(typeof(ArgumentNullException))]
    public void NullTypeName() {
      MessageUtil.GetDefaultMessage((string)null);
    }

    [Test]
    [ExpectedException(typeof(ArgumentException))]
    public void InvalidTypeName() {
      MessageUtil.GetDefaultMessage("invalidtypename");
    }

    [Test]
    public void ValidTypeName() {
      Assert.AreSame(TestAllTypes.DefaultInstance, MessageUtil.GetDefaultMessage(typeof(TestAllTypes).AssemblyQualifiedName));
    }

    [Test]
    [ExpectedException(typeof(ArgumentNullException))]
    public void NullType() {
      MessageUtil.GetDefaultMessage((Type)null);
    }

    [Test]
    [ExpectedException(typeof(ArgumentException))]
    public void NonMessageType() {
      MessageUtil.GetDefaultMessage(typeof(string));
    }

    [Test]
    public void ValidType() {
      Assert.AreSame(TestAllTypes.DefaultInstance, MessageUtil.GetDefaultMessage(typeof(TestAllTypes)));
    }
  }
}
