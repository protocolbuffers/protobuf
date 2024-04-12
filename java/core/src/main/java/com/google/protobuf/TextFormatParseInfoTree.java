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

import com.google.protobuf.Descriptors.FieldDescriptor;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Data structure which is populated with the locations of each field value parsed from the text.
 *
 * <p>The locations of primary fields values are retrieved by {@code getLocation} or {@code
 * getLocations}. The locations of sub message values are within nested {@code
 * TextFormatParseInfoTree}s and are retrieve by {@code getNestedTree} or {@code getNestedTrees}.
 *
 * <p>The {@code TextFormatParseInfoTree} is created by a Builder.
 */
public class TextFormatParseInfoTree {

  // Defines a mapping between each field's descriptor to the list of locations where
  // its value(s) were was encountered.
  private Map<FieldDescriptor, List<TextFormatParseLocation>> locationsFromField;

  // Defines a mapping between a field's descriptor to a list of TextFormatParseInfoTrees for
  // sub message location information.
  Map<FieldDescriptor, List<TextFormatParseInfoTree>> subtreesFromField;

  /**
   * Construct a {@code TextFormatParseInfoTree}.
   *
   * @param locationsFromField a map of fields to location in the source code
   * @param subtreeBuildersFromField a map of fields to parse tree location information builders
   */
  private TextFormatParseInfoTree(
      Map<FieldDescriptor, List<TextFormatParseLocation>> locationsFromField,
      Map<FieldDescriptor, List<TextFormatParseInfoTree.Builder>> subtreeBuildersFromField) {

    // The maps are unmodifiable.  The values in the maps are unmodifiable.
    Map<FieldDescriptor, List<TextFormatParseLocation>> locs =
        new HashMap<FieldDescriptor, List<TextFormatParseLocation>>();
    for (Entry<FieldDescriptor, List<TextFormatParseLocation>> kv : locationsFromField.entrySet()) {
      locs.put(kv.getKey(), Collections.unmodifiableList(kv.getValue()));
    }
    this.locationsFromField = Collections.unmodifiableMap(locs);

    Map<FieldDescriptor, List<TextFormatParseInfoTree>> subs =
        new HashMap<FieldDescriptor, List<TextFormatParseInfoTree>>();
    for (Entry<FieldDescriptor, List<Builder>> kv : subtreeBuildersFromField.entrySet()) {
      List<TextFormatParseInfoTree> submessagesOfField = new ArrayList<TextFormatParseInfoTree>();
      for (Builder subBuilder : kv.getValue()) {
        submessagesOfField.add(subBuilder.build());
      }
      subs.put(kv.getKey(), Collections.unmodifiableList(submessagesOfField));
    }
    this.subtreesFromField = Collections.unmodifiableMap(subs);
  }

  /**
   * Retrieve all the locations of a field.
   *
   * @param fieldDescriptor the {@link FieldDescriptor} of the desired field
   * @return a list of the locations of values of the field. If there are not values or the field
   *     doesn't exist, an empty list is returned.
   */
  public List<TextFormatParseLocation> getLocations(final FieldDescriptor fieldDescriptor) {
    List<TextFormatParseLocation> result = locationsFromField.get(fieldDescriptor);
    return (result == null) ? Collections.<TextFormatParseLocation>emptyList() : result;
  }

  /**
   * Get the location in the source of a field's value.
   *
   * <p>Returns the {@link TextFormatParseLocation} for index-th value of the field in the parsed
   * text.
   *
   * @param fieldDescriptor the {@link FieldDescriptor} of the desired field
   * @param index the index of the value.
   * @return the {@link TextFormatParseLocation} of the value
   * @throws IllegalArgumentException index is out of range
   */
  public TextFormatParseLocation getLocation(final FieldDescriptor fieldDescriptor, int index) {
    return getFromList(getLocations(fieldDescriptor), index, fieldDescriptor);
  }

