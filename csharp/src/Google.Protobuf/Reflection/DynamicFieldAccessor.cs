#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections;
using System.Collections.Generic;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Implementation of <see cref="IFieldAccessor"/> for use with <see cref="DynamicMessage"/>.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This accessor provides reflective access to fields within a DynamicMessage.
    /// Unlike generated message accessors which use delegates to access CLR properties,
    /// this accessor directly manipulates the DynamicMessage's internal field storage.
    /// </para>
    /// </remarks>
    internal sealed class DynamicFieldAccessor : IFieldAccessor
    {
        private readonly FieldDescriptor descriptor;

        /// <summary>
        /// Creates a new DynamicFieldAccessor for the specified field.
        /// </summary>
        /// <param name="descriptor">The field descriptor.</param>
        internal DynamicFieldAccessor(FieldDescriptor descriptor)
        {
            this.descriptor = descriptor;
        }

        /// <inheritdoc/>
        public FieldDescriptor Descriptor => descriptor;

        /// <inheritdoc/>
        public void Clear(IMessage message)
        {
            if (message is DynamicMessage dynamicMessage)
            {
                dynamicMessage.ClearField(descriptor);
                return;
            }

            throw new ArgumentException(
                $"Message must be a DynamicMessage for field '{descriptor.FullName}'.",
                nameof(message));
        }

        /// <inheritdoc/>
        public object GetValue(IMessage message)
        {
            if (message is DynamicMessage dynamicMessage)
            {
                return dynamicMessage.GetField(descriptor);
            }

            throw new ArgumentException(
                $"Message must be a DynamicMessage for field '{descriptor.FullName}'.",
                nameof(message));
        }

        /// <inheritdoc/>
        public bool HasValue(IMessage message)
        {
            if (descriptor.IsMap)
            {
                // Map fields are always "present" but may be empty.
                return ((IDictionary)GetValue(message)).Count > 0;
            }
            if (descriptor.IsRepeated)
            {
                // Repeated fields are always "present" but may be empty.
                return ((IList)GetValue(message)).Count > 0;
            }

            if (message is DynamicMessage dynamicMessage)
            {
                return dynamicMessage.HasField(descriptor);
            }

            throw new ArgumentException(
                $"Message must be a DynamicMessage for field '{descriptor.FullName}'.",
                nameof(message));
        }

        /// <inheritdoc/>
        public void SetValue(IMessage message, object value)
        {
            if (message is DynamicMessage dynamicMessage)
            {
                if (descriptor.IsMap)
                {
                    // For map fields, replace the entire map
                    dynamicMessage.ClearField(descriptor);
                    if (value is IDictionary dict)
                    {
                        // Use SetField to set the map value
                        dynamicMessage.SetField(descriptor, ConvertToDictionary(dict));
                        return;
                    }
                    throw new ArgumentException("Value for map field must be a dictionary.", nameof(value));
                }

                if (descriptor.IsRepeated)
                {
                    // For repeated fields, replace the entire list
                    dynamicMessage.ClearField(descriptor);
                    if (value is IEnumerable enumerable)
                    {
                        foreach (var item in enumerable)
                        {
                            dynamicMessage.AddRepeatedField(descriptor, item);
                        }
                        return;
                    }
                    throw new ArgumentException("Value for repeated field must be an enumerable.", nameof(value));
                }

                dynamicMessage.SetField(descriptor, value);
                return;
            }

            throw new ArgumentException(
                $"Message must be a DynamicMessage for field '{descriptor.FullName}'.",
                nameof(message));
        }

        private static Dictionary<object, object> ConvertToDictionary(IDictionary source)
        {
            var result = new Dictionary<object, object>();
            foreach (DictionaryEntry entry in source)
            {
                result[entry.Key] = entry.Value;
            }
            return result;
        }
    }
}
