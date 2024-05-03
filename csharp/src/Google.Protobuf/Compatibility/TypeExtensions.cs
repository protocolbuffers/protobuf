#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;

namespace Google.Protobuf.Compatibility
{
    /// <summary>
    /// Provides extension methods on Type that just proxy to TypeInfo.
    /// These are used to support the new type system from .NET 4.5, without
    /// having calls to GetTypeInfo all over the place. While the methods here are meant to be
    /// broadly compatible with the desktop framework, there are some subtle differences in behaviour - but
    /// they're not expected to affect our use cases. While the class is internal, that should be fine: we can
    /// evaluate each new use appropriately.
    /// </summary>
    internal static class TypeExtensions
    {
        /// <summary>
        /// See https://msdn.microsoft.com/en-us/library/system.type.isassignablefrom
        /// </summary>
        internal static bool IsAssignableFrom(this Type target, Type c)
        {
            return target.GetTypeInfo().IsAssignableFrom(c.GetTypeInfo());
        }

        /// <summary>
        /// Returns a representation of the public property associated with the given name in the given type,
        /// including inherited properties or null if there is no such public property.
        /// Here, "public property" means a property where either the getter, or the setter, or both, is public.
        /// </summary>
        [UnconditionalSuppressMessage("Trimming", "IL2072",
            Justification = "The BaseType of the target will have all properties because of the annotation.")]
        internal static PropertyInfo GetProperty(
            [DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicProperties | DynamicallyAccessedMemberTypes.NonPublicProperties)]
            this Type target, string name)
        {
            // GetDeclaredProperty only returns properties declared in the given type, so we need to recurse.
            while (target != null)
            {
                var typeInfo = target.GetTypeInfo();
                var ret = typeInfo.GetDeclaredProperty(name);
                if (ret != null && ((ret.CanRead && ret.GetMethod.IsPublic) || (ret.CanWrite && ret.SetMethod.IsPublic)))
                {
                    return ret;
                }
                target = typeInfo.BaseType;
            }
            return null;
        }

        /// <summary>
        /// Returns a representation of the public method associated with the given name in the given type,
        /// including inherited methods.
        /// </summary>
        /// <remarks>
        /// This has a few differences compared with Type.GetMethod in the desktop framework. It will throw
        /// if there is an ambiguous match even between a private method and a public one, but it *won't* throw
        /// if there are two overloads at different levels in the type hierarchy (e.g. class Base declares public void Foo(int) and
        /// class Child : Base declares public void Foo(long)).
        /// </remarks>
        /// <exception cref="AmbiguousMatchException">One type in the hierarchy declared more than one method with the same name</exception>
        [UnconditionalSuppressMessage("Trimming", "IL2072",
            Justification = "The BaseType of the target will have all properties because of the annotation.")]
        internal static MethodInfo GetMethod(
            [DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicMethods | DynamicallyAccessedMemberTypes.NonPublicMethods)]
            this Type target, string name)
        {
            // GetDeclaredMethod only returns methods declared in the given type, so we need to recurse.
            while (target != null)
            {
                var typeInfo = target.GetTypeInfo();
                var ret = typeInfo.GetDeclaredMethod(name);
                if (ret != null && ret.IsPublic)
                {
                    return ret;
                }
                target = typeInfo.BaseType;
            }
            return null;
        }
    }
}
