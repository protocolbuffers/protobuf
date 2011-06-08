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
using System.Threading;

namespace Google.ProtocolBuffers.ProtoBench
{
    /// <summary>
    /// Simple benchmarking of arbitrary messages.
    /// </summary>
    public sealed class Program
    {
        private static TimeSpan MinSampleTime = TimeSpan.FromSeconds(2);
        private static TimeSpan TargetTime = TimeSpan.FromSeconds(30);
        private static bool FastTest = false;
        private static bool Verbose = false;
        // Avoid a .NET 3.5 dependency
        private delegate void Action();

        private delegate void BenchmarkTest(string name, long dataSize, Action action);

        private static BenchmarkTest RunBenchmark;

        public static int Main(string[] args)
        {
            List<string> temp = new List<string>(args);

            FastTest = temp.Remove("/fast") || temp.Remove("-fast");
            Verbose = temp.Remove("/verbose") || temp.Remove("-verbose");

            RunBenchmark = BenchmarkV1;
            if (temp.Remove("/v2") || temp.Remove("-v2"))
            {
                string cpu = temp.Find(x => x.StartsWith("-cpu:"));
                int cpuIx = 1;
                if (cpu != null) cpuIx = 1 << Math.Max(0, int.Parse(cpu.Substring(5)));

                //pin the entire process to a single CPU
                Process.GetCurrentProcess().ProcessorAffinity = new IntPtr(cpuIx);
                Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.RealTime;
                RunBenchmark = BenchmarkV2;
            }
            args = temp.ToArray();

            if (args.Length < 2 || (args.Length%2) != 0)
            {
                Console.Error.WriteLine("Usage: ProtoBench [/fast] <descriptor type name> <input data>");
                Console.Error.WriteLine("The descriptor type name is the fully-qualified message name,");
                Console.Error.WriteLine("including assembly - e.g. Google.ProtocolBuffers.BenchmarkProtos.Message1,ProtoBench");
                Console.Error.WriteLine("(You can specify multiple pairs of descriptor type name and input data.)");
                return 1;
            }

            bool success = true;
            for (int i = 0; i < args.Length; i += 2)
            {
                success &= RunTest(args[i], args[i + 1]);
            }
            return success ? 0 : 1;
        }
        
        /// <summary>
        /// Runs a single test. Error messages are displayed to Console.Error, and the return value indicates
        /// general success/failure.
        /// </summary>
        public static bool RunTest(string typeName, string file)
        {
            Console.WriteLine("Benchmarking {0} with file {1}", typeName, file);
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
                byte[] inputData = File.ReadAllBytes(file);
                MemoryStream inputStream = new MemoryStream(inputData);
                ByteString inputString = ByteString.CopyFrom(inputData);
                IMessage sampleMessage =
                    defaultMessage.WeakCreateBuilderForType().WeakMergeFrom(inputString).WeakBuild();
                if(!FastTest) RunBenchmark("Serialize to byte string", inputData.Length, () => sampleMessage.ToByteString());
                RunBenchmark("Serialize to byte array", inputData.Length, () => sampleMessage.ToByteArray());
                if (!FastTest) RunBenchmark("Serialize to memory stream", inputData.Length,
                          () => sampleMessage.WriteTo(new MemoryStream()));
                if (!FastTest) RunBenchmark("Deserialize from byte string", inputData.Length,
                          () => defaultMessage.WeakCreateBuilderForType()
                                    .WeakMergeFrom(inputString)
                                    .WeakBuild()
                    );
                RunBenchmark("Deserialize from byte array", inputData.Length,
                          () => defaultMessage.WeakCreateBuilderForType()
                                    .WeakMergeFrom(CodedInputStream.CreateInstance(inputData))
                                    .WeakBuild()
                    );
                if (!FastTest) RunBenchmark("Deserialize from memory stream", inputData.Length, 
                    () => {
                      inputStream.Position = 0;
                      defaultMessage.WeakCreateBuilderForType().WeakMergeFrom(
                              CodedInputStream.CreateInstance(inputStream))
                          .WeakBuild();
                  });
                Console.WriteLine();
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
            TimeSpan elapsed = TimeSpan.Zero;
            long runs = 0;
            long totalCount = 0;
            double best = double.MinValue, worst = double.MaxValue;

            ThreadStart threadProc = 
                delegate()
                    {
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
                        iterations = (int)((target.Ticks * iterations) / (double)elapsed.Ticks);
                        elapsed = TimeAction(action, iterations);
                        iterations = (int)((target.Ticks * iterations) / (double)elapsed.Ticks);
                        elapsed = TimeAction(action, iterations);
                        iterations = (int)((target.Ticks * iterations) / (double)elapsed.Ticks);

                        double first = (iterations * dataSize) / (elapsed.TotalSeconds * 1024 * 1024);
                        if (Verbose) Console.WriteLine("Round ---: Count = {1,6}, Bps = {2,8:f3}", 0, iterations, first);
                        elapsed = TimeSpan.Zero;
                        int max = FastTest ? 10 : 30;

                        while (runs < max)
                        {
                            TimeSpan cycle = TimeAction(action, iterations);
                            // Accumulate and scale for next cycle.
                            
                            double bps = (iterations * dataSize) / (cycle.TotalSeconds * 1024 * 1024);
                            if (Verbose) Console.WriteLine("Round {0,3}: Count = {1,6}, Bps = {2,8:f3}", runs, iterations, bps);

                            best = Math.Max(best, bps);
                            worst = Math.Min(worst, bps);

                            runs++;
                            elapsed += cycle;
                            totalCount += iterations;
                            iterations = (int) ((target.Ticks*totalCount)/(double) elapsed.Ticks);
                        }
                    };

            Thread work = new Thread(threadProc);
            work.Name = "Worker";
            work.Priority = ThreadPriority.Highest;
            work.SetApartmentState(ApartmentState.STA);
            work.Start();
            work.Join();

            Console.WriteLine("{0}: averages {1} per {2:f3}s for {3} runs; avg: {4:f3}mbps; best: {5:f3}mbps; worst: {6:f3}mbps",
                              name, totalCount / runs, elapsed.TotalSeconds / runs, runs,
                              (totalCount * dataSize) / (elapsed.TotalSeconds * 1024 * 1024), best, worst);
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
            Console.WriteLine("{0}: {1} iterations in {2:f3}s; {3:f3}MB/s",
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
    }
}