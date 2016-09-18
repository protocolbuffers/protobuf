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
using System.Collections;
using System.Globalization;
using System.Text;
using Google.Protobuf.Reflection;
using Google.Protobuf.WellKnownTypes;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Reflection;

namespace Google.Protobuf
{
    /// <summary>
    /// Reflection-based converter from messages to JSON.
    /// </summary>
    /// <remarks>
    /// <para>
    /// Instances of this class are thread-safe, with no mutable state.
    /// </para>
    /// <para>
    /// This is a simple start to get JSON formatting working. As it's reflection-based,
    /// it's not as quick as baking calls into generated messages - but is a simpler implementation.
    /// (This code is generally not heavily optimized.)
    /// </para>
    /// </remarks>
    public sealed class JsonFormatter
    {
        internal const string AnyTypeUrlField = "@type";
        internal const string AnyDiagnosticValueField = "@value";
        internal const string AnyWellKnownTypeValueField = "value";
        private const string TypeUrlPrefix = "type.googleapis.com";
        private const string NameValueSeparator = ": ";
        private const string PropertySeparator = ", ";

        /// <summary>
        /// Returns a formatter using the default settings.
        /// </summary>
        public static JsonFormatter Default { get; } = new JsonFormatter(Settings.Default);

        // A JSON formatter which *only* exists 
        private static readonly JsonFormatter diagnosticFormatter = new JsonFormatter(Settings.Default);

        /// <summary>
        /// The JSON representation of the first 160 characters of Unicode.
        /// Empty strings are replaced by the static constructor.
        /// </summary>
        private static readonly string[] CommonRepresentations = {
            // C0 (ASCII and derivatives) control characters
            "\\u0000", "\\u0001", "\\u0002", "\\u0003",  // 0x00
          "\\u0004", "\\u0005", "\\u0006", "\\u0007",
          "\\b",     "\\t",     "\\n",     "\\u000b",
          "\\f",     "\\r",     "\\u000e", "\\u000f",
          "\\u0010", "\\u0011", "\\u0012", "\\u0013",  // 0x10
          "\\u0014", "\\u0015", "\\u0016", "\\u0017",
          "\\u0018", "\\u0019", "\\u001a", "\\u001b",
          "\\u001c", "\\u001d", "\\u001e", "\\u001f",
            // Escaping of " and \ are required by www.json.org string definition.
            // Escaping of < and > are required for HTML security.
            "", "", "\\\"", "", "",        "", "",        "",  // 0x20
          "", "", "",     "", "",        "", "",        "",
          "", "", "",     "", "",        "", "",        "",  // 0x30
          "", "", "",     "", "\\u003c", "", "\\u003e", "",
          "", "", "",     "", "",        "", "",        "",  // 0x40
          "", "", "",     "", "",        "", "",        "",
          "", "", "",     "", "",        "", "",        "",  // 0x50
          "", "", "",     "", "\\\\",    "", "",        "",
          "", "", "",     "", "",        "", "",        "",  // 0x60
          "", "", "",     "", "",        "", "",        "",
          "", "", "",     "", "",        "", "",        "",  // 0x70
          "", "", "",     "", "",        "", "",        "\\u007f",
            // C1 (ISO 8859 and Unicode) extended control characters
            "\\u0080", "\\u0081", "\\u0082", "\\u0083",  // 0x80
          "\\u0084", "\\u0085", "\\u0086", "\\u0087",
          "\\u0088", "\\u0089", "\\u008a", "\\u008b",
          "\\u008c", "\\u008d", "\\u008e", "\\u008f",
          "\\u0090", "\\u0091", "\\u0092", "\\u0093",  // 0x90
          "\\u0094", "\\u0095", "\\u0096", "\\u0097",
          "\\u0098", "\\u0099", "\\u009a", "\\u009b",
          "\\u009c", "\\u009d", "\\u009e", "\\u009f"
        };

