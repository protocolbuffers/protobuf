// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.Edition;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumOptions;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueOptions;
import com.google.protobuf.DescriptorProtos.FeatureSet;
import com.google.protobuf.DescriptorProtos.FeatureSetDefaults;
import com.google.protobuf.DescriptorProtos.FeatureSetDefaults.FeatureSetEditionDefault;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import com.google.protobuf.DescriptorProtos.MethodDescriptorProto;
import com.google.protobuf.DescriptorProtos.MethodOptions;
import com.google.protobuf.DescriptorProtos.OneofDescriptorProto;
import com.google.protobuf.DescriptorProtos.OneofOptions;
import com.google.protobuf.DescriptorProtos.ServiceDescriptorProto;
import com.google.protobuf.DescriptorProtos.ServiceOptions;
import com.google.protobuf.JavaFeaturesProto.JavaFeatures;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.ToIntFunction;
import java.util.logging.Logger;

/**
 * Contains a collection of classes which describe protocol message types.
 *
 * <p>Every message type has a {@link Descriptor}, which lists all its fields and other information
 * about a type. You can get a message type's descriptor by calling {@code
 * MessageType.getDescriptor()}, or (given a message object of the type) {@code
 * message.getDescriptorForType()}. Furthermore, each message is associated with a {@link
 * FileDescriptor} for a relevant {@code .proto} file. You can obtain it by calling {@code
 * Descriptor.getFile()}. A {@link FileDescriptor} contains descriptors for all the messages defined
 * in that file, and file descriptors for all the imported {@code .proto} files.
 *
 * <p>Descriptors are built from DescriptorProtos, as defined in {@code
 * google/protobuf/descriptor.proto}.
 *
 * @author kenton@google.com Kenton Varda
 */
@CheckReturnValue
public final class Descriptors {
  private static final Logger logger = Logger.getLogger(Descriptors.class.getName());
  private static final int[] EMPTY_INT_ARRAY = new int[0];
  private static final Descriptor[] EMPTY_DESCRIPTORS = new Descriptor[0];
  private static final FieldDescriptor[] EMPTY_FIELD_DESCRIPTORS = new FieldDescriptor[0];
  private static final EnumDescriptor[] EMPTY_ENUM_DESCRIPTORS = new EnumDescriptor[0];
  private static final ServiceDescriptor[] EMPTY_SERVICE_DESCRIPTORS = new ServiceDescriptor[0];
  private static final OneofDescriptor[] EMPTY_ONEOF_DESCRIPTORS = new OneofDescriptor[0];
  private static final ConcurrentHashMap<FeatureSet, FeatureSet> FEATURE_CACHE =
      new ConcurrentHashMap<>();

  @SuppressWarnings("NonFinalStaticField")
  private static volatile FeatureSetDefaults javaEditionDefaults = null;

  /** Sets the default feature mappings used during the build. Exposed for tests. */
  static void setTestJavaEditionDefaults(FeatureSetDefaults defaults) {
    javaEditionDefaults = defaults;
  }

  /** Gets the default feature mappings used during the build. */
  static FeatureSetDefaults getJavaEditionDefaults() {
    // Force explicit initialization before synchronized block which can trigger initialization in
    // `JavaFeaturesProto.registerAllExtensions()` and `FeatureSetdefaults.parseFrom()` calls.
    // Otherwise, this can result in deadlock if another threads holds the static init block's
    // implicit lock. This operation should be cheap if initialization has already occurred.
    Descriptor unused1 = FeatureSetDefaults.getDescriptor();
    FileDescriptor unused2 = JavaFeaturesProto.getDescriptor();
    if (javaEditionDefaults == null) {
      synchronized (Descriptors.class) {
        if (javaEditionDefaults == null) {
          try {
            ExtensionRegistry registry = ExtensionRegistry.newInstance();
            registry.add(JavaFeaturesProto.java_);
            setTestJavaEditionDefaults(
                FeatureSetDefaults.parseFrom(
                    JavaEditionDefaults.PROTOBUF_INTERNAL_JAVA_EDITION_DEFAULTS.getBytes(
                        Internal.ISO_8859_1),
                    registry));
          } catch (Exception e) {
            throw new AssertionError(e);
          }
        }
      }
    }
    return javaEditionDefaults;
  }

  static FeatureSet getEditionDefaults(Edition edition) {
    FeatureSetDefaults javaEditionDefaults = getJavaEditionDefaults();
    if (edition.getNumber() < javaEditionDefaults.getMinimumEdition().getNumber()) {
      throw new IllegalArgumentException(
          "Edition "
              + edition
              + " is lower than the minimum supported edition "
              + javaEditionDefaults.getMinimumEdition()
              + "!");
    }
    if (edition.getNumber() > javaEditionDefaults.getMaximumEdition().getNumber()) {
      throw new IllegalArgumentException(
          "Edition "
              + edition
              + " is greater than the maximum supported edition "
              + javaEditionDefaults.getMaximumEdition()
              + "!");
    }
    FeatureSetEditionDefault found = null;
    for (FeatureSetEditionDefault editionDefault : javaEditionDefaults.getDefaultsList()) {
      if (editionDefault.getEdition().getNumber() > edition.getNumber()) {
        break;
      }
      found = editionDefault;
    }
    if (found == null) {
      throw new IllegalArgumentException(
          "Edition " + edition + " does not have a valid default FeatureSet!");
    }
    return found.getFixedFeatures().toBuilder().mergeFrom(found.getOverridableFeatures()).build();
  }

  private static FeatureSet internFeatures(FeatureSet features) {
    FeatureSet cached = FEATURE_CACHE.putIfAbsent(features, features);
    if (cached == null) {
      return features;
    }
    return cached;
  }

  /**
   * Describes a {@code .proto} file, including everything defined within. That includes, in
   * particular, descriptors for all the messages and file descriptors for all other imported {@code
   * .proto} files (dependencies).
   */
  public static final class FileDescriptor extends GenericDescriptor {
    /** Convert the descriptor to its protocol message representation. */
    @Override
    public FileDescriptorProto toProto() {
      return proto;
    }

    /** Get the file name. */
    @Override
    public String getName() {
      return proto.getName();
    }

    /** Returns this object. */
    @Override
    public FileDescriptor getFile() {
      return this;
    }

    @Override
    GenericDescriptor getParent() {
      return null;
    }

    public boolean isPlaceholder() {
      return placeholder;
    }

    /** Returns the same as getName(). */
    @Override
    public String getFullName() {
      return proto.getName();
    }

    /**
     * Get the proto package name. This is the package name given by the {@code package} statement
     * in the {@code .proto} file, which differs from the Java package.
     */
    public String getPackage() {
      return proto.getPackage();
    }

    /** Get the {@code FileOptions}, defined in {@code descriptor.proto}. */
    public FileOptions getOptions() {
      if (this.options == null) {
        FileOptions strippedOptions = this.proto.getOptions();
        if (strippedOptions.hasFeatures()) {
          // Clients should be using feature accessor methods, not accessing features on the
          // options
          // proto.
          strippedOptions = strippedOptions.toBuilder().clearFeatures().build();
        }
        synchronized (this) {
          if (this.options == null) {
            this.options = strippedOptions;
          }
        }
      }
      return this.options;
    }

    /** Get a list of top-level message types declared in this file. */
    public List<Descriptor> getMessageTypes() {
      return Collections.unmodifiableList(Arrays.asList(messageTypes));
    }

    public int getMessageTypeCount() {
      return messageTypes.length;
    }

    public Descriptor getMessageType(int index) {
      return messageTypes[index];
    }

    /** Get a list of top-level enum types declared in this file. */
    public List<EnumDescriptor> getEnumTypes() {
      return Collections.unmodifiableList(Arrays.asList(enumTypes));
    }

    public int getEnumTypeCount() {
      return enumTypes.length;
    }

    public EnumDescriptor getEnumType(int index) {
      return enumTypes[index];
    }

    /** Get a list of top-level services declared in this file. */
    public List<ServiceDescriptor> getServices() {
      return Collections.unmodifiableList(Arrays.asList(services));
    }

    public int getServiceCount() {
      return services.length;
    }

    public ServiceDescriptor getService(int index) {
      return services[index];
    }

    /** Get a list of top-level extensions declared in this file. */
    public List<FieldDescriptor> getExtensions() {
      return Collections.unmodifiableList(Arrays.asList(extensions));
    }

    public int getExtensionCount() {
      return extensions.length;
    }

    public FieldDescriptor getExtension(int index) {
      return extensions[index];
    }

    /** Get a list of this file's dependencies (imports). */
    public List<FileDescriptor> getDependencies() {
      return Collections.unmodifiableList(Arrays.asList(dependencies));
    }

    /** Get a list of this file's public dependencies (public imports). */
    public List<FileDescriptor> getPublicDependencies() {
      return Collections.unmodifiableList(Arrays.asList(publicDependencies));
    }

    /** Get the edition of the .proto file. */
    Edition getEdition() {
      switch (proto.getSyntax()) {
        case "editions":
          return proto.getEdition();
        case "proto3":
          return Edition.EDITION_PROTO3;
        default:
          return Edition.EDITION_PROTO2;
      }
    }

    public void copyHeadingTo(FileDescriptorProto.Builder protoBuilder) {
      protoBuilder.setName(getName()).setSyntax(proto.getSyntax());
      if (!getPackage().isEmpty()) {
        protoBuilder.setPackage(getPackage());
      }
      if (proto.getSyntax().equals("editions")) {
        protoBuilder.setEdition(proto.getEdition());
      }
      if (proto.hasOptions() && !proto.getOptions().equals(FileOptions.getDefaultInstance())) {
        protoBuilder.setOptions(proto.getOptions());
      }
    }

    /**
     * Find a message type in the file by name. Does not find nested types.
     *
     * @param name The unqualified type name to look for.
     * @return The message type's descriptor, or {@code null} if not found.
     */
    public Descriptor findMessageTypeByName(String name) {
      // Don't allow looking up nested types.  This will make optimization
      // easier later.
      if (name.indexOf('.') != -1) {
        return null;
      }
      final String packageName = getPackage();
      if (!packageName.isEmpty()) {
        name = packageName + '.' + name;
      }
      final GenericDescriptor result = tables.findSymbol(name);
      if (result instanceof Descriptor && result.getFile() == this) {
        return (Descriptor) result;
      } else {
        return null;
      }
    }

    /**
     * Find an enum type in the file by name. Does not find nested types.
     *
     * @param name The unqualified type name to look for.
     * @return The enum type's descriptor, or {@code null} if not found.
     */
    public EnumDescriptor findEnumTypeByName(String name) {
      // Don't allow looking up nested types.  This will make optimization
      // easier later.
      if (name.indexOf('.') != -1) {
        return null;
      }
      final String packageName = getPackage();
      if (!packageName.isEmpty()) {
        name = packageName + '.' + name;
      }
      final GenericDescriptor result = tables.findSymbol(name);
      if (result instanceof EnumDescriptor && result.getFile() == this) {
        return (EnumDescriptor) result;
      } else {
        return null;
      }
    }

    /**
     * Find a service type in the file by name.
     *
     * @param name The unqualified type name to look for.
     * @return The service type's descriptor, or {@code null} if not found.
     */
    public ServiceDescriptor findServiceByName(String name) {
      // Don't allow looking up nested types.  This will make optimization
      // easier later.
      if (name.indexOf('.') != -1) {
        return null;
      }
      final String packageName = getPackage();
      if (!packageName.isEmpty()) {
        name = packageName + '.' + name;
      }
      final GenericDescriptor result = tables.findSymbol(name);
      if (result instanceof ServiceDescriptor && result.getFile() == this) {
        return (ServiceDescriptor) result;
      } else {
        return null;
      }
    }

    /**
     * Find an extension in the file by name. Does not find extensions nested inside message types.
     *
     * @param name The unqualified extension name to look for.
     * @return The extension's descriptor, or {@code null} if not found.
     */
    public FieldDescriptor findExtensionByName(String name) {
      if (name.indexOf('.') != -1) {
        return null;
      }
      final String packageName = getPackage();
      if (!packageName.isEmpty()) {
        name = packageName + '.' + name;
      }
      final GenericDescriptor result = tables.findSymbol(name);
      if (result instanceof FieldDescriptor && result.getFile() == this) {
        return (FieldDescriptor) result;
      } else {
        return null;
      }
    }

    /**
     * Construct a {@code FileDescriptor}.
     *
     * @param proto the protocol message form of the FileDescriptort
     * @param dependencies {@code FileDescriptor}s corresponding to all of the file's dependencies.
     * @throws DescriptorValidationException {@code proto} is not a valid descriptor. This can occur
     *     for a number of reasons; for instance, because a field has an undefined type or because
     *     two messages were defined with the same name.
     */
    public static FileDescriptor buildFrom(FileDescriptorProto proto, FileDescriptor[] dependencies)
        throws DescriptorValidationException {
      return buildFrom(proto, dependencies, false);
    }

    /**
     * Construct a {@code FileDescriptor}.
     *
     * @param proto the protocol message form of the FileDescriptor
     * @param dependencies {@code FileDescriptor}s corresponding to all of the file's dependencies
     * @param allowUnknownDependencies if true, non-existing dependencies will be ignored and
     *     undefined message types will be replaced with a placeholder type. Undefined enum types
     *     still cause a DescriptorValidationException.
     * @throws DescriptorValidationException {@code proto} is not a valid descriptor. This can occur
     *     for a number of reasons; for instance, because a field has an undefined type or because
     *     two messages were defined with the same name.
     */
    public static FileDescriptor buildFrom(
        FileDescriptorProto proto, FileDescriptor[] dependencies, boolean allowUnknownDependencies)
        throws DescriptorValidationException {
      return buildFrom(proto, dependencies, allowUnknownDependencies, false);
    }

    private static FileDescriptor buildFrom(
        FileDescriptorProto proto,
        FileDescriptor[] dependencies,
        boolean allowUnknownDependencies,
        boolean allowUnresolvedFeatures)
        throws DescriptorValidationException {
      // Building descriptors involves two steps:  translating and linking.
      // In the translation step (implemented by FileDescriptor's
      // constructor), we build an object tree mirroring the
      // FileDescriptorProto's tree and put all of the descriptors into the
      // FileDescriptorTables's lookup tables.  In the linking step, we look up all
      // type references in the FileDescriptorTables, so that, for example, a
      // FieldDescriptor for an embedded message contains a pointer directly
      // to the Descriptor for that message's type.  We also detect undefined
      // types in the linking step.
      FileDescriptorTables tables =
          new FileDescriptorTables(dependencies, allowUnknownDependencies);
      FileDescriptor result =
          new FileDescriptor(proto, dependencies, tables, allowUnknownDependencies);
      result.crossLink();
      // Skip feature resolution until later for calls from gencode.
      if (!allowUnresolvedFeatures) {
        // We do not need to force feature resolution for proto1 dependencies
        // since dependencies from non-gencode should already be fully feature resolved.
        result.resolveAllFeaturesInternal();
      }
      return result;
    }

