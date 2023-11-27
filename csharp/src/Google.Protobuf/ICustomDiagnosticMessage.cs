#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf
{
    /// <summary>
    /// A message type that has a custom string format for diagnostic purposes.
    /// </summary>
    /// <remarks>
    /// <para>
    /// Calling <see cref="object.ToString"/> on a generated message type normally
    /// returns the JSON representation. If a message type implements this interface,
    /// then the <see cref="ToDiagnosticString"/> method will be called instead of the regular
    /// JSON formatting code, but only when <c>ToString()</c> is called either on the message itself
    /// or on another message which contains it. This does not affect the normal JSON formatting of
    /// the message.
    /// </para>
    /// <para>
    /// For example, if you create a proto message representing a GUID, the internal
    /// representation may be a <c>bytes</c> field or four <c>fixed32</c> fields. However, when debugging
    /// it may be more convenient to see a result in the same format as <see cref="System.Guid"/> provides.
    /// </para>
    /// <para>This interface extends <see cref="IMessage"/> to avoid it accidentally being implemented
    /// on types other than messages, where it would not be used by anything in the framework.</para>
    /// </remarks>
    public interface ICustomDiagnosticMessage : IMessage
    {
        /// <summary>
        /// Returns a string representation of this object, for diagnostic purposes.
        /// </summary>
        /// <remarks>
        /// This method is called when a message is formatted as part of a <see cref="object.ToString"/>
        /// call. It does not affect the JSON representation used by <see cref="JsonFormatter"/> other than
        /// in calls to <see cref="JsonFormatter.ToDiagnosticString(IMessage)"/>. While it is recommended
        /// that the result is valid JSON, this is never assumed by the Protobuf library.
        /// </remarks>
        /// <returns>A string representation of this object, for diagnostic purposes.</returns>
        string ToDiagnosticString();
    }
}
