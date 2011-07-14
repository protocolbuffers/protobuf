using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace Google.ProtocolBuffers.ProtoGen
{
    /// <summary>
    /// Preprocesses any input files with an extension of '.proto' by running protoc.exe.  If arguments
    /// are supplied with '--' prefix they are provided to protoc.exe, otherwise they are assumed to
    /// be used for ProtoGen.exe which is run on the resulting output proto buffer.  If the option
    /// --descriptor_set_out= is specified the proto buffer file is kept, otherwise it will be removed
    /// after code generation.
    /// </summary>
    public class ProgramPreprocess
    {
        private const string ProtocExecutable = "protoc.exe";
        private const string ProtocDirectoryArg = "--protoc_dir=";

        private static int Main(string[] args)
        {
            try
            {
                return Environment.ExitCode = Run(args);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex);
                return Environment.ExitCode = 2;
            }
        }

        public static int Run(params string[] args)
        {
            bool deleteFile = false;
            string tempFile = null;
            int result;
            bool doHelp = args.Length == 0;
            try
            {
                List<string> protocArgs = new List<string>();
                List<string> protoGenArgs = new List<string>();

                string protocFile = GuessProtocFile(args);

                foreach (string arg in args)
                {
                    doHelp |= StringComparer.OrdinalIgnoreCase.Equals(arg, "/?");
                    doHelp |= StringComparer.OrdinalIgnoreCase.Equals(arg, "/help");
                    doHelp |= StringComparer.OrdinalIgnoreCase.Equals(arg, "-?");
                    doHelp |= StringComparer.OrdinalIgnoreCase.Equals(arg, "-help");

                    if (arg.StartsWith("--descriptor_set_out="))
                    {
                        tempFile = arg.Substring("--descriptor_set_out=".Length);
                        protoGenArgs.Add(tempFile);
                    }
                }

                if (doHelp)
                {
                    Console.WriteLine();
                    Console.WriteLine("PROTOC.exe: Use any of the following options that begin with '--':");
                    Console.WriteLine();
                    try
                    {
                        RunProtoc(protocFile, "--help");
                    }
                    catch (Exception ex)
                    {
                        Console.Error.WriteLine(ex.Message);
                    }
                    Console.WriteLine();
                    Console.WriteLine();
                    Console.WriteLine(
                        "PROTOGEN.exe: The following options are used to specify defaults for code generation.");
                    Console.WriteLine();
                    Program.Main(new string[0]);
                    Console.WriteLine();
                    Console.WriteLine("The following option enables PROTOGEN.exe to find PROTOC.exe");
                    Console.WriteLine("{0}<directory containing protoc.exe>", ProtocDirectoryArg);
                    return 0;
                }

                foreach (string arg in args)
                {
                    if (arg.StartsWith(ProtocDirectoryArg))
                    {
                        // Handled earlier
                        continue;
                    }
                    if (arg.StartsWith("--"))
                    {
                        protocArgs.Add(arg);
                    }
                    else if (File.Exists(arg) &&
                             StringComparer.OrdinalIgnoreCase.Equals(".proto", Path.GetExtension(arg)))
                    {
                        if (tempFile == null)
                        {
                            deleteFile = true;
                            tempFile = Path.GetTempFileName();
                            protocArgs.Add(String.Format("--descriptor_set_out={0}", tempFile));
                            protoGenArgs.Add(tempFile);
                        }
                        protocArgs.Add(arg);
                    }
                    else
                    {
                        protoGenArgs.Add(arg);
                    }
                }

                if (tempFile != null)
                {
                    result = RunProtoc(protocFile, protocArgs.ToArray());
                    if (result != 0)
                    {
                        return result;
                    }
                }

                result = Program.Main(protoGenArgs.ToArray());
            }
            finally
            {
                if (deleteFile && tempFile != null && File.Exists(tempFile))
                {
                    File.Delete(tempFile);
                }
            }
            return result;
        }

        /// <summary>
        /// Tries to work out where protoc is based on command line arguments, the current
        /// directory, the directory containing protogen, and the path.
        /// </summary>
        /// <returns>The path to protoc.exe, or null if it can't be found.</returns>
        private static string GuessProtocFile(params string[] args)
        {
            // Why oh why is this not in System.IO.Path or Environment...?
            List<string> searchPath = new List<string>();
            foreach (string arg in args)
            {
                if (arg.StartsWith("--protoc_dir="))
                {
                    searchPath.Add(arg.Substring(ProtocDirectoryArg.Length));
                }
            }
            searchPath.Add(Environment.CurrentDirectory);
            searchPath.Add(AppDomain.CurrentDomain.BaseDirectory);
            searchPath.AddRange((Environment.GetEnvironmentVariable("PATH") ?? String.Empty).Split(Path.PathSeparator));

            foreach (string path in searchPath)
            {
                string exeFile = Path.Combine(path, ProtocExecutable);
                if (File.Exists(exeFile))
                {
                    return exeFile;
                }
            }
            return null;
        }

        private static int RunProtoc(string exeFile, params string[] args)
        {
            if (exeFile == null)
            {
                throw new FileNotFoundException(
                    "Unable to locate " + ProtocExecutable +
                    " make sure it is in the PATH, cwd, or exe dir, or use --protoc_dir=...");
            }

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i].IndexOf(' ') > 0 && args[i][0] != '"')
                {
                    args[i] = '"' + args[i] + '"';
                }
            }

            ProcessStartInfo psi = new ProcessStartInfo(exeFile);
            psi.Arguments = String.Join(" ", args);
            psi.RedirectStandardError = true;
            psi.RedirectStandardInput = false;
            psi.RedirectStandardOutput = true;
            psi.ErrorDialog = false;
            psi.CreateNoWindow = true;
            psi.UseShellExecute = false;
            psi.WorkingDirectory = Environment.CurrentDirectory;

            Process process = Process.Start(psi);
            if (process == null)
            {
                return 1;
            }

            process.WaitForExit();

            string tmp = process.StandardOutput.ReadToEnd();
            if (tmp.Trim().Length > 0)
            {
                Console.Out.WriteLine(tmp);
            }
            tmp = process.StandardError.ReadToEnd();
            if (tmp.Trim().Length > 0)
            {
                Console.Error.WriteLine(tmp);
            }
            return process.ExitCode;
        }
    }
}