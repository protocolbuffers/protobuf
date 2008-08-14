using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Descriptors;
using System.Collections;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Provides ASCII text formatting support for messages.
  /// TODO(jonskeet): Parsing support.
  /// </summary>
  public static class TextFormat {

    /// <summary>
    /// Outputs a textual representation of the Protocol Message supplied into
    /// the parameter output.
    /// </summary>
    public static void Print(IMessage message, TextWriter output) {
      TextGenerator generator = new TextGenerator(output);
      Print(message, generator);
    }

    /// <summary>
    /// Outputs a textual representation of <paramref name="fields" /> to <paramref name="output"/>.
    /// </summary>
    /// <param name="fields"></param>
    /// <param name="output"></param>
    public static void Print(UnknownFieldSet fields, TextWriter output) {
      TextGenerator generator = new TextGenerator(output);
      PrintUnknownFields(fields, generator);
    }

    public static string PrintToString(IMessage message) {
      StringWriter text = new StringWriter();
      Print(message, text);
      return text.ToString();
    }

    public static string PrintToString(UnknownFieldSet fields) {
      StringWriter text = new StringWriter();
      Print(fields, text);
      return text.ToString();
    }

    private static void Print(IMessage message, TextGenerator generator) {
      MessageDescriptor descriptor = message.DescriptorForType;
      foreach (KeyValuePair<FieldDescriptor, object> entry in message.AllFields) {
        PrintField(entry.Key, entry.Value, generator);
      }
      PrintUnknownFields(message.UnknownFields, generator);
    }

    internal static void PrintField(FieldDescriptor field, object value, TextGenerator generator) {
      if (field.IsRepeated) {
        // Repeated field.  Print each element.
        foreach (object element in (IEnumerable) value) {
          PrintSingleField(field, element, generator);
        }
      } else {
        PrintSingleField(field, value, generator);
      }
    }

    private static void PrintSingleField(FieldDescriptor field, Object value, TextGenerator generator) {
      if (field.IsExtension) {
        generator.Print("[");
        // We special-case MessageSet elements for compatibility with proto1.
        if (field.ContainingType.Options.MessageSetWireFormat
            && field.FieldType == FieldType.Message
            && field.IsOptional
            // object equality (TODO(jonskeet): Work out what this comment means!)
            && field.ExtensionScope == field.MessageType) {
          generator.Print(field.MessageType.FullName);
        } else {
          generator.Print(field.FullName);
        }
        generator.Print("]");
      } else {
        if (field.FieldType == FieldType.Group) {
          // Groups must be serialized with their original capitalization.
          generator.Print(field.MessageType.Name);
        } else {
          generator.Print(field.Name);
        }
      }

      if (field.MappedType == MappedType.Message) {
        generator.Print(" {\n");
        generator.Indent();
      } else {
        generator.Print(": ");
      }

      PrintFieldValue(field, value, generator);

      if (field.MappedType == MappedType.Message) {
        generator.Outdent();
        generator.Print("}");
      }
      generator.Print("\n");
    }

    private static void PrintFieldValue(FieldDescriptor field, object value, TextGenerator generator) {
      switch (field.FieldType) {
        case FieldType.Int32:
        case FieldType.Int64:
        case FieldType.SInt32:
        case FieldType.SInt64:
        case FieldType.SFixed32:
        case FieldType.SFixed64:
        case FieldType.Float:
        case FieldType.Double:
        case FieldType.UInt32:
        case FieldType.UInt64:
        case FieldType.Fixed32:
        case FieldType.Fixed64:
          // Good old ToString() does what we want for these types. (Including the
          // unsigned ones, unlike with Java.)
          generator.Print(value.ToString());
          break;
        case FieldType.Bool:
          // Explicitly use the Java true/false
          generator.Print((bool) value ? "true" : "false");
          break;

        case FieldType.String:
          generator.Print("\"");
          generator.Print(EscapeText((string) value));
          generator.Print("\"");
          break;

        case FieldType.Bytes: {
          generator.Print("\"");
          generator.Print(EscapeBytes((ByteString) value));
          generator.Print("\"");
          break;
        }

        case FieldType.Enum: {
          generator.Print(((EnumValueDescriptor) value).Name);
          break;
        }

        case FieldType.Message:
        case FieldType.Group:
          Print((IMessage) value, generator);
          break;
      }
    }

    private static void PrintUnknownFields(UnknownFieldSet unknownFields, TextGenerator generator) {
      foreach (KeyValuePair<int, UnknownField> entry in unknownFields.FieldDictionary) {
        String prefix = entry.Key.ToString() + ": ";
        UnknownField field = entry.Value;

        foreach (ulong value in field.VarintList) {
          generator.Print(entry.Key.ToString());
          generator.Print(": ");
          generator.Print(value.ToString());
          generator.Print("\n");
        }
        foreach (uint value in field.Fixed32List) {
          generator.Print(entry.Key.ToString());
          generator.Print(": ");
          // FIXME(jonskeet): Get format of this right; in Java it's %08x. Find out what this means
          // Also check we're okay in terms of signed/unsigned.
          generator.Print(string.Format("0x{0:x}", value));
          generator.Print("\n");
        }
        foreach (ulong value in field.Fixed64List) {
          generator.Print(entry.Key.ToString());
          generator.Print(": ");
          // FIXME(jonskeet): Get format of this right; in Java it's %016x. Find out what this means
          // Also check we're okay in terms of signed/unsigned.
          generator.Print(string.Format("0x{0:x}", value));
          generator.Print("\n");
        }
        foreach (ByteString value in field.LengthDelimitedList) {
          generator.Print(entry.Key.ToString());
          generator.Print(": \"");
          generator.Print(EscapeBytes(value));
          generator.Print("\"\n");
        }
        foreach (UnknownFieldSet value in field.GroupList) {
          generator.Print(entry.Key.ToString());
          generator.Print(" {\n");
          generator.Indent();
          PrintUnknownFields(value, generator);
          generator.Outdent();
          generator.Print("}\n");
        }
      }
    }


    internal static ulong ParseUInt64(string text) {
      return (ulong) ParseInteger(text, true, false);
    }

    internal static long ParseInt64(string text) {
      return ParseInteger(text, true, false);
    }

    internal static uint ParseUInt32(string text) {
      return (uint) ParseInteger(text, true, false);
    }

    internal static int ParseInt32(string text) {
      return (int) ParseInteger(text, true, false);
    }

    /// <summary>
    /// Parses an integer in hex (leading 0x), decimal (no prefix) or octal (leading 0).
    /// Only a negative sign is permitted, and it must come before the radix indicator.
    /// </summary>
    private static long ParseInteger(string text, bool isSigned, bool isLong) {
      string original = text;
      bool negative = false;
      if (text.StartsWith("-")) {
        if (!isSigned) {
          throw new FormatException("Number must be positive: " + original);
        }
        negative = true;
        text = text.Substring(1);
      }

      int radix = 10;
      if (text.StartsWith("0x")) {
        radix = 16;
        text = text.Substring(2);
      } else if (text.StartsWith("0")) {
        radix = 8;
        text = text.Substring(1);
      }

      ulong result = Convert.ToUInt64(text, radix);

      if (negative) {
        ulong max = isLong ? 0x8000000UL : 0x8000L;
        if (result > max) {
          throw new FormatException("Number of out range: " + original);
        }
        return -((long) result);
      } else {
        ulong max = isSigned 
            ? (isLong ? (ulong) long.MaxValue : int.MaxValue)
            : (isLong ? ulong.MaxValue : uint.MaxValue);
        if (result > max) {
          throw new FormatException("Number of out range: " + original);
        }
        return (long) result;
      }
    }

    /// <summary>
    /// Tests a character to see if it's an octal digit.
    /// </summary>
    private static bool IsOctal(char c) {
      return '0' <= c && c <= '7';
    }

    /// <summary>
    /// Tests a character to see if it's a hex digit.
    /// </summary>
    private static bool IsHex(char c) {
      return ('0' <= c && c <= '9') ||
             ('a' <= c && c <= 'f') ||
             ('A' <= c && c <= 'F');
    }

    /// <summary>
    /// Interprets a character as a digit (in any base up to 36) and returns the
    /// numeric value.
    /// </summary>
    private static int ParseDigit(char c) {
      if ('0' <= c && c <= '9') {
        return c - '0';
      } else if ('a' <= c && c <= 'z') {
        return c - 'a' + 10;
      } else {
        return c - 'A' + 10;
      }
    }

    /// <summary>
    /// Like <see cref="EscapeBytes" /> but escapes a text string.
    /// The string is first encoded as UTF-8, then each byte escaped individually.
    /// The returned value is guaranteed to be entirely ASCII.
    /// </summary>
    static String EscapeText(string input) {
      return EscapeBytes(ByteString.CopyFromUtf8(input));
    }
    /// <summary>
    /// Escapes bytes in the format used in protocol buffer text format, which
    /// is the same as the format used for C string literals.  All bytes
    /// that are not printable 7-bit ASCII characters are escaped, as well as
    /// backslash, single-quote, and double-quote characters.  Characters for
    /// which no defined short-hand escape sequence is defined will be escaped
    /// using 3-digit octal sequences.
    /// The returned value is guaranteed to be entirely ASCII.
    /// </summary>
    private static String EscapeBytes(ByteString input) {
      StringBuilder builder = new StringBuilder(input.Length);
      foreach (byte b in input) {
        switch (b) {
          // C# does not use \a or \v
          case 0x07: builder.Append("\\a" ); break;
          case (byte)'\b': builder.Append("\\b" ); break;
          case (byte)'\f': builder.Append("\\f" ); break;
          case (byte)'\n': builder.Append("\\n" ); break;
          case (byte)'\r': builder.Append("\\r" ); break;
          case (byte)'\t': builder.Append("\\t" ); break;
          case 0x0b: builder.Append("\\v" ); break;
          case (byte)'\\': builder.Append("\\\\"); break;
          case (byte)'\'': builder.Append("\\\'"); break;
          case (byte)'"' : builder.Append("\\\""); break;
          default:
            if (b >= 0x20) {
              builder.Append((char) b);
            } else {
              builder.Append('\\');
              builder.Append((char) ('0' + ((b >> 6) & 3)));
              builder.Append((char) ('0' + ((b >> 3) & 7)));
              builder.Append((char) ('0' + (b & 7)));
            }
            break;
        }
      }
      return builder.ToString();
    }

    /// <summary>
    /// Performs string unescaping from C style (octal, hex, form feeds, tab etc) into a byte string.
    /// </summary>
    internal static ByteString UnescapeBytes(string input) {
      byte[] result = new byte[input.Length];
      int pos = 0;
      for (int i = 0; i < input.Length; i++) {
        char c = input[i];
        if (c > 127 || c < 32) {
          throw new FormatException("Escaped string must only contain ASCII");
        }
        if (c != '\\') {
          result[pos++] = (byte) c;
          continue;
        }
        if (i + 1 >= input.Length) {
          throw new FormatException("Invalid escape sequence: '\\' at end of string.");
        }

        i++;
        c = input[i];
        if (c >= '0' && c <= '7') {
          // Octal escape. 
          int code = ParseDigit(c);
          if (i + 1 < input.Length && IsOctal(input[i+1])) {
            i++;
            code = code * 8 + ParseDigit(input[i]);
          }
          if (i + 1 < input.Length && IsOctal(input[i+1])) {
            i++;
            code = code * 8 + ParseDigit(input[i]);
          }
          result[pos++] = (byte) code;
        } else {
          switch (c) {
            case 'a': result[pos++] = 0x07; break;
            case 'b': result[pos++] = (byte) '\b'; break;
            case 'f': result[pos++] = (byte) '\f'; break;
            case 'n': result[pos++] = (byte) '\n'; break;
            case 'r': result[pos++] = (byte) '\r'; break;
            case 't': result[pos++] = (byte) '\t'; break;
            case 'v': result[pos++] = 0x0b; break;
            case '\\': result[pos++] = (byte) '\\'; break;
            case '\'': result[pos++] = (byte) '\''; break;
            case '"': result[pos++] = (byte) '\"'; break;

            case 'x':
              // hex escape
              int code;
              if (i + 1 < input.Length && IsHex(input[i+1])) {
                i++;
                code = ParseDigit(input[i]);
              } else {
                throw new FormatException("Invalid escape sequence: '\\x' with no digits");
              }
              if (i + 1 < input.Length && IsHex(input[i+1])) {
                ++i;
                code = code * 16 + ParseDigit(input[i]);
              }
              result[pos++] = (byte)code;
              break;

            default:
              throw new FormatException("Invalid escape sequence: '\\" + c + "'");
          }
        }
      }

      return ByteString.CopyFrom(result, 0, pos);
    }
  }
}
