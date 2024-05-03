#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf.WellKnownTypes
{
    public partial class Value
    {
        /// <summary>
        /// Convenience method to create a Value message with a string value.
        /// </summary>
        /// <param name="value">Value to set for the StringValue property.</param>
        /// <returns>A newly-created Value message with the given value.</returns>
        public static Value ForString(string value)
        {
            ProtoPreconditions.CheckNotNull(value, nameof(value));
            return new Value { StringValue = value };
        }

        /// <summary>
        /// Convenience method to create a Value message with a number value.
        /// </summary>
        /// <param name="value">Value to set for the NumberValue property.</param>
        /// <returns>A newly-created Value message with the given value.</returns>
        public static Value ForNumber(double value)
        {
            return new Value { NumberValue = value };
        }

        /// <summary>
        /// Convenience method to create a Value message with a Boolean value.
        /// </summary>
        /// <param name="value">Value to set for the BoolValue property.</param>
        /// <returns>A newly-created Value message with the given value.</returns>
        public static Value ForBool(bool value)
        {
            return new Value { BoolValue = value };
        }

        /// <summary>
        /// Convenience method to create a Value message with a null initial value.
        /// </summary>
        /// <returns>A newly-created Value message a null initial value.</returns>
        public static Value ForNull()
        {
            return new Value { NullValue = 0 };
        }

        /// <summary>
        /// Convenience method to create a Value message with an initial list of values.
        /// </summary>
        /// <remarks>The values provided are not cloned; the references are copied directly.</remarks>
        /// <returns>A newly-created Value message an initial list value.</returns>
        public static Value ForList(params Value[] values)
        {
            ProtoPreconditions.CheckNotNull(values, nameof(values));
            return new Value { ListValue = new ListValue { Values = { values } } };
        }

        /// <summary>
        /// Convenience method to create a Value message with an initial struct value
        /// </summary>
        /// <remarks>The value provided is not cloned; the reference is copied directly.</remarks>
        /// <returns>A newly-created Value message an initial struct value.</returns>
        public static Value ForStruct(Struct value)
        {
            ProtoPreconditions.CheckNotNull(value, nameof(value));
            return new Value { StructValue = value };
        }
    }
}
