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

using Benchmarks;
using Google.Protobuf.Reflection;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace Google.Protobuf.Benchmarks
{
    /// <summary>
    /// The configuration for a single serialization test, loaded from a dataset.
    /// </summary>
    public class SerializationConfig
    {
        private static readonly Dictionary<string, MessageParser> parsersByMessageName = 
            typeof(SerializationBenchmark).Assembly.GetTypes()
                .Where(t => typeof(IMessage).IsAssignableFrom(t))
                .ToDictionary(
                    t => ((MessageDescriptor) t.GetProperty("Descriptor", BindingFlags.Static | BindingFlags.Public).GetValue(null)).FullName,
                    t => ((MessageParser) t.GetProperty("Parser", BindingFlags.Static | BindingFlags.Public).GetValue(null)));

        public MessageParser Parser { get; }
        public IEnumerable<ByteString> Payloads { get; }
        public string Name { get; }

        public SerializationConfig(string resource)
        {
            var data = LoadData(resource);
            var dataset = BenchmarkDataset.Parser.ParseFrom(data);

            if (!parsersByMessageName.TryGetValue(dataset.MessageName, out var parser))
            {
                throw new ArgumentException($"No parser for message {dataset.MessageName} in this assembly");
            }
            Parser = parser;
            Payloads = dataset.Payload;
            Name = dataset.Name;
        }

        private static byte[] LoadData(string resource)
        {
            using (var stream = typeof(SerializationBenchmark).Assembly.GetManifestResourceStream($"Google.Protobuf.Benchmarks.{resource}"))
            {
                if (stream == null)
                {
                    throw new ArgumentException($"Unable to load embedded resource {resource}");
                }
                var copy = new MemoryStream();
                stream.CopyTo(copy);
                return copy.ToArray();
            }
        }

        public override string ToString() => Name;
    }
}
