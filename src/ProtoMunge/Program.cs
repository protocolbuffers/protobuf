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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoMunge
{
  /// <summary>
  /// Utility console application which takes a message descriptor and a corresponding message,
  /// and produces a new message with similar but random data. The data is the same length
  /// as the original, but with random values within appropriate bands. (For instance, a compressed
  /// integer in the range 0-127 will end up as another integer in the same range, to keep the length
  /// the same.)
  /// TODO(jonskeet): Potentially refactor to use an instance instead, making it simpler to
  /// be thread-safe for external use.
  /// </summary>
  public sealed class Program {

    static readonly Random rng = new Random();

    static int Main(string[] args) {
      if (args.Length != 3) {
        Console.Error.WriteLine("Usage: ProtoMunge <descriptor type name> <input data> <output file>");
        Console.Error.WriteLine("The descriptor type name is the fully-qualified message name, including assembly.");
        Console.Error.WriteLine("(At a future date it may be possible to do this without building the .NET assembly at all.)");
        return 1;
      }
      IMessage defaultMessage;
      try {
        defaultMessage = MessageUtil.GetDefaultMessage(args[0]);
      } catch (ArgumentException e) {
        Console.Error.WriteLine(e.Message);
        return 1;
      }
      try {
        IBuilder builder = defaultMessage.WeakCreateBuilderForType();
        byte[] inputData = File.ReadAllBytes(args[1]);
        builder.WeakMergeFrom(ByteString.CopyFrom(inputData));
        IMessage original = builder.WeakBuild();
        IMessage munged = Munge(original);
        if (original.SerializedSize != munged.SerializedSize) {
          throw new Exception("Serialized sizes don't match");
        }
        File.WriteAllBytes(args[2], munged.ToByteArray());
        return 0;
      } catch (Exception e) {
        Console.Error.WriteLine("Error: {0}", e.Message);
        Console.Error.WriteLine();
        Console.Error.WriteLine("Detailed exception information: {0}", e);
        return 1;
      }
    }

    /// <summary>
    /// Munges a message recursively.
    /// </summary>
    /// <returns>A new message of the same type as the original message,
    /// but munged so that all the data is desensitised.</returns>
    private static IMessage Munge(IMessage message) {
      IBuilder builder = message.WeakCreateBuilderForType();
      foreach (var pair in message.AllFields) {
        if (pair.Key.IsRepeated) {
          foreach (object singleValue in (IEnumerable)pair.Value) {
            builder.WeakAddRepeatedField(pair.Key, CheckedMungeValue(pair.Key, singleValue));
          }
        } else {
          builder[pair.Key] = CheckedMungeValue(pair.Key, pair.Value);
        }
      }
      IMessage munged = builder.WeakBuild();
      if (message.SerializedSize != munged.SerializedSize) {
        Console.WriteLine("Sub message sizes: {0}/{1}", message.SerializedSize, munged.SerializedSize);
      }
      return munged;
    }

    /// <summary>
    /// Munges a single value and checks that the length ends up the same as it was before.
    /// </summary>
    private static object CheckedMungeValue(FieldDescriptor fieldDescriptor, object value) {
      int currentSize = CodedOutputStream.ComputeFieldSize(fieldDescriptor.FieldType, fieldDescriptor.FieldNumber, value);
      object mungedValue = MungeValue(fieldDescriptor, value);
      int mungedSize = CodedOutputStream.ComputeFieldSize(fieldDescriptor.FieldType, fieldDescriptor.FieldNumber, mungedValue);
      // Exceptions log more easily than assertions
      if (currentSize != mungedSize) {
        throw new Exception("Munged value had wrong size. Field type: " + fieldDescriptor.FieldType
            + "; old value: " + value + "; new value: " + mungedValue);
      }
      return mungedValue;
    }

    /// <summary>
    /// Munges a single value of the specified field descriptor. (i.e. if the field is
    /// actually a repeated int, this method receives a single int value to munge, and
    /// is called multiple times).
    /// </summary>
    private static object MungeValue(FieldDescriptor fieldDescriptor, object value) {
      switch (fieldDescriptor.FieldType) {
        case FieldType.SInt64:
        case FieldType.Int64:
          return (long) MungeVarint64((ulong) (long)value);
        case FieldType.UInt64:
          return MungeVarint64((ulong)value);
        case FieldType.SInt32:
          return (int)MungeVarint32((uint)(int)value);
        case FieldType.Int32:
          return MungeInt32((int) value);
        case FieldType.UInt32:
          return MungeVarint32((uint)value);
        case FieldType.Double:
          return rng.NextDouble();
        case FieldType.Float:
          return (float)rng.NextDouble();
        case FieldType.Fixed64: {
          byte[] data = new byte[8];
          rng.NextBytes(data);
          return BitConverter.ToUInt64(data, 0);
        }
        case FieldType.Fixed32:  {
          byte[] data = new byte[4];
          rng.NextBytes(data);
          return BitConverter.ToUInt32(data, 0);
        }
        case FieldType.Bool:
          return rng.Next(2) == 1;
        case FieldType.String:
          return MungeString((string)value);
        case FieldType.Group:
        case FieldType.Message:
          return Munge((IMessage)value);
        case FieldType.Bytes:
          return MungeByteString((ByteString)value);
        case FieldType.SFixed64: {
            byte[] data = new byte[8];
            rng.NextBytes(data);
            return BitConverter.ToInt64(data, 0);
          }
        case FieldType.SFixed32: {
            byte[] data = new byte[4];
            rng.NextBytes(data);
            return BitConverter.ToInt32(data, 0);
          }
        case FieldType.Enum:
          return MungeEnum(fieldDescriptor, (EnumValueDescriptor) value);
        default:
          // TODO(jonskeet): Different exception?
          throw new ArgumentException("Invalid field descriptor");
      }
    }

    private static object MungeString(string original) {
      foreach (char c in original) {
        if (c > 127) {
          throw new ArgumentException("Can't handle non-ascii yet");
        }
      }
      char[] chars = new char[original.Length];
      // Convert to pure ASCII - no control characters.
      for (int i = 0; i < chars.Length; i++) {
        chars[i] = (char) rng.Next(32, 127);
      }
      return new string(chars);
    }

    /// <summary>
    /// Int32 fields are slightly strange - we need to keep the sign the same way it is:
    /// negative numbers can munge to any other negative number (it'll always take
    /// 10 bytes) but positive numbers have to stay positive, so we can't use the
    /// full range of 32 bits.
    /// </summary>
    private static int MungeInt32(int value) {
      if (value < 0) {
        return rng.Next(int.MinValue, 0);
      }
      int length = CodedOutputStream.ComputeRawVarint32Size((uint) value);
      uint min = length == 1 ? 0 : 1U << ((length - 1) * 7);
      uint max = length == 5 ? int.MaxValue : (1U << (length * 7)) - 1;
      return (int) NextRandomUInt64(min, max);
    }

    private static uint MungeVarint32(uint original) {
      int length = CodedOutputStream.ComputeRawVarint32Size(original);
      uint min = length == 1 ? 0 : 1U << ((length - 1) * 7);
      uint max = length == 5 ? uint.MaxValue : (1U << (length * 7)) - 1;
      return (uint)NextRandomUInt64(min, max);
    }

    private static ulong MungeVarint64(ulong original) {
      int length = CodedOutputStream.ComputeRawVarint64Size(original);
      ulong min = length == 1 ? 0 : 1UL << ((length - 1) * 7);
      ulong max = length == 10 ? ulong.MaxValue : (1UL<< (length * 7)) - 1;
      return NextRandomUInt64(min, max);
    }

    /// <summary>
    /// Returns a random number in the range [min, max] (both inclusive).
    /// </summary>    
    private static ulong NextRandomUInt64(ulong min, ulong max) {
      if (min > max) {
        throw new ArgumentException("min must be <= max; min=" + min + "; max = " + max);
      }
      ulong range = max - min;
      // This isn't actually terribly good at very large ranges - but it doesn't really matter for the sake
      // of this program.
      return min + (ulong)(range * rng.NextDouble());
    }

    private static object MungeEnum(FieldDescriptor fieldDescriptor, EnumValueDescriptor original) {
      // Find all the values which get encoded to the same size as the current value, and pick one at random
      int originalSize = CodedOutputStream.ComputeRawVarint32Size((uint)original.Number);
      List<EnumValueDescriptor> sameSizeValues = new List<EnumValueDescriptor> ();
      foreach (EnumValueDescriptor candidate in fieldDescriptor.EnumType.Values) {
        if (CodedOutputStream.ComputeRawVarint32Size((uint)candidate.Number) == originalSize) {
          sameSizeValues.Add(candidate);
        }
      }
      return sameSizeValues[rng.Next(sameSizeValues.Count)];
    }

    private static object MungeByteString(ByteString byteString) {
      byte[] data = new byte[byteString.Length];
      rng.NextBytes(data);
      return ByteString.CopyFrom(data);
    }
  }
}