  /**
   * Retrieve a list of all the location information trees for a sub message field.
   *
   * @param fieldDescriptor the {@link FieldDescriptor} of the desired field
   * @return A list of {@link TextFormatParseInfoTree}
   */
  public List<TextFormatParseInfoTree> getNestedTrees(final FieldDescriptor fieldDescriptor) {
    List<TextFormatParseInfoTree> result = subtreesFromField.get(fieldDescriptor);
    return result == null ? Collections.<TextFormatParseInfoTree>emptyList() : result;
  }

  /**
   * Returns the parse info tree for the given field, which must be a message type.
   *
   * @param fieldDescriptor the {@link FieldDescriptor} of the desired sub message
   * @param index the index of message value.
   * @return the {@code ParseInfoTree} of the message value. {@code null} is returned if the field
   *     doesn't exist or the index is out of range.
   * @throws IllegalArgumentException if index is out of range
   */
  public TextFormatParseInfoTree getNestedTree(final FieldDescriptor fieldDescriptor, int index) {
    return getFromList(getNestedTrees(fieldDescriptor), index, fieldDescriptor);
  }

  /**
   * Create a builder for a {@code ParseInfoTree}.
   *
   * @return the builder
   */
  public static Builder builder() {
    return new Builder();
  }

  private static <T> T getFromList(List<T> list, int index, FieldDescriptor fieldDescriptor) {
    if (index >= list.size() || index < 0) {
      throw new IllegalArgumentException(
          String.format(
              "Illegal index field: %s, index %d",
              fieldDescriptor == null ? "<null>" : fieldDescriptor.getName(), index));
    }
    return list.get(index);
  }

  /** Builder for a {@link TextFormatParseInfoTree}. */
  public static class Builder {

    private Map<FieldDescriptor, List<TextFormatParseLocation>> locationsFromField;

    // Defines a mapping between a field's descriptor to a list of ParseInfoTrees builders for
    // sub message location information.
    private Map<FieldDescriptor, List<Builder>> subtreeBuildersFromField;

    /** Create a root level {@ParseInfoTree} builder. */
    private Builder() {
      locationsFromField = new HashMap<FieldDescriptor, List<TextFormatParseLocation>>();
      subtreeBuildersFromField = new HashMap<FieldDescriptor, List<Builder>>();
    }

    /**
     * Record the starting location of a single value for a field.
     *
     * @param fieldDescriptor the field
     * @param location source code location information
     */
    public Builder setLocation(
        final FieldDescriptor fieldDescriptor, TextFormatParseLocation location) {
      List<TextFormatParseLocation> fieldLocations = locationsFromField.get(fieldDescriptor);
      if (fieldLocations == null) {
        fieldLocations = new ArrayList<TextFormatParseLocation>();
        locationsFromField.put(fieldDescriptor, fieldLocations);
      }
      fieldLocations.add(location);
      return this;
    }

    /**
     * Set for a sub message.
     *
     * <p>A new builder is created for a sub message. The builder that is returned is a new builder.
     * The return is <em>not</em> the invoked {@code builder.getBuilderForSubMessageField}.
     *
     * @param fieldDescriptor the field whose value is the submessage
     * @return a new Builder for the sub message
     */
    public Builder getBuilderForSubMessageField(final FieldDescriptor fieldDescriptor) {
      List<Builder> submessageBuilders = subtreeBuildersFromField.get(fieldDescriptor);
      if (submessageBuilders == null) {
        submessageBuilders = new ArrayList<Builder>();
        subtreeBuildersFromField.put(fieldDescriptor, submessageBuilders);
      }
      Builder subtreeBuilder = new Builder();
      submessageBuilders.add(subtreeBuilder);
      return subtreeBuilder;
    }

    /**
     * Build the {@code TextFormatParseInfoTree}.
     *
     * @return the {@code TextFormatParseInfoTree}
     */
    public TextFormatParseInfoTree build() {
      return new TextFormatParseInfoTree(locationsFromField, subtreeBuildersFromField);
    }
  }
}
