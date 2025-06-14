// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.MessageReflection.MergeTarget;
import java.io.IOException;
import java.math.BigInteger;
import java.nio.CharBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.logging.Logger;
import java.util.regex.Pattern;

/**
 * Provide text parsing and formatting support for proto2 instances. The implementation largely
 * follows text_format.cc.
 *
 * @author wenboz@google.com Wenbo Zhu
 * @author kenton@google.com Kenton Varda
 */
public final class TextFormat {
  private TextFormat() {}

  private static final Logger logger = Logger.getLogger(TextFormat.class.getName());

  private static final String DEBUG_STRING_SILENT_MARKER = " \t ";
  private static final String ENABLE_INSERT_SILENT_MARKER_ENV_NAME =
      "SILENT_MARKER_INSERTION_ENABLED";
  private static final boolean ENABLE_INSERT_SILENT_MARKER =
      System.getenv().getOrDefault(ENABLE_INSERT_SILENT_MARKER_ENV_NAME, "true").equals("true");

  private static final String REDACTED_MARKER = "[REDACTED]";

  /**
   * Generates a human readable form of this message, useful for debugging and other purposes, with
   * no newline characters. This is just a trivial wrapper around {@link
   * TextFormat.Printer#shortDebugString(MessageOrBuilder)}.
   *
   * @deprecated Use {@code printer().emittingSingleLine(true).printToString(MessageOrBuilder)}
   */
  @Deprecated
  public static String shortDebugString(final MessageOrBuilder message) {
    return printer()
        .emittingSingleLine(true)
        .printToString(message, Printer.FieldReporterLevel.SHORT_DEBUG_STRING);
  }

  /**
   * Outputs a textual representation of the value of an unknown field.
   *
   * @param tag the field's tag number
   * @param value the value of the field
   * @param output the output to which to append the formatted value
   * @throws ClassCastException if the value is not appropriate for the given field descriptor
   * @throws IOException if there is an exception writing to the output
   */
  public static void printUnknownFieldValue(
      final int tag, final Object value, final Appendable output) throws IOException {
    printUnknownFieldValue(tag, value, setSingleLineOutput(output, false), false);
  }

