#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using Google.Protobuf.Reflection;
using Google.Protobuf.WellKnownTypes;

namespace Google.Protobuf
{
    /// <summary>
    /// <para>A tree representation of a FieldMask. Each leaf node in this tree represent
    /// a field path in the FieldMask.</para>
    ///
    /// <para>For example, FieldMask "foo.bar,foo.baz,bar.baz" as a tree will be:</para>
    /// <code>
    ///   [root] -+- foo -+- bar
    ///           |       |
    ///           |       +- baz
    ///           |
    ///           +- bar --- baz
    /// </code>
    ///
    /// <para>By representing FieldMasks with this tree structure we can easily convert
    /// a FieldMask to a canonical form, merge two FieldMasks, calculate the
    /// intersection to two FieldMasks and traverse all fields specified by the
    /// FieldMask in a message tree.</para>
    /// </summary>
    internal sealed class FieldMaskTree
    {
        private const char FIELD_PATH_SEPARATOR = '.';

        internal sealed class Node
        {
            public Dictionary<string, Node> Children { get; } = new Dictionary<string, Node>();
        }

        private readonly Node root = new Node();

        /// <summary>
        /// Creates an empty FieldMaskTree.
        /// </summary>
        public FieldMaskTree()
        {
        }

        /// <summary>
        /// Creates a FieldMaskTree for a given FieldMask.
        /// </summary>
        public FieldMaskTree(FieldMask mask)
        {
            MergeFromFieldMask(mask);
        }

        public override string ToString()
        {
            return ToFieldMask().ToString();
        }

        /// <summary>
        /// Adds a field path to the tree. In a FieldMask, every field path matches the
        /// specified field as well as all its sub-fields. For example, a field path
        /// "foo.bar" matches field "foo.bar" and also "foo.bar.baz", etc. When adding
        /// a field path to the tree, redundant sub-paths will be removed. That is,
        /// after adding "foo.bar" to the tree, "foo.bar.baz" will be removed if it
        /// exists, which will turn the tree node for "foo.bar" to a leaf node.
        /// Likewise, if the field path to add is a sub-path of an existing leaf node,
        /// nothing will be changed in the tree.
        /// </summary>
        public FieldMaskTree AddFieldPath(string path)
        {
            var parts = path.Split(FIELD_PATH_SEPARATOR);
            if (parts.Length == 0)
            {
                return this;
            }

            var node = root;
            var createNewBranch = false;

            // Find the matching node in the tree.
            foreach (var part in parts)
            {
                // Check whether the path matches an existing leaf node.
                if (!createNewBranch
                    && node != root
                    && node.Children.Count == 0)
                {
                    // The path to add is a sub-path of an existing leaf node.
                    return this;
                }

                if (!node.Children.TryGetValue(part, out Node childNode))
                {
                    createNewBranch = true;
                    childNode = new Node();
                    node.Children.Add(part, childNode);
                }
                node = childNode;
            }

            // Turn the matching node into a leaf node (i.e., remove sub-paths).
            node.Children.Clear();
            return this;
        }

        /// <summary>
        /// Merges all field paths in a FieldMask into this tree.
        /// </summary>
        public FieldMaskTree MergeFromFieldMask(FieldMask mask)
        {
            foreach (var path in mask.Paths)
            {
                AddFieldPath(path);
            }

            return this;
        }

        /// <summary>
        /// Converts this tree to a FieldMask.
        /// </summary>
        public FieldMask ToFieldMask()
        {
            var mask = new FieldMask();
            if (root.Children.Count != 0)
            {
                var paths = new List<string>();
                GetFieldPaths(root, "", paths);
                mask.Paths.AddRange(paths);
            }

            return mask;
        }

        /// <summary>
        /// Gathers all field paths in a sub-tree.
        /// </summary>
        private void GetFieldPaths(Node node, string path, List<string> paths)
        {
            if (node.Children.Count == 0)
            {
                paths.Add(path);
                return;
            }

            foreach (var entry in node.Children)
            {
                var childPath = path.Length == 0 ? entry.Key : path + "." + entry.Key;
                GetFieldPaths(entry.Value, childPath, paths);
            }
        }

