using System;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NUnit.Framework;

namespace Google.Protobuf.Collections;

public class UnsafeCollectionOperationsTest
{
    [Test]
    public unsafe void NullFieldAsSpanValueType()
    {
        RepeatedField<int> field = null;
        var span = UnsafeCollectionOperations.AsSpan(field);

        Assert.AreEqual(0, span.Length);

        fixed (int* pSpan = span)
        {
            Assert.True(pSpan == null);
        }
    }

    [Test]
    public void NullFieldAsSpanClass()
    {
        RepeatedField<object> field = null;
        var span = UnsafeCollectionOperations.AsSpan(field);

        Assert.AreEqual(0, span.Length);
    }

    [Test]
    public void AsSpanOnConcurrentFieldAcccessThrows()
    {
        var field = new RepeatedField<int>();
        field.count = 100;

        Assert.Throws<InvalidOperationException>(() => UnsafeCollectionOperations.AsSpan(field));
    }

    [Test]
    public void FieldAsSpanValueType()
    {
        var field = new RepeatedField<int>();
        foreach (var length in Enumerable.Range(0, 36))
        {
            field.Clear();
            ValidateContentEquality(field, UnsafeCollectionOperations.AsSpan(field));

            for (var i = 0; i < length; i++)
            {
                field.Add(i);
            }
            ValidateContentEquality(field, UnsafeCollectionOperations.AsSpan(field));

            field.Add(length + 1);
            ValidateContentEquality(field, UnsafeCollectionOperations.AsSpan(field));
        }

        static void ValidateContentEquality(RepeatedField<int> field, Span<int> span)
        {
            Assert.AreEqual(field.Count, span.Length);

            for (var i = 0; i < span.Length; i++)
            {
                Assert.AreEqual(field[i], span[i]);
            }
        }
    }

    [Test]
    public void FieldAsSpanClass()
    {
        var field = new RepeatedField<IntAsObject>();
        foreach (var length in Enumerable.Range(0, 36))
        {
            field.Clear();
            ValidateContentEquality(field, UnsafeCollectionOperations.AsSpan(field));

            for (var i = 0; i < length; i++)
            {
                field.Add(new IntAsObject { Value = i });
            }
            ValidateContentEquality(field, UnsafeCollectionOperations.AsSpan(field));

            field.Add(new IntAsObject { Value = length + 1 });
            ValidateContentEquality(field, UnsafeCollectionOperations.AsSpan(field));
        }

        static void ValidateContentEquality(RepeatedField<IntAsObject> field, Span<IntAsObject> span)
        {
            Assert.AreEqual(field.Count, span.Length);

            for (var i = 0; i < span.Length; i++)
            {
                Assert.AreEqual(field[i].Value, span[i].Value);
            }
        }
    }

    [Test]
    public void FieldAsSpanLinkBreaksOnResize()
    {
        var field = new RepeatedField<int>();

        for (var i = 0; i < 8; i++)
        {
            field.Add(i);
        }

        var span = UnsafeCollectionOperations.AsSpan(field);

        var startCapacity = field.Capacity;
        var startCount = field.Count;
        Assert.AreEqual(startCount, startCapacity);
        Assert.AreEqual(startCount, span.Length);

        for (var i = 0; i < span.Length; i++)
        {
            span[i]++;
            Assert.AreEqual(field[i], span[i]);

            field[i]++;
            Assert.AreEqual(field[i], span[i]);
        }

        // Resize to break link between Span and RepeatedField
        field.Add(11);

        Assert.AreNotEqual(startCapacity, field.Capacity);
        Assert.AreNotEqual(startCount, field.Count);
        Assert.AreEqual(startCount, span.Length);

        for (var i = 0; i < span.Length; i++)
        {
            span[i] += 2;
            Assert.AreNotEqual(field[i], span[i]);

            field[i] += 3;
            Assert.AreNotEqual(field[i], span[i]);
        }
    }

    [Test]
    public void FieldSetCount()
    {
        RepeatedField<int> field = null;
        Assert.Throws<NullReferenceException>(() => UnsafeCollectionOperations.SetCount(field, 3));

        Assert.Throws<ArgumentOutOfRangeException>(() => UnsafeCollectionOperations.SetCount(field, -1));

        field = new RepeatedField<int>();
        Assert.Throws<ArgumentOutOfRangeException>(() => UnsafeCollectionOperations.SetCount(field, -1));

        UnsafeCollectionOperations.SetCount(field, 5);
        Assert.AreEqual(5, field.Count);

        field = new RepeatedField<int> { 1, 2, 3, 4, 5 };
        ref var intRef = ref MemoryMarshal.GetReference(UnsafeCollectionOperations.AsSpan(field));

        // make sure that size decrease preserves content
        UnsafeCollectionOperations.SetCount(field, 3);
        Assert.AreEqual(3, field.Count);
        Assert.Throws<ArgumentOutOfRangeException>(() => field[3] = 42);
        SequenceEqual(UnsafeCollectionOperations.AsSpan(field), new[] { 1, 2, 3 });
        Assert.True(Unsafe.AreSame(ref intRef, ref MemoryMarshal.GetReference(UnsafeCollectionOperations.AsSpan(field))));

        // make sure that size increase preserves content and doesn't clear
        UnsafeCollectionOperations.SetCount(field, 5);
        SequenceEqual(UnsafeCollectionOperations.AsSpan(field), new [] { 1, 2, 3, 4, 5 });
        Assert.True(Unsafe.AreSame(ref intRef, ref MemoryMarshal.GetReference(UnsafeCollectionOperations.AsSpan(field))));

        // make sure that reallocations preserve content
        var newCount = field.Capacity * 2;
        UnsafeCollectionOperations.SetCount(field, newCount);
        Assert.AreEqual(newCount, field.Count);
        SequenceEqual(UnsafeCollectionOperations.AsSpan(field).Slice(0, 3), new[] { 1, 2, 3 });
        Assert.True(!Unsafe.AreSame(ref intRef, ref MemoryMarshal.GetReference(UnsafeCollectionOperations.AsSpan(field))));

        RepeatedField<string> listReference = new() { "a", "b", "c", "d", "e" };
        ref var stringRef = ref MemoryMarshal.GetReference(UnsafeCollectionOperations.AsSpan(listReference));
        UnsafeCollectionOperations.SetCount(listReference, 3);

        // verify that reference types aren't cleared
        SequenceEqual(UnsafeCollectionOperations.AsSpan(listReference), new [] { "a", "b", "c" });
        Assert.True(Unsafe.AreSame(ref stringRef, ref MemoryMarshal.GetReference(UnsafeCollectionOperations.AsSpan(listReference))));
        UnsafeCollectionOperations.SetCount(listReference, 5);

        // verify that removed reference types are cleared
        SequenceEqual(UnsafeCollectionOperations.AsSpan(listReference), new [] { "a", "b", "c", null, null });
        Assert.True(Unsafe.AreSame(ref stringRef, ref MemoryMarshal.GetReference(UnsafeCollectionOperations.AsSpan(listReference))));
    }

    private static void SequenceEqual<T>(Span<T> span,  Span<T> expected)
    {
        Assert.AreEqual(expected.Length, span.Length);
        for (var i = 0; i < expected.Length; i++)
        {
            Assert.AreEqual(expected[i], span[i]);
        }
    }

    private class IntAsObject
    {
        public int Value;
        public int Property { get; set; }
    }
}