    private static byte[] latin1Cat(final String[] strings) {
      // Hack:  We can't embed a raw byte array inside generated Java code
      //   (at least, not efficiently), but we can embed Strings.  So, the
      //   protocol compiler embeds the FileDescriptorProto as a giant
      //   string literal which is passed to this function to construct the
      //   file's FileDescriptor.  The string literal contains only 8-bit
      //   characters, each one representing a byte of the FileDescriptorProto's
      //   serialized form.  So, if we convert it to bytes in ISO-8859-1, we
      //   should get the original bytes that we want.
      // Literal strings are limited to 64k, so it may be split into multiple strings.
      if (strings.length == 1) {
        return strings[0].getBytes(Internal.ISO_8859_1);
      }
      StringBuilder descriptorData = new StringBuilder();
      for (String part : strings) {
        descriptorData.append(part);
      }
      return descriptorData.toString().getBytes(Internal.ISO_8859_1);
    }

    private static FileDescriptor[] findDescriptors(
        final Class<?> descriptorOuterClass,
        final String[] dependencyClassNames,
        final String[] dependencyFileNames) {
      List<FileDescriptor> descriptors = new ArrayList<>();
      for (int i = 0; i < dependencyClassNames.length; i++) {
        try {
          Class<?> clazz = descriptorOuterClass.getClassLoader().loadClass(dependencyClassNames[i]);
          descriptors.add((FileDescriptor) clazz.getField("descriptor").get(null));
        } catch (Exception e) {
          // We allow unknown dependencies by default. If a dependency cannot
          // be found we only generate a warning.
          logger.warning("Descriptors for \"" + dependencyFileNames[i] + "\" can not be found.");
        }
      }
      return descriptors.toArray(new FileDescriptor[0]);
    }

    /**
     * This method is for backward compatibility with generated code which passed an
     * InternalDescriptorAssigner.
     *
     * @deprecated Any gencode which is using this method is far out of support and should be
     *     regenerated with a newer version of the protobuf compiler.
     */
    @Deprecated
    public static void internalBuildGeneratedFileFrom(
        final String[] descriptorDataParts,
        final FileDescriptor[] dependencies,
        final InternalDescriptorAssigner descriptorAssigner) {
      final byte[] descriptorBytes = latin1Cat(descriptorDataParts);

      FileDescriptorProto proto;
      try {
        proto = FileDescriptorProto.parseFrom(descriptorBytes);
      } catch (InvalidProtocolBufferException e) {
        throw new IllegalArgumentException(
            "Failed to parse protocol buffer descriptor for generated code.", e);
      }

      final FileDescriptor result;
      try {
        // When building descriptors for generated code, we allow unknown
        // dependencies by default.
        result = buildFrom(proto, dependencies, true);
      } catch (DescriptorValidationException e) {
        throw new IllegalArgumentException(
            "Invalid embedded descriptor for \"" + proto.getName() + "\".", e);
      }

      final ExtensionRegistry registry = descriptorAssigner.assignDescriptors(result);

      if (registry != null) {
        throw new RuntimeException("assignDescriptors must return null");
      }
    }

    /**
     * This method is to be called by generated code only. It is equivalent to {@code buildFrom}
     * except that the {@code FileDescriptorProto} is encoded in protocol buffer wire format.
     */
    public static FileDescriptor internalBuildGeneratedFileFrom(
        final String[] descriptorDataParts, final FileDescriptor[] dependencies) {
      final byte[] descriptorBytes = latin1Cat(descriptorDataParts);

      FileDescriptorProto proto;
      try {
        proto = FileDescriptorProto.parseFrom(descriptorBytes);
      } catch (InvalidProtocolBufferException e) {
        throw new IllegalArgumentException(
            "Failed to parse protocol buffer descriptor for generated code.", e);
      }

      try {
        // When building descriptors for generated code, we allow unknown
        // dependencies by default and delay feature resolution until later.
        return buildFrom(proto, dependencies, true, true);
      } catch (DescriptorValidationException e) {
        throw new IllegalArgumentException(
            "Invalid embedded descriptor for \"" + proto.getName() + "\".", e);
      }
    }

    /**
     * This method is to be called by generated code only. It uses Java reflection to load the
     * dependencies' descriptors.
     */
    public static FileDescriptor internalBuildGeneratedFileFrom(
        final String[] descriptorDataParts,
        final Class<?> descriptorOuterClass,
        final String[] dependencyClassNames,
        final String[] dependencyFileNames) {
      FileDescriptor[] dependencies =
          findDescriptors(descriptorOuterClass, dependencyClassNames, dependencyFileNames);
      return internalBuildGeneratedFileFrom(descriptorDataParts, dependencies);
    }

    /**
     * This method is to be called by generated code only. It updates the FileDescriptorProto
     * associated with the descriptor by parsing it again with the given ExtensionRegistry. This is
     * needed to recognize custom options.
     */
    public static void internalUpdateFileDescriptor(
        FileDescriptor descriptor, ExtensionRegistry registry) {
      ByteString bytes = descriptor.proto.toByteString();
      try {
        FileDescriptorProto proto = FileDescriptorProto.parseFrom(bytes, registry);
        descriptor.setProto(proto);
      } catch (InvalidProtocolBufferException e) {
        throw new IllegalArgumentException(
            "Failed to parse protocol buffer descriptor for generated code.", e);
      }
    }

    /**
     * This class should be used by generated code only. When calling {@link
     * FileDescriptor#internalBuildGeneratedFileFrom}, the caller provides a callback implementing
     * this interface. The callback is called after the FileDescriptor has been constructed, in
     * order to assign all the global variables defined in the generated code which point at parts
     * of the FileDescriptor. The callback returns an ExtensionRegistry which contains any
     * extensions which might be used in the descriptor -- that is, extensions of the various
     * "Options" messages defined in descriptor.proto. The callback may also return null to indicate
     * that no extensions are used in the descriptor.
     *
     * <p>This interface is deprecated. Use the return value of internalBuildGeneratedFrom()
     * instead.
     */
    @Deprecated
    public interface InternalDescriptorAssigner {
      ExtensionRegistry assignDescriptors(FileDescriptor root);
    }

    private FileDescriptorProto proto;
    private volatile FileOptions options;
    private final Descriptor[] messageTypes;
    private final EnumDescriptor[] enumTypes;
    private final ServiceDescriptor[] services;
    private final FieldDescriptor[] extensions;
    private final FileDescriptor[] dependencies;
    private final FileDescriptor[] publicDependencies;
    private final FileDescriptorTables tables;
    private final boolean placeholder;
    private volatile boolean featuresResolved;

    private FileDescriptor(
        final FileDescriptorProto proto,
        final FileDescriptor[] dependencies,
        final FileDescriptorTables tables,
        boolean allowUnknownDependencies)
        throws DescriptorValidationException {
      this.tables = tables;
      this.proto = proto;
      this.dependencies = dependencies.clone();
      this.featuresResolved = false;
      HashMap<String, FileDescriptor> nameToFileMap = new HashMap<>();
      for (FileDescriptor file : dependencies) {
        nameToFileMap.put(file.getName(), file);
      }
      List<FileDescriptor> publicDependencies = new ArrayList<>();
      for (int i = 0; i < proto.getPublicDependencyCount(); i++) {
        int index = proto.getPublicDependency(i);
        if (index < 0 || index >= proto.getDependencyCount()) {
          throw new DescriptorValidationException(this, "Invalid public dependency index.");
        }
        String name = proto.getDependency(index);
        FileDescriptor file = nameToFileMap.get(name);
        if (file == null) {
          if (!allowUnknownDependencies) {
            throw new DescriptorValidationException(this, "Invalid public dependency: " + name);
          }
          // Ignore unknown dependencies.
        } else {
          publicDependencies.add(file);
        }
      }
      this.publicDependencies = new FileDescriptor[publicDependencies.size()];
      publicDependencies.toArray(this.publicDependencies);

      placeholder = false;

      tables.addPackage(getPackage(), this);

      messageTypes =
          (proto.getMessageTypeCount() > 0)
              ? new Descriptor[proto.getMessageTypeCount()]
              : EMPTY_DESCRIPTORS;
      for (int i = 0; i < proto.getMessageTypeCount(); i++) {
        messageTypes[i] = new Descriptor(proto.getMessageType(i), this, null, i);
      }

      enumTypes =
          (proto.getEnumTypeCount() > 0)
              ? new EnumDescriptor[proto.getEnumTypeCount()]
              : EMPTY_ENUM_DESCRIPTORS;
      for (int i = 0; i < proto.getEnumTypeCount(); i++) {
        enumTypes[i] = new EnumDescriptor(proto.getEnumType(i), this, null, i);
      }

      services =
          (proto.getServiceCount() > 0)
              ? new ServiceDescriptor[proto.getServiceCount()]
              : EMPTY_SERVICE_DESCRIPTORS;
      for (int i = 0; i < proto.getServiceCount(); i++) {
        services[i] = new ServiceDescriptor(proto.getService(i), this, i);
      }

      extensions =
          (proto.getExtensionCount() > 0)
              ? new FieldDescriptor[proto.getExtensionCount()]
              : EMPTY_FIELD_DESCRIPTORS;
      for (int i = 0; i < proto.getExtensionCount(); i++) {
        extensions[i] = new FieldDescriptor(proto.getExtension(i), this, null, i, true);
      }
    }

    /** Create a placeholder FileDescriptor */
    private FileDescriptor(
        String packageName,
        Descriptor[] messages,
        EnumDescriptor[] enums,
        FileDescriptorProto proto)
        throws DescriptorValidationException {
      this.tables = new FileDescriptorTables(new FileDescriptor[0], true);
      this.proto = proto;
      this.dependencies = new FileDescriptor[0];
      this.publicDependencies = new FileDescriptor[0];
      this.featuresResolved = false;

      messageTypes = messages;
      enumTypes = enums;
      services = EMPTY_SERVICE_DESCRIPTORS;
      extensions = EMPTY_FIELD_DESCRIPTORS;

      placeholder = true;

      tables.addPackage(packageName, this);
    }

    /** Create a placeholder FileDescriptor for a message Descriptor. */
    FileDescriptor(String packageName, Descriptor message) throws DescriptorValidationException {
      this(
          packageName,
          new Descriptor[] {message},
          EMPTY_ENUM_DESCRIPTORS,
          FileDescriptorProto.newBuilder()
              .setName(message.getFullName() + ".placeholder.proto")
              .setPackage(packageName)
              .addMessageType(message.toProto())
              .build());
    }

    /** Create a placeholder FileDescriptor for an EnumDescriptor. */
    private FileDescriptor(String packageName, EnumDescriptor enm)
        throws DescriptorValidationException {
      this(
          packageName,
          EMPTY_DESCRIPTORS,
          new EnumDescriptor[] {enm},
          FileDescriptorProto.newBuilder()
              .setName(enm.getFullName() + ".placeholder.proto")
              .setPackage(packageName)
              .addEnumType(enm.toProto())
              .build());
    }

    public void resolveAllFeaturesImmutable() {
      try {
        resolveAllFeaturesInternal();
      } catch (DescriptorValidationException e) {
        throw new IllegalArgumentException("Invalid features for \"" + proto.getName() + "\".", e);
      }
    }

    /**
     * This method is to be called by generated code only. It resolves features for the descriptor
     * and all of its children.
     */
    private void resolveAllFeaturesInternal() throws DescriptorValidationException {
      if (this.featuresResolved) {
        return;
      }

      synchronized (this) {
        if (this.featuresResolved) {
          return;
        }
        resolveFeatures(proto.getOptions().getFeatures());

        for (Descriptor messageType : messageTypes) {
          messageType.resolveAllFeatures();
        }

        for (EnumDescriptor enumType : enumTypes) {
          enumType.resolveAllFeatures();
        }

        for (ServiceDescriptor service : services) {
          service.resolveAllFeatures();
        }

        for (FieldDescriptor extension : extensions) {
          extension.resolveAllFeatures();
        }
        this.featuresResolved = true;
      }
    }

    @Override
    FeatureSet inferLegacyProtoFeatures() {
      if (getEdition().getNumber() >= Edition.EDITION_2023.getNumber()) {
        return FeatureSet.getDefaultInstance();
      }

      FeatureSet.Builder features = null;
      if (getEdition() == Edition.EDITION_PROTO2) {
        if (proto.getOptions().getJavaStringCheckUtf8()) {
          features = FeatureSet.newBuilder();
          features.setExtension(
              JavaFeaturesProto.java_,
              JavaFeatures.newBuilder()
                  .setUtf8Validation(JavaFeatures.Utf8Validation.VERIFY)
                  .build());
        }
      }

      return features != null ? features.build() : FeatureSet.getDefaultInstance();
    }

    /** Look up and cross-link all field types, etc. */
    private void crossLink() throws DescriptorValidationException {
      for (final Descriptor messageType : messageTypes) {
        messageType.crossLink();
      }

      for (final ServiceDescriptor service : services) {
        service.crossLink();
      }

      for (final FieldDescriptor extension : extensions) {
        extension.crossLink();
      }
    }

    /**
     * Replace our {@link FileDescriptorProto} with the given one, which is identical except that it
     * might contain extensions that weren't present in the original. This method is needed for
     * bootstrapping when a file defines custom options. The options may be defined in the file
     * itself, so we can't actually parse them until we've constructed the descriptors, but to
     * construct the descriptors we have to have parsed the descriptor protos. So, we have to parse
     * the descriptor protos a second time after constructing the descriptors.
     */
    private synchronized void setProto(final FileDescriptorProto proto) {
      this.proto = proto;
      this.options = null;
      try {
        resolveFeatures(proto.getOptions().getFeatures());

        for (int i = 0; i < messageTypes.length; i++) {
          messageTypes[i].setProto(proto.getMessageType(i));
        }

        for (int i = 0; i < enumTypes.length; i++) {
          enumTypes[i].setProto(proto.getEnumType(i));
        }

        for (int i = 0; i < services.length; i++) {
          services[i].setProto(proto.getService(i));
        }

        for (int i = 0; i < extensions.length; i++) {
          extensions[i].setProto(proto.getExtension(i));
        }
      } catch (DescriptorValidationException e) {
        throw new IllegalArgumentException("Invalid features for \"" + proto.getName() + "\".", e);
      }
    }
  }