        static JsonFormatter()
        {
            for (int i = 0; i < CommonRepresentations.Length; i++)
            {
                if (CommonRepresentations[i] == "")
                {
                    CommonRepresentations[i] = ((char) i).ToString();
                }
            }
        }

        private readonly Settings settings;

        private bool DiagnosticOnly => ReferenceEquals(this, diagnosticFormatter);

        /// <summary>
        /// Creates a new formatted with the given settings.
        /// </summary>
        /// <param name="settings">The settings.</param>
        public JsonFormatter(Settings settings)
        {
            this.settings = settings;
        }

        /// <summary>
        /// Formats the specified message as JSON.
        /// </summary>
        /// <param name="message">The message to format.</param>
        /// <returns>The formatted message.</returns>
        public string Format(IMessage message)
        {
            var writer = new StringWriter();
            Format(message, writer);
            return writer.ToString();
        }

        /// <summary>
        /// Formats the specified message as JSON.
        /// </summary>
        /// <param name="message">The message to format.</param>
        /// <param name="writer">The TextWriter to write the formatted message to.</param>
        /// <returns>The formatted message.</returns>
        public void Format(IMessage message, TextWriter writer)
        {
            ProtoPreconditions.CheckNotNull(message, nameof(message));
            ProtoPreconditions.CheckNotNull(writer, nameof(writer));

            if (message.Descriptor.IsWellKnownType)
            {
                WriteWellKnownTypeValue(writer, message.Descriptor, message);
            }
            else
            {
                WriteMessage(writer, message);
            }
        }

        /// <summary>
        /// Converts a message to JSON for diagnostic purposes with no extra context.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This differs from calling <see cref="Format(IMessage)"/> on the default JSON
        /// formatter in its handling of <see cref="Any"/>. As no type registry is available
        /// in <see cref="object.ToString"/> calls, the normal way of resolving the type of
        /// an <c>Any</c> message cannot be applied. Instead, a JSON property named <c>@value</c>
        /// is included with the base64 data from the <see cref="Any.Value"/> property of the message.
        /// </para>
        /// <para>The value returned by this method is only designed to be used for diagnostic
        /// purposes. It may not be parsable by <see cref="JsonParser"/>, and may not be parsable
        /// by other Protocol Buffer implementations.</para>
        /// </remarks>
        /// <param name="message">The message to format for diagnostic purposes.</param>
        /// <returns>The diagnostic-only JSON representation of the message</returns>
        public static string ToDiagnosticString(IMessage message)
        {
            ProtoPreconditions.CheckNotNull(message, nameof(message));
            return diagnosticFormatter.Format(message);
        }

        private void WriteMessage(TextWriter writer, IMessage message)
        {
            if (message == null)
            {
                WriteNull(writer);
                return;
            }
            if (DiagnosticOnly)
            {
                ICustomDiagnosticMessage customDiagnosticMessage = message as ICustomDiagnosticMessage;
                if (customDiagnosticMessage != null)
                {
                    writer.Write(customDiagnosticMessage.ToDiagnosticString());
                    return;
                }
            }
            writer.Write("{ ");
            bool writtenFields = WriteMessageFields(writer, message, false);
            writer.Write(writtenFields ? " }" : "}");
        }

        private bool WriteMessageFields(TextWriter writer, IMessage message, bool assumeFirstFieldWritten)
        {
            var fields = message.Descriptor.Fields;
            bool first = !assumeFirstFieldWritten;
            // First non-oneof fields
            foreach (var field in fields.InFieldNumberOrder())
            {
                var accessor = field.Accessor;
                if (field.ContainingOneof != null && field.ContainingOneof.Accessor.GetCaseFieldDescriptor(message) != field)
                {
                    continue;
                }
                // Omit default values unless we're asked to format them, or they're oneofs (where the default
                // value is still formatted regardless, because that's how we preserve the oneof case).
                object value = accessor.GetValue(message);
                if (field.ContainingOneof == null && !settings.FormatDefaultValues && IsDefaultValue(accessor, value))
                {
                    continue;
                }

                // Okay, all tests complete: let's write the field value...
                if (!first)
                {
                    writer.Write(PropertySeparator);
                }

                WriteString(writer, accessor.Descriptor.JsonName);
                writer.Write(NameValueSeparator);
                WriteValue(writer, value);

                first = false;
            }
            return !first;
        }

