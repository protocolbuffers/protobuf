#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

using System.Collections;
using System.Reflection;
using ProtobufTestMessages.Proto3;
using UnitTest.Issues.TestProtos;

namespace Google.Protobuf.Test.NativeAot.Reflection
{
    internal static class Descriptors
    {
        public static void GetFileDescriptorMessages()
        {
            var messageTypes = TestMessagesProto3Reflection.Descriptor.MessageTypes;
            Assert.AreNotEqual(0, messageTypes.Count);
            
            foreach (var item in messageTypes)
            {
                Console.WriteLine($"{item.Name} message");
            }
            Console.WriteLine($"{messageTypes.Count} messages");
        }
        
        public static void GetMessageFields()
        {
            var fields = TestAllTypesProto3.Descriptor.Fields.InDeclarationOrder();
            Assert.AreNotEqual(0, fields.Count);
            
            foreach (var item in fields)
            {
                Console.WriteLine($"{item.Name} field");
            }
            Console.WriteLine($"{fields.Count} fields");
        }

        public static void GetMessageEnumTypes()
        {
            var enumTypes = TestAllTypesProto3.Descriptor.EnumTypes;
            Assert.AreNotEqual(0, enumTypes.Count);

            foreach (var item in enumTypes)
            {
                Console.WriteLine($"{item.Name} enum type");
            }
            Console.WriteLine($"{enumTypes.Count} enum types");
        }

        public static void GetEnumFields()
        {
            var enumFields = typeof(TestAllTypesProto3.Types.NestedEnum).GetFields(BindingFlags.Static | BindingFlags.Public);
            Assert.AreEqual(4, enumFields.Length);

            foreach (var item in enumFields)
            {
                Console.WriteLine($"{item.Name} enum field");
            }
            Console.WriteLine($"{enumFields.Length} enum fields");
        }

        public static void GetEnumDescriptorFields()
        {
            var enumDescriptor = TestAllTypesProto3.Descriptor.EnumTypes.Single(t => t.Name == "NestedEnum");
            Assert.AreEqual(4, enumDescriptor.Values.Count);

            foreach (var item in enumDescriptor.Values)
            {
                Console.WriteLine($"{item.Name} {item.Number} enum value");
            }
            Console.WriteLine($"{enumDescriptor.Values.Count} enum values");
        }

        public static void AccessorSingleFieldReflection()
        {
            var field = TestAllTypesProto3.Descriptor.FindFieldByName("fieldname1");
            var message = (IMessage)Activator.CreateInstance(TestAllTypesProto3.Descriptor.ClrType)!;

            field.Accessor.SetValue(message, 101);

            var value = (int)field.Accessor.GetValue(message);
            Assert.AreEqual(101, value);

            field.Accessor.Clear(message);

            value = (int) field.Accessor.GetValue(message);
            Assert.AreEqual(0, value);
        }

        public static void AccessorOneOfFieldReflection()
        {
            var bytesfield = TestAllTypesProto3.Descriptor.FindFieldByName("oneof_bytes");
            var stringfield = TestAllTypesProto3.Descriptor.FindFieldByName("oneof_string");
            var message = (IMessage) Activator.CreateInstance(TestAllTypesProto3.Descriptor.ClrType)!;

            bytesfield.Accessor.SetValue(message, ByteString.CopyFrom(new byte[] { 1, 2, 3 }));

            var bytesValue = (ByteString) bytesfield.Accessor.GetValue(message);
            if (!bytesValue.Span.SequenceEqual(new byte[] { 1, 2, 3 }))
            {
                throw new Exception($"Expected 1,2,3.");
            }

            Assert.AreEqual(true, bytesfield.Accessor.HasValue(message));

            stringfield.Accessor.SetValue(message, "abc");

            Assert.AreEqual("abc", (string) stringfield.Accessor.GetValue(message));
            Assert.AreEqual(false, bytesfield.Accessor.HasValue(message));
        }

        public static void AccessorRepeatingFieldReflection()
        {
            var field = TestAllTypesProto3.Descriptor.FindFieldByName("repeated_int32");
            var message = (IMessage) Activator.CreateInstance(TestAllTypesProto3.Descriptor.ClrType)!;

            var list = (IList) field.Accessor.GetValue(message);
            Assert.IsNotNull(list);
        }

        public static void GetFileDescriptorServices()
        {
            var serviceDescriptor = UnittestCustomOptionsProto3Reflection.Descriptor.Services.Single(s => s.Name == "TestServiceWithCustomOptions");
            Assert.IsNotNull(serviceDescriptor);
            Assert.AreNotEqual(0, serviceDescriptor.Methods.Count);

            foreach (var method in serviceDescriptor.Methods)
            {
                Console.WriteLine($"{method.Name} method");
            }
        }
    }
}
