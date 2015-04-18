#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Threading;
using System.Xml;
using Google.ProtocolBuffers.Serialization;
using Google.ProtocolBuffers.TestProtos;

namespace Google.ProtocolBuffers.ProtoBench
{
    /// <summary>
    /// Simple benchmarking of arbitrary messages.
    /// </summary>
    public sealed class Program
    {
        private static TimeSpan MinSampleTime = TimeSpan.FromSeconds(2);
        private static TimeSpan TargetTime = TimeSpan.FromSeconds(30);
        private static bool Verbose = false, FastTest = false, OtherFormats = false;
        // Avoid a .NET 3.5 dependency
        private delegate void Action();

        private delegate void BenchmarkTest(string name, long dataSize, Action action);

        private static BenchmarkTest RunBenchmark;

        private static string _logFile;
        static void WriteLine(string format, params object[] arg)
        {
            if (arg.Length > 0) format = String.Format(format, arg);
            Console.Out.WriteLine(format);
            if (!String.IsNullOrEmpty(_logFile))
                File.AppendAllText(_logFile, format + Environment.NewLine);
        }

        [STAThread]
        public static int Main(string[] args)
        {
            List<string> temp = new List<string>(args);

            Verbose = temp.Remove("/verbose") || temp.Remove("-verbose");
            OtherFormats = temp.Remove("/formats") || temp.Remove("-formats");

            foreach (string arg in temp)
            {
                if (arg.StartsWith("/log:", StringComparison.OrdinalIgnoreCase) || arg.StartsWith("-log:", StringComparison.OrdinalIgnoreCase))
                {
                    _logFile = arg.Substring(5);
                    if (!String.IsNullOrEmpty(_logFile))
                        File.AppendAllText(_logFile, Environment.NewLine + "Started benchmarks at " + DateTime.Now + Environment.NewLine);
                    temp.Remove(arg);
                    break;
                }
            }

            if (true == (FastTest = (temp.Remove("/fast") || temp.Remove("-fast"))))
            {
                TargetTime = TimeSpan.FromSeconds(10);
            }

            RunBenchmark = BenchmarkV1;
            if (temp.Remove("/v2") || temp.Remove("-v2"))
            {
                Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.RealTime;
                Process.GetCurrentProcess().ProcessorAffinity = new IntPtr(1);
                RunBenchmark = BenchmarkV2;
            }
            if (temp.Remove("/all") || temp.Remove("-all"))
            {
                if (FastTest)
                {
                    TargetTime = TimeSpan.FromSeconds(5);
                }
                foreach (KeyValuePair<string, string> item in MakeTests())
                {
                    temp.Add(item.Key);
                    temp.Add(item.Value);
                }
            }
            args = temp.ToArray();

            if (args.Length < 2 || (args.Length%2) != 0)
            {
                Console.Error.WriteLine("Usage: ProtoBench [/fast] <descriptor type name> <input data>");
                Console.Error.WriteLine("The descriptor type name is the fully-qualified message name,");
                Console.Error.WriteLine(
                    "including assembly - e.g. Google.ProtocolBuffers.BenchmarkProtos.Message1,ProtoBench");
                Console.Error.WriteLine("(You can specify multiple pairs of descriptor type name and input data.)");
                return 1;
            }

            bool success = true;
            for (int i = 0; i < args.Length; i += 2)
            {
                success &= RunTest(args[i], args[i + 1], null);
            }
            return success ? 0 : 1;
        }