        /// <summary>
        /// Camel-case converter with added strictness for field mask formatting.
        /// </summary>
        /// <exception cref="InvalidOperationException">The field mask is invalid for JSON representation</exception>
        private static string ToCamelCaseForFieldMask(string input)
        {
            for (int i = 0; i < input.Length; i++)
            {
                char c = input[i];
                if (c >= 'A' && c <= 'Z')
                {
                    throw new InvalidOperationException($"Invalid field mask to be converted to JSON: {input}");
                }
                if (c == '_' && i < input.Length - 1)
                {
                    char next = input[i + 1];
                    if (next < 'a' || next > 'z')
                    {
                        throw new InvalidOperationException($"Invalid field mask to be converted to JSON: {input}");
                    }
                }
            }
            return ToCamelCase(input);
        }

        // Converted from src/google/protobuf/util/internal/utility.cc ToCamelCase
        internal static string ToCamelCase(string input)
        {
            bool capitalizeNext = false;
            bool wasCap = true;
            bool isCap = false;
            bool firstWord = true;
            StringBuilder result = new StringBuilder(input.Length);

            for (int i = 0; i < input.Length; i++, wasCap = isCap)
            {
                isCap = char.IsUpper(input[i]);
                if (input[i] == '_')
                {
                    capitalizeNext = true;
                    if (result.Length != 0)
                    {
                        firstWord = false;
                    }
                    continue;
                }
                else if (firstWord)
                {
                    // Consider when the current character B is capitalized,
                    // first word ends when:
                    // 1) following a lowercase:   "...aB..."
                    // 2) followed by a lowercase: "...ABc..."
                    if (result.Length != 0 && isCap &&
                        (!wasCap || (i + 1 < input.Length && char.IsLower(input[i + 1]))))
                    {
                        firstWord = false;
                        result.Append(input[i]);
                    }
                    else
                    {
                        result.Append(char.ToLowerInvariant(input[i]));
                        continue;
                    }
                }
                else if (capitalizeNext)
                {
                    capitalizeNext = false;
                    if (char.IsLower(input[i]))
                    {
                        result.Append(char.ToUpperInvariant(input[i]));
                        continue;
                    }
                    else
                    {
                        result.Append(input[i]);
                        continue;
                    }
                }
                else
                {
                    result.Append(char.ToLowerInvariant(input[i]));
                }
            }
            return result.ToString();
        }
        
        private static void WriteNull(TextWriter writer)
        {
            writer.Write("null");
        }

        private static bool IsDefaultValue(IFieldAccessor accessor, object value)
        {
            if (accessor.Descriptor.IsMap)
            {
                IDictionary dictionary = (IDictionary) value;
                return dictionary.Count == 0;
            }
            if (accessor.Descriptor.IsRepeated)
            {
                IList list = (IList) value;
                return list.Count == 0;
            }
            switch (accessor.Descriptor.FieldType)
            {
                case FieldType.Bool:
                    return (bool) value == false;
                case FieldType.Bytes:
                    return (ByteString) value == ByteString.Empty;
                case FieldType.String:
                    return (string) value == "";
                case FieldType.Double:
                    return (double) value == 0.0;
                case FieldType.SInt32:
                case FieldType.Int32:
                case FieldType.SFixed32:
                case FieldType.Enum:
                    return (int) value == 0;
                case FieldType.Fixed32:
                case FieldType.UInt32:
                    return (uint) value == 0;
                case FieldType.Fixed64:
                case FieldType.UInt64:
                    return (ulong) value == 0;
                case FieldType.SFixed64:
                case FieldType.Int64:
                case FieldType.SInt64:
                    return (long) value == 0;
                case FieldType.Float:
                    return (float) value == 0f;
                case FieldType.Message:
                case FieldType.Group: // Never expect to get this, but...
                    return value == null;
                default:
                    throw new ArgumentException("Invalid field type");
            }
        }

