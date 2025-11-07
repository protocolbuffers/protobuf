// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import com.google.protobuf.DescriptorProtos.FeatureSet;
import com.google.protobuf.DescriptorProtos.FeatureSetDefaults;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.DescriptorNameMap;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.GenericDescriptor;
import com.google.protobuf.Descriptors.MethodDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import java.util.Arrays;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

final class DynamicDescriptorPool extends DescriptorPool {

  private final Lock stateLock = new ReentrantLock();

  private final boolean allowUnknownDependencies;
  private final FeatureSetDefaults featureSetDefaults;

  private final ReadWriteLock readWriteLock = new ReentrantReadWriteLock();
  private final Lock readLock = readWriteLock.readLock();
  private final Lock writeLock = readWriteLock.writeLock();
  private final DescriptorNameMap<FileDescriptor> filesByName =
      new DescriptorNameMap<>(FileDescriptor::getFullName);
  private final DescriptorNameMap<GenericDescriptor> genericsByName =
      new DescriptorNameMap<>(GenericDescriptor::getFullName);
  private final ExtensionNumberMap<FieldDescriptor, String> extensionsByNumber =
      new ExtensionNumberMap<>(
          FieldDescriptor::getContainingTypeFullName, FieldDescriptor::getNumber);

  DynamicDescriptorPool(Builder builder) {
    this.allowUnknownDependencies = builder.allowUnknownDependencies;
    this.featureSetDefaults = builder.featureSetDefaults;
  }

  private final ConcurrentHashMap<FeatureSet, FeatureSet> featureSets = new ConcurrentHashMap<>();

  @Override
  @CanIgnoreReturnValue
  public FileDescriptor buildFile(FileDescriptorProto file) throws DescriptorValidationException {
    checkNotNull(file);

    FileDescriptor descriptor;

    stateLock.lock();
    try {
      descriptor =
          FileDescriptor.buildFrom(
              this,
              file,
              getDependencies(file),
              featureSetDefaults,
              allowUnknownDependencies,
              /* allowUnresolvedFeatures= */ false);
      addFile(descriptor);
    } finally {
      stateLock.unlock();
    }

    return descriptor;
  }

  @Override
  public FileDescriptor findFileByName(String name) {
    if (name.isEmpty()) {
      return null;
    }

    FileDescriptor descriptor;

    readLock.lock();
    try {
      descriptor = filesByName.get(name);
    } finally {
      readLock.unlock();
    }

    return descriptor;
  }

  @Override
  public FileDescriptor findFileContainingSymbol(String symbol) {
    if (symbol.isEmpty() || symbol.startsWith(".")) {
      return null;
    }

    GenericDescriptor descriptor;
    boolean useTable = false;

    readLock.lock();
    try {
      descriptor = genericsByName.get(symbol);
      if (descriptor == null) {
        int pos = symbol.lastIndexOf('.');
        if (pos > 0 && pos < symbol.length() - 1) {
          descriptor = genericsByName.get(symbol.substring(0, pos));
          useTable = true;
        }
      }
    } finally {
      readLock.unlock();
    }

    if (descriptor != null) {
      if (useTable) {
        // We only allow looking up non-extension fields, oneofs, and methods this way.
        descriptor =
            descriptor.getFile().tables.findSymbol(symbol, /* searchDependencies= */ false);
        if (descriptor == null) {
          return null;
        }
        if (descriptor instanceof FieldDescriptor) {
          FieldDescriptor fieldDescriptor = (FieldDescriptor) descriptor;
          if (fieldDescriptor.isExtension()) {
            return null;
          }
        } else if (!(descriptor instanceof MethodDescriptor)
            && !(descriptor instanceof OneofDescriptor)) {
          return null;
        }
      }
      return descriptor.getFile();
    }
    return null;
  }

  @Override
  public Descriptor findMessageTypeByName(String name) {
    return findGenericByName(name, Descriptor.class);
  }

  @Override
  public FieldDescriptor findFieldByName(String name) {
    if (name.startsWith(".")) {
      return null;
    }
    int pos = name.lastIndexOf('.');
    if (pos <= 0 || pos >= name.length() - 1) {
      return null;
    }
    Descriptor containingType = findMessageTypeByName(name.substring(0, pos));
    if (containingType == null) {
      return null;
    }
    FieldDescriptor descriptor =
        containingType
            .getFile()
            .tables
            .findSymbol(name, FieldDescriptor.class, /* searchDependencies= */ false);
    if (descriptor == null || descriptor.isExtension()) {
      return null;
    }
    return descriptor;
  }