        /// <summary>
        /// Adds the intersection of this tree with the given <paramref name="path"/> to <paramref name="output"/>.
        /// </summary>
        public void IntersectFieldPath(string path, FieldMaskTree output)
        {
            if (root.Children.Count == 0)
            {
                return;
            }

            var parts = path.Split(FIELD_PATH_SEPARATOR);
            if (parts.Length == 0)
            {
                return;
            }

            var node = root;
            foreach (var part in parts)
            {
                if (node != root
                    && node.Children.Count == 0)
                {
                    // The given path is a sub-path of an existing leaf node in the tree.
                    output.AddFieldPath(path);
                    return;
                }

                if (!node.Children.TryGetValue(part, out node))
                {
                    return;
                }
            }

            // We found a matching node for the path. All leaf children of this matching
            // node is in the intersection.
            var paths = new List<string>();
            GetFieldPaths(node, path, paths);
            foreach (var value in paths)
            {
                output.AddFieldPath(value);
            }
        }

        /// <summary>
        /// Merges all fields specified by this FieldMaskTree from <paramref name="source"/> to <paramref name="destination"/>.
        /// </summary>
        public void Merge(IMessage source, IMessage destination, FieldMask.MergeOptions options)
        {
            if (source.Descriptor != destination.Descriptor)
            {
                throw new InvalidProtocolBufferException("Cannot merge messages of different types.");
            }

            if (root.Children.Count == 0)
            {
                return;
            }

            Merge(root, "", source, destination, options);
        }

        /// <summary>
        /// Maps an arbitrary key to the appropriate type given a FieldDescriptor.
        /// </summary>
        /// <param name="key"></param>
        /// <param name="keyDescriptor"></param>
        /// <returns>the key parsed to the correct type provided by <paramref name="key"/> or null, if not possible.</returns>
        /// <exception cref="InvalidOperationException">Thrown for impossible cases, e.g. when keyDescriptor is a message.</exception>
        private object MapKeyToDescriptor(string key, FieldDescriptor keyDescriptor)
        {
            switch (keyDescriptor.FieldType)
            {
                case FieldType.Bool:
                    {
                        if (bool.TryParse(key, out var result))
                        {
                            return result;
                        }
                        return default;
                    }
                case FieldType.String:
                    return key;
                case FieldType.UInt64:
                case FieldType.Fixed64:
                    {
                        if (ulong.TryParse(key, out var result))
                        {
                            return result;
                        }
                        return default;
                    }
                case FieldType.Fixed32:
                case FieldType.UInt32:
                    {
                        if (uint.TryParse(key, out var result))
                        {
                            return result;
                        }
                        return default;
                    }
                case FieldType.SFixed64:
                case FieldType.SInt64:
                case FieldType.Int64:
                    {
                        if (long.TryParse(key, out var result))
                        {
                            return result;
                        }
                        return default;
                    }
                case FieldType.SInt32:
                case FieldType.SFixed32:
                case FieldType.Int32:
                    {
                        if (int.TryParse(key, out var result))
                        {
                            return result;
                        }
                        return default;
                    }

                case FieldType.Enum:
                case FieldType.Group:
                case FieldType.Message:
                case FieldType.Bytes:
                case FieldType.Double:
                case FieldType.Float:
                default:
                    {
                        // These cases are not valid keys anyway, and would throw problems way before reaching here.
                        throw new InvalidOperationException("Impossible cases for map keys.");
                    }
            }
        }

