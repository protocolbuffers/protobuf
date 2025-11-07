// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.DescriptorProtos.FeatureSet;
import com.google.protobuf.DescriptorProtos.FeatureSetDefaults;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.MethodDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;

/** DescriptorPool is used to construct descriptors and provide access to those descriptors. */
abstract class DescriptorPool {

  /** Builder for {@link DescriptorPool}. */
  public static final class Builder {

    FeatureSetDefaults featureSetDefaults = null;
    boolean allowUnknownDependencies = false;

    /**
     * By default, it is an error if a FileDescriptorProto contains references to types or other
     * files that are not found in the DescriptorPool. If you call {@link
     * DescriptorPool#allowUnknownDependencies}, however, then unknown types and files will be
     * replaced by placeholder descriptors (which can be identified by the isPlaceholder() method).
     * This can allow you to perform some useful operations with a .proto file even if you do not
     * have access to other .proto files on which it depends. However, some heuristics must be used
     * to fill in the gaps in information, and these can lead to descriptors which are inaccurate.
     * For example, the DescriptorPool may be forced to guess whether an unknown type is a message
     * or an enum, as well as what package it resides in. Furthermore, placeholder types will not be
     * discoverable via {@link DescriptorPool#findMessageTypeByName} and similar methods, which
     * could confuse some descriptor-based algorithms. Generally, the results of this option should
     * be handled with extreme care.
     */
    @CanIgnoreReturnValue
    public Builder setAllowUnknownDependencies(boolean allowUnknownDependencies) {
      this.allowUnknownDependencies = true;
      return this;
    }

    public boolean getAllowUnknownDependencies() {
      return allowUnknownDependencies;
    }

    /**
     * Sets the default feature mappings used during the build. If this function isn't called, the
     * Java feature set defaults are used. If this function is called, these defaults will be used
     * instead. FeatureSetDefaults includes a minimum/maximum supported edition, which will be
     * enforced while building proto files.
     */
    @CanIgnoreReturnValue
    public Builder setFeatureSetDefaults(FeatureSetDefaults featureSetDefaults) {
      Descriptors.checkFeatureSetDefaults(featureSetDefaults);
      this.featureSetDefaults = featureSetDefaults;
      return this;
    }

    public FeatureSetDefaults getFeatureSetDefaults() {
      if (featureSetDefaults == null) {
        return Descriptors.getJavaEditionDefaults();
      }
      return featureSetDefaults;
    }

    public DescriptorPool build() {
      return new DynamicDescriptorPool(this);
    }

    private Builder() {}
  }

  /**
   * Returns a new empty instance of {@link DescriptorPool} with the default recommended
   * configuration. Descriptors can be added to the pool by calling {@link
   * DescriptorPool#buildFile}.
   */
  public static Builder newBuilder() {
    return new Builder();
  }

  // Package-protected default constructor to prevent custom implementations.
  DescriptorPool() {}

  @CanIgnoreReturnValue
  public abstract FileDescriptor buildFile(FileDescriptorProto file)
      throws DescriptorValidationException;

  /**
   * Find a {@link FileDescriptor} by its {@link FileDescriptor#getName}.
   *
   * @param name the name of the file descriptor
   * @return the file descriptor or {@code null}.
   */
  public abstract FileDescriptor findFileByName(String name);

  /**
   * Find a {@link FileDescriptor} which contains the symbol.
   *
   * @param symbol the name of the symbol
   * @return the file descriptor or {@code null}.
   */
  public abstract FileDescriptor findFileContainingSymbol(String symbol);

  /**
   * Find a {@link Descriptor} by its {@link Descriptor#getFullName}.
   *
   * @param name the name of the descriptor
   * @return the descriptor or {@code null}.
   */
  public abstract Descriptor findMessageTypeByName(String name);

  /**
   * Find a {@link FieldDescriptor} by its {@link FieldDescriptor#getFullName}.
   *
   * @param name the name of the field descriptor
   * @return the field descriptor or {@code null}.
   */
  public abstract FieldDescriptor findFieldByName(String name);

  /**
   * Find an extension {@link FieldDescriptor} by its {@link FieldDescriptor#getFullName}.
   *
   * @param name the name of the extension
   * @return the extension or {@code null}.
   */
  public abstract FieldDescriptor findExtensionByName(String name);

  /**
   * Find a {@link OneofDescriptor} by its {@link OneofDescriptor#getFullName}.
   *
   * @param name the name of the oneof descriptor
   * @return the oneof descriptor or {@code null}.
   */
  public abstract OneofDescriptor findOneofByName(String name);

  /**
   * Find a {@link EnumDescriptor} by its {@link EnumDescriptor#getFullName}.
   *
   * @param name the name of the enum descriptor
   * @return the enum descriptor or {@code null}.
   */
  public abstract EnumDescriptor findEnumTypeByName(String name);

  /**
   * Find a {@link EnumValueDescriptor} by its {@link EnumValueDescriptor#getFullName}.
   *
   * @param name the name of the enum value descriptor
   * @return the enum value descriptor or {@code null}.
   */
  public abstract EnumValueDescriptor findEnumValueByName(String name);

  /**
   * Find a {@link ServiceDescriptor} by its {@link ServiceDescriptor#getFullName}.
   *
   * @param name the name of the service descriptor
   * @return the service descriptor or {@code null}.
   */
  public abstract ServiceDescriptor findServiceByName(String name);

  /**
   * Find a {@link MethodDescriptor} by its {@link MethodDescriptor#getFullName}.
   *
   * @param name the name of the method descriptor
   * @return the method descriptor or {@code null}.
   */
  public abstract MethodDescriptor findMethodByName(String name);

  /**
   * Find an extension {@link FieldDescriptor} by its {@link FieldDescriptor#getContainingType} and
   * {@link FieldDescriptor#getNumber}.
   *
   * @param extendee the descriptor that the extension extends
   * @param number the number of the extension
   * @return the extension or {@code null}.
   */
  public abstract FieldDescriptor findExtensionByNumber(Descriptor extendee, int number);

  abstract FeatureSet internFeatures(FeatureSet features);
}
