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
using System.Diagnostics.CodeAnalysis;

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
        private const string NameValueSeparator = ": ";
        private const string ValueSeparator = ", ";
        private const string MultilineValueSeparator = ",";
        private const char ObjectOpenBracket = '{';
        private const char ObjectCloseBracket = '}';
        private const char ListBracketOpen = '[';
        private const char ListBracketClose = ']';

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
            this.settings = ProtoPreconditions.CheckNotNull(settings, nameof(settings));
        }

        /// <summary>
        /// Formats the specified message as JSON.
        /// </summary>
        /// <param name="message">The message to format.</param>
        /// <remarks>This method delegates to <c>Format(IMessage, int)</c> with <c>indentationLevel = 0</c>.</remarks>
        /// <returns>The formatted message.</returns>
        public string Format(IMessage message) => Format(message, indentationLevel: 0);

        /// <summary>
        /// Formats the specified message as JSON.
        /// </summary>
        /// <param name="message">The message to format.</param>
        /// <param name="indentationLevel">Indentation level to start at.</param>
        /// <remarks>To keep consistent indentation when embedding a message inside another JSON string, set <paramref name="indentationLevel"/>. E.g:
        /// <code>
        /// var response = $@"{{
        ///   ""data"": { Format(message, indentationLevel: 1) }
        /// }}"</code>
        /// </remarks>
        /// <returns>The formatted message.</returns>
        public string Format(IMessage message, int indentationLevel)
        {
            var writer = new StringWriter();
            Format(message, writer, indentationLevel);
            return writer.ToString();
        }

        /// <summary>
        /// Formats the specified message as JSON.
        /// </summary>
        /// <param name="message">The message to format.</param>
        /// <param name="writer">The TextWriter to write the formatted message to.</param>
        /// <remarks>This method delegates to <c>Format(IMessage, TextWriter, int)</c> with <c>indentationLevel = 0</c>.</remarks>
        /// <returns>The formatted message.</returns>
        public void Format(IMessage message, TextWriter writer) => Format(message, writer, indentationLevel: 0);

        /// <summary>
        /// Formats the specified message as JSON. When <see cref="Settings.Indentation"/> is not null, start indenting at the specified <paramref name="indentationLevel"/>.
        /// </summary>
        /// <param name="message">The message to format.</param>
        /// <param name="writer">The TextWriter to write the formatted message to.</param>
        /// <param name="indentationLevel">Indentation level to start at.</param>
        /// <remarks>To keep consistent indentation when embedding a message inside another JSON string, set <paramref name="indentationLevel"/>.</remarks>
        public void Format(IMessage message, TextWriter writer, int indentationLevel)
        {
            ProtoPreconditions.CheckNotNull(message, nameof(message));
            ProtoPreconditions.CheckNotNull(writer, nameof(writer));

            if (message.Descriptor.IsWellKnownType)
            {
                WriteWellKnownTypeValue(writer, message.Descriptor, message, indentationLevel);
            }
            else
            {
                WriteMessage(writer, message, indentationLevel);
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

        private void WriteMessage(TextWriter writer, IMessage message, int indentationLevel)
        {
            if (message == null)
            {
                WriteNull(writer);
                return;
            }
            if (DiagnosticOnly)
            {
                if (message is ICustomDiagnosticMessage customDiagnosticMessage)
                {
                    writer.Write(customDiagnosticMessage.ToDiagnosticString());
                    return;
                }
            }

            WriteBracketOpen(writer, ObjectOpenBracket);
            bool writtenFields = WriteMessageFields(writer, message, false, indentationLevel + 1);
            WriteBracketClose(writer, ObjectCloseBracket, writtenFields, indentationLevel);
        }

        private bool WriteMessageFields(TextWriter writer, IMessage message, bool assumeFirstFieldWritten, int indentationLevel)
        {
            var fields = message.Descriptor.Fields;
            bool first = !assumeFirstFieldWritten;
            // First non-oneof fields
            foreach (var field in fields.InFieldNumberOrder())
            {
                var accessor = field.Accessor;
                var value = accessor.GetValue(message);
                if (!ShouldFormatFieldValue(message, field, value))
                {
                    continue;
                }

                MaybeWriteValueSeparator(writer, first);
                MaybeWriteValueWhitespace(writer, indentationLevel);

                if (settings.PreserveProtoFieldNames)
                {
                    WriteString(writer, accessor.Descriptor.Name);
                }
                else
                {
                    WriteString(writer, accessor.Descriptor.JsonName);
                }
                writer.Write(NameValueSeparator);
                WriteValue(writer, value, indentationLevel);

                first = false;
            }
            return !first;
        }

        private void MaybeWriteValueSeparator(TextWriter writer, bool first)
        {
            if (first)
            {
                return;
            }

            writer.Write(settings.Indentation == null ? ValueSeparator : MultilineValueSeparator);
        }

        /// <summary>
        /// Determines whether or not a field value should be serialized according to the field,
        /// its value in the message, and the settings of this formatter.
        /// </summary>
        private bool ShouldFormatFieldValue(IMessage message, FieldDescriptor field, object value) =>
            field.HasPresence
            // Fields that support presence *just* use that
            ? field.Accessor.HasValue(message)
            // Otherwise, format if either we've been asked to format default values, or if it's
            // not a default value anyway.
            : settings.FormatDefaultValues || !IsDefaultValue(field, value);

        // Converted from java/core/src/main/java/com/google/protobuf/Descriptors.java
        internal static string ToJsonName(string name)
        {
            StringBuilder result = new StringBuilder(name.Length);
            bool isNextUpperCase = false;
            foreach (char ch in name)
            {
                if (ch == '_')
                {
                    isNextUpperCase = true;
                }
                else if (isNextUpperCase)
                {
                    result.Append(char.ToUpperInvariant(ch));
                    isNextUpperCase = false;
                }
                else
                {
                    result.Append(ch);
                }
            }
            return result.ToString();
        }

        internal static string FromJsonName(string name)
        {
            StringBuilder result = new StringBuilder(name.Length);
            foreach (char ch in name)
            {
                if (char.IsUpper(ch))
                {
                    result.Append('_');
                    result.Append(char.ToLowerInvariant(ch));
                }
                else
                {
                    result.Append(ch);
                }
            }
            return result.ToString();
        }

        private static void WriteNull(TextWriter writer)
        {
            writer.Write("null");
        }

        private static bool IsDefaultValue(FieldDescriptor descriptor, object value)
        {
            if (descriptor.IsMap)
            {
                IDictionary dictionary = (IDictionary) value;
                return dictionary.Count == 0;
            }
            if (descriptor.IsRepeated)
            {
                IList list = (IList) value;
                return list.Count == 0;
            }
            return descriptor.FieldType switch
            {
                FieldType.Bool => (bool) value == false,
                FieldType.Bytes => (ByteString) value == ByteString.Empty,
                FieldType.String => (string) value == "",
                FieldType.Double => (double) value == 0.0,
                FieldType.SInt32 or FieldType.Int32 or FieldType.SFixed32 or FieldType.Enum => (int) value == 0,
                FieldType.Fixed32 or FieldType.UInt32 => (uint) value == 0,
                FieldType.Fixed64 or FieldType.UInt64 => (ulong) value == 0,
                FieldType.SFixed64 or FieldType.Int64 or FieldType.SInt64 => (long) value == 0,
                FieldType.Float => (float) value == 0f,
                FieldType.Message or FieldType.Group => value == null,
                _ => throw new ArgumentException("Invalid field type"),
            };
        }

        /// <summary>
        /// Writes a single value to the given writer as JSON. Only types understood by
        /// Protocol Buffers can be written in this way. This method is only exposed for
        /// advanced use cases; most users should be using <see cref="Format(IMessage)"/>
        /// or <see cref="Format(IMessage, TextWriter)"/>.
        /// </summary>
        /// <param name="writer">The writer to write the value to. Must not be null.</param>
        /// <param name="value">The value to write. May be null.</param>
        /// <remarks>Delegates to <c>WriteValue(TextWriter, object, int)</c> with <c>indentationLevel = 0</c>.</remarks>
        public void WriteValue(TextWriter writer, object value) => WriteValue(writer, value, 0);

        /// <summary>
        /// Writes a single value to the given writer as JSON. Only types understood by
        /// Protocol Buffers can be written in this way. This method is only exposed for
        /// advanced use cases; most users should be using <see cref="Format(IMessage)"/>
        /// or <see cref="Format(IMessage, TextWriter)"/>.
        /// </summary>
        /// <param name="writer">The writer to write the value to. Must not be null.</param>
        /// <param name="value">The value to write. May be null.</param>
        /// <param name="indentationLevel">The current indentationLevel. Not used when <see cref="Settings.Indentation"/> is null.</param>
        public void WriteValue(TextWriter writer, object value, int indentationLevel)
        {
            if (value == null || value is NullValue)
            {
                WriteNull(writer);
            }
            else if (value is bool b)
            {
                writer.Write(b ? "true" : "false");
            }
            else if (value is ByteString byteString)
            {
                // Nothing in Base64 needs escaping
                writer.Write('"');
                writer.Write(byteString.ToBase64());
                writer.Write('"');
            }
            else if (value is string str)
            {
                WriteString(writer, str);
            }
            else if (value is IDictionary dictionary)
            {
                WriteDictionary(writer, dictionary, indentationLevel);
            }
            else if (value is IList list)
            {
                WriteList(writer, list, indentationLevel);
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
                if (settings.FormatEnumsAsIntegers)
                {
                    WriteValue(writer, (int)value);
                }
                else
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
            else if (value is IMessage message)
            {
                Format(message, writer, indentationLevel);
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
        private void WriteWellKnownTypeValue(TextWriter writer, MessageDescriptor descriptor, object value, int indentationLevel)
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
                if (value is IMessage message)
                {
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
                WriteStruct(writer, (IMessage)value, indentationLevel);
                return;
            }
            if (descriptor.FullName == ListValue.Descriptor.FullName)
            {
                var fieldAccessor = descriptor.Fields[ListValue.ValuesFieldNumber].Accessor;
                WriteList(writer, (IList)fieldAccessor.GetValue((IMessage)value), indentationLevel);
                return;
            }
            if (descriptor.FullName == Value.Descriptor.FullName)
            {
                WriteStructFieldValue(writer, (IMessage)value, indentationLevel);
                return;
            }
            if (descriptor.FullName == Any.Descriptor.FullName)
            {
                WriteAny(writer, (IMessage)value, indentationLevel);
                return;
            }
            WriteMessage(writer, (IMessage)value, indentationLevel);
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

        private void WriteAny(TextWriter writer, IMessage value, int indentationLevel)
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
            WriteBracketOpen(writer, ObjectOpenBracket);
            WriteString(writer, AnyTypeUrlField);
            writer.Write(NameValueSeparator);
            WriteString(writer, typeUrl);

            if (descriptor.IsWellKnownType)
            {
                writer.Write(ValueSeparator);
                WriteString(writer, AnyWellKnownTypeValueField);
                writer.Write(NameValueSeparator);
                WriteWellKnownTypeValue(writer, descriptor, message, indentationLevel);
            }
            else
            {
                WriteMessageFields(writer, message, true, indentationLevel);
            }
            WriteBracketClose(writer, ObjectCloseBracket, true, indentationLevel);
        }

        private void WriteDiagnosticOnlyAny(TextWriter writer, IMessage value)
        {
            string typeUrl = (string) value.Descriptor.Fields[Any.TypeUrlFieldNumber].Accessor.GetValue(value);
            ByteString data = (ByteString) value.Descriptor.Fields[Any.ValueFieldNumber].Accessor.GetValue(value);
            writer.Write("{ ");
            WriteString(writer, AnyTypeUrlField);
            writer.Write(NameValueSeparator);
            WriteString(writer, typeUrl);
            writer.Write(ValueSeparator);
            WriteString(writer, AnyDiagnosticValueField);
            writer.Write(NameValueSeparator);
            writer.Write('"');
            writer.Write(data.ToBase64());
            writer.Write('"');
            writer.Write(" }");
        }

        private void WriteStruct(TextWriter writer, IMessage message, int indentationLevel)
        {
            WriteBracketOpen(writer, ObjectOpenBracket);
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

                MaybeWriteValueSeparator(writer, first);
                MaybeWriteValueWhitespace(writer, indentationLevel + 1);
                WriteString(writer, key);
                writer.Write(NameValueSeparator);
                WriteStructFieldValue(writer, value, indentationLevel + 1);
                first = false;
            }
            WriteBracketClose(writer, ObjectCloseBracket, !first, indentationLevel);
        }

        private void WriteStructFieldValue(TextWriter writer, IMessage message, int indentationLevel)
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
                    WriteWellKnownTypeValue(writer, nestedMessage.Descriptor, nestedMessage, indentationLevel);
                    return;
                case Value.NullValueFieldNumber:
                    WriteNull(writer);
                    return;
                default:
                    throw new InvalidOperationException("Unexpected case in struct field: " + specifiedField.FieldNumber);
            }
        }

        internal void WriteList(TextWriter writer, IList list, int indentationLevel = 0)
        {
            WriteBracketOpen(writer, ListBracketOpen);

            bool first = true;
            foreach (var value in list)
            {
                MaybeWriteValueSeparator(writer, first);
                MaybeWriteValueWhitespace(writer, indentationLevel + 1);
                WriteValue(writer, value, indentationLevel + 1);
                first = false;
            }

            WriteBracketClose(writer, ListBracketClose, !first, indentationLevel);
        }

        internal void WriteDictionary(TextWriter writer, IDictionary dictionary, int indentationLevel = 0)
        {
            WriteBracketOpen(writer, ObjectOpenBracket);

            bool first = true;
            // This will box each pair. Could use IDictionaryEnumerator, but that's ugly in terms of disposal.
            foreach (DictionaryEntry pair in dictionary)
            {
                string keyText;
                if (pair.Key is string s)
                {
                    keyText = s;
                }
                else if (pair.Key is bool b)
                {
                    keyText = b ? "true" : "false";
                }
                else if (pair.Key is int || pair.Key is uint || pair.Key is long || pair.Key is ulong)
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

                MaybeWriteValueSeparator(writer, first);
                MaybeWriteValueWhitespace(writer, indentationLevel + 1);
                WriteString(writer, keyText);
                writer.Write(NameValueSeparator);
                WriteValue(writer, pair.Value);
                first = false;
            }

            WriteBracketClose(writer, ObjectCloseBracket, !first, indentationLevel);
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

        private void WriteBracketOpen(TextWriter writer, char openChar)
        {
            writer.Write(openChar);
            if (settings.Indentation == null)
            {
                writer.Write(' ');
            }
        }

        private void WriteBracketClose(TextWriter writer, char closeChar, bool hasFields, int indentationLevel)
        {
            if (hasFields)
            {
                if (settings.Indentation != null)
                {
                    writer.WriteLine();
                    WriteIndentation(writer, indentationLevel);
                }
                else
                {
                    writer.Write(" ");
                }
            }

            writer.Write(closeChar);
        }

        private void MaybeWriteValueWhitespace(TextWriter writer, int indentationLevel)
        {
            if (settings.Indentation != null) {
                writer.WriteLine();
                WriteIndentation(writer, indentationLevel);
            }
        }

        private void WriteIndentation(TextWriter writer, int indentationLevel)
        {
            for (int i = 0; i < indentationLevel; i++)
            {
                writer.Write(settings.Indentation);
            }
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
            /// Whether fields which would otherwise not be included in the formatted data
            /// should be formatted even when the value is not present, or has the default value.
            /// This option only affects fields which don't support "presence" (e.g.
            /// singular non-optional proto3 primitive fields).
            /// </summary>
            public bool FormatDefaultValues { get; }

            /// <summary>
            /// The type registry used to format <see cref="Any"/> messages.
            /// </summary>
            public TypeRegistry TypeRegistry { get; }

            /// <summary>
            /// Whether to format enums as ints. Defaults to false.
            /// </summary>
            public bool FormatEnumsAsIntegers { get; }

            /// <summary>
            /// Whether to use the original proto field names as defined in the .proto file. Defaults to false.
            /// </summary>
            public bool PreserveProtoFieldNames { get; }

            /// <summary>
            /// Indentation string, used for formatting. Setting null disables indentation.
            /// </summary>
            public string Indentation { get; }

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
            public Settings(bool formatDefaultValues, TypeRegistry typeRegistry) : this(formatDefaultValues, typeRegistry, false, false)
            {
            }

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified parameters.
            /// </summary>
            /// <param name="formatDefaultValues"><c>true</c> if default values (0, empty strings etc) should be formatted; <c>false</c> otherwise.</param>
            /// <param name="typeRegistry">The <see cref="TypeRegistry"/> to use when formatting <see cref="Any"/> messages. TypeRegistry.Empty will be used if it is null.</param>
            /// <param name="formatEnumsAsIntegers"><c>true</c> to format the enums as integers; <c>false</c> to format enums as enum names.</param>
            /// <param name="preserveProtoFieldNames"><c>true</c> to preserve proto field names; <c>false</c> to convert them to lowerCamelCase.</param>
            /// <param name="indentation">The indentation string to use for multi-line formatting. <c>null</c> to disable multi-line format.</param>
            private Settings(bool formatDefaultValues,
                            TypeRegistry typeRegistry,
                            bool formatEnumsAsIntegers,
                            bool preserveProtoFieldNames,
                            string indentation = null)
            {
                FormatDefaultValues = formatDefaultValues;
                TypeRegistry = typeRegistry ?? TypeRegistry.Empty;
                FormatEnumsAsIntegers = formatEnumsAsIntegers;
                PreserveProtoFieldNames = preserveProtoFieldNames;
                Indentation = indentation;
            }

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified formatting of default values and the current settings.
            /// </summary>
            /// <param name="formatDefaultValues"><c>true</c> if default values (0, empty strings etc) should be formatted; <c>false</c> otherwise.</param>
            public Settings WithFormatDefaultValues(bool formatDefaultValues) => new Settings(formatDefaultValues, TypeRegistry, FormatEnumsAsIntegers, PreserveProtoFieldNames, Indentation);

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified type registry and the current settings.
            /// </summary>
            /// <param name="typeRegistry">The <see cref="TypeRegistry"/> to use when formatting <see cref="Any"/> messages.</param>
            public Settings WithTypeRegistry(TypeRegistry typeRegistry) => new Settings(FormatDefaultValues, typeRegistry, FormatEnumsAsIntegers, PreserveProtoFieldNames, Indentation);

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified enums formatting option and the current settings.
            /// </summary>
            /// <param name="formatEnumsAsIntegers"><c>true</c> to format the enums as integers; <c>false</c> to format enums as enum names.</param>
            public Settings WithFormatEnumsAsIntegers(bool formatEnumsAsIntegers) => new Settings(FormatDefaultValues, TypeRegistry, formatEnumsAsIntegers, PreserveProtoFieldNames, Indentation);

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified field name formatting option and the current settings.
            /// </summary>
            /// <param name="preserveProtoFieldNames"><c>true</c> to preserve proto field names; <c>false</c> to convert them to lowerCamelCase.</param>
            public Settings WithPreserveProtoFieldNames(bool preserveProtoFieldNames) => new Settings(FormatDefaultValues, TypeRegistry, FormatEnumsAsIntegers, preserveProtoFieldNames, Indentation);

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified indentation and the current settings.
            /// </summary>
            /// <param name="indentation">The string to output for each level of indentation (nesting). The default is two spaces per level. Use null to disable indentation entirely.</param>
            /// <remarks>A non-null value for <see cref="Indentation"/> will insert additional line-breaks to the JSON output.
            /// Each line will contain either a single value, or braces. The default line-break is determined by <see cref="Environment.NewLine"/>,
            /// which is <c>"\n"</c> on Unix platforms, and <c>"\r\n"</c> on Windows. If <see cref="JsonFormatter"/> seems to produce empty lines,
            /// you need to pass a <see cref="TextWriter"/> that uses a <c>"\n"</c> newline. See <see cref="JsonFormatter.Format(Google.Protobuf.IMessage, TextWriter)"/>.
            /// </remarks>
            public Settings WithIndentation(string indentation = "  ") => new Settings(FormatDefaultValues, TypeRegistry, FormatEnumsAsIntegers, PreserveProtoFieldNames, indentation);
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

            [UnconditionalSuppressMessage("Trimming", "IL2072",
                Justification = "The field for the value must still be present. It will be returned by reflection, will be in this collection, and its name can be resolved.")]
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

                // If this returns false, originalName will be null, which is what we want.
                nameMapping.TryGetValue(value, out string originalName);
                return originalName;
            }

            private static Dictionary<object, string> GetNameMapping(
                [DynamicallyAccessedMembers(
                    DynamicallyAccessedMemberTypes.PublicFields |
                    DynamicallyAccessedMemberTypes.NonPublicFields)]
                System.Type enumType)
            {
                return enumType.GetTypeInfo().DeclaredFields
                    .Where(f => f.IsStatic)
                    .Where(f => f.GetCustomAttributes<OriginalNameAttribute>()
                                 .FirstOrDefault()?.PreferredAlias ?? true)
                    .ToDictionary(f => f.GetValue(null),
                                  f => f.GetCustomAttributes<OriginalNameAttribute>()
                                        .FirstOrDefault()
                                        // If the attribute hasn't been applied, fall back to the name of the field.
                                        ?.Name ?? f.Name);
            }
        }
    }
}
