#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections.Generic;
using System.Text;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Contains lookup tables containing all the descriptors defined in a particular file.
    /// </summary>
    internal sealed class DescriptorPool
    {
        private readonly IDictionary<string, IDescriptor> descriptorsByName =
            new Dictionary<string, IDescriptor>();

        private readonly IDictionary<ObjectIntPair<IDescriptor>, FieldDescriptor> fieldsByNumber =
            new Dictionary<ObjectIntPair<IDescriptor>, FieldDescriptor>();

        private readonly IDictionary<ObjectIntPair<IDescriptor>, EnumValueDescriptor> enumValuesByNumber =
            new Dictionary<ObjectIntPair<IDescriptor>, EnumValueDescriptor>();

        private readonly IDictionary<EnumValueByNameDescriptorKey, EnumValueDescriptor> enumValuesByName =
            new Dictionary<EnumValueByNameDescriptorKey, EnumValueDescriptor>();

        private readonly HashSet<FileDescriptor> dependencies = new HashSet<FileDescriptor>();

        internal DescriptorPool(IEnumerable<FileDescriptor> dependencyFiles)
        {
            foreach (FileDescriptor dependencyFile in dependencyFiles)
            {
                dependencies.Add(dependencyFile);
                ImportPublicDependencies(dependencyFile);
            }

            foreach (FileDescriptor dependency in dependencyFiles)
            {
                AddPackage(dependency.Package, dependency);
            }
        }

        private void ImportPublicDependencies(FileDescriptor file)
        {
            foreach (FileDescriptor dependency in file.PublicDependencies)
            {
                if (dependencies.Add(dependency))
                {
                    ImportPublicDependencies(dependency);
                }
            }
        }

        /// <summary>
        /// Finds a symbol of the given name within the pool.
        /// </summary>
        /// <typeparam name="T">The type of symbol to look for</typeparam>
        /// <param name="fullName">Fully-qualified name to look up</param>
        /// <returns>The symbol with the given name and type,
        /// or null if the symbol doesn't exist or has the wrong type</returns>
        internal T FindSymbol<T>(string fullName) where T : class
        {
            descriptorsByName.TryGetValue(fullName, out IDescriptor result);
            if (result is T descriptor)
            {
                return descriptor;
            }

            // dependencies contains direct dependencies and any *public* dependencies
            // of those dependencies (transitively)... so we don't need to recurse here.
            foreach (FileDescriptor dependency in dependencies)
            {
                dependency.DescriptorPool.descriptorsByName.TryGetValue(fullName, out result);
                descriptor = result as T;
                if (descriptor != null)
                {
                    return descriptor;
                }
            }

            return null;
        }

        /// <summary>
        /// Adds a package to the symbol tables. If a package by the same name
        /// already exists, that is fine, but if some other kind of symbol
        /// exists under the same name, an exception is thrown. If the package
        /// has multiple components, this also adds the parent package(s).
        /// </summary>
        internal void AddPackage(string fullName, FileDescriptor file)
        {
            int dotpos = fullName.LastIndexOf('.');
            String name;
            if (dotpos != -1)
            {
                AddPackage(fullName.Substring(0, dotpos), file);
                name = fullName.Substring(dotpos + 1);
            }
            else
            {
                name = fullName;
            }

            if (descriptorsByName.TryGetValue(fullName, out IDescriptor old))
            {
                if (old is not PackageDescriptor)
                {
                    throw new DescriptorValidationException(file,
                                                            "\"" + name +
                                                            "\" is already defined (as something other than a " +
                                                            "package) in file \"" + old.File.Name + "\".");
                }
            }
            descriptorsByName[fullName] = new PackageDescriptor(name, fullName, file);
        }

        /// <summary>
        /// Adds a symbol to the symbol table.
        /// </summary>
        /// <exception cref="DescriptorValidationException">The symbol already existed
        /// in the symbol table.</exception>
        internal void AddSymbol(IDescriptor descriptor)
        {
            ValidateSymbolName(descriptor);
            string fullName = descriptor.FullName;

            if (descriptorsByName.TryGetValue(fullName, out IDescriptor old))
            {
                throw new DescriptorValidationException(descriptor,
                    GetDescriptorAlreadyAddedExceptionMessage(descriptor, fullName, old));
            }
            descriptorsByName[fullName] = descriptor;
        }

        private static string GetDescriptorAlreadyAddedExceptionMessage(IDescriptor descriptor, string fullName, IDescriptor old)
        {
            int dotPos = fullName.LastIndexOf('.');
            return descriptor.File != old.File ? $"\"{fullName}\" is already defined in file \"{old.File.Name}\"."
                : dotPos == -1 ? $"{fullName} is already defined."
                : $"\"{fullName.Substring(dotPos + 1)}\" is already defined in \"{fullName.Substring(0, dotPos)}\".";
        }

        /// <summary>
        /// Verifies that the descriptor's name is valid (i.e. it contains
        /// only letters, digits and underscores, and does not start with a digit).
        /// </summary>
        /// <param name="descriptor"></param>
        private static void ValidateSymbolName(IDescriptor descriptor)
        {
            if (descriptor.Name.Length == 0)
            {
                throw new DescriptorValidationException(descriptor, "Missing name.");
            }

            // Symbol name must start with a letter or underscore, and it can contain letters,
            // numbers and underscores.
            string name = descriptor.Name;
            if (!IsAsciiLetter(name[0]) && name[0] != '_')
            {
                ThrowInvalidSymbolNameException(descriptor);
            }
            for (int i = 1; i < name.Length; i++)
            {
                if (!IsAsciiLetter(name[i]) && !IsAsciiDigit(name[i]) && name[i] != '_')
                {
                    ThrowInvalidSymbolNameException(descriptor);
                }
            }

            static bool IsAsciiLetter(char c) => (uint)((c | 0x20) - 'a') <= 'z' - 'a';
            static bool IsAsciiDigit(char c) => (uint)(c - '0') <= '9' - '0';
            static void ThrowInvalidSymbolNameException(IDescriptor descriptor) =>
                throw new DescriptorValidationException(
                    descriptor, "\"" + descriptor.Name + "\" is not a valid identifier.");
        }

        /// <summary>
        /// Returns the field with the given number in the given descriptor,
        /// or null if it can't be found.
        /// </summary>
        internal FieldDescriptor FindFieldByNumber(MessageDescriptor messageDescriptor, int number)
        {
            fieldsByNumber.TryGetValue(new ObjectIntPair<IDescriptor>(messageDescriptor, number), out FieldDescriptor ret);
            return ret;
        }

        internal EnumValueDescriptor FindEnumValueByNumber(EnumDescriptor enumDescriptor, int number)
        {
            enumValuesByNumber.TryGetValue(new ObjectIntPair<IDescriptor>(enumDescriptor, number), out EnumValueDescriptor ret);
            return ret;
        }

        internal EnumValueDescriptor FindEnumValueByName(EnumDescriptor enumDescriptor, string name)
        {
            enumValuesByName.TryGetValue(new EnumValueByNameDescriptorKey(enumDescriptor, name), out EnumValueDescriptor ret);
            return ret;
        }

        /// <summary>
        /// Adds a field to the fieldsByNumber table.
        /// </summary>
        /// <exception cref="DescriptorValidationException">A field with the same
        /// containing type and number already exists.</exception>
        internal void AddFieldByNumber(FieldDescriptor field)
        {
            // for extensions, we use the extended type, otherwise we use the containing type
            ObjectIntPair<IDescriptor> key = new ObjectIntPair<IDescriptor>(field.Proto.HasExtendee ? field.ExtendeeType : field.ContainingType, field.FieldNumber);
            if (fieldsByNumber.TryGetValue(key, out FieldDescriptor old))
            {
                throw new DescriptorValidationException(field, "Field number " + field.FieldNumber +
                                                               "has already been used in \"" +
                                                               field.ContainingType.FullName +
                                                               "\" by field \"" + old.Name + "\".");
            }
            fieldsByNumber[key] = field;
        }

        /// <summary>
        /// Adds an enum value to the enumValuesByNumber and enumValuesByName tables. If an enum value
        /// with the same type and number already exists, this method does nothing to enumValuesByNumber.
        /// (This is allowed; the first value defined with the number takes precedence.) If an enum
        /// value with the same name already exists, this method throws DescriptorValidationException.
        /// (It is expected that this method is called after AddSymbol, which would already have thrown
        /// an exception in this failure case.)
        /// </summary>
        internal void AddEnumValue(EnumValueDescriptor enumValue)
        {
            ObjectIntPair<IDescriptor> numberKey = new ObjectIntPair<IDescriptor>(enumValue.EnumDescriptor, enumValue.Number);
            if (!enumValuesByNumber.ContainsKey(numberKey))
            {
                enumValuesByNumber[numberKey] = enumValue;
            }

            EnumValueByNameDescriptorKey nameKey = new EnumValueByNameDescriptorKey(enumValue.EnumDescriptor, enumValue.Name);
            if (enumValuesByName.TryGetValue(nameKey, out EnumValueDescriptor old))
            {
                throw new DescriptorValidationException(enumValue,
                GetDescriptorAlreadyAddedExceptionMessage(enumValue, enumValue.FullName, old));
            }
            enumValuesByName[nameKey] = enumValue;
        }

        /// <summary>
        /// Looks up a descriptor by name, relative to some other descriptor.
        /// The name may be fully-qualified (with a leading '.'), partially-qualified,
        /// or unqualified. C++-like name lookup semantics are used to search for the
        /// matching descriptor.
        /// </summary>
        /// <remarks>
        /// This isn't heavily optimized, but it's only used during cross linking anyway.
        /// If it starts being used more widely, we should look at performance more carefully.
        /// </remarks>
        internal IDescriptor LookupSymbol(string name, IDescriptor relativeTo)
        {
            IDescriptor result;
            if (name.StartsWith("."))
            {
                // Fully-qualified name.
                result = FindSymbol<IDescriptor>(name.Substring(1));
            }
            else
            {
                // If "name" is a compound identifier, we want to search for the
                // first component of it, then search within it for the rest.
                int firstPartLength = name.IndexOf('.');
                string firstPart = firstPartLength == -1 ? name : name.Substring(0, firstPartLength);

                // We will search each parent scope of "relativeTo" looking for the
                // symbol.
                StringBuilder scopeToTry = new StringBuilder(relativeTo.FullName);

                while (true)
                {
                    // Chop off the last component of the scope.

                    int dotpos = scopeToTry.ToString().LastIndexOf('.');
                    if (dotpos == -1)
                    {
                        result = FindSymbol<IDescriptor>(name);
                        break;
                    }
                    else
                    {
                        scopeToTry.Length = dotpos + 1;

                        // Append firstPart and try to find.
                        scopeToTry.Append(firstPart);
                        result = FindSymbol<IDescriptor>(scopeToTry.ToString());

                        if (result != null)
                        {
                            if (firstPartLength != -1)
                            {
                                // We only found the first part of the symbol.  Now look for
                                // the whole thing.  If this fails, we *don't* want to keep
                                // searching parent scopes.
                                scopeToTry.Length = dotpos + 1;
                                scopeToTry.Append(name);
                                result = FindSymbol<IDescriptor>(scopeToTry.ToString());
                            }
                            break;
                        }

                        // Not found.  Remove the name so we can try again.
                        scopeToTry.Length = dotpos;
                    }
                }
            }

            if (result == null)
            {
                throw new DescriptorValidationException(relativeTo, "\"" + name + "\" is not defined.");
            }
            else
            {
                return result;
            }
        }

        /// <summary>
        /// Struct used to hold the keys for the enumValuesByName table.
        /// </summary>
        private struct EnumValueByNameDescriptorKey : IEquatable<EnumValueByNameDescriptorKey>
        {
            private readonly string name;
            private readonly IDescriptor descriptor;

            internal EnumValueByNameDescriptorKey(EnumDescriptor descriptor, string valueName)
            {
                this.descriptor = descriptor;
                this.name = valueName;
            }

            public bool Equals(EnumValueByNameDescriptorKey other) =>
                descriptor == other.descriptor
                && name == other.name;

            public override bool Equals(object obj) =>
                obj is EnumValueByNameDescriptorKey pair && Equals(pair);

            public override int GetHashCode()
            {
                unchecked
                {
                    var hashCode = descriptor.GetHashCode();
                    hashCode = (hashCode * 397) ^ (name != null ? name.GetHashCode() : 0);
                    return hashCode;
                }
            }
        }
   }
}