  // =================================================================

  /** Describes a message type. */
  public static final class Descriptor extends GenericDescriptor {
    /**
     * Get the index of this descriptor within its parent. In other words, given a {@link
     * FileDescriptor} {@code file}, the following is true:
     *
     * <pre>
     *   for all i in [0, file.getMessageTypeCount()):
     *     file.getMessageType(i).getIndex() == i
     * </pre>
     *
     * Similarly, for a {@link Descriptor} {@code messageType}:
     *
     * <pre>
     *   for all i in [0, messageType.getNestedTypeCount()):
     *     messageType.getNestedType(i).getIndex() == i
     * </pre>
     */
    public int getIndex() {
      return index;
    }

    /** Convert the descriptor to its protocol message representation. */
    @Override
    public DescriptorProto toProto() {
      return proto;
    }

    /** Get the type's unqualified name. */
    @Override
    public String getName() {
      return proto.getName();
    }

    /**
     * Get the type's fully-qualified name, within the proto language's namespace. This differs from
     * the Java name. For example, given this {@code .proto}:
     *
     * <pre>
     *   package foo.bar;
     *   option java_package = "com.example.protos"
     *   message Baz {}
     * </pre>
     *
     * {@code Baz}'s full name is "foo.bar.Baz".
     */
    @Override
    public String getFullName() {
      return fullName;
    }

    /** Get the {@link FileDescriptor} containing this descriptor. */
    @Override
    public FileDescriptor getFile() {
      return parent.getFile();
    }

    @Override
    GenericDescriptor getParent() {
      return parent;
    }

    public boolean isPlaceholder() {
      return placeholder;
    }

    /** If this is a nested type, get the outer descriptor, otherwise null. */
    public Descriptor getContainingType() {
      if (parent instanceof Descriptor) {
        return (Descriptor) parent;
      }
      return null;
    }

    /** Get the {@code MessageOptions}, defined in {@code descriptor.proto}. */
    public MessageOptions getOptions() {
      if (this.options == null) {
        MessageOptions strippedOptions = this.proto.getOptions();
        if (strippedOptions.hasFeatures()) {
          // Clients should be using feature accessor methods, not accessing features on the
          // options
          // proto.
          strippedOptions = strippedOptions.toBuilder().clearFeatures().build();
        }
        synchronized (this) {
          if (this.options == null) {
            this.options = strippedOptions;
          }
        }
      }
      return this.options;
    }

    /** Get a list of this message type's fields. */
    public List<FieldDescriptor> getFields() {
      return Collections.unmodifiableList(Arrays.asList(fields));
    }

    /** Get a count of this message type's fields. */
    public int getFieldCount() {
      return fields.length;
    }

    /** Get the message type's field at the specified index. */
    public FieldDescriptor getField(int index) {
      return fields[index];
    }

    /** Get a list of this message type's oneofs. */
    public List<OneofDescriptor> getOneofs() {
      return Collections.unmodifiableList(Arrays.asList(oneofs));
    }

    /** Get a count of this message type's oneofs. */
    public int getOneofCount() {
      return oneofs.length;
    }

    /** Get the message type's oneof at the specified index. */
    public OneofDescriptor getOneof(int index) {
      return oneofs[index];
    }

    /** Get a list of this message type's real oneofs. */
    public List<OneofDescriptor> getRealOneofs() {
      return Collections.unmodifiableList(Arrays.asList(oneofs).subList(0, realOneofCount));
    }

    /** Get a count of this message type's real oneofs. */
    public int getRealOneofCount() {
      return realOneofCount;
    }

    /** Get the message type's real oneof at the specified index. */
    public OneofDescriptor getRealOneof(int index) {
      if (index >= realOneofCount) {
        throw new ArrayIndexOutOfBoundsException(index);
      }
      return oneofs[index];
    }

    /** Get a list of the extensions defined nested within this message type's scope. */
    public List<FieldDescriptor> getExtensions() {
      return Collections.unmodifiableList(Arrays.asList(extensions));
    }

    /** Get a count of the extensions defined nested within this message type's scope. */
    public int getExtensionCount() {
      return extensions.length;
    }

    /** Get the extension defined nested within this message type's scope at the specified index. */
    public FieldDescriptor getExtension(int index) {
      return extensions[index];
    }

    /** Get a list of message types nested within this one. */
    public List<Descriptor> getNestedTypes() {
      return Collections.unmodifiableList(Arrays.asList(nestedTypes));
    }

    /** Get a count of message types nested within this one. */
    public int getNestedTypeCount() {
      return nestedTypes.length;
    }

    /** Get the message type's nested message type at the specified index. */
    public Descriptor getNestedType(int index) {
      return nestedTypes[index];
    }

    /** Get a list of enum types nested within this one. */
    public List<EnumDescriptor> getEnumTypes() {
      return Collections.unmodifiableList(Arrays.asList(enumTypes));
    }

    /** Get a count of enum types nested within this one. */
    public int getEnumTypeCount() {
      return enumTypes.length;
    }

    /** Get the message type's nested enum type at the specified index. */
    public EnumDescriptor getEnumType(int index) {
      return enumTypes[index];
    }

    /** Determines if the given field number is an extension. */
    public boolean isExtensionNumber(final int number) {
      int index = Arrays.binarySearch(extensionRangeLowerBounds, number);
      if (index < 0) {
        index = ~index - 1;
      }
      // extensionRangeLowerBounds[index] is the biggest value <= number
      return index >= 0 && number < extensionRangeUpperBounds[index];
    }

    /** Determines if the given field number is reserved. */
    public boolean isReservedNumber(final int number) {
      for (final DescriptorProto.ReservedRange range : proto.getReservedRangeList()) {
        if (range.getStart() <= number && number < range.getEnd()) {
          return true;
        }
      }
      return false;
    }

    /** Determines if the given field name is reserved. */
    public boolean isReservedName(final String name) {
      checkNotNull(name);
      for (final String reservedName : proto.getReservedNameList()) {
        if (reservedName.equals(name)) {
          return true;
        }
      }
      return false;
    }

    /**
     * Indicates whether the message can be extended. That is, whether it has any "extensions x to
     * y" ranges declared on it.
     */
    public boolean isExtendable() {
      return !proto.getExtensionRangeList().isEmpty();
    }

    /**
     * Finds a field by name.
     *
     * @param name The unqualified name of the field (e.g. "foo"). For protocol buffer messages that
     *     follow <a
     *     href=https://developers.google.com/protocol-buffers/docs/style#message_and_field_names>Google's
     *     guidance on naming</a> this will be a snake case string, such as
     *     <pre>song_name</pre>
     *     .
     * @return The field's descriptor, or {@code null} if not found.
     */
    public FieldDescriptor findFieldByName(final String name) {
      final GenericDescriptor result = getFile().tables.findSymbol(fullName + '.' + name);
      if (result instanceof FieldDescriptor) {
        return (FieldDescriptor) result;
      } else {
        return null;
      }
    }

    /**
     * Finds a field by field number.
     *
     * @param number The field number within this message type.
     * @return The field's descriptor, or {@code null} if not found.
     */
    public FieldDescriptor findFieldByNumber(final int number) {
      return binarySearch(
          fieldsSortedByNumber, fieldsSortedByNumber.length, FieldDescriptor.NUMBER_GETTER, number);
    }

    /**
     * Finds a nested message type by name.
     *
     * @param name The unqualified name of the nested type such as "Foo"
     * @return The types's descriptor, or {@code null} if not found.
     */
    public Descriptor findNestedTypeByName(final String name) {
      final GenericDescriptor result = getFile().tables.findSymbol(fullName + '.' + name);
      if (result instanceof Descriptor) {
        return (Descriptor) result;
      } else {
        return null;
      }
    }

    /**
     * Finds a nested enum type by name.
     *
     * @param name The unqualified name of the nested type such as "Foo"
     * @return The types's descriptor, or {@code null} if not found.
     */
    public EnumDescriptor findEnumTypeByName(final String name) {
      final GenericDescriptor result = getFile().tables.findSymbol(fullName + '.' + name);
      if (result instanceof EnumDescriptor) {
        return (EnumDescriptor) result;
      } else {
        return null;
      }
    }

    private final int index;
    private DescriptorProto proto;
    private volatile MessageOptions options;
    private final String fullName;
    private final GenericDescriptor parent;
    private final Descriptor[] nestedTypes;
    private final EnumDescriptor[] enumTypes;
    private final FieldDescriptor[] fields;
    private final FieldDescriptor[] fieldsSortedByNumber;
    private final FieldDescriptor[] extensions;
    private final OneofDescriptor[] oneofs;
    private final int realOneofCount;

    private final int[] extensionRangeLowerBounds;
    private final int[] extensionRangeUpperBounds;

    private final boolean placeholder;

    // Used to create a placeholder when the type cannot be found.
    Descriptor(final String fullname) throws DescriptorValidationException {
      String name = fullname;
      String packageName = "";
      int pos = fullname.lastIndexOf('.');
      if (pos != -1) {
        name = fullname.substring(pos + 1);
        packageName = fullname.substring(0, pos);
      }
      this.index = 0;
      this.proto =
          DescriptorProto.newBuilder()
              .setName(name)
              .addExtensionRange(
                  DescriptorProto.ExtensionRange.newBuilder()
                      .setStart(1)
                      // 2^31 - 1, which is the largest possible extension for messages that set
                      // message_set_wire_format = true.  Since this is a placeholder and we don't
                      // have a schema, this is safest setting.
                      .setEnd(2147483647)
                      .build())
              .build();
      this.fullName = fullname;

      this.nestedTypes = EMPTY_DESCRIPTORS;
      this.enumTypes = EMPTY_ENUM_DESCRIPTORS;
      this.fields = EMPTY_FIELD_DESCRIPTORS;
      this.fieldsSortedByNumber = EMPTY_FIELD_DESCRIPTORS;
      this.extensions = EMPTY_FIELD_DESCRIPTORS;
      this.oneofs = EMPTY_ONEOF_DESCRIPTORS;
      this.realOneofCount = 0;

      // Create a placeholder FileDescriptor to hold this message.
      this.parent = new FileDescriptor(packageName, this);

      extensionRangeLowerBounds = new int[] {this.proto.getExtensionRange(0).getStart()};
      extensionRangeUpperBounds = new int[] {this.proto.getExtensionRange(0).getEnd()};

      placeholder = true;
    }

    private Descriptor(
        final DescriptorProto proto,
        final FileDescriptor file,
        final Descriptor parent,
        final int index)
        throws DescriptorValidationException {
      if (parent == null) {
        this.parent = file;
      } else {
        this.parent = parent;
      }
      this.index = index;
      this.proto = proto;
      fullName = computeFullName(file, parent, proto.getName());

      oneofs =
          (proto.getOneofDeclCount() > 0)
              ? new OneofDescriptor[proto.getOneofDeclCount()]
              : EMPTY_ONEOF_DESCRIPTORS;
      for (int i = 0; i < proto.getOneofDeclCount(); i++) {
        oneofs[i] = new OneofDescriptor(proto.getOneofDecl(i), this, i);
      }

      nestedTypes =
          (proto.getNestedTypeCount() > 0)
              ? new Descriptor[proto.getNestedTypeCount()]
              : EMPTY_DESCRIPTORS;
      for (int i = 0; i < proto.getNestedTypeCount(); i++) {
        nestedTypes[i] = new Descriptor(proto.getNestedType(i), file, this, i);
      }

      enumTypes =
          (proto.getEnumTypeCount() > 0)
              ? new EnumDescriptor[proto.getEnumTypeCount()]
              : EMPTY_ENUM_DESCRIPTORS;
      for (int i = 0; i < proto.getEnumTypeCount(); i++) {
        enumTypes[i] = new EnumDescriptor(proto.getEnumType(i), file, this, i);
      }

      fields =
          (proto.getFieldCount() > 0)
              ? new FieldDescriptor[proto.getFieldCount()]
              : EMPTY_FIELD_DESCRIPTORS;
      for (int i = 0; i < proto.getFieldCount(); i++) {
        fields[i] = new FieldDescriptor(proto.getField(i), file, this, i, false);
      }
      this.fieldsSortedByNumber =
          (proto.getFieldCount() > 0) ? fields.clone() : EMPTY_FIELD_DESCRIPTORS;

      extensions =
          (proto.getExtensionCount() > 0)
              ? new FieldDescriptor[proto.getExtensionCount()]
              : EMPTY_FIELD_DESCRIPTORS;
      for (int i = 0; i < proto.getExtensionCount(); i++) {
        extensions[i] = new FieldDescriptor(proto.getExtension(i), file, this, i, true);
      }

      for (int i = 0; i < proto.getOneofDeclCount(); i++) {
        oneofs[i].fields = new FieldDescriptor[oneofs[i].getFieldCount()];
        oneofs[i].fieldCount = 0;
      }
      for (int i = 0; i < proto.getFieldCount(); i++) {
        OneofDescriptor oneofDescriptor = fields[i].getContainingOneof();
        if (oneofDescriptor != null) {
          oneofDescriptor.fields[oneofDescriptor.fieldCount++] = fields[i];
        }
      }

      int syntheticOneofCount = 0;
      for (OneofDescriptor oneof : this.oneofs) {
        if (oneof.isSynthetic()) {
          syntheticOneofCount++;
        } else {
          if (syntheticOneofCount > 0) {
            throw new DescriptorValidationException(this, "Synthetic oneofs must come last.");
          }
        }
      }
      this.realOneofCount = this.oneofs.length - syntheticOneofCount;

      placeholder = false;

      file.tables.addSymbol(this);

      // NOTE: The defined extension ranges are guaranteed to be disjoint.
      if (proto.getExtensionRangeCount() > 0) {
        extensionRangeLowerBounds = new int[proto.getExtensionRangeCount()];
        extensionRangeUpperBounds = new int[proto.getExtensionRangeCount()];
        int i = 0;
        for (final DescriptorProto.ExtensionRange range : proto.getExtensionRangeList()) {
          extensionRangeLowerBounds[i] = range.getStart();
          extensionRangeUpperBounds[i] = range.getEnd();
          i++;
        }
        // Since the ranges are disjoint, sorting these independently must still produce the correct
        // order.
        Arrays.sort(extensionRangeLowerBounds);
        Arrays.sort(extensionRangeUpperBounds);
      } else {
        extensionRangeLowerBounds = EMPTY_INT_ARRAY;
        extensionRangeUpperBounds = EMPTY_INT_ARRAY;
      }
    }

