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
using System.IO;
using System.Text.RegularExpressions;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen
{
    /// <summary>
    /// All the configuration required for the generator - where to generate
    /// output files, the location of input files etc. While this isn't immutable
    /// in practice, the contents shouldn't be changed after being passed to
    /// the generator.
    /// </summary>
    public sealed class GeneratorOptions
    {
        private static Dictionary<string, string> LineBreaks =
            new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase)
                {
                    {"Windows", "\r\n"},
                    {"Unix", "\n"},
                    {"Default", Environment.NewLine}
                };

        public IList<string> InputFiles { get; set; }

        public GeneratorOptions()
        {
            LineBreak = Environment.NewLine;
        }

        /// <summary>
        /// Attempts to validate the options, but doesn't throw an exception if they're invalid.
        /// Instead, when this method returns false, the output variable will contain a collection
        /// of reasons for the validation failure.
        /// </summary>
        /// <param name="reasons">Variable to receive a list of reasons in case of validation failure.</param>
        /// <returns>true if the options are valid; false otherwise</returns>
        public bool TryValidate(out IList<string> reasons)
        {
            List<string> tmpReasons = new List<string>();

            ParseArguments(tmpReasons);

            // Output directory validation
            if (string.IsNullOrEmpty(FileOptions.OutputDirectory))
            {
                tmpReasons.Add("No output directory specified");
            }
            else
            {
                if (!Directory.Exists(FileOptions.OutputDirectory))
                {
                    tmpReasons.Add("Specified output directory (" + FileOptions.OutputDirectory + " doesn't exist.");
                }
            }

            // Input file validation (just in terms of presence)
            if (InputFiles == null || InputFiles.Count == 0)
            {
                tmpReasons.Add("No input files specified");
            }
            else
            {
                foreach (string input in InputFiles)
                {
                    FileInfo fi = new FileInfo(input);
                    if (!fi.Exists)
                    {
                        tmpReasons.Add("Input file " + input + " doesn't exist.");
                    }
                }
            }

            if (tmpReasons.Count != 0)
            {
                reasons = tmpReasons;
                return false;
            }

            reasons = null;
            return true;
        }

        /// <summary>
        /// Validates that all the options have been set and are valid,
        /// throwing an exception if they haven't.
        /// </summary>
        /// <exception cref="InvalidOptionsException">The options are invalid.</exception>
        public void Validate()
        {
            IList<string> reasons;
            if (!TryValidate(out reasons))
            {
                throw new InvalidOptionsException(reasons);
            }
        }

        // Raw arguments, used to provide defaults for proto file options
        public IList<string> Arguments { get; set; }

        [Obsolete("Please use GeneratorOptions.FileOptions.OutputDirectory instead")]
        public string OutputDirectory
        {
            get { return FileOptions.OutputDirectory; }
            set
            {
                CSharpFileOptions.Builder bld = FileOptions.ToBuilder();
                bld.OutputDirectory = value;
                FileOptions = bld.Build();
            }
        }

        private static readonly Regex ArgMatch = new Regex(@"^[-/](?<name>[\w_]+?)[:=](?<value>.*)$");
        private CSharpFileOptions fileOptions;

        public CSharpFileOptions FileOptions
        {
            get { return fileOptions ?? (fileOptions = CSharpFileOptions.DefaultInstance); }
            set { fileOptions = value; }
        }

        public string LineBreak { get; set; }

        private void ParseArguments(IList<string> tmpReasons)
        {
            bool doHelp = Arguments.Count == 0;

            InputFiles = new List<string>();
            CSharpFileOptions.Builder builder = FileOptions.ToBuilder();
            Dictionary<string, FieldDescriptor> fields =
                new Dictionary<string, FieldDescriptor>(StringComparer.OrdinalIgnoreCase);
            foreach (FieldDescriptor fld in builder.DescriptorForType.Fields)
            {
                fields.Add(fld.Name, fld);
            }

            foreach (string argument in Arguments)
            {
                if (StringComparer.OrdinalIgnoreCase.Equals("-help", argument) ||
                    StringComparer.OrdinalIgnoreCase.Equals("/help", argument) ||
                    StringComparer.OrdinalIgnoreCase.Equals("-?", argument) ||
                    StringComparer.OrdinalIgnoreCase.Equals("/?", argument))
                {
                    doHelp = true;
                    break;
                }

                Match m = ArgMatch.Match(argument);
                if (m.Success)
                {
                    FieldDescriptor fld;
                    string name = m.Groups["name"].Value;
                    string value = m.Groups["value"].Value;

                    if (fields.TryGetValue(name, out fld))
                    {
                        object obj;
                        if (TryCoerceType(value, fld, out obj, tmpReasons))
                        {
                            builder[fld] = obj;
                        }
                    }
                    else if (name == "line_break")
                    {
                        string tmp;
                        if (LineBreaks.TryGetValue(value, out tmp))
                        {
                            LineBreak = tmp;
                        }
                        else
                        {
                            tmpReasons.Add("Invalid value for 'line_break': " + value + ".");
                        }
                    }
                    else if (!File.Exists(argument))
                    {
                        doHelp = true;
                        tmpReasons.Add("Unknown argument '" + name + "'.");
                    }
                    else
                    {
                        InputFiles.Add(argument);
                    }
                }
                else
                {
                    InputFiles.Add(argument);
                }
            }

            if (doHelp || InputFiles.Count == 0)
            {
                tmpReasons.Add("Arguments:");
                foreach (KeyValuePair<string, FieldDescriptor> field in fields)
                {
                    tmpReasons.Add(String.Format("-{0}=[{1}]", field.Key, field.Value.FieldType));
                }
                tmpReasons.Add("-line_break=[" + string.Join("|", new List<string>(LineBreaks.Keys).ToArray()) + "]");
                tmpReasons.Add("followed by one or more file paths.");
            }
            else
            {
                FileOptions = builder.Build();
            }
        }

        private static bool TryCoerceType(string text, FieldDescriptor field, out object value, IList<string> tmpReasons)
        {
            value = null;

            switch (field.FieldType)
            {
                case FieldType.Int32:
                case FieldType.SInt32:
                case FieldType.SFixed32:
                    value = Int32.Parse(text);
                    break;

                case FieldType.Int64:
                case FieldType.SInt64:
                case FieldType.SFixed64:
                    value = Int64.Parse(text);
                    break;

                case FieldType.UInt32:
                case FieldType.Fixed32:
                    value = UInt32.Parse(text);
                    break;

                case FieldType.UInt64:
                case FieldType.Fixed64:
                    value = UInt64.Parse(text);
                    break;

                case FieldType.Float:
                    value = float.Parse(text);
                    break;

                case FieldType.Double:
                    value = Double.Parse(text);
                    break;

                case FieldType.Bool:
                    value = Boolean.Parse(text);
                    break;

                case FieldType.String:
                    value = text;
                    break;

                case FieldType.Enum:
                    {
                        EnumDescriptor enumType = field.EnumType;

                        int number;
                        if (int.TryParse(text, out number))
                        {
                            value = enumType.FindValueByNumber(number);
                            if (value == null)
                            {
                                tmpReasons.Add(
                                    "Enum type \"" + enumType.FullName +
                                    "\" has no value with number " + number + ".");
                                return false;
                            }
                        }
                        else
                        {
                            value = enumType.FindValueByName(text);
                            if (value == null)
                            {
                                tmpReasons.Add(
                                    "Enum type \"" + enumType.FullName +
                                    "\" has no value named \"" + text + "\".");
                                return false;
                            }
                        }

                        break;
                    }

                case FieldType.Bytes:
                case FieldType.Message:
                case FieldType.Group:
                    tmpReasons.Add("Unhandled field type " + field.FieldType.ToString() + ".");
                    return false;
            }

            return true;
        }
    }
}