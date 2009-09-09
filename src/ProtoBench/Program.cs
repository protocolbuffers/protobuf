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
using System.Diagnostics;
using System.IO;

namespace Google.ProtocolBuffers.ProtoBench
{
  /// <summary>
  /// Simple benchmarking of arbitrary messages.
  /// </summary>
  public sealed class Program {

    private static readonly TimeSpan MinSampleTime = TimeSpan.FromSeconds(2);
    private static readonly TimeSpan TargetTime = TimeSpan.FromSeconds(30);

    // Avoid a .NET 3.5 dependency
    delegate void Action();

    public static int Main(string[] args) {
      if (args.Length < 2 || (args.Length % 2) != 0) {
        Console.Error.WriteLine("Usage: ProtoBench <descriptor type name> <input data>");
        Console.Error.WriteLine("The descriptor type name is the fully-qualified message name,");
        Console.Error.WriteLine("including assembly - e.g. Google.ProtocolBuffers.BenchmarkProtos.Message1,ProtoBench");
        Console.Error.WriteLine("(You can specify multiple pairs of descriptor type name and input data.)");
        return 1;
      }
      bool success = true;
      for (int i = 0; i < args.Length; i += 2) {
        success &= RunTest(args[i], args[i + 1]);
      }
      return success ? 0 : 1;
    }

    /// <summary>
    /// Runs a single test. Error messages are displayed to Console.Error, and the return value indicates
    /// general success/failure.
    /// </summary>
    public static bool RunTest(string typeName, string file) {
      Console.WriteLine("Benchmarking {0} with file {1}", typeName, file);
      IMessage defaultMessage;
      try {
        defaultMessage = MessageUtil.GetDefaultMessage(typeName);
      } catch (ArgumentException e) {
        Console.Error.WriteLine(e.Message);
        return false;
      }
      try {
        byte[] inputData = File.ReadAllBytes(file);
        MemoryStream inputStream = new MemoryStream(inputData);
        ByteString inputString = ByteString.CopyFrom(inputData);
        IMessage sampleMessage = defaultMessage.WeakCreateBuilderForType().WeakMergeFrom(inputString).WeakBuild();
        Benchmark("Serialize to byte string", inputData.Length, () => sampleMessage.ToByteString());
        Benchmark("Serialize to byte array", inputData.Length, () => sampleMessage.ToByteArray());
        Benchmark("Serialize to memory stream", inputData.Length, () => sampleMessage.WriteTo(new MemoryStream()));
        Benchmark("Deserialize from byte string", inputData.Length,
          () => defaultMessage.WeakCreateBuilderForType()
                              .WeakMergeFrom(inputString)
                              .WeakBuild()
        );
        Benchmark("Deserialize from byte array", inputData.Length, 
          () => defaultMessage.WeakCreateBuilderForType()
                              .WeakMergeFrom(CodedInputStream.CreateInstance(inputData))
                              .WeakBuild()
        );
        Benchmark("Deserialize from memory stream", inputData.Length, () => {
          inputStream.Position = 0;
          defaultMessage.WeakCreateBuilderForType()
            .WeakMergeFrom(CodedInputStream.CreateInstance(inputStream))
            .WeakBuild();
        });
        Console.WriteLine();
        return true;
      } catch (Exception e) {
        Console.Error.WriteLine("Error: {0}", e.Message);
        Console.Error.WriteLine();
        Console.Error.WriteLine("Detailed exception information: {0}", e);
        return false;
      }
    }

    private static void Benchmark(string name, long dataSize, Action action) {
      // Make sure it's JITted
      action();
      // Run it progressively more times until we've got a reasonable sample

      int iterations = 1;
      TimeSpan elapsed = TimeAction(action, iterations);
      while (elapsed < MinSampleTime) {
        iterations *= 2;
        elapsed = TimeAction(action, iterations);
      }
      // Upscale the sample to the target time. Do this in floating point arithmetic
      // to avoid overflow issues.
      iterations = (int) ((TargetTime.Ticks / (double)elapsed.Ticks) * iterations);
      elapsed = TimeAction(action, iterations);
      Console.WriteLine("{0}: {1} iterations in {2:f3}s; {3:f3}MB/s",
                        name, iterations, elapsed.TotalSeconds,
                        (iterations * dataSize) / (elapsed.TotalSeconds * 1024 * 1024));
    }

    private static TimeSpan TimeAction(Action action, int iterations) {
      GC.Collect();
      GC.WaitForPendingFinalizers();
      Stopwatch sw = Stopwatch.StartNew();
      for (int i = 0; i < iterations; i++) {
        action();
      }
      sw.Stop();
      return sw.Elapsed;
    }
  }
}