    /** See {@link FileDescriptor#resolveAllFeatures}. */
    private void resolveAllFeatures() throws DescriptorValidationException {
      resolveFeatures(proto.getOptions().getFeatures());

      for (Descriptor nestedType : nestedTypes) {
        nestedType.resolveAllFeatures();
      }

      for (EnumDescriptor enumType : enumTypes) {
        enumType.resolveAllFeatures();
      }

      // Oneofs must be resolved before any children oneof fields.
      for (OneofDescriptor oneof : oneofs) {
        oneof.resolveAllFeatures();
      }

      for (FieldDescriptor field : fields) {
        field.resolveAllFeatures();
      }

      for (FieldDescriptor extension : extensions) {
        extension.resolveAllFeatures();
      }
    }

    /** Look up and cross-link all field types, etc. */
    private void crossLink() throws DescriptorValidationException {
      for (final Descriptor nestedType : nestedTypes) {
        nestedType.crossLink();
      }

      for (final FieldDescriptor field : fields) {
        field.crossLink();
      }
      Arrays.sort(fieldsSortedByNumber);
      validateNoDuplicateFieldNumbers();

      for (final FieldDescriptor extension : extensions) {
        extension.crossLink();
      }
    }

    private void validateNoDuplicateFieldNumbers() throws DescriptorValidationException {
      for (int i = 0; i + 1 < fieldsSortedByNumber.length; i++) {
        FieldDescriptor old = fieldsSortedByNumber[i];
        FieldDescriptor field = fieldsSortedByNumber[i + 1];
        if (old.getNumber() == field.getNumber()) {
          throw new DescriptorValidationException(
              field,
              "Field number "
                  + field.getNumber()
                  + " has already been used in \""
                  + field.getContainingType().getFullName()
                  + "\" by field \""
                  + old.getName()
                  + "\".");
        }
      }
    }

    /** See {@link FileDescriptor#setProto}. */
    private void setProto(final DescriptorProto proto) throws DescriptorValidationException {
      this.proto = proto;
      this.options = null;
      resolveFeatures(proto.getOptions().getFeatures());

      for (int i = 0; i < nestedTypes.length; i++) {
        nestedTypes[i].setProto(proto.getNestedType(i));
      }

      for (int i = 0; i < oneofs.length; i++) {
        oneofs[i].setProto(proto.getOneofDecl(i));
      }

      for (int i = 0; i < enumTypes.length; i++) {
        enumTypes[i].setProto(proto.getEnumType(i));
      }

      for (int i = 0; i < fields.length; i++) {
        fields[i].setProto(proto.getField(i));
      }

      for (int i = 0; i < extensions.length; i++) {
        extensions[i].setProto(proto.getExtension(i));
      }
    }
  }

  // =================================================================

