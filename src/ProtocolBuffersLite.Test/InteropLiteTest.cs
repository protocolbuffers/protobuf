#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#endregion

using System;
using System.Collections.Generic;
using Google.ProtocolBuffers;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class InteropLiteTest {

    [Test]
    public void TestConvertFromFullMinimal() {
      TestInteropPerson person = TestInteropPerson.CreateBuilder()
        .SetId(123)
        .SetName("abc")
        .Build();
      Assert.IsTrue(person.IsInitialized);

      TestInteropPersonLite copy = TestInteropPersonLite.ParseFrom(person.ToByteArray());

      Assert.AreEqual(person.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestConvertFromFullComplete() {
      TestInteropPerson person = TestInteropPerson.CreateBuilder()
        .SetId(123)
        .SetName("abc")
        .SetEmail("abc@123.com")
        .AddRangeCodes(new[] { 1, 2, 3 })
        .AddPhone(TestInteropPerson.Types.PhoneNumber.CreateBuilder().SetNumber("555-1234").Build())
        .AddPhone(TestInteropPerson.Types.PhoneNumber.CreateBuilder().SetNumber("555-5678").Build())
        .AddAddresses(TestInteropPerson.Types.Addresses.CreateBuilder().SetAddress("123 Seseme").SetCity("Wonderland").SetState("NA").SetZip(12345).Build())
        .SetExtension(UnitTestExtrasFullProtoFile.EmployeeId, TestInteropEmployeeId.CreateBuilder().SetNumber("123").Build())
        .Build();
      Assert.IsTrue(person.IsInitialized);

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      UnitTestExtrasLiteProtoFile.RegisterAllExtensions(registry);

      TestInteropPersonLite copy = TestInteropPersonLite.ParseFrom(person.ToByteArray(), registry);

      Assert.AreEqual(person.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestConvertFromLiteMinimal() {
      TestInteropPersonLite person = TestInteropPersonLite.CreateBuilder()
        .SetId(123)
        .SetName("abc")
        .Build();
      Assert.IsTrue(person.IsInitialized);

      TestInteropPerson copy = TestInteropPerson.ParseFrom(person.ToByteArray());

      Assert.AreEqual(person.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestConvertFromLiteComplete() {
      TestInteropPersonLite person = TestInteropPersonLite.CreateBuilder()
        .SetId(123)
        .SetName("abc")
        .SetEmail("abc@123.com")
        .AddRangeCodes(new[] { 1, 2, 3 })
        .AddPhone(TestInteropPersonLite.Types.PhoneNumber.CreateBuilder().SetNumber("555-1234").Build())
        .AddPhone(TestInteropPersonLite.Types.PhoneNumber.CreateBuilder().SetNumber("555-5678").Build())
        .AddAddresses(TestInteropPersonLite.Types.Addresses.CreateBuilder().SetAddress("123 Seseme").SetCity("Wonderland").SetState("NA").SetZip(12345).Build())
        .SetExtension(UnitTestExtrasLiteProtoFile.EmployeeIdLite, TestInteropEmployeeIdLite.CreateBuilder().SetNumber("123").Build())
        .Build();
      Assert.IsTrue(person.IsInitialized);

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      UnitTestExtrasFullProtoFile.RegisterAllExtensions(registry);

      TestInteropPerson copy = TestInteropPerson.ParseFrom(person.ToByteArray(), registry);

      Assert.AreEqual(person.ToByteArray(), copy.ToByteArray());
    }

    public ByteString AllBytes {
      get {
        byte[] bytes = new byte[256];
        for (int i = 0; i < bytes.Length; i++)
          bytes[i] = (byte)i;
        return ByteString.CopyFrom(bytes);
      }
    }

      [Test]
    public void TestCompareStringValues() {
      TestInteropPersonLite person = TestInteropPersonLite.CreateBuilder()
        .SetId(123)
        .SetName("abc")
        .SetEmail("abc@123.com")
        .AddRangeCodes(new[] { 1, 2, 3 })
        .AddPhone(TestInteropPersonLite.Types.PhoneNumber.CreateBuilder().SetNumber("555-1234").Build())
        .AddPhone(TestInteropPersonLite.Types.PhoneNumber.CreateBuilder().SetNumber(System.Text.Encoding.ASCII.GetString(AllBytes.ToByteArray())).Build())
        .AddAddresses(TestInteropPersonLite.Types.Addresses.CreateBuilder().SetAddress("123 Seseme").SetCity("Wonderland").SetState("NA").SetZip(12345).Build())
        .SetExtension(UnitTestExtrasLiteProtoFile.EmployeeIdLite, TestInteropEmployeeIdLite.CreateBuilder().SetNumber("123").Build())
        .Build();
      Assert.IsTrue(person.IsInitialized);

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      UnitTestExtrasFullProtoFile.RegisterAllExtensions(registry);

      TestInteropPerson copy = TestInteropPerson.ParseFrom(person.ToByteArray(), registry);

      Assert.AreEqual(person.ToByteArray(), copy.ToByteArray());

      TestInteropPerson.Builder copyBuilder = TestInteropPerson.CreateBuilder();
      TextFormat.Merge(person.ToString().Replace("[protobuf_unittest_extra.employee_id_lite]", "[protobuf_unittest_extra.employee_id]"), registry, copyBuilder);

      copy = copyBuilder.Build();
      Assert.AreEqual(person.ToByteArray(), copy.ToByteArray());

      string liteText = person.ToString().TrimEnd().Replace("\r", "");
      string fullText = copy.ToString().TrimEnd().Replace("\r", "");
      //map the extension type
      liteText = liteText.Replace("[protobuf_unittest_extra.employee_id_lite]", "[protobuf_unittest_extra.employee_id]");
      //lite version does not indent
      while (fullText.IndexOf("\n ") >= 0)
        fullText = fullText.Replace("\n ", "\n");

      Assert.AreEqual(fullText, liteText);
    }
  }
}
