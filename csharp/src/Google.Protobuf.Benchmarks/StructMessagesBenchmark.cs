using System;
using System.IO;
using BenchmarkDotNet.Attributes;
using Google.Protobuf.TestProtos;
using Microsoft.Diagnostics.Runtime.Interop;

namespace Google.Protobuf.Benchmarks;

/// <summary>
/// Benchmark for struct vs class marshalling
/// </summary>
[MemoryDiagnoser]
public class StructMessagesBenchmark
{
    private byte[] _buffer;
    private TestPoint _testPoint;
    private TestPoint _testPoint2;
    private TestPointOptimized _testPointOptimized;
    private TestPointOptimized _testPointOptimized2;

    public StructMessagesBenchmark()
    {
        _buffer = new byte[64 * 1024];
        _testPoint = new TestPoint()
        {
            FirstPoint = new Point() { X = 1, Y = 2 },
            NestedPoint = new NestedPoint() { Left = new Point() { X = 1, Y = 2 }, Right = new Point() { X = 1, Y = 2 } }
        };
        _testPoint2 = new TestPoint();
        _testPointOptimized = new TestPointOptimized()
        {
            FirstPoint = new PointOptimized() { X = 1, Y = 2 },
            NestedPoint = new NestedPointOptimized() { Left = new PointOptimized() { X = 1, Y = 2 }, Right = new PointOptimized() { X = 1, Y = 2 } }
        };
        _testPointOptimized2 = new TestPointOptimized();

        const int pointCount = 64;

        for (int i = 0; i < pointCount; i++)
        {
            _testPoint.RemainingNestedPoints.Add(new NestedPoint() { Left = new Point() { X = 1, Y = 2 }, Right = new Point() { X = 3, Y = 4 } });
            _testPointOptimized.RemainingNestedPoints.Add(new NestedPointOptimized() { Left = new PointOptimized() { X = 1, Y = 2 }, Right = new PointOptimized() { X = 3, Y = 4 } });
        }
    }

    [Benchmark]
    public void ManagedMessage()
    {
        var output = new CodedOutputStream(_buffer);
        _testPoint.WriteTo(output);
        output.Flush();

        _testPoint2.RemainingNestedPoints.Clear();
        _testPoint2.MergeFrom(new CodedInputStream(_buffer, 0, (int)output.Position));
    }

    [Benchmark]
    public void StructMessage()
    {
        var output = new CodedOutputStream(_buffer);
        _testPointOptimized.WriteTo(output);
        output.Flush();

        _testPointOptimized2.RemainingNestedPoints.Clear();
        _testPointOptimized2.MergeFrom(new CodedInputStream(_buffer, 0, (int)output.Position));
    }
}