using Google.Protobuf.TestProtos;
using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Google.Protobuf.Reflection
{
    public class DynamicMessageTest
    {



        [Test]
        public void TestDynamicMessageUsage()
        {
            string fileName = "testFile.txt";
            MessageDescriptor desc = TestAllTypes.Descriptor;

            //create 
            DynamicMessage dm = new DynamicMessage(desc);
            dm.Add("single_string", "sss");
            //serialize
            //Stream output = WriteTo(dm);
            using (var output = File.Create(fileName))
            {
                dm.WriteTo(output);
            }
            //deserialize
            DynamicMessage deserializedDm;
            DynamicMessage tempdm = new DynamicMessage(desc);
            using (var input = File.OpenRead(fileName))
            {
                deserializedDm = (DynamicMessage) tempdm.Parser.ParseFrom(input);
            }

        }


        [Test]
        public void TestDynamicMessageParsingSingleString()
        {
            string val = "str1";
            TestAllTypes msg = new()
            {
                SingleString = val
            };

            ByteString byteStr = msg.ToByteString();
            MessageDescriptor desc = TestAllTypes.Descriptor;
            FieldDescriptor fd = desc.FindFieldByName("single_string");
            DynamicMessage res = ParseFrom(desc, byteStr);
            Assert.NotNull(res);
            Assert.AreEqual(val, res.FieldSet.GetField(fd));
        }

        public DynamicMessage ParseFrom(MessageDescriptor type, ByteString data)
        {
            DynamicMessage dynMsg = new DynamicMessage(type);
            dynMsg = (DynamicMessage) dynMsg.Parser.ParseFrom(data);
            return dynMsg;
        }

        [Test]
        public void TestDynamicMessageParseFromAllTypes()
        {
            TestAllTypes message = GetAllTypesMessage();
            MessageDescriptor desc = TestAllTypes.Descriptor;
            ByteString byteStr = message.ToByteString();
            DynamicMessage res = ParseFrom(desc, byteStr);
            Assertions(desc, res);
        }

        [Test]
        public void TestDynamicMessageWriteTo()
        {
            MessageDescriptor desc = TestAllTypes.Descriptor;
            DynamicMessage dm = new DynamicMessage(desc);
            string fieldName = "single_string";
            string fieldValue = "sss";
            dm.Add(fieldName, fieldValue);

            Stream stream = WriteTo(dm);

            var input = new CodedInputStream(stream);
            int fieldNumber = WireFormat.GetTagFieldNumber(input.ReadTag());
            Assert.AreEqual(desc.FindFieldByNumber(fieldNumber).Name, fieldName);
            Assert.AreEqual(fieldValue, input.ReadString());
        }



        [Test]
        public void TestDynamicMessageSetterRejectsNull()
        {
            MessageDescriptor desc = TestAllTypes.Descriptor;
            DynamicMessage dm = new DynamicMessage(desc);
            var exc = Assert.Throws<ArgumentNullException>(() => dm.FieldSet.SetField(desc.FindFieldByName("single_string"), null));
            Assert.That(exc.ParamName, Is.EqualTo("single_string"));
        }

        [Test]
        public void TestDynamicMessageSerializedSize()
        {
            var message = GetAllTypesMessage();
            MessageDescriptor desc = TestAllTypes.Descriptor;
            ByteString byteStr = message.ToByteString();
            DynamicMessage dm = ParseFrom(desc, byteStr);
            Assert.AreEqual(1386, dm.CalculateSize());
        }


        [Test]
        public void TestUnknownField()
        {

            TestAllTypes testAllTypes = new TestAllTypes
            {
                SingleString = "test"
            };

            var fdProto = new FieldDescriptorProto()
            {
                Name = "single_string2",
                Type = FieldDescriptorProto.Types.Type.String,
                Number = 1,
                Label = FieldDescriptorProto.Types.Label.Optional

            };
            List<FieldDescriptorProto> fields = new List<FieldDescriptorProto>();
            fields.Add(fdProto);
            MessageDescriptor desc = GetMessageDescriptor("Test", fields);

            DynamicMessage dm = (DynamicMessage) new DynamicMessage(desc).Parser.ParseFrom(testAllTypes.ToByteArray());

            Stream stream = WriteTo(dm);

            var input = new CodedInputStream(stream);
            uint tag = input.ReadTag();
            int fieldNumber = WireFormat.GetTagFieldNumber(tag);
            Assert.AreEqual(fieldNumber, 14);
            Assert.AreEqual("test", input.ReadString());
        }


        private static MessageDescriptor GetMessageDescriptor(string baseFieldName, List<FieldDescriptorProto> fields)
        {
            var fileDescProto = new FileDescriptorProto();
            fileDescProto.Name = baseFieldName;
            var descP = new DescriptorProto();
            descP.Name = baseFieldName;
            foreach (FieldDescriptorProto fdProto in fields)
            {
                descP.Field.Add(fdProto);
            }
            fileDescProto.MessageType.Add(descP);
            return ConvertToDescriptor(fileDescProto, baseFieldName);
        }
        private static MessageDescriptor ConvertToDescriptor(FileDescriptorProto fileDescProto, string baseFieldName)
        {
            var fileDescSet = new FileDescriptorSet();
            fileDescSet.File.Add(fileDescProto);
            var byteStrings = fileDescSet.File.Select(f => f.ToByteString()).ToList();
            IReadOnlyList<FileDescriptor> descriptors = FileDescriptor.BuildFromByteStrings(byteStrings);
            //In the descriptors only 1 element is added.
            return descriptors.First().FindTypeByName<MessageDescriptor>(baseFieldName);
        }

        private static Stream WriteTo(DynamicMessage dm)
        {
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            dm.WriteTo(output);
            output.Flush();
            stream.Position = 0;// TODO check if reqd
            return stream;
        }

        private static TestAllTypes GetAllTypesMessage()
        {
            return new TestAllTypes
            {
                SingleBool = true,
                SingleBytes = ByteString.CopyFrom(1, 2, 3, 4),
                SingleDouble = 23.5,
                SingleFixed32 = 23,
                SingleFixed64 = 1234567890123,
                SingleFloat = 12.25f,
                SingleForeignEnum = ForeignEnum.ForeignBar,
                SingleForeignMessage = new ForeignMessage { C = 10 },
                SingleImportEnum = ImportEnum.ImportBaz,
                SingleImportMessage = new ImportMessage { D = 20 },
                SingleInt32 = 100,
                SingleInt64 = 3210987654321,
                SingleNestedEnum = TestAllTypes.Types.NestedEnum.Foo,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 35 },
                SinglePublicImportMessage = new PublicImportMessage { E = 54 },
                SingleSfixed32 = -123,
                SingleSfixed64 = -12345678901234,
                SingleSint32 = -456,
                SingleSint64 = -12345678901235,
                SingleString = "test",
                SingleUint32 = UInt32.MaxValue,
                SingleUint64 = UInt64.MaxValue,
                RepeatedBytes = { ByteString.CopyFrom(1, 2, 3, 4), ByteString.CopyFrom(5, 6), ByteString.CopyFrom(new byte[1000]) },
                RepeatedForeignMessage = { new ForeignMessage(), new ForeignMessage { C = 10 } },
                RepeatedImportMessage = { new ImportMessage { D = 20 }, new ImportMessage { D = 25 } },
                RepeatedNestedMessage = { new TestAllTypes.Types.NestedMessage { Bb = 35 }, new TestAllTypes.Types.NestedMessage { Bb = 10 } },
                RepeatedPublicImportMessage = { new PublicImportMessage { E = 54 }, new PublicImportMessage { E = -1 } },
                RepeatedString = { "foo", "bar" },
                OneofString = "Oneof string",
                RepeatedBool = { true, false },
                RepeatedSfixed32 = { -123, 123 },
                RepeatedSfixed64 = { -12345678901234, 12345678901234 },
                RepeatedSint32 = { -456, 100 },
                RepeatedSint64 = { -12345678901235, 123 },
                RepeatedInt32 = { 100, 200 },
                RepeatedInt64 = { 3210987654321, Int64.MaxValue },
                RepeatedDouble = { -12.25, 23.5 },
                RepeatedFixed32 = { UInt32.MaxValue, 23 },
                RepeatedFixed64 = { UInt64.MaxValue, 1234567890123 },
                RepeatedFloat = { 100f, 12.25f },
                RepeatedUint32 = { UInt32.MaxValue, UInt32.MinValue },
                RepeatedUint64 = { UInt64.MaxValue, UInt64.MinValue }
            };
        }

        private static void Assertions(MessageDescriptor desc, DynamicMessage res)
        {
            Assert.NotNull(res);
            Assert.AreEqual("test", GetField(desc, res, "single_string"));
            Assert.AreEqual(100, GetField(desc, res, "repeated_int32[0]"));
            Assert.AreEqual(4294967295, GetField(desc, res, "repeated_fixed32[0]"));
            Assert.AreEqual(10, GetField(desc, res, "single_foreign_message.c"));
            Assert.AreEqual(20, GetField(desc, res, "single_import_message.d"));
            Assert.AreEqual((int) TestAllTypes.Types.NestedEnum.Foo, GetField(desc, res, "single_nested_enum"));
            Assert.AreEqual(54, GetField(desc, res, "single_public_import_message.e"));
            Assert.AreEqual((int) ForeignEnum.ForeignBar, GetField(desc, res, "single_foreign_enum"));
            Assert.AreEqual(ByteString.CopyFrom(1, 2, 3, 4), GetField(desc, res, "repeated_bytes[0]"));
            Assert.AreEqual(ByteString.CopyFrom(5, 6), GetField(desc, res, "repeated_bytes[1]"));
            Assert.AreEqual(10, GetField(desc, res, "repeated_foreign_message[1].c"));
            Assert.AreEqual(null, GetField(desc, res, "repeated_foreign_message[0].c"));
            Assert.AreEqual(10, GetField(desc, res, "repeated_foreign_message[1].c"));
            Assert.AreEqual(20, GetField(desc, res, "repeated_import_message[0].d"));
            Assert.AreEqual(25, GetField(desc, res, "repeated_import_message[1].d"));
            Assert.AreEqual(35, GetField(desc, res, "repeated_nested_message[0].bb"));
            Assert.AreEqual(10, GetField(desc, res, "repeated_nested_message[1].bb"));
            Assert.AreEqual(54, GetField(desc, res, "repeated_public_import_message[0].e"));
            Assert.AreEqual(-1, GetField(desc, res, "repeated_public_import_message[1].e"));
            Assert.AreEqual("foo", GetField(desc, res, "repeated_string[0]"));
            Assert.AreEqual("bar", GetField(desc, res, "repeated_string[1]"));
            Assert.AreEqual("Oneof string", GetField(desc, res, "oneof_string"));
            Assert.AreEqual(UInt64.MaxValue, GetField(desc, res, "repeated_fixed64[0]"));
            Assert.AreEqual(1234567890123, GetField(desc, res, "repeated_fixed64[1]"));
            Assert.AreEqual(100f, GetField(desc, res, "repeated_float[0]"));
            Assert.AreEqual(12.25f, GetField(desc, res, "repeated_float[1]"));
            Assert.AreEqual(UInt32.MaxValue, GetField(desc, res, "repeated_uint32[0]"));
            Assert.AreEqual(UInt32.MinValue, GetField(desc, res, "repeated_uint32[1]"));
            Assert.AreEqual(UInt64.MaxValue, GetField(desc, res, "repeated_uint64[0]"));
            Assert.AreEqual(UInt64.MinValue, GetField(desc, res, "repeated_uint64[1]"));
        }

        private static object GetField(MessageDescriptor desc, DynamicMessage dm, String fieldFullName)
        {
            if (!fieldFullName.Contains(".") && !fieldFullName.Contains("["))
            {
                return dm.FieldSet.GetField(desc.FindFieldByName(fieldFullName));
            }
            else if (fieldFullName.Contains("[") && !fieldFullName.Contains("."))
            {
                string fieldName = fieldFullName.Substring(0, fieldFullName.IndexOf("["));
                int index = int.Parse(fieldFullName.Substring(fieldFullName.IndexOf("[") + 1, 1));
                List<object> list = (List<object>) GetField(desc, dm, fieldName);
                Assert.NotNull(list);
                return list[index];
            }
            else
            {
                string[] fields = fieldFullName.Split('.');
                DynamicMessage tempDm = dm;
                MessageDescriptor tempDesc = desc;
                for (int i = 0; i < fields.Length; i++)
                {
                    string field = fields[i];

                    if (i == fields.Length - 1)
                        return GetField(tempDesc, tempDm, field);
                    else
                    {
                        tempDm = (DynamicMessage) GetField(tempDesc, tempDm, field);
                        string tempField = field;
                        if (field.Contains("["))
                        {
                            tempField = field.Substring(0, field.IndexOf("["));
                        }
                        tempDesc = tempDesc.FindFieldByName(tempField).MessageType;
                    }

                }
            }
            return null;
        }

    }
}
