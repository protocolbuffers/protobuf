#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

using NUnit.Framework;
using System.Diagnostics;
using System;
using System.Reflection;
using System.IO;

namespace Google.Protobuf
{
    public class RefStructCompatibilityTest
    {
        /// <summary>
        /// Checks that the generated code can be compiler with an old C# compiler.
        /// The reason why this test is needed is that even though dotnet SDK projects allow to set LangVersion,
        /// this setting doesn't accurately simulate compilation with an actual old pre-roslyn C# compiler.
        /// For instance, the "ref struct" types are only supported by C# 7.2 and higher, but even if
        /// LangVersion is set low, the roslyn compiler still understands the concept of ref struct
        /// and silently accepts them. Therefore we try to build the generated code with an actual old C# compiler
        /// to be able to catch these sort of compatibility problems.
        /// </summary>
        [Test]
        public void GeneratedCodeCompilesWithOldCsharpCompiler()
        {
            if (Environment.OSVersion.Platform != PlatformID.Win32NT)
            {
                // This tests needs old C# compiler which is only available on Windows. Skipping it on all other platforms.
                return;
            }

            var currentAssemblyDir = Path.GetDirectoryName(typeof(RefStructCompatibilityTest).GetTypeInfo().Assembly.Location);
            var testProtosProjectDir = Path.GetFullPath(Path.Combine(currentAssemblyDir, "..", "..", "..", "..", "Google.Protobuf.Test.TestProtos"));
            var testProtosOutputDir = (currentAssemblyDir.Contains("bin/Debug/") || currentAssemblyDir.Contains("bin\\Debug\\")) ? "bin\\Debug\\net462" : "bin\\Release\\net462";

            // If "ref struct" types are used in the generated code, compilation with an old compiler will fail with the following error:
            // "XYZ is obsolete: 'Types with embedded references are not supported in this version of your compiler.'"
            // We build the code with GOOGLE_PROTOBUF_REFSTRUCT_COMPATIBILITY_MODE to avoid the use of ref struct in the generated code.
            var compatibilityFlag = "-define:GOOGLE_PROTOBUF_REFSTRUCT_COMPATIBILITY_MODE";
            var sources = "*.cs";  // the generated sources from the TestProtos project
            var args = $"-langversion:3 -target:library {compatibilityFlag} -reference:{testProtosOutputDir}\\Google.Protobuf.dll -out:{testProtosOutputDir}\\TestProtos.RefStructCompatibilityTest.OldCompiler.dll {sources}";
            RunOldCsharpCompilerAndCheckSuccess(args, testProtosProjectDir);
        }

        /// <summary>
        /// Invoke an old C# compiler in a subprocess and check it finished successful.
        /// </summary>
        /// <param name="args"></param>
        /// <param name="workingDirectory"></param>
        private void RunOldCsharpCompilerAndCheckSuccess(string args, string workingDirectory)
        {  
            using (var process = new Process())
            {
                // Get the path to the old C# 5 compiler from .NET framework. This approach is not 100% reliable, but works on most machines.
                // Alternative way of getting the framework path is System.Runtime.InteropServices.RuntimeEnvironment.GetRuntimeDirectory()
                // but it only works with the net45 target.
                var oldCsharpCompilerPath = Path.Combine(Environment.GetEnvironmentVariable("WINDIR"), "Microsoft.NET", "Framework", "v4.0.30319", "csc.exe");
                process.StartInfo.FileName = oldCsharpCompilerPath;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.RedirectStandardError = true;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.Arguments = args;
                process.StartInfo.WorkingDirectory = workingDirectory;

                process.OutputDataReceived += (sender, e) =>
                {
                    if (e.Data != null)
                    {
                        Console.WriteLine(e.Data);
                    }
                };
                process.ErrorDataReceived += (sender, e) =>
                {
                    if (e.Data != null)
                    {
                        Console.WriteLine(e.Data);
                    }
                };

                process.Start();

                process.BeginErrorReadLine();
                process.BeginOutputReadLine();

                process.WaitForExit();
                Assert.AreEqual(0, process.ExitCode);
            }
        }
    }
}