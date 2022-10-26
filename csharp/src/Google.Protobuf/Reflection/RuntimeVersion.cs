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
using System.Reflection;

namespace Google.Protobuf.Reflection;

/// <summary>
/// Utility methods regarding the version of the Protobuf runtime
/// (i.e. the code in Google.Protobuf and related namespaces).
/// </summary>
public static class RuntimeVersion
{
    private static readonly Version current = GetCurrentVersion();

    private static Version GetCurrentVersion()
    {
        // To avoid confusion, we trim the 4-value version number to a 3-value version number.
        var fullVersion = typeof(RuntimeVersion).GetTypeInfo().Assembly.GetName().Version;
        return new Version(fullVersion.Major, fullVersion.Minor, fullVersion.Build);
    }

    /// <summary>
    /// Returns the current version of the runtime, i.e. the version of the assembly
    /// containing this class.
    /// </summary>
    public static Version Current => current;

    /// <summary>
    /// Validates that the given runtime version corresponding to the version
    /// of protoc used to generate the calling code is valid for this runtime.
    /// </summary>
    /// <remarks>
    /// This method is usually called automatically by generated code, as part of
    /// message type or file descriptor initialization.
    /// </remarks>
    /// <param name="runtimeVersionInGeneratedCode">The runtime version to check for compatibility.</param>
    public static void Validate(Version runtimeVersionInGeneratedCode) =>
        Validate(current, runtimeVersionInGeneratedCode);

    // Internal method which doesn't depend on the *actual* current version, for testing purposes.
    internal static void Validate(Version currentVersion, Version targetVersion)
    {
        if (currentVersion.Major != targetVersion.Major)
        {
            throw new InvalidOperationException(GetErrorMessage("major"));
        }
        if (currentVersion.Minor < targetVersion.Minor)
        {
            throw new InvalidOperationException(GetErrorMessage("minor"));
        }
        if (currentVersion.Minor == targetVersion.Minor &&
            currentVersion.Build < targetVersion.Build)
        {
            throw new InvalidOperationException(GetErrorMessage("patch"));
        }

        string GetErrorMessage(string differenceType) =>
            $"The runtime version expected by the generated code {targetVersion} is not compatible " +
            $"with the current runtime version {currentVersion}. The major versions much match exactly, the current minor version must " +
            $"be greater than or equal to the expected minor version, and if the minor versions are the same, the " +
            $"current patch version must be greater than or equal to the expected patch version. Incompatibility type: {differenceType}.";
    }
}
