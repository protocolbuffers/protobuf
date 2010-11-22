#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using NUnit.Framework;

namespace Google.ProtocolBuffers.ProtoGen
{
    [TestFixture]
    [Category("Preprocessor")]
    public partial class TestPreprocessing
    {
        private static readonly string TempPath = Path.Combine(Path.GetTempPath(), "proto-gen-test");
        private const string DefaultProto = @"
package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
}";

        #region TestFixture SetUp/TearDown
        private static readonly string OriginalWorkingDirectory = Environment.CurrentDirectory;
        [TestFixtureSetUp]
        public virtual void Setup()
        {
            Teardown();
            Directory.CreateDirectory(TempPath);
            Environment.CurrentDirectory = TempPath;
        }

        [TestFixtureTearDown]
        public virtual void Teardown()
        {
            Environment.CurrentDirectory = OriginalWorkingDirectory;
            if (Directory.Exists(TempPath))
            {
                Directory.Delete(TempPath, true);
            }
        }
        #endregion
        #region Helper Methods RunProtoGen / RunCsc
        void RunProtoGen(int expect, params string[] args)
        {
            TextWriter tout = Console.Out, terr = Console.Error;
            StringWriter temp = new StringWriter();
            Console.SetOut(temp);
            Console.SetError(temp);
            try
            {
                Assert.AreEqual(expect, ProgramPreprocess.Run(args), "ProtoGen Failed: {0}", temp);
            }
            finally
            {
                Console.SetOut(tout);
                Console.SetError(terr);
            }
        }

        private Assembly RunCsc(int expect, params string[] sources)
        {
            using (TempFile tempDll = new TempFile(String.Empty))
            {
                tempDll.ChangeExtension(".dll");
                List<string> args = new List<string>();
                args.Add("/nologo");
                args.Add("/target:library");
                args.Add("/debug-");
                args.Add(String.Format(@"""/out:{0}""", tempDll.TempPath));
                args.Add("/r:System.dll");
                args.Add(String.Format(@"""/r:{0}""", typeof (Google.ProtocolBuffers.DescriptorProtos.DescriptorProto).Assembly.Location));
                args.AddRange(sources);

                string exe = Path.Combine(System.Runtime.InteropServices.RuntimeEnvironment.GetRuntimeDirectory(), "csc.exe");
                ProcessStartInfo psi = new ProcessStartInfo(exe);
                psi.CreateNoWindow = true;
                psi.UseShellExecute = false;
                psi.RedirectStandardOutput = true;
                psi.RedirectStandardError = true;
                psi.Arguments = string.Join(" ", args.ToArray());
                Process p = Process.Start(psi);
                p.WaitForExit();
                string errorText = p.StandardOutput.ReadToEnd() + p.StandardError.ReadToEnd();
                Assert.AreEqual(expect, p.ExitCode, "CSC.exe Failed: {0}", errorText);

                Assembly asm = null;
                if (p.ExitCode == 0)
                {
                    byte[] allbytes = File.ReadAllBytes(tempDll.TempPath);
                    asm = Assembly.Load(allbytes);

                    foreach (Type t in asm.GetTypes())
                    {
                        Debug.WriteLine(t.FullName, asm.FullName);
                    }
                }
                return asm;
            }
        }
        #endregion

        // *******************************************************************
        // The following tests excercise options for protogen.exe
        // *******************************************************************

