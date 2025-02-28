// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableSet;
import com.google.common.io.BaseEncoding;
import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonIOException;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonPrimitive;
import com.google.gson.stream.JsonReader;
import com.google.protobuf.Any;
import com.google.protobuf.BoolValue;
import com.google.protobuf.ByteString;
import com.google.protobuf.BytesValue;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.DoubleValue;
import com.google.protobuf.Duration;
import com.google.protobuf.DynamicMessage;
import com.google.protobuf.FieldMask;
import com.google.protobuf.FloatValue;
import com.google.protobuf.Int32Value;
import com.google.protobuf.Int64Value;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.ListValue;
import com.google.protobuf.Message;
import com.google.protobuf.MessageOrBuilder;
import com.google.protobuf.NullValue;
import com.google.protobuf.StringValue;
import com.google.protobuf.Struct;
import com.google.protobuf.Timestamp;
import com.google.protobuf.UInt32Value;
import com.google.protobuf.UInt64Value;
import com.google.protobuf.Value;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.text.ParseException;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.logging.Logger;
import javax.annotation.Nullable;

/**
 * Utility class to convert protobuf messages to/from the <a href=
 * 'https://developers.google.com/protocol-buffers/docs/proto3#json'>Proto3 JSON format.</a>
 * Only proto3 features are supported. Proto2 only features such as extensions and unknown fields
 * are discarded in the conversion. That is, when converting proto2 messages to JSON format,
 * extensions and unknown fields are treated as if they do not exist. This applies to proto2
 * messages embedded in proto3 messages as well.
 */
public class JsonFormat {
  private static final Logger logger = Logger.getLogger(JsonFormat.class.getName());

  private JsonFormat() {}

  /**
   * Creates a {@link Printer} with default configurations.
   */
  public static Printer printer() {
    return new Printer(
        com.google.protobuf.TypeRegistry.getEmptyTypeRegistry(),
        TypeRegistry.getEmptyTypeRegistry(),
        ShouldPrintDefaults.ONLY_IF_PRESENT,
        /* includingDefaultValueFields */ ImmutableSet.of(),
        /* preservingProtoFieldNames */ false,
        /* omittingInsignificantWhitespace */ false,
        /* printingEnumsAsInts */ false,
        /* sortingMapKeys */ false,
        /* unsafeDisableCodepointsForHtmlSymbols */ false);
  }

  private enum ShouldPrintDefaults {
    ONLY_IF_PRESENT, // The "normal" behavior; the others add more compared to this baseline.
    ALWAYS_PRINT_EXCEPT_MESSAGES_AND_ONEOFS,
    ALWAYS_PRINT_WITHOUT_PRESENCE_FIELDS,
    ALWAYS_PRINT_SPECIFIED_FIELDS
  }

  /** A Printer converts a protobuf message to the proto3 JSON format. */
  public static class Printer {
    private final com.google.protobuf.TypeRegistry registry;
    private final TypeRegistry oldRegistry;
    private final ShouldPrintDefaults shouldPrintDefaults;

    // Empty unless shouldPrintDefaults is set to ALWAYS_PRINT_SPECIFIED_FIELDS.
    private final Set<FieldDescriptor> includingDefaultValueFields;

    private final boolean preservingProtoFieldNames;
    private final boolean omittingInsignificantWhitespace;
    private final boolean printingEnumsAsInts;
    private final boolean sortingMapKeys;
    private final boolean unsafeDisableCodepointsForHtmlSymbols;

    private Printer(
        com.google.protobuf.TypeRegistry registry,
        TypeRegistry oldRegistry,
        ShouldPrintDefaults shouldOutputDefaults,
        Set<FieldDescriptor> includingDefaultValueFields,
        boolean preservingProtoFieldNames,
        boolean omittingInsignificantWhitespace,
        boolean printingEnumsAsInts,
        boolean sortingMapKeys,
        boolean unsafeDisableCodepointsForHtmlSymbols) {
      this.registry = registry;
      this.oldRegistry = oldRegistry;
      this.shouldPrintDefaults = shouldOutputDefaults;
      this.includingDefaultValueFields = includingDefaultValueFields;
      this.preservingProtoFieldNames = preservingProtoFieldNames;
      this.omittingInsignificantWhitespace = omittingInsignificantWhitespace;
      this.printingEnumsAsInts = printingEnumsAsInts;
      this.sortingMapKeys = sortingMapKeys;
      this.unsafeDisableCodepointsForHtmlSymbols = unsafeDisableCodepointsForHtmlSymbols;
    }

    /**
     * Creates a new {@link Printer} using the given registry. The new Printer clones all other
     * configurations from the current {@link Printer}.
     *
     * @throws IllegalArgumentException if a registry is already set
     */
    public Printer usingTypeRegistry(TypeRegistry oldRegistry) {
      if (this.oldRegistry != TypeRegistry.getEmptyTypeRegistry()
          || this.registry != com.google.protobuf.TypeRegistry.getEmptyTypeRegistry()) {
        throw new IllegalArgumentException("Only one registry is allowed.");
      }
      return new Printer(
          com.google.protobuf.TypeRegistry.getEmptyTypeRegistry(),
          oldRegistry,
          shouldPrintDefaults,
          includingDefaultValueFields,
          preservingProtoFieldNames,
          omittingInsignificantWhitespace,
          printingEnumsAsInts,
          sortingMapKeys,
          unsafeDisableCodepointsForHtmlSymbols);
    }

    /**
     * Creates a new {@link Printer} using the given registry. The new Printer clones all other
     * configurations from the current {@link Printer}.
     *
     * @throws IllegalArgumentException if a registry is already set
     */
    public Printer usingTypeRegistry(com.google.protobuf.TypeRegistry registry) {
      if (this.oldRegistry != TypeRegistry.getEmptyTypeRegistry()
          || this.registry != com.google.protobuf.TypeRegistry.getEmptyTypeRegistry()) {
        throw new IllegalArgumentException("Only one registry is allowed.");
      }
      return new Printer(
          registry,
          oldRegistry,
          shouldPrintDefaults,
          includingDefaultValueFields,
          preservingProtoFieldNames,
          omittingInsignificantWhitespace,
          printingEnumsAsInts,
          sortingMapKeys,
          unsafeDisableCodepointsForHtmlSymbols);
    }

    /**
     * Creates a new {@link Printer} that will always print fields unless they are a message type or
     * in a oneof.
     *
     * <p>Note that this does print Proto2 Optional but does not print Proto3 Optional fields, as
     * the latter is represented using a synthetic oneof.
     *
     * <p>The new Printer clones all other configurations from the current {@link Printer}.
     *
     * @deprecated This method is deprecated, and slated for removal in the next Java breaking
     *     change (5.x). Prefer {@link #alwaysPrintFieldsWithNoPresence}
     */
    @Deprecated
    public Printer includingDefaultValueFields() {
      if (shouldPrintDefaults != ShouldPrintDefaults.ONLY_IF_PRESENT) {
        throw new IllegalStateException(
            "JsonFormat includingDefaultValueFields has already been set.");
      }
      return new Printer(
          registry,
          oldRegistry,
          ShouldPrintDefaults.ALWAYS_PRINT_EXCEPT_MESSAGES_AND_ONEOFS,
          ImmutableSet.of(),
          preservingProtoFieldNames,
          omittingInsignificantWhitespace,
          printingEnumsAsInts,
          sortingMapKeys,
          unsafeDisableCodepointsForHtmlSymbols);
    }

    /**
     * Creates a new {@link Printer} that will also print default-valued fields if their
     * FieldDescriptors are found in the supplied set. Empty repeated fields and map fields will be
     * printed as well, if they match. The new Printer clones all other configurations from the
     * current {@link Printer}.
     *
     * <p>Note that non-repeated message fields or fields in a oneof are not honored if provided
     * here.
     */
    public Printer includingDefaultValueFields(Set<FieldDescriptor> fieldsToAlwaysOutput) {
      Preconditions.checkArgument(
          null != fieldsToAlwaysOutput && !fieldsToAlwaysOutput.isEmpty(),
          "Non-empty Set must be supplied for includingDefaultValueFields.");
      if (shouldPrintDefaults != ShouldPrintDefaults.ONLY_IF_PRESENT) {
        throw new IllegalStateException(
            "JsonFormat includingDefaultValueFields has already been set.");
      }
      return new Printer(
          registry,
          oldRegistry,
          ShouldPrintDefaults.ALWAYS_PRINT_SPECIFIED_FIELDS,
          ImmutableSet.copyOf(fieldsToAlwaysOutput),
          preservingProtoFieldNames,
          omittingInsignificantWhitespace,
          printingEnumsAsInts,
          sortingMapKeys,
          unsafeDisableCodepointsForHtmlSymbols);
    }

