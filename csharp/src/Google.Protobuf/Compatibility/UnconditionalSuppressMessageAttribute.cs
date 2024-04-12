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

#if !NET5_0_OR_GREATER
// Copied with permission from https://github.com/dotnet/runtime/tree/8fbf206d0e518b45ca855832e8bfb391afa85972/src/libraries/System.Private.CoreLib/src/System/Diagnostics/CodeAnalysis
namespace System.Diagnostics.CodeAnalysis
{
    /// <summary>
    /// Suppresses reporting of a specific rule violation, allowing multiple suppressions on a
    /// single code artifact.
    /// </summary>
    /// <remarks>
    /// <see cref="UnconditionalSuppressMessageAttribute"/> is different than
    /// <see cref="SuppressMessageAttribute"/> in that it doesn't have a
    /// <see cref="ConditionalAttribute"/>. So it is always preserved in the compiled assembly.
    /// </remarks>
    [AttributeUsage(AttributeTargets.All, Inherited = false, AllowMultiple = true)]
    internal sealed class UnconditionalSuppressMessageAttribute : Attribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UnconditionalSuppressMessageAttribute"/>
        /// class, specifying the category of the tool and the identifier for an analysis rule.
        /// </summary>
        /// <param name="category">The category for the attribute.</param>
        /// <param name="checkId">The identifier of the analysis rule the attribute applies to.</param>
        public UnconditionalSuppressMessageAttribute(string category, string checkId)
        {
            Category = category;
            CheckId = checkId;
        }

        /// <summary>
        /// Gets the category identifying the classification of the attribute.
        /// </summary>
        /// <remarks>
        /// The <see cref="Category"/> property describes the tool or tool analysis category
        /// for which a message suppression attribute applies.
        /// </remarks>
        public string Category { get; }

        /// <summary>
        /// Gets the identifier of the analysis tool rule to be suppressed.
        /// </summary>
        /// <remarks>
        /// Concatenated together, the <see cref="Category"/> and <see cref="CheckId"/>
        /// properties form a unique check identifier.
        /// </remarks>
        public string CheckId { get; }

        /// <summary>
        /// Gets or sets the scope of the code that is relevant for the attribute.
        /// </summary>
        /// <remarks>
        /// The Scope property is an optional argument that specifies the metadata scope for which
        /// the attribute is relevant.
        /// </remarks>
        public string Scope { get; set; }

        /// <summary>
        /// Gets or sets a fully qualified path that represents the target of the attribute.
        /// </summary>
        /// <remarks>
        /// The <see cref="Target"/> property is an optional argument identifying the analysis target
        /// of the attribute. An example value is "System.IO.Stream.ctor():System.Void".
        /// Because it is fully qualified, it can be long, particularly for targets such as parameters.
        /// The analysis tool user interface should be capable of automatically formatting the parameter.
        /// </remarks>
        public string Target { get; set; }

        /// <summary>
        /// Gets or sets an optional argument expanding on exclusion criteria.
        /// </summary>
        /// <remarks>
        /// The <see cref="MessageId "/> property is an optional argument that specifies additional
        /// exclusion where the literal metadata target is not sufficiently precise. For example,
        /// the <see cref="UnconditionalSuppressMessageAttribute"/> cannot be applied within a method,
        /// and it may be desirable to suppress a violation against a statement in the method that will
        /// give a rule violation, but not against all statements in the method.
        /// </remarks>
        public string MessageId { get; set; }

        /// <summary>
        /// Gets or sets the justification for suppressing the code analysis message.
        /// </summary>
        public string Justification { get; set; }
    }
}
#endif
