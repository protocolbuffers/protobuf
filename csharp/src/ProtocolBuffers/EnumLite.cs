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
using System.Collections.Generic;

namespace Google.ProtocolBuffers
{
    ///<summary>
    ///Interface for an enum value or value descriptor, to be used in FieldSet.
    ///The lite library stores enum values directly in FieldSets but the full
    ///library stores EnumValueDescriptors in order to better support reflection.
    ///</summary>
    public interface IEnumLite
    {
        int Number { get; }
        string Name { get; }
    }

    ///<summary>
    ///Interface for an object which maps integers to {@link EnumLite}s.
    ///{@link Descriptors.EnumDescriptor} implements this interface by mapping
    ///numbers to {@link Descriptors.EnumValueDescriptor}s.  Additionally,
    ///every generated enum type has a static method internalGetValueMap() which
    ///returns an implementation of this type that maps numbers to enum values.
    ///</summary>
    public interface IEnumLiteMap<T> : IEnumLiteMap
        where T : IEnumLite
    {
        new T FindValueByNumber(int number);
    }

    public interface IEnumLiteMap
    {
        bool IsValidValue(IEnumLite value);
        IEnumLite FindValueByNumber(int number);
        IEnumLite FindValueByName(string name);
    }

    public class EnumLiteMap<TEnum> : IEnumLiteMap<IEnumLite>
        where TEnum : struct, IComparable, IFormattable
    {
        private struct EnumValue : IEnumLite
        {
            private readonly TEnum value;

            public EnumValue(TEnum value)
            {
                this.value = value;
            }

            int IEnumLite.Number
            {
                get { return Convert.ToInt32(value); }
            }

            string IEnumLite.Name
            {
                get { return value.ToString(); }
            }
        }

        public IEnumLite FindValueByNumber(int number)
        {
            TEnum val = default(TEnum);
            if (EnumParser<TEnum>.TryConvert(number, ref val))
            {
                return new EnumValue(val);
            }
            return null;
        }

        public IEnumLite FindValueByName(string name)
        {
            TEnum val = default(TEnum);
            if (EnumParser<TEnum>.TryConvert(name, ref val))
            {
                return new EnumValue(val);
            }
            return null;
        }

        public bool IsValidValue(IEnumLite value)
        {
            TEnum val = default(TEnum);
            return EnumParser<TEnum>.TryConvert(value.Number, ref val);
        }
    }

    public static class EnumParser<T> where T : struct, IComparable, IFormattable
    {
        private static readonly Dictionary<int, T> _byNumber;
        private static Dictionary<string, T> _byName;

        static EnumParser()
        {
            int[] array;
            try
            {
#if CLIENTPROFILE
                // It will actually be a T[], but the CLR will let us convert.
                array = (int[])Enum.GetValues(typeof(T));
#else
                var temp = new List<T>();
                foreach (var fld in typeof (T).GetFields(System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.Static))
                {
                    if (fld.IsLiteral && fld.FieldType == typeof(T))
                    {
                        temp.Add((T)fld.GetValue(null));
                    }
                }
                array = (int[])(object)temp.ToArray();
#endif
            }
            catch
            {
                _byNumber = null;
                return;
            }

            _byNumber = new Dictionary<int, T>(array.Length);
            foreach (int i in array)
            {
                _byNumber[i] = (T)(object)i;
            }
        }

        public static bool TryConvert(object input, ref T value)
        {
            if (input is int || input is T)
            {
                return TryConvert((int)input, ref value);
            }
            if (input is string)
            {
                return TryConvert((string)input, ref value);
            }
            return false;
        }

        /// <summary>
        /// Tries to convert an integer to its enum representation. This would take an out parameter,
        /// but the caller uses ref, so this approach is simpler.
        /// </summary>
        public static bool TryConvert(int number, ref T value)
        {
            // null indicates an exception at construction, use native IsDefined.
            if (_byNumber == null)
            {
                return Enum.IsDefined(typeof(T), number);
            }
            T converted;
            if (_byNumber != null && _byNumber.TryGetValue(number, out converted))
            {
                value = converted;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Tries to convert a string to its enum representation. This would take an out parameter,
        /// but the caller uses ref, so this approach is simpler.
        /// </summary>
        public static bool TryConvert(string name, ref T value)
        {
            // null indicates an exception at construction, use native IsDefined/Parse.
            if (_byNumber == null)
            {
                if (Enum.IsDefined(typeof(T), name))
                {
                    value = (T)Enum.Parse(typeof(T), name, false);
                    return true;
                }
                return false;
            }

            // known race, possible multiple threads each build their own copy; however, last writer will win
            var map = _byName;
            if (map == null)
            {
                map = new Dictionary<string, T>(StringComparer.Ordinal);
                foreach (var possible in _byNumber.Values)
                {
                    map[possible.ToString()] = possible;
                }
                _byName = map;
            }

            T converted;
            if (map.TryGetValue(name, out converted))
            {
                value = converted;
                return true;
            }

            return false;
        }
    }
}