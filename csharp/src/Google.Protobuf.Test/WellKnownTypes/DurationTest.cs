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

using NUnit.Framework;
using System;

namespace Google.Protobuf.WellKnownTypes
{
    public class DurationTest
    {
        [Test]
        public void ToTimeSpan()
        {
            Assert.AreEqual(TimeSpan.FromSeconds(1), new Duration { Seconds = 1 }.ToTimeSpan());
            Assert.AreEqual(TimeSpan.FromSeconds(-1), new Duration { Seconds = -1 }.ToTimeSpan());
            Assert.AreEqual(TimeSpan.FromMilliseconds(1), new Duration { Nanos = 1000000 }.ToTimeSpan());
            Assert.AreEqual(TimeSpan.FromMilliseconds(-1), new Duration { Nanos = -1000000 }.ToTimeSpan());
            Assert.AreEqual(TimeSpan.FromTicks(1), new Duration { Nanos = 100 }.ToTimeSpan());
            Assert.AreEqual(TimeSpan.FromTicks(-1), new Duration { Nanos = -100 }.ToTimeSpan());

            // Rounding is towards 0
            Assert.AreEqual(TimeSpan.FromTicks(2), new Duration { Nanos = 250 }.ToTimeSpan());
            Assert.AreEqual(TimeSpan.FromTicks(-2), new Duration { Nanos = -250 }.ToTimeSpan());
        }

        [Test]
        public void Addition()
        {
            Assert.AreEqual(new Duration { Seconds = 2, Nanos = 100000000 },
                new Duration { Seconds = 1, Nanos = 600000000 } + new Duration { Nanos = 500000000 });
            Assert.AreEqual(new Duration { Seconds = -2, Nanos = -100000000 },
                new Duration { Seconds = -1, Nanos = -600000000 } + new Duration { Nanos = -500000000 });
            Assert.AreEqual(new Duration { Seconds = 1, Nanos = 100000000 },
                new Duration { Seconds = 1, Nanos = 600000000 } + new Duration { Nanos = -500000000 });

            // Non-normalized durations, or non-normalized intermediate results
            Assert.AreEqual(new Duration { Seconds = 1 },
                new Duration { Seconds = 1, Nanos = -500000000 } + new Duration { Nanos = 500000000 });

            Assert.AreEqual(new Duration { Nanos = -900000000 },
                new Duration { Seconds = -1, Nanos = -100000000 } + new Duration { Nanos = 200000000 });
            Assert.AreEqual(new Duration { Nanos = 900000000 },
                new Duration { Seconds = 1, Nanos = 100000000 } + new Duration { Nanos = -200000000 });
        }

        [Test]
        public void Subtraction()
        {
            Assert.AreEqual(new Duration { Seconds = 1, Nanos = 100000000 },
                new Duration { Seconds = 1, Nanos = 600000000 } - new Duration { Nanos = 500000000 });
            Assert.AreEqual(new Duration { Seconds = -1, Nanos = -100000000 },
                new Duration { Seconds = -1, Nanos = -600000000 } - new Duration { Nanos = -500000000 });
            Assert.AreEqual(new Duration { Seconds = 2, Nanos = 100000000 },
                new Duration { Seconds = 1, Nanos = 600000000 } - new Duration { Nanos = -500000000 });

            // Non-normalized durations
            Assert.AreEqual(new Duration(),
                new Duration { Seconds = 1, Nanos = -500000000 } - new Duration { Nanos = 500000000 });
            Assert.AreEqual(new Duration { Seconds = 1 },
                new Duration { Nanos = 2000000000 } - new Duration { Nanos = 1000000000 });
        }

        [Test]
        public void FromTimeSpan()
        {
            Assert.AreEqual(new Duration { Seconds = 1 }, Duration.FromTimeSpan(TimeSpan.FromSeconds(1)));
            Assert.AreEqual(new Duration { Nanos = Duration.NanosecondsPerTick }, Duration.FromTimeSpan(TimeSpan.FromTicks(1)));
        }

        [Test]
        [TestCase(0, Duration.MaxNanoseconds + 1)]
        [TestCase(0, Duration.MinNanoseconds - 1)]
        [TestCase(Duration.MinSeconds - 1, 0)]
        [TestCase(Duration.MaxSeconds + 1, 0)]
        [TestCase(1, -1)]
        [TestCase(-1, 1)]
        public void ToTimeSpan_Invalid(long seconds, int nanoseconds)
        {
            var duration = new Duration { Seconds = seconds, Nanos = nanoseconds };
            Assert.Throws<InvalidOperationException>(() => duration.ToTimeSpan());
        }

        [Test]
        [TestCase(0, Duration.MaxNanoseconds)]
        [TestCase(0, Duration.MinNanoseconds)]
        [TestCase(Duration.MinSeconds, Duration.MinNanoseconds)]
        [TestCase(Duration.MaxSeconds, Duration.MaxNanoseconds)]
        public void ToTimeSpan_Valid(long seconds, int nanoseconds)
        {
            // Only testing that these values don't throw, unlike their similar tests in ToTimeSpan_Invalid
            var duration = new Duration { Seconds = seconds, Nanos = nanoseconds };
            duration.ToTimeSpan();
        }

        [Test]
        public void ToString_NonNormalized()
        {
            // Just a single example should be sufficient...
            var duration = new Duration { Seconds = 1, Nanos = -1 };
            Assert.AreEqual("{ \"@warning\": \"Invalid Duration\", \"seconds\": \"1\", \"nanos\": -1 }", duration.ToString());
        }

        [Test]
        public void Comparability()
        {
            Duration[] durationsInExpectedSortOrder =
            {
                null,
                new Duration { Seconds = -10, Nanos = -10 },
                new Duration { Seconds = -10, Nanos = -1 },
                new Duration { Seconds = -1, Nanos = -10 },
                new Duration { Seconds = -1, Nanos = -1 },
                new Duration(),
                new Duration { Seconds = 1, Nanos = 1 },
                new Duration { Seconds = 1, Nanos = 10 },
                new Duration { Seconds = 10, Nanos = 1 },
                new Duration { Seconds = 10, Nanos = 10 }
            };

            for (int i = 0; i < durationsInExpectedSortOrder.Length; i++)
            {
                var target = durationsInExpectedSortOrder[i];
                if (target is null)
                {
                    continue;
                }
                for (int j = 0; j < durationsInExpectedSortOrder.Length; j++)
                {
                    var expectedResult = Math.Sign(i - j);
                    var actualResult = target.CompareTo(durationsInExpectedSortOrder[j]);
                    Assert.AreEqual(expectedResult, actualResult);
                }
            }
        }
    }
}
