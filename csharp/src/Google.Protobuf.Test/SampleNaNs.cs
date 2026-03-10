#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf
{
    /// <summary>
    /// Samples of different not-a-number values, for testing equality comparisons.
    /// </summary>
    public static class SampleNaNs
    {
        public static double Regular { get; } = double.NaN;

        // Signalling bit is inverted compared with double.NaN. Doesn't really matter
        // whether that makes it quiet or signalling - it's different.
        public static double SignallingFlipped { get; } =
            BitConverter.Int64BitsToDouble(BitConverter.DoubleToInt64Bits(double.NaN) ^ -0x8000_0000_0000_0000L);

        // A bit in the middle of the mantissa is flipped; this difference is preserved when casting to float.
        public static double PayloadFlipped { get; } =
            BitConverter.Int64BitsToDouble(BitConverter.DoubleToInt64Bits(double.NaN) ^ 0x1_0000_0000L);
    }
}
