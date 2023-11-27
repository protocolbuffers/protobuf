#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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
    }
}
