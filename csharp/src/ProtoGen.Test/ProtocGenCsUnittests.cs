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

using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;

namespace Google.ProtocolBuffers.ProtoGen
{
    /// <summary>
    /// Tests protoc-gen-cs plugin.
    /// </summary>
    [TestFixture]
    [Category("Preprocessor")]
    public partial class ProtocGenCsUnittests
    {
        private static readonly string TempPath = Path.Combine(Path.GetTempPath(), "protoc-gen-cs.Test");

        private const string DefaultProto =
            @"
package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
}";

        #region TestFixture SetUp/TearDown

        private static readonly string OriginalWorkingDirectory = Environment.CurrentDirectory;

        private StringBuilder buffer = new StringBuilder();

        [TestFixtureSetUp]
        public virtual void Setup()
        {
            Teardown();
            Directory.CreateDirectory(TempPath);
            Environment.CurrentDirectory = TempPath;
            this.buffer.Length = 0;
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

        private void RunProtoc(int expect, string protoFile, params string[] args)
        {
            string protoPath = string.Format("-I. -I\"{0}\"", OriginalWorkingDirectory);
            string plugin = string.Format("--plugin=\"{0}\"", Path.Combine(OriginalWorkingDirectory, "protoc-gen-cs.exe"));
            string csOut = args.Length == 0 ? "--cs_out=." : string.Format("--cs_out=\"{0}:.\"", string.Join(" ", args));
            // Start the child process.
            Process p = new Process();
            // Redirect the output stream of the child process.
            p.StartInfo.CreateNoWindow = true;
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardError = true;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.WorkingDirectory = TempPath;
            p.StartInfo.FileName = Path.Combine(OriginalWorkingDirectory, "protoc.exe");
            p.StartInfo.Arguments = string.Join(" ", new string[] { plugin, csOut, protoPath, protoFile });
            p.Start();
            // Read the output stream first and then wait.
            buffer.AppendLine(string.Format("{0}> \"{1}\" {2}", p.StartInfo.WorkingDirectory, p.StartInfo.FileName, p.StartInfo.Arguments));
            buffer.AppendLine(p.StandardError.ReadToEnd());
            buffer.AppendLine(p.StandardOutput.ReadToEnd());
            p.WaitForExit();
            Assert.AreEqual(expect, p.ExitCode, this.buffer.ToString());
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
                args.Add(String.Format(@"""/r:{0}""",
                                       typeof(Google.ProtocolBuffers.DescriptorProtos.DescriptorProto).Assembly.
                                           Location));
                args.AddRange(sources);

                string exe = Path.Combine(System.Runtime.InteropServices.RuntimeEnvironment.GetRuntimeDirectory(),
                                          "csc.exe");
                ProcessStartInfo psi = new ProcessStartInfo(exe);
                psi.WorkingDirectory = TempPath;
                psi.CreateNoWindow = true;
                psi.UseShellExecute = false;
                psi.RedirectStandardOutput = true;
                psi.RedirectStandardError = true;
                psi.Arguments = string.Join(" ", args.ToArray());
                Process p = Process.Start(psi);
                buffer.AppendLine(string.Format("{0}> \"{1}\" {2}", p.StartInfo.WorkingDirectory, p.StartInfo.FileName, p.StartInfo.Arguments));
                buffer.AppendLine(p.StandardError.ReadToEnd());
                buffer.AppendLine(p.StandardOutput.ReadToEnd());
                p.WaitForExit();
                Assert.AreEqual(expect, p.ExitCode, this.buffer.ToString());

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
                RunProtoc(0, proto.TempPath);
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
            using (
                ProtoFile proto = new ProtoFile(test + ".proto",
                                                @"
package nunit.simple;
// Test a very simple message.
message " +
                                                test + @" {
  optional string name = 1;
} "))
            {
                RunProtoc(0, proto.TempPath);
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
                RunProtoc(0, proto.TempPath, "-namespace=MyNewNamespace");
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
                RunProtoc(0, proto.TempPath, "/umbrella_classname=MyUmbrellaClassname");
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
                RunProtoc(0, proto.TempPath, "-nest_classes=true");
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
                RunProtoc(0, proto.TempPath, "-expand_namespace_directories=true");
                Assembly a = RunCsc(0, source.TempPath);
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
                RunProtoc(0, proto.TempPath, "-file_extension=.Generated.cs");
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
                RunProtoc(0, proto.TempPath, "-umbrella_namespace=MyUmbrella.Namespace");
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
                RunProtoc(0, proto.TempPath, "-nest_classes=true", "-umbrella_namespace=MyUmbrella.Namespace");
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
            using (
                ProtoFile proto = new ProtoFile(test + ".proto",
                                                @"
package nunit.simple;
// Test a very simple message.
message " +
                                                test + @" {
  optional string name = 1;
} "))
            {
                //Forces the umbrella class to not use a namespace even if a collision with a type is detected.
                RunProtoc(0, proto.TempPath, "-umbrella_namespace=");
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
                RunProtoc(1, proto.TempPath, "-output_directory=generated-code");
                Directory.CreateDirectory("generated-code");
                RunProtoc(0, proto.TempPath, "-output_directory=generated-code");
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
            using (
                ProtoFile proto = new ProtoFile(test + ".proto",
                                                @"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).namespace = ""MyNewNamespace"";
" +
                                                DefaultProto))
            {
                string google = Path.Combine(TempPath, "google\\protobuf");
                Directory.CreateDirectory(google);
                foreach (string file in Directory.GetFiles(Path.Combine(OriginalWorkingDirectory, "google\\protobuf")))
                {
                    File.Copy(file, Path.Combine(google, Path.GetFileName(file)));
                }

                Assert.AreEqual(0, Directory.GetFiles(TempPath, "*.cs").Length);
                RunProtoc(0, proto.TempPath);
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
            using (
                ProtoFile proto = new ProtoFile(test + ".proto",
                                                @"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).namespace = ""MyNewNamespace"";
" +
                                                DefaultProto))
            {
                string google = Path.Combine(TempPath, "google\\protobuf");
                Directory.CreateDirectory(google);
                foreach (string file in Directory.GetFiles(Path.Combine(OriginalWorkingDirectory, "google\\protobuf")))
                {
                    File.Copy(file, Path.Combine(google, Path.GetFileName(file)));
                }

                Assert.AreEqual(0, Directory.GetFiles(TempPath, "*.cs").Length);
                //Without the option this fails due to being unable to resolve google/protobuf descriptors
                RunProtoc(0, proto.TempPath);
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
            using (
                ProtoFile proto = new ProtoFile(test + ".proto",
                                                @"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).namespace = ""MyNewNamespace"";

package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
} ")
                )
            {
                string google = Path.Combine(TempPath, "google\\protobuf");
                Directory.CreateDirectory(google);
                foreach (string file in Directory.GetFiles(Path.Combine(OriginalWorkingDirectory, "google\\protobuf")))
                {
                    File.Copy(file, Path.Combine(google, Path.GetFileName(file)));
                }

                Assert.AreEqual(0, Directory.GetFiles(TempPath, "*.cs").Length);
                //if you specify the protoc option --include_imports this should build three source files
                RunProtoc(0, proto.TempPath);
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

        //Seems the --proto_path or -I option is non-functional for me.  Maybe others have luck?
        [Test]
        public void TestProtoFileInDifferentDirectory()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto", DefaultProto))
            {
                Environment.CurrentDirectory = OriginalWorkingDirectory;
                RunProtoc(0, proto.TempPath);
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
            using (
                ProtoFile proto1 = new ProtoFile("MyMessage.proto",
                                                 @"
package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
}")
                )
            using (TempFile source2 = TempFile.Attach("MyMessageList.cs"))
            using (
                ProtoFile proto2 = new ProtoFile("MyMessageList.proto",
                                                 @"
package nunit.simple;
import ""MyMessage.proto"";
// Test a very simple message.
message MyMessageList {
  repeated MyMessage messages = 1;
}")
                )
            {
                RunProtoc(0, proto1.TempPath);
                RunProtoc(0, proto2.TempPath);
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
            using (
                ProtoFile proto1 = new ProtoFile("MyMessage.proto",
                                                 @"
package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
}")
                )
            using (TempFile source2 = TempFile.Attach("MyMessageList.cs"))
            using (
                ProtoFile proto2 = new ProtoFile("MyMessageList.proto",
                                                 @"
package nunit.simple;
import ""MyMessage.proto"";
// Test a very simple message.
message MyMessageList {
  repeated MyMessage messages = 1;
}")
                )
            {
                //build the proto buffer for MyMessage
                RunProtoc(0, proto1.TempPath);
                //build the MyMessageList proto-buffer and generate code by including MyMessage.pb
                RunProtoc(0, proto2.TempPath);
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
        public void TestProtoFileWithService()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto",
@"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).service_generator_type = GENERIC;

package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
}
// test a very simple service.
service TestService {
  rpc Execute (MyMessage) returns (MyMessage);
}"))
            {
                CopyInGoogleProtoFiles();

                RunProtoc(0, proto.TempPath, "-nest_classes=false");
                Assert.AreEqual(1, Directory.GetFiles(TempPath, "*.cs").Length);

                Assembly a = RunCsc(0, source.TempPath);
                //assert that the service type is in the expected namespace
                Type t1 = a.GetType("nunit.simple.TestService", true, true);
                Assert.IsTrue(typeof(IService).IsAssignableFrom(t1), "Expect an IService");
                Assert.IsTrue(t1.IsAbstract, "Expect abstract class");
                //assert that the Stub subclass type is in the expected namespace
                Type t2 = a.GetType("nunit.simple.TestService+Stub", true, true);
                Assert.IsTrue(t1.IsAssignableFrom(t2), "Expect a sub of TestService");
                Assert.IsFalse(t2.IsAbstract, "Expect concrete class");
            }
        }

        [Test]
        public void TestProtoFileWithServiceInternal()
        {
            string test = new StackFrame(false).GetMethod().Name;
            Setup();
            using (TempFile source = TempFile.Attach(test + ".cs"))
            using (ProtoFile proto = new ProtoFile(test + ".proto",
@"
import ""google/protobuf/csharp_options.proto"";
option (google.protobuf.csharp_file_options).service_generator_type = GENERIC;

package nunit.simple;
// Test a very simple message.
message MyMessage {
  optional string name = 1;
}
// test a very simple service.
service TestService {
  rpc Execute (MyMessage) returns (MyMessage);
}"))
            {
                CopyInGoogleProtoFiles();

                RunProtoc(0, proto.TempPath, "-nest_classes=false", "-public_classes=false");
                Assert.AreEqual(1, Directory.GetFiles(TempPath, "*.cs").Length);

                Assembly a = RunCsc(0, source.TempPath);
                //assert that the service type is in the expected namespace
                Type t1 = a.GetType("nunit.simple.TestService", true, true);
                Assert.IsTrue(typeof(IService).IsAssignableFrom(t1), "Expect an IService");
                Assert.IsTrue(t1.IsAbstract, "Expect abstract class");
                //assert that the Stub subclass type is in the expected namespace
                Type t2 = a.GetType("nunit.simple.TestService+Stub", true, true);
                Assert.IsTrue(t1.IsAssignableFrom(t2), "Expect a sub of TestService");
                Assert.IsFalse(t2.IsAbstract, "Expect concrete class");
            }
        }

        private static void CopyInGoogleProtoFiles()
        {
            string google = Path.Combine(TempPath, "google\\protobuf");
            Directory.CreateDirectory(google);
            foreach (string file in Directory.GetFiles(Path.Combine(OriginalWorkingDirectory, "google\\protobuf")))
            {
                File.Copy(file, Path.Combine(google, Path.GetFileName(file)));
            }
        }
    }
}