        /// <summary>
        /// Writes a single value to the given writer as JSON. Only types understood by
        /// Protocol Buffers can be written in this way. This method is only exposed for
        /// advanced use cases; most users should be using <see cref="Format(IMessage)"/>
        /// or <see cref="Format(IMessage, TextWriter)"/>.
        /// </summary>
        /// <param name="writer">The writer to write the value to. Must not be null.</param>
        /// <param name="value">The value to write. May be null.</param>
        public void WriteValue(TextWriter writer, object value)
        {
            if (value == null)
            {
                WriteNull(writer);
            }
            else if (value is bool)
            {
                writer.Write((bool)value ? "true" : "false");
            }
            else if (value is ByteString)
            {
                // Nothing in Base64 needs escaping
                writer.Write('"');
                writer.Write(((ByteString)value).ToBase64());
                writer.Write('"');
            }
            else if (value is string)
            {
                WriteString(writer, (string)value);
            }
            else if (value is IDictionary)
            {
                WriteDictionary(writer, (IDictionary)value);
            }
            else if (value is IList)
            {
                WriteList(writer, (IList)value);
            }
            else if (value is int || value is uint)
            {
                IFormattable formattable = (IFormattable) value;
                writer.Write(formattable.ToString("d", CultureInfo.InvariantCulture));
            }
            else if (value is long || value is ulong)
            {
                writer.Write('"');
                IFormattable formattable = (IFormattable) value;
                writer.Write(formattable.ToString("d", CultureInfo.InvariantCulture));
                writer.Write('"');
            }
            else if (value is System.Enum)
            {
                string name = OriginalEnumValueHelper.GetOriginalName(value);
                if (name != null)
                {
                    WriteString(writer, name);
                }
                else
                {
                    WriteValue(writer, (int)value);
                }
            }
            else if (value is float || value is double)
            {
                string text = ((IFormattable) value).ToString("r", CultureInfo.InvariantCulture);
                if (text == "NaN" || text == "Infinity" || text == "-Infinity")
                {
                    writer.Write('"');
                    writer.Write(text);
                    writer.Write('"');
                }
                else
                {
                    writer.Write(text);
                }
            }
            else if (value is IMessage)
            {
                Format((IMessage)value, writer);
            }
            else
            {
                throw new ArgumentException("Unable to format value of type " + value.GetType());
            }
        }

