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

using System;
using System.IO;
using Benchmarks;
using Benchmarks.Proto3;

namespace Google.Protobuf.Benchmarks
{
    public static class GoogleMessageBenchmark
    {
        public static ByteString GetMessageData(MessageSize messageSize)
        {
            var ms = new MemoryStream();
            var output = new CodedOutputStream(ms);

            var googleMessage1 = new GoogleMessage1();
            switch (messageSize)
            {
                case MessageSize.Empty:
                    break;
                case MessageSize.Small:
                    SetString(googleMessage1, 1);
                    AddRepeatingItems(googleMessage1, 1);
                    googleMessage1.Field2 = 2;
                    googleMessage1.Field15 = new GoogleMessage1SubMessage();
                    googleMessage1.Field15.Field1 = 1;
                    break;
                case MessageSize.Medium:
                    SetString(googleMessage1, 1024);
                    AddRepeatingItems(googleMessage1, 1024);
                    googleMessage1.Field2 = 2;
                    googleMessage1.Field15 = new GoogleMessage1SubMessage();
                    googleMessage1.Field15.Field1 = 1;
                    break;
                case MessageSize.Large:
                    SetString(googleMessage1, 1024 * 512);
                    AddRepeatingItems(googleMessage1, 1024 * 512);
                    googleMessage1.Field2 = 2;
                    googleMessage1.Field15 = new GoogleMessage1SubMessage();
                    googleMessage1.Field15.Field1 = 1;
                    break;
                default:
                    throw new ArgumentOutOfRangeException(nameof(MessageSize));
            }

            googleMessage1.WriteTo(output);
            output.Flush();

            return ByteString.CopyFrom(ms.ToArray());
        }

        private static void SetString(GoogleMessage1 googleMessage1, int count)
        {
            googleMessage1.Field1 = "Text" + new string('!', count);
        }

        private static void AddRepeatingItems(GoogleMessage1 googleMessage1, ulong count)
        {
            for (var i = 0UL; i < count; i++)
            {
                googleMessage1.Field5.Add(i);
            }
        }

        public static SerializationConfig CreateGoogleMessageConfig(MessageSize messageSize)
        {
            return new SerializationConfig(new BenchmarkDataset
            {
                MessageName = "benchmarks.proto3.GoogleMessage1",
                Name = "GoogleMessage1 " + messageSize,
                Payload = { GoogleMessageBenchmark.GetMessageData(messageSize) }
            });
        }
    }

    public enum MessageSize
    {
        Empty,
        Small,
        Medium,
        Large
    }
}
