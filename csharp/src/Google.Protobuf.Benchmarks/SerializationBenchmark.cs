#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2019 Google Inc.  All rights reserved.
// https://github.com/protocolbuffers/protobuf
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

using BenchmarkDotNet.Attributes;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Google.Protobuf.Benchmarks
{
    /// <summary>
    /// Benchmark for serializing (to a MemoryStream) and deserializing (from a ByteString).
    /// Over time we may wish to test the various different approaches to serialization and deserialization separately.
    /// </summary>
    [MemoryDiagnoser]
    public class SerializationBenchmark
    {
        /// <summary>
        /// All the configurations to be tested. Add more datasets to the array as they're available.
        /// (When C# supports proto2, this will increase significantly.)
        /// </summary>
        public static SerializationConfig[] Configurations => new[]
        {
            new SerializationConfig("dataset.google_message1_proto3.pb")
        };

        [ParamsSource(nameof(Configurations))]
        public SerializationConfig Configuration { get; set; }

        private MessageParser parser;
        /// <summary>
        /// Each data set can contain multiple messages in a single file.
        /// Each "write" operation should write each message in turn, and each "parse"
        /// operation should parse each message in turn.
        /// </summary>
        private List<SubTest> subTests;

        [GlobalSetup]
        public void GlobalSetup()
        {
            parser = Configuration.Parser;
            subTests = Configuration.Payloads.Select(p => new SubTest(p, parser.ParseFrom(p))).ToList();
        }

        [Benchmark]
        public void WriteToStream() => subTests.ForEach(item => item.WriteToStream());

        [Benchmark]
        public void ToByteArray() => subTests.ForEach(item => item.ToByteArray());

        [Benchmark]
        public void ParseFromByteString() => subTests.ForEach(item => item.ParseFromByteString(parser));

        [Benchmark]
        public void ParseFromStream() => subTests.ForEach(item => item.ParseFromStream(parser));

        private class SubTest
        {
            private readonly Stream destinationStream;
            private readonly Stream sourceStream;
            private readonly ByteString data;
            private readonly IMessage message;

            public SubTest(ByteString data, IMessage message)
            {
                destinationStream = new MemoryStream(data.Length);
                sourceStream = new MemoryStream(data.ToByteArray());
                this.data = data;
                this.message = message;
            }

            public void Reset() => destinationStream.Position = 0;

            public void WriteToStream()
            {
                destinationStream.Position = 0;
                message.WriteTo(destinationStream);
            }

            public void ToByteArray() => message.ToByteArray();

            public void ParseFromByteString(MessageParser parser) => parser.ParseFrom(data);

            public void ParseFromStream(MessageParser parser)
            {
                sourceStream.Position = 0;
                parser.ParseFrom(sourceStream);
            }
        }
    }
}
