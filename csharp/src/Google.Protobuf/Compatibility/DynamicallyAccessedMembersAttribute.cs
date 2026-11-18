#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

#if !NET5_0_OR_GREATER
// Copied with permission from https://github.com/dotnet/runtime/tree/8fbf206d0e518b45ca855832e8bfb391afa85972/src/libraries/System.Private.CoreLib/src/System/Diagnostics/CodeAnalysis
namespace System.Diagnostics.CodeAnalysis
{
    /// <summary>
    /// Indicates that certain members on a specified <see cref="Type"/> are accessed dynamically,
    /// for example through <see cref="System.Reflection"/>.
    /// </summary>
    /// <remarks>
    /// This allows tools to understand which members are being accessed during the execution
    /// of a program.
    ///
    /// This attribute is valid on members whose type is <see cref="Type"/> or <see cref="string"/>.
    ///
    /// When this attribute is applied to a location of type <see cref="string"/>, the assumption is
    /// that the string represents a fully qualified type name.
    ///
    /// When this attribute is applied to a class, interface, or struct, the members specified
    /// can be accessed dynamically on <see cref="Type"/> instances returned from calling
    /// <see cref="object.GetType"/> on instances of that class, interface, or struct.
    ///
    /// If the attribute is applied to a method it's treated as a special case and it implies
    /// the attribute should be applied to the "this" parameter of the method. As such the attribute
    /// should only be used on instance methods of types assignable to System.Type (or string, but no methods
    /// will use it there).
    /// </remarks>
    [AttributeUsage(
        AttributeTargets.Field | AttributeTargets.ReturnValue | AttributeTargets.GenericParameter |
        AttributeTargets.Parameter | AttributeTargets.Property | AttributeTargets.Method |
        AttributeTargets.Class | AttributeTargets.Interface | AttributeTargets.Struct,
        Inherited = false)]
    internal sealed class DynamicallyAccessedMembersAttribute : Attribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DynamicallyAccessedMembersAttribute"/> class
        /// with the specified member types.
        /// </summary>
        /// <param name="memberTypes">The types of members dynamically accessed.</param>
        public DynamicallyAccessedMembersAttribute(DynamicallyAccessedMemberTypes memberTypes)
        {
            MemberTypes = memberTypes;
        }

        /// <summary>
        /// Gets the <see cref="DynamicallyAccessedMemberTypes"/> which specifies the type
        /// of members dynamically accessed.
        /// </summary>
        public DynamicallyAccessedMemberTypes MemberTypes { get; }
    }
}
#endif
