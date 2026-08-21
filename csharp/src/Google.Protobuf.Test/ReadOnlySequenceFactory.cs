#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Buffers;
using System.Collections.Generic;

namespace Google.Protobuf
{
    internal static class ReadOnlySequenceFactory
    {
        /// <summary>
        /// Create a sequence from the specified data. The data will be divided up into segments in the sequence.
        /// </summary>
        public static ReadOnlySequence<byte> CreateWithContent(byte[] data, int segmentSize = 1, bool addEmptySegmentDelimiters = true)
        {
            var segments = new List<byte[]>();

            if (addEmptySegmentDelimiters)
            {
                segments.Add(Array.Empty<byte>());
            }

            var currentIndex = 0;
            while (currentIndex < data.Length)
            {
                var segment = new List<byte>();
                while (segment.Count < segmentSize && currentIndex < data.Length)
                {
                    segment.Add(data[currentIndex++]);
                }
                segments.Add(segment.ToArray());

                if (addEmptySegmentDelimiters)
                {
                    segments.Add(Array.Empty<byte>());
                }
            }

            return CreateSegments(segments.ToArray());
        }

        /// <summary>
        /// Originally from corefx, and has been contributed to Protobuf
        /// https://github.com/dotnet/corefx/blob/e99ec129cfd594d53f4390bf97d1d736cff6f860/src/System.Memory/tests/ReadOnlyBuffer/ReadOnlySequenceFactory.byte.cs
        /// </summary>
        private static ReadOnlySequence<byte> CreateSegments(params byte[][] inputs)
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