        /// <summary>
        /// Central interception point for well-known type formatting. Any well-known types which
        /// don't need special handling can fall back to WriteMessage. We avoid assuming that the
        /// values are using the embedded well-known types, in order to allow for dynamic messages
        /// in the future.
        /// </summary>
        private void WriteWellKnownTypeValue(TextWriter writer, MessageDescriptor descriptor, object value)
        {
            // Currently, we can never actually get here, because null values are always handled by the caller. But if we *could*,
            // this would do the right thing.
            if (value == null)
            {
                WriteNull(writer);
                return;
            }
            // For wrapper types, the value will either be the (possibly boxed) "native" value,
            // or the message itself if we're formatting it at the top level (e.g. just calling ToString on the object itself).
            // If it's the message form, we can extract the value first, which *will* be the (possibly boxed) native value,
            // and then proceed, writing it as if we were definitely in a field. (We never need to wrap it in an extra string...
            // WriteValue will do the right thing.)
            if (descriptor.IsWrapperType)
            {
                if (value is IMessage)
                {
                    var message = (IMessage) value;
                    value = message.Descriptor.Fields[WrappersReflection.WrapperValueFieldNumber].Accessor.GetValue(message);
                }
                WriteValue(writer, value);
                return;
            }
            if (descriptor.FullName == Timestamp.Descriptor.FullName)
            {
                WriteTimestamp(writer, (IMessage)value);
                return;
            }
            if (descriptor.FullName == Duration.Descriptor.FullName)
            {
                WriteDuration(writer, (IMessage)value);
                return;
            }
            if (descriptor.FullName == FieldMask.Descriptor.FullName)
            {
                WriteFieldMask(writer, (IMessage)value);
                return;
            }
            if (descriptor.FullName == Struct.Descriptor.FullName)
            {
                WriteStruct(writer, (IMessage)value);
                return;
            }
            if (descriptor.FullName == ListValue.Descriptor.FullName)
            {
                var fieldAccessor = descriptor.Fields[ListValue.ValuesFieldNumber].Accessor;
                WriteList(writer, (IList)fieldAccessor.GetValue((IMessage)value));
                return;
            }
            if (descriptor.FullName == Value.Descriptor.FullName)
            {
                WriteStructFieldValue(writer, (IMessage)value);
                return;
            }
            if (descriptor.FullName == Any.Descriptor.FullName)
            {
                WriteAny(writer, (IMessage)value);
                return;
            }
            WriteMessage(writer, (IMessage)value);
        }

        private void WriteTimestamp(TextWriter writer, IMessage value)
        {
            // TODO: In the common case where this *is* using the built-in Timestamp type, we could
            // avoid all the reflection at this point, by casting to Timestamp. In the interests of
            // avoiding subtle bugs, don't do that until we've implemented DynamicMessage so that we can prove
            // it still works in that case.
            int nanos = (int) value.Descriptor.Fields[Timestamp.NanosFieldNumber].Accessor.GetValue(value);
            long seconds = (long) value.Descriptor.Fields[Timestamp.SecondsFieldNumber].Accessor.GetValue(value);
            writer.Write(Timestamp.ToJson(seconds, nanos, DiagnosticOnly));
        }

        private void WriteDuration(TextWriter writer, IMessage value)
        {
            // TODO: Same as for WriteTimestamp
            int nanos = (int) value.Descriptor.Fields[Duration.NanosFieldNumber].Accessor.GetValue(value);
            long seconds = (long) value.Descriptor.Fields[Duration.SecondsFieldNumber].Accessor.GetValue(value);
            writer.Write(Duration.ToJson(seconds, nanos, DiagnosticOnly));
        }

        private void WriteFieldMask(TextWriter writer, IMessage value)
        {
            var paths = (IList<string>) value.Descriptor.Fields[FieldMask.PathsFieldNumber].Accessor.GetValue(value);
            writer.Write(FieldMask.ToJson(paths, DiagnosticOnly));
        }

        private void WriteAny(TextWriter writer, IMessage value)
        {
            if (DiagnosticOnly)
            {
                WriteDiagnosticOnlyAny(writer, value);
                return;
            }

            string typeUrl = (string) value.Descriptor.Fields[Any.TypeUrlFieldNumber].Accessor.GetValue(value);
            ByteString data = (ByteString) value.Descriptor.Fields[Any.ValueFieldNumber].Accessor.GetValue(value);
            string typeName = Any.GetTypeName(typeUrl);
            MessageDescriptor descriptor = settings.TypeRegistry.Find(typeName);
            if (descriptor == null)
            {
                throw new InvalidOperationException($"Type registry has no descriptor for type name '{typeName}'");
            }
            IMessage message = descriptor.Parser.ParseFrom(data);
            writer.Write("{ ");
            WriteString(writer, AnyTypeUrlField);
            writer.Write(NameValueSeparator);
            WriteString(writer, typeUrl);

            if (descriptor.IsWellKnownType)
            {
                writer.Write(PropertySeparator);
                WriteString(writer, AnyWellKnownTypeValueField);
                writer.Write(NameValueSeparator);
                WriteWellKnownTypeValue(writer, descriptor, message);
            }
            else
            {
                WriteMessageFields(writer, message, true);
            }
            writer.Write(" }");
        }