  private static void printUnknownFieldValue(
      final int tag, final Object value, final TextGenerator generator, boolean redact)
      throws IOException {
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        generator.print(unsignedToString((Long) value));
        break;
      case WireFormat.WIRETYPE_FIXED32:
        generator.print(String.format((Locale) null, "0x%08x", (Integer) value));
        break;
      case WireFormat.WIRETYPE_FIXED64:
        generator.print(String.format((Locale) null, "0x%016x", (Long) value));
        break;
      case WireFormat.WIRETYPE_LENGTH_DELIMITED:
        try {
          // Try to parse and print the field as an embedded message
          UnknownFieldSet message = UnknownFieldSet.parseFrom((ByteString) value);
          generator.print("{");
          generator.eol();
          generator.indent();
          Printer.printUnknownFields(message, generator, redact);
          generator.outdent();
          generator.print("}");
        } catch (InvalidProtocolBufferException e) {
          // If not parseable as a message, print as a String
          generator.print("\"");
          generator.print(escapeBytes((ByteString) value));
          generator.print("\"");
        }
        break;
      case WireFormat.WIRETYPE_START_GROUP:
        Printer.printUnknownFields((UnknownFieldSet) value, generator, redact);
        break;
      default:
        throw new IllegalArgumentException("Bad tag: " + tag);
    }
  }

  /** Printer instance which escapes non-ASCII characters. */
  public static Printer printer() {
    return Printer.DEFAULT_TEXT_FORMAT;
  }

  /** Printer instance which escapes non-ASCII characters and prints in the debug format. */
  public static Printer debugFormatPrinter() {
    return Printer.DEFAULT_DEBUG_FORMAT;
  }

  /** Printer instance which escapes non-ASCII characters and prints in the debug format. */
  public static Printer defaultFormatPrinter() {
    return Printer.DEFAULT_FORMAT;
  }

  /** Helper class for converting protobufs to text. */
  public static final class Printer {

    // Printer instance which escapes non-ASCII characters and prints in the text format.
    private static final Printer DEFAULT_TEXT_FORMAT =
        new Printer(
            /* escapeNonAscii= */ true,
            /* useShortRepeatedPrimitives= */ false,
            TypeRegistry.getEmptyTypeRegistry(),
            ExtensionRegistryLite.getEmptyRegistry(),
            /* enablingSafeDebugFormat= */ false,
            /* singleLine= */ false);

    // Printer instance which escapes non-ASCII characters and prints in the debug format.
    private static final Printer DEFAULT_DEBUG_FORMAT =
        new Printer(
            /* escapeNonAscii= */ true,
            /* useShortRepeatedPrimitives= */ false,
            TypeRegistry.getEmptyTypeRegistry(),
            ExtensionRegistryLite.getEmptyRegistry(),
            /* enablingSafeDebugFormat= */ true,
            /* singleLine= */ false);

    // Printer instance which escapes non-ASCII characters and inserts a silent marker.
    private static final Printer DEFAULT_FORMAT =
        new Printer(
                /* escapeNonAscii= */ true,
                /* useShortRepeatedPrimitives= */ false,
                TypeRegistry.getEmptyTypeRegistry(),
                ExtensionRegistryLite.getEmptyRegistry(),
                /* enablingSafeDebugFormat= */ false,
                /* singleLine= */ false)
            .setInsertSilentMarker(ENABLE_INSERT_SILENT_MARKER);

    static Printer getOutputModePrinter() {
      if (ProtobufToStringOutput.isDefaultFormat()) {
        return defaultFormatPrinter();
      } else if (ProtobufToStringOutput.shouldOutputDebugFormat()) {
        return debugFormatPrinter();
      } else {
        return printer();
      }
    }

    /**
     * A list of the public APIs that output human-readable text from a message. A higher-level API
     * must be larger than any lower-level APIs it calls under the hood, e.g
     * DEBUG_MULTILINE.compareTo(PRINTER_PRINT_TO_STRING) > 0. The inverse is not necessarily true.
     */
    static enum FieldReporterLevel {
      REPORT_ALL(0),
      TEXT_GENERATOR(1),
      PRINT(2),
      PRINTER_PRINT_TO_STRING(3),
      TEXTFORMAT_PRINT_TO_STRING(4),
      PRINT_UNICODE(5),
      SHORT_DEBUG_STRING(6),
      LEGACY_MULTILINE(7),
      LEGACY_SINGLE_LINE(8),
      DEBUG_MULTILINE(9),
      DEBUG_SINGLE_LINE(10),
      ABSTRACT_TO_STRING(11),
      ABSTRACT_BUILDER_TO_STRING(12),
      ABSTRACT_MUTABLE_TO_STRING(13),
      REPORT_NONE(14);
      private final int index;

      FieldReporterLevel(int index) {
        this.index = index;
      }
    }

    /** Whether to escape non ASCII characters with backslash and octal. */
    private final boolean escapeNonAscii;

    /** Whether to print repeated primitive fields using short square bracket notation. */
    private final boolean useShortRepeatedPrimitives;

    private final TypeRegistry typeRegistry;
    private final ExtensionRegistryLite extensionRegistry;

    /**
     * Whether to enable redaction of sensitive fields and introduce randomization. Note that when
     * this is enabled, the output will no longer be deserializable.
     */
    private final boolean enablingSafeDebugFormat;

    private final boolean singleLine;

    private boolean insertSilentMarker;

    @CanIgnoreReturnValue
    private Printer setInsertSilentMarker(boolean insertSilentMarker) {
      this.insertSilentMarker = insertSilentMarker;
      return this;
    }

    // Any API level equal to or greater than this level will be reported. This is set to
    // REPORT_NONE by default to prevent reporting for now.
    private static final ThreadLocal<FieldReporterLevel> sensitiveFieldReportingLevel =
        new ThreadLocal<FieldReporterLevel>() {
          @Override
          protected FieldReporterLevel initialValue() {
            return FieldReporterLevel.ABSTRACT_TO_STRING;
          }
        };

    private Printer(
        boolean escapeNonAscii,
        boolean useShortRepeatedPrimitives,
        TypeRegistry typeRegistry,
        ExtensionRegistryLite extensionRegistry,
        boolean enablingSafeDebugFormat,
        boolean singleLine) {
      this.escapeNonAscii = escapeNonAscii;
      this.useShortRepeatedPrimitives = useShortRepeatedPrimitives;
      this.typeRegistry = typeRegistry;
      this.extensionRegistry = extensionRegistry;
      this.enablingSafeDebugFormat = enablingSafeDebugFormat;
      this.singleLine = singleLine;
      this.insertSilentMarker = false;
    }

    /**
     * Return a new Printer instance with the specified escape mode.
     *
     * @param escapeNonAscii If true, the new Printer will escape non-ASCII characters (this is the
     *     default behavior. If false, the new Printer will print non-ASCII characters as is. In
     *     either case, the new Printer still escapes newlines and quotes in strings.
     * @return a new Printer that clones all other configurations from the current {@link Printer},
     *     with the escape mode set to the given parameter.
     */
    public Printer escapingNonAscii(boolean escapeNonAscii) {
      return new Printer(
          escapeNonAscii,
          useShortRepeatedPrimitives,
          typeRegistry,
          extensionRegistry,
          enablingSafeDebugFormat,
          singleLine);
    }

    /**
     * Creates a new {@link Printer} using the given typeRegistry. The new Printer clones all other
     * configurations from the current {@link Printer}.
     *
     * @throws IllegalArgumentException if a registry is already set.
     */
    public Printer usingTypeRegistry(TypeRegistry typeRegistry) {
      if (this.typeRegistry != TypeRegistry.getEmptyTypeRegistry()) {
        throw new IllegalArgumentException("Only one typeRegistry is allowed.");
      }
      return new Printer(
          escapeNonAscii,
          useShortRepeatedPrimitives,
          typeRegistry,
          extensionRegistry,
          enablingSafeDebugFormat,
          singleLine);
    }

    /**
     * Creates a new {@link Printer} using the given extensionRegistry. The new Printer clones all
     * other configurations from the current {@link Printer}.
     *
     * @throws IllegalArgumentException if a registry is already set.
     */
    public Printer usingExtensionRegistry(ExtensionRegistryLite extensionRegistry) {
      if (this.extensionRegistry != ExtensionRegistryLite.getEmptyRegistry()) {
        throw new IllegalArgumentException("Only one extensionRegistry is allowed.");
      }
      return new Printer(
          escapeNonAscii,
          useShortRepeatedPrimitives,
          typeRegistry,
          extensionRegistry,
          enablingSafeDebugFormat,
          singleLine);
    }

    /**
     * Return a new Printer instance that outputs a redacted and unstable format suitable for
     * debugging.
     *
     * @param enablingSafeDebugFormat If true, the new Printer will redact all proto fields that are
     *     marked by a debug_redact=true option, and apply an unstable prefix to the output.
     * @return a new Printer that clones all other configurations from the current {@link Printer},
     *     with the enablingSafeDebugFormat mode set to the given parameter.
     */
    Printer enablingSafeDebugFormat(boolean enablingSafeDebugFormat) {
      return new Printer(
          escapeNonAscii,
          useShortRepeatedPrimitives,
          typeRegistry,
          extensionRegistry,
          enablingSafeDebugFormat,
          singleLine);
    }

    /**
     * Return a new Printer instance that outputs primitive repeated fields in short notation
     *
     * @param useShortRepeatedPrimitives If true, repeated fields with a primitive type are printed
     *     using the short hand notation with comma-delimited field values in square brackets.
     * @return a new Printer that clones all other configurations from the current {@link Printer},
     *     with the useShortRepeatedPrimitives mode set to the given parameter.
     */
    public Printer usingShortRepeatedPrimitives(boolean useShortRepeatedPrimitives) {
      return new Printer(
          escapeNonAscii,
          useShortRepeatedPrimitives,
          typeRegistry,
          extensionRegistry,
          enablingSafeDebugFormat,
          singleLine);
    }

    /**
     * Return a new Printer instance with the specified line formatting status.
     *
     * @param singleLine If true, the new Printer will output no newline characters.
     * @return a new Printer that clones all other configurations from the current {@link Printer},
     *     with the singleLine mode set to the given parameter.
     */
    public Printer emittingSingleLine(boolean singleLine) {
      return new Printer(
          escapeNonAscii,
          useShortRepeatedPrimitives,
          typeRegistry,
          extensionRegistry,
          enablingSafeDebugFormat,
          singleLine);
    }

    void setSensitiveFieldReportingLevel(FieldReporterLevel level) {
      Printer.sensitiveFieldReportingLevel.set(level);
    }

    /**
     * Outputs a textual representation of the Protocol Message supplied into the parameter output.
     * (This representation is the new version of the classic "ProtocolPrinter" output from the
     * original Protocol Buffer system)
     */
    public void print(final MessageOrBuilder message, final Appendable output) throws IOException {
      print(message, output, FieldReporterLevel.PRINT);
    }

    void print(final MessageOrBuilder message, final Appendable output, FieldReporterLevel level)
        throws IOException {
      TextGenerator generator =
          setSingleLineOutput(
              output,
              this.singleLine,
              message.getDescriptorForType(),
              level,
              this.insertSilentMarker);
      print(message, generator);
    }

    /** Outputs a textual representation of {@code fields} to {@code output}. */
    public void print(final UnknownFieldSet fields, final Appendable output) throws IOException {
      printUnknownFields(
          fields, setSingleLineOutput(output, this.singleLine), this.enablingSafeDebugFormat);
    }

    private void print(final MessageOrBuilder message, final TextGenerator generator)
        throws IOException {
      if (message.getDescriptorForType().getFullName().equals("google.protobuf.Any")
          && printAny(message, generator)) {
        return;
      }
      printMessage(message, generator);
    }

    private void applyUnstablePrefix(final Appendable output) {
      try {
        output.append("");
      } catch (IOException e) {
        throw new IllegalStateException(e);
      }
    }

    /**
     * Attempt to print the 'google.protobuf.Any' message in a human-friendly format. Returns false
     * if the message isn't a valid 'google.protobuf.Any' message (in which case the message should
     * be rendered just like a regular message to help debugging).
     */
    private boolean printAny(final MessageOrBuilder message, final TextGenerator generator)
        throws IOException {
      Descriptor messageType = message.getDescriptorForType();
      FieldDescriptor typeUrlField = messageType.findFieldByNumber(1);
      FieldDescriptor valueField = messageType.findFieldByNumber(2);
      if (typeUrlField == null
          || typeUrlField.getType() != FieldDescriptor.Type.STRING
          || valueField == null
          || valueField.getType() != FieldDescriptor.Type.BYTES) {
        // The message may look like an Any but isn't actually an Any message (might happen if the
        // user tries to use DynamicMessage to construct an Any from incomplete Descriptor).
        return false;
      }
      String typeUrl = (String) message.getField(typeUrlField);
      // If type_url is not set, we will not be able to decode the content of the value, so just
      // print out the Any like a regular message.
      if (typeUrl.isEmpty()) {
        return false;
      }
      Object value = message.getField(valueField);

      Message.Builder contentBuilder = null;
      try {
        Descriptor contentType = typeRegistry.getDescriptorForTypeUrl(typeUrl);
        if (contentType == null) {
          return false;
        }
        contentBuilder = DynamicMessage.getDefaultInstance(contentType).newBuilderForType();
        contentBuilder.mergeFrom((ByteString) value, extensionRegistry);
      } catch (InvalidProtocolBufferException e) {
        // The value of Any is malformed. We cannot print it out nicely, so fallback to printing out
        // the type_url and value as bytes. Note that we fail open here to be consistent with
        // text_format.cc, and also to allow a way for users to inspect the content of the broken
        // message.
        return false;
      }
      generator.print("[");
      generator.print(typeUrl);
      generator.print("]");
      generator.maybePrintSilentMarker();
      generator.print("{");
      generator.eol();
      generator.indent();
      print(contentBuilder, generator);
      generator.outdent();
      generator.print("}");
      generator.eol();
      return true;
    }

    public String printFieldToString(final FieldDescriptor field, final Object value) {
      try {
        final StringBuilder text = new StringBuilder();
        if (enablingSafeDebugFormat) {
          applyUnstablePrefix(text);
        }
        printField(field, value, text);
        return text.toString();
      } catch (IOException e) {
        throw new IllegalStateException(e);
      }
    }

    public void printField(final FieldDescriptor field, final Object value, final Appendable output)
        throws IOException {
      printField(field, value, setSingleLineOutput(output, this.singleLine));
    }

    private void printField(
        final FieldDescriptor field, final Object value, final TextGenerator generator)
        throws IOException {
      // Sort map field entries by key
      if (field.isMapField()) {
        List<MapEntryAdapter> adapters = new ArrayList<>();
        for (Object entry : (List<?>) value) {
          adapters.add(new MapEntryAdapter(entry, field));
        }
        Collections.sort(adapters);
        for (MapEntryAdapter adapter : adapters) {
          printSingleField(field, adapter.getEntry(), generator);
        }
      } else if (field.isRepeated()) {
        if (useShortRepeatedPrimitives && field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
          printShortRepeatedField(field, value, generator);
        } else {
          for (Object element : (List<?>) value) {
            printSingleField(field, element, generator);
          }
        }
      } else {
        printSingleField(field, value, generator);
      }
    }

    /** An adapter class that can take a {@link MapEntry} and returns its key and entry. */
    static class MapEntryAdapter implements Comparable<MapEntryAdapter> {
      private Object entry;
      private Message messageEntry;
      private final FieldDescriptor keyField;

      MapEntryAdapter(Object entry, FieldDescriptor fieldDescriptor) {
        if (entry instanceof Message) {
          this.messageEntry = (Message) entry;
        } else {
          this.entry = entry;
        }
        this.keyField = fieldDescriptor.getMessageType().findFieldByName("key");
      }

      Object getKey() {
        if (messageEntry != null && keyField != null) {
          return messageEntry.getField(keyField);
        }
        return null;
      }

      Object getEntry() {
        if (messageEntry != null) {
          return messageEntry;
        }
        return entry;
      }

      @Override
      public int compareTo(MapEntryAdapter b) {
        Object aKey = getKey();
        Object bKey = b.getKey();
        if (aKey == null && bKey == null) {
          return 0;
        } else if (aKey == null) {
          return -1;
        } else if (bKey == null) {
          return 1;
        } else {
          switch (keyField.getJavaType()) {
            case BOOLEAN:
              return ((Boolean) aKey).compareTo((Boolean) bKey);
            case LONG:
              return ((Long) aKey).compareTo((Long) bKey);
            case INT:
              return ((Integer) aKey).compareTo((Integer) bKey);
            case STRING:
              return ((String) aKey).compareTo((String) bKey);
            default:
              return 0;
          }
        }
      }
    }

    /**
     * Outputs a textual representation of the value of given field value.
     *
     * @param field the descriptor of the field
     * @param value the value of the field
     * @param output the output to which to append the formatted value
     * @throws ClassCastException if the value is not appropriate for the given field descriptor
     * @throws IOException if there is an exception writing to the output
     */
    public void printFieldValue(
        final FieldDescriptor field, final Object value, final Appendable output)
        throws IOException {
      printFieldValue(field, value, setSingleLineOutput(output, this.singleLine));
    }

    private void printFieldValue(
        final FieldDescriptor field, final Object value, final TextGenerator generator)
        throws IOException {
      if (shouldRedact(field, generator)) {
        generator.print(REDACTED_MARKER);
        if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          generator.eol();
        }
        return;
      }
      switch (field.getType()) {
        case INT32:
        case SINT32:
        case SFIXED32:
          generator.print(((Integer) value).toString());
          break;

        case INT64:
        case SINT64:
        case SFIXED64:
          generator.print(((Long) value).toString());
          break;

        case BOOL:
          generator.print(((Boolean) value).toString());
          break;

        case FLOAT:
          generator.print(((Float) value).toString());
          break;

        case DOUBLE:
          generator.print(((Double) value).toString());
          break;

        case UINT32:
        case FIXED32:
          generator.print(unsignedToString((Integer) value));
          break;

        case UINT64:
        case FIXED64:
          generator.print(unsignedToString((Long) value));
          break;

        case STRING:
          generator.print("\"");
          generator.print(
              escapeNonAscii
                  ? TextFormatEscaper.escapeText((String) value)
                  : escapeDoubleQuotesAndBackslashes((String) value).replace("\n", "\\n"));
          generator.print("\"");
          break;

        case BYTES:
          generator.print("\"");
          if (value instanceof ByteString) {
            generator.print(escapeBytes((ByteString) value));
          } else {
            generator.print(escapeBytes((byte[]) value));
          }
          generator.print("\"");
          break;

        case ENUM:
          if (((EnumValueDescriptor) value).getIndex() == -1) {
            // Unknown enum value, print the number instead of the name.
            generator.print(Integer.toString(((EnumValueDescriptor) value).getNumber()));
          } else {
            generator.print(((EnumValueDescriptor) value).getName());
          }
          break;

        case MESSAGE:
        case GROUP:
          print((MessageOrBuilder) value, generator);
          break;
      }
    }

    // The criteria for redacting a field is as follows: 1) The enablingSafeDebugFormat printer
    // option must be on. 2) The field must be considered "sensitive". A sensitive field can be
    // marked as sensitive via two methods: a) via a direct debug_redact=true annotation on the
    // field, b) via an enum field marked with debug_redact=true that is within the proto's
    // FieldOptions, either directly or indirectly via a message option.
    private boolean shouldRedact(final FieldDescriptor field, TextGenerator generator) {
      return enablingSafeDebugFormat && field.isSensitive();
    }

    /** Like {@code print()}, but writes directly to a {@code String} and returns it. */
    public String printToString(final MessageOrBuilder message) {
      return printToString(message, FieldReporterLevel.PRINTER_PRINT_TO_STRING);
    }

    String printToString(final MessageOrBuilder message, FieldReporterLevel level) {
      try {
        final StringBuilder text = new StringBuilder();
        if (enablingSafeDebugFormat) {
          applyUnstablePrefix(text);
        }
        print(message, text, level);
        return text.toString();
      } catch (IOException e) {
        throw new IllegalStateException(e);
      }
    }

    /** Like {@code print()}, but writes directly to a {@code String} and returns it. */
    public String printToString(final UnknownFieldSet fields) {
      try {
        final StringBuilder text = new StringBuilder();
        if (enablingSafeDebugFormat) {
          applyUnstablePrefix(text);
        }
        print(fields, text);
        return text.toString();
      } catch (IOException e) {
        throw new IllegalStateException(e);
      }
    }

    /**
     * Generates a human readable form of this message, useful for debugging and other purposes,
     * with no newline characters.
     *
     * @deprecated Use {@code this.emittingSingleLine(true).printToString(MessageOrBuilder)}
     */
    @Deprecated
    public String shortDebugString(final MessageOrBuilder message) {
      return this.emittingSingleLine(true)
          .printToString(message, FieldReporterLevel.SHORT_DEBUG_STRING);
    }

    /**
     * Generates a human readable form of the field, useful for debugging and other purposes, with
     * no newline characters.
     *
     * @deprecated Use {@code this.emittingSingleLine(true).printFieldToString(FieldDescriptor,
     *     Object)}
     */
    @Deprecated
    @InlineMe(replacement = "this.emittingSingleLine(true).printFieldToString(field, value)")
    public String shortDebugString(final FieldDescriptor field, final Object value) {
      return this.emittingSingleLine(true).printFieldToString(field, value);
    }

    /**
     * Generates a human readable form of the unknown fields, useful for debugging and other
     * purposes, with no newline characters.
     *
     * @deprecated Use {@code this.emittingSingleLine(true).printToString(UnknownFieldSet)}
     */
    @Deprecated
    @InlineMe(replacement = "this.emittingSingleLine(true).printToString(fields)")
    public String shortDebugString(final UnknownFieldSet fields) {
      return this.emittingSingleLine(true).printToString(fields);
    }

    private static void printUnknownFieldValue(
        final int tag, final Object value, final TextGenerator generator, boolean redact)
        throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          generator.print(
              redact
                  ? String.format("UNKNOWN_VARINT %s", REDACTED_MARKER)
                  : unsignedToString((Long) value));
          break;
        case WireFormat.WIRETYPE_FIXED32:
          generator.print(
              redact
                  ? String.format("UNKNOWN_FIXED32 %s", REDACTED_MARKER)
                  : String.format((Locale) null, "0x%08x", (Integer) value));
          break;
        case WireFormat.WIRETYPE_FIXED64:
          generator.print(
              redact
                  ? String.format("UNKNOWN_FIXED64 %s", REDACTED_MARKER)
                  : String.format((Locale) null, "0x%016x", (Long) value));
          break;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          try {
            // Try to parse and print the field as an embedded message
            UnknownFieldSet message = UnknownFieldSet.parseFrom((ByteString) value);
            generator.print("{");
            generator.eol();
            generator.indent();
            printUnknownFields(message, generator, redact);
            generator.outdent();
            generator.print("}");
          } catch (InvalidProtocolBufferException e) {
            // If not parseable as a message, print as a String
            if (redact) {
              generator.print(String.format("UNKNOWN_STRING %s", REDACTED_MARKER));
              break;
            }
            generator.print("\"");
            generator.print(escapeBytes((ByteString) value));
            generator.print("\"");
          }
          break;
        case WireFormat.WIRETYPE_START_GROUP:
          printUnknownFields((UnknownFieldSet) value, generator, redact);
          break;
        default:
          throw new IllegalArgumentException("Bad tag: " + tag);
      }
    }

    private void printMessage(final MessageOrBuilder message, final TextGenerator generator)
        throws IOException {
      for (Map.Entry<FieldDescriptor, Object> field : message.getAllFields().entrySet()) {
        printField(field.getKey(), field.getValue(), generator);
      }
      printUnknownFields(message.getUnknownFields(), generator, this.enablingSafeDebugFormat);
    }

    private void printShortRepeatedField(
        final FieldDescriptor field, final Object value, final TextGenerator generator)
        throws IOException {
      generator.print(field.getName());
      generator.print(": ");
      generator.print("[");
      String separator = "";
      for (Object element : (List<?>) value) {
        generator.print(separator);
        printFieldValue(field, element, generator);
        separator = ", ";
      }
      generator.print("]");
      generator.eol();
    }

    private void printSingleField(
        final FieldDescriptor field, final Object value, final TextGenerator generator)
        throws IOException {
      if (field.isExtension()) {
        generator.print("[");
        // We special-case MessageSet elements for compatibility with proto1.
        if (field.getContainingType().getOptions().getMessageSetWireFormat()
            && (field.getType() == FieldDescriptor.Type.MESSAGE)
            && (field.isOptional())
            // object equality
            && (field.getExtensionScope() == field.getMessageType())) {
          generator.print(field.getMessageType().getFullName());
        } else {
          generator.print(field.getFullName());
        }
        generator.print("]");
      } else {
        if (field.isGroupLike()) {
          // Groups must be serialized with their original capitalization.
          generator.print(field.getMessageType().getName());
        } else {
          generator.print(field.getName());
        }
      }

      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        generator.maybePrintSilentMarker();
        generator.print("{");
        generator.eol();
        generator.indent();
      } else {
        generator.print(":");
        generator.maybePrintSilentMarker();
      }

      printFieldValue(field, value, generator);

      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        generator.outdent();
        generator.print("}");
      }
      generator.eol();
    }

    private static void printUnknownFields(
        final UnknownFieldSet unknownFields, final TextGenerator generator, boolean redact)
        throws IOException {
      if (unknownFields.isEmpty()) {
        return;
      }
      for (Map.Entry<Integer, UnknownFieldSet.Field> entry : unknownFields.asMap().entrySet()) {
        final int number = entry.getKey();
        final UnknownFieldSet.Field field = entry.getValue();
        printUnknownField(
            number, WireFormat.WIRETYPE_VARINT, field.getVarintList(), generator, redact);
        printUnknownField(
            number, WireFormat.WIRETYPE_FIXED32, field.getFixed32List(), generator, redact);
        printUnknownField(
            number, WireFormat.WIRETYPE_FIXED64, field.getFixed64List(), generator, redact);
        printUnknownField(
            number,
            WireFormat.WIRETYPE_LENGTH_DELIMITED,
            field.getLengthDelimitedList(),
            generator,
            redact);
        for (final UnknownFieldSet value : field.getGroupList()) {
          generator.print(entry.getKey().toString());
          generator.maybePrintSilentMarker();
          generator.print("{");
          generator.eol();
          generator.indent();
          printUnknownFields(value, generator, redact);
          generator.outdent();
          generator.print("}");
          generator.eol();
        }
      }
    }

    private static void printUnknownField(
        final int number,
        final int wireType,
        final List<?> values,
        final TextGenerator generator,
        boolean redact)
        throws IOException {
      for (final Object value : values) {
        generator.print(String.valueOf(number));
        generator.print(":");
        generator.maybePrintSilentMarker();
        printUnknownFieldValue(wireType, value, generator, redact);
        generator.eol();
      }
    }
  }

  /** Convert an unsigned 32-bit integer to a string. */
  public static String unsignedToString(final int value) {
    if (value >= 0) {
      return Integer.toString(value);
    } else {
      return Long.toString(value & 0x00000000FFFFFFFFL);
    }
  }

  /** Convert an unsigned 64-bit integer to a string. */
  public static String unsignedToString(final long value) {
    if (value >= 0) {
      return Long.toString(value);
    } else {
      // Pull off the most-significant bit so that BigInteger doesn't think
      // the number is negative, then set it again using setBit().
      return BigInteger.valueOf(value & 0x7FFFFFFFFFFFFFFFL).setBit(63).toString();
    }
  }

  private static TextGenerator setSingleLineOutput(Appendable output, boolean singleLine) {
    return new TextGenerator(
        output, singleLine, null, Printer.FieldReporterLevel.TEXT_GENERATOR, false);
  }

  private static TextGenerator setSingleLineOutput(
      Appendable output,
      boolean singleLine,
      Descriptor rootMessageType,
      Printer.FieldReporterLevel fieldReporterLevel,
      boolean shouldEmitSilentMarker) {
    return new TextGenerator(
        output, singleLine, rootMessageType, fieldReporterLevel, shouldEmitSilentMarker);
  }

  /** An inner class for writing text to the output stream. */
  private static final class TextGenerator {
    private final Appendable output;
    private final StringBuilder indent = new StringBuilder();
    private final boolean singleLineMode;
    private boolean shouldEmitSilentMarker;
    // While technically we are "at the start of a line" at the very beginning of the output, all
    // we would do in response to this is emit the (zero length) indentation, so it has no effect.
    // Setting it false here does however suppress an unwanted leading space in single-line mode.
    private boolean atStartOfLine = false;
    // Indicate which Protobuf public stringification API (e.g AbstractMessage.toString()) is
    // called.
    private final Printer.FieldReporterLevel fieldReporterLevel;
    // The root message type being printed. Null if the root message type is not known (e.g.
    // printing a field).
    private final Descriptor rootMessageType;

    private TextGenerator(
        final Appendable output,
        boolean singleLineMode,
        Descriptor rootMessageType,
        Printer.FieldReporterLevel fieldReporterLevel,
        boolean shouldEmitSilentMarker) {
      this.output = output;
      this.singleLineMode = singleLineMode;
      this.rootMessageType = rootMessageType;
      this.fieldReporterLevel = fieldReporterLevel;
      this.shouldEmitSilentMarker = shouldEmitSilentMarker;
    }

    /**
     * Indent text by two spaces. After calling Indent(), two spaces will be inserted at the
     * beginning of each line of text. Indent() may be called multiple times to produce deeper
     * indents.
     */
    public void indent() {
      indent.append("  ");
    }

    /** Reduces the current indent level by two spaces, or crashes if the indent level is zero. */
    public void outdent() {
      final int length = indent.length();
      if (length == 0) {
        throw new IllegalArgumentException(" Outdent() without matching Indent().");
      }
      indent.setLength(length - 2);
    }

    /**
     * Print text to the output stream. Bare newlines are never expected to be passed to this
     * method; to indicate the end of a line, call "eol()".
     */
    public void print(final CharSequence text) throws IOException {
      if (atStartOfLine) {
        atStartOfLine = false;
        output.append(singleLineMode ? " " : indent);
      }
      output.append(text);
    }

    /**
     * Signifies reaching the "end of the current line" in the output. In single-line mode, this
     * does not result in a newline being emitted, but ensures that a separating space is written
     * before the next output.
     */
    public void eol() throws IOException {
      if (!singleLineMode) {
        output.append("\n");
      }
      atStartOfLine = true;
    }

    void maybePrintSilentMarker() throws IOException {
      if (shouldEmitSilentMarker) {
        output.append(DEBUG_STRING_SILENT_MARKER);
        shouldEmitSilentMarker = false;
      } else {
        output.append(" ");
      }
    }
  }

  // =================================================================
  // Parsing

  /**
   * Represents a stream of tokens parsed from a {@code String}.
   *
   * <p>The Java standard library provides many classes that you might think would be useful for
   * implementing this, but aren't. For example:
   *
   * <ul>
   *   <li>{@code java.io.StreamTokenizer}: This almost does what we want -- or, at least, something
   *       that would get us close to what we want -- except for one fatal flaw: It automatically
   *       un-escapes strings using Java escape sequences, which do not include all the escape
   *       sequences we need to support (e.g. '\x').
   *   <li>{@code java.util.Scanner}: This seems like a great way at least to parse regular
   *       expressions out of a stream (so we wouldn't have to load the entire input into a single
   *       string before parsing). Sadly, {@code Scanner} requires that tokens be delimited with
   *       some delimiter. Thus, although the text "foo:" should parse to two tokens ("foo" and
   *       ":"), {@code Scanner} would recognize it only as a single token. Furthermore, {@code
   *       Scanner} provides no way to inspect the contents of delimiters, making it impossible to
   *       keep track of line and column numbers.
   * </ul>
   */
  private static final class Tokenizer {
    private final CharSequence text;
    private String currentToken;

    // The character index within this.text at which the current token begins.
    private int pos = 0;

    // The line and column numbers of the current token.
    private int line = 0;
    private int column = 0;
    private int lineInfoTrackingPos = 0;

    // The line and column numbers of the previous token (allows throwing
    // errors *after* consuming).
    private int previousLine = 0;
    private int previousColumn = 0;

    /**
     * {@link containsSilentMarkerAfterCurrentToken} indicates if there is a silent marker after the
     * current token. This value is moved to {@link containsSilentMarkerAfterPrevToken} every time
     * the next token is parsed.
     */
    private boolean containsSilentMarkerAfterCurrentToken = false;

    private boolean containsSilentMarkerAfterPrevToken = false;

    /** Construct a tokenizer that parses tokens from the given text. */
    private Tokenizer(final CharSequence text) {
      this.text = text;
      skipWhitespace();
      nextToken();
    }

    int getPreviousLine() {
      return previousLine;
    }

    int getPreviousColumn() {
      return previousColumn;
    }

    int getLine() {
      return line;
    }

    int getColumn() {
      return column;
    }

    boolean getContainsSilentMarkerAfterCurrentToken() {
      return containsSilentMarkerAfterCurrentToken;
    }

    boolean getContainsSilentMarkerAfterPrevToken() {
      return containsSilentMarkerAfterPrevToken;
    }

    /** Are we at the end of the input? */
    boolean atEnd() {
      return currentToken.length() == 0;
    }

    /** Advance to the next token. */
    void nextToken() {
      previousLine = line;
      previousColumn = column;

      // Advance the line counter to the current position.
      while (lineInfoTrackingPos < pos) {
        if (text.charAt(lineInfoTrackingPos) == '\n') {
          ++line;
          column = 0;
        } else {
          ++column;
        }
        ++lineInfoTrackingPos;
      }

      // Match the next token.
      if (pos == text.length()) {
        currentToken = ""; // EOF
      } else {
        currentToken = nextTokenInternal();
        skipWhitespace();
      }
    }

    private String nextTokenInternal() {
      final int textLength = this.text.length();
      final int startPos = this.pos;
      final char startChar = this.text.charAt(startPos);

      int endPos = pos;
      if (isAlphaUnder(startChar)) { // Identifier
        while (++endPos != textLength) {
          char c = this.text.charAt(endPos);
          if (!(isAlphaUnder(c) || isDigitPlusMinus(c))) {
            break;
          }
        }
      } else if (isDigitPlusMinus(startChar) || startChar == '.') { // Number
        if (startChar == '.') { // Optional leading dot
          if (++endPos == textLength) {
            return nextTokenSingleChar();
          }

          if (!isDigitPlusMinus(this.text.charAt(endPos))) { // Mandatory first digit
            return nextTokenSingleChar();
          }
        }

        while (++endPos != textLength) {
          char c = this.text.charAt(endPos);
          if (!(isDigitPlusMinus(c) || isAlphaUnder(c) || c == '.')) {
            break;
          }
        }
      } else if (startChar == '"' || startChar == '\'') { // String
        while (++endPos != textLength) {
          char c = this.text.charAt(endPos);
          if (c == startChar) {
            ++endPos;
            break; // Quote terminates
          } else if (c == '\n') {
            break; // Newline terminates (error during parsing) (not consumed)
          } else if (c == '\\') {
            if (++endPos == textLength) {
              break; // Escape into end-of-text terminates (error during parsing)
            } else if (this.text.charAt(endPos) == '\n') {
              break; // Escape into newline terminates (error during parsing) (not consumed)
            } else {
              // Otherwise the escaped char is legal and consumed
            }
          } else {
            // Otherwise the char is a legal and consumed
          }
        }
      } else {
        return nextTokenSingleChar(); // Unrecognized start character
      }

      this.pos = endPos;
      return this.text.subSequence(startPos, endPos).toString();
    }

    private static boolean isAlphaUnder(char c) {
      // Defining this char-class with numeric comparisons is much faster than using a regex.
      return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
    }

    private static boolean isDigitPlusMinus(char c) {
      // Defining this char-class with numeric comparisons is much faster than using a regex.
      return ('0' <= c && c <= '9') || c == '+' || c == '-';
    }

    private static boolean isWhitespace(char c) {
      // Defining this char-class with numeric comparisons is much faster than using a regex.
      return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t';
    }

    /**
     * Produce a token for the single char at the current position.
     *
     * <p>We hardcode the expected single-char tokens to avoid allocating a unique string every
     * time, which is a GC risk. String-literals are always loaded from the class constant pool.
     *
     * <p>This method must not be called if the current position is after the end-of-text.
     */
    private String nextTokenSingleChar() {
      final char c = this.text.charAt(this.pos++);
      switch (c) {
        case ':':
          return ":";
        case ',':
          return ",";
        case '[':
          return "[";
        case ']':
          return "]";
        case '{':
          return "{";
        case '}':
          return "}";
        case '<':
          return "<";
        case '>':
          return ">";
        default:
          // If we don't recognize the char, create a string and let the parser report any errors
          return String.valueOf(c);
      }
    }

    /** Skip over any whitespace so that the matcher region starts at the next token. */
    private void skipWhitespace() {
      final int textLength = this.text.length();
      final int startPos = this.pos;

      int endPos = this.pos - 1;
      while (++endPos != textLength) {
        char c = this.text.charAt(endPos);
        if (c == '#') {
          while (++endPos != textLength) {
            if (this.text.charAt(endPos) == '\n') {
              break; // Consume the newline as whitespace.
            }
          }
          if (endPos == textLength) {
            break;
          }
        } else if (isWhitespace(c)) {
          // OK
        } else {
          break;
        }
      }

      this.pos = endPos;
    }

    /**
     * If the next token exactly matches {@code token}, consume it and return {@code true}.
     * Otherwise, return {@code false} without doing anything.
     */
    boolean tryConsume(final String token) {
      if (currentToken.equals(token)) {
        nextToken();
        return true;
      } else {
        return false;
      }
    }

    /**
     * If the next token exactly matches {@code token}, consume it. Otherwise, throw a {@link
     * ParseException}.
     */
    void consume(final String token) throws ParseException {
      if (!tryConsume(token)) {
        throw parseException("Expected \"" + token + "\".");
      }
    }

    /** Returns {@code true} if the next token is an integer, but does not consume it. */
    boolean lookingAtInteger() {
      if (currentToken.length() == 0) {
        return false;
      }

      return isDigitPlusMinus(currentToken.charAt(0));
    }

    /** Returns {@code true} if the current token's text is equal to that specified. */
    boolean lookingAt(String text) {
      return currentToken.equals(text);
    }

    /**
     * If the next token is an identifier, consume it and return its value. Otherwise, throw a
     * {@link ParseException}.
     */
    String consumeIdentifier() throws ParseException {
      for (int i = 0; i < currentToken.length(); i++) {
        final char c = currentToken.charAt(i);
        if (isAlphaUnder(c) || ('0' <= c && c <= '9') || (c == '.')) {
          // OK
        } else {
          throw parseException("Expected identifier. Found '" + currentToken + "'");
        }
      }

      final String result = currentToken;
      nextToken();
      return result;
    }

    /**
     * If the next token is an identifier, consume it and return {@code true}. Otherwise, return
     * {@code false} without doing anything.
     */
    boolean tryConsumeIdentifier() {
      try {
        consumeIdentifier();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a 32-bit signed integer, consume it and return its value. Otherwise,
     * throw a {@link ParseException}.
     */
    int consumeInt32() throws ParseException {
      try {
        final int result = parseInt32(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw integerParseException(e);
      }
    }

    /**
     * If the next token is a 32-bit unsigned integer, consume it and return its value. Otherwise,
     * throw a {@link ParseException}.
     */
    int consumeUInt32() throws ParseException {
      try {
        final int result = parseUInt32(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw integerParseException(e);
      }
    }

    /**
     * If the next token is a 64-bit signed integer, consume it and return its value. Otherwise,
     * throw a {@link ParseException}.
     */
    long consumeInt64() throws ParseException {
      try {
        final long result = parseInt64(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw integerParseException(e);
      }
    }

    /**
     * If the next token is a 64-bit signed integer, consume it and return {@code true}. Otherwise,
     * return {@code false} without doing anything.
     */
    boolean tryConsumeInt64() {
      try {
        consumeInt64();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a 64-bit unsigned integer, consume it and return its value. Otherwise,
     * throw a {@link ParseException}.
     */
    long consumeUInt64() throws ParseException {
      try {
        final long result = parseUInt64(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw integerParseException(e);
      }
    }

    /**
     * If the next token is a 64-bit unsigned integer, consume it and return {@code true}.
     * Otherwise, return {@code false} without doing anything.
     */
    public boolean tryConsumeUInt64() {
      try {
        consumeUInt64();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a double, consume it and return its value. Otherwise, throw a {@link
     * ParseException}.
     */
    public double consumeDouble() throws ParseException {
      // We need to parse infinity and nan separately because
      // Double.parseDouble() does not accept "inf", "infinity", or "nan".
      switch (currentToken.toLowerCase(Locale.ROOT)) {
        case "-inf":
        case "-infinity":
          nextToken();
          return Double.NEGATIVE_INFINITY;
        case "inf":
        case "infinity":
          nextToken();
          return Double.POSITIVE_INFINITY;
        case "nan":
          nextToken();
          return Double.NaN;
        default:
          // fall through
      }

      try {
        final double result = Double.parseDouble(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw floatParseException(e);
      }
    }

    /**
     * If the next token is a double, consume it and return {@code true}. Otherwise, return {@code
     * false} without doing anything.
     */
    public boolean tryConsumeDouble() {
      try {
        consumeDouble();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a float, consume it and return its value. Otherwise, throw a {@link
     * ParseException}.
     */
    public float consumeFloat() throws ParseException {
      // We need to parse infinity and nan separately because
      // Float.parseFloat() does not accept "inf", "infinity", or "nan".
      switch (currentToken.toLowerCase(Locale.ROOT)) {
        case "-inf":
        case "-inff":
        case "-infinity":
        case "-infinityf":
          nextToken();
          return Float.NEGATIVE_INFINITY;
        case "inf":
        case "inff":
        case "infinity":
        case "infinityf":
          nextToken();
          return Float.POSITIVE_INFINITY;
        case "nan":
        case "nanf":
          nextToken();
          return Float.NaN;
        default:
          // fall through
      }

      try {
        final float result = Float.parseFloat(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw floatParseException(e);
      }
    }

    /**
     * If the next token is a float, consume it and return {@code true}. Otherwise, return {@code
     * false} without doing anything.
     */
    public boolean tryConsumeFloat() {
      try {
        consumeFloat();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a boolean, consume it and return its value. Otherwise, throw a {@link
     * ParseException}.
     */
    public boolean consumeBoolean() throws ParseException {
      if (currentToken.equals("true")
          || currentToken.equals("True")
          || currentToken.equals("t")
          || currentToken.equals("1")) {
        nextToken();
        return true;
      } else if (currentToken.equals("false")
          || currentToken.equals("False")
          || currentToken.equals("f")
          || currentToken.equals("0")) {
        nextToken();
        return false;
      } else {
        throw parseException("Expected \"true\" or \"false\". Found \"" + currentToken + "\".");
      }
    }

    /**
     * If the next token is a string, consume it and return its (unescaped) value. Otherwise, throw
     * a {@link ParseException}.
     */
    public String consumeString() throws ParseException {
      return consumeByteString().toStringUtf8();
    }

    /**
     * If the next token is a string, consume it, unescape it as a {@link ByteString}, and return
     * it. Otherwise, throw a {@link ParseException}.
     */
    @CanIgnoreReturnValue
    ByteString consumeByteString() throws ParseException {
      List<ByteString> list = new ArrayList<ByteString>();
      consumeByteString(list);
      while (currentToken.startsWith("'") || currentToken.startsWith("\"")) {
        consumeByteString(list);
      }
      return ByteString.copyFrom(list);
    }

    /** If the next token is a string, consume it and return true. Otherwise, return false. */
    boolean tryConsumeByteString() {
      try {
        consumeByteString();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * Like {@link #consumeByteString()} but adds each token of the string to the given list. String
     * literals (whether bytes or text) may come in multiple adjacent tokens which are automatically
     * concatenated, like in C or Python.
     */
    private void consumeByteString(List<ByteString> list) throws ParseException {
      final char quote = currentToken.length() > 0 ? currentToken.charAt(0) : '\0';
      if (quote != '\"' && quote != '\'') {
        throw parseException("Expected string.");
      }

      if (currentToken.length() < 2 || currentToken.charAt(currentToken.length() - 1) != quote) {
        throw parseException("String missing ending quote.");
      }

      try {
        final String escaped = currentToken.substring(1, currentToken.length() - 1);
        final ByteString result = unescapeBytes(escaped);
        nextToken();
        list.add(result);
      } catch (InvalidEscapeSequenceException e) {
        throw parseException(e.getMessage());
      }
    }

    /**
     * Returns a {@link ParseException} with the current line and column numbers in the description,
     * suitable for throwing.
     */
    ParseException parseException(final String description) {
      // Note:  People generally prefer one-based line and column numbers.
      return new ParseException(line + 1, column + 1, description);
    }

    /**
     * Returns a {@link ParseException} with the line and column numbers of the previous token in
     * the description, suitable for throwing.
     */
    ParseException parseExceptionPreviousToken(final String description) {
      // Note:  People generally prefer one-based line and column numbers.
      return new ParseException(previousLine + 1, previousColumn + 1, description);
    }

    /**
     * Constructs an appropriate {@link ParseException} for the given {@code NumberFormatException}
     * when trying to parse an integer.
     */
    private ParseException integerParseException(final NumberFormatException e) {
      return parseException("Couldn't parse integer: " + e.getMessage());
    }

    /**
     * Constructs an appropriate {@link ParseException} for the given {@code NumberFormatException}
     * when trying to parse a float or double.
     */
    private ParseException floatParseException(final NumberFormatException e) {
      return parseException("Couldn't parse number: " + e.getMessage());
    }
  }

  /** Thrown when parsing an invalid text format message. */
  public static class ParseException extends IOException {
    private static final long serialVersionUID = 3196188060225107702L;

    private final int line;
    private final int column;

    /** Create a new instance, with -1 as the line and column numbers. */
    public ParseException(final String message) {
      this(-1, -1, message);
    }

    /**
     * Create a new instance
     *
     * @param line the line number where the parse error occurred, using 1-offset.
     * @param column the column number where the parser error occurred, using 1-offset.
     */
    public ParseException(final int line, final int column, final String message) {
      super(Integer.toString(line) + ":" + column + ": " + message);
      this.line = line;
      this.column = column;
    }

    /**
     * Return the line where the parse exception occurred, or -1 when none is provided. The value is
     * specified as 1-offset, so the first line is line 1.
     */
    public int getLine() {
      return line;
    }

    /**
     * Return the column where the parse exception occurred, or -1 when none is provided. The value
     * is specified as 1-offset, so the first line is line 1.
     */
    public int getColumn() {
      return column;
    }
  }

  /** Obsolete exception, once thrown when encountering an unknown field while parsing a text
  format message.
  *
  * @deprecated This exception is unused and will be removed in the next breaking release
  (v5.x.x).
  */
  @Deprecated
  public static class UnknownFieldParseException extends ParseException {
    private final String unknownField;

    /**
     * Create a new instance, with -1 as the line and column numbers, and an empty unknown field
     * name.
     */
    public UnknownFieldParseException(final String message) {
      this(-1, -1, "", message);
    }

    /**
     * Create a new instance
     *
     * @param line the line number where the parse error occurred, using 1-offset.
     * @param column the column number where the parser error occurred, using 1-offset.
     * @param unknownField the name of the unknown field found while parsing.
     */
    public UnknownFieldParseException(
        final int line, final int column, final String unknownField, final String message) {
      super(line, column, message);
      this.unknownField = unknownField;
    }

    /**
     * Return the name of the unknown field encountered while parsing the protocol buffer string.
     */
    public String getUnknownField() {
      return unknownField;
    }
  }

  private static final Parser PARSER = Parser.newBuilder().build();

  /**
   * Return a {@link Parser} instance which can parse text-format messages. The returned instance is
   * thread-safe.
   */
  public static Parser getParser() {
    return PARSER;
  }

  /** Parse a text-format message from {@code input} and merge the contents into {@code builder}. */
  public static void merge(final Readable input, final Message.Builder builder) throws IOException {
    PARSER.merge(input, builder);
  }

  /** Parse a text-format message from {@code input} and merge the contents into {@code builder}. */
  public static void merge(final CharSequence input, final Message.Builder builder)
      throws ParseException {
    PARSER.merge(input, builder);
  }

  /**
   * Parse a text-format message from {@code input}.
   *
   * @return the parsed message, guaranteed initialized
   */
  public static <T extends Message> T parse(final CharSequence input, final Class<T> protoClass)
      throws ParseException {
    Message.Builder builder = Internal.getDefaultInstance(protoClass).newBuilderForType();
    merge(input, builder);
    @SuppressWarnings("unchecked")
    T output = (T) builder.build();
    return output;
  }

  /**
   * Parse a text-format message from {@code input} and merge the contents into {@code builder}.
   * Extensions will be recognized if they are registered in {@code extensionRegistry}.
   */
  public static void merge(
      final Readable input,
      final ExtensionRegistry extensionRegistry,
      final Message.Builder builder)
      throws IOException {
    PARSER.merge(input, extensionRegistry, builder);
  }

  /**
   * Parse a text-format message from {@code input} and merge the contents into {@code builder}.
   * Extensions will be recognized if they are registered in {@code extensionRegistry}.
   */
  public static void merge(
      final CharSequence input,
      final ExtensionRegistry extensionRegistry,
      final Message.Builder builder)
      throws ParseException {
    PARSER.merge(input, extensionRegistry, builder);
  }

  /**
   * Parse a text-format message from {@code input}. Extensions will be recognized if they are
   * registered in {@code extensionRegistry}.
   *
   * @return the parsed message, guaranteed initialized
   */
  public static <T extends Message> T parse(
      final CharSequence input,
      final ExtensionRegistry extensionRegistry,
      final Class<T> protoClass)
      throws ParseException {
    Message.Builder builder = Internal.getDefaultInstance(protoClass).newBuilderForType();
    merge(input, extensionRegistry, builder);
    @SuppressWarnings("unchecked")
    T output = (T) builder.build();
    return output;
  }

  /**
   * Parser for text-format proto2 instances. This class is thread-safe. The implementation largely
   * follows google/protobuf/text_format.cc.
   *
   * <p>Use {@link TextFormat#getParser()} to obtain the default parser, or {@link Builder} to
   * control the parser behavior.
   */
  public static class Parser {

    /**
     * A valid silent marker appears between a field name and its value. If there is a ":" in
     * between, the silent marker will only appear after the colon. This is called after a field
     * name is parsed, and before the ":" if it exists. If the current token is ":", then
     * containsSilentMarkerAfterCurrentToken indicates if there is a valid silent marker. Otherwise,
     * the current token is part of the field value, so the silent marker is indicated by
     * containsSilentMarkerAfterPrevToken.
     */
    private void detectSilentMarker(
        Tokenizer tokenizer, Descriptor immediateMessageType, String fieldName) {
    }

    /**
     * Determines if repeated values for non-repeated fields and oneofs are permitted. For example,
     * given required/optional field "foo" and a oneof containing "baz" and "moo":
     *
     * <ul>
     *   <li>"foo: 1 foo: 2"
     *   <li>"baz: 1 moo: 2"
     *   <li>merging "foo: 2" into a proto in which foo is already set, or
     *   <li>merging "moo: 2" into a proto in which baz is already set.
     * </ul>
     */
    public enum SingularOverwritePolicy {
      /**
       * Later values are merged with earlier values. For primitive fields or conflicting oneofs,
       * the last value is retained.
       */
      ALLOW_SINGULAR_OVERWRITES,
      /** An error is issued. */
      FORBID_SINGULAR_OVERWRITES
    }

    private final TypeRegistry typeRegistry;
    private final boolean allowUnknownFields;
    private final boolean allowUnknownEnumValues;
    private final boolean allowUnknownExtensions;
    private final SingularOverwritePolicy singularOverwritePolicy;
    private TextFormatParseInfoTree.Builder parseInfoTreeBuilder;
    private final int recursionLimit;

    private Parser(
        TypeRegistry typeRegistry,
        boolean allowUnknownFields,
        boolean allowUnknownEnumValues,
        boolean allowUnknownExtensions,
        SingularOverwritePolicy singularOverwritePolicy,
        TextFormatParseInfoTree.Builder parseInfoTreeBuilder,
        int recursionLimit) {
      this.typeRegistry = typeRegistry;
      this.allowUnknownFields = allowUnknownFields;
      this.allowUnknownEnumValues = allowUnknownEnumValues;
      this.allowUnknownExtensions = allowUnknownExtensions;
      this.singularOverwritePolicy = singularOverwritePolicy;
      this.parseInfoTreeBuilder = parseInfoTreeBuilder;
      this.recursionLimit = recursionLimit;
    }

    /** Returns a new instance of {@link Builder}. */
    public static Builder newBuilder() {
      return new Builder();
    }

    /** Builder that can be used to obtain new instances of {@link Parser}. */
    public static class Builder {
      private boolean allowUnknownFields = false;
      private boolean allowUnknownEnumValues = false;
      private boolean allowUnknownExtensions = false;
      private SingularOverwritePolicy singularOverwritePolicy =
          SingularOverwritePolicy.ALLOW_SINGULAR_OVERWRITES;
      private TextFormatParseInfoTree.Builder parseInfoTreeBuilder = null;
      private TypeRegistry typeRegistry = TypeRegistry.getEmptyTypeRegistry();
      private int recursionLimit = 100;

      /**
       * Sets the TypeRegistry for resolving Any. If this is not set, TextFormat will not be able to
       * parse Any unless Any is write as bytes.
       *
       * @throws IllegalArgumentException if a registry is already set.
       */
      public Builder setTypeRegistry(TypeRegistry typeRegistry) {
        this.typeRegistry = typeRegistry;
        return this;
      }

      /**
       * Set whether this parser will allow unknown fields. By default, an exception is thrown if an
       * unknown field is encountered. If this is set, the parser will only log a warning. Allow
       * unknown fields will also allow unknown extensions.
       *
       * <p>Use of this parameter is discouraged which may hide some errors (e.g. spelling error on
       * field name).
       */
      public Builder setAllowUnknownFields(boolean allowUnknownFields) {
        this.allowUnknownFields = allowUnknownFields;
        return this;
      }

      /**
       * Set whether this parser will allow unknown extensions. By default, an exception is thrown
       * if unknown extension is encountered. If this is set true, the parser will only log a
       * warning. Allow unknown extensions does not mean allow normal unknown fields.
       */
      public Builder setAllowUnknownExtensions(boolean allowUnknownExtensions) {
        this.allowUnknownExtensions = allowUnknownExtensions;
        return this;
      }

      /** Sets parser behavior when a non-repeated field appears more than once. */
      public Builder setSingularOverwritePolicy(SingularOverwritePolicy p) {
        this.singularOverwritePolicy = p;
        return this;
      }

      public Builder setParseInfoTreeBuilder(TextFormatParseInfoTree.Builder parseInfoTreeBuilder) {
        this.parseInfoTreeBuilder = parseInfoTreeBuilder;
        return this;
      }

      /**
       * Set the maximum recursion limit that the parser will allow. If the depth of the message
       * exceeds this limit then the parser will stop and throw an exception.
       */
      public Builder setRecursionLimit(int recursionLimit) {
        this.recursionLimit = recursionLimit;
        return this;
      }

      public Parser build() {
        return new Parser(
            typeRegistry,
            allowUnknownFields,
            allowUnknownEnumValues,
            allowUnknownExtensions,
            singularOverwritePolicy,
            parseInfoTreeBuilder,
            recursionLimit);
      }
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents into {@code builder}.
     */
    public void merge(final Readable input, final Message.Builder builder) throws IOException {
      merge(input, ExtensionRegistry.getEmptyRegistry(), builder);
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents into {@code builder}.
     */
    public void merge(final CharSequence input, final Message.Builder builder)
        throws ParseException {
      merge(input, ExtensionRegistry.getEmptyRegistry(), builder);
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents into {@code builder}.
     * Extensions will be recognized if they are registered in {@code extensionRegistry}.
     */
    public void merge(
        final Readable input,
        final ExtensionRegistry extensionRegistry,
        final Message.Builder builder)
        throws IOException {
      // Read the entire input to a String then parse that.

      // If StreamTokenizer was not so limited, or if there were a kind
      // of Reader that could read in chunks that match some particular regex,
      // or if we wanted to write a custom Reader to tokenize our stream, then
      // we would not have to read to one big String.  Alas, none of these is
      // the case.  Oh well.

      merge(toStringBuilder(input), extensionRegistry, builder);
    }

    private static final int BUFFER_SIZE = 4096;

    // TODO: See if working around java.io.Reader#read(CharBuffer)
    // overhead is worthwhile
    private static StringBuilder toStringBuilder(final Readable input) throws IOException {
      final StringBuilder text = new StringBuilder();
      final CharBuffer buffer = CharBuffer.allocate(BUFFER_SIZE);
      while (true) {
        final int n = input.read(buffer);
        if (n == -1) {
          break;
        }
        Java8Compatibility.flip(buffer);
        text.append(buffer, 0, n);
      }
      return text;
    }

    static final class UnknownField {
      static enum Type {
        FIELD,
        EXTENSION;
      }

      final String message;
      final Type type;

      UnknownField(String message, Type type) {
        this.message = message;
        this.type = type;
      }
    }

    // Check both unknown fields and unknown extensions and log warning messages
    // or throw exceptions according to the flag.
    private void checkUnknownFields(final List<UnknownField> unknownFields) throws ParseException {
      if (unknownFields.isEmpty()) {
        return;
      }

      StringBuilder msg = new StringBuilder("Input contains unknown fields and/or extensions:");
      for (UnknownField field : unknownFields) {
        msg.append('\n').append(field.message);
      }

      if (allowUnknownFields) {
        logger.warning(msg.toString());
        return;
      }

      int firstErrorIndex = 0;
      if (allowUnknownExtensions) {
        boolean allUnknownExtensions = true;
        for (UnknownField field : unknownFields) {
          if (field.type == UnknownField.Type.FIELD) {
            allUnknownExtensions = false;
            break;
          }
          ++firstErrorIndex;
        }
        if (allUnknownExtensions) {
          logger.warning(msg.toString());
          return;
        }
      }

      String[] lineColumn = unknownFields.get(firstErrorIndex).message.split(":");
      throw new ParseException(
          Integer.parseInt(lineColumn[0]), Integer.parseInt(lineColumn[1]), msg.toString());
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents into {@code builder}.
     * Extensions will be recognized if they are registered in {@code extensionRegistry}.
     */
    public void merge(
        final CharSequence input,
        final ExtensionRegistry extensionRegistry,
        final Message.Builder builder)
        throws ParseException {
      final Tokenizer tokenizer = new Tokenizer(input);
      MessageReflection.BuilderAdapter target = new MessageReflection.BuilderAdapter(builder);
      List<UnknownField> unknownFields = new ArrayList<UnknownField>();

      while (!tokenizer.atEnd()) {
        mergeField(tokenizer, extensionRegistry, target, unknownFields, recursionLimit);
      }
      checkUnknownFields(unknownFields);
    }

    /** Parse a single field from {@code tokenizer} and merge it into {@code builder}. */
    private void mergeField(
        final Tokenizer tokenizer,
        final ExtensionRegistry extensionRegistry,
        final MessageReflection.MergeTarget target,
        List<UnknownField> unknownFields,
        int recursionLimit)
        throws ParseException {
      mergeField(
          tokenizer,
          extensionRegistry,
          target,
          parseInfoTreeBuilder,
          unknownFields,
          recursionLimit);
    }

    /** Parse a single field from {@code tokenizer} and merge it into {@code target}. */
    private void mergeField(
        final Tokenizer tokenizer,
        final ExtensionRegistry extensionRegistry,
        final MessageReflection.MergeTarget target,
        TextFormatParseInfoTree.Builder parseTreeBuilder,
        List<UnknownField> unknownFields,
        int recursionLimit)
        throws ParseException {
      FieldDescriptor field = null;
      String name;
      int startLine = tokenizer.getLine();
      int startColumn = tokenizer.getColumn();
      final Descriptor type = target.getDescriptorForType();
      ExtensionRegistry.ExtensionInfo extension = null;

      if ("google.protobuf.Any".equals(type.getFullName()) && tokenizer.tryConsume("[")) {
        if (recursionLimit < 1) {
          throw tokenizer.parseException("Message is nested too deep");
        }
        mergeAnyFieldValue(
            tokenizer,
            extensionRegistry,
            target,
            parseTreeBuilder,
            unknownFields,
            type,
            recursionLimit - 1);
        return;
      }

      if (tokenizer.tryConsume("[")) {
        // An extension.
        StringBuilder nameBuilder = new StringBuilder(tokenizer.consumeIdentifier());
        while (tokenizer.tryConsume(".")) {
          nameBuilder.append('.');
          nameBuilder.append(tokenizer.consumeIdentifier());
        }
        name = nameBuilder.toString();

        extension = target.findExtensionByName(extensionRegistry, name);

        if (extension == null) {
          String message =
              (tokenizer.getPreviousLine() + 1)
                  + ":"
                  + (tokenizer.getPreviousColumn() + 1)
                  + ":\t"
                  + type.getFullName()
                  + ".["
                  + name
                  + "]";
          unknownFields.add(new UnknownField(message, UnknownField.Type.EXTENSION));
        } else {
          if (extension.descriptor.getContainingType() != type) {
            throw tokenizer.parseExceptionPreviousToken(
                "Extension \""
                    + name
                    + "\" does not extend message type \""
                    + type.getFullName()
                    + "\".");
          }
          field = extension.descriptor;
        }

        tokenizer.consume("]");
      } else {
        name = tokenizer.consumeIdentifier();
        field = type.findFieldByName(name);

        // Group names are expected to be capitalized as they appear in the
        // .proto file, which actually matches their type names, not their field
        // names.
        if (field == null) {
          // Explicitly specify US locale so that this code does not break when
          // executing in Turkey.
          final String lowerName = name.toLowerCase(Locale.US);
          field = type.findFieldByName(lowerName);
          // If the case-insensitive match worked but the field is NOT a group,
          if (field != null && !field.isGroupLike()) {
            field = null;
          }
          if (field != null && !field.getMessageType().getName().equals(name)) {
            field = null;
          }
        }

        if (field == null) {
          String message =
              (tokenizer.getPreviousLine() + 1)
                  + ":"
                  + (tokenizer.getPreviousColumn() + 1)
                  + ":\t"
                  + type.getFullName()
                  + "."
                  + name;
          unknownFields.add(new UnknownField(message, UnknownField.Type.FIELD));
        }
      }

      // Skips unknown fields.
      if (field == null) {
        detectSilentMarker(tokenizer, type, name);
        guessFieldTypeAndSkip(tokenizer, type, recursionLimit);
        return;
      }

      // Handle potential ':'.
      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        detectSilentMarker(tokenizer, type, field.getFullName());
        tokenizer.tryConsume(":"); // optional
        if (parseTreeBuilder != null) {
          TextFormatParseInfoTree.Builder childParseTreeBuilder =
              parseTreeBuilder.getBuilderForSubMessageField(field);
          consumeFieldValues(
              tokenizer,
              extensionRegistry,
              target,
              field,
              extension,
              childParseTreeBuilder,
              unknownFields,
              recursionLimit);
        } else {
          consumeFieldValues(
              tokenizer,
              extensionRegistry,
              target,
              field,
              extension,
              parseTreeBuilder,
              unknownFields,
              recursionLimit);
        }
      } else {
        detectSilentMarker(tokenizer, type, field.getFullName());
        tokenizer.consume(":"); // required
        consumeFieldValues(
            tokenizer,
            extensionRegistry,
            target,
            field,
            extension,
            parseTreeBuilder,
            unknownFields,
            recursionLimit);
      }

      if (parseTreeBuilder != null) {
        parseTreeBuilder.setLocation(field, TextFormatParseLocation.create(startLine, startColumn));
      }

      // For historical reasons, fields may optionally be separated by commas or
      // semicolons.
      if (!tokenizer.tryConsume(";")) {
        tokenizer.tryConsume(",");
      }
    }

    private String consumeFullTypeName(Tokenizer tokenizer) throws ParseException {
      // If there is not a leading `[`, this is just a type name.
      if (!tokenizer.tryConsume("[")) {
        return tokenizer.consumeIdentifier();
      }

      // Otherwise, this is an extension or google.protobuf.Any type URL: we consume proto path
      // elements until we've addressed the type.
      String name = tokenizer.consumeIdentifier();
      while (tokenizer.tryConsume(".")) {
        name += "." + tokenizer.consumeIdentifier();
      }
      if (tokenizer.tryConsume("/")) {
        name += "/" + tokenizer.consumeIdentifier();
        while (tokenizer.tryConsume(".")) {
          name += "." + tokenizer.consumeIdentifier();
        }
      }
      tokenizer.consume("]");
      return name;
    }

    /**
     * Parse a one or more field values from {@code tokenizer} and merge it into {@code builder}.
     */
    private void consumeFieldValues(
        final Tokenizer tokenizer,
        final ExtensionRegistry extensionRegistry,
        final MessageReflection.MergeTarget target,
        final FieldDescriptor field,
        final ExtensionRegistry.ExtensionInfo extension,
        final TextFormatParseInfoTree.Builder parseTreeBuilder,
        List<UnknownField> unknownFields,
        int recursionLimit)
        throws ParseException {
      // Support specifying repeated field values as a comma-separated list.
      // Ex."foo: [1, 2, 3]"
      if (field.isRepeated() && tokenizer.tryConsume("[")) {
        if (!tokenizer.tryConsume("]")) { // Allow "foo: []" to be treated as empty.
          while (true) {
            consumeFieldValue(
                tokenizer,
                extensionRegistry,
                target,
                field,
                extension,
                parseTreeBuilder,
                unknownFields,
                recursionLimit);
            if (tokenizer.tryConsume("]")) {
              // End of list.
              break;
            }
            tokenizer.consume(",");
          }
        }
      } else {
        consumeFieldValue(
            tokenizer,
            extensionRegistry,
            target,
            field,
            extension,
            parseTreeBuilder,
            unknownFields,
            recursionLimit);
      }
    }

    /** Parse a single field value from {@code tokenizer} and merge it into {@code builder}. */
    private void consumeFieldValue(
        final Tokenizer tokenizer,
        final ExtensionRegistry extensionRegistry,
        final MessageReflection.MergeTarget target,
        final FieldDescriptor field,
        final ExtensionRegistry.ExtensionInfo extension,
        final TextFormatParseInfoTree.Builder parseTreeBuilder,
        List<UnknownField> unknownFields,
        int recursionLimit)
        throws ParseException {
      if (singularOverwritePolicy == SingularOverwritePolicy.FORBID_SINGULAR_OVERWRITES
          && !field.isRepeated()) {
        if (target.hasField(field)) {
          throw tokenizer.parseExceptionPreviousToken(
              "Non-repeated field \"" + field.getFullName() + "\" cannot be overwritten.");
        } else if (field.getContainingOneof() != null
            && target.hasOneof(field.getContainingOneof())) {
          Descriptors.OneofDescriptor oneof = field.getContainingOneof();
          throw tokenizer.parseExceptionPreviousToken(
              "Field \""
                  + field.getFullName()
                  + "\" is specified along with field \""
                  + target.getOneofFieldDescriptor(oneof).getFullName()
                  + "\", another member of oneof \""
                  + oneof.getName()
                  + "\".");
        }
      }

      Object value = null;

      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        if (recursionLimit < 1) {
          throw tokenizer.parseException("Message is nested too deep");
        }

        final String endToken;
        if (tokenizer.tryConsume("<")) {
          endToken = ">";
        } else {
          tokenizer.consume("{");
          endToken = "}";
        }

        Message defaultInstance = (extension == null) ? null : extension.defaultInstance;
        MessageReflection.MergeTarget subField =
            target.newMergeTargetForField(field, defaultInstance);

        while (!tokenizer.tryConsume(endToken)) {
          if (tokenizer.atEnd()) {
            throw tokenizer.parseException("Expected \"" + endToken + "\".");
          }
          mergeField(
              tokenizer,
              extensionRegistry,
              subField,
              parseTreeBuilder,
              unknownFields,
              recursionLimit - 1);
        }

        value = subField.finish();
      } else {
        switch (field.getType()) {
          case INT32:
          case SINT32:
          case SFIXED32:
            value = tokenizer.consumeInt32();
            break;

          case INT64:
          case SINT64:
          case SFIXED64:
            value = tokenizer.consumeInt64();
            break;

          case UINT32:
          case FIXED32:
            value = tokenizer.consumeUInt32();
            break;

          case UINT64:
          case FIXED64:
            value = tokenizer.consumeUInt64();
            break;

          case FLOAT:
            value = tokenizer.consumeFloat();
            break;

          case DOUBLE:
            value = tokenizer.consumeDouble();
            break;

          case BOOL:
            value = tokenizer.consumeBoolean();
            break;

          case STRING:
            value = tokenizer.consumeString();
            break;

          case BYTES:
            value = tokenizer.consumeByteString();
            break;

          case ENUM:
            final EnumDescriptor enumType = field.getEnumType();

            if (tokenizer.lookingAtInteger()) {
              final int number = tokenizer.consumeInt32();
              value =
                  enumType.isClosed()
                      ? enumType.findValueByNumber(number)
                      : enumType.findValueByNumberCreatingIfUnknown(number);
              if (value == null) {
                String unknownValueMsg =
                    "Enum type \""
                        + enumType.getFullName()
                        + "\" has no value with number "
                        + number
                        + '.';
                if (allowUnknownEnumValues) {
                  logger.warning(unknownValueMsg);
                  return;
                } else {
                  throw tokenizer.parseExceptionPreviousToken(
                      "Enum type \""
                          + enumType.getFullName()
                          + "\" has no value with number "
                          + number
                          + '.');
                }
              }
            } else {
              final String id = tokenizer.consumeIdentifier();
              value = enumType.findValueByName(id);
              if (value == null) {
                String unknownValueMsg =
                    "Enum type \""
                        + enumType.getFullName()
                        + "\" has no value named \""
                        + id
                        + "\".";
                if (allowUnknownEnumValues) {
                  logger.warning(unknownValueMsg);
                  return;
                } else {
                  throw tokenizer.parseExceptionPreviousToken(unknownValueMsg);
                }
              }
            }

            break;

          case MESSAGE:
          case GROUP:
            throw new RuntimeException("Can't get here.");
        }
      }

      if (field.isRepeated()) {
        // TODO: If field.isMapField() and FORBID_SINGULAR_OVERWRITES mode,
        //     check for duplicate map keys here.
        target.addRepeatedField(field, value);
      } else {
        target.setField(field, value);
      }
    }

    private void mergeAnyFieldValue(
        final Tokenizer tokenizer,
        final ExtensionRegistry extensionRegistry,
        MergeTarget target,
        final TextFormatParseInfoTree.Builder parseTreeBuilder,
        List<UnknownField> unknownFields,
        Descriptor anyDescriptor,
        int recursionLimit)
        throws ParseException {
      // Try to parse human readable format of Any in the form: [type_url]: { ... }
      StringBuilder typeUrlBuilder = new StringBuilder();
      // Parse the type_url inside [].
      while (true) {
        typeUrlBuilder.append(tokenizer.consumeIdentifier());
        if (tokenizer.tryConsume("]")) {
          break;
        }
        if (tokenizer.tryConsume("/")) {
          typeUrlBuilder.append("/");
        } else if (tokenizer.tryConsume(".")) {
          typeUrlBuilder.append(".");
        } else {
          throw tokenizer.parseExceptionPreviousToken("Expected a valid type URL.");
        }
      }
      detectSilentMarker(tokenizer, anyDescriptor, typeUrlBuilder.toString());
      tokenizer.tryConsume(":");
      final String anyEndToken;
      if (tokenizer.tryConsume("<")) {
        anyEndToken = ">";
      } else {
        tokenizer.consume("{");
        anyEndToken = "}";
      }
      String typeUrl = typeUrlBuilder.toString();
      Descriptor contentType = null;
      try {
        contentType = typeRegistry.getDescriptorForTypeUrl(typeUrl);
      } catch (InvalidProtocolBufferException e) {
        throw tokenizer.parseException("Invalid valid type URL. Found: " + typeUrl);
      }
      if (contentType == null) {
        throw tokenizer.parseException(
            "Unable to parse Any of type: "
                + typeUrl
                + ". Please make sure that the TypeRegistry contains the descriptors for the given"
                + " types.");
      }
      Message.Builder contentBuilder =
          DynamicMessage.getDefaultInstance(contentType).newBuilderForType();
      MessageReflection.BuilderAdapter contentTarget =
          new MessageReflection.BuilderAdapter(contentBuilder);
      while (!tokenizer.tryConsume(anyEndToken)) {
        mergeField(
            tokenizer,
            extensionRegistry,
            contentTarget,
            parseTreeBuilder,
            unknownFields,
            recursionLimit);
      }

      target.setField(anyDescriptor.findFieldByName("type_url"), typeUrlBuilder.toString());
      target.setField(
          anyDescriptor.findFieldByName("value"), contentBuilder.build().toByteString());
    }

    /** Skips the next field including the field's name and value. */
    private void skipField(Tokenizer tokenizer, Descriptor type, int recursionLimit)
        throws ParseException {
      String name = consumeFullTypeName(tokenizer);
      detectSilentMarker(tokenizer, type, name);
      guessFieldTypeAndSkip(tokenizer, type, recursionLimit);

      // For historical reasons, fields may optionally be separated by commas or
      // semicolons.
      if (!tokenizer.tryConsume(";")) {
        tokenizer.tryConsume(",");
      }
    }

    /**
     * Skips the whole body of a message including the beginning delimiter and the ending delimiter.
     */
    private void skipFieldMessage(Tokenizer tokenizer, Descriptor type, int recursionLimit)
        throws ParseException {
      final String delimiter;
      if (tokenizer.tryConsume("<")) {
        delimiter = ">";
      } else {
        tokenizer.consume("{");
        delimiter = "}";
      }
      while (!tokenizer.lookingAt(">") && !tokenizer.lookingAt("}")) {
        skipField(tokenizer, type, recursionLimit);
      }
      tokenizer.consume(delimiter);
    }

    /** Skips a field value. */
    private void skipFieldValue(Tokenizer tokenizer) throws ParseException {
      if (!tokenizer.tryConsumeByteString()
          && !tokenizer.tryConsumeIdentifier() // includes enum & boolean
          && !tokenizer.tryConsumeInt64() // includes int32
          && !tokenizer.tryConsumeUInt64() // includes uint32
          && !tokenizer.tryConsumeDouble()
          && !tokenizer.tryConsumeFloat()) {
        throw tokenizer.parseException("Invalid field value: " + tokenizer.currentToken);
      }
    }

    /**
     * Tries to guess the type of this field and skip it.
     *
     * <p>If this field is not a message, there should be a ":" between the field name and the field
     * value and also the field value should not start with "{" or "<" which indicates the beginning
     * of a message body. If there is no ":" or there is a "{" or "<" after ":", this field has to
     * be a message or the input is ill-formed. For short-formed repeated fields (i.e. with "[]"),
     * if it is repeated scalar, there must be a ":" between the field name and the starting "[" .
     */
    private void guessFieldTypeAndSkip(Tokenizer tokenizer, Descriptor type, int recursionLimit)
        throws ParseException {
      boolean semicolonConsumed = tokenizer.tryConsume(":");
      if (tokenizer.lookingAt("[")) {
        // Short repeated field form. If a semicolon was consumed, it could be repeated scalar or
        // repeated message. If not, it must be repeated message.
        skipFieldShortFormedRepeated(tokenizer, semicolonConsumed, type, recursionLimit);
      } else if (semicolonConsumed && !tokenizer.lookingAt("{") && !tokenizer.lookingAt("<")) {
        skipFieldValue(tokenizer);
      } else {
        if (recursionLimit < 1) {
          throw tokenizer.parseException("Message is nested too deep");
        }
        skipFieldMessage(tokenizer, type, recursionLimit - 1);
      }
    }

    /**
     * Skips a short-formed repeated field value.
     *
     * <p>Reports an error if scalar type is not allowed but showing up inside "[]".
     */
    private void skipFieldShortFormedRepeated(
        Tokenizer tokenizer, boolean scalarAllowed, Descriptor type, int recursionLimit)
        throws ParseException {
      if (!tokenizer.tryConsume("[") || tokenizer.tryConsume("]")) {
        // Try skipping "[]".
        return;
      }

      while (true) {
        if (tokenizer.lookingAt("{") || tokenizer.lookingAt("<")) {
          // Try skipping message field inside "[]"
          if (recursionLimit < 1) {
            throw tokenizer.parseException("Message is nested too deep");
          }
          skipFieldMessage(tokenizer, type, recursionLimit - 1);
        } else if (scalarAllowed) {
          // Try skipping scalar field inside "[]".
          skipFieldValue(tokenizer);
        } else {
          throw tokenizer.parseException(
              "Invalid repeated scalar field: missing \":\" before \"[\".");
        }
        if (tokenizer.tryConsume("]")) {
          break;
        }
        tokenizer.consume(",");
      }
    }
  }

  // =================================================================
  // Utility functions
  //
  // Some of these methods are package-private because Descriptors.java uses
  // them.

  /**
   * Escapes bytes in the format used in protocol buffer text format, which is the same as the
   * format used for C string literals. All bytes that are not printable 7-bit ASCII characters are
   * escaped, as well as backslash, single-quote, and double-quote characters. Characters for which
   * no defined short-hand escape sequence is defined will be escaped using 3-digit octal sequences.
   */
  public static String escapeBytes(ByteString input) {
    return TextFormatEscaper.escapeBytes(input);
  }

  /** Like {@link #escapeBytes(ByteString)}, but used for byte array. */
  public static String escapeBytes(byte[] input) {
    return TextFormatEscaper.escapeBytes(input);
  }

  /**
   * Un-escape a byte sequence as escaped using {@link #escapeBytes(ByteString)}. Two-digit hex
   * escapes (starting with "\x") are also recognized.
   */
  public static ByteString unescapeBytes(CharSequence charString)
      throws InvalidEscapeSequenceException {
    // First convert the Java character sequence to UTF-8 bytes.
    ByteString input = ByteString.copyFromUtf8(charString.toString());
    // Then unescape certain byte sequences introduced by ASCII '\\'.  The valid
    // escapes can all be expressed with ASCII characters, so it is safe to
    // operate on bytes here.
    //
    // Unescaping the input byte array will result in a byte sequence that's no
    // longer than the input.  That's because each escape sequence is between
    // two and four bytes long and stands for a single byte.
    final byte[] result = new byte[input.size()];
    int pos = 0;
    for (int i = 0; i < input.size(); i++) {
      byte c = input.byteAt(i);
      if (c == '\\') {
        if (i + 1 < input.size()) {
          ++i;
          c = input.byteAt(i);
          if (isOctal(c)) {
            // Octal escape.
            int code = digitValue(c);
            if (i + 1 < input.size() && isOctal(input.byteAt(i + 1))) {
              ++i;
              code = code * 8 + digitValue(input.byteAt(i));
            }
            if (i + 1 < input.size() && isOctal(input.byteAt(i + 1))) {
              ++i;
              code = code * 8 + digitValue(input.byteAt(i));
            }
            // TODO: Check that 0 <= code && code <= 0xFF.
            result[pos++] = (byte) code;
          } else {
            switch (c) {
              case 'a':
                result[pos++] = 0x07;
                break;
              case 'b':
                result[pos++] = '\b';
                break;
              case 'f':
                result[pos++] = '\f';
                break;
              case 'n':
                result[pos++] = '\n';
                break;
              case 'r':
                result[pos++] = '\r';
                break;
              case 't':
                result[pos++] = '\t';
                break;
              case 'v':
                result[pos++] = 0x0b;
                break;
              case '\\':
                result[pos++] = '\\';
                break;
              case '\'':
                result[pos++] = '\'';
                break;
              case '"':
                result[pos++] = '\"';
                break;
              case '?':
                result[pos++] = '?';
                break;

              case 'x':
                // hex escape
                int code = 0;
                if (i + 1 < input.size() && isHex(input.byteAt(i + 1))) {
                  ++i;
                  code = digitValue(input.byteAt(i));
                } else {
                  throw new InvalidEscapeSequenceException(
                      "Invalid escape sequence: '\\x' with no digits");
                }
                if (i + 1 < input.size() && isHex(input.byteAt(i + 1))) {
                  ++i;
                  code = code * 16 + digitValue(input.byteAt(i));
                }
                result[pos++] = (byte) code;
                break;

              case 'u':
                // Unicode escape
                ++i;
                if (i + 3 < input.size()
                    && isHex(input.byteAt(i))
                    && isHex(input.byteAt(i + 1))
                    && isHex(input.byteAt(i + 2))
                    && isHex(input.byteAt(i + 3))) {
                  char ch =
                      (char)
                          (digitValue(input.byteAt(i)) << 12
                              | digitValue(input.byteAt(i + 1)) << 8
                              | digitValue(input.byteAt(i + 2)) << 4
                              | digitValue(input.byteAt(i + 3)));

                  if (ch >= Character.MIN_SURROGATE && ch <= Character.MAX_SURROGATE) {
                    throw new InvalidEscapeSequenceException(
                        "Invalid escape sequence: '\\u' refers to a surrogate");
                  }
                  byte[] chUtf8 = Character.toString(ch).getBytes(Internal.UTF_8);
                  System.arraycopy(chUtf8, 0, result, pos, chUtf8.length);
                  pos += chUtf8.length;
                  i += 3;
                } else {
                  throw new InvalidEscapeSequenceException(
                      "Invalid escape sequence: '\\u' with too few hex chars");
                }
                break;

              case 'U':
                // Unicode escape
                ++i;
                if (i + 7 >= input.size()) {
                  throw new InvalidEscapeSequenceException(
                      "Invalid escape sequence: '\\U' with too few hex chars");
                }
                int codepoint = 0;
                for (int offset = i; offset < i + 8; offset++) {
                  byte b = input.byteAt(offset);
                  if (!isHex(b)) {
                    throw new InvalidEscapeSequenceException(
                        "Invalid escape sequence: '\\U' with too few hex chars");
                  }
                  codepoint = (codepoint << 4) | digitValue(b);
                }
                if (!Character.isValidCodePoint(codepoint)) {
                  throw new InvalidEscapeSequenceException(
                      "Invalid escape sequence: '\\U"
                          + input.substring(i, i + 8).toStringUtf8()
                          + "' is not a valid code point value");
                }
                Character.UnicodeBlock unicodeBlock = Character.UnicodeBlock.of(codepoint);
                if (unicodeBlock != null
                    && (unicodeBlock.equals(Character.UnicodeBlock.LOW_SURROGATES)
                        || unicodeBlock.equals(Character.UnicodeBlock.HIGH_SURROGATES)
                        || unicodeBlock.equals(
                            Character.UnicodeBlock.HIGH_PRIVATE_USE_SURROGATES))) {
                  throw new InvalidEscapeSequenceException(
                      "Invalid escape sequence: '\\U"
                          + input.substring(i, i + 8).toStringUtf8()
                          + "' refers to a surrogate code unit");
                }
                int[] codepoints = new int[1];
                codepoints[0] = codepoint;
                byte[] chUtf8 = new String(codepoints, 0, 1).getBytes(Internal.UTF_8);
                System.arraycopy(chUtf8, 0, result, pos, chUtf8.length);
                pos += chUtf8.length;
                i += 7;
                break;

              default:
                throw new InvalidEscapeSequenceException(
                    "Invalid escape sequence: '\\" + (char) c + '\'');
            }
          }
        } else {
          throw new InvalidEscapeSequenceException(
              "Invalid escape sequence: '\\' at end of string.");
        }
      } else {
        result[pos++] = c;
      }
    }

    return result.length == pos
        ? ByteString.wrap(result) // This reference has not been out of our control.
        : ByteString.copyFrom(result, 0, pos);
  }

  /**
   * Thrown by {@link TextFormat#unescapeBytes} and {@link TextFormat#unescapeText} when an invalid
   * escape sequence is seen.
   */
  public static class InvalidEscapeSequenceException extends IOException {
    private static final long serialVersionUID = -8164033650142593304L;

    InvalidEscapeSequenceException(final String description) {
      super(description);
    }
  }

  /**
   * Like {@link #escapeBytes(ByteString)}, but escapes a text string. Non-ASCII characters are
   * first encoded as UTF-8, then each byte is escaped individually as a 3-digit octal escape. Yes,
   * it's weird.
   */
  static String escapeText(final String input) {
    return escapeBytes(ByteString.copyFromUtf8(input));
  }

  /** Escape double quotes and backslashes in a String for emittingUnicode output of a message. */
  public static String escapeDoubleQuotesAndBackslashes(final String input) {
    return TextFormatEscaper.escapeDoubleQuotesAndBackslashes(input);
  }

  /**
   * Un-escape a text string as escaped using {@link #escapeText(String)}. Two-digit hex escapes
   * (starting with "\x") are also recognized.
   */
  static String unescapeText(final String input) throws InvalidEscapeSequenceException {
    return unescapeBytes(input).toStringUtf8();
  }

  /** Is this an octal digit? */
  private static boolean isOctal(final byte c) {
    return '0' <= c && c <= '7';
  }

  /** Is this a hex digit? */
  private static boolean isHex(final byte c) {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
  }

  /**
   * Interpret a character as a digit (in any base up to 36) and return the numeric value. This is
   * like {@code Character.digit()} but we don't accept non-ASCII digits.
   */
  private static int digitValue(final byte c) {
    if ('0' <= c && c <= '9') {
      return c - '0';
    } else if ('a' <= c && c <= 'z') {
      return c - 'a' + 10;
    } else {
      return c - 'A' + 10;
    }
  }

  /**
   * Parse a 32-bit signed integer from the text. Unlike the Java standard {@code
   * Integer.parseInt()}, this function recognizes the prefixes "0x" and "0" to signify hexadecimal
   * and octal numbers, respectively.
   */
  static int parseInt32(final String text) throws NumberFormatException {
    return (int) parseInteger(text, true, false);
  }

  /**
   * Parse a 32-bit unsigned integer from the text. Unlike the Java standard {@code
   * Integer.parseInt()}, this function recognizes the prefixes "0x" and "0" to signify hexadecimal
   * and octal numbers, respectively. The result is coerced to a (signed) {@code int} when returned
   * since Java has no unsigned integer type.
   */
  static int parseUInt32(final String text) throws NumberFormatException {
    return (int) parseInteger(text, false, false);
  }

  /**
   * Parse a 64-bit signed integer from the text. Unlike the Java standard {@code
   * Integer.parseInt()}, this function recognizes the prefixes "0x" and "0" to signify hexadecimal
   * and octal numbers, respectively.
   */
  static long parseInt64(final String text) throws NumberFormatException {
    return parseInteger(text, true, true);
  }

  /**
   * Parse a 64-bit unsigned integer from the text. Unlike the Java standard {@code
   * Integer.parseInt()}, this function recognizes the prefixes "0x" and "0" to signify hexadecimal
   * and octal numbers, respectively. The result is coerced to a (signed) {@code long} when returned
   * since Java has no unsigned long type.
   */
  static long parseUInt64(final String text) throws NumberFormatException {
    return parseInteger(text, false, true);
  }

  private static long parseInteger(final String text, final boolean isSigned, final boolean isLong)
      throws NumberFormatException {
    int pos = 0;

    boolean negative = false;
    if (text.startsWith("-", pos)) {
      if (!isSigned) {
        throw new NumberFormatException("Number must be positive: " + text);
      }
      ++pos;
      negative = true;
    }

    int radix = 10;
    if (text.startsWith("0x", pos)) {
      pos += 2;
      radix = 16;
    } else if (text.startsWith("0", pos)) {
      radix = 8;
    }

    final String numberText = text.substring(pos);

    long result = 0;
    if (numberText.length() < 16) {
      // Can safely assume no overflow.
      result = Long.parseLong(numberText, radix);
      if (negative) {
        result = -result;
      }

      // Check bounds.
      // No need to check for 64-bit numbers since they'd have to be 16 chars
      // or longer to overflow.
      if (!isLong) {
        if (isSigned) {
          if (result > Integer.MAX_VALUE || result < Integer.MIN_VALUE) {
            throw new NumberFormatException(
                "Number out of range for 32-bit signed integer: " + text);
          }
        } else {
          if (result >= (1L << 32) || result < 0) {
            throw new NumberFormatException(
                "Number out of range for 32-bit unsigned integer: " + text);
          }
        }
      }
    } else {
      BigInteger bigValue = new BigInteger(numberText, radix);
      if (negative) {
        bigValue = bigValue.negate();
      }

      // Check bounds.
      if (!isLong) {
        if (isSigned) {
          if (bigValue.bitLength() > 31) {
            throw new NumberFormatException(
                "Number out of range for 32-bit signed integer: " + text);
          }
        } else {
          if (bigValue.bitLength() > 32) {
            throw new NumberFormatException(
                "Number out of range for 32-bit unsigned integer: " + text);
          }
        }
      } else {
        if (isSigned) {
          if (bigValue.bitLength() > 63) {
            throw new NumberFormatException(
                "Number out of range for 64-bit signed integer: " + text);
          }
        } else {
          if (bigValue.bitLength() > 64) {
            throw new NumberFormatException(
                "Number out of range for 64-bit unsigned integer: " + text);
          }
        }
      }

      result = bigValue.longValue();
    }

    return result;
  }
}