        /// <summary>
        /// Runs a single test. Error messages are displayed to Console.Error, and the return value indicates
        /// general success/failure.
        /// </summary>
        public static bool RunTest(string typeName, string file, byte[] inputData)
        {
            WriteLine("Benchmarking {0} with file {1}", typeName, file);
            IMessage defaultMessage;
            try
            {
                defaultMessage = MessageUtil.GetDefaultMessage(typeName);
            }
            catch (ArgumentException e)
            {
                Console.Error.WriteLine(e.Message);
                return false;
            }
            try
            {
                ExtensionRegistry registry = ExtensionRegistry.Empty;
                inputData = inputData ?? File.ReadAllBytes(file);
                MemoryStream inputStream = new MemoryStream(inputData);
                ByteString inputString = ByteString.CopyFrom(inputData);
                IMessage sampleMessage =
                    defaultMessage.WeakCreateBuilderForType().WeakMergeFrom(inputString, registry).WeakBuild();

                IDictionary<string, object> dictionary = null;
                byte[] jsonBytes = null, xmlBytes = null; /*no pun intended, well... maybe for xml*/
                if (OtherFormats)
                {
                    using (MemoryStream temp = new MemoryStream())
                    {
                        XmlFormatWriter.CreateInstance(temp).WriteMessage(sampleMessage);
                        xmlBytes = temp.ToArray();
                    }
                    using (MemoryStream temp = new MemoryStream())
                    {
                        JsonFormatWriter.CreateInstance(temp).WriteMessage(sampleMessage);
                        jsonBytes = temp.ToArray();
                    }
                    dictionary = new Dictionary<string, object>(StringComparer.Ordinal);
                    new DictionaryWriter(dictionary).WriteMessage(sampleMessage);
                }

                //Serializers
                if (!FastTest)
                {
                    RunBenchmark("Serialize to byte string", inputData.Length, () => sampleMessage.ToByteString());
                }
                RunBenchmark("Serialize to byte array", inputData.Length, () => sampleMessage.ToByteArray());
                if (!FastTest)
                {
                    RunBenchmark("Serialize to memory stream", inputData.Length,
                                 () => sampleMessage.WriteTo(new MemoryStream()));
                }

                if (OtherFormats)
                {
                    RunBenchmark("Serialize to xml", xmlBytes.Length,
                                 () =>
                                     {
                                         XmlFormatWriter.CreateInstance(new MemoryStream(), Encoding.UTF8).WriteMessage(sampleMessage);
                                     });
                    RunBenchmark("Serialize to json", jsonBytes.Length,
                                 () => { JsonFormatWriter.CreateInstance().WriteMessage(sampleMessage); });
                    RunBenchmark("Serialize to json via xml", jsonBytes.Length,
                                 () =>
                                 XmlFormatWriter.CreateInstance(
                                     JsonReaderWriterFactory.CreateJsonWriter(new MemoryStream(), Encoding.UTF8))
                                     .SetOptions(XmlWriterOptions.OutputJsonTypes)
                                     .WriteMessage(sampleMessage)
                        );

                    RunBenchmark("Serialize to dictionary", sampleMessage.SerializedSize,
                                 () => new DictionaryWriter().WriteMessage(sampleMessage));
                }
                //Deserializers
                if (!FastTest)
                {
                    RunBenchmark("Deserialize from byte string", inputData.Length,
                                 () => defaultMessage.WeakCreateBuilderForType()
                                           .WeakMergeFrom(inputString, registry)
                                           .WeakBuild()
                        );
                }

                RunBenchmark("Deserialize from byte array", inputData.Length,
                             () => defaultMessage.WeakCreateBuilderForType()
                                       .WeakMergeFrom(CodedInputStream.CreateInstance(inputData), registry)
                                       .WeakBuild()
                    );
                if (!FastTest)
                {
                    RunBenchmark("Deserialize from memory stream", inputData.Length,
                                 () =>
                                     {
                                         inputStream.Position = 0;
                                         defaultMessage.WeakCreateBuilderForType().WeakMergeFrom(
                                             CodedInputStream.CreateInstance(inputStream), registry)
                                             .WeakBuild();
                                     });
                }

                if (OtherFormats)
                {
                    RunBenchmark("Deserialize from xml", xmlBytes.Length,
                                 () =>
                                 XmlFormatReader.CreateInstance(xmlBytes).Merge(
                                     defaultMessage.WeakCreateBuilderForType()).WeakBuild());
                    RunBenchmark("Deserialize from json", jsonBytes.Length,
                                 () =>
                                 JsonFormatReader.CreateInstance(jsonBytes).Merge(
                                     defaultMessage.WeakCreateBuilderForType()).WeakBuild());
                    RunBenchmark("Deserialize from json via xml", jsonBytes.Length,
                                 () =>
                                 XmlFormatReader.CreateInstance(JsonReaderWriterFactory.CreateJsonReader(jsonBytes, XmlDictionaryReaderQuotas.Max))
                                     .SetOptions(XmlReaderOptions.ReadNestedArrays).Merge(
                                         defaultMessage.WeakCreateBuilderForType()).WeakBuild());

                    RunBenchmark("Deserialize from dictionary", sampleMessage.SerializedSize,
                                 () =>
                                 new DictionaryReader(dictionary).Merge(defaultMessage.WeakCreateBuilderForType()).
                                     WeakBuild());
                }
                WriteLine(String.Empty);
                return true;
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("Error: {0}", e.Message);
                Console.Error.WriteLine();
                Console.Error.WriteLine("Detailed exception information: {0}", e);
                return false;
            }
        }