        private void WriteDiagnosticOnlyAny(TextWriter writer, IMessage value)
        {
            string typeUrl = (string) value.Descriptor.Fields[Any.TypeUrlFieldNumber].Accessor.GetValue(value);
            ByteString data = (ByteString) value.Descriptor.Fields[Any.ValueFieldNumber].Accessor.GetValue(value);
            writer.Write("{ ");
            WriteString(writer, AnyTypeUrlField);
            writer.Write(NameValueSeparator);
            WriteString(writer, typeUrl);
            writer.Write(PropertySeparator);
            WriteString(writer, AnyDiagnosticValueField);
            writer.Write(NameValueSeparator);
            writer.Write('"');
            writer.Write(data.ToBase64());
            writer.Write('"');
            writer.Write(" }");
        }        

        private void WriteStruct(TextWriter writer, IMessage message)
        {
            writer.Write("{ ");
            IDictionary fields = (IDictionary) message.Descriptor.Fields[Struct.FieldsFieldNumber].Accessor.GetValue(message);
            bool first = true;
            foreach (DictionaryEntry entry in fields)
            {
                string key = (string) entry.Key;
                IMessage value = (IMessage) entry.Value;
                if (string.IsNullOrEmpty(key) || value == null)
                {
                    throw new InvalidOperationException("Struct fields cannot have an empty key or a null value.");
                }

                if (!first)
                {
                    writer.Write(PropertySeparator);
                }
                WriteString(writer, key);
                writer.Write(NameValueSeparator);
                WriteStructFieldValue(writer, value);
                first = false;
            }
            writer.Write(first ? "}" : " }");
        }

        private void WriteStructFieldValue(TextWriter writer, IMessage message)
        {
            var specifiedField = message.Descriptor.Oneofs[0].Accessor.GetCaseFieldDescriptor(message);
            if (specifiedField == null)
            {
                throw new InvalidOperationException("Value message must contain a value for the oneof.");
            }

            object value = specifiedField.Accessor.GetValue(message);
            
            switch (specifiedField.FieldNumber)
            {
                case Value.BoolValueFieldNumber:
                case Value.StringValueFieldNumber:
                case Value.NumberValueFieldNumber:
                    WriteValue(writer, value);
                    return;
                case Value.StructValueFieldNumber:
                case Value.ListValueFieldNumber:
                    // Structs and ListValues are nested messages, and already well-known types.
                    var nestedMessage = (IMessage) specifiedField.Accessor.GetValue(message);
                    WriteWellKnownTypeValue(writer, nestedMessage.Descriptor, nestedMessage);
                    return;
                case Value.NullValueFieldNumber:
                    WriteNull(writer);
                    return;
                default:
                    throw new InvalidOperationException("Unexpected case in struct field: " + specifiedField.FieldNumber);
            }
        }

        internal void WriteList(TextWriter writer, IList list)
        {
            writer.Write("[ ");
            bool first = true;
            foreach (var value in list)
            {
                if (!first)
                {
                    writer.Write(PropertySeparator);
                }
                WriteValue(writer, value);
                first = false;
            }
            writer.Write(first ? "]" : " ]");
        }

