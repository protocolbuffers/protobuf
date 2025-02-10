#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

#if !NET6_0_OR_GREATER

// Polyglot for easier definition of the tests. Assume that nullability is unknown prior to .NET 6.
namespace System.Reflection
{
    internal enum NullabilityState
    {
        Unknown,
        NotNull,
        Nullable
    }

    internal sealed class NullabilityInfo
    {
        public static readonly NullabilityInfo Default = new();

        private NullabilityInfo()
        {
        }

        public NullabilityState ReadState => NullabilityState.Unknown;
        public NullabilityState WriteState => NullabilityState.Unknown;
    }

    internal sealed class NullabilityInfoContext
    {
        // ReSharper disable once ParameterOnlyUsedForPreconditionCheck.Global
        public NullabilityInfo Create(PropertyInfo propertyInfo)
        {
            if (propertyInfo is null)
            {
                throw new ArgumentNullException(nameof(propertyInfo));
            }

            return NullabilityInfo.Default;
        }
    }
}

#endif