        private static void BenchmarkV2(string name, long dataSize, Action action)
        {
            Thread.BeginThreadAffinity();
            TimeSpan elapsed = TimeSpan.Zero;
            long runs = 0;
            long totalCount = 0;
            double best = double.MinValue, worst = double.MaxValue;

            action();
            // Run it progressively more times until we've got a reasonable sample

            int iterations = 100;
            elapsed = TimeAction(action, iterations);
            while (elapsed.TotalMilliseconds < 1000)
            {
                elapsed += TimeAction(action, iterations);
                iterations *= 2;
            }

            TimeSpan target = TimeSpan.FromSeconds(1);

            elapsed = TimeAction(action, iterations);
            iterations = (int) ((target.Ticks*iterations)/(double) elapsed.Ticks);
            elapsed = TimeAction(action, iterations);
            iterations = (int) ((target.Ticks*iterations)/(double) elapsed.Ticks);
            elapsed = TimeAction(action, iterations);
            iterations = (int) ((target.Ticks*iterations)/(double) elapsed.Ticks);

            double first = (iterations*dataSize)/(elapsed.TotalSeconds*1024*1024);
            if (Verbose)
            {
                WriteLine("Round ---: Count = {1,6}, Bps = {2,8:f3}", 0, iterations, first);
            }
            elapsed = TimeSpan.Zero;
            int max = (int) TargetTime.TotalSeconds;

            while (runs < max)
            {
                TimeSpan cycle = TimeAction(action, iterations);
                // Accumulate and scale for next cycle.

                double bps = (iterations*dataSize)/(cycle.TotalSeconds*1024*1024);
                if (Verbose)
                {
                    WriteLine("Round {1,3}: Count = {2,6}, Bps = {3,8:f3}",
                                      0, runs, iterations, bps);
                }

                best = Math.Max(best, bps);
                worst = Math.Min(worst, bps);

                runs++;
                elapsed += cycle;
                totalCount += iterations;
                iterations = (int) ((target.Ticks*totalCount)/(double) elapsed.Ticks);
            }

            Thread.EndThreadAffinity();
            WriteLine(
                "{1}: averages {2} per {3:f3}s for {4} runs; avg: {5:f3}mbps; best: {6:f3}mbps; worst: {7:f3}mbps",
                0, name, totalCount/runs, elapsed.TotalSeconds/runs, runs,
                (totalCount*dataSize)/(elapsed.TotalSeconds*1024*1024), best, worst);
        }

        private static void BenchmarkV1(string name, long dataSize, Action action)
        {
            // Make sure it's JITted
            action();
            // Run it progressively more times until we've got a reasonable sample

            int iterations = 1;
            TimeSpan elapsed = TimeAction(action, iterations);
            while (elapsed < MinSampleTime)
            {
                iterations *= 2;
                elapsed = TimeAction(action, iterations);
            }
            // Upscale the sample to the target time. Do this in floating point arithmetic
            // to avoid overflow issues.
            iterations = (int) ((TargetTime.Ticks/(double) elapsed.Ticks)*iterations);
            elapsed = TimeAction(action, iterations);
            WriteLine("{0}: {1} iterations in {2:f3}s; {3:f3}MB/s",
                              name, iterations, elapsed.TotalSeconds,
                              (iterations*dataSize)/(elapsed.TotalSeconds*1024*1024));
        }

        private static TimeSpan TimeAction(Action action, int iterations)
        {
            GC.Collect();
            GC.GetTotalMemory(true);
            GC.WaitForPendingFinalizers();

            Stopwatch sw = Stopwatch.StartNew();
            for (int i = 0; i < iterations; i++)
            {
                action();
            }
            sw.Stop();
            return sw.Elapsed;
        }

