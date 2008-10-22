// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System;
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Helper class to control indentation. Used for TextFormat and by ProtoGen.
  /// </summary>
  public sealed class TextGenerator {

    /// <summary>
    /// Writer to write formatted text to.
    /// </summary>
    private readonly TextWriter writer;

    /// <summary>
    /// Keeps track of whether the next piece of text should be indented
    /// </summary>
    bool atStartOfLine = true;

    /// <summary>
    /// Keeps track of the current level of indentation
    /// </summary>
    readonly StringBuilder indent = new StringBuilder();

    /// <summary>
    /// Creates a generator writing to the given writer. The writer
    /// is not closed by this class.
    /// </summary>
    public TextGenerator(TextWriter writer) {
      this.writer = writer;
    }

    /// <summary>
    /// Indents text by two spaces. After calling Indent(), two spaces
    /// will be inserted at the beginning of each line of text. Indent() may
    /// be called multiple times to produce deeper indents.
    /// </summary>
    public void Indent() {
      indent.Append("  ");
    }

    /// <summary>
    /// Reduces the current indent level by two spaces.
    /// </summary>
    public void Outdent() {
      if (indent.Length == 0) {
        throw new InvalidOperationException("Too many calls to Outdent()");
      }
      indent.Length -= 2;
    }

    public void WriteLine(string text) {
      Print(text);
      Print("\n");
    }

    public void WriteLine(string format, params object[] args) {
      WriteLine(string.Format(format, args));
    }

    public void WriteLine() {
      WriteLine("");
    }

    /// <summary>
    /// Prints the given text to the output stream, indenting at line boundaries.
    /// </summary>
    /// <param name="text"></param>
    public void Print(string text) {
      int pos = 0;

      for (int i = 0; i < text.Length; i++) {
        if (text[i] == '\n') {
          // TODO(jonskeet): Use Environment.NewLine?
          Write(text.Substring(pos, i - pos + 1));
          pos = i + 1;
          atStartOfLine = true;
        }
      }
      Write(text.Substring(pos));
    }

    public void Write(string format, params object[] args) {
      Write(string.Format(format, args));
    }
    
    private void Write(string data) {
      if (data.Length == 0) {
        return;
      }
      if (atStartOfLine) {
        atStartOfLine = false;
        writer.Write(indent);
      }
      writer.Write(data);
    }
  }
}
