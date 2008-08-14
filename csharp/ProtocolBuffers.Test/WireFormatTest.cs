using System;
using System.Collections.Generic;
using System.Text;
using NUnit.Framework;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class WireFormatTest {

    [Test]
    public void FieldTypeToWireTypeMapping() {

      // Just test a few values
      Assert.AreEqual(WireFormat.WireType.Fixed64, WireFormat.FieldTypeToWireFormatMap[FieldType.SFixed64]);
      Assert.AreEqual(WireFormat.WireType.LengthDelimited, WireFormat.FieldTypeToWireFormatMap[FieldType.String]);
      Assert.AreEqual(WireFormat.WireType.LengthDelimited, WireFormat.FieldTypeToWireFormatMap[FieldType.Message]);
    }
  }
}
