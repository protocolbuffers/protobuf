#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
using System;
using System.Buffers;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Google.Protobuf.Buffers
{
    /// <summary>
    /// ReadOnlySequenceFactory is originally from corefx, and has been contributed to Protobuf
    /// https://github.com/dotnet/corefx/blob/e99ec129cfd594d53f4390bf97d1d736cff6f860/src/System.Memory/tests/ReadOnlyBuffer/ReadOnlySequenceFactory.byte.cs
    /// </summary>
    internal abstract class ReadOnlySequenceFactory
    {
        public static ReadOnlySequenceFactory ArrayFactory { get; } = new ArrayTestSequenceFactory();
        public static ReadOnlySequenceFactory MemoryFactory { get; } = new MemoryTestSequenceFactory();
        public static ReadOnlySequenceFactory SingleSegmentFactory { get; } = new SingleSegmentTestSequenceFactory();
        public static ReadOnlySequenceFactory SegmentPerByteFactory { get; } = new BytePerSegmentTestSequenceFactory();

        public abstract ReadOnlySequence<byte> CreateOfSize(int size);
        public abstract ReadOnlySequence<byte> CreateWithContent(byte[] data);

        public ReadOnlySequence<byte> CreateWithContent(string data)
        {
            return CreateWithContent(Encoding.ASCII.GetBytes(data));
        }

        internal class ArrayTestSequenceFactory : ReadOnlySequenceFactory
        {
            public override ReadOnlySequence<byte> CreateOfSize(int size)
            {
                return new ReadOnlySequence<byte>(new byte[size + 20], 10, size);
            }

            public override ReadOnlySequence<byte> CreateWithContent(byte[] data)
            {
                var startSegment = new byte[data.Length + 20];
                Array.Copy(data, 0, startSegment, 10, data.Length);
                return new ReadOnlySequence<byte>(startSegment, 10, data.Length);
            }
        }

        internal class MemoryTestSequenceFactory : ReadOnlySequenceFactory
        {
            public override ReadOnlySequence<byte> CreateOfSize(int size)
            {
                return CreateWithContent(new byte[size]);
            }

            public override ReadOnlySequence<byte> CreateWithContent(byte[] data)
            {
                var startSegment = new byte[data.Length + 20];
                Array.Copy(data, 0, startSegment, 10, data.Length);
                return new ReadOnlySequence<byte>(new Memory<byte>(startSegment, 10, data.Length));
            }
        }

        internal class SingleSegmentTestSequenceFactory : ReadOnlySequenceFactory
        {
            public override ReadOnlySequence<byte> CreateOfSize(int size)
            {
                return CreateWithContent(new byte[size]);
            }

            public override ReadOnlySequence<byte> CreateWithContent(byte[] data)
            {
                return CreateSegments(data);
            }
        }

        internal class BytePerSegmentTestSequenceFactory : ReadOnlySequenceFactory
        {
            public override ReadOnlySequence<byte> CreateOfSize(int size)
            {
                return CreateWithContent(new byte[size]);
            }

            public override ReadOnlySequence<byte> CreateWithContent(byte[] data)
            {
                var segments = new List<byte[]>();

                segments.Add(new byte[0]);
                foreach (var b in data)
                {
                    segments.Add(new[] { b });
                    segments.Add(new byte[0]);
                }

                return CreateSegments(segments.ToArray());
            }
        }

        public static ReadOnlySequence<byte> CreateSegments(params byte[][] inputs)
        {
            if (inputs == null || inputs.Length == 0)
            {
                throw new InvalidOperationException();
            }

            int i = 0;

            BufferSegment last = null;
            BufferSegment first = null;

            do
            {
                byte[] s = inputs[i];
                int length = s.Length;
                int dataOffset = length;
                var chars = new byte[length * 2];

                for (int j = 0; j < length; j++)
                {
                    chars[dataOffset + j] = s[j];
                }

                // Create a segment that has offset relative to the OwnedMemory and OwnedMemory itself has offset relative to array
                var memory = new Memory<byte>(chars).Slice(length, length);

                if (first == null)
                {
                    first = new BufferSegment(memory);
                    last = first;
                }
                else
                {
                    last = last.Append(memory);
                }
                i++;
            } while (i < inputs.Length);

            return new ReadOnlySequence<byte>(first, 0, last, last.Memory.Length);
        }

        private class BufferSegment : ReadOnlySequenceSegment<byte>
        {
            public BufferSegment(Memory<byte> memory)
            {
                Memory = memory;
            }

            public BufferSegment Append(Memory<byte> memory)
            {
                var segment = new BufferSegment(memory)
                {
                    RunningIndex = RunningIndex + Memory.Length
                };
                Next = segment;
                return segment;
            }
        }
    }
}
#endif