    /**
     * Creates a new {@link Printer} that will print any field that does not support presence even
     * if it would not otherwise be printed (empty repeated fields, empty map fields, and implicit
     * presence scalars set to their default value). The new Printer clones all other configurations
     * from the current {@link Printer}.
     */
    public Printer alwaysPrintFieldsWithNoPresence() {
      if (shouldPrintDefaults != ShouldPrintDefaults.ONLY_IF_PRESENT) {
        throw new IllegalStateException("Only one of the JsonFormat defaults options can be set.");
      }
      return new Printer(
          registry,
          oldRegistry,
          ShouldPrintDefaults.ALWAYS_PRINT_WITHOUT_PRESENCE_FIELDS,
          ImmutableSet.of(),
          preservingProtoFieldNames,
          omittingInsignificantWhitespace,
          printingEnumsAsInts,
          sortingMapKeys,
          unsafeDisableCodepointsForHtmlSymbols);
    }

    /**
     * Creates a new {@link Printer} that prints enum field values as integers instead of as string.
     * The new Printer clones all other configurations from the current {@link Printer}.
     */
    public Printer printingEnumsAsInts() {
      checkUnsetPrintingEnumsAsInts();
      return new Printer(
          registry,
          oldRegistry,
          shouldPrintDefaults,
          includingDefaultValueFields,
          preservingProtoFieldNames,
          omittingInsignificantWhitespace,
          true,
          sortingMapKeys,
          unsafeDisableCodepointsForHtmlSymbols);
    }

    private void checkUnsetPrintingEnumsAsInts() {
      if (printingEnumsAsInts) {
        throw new IllegalStateException("JsonFormat printingEnumsAsInts has already been set.");
      }
    }

    /**
     * Creates a new {@link Printer} that is configured to use the original proto
     * field names as defined in the .proto file rather than converting them to
     * lowerCamelCase. The new Printer clones all other configurations from the
     * current {@link Printer}.
     */
    public Printer preservingProtoFieldNames() {
      return new Printer(
          registry,
          oldRegistry,
          shouldPrintDefaults,
          includingDefaultValueFields,
          true,
          omittingInsignificantWhitespace,
          printingEnumsAsInts,
          sortingMapKeys,
          unsafeDisableCodepointsForHtmlSymbols);
    }


    /**
     * Create a new {@link Printer} that omits insignificant whitespace in the JSON output.
     * This new Printer clones all other configurations from the current Printer. Insignificant
     * whitespace is defined by the JSON spec as whitespace that appears between JSON structural
     * elements:
     *
     * <pre>
     * ws = *(
     * %x20 /              ; Space
     * %x09 /              ; Horizontal tab
     * %x0A /              ; Line feed or New line
     * %x0D )              ; Carriage return
     * </pre>
     *
     * See <a href="https://tools.ietf.org/html/rfc7159">https://tools.ietf.org/html/rfc7159</a>.
     */
    public Printer omittingInsignificantWhitespace() {
      return new Printer(
          registry,
          oldRegistry,
          shouldPrintDefaults,
          includingDefaultValueFields,
          preservingProtoFieldNames,
          true,
          printingEnumsAsInts,
          sortingMapKeys,
          unsafeDisableCodepointsForHtmlSymbols);
    }

    /**
     * Create a new {@link Printer} that will sort the map keys in the JSON output.
     *
     * <p>Use of this modifier is discouraged. The generated JSON messages are equivalent with and
     * without this option set, but there are some corner use cases that demand a stable output,
     * while order of map keys is otherwise arbitrary.
     *
     * <p>The generated order is not well-defined and should not be depended on, but it's stable.
     *
     * <p>This new Printer clones all other configurations from the current {@link Printer}.
     */
    public Printer sortingMapKeys() {
      return new Printer(
          registry,
          oldRegistry,
          shouldPrintDefaults,
          includingDefaultValueFields,
          preservingProtoFieldNames,
          omittingInsignificantWhitespace,
          printingEnumsAsInts,
          true,
          unsafeDisableCodepointsForHtmlSymbols);
    }

    /**
     * Converts a protobuf message to the proto3 JSON format.
     *
     * @throws InvalidProtocolBufferException if the message contains Any types that can't be
     *     resolved
     * @throws IOException if writing to the output fails
     */
    public void appendTo(MessageOrBuilder message, Appendable output) throws IOException {
      // TODO: Investigate the allocation overhead and optimize for
      // mobile.
      new PrinterImpl(
              registry,
              oldRegistry,
              shouldPrintDefaults,
              includingDefaultValueFields,
              preservingProtoFieldNames,
              output,
              omittingInsignificantWhitespace,
              printingEnumsAsInts,
              sortingMapKeys,
              unsafeDisableCodepointsForHtmlSymbols)
          .print(message);
    }

    /**
     * Converts a protobuf message to the proto3 JSON format. Throws exceptions if there
     * are unknown Any types in the message.
     */
    public String print(MessageOrBuilder message) throws InvalidProtocolBufferException {
      try {
        StringBuilder builder = new StringBuilder();
        appendTo(message, builder);
        return builder.toString();
      } catch (InvalidProtocolBufferException e) {
        throw e;
      } catch (IOException e) {
        // Unexpected IOException.
        throw new IllegalStateException(e);
      }
    }
  }

  /**
   * Creates a {@link Parser} with default configuration.
   */
  public static Parser parser() {
    return new Parser(
        com.google.protobuf.TypeRegistry.getEmptyTypeRegistry(),
        TypeRegistry.getEmptyTypeRegistry(),
        false,
        Parser.DEFAULT_RECURSION_LIMIT);
  }

  /**
   * A Parser parses the proto3 JSON format into a protobuf message.
   */
  public static class Parser {
    private final com.google.protobuf.TypeRegistry registry;
    private final TypeRegistry oldRegistry;
    private final boolean ignoringUnknownFields;
    private final int recursionLimit;

    // The default parsing recursion limit is aligned with the proto binary parser.
    private static final int DEFAULT_RECURSION_LIMIT = 100;

    private Parser(
        com.google.protobuf.TypeRegistry registry,
        TypeRegistry oldRegistry,
        boolean ignoreUnknownFields,
        int recursionLimit) {
      this.registry = registry;
      this.oldRegistry = oldRegistry;
      this.ignoringUnknownFields = ignoreUnknownFields;
      this.recursionLimit = recursionLimit;
    }

    /**
     * Creates a new {@link Parser} using the given registry. The new Parser clones all other
     * configurations from this Parser.
     *
     * @throws IllegalArgumentException if a registry is already set
     */
    public Parser usingTypeRegistry(TypeRegistry oldRegistry) {
      if (this.oldRegistry != TypeRegistry.getEmptyTypeRegistry()
          || this.registry != com.google.protobuf.TypeRegistry.getEmptyTypeRegistry()) {
        throw new IllegalArgumentException("Only one registry is allowed.");
      }
      return new Parser(
          com.google.protobuf.TypeRegistry.getEmptyTypeRegistry(),
          oldRegistry,
          ignoringUnknownFields,
          recursionLimit);
    }

    /**
     * Creates a new {@link Parser} using the given registry. The new Parser clones all other
     * configurations from this Parser.
     *
     * @throws IllegalArgumentException if a registry is already set
     */
    public Parser usingTypeRegistry(com.google.protobuf.TypeRegistry registry) {
      if (this.oldRegistry != TypeRegistry.getEmptyTypeRegistry()
          || this.registry != com.google.protobuf.TypeRegistry.getEmptyTypeRegistry()) {
        throw new IllegalArgumentException("Only one registry is allowed.");
      }
      return new Parser(registry, oldRegistry, ignoringUnknownFields, recursionLimit);
    }

    /**
     * Creates a new {@link Parser} configured to not throw an exception when an unknown field is
     * encountered. The new Parser clones all other configurations from this Parser.
     */
    public Parser ignoringUnknownFields() {
      return new Parser(this.registry, oldRegistry, true, recursionLimit);
    }

    /**
     * Parses from the proto3 JSON format into a protobuf message.
     *
     * @throws InvalidProtocolBufferException if the input is not valid JSON
     *         proto3 format or there are unknown fields in the input.
     */
    public void merge(String json, Message.Builder builder) throws InvalidProtocolBufferException {
      // TODO: Investigate the allocation overhead and optimize for
      // mobile.
      new ParserImpl(registry, oldRegistry, ignoringUnknownFields, recursionLimit)
          .merge(json, builder);
    }