        [Test]
        public void TestProtoFile()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(0, proto.TempPath);
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithConflictingType()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", @"
package nunit.simple;
// Test a very simple message.
message " + test + @" {
  optional string name = 1;
} "))
            {
                RunProtoGen(0, proto.TempPath);
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple." + test, true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple.Proto." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithNamespace()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(0, proto.TempPath, "-namespace:MyNewNamespace");
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("MyNewNamespace.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("MyNewNamespace." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithUmbrellaClassName()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach("MyUmbrellaClassname.cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(0, proto.TempPath, "/umbrella_classname=MyUmbrellaClassname");
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple.MyUmbrellaClassname", true, true);
            }
        }
        [Test]
        public void TestProtoFileWithNestedClass()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(0, proto.TempPath, "-nest_classes:true");
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple." + test + "+MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithExpandedNsDirectories()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(@"nunit\simple\" + test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(0, proto.TempPath, "-expand_namespace_directories:true");
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileDisablingClsComplianceFlags()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", @"
package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional uint32 name = 1;
} "))
            {
                //CS3021: Warning as Error: xx does not need a CLSCompliant attribute because the assembly does not have a CLSCompliant attribute
                RunProtoGen(0, proto.TempPath);
                RunCsc(1, source.TempPath, "/warnaserror+");
                //Now we know it fails, make it pass by turning off cls_compliance generation
                RunProtoGen(0, proto.TempPath, "-cls_compliance:false");
                Assembly a = RunCsc(0, source.TempPath, "/warnaserror+");
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithNewExtension()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".Generated.cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(0, proto.TempPath, "-file_extension:.Generated.cs");
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithUmbrellaNamespace()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(0, proto.TempPath, "-umbrella_namespace:MyUmbrella.Namespace");
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple.MyUmbrella.Namespace." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithIgnoredUmbrellaNamespaceDueToNesting()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(0, proto.TempPath, "-nest_classes:true", "-umbrella_namespace:MyUmbrella.Namespace");
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple." + test + "+MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithExplicitEmptyUmbrellaNamespace()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", @"
package nunit.simple;
// Test a very simple message.
message " + test + @" {
  optional string name = 1;
} "))
            {
                //Forces the umbrella class to not use a namespace even if a collision with a type is detected.
                RunProtoGen(0, proto.TempPath, "-umbrella_namespace:");
                //error CS0441: 'nunit.simple.TestProtoFileWithExplicitEmptyUmbrellaNamespace': a class cannot be both static and sealed
                RunCsc(1, source.TempPath);
            }
        }
        [Test]
        public void TestProtoFileWithNewOutputFolder()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(@"generated-code\" + test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                RunProtoGen(1, proto.TempPath, "-output_directory:generated-code");
                Directory.CreateDirectory("generated-code");
                RunProtoGen(0, proto.TempPath, "-output_directory:generated-code");
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileAndIgnoreGoogleProtobuf()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", @"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).namespace = ""MyNewNamespace"";
" + DefaultProto))
            {
                string google = Path.Combine(TempPath, "google\\protobuf");
                Directory.CreateDirectory(google);
                foreach (string file in Directory.GetFiles(Path.Combine(OriginalWorkingDirectory, "google\\protobuf")))
                {
                    File.Copy(file, Path.Combine(google, Path.GetFileName(file)));
                }

                Assert.AreEqual(0, Directory.GetFiles(TempPath, "*.cs").Length);
                RunProtoGen(0, proto.TempPath, "-ignore_google_protobuf:true");
                Assert.AreEqual(1, Directory.GetFiles(TempPath, "*.cs").Length);

                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("MyNewNamespace.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("MyNewNamespace." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithoutIgnoreGoogleProtobuf()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", @"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).namespace = ""MyNewNamespace"";
" + DefaultProto))
            {
                string google = Path.Combine(TempPath, "google\\protobuf");
                Directory.CreateDirectory(google);
                foreach (string file in Directory.GetFiles(Path.Combine(OriginalWorkingDirectory, "google\\protobuf")))
                {
                    File.Copy(file, Path.Combine(google, Path.GetFileName(file)));
                }

                Assert.AreEqual(0, Directory.GetFiles(TempPath, "*.cs").Length);
                //Without the option this fails due to being unable to resolve google/protobuf descriptors
                RunProtoGen(1, proto.TempPath, "-ignore_google_protobuf:false");
            }
        }

        // *******************************************************************
        // The following tests excercise options for protoc.exe
        // *******************************************************************

        [Test]
        public void TestProtoFileWithIncludeImports()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", @"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).namespace = ""MyNewNamespace"";

package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
} "))
            {
                string google = Path.Combine(TempPath, "google\\protobuf");
                Directory.CreateDirectory(google);
                foreach (string file in Directory.GetFiles(Path.Combine(OriginalWorkingDirectory, "google\\protobuf")))
                {
                    File.Copy(file, Path.Combine(google, Path.GetFileName(file)));
                }

                Assert.AreEqual(0, Directory.GetFiles(TempPath, "*.cs").Length);
                //if you specify the protoc option --include_imports this should build three source files
                RunProtoGen(0, proto.TempPath, "--include_imports");
                Assert.AreEqual(3, Directory.GetFiles(TempPath, "*.cs").Length);

                //you can (and should) simply omit the inclusion of the extra source files in your project
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("MyNewNamespace.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("MyNewNamespace." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileWithIncludeImportsAndIgnoreGoogleProtobuf()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", @"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).namespace = ""MyNewNamespace"";

package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
} "))
            {
                string google = Path.Combine(TempPath, "google\\protobuf");
                Directory.CreateDirectory(google);
                foreach (string file in Directory.GetFiles(Path.Combine(OriginalWorkingDirectory, "google\\protobuf")))
                    File.Copy(file, Path.Combine(google, Path.GetFileName(file)));

                Assert.AreEqual(0, Directory.GetFiles(TempPath, "*.cs").Length);
                //Even with --include_imports, if you provide -ignore_google_protobuf:true you only get the one source file
                RunProtoGen(0, proto.TempPath, "-ignore_google_protobuf:true", "--include_imports");
                Assert.AreEqual(1, Directory.GetFiles(TempPath, "*.cs").Length);

                //you can (and should) simply omit the inclusion of the extra source files in your project
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("MyNewNamespace.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("MyNewNamespace." + test, true, true);
            }
        }
        [Test]
        public void TestProtoFileKeepingTheProtoBuffer()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile protobuf = TempFile.Attach(test + ".pb"))
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", @"
package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
} "))
            {
                RunProtoGen(0, proto.TempPath, "--descriptor_set_out=" + protobuf.TempPath);
                Assert.IsTrue(File.Exists(protobuf.TempPath), "Missing: " + protobuf.TempPath);
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }
        //Seems the --proto_path or -I option is non-functional for me.  Maybe others have luck?
        [Test, Ignore("http://code.google.com/p/protobuf/issues/detail?id=40")]
        public void TestProtoFileInDifferentDirectory()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                Environment.CurrentDirectory = OriginalWorkingDirectory;
                RunProtoGen(0, proto.TempPath, "--proto_path=" + TempPath);
                Assembly a = RunCsc(0, source.TempPath);
                //assert that the message type is in the expected namespace
                Type t = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple." + test, true, true);
            }
        }

        // *******************************************************************
        // Handling of mutliple input files
        // *******************************************************************

        [Test]
        public void TestMultipleProtoFiles()
        {
            Setup();
            using (TempFile source1 = TempFile.Attach("MyMessage.cs"))
            using (ProtoFile proto1 = new ProtoFile("MyMessage.proto", @"
package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
}"))
            using (TempFile source2 = TempFile.Attach("MyMessageList.cs"))
            using (ProtoFile proto2 = new ProtoFile("MyMessageList.proto", @"
package nunit.simple;
import ""MyMessage.proto"";
// Test a very simple message.
message MyMessageList {
  repeated MyMessage messages = 1;
}"))
            {
                RunProtoGen(0, proto1.TempPath, proto2.TempPath);
                Assembly a = RunCsc(0, source1.TempPath, source2.TempPath);
                //assert that the message type is in the expected namespace
                Type t1 = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t1), "Expect an IMessage");
                //assert that the message type is in the expected namespace
                Type t2 = a.GetType("nunit.simple.MyMessageList", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t2), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple.Proto.MyMessage", true, true);
                a.GetType("nunit.simple.Proto.MyMessageList", true, true);
            }
        }
        [Test]
        public void TestOneProtoFileWithBufferFile()
        {
            Setup();
            using (TempFile source1 = TempFile.Attach("MyMessage.cs"))
            using (TempFile protobuf = TempFile.Attach("MyMessage.pb"))
            using (ProtoFile proto1 = new ProtoFile("MyMessage.proto", @"
package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
}"))
            using (TempFile source2 = TempFile.Attach("MyMessageList.cs"))
            using (ProtoFile proto2 = new ProtoFile("MyMessageList.proto", @"
package nunit.simple;
import ""MyMessage.proto"";
// Test a very simple message.
message MyMessageList {
  repeated MyMessage messages = 1;
}"))
            {
                //build the proto buffer for MyMessage
                RunProtoGen(0, proto1.TempPath, "--descriptor_set_out=" + protobuf.TempPath);
                //build the MyMessageList proto-buffer and generate code by including MyMessage.pb
                RunProtoGen(0, proto2.TempPath, protobuf.TempPath);
                Assembly a = RunCsc(0, source1.TempPath, source2.TempPath);
                //assert that the message type is in the expected namespace
                Type t1 = a.GetType("nunit.simple.MyMessage", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t1), "Expect an IMessage");
                //assert that the message type is in the expected namespace
                Type t2 = a.GetType("nunit.simple.MyMessageList", true, true);
                Assert.IsTrue(typeof(IMessage).IsAssignableFrom(t2), "Expect an IMessage");
                //assert that we can find the static descriptor type
                a.GetType("nunit.simple.Proto.MyMessage", true, true);
                a.GetType("nunit.simple.Proto.MyMessageList", true, true);
            }
        }
    }
}