        internal void WriteDictionary(TextWriter writer, IDictionary dictionary)
        {
            writer.Write("{ ");
            bool first = true;
            // This will box each pair. Could use IDictionaryEnumerator, but that's ugly in terms of disposal.
            foreach (DictionaryEntry pair in dictionary)
            {
                if (!first)
                {
                    writer.Write(PropertySeparator);
                }
                string keyText;
                if (pair.Key is string)
                {
                    keyText = (string) pair.Key;
                }
                else if (pair.Key is bool)
                {
                    keyText = (bool) pair.Key ? "true" : "false";
                }
                else if (pair.Key is int || pair.Key is uint | pair.Key is long || pair.Key is ulong)
                {
                    keyText = ((IFormattable) pair.Key).ToString("d", CultureInfo.InvariantCulture);
                }
                else
                {
                    if (pair.Key == null)
                    {
                        throw new ArgumentException("Dictionary has entry with null key");
                    }
                    throw new ArgumentException("Unhandled dictionary key type: " + pair.Key.GetType());
                }
                WriteString(writer, keyText);
                writer.Write(NameValueSeparator);
                WriteValue(writer, pair.Value);
                first = false;
            }
            writer.Write(first ? "}" : " }");
        }

        /// <summary>
        /// Writes a string (including leading and trailing double quotes) to a builder, escaping as required.
        /// </summary>
        /// <remarks>
        /// Other than surrogate pair handling, this code is mostly taken from src/google/protobuf/util/internal/json_escaping.cc.
        /// </remarks>
        internal static void WriteString(TextWriter writer, string text)
        {
            writer.Write('"');
            for (int i = 0; i < text.Length; i++)
            {
                char c = text[i];
                if (c < 0xa0)
                {
                    writer.Write(CommonRepresentations[c]);
                    continue;
                }
                if (char.IsHighSurrogate(c))
                {
                    // Encountered first part of a surrogate pair.
                    // Check that we have the whole pair, and encode both parts as hex.
                    i++;
                    if (i == text.Length || !char.IsLowSurrogate(text[i]))
                    {
                        throw new ArgumentException("String contains low surrogate not followed by high surrogate");
                    }
                    HexEncodeUtf16CodeUnit(writer, c);
                    HexEncodeUtf16CodeUnit(writer, text[i]);
                    continue;
                }
                else if (char.IsLowSurrogate(c))
                {
                    throw new ArgumentException("String contains high surrogate not preceded by low surrogate");
                }
                switch ((uint) c)
                {
                    // These are not required by json spec
                    // but used to prevent security bugs in javascript.
                    case 0xfeff:  // Zero width no-break space
                    case 0xfff9:  // Interlinear annotation anchor
                    case 0xfffa:  // Interlinear annotation separator
                    case 0xfffb:  // Interlinear annotation terminator

                    case 0x00ad:  // Soft-hyphen
                    case 0x06dd:  // Arabic end of ayah
                    case 0x070f:  // Syriac abbreviation mark
                    case 0x17b4:  // Khmer vowel inherent Aq
                    case 0x17b5:  // Khmer vowel inherent Aa
                        HexEncodeUtf16CodeUnit(writer, c);
                        break;

                    default:
                        if ((c >= 0x0600 && c <= 0x0603) ||  // Arabic signs
                            (c >= 0x200b && c <= 0x200f) ||  // Zero width etc.
                            (c >= 0x2028 && c <= 0x202e) ||  // Separators etc.
                            (c >= 0x2060 && c <= 0x2064) ||  // Invisible etc.
                            (c >= 0x206a && c <= 0x206f))
                        {
                            HexEncodeUtf16CodeUnit(writer, c);
                        }
                        else
                        {
                            // No handling of surrogates here - that's done earlier
                            writer.Write(c);
                        }
                        break;
                }
            }
            writer.Write('"');
        }

        private const string Hex = "0123456789abcdef";
        private static void HexEncodeUtf16CodeUnit(TextWriter writer, char c)
        {
            writer.Write("\\u");
            writer.Write(Hex[(c >> 12) & 0xf]);
            writer.Write(Hex[(c >> 8) & 0xf]);
            writer.Write(Hex[(c >> 4) & 0xf]);
            writer.Write(Hex[(c >> 0) & 0xf]);
        }

