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
using System;
using System.Collections;
using System.Collections.Generic;

namespace Google.ProtocolBuffers.Collections
{
    /// <summary>
    /// Utility class for dictionaries.
    /// </summary>
    public static class Dictionaries
    {
        /// <summary>
        /// Compares two dictionaries for equality. Each value is compared with equality using Equals
        /// for non-IEnumerable implementations, and using EnumerableEquals otherwise.
        /// TODO(jonskeet): This is clearly pretty slow, and involves lots of boxing/unboxing...
        /// </summary>
        public static bool Equals<TKey, TValue>(IDictionary<TKey, TValue> left, IDictionary<TKey, TValue> right)
        {
            if (left.Count != right.Count)
            {
                return false;
            }
            foreach (KeyValuePair<TKey, TValue> leftEntry in left)
            {
                TValue rightValue;
                if (!right.TryGetValue(leftEntry.Key, out rightValue))
                {
                    return false;
                }

                IEnumerable leftEnumerable = leftEntry.Value as IEnumerable;
                IEnumerable rightEnumerable = rightValue as IEnumerable;
                if (leftEnumerable == null || rightEnumerable == null)
                {
                    if (!Equals(leftEntry.Value, rightValue))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!Enumerables.Equals(leftEnumerable, rightEnumerable))
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        public static IDictionary<TKey, TValue> AsReadOnly<TKey, TValue>(IDictionary<TKey, TValue> dictionary)
        {
            return dictionary.IsReadOnly ? dictionary : new ReadOnlyDictionary<TKey, TValue>(dictionary);
        }

        /// <summary>
        /// Creates a hashcode for a dictionary by XORing the hashcodes of all the fields
        /// and values. (By XORing, we avoid ordering issues.)
        /// TODO(jonskeet): Currently XORs other stuff too, and assumes non-null values.
        /// </summary>
        public static int GetHashCode<TKey, TValue>(IDictionary<TKey, TValue> dictionary)
        {
            int ret = 31;
            foreach (KeyValuePair<TKey, TValue> entry in dictionary)
            {
                int hash = entry.Key.GetHashCode() ^ GetDeepHashCode(entry.Value);
                ret ^= hash;
            }
            return ret;
        }

        /// <summary>
        /// Determines the hash of a value by either taking it directly or hashing all the elements
        /// for IEnumerable implementations.
        /// </summary>
        private static int GetDeepHashCode(object value)
        {
            IEnumerable iterable = value as IEnumerable;
            if (iterable == null)
            {
                return value.GetHashCode();
            }
            int hash = 29;
            foreach (object element in iterable)
            {
                hash = hash*37 + element.GetHashCode();
            }
            return hash;
        }
    }
}