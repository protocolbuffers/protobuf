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
using Benchmarks;
using Google.Protobuf.Buffers;
using System;
using System.Buffers;
using System.Collections.Generic;
using System.IO;
using System.IO.Pipelines;
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
            new SerializationConfig("dataset.google_message1_proto3.pb"),
            GoogleMessageBenchmark.CreateGoogleMessageConfig(MessageSize.Empty),
            GoogleMessageBenchmark.CreateGoogleMessageConfig(MessageSize.Small),
            GoogleMessageBenchmark.CreateGoogleMessageConfig(MessageSize.Medium),
            GoogleMessageBenchmark.CreateGoogleMessageConfig(MessageSize.Large)
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
            subTests = Configuration.Payloads.Select(p => new SubTest(p, (IBufferMessage)parser.ParseFrom(p))).ToList();
        }

        [Benchmark]
        public void WriteToStream()
        {
            foreach (var subTest in subTests)
            {
                subTest.WriteToStream();
            }
        }

        [Benchmark]
        public void WriteToBufferWriter()
        {
            foreach (var subTest in subTests)
            {
                subTest.WriteToBufferWriter();
            }
        }

        [Benchmark]
        public void WriteToPipeWriter()
        {
            foreach (var subTest in subTests)
            {
                subTest.WriteToPipeWriter();
            }
        }

        [Benchmark]
        public void ToByteArray()
        {
            foreach (var subTest in subTests)
            {
                subTest.ToByteArray();
            }
        }

        [Benchmark]
        public void ParseFromByteString()
        {
            foreach (var subTest in subTests)
            {
                subTest.ParseFromByteString(parser);
            }
        }

        [Benchmark]
        public void ParseFromStream()
        {
            foreach (var subTest in subTests)
            {
                subTest.ParseFromStream(parser);
            }
        }

        [Benchmark]
        public void ParseFromReadOnlySequence()
        {
            foreach (var subTest in subTests)
            {
                subTest.ParseFromReadOnlySequence(parser);
            }
        }

        private class SubTest
        {
            private readonly Stream destinationStream;
            private readonly Google.Protobuf.Buffers.ArrayBufferWriter<byte> bufferWriter;
            private readonly Stream sourceStream;
            private readonly ReadOnlySequence<byte> readOnlySequence;
            private readonly ByteString data;
            private readonly IBufferMessage message;
            private readonly Pipe pipe;

            public SubTest(ByteString data, IBufferMessage message)
            {
                destinationStream = new MemoryStream(data.Length);
                bufferWriter = new Google.Protobuf.Buffers.ArrayBufferWriter<byte>();
                sourceStream = new MemoryStream(data.ToByteArray());
                readOnlySequence = new ReadOnlySequence<byte>(data.ToByteArray());
                pipe = new Pipe();
                this.data = data;
                this.message = message;
            }

            public void WriteToStream()
            {
                destinationStream.Position = 0;
                message.WriteTo(destinationStream);
            }

            public void WriteToPipeWriter()
            {
                var writer = new CodedOutputWriter(pipe.Writer);
                message.WriteTo(ref writer);

                pipe.Writer.Complete();
                pipe.Reader.Complete();
                pipe.Reset();
            }

            public void WriteToBufferWriter()
            {
                bufferWriter.Clear();
                var writer = new CodedOutputWriter(bufferWriter);
                message.WriteTo(ref writer);
            }

            public void ToByteArray() => message.ToByteArray();

            public void ParseFromByteString(MessageParser parser) => parser.ParseFrom(data);

            public void ParseFromStream(MessageParser parser)
            {
                sourceStream.Position = 0;
                parser.ParseFrom(sourceStream);
            }

            public void ParseFromReadOnlySequence(MessageParser parser)
            {
                var reader = new CodedInputReader(readOnlySequence);
                parser.ParseFrom(ref reader);
            }
        }
    }
}