  @Override
  public FieldDescriptor findExtensionByName(String name) {
    FieldDescriptor field = findGenericByName(name, FieldDescriptor.class);
    if (field == null || !field.isExtension()) {
      return null;
    }
    return field;
  }

  @Override
  public OneofDescriptor findOneofByName(String name) {
    if (name.startsWith(".")) {
      return null;
    }
    int pos = name.lastIndexOf('.');
    if (pos <= 0 || pos >= name.length() - 1) {
      return null;
    }
    Descriptor containingType = findMessageTypeByName(name.substring(0, pos));
    if (containingType == null) {
      return null;
    }
    return containingType
        .getFile()
        .tables
        .findSymbol(name, OneofDescriptor.class, /* searchDependencies= */ false);
  }

  @Override
  public EnumDescriptor findEnumTypeByName(String name) {
    return findGenericByName(name, EnumDescriptor.class);
  }

  @Override
  public EnumValueDescriptor findEnumValueByName(String name) {
    return findGenericByName(name, EnumValueDescriptor.class);
  }

  @Override
  public ServiceDescriptor findServiceByName(String name) {
    return findGenericByName(name, ServiceDescriptor.class);
  }

  @Override
  public MethodDescriptor findMethodByName(String name) {
    if (name.startsWith(".")) {
      return null;
    }
    int pos = name.lastIndexOf('.');
    if (pos <= 0 || pos >= name.length() - 1) {
      return null;
    }
    ServiceDescriptor containingType = findServiceByName(name.substring(0, pos));
    if (containingType == null) {
      return null;
    }
    return containingType
        .getFile()
        .tables
        .findSymbol(name, MethodDescriptor.class, /* searchDependencies= */ false);
  }

  @Override
  public FieldDescriptor findExtensionByNumber(Descriptor extendee, int number) {
    if (extendee.getExtensionRangeCount() == 0 || number <= 0) {
      return null;
    }

    FieldDescriptor descriptor;

    readLock.lock();
    try {
      descriptor = extensionsByNumber.get(extendee.getFullName(), number);
    } finally {
      readLock.unlock();
    }

    if (descriptor == null || descriptor.getContainingType() != extendee) {
      return null;
    }
    return descriptor;
  }

  private FileDescriptor[] getDependencies(FileDescriptorProto file) {
    final int dependencyCount = file.getDependencyCount();
    final int optionDependencyCount = file.getOptionDependencyCount();
    final int dependenciesCapacity = dependencyCount + optionDependencyCount;
    if (dependenciesCapacity == 0) {
      return Descriptors.EMPTY_FILE_DESCRIPTORS;
    }
    FileDescriptor[] dependencies = new FileDescriptor[dependenciesCapacity];
    int dependenciesSize = 0;

    readLock.lock();
    try {
      for (int i = 0; i < dependencyCount; ++i) {
        FileDescriptor dependency = filesByName.get(file.getDependency(i));
        if (dependency != null) {
          dependencies[dependenciesSize++] = dependency;
        }
      }
      for (int i = 0; i < optionDependencyCount; ++i) {
        FileDescriptor dependency = filesByName.get(file.getOptionDependency(i));
        if (dependency != null) {
          dependencies[dependenciesSize++] = dependency;
        }
      }
    } finally {
      readLock.unlock();
    }

    if (dependenciesSize < dependenciesCapacity) {
      return Arrays.copyOf(dependencies, dependenciesSize);
    }
    return dependencies;
  }

  @Override
  FeatureSet internFeatures(FeatureSet features) {
    // This is only called when building descriptors and stateLock is already held when that is
    // happening.
    FeatureSet internedFeatures = featureSets.putIfAbsent(features, features);
    if (internedFeatures != null) {
      return internedFeatures;
    }
    return features;
  }

