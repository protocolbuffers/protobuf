#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

using Conformance;
using Google.Protobuf.Reflection;
using System;
using System.IO;

namespace Google.Protobuf.Conformance
{
    /// <summary>
    /// Conformance tests. The test runner will provide JSON or proto data on stdin,
    /// and this program will produce its output on stdout.
    /// </summary>
    class Program
    {
        private static void Main(string[] args)
        {
            // This way we get the binary streams instead of readers/writers.
            var input = new BinaryReader(Console.OpenStandardInput());
            var output = new BinaryWriter(Console.OpenStandardOutput());
            var typeRegistry = TypeRegistry.FromMessages(ProtobufTestMessages.Proto3.TestAllTypes.Descriptor);

            int count = 0;
            while (RunTest(input, output, typeRegistry))
            {
                count++;
            }
            Console.Error.WriteLine("Received EOF after {0} tests", count);
        }

        private static bool RunTest(BinaryReader input, BinaryWriter output, TypeRegistry typeRegistry)
        {
            int? size = ReadInt32(input);
            if (size == null)
            {
                return false;
            }
            byte[] inputData = input.ReadBytes(size.Value);
            if (inputData.Length != size.Value)
            {
                throw new EndOfStreamException("Read " + inputData.Length + " bytes of data when expecting " + size);
            }
            ConformanceRequest request = ConformanceRequest.Parser.ParseFrom(inputData);
            ConformanceResponse response = PerformRequest(request, typeRegistry);
            byte[] outputData = response.ToByteArray();
            output.Write(outputData.Length);
            output.Write(outputData);
            // Ready for another test...
            return true;
        }

        private static ConformanceResponse PerformRequest(ConformanceRequest request, TypeRegistry typeRegistry)
        {
            ProtobufTestMessages.Proto3.TestAllTypes message;
            try
            {
                switch (request.PayloadCase)
                {
                    case ConformanceRequest.PayloadOneofCase.JsonPayload:
                        var parser = new JsonParser(new JsonParser.Settings(20, typeRegistry));
                        message = parser.Parse<ProtobufTestMessages.Proto3.TestAllTypes>(request.JsonPayload);
                        break;
                    case ConformanceRequest.PayloadOneofCase.ProtobufPayload:
                        message = ProtobufTestMessages.Proto3.TestAllTypes.Parser.ParseFrom(request.ProtobufPayload);
                        break;
                    default:
                        throw new Exception("Unsupported request payload: " + request.PayloadCase);
                }
            }
            catch (InvalidProtocolBufferException e)
            {
                return new ConformanceResponse { ParseError = e.Message };
            }
            catch (InvalidJsonException e)
            {
                return new ConformanceResponse { ParseError = e.Message };
            }
            try
            {
                switch (request.RequestedOutputFormat)
                {
                    case global::Conformance.WireFormat.Json:
                        var formatter = new JsonFormatter(new JsonFormatter.Settings(false, typeRegistry));
                        return new ConformanceResponse { JsonPayload = formatter.Format(message) };
                    case global::Conformance.WireFormat.Protobuf:
                        return new ConformanceResponse { ProtobufPayload = message.ToByteString() };
                    default:
                        throw new Exception("Unsupported request output format: " + request.PayloadCase);
                }
            }
            catch (InvalidOperationException e)
            {
                return new ConformanceResponse { SerializeError = e.Message };
            }
        }

        private static int? ReadInt32(BinaryReader input)
        {
            byte[] bytes = input.ReadBytes(4);
            if (bytes.Length == 0)
            {
                // Cleanly reached the end of the stream
                return null;
            }
            if (bytes.Length != 4)
            {
                throw new EndOfStreamException("Read " + bytes.Length + " bytes of size when expecting 4");
            }
            return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
        }
    }
}