        private static IEnumerable<KeyValuePair<string, string>> MakeTests()
        {
            //Aggregate Tests
            yield return MakeWorkItem("all-types", MakeTestAllTypes());
            yield return MakeWorkItem("repeated-100", MakeRepeatedTestAllTypes(100));
            yield return MakeWorkItem("packed-100", MakeTestPackedTypes(100));

            //Discrete Tests
            foreach (KeyValuePair<string, Action<TestAllTypes.Builder>> item in MakeTestAllTypes())
            {
                yield return MakeWorkItem(item.Key, new[] {item});
            }

            foreach (KeyValuePair<string, Action<TestAllTypes.Builder>> item in MakeRepeatedTestAllTypes(100))
            {
                yield return MakeWorkItem(item.Key, new[] {item});
            }

            foreach (KeyValuePair<string, Action<TestPackedTypes.Builder>> item in MakeTestPackedTypes(100))
            {
                yield return MakeWorkItem(item.Key, new[] {item});
            }
        }

        private static IEnumerable<KeyValuePair<string, Action<TestAllTypes.Builder>>> MakeTestAllTypes()
        {
            // Many of the raw type serializers below perform poorly due to the numerous fields defined
            // in TestAllTypes.

            //single values
            yield return MakeItem<TestAllTypes.Builder>("int32", 1, x => x.SetOptionalInt32(1001));
            yield return MakeItem<TestAllTypes.Builder>("int64", 1, x => x.SetOptionalInt64(1001));
            yield return MakeItem<TestAllTypes.Builder>("uint32", 1, x => x.SetOptionalUint32(1001));
            yield return MakeItem<TestAllTypes.Builder>("uint64", 1, x => x.SetOptionalUint64(1001));
            yield return MakeItem<TestAllTypes.Builder>("sint32", 1, x => x.SetOptionalSint32(-1001));
            yield return MakeItem<TestAllTypes.Builder>("sint64", 1, x => x.SetOptionalSint64(-1001));
            yield return MakeItem<TestAllTypes.Builder>("fixed32", 1, x => x.SetOptionalFixed32(1001));
            yield return MakeItem<TestAllTypes.Builder>("fixed64", 1, x => x.SetOptionalFixed64(1001));
            yield return MakeItem<TestAllTypes.Builder>("sfixed32", 1, x => x.SetOptionalSfixed32(-1001));
            yield return MakeItem<TestAllTypes.Builder>("sfixed64", 1, x => x.SetOptionalSfixed64(-1001));
            yield return MakeItem<TestAllTypes.Builder>("float", 1, x => x.SetOptionalFloat(1001.1001f));
            yield return MakeItem<TestAllTypes.Builder>("double", 1, x => x.SetOptionalDouble(1001.1001));
            yield return MakeItem<TestAllTypes.Builder>("bool", 1, x => x.SetOptionalBool(true));
            yield return MakeItem<TestAllTypes.Builder>("string", 1, x => x.SetOptionalString("this is a string value"))
                ;
            yield return
                MakeItem<TestAllTypes.Builder>("bytes", 1,
                                               x =>
                                               x.SetOptionalBytes(ByteString.CopyFromUtf8("this is an array of bytes")))
                ;
            yield return
                MakeItem<TestAllTypes.Builder>("group", 1,
                                               x =>
                                               x.SetOptionalGroup(
                                                   new TestAllTypes.Types.OptionalGroup.Builder().SetA(1001)));
            yield return
                MakeItem<TestAllTypes.Builder>("message", 1,
                                               x =>
                                               x.SetOptionalNestedMessage(
                                                   new TestAllTypes.Types.NestedMessage.Builder().SetBb(1001)));
            yield return
                MakeItem<TestAllTypes.Builder>("enum", 1,
                                               x => x.SetOptionalNestedEnum(TestAllTypes.Types.NestedEnum.FOO));
        }