  /** Describes a field of a message type. */
  public static final class FieldDescriptor extends GenericDescriptor
      implements Comparable<FieldDescriptor>, FieldSet.FieldDescriptorLite<FieldDescriptor> {
    private static final ToIntFunction<FieldDescriptor> NUMBER_GETTER = FieldDescriptor::getNumber;

    /**
     * Get the index of this descriptor within its parent.
     *
     * @see Descriptors.Descriptor#getIndex()
     */
    public int getIndex() {
      return index;
    }

    /** Convert the descriptor to its protocol message representation. */
    @Override
    public FieldDescriptorProto toProto() {
      return proto;
    }

    /** Get the field's unqualified name. */
    @Override
    public String getName() {
      return proto.getName();
    }

    /** Get the field's number. */
    @Override
    public int getNumber() {
      return proto.getNumber();
    }

    /**
     * Get the field's fully-qualified name.
     *
     * @see Descriptors.Descriptor#getFullName()
     */
    @Override
    public String getFullName() {
      return fullName;
    }

    /** Get the JSON name of this field. */
    public String getJsonName() {
      String result = jsonName;
      if (result != null) {
        return result;
      } else if (proto.hasJsonName()) {
        return jsonName = proto.getJsonName();
      } else {
        return jsonName = fieldNameToJsonName(proto.getName());
      }
    }

    /**
     * Get the field's java type. This is just for convenience. Every {@code
     * FieldDescriptorProto.Type} maps to exactly one Java type.
     */
    public JavaType getJavaType() {
      return getType().getJavaType();
    }

    /** For internal use only. */
    @Override
    public WireFormat.JavaType getLiteJavaType() {
      return getLiteType().getJavaType();
    }

    /** Get the {@code FileDescriptor} containing this descriptor. */
    @Override
    public FileDescriptor getFile() {
      return parent.getFile();
    }

    @Override
    GenericDescriptor getParent() {
      return parent;
    }

    /** Get the field's declared type. */
    public Type getType() {
      // Override delimited messages as legacy group type. Leaves unresolved messages as-is
      // since these are used before feature resolution when parsing java feature set defaults
      // (custom options) into unknown fields.
      if (type == Type.MESSAGE
          && !(typeDescriptor != null
              && ((Descriptor) typeDescriptor).toProto().getOptions().getMapEntry())
          && !(containingType != null && containingType.toProto().getOptions().getMapEntry())
          && this.features != null
          && getFeatures().getMessageEncoding() == FeatureSet.MessageEncoding.DELIMITED) {
        return Type.GROUP;
      }
      return type;
    }

    /** For internal use only. */
    @Override
    public WireFormat.FieldType getLiteType() {
      return table[getType().ordinal()];
    }

    /** For internal use only. */
    public boolean needsUtf8Check() {
      if (getType() != Type.STRING) {
        return false;
      }
      if (getContainingType().toProto().getOptions().getMapEntry()) {
        // Always enforce strict UTF-8 checking for map fields.
        return true;
      }
      if (getFeatures()
          .getExtension(JavaFeaturesProto.java_)
          .getUtf8Validation()
          .equals(JavaFeatures.Utf8Validation.VERIFY)) {
        return true;
      }
      return getFeatures().getUtf8Validation().equals(FeatureSet.Utf8Validation.VERIFY);
    }

    public boolean isMapField() {
      return getType() == Type.MESSAGE
          && isRepeated()
          && getMessageType().toProto().getOptions().getMapEntry();
    }

    // I'm pretty sure values() constructs a new array every time, since there
    // is nothing stopping the caller from mutating the array.  Therefore we
    // make a static copy here.
    private static final WireFormat.FieldType[] table = WireFormat.FieldType.values();

    /** Is this field declared required? */
    public boolean isRequired() {
      return getFeatures().getFieldPresence()
          == DescriptorProtos.FeatureSet.FieldPresence.LEGACY_REQUIRED;
    }

    /**
     * Is this field declared optional? *
     *
     * <p>This method is deprecated. Use !isRequired() && !isRepeated() instead.
     */
    @Deprecated
    public boolean isOptional() {
      return proto.getLabel() == FieldDescriptorProto.Label.LABEL_OPTIONAL
          && getFeatures().getFieldPresence()
              != DescriptorProtos.FeatureSet.FieldPresence.LEGACY_REQUIRED;
    }

    /** Is this field declared repeated? */
    @Override
    public boolean isRepeated() {
      return proto.getLabel() == FieldDescriptorProto.Label.LABEL_REPEATED;
    }

    /**
     * Does this field have the {@code [packed = true]} option or is this field packable in proto3
     * and not explicitly set to unpacked?
     */
    @Override
    public boolean isPacked() {
      if (!isPackable()) {
        return false;
      }
      return getFeatures()
          .getRepeatedFieldEncoding()
          .equals(FeatureSet.RepeatedFieldEncoding.PACKED);
    }

    /** Can this field be packed? That is, is it a repeated primitive field? */
    public boolean isPackable() {
      return isRepeated() && getLiteType().isPackable();
    }

    /** Returns true if the field had an explicitly-defined default value. */
    public boolean hasDefaultValue() {
      return proto.hasDefaultValue();
    }

    /**
     * Returns the field's default value. Valid for all types except for messages and groups. For
     * all other types, the object returned is of the same class that would returned by
     * Message.getField(this).
     */
    public Object getDefaultValue() {
      if (getJavaType() == JavaType.MESSAGE) {
        throw new UnsupportedOperationException(
            "FieldDescriptor.getDefaultValue() called on an embedded message field.");
      }
      return defaultValue;
    }

    /** Get the {@code FieldOptions}, defined in {@code descriptor.proto}. */
    public FieldOptions getOptions() {
      if (this.options == null) {
        FieldOptions strippedOptions = this.proto.getOptions();
        if (strippedOptions.hasFeatures()) {
          // Clients should be using feature accessor methods, not accessing features on the
          // options
          // proto.
          strippedOptions = strippedOptions.toBuilder().clearFeatures().build();
        }
        synchronized (this) {
          if (this.options == null) {
            this.options = strippedOptions;
          }
        }
      }
      return this.options;
    }

    /** Is this field an extension? */
    public boolean isExtension() {
      return proto.hasExtendee();
    }

    /**
     * Get the field's containing type. For extensions, this is the type being extended, not the
     * location where the extension was defined. See {@link #getExtensionScope()}.
     */
    public Descriptor getContainingType() {
      return containingType;
    }

    /** Get the field's containing oneof. */
    public OneofDescriptor getContainingOneof() {
      return containingOneof;
    }

    /** Get the field's containing oneof, only if non-synthetic. */
    public OneofDescriptor getRealContainingOneof() {
      return containingOneof != null && !containingOneof.isSynthetic() ? containingOneof : null;
    }

    /**
     * Returns true if this field was syntactically written with "optional" in the .proto file.
     * Excludes singular proto3 fields that do not have a label.
     */
    boolean hasOptionalKeyword() {
      return isProto3Optional
          || (getFile().getEdition() == Edition.EDITION_PROTO2
              && !isRequired()
              && !isRepeated()
              && getContainingOneof() == null);
    }

    /**
     * Returns true if this field tracks presence, ie. does the field distinguish between "unset"
     * and "present with default value."
     *
     * <p>This includes required, optional, and oneof fields. It excludes maps, repeated fields, and
     * singular proto3 fields without "optional".
     *
     * <p>For fields where hasPresence() == true, the return value of msg.hasField() is semantically
     * meaningful.
     */
    public boolean hasPresence() {
      if (isRepeated()) {
        return false;
      }
      return isProto3Optional
          || getType() == Type.MESSAGE
          || getType() == Type.GROUP
          || isExtension()
          || getContainingOneof() != null
          || getFeatures().getFieldPresence() != DescriptorProtos.FeatureSet.FieldPresence.IMPLICIT;
    }

    /**
     * Returns true if this field is structured like the synthetic field of a proto2 group. This
     * allows us to expand our treatment of delimited fields without breaking proto2 files that have
     * been upgraded to editions.
     */
    boolean isGroupLike() {
      if (getType() != Type.GROUP) {
        // Groups are always tag-delimited.
        return false;
      }

      if (!getMessageType().getName().toLowerCase(Locale.ROOT).equals(getName())) {
        // Group fields always are always the lowercase type name.
        return false;
      }

      if (getMessageType().getFile() != getFile()) {
        // Groups could only be defined in the same file they're used.
        return false;
      }

      // Group messages are always defined in the same scope as the field.  File level extensions
      // will compare NULL == NULL here, which is why the file comparison above is necessary to
      // ensure both come from the same file.
      return isExtension()
          ? getMessageType().getContainingType() == getExtensionScope()
          : getMessageType().getContainingType() == getContainingType();
    }

    /**
     * For extensions defined nested within message types, gets the outer type. Not valid for
     * non-extension fields. For example, consider this {@code .proto} file:
     *
     * <pre>
     *   message Foo {
     *     extensions 1000 to max;
     *   }
     *   extend Foo {
     *     optional int32 baz = 1234;
     *   }
     *   message Bar {
     *     extend Foo {
     *       optional int32 moo = 4321;
     *     }
     *   }
     * </pre>
     *
     * Both {@code baz}'s and {@code moo}'s containing type is {@code Foo}. However, {@code baz}'s
     * extension scope is {@code null} while {@code moo}'s extension scope is {@code Bar}.
     */
    public Descriptor getExtensionScope() {
      if (!isExtension()) {
        throw new UnsupportedOperationException(
            String.format("This field is not an extension. (%s)", fullName));
      }
      return extensionScope;
    }

    /** For embedded message and group fields, gets the field's type. */
    public Descriptor getMessageType() {
      if (getJavaType() != JavaType.MESSAGE) {
        throw new UnsupportedOperationException(
            String.format("This field is not of message type. (%s)", fullName));
      }
      return (Descriptor) typeDescriptor;
    }

    /** For enum fields, gets the field's type. */
    @Override
    public EnumDescriptor getEnumType() {
      if (getJavaType() != JavaType.ENUM) {
        throw new UnsupportedOperationException(
            String.format("This field is not of enum type. (%s)", fullName));
      }
      return (EnumDescriptor) typeDescriptor;
    }

    /**
     * Determines if the given enum field is treated as closed based on legacy non-conformant
     * behavior.
     *
     * <p>Conformant behavior determines closedness based on the enum and can be queried using
     * {@code EnumDescriptor.isClosed()}.
     *
     * <p>Some runtimes currently have a quirk where non-closed enums are treated as closed when
     * used as the type of fields defined in a `syntax = proto2;` file. This quirk is not present in
     * all runtimes; as of writing, we know that:
     *
     * <ul>
     *   <li>C++, Java, and C++-based Python share this quirk.
     *   <li>UPB and UPB-based Python do not.
     *   <li>PHP and Ruby treat all enums as open regardless of declaration.
     * </ul>
     *
     * <p>Care should be taken when using this function to respect the target runtime's enum
     * handling quirks.
     */
    public boolean legacyEnumFieldTreatedAsClosed() {
      // Don't check JavaFeaturesProto extension for files without dependencies.
      // This is especially important for descriptor.proto since getting the JavaFeaturesProto
      // extension itself involves calling legacyEnumFieldTreatedAsClosed() which would otherwise
      // infinite loop.
      if (getFile().getDependencies().isEmpty()) {
        return getType() == Type.ENUM && getEnumType().isClosed();
      }

      return getType() == Type.ENUM
          && (getFeatures().getExtension(JavaFeaturesProto.java_).getLegacyClosedEnum()
              || getEnumType().isClosed());
    }

    /**
     * Compare with another {@code FieldDescriptor}. This orders fields in "canonical" order, which
     * simply means ascending order by field number. {@code other} must be a field of the same type.
     * That is, {@code getContainingType()} must return the same {@code Descriptor} for both fields.
     *
     * @return negative, zero, or positive if {@code this} is less than, equal to, or greater than
     *     {@code other}, respectively
     */
    @Override
    public int compareTo(final FieldDescriptor other) {
      if (other.containingType != containingType) {
        throw new IllegalArgumentException(
            "FieldDescriptors can only be compared to other FieldDescriptors "
                + "for fields of the same message type.");
      }
      return getNumber() - other.getNumber();
    }

    @Override
    public String toString() {
      return getFullName();
    }

    private final int index;

    private FieldDescriptorProto proto;
    private volatile FieldOptions options;
    private final String fullName;
    private String jsonName;
    private final GenericDescriptor parent;
    private final Descriptor extensionScope;
    private final boolean isProto3Optional;

    static final class RedactionState {
      private static final RedactionState FALSE_FALSE = new RedactionState(false, false);
      private static final RedactionState FALSE_TRUE = new RedactionState(false, true);
      private static final RedactionState TRUE_FALSE = new RedactionState(true, false);
      private static final RedactionState TRUE_TRUE = new RedactionState(true, true);

      final boolean redact;
      final boolean report;

      private RedactionState(boolean redact, boolean report) {
        this.redact = redact;
        this.report = report;
      }

      private static RedactionState of(boolean redact) {
        return of(redact, false);
      }

      private static RedactionState of(boolean redact, boolean report) {
        if (redact) {
          return report ? TRUE_TRUE : TRUE_FALSE;
        }
        return report ? FALSE_TRUE : FALSE_FALSE;
      }

      private static RedactionState combine(RedactionState lhs, RedactionState rhs) {
        return of(lhs.redact || rhs.redact, rhs.report);
      }
    }

    private volatile RedactionState redactionState;

    // Possibly initialized during cross-linking.
    private Type type;
    private Descriptor containingType;
    private OneofDescriptor containingOneof;
    private GenericDescriptor typeDescriptor;
    private Object defaultValue;

    public enum Type {
      DOUBLE(JavaType.DOUBLE),
      FLOAT(JavaType.FLOAT),
      INT64(JavaType.LONG),
      UINT64(JavaType.LONG),
      INT32(JavaType.INT),
      FIXED64(JavaType.LONG),
      FIXED32(JavaType.INT),
      BOOL(JavaType.BOOLEAN),
      STRING(JavaType.STRING),
      GROUP(JavaType.MESSAGE),
      MESSAGE(JavaType.MESSAGE),
      BYTES(JavaType.BYTE_STRING),
      UINT32(JavaType.INT),
      ENUM(JavaType.ENUM),
      SFIXED32(JavaType.INT),
      SFIXED64(JavaType.LONG),
      SINT32(JavaType.INT),
      SINT64(JavaType.LONG);

      // Private copy to avoid repeated allocations from calls to values() in valueOf().
      private static final Type[] types = values();

      Type(JavaType javaType) {
        this.javaType = javaType;
      }

      private final JavaType javaType;

      public FieldDescriptorProto.Type toProto() {
        return FieldDescriptorProto.Type.forNumber(ordinal() + 1);
      }

      public JavaType getJavaType() {
        return javaType;
      }

      public static Type valueOf(final FieldDescriptorProto.Type type) {
        return types[type.getNumber() - 1];
      }
    }

    static {
      // Refuse to init if someone added a new declared type.
      if (Type.types.length != FieldDescriptorProto.Type.values().length) {
        throw new RuntimeException(
            "descriptor.proto has a new declared type but Descriptors.java wasn't updated.");
      }
    }

    public enum JavaType {
      INT(0),
      LONG(0L),
      FLOAT(0F),
      DOUBLE(0D),
      BOOLEAN(false),
      STRING(""),
      BYTE_STRING(ByteString.EMPTY),
      ENUM(null),
      MESSAGE(null);

      JavaType(final Object defaultDefault) {
        this.defaultDefault = defaultDefault;
      }

      /**
       * The default default value for fields of this type, if it's a primitive type. This is meant
       * for use inside this file only, hence is private.
       */
      private final Object defaultDefault;
    }

    // This method should match exactly with the ToJsonName() function in C++
    // descriptor.cc.
    private static String fieldNameToJsonName(String name) {
      final int length = name.length();
      StringBuilder result = new StringBuilder(length);
      boolean isNextUpperCase = false;
      for (int i = 0; i < length; i++) {
        char ch = name.charAt(i);
        if (ch == '_') {
          isNextUpperCase = true;
        } else if (isNextUpperCase) {
          // This closely matches the logic for ASCII characters in:
          // http://google3/google/protobuf/descriptor.cc?l=249-251&rcl=228891689
          if ('a' <= ch && ch <= 'z') {
            ch = (char) (ch - 'a' + 'A');
          }
          result.append(ch);
          isNextUpperCase = false;
        } else {
          result.append(ch);
        }
      }
      return result.toString();
    }

    private FieldDescriptor(
        final FieldDescriptorProto proto,
        final FileDescriptor file,
        final Descriptor parent,
        final int index,
        final boolean isExtension)
        throws DescriptorValidationException {
      this.index = index;
      this.proto = proto;
      fullName = computeFullName(file, parent, proto.getName());

      if (proto.hasType()) {
        type = Type.valueOf(proto.getType());
      }

      isProto3Optional = proto.getProto3Optional();

      if (getNumber() <= 0) {
        throw new DescriptorValidationException(this, "Field numbers must be positive integers.");
      }

      if (isExtension) {
        if (!proto.hasExtendee()) {
          throw new DescriptorValidationException(
              this, "FieldDescriptorProto.extendee not set for extension field.");
        }
        containingType = null; // Will be filled in when cross-linking
        if (parent != null) {
          // Message scoped extension.
          extensionScope = parent;
          this.parent = parent;
        } else {
          // File scoped extension.
          extensionScope = null;
          this.parent = checkNotNull(file);
        }

        if (proto.hasOneofIndex()) {
          throw new DescriptorValidationException(
              this, "FieldDescriptorProto.oneof_index set for extension field.");
        }
        containingOneof = null;
      } else {
        if (proto.hasExtendee()) {
          throw new DescriptorValidationException(
              this, "FieldDescriptorProto.extendee set for non-extension field.");
        }
        containingType = parent;

        if (proto.hasOneofIndex()) {
          if (proto.getOneofIndex() < 0
              || proto.getOneofIndex() >= parent.toProto().getOneofDeclCount()) {
            throw new DescriptorValidationException(
                this,
                "FieldDescriptorProto.oneof_index is out of range for type " + parent.getName());
          }
          containingOneof = parent.getOneofs().get(proto.getOneofIndex());
          containingOneof.fieldCount++;
          this.parent = checkNotNull(containingOneof);
        } else {
          containingOneof = null;
          this.parent = checkNotNull(parent);
        }
        extensionScope = null;
      }

      file.tables.addSymbol(this);
    }

    @SuppressWarnings("unchecked") // List<EnumValueDescriptor> guaranteed by protobuf runtime.
    private static RedactionState isOptionSensitive(FieldDescriptor field, Object value) {
      if (field.getType() == Descriptors.FieldDescriptor.Type.ENUM) {
        if (field.isRepeated()) {
          for (EnumValueDescriptor v : (List<EnumValueDescriptor>) value) {
            if (v.getOptions().getDebugRedact()) {
              return RedactionState.of(true, false);
            }
          }
        } else {
          if (((EnumValueDescriptor) value).getOptions().getDebugRedact()) {
            return RedactionState.of(true, false);
          }
        }
      } else if (field.getJavaType() == Descriptors.FieldDescriptor.JavaType.MESSAGE) {
        if (field.isRepeated()) {
          for (Message m : (List<Message>) value) {
            for (Map.Entry<FieldDescriptor, Object> entry : m.getAllFields().entrySet()) {
              RedactionState state = isOptionSensitive(entry.getKey(), entry.getValue());
              if (state.redact) {
                return state;
              }
            }
          }
        } else {
          for (Map.Entry<FieldDescriptor, Object> entry :
              ((Message) value).getAllFields().entrySet()) {
            RedactionState state = isOptionSensitive(entry.getKey(), entry.getValue());
            if (state.redact) {
              return state;
            }
          }
        }
      }
      return RedactionState.of(false);
    }

    // Lazily calculates the redact state of the field, caching the result.
    RedactionState getRedactionState() {
      RedactionState state = redactionState;
      if (state == null) {
        // If the field is directly marked with debug_redact=true, then it is sensitive.
        synchronized (this) {
          state = redactionState;
          if (state == null) {
            FieldOptions options = getOptions();
            state = RedactionState.of(options.getDebugRedact());
            // Check if the FieldOptions contain any enums that are marked as debug_redact=true,
            // either directly or indirectly via a message option.
            for (Map.Entry<Descriptors.FieldDescriptor, Object> entry :
                options.getAllFields().entrySet()) {
              state =
                  RedactionState.combine(
                      state, isOptionSensitive(entry.getKey(), entry.getValue()));
              if (state.redact) {
                break;
              }
            }
            redactionState = state;
          }
        }
      }
      return state;
    }

    /** See {@link FileDescriptor#resolveAllFeatures}. */
    private void resolveAllFeatures() throws DescriptorValidationException {
      resolveFeatures(proto.getOptions().getFeatures());
    }

    @Override
    FeatureSet inferLegacyProtoFeatures() {
      if (getFile().getEdition().getNumber() >= Edition.EDITION_2023.getNumber()) {
        return FeatureSet.getDefaultInstance();
      }

      FeatureSet.Builder features = null;

      if (proto.getLabel() == FieldDescriptorProto.Label.LABEL_REQUIRED) {
        features = FeatureSet.newBuilder();
        features.setFieldPresence(FeatureSet.FieldPresence.LEGACY_REQUIRED);
      }

      if (proto.getType() == FieldDescriptorProto.Type.TYPE_GROUP) {
        if (features == null) {
          features = FeatureSet.newBuilder();
        }
        features.setMessageEncoding(FeatureSet.MessageEncoding.DELIMITED);
      }

      if (getFile().getEdition() == Edition.EDITION_PROTO2 && proto.getOptions().getPacked()) {
        if (features == null) {
          features = FeatureSet.newBuilder();
        }
        features.setRepeatedFieldEncoding(FeatureSet.RepeatedFieldEncoding.PACKED);
      }

      if (getFile().getEdition() == Edition.EDITION_PROTO3) {
        if (proto.getOptions().hasPacked() && !proto.getOptions().getPacked()) {
          if (features == null) {
            features = FeatureSet.newBuilder();
          }
          features.setRepeatedFieldEncoding(FeatureSet.RepeatedFieldEncoding.EXPANDED);
        }

      }

      return features != null ? features.build() : FeatureSet.getDefaultInstance();
    }

    @Override
    void validateFeatures() throws DescriptorValidationException {
      if (containingType != null
          && containingType.toProto().getOptions().getMessageSetWireFormat()) {
        if (isExtension()) {
          if (isRequired() || isRepeated() || getType() != Type.MESSAGE) {
            throw new DescriptorValidationException(
                this, "Extensions of MessageSets may not be required or repeated messages.");
          }
        }
      }
    }

    /** Look up and cross-link all field types, etc. */
    private void crossLink() throws DescriptorValidationException {
      if (proto.hasExtendee()) {
        final GenericDescriptor extendee =
            getFile()
                .tables
                .lookupSymbol(
                    proto.getExtendee(),
                    this,
                    FileDescriptorTables.SearchFilter.TYPES_ONLY,
                    /* expectingEnum= */ false);
        if (!(extendee instanceof Descriptor)) {
          throw new DescriptorValidationException(
              this, '\"' + proto.getExtendee() + "\" is not a message type.");
        }
        containingType = (Descriptor) extendee;

        if (!getContainingType().isExtensionNumber(getNumber())) {
          throw new DescriptorValidationException(
              this,
              '\"'
                  + getContainingType().getFullName()
                  + "\" does not declare "
                  + getNumber()
                  + " as an extension number.");
        }
      }

      if (proto.hasTypeName()) {
        boolean expectingEnum =
            proto.getType() == FieldDescriptorProto.Type.TYPE_ENUM || proto.hasDefaultValue();
        final GenericDescriptor typeDescriptor =
            getFile()
                .tables
                .lookupSymbol(
                    proto.getTypeName(),
                    this,
                    FileDescriptorTables.SearchFilter.TYPES_ONLY,
                    expectingEnum);

        if (!proto.hasType()) {
          // Choose field type based on symbol.
          if (typeDescriptor instanceof Descriptor) {
            type = Type.MESSAGE;
          } else if (typeDescriptor instanceof EnumDescriptor) {
            type = Type.ENUM;
          } else {
            throw new DescriptorValidationException(
                this, '\"' + proto.getTypeName() + "\" is not a type.");
          }
        }

        // Use raw type since inferred type considers messageType which may not be fully cross
        // linked yet.
        if (type.getJavaType() == JavaType.MESSAGE) {
          if (!(typeDescriptor instanceof Descriptor)) {
            throw new DescriptorValidationException(
                this, '\"' + proto.getTypeName() + "\" is not a message type.");
          }
          this.typeDescriptor = typeDescriptor;

          if (proto.hasDefaultValue()) {
            throw new DescriptorValidationException(this, "Messages can't have default values.");
          }
        } else if (type.getJavaType() == JavaType.ENUM) {
          if (!(typeDescriptor instanceof EnumDescriptor)) {
            throw new DescriptorValidationException(
                this, '\"' + proto.getTypeName() + "\" is not an enum type.");
          }
          this.typeDescriptor = typeDescriptor;
        } else {
          throw new DescriptorValidationException(this, "Field with primitive type has type_name.");
        }
      } else {
        if (type.getJavaType() == JavaType.MESSAGE || type.getJavaType() == JavaType.ENUM) {
          throw new DescriptorValidationException(
              this, "Field with message or enum type missing type_name.");
        }
      }

      // Only repeated primitive fields may be packed.
      if (proto.getOptions().getPacked() && !isPackable()) {
        throw new DescriptorValidationException(
            this, "[packed = true] can only be specified for repeated primitive fields.");
      }

      // We don't attempt to parse the default value until here because for
      // enums we need the enum type's descriptor.
      if (proto.hasDefaultValue()) {
        if (isRepeated()) {
          throw new DescriptorValidationException(
              this, "Repeated fields cannot have default values.");
        }

        try {
          switch (type) {
            case INT32:
            case SINT32:
            case SFIXED32:
              defaultValue = TextFormat.parseInt32(proto.getDefaultValue());
              break;
            case UINT32:
            case FIXED32:
              defaultValue = TextFormat.parseUInt32(proto.getDefaultValue());
              break;
            case INT64:
            case SINT64:
            case SFIXED64:
              defaultValue = TextFormat.parseInt64(proto.getDefaultValue());
              break;
            case UINT64:
            case FIXED64:
              defaultValue = TextFormat.parseUInt64(proto.getDefaultValue());
              break;
            case FLOAT:
              if (proto.getDefaultValue().equals("inf")) {
                defaultValue = Float.POSITIVE_INFINITY;
              } else if (proto.getDefaultValue().equals("-inf")) {
                defaultValue = Float.NEGATIVE_INFINITY;
              } else if (proto.getDefaultValue().equals("nan")) {
                defaultValue = Float.NaN;
              } else {
                defaultValue = Float.valueOf(proto.getDefaultValue());
              }
              break;
            case DOUBLE:
              if (proto.getDefaultValue().equals("inf")) {
                defaultValue = Double.POSITIVE_INFINITY;
              } else if (proto.getDefaultValue().equals("-inf")) {
                defaultValue = Double.NEGATIVE_INFINITY;
              } else if (proto.getDefaultValue().equals("nan")) {
                defaultValue = Double.NaN;
              } else {
                defaultValue = Double.valueOf(proto.getDefaultValue());
              }
              break;
            case BOOL:
              defaultValue = Boolean.valueOf(proto.getDefaultValue());
              break;
            case STRING:
              defaultValue = proto.getDefaultValue();
              break;
            case BYTES:
              try {
                defaultValue = TextFormat.unescapeBytes(proto.getDefaultValue());
              } catch (TextFormat.InvalidEscapeSequenceException e) {
                throw new DescriptorValidationException(
                    this, "Couldn't parse default value: " + e.getMessage(), e);
              }
              break;
            case ENUM:
              if (!getEnumType().isPlaceholder()) {
                defaultValue = getEnumType().findValueByName(proto.getDefaultValue());
                if (defaultValue == null) {
                  throw new DescriptorValidationException(
                      this, "Unknown enum default value: \"" + proto.getDefaultValue() + '\"');
                }
              }
              break;
            case MESSAGE:
            case GROUP:
              throw new DescriptorValidationException(this, "Message type had default value.");
          }
        } catch (NumberFormatException e) {
          throw new DescriptorValidationException(
              this, "Could not parse default value: \"" + proto.getDefaultValue() + '\"', e);
        }
      } else {
        // Determine the default default for this field.
        if (isRepeated()) {
          defaultValue = Collections.emptyList();
        } else {
          switch (type.getJavaType()) {
            case ENUM:
              // We guarantee elsewhere that an enum type always has at least
              // one possible value.
              defaultValue = getEnumType().getValue(0);
              break;
            case MESSAGE:
              defaultValue = null;
              break;
            default:
              defaultValue = type.getJavaType().defaultDefault;
              break;
          }
        }
      }
    }

    /** See {@link FileDescriptor#setProto}. */
    private void setProto(final FieldDescriptorProto proto) throws DescriptorValidationException {
      this.proto = proto;
      this.options = null;
      resolveFeatures(proto.getOptions().getFeatures());
    }

    @Override
    public boolean internalMessageIsImmutable(Object message) {
      return message instanceof MessageLite;
    }

    /** For internal use only. This is to satisfy the FieldDescriptorLite interface. */
    @Override
    public void internalMergeFrom(Object to, Object from) {

      ((Message.Builder) to).mergeFrom((Message) from);
    }
  }

