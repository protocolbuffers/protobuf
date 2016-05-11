// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

package com.google.protobuf;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Equivalent to {@link ExtensionRegistry} but supports only "lite" types.
 * <p>
 * If all of your types are lite types, then you only need to use
 * {@code ExtensionRegistryLite}.  Similarly, if all your types are regular
 * types, then you only need {@link ExtensionRegistry}.  Typically it does not
 * make sense to mix the two, since if you have any regular types in your
 * program, you then require the full runtime and lose all the benefits of
 * the lite runtime, so you might as well make all your types be regular types.
 * However, in some cases (e.g. when depending on multiple third-party libraries
 * where one uses lite types and one uses regular), you may find yourself
 * wanting to mix the two.  In this case things get more complicated.
 * <p>
 * There are three factors to consider:  Whether the type being extended is
 * lite, whether the embedded type (in the case of a message-typed extension)
 * is lite, and whether the extension itself is lite.  Since all three are
 * declared in different files, they could all be different.  Here are all
 * the combinations and which type of registry to use:
 * <pre>
 *   Extended type     Inner type    Extension         Use registry
 *   =======================================================================
 *   lite              lite          lite              ExtensionRegistryLite
 *   lite              regular       lite              ExtensionRegistry
 *   regular           regular       regular           ExtensionRegistry
 *   all other combinations                            not supported
 * </pre>
 * <p>
 * Note that just as regular types are not allowed to contain lite-type fields,
 * they are also not allowed to contain lite-type extensions.  This is because
 * regular types must be fully accessible via reflection, which in turn means
 * that all the inner messages must also support reflection.  On the other hand,
 * since regular types implement the entire lite interface, there is no problem
 * with embedding regular types inside lite types.
 *
 * @author kenton@google.com Kenton Varda
 */
public class ExtensionRegistryLite {

  // Set true to enable lazy parsing feature for MessageSet.
  //
  // TODO(xiangl): Now we use a global flag to control whether enable lazy
  // parsing feature for MessageSet, which may be too crude for some
  // applications. Need to support this feature on smaller granularity.
  private static volatile boolean eagerlyParseMessageSets = false;

  public static boolean isEagerlyParseMessageSets() {
    return eagerlyParseMessageSets;
  }

  public static void setEagerlyParseMessageSets(boolean isEagerlyParse) {
    eagerlyParseMessageSets = isEagerlyParse;
  }

  /** Construct a new, empty instance. */
  public static ExtensionRegistryLite newInstance() {
    return new ExtensionRegistryLite();
  }

  /** Get the unmodifiable singleton empty instance. */
  public static ExtensionRegistryLite getEmptyRegistry() {
    return EMPTY;
  }

  /** Returns an unmodifiable view of the registry. */
  public ExtensionRegistryLite getUnmodifiable() {
    return new ExtensionRegistryLite(this);
  }

  /**
   * Find an extension by containing type and field number.
   *
   * @return Information about the extension if found, or {@code null}
   *         otherwise.
   */
  @SuppressWarnings("unchecked")
  public <ContainingType extends MessageLite>
      GeneratedMessageLite.GeneratedExtension<ContainingType, ?>
        findLiteExtensionByNumber(
          final ContainingType containingTypeDefaultInstance,
          final int fieldNumber) {
    return (GeneratedMessageLite.GeneratedExtension<ContainingType, ?>)
      extensionsByNumber.get(
        new ObjectIntPair(containingTypeDefaultInstance, fieldNumber));
  }

  /** Add an extension from a lite generated file to the registry. */
  public final void add(
      final GeneratedMessageLite.GeneratedExtension<?, ?> extension) {
    extensionsByNumber.put(
      new ObjectIntPair(extension.getContainingTypeDefaultInstance(),
                        extension.getNumber()),
      extension);
  }

  // =================================================================
  // Private stuff.

  // Constructors are package-private so that ExtensionRegistry can subclass
  // this.

  ExtensionRegistryLite() {
    this.extensionsByNumber =
        new HashMap<ObjectIntPair,
                    GeneratedMessageLite.GeneratedExtension<?, ?>>();
  }

  ExtensionRegistryLite(ExtensionRegistryLite other) {
    if (other == EMPTY) {
      this.extensionsByNumber = Collections.emptyMap();
    } else {
      this.extensionsByNumber =
        Collections.unmodifiableMap(other.extensionsByNumber);
    }
  }

  private final Map<ObjectIntPair,
                    GeneratedMessageLite.GeneratedExtension<?, ?>>
      extensionsByNumber;

  private ExtensionRegistryLite(boolean empty) {
    this.extensionsByNumber = Collections.emptyMap();
  }
  private static final ExtensionRegistryLite EMPTY =
    new ExtensionRegistryLite(true);

  /** A (Object, int) pair, used as a map key. */
  private static final class ObjectIntPair {
    private final Object object;
    private final int number;

    ObjectIntPair(final Object object, final int number) {
      this.object = object;
      this.number = number;
    }

    @Override
    public int hashCode() {
      return System.identityHashCode(object) * ((1 << 16) - 1) + number;
    }
    @Override
    public boolean equals(final Object obj) {
      if (!(obj instanceof ObjectIntPair)) {
        return false;
      }
      final ObjectIntPair other = (ObjectIntPair)obj;
      return object == other.object && number == other.number;
    }
  }
}
