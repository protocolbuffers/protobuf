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

        private readonly SortedList<int, IEnumLite> items;

        public EnumLiteMap()
        {
            items = new SortedList<int, IEnumLite>();
#if SILVERLIGHT
    // Silverlight doesn't support Enum.GetValues
    // TODO(jonskeet): Validate that this reflection is permitted, e.g. in Windows Phone 7
            foreach (System.Reflection.FieldInfo fi in typeof(TEnum).GetFields(System.Reflection.BindingFlags.Static | System.Reflection.BindingFlags.Public))
            {
                TEnum evalue = (TEnum) fi.GetValue(null);
                items.Add(Convert.ToInt32(evalue), new EnumValue(evalue));
            }
#else
            foreach (TEnum evalue in Enum.GetValues(typeof (TEnum)))
            {
                items.Add(Convert.ToInt32(evalue), new EnumValue(evalue));
            }
#endif
        }

        IEnumLite IEnumLiteMap.FindValueByNumber(int number)
        {
            return FindValueByNumber(number);
        }

        public IEnumLite FindValueByNumber(int number)
        {
            IEnumLite val;
            return items.TryGetValue(number, out val) ? val : null;
        }

        public IEnumLite FindValueByName(string name)
        {
            IEnumLite val;
            if (Enum.IsDefined(typeof (TEnum), name))
            {
                return items.TryGetValue((int) Enum.Parse(typeof (TEnum), name, false), out val) ? val : null;
            }
            return null;
        }

        public bool IsValidValue(IEnumLite value)
        {
            return items.ContainsKey(value.Number);
        }
    }
}