using System;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers.CompatTests
{
    public abstract class CompatibilityTests
    {
        protected abstract string TestName { get; }
        protected abstract object SerializeMessage<TMessage, TBuilder>(TMessage message)
            where TMessage : IMessageLite<TMessage, TBuilder>
            where TBuilder : IBuilderLite<TMessage, TBuilder>;

        protected abstract TBuilder DeerializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
            where TMessage : IMessageLite<TMessage, TBuilder>
            where TBuilder : IBuilderLite<TMessage, TBuilder>;

        #region RunBenchmark

        protected void RunBenchmark<TMessage, TBuilder>(byte[] buffer, bool write)
            where TMessage : IMessageLite<TMessage, TBuilder>
            where TBuilder : IBuilderLite<TMessage, TBuilder>, new()
        {
            TBuilder builder = new TBuilder();
            TMessage message = new TBuilder().MergeFrom(buffer).Build();
            System.Diagnostics.Stopwatch watch = new System.Diagnostics.Stopwatch();
            //simple warm-up
            object content = SerializeMessage<TMessage, TBuilder>(message);
            Assert.AreEqual(message, DeerializeMessage<TMessage, TBuilder>(content, new TBuilder(), ExtensionRegistry.Empty).Build());
            //timming
            long time = 0, sample = 1;
            while (time < 100)
            {
                sample *= 10;
                watch.Reset();
                watch.Start();
                if (write)
                {
                    for (int i = 0; i < sample; i++)
                        SerializeMessage<TMessage, TBuilder>(message);
                }
                else
                {
                    for (int i = 0; i < sample; i++)
                        DeerializeMessage<TMessage, TBuilder>(content, builder, ExtensionRegistry.Empty);
                }
                watch.Stop();
                time = watch.ElapsedMilliseconds;
            }

            ulong rounds = (ulong)((100.0 / watch.ElapsedMilliseconds) * sample);
            //test
            watch.Reset();
            watch.Start();

            if (write)
            {
                for (ulong i = 0; i < rounds; i++)
                    SerializeMessage<TMessage, TBuilder>(message);
            }
            else
            {
                for (ulong i = 0; i < rounds; i++)
                    DeerializeMessage<TMessage, TBuilder>(content, builder, ExtensionRegistry.Empty);
            }

            watch.Stop();
            System.Diagnostics.Trace.TraceInformation(
                "\r\n{0} {4} {5} {3:n0} rps ({1:n0} rounds in {2:n0} ms)", typeof(TMessage).Name, rounds,
                watch.ElapsedMilliseconds, (1000.0 / watch.ElapsedMilliseconds) * (double)rounds, TestName, write ? " write" : " read");
            GC.GetTotalMemory(true);
            GC.WaitForPendingFinalizers();
        }

        [Test]
        public virtual void Message1OptimizeSizeWriterPerf()
        {
            RunBenchmark<SizeMessage1, SizeMessage1.Builder>(TestResources.google_message1, true);
        }
        [Test]
        public virtual void Message1OptimizeSpeedWriterPerf()
        {
            RunBenchmark<SpeedMessage1, SpeedMessage1.Builder>(TestResources.google_message1, true);
        }
        [Test]
        public virtual void Message2OptimizeSizeWriterPerf()
        {
            RunBenchmark<SizeMessage2, SizeMessage2.Builder>(TestResources.google_message2, true);
        }
        [Test]
        public virtual void Message2OptimizeSpeedWriterPerf()
        {
            RunBenchmark<SpeedMessage2, SpeedMessage2.Builder>(TestResources.google_message2, true);
        }

        [Test]
        public virtual void Message1OptimizeSizeReadPerf()
        {
            RunBenchmark<SizeMessage1, SizeMessage1.Builder>(TestResources.google_message1, false);
        }
        [Test]
        public virtual void Message1OptimizeSpeedReadPerf()
        {
            RunBenchmark<SpeedMessage1, SpeedMessage1.Builder>(TestResources.google_message1, false);
        }
        [Test]
        public virtual void Message2OptimizeSizeReadPerf()
        {
            RunBenchmark<SizeMessage2, SizeMessage2.Builder>(TestResources.google_message2, false);
        }
        [Test]
        public virtual void Message2OptimizeSpeedReadPerf()
        {
            RunBenchmark<SpeedMessage2, SpeedMessage2.Builder>(TestResources.google_message2, false);
        }
        
        #endregion

        [Test]
        public virtual void RoundTripMessage1OptimizeSize()
        {
            SizeMessage1 msg = SizeMessage1.CreateBuilder().MergeFrom(TestResources.google_message1).Build();
            object content = SerializeMessage<SizeMessage1, SizeMessage1.Builder>(msg);

            SizeMessage1 copy = DeerializeMessage<SizeMessage1, SizeMessage1.Builder>(content, SizeMessage1.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            Assert.AreEqual(content, SerializeMessage<SizeMessage1,SizeMessage1.Builder>(copy));
            Assert.AreEqual(TestResources.google_message1, copy.ToByteArray());
        }

        [Test]
        public virtual void RoundTripMessage2OptimizeSize()
        {
            SizeMessage2 msg = SizeMessage2.CreateBuilder().MergeFrom(TestResources.google_message2).Build();
            object content = SerializeMessage<SizeMessage2, SizeMessage2.Builder>(msg);

            SizeMessage2 copy = DeerializeMessage<SizeMessage2, SizeMessage2.Builder>(content, SizeMessage2.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            Assert.AreEqual(content, SerializeMessage<SizeMessage2, SizeMessage2.Builder>(copy));
            Assert.AreEqual(TestResources.google_message2, copy.ToByteArray());
        }

        [Test]
        public virtual void RoundTripMessage1OptimizeSpeed()
        {
            SpeedMessage1 msg = SpeedMessage1.CreateBuilder().MergeFrom(TestResources.google_message1).Build();
            object content = SerializeMessage<SpeedMessage1, SpeedMessage1.Builder>(msg);

            SpeedMessage1 copy = DeerializeMessage<SpeedMessage1, SpeedMessage1.Builder>(content, SpeedMessage1.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            Assert.AreEqual(content, SerializeMessage<SpeedMessage1, SpeedMessage1.Builder>(copy));
            Assert.AreEqual(TestResources.google_message1, copy.ToByteArray());
        }

        [Test]
        public virtual void RoundTripMessage2OptimizeSpeed()
        {
            SpeedMessage2 msg = SpeedMessage2.CreateBuilder().MergeFrom(TestResources.google_message2).Build();
            object content = SerializeMessage<SpeedMessage2, SpeedMessage2.Builder>(msg);

            SpeedMessage2 copy = DeerializeMessage<SpeedMessage2, SpeedMessage2.Builder>(content, SpeedMessage2.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            Assert.AreEqual(content, SerializeMessage<SpeedMessage2, SpeedMessage2.Builder>(copy));
            Assert.AreEqual(TestResources.google_message2, copy.ToByteArray());
        }

    }
}