  // =================================================================

  /** Describes an enum type. */
  public static final class EnumDescriptor extends GenericDescriptor
      implements Internal.EnumLiteMap<EnumValueDescriptor> {
    /**
     * Get the index of this descriptor within its parent.
     *
     * @see Descriptors.Descriptor#getIndex()
     */
    public int getIndex() {
      return index;
    }

    /** Convert the descriptor to its protocol message representation. */
    @Override
    public EnumDescriptorProto toProto() {
      return proto;
    }

    /** Get the type's unqualified name. */
    @Override
    public String getName() {
      return proto.getName();
    }

    /**
     * Get the type's fully-qualified name.
     *
     * @see Descriptors.Descriptor#getFullName()
     */
    @Override
    public String getFullName() {
      return fullName;
    }

    /** Get the {@link FileDescriptor} containing this descriptor. */
    @Override
    public FileDescriptor getFile() {
      return parent.getFile();
    }

    @Override
    GenericDescriptor getParent() {
      return parent;
    }

    public boolean isPlaceholder() {
      return placeholder;
    }

    /**
     * Determines if the given enum is closed.
     *
     * <p>Closed enum means that it:
     *
     * <ul>
     *   <li>Has a fixed set of values, rather than being equivalent to an int32.
     *   <li>Encountering values not in this set causes them to be treated as unknown fields.
     *   <li>The first value (i.e., the default) may be nonzero.
     * </ul>
     *
     * <p>WARNING: Some runtimes currently have a quirk where non-closed enums are treated as closed
     * when used as the type of fields defined in a `syntax = proto2;` file. This quirk is not
     * present in all runtimes; as of writing, we know that:
     *
     * <ul>
     *   <li>C++, Java, and C++-based Python share this quirk.
     *   <li>UPB and UPB-based Python do not.
     *   <li>PHP and Ruby treat all enums as open regardless of declaration.
     * </ul>
     *
     * <p>Care should be taken when using this function to respect the target runtime's enum
     * handling quirks.
     */
    public boolean isClosed() {
      return getFeatures().getEnumType() == DescriptorProtos.FeatureSet.EnumType.CLOSED;
    }

    /** If this is a nested type, get the outer descriptor, otherwise null. */
    public Descriptor getContainingType() {
      if (parent instanceof Descriptor) {
        return (Descriptor) parent;
      }
      return null;
    }

    /** Get the {@code EnumOptions}, defined in {@code descriptor.proto}. */
    public EnumOptions getOptions() {
      if (this.options == null) {
        EnumOptions strippedOptions = this.proto.getOptions();
        if (strippedOptions.hasFeatures()) {
          // Clients should be using feature accessor methods, not accessing features on the
          // options
          // proto.
          strippedOptions = strippedOptions.toBuilder().clearFeatures().build();
        }
        synchronized (this) {
          if (this.options == null) {
            this.options = strippedOptions;
          }
        }
      }
      return this.options;
    }

    /** Get a list of defined values for this enum. */
    public List<EnumValueDescriptor> getValues() {
      return Collections.unmodifiableList(Arrays.asList(values));
    }

    public int getValueCount() {
      return values.length;
    }

    public EnumValueDescriptor getValue(int index) {
      return values[index];
    }

    /** Determines if the given field number is reserved. */
    public boolean isReservedNumber(final int number) {
      for (final EnumDescriptorProto.EnumReservedRange range : proto.getReservedRangeList()) {
        if (range.getStart() <= number && number <= range.getEnd()) {
          return true;
        }
      }
      return false;
    }

    /** Determines if the given field name is reserved. */
    public boolean isReservedName(final String name) {
      checkNotNull(name);
      for (final String reservedName : proto.getReservedNameList()) {
        if (reservedName.equals(name)) {
          return true;
        }
      }
      return false;
    }

    /**
     * Find an enum value by name.
     *
     * @param name the unqualified name of the value such as "FOO"
     * @return the value's descriptor, or {@code null} if not found
     */
    public EnumValueDescriptor findValueByName(final String name) {
      final GenericDescriptor result = getFile().tables.findSymbol(fullName + '.' + name);
      if (result instanceof EnumValueDescriptor) {
        return (EnumValueDescriptor) result;
      } else {
        return null;
      }
    }

    /**
     * Find an enum value by number. If multiple enum values have the same number, this returns the
     * first defined value with that number.
     *
     * @param number The value's number.
     * @return the value's descriptor, or {@code null} if not found.
     */
    @Override
    public EnumValueDescriptor findValueByNumber(final int number) {
      return binarySearch(
          valuesSortedByNumber, distinctNumbers, EnumValueDescriptor.NUMBER_GETTER, number);
    }

    private static class UnknownEnumValueReference extends WeakReference<EnumValueDescriptor> {
      private final int number;

      private UnknownEnumValueReference(int number, EnumValueDescriptor descriptor) {
        super(descriptor);
        this.number = number;
      }
    }

    /**
     * Get the enum value for a number. If no enum value has this number, construct an
     * EnumValueDescriptor for it.
     */
    public EnumValueDescriptor findValueByNumberCreatingIfUnknown(final int number) {
      EnumValueDescriptor result = findValueByNumber(number);
      if (result != null) {
        return result;
      }
      // The number represents an unknown enum value.
      synchronized (this) {
        if (cleanupQueue == null) {
          cleanupQueue = new ReferenceQueue<>();
          unknownValues = new HashMap<>();
        } else {
          while (true) {
            UnknownEnumValueReference toClean = (UnknownEnumValueReference) cleanupQueue.poll();
            if (toClean == null) {
              break;
            }
            unknownValues.remove(toClean.number);
          }
        }

        // There are two ways we can be missing a value: it wasn't in the map, or the reference
        // has been GC'd.  (It may even have been GC'd since we cleaned up the references a few
        // lines of code ago.)  So get out the reference, if it's still present...
        WeakReference<EnumValueDescriptor> reference = unknownValues.get(number);
        result = (reference == null) ? null : reference.get();

        if (result == null) {
          result = new EnumValueDescriptor(this, number);
          unknownValues.put(number, new UnknownEnumValueReference(number, result));
        }
      }
      return result;
    }

    // Used in tests only.
    int getUnknownEnumValueDescriptorCount() {
      return unknownValues.size();
    }

    private final int index;
    private EnumDescriptorProto proto;
    private volatile EnumOptions options;
    private final String fullName;
    private final GenericDescriptor parent;
    private final EnumValueDescriptor[] values;
    private final EnumValueDescriptor[] valuesSortedByNumber;
    private final int distinctNumbers;
    private Map<Integer, WeakReference<EnumValueDescriptor>> unknownValues = null;
    private ReferenceQueue<EnumValueDescriptor> cleanupQueue = null;
    private final boolean placeholder;

    // Used to create a placeholder when the type cannot be found.
    private EnumDescriptor(final String fullName) throws DescriptorValidationException {
      String name = fullName;
      String packageName = "";
      int pos = fullName.lastIndexOf('.');
      if (pos != -1) {
        name = fullName.substring(pos + 1);
        packageName = fullName.substring(0, pos);
      }
      this.index = 0;
      this.proto =
          EnumDescriptorProto.newBuilder()
              .setName(name)
              .addValue(
                  EnumValueDescriptorProto.newBuilder()
                      .setName("PLACEHOLDER_VALUE")
                      .setNumber(0)
                      .build())
              .build();
      this.fullName = fullName;

      this.values = new EnumValueDescriptor[1];
      this.values[0] = new EnumValueDescriptor(this, 0);
      this.valuesSortedByNumber = this.values.clone();
      this.distinctNumbers = 1;

      // Create a placeholder FileDescriptor to hold this message.
      this.parent = new FileDescriptor(packageName, this);

      placeholder = true;
    }

    private EnumDescriptor(
        final EnumDescriptorProto proto,
        final FileDescriptor file,
        final Descriptor parent,
        final int index)
        throws DescriptorValidationException {
      if (parent == null) {
        this.parent = file;
      } else {
        this.parent = parent;
      }
      this.index = index;
      this.proto = proto;
      this.placeholder = false;
      fullName = computeFullName(file, parent, proto.getName());

      if (proto.getValueCount() == 0) {
        // We cannot allow enums with no values because this would mean there
        // would be no valid default value for fields of this type.
        throw new DescriptorValidationException(this, "Enums must contain at least one value.");
      }

      values = new EnumValueDescriptor[proto.getValueCount()];
      for (int i = 0; i < proto.getValueCount(); i++) {
        values[i] = new EnumValueDescriptor(proto.getValue(i), this, i);
      }
      valuesSortedByNumber = values.clone();
      Arrays.sort(valuesSortedByNumber, EnumValueDescriptor.BY_NUMBER);
      // deduplicate
      int j = 0;
      for (int i = 1; i < proto.getValueCount(); i++) {
        EnumValueDescriptor oldValue = valuesSortedByNumber[j];
        EnumValueDescriptor newValue = valuesSortedByNumber[i];
        if (oldValue.getNumber() != newValue.getNumber()) {
          valuesSortedByNumber[++j] = newValue;
        }
      }
      this.distinctNumbers = j + 1;
      Arrays.fill(valuesSortedByNumber, distinctNumbers, proto.getValueCount(), null);

      file.tables.addSymbol(this);
    }

    /** See {@link FileDescriptor#resolveAllFeatures}. */
    private void resolveAllFeatures() throws DescriptorValidationException {
      resolveFeatures(proto.getOptions().getFeatures());

      for (EnumValueDescriptor value : values) {
        value.resolveAllFeatures();
      }
    }

    /** See {@link FileDescriptor#setProto}. */
    private void setProto(final EnumDescriptorProto proto) throws DescriptorValidationException {
      this.proto = proto;
      this.options = null;
      resolveFeatures(proto.getOptions().getFeatures());

      for (int i = 0; i < values.length; i++) {
        values[i].setProto(proto.getValue(i));
      }
    }
  }

  // =================================================================

