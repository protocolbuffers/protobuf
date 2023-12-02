#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// A collection to simplify retrieving the descriptors of extensions in a descriptor for a message
    /// </summary>
    [DebuggerDisplay("Count = {UnorderedExtensions.Count}")]
    [DebuggerTypeProxy(typeof(ExtensionCollectionDebugView))]
    public sealed class ExtensionCollection
    {
        private IDictionary<MessageDescriptor, IList<FieldDescriptor>> extensionsByTypeInDeclarationOrder;
        private IDictionary<MessageDescriptor, IList<FieldDescriptor>> extensionsByTypeInNumberOrder;

        internal ExtensionCollection(FileDescriptor file, Extension[] extensions)
        {
            UnorderedExtensions = DescriptorUtil.ConvertAndMakeReadOnly(
                file.Proto.Extension,
                (extension, i) => {
                    if (extensions?.Length != 0)
                    {
                        return new FieldDescriptor(extension, file, null, i, null, extensions?[i]);
                    }
                    else
                    {
                        return new FieldDescriptor(extension, file, null, i, null, null); // return null if there's no extensions in this array for old code-gen
                    }
                });
        }

        internal ExtensionCollection(MessageDescriptor message, Extension[] extensions)
        {
            UnorderedExtensions = DescriptorUtil.ConvertAndMakeReadOnly(
                message.Proto.Extension,
                (extension, i) => {
                    if (extensions?.Length != 0)
                    {
                        return new FieldDescriptor(extension, message.File, message, i, null, extensions?[i]);
                    }
                    else
                    {
                        return new FieldDescriptor(extension, message.File, message, i, null, null);
                    }
                });
        }

        /// <summary>
        /// Returns a readonly list of all the extensions defined in this type in 
        /// the order they were defined in the source .proto file
        /// </summary>
        public IList<FieldDescriptor> UnorderedExtensions { get; }

        /// <summary>
        /// Returns a readonly list of all the extensions define in this type that extend 
        /// the provided descriptor type in the order they were defined in the source .proto file
        /// </summary>
        public IList<FieldDescriptor> GetExtensionsInDeclarationOrder(MessageDescriptor descriptor)
        {
            return extensionsByTypeInDeclarationOrder[descriptor];
        }

        /// <summary>
        /// Returns a readonly list of all the extensions define in this type that extend 
        /// the provided descriptor type in ascending field order
        /// </summary>
        public IList<FieldDescriptor> GetExtensionsInNumberOrder(MessageDescriptor descriptor)
        {
            return extensionsByTypeInNumberOrder[descriptor];
        }

        internal void CrossLink()
        {
            Dictionary<MessageDescriptor, IList<FieldDescriptor>> declarationOrder = new Dictionary<MessageDescriptor, IList<FieldDescriptor>>();
            foreach (FieldDescriptor descriptor in UnorderedExtensions)
            {
                descriptor.CrossLink();

                if (!declarationOrder.TryGetValue(descriptor.ExtendeeType, out IList<FieldDescriptor> list))
                {
                    list = new List<FieldDescriptor>();
                    declarationOrder.Add(descriptor.ExtendeeType, list);
                }

                list.Add(descriptor);
            }

            extensionsByTypeInDeclarationOrder = declarationOrder
                .ToDictionary(kvp => kvp.Key, kvp => (IList<FieldDescriptor>)new ReadOnlyCollection<FieldDescriptor>(kvp.Value));
            extensionsByTypeInNumberOrder = declarationOrder
                .ToDictionary(kvp => kvp.Key, kvp => (IList<FieldDescriptor>)new ReadOnlyCollection<FieldDescriptor>(kvp.Value.OrderBy(field => field.FieldNumber).ToArray()));
        }

        private sealed class ExtensionCollectionDebugView
        {
            private readonly ExtensionCollection list;

            public ExtensionCollectionDebugView(ExtensionCollection list)
            {
                this.list = list;
            }

            [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
            public FieldDescriptor[] Items => list.UnorderedExtensions.ToArray();
        }
    }
}