    /**
     * Parses from the proto3 JSON encoding into a protobuf message.
     *
     * @throws InvalidProtocolBufferException if the input is not valid proto3 JSON
     *         format or there are unknown fields in the input
     * @throws IOException if reading from the input throws
     */
    public void merge(Reader json, Message.Builder builder) throws IOException {
      // TODO: Investigate the allocation overhead and optimize for
      // mobile.
      new ParserImpl(registry, oldRegistry, ignoringUnknownFields, recursionLimit)
          .merge(json, builder);
    }

    // For testing only.
    Parser usingRecursionLimit(int recursionLimit) {
      return new Parser(registry, oldRegistry, ignoringUnknownFields, recursionLimit);
    }
  }

  /**
   * A TypeRegistry is used to resolve Any messages in the JSON conversion.
   * You must provide a TypeRegistry containing all message types used in
   * Any message fields, or the JSON conversion will fail because data
   * in Any message fields is unrecognizable. You don't need to supply a
   * TypeRegistry if you don't use Any message fields.
   */
  public static class TypeRegistry {
    private static class EmptyTypeRegistryHolder {
      private static final TypeRegistry EMPTY =
          new TypeRegistry(Collections.<String, Descriptor>emptyMap());
    }

    public static TypeRegistry getEmptyTypeRegistry() {
      return EmptyTypeRegistryHolder.EMPTY;
    }

    public static Builder newBuilder() {
      return new Builder();
    }

    /**
     * Find a type by its full name. Returns null if it cannot be found in this {@link
     * TypeRegistry}.
     */
    @Nullable
    public Descriptor find(String name) {
      return types.get(name);
    }

    @Nullable
    Descriptor getDescriptorForTypeUrl(String typeUrl) throws InvalidProtocolBufferException {
      return find(getTypeName(typeUrl));
    }

    private final Map<String, Descriptor> types;

    private TypeRegistry(Map<String, Descriptor> types) {
      this.types = types;
    }

    /** A Builder is used to build {@link TypeRegistry}. */
    public static class Builder {
      private Builder() {}

      /**
       * Adds a message type and all types defined in the same .proto file as well as all
       * transitively imported .proto files to this {@link Builder}.
       */
      @CanIgnoreReturnValue
      public Builder add(Descriptor messageType) {
        if (built) {
          throw new IllegalStateException("A TypeRegistry.Builder can only be used once.");
        }
        addFile(messageType.getFile());
        return this;
      }

      /**
       * Adds message types and all types defined in the same .proto file as well as all
       * transitively imported .proto files to this {@link Builder}.
       */
      @CanIgnoreReturnValue
      public Builder add(Iterable<Descriptor> messageTypes) {
        if (built) {
          throw new IllegalStateException("A TypeRegistry.Builder can only be used once.");
        }
        for (Descriptor type : messageTypes) {
          addFile(type.getFile());
        }
        return this;
      }

      /**
       * Builds a {@link TypeRegistry}. This method can only be called once for
       * one Builder.
       */
      public TypeRegistry build() {
        built = true;
        return new TypeRegistry(types);
      }

      private void addFile(FileDescriptor file) {
        // Skip the file if it's already added.
        if (!files.add(file.getFullName())) {
          return;
        }
        for (FileDescriptor dependency : file.getDependencies()) {
          addFile(dependency);
        }
        for (Descriptor message : file.getMessageTypes()) {
          addMessage(message);
        }
      }

      private void addMessage(Descriptor message) {
        for (Descriptor nestedType : message.getNestedTypes()) {
          addMessage(nestedType);
        }

        if (types.containsKey(message.getFullName())) {
          logger.warning("Type " + message.getFullName() + " is added multiple times.");
          return;
        }

        types.put(message.getFullName(), message);
      }

      private final Set<String> files = new HashSet<>();
      private final Map<String, Descriptor> types = new HashMap<>();
      private boolean built = false;
    }
  }

  /**
   * An interface for JSON formatting that can be used in
   * combination with the omittingInsignificantWhitespace() method.
   */
  interface TextGenerator {
    void indent();

    void outdent();

    void print(final CharSequence text) throws IOException;
  }

  /**
   * Format the JSON without indentation
   */
  private static final class CompactTextGenerator implements TextGenerator {
    private final Appendable output;

    private CompactTextGenerator(final Appendable output) {
      this.output = output;
    }

    /** ignored by compact printer */
    @Override
    public void indent() {}

    /** ignored by compact printer */
    @Override
    public void outdent() {}

    /** Print text to the output stream. */
    @Override
    public void print(final CharSequence text) throws IOException {
      output.append(text);
    }
  }
  /**
   * A TextGenerator adds indentation when writing formatted text.
   */
  private static final class PrettyTextGenerator implements TextGenerator {
    private final Appendable output;
    private final StringBuilder indent = new StringBuilder();
    private boolean atStartOfLine = true;

    private PrettyTextGenerator(final Appendable output) {
      this.output = output;
    }

    /**
     * Indent text by two spaces. After calling Indent(), two spaces will be inserted at the
     * beginning of each line of text. Indent() may be called multiple times to produce deeper
     * indents.
     */
    @Override
    public void indent() {
      indent.append("  ");
    }

    /** Reduces the current indent level by two spaces, or crashes if the indent level is zero. */
    @Override
    public void outdent() {
      final int length = indent.length();
      if (length < 2) {
        throw new IllegalArgumentException(" Outdent() without matching Indent().");
      }
      indent.delete(length - 2, length);
    }

    /** Print text to the output stream. */
    @Override
    public void print(final CharSequence text) throws IOException {
      final int size = text.length();
      int pos = 0;

      for (int i = 0; i < size; i++) {
        if (text.charAt(i) == '\n') {
          write(text.subSequence(pos, i + 1));
          pos = i + 1;
          atStartOfLine = true;
        }
      }
      write(text.subSequence(pos, size));
    }

    private void write(final CharSequence data) throws IOException {
      if (data.length() == 0) {
        return;
      }
      if (atStartOfLine) {
        atStartOfLine = false;
        output.append(indent);
      }
      output.append(data);
    }
  }

  /**
   * A Printer converts protobuf messages to the proto3 JSON format.
   */
  private static final class PrinterImpl {
    private final com.google.protobuf.TypeRegistry registry;
    private final TypeRegistry oldRegistry;
    private final ShouldPrintDefaults shouldPrintDefaults;
    private final Set<FieldDescriptor> includingDefaultValueFields;
    private final boolean preservingProtoFieldNames;
    private final boolean printingEnumsAsInts;
    private final boolean sortingMapKeys;
    private final TextGenerator generator;
    // We use Gson to help handle string escapes.
    private final Gson gson;
    private final CharSequence blankOrSpace;
    private final CharSequence blankOrNewLine;

    private static class GsonHolder {
      private static final Gson DEFAULT_GSON = new GsonBuilder().create();
      private static final Gson GSON_WITHOUT_HTML_ESCAPING =
          new GsonBuilder().disableHtmlEscaping().create();
    }

    PrinterImpl(
        com.google.protobuf.TypeRegistry registry,
        TypeRegistry oldRegistry,
        ShouldPrintDefaults shouldPrintDefaults,
        Set<FieldDescriptor> includingDefaultValueFields,
        boolean preservingProtoFieldNames,
        Appendable jsonOutput,
        boolean omittingInsignificantWhitespace,
        boolean printingEnumsAsInts,
        boolean sortingMapKeys,
        boolean unsafeDisableCodepointsForHtmlSymbols) {
      this.registry = registry;
      this.oldRegistry = oldRegistry;
      this.shouldPrintDefaults = shouldPrintDefaults;
      this.includingDefaultValueFields = includingDefaultValueFields;
      this.preservingProtoFieldNames = preservingProtoFieldNames;
      this.printingEnumsAsInts = printingEnumsAsInts;
      this.sortingMapKeys = sortingMapKeys;
      this.gson =
          unsafeDisableCodepointsForHtmlSymbols
              ? GsonHolder.GSON_WITHOUT_HTML_ESCAPING
              : GsonHolder.DEFAULT_GSON;
      // json format related properties, determined by printerType
      if (omittingInsignificantWhitespace) {
        this.generator = new CompactTextGenerator(jsonOutput);
        this.blankOrSpace = "";
        this.blankOrNewLine = "";
      } else {
        this.generator = new PrettyTextGenerator(jsonOutput);
        this.blankOrSpace = " ";
        this.blankOrNewLine = "\n";
      }
    }

    void print(MessageOrBuilder message) throws IOException {
      WellKnownTypePrinter specialPrinter =
          wellKnownTypePrinters.get(message.getDescriptorForType().getFullName());
      if (specialPrinter != null) {
        specialPrinter.print(this, message);
        return;
      }
      print(message, null);
    }

    private interface WellKnownTypePrinter {
      void print(PrinterImpl printer, MessageOrBuilder message) throws IOException;
    }

    private static final Map<String, WellKnownTypePrinter> wellKnownTypePrinters =
        buildWellKnownTypePrinters();

