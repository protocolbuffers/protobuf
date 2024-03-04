#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections.Generic;

namespace Google.Protobuf
{
    /// <summary>
    /// Struct used to hold the keys for the enumValuesByFullName table in DescriptorPool.
    /// </summary>
    internal struct ObjectFullNamePair<T> : IEquatable<ObjectFullNamePair<T>> where T : class
    {
        private readonly string fullName;
        private readonly string name;
        private readonly T obj;

        internal ObjectFullNamePair(T obj, string fullName, string name)
        {
            this.fullName = fullName;
            this.name = name;
            this.obj = obj;
        }

        public bool Equals(ObjectFullNamePair<T> other)
        {
            return obj == other.obj
                   && fullName == other.fullName
                   && name == other.name;
        }

        public override bool Equals(object obj) => obj is ObjectFullNamePair<T> pair && Equals(pair);

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = (fullName != null ? fullName.GetHashCode() : 0);
                hashCode = (hashCode * 397) ^ (name != null ? name.GetHashCode() : 0);
                hashCode = (hashCode * 397) ^ EqualityComparer<T>.Default.GetHashCode(obj);
                return hashCode;
            }
        }
    }
}
