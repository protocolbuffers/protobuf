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
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Helper class to control indentation. Used for TextFormat and by ProtoGen.
    /// </summary>
    public sealed class TextGenerator
    {
        /// <summary>
        /// The string to use at the end of each line. We assume that "Print" is only called using \n
        /// to indicate a line break; that's what we use to detect when we need to indent etc, and
        /// *just* the \n is replaced with the contents of lineBreak.
        /// </summary>
        private readonly string lineBreak;

        /// <summary>
        /// Writer to write formatted text to.
        /// </summary>
        private readonly TextWriter writer;

        /// <summary>
        /// Keeps track of whether the next piece of text should be indented
        /// </summary>
        private bool atStartOfLine = true;

        /// <summary>
        /// Keeps track of the current level of indentation
        /// </summary>
        private readonly StringBuilder indent = new StringBuilder();

        /// <summary>
        /// Creates a generator writing to the given writer. The writer
        /// is not closed by this class.
        /// </summary>
        public TextGenerator(TextWriter writer, string lineBreak)
        {
            this.writer = writer;
            this.lineBreak = lineBreak;
        }

        /// <summary>
        /// Indents text by two spaces. After calling Indent(), two spaces
        /// will be inserted at the beginning of each line of text. Indent() may
        /// be called multiple times to produce deeper indents.
        /// </summary>
        public void Indent()
        {
            indent.Append("  ");
        }

        /// <summary>
        /// Reduces the current indent level by two spaces.
        /// </summary>
        public void Outdent()
        {
            if (indent.Length == 0)
            {
                throw new InvalidOperationException("Too many calls to Outdent()");
            }
            indent.Length -= 2;
        }

        public void WriteLine(string text)
        {
            Print(text);
            Print("\n");
        }

        public void WriteLine(string format, params object[] args)
        {
            WriteLine(string.Format(format, args));
        }

        public void WriteLine()
        {
            WriteLine("");
        }

        /// <summary>
        /// Prints the given text to the output stream, indenting at line boundaries.
        /// </summary>
        /// <param name="text"></param>
        public void Print(string text)
        {
            int pos = 0;

            for (int i = 0; i < text.Length; i++)
            {
                if (text[i] == '\n')
                {
                    // Strip off the \n from what we write
                    Write(text.Substring(pos, i - pos));
                    Write(lineBreak);
                    pos = i + 1;
                    atStartOfLine = true;
                }
            }
            Write(text.Substring(pos));
        }

        public void Write(string format, params object[] args)
        {
            Write(string.Format(format, args));
        }

        private void Write(string data)
        {
            if (data.Length == 0)
            {
                return;
            }
            if (atStartOfLine)
            {
                atStartOfLine = false;
                writer.Write(indent);
            }
            writer.Write(data);
        }
    }
}