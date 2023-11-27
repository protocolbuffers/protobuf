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
    public class TimestampTest
    {
        [Test]
        public void FromAndToDateTime()
        {
            DateTime utcMin = DateTime.SpecifyKind(DateTime.MinValue, DateTimeKind.Utc);
            DateTime utcMax = DateTime.SpecifyKind(DateTime.MaxValue, DateTimeKind.Utc);
            AssertRoundtrip(new Timestamp { Seconds = -62135596800 }, utcMin);
            AssertRoundtrip(new Timestamp { Seconds = 253402300799, Nanos = 999999900 }, utcMax);
            AssertRoundtrip(new Timestamp(), new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc));
            AssertRoundtrip(new Timestamp { Nanos = 1000000}, new DateTime(1970, 1, 1, 0, 0, 0, 1, DateTimeKind.Utc));
            AssertRoundtrip(new Timestamp { Seconds = -1, Nanos = 999000000 }, new DateTime(1969, 12, 31, 23, 59, 59, 999, DateTimeKind.Utc));
            AssertRoundtrip(new Timestamp { Seconds = 3600 }, new DateTime(1970, 1, 1, 1, 0, 0, DateTimeKind.Utc));
            AssertRoundtrip(new Timestamp { Seconds = -3600 }, new DateTime(1969, 12, 31, 23, 0, 0, DateTimeKind.Utc));
        }

        [Test]
        public void ToDateTimeTruncation()
        {
            var t1 = new Timestamp { Seconds = 1, Nanos = 1000000 + Duration.NanosecondsPerTick - 1 };
            Assert.AreEqual(new DateTime(1970, 1, 1, 0, 0, 1, DateTimeKind.Utc).AddMilliseconds(1), t1.ToDateTime());

            var t2 = new Timestamp { Seconds = -1, Nanos = 1000000 + Duration.NanosecondsPerTick - 1 };
            Assert.AreEqual(new DateTime(1969, 12, 31, 23, 59, 59).AddMilliseconds(1), t2.ToDateTime());
        }

        [Test]
        [TestCase(Timestamp.UnixSecondsAtBclMinValue - 1, Timestamp.MaxNanos)]
        [TestCase(Timestamp.UnixSecondsAtBclMaxValue + 1, 0)]
        [TestCase(0, -1)]
        [TestCase(0, Timestamp.MaxNanos + 1)]
        public void ToDateTime_OutOfRange(long seconds, int nanoseconds)
        {
            var value = new Timestamp { Seconds = seconds, Nanos = nanoseconds };
            Assert.Throws<InvalidOperationException>(() => value.ToDateTime());
        }

        // 1ns larger or smaller than the above values
        [Test]
        [TestCase(Timestamp.UnixSecondsAtBclMinValue, 0)]
        [TestCase(Timestamp.UnixSecondsAtBclMaxValue, Timestamp.MaxNanos)]
        [TestCase(0, 0)]
        [TestCase(0, Timestamp.MaxNanos)]
        public void ToDateTime_ValidBoundaries(long seconds, int nanoseconds)
        {
            var value = new Timestamp { Seconds = seconds, Nanos = nanoseconds };
            value.ToDateTime();
        }

        private static void AssertRoundtrip(Timestamp timestamp, DateTime dateTime)
        {
            Assert.AreEqual(timestamp, Timestamp.FromDateTime(dateTime));
            Assert.AreEqual(dateTime, timestamp.ToDateTime());
            Assert.AreEqual(DateTimeKind.Utc, timestamp.ToDateTime().Kind);
        }

        [Test]
        public void Arithmetic()
        {
            Timestamp t1 = new Timestamp { Seconds = 10000, Nanos = 5000 };
            Timestamp t2 = new Timestamp { Seconds = 8000, Nanos = 10000 };
            Duration difference = new Duration { Seconds = 1999, Nanos = Duration.NanosecondsPerSecond - 5000 };
            Assert.AreEqual(difference, t1 - t2);
            Assert.AreEqual(-difference, t2 - t1);

            Assert.AreEqual(t1, t2 + difference);
            Assert.AreEqual(t2, t1 - difference);
        }

        [Test]
        public void ToString_NonNormalized()
        {
            // Just a single example should be sufficient...
            var duration = new Timestamp { Seconds = 1, Nanos = -1 };
            Assert.AreEqual("{ \"@warning\": \"Invalid Timestamp\", \"seconds\": \"1\", \"nanos\": -1 }", duration.ToString());
        }
    }
}
