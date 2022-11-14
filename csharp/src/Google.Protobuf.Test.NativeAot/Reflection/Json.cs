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

using Google.Protobuf.TestProtos.Proto2;

namespace Google.Protobuf.Test.NativeAot.Reflection
{
    internal static class Json
    {
        public static void FormatJson()
        {
            var message = new TestAllTypes
            {
                OptionalNestedMessage = new TestAllTypes.Types.NestedMessage() { Bb = 42 },
                OneofString = "Value!",
                RepeatedBool = { true, false, true },
            };

            var json = JsonFormatter.Default.Format(message);
            Assert.AreEqual("{ \"optionalNestedMessage\": { \"bb\": 42 }, \"repeatedBool\": [ true, false, true ], \"oneofString\": \"Value!\" }", json);

            Console.WriteLine(json);
        }
        
        public static void ParseJson()
        {
            var json = "{ \"optionalNestedMessage\": { \"bb\": 42 }, \"repeatedBool\": [ true, false, true ], \"oneofString\": \"Value!\" }";

            var message = (TestAllTypes) JsonParser.Default.Parse(json, TestAllTypes.Descriptor);
            Assert.AreEqual(42, message.OptionalNestedMessage.Bb);
            Assert.AreEqual("Value!", message.OneofString);
            Assert.AreEqual(true, message.RepeatedBool[0]);
            Assert.AreEqual(false, message.RepeatedBool[1]);
            Assert.AreEqual(true, message.RepeatedBool[2]);
        }
    }
}
