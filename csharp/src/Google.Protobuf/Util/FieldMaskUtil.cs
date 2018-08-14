#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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
using System.Text;
using Google.Protobuf.Reflection;
using Google.Protobuf.WellKnownTypes;

namespace Google.Protobuf.Util
{
    public static class FieldMaskUtil
    {
        private const char FIELD_PATH_SEPARATOR = ',';
        private const char FIELD_SEPARATOR_REGEX = '.';

        /// <summary>
        /// Converts a FieldMask to a string.
        /// </summary>
        public static string ToString(FieldMask mask)
        {
            var result = new StringBuilder();
            var first = true;
            foreach (var value in mask.Paths)
            {
                if (value.Length == 0)
                {
                    // Ignore empty paths.
                    continue;
                }
                if (first)
                {
                    first = false;
                }
                else
                {
                    result.Append(FIELD_PATH_SEPARATOR);
                }
                result.Append(value);
            }
            return result.ToString();
        }

        /// <summary>
        /// Parses from a string to a FieldMask.
        /// </summary>
        public static FieldMask FromString(string value)
        {
            return FromStringEnumerable<Empty>(new List<string>(value.Split(FIELD_PATH_SEPARATOR)));
        }

        /// <summary>
        /// Parses from a string to a FieldMask and validates all field paths.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static FieldMask FromString<T>(string value) where T : IMessage
        {
            return FromStringEnumerable<T>(new List<string>(value.Split(FIELD_PATH_SEPARATOR)));
        }

        /// <summary>
        /// Constructs a FieldMask for a list of field paths in a certain type.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static FieldMask FromStringEnumerable<T>(IEnumerable<string> paths) where T : IMessage
        {
            var mask = new FieldMask();
            foreach (var path in paths)
            {
                if (path.Length == 0)
                {
                    // Ignore empty field paths.
                    continue;
                }

                if (typeof(T) != typeof(Empty)
                    && !IsValid<T>(path))
                {
                    throw new InvalidProtocolBufferException(path + " is not a valid path for " + typeof(T));
                }

                mask.Paths.Add(path);
            }

            return mask;
        }

        /// <summary>
        /// Constructs a FieldMask from the passed field numbers.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static FieldMask FromFieldNumbers<T>(params int[] fieldNumbers) where T : IMessage
        {
            return FromFieldNumbers<T>((IEnumerable<int>)fieldNumbers);
        }

        /// <summary>
        /// Constructs a FieldMask from the passed field numbers.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static FieldMask FromFieldNumbers<T>(IEnumerable<int> fieldNumbers) where T : IMessage
        {
            var descriptor = Activator.CreateInstance<T>().Descriptor;

            var mask = new FieldMask();
            foreach (var fieldNumber in fieldNumbers)
            {
                var field = descriptor.FindFieldByNumber(fieldNumber);
                if (field == null)
                {
                    throw new ArgumentNullException($"{fieldNumber} is not a valid field number for {descriptor.Name}");
                }

                mask.Paths.Add(field.Name);
            }

            return mask;
        }

        /// <summary>
        /// Converts a field mask to a Proto3 JSON string, that is converting from snake case to camel 
        /// case and joining all paths into one string with commas.
        /// </summary>
        public static string ToJsonString(FieldMask fieldMask)
        {
            var paths = new List<string>(fieldMask.Paths.Count);
            foreach (var path in fieldMask.Paths)
            {
                if (path.Length == 0)
                {
                    continue;
                }

                paths.Add(JsonFormatter.ToJsonName(path));
            }

            return string.Join(FIELD_PATH_SEPARATOR.ToString(), paths);
        }

        /// <summary>
        /// Converts a field mask from a Proto3 JSON string, that is splitting the paths along commas and 
        /// converting from camel case to snake case.
        /// </summary>
        public static FieldMask FromJsonString(string value)
        {
            var paths = value.Split(FIELD_PATH_SEPARATOR);
            var mask = new FieldMask();
            foreach (var path in paths)
            {
                if (path.Length == 0)
                {
                    continue;
                }

                mask.Paths.Add(JsonFormatter.FromJsonName(path));
            }
            return mask;
        }

        /// <summary>
        /// Checks whether paths in a given fields mask are valid.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static bool IsValid<T>(FieldMask fieldMask) where T : IMessage
        {
            var descriptor = Activator.CreateInstance<T>().Descriptor;

            return IsValid(descriptor, fieldMask);
        }

        /// <summary>
        /// Checks whether paths in a given fields mask are valid.
        /// </summary>
        public static bool IsValid(MessageDescriptor descriptor, FieldMask fieldMask)
        {
            foreach (var path in fieldMask.Paths)
            {
                if (!IsValid(descriptor, path))
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Checks whether a given field path is valid.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static bool IsValid<T>(string path) where T : IMessage
        {
            var descriptor = Activator.CreateInstance<T>().Descriptor;

            return IsValid(descriptor, path);
        }

        /// <summary>
        /// Checks whether paths in a given fields mask are valid.
        /// </summary>
        public static bool IsValid(MessageDescriptor descriptor, string path)
        {
            var parts = path.Split(FIELD_SEPARATOR_REGEX);
            if (parts.Length == 0)
            {
                return false;
            }

            foreach (var name in parts)
            {
                var field = descriptor?.FindFieldByName(name);
                if (field == null)
                {
                    return false;
                }

                if (!field.IsRepeated
                    && field.FieldType == FieldType.Message)
                {
                    descriptor = field.MessageType;
                }
                else
                {
                    descriptor = null;
                }
            }

            return true;
        }

        /// <summary>
        /// Converts a FieldMask to its canonical form. In the canonical form of a
        /// FieldMask, all field paths are sorted alphabetically and redundant field
        /// paths are removed.
        /// </summary>
        public static FieldMask Normalize(FieldMask mask)
        {
            return new FieldMaskTree(mask).ToFieldMask();
        }

        /// <summary>
        /// Creates a union of two or more FieldMasks.
        /// </summary>
        public static FieldMask Union(FieldMask baseMask, params FieldMask[] otherMasks)
        {
            var maskTree = new FieldMaskTree(baseMask);
            foreach (var mask in otherMasks)
            {
                maskTree.MergeFromFieldMask(mask);
            }

            return maskTree.ToFieldMask();
        }

        /// <summary>
        /// Calculates the intersection of two FieldMasks.
        /// </summary>
        public static FieldMask Intersection(FieldMask mask1, FieldMask mask2)
        {
            var tree = new FieldMaskTree(mask1);
            var result = new FieldMaskTree();
            foreach (var path in mask2.Paths)
            {
                tree.IntersectFieldPath(path, result);
            }

            return result.ToFieldMask();
        }

        /// <summary>
        /// Merges fields specified by a FieldMask from one message to another with the
        /// specified merge options.
        /// </summary>
        public static void Merge(FieldMask mask, IMessage source, IMessage destination, MergeOptions options)
        {
            new FieldMaskTree(mask).Merge(source, destination, options);
        }

        /// <summary>
        /// Merges fields specified by a FieldMask from one message to another.
        /// </summary>
        public static void Merge(FieldMask mask, IMessage source, IMessage destination)
        {
            Merge(mask, source, destination, new MergeOptions());
        }

        /// <summary>
        /// Options to customize merging behavior.
        /// </summary>
        public sealed class MergeOptions
        {
            /// <summary>
            /// Whether to replace message fields(i.e., discard existing content in
            /// destination message fields) when merging.
            /// Default behavior is to merge the source message field into the
            /// destination message field.
            /// </summary>
            public bool ReplaceMessageFields { get; set; } = false;

            /// <summary>
            /// Whether to replace repeated fields (i.e., discard existing content in
            /// destination repeated fields) when merging.
            /// Default behavior is to append elements from source repeated field to the
            /// destination repeated field.
            /// </summary>
            public bool ReplaceRepeatedFields { get; set; } = false;

            /// <summary>
            /// Whether to replace primitive (non-repeated and non-message) fields in
            /// destination message fields with the source primitive fields (i.e., if the
            /// field is set in the source, the value is copied to the
            /// destination; if the field is unset in the source, the field is cleared
            /// from the destination) when merging.
            /// 
            /// Default behavior is to always set the value of the source primitive
            /// field to the destination primitive field, and if the source field is
            /// unset, the default value of the source field is copied to the
            /// destination.
            /// </summary>
            public bool ReplacePrimitiveFields { get; set; } = false;
        }
    }
}