        /// <summary>
        /// Merges all keys specified by a sub-tree from <paramref name="sourceMap"/> to <paramref name="destinationMap"/>.
        ///
        /// This method is only used for merging map fields.
        /// </summary>
        private void Merge(
            Node node,
            string path,
            // mapField.IsMap is always true
            FieldDescriptor mapField,
            IDictionary sourceMap,
            IDictionary destinationMap,
            FieldMask.MergeOptions options)
        {
            var entryMessage = mapField.MessageType;
            var keyDescriptor = entryMessage.FindFieldByName("key");
            var valueDescriptor = entryMessage.FindFieldByName("value");

            foreach (var entry in node.Children)
            {
                var originalKey = entry.Key;
                var key = MapKeyToDescriptor(originalKey, keyDescriptor);
                if (key == default)
                {
                    Debug.WriteLine($"Key {originalKey} is not valid for Field \"{valueDescriptor.FullName}\", which has key type: {keyDescriptor.FieldType}");
                    continue;
                }
                // "sourceField" and "destinationField" are reflected via "valueDescriptor
                var sourceField = sourceMap.Contains(key) ? sourceMap[key] : null;
                var destinationField = destinationMap.Contains(key) ? destinationMap[key] : null;
                if (entry.Value.Children.Count != 0)
                {
                    // here we check if the path is valid, by checking if the entry represents a message.
                    // so a map<string, string> should never have a child node,
                    // but a map<string, Message> can have a child.
                    if (valueDescriptor.FieldType != FieldType.Message)
                    {
                        Debug.WriteLine($"Field \"{valueDescriptor.FullName}\" is not a message field and cannot have sub-fields.");
                        continue;
                    }

                    if (sourceField == null
                        && destinationField == null)
                    {
                        // If the message field is not present in both source and destination, skip recursing
                        // so we don't create unnecessary empty messages.
                        continue;
                    }

                    if (destinationField == null)
                    {
                        // If we have to merge but the destination does not contain the field, create it.
                        destinationField = valueDescriptor.MessageType.Parser.CreateTemplate();
                        destinationMap[key] = destinationField;
                    }

                    if (sourceField == null)
                    {
                        // If the message field is not present in the source but is in the destination, create an empty one
                        // so we can properly handle child entries
                        sourceField = valueDescriptor.MessageType.Parser.CreateTemplate();
                    }

                    var childPath = path.Length == 0 ? entry.Key : path + "." + entry.Key;
                    Merge(entry.Value, childPath, (IMessage) sourceField, (IMessage) destinationField, options);
                    continue;
                }

                if (valueDescriptor.FieldType == FieldType.Message && !valueDescriptor.MessageType.IsWrapperType)
                {
                    var sourceByteString = sourceField == null ? null : ((IMessage) sourceField).ToByteString();
                    if (options.ReplaceMessageFields)
                    {
                        if (sourceByteString == null)
                        {
                            destinationMap.Remove(key);
                        }
                        else
                        {
                            destinationMap[key] = valueDescriptor.MessageType.Parser.ParseFrom(sourceByteString);
                        }
                    }
                    else
                    {
                        if (sourceByteString != null)
                        {
                            if (destinationField != null)
                            {
                                ((IMessage) destinationField).MergeFrom(sourceByteString);
                            }
                            else
                            {
                                destinationMap[key] = valueDescriptor.MessageType.Parser.ParseFrom(sourceByteString);
                            }
                        }
                    }
                }
                else
                {
                    if (sourceField != null)
                    {
                        destinationMap[key] = sourceField;
                    }
                    else
                    {
                        // Since map fields can't have null values, we don't respect the ReplacePrimitiveFields option here.
                        destinationMap.Remove(key);
                    }
                }
            }
        }

