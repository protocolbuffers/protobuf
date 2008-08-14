using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Helper class to control indentation
  /// </summary>
  internal class TextGenerator {

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
    /// Creates a generator writing to the given writer.
    /// </summary>
    internal TextGenerator(TextWriter writer) {
      this.writer = writer;
    }

    /// <summary>
    /// Indents text by two spaces. After calling Indent(), two spaces
    /// will be inserted at the beginning of each line of text. Indent() may
    /// be called multiple times to produce deeper indents.
    /// </summary>
    internal void Indent() {
      indent.Append("  ");
    }

    /// <summary>
    /// Reduces the current indent level by two spaces.
    /// </summary>
    internal void Outdent() {
      if (indent.Length == 0) {
        throw new InvalidOperationException("Too many calls to Outdent()");
      }
      indent.Length -= 2;
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

    private void Write(string data) {
      if (atStartOfLine) {
        atStartOfLine = false;
        writer.Write(indent);
      }
      writer.Write(data);
    }
  }
}
