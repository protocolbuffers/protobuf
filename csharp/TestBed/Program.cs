using System;
using System.Diagnostics;
using System.IO;

namespace TestBed {

  // Avoid using the .NET 3.5 System.Action delegate
  delegate void Action();

  class Program {

    private static readonly TimeSpan TimeLimit = TimeSpan.FromMinutes(1);
    private const int IterationsPerChunk = 50;

    static void Main(string[] args) {

      // Deserialize once to warm up the JIT and give us data use later
      byte[] data = File.ReadAllBytes(args[0]);
      Northwind.Database fast = Northwind.Database.ParseFrom(data);
      SlowNorthwind.Database slow = SlowNorthwind.Database.ParseFrom(data);

      Benchmark("Fast deserialize", () => Northwind.Database.ParseFrom(data));
      Benchmark("Fast serialize", () => fast.ToByteArray());
      Benchmark("Slow deserialize", () => SlowNorthwind.Database.ParseFrom(data));
      Benchmark("Slow serialize", () => slow.ToByteArray());
      //Console.ReadLine();
    }

    private static void Benchmark(string description, Action actionUnderTest) {
      int totalIterations = 0;
      Stopwatch sw = Stopwatch.StartNew();
      while (sw.Elapsed < TimeLimit) {
        for (int i = 0; i < IterationsPerChunk; i++) {
          actionUnderTest();
        }
        totalIterations += IterationsPerChunk;
      }
      sw.Stop();
      Console.WriteLine("{0}: {1} iterations in {2}ms; {3:f2}ms per iteration",
          description, totalIterations, sw.ElapsedMilliseconds,
          (double)sw.ElapsedMilliseconds / totalIterations);
    }
  }
}