  /**
   * Describes one value within an enum type. Note that multiple defined values may have the same
   * number. In generated Java code, all values with the same number after the first become aliases
   * of the first. However, they still have independent EnumValueDescriptors.
   */
  @SuppressWarnings("ShouldNotSubclass")
  public static final class EnumValueDescriptor extends GenericDescriptor
      implements Internal.EnumLite {
    static final Comparator<EnumValueDescriptor> BY_NUMBER =
        new Comparator<EnumValueDescriptor>() {
          @Override
          public int compare(EnumValueDescriptor o1, EnumValueDescriptor o2) {
            return Integer.compare(o1.getNumber(), o2.getNumber());
          }
        };

    static final ToIntFunction<EnumValueDescriptor> NUMBER_GETTER = EnumValueDescriptor::getNumber;

    /**
     * Get the index of this descriptor within its parent.
     *
     * @see Descriptors.Descriptor#getIndex()
     */
    public int getIndex() {
      return index;
    }

    /** Convert the descriptor to its protocol message representation. */
    @Override
    public EnumValueDescriptorProto toProto() {
      return proto;
    }

    /** Get the value's unqualified name. */
    @Override
    public String getName() {
      return proto.getName();
    }

    /** Get the value's number. */
    @Override
    public int getNumber() {
      return proto.getNumber();
    }

    @Override
    public String toString() {
      return proto.getName();
    }

    /**
     * Get the value's fully-qualified name.
     *
     * @see Descriptors.Descriptor#getFullName()
     */
    @Override
    public String getFullName() {
      return fullName;
    }

    /** Get the {@link FileDescriptor} containing this descriptor. */
    @Override
    public FileDescriptor getFile() {
      return type.getFile();
    }

    @Override
    GenericDescriptor getParent() {
      return type;
    }

    /** Get the value's enum type. */
    public EnumDescriptor getType() {
      return type;
    }

    /** Get the {@code EnumValueOptions}, defined in {@code descriptor.proto}. */
    public EnumValueOptions getOptions() {
      if (this.options == null) {
        EnumValueOptions strippedOptions = this.proto.getOptions();
        if (strippedOptions.hasFeatures()) {
          // Clients should be using feature accessor methods, not accessing features on the
          // options
          // proto.
          strippedOptions = strippedOptions.toBuilder().clearFeatures().build();
        }
        synchronized (this) {
          if (this.options == null) {
            this.options = strippedOptions;
          }
        }
      }
      return this.options;
    }

    private final int index;
    private EnumValueDescriptorProto proto;
    private volatile EnumValueOptions options;
    private final String fullName;
    private final EnumDescriptor type;

    private EnumValueDescriptor(
        final EnumValueDescriptorProto proto, final EnumDescriptor parent, final int index)
        throws DescriptorValidationException {
      this.index = index;
      this.proto = proto;
      this.type = parent;
      this.fullName = parent.getFullName() + '.' + proto.getName();
      type.getFile().tables.addSymbol(this);
    }

    // Create an unknown enum value.
    private EnumValueDescriptor(final EnumDescriptor parent, final Integer number) {
      String name = "UNKNOWN_ENUM_VALUE_" + parent.getName() + "_" + number;
      EnumValueDescriptorProto proto =
          EnumValueDescriptorProto.newBuilder().setName(name).setNumber(number).build();
      this.index = -1;
      this.proto = proto;
      this.type = parent;
      this.fullName = parent.getFullName() + '.' + proto.getName();

      // Don't add this descriptor into tables.
    }

    /** See {@link FileDescriptor#resolveAllFeatures}. */
    private void resolveAllFeatures() throws DescriptorValidationException {
      resolveFeatures(proto.getOptions().getFeatures());
    }

    /** See {@link FileDescriptor#setProto}. */
    private void setProto(final EnumValueDescriptorProto proto)
        throws DescriptorValidationException {
      this.proto = proto;
      this.options = null;
      resolveFeatures(proto.getOptions().getFeatures());
    }
  }

  // =================================================================

  /** Describes a service type. */
  public static final class ServiceDescriptor extends GenericDescriptor {
    /**
     * Get the index of this descriptor within its parent. * @see Descriptors.Descriptor#getIndex()
     */
    public int getIndex() {
      return index;
    }

    /** Convert the descriptor to its protocol message representation. */
    @Override
    public ServiceDescriptorProto toProto() {
      return proto;
    }

    /** Get the type's unqualified name. */
    @Override
    public String getName() {
      return proto.getName();
    }

    /**
     * Get the type's fully-qualified name.
     *
     * @see Descriptors.Descriptor#getFullName()
     */
    @Override
    public String getFullName() {
      return fullName;
    }

    /** Get the {@link FileDescriptor} containing this descriptor. */
    @Override
    public FileDescriptor getFile() {
      return file;
    }

    @Override
    GenericDescriptor getParent() {
      return file;
    }

    /** Get the {@code ServiceOptions}, defined in {@code descriptor.proto}. */
    public ServiceOptions getOptions() {
      if (this.options == null) {
        ServiceOptions strippedOptions = this.proto.getOptions();
        if (strippedOptions.hasFeatures()) {
          // Clients should be using feature accessor methods, not accessing features on the
          // options
          // proto.
          strippedOptions = strippedOptions.toBuilder().clearFeatures().build();
        }
        synchronized (this) {
          if (this.options == null) {
            this.options = strippedOptions;
          }
        }
      }
      return this.options;
    }

    /** Get a list of methods for this service. */
    public List<MethodDescriptor> getMethods() {
      return Collections.unmodifiableList(Arrays.asList(methods));
    }

    /** Get a count of methods for this service. */
    public int getMethodCount() {
      return methods.length;
    }

    /** Get the service's method at the specified index. */
    public MethodDescriptor getMethod(int index) {
      return methods[index];
    }

    /**
     * Find a method by name.
     *
     * @param name the unqualified name of the method such as "Foo"
     * @return the method's descriptor, or {@code null} if not found
     */
    public MethodDescriptor findMethodByName(final String name) {
      final GenericDescriptor result = file.tables.findSymbol(fullName + '.' + name);
      if (result instanceof MethodDescriptor) {
        return (MethodDescriptor) result;
      } else {
        return null;
      }
    }

    private final int index;
    private ServiceDescriptorProto proto;
    private volatile ServiceOptions options;
    private final String fullName;
    private final FileDescriptor file;
    private MethodDescriptor[] methods;

    private ServiceDescriptor(
        final ServiceDescriptorProto proto, final FileDescriptor file, final int index)
        throws DescriptorValidationException {
      this.index = index;
      this.proto = proto;
      fullName = computeFullName(file, null, proto.getName());
      this.file = file;

      methods = new MethodDescriptor[proto.getMethodCount()];
      for (int i = 0; i < proto.getMethodCount(); i++) {
        methods[i] = new MethodDescriptor(proto.getMethod(i), this, i);
      }

      file.tables.addSymbol(this);
    }

    /** See {@link FileDescriptor#resolveAllFeatures}. */
    private void resolveAllFeatures() throws DescriptorValidationException {
      resolveFeatures(proto.getOptions().getFeatures());

      for (MethodDescriptor method : methods) {
        method.resolveAllFeatures();
      }
    }

    private void crossLink() throws DescriptorValidationException {
      for (final MethodDescriptor method : methods) {
        method.crossLink();
      }
    }

    /** See {@link FileDescriptor#setProto}. */
    private void setProto(final ServiceDescriptorProto proto) throws DescriptorValidationException {
      this.proto = proto;
      this.options = null;
      resolveFeatures(proto.getOptions().getFeatures());

      for (int i = 0; i < methods.length; i++) {
        methods[i].setProto(proto.getMethod(i));
      }
    }
  }

  // =================================================================

  /** Describes one method within a service type. */
  public static final class MethodDescriptor extends GenericDescriptor {
    /**
     * Get the index of this descriptor within its parent. * @see Descriptors.Descriptor#getIndex()
     */
    public int getIndex() {
      return index;
    }

    /** Convert the descriptor to its protocol message representation. */
    @Override
    public MethodDescriptorProto toProto() {
      return proto;
    }

    /** Get the method's unqualified name. */
    @Override
    public String getName() {
      return proto.getName();
    }

    /**
     * Get the method's fully-qualified name.
     *
     * @see Descriptors.Descriptor#getFullName()
     */
    @Override
    public String getFullName() {
      return fullName;
    }

    /** Get the {@link FileDescriptor} containing this descriptor. */
    @Override
    public FileDescriptor getFile() {
      return service.file;
    }

    @Override
    GenericDescriptor getParent() {
      return service;
    }

    /** Get the method's service type. */
    public ServiceDescriptor getService() {
      return service;
    }

    /** Get the method's input type. */
    public Descriptor getInputType() {
      return inputType;
    }

    /** Get the method's output type. */
    public Descriptor getOutputType() {
      return outputType;
    }

    /** Get whether or not the inputs are streaming. */
    public boolean isClientStreaming() {
      return proto.getClientStreaming();
    }

    /** Get whether or not the outputs are streaming. */
    public boolean isServerStreaming() {
      return proto.getServerStreaming();
    }

    /** Get the {@code MethodOptions}, defined in {@code descriptor.proto}. */
    public MethodOptions getOptions() {
      if (this.options == null) {
        MethodOptions strippedOptions = this.proto.getOptions();
        if (strippedOptions.hasFeatures()) {
          // Clients should be using feature accessor methods, not accessing features on the
          // options
          // proto.
          strippedOptions = strippedOptions.toBuilder().clearFeatures().build();
        }
        synchronized (this) {
          if (this.options == null) {
            this.options = strippedOptions;
          }
        }
      }
      return this.options;
    }

    private final int index;
    private MethodDescriptorProto proto;
    private volatile MethodOptions options;
    private final String fullName;
    private final ServiceDescriptor service;

    // Initialized during cross-linking.
    private Descriptor inputType;
    private Descriptor outputType;

    private MethodDescriptor(
        final MethodDescriptorProto proto, final ServiceDescriptor parent, final int index)
        throws DescriptorValidationException {
      this.index = index;
      this.proto = proto;
      this.service = parent;

      fullName = parent.getFullName() + '.' + proto.getName();

      service.file.tables.addSymbol(this);
    }

    /** See {@link FileDescriptor#resolveAllFeatures}. */
    private void resolveAllFeatures() throws DescriptorValidationException {
      resolveFeatures(proto.getOptions().getFeatures());
    }

    private void crossLink() throws DescriptorValidationException {
      final GenericDescriptor input =
          getFile()
              .tables
              .lookupSymbol(
                  proto.getInputType(),
                  this,
                  FileDescriptorTables.SearchFilter.TYPES_ONLY,
                  /* expectingEnum= */ false);
      if (!(input instanceof Descriptor)) {
        throw new DescriptorValidationException(
            this, '\"' + proto.getInputType() + "\" is not a message type.");
      }
      inputType = (Descriptor) input;

      final GenericDescriptor output =
          getFile()
              .tables
              .lookupSymbol(
                  proto.getOutputType(),
                  this,
                  FileDescriptorTables.SearchFilter.TYPES_ONLY,
                  /* expectingEnum= */ false);
      if (!(output instanceof Descriptor)) {
        throw new DescriptorValidationException(
            this, '\"' + proto.getOutputType() + "\" is not a message type.");
      }
      outputType = (Descriptor) output;
    }

    /** See {@link FileDescriptor#setProto}. */
    private void setProto(final MethodDescriptorProto proto) throws DescriptorValidationException {
      this.proto = proto;
      this.options = null;
      resolveFeatures(proto.getOptions().getFeatures());
    }
  }

  // =================================================================

  /**
   * @throws NullPointerException if both `file` and `parent` are null.
   */
  private static String computeFullName(
      final FileDescriptor file, final Descriptor parent, final String name) {
    if (parent != null) {
      return parent.getFullName() + '.' + name;
    }

    final String packageName = file.getPackage();
    if (!packageName.isEmpty()) {
      return packageName + '.' + name;
    }

    return name;
  }

  // =================================================================

  /**
   * All descriptors implement this to make it easier to implement tools like {@code
   * FileDescriptorTables}.
   */
  public abstract static class GenericDescriptor {
    // Private constructor to prevent subclasses outside of com.google.protobuf.Descriptors
    private GenericDescriptor() {}

    public abstract Message toProto();

    public abstract String getName();

    public abstract String getFullName();

    public abstract FileDescriptor getFile();

    abstract GenericDescriptor getParent();

    void resolveFeatures(FeatureSet unresolvedFeatures) throws DescriptorValidationException {
      GenericDescriptor parent = getParent();
      FeatureSet inferredLegacyFeatures = null;
      if (parent != null
          && unresolvedFeatures.equals(FeatureSet.getDefaultInstance())
          && (inferredLegacyFeatures = inferLegacyProtoFeatures())
              .equals(FeatureSet.getDefaultInstance())) {
        this.features = parent.features;
        validateFeatures();
        return;
      }

      // Java features from a custom pool (i.e. buildFrom) may end up in unknown fields or
      // use a different descriptor from the generated pool used by the Java runtime.
      boolean hasPossibleCustomJavaFeature = false;
      for (FieldDescriptor f : unresolvedFeatures.getExtensionFields().keySet()) {
        if (f.getNumber() == JavaFeaturesProto.java_.getNumber()
            && f != JavaFeaturesProto.java_.getDescriptor()) {
          hasPossibleCustomJavaFeature = true;
          break;
        }
      }
      boolean hasPossibleUnknownJavaFeature =
          !unresolvedFeatures.getUnknownFields().isEmpty()
              && (unresolvedFeatures
                      .getUnknownFields()
                      .hasField(JavaFeaturesProto.java_.getNumber())
              );
      if (hasPossibleCustomJavaFeature || hasPossibleUnknownJavaFeature) {
        ExtensionRegistry registry = ExtensionRegistry.newInstance();
        registry.add(JavaFeaturesProto.java_);
        ByteString bytes = unresolvedFeatures.toByteString();
        try {
          unresolvedFeatures = FeatureSet.parseFrom(bytes, registry);
        } catch (InvalidProtocolBufferException e) {
          throw new DescriptorValidationException(
              this, "Failed to parse features with Java feature extension registry.", e);
        }
      }

      FeatureSet.Builder features;
      if (parent == null) {
        Edition edition = getFile().getEdition();
        features = getEditionDefaults(edition).toBuilder();
      } else {
        features = parent.features.toBuilder();
      }
      if (inferredLegacyFeatures == null) {
        inferredLegacyFeatures = inferLegacyProtoFeatures();
      }
      features.mergeFrom(inferredLegacyFeatures);
      features.mergeFrom(unresolvedFeatures);
      this.features = internFeatures(features.build());
      validateFeatures();
    }

    FeatureSet inferLegacyProtoFeatures() {
      return FeatureSet.getDefaultInstance();
    }

    void validateFeatures() throws DescriptorValidationException {}

    FeatureSet getFeatures() {
      // TODO: Remove lazy resolution of unresolved features for legacy syntax for
      // compatibility with older <4.26.x gencode in the next breaking release.
      if (this.features == null
          && (getFile().getEdition() == Edition.EDITION_PROTO2
              || getFile().getEdition() == Edition.EDITION_PROTO3)) {
        getFile().resolveAllFeaturesImmutable();
      }
      if (this.features == null) {
        throw new NullPointerException(
            String.format("Features not yet loaded for %s.", getFullName()));
      }
      return this.features;
    }

    volatile FeatureSet features;
  }

  /** Thrown when building descriptors fails because the source DescriptorProtos are not valid. */
  public static class DescriptorValidationException extends Exception {
    private static final long serialVersionUID = 5750205775490483148L;

    /** Gets the full name of the descriptor where the error occurred. */
    public String getProblemSymbolName() {
      return name;
    }

    /** Gets the protocol message representation of the invalid descriptor. */
    public Message getProblemProto() {
      return proto;
    }

    /** Gets a human-readable description of the error. */
    public String getDescription() {
      return description;
    }

    private final String name;
    private final Message proto;
    private final String description;

