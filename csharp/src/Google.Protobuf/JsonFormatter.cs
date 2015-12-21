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
using System.Linq;

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
            Preconditions.CheckNotNull(message, nameof(message));
            StringBuilder builder = new StringBuilder();
            if (message.Descriptor.IsWellKnownType)
            {
                WriteWellKnownTypeValue(builder, message.Descriptor, message, false);
            }
            else
            {
                WriteMessage(builder, message);
            }
            return builder.ToString();
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
            Preconditions.CheckNotNull(message, nameof(message));
            return diagnosticFormatter.Format(message);
        }

        private void WriteMessage(StringBuilder builder, IMessage message)
        {
            if (message == null)
            {
                WriteNull(builder);
                return;
            }
            builder.Append("{ ");
            bool writtenFields = WriteMessageFields(builder, message, false);
            builder.Append(writtenFields ? " }" : "}");
        }

        private bool WriteMessageFields(StringBuilder builder, IMessage message, bool assumeFirstFieldWritten)
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
                // Omit awkward (single) values such as unknown enum values
                if (!field.IsRepeated && !field.IsMap && !CanWriteSingleValue(value))
                {
                    continue;
                }

                // Okay, all tests complete: let's write the field value...
                if (!first)
                {
                    builder.Append(PropertySeparator);
                }
                WriteString(builder, ToCamelCase(accessor.Descriptor.Name));
                builder.Append(NameValueSeparator);
                WriteValue(builder, value);
                first = false;
            }            
            return !first;
        }

        // Converted from src/google/protobuf/util/internal/utility.cc ToCamelCase
        // TODO: Use the new field in FieldDescriptor.
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
                }
                result.Append(input[i]);
            }
            return result.ToString();
        }
        
        private static void WriteNull(StringBuilder builder)
        {
            builder.Append("null");
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
        
        private void WriteValue(StringBuilder builder, object value)
        {
            if (value == null)
            {
                WriteNull(builder);
            }
            else if (value is bool)
            {
                builder.Append((bool) value ? "true" : "false");
            }
            else if (value is ByteString)
            {
                // Nothing in Base64 needs escaping
                builder.Append('"');
                builder.Append(((ByteString) value).ToBase64());
                builder.Append('"');
            }
            else if (value is string)
            {
                WriteString(builder, (string) value);
            }
            else if (value is IDictionary)
            {
                WriteDictionary(builder, (IDictionary) value);
            }
            else if (value is IList)
            {
                WriteList(builder, (IList) value);
            }
            else if (value is int || value is uint)
            {
                IFormattable formattable = (IFormattable) value;
                builder.Append(formattable.ToString("d", CultureInfo.InvariantCulture));
            }
            else if (value is long || value is ulong)
            {
                builder.Append('"');
                IFormattable formattable = (IFormattable) value;
                builder.Append(formattable.ToString("d", CultureInfo.InvariantCulture));
                builder.Append('"');
            }
            else if (value is System.Enum)
            {
                WriteString(builder, value.ToString());
            }
            else if (value is float || value is double)
            {
                string text = ((IFormattable) value).ToString("r", CultureInfo.InvariantCulture);
                if (text == "NaN" || text == "Infinity" || text == "-Infinity")
                {
                    builder.Append('"');
                    builder.Append(text);
                    builder.Append('"');
                }
                else
                {
                    builder.Append(text);
                }
            }
            else if (value is IMessage)
            {
                IMessage message = (IMessage) value;
                if (message.Descriptor.IsWellKnownType)
                {
                    WriteWellKnownTypeValue(builder, message.Descriptor, value, true);
                }
                else
                {
                    WriteMessage(builder, (IMessage) value);
                }
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
        private void WriteWellKnownTypeValue(StringBuilder builder, MessageDescriptor descriptor, object value, bool inField)
        {
            // Currently, we can never actually get here, because null values are always handled by the caller. But if we *could*,
            // this would do the right thing.
            if (value == null)
            {
                WriteNull(builder);
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
                WriteValue(builder, value);
                return;
            }
            if (descriptor.FullName == Timestamp.Descriptor.FullName)
            {
                MaybeWrapInString(builder, value, WriteTimestamp, inField);
                return;
            }
            if (descriptor.FullName == Duration.Descriptor.FullName)
            {
                MaybeWrapInString(builder, value, WriteDuration, inField);
                return;
            }
            if (descriptor.FullName == FieldMask.Descriptor.FullName)
            {
                MaybeWrapInString(builder, value, WriteFieldMask, inField);
                return;
            }
            if (descriptor.FullName == Struct.Descriptor.FullName)
            {
                WriteStruct(builder, (IMessage) value);
                return;
            }
            if (descriptor.FullName == ListValue.Descriptor.FullName)
            {
                var fieldAccessor = descriptor.Fields[ListValue.ValuesFieldNumber].Accessor;
                WriteList(builder, (IList) fieldAccessor.GetValue((IMessage) value));
                return;
            }
            if (descriptor.FullName == Value.Descriptor.FullName)
            {
                WriteStructFieldValue(builder, (IMessage) value);
                return;
            }
            if (descriptor.FullName == Any.Descriptor.FullName)
            {
                WriteAny(builder, (IMessage) value);
                return;
            }
            WriteMessage(builder, (IMessage) value);
        }

        /// <summary>
        /// Some well-known types end up as string values... so they need wrapping in quotes, but only
        /// when they're being used as fields within another message.
        /// </summary>
        private void MaybeWrapInString(StringBuilder builder, object value, Action<StringBuilder, IMessage> action, bool inField)
        {
            if (inField)
            {
                builder.Append('"');
                action(builder, (IMessage) value);
                builder.Append('"');
            }
            else
            {
                action(builder, (IMessage) value);
            }
        }

        private void WriteTimestamp(StringBuilder builder, IMessage value)
        {
            // TODO: In the common case where this *is* using the built-in Timestamp type, we could
            // avoid all the reflection at this point, by casting to Timestamp. In the interests of
            // avoiding subtle bugs, don't do that until we've implemented DynamicMessage so that we can prove
            // it still works in that case.
            int nanos = (int) value.Descriptor.Fields[Timestamp.NanosFieldNumber].Accessor.GetValue(value);
            long seconds = (long) value.Descriptor.Fields[Timestamp.SecondsFieldNumber].Accessor.GetValue(value);

            // Even if the original message isn't using the built-in classes, we can still build one... and then
            // rely on it being normalized.
            Timestamp normalized = Timestamp.Normalize(seconds, nanos);
            // Use .NET's formatting for the value down to the second, including an opening double quote (as it's a string value)
            DateTime dateTime = normalized.ToDateTime();
            builder.Append(dateTime.ToString("yyyy'-'MM'-'dd'T'HH:mm:ss", CultureInfo.InvariantCulture));
            AppendNanoseconds(builder, Math.Abs(normalized.Nanos));
            builder.Append('Z');
        }

        private void WriteDuration(StringBuilder builder, IMessage value)
        {
            // TODO: Same as for WriteTimestamp
            int nanos = (int) value.Descriptor.Fields[Duration.NanosFieldNumber].Accessor.GetValue(value);
            long seconds = (long) value.Descriptor.Fields[Duration.SecondsFieldNumber].Accessor.GetValue(value);

            // Even if the original message isn't using the built-in classes, we can still build one... and then
            // rely on it being normalized.
            Duration normalized = Duration.Normalize(seconds, nanos);

            // The seconds part will normally provide the minus sign if we need it, but not if it's 0...
            if (normalized.Seconds == 0 && normalized.Nanos < 0)
            {
                builder.Append('-');
            }

            builder.Append(normalized.Seconds.ToString("d", CultureInfo.InvariantCulture));
            AppendNanoseconds(builder, Math.Abs(normalized.Nanos));
            builder.Append('s');
        }

        private void WriteFieldMask(StringBuilder builder, IMessage value)
        {
            IList paths = (IList) value.Descriptor.Fields[FieldMask.PathsFieldNumber].Accessor.GetValue(value);
            AppendEscapedString(builder, string.Join(",", paths.Cast<string>().Select(ToCamelCase)));
        }

        private void WriteAny(StringBuilder builder, IMessage value)
        {
            if (ReferenceEquals(this, diagnosticFormatter))
            {
                WriteDiagnosticOnlyAny(builder, value);
                return;
            }

            string typeUrl = (string) value.Descriptor.Fields[Any.TypeUrlFieldNumber].Accessor.GetValue(value);
            ByteString data = (ByteString) value.Descriptor.Fields[Any.ValueFieldNumber].Accessor.GetValue(value);
            string typeName = GetTypeName(typeUrl);
            MessageDescriptor descriptor = settings.TypeRegistry.Find(typeName);
            if (descriptor == null)
            {
                throw new InvalidOperationException($"Type registry has no descriptor for type name '{typeName}'");
            }
            IMessage message = descriptor.Parser.ParseFrom(data);
            builder.Append("{ ");
            WriteString(builder, AnyTypeUrlField);
            builder.Append(NameValueSeparator);
            WriteString(builder, typeUrl);

            if (descriptor.IsWellKnownType)
            {
                builder.Append(PropertySeparator);
                WriteString(builder, AnyWellKnownTypeValueField);
                builder.Append(NameValueSeparator);
                WriteWellKnownTypeValue(builder, descriptor, message, true);
            }
            else
            {
                WriteMessageFields(builder, message, true);
            }
            builder.Append(" }");
        }

        private void WriteDiagnosticOnlyAny(StringBuilder builder, IMessage value)
        {
            string typeUrl = (string) value.Descriptor.Fields[Any.TypeUrlFieldNumber].Accessor.GetValue(value);
            ByteString data = (ByteString) value.Descriptor.Fields[Any.ValueFieldNumber].Accessor.GetValue(value);
            builder.Append("{ ");
            WriteString(builder, AnyTypeUrlField);
            builder.Append(NameValueSeparator);
            WriteString(builder, typeUrl);
            builder.Append(PropertySeparator);
            WriteString(builder, AnyDiagnosticValueField);
            builder.Append(NameValueSeparator);
            builder.Append('"');
            builder.Append(data.ToBase64());
            builder.Append('"');
            builder.Append(" }");
        }

        internal static string GetTypeName(String typeUrl)
        {
            string[] parts = typeUrl.Split('/');
            if (parts.Length != 2 || parts[0] != TypeUrlPrefix)
            {
                throw new InvalidProtocolBufferException($"Invalid type url: {typeUrl}");
            }
            return parts[1];
        }

        /// <summary>
        /// Appends a number of nanoseconds to a StringBuilder. Either 0 digits are added (in which
        /// case no "." is appended), or 3 6 or 9 digits.
        /// </summary>
        private static void AppendNanoseconds(StringBuilder builder, int nanos)
        {
            if (nanos != 0)
            {
                builder.Append('.');
                // Output to 3, 6 or 9 digits.
                if (nanos % 1000000 == 0)
                {
                    builder.Append((nanos / 1000000).ToString("d", CultureInfo.InvariantCulture));
                }
                else if (nanos % 1000 == 0)
                {
                    builder.Append((nanos / 1000).ToString("d", CultureInfo.InvariantCulture));
                }
                else
                {
                    builder.Append(nanos.ToString("d", CultureInfo.InvariantCulture));
                }
            }
        }

        private void WriteStruct(StringBuilder builder, IMessage message)
        {
            builder.Append("{ ");
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
                    builder.Append(PropertySeparator);
                }
                WriteString(builder, key);
                builder.Append(NameValueSeparator);
                WriteStructFieldValue(builder, value);
                first = false;
            }
            builder.Append(first ? "}" : " }");
        }

        private void WriteStructFieldValue(StringBuilder builder, IMessage message)
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
                    WriteValue(builder, value);
                    return;
                case Value.StructValueFieldNumber:
                case Value.ListValueFieldNumber:
                    // Structs and ListValues are nested messages, and already well-known types.
                    var nestedMessage = (IMessage) specifiedField.Accessor.GetValue(message);
                    WriteWellKnownTypeValue(builder, nestedMessage.Descriptor, nestedMessage, true);
                    return;
                case Value.NullValueFieldNumber:
                    WriteNull(builder);
                    return;
                default:
                    throw new InvalidOperationException("Unexpected case in struct field: " + specifiedField.FieldNumber);
            }
        }

        internal void WriteList(StringBuilder builder, IList list)
        {
            builder.Append("[ ");
            bool first = true;
            foreach (var value in list)
            {
                if (!CanWriteSingleValue(value))
                {
                    continue;
                }
                if (!first)
                {
                    builder.Append(PropertySeparator);
                }
                WriteValue(builder, value);
                first = false;
            }
            builder.Append(first ? "]" : " ]");
        }

        internal void WriteDictionary(StringBuilder builder, IDictionary dictionary)
        {
            builder.Append("{ ");
            bool first = true;
            // This will box each pair. Could use IDictionaryEnumerator, but that's ugly in terms of disposal.
            foreach (DictionaryEntry pair in dictionary)
            {
                if (!CanWriteSingleValue(pair.Value))
                {
                    continue;
                }
                if (!first)
                {
                    builder.Append(PropertySeparator);
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
                WriteString(builder, keyText);
                builder.Append(NameValueSeparator);
                WriteValue(builder, pair.Value);
                first = false;
            }
            builder.Append(first ? "}" : " }");
        }

        /// <summary>
        /// Returns whether or not a singular value can be represented in JSON.
        /// Currently only relevant for enums, where unknown values can't be represented.
        /// For repeated/map fields, this always returns true.
        /// </summary>
        private bool CanWriteSingleValue(object value)
        {
            if (value is System.Enum)
            {
                return System.Enum.IsDefined(value.GetType(), value);
            }
            return true;
        }

        /// <summary>
        /// Writes a string (including leading and trailing double quotes) to a builder, escaping as required.
        /// </summary>
        /// <remarks>
        /// Other than surrogate pair handling, this code is mostly taken from src/google/protobuf/util/internal/json_escaping.cc.
        /// </remarks>
        private void WriteString(StringBuilder builder, string text)
        {
            builder.Append('"');
            AppendEscapedString(builder, text);
            builder.Append('"');
        }

        /// <summary>
        /// Appends the given text to the string builder, escaping as required.
        /// </summary>
        private void AppendEscapedString(StringBuilder builder, string text)
        {
            for (int i = 0; i < text.Length; i++)
            {
                char c = text[i];
                if (c < 0xa0)
                {
                    builder.Append(CommonRepresentations[c]);
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
                    HexEncodeUtf16CodeUnit(builder, c);
                    HexEncodeUtf16CodeUnit(builder, text[i]);
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
                        HexEncodeUtf16CodeUnit(builder, c);
                        break;

                    default:
                        if ((c >= 0x0600 && c <= 0x0603) ||  // Arabic signs
                            (c >= 0x200b && c <= 0x200f) ||  // Zero width etc.
                            (c >= 0x2028 && c <= 0x202e) ||  // Separators etc.
                            (c >= 0x2060 && c <= 0x2064) ||  // Invisible etc.
                            (c >= 0x206a && c <= 0x206f))
                        {
                            HexEncodeUtf16CodeUnit(builder, c);
                        }
                        else
                        {
                            // No handling of surrogates here - that's done earlier
                            builder.Append(c);
                        }
                        break;
                }
            }
        }

        private const string Hex = "0123456789abcdef";
        private static void HexEncodeUtf16CodeUnit(StringBuilder builder, char c)
        {
            builder.Append("\\u");
            builder.Append(Hex[(c >> 12) & 0xf]);
            builder.Append(Hex[(c >> 8) & 0xf]);
            builder.Append(Hex[(c >> 4) & 0xf]);
            builder.Append(Hex[(c >> 0) & 0xf]);
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
                TypeRegistry = Preconditions.CheckNotNull(typeRegistry, nameof(typeRegistry));
            }
        }
    }
}
