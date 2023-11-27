#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf
{
    /// <summary>
    /// Struct used to hold the keys for the fieldByNumber table in DescriptorPool and the keys for the 
    /// extensionByNumber table in ExtensionRegistry.
    /// </summary>
    internal struct ObjectIntPair<T> : IEquatable<ObjectIntPair<T>> where T : class
    {
        private readonly int number;
        private readonly T obj;

        internal ObjectIntPair(T obj, int number)
        {
            this.number = number;
            this.obj = obj;
        }

        public bool Equals(ObjectIntPair<T> other)
        {
            return obj == other.obj
                   && number == other.number;
        }

        public override bool Equals(object obj) => obj is ObjectIntPair<T> pair && Equals(pair);

        public override int GetHashCode()
        {
            return obj.GetHashCode() * ((1 << 16) - 1) + number;
        }
    }
}