        private static IEnumerable<KeyValuePair<string, Action<TestAllTypes.Builder>>> MakeRepeatedTestAllTypes(int size)
        {
            //repeated values
            yield return MakeItem<TestAllTypes.Builder>("repeated-int32", size, x => x.AddRepeatedInt32(1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-int64", size, x => x.AddRepeatedInt64(1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-uint32", size, x => x.AddRepeatedUint32(1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-uint64", size, x => x.AddRepeatedUint64(1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-sint32", size, x => x.AddRepeatedSint32(-1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-sint64", size, x => x.AddRepeatedSint64(-1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-fixed32", size, x => x.AddRepeatedFixed32(1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-fixed64", size, x => x.AddRepeatedFixed64(1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-sfixed32", size, x => x.AddRepeatedSfixed32(-1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-sfixed64", size, x => x.AddRepeatedSfixed64(-1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-float", size, x => x.AddRepeatedFloat(1001.1001f));
            yield return MakeItem<TestAllTypes.Builder>("repeated-double", size, x => x.AddRepeatedDouble(1001.1001));
            yield return MakeItem<TestAllTypes.Builder>("repeated-bool", size, x => x.AddRepeatedBool(true));
            yield return
                MakeItem<TestAllTypes.Builder>("repeated-string", size,
                                               x => x.AddRepeatedString("this is a string value"));
            yield return
                MakeItem<TestAllTypes.Builder>("repeated-bytes", size,
                                               x =>
                                               x.AddRepeatedBytes(ByteString.CopyFromUtf8("this is an array of bytes")))
                ;
            yield return
                MakeItem<TestAllTypes.Builder>("repeated-group", size,
                                               x =>
                                               x.AddRepeatedGroup(
                                                   new TestAllTypes.Types.RepeatedGroup.Builder().SetA(1001)));
            yield return
                MakeItem<TestAllTypes.Builder>("repeated-message", size,
                                               x =>
                                               x.AddRepeatedNestedMessage(
                                                   new TestAllTypes.Types.NestedMessage.Builder().SetBb(1001)));
            yield return
                MakeItem<TestAllTypes.Builder>("repeated-enum", size,
                                               x => x.AddRepeatedNestedEnum(TestAllTypes.Types.NestedEnum.FOO));
        }

        private static IEnumerable<KeyValuePair<string, Action<TestPackedTypes.Builder>>> MakeTestPackedTypes(int size)
        {
            //packed values
            yield return MakeItem<TestPackedTypes.Builder>("packed-int32", size, x => x.AddPackedInt32(1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-int64", size, x => x.AddPackedInt64(1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-uint32", size, x => x.AddPackedUint32(1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-uint64", size, x => x.AddPackedUint64(1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-sint32", size, x => x.AddPackedSint32(-1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-sint64", size, x => x.AddPackedSint64(-1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-fixed32", size, x => x.AddPackedFixed32(1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-fixed64", size, x => x.AddPackedFixed64(1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-sfixed32", size, x => x.AddPackedSfixed32(-1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-sfixed64", size, x => x.AddPackedSfixed64(-1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-float", size, x => x.AddPackedFloat(1001.1001f));
            yield return MakeItem<TestPackedTypes.Builder>("packed-double", size, x => x.AddPackedDouble(1001.1001));
            yield return MakeItem<TestPackedTypes.Builder>("packed-bool", size, x => x.AddPackedBool(true));
            yield return
                MakeItem<TestPackedTypes.Builder>("packed-enum", size, x => x.AddPackedEnum(ForeignEnum.FOREIGN_FOO));
        }

        private static KeyValuePair<string, Action<T>> MakeItem<T>(string name, int repeated, Action<T> build)
            where T : IBuilderLite, new()
        {
            if (repeated == 1)
            {
                return new KeyValuePair<string, Action<T>>(name, build);
            }

            return new KeyValuePair<string, Action<T>>(
                String.Format("{0}[{1}]", name, repeated),
                x =>
                    {
                        for (int i = 0; i < repeated; i++)
                        {
                            build(x);
                        }
                    }
                );
        }

        private static KeyValuePair<string, string> MakeWorkItem<T>(string name,
                                                                    IEnumerable<KeyValuePair<string, Action<T>>>
                                                                        builders) where T : IBuilderLite, new()
        {
            T builder = new T();

            foreach (KeyValuePair<string, Action<T>> item in builders)
            {
                item.Value(builder);
            }

            IMessageLite msg = builder.WeakBuild();
            string fname = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "unittest_" + name + ".dat");
            File.WriteAllBytes(fname, msg.ToByteArray());
            return
                new KeyValuePair<string, string>(
                    String.Format("{0},{1}", msg.GetType().FullName, msg.GetType().Assembly.GetName().Name), fname);
        }
    }
}