  @SuppressWarnings("unchecked") // We check with isInstance
  private <T extends GenericDescriptor> T findGenericByName(String name, Class<T> clazz) {
    if (name.isEmpty() || name.startsWith(".")) {
      return null;
    }
    checkNotNull(clazz);

    GenericDescriptor descriptor;

    readLock.lock();
    try {
      descriptor = genericsByName.get(name);
    } finally {
      readLock.unlock();
    }

    if (descriptor == null || !clazz.isInstance(descriptor)) {
      return null;
    }
    return (T) descriptor;
  }

  /**
   * Adds the symbols from the file descriptor tables to the descriptor pool in a single
   * transaction. If duplicate symbols are somehow detected, the transaction is rolled back. All
   * callers of the descriptor table will only observe the descriptor pool immediately before the
   * symbols are added or immediately after, no partials.
   */
  private void addFile(FileDescriptor file) throws DescriptorValidationException {
    checkNotNull(file);
    String[] insertedGenericDescriptors = new String[file.tables.numDescriptorsForPool];
    FieldDescriptor[] insertedExtensions = new FieldDescriptor[file.tables.numExtensionsForPool];
    writeLock.lock();
    try {
      String insertedFileDescriptor = null;
      int insertedGenericDescriptorsSize = 0;
      int insertedExtensionsSize = 0;
      try {
        String key = file.getFullName();
        FileDescriptor oldFile = filesByName.putIfAbsent(file);
        if (oldFile != null && oldFile != file) {
          throw new DescriptorValidationException(
              file, "descriptor collision between " + oldFile + " and " + file);
        }
        if (oldFile == null) {
          insertedFileDescriptor = key;
        }
        for (GenericDescriptor descriptor : file.tables.descriptorsByName.values()) {
          if (descriptor instanceof FieldDescriptor) {
            FieldDescriptor fieldDescriptor = (FieldDescriptor) descriptor;
            if (!fieldDescriptor.isExtension()) {
              continue;
            }
            key = fieldDescriptor.getFullName();
          } else if (descriptor instanceof Descriptor
              || descriptor instanceof EnumDescriptor
              || descriptor instanceof EnumValueDescriptor
              || descriptor instanceof ServiceDescriptor) {
            key = descriptor.getFullName();
          } else {
            continue;
          }
          GenericDescriptor oldDescriptor = genericsByName.putIfAbsent(descriptor);
          if (oldDescriptor != null && oldDescriptor != descriptor) {
            throw new DescriptorValidationException(
                descriptor,
                "descriptor collision between "
                    + oldDescriptor
                    + " and "
                    + descriptor
                    + " for "
                    + key);
          }
          if (oldDescriptor == null) {
            try {
              insertedGenericDescriptors[insertedGenericDescriptorsSize] = key;
            } catch (Error | RuntimeException e) {
              genericsByName.remove(key);
              throw e;
            }
            ++insertedGenericDescriptorsSize;
          }
          if (descriptor instanceof FieldDescriptor) {
            FieldDescriptor fieldDescriptor = (FieldDescriptor) descriptor;
            if (fieldDescriptor.isExtension()) {
              FieldDescriptor oldExtension = extensionsByNumber.putIfAbsent(fieldDescriptor);
              if (oldExtension != null && oldExtension != fieldDescriptor) {
                throw new DescriptorValidationException(
                    descriptor,
                    "descriptor collision between " + oldExtension + " and " + fieldDescriptor);
              }
              if (oldExtension == null) {
                try {
                  insertedExtensions[insertedExtensionsSize] = fieldDescriptor;
                } catch (Error | RuntimeException e) {
                  extensionsByNumber.remove(fieldDescriptor);
                  throw e;
                }
                ++insertedExtensionsSize;
              }
            }
          }
        }
      } catch (DescriptorValidationException | Error | RuntimeException e) {
        for (; insertedExtensionsSize > 0; --insertedExtensionsSize) {
          extensionsByNumber.remove(insertedExtensions[insertedExtensionsSize - 1]);
        }
        for (; insertedGenericDescriptorsSize > 0; --insertedGenericDescriptorsSize) {
          genericsByName.remove(insertedGenericDescriptors[insertedGenericDescriptorsSize - 1]);
        }
        if (insertedFileDescriptor != null) {
          filesByName.remove(insertedFileDescriptor);
        }
        throw e;
      }
    } finally {
      writeLock.unlock();
    }
  }
}
