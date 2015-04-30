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

using Google.ProtocolBuffers.TestProtos;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class MissingFieldAndExtensionTest
    {
        [Fact]
        public void TestRecoverMissingExtensions()
        {
            const int optionalInt32 = 12345678;
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            builder.SetExtension(Unittest.OptionalInt32Extension, optionalInt32);
            builder.AddExtension(Unittest.RepeatedDoubleExtension, 1.1);
            builder.AddExtension(Unittest.RepeatedDoubleExtension, 1.2);
            builder.AddExtension(Unittest.RepeatedDoubleExtension, 1.3);
            TestAllExtensions msg = builder.Build();

            Assert.True(msg.HasExtension(Unittest.OptionalInt32Extension));
            Assert.Equal(3, msg.GetExtensionCount(Unittest.RepeatedDoubleExtension));

            byte[] bits = msg.ToByteArray();
            TestAllExtensions copy = TestAllExtensions.ParseFrom(bits);
            Assert.False(copy.HasExtension(Unittest.OptionalInt32Extension));
            Assert.Equal(0, copy.GetExtensionCount(Unittest.RepeatedDoubleExtension));
            Assert.NotEqual(msg, copy);

            //Even though copy does not understand the typees they serialize correctly
            byte[] copybits = copy.ToByteArray();
            Assert.Equal(bits, copybits);

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            Unittest.RegisterAllExtensions(registry);

            //Now we can take those copy bits and restore the full message with extensions
            copy = TestAllExtensions.ParseFrom(copybits, registry);
            Assert.True(copy.HasExtension(Unittest.OptionalInt32Extension));
            Assert.Equal(3, copy.GetExtensionCount(Unittest.RepeatedDoubleExtension));

            Assert.Equal(msg, copy);
            Assert.Equal(bits, copy.ToByteArray());

            //If we modify the object this should all continue to work as before
            copybits = copy.ToBuilder().Build().ToByteArray();
            Assert.Equal(bits, copybits);

            //If we replace extension the object this should all continue to work as before
            copybits = copy.ToBuilder()
                .SetExtension(Unittest.OptionalInt32Extension, optionalInt32)
                .Build().ToByteArray();
            Assert.Equal(bits, copybits);
        }

        [Fact]
        public void TestRecoverMissingFields()
        {
            TestMissingFieldsA msga = TestMissingFieldsA.CreateBuilder()
                .SetId(1001)
                .SetName("Name")
                .SetEmail("missing@field.value")
                .Build();

            //serialize to type B and verify all fields exist
            TestMissingFieldsB msgb = TestMissingFieldsB.ParseFrom(msga.ToByteArray());
            Assert.Equal(1001, msgb.Id);
            Assert.Equal("Name", msgb.Name);
            Assert.False(msgb.HasWebsite);
            Assert.Equal(1, msgb.UnknownFields.FieldDictionary.Count);
            Assert.Equal("missing@field.value",
                            msgb.UnknownFields[TestMissingFieldsA.EmailFieldNumber].LengthDelimitedList[0].ToStringUtf8());

            //serializes exactly the same (at least for this simple example)
            Assert.Equal(msga.ToByteArray(), msgb.ToByteArray());
            Assert.Equal(msga, TestMissingFieldsA.ParseFrom(msgb.ToByteArray()));

            //now re-create an exact copy of A from serialized B
            TestMissingFieldsA copya = TestMissingFieldsA.ParseFrom(msgb.ToByteArray());
            Assert.Equal(msga, copya);
            Assert.Equal(1001, copya.Id);
            Assert.Equal("Name", copya.Name);
            Assert.Equal("missing@field.value", copya.Email);

            //Now we modify B... and try again
            msgb = msgb.ToBuilder().SetWebsite("http://new.missing.field").Build();
            //Does B still have the missing field?
            Assert.Equal(1, msgb.UnknownFields.FieldDictionary.Count);

            //Convert back to A and see if all fields are there?
            copya = TestMissingFieldsA.ParseFrom(msgb.ToByteArray());
            Assert.NotEqual(msga, copya);
            Assert.Equal(1001, copya.Id);
            Assert.Equal("Name", copya.Name);
            Assert.Equal("missing@field.value", copya.Email);
            Assert.Equal(1, copya.UnknownFields.FieldDictionary.Count);
            Assert.Equal("http://new.missing.field",
                            copya.UnknownFields[TestMissingFieldsB.WebsiteFieldNumber].LengthDelimitedList[0].
                                ToStringUtf8());

            //Lastly we can even still trip back to type B and see all fields:
            TestMissingFieldsB copyb = TestMissingFieldsB.ParseFrom(copya.ToByteArray());
            Assert.Equal(copya.ToByteArray().Length, copyb.ToByteArray().Length); //not exact order.
            Assert.Equal(1001, copyb.Id);
            Assert.Equal("Name", copyb.Name);
            Assert.Equal("http://new.missing.field", copyb.Website);
            Assert.Equal(1, copyb.UnknownFields.FieldDictionary.Count);
            Assert.Equal("missing@field.value",
                            copyb.UnknownFields[TestMissingFieldsA.EmailFieldNumber].LengthDelimitedList[0].ToStringUtf8
                                ());
        }

        [Fact]
        public void TestRecoverMissingMessage()
        {
            TestMissingFieldsA.Types.SubA suba =
                TestMissingFieldsA.Types.SubA.CreateBuilder().SetCount(3).AddValues("a").AddValues("b").AddValues("c").
                    Build();
            TestMissingFieldsA msga = TestMissingFieldsA.CreateBuilder()
                .SetId(1001)
                .SetName("Name")
                .SetTestA(suba)
                .Build();

            //serialize to type B and verify all fields exist
            TestMissingFieldsB msgb = TestMissingFieldsB.ParseFrom(msga.ToByteArray());
            Assert.Equal(1001, msgb.Id);
            Assert.Equal("Name", msgb.Name);
            Assert.Equal(1, msgb.UnknownFields.FieldDictionary.Count);
            Assert.Equal(suba.ToString(),
                            TestMissingFieldsA.Types.SubA.ParseFrom(
                                msgb.UnknownFields[TestMissingFieldsA.TestAFieldNumber].LengthDelimitedList[0]).ToString
                                ());

            //serializes exactly the same (at least for this simple example)
            Assert.Equal(msga.ToByteArray(), msgb.ToByteArray());
            Assert.Equal(msga, TestMissingFieldsA.ParseFrom(msgb.ToByteArray()));

            //now re-create an exact copy of A from serialized B
            TestMissingFieldsA copya = TestMissingFieldsA.ParseFrom(msgb.ToByteArray());
            Assert.Equal(msga, copya);
            Assert.Equal(1001, copya.Id);
            Assert.Equal("Name", copya.Name);
            Assert.Equal(suba, copya.TestA);

            //Now we modify B... and try again
            TestMissingFieldsB.Types.SubB subb =
                TestMissingFieldsB.Types.SubB.CreateBuilder().AddValues("test-b").Build();
            msgb = msgb.ToBuilder().SetTestB(subb).Build();
            //Does B still have the missing field?
            Assert.Equal(1, msgb.UnknownFields.FieldDictionary.Count);

            //Convert back to A and see if all fields are there?
            copya = TestMissingFieldsA.ParseFrom(msgb.ToByteArray());
            Assert.NotEqual(msga, copya);
            Assert.Equal(1001, copya.Id);
            Assert.Equal("Name", copya.Name);
            Assert.Equal(suba, copya.TestA);
            Assert.Equal(1, copya.UnknownFields.FieldDictionary.Count);
            Assert.Equal(subb.ToByteArray(),
                            copya.UnknownFields[TestMissingFieldsB.TestBFieldNumber].LengthDelimitedList[0].ToByteArray());

            //Lastly we can even still trip back to type B and see all fields:
            TestMissingFieldsB copyb = TestMissingFieldsB.ParseFrom(copya.ToByteArray());
            Assert.Equal(copya.ToByteArray().Length, copyb.ToByteArray().Length); //not exact order.
            Assert.Equal(1001, copyb.Id);
            Assert.Equal("Name", copyb.Name);
            Assert.Equal(subb, copyb.TestB);
            Assert.Equal(1, copyb.UnknownFields.FieldDictionary.Count);
        }

        [Fact]
        public void TestRestoreFromOtherType()
        {
            TestInteropPerson person = TestInteropPerson.CreateBuilder()
                .SetId(123)
                .SetName("abc")
                .SetEmail("abc@123.com")
                .AddRangeCodes(new[] {1, 2, 3})
                .AddPhone(TestInteropPerson.Types.PhoneNumber.CreateBuilder().SetNumber("555-1234").Build())
                .AddPhone(TestInteropPerson.Types.PhoneNumber.CreateBuilder().SetNumber("555-5678").Build())
                .AddAddresses(
                    TestInteropPerson.Types.Addresses.CreateBuilder().SetAddress("123 Seseme").SetCity("Wonderland").
                        SetState("NA").SetZip(12345).Build())
                .SetExtension(UnittestExtrasFull.EmployeeId,
                              TestInteropEmployeeId.CreateBuilder().SetNumber("123").Build())
                .Build();
            Assert.True(person.IsInitialized);

            TestEmptyMessage temp = TestEmptyMessage.ParseFrom(person.ToByteArray());
            Assert.Equal(7, temp.UnknownFields.FieldDictionary.Count);
            temp = temp.ToBuilder().Build();
            Assert.Equal(7, temp.UnknownFields.FieldDictionary.Count);

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestExtrasFull.RegisterAllExtensions(registry);

            TestInteropPerson copy = TestInteropPerson.ParseFrom(temp.ToByteArray(), registry);
            Assert.Equal(person, copy);
            Assert.Equal(person.ToByteArray(), copy.ToByteArray());
        }
    }
}