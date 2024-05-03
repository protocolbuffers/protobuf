#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using Google.Protobuf.Reflection;

namespace Google.Protobuf.WellKnownTypes
{
    // Manually-written partial class for the FieldMask well-known type.
    public partial class FieldMask : ICustomDiagnosticMessage
    {
        private const char FIELD_PATH_SEPARATOR = ',';
        private const char FIELD_SEPARATOR_REGEX = '.';

        /// <summary>
        /// Converts a field mask specified by paths to a string.
        /// </summary>
        /// <remarks>
        /// If the value is a normalized duration in the range described in <c>field_mask.proto</c>,
        /// <paramref name="diagnosticOnly"/> is ignored. Otherwise, if the parameter is <c>true</c>,
        /// a JSON object with a warning is returned; if it is <c>false</c>, an <see cref="InvalidOperationException"/> is thrown.
        /// </remarks>
        /// <param name="paths">Paths in the field mask</param>
        /// <param name="diagnosticOnly">Determines the handling of non-normalized values</param>
        /// <exception cref="InvalidOperationException">The represented field mask is invalid, and <paramref name="diagnosticOnly"/> is <c>false</c>.</exception>
        internal static string ToJson(IList<string> paths, bool diagnosticOnly)
        {
            var firstInvalid = paths.FirstOrDefault(p => !IsPathValid(p));
            if (firstInvalid == null)
            {
                var writer = new StringWriter();
                JsonFormatter.WriteString(writer, string.Join(",", paths.Select(JsonFormatter.ToJsonName)));
                return writer.ToString();
            }
            else
            {
                if (diagnosticOnly)
                {
                    var writer = new StringWriter();
                    writer.Write("{ \"@warning\": \"Invalid FieldMask\", \"paths\": ");
                    JsonFormatter.Default.WriteList(writer, (IList)paths);
                    writer.Write(" }");
                    return writer.ToString();
                }
                else
                {
                    throw new InvalidOperationException($"Invalid field mask to be converted to JSON: {firstInvalid}");
                }
            }
        }

        /// <summary>
        /// Returns a string representation of this <see cref="FieldMask"/> for diagnostic purposes.
        /// </summary>
        /// <remarks>
        /// Normally the returned value will be a JSON string value (including leading and trailing quotes) but
        /// when the value is non-normalized or out of range, a JSON object representation will be returned
        /// instead, including a warning. This is to avoid exceptions being thrown when trying to
        /// diagnose problems - the regular JSON formatter will still throw an exception for non-normalized
        /// values.
        /// </remarks>
        /// <returns>A string representation of this value.</returns>
        public string ToDiagnosticString()
        {
            return ToJson(Paths, true);
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
        public static FieldMask FromString<[DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]T>(string value) where T : IMessage
        {
            return FromStringEnumerable<T>(new List<string>(value.Split(FIELD_PATH_SEPARATOR)));
        }

        /// <summary>
        /// Constructs a FieldMask for a list of field paths in a certain type.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static FieldMask FromStringEnumerable<[DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]T>(IEnumerable<string> paths) where T : IMessage
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
        public static FieldMask FromFieldNumbers<[DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]T>(params int[] fieldNumbers) where T : IMessage
        {
            return FromFieldNumbers<T>((IEnumerable<int>)fieldNumbers);
        }

        /// <summary>
        /// Constructs a FieldMask from the passed field numbers.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static FieldMask FromFieldNumbers<[DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]T>(IEnumerable<int> fieldNumbers) where T : IMessage
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
        /// Checks whether the given path is valid for a field mask.
        /// </summary>
        /// <returns>true if the path is valid; false otherwise</returns>
        private static bool IsPathValid(string input)
        {
            for (int i = 0; i < input.Length; i++)
            {
                char c = input[i];
                if (c >= 'A' && c <= 'Z')
                {
                    return false;
                }
                if (c == '_' && i < input.Length - 1)
                {
                    char next = input[i + 1];
                    if (next < 'a' || next > 'z')
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        /// <summary>
        /// Checks whether paths in a given fields mask are valid.
        /// </summary>
        /// <typeparam name="T">The type to validate the field paths against.</typeparam>
        public static bool IsValid<[DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]T>(FieldMask fieldMask) where T : IMessage
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
        public static bool IsValid<[DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]T>(string path) where T : IMessage
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
        /// Converts this FieldMask to its canonical form. In the canonical form of a
        /// FieldMask, all field paths are sorted alphabetically and redundant field
        /// paths are removed.
        /// </summary>
        public FieldMask Normalize()
        {
            return new FieldMaskTree(this).ToFieldMask();
        }

        /// <summary>
        /// Creates a union of two or more FieldMasks.
        /// </summary>
        public FieldMask Union(params FieldMask[] otherMasks)
        {
            var maskTree = new FieldMaskTree(this);
            foreach (var mask in otherMasks)
            {
                maskTree.MergeFromFieldMask(mask);
            }

            return maskTree.ToFieldMask();
        }

        /// <summary>
        /// Calculates the intersection of two FieldMasks.
        /// </summary>
        public FieldMask Intersection(FieldMask additionalMask)
        {
            var tree = new FieldMaskTree(this);
            var result = new FieldMaskTree();
            foreach (var path in additionalMask.Paths)
            {
                tree.IntersectFieldPath(path, result);
            }

            return result.ToFieldMask();
        }

        /// <summary>
        /// Merges fields specified by this FieldMask from one message to another with the
        /// specified merge options.
        /// </summary>
        public void Merge(IMessage source, IMessage destination, MergeOptions options)
        {
            new FieldMaskTree(this).Merge(source, destination, options);
        }

        /// <summary>
        /// Merges fields specified by this FieldMask from one message to another.
        /// </summary>
        public void Merge(IMessage source, IMessage destination)
        {
            Merge(source, destination, new MergeOptions());
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