    private DescriptorValidationException(
        final GenericDescriptor problemDescriptor, final String description) {
      super(problemDescriptor.getFullName() + ": " + description);

      // Note that problemDescriptor may be partially uninitialized, so we
      // don't want to expose it directly to the user.  So, we only provide
      // the name and the original proto.
      name = problemDescriptor.getFullName();
      proto = problemDescriptor.toProto();
      this.description = description;
    }

    private DescriptorValidationException(
        final GenericDescriptor problemDescriptor,
        final String description,
        final Throwable cause) {
      this(problemDescriptor, description);
      initCause(cause);
    }

    private DescriptorValidationException(
        final FileDescriptor problemDescriptor, final String description) {
      super(problemDescriptor.getName() + ": " + description);

      // Note that problemDescriptor may be partially uninitialized, so we
      // don't want to expose it directly to the user.  So, we only provide
      // the name and the original proto.
      name = problemDescriptor.getName();
      proto = problemDescriptor.toProto();
      this.description = description;
    }
  }

  // =================================================================

  /**
   * A private helper class which contains lookup tables containing all the descriptors defined in a
   * particular file.
   */
  private static final class FileDescriptorTables {

    /** Defines what subclass of descriptors to search in the descriptor tables. */
    enum SearchFilter {
      TYPES_ONLY,
      AGGREGATES_ONLY,
      ALL_SYMBOLS
    }

    FileDescriptorTables(final FileDescriptor[] dependencies, boolean allowUnknownDependencies) {
      this.dependencies =
          Collections.newSetFromMap(
              new IdentityHashMap<FileDescriptor, Boolean>(dependencies.length));
      this.allowUnknownDependencies = allowUnknownDependencies;

      for (Descriptors.FileDescriptor dependency : dependencies) {
        this.dependencies.add(dependency);
        importPublicDependencies(dependency);
      }

      for (final FileDescriptor dependency : this.dependencies) {
        try {
          addPackage(dependency.getPackage(), dependency);
        } catch (DescriptorValidationException e) {
          // Can't happen, because addPackage() only fails when the name
          // conflicts with a non-package, but we have not yet added any
          // non-packages at this point.
          throw new AssertionError(e);
        }
      }
    }

    /** Find and put public dependencies of the file into dependencies set. */
    private void importPublicDependencies(final FileDescriptor file) {
      for (FileDescriptor dependency : file.getPublicDependencies()) {
        if (dependencies.add(dependency)) {
          importPublicDependencies(dependency);
        }
      }
    }

    private final Set<FileDescriptor> dependencies;
    private final boolean allowUnknownDependencies;

    private final Map<String, GenericDescriptor> descriptorsByName = new HashMap<>();

    /** Find a generic descriptor by fully-qualified name. */
    GenericDescriptor findSymbol(final String fullName) {
      return findSymbol(fullName, SearchFilter.ALL_SYMBOLS);
    }

    /**
     * Find a descriptor by fully-qualified name and given option to only search valid field type
     * descriptors.
     */
    GenericDescriptor findSymbol(final String fullName, final SearchFilter filter) {
      GenericDescriptor result = descriptorsByName.get(fullName);
      if (result != null) {
        if ((filter == SearchFilter.ALL_SYMBOLS)
            || ((filter == SearchFilter.TYPES_ONLY) && isType(result))
            || ((filter == SearchFilter.AGGREGATES_ONLY) && isAggregate(result))) {
          return result;
        }
      }

      for (final FileDescriptor dependency : dependencies) {
        result = dependency.tables.descriptorsByName.get(fullName);
        if (result != null) {
          if ((filter == SearchFilter.ALL_SYMBOLS)
              || ((filter == SearchFilter.TYPES_ONLY) && isType(result))
              || ((filter == SearchFilter.AGGREGATES_ONLY) && isAggregate(result))) {
            return result;
          }
        }
      }

      return null;
    }

    /** Checks if the descriptor is a valid type for a message field. */
    boolean isType(GenericDescriptor descriptor) {
      return (descriptor instanceof Descriptor) || (descriptor instanceof EnumDescriptor);
    }

    /** Checks if the descriptor is a valid namespace type. */
    boolean isAggregate(GenericDescriptor descriptor) {
      return (descriptor instanceof Descriptor)
          || (descriptor instanceof EnumDescriptor)
          || (descriptor instanceof PackageDescriptor)
          || (descriptor instanceof ServiceDescriptor);
    }

    /**
     * Look up a type descriptor by name, relative to some other descriptor. The name may be
     * fully-qualified (with a leading '.'), partially-qualified, or unqualified. C++-like name
     * lookup semantics are used to search for the matching descriptor.
     */
    GenericDescriptor lookupSymbol(
        final String name,
        final GenericDescriptor relativeTo,
        final FileDescriptorTables.SearchFilter filter,
        final boolean expectingEnum)
        throws DescriptorValidationException {

      GenericDescriptor result;
      String fullname;
      if (name.startsWith(".")) {
        // Fully-qualified name.
        fullname = name.substring(1);
        result = findSymbol(fullname, filter);
      } else {
        // If "name" is a compound identifier, we want to search for the
        // first component of it, then search within it for the rest.
        // If name is something like "Foo.Bar.baz", and symbols named "Foo" are
        // defined in multiple parent scopes, we only want to find "Bar.baz" in
        // the innermost one.  E.g., the following should produce an error:
        //   message Bar { message Baz {} }
        //   message Foo {
        //     message Bar {
        //     }
        //     optional Bar.Baz baz = 1;
        //   }
        // So, we look for just "Foo" first, then look for "Bar.baz" within it
        // if found.
        final int firstPartLength = name.indexOf('.');
        final String firstPart;
        if (firstPartLength == -1) {
          firstPart = name;
        } else {
          firstPart = name.substring(0, firstPartLength);
        }

        // We will search each parent scope of "relativeTo" looking for the
        // symbol.
        final StringBuilder scopeToTry = new StringBuilder(relativeTo.getFullName());

        while (true) {
          // Chop off the last component of the scope.
          final int dotpos = scopeToTry.lastIndexOf(".");
          if (dotpos == -1) {
            fullname = name;
            result = findSymbol(name, filter);
            break;
          } else {
            scopeToTry.setLength(dotpos + 1);

            // Append firstPart and try to find
            scopeToTry.append(firstPart);
            result =
                findSymbol(
                    scopeToTry.toString(), FileDescriptorTables.SearchFilter.AGGREGATES_ONLY);

            if (result != null) {
              if (firstPartLength != -1) {
                // We only found the first part of the symbol.  Now look for
                // the whole thing.  If this fails, we *don't* want to keep
                // searching parent scopes.
                scopeToTry.setLength(dotpos + 1);
                scopeToTry.append(name);
                result = findSymbol(scopeToTry.toString(), filter);
                fullname = scopeToTry.toString();
                break;
              }
              if (filter != FileDescriptorTables.SearchFilter.TYPES_ONLY || isType(result)) {
                fullname = scopeToTry.toString();
                break;
              }
              // We found the symbol, but it's not a type and we're looking for types only.
              // Keep looking.
            }

            // Not found.  Remove the name so we can try again.
            scopeToTry.setLength(dotpos);
          }
        }
      }

      if (result == null) {
        if (allowUnknownDependencies && filter == SearchFilter.TYPES_ONLY) {
          logger.warning(
              "The descriptor for type \""
                  + name
                  + "\" cannot be found and a placeholder is created for it");
          // If we have good reason to believe that the type is an enum, create an EnumDescriptor
          // placeholder here. Otherwise, create a Descriptor placeholder.  If we're wrong, a
          // DescriptorValidationException will be thrown later.
          if (expectingEnum) {
            result = new EnumDescriptor(fullname);
          } else {
            result = new Descriptor(fullname);
          }
          // Add the placeholder file as a dependency so we can find the
          // placeholder symbol when resolving other references.
          this.dependencies.add(result.getFile());
          return result;
        } else {
          throw new DescriptorValidationException(relativeTo, '\"' + name + "\" is not defined.");
        }
      } else {
        return result;
      }
    }

    /**
     * Adds a symbol to the symbol table. If a symbol with the same name already exists, throws an
     * error.
     */
    void addSymbol(final GenericDescriptor descriptor) throws DescriptorValidationException {
      validateSymbolName(descriptor);

      final String fullName = descriptor.getFullName();

      final GenericDescriptor old = descriptorsByName.put(fullName, descriptor);
      if (old != null) {
        descriptorsByName.put(fullName, old);

        if (descriptor.getFile() == old.getFile()) {
          final int dotpos = fullName.lastIndexOf('.');
          if (dotpos == -1) {
            throw new DescriptorValidationException(
                descriptor, '\"' + fullName + "\" is already defined.");
          } else {
            throw new DescriptorValidationException(
                descriptor,
                '\"'
                    + fullName.substring(dotpos + 1)
                    + "\" is already defined in \""
                    + fullName.substring(0, dotpos)
                    + "\".");
          }
        } else {
          throw new DescriptorValidationException(
              descriptor,
              '\"'
                  + fullName
                  + "\" is already defined in file \""
                  + old.getFile().getName()
                  + "\".");
        }
      }
    }

    /**
     * Represents a package in the symbol table. We use PackageDescriptors just as placeholders so
     * that someone cannot define, say, a message type that has the same name as an existing
     * package.
     */
    private static final class PackageDescriptor extends GenericDescriptor {
      @Override
      public Message toProto() {
        return file.toProto();
      }

      @Override
      public String getName() {
        return name;
      }

      @Override
      public String getFullName() {
        return fullName;
      }

      @Override
      GenericDescriptor getParent() {
        return file;
      }

      @Override
      public FileDescriptor getFile() {
        return file;
      }

      PackageDescriptor(final String name, final String fullName, final FileDescriptor file) {
        this.file = file;
        this.fullName = fullName;
        this.name = name;
      }

      private final String name;
      private final String fullName;
      private final FileDescriptor file;
    }

    /**
     * Adds a package to the symbol tables. If a package by the same name already exists, that is
     * fine, but if some other kind of symbol exists under the same name, an exception is thrown. If
     * the package has multiple components, this also adds the parent package(s).
     */
    void addPackage(final String fullName, final FileDescriptor file)
        throws DescriptorValidationException {
      final int dotpos = fullName.lastIndexOf('.');
      final String name;
      if (dotpos == -1) {
        name = fullName;
      } else {
        addPackage(fullName.substring(0, dotpos), file);
        name = fullName.substring(dotpos + 1);
      }

      final GenericDescriptor old =
          descriptorsByName.put(fullName, new PackageDescriptor(name, fullName, file));
      if (old != null) {
        descriptorsByName.put(fullName, old);
        if (!(old instanceof PackageDescriptor)) {
          throw new DescriptorValidationException(
              file,
              '\"'
                  + name
                  + "\" is already defined (as something other than a "
                  + "package) in file \""
                  + old.getFile().getName()
                  + "\".");
        }
      }
    }

    /**
     * Verifies that the descriptor's name is valid. That is, it contains only letters, digits, and
     * underscores, and does not start with a digit.
     */
    static void validateSymbolName(final GenericDescriptor descriptor)
        throws DescriptorValidationException {
      final String name = descriptor.getName();
      if (name.length() == 0) {
        throw new DescriptorValidationException(descriptor, "Missing name.");
      }

      // Non-ASCII characters are not valid in protobuf identifiers, even
      // if they are letters or digits.
      // The first character must be a letter or '_'.
      // Subsequent characters may be letters, numbers, or digits.
      for (int i = 0; i < name.length(); i++) {
        final char c = name.charAt(i);
        if (('a' <= c && c <= 'z')
            || ('A' <= c && c <= 'Z')
            || (c == '_')
            || ('0' <= c && c <= '9' && i > 0)) {
          // Valid
          continue;
        }
        throw new DescriptorValidationException(
            descriptor, '\"' + name + "\" is not a valid identifier.");
      }
    }
  }

  /** Describes a oneof of a message type. */
  public static final class OneofDescriptor extends GenericDescriptor {
    /** Get the index of this descriptor within its parent. */
    public int getIndex() {
      return index;
    }

    @Override
    public String getName() {
      return proto.getName();
    }

    @Override
    public FileDescriptor getFile() {
      return containingType.getFile();
    }

    @Override
    GenericDescriptor getParent() {
      return containingType;
    }

    @Override
    public String getFullName() {
      return fullName;
    }

    public Descriptor getContainingType() {
      return containingType;
    }

    public int getFieldCount() {
      return fieldCount;
    }

    public OneofOptions getOptions() {
      if (this.options == null) {
        OneofOptions strippedOptions = this.proto.getOptions();
        if (strippedOptions.hasFeatures()) {
          // Clients should be using feature accessor methods, not accessing features on the
          // options
          // proto.
          strippedOptions = strippedOptions.toBuilder().clearFeatures().build();
        }
        synchronized (this) {
          if (this.options == null) {
            this.options = strippedOptions;
          }
        }
      }
      return this.options;
    }

    /** Get a list of this message type's fields. */
    public List<FieldDescriptor> getFields() {
      return Collections.unmodifiableList(Arrays.asList(fields));
    }

    public FieldDescriptor getField(int index) {
      return fields[index];
    }

    @Override
    public OneofDescriptorProto toProto() {
      return proto;
    }

    boolean isSynthetic() {
      return fields.length == 1 && fields[0].isProto3Optional;
    }

    /** See {@link FileDescriptor#resolveAllFeatures}. */
    private void resolveAllFeatures() throws DescriptorValidationException {
      resolveFeatures(proto.getOptions().getFeatures());
    }

    private void setProto(final OneofDescriptorProto proto) throws DescriptorValidationException {
      this.proto = proto;
      this.options = null;
      resolveFeatures(proto.getOptions().getFeatures());
    }

    private OneofDescriptor(
        final OneofDescriptorProto proto, final Descriptor parent, final int index)
        throws DescriptorValidationException {
      this.proto = proto;
      fullName = computeFullName(null, parent, proto.getName());
      this.index = index;

      containingType = parent;
      fieldCount = 0;
    }

    private final int index;
    private OneofDescriptorProto proto;
    private volatile OneofOptions options;
    private final String fullName;
    private final Descriptor containingType;

    private int fieldCount;
    private FieldDescriptor[] fields;
  }

  private static <T> T binarySearch(T[] array, int size, ToIntFunction<T> getter, int number) {
    int left = 0;
    int right = size - 1;

    while (left <= right) {
      int mid = (left + right) / 2;
      T midValue = array[mid];
      int midValueNumber = getter.applyAsInt(midValue);
      if (number < midValueNumber) {
        right = mid - 1;
      } else if (number > midValueNumber) {
        left = mid + 1;
      } else {
        return midValue;
      }
    }
    return null;
  }
}