    private static Map<String, WellKnownTypePrinter> buildWellKnownTypePrinters() {
      Map<String, WellKnownTypePrinter> printers = new HashMap<String, WellKnownTypePrinter>();
      // Special-case Any.
      printers.put(
          Any.getDescriptor().getFullName(),
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printAny(message);
            }
          });
      // Special-case wrapper types.
      WellKnownTypePrinter wrappersPrinter =
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printWrapper(message);
            }
          };
      printers.put(BoolValue.getDescriptor().getFullName(), wrappersPrinter);
      printers.put(Int32Value.getDescriptor().getFullName(), wrappersPrinter);
      printers.put(UInt32Value.getDescriptor().getFullName(), wrappersPrinter);
      printers.put(Int64Value.getDescriptor().getFullName(), wrappersPrinter);
      printers.put(UInt64Value.getDescriptor().getFullName(), wrappersPrinter);
      printers.put(StringValue.getDescriptor().getFullName(), wrappersPrinter);
      printers.put(BytesValue.getDescriptor().getFullName(), wrappersPrinter);
      printers.put(FloatValue.getDescriptor().getFullName(), wrappersPrinter);
      printers.put(DoubleValue.getDescriptor().getFullName(), wrappersPrinter);
      // Special-case Timestamp.
      printers.put(
          Timestamp.getDescriptor().getFullName(),
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printTimestamp(message);
            }
          });
      // Special-case Duration.
      printers.put(
          Duration.getDescriptor().getFullName(),
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printDuration(message);
            }
          });
      // Special-case FieldMask.
      printers.put(
          FieldMask.getDescriptor().getFullName(),
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printFieldMask(message);
            }
          });
      // Special-case Struct.
      printers.put(
          Struct.getDescriptor().getFullName(),
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printStruct(message);
            }
          });
      // Special-case Value.
      printers.put(
          Value.getDescriptor().getFullName(),
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printValue(message);
            }
          });
      // Special-case ListValue.
      printers.put(
          ListValue.getDescriptor().getFullName(),
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printListValue(message);
            }
          });
      return printers;
    }

    /** Prints google.protobuf.Any */
    private void printAny(MessageOrBuilder message) throws IOException {
      if (message.getDefaultInstanceForType().equals(message)) {
        generator.print("{}");
        return;
      }
      Descriptor descriptor = message.getDescriptorForType();
      FieldDescriptor typeUrlField = descriptor.findFieldByName("type_url");
      FieldDescriptor valueField = descriptor.findFieldByName("value");
      // Validates type of the message. Note that we can't just cast the message
      // to com.google.protobuf.Any because it might be a DynamicMessage.
      if (typeUrlField == null
          || valueField == null
          || typeUrlField.getType() != FieldDescriptor.Type.STRING
          || valueField.getType() != FieldDescriptor.Type.BYTES) {
        throw new InvalidProtocolBufferException("Invalid Any type.");
      }
      String typeUrl = (String) message.getField(typeUrlField);
      Descriptor type = registry.getDescriptorForTypeUrl(typeUrl);
      if (type == null) {
        type = oldRegistry.getDescriptorForTypeUrl(typeUrl);
        if (type == null) {
          throw new InvalidProtocolBufferException("Cannot find type for url: " + typeUrl);
        }
      }
      ByteString content = (ByteString) message.getField(valueField);
      Message contentMessage =
          DynamicMessage.getDefaultInstance(type).getParserForType().parseFrom(content);
      WellKnownTypePrinter printer = wellKnownTypePrinters.get(getTypeName(typeUrl));
      if (printer != null) {
        // If the type is one of the well-known types, we use a special
        // formatting.
        generator.print("{" + blankOrNewLine);
        generator.indent();
        generator.print("\"@type\":" + blankOrSpace + gson.toJson(typeUrl) + "," + blankOrNewLine);
        generator.print("\"value\":" + blankOrSpace);
        printer.print(this, contentMessage);
        generator.print(blankOrNewLine);
        generator.outdent();
        generator.print("}");
      } else {
        // Print the content message instead (with a "@type" field added).
        print(contentMessage, typeUrl);
      }
    }

    /** Prints wrapper types (e.g., google.protobuf.Int32Value) */
    private void printWrapper(MessageOrBuilder message) throws IOException {
      Descriptor descriptor = message.getDescriptorForType();
      FieldDescriptor valueField = descriptor.findFieldByName("value");
      if (valueField == null) {
        throw new InvalidProtocolBufferException("Invalid Wrapper type.");
      }
      // When formatting wrapper types, we just print its value field instead of
      // the whole message.
      printSingleFieldValue(valueField, message.getField(valueField));
    }

    private ByteString toByteString(MessageOrBuilder message) {
      if (message instanceof Message) {
        return ((Message) message).toByteString();
      } else {
        return ((Message.Builder) message).build().toByteString();
      }
    }

    /** Prints google.protobuf.Timestamp */
    private void printTimestamp(MessageOrBuilder message) throws IOException {
      Timestamp value = Timestamp.parseFrom(toByteString(message));
      generator.print("\"" + Timestamps.toString(value) + "\"");
    }

    /** Prints google.protobuf.Duration */
    private void printDuration(MessageOrBuilder message) throws IOException {
      Duration value = Duration.parseFrom(toByteString(message));
      generator.print("\"" + Durations.toString(value) + "\"");
    }

    /** Prints google.protobuf.FieldMask */
    private void printFieldMask(MessageOrBuilder message) throws IOException {
      FieldMask value = FieldMask.parseFrom(toByteString(message));
      generator.print("\"" + FieldMaskUtil.toJsonString(value) + "\"");
    }

    /** Prints google.protobuf.Struct */
    private void printStruct(MessageOrBuilder message) throws IOException {
      Descriptor descriptor = message.getDescriptorForType();
      FieldDescriptor field = descriptor.findFieldByName("fields");
      if (field == null) {
        throw new InvalidProtocolBufferException("Invalid Struct type.");
      }
      // Struct is formatted as a map object.
      printMapFieldValue(field, message.getField(field));
    }

    /** Prints google.protobuf.Value */
    private void printValue(MessageOrBuilder message) throws IOException {
      // For a Value message, only the value of the field is formatted.
      Map<FieldDescriptor, Object> fields = message.getAllFields();
      if (fields.isEmpty()) {
        // No value set.
        generator.print("null");
        return;
      }
      // A Value message can only have at most one field set (it only contains
      // an oneof).
      if (fields.size() != 1) {
        throw new InvalidProtocolBufferException("Invalid Value type.");
      }
      for (Map.Entry<FieldDescriptor, Object> entry : fields.entrySet()) {
        FieldDescriptor field = entry.getKey();
        if (field.getType() == FieldDescriptor.Type.DOUBLE) {
          Double doubleValue = (Double) entry.getValue();
          if (doubleValue.isNaN() || doubleValue.isInfinite()) {
            throw new IllegalArgumentException(
                "google.protobuf.Value cannot encode double values for "
                + "infinity or nan, because they would be parsed as a string.");
          }
        }
        printSingleFieldValue(field, entry.getValue());
      }
    }

    /** Prints google.protobuf.ListValue */
    private void printListValue(MessageOrBuilder message) throws IOException {
      Descriptor descriptor = message.getDescriptorForType();
      FieldDescriptor field = descriptor.findFieldByName("values");
      if (field == null) {
        throw new InvalidProtocolBufferException("Invalid ListValue type.");
      }
      printRepeatedFieldValue(field, message.getField(field));
    }

    // Whether a set option means the corresponding field should be printed even if it normally
    // wouldn't be.
    private boolean shouldSpeciallyPrint(FieldDescriptor field) {
      switch (shouldPrintDefaults) {
        case ONLY_IF_PRESENT:
          return false;
        case ALWAYS_PRINT_EXCEPT_MESSAGES_AND_ONEOFS:
          return !field.hasPresence()
              || (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE
                  && field.getContainingOneof() == null);
        case ALWAYS_PRINT_WITHOUT_PRESENCE_FIELDS:
          return !field.hasPresence();
        case ALWAYS_PRINT_SPECIFIED_FIELDS:
          // For legacy code compatibility, we don't honor non-repeated message or oneof fields even
          // if they're explicitly requested. :(
          return !(field.getJavaType() == FieldDescriptor.JavaType.MESSAGE && !field.isRepeated())
              && field.getContainingOneof() == null
              && includingDefaultValueFields.contains(field);
      }
      throw new AssertionError("Unknown shouldPrintDefaults: " + shouldPrintDefaults);
    }

    /** Prints a regular message with an optional type URL. */
    private void print(MessageOrBuilder message, @Nullable String typeUrl) throws IOException {
      generator.print("{" + blankOrNewLine);
      generator.indent();

      boolean printedField = false;
      if (typeUrl != null) {
        generator.print("\"@type\":" + blankOrSpace + gson.toJson(typeUrl));
        printedField = true;
      }

      // message.getAllFields() will already contain all of the fields that would be
      // printed normally (non-empty repeated fields, with-presence fields that are set, implicit
      // presence fields that have a nonzero value). Loop over all of the fields to add any more
      // fields that should be printed based on the shouldPrintDefaults setting.
      Map<FieldDescriptor, Object> fieldsToPrint;
      if (shouldPrintDefaults == ShouldPrintDefaults.ONLY_IF_PRESENT) {
        fieldsToPrint = message.getAllFields();
      } else {
        fieldsToPrint = new TreeMap<FieldDescriptor, Object>(message.getAllFields());
        for (FieldDescriptor field : message.getDescriptorForType().getFields()) {
          if (shouldSpeciallyPrint(field)) {
            fieldsToPrint.put(field, message.getField(field));
          }
        }
      }

      for (Map.Entry<FieldDescriptor, Object> field : fieldsToPrint.entrySet()) {
        if (printedField) {
          // Add line-endings for the previous field.
          generator.print("," + blankOrNewLine);
        } else {
          printedField = true;
        }
        printField(field.getKey(), field.getValue());
      }

      // Add line-endings for the last field.
      if (printedField) {
        generator.print(blankOrNewLine);
      }
      generator.outdent();
      generator.print("}");
    }

    private void printField(FieldDescriptor field, Object value) throws IOException {
      if (preservingProtoFieldNames) {
        generator.print("\"" + field.getName() + "\":" + blankOrSpace);
      } else {
        generator.print("\"" + field.getJsonName() + "\":" + blankOrSpace);
      }
      if (field.isMapField()) {
        printMapFieldValue(field, value);
      } else if (field.isRepeated()) {
        printRepeatedFieldValue(field, value);
      } else {
        printSingleFieldValue(field, value);
      }
    }

    @SuppressWarnings("rawtypes")
    private void printRepeatedFieldValue(FieldDescriptor field, Object value) throws IOException {
      generator.print("[");
      boolean printedElement = false;
      for (Object element : (List) value) {
        if (printedElement) {
          generator.print("," + blankOrSpace);
        } else {
          printedElement = true;
        }
        printSingleFieldValue(field, element);
      }
      generator.print("]");
    }

    private void printMapFieldValue(FieldDescriptor field, Object value) throws IOException {
      Descriptor type = field.getMessageType();
      FieldDescriptor keyField = type.findFieldByName("key");
      FieldDescriptor valueField = type.findFieldByName("value");
      if (keyField == null || valueField == null) {
        throw new InvalidProtocolBufferException("Invalid map field.");
      }
      generator.print("{" + blankOrNewLine);
      generator.indent();

      @SuppressWarnings("unchecked") // Object guaranteed to be a List for a map field.
      Collection<Object> elements = (List<Object>) value;
      if (sortingMapKeys && !elements.isEmpty()) {
        Comparator<Object> cmp = null;
        if (keyField.getType() == FieldDescriptor.Type.STRING) {
          cmp = new Comparator<Object>() {
            @Override
            public int compare(final Object o1, final Object o2) {
              ByteString s1 = ByteString.copyFromUtf8((String) o1);
              ByteString s2 = ByteString.copyFromUtf8((String) o2);
              return ByteString.unsignedLexicographicalComparator().compare(s1, s2);
            }
          };
        }
        TreeMap<Object, Object> tm = new TreeMap<>(cmp);
        for (Object element : elements) {
          Message entry = (Message) element;
          Object entryKey = entry.getField(keyField);
          tm.put(entryKey, element);
        }
        elements = tm.values();
      }

      boolean printedElement = false;
      for (Object element : elements) {
        Message entry = (Message) element;
        Object entryKey = entry.getField(keyField);
        Object entryValue = entry.getField(valueField);
        if (printedElement) {
          generator.print("," + blankOrNewLine);
        } else {
          printedElement = true;
        }
        // Key fields are always double-quoted.
        printSingleFieldValue(keyField, entryKey, true);
        generator.print(":" + blankOrSpace);
        printSingleFieldValue(valueField, entryValue);
      }
      if (printedElement) {
        generator.print(blankOrNewLine);
      }
      generator.outdent();
      generator.print("}");
    }

    private void printSingleFieldValue(FieldDescriptor field, Object value) throws IOException {
      printSingleFieldValue(field, value, false);
    }

    /**
     * Prints a field's value in the proto3 JSON format.
     *
     * @param alwaysWithQuotes whether to always add double-quotes to primitive
     *        types
     */
    private void printSingleFieldValue(
        final FieldDescriptor field, final Object value, boolean alwaysWithQuotes)
        throws IOException {
      switch (field.getType()) {
        case INT32:
        case SINT32:
        case SFIXED32:
          if (alwaysWithQuotes) {
            generator.print("\"");
          }
          generator.print(((Integer) value).toString());
          if (alwaysWithQuotes) {
            generator.print("\"");
          }
          break;

        case INT64:
        case SINT64:
        case SFIXED64:
          generator.print("\"" + ((Long) value).toString() + "\"");
          break;

        case BOOL:
          if (alwaysWithQuotes) {
            generator.print("\"");
          }
          if (((Boolean) value).booleanValue()) {
            generator.print("true");
          } else {
            generator.print("false");
          }
          if (alwaysWithQuotes) {
            generator.print("\"");
          }
          break;

        case FLOAT:
          Float floatValue = (Float) value;
          if (floatValue.isNaN()) {
            generator.print("\"NaN\"");
          } else if (floatValue.isInfinite()) {
            if (floatValue < 0) {
              generator.print("\"-Infinity\"");
            } else {
              generator.print("\"Infinity\"");
            }
          } else {
            if (alwaysWithQuotes) {
              generator.print("\"");
            }
            generator.print(floatValue.toString());
            if (alwaysWithQuotes) {
              generator.print("\"");
            }
          }
          break;

        case DOUBLE:
          Double doubleValue = (Double) value;
          if (doubleValue.isNaN()) {
            generator.print("\"NaN\"");
          } else if (doubleValue.isInfinite()) {
            if (doubleValue < 0) {
              generator.print("\"-Infinity\"");
            } else {
              generator.print("\"Infinity\"");
            }
          } else {
            if (alwaysWithQuotes) {
              generator.print("\"");
            }
            generator.print(doubleValue.toString());
            if (alwaysWithQuotes) {
              generator.print("\"");
            }
          }
          break;

        case UINT32:
        case FIXED32:
          if (alwaysWithQuotes) {
            generator.print("\"");
          }
          generator.print(Integer.toUnsignedString((int) value));
          if (alwaysWithQuotes) {
            generator.print("\"");
          }
          break;

        case UINT64:
        case FIXED64:
          generator.print("\"" + Long.toUnsignedString((long) value) + "\"");
          break;

        case STRING:
          generator.print(gson.toJson(value));
          break;

        case BYTES:
          generator.print("\"");
          generator.print(BaseEncoding.base64().encode(((ByteString) value).toByteArray()));
          generator.print("\"");
          break;

        case ENUM:
          // Special-case google.protobuf.NullValue (it's an Enum).
          if (field.getEnumType().getFullName().equals("google.protobuf.NullValue")) {
            // No matter what value it contains, we always print it as "null".
            if (alwaysWithQuotes) {
              generator.print("\"");
            }
            generator.print("null");
            if (alwaysWithQuotes) {
              generator.print("\"");
            }
          } else {
            if (printingEnumsAsInts || ((EnumValueDescriptor) value).getIndex() == -1) {
              generator.print(String.valueOf(((EnumValueDescriptor) value).getNumber()));
            } else {
              generator.print("\"" + ((EnumValueDescriptor) value).getName() + "\"");
            }
          }
          break;

        case MESSAGE:
        case GROUP:
          print((Message) value);
          break;
      }
    }
  }

  private static String getTypeName(String typeUrl) throws InvalidProtocolBufferException {
    String[] parts = typeUrl.split("/");
    if (parts.length == 1) {
      throw new InvalidProtocolBufferException("Invalid type url found: " + typeUrl);
    }
    return parts[parts.length - 1];
  }

  private static class ParserImpl {
    private final com.google.protobuf.TypeRegistry registry;
    private final TypeRegistry oldRegistry;
    private final boolean ignoringUnknownFields;
    private final int recursionLimit;
    private int currentDepth;

    ParserImpl(
        com.google.protobuf.TypeRegistry registry,
        TypeRegistry oldRegistry,
        boolean ignoreUnknownFields,
        int recursionLimit) {
      this.registry = registry;
      this.oldRegistry = oldRegistry;
      this.ignoringUnknownFields = ignoreUnknownFields;
      this.recursionLimit = recursionLimit;
      this.currentDepth = 0;
    }

    void merge(Reader json, Message.Builder builder) throws IOException {
      try {
        JsonReader reader = new JsonReader(json);
        reader.setLenient(false);
        merge(JsonParser.parseReader(reader), builder);
      } catch (JsonIOException e) {
        // Unwrap IOException.
        if (e.getCause() instanceof IOException) {
          throw (IOException) e.getCause();
        } else {
          throw new InvalidProtocolBufferException(e.getMessage(), e);
        }
      } catch (RuntimeException e) {
        // We convert all exceptions from JSON parsing to our own exceptions.
        throw new InvalidProtocolBufferException(e.getMessage(), e);
      }
    }

    void merge(String json, Message.Builder builder) throws InvalidProtocolBufferException {
      try {
        JsonReader reader = new JsonReader(new StringReader(json));
        reader.setLenient(false);
        merge(JsonParser.parseReader(reader), builder);
      } catch (RuntimeException e) {
        // We convert all exceptions from JSON parsing to our own exceptions.
        InvalidProtocolBufferException toThrow = new InvalidProtocolBufferException(e.getMessage());
        toThrow.initCause(e);
        throw toThrow;
      }
    }

    private interface WellKnownTypeParser {
      void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
          throws InvalidProtocolBufferException;
    }

    private static final Map<String, WellKnownTypeParser> wellKnownTypeParsers =
        buildWellKnownTypeParsers();

    private static Map<String, WellKnownTypeParser> buildWellKnownTypeParsers() {
      Map<String, WellKnownTypeParser> parsers = new HashMap<String, WellKnownTypeParser>();
      // Special-case Any.
      parsers.put(
          Any.getDescriptor().getFullName(),
          new WellKnownTypeParser() {
            @Override
            public void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
                throws InvalidProtocolBufferException {
              parser.mergeAny(json, builder);
            }
          });
      // Special-case wrapper types.
      WellKnownTypeParser wrappersPrinter =
          new WellKnownTypeParser() {
            @Override
            public void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
                throws InvalidProtocolBufferException {
              parser.mergeWrapper(json, builder);
            }
          };
      parsers.put(BoolValue.getDescriptor().getFullName(), wrappersPrinter);
      parsers.put(Int32Value.getDescriptor().getFullName(), wrappersPrinter);
      parsers.put(UInt32Value.getDescriptor().getFullName(), wrappersPrinter);
      parsers.put(Int64Value.getDescriptor().getFullName(), wrappersPrinter);
      parsers.put(UInt64Value.getDescriptor().getFullName(), wrappersPrinter);
      parsers.put(StringValue.getDescriptor().getFullName(), wrappersPrinter);
      parsers.put(BytesValue.getDescriptor().getFullName(), wrappersPrinter);
      parsers.put(FloatValue.getDescriptor().getFullName(), wrappersPrinter);
      parsers.put(DoubleValue.getDescriptor().getFullName(), wrappersPrinter);
      // Special-case Timestamp.
      parsers.put(
          Timestamp.getDescriptor().getFullName(),
          new WellKnownTypeParser() {
            @Override
            public void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
                throws InvalidProtocolBufferException {
              parser.mergeTimestamp(json, builder);
            }
          });
      // Special-case Duration.
      parsers.put(
          Duration.getDescriptor().getFullName(),
          new WellKnownTypeParser() {
            @Override
            public void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
                throws InvalidProtocolBufferException {
              parser.mergeDuration(json, builder);
            }
          });
      // Special-case FieldMask.
      parsers.put(
          FieldMask.getDescriptor().getFullName(),
          new WellKnownTypeParser() {
            @Override
            public void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
                throws InvalidProtocolBufferException {
              parser.mergeFieldMask(json, builder);
            }
          });
      // Special-case Struct.
      parsers.put(
          Struct.getDescriptor().getFullName(),
          new WellKnownTypeParser() {
            @Override
            public void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
                throws InvalidProtocolBufferException {
              parser.mergeStruct(json, builder);
            }
          });
      // Special-case ListValue.
      parsers.put(
          ListValue.getDescriptor().getFullName(),
          new WellKnownTypeParser() {
            @Override
            public void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
                throws InvalidProtocolBufferException {
              parser.mergeListValue(json, builder);
            }
          });
      // Special-case Value.
      parsers.put(
          Value.getDescriptor().getFullName(),
          new WellKnownTypeParser() {
            @Override
            public void merge(ParserImpl parser, JsonElement json, Message.Builder builder)
                throws InvalidProtocolBufferException {
              parser.mergeValue(json, builder);
            }
          });
      return parsers;
    }

    private void merge(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      WellKnownTypeParser specialParser =
          wellKnownTypeParsers.get(builder.getDescriptorForType().getFullName());
      if (specialParser != null) {
        specialParser.merge(this, json, builder);
        return;
      }
      mergeMessage(json, builder, false);
    }

    // Maps from camel-case field names to FieldDescriptor.
    private final Map<Descriptor, Map<String, FieldDescriptor>> fieldNameMaps =
        new HashMap<Descriptor, Map<String, FieldDescriptor>>();

    private Map<String, FieldDescriptor> getFieldNameMap(Descriptor descriptor) {
      if (!fieldNameMaps.containsKey(descriptor)) {
        Map<String, FieldDescriptor> fieldNameMap = new HashMap<String, FieldDescriptor>();
        for (FieldDescriptor field : descriptor.getFields()) {
          fieldNameMap.put(field.getName(), field);
          fieldNameMap.put(field.getJsonName(), field);
        }
        fieldNameMaps.put(descriptor, fieldNameMap);
        return fieldNameMap;
      }
      return fieldNameMaps.get(descriptor);
    }

    private void mergeMessage(JsonElement json, Message.Builder builder, boolean skipTypeUrl)
        throws InvalidProtocolBufferException {
      if (!(json instanceof JsonObject)) {
        throw new InvalidProtocolBufferException("Expect message object but got: " + json);
      }
      JsonObject object = (JsonObject) json;
      Map<String, FieldDescriptor> fieldNameMap = getFieldNameMap(builder.getDescriptorForType());
      for (Map.Entry<String, JsonElement> entry : object.entrySet()) {
        if (skipTypeUrl && entry.getKey().equals("@type")) {
          continue;
        }
        FieldDescriptor field = fieldNameMap.get(entry.getKey());
        if (field == null) {
          if (ignoringUnknownFields) {
            continue;
          }
          throw new InvalidProtocolBufferException(
              "Cannot find field: "
                  + entry.getKey()
                  + " in message "
                  + builder.getDescriptorForType().getFullName());
        }
        mergeField(field, entry.getValue(), builder);
      }
    }

    private void mergeAny(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      Descriptor descriptor = builder.getDescriptorForType();
      FieldDescriptor typeUrlField = descriptor.findFieldByName("type_url");
      FieldDescriptor valueField = descriptor.findFieldByName("value");
      // Validates type of the message. Note that we can't just cast the message
      // to com.google.protobuf.Any because it might be a DynamicMessage.
      if (typeUrlField == null
          || valueField == null
          || typeUrlField.getType() != FieldDescriptor.Type.STRING
          || valueField.getType() != FieldDescriptor.Type.BYTES) {
        throw new InvalidProtocolBufferException("Invalid Any type.");
      }

      if (!(json instanceof JsonObject)) {
        throw new InvalidProtocolBufferException("Expect message object but got: " + json);
      }
      JsonObject object = (JsonObject) json;
      if (object.entrySet().isEmpty()) {
        return; // builder never modified, so it will end up building the default instance of Any
      }
      JsonElement typeUrlElement = object.get("@type");
      if (typeUrlElement == null) {
        throw new InvalidProtocolBufferException("Missing type url when parsing: " + json);
      }
      String typeUrl = typeUrlElement.getAsString();
      Descriptor contentType = registry.getDescriptorForTypeUrl(typeUrl);
      if (contentType == null) {
        contentType = oldRegistry.getDescriptorForTypeUrl(typeUrl);
        if (contentType == null) {
          throw new InvalidProtocolBufferException("Cannot resolve type: " + typeUrl);
        }
      }
      builder.setField(typeUrlField, typeUrl);
      Message.Builder contentBuilder =
          DynamicMessage.getDefaultInstance(contentType).newBuilderForType();
      WellKnownTypeParser specialParser = wellKnownTypeParsers.get(contentType.getFullName());
      if (specialParser != null) {
        JsonElement value = object.get("value");
        if (value != null) {
          specialParser.merge(this, value, contentBuilder);
        }
      } else {
        mergeMessage(json, contentBuilder, true);
      }
      builder.setField(valueField, contentBuilder.build().toByteString());
    }

    private void mergeFieldMask(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      FieldMask value = FieldMaskUtil.fromJsonString(json.getAsString());
      builder.mergeFrom(value.toByteString());
    }

    private void mergeTimestamp(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      try {
        Timestamp value = Timestamps.parse(json.getAsString());
        builder.mergeFrom(value.toByteString());
      } catch (ParseException | UnsupportedOperationException e) {
        InvalidProtocolBufferException ex = new InvalidProtocolBufferException(
            "Failed to parse timestamp: " + json);
        ex.initCause(e);
        throw ex;
      }
    }

    private void mergeDuration(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      try {
        Duration value = Durations.parse(json.getAsString());
        builder.mergeFrom(value.toByteString());
      } catch (ParseException | UnsupportedOperationException e) {
        InvalidProtocolBufferException ex = new InvalidProtocolBufferException(
            "Failed to parse duration: " + json);
        ex.initCause(e);
        throw ex;
      }
    }

    private void mergeStruct(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      Descriptor descriptor = builder.getDescriptorForType();
      FieldDescriptor field = descriptor.findFieldByName("fields");
      if (field == null) {
        throw new InvalidProtocolBufferException("Invalid Struct type.");
      }
      mergeMapField(field, json, builder);
    }

    private void mergeListValue(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      Descriptor descriptor = builder.getDescriptorForType();
      FieldDescriptor field = descriptor.findFieldByName("values");
      if (field == null) {
        throw new InvalidProtocolBufferException("Invalid ListValue type.");
      }
      mergeRepeatedField(field, json, builder);
    }

    private void mergeValue(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      Descriptor type = builder.getDescriptorForType();
      if (json instanceof JsonPrimitive) {
        JsonPrimitive primitive = (JsonPrimitive) json;
        if (primitive.isBoolean()) {
          builder.setField(type.findFieldByName("bool_value"), primitive.getAsBoolean());
        } else if (primitive.isNumber()) {
          builder.setField(type.findFieldByName("number_value"), primitive.getAsDouble());
        } else {
          builder.setField(type.findFieldByName("string_value"), primitive.getAsString());
        }
      } else if (json instanceof JsonObject) {
        FieldDescriptor field = type.findFieldByName("struct_value");
        Message.Builder structBuilder = builder.newBuilderForField(field);
        merge(json, structBuilder);
        builder.setField(field, structBuilder.build());
      } else if (json instanceof JsonArray) {
        FieldDescriptor field = type.findFieldByName("list_value");
        Message.Builder listBuilder = builder.newBuilderForField(field);
        merge(json, listBuilder);
        builder.setField(field, listBuilder.build());
      } else if (json instanceof JsonNull) {
        builder.setField(
            type.findFieldByName("null_value"), NullValue.NULL_VALUE.getValueDescriptor());
      } else {
        throw new IllegalStateException("Unexpected json data: " + json);
      }
    }

    private void mergeWrapper(JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      Descriptor type = builder.getDescriptorForType();
      FieldDescriptor field = type.findFieldByName("value");
      if (field == null) {
        throw new InvalidProtocolBufferException("Invalid wrapper type: " + type.getFullName());
      }
      builder.setField(field, parseFieldValue(field, json, builder));
    }

    private void mergeField(FieldDescriptor field, JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      if (field.isRepeated()) {
        if (builder.getRepeatedFieldCount(field) > 0) {
          throw new InvalidProtocolBufferException(
              "Field " + field.getFullName() + " has already been set.");
        }
      } else {
        if (builder.hasField(field)) {
          throw new InvalidProtocolBufferException(
              "Field " + field.getFullName() + " has already been set.");
        }
      }
      if (field.isRepeated() && json instanceof JsonNull) {
        // We allow "null" as value for all field types and treat it as if the
        // field is not present.
        return;
      }
      if (field.isMapField()) {
        mergeMapField(field, json, builder);
      } else if (field.isRepeated()) {
        mergeRepeatedField(field, json, builder);
      } else if (field.getContainingOneof() != null) {
        mergeOneofField(field, json, builder);
      } else {
        Object value = parseFieldValue(field, json, builder);
        if (value != null) {
          // A field interpreted as "null" is means it's treated as absent.
          builder.setField(field, value);
        }
      }
    }

    private void mergeMapField(FieldDescriptor field, JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      if (!(json instanceof JsonObject)) {
        throw new InvalidProtocolBufferException("Expect a map object but found: " + json);
      }
      Descriptor type = field.getMessageType();
      FieldDescriptor keyField = type.findFieldByName("key");
      FieldDescriptor valueField = type.findFieldByName("value");
      if (keyField == null || valueField == null) {
        throw new InvalidProtocolBufferException("Invalid map field: " + field.getFullName());
      }
      JsonObject object = (JsonObject) json;
      for (Map.Entry<String, JsonElement> entry : object.entrySet()) {
        Message.Builder entryBuilder = builder.newBuilderForField(field);
        Object key = parseFieldValue(keyField, new JsonPrimitive(entry.getKey()), entryBuilder);
        Object value = parseFieldValue(valueField, entry.getValue(), entryBuilder);
        if (value == null) {
          if (ignoringUnknownFields && valueField.getType() == FieldDescriptor.Type.ENUM) {
            continue;
          } else {
            throw new InvalidProtocolBufferException("Map value cannot be null.");
          }
        }
        entryBuilder.setField(keyField, key);
        entryBuilder.setField(valueField, value);
        builder.addRepeatedField(field, entryBuilder.build());
      }
    }

    private void mergeOneofField(FieldDescriptor field, JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      Object value = parseFieldValue(field, json, builder);
      if (value == null) {
        // A field interpreted as "null" is means it's treated as absent.
        return;
      }
      if (builder.getOneofFieldDescriptor(field.getContainingOneof()) != null) {
        throw new InvalidProtocolBufferException(
            "Cannot set field "
                + field.getFullName()
                + " because another field "
                + builder.getOneofFieldDescriptor(field.getContainingOneof()).getFullName()
                + " belonging to the same oneof has already been set ");
      }
      builder.setField(field, value);
    }

    private void mergeRepeatedField(
        FieldDescriptor field, JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      if (!(json instanceof JsonArray)) {
        throw new InvalidProtocolBufferException(
            "Expected an array for " + field.getName() + " but found " + json);
      }
      JsonArray array = (JsonArray) json;
      for (int i = 0; i < array.size(); ++i) {
        Object value = parseFieldValue(field, array.get(i), builder);
        if (value == null) {
          if (ignoringUnknownFields && field.getType() == FieldDescriptor.Type.ENUM) {
            continue;
          } else {
            throw new InvalidProtocolBufferException(
                "Repeated field elements cannot be null in field: " + field.getFullName());
          }
        }
        builder.addRepeatedField(field, value);
      }
    }

    private int parseInt32(JsonElement json) throws InvalidProtocolBufferException {
      try {
        return Integer.parseInt(json.getAsString());
      } catch (RuntimeException e) {
        // Fall through.
      }
      // JSON doesn't distinguish between integer values and floating point values so "1" and
      // "1.000" are treated as equal in JSON. For this reason we accept floating point values for
      // integer fields as well as long as it actually is an integer (i.e., round(value) == value).
      try {
        BigDecimal value = new BigDecimal(json.getAsString());
        return value.intValueExact();
      } catch (RuntimeException e) {
        InvalidProtocolBufferException ex = new InvalidProtocolBufferException(
            "Not an int32 value: " + json);
        ex.initCause(e);
        throw ex;
      }
    }

    private long parseInt64(JsonElement json) throws InvalidProtocolBufferException {
      try {
        return Long.parseLong(json.getAsString());
      } catch (RuntimeException e) {
        // Fall through.
      }
      // JSON doesn't distinguish between integer values and floating point values so "1" and
      // "1.000" are treated as equal in JSON. For this reason we accept floating point values for
      // integer fields as well as long as it actually is an integer (i.e., round(value) == value).
      try {
        BigDecimal value = new BigDecimal(json.getAsString());
        return value.longValueExact();
      } catch (RuntimeException e) {
        InvalidProtocolBufferException ex = new InvalidProtocolBufferException(
            "Not an int64 value: " + json);
        ex.initCause(e);
        throw ex;
      }
    }

    private int parseUint32(JsonElement json) throws InvalidProtocolBufferException {
      try {
        long result = Long.parseLong(json.getAsString());
        if (result < 0 || result > 0xFFFFFFFFL) {
          throw new InvalidProtocolBufferException("Out of range uint32 value: " + json);
        }
        return (int) result;
      } catch (RuntimeException e) {
        // Fall through.
      }
      // JSON doesn't distinguish between integer values and floating point values so "1" and
      // "1.000" are treated as equal in JSON. For this reason we accept floating point values for
      // integer fields as well as long as it actually is an integer (i.e., round(value) == value).
      try {
        BigDecimal decimalValue = new BigDecimal(json.getAsString());
        BigInteger value = decimalValue.toBigIntegerExact();
        if (value.signum() < 0 || value.compareTo(MAX_UINT32) > 0) {
          throw new InvalidProtocolBufferException("Out of range uint32 value: " + json);
        }
        return value.intValue();
      } catch (RuntimeException e) {
        InvalidProtocolBufferException ex = new InvalidProtocolBufferException(
            "Not an uint32 value: " + json);
        ex.initCause(e);
        throw ex;
      }
    }

    private static final BigInteger MAX_UINT32 = new BigInteger("FFFFFFFF", 16);
    private static final BigInteger MAX_UINT64 = new BigInteger("FFFFFFFFFFFFFFFF", 16);

    private long parseUint64(JsonElement json) throws InvalidProtocolBufferException {
      try {
        BigDecimal decimalValue = new BigDecimal(json.getAsString());
        BigInteger value = decimalValue.toBigIntegerExact();
        if (value.compareTo(BigInteger.ZERO) < 0 || value.compareTo(MAX_UINT64) > 0) {
          throw new InvalidProtocolBufferException("Out of range uint64 value: " + json);
        }
        return value.longValue();
      } catch (RuntimeException e) {
        InvalidProtocolBufferException ex = new InvalidProtocolBufferException(
            "Not an uint64 value: " + json);
        ex.initCause(e);
        throw ex;
      }
    }

    private boolean parseBool(JsonElement json) throws InvalidProtocolBufferException {
      if (json.getAsString().equals("true")) {
        return true;
      }
      if (json.getAsString().equals("false")) {
        return false;
      }
      throw new InvalidProtocolBufferException("Invalid bool value: " + json);
    }

    private static final double EPSILON = 1e-6;

    private float parseFloat(JsonElement json) throws InvalidProtocolBufferException {
      if (json.getAsString().equals("NaN")) {
        return Float.NaN;
      } else if (json.getAsString().equals("Infinity")) {
        return Float.POSITIVE_INFINITY;
      } else if (json.getAsString().equals("-Infinity")) {
        return Float.NEGATIVE_INFINITY;
      }
      try {
        // We don't use Float.parseFloat() here because that function simply
        // accepts all double values. Here we parse the value into a Double
        // and do explicit range check on it.
        double value = Double.parseDouble(json.getAsString());
        // When a float value is printed, the printed value might be a little
        // larger or smaller due to precision loss. Here we need to add a bit
        // of tolerance when checking whether the float value is in range.
        if (value > Float.MAX_VALUE * (1.0 + EPSILON)
            || value < -Float.MAX_VALUE * (1.0 + EPSILON)) {
          throw new InvalidProtocolBufferException("Out of range float value: " + json);
        }
        return (float) value;
      } catch (RuntimeException e) {
        InvalidProtocolBufferException ex = new InvalidProtocolBufferException(
            "Not a float value: " + json);
        ex.initCause(e);
        throw e;
      }
    }

    private static final BigDecimal MORE_THAN_ONE = new BigDecimal(String.valueOf(1.0 + EPSILON));
    // When a float value is printed, the printed value might be a little
    // larger or smaller due to precision loss. Here we need to add a bit
    // of tolerance when checking whether the float value is in range.
    private static final BigDecimal MAX_DOUBLE =
        new BigDecimal(String.valueOf(Double.MAX_VALUE)).multiply(MORE_THAN_ONE);
    private static final BigDecimal MIN_DOUBLE =
        new BigDecimal(String.valueOf(-Double.MAX_VALUE)).multiply(MORE_THAN_ONE);

    private double parseDouble(JsonElement json) throws InvalidProtocolBufferException {
      if (json.getAsString().equals("NaN")) {
        return Double.NaN;
      } else if (json.getAsString().equals("Infinity")) {
        return Double.POSITIVE_INFINITY;
      } else if (json.getAsString().equals("-Infinity")) {
        return Double.NEGATIVE_INFINITY;
      }
      try {
        // We don't use Double.parseDouble() here because that function simply
        // accepts all values. Here we parse the value into a BigDecimal and do
        // explicit range check on it.
        BigDecimal value = new BigDecimal(json.getAsString());
        if (value.compareTo(MAX_DOUBLE) > 0 || value.compareTo(MIN_DOUBLE) < 0) {
          throw new InvalidProtocolBufferException("Out of range double value: " + json);
        }
        return value.doubleValue();
      } catch (RuntimeException e) {
        InvalidProtocolBufferException ex = new InvalidProtocolBufferException(
            "Not a double value: " + json);
        ex.initCause(e);
        throw ex;
      }
    }

    private String parseString(JsonElement json) {
      return json.getAsString();
    }

    private ByteString parseBytes(JsonElement json) {
      try {
        return ByteString.copyFrom(BaseEncoding.base64().decode(json.getAsString()));
      } catch (IllegalArgumentException e) {
        return ByteString.copyFrom(BaseEncoding.base64Url().decode(json.getAsString()));
      }
    }

    @Nullable
    private EnumValueDescriptor parseEnum(EnumDescriptor enumDescriptor, JsonElement json)
        throws InvalidProtocolBufferException {
      String value = json.getAsString();
      EnumValueDescriptor result = enumDescriptor.findValueByName(value);
      if (result == null) {
        // Try to interpret the value as a number.
        try {
          int numericValue = parseInt32(json);
          if (enumDescriptor.isClosed()) {
            result = enumDescriptor.findValueByNumber(numericValue);
          } else {
            result = enumDescriptor.findValueByNumberCreatingIfUnknown(numericValue);
          }
        } catch (InvalidProtocolBufferException e) {
          // Fall through. This exception is about invalid int32 value we get from parseInt32() but
          // that's not the exception we want the user to see. Since result == null, we will throw
          // an exception later.
        }

        // todo(elharo): if we are ignoring unknown fields, shouldn't we still
        // throw InvalidProtocolBufferException for a non-numeric value here?
        if (result == null && !ignoringUnknownFields) {
          throw new InvalidProtocolBufferException(
              "Invalid enum value: " + value + " for enum type: " + enumDescriptor.getFullName());
        }
      }
      return result;
    }

    @Nullable
    private Object parseFieldValue(FieldDescriptor field, JsonElement json, Message.Builder builder)
        throws InvalidProtocolBufferException {
      if (json instanceof JsonNull) {
        if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE
            && field.getMessageType().getFullName().equals(Value.getDescriptor().getFullName())) {
          // For every other type, "null" means absence, but for the special
          // Value message, it means the "null_value" field has been set.
          Value value = Value.newBuilder().setNullValueValue(0).build();
          return builder.newBuilderForField(field).mergeFrom(value.toByteString()).build();
        } else if (field.getJavaType() == FieldDescriptor.JavaType.ENUM
            && field.getEnumType().getFullName().equals(NullValue.getDescriptor().getFullName())) {
          // If the type of the field is a NullValue, then the value should be explicitly set.
          return field.getEnumType().findValueByNumber(0);
        }
        return null;
      } else if (json instanceof JsonObject) {
        if (field.getType() != FieldDescriptor.Type.MESSAGE
            && field.getType() != FieldDescriptor.Type.GROUP) {
          // If the field type is primitive, but the json type is JsonObject rather than
          // JsonElement, throw a type mismatch error.
          throw new InvalidProtocolBufferException(
              String.format("Invalid value: %s for expected type: %s", json, field.getType()));
        }
      }
      switch (field.getType()) {
        case INT32:
        case SINT32:
        case SFIXED32:
          return parseInt32(json);

        case INT64:
        case SINT64:
        case SFIXED64:
          return parseInt64(json);

        case BOOL:
          return parseBool(json);

        case FLOAT:
          return parseFloat(json);

        case DOUBLE:
          return parseDouble(json);

        case UINT32:
        case FIXED32:
          return parseUint32(json);

        case UINT64:
        case FIXED64:
          return parseUint64(json);

        case STRING:
          return parseString(json);

        case BYTES:
          return parseBytes(json);

        case ENUM:
          return parseEnum(field.getEnumType(), json);

        case MESSAGE:
        case GROUP:
          if (currentDepth >= recursionLimit) {
            throw new InvalidProtocolBufferException("Hit recursion limit.");
          }
          ++currentDepth;
          Message.Builder subBuilder = builder.newBuilderForField(field);
          merge(json, subBuilder);
          --currentDepth;
          return subBuilder.build();

        default:
          throw new InvalidProtocolBufferException("Invalid field type: " + field.getType());
      }
    }
  }
}