        /// <summary>
        /// Merges all fields specified by a sub-tree from <paramref name="source"/> to <paramref name="destination"/>.
        /// </summary>
        private void Merge(
            Node node,
            string path,
            IMessage source,
            IMessage destination,
            FieldMask.MergeOptions options)
        {
            if (source.Descriptor != destination.Descriptor)
            {
                throw new InvalidProtocolBufferException($"source ({source.Descriptor}) and destination ({destination.Descriptor}) descriptor must be equal");
            }

            var descriptor = source.Descriptor;
            foreach (var entry in node.Children)
            {
                var field = descriptor.FindFieldByName(entry.Key);
                if (field == null)
                {
                    Debug.WriteLine($"Cannot find field \"{entry.Key}\" in message type \"{descriptor.FullName}\"");
                    continue;
                }

                if (entry.Value.Children.Count != 0)
                {
                    if (field.IsRepeated
                        || field.FieldType != FieldType.Message)
                    {
                        if (field.IsMap)
                        {
                            // Use the Merge overload that accepts map fields.
                            var sourceMap = (IDictionary) field.Accessor.GetValue(source);
                            var destinationMap = (IDictionary) field.Accessor.GetValue(destination);

                            var childPathForMap = path.Length == 0 ? entry.Key : path + "." + entry.Key;
                            Merge(entry.Value, childPathForMap, field, sourceMap, destinationMap, options);
                            continue;
                        }

                        Debug.WriteLine($"Field \"{field.FullName}\" is not a singular message field nor a map field and cannot have sub-fields.");
                        continue;
                    }

                    var sourceField = field.Accessor.GetValue(source);
                    var destinationField = field.Accessor.GetValue(destination);
                    if (sourceField == null
                        && destinationField == null)
                    {
                        // If the message field is not present in both source and destination, skip recursing
                        // so we don't create unnecessary empty messages.
                        continue;
                    }

                    if (destinationField == null)
                    {
                        // If we have to merge but the destination does not contain the field, create it.
                        destinationField = field.MessageType.Parser.CreateTemplate();
                        field.Accessor.SetValue(destination, destinationField);
                    }

                    if (sourceField == null)
                    {
                        // If the message field is not present in the source but is in the destination, create an empty one
                        // so we can properly handle child entries
                        sourceField = field.MessageType.Parser.CreateTemplate();
                    }

                    var childPath = path.Length == 0 ? entry.Key : path + "." + entry.Key;
                    Merge(entry.Value, childPath, (IMessage)sourceField, (IMessage)destinationField, options);
                    continue;
                }

                if (field.IsRepeated && !field.IsMap)
                {
                    if (options.ReplaceRepeatedFields)
                    {
                        field.Accessor.Clear(destination);
                    }

                    var sourceField = (IList)field.Accessor.GetValue(source);
                    var destinationField = (IList)field.Accessor.GetValue(destination);
                    foreach (var element in sourceField)
                    {
                        destinationField.Add(element);
                    }
                }
                else if (field.IsMap)
                {
                    var sourceMap = (IDictionary) field.Accessor.GetValue(source);
                    var destinationMap = (IDictionary) field.Accessor.GetValue(destination);
                    // fields always have an implicit Entry message.
                    var entryMessage = field.MessageType;
                    var valueDescriptor = entryMessage.FindFieldByName("value");
                    var sourceEnumerator = sourceMap.GetEnumerator();
                    while (sourceEnumerator.MoveNext())
                    {
                        var sourceEntry = sourceEnumerator.Entry;
                        var key = sourceEntry.Key;
                        // sourceValue is never null.
                        var sourceValue = sourceMap[key];
                        if (valueDescriptor.FieldType == FieldType.Message)
                        {
                            if (valueDescriptor.MessageType.IsWrapperType)
                            {
                                // Well-known wrapper types are represented as nullable primitive types, so we do not "merge" them.
                                // Instead, any non-null value just overwrites the previous value directly.
                                destinationMap[key] = sourceValue;
                            }
                            else
                            {
                                var sourceByteString = ((IMessage) sourceValue).ToByteString();
                                if (destinationMap.Contains(key))
                                {
                                    if (options.ReplaceMessageFields)
                                    {
                                        destinationMap[key] = valueDescriptor.MessageType.Parser.ParseFrom(sourceByteString);
                                    }
                                    else
                                    {
                                        ((IMessage) destinationMap[key]).MergeFrom(sourceByteString);
                                    }
                                }
                                else
                                {
                                    // if the destination map does not contain the key, just add it.
                                    destinationMap[key] = valueDescriptor.MessageType.Parser.ParseFrom(sourceByteString);
                                }
                            }
                        }
                        else
                        {
                            // the destination is NOT a message, so it's always a primitive type.
                            destinationMap[key] = sourceValue;
                        }
                    }
                    // some enumerators implement IDisposable, for example: Dictionary<,>.Enumerator
                    if (sourceEnumerator is IDisposable sourceEnumeratorDisposable)
                    {
                        sourceEnumeratorDisposable.Dispose();
                    }
                }
                else
                {
                    var sourceField = field.Accessor.GetValue(source);
                    if (field.FieldType == FieldType.Message)
                    {
                        if (options.ReplaceMessageFields)
                        {
                            if (sourceField == null)
                            {
                                field.Accessor.Clear(destination);
                            }
                            else
                            {
                                field.Accessor.SetValue(destination, sourceField);
                            }
                        }
                        else
                        {
                            if (sourceField != null)
                            {
                                // Well-known wrapper types are represented as nullable primitive types, so we do not "merge" them.
                                // Instead, any non-null value just overwrites the previous value directly.
                                if (field.MessageType.IsWrapperType)
                                {
                                    field.Accessor.SetValue(destination, sourceField);
                                }
                                else
                                {
                                    var sourceByteString = ((IMessage)sourceField).ToByteString();
                                    var destinationValue = (IMessage)field.Accessor.GetValue(destination);
                                    if (destinationValue != null)
                                    {
                                        destinationValue.MergeFrom(sourceByteString);
                                    }
                                    else
                                    {
                                        field.Accessor.SetValue(destination, field.MessageType.Parser.ParseFrom(sourceByteString));
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        if (sourceField != null
                            || !options.ReplacePrimitiveFields)
                        {
                            field.Accessor.SetValue(destination, sourceField);
                        }
                        else
                        {
                            field.Accessor.Clear(destination);
                        }
                    }
                }
            }
        }
    }
}