        /// <summary>
        /// Settings controlling JSON formatting.
        /// </summary>
        public sealed class Settings
        {
            /// <summary>
            /// Default settings, as used by <see cref="JsonFormatter.Default"/>
            /// </summary>
            public static Settings Default { get; }

            // Workaround for the Mono compiler complaining about XML comments not being on
            // valid language elements.
            static Settings()
            {
                Default = new Settings(false);
            }

            /// <summary>
            /// Whether fields whose values are the default for the field type (e.g. 0 for integers)
            /// should be formatted (true) or omitted (false).
            /// </summary>
            public bool FormatDefaultValues { get; }

            /// <summary>
            /// The type registry used to format <see cref="Any"/> messages.
            /// </summary>
            public TypeRegistry TypeRegistry { get; }

            // TODO: Work out how we're going to scale this to multiple settings. "WithXyz" methods?

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified formatting of default values
            /// and an empty type registry.
            /// </summary>
            /// <param name="formatDefaultValues"><c>true</c> if default values (0, empty strings etc) should be formatted; <c>false</c> otherwise.</param>
            public Settings(bool formatDefaultValues) : this(formatDefaultValues, TypeRegistry.Empty)
            {
            }

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified formatting of default values
            /// and type registry.
            /// </summary>
            /// <param name="formatDefaultValues"><c>true</c> if default values (0, empty strings etc) should be formatted; <c>false</c> otherwise.</param>
            /// <param name="typeRegistry">The <see cref="TypeRegistry"/> to use when formatting <see cref="Any"/> messages.</param>
            public Settings(bool formatDefaultValues, TypeRegistry typeRegistry)
            {
                FormatDefaultValues = formatDefaultValues;
                TypeRegistry = ProtoPreconditions.CheckNotNull(typeRegistry, nameof(typeRegistry));
            }
        }

        // Effectively a cache of mapping from enum values to the original name as specified in the proto file,
        // fetched by reflection.
        // The need for this is unfortunate, as is its unbounded size, but realistically it shouldn't cause issues.
        private static class OriginalEnumValueHelper
        {
            // TODO: In the future we might want to use ConcurrentDictionary, at the point where all
            // the platforms we target have it.
            private static readonly Dictionary<System.Type, Dictionary<object, string>> dictionaries
                = new Dictionary<System.Type, Dictionary<object, string>>();
            
            internal static string GetOriginalName(object value)
            {
                var enumType = value.GetType();
                Dictionary<object, string> nameMapping;
                lock (dictionaries)
                {
                    if (!dictionaries.TryGetValue(enumType, out nameMapping))
                    {
                        nameMapping = GetNameMapping(enumType);
                        dictionaries[enumType] = nameMapping;
                    }
                }

                string originalName;
                // If this returns false, originalName will be null, which is what we want.
                nameMapping.TryGetValue(value, out originalName);
                return originalName;
            }

#if DOTNET35
            // TODO: Consider adding functionality to TypeExtensions to avoid this difference.
            private static Dictionary<object, string> GetNameMapping(System.Type enumType) =>
                enumType.GetFields(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Static)
                    .ToDictionary(f => f.GetValue(null),
                                  f => (f.GetCustomAttributes(typeof(OriginalNameAttribute), false)
                                        .FirstOrDefault() as OriginalNameAttribute)
                                        // If the attribute hasn't been applied, fall back to the name of the field.
                                        ?.Name ?? f.Name);
#else
            private static Dictionary<object, string> GetNameMapping(System.Type enumType) =>
                enumType.GetTypeInfo().DeclaredFields
                    .Where(f => f.IsStatic)
                    .ToDictionary(f => f.GetValue(null),
                                  f => f.GetCustomAttributes<OriginalNameAttribute>()
                                        .FirstOrDefault()
                                        // If the attribute hasn't been applied, fall back to the name of the field.
                                        ?.Name ?? f.Name);
#endif
        }
    }
}
