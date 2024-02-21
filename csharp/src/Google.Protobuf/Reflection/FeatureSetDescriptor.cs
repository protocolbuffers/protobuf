#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections.Concurrent;
using static Google.Protobuf.Reflection.FeatureSet.Types;

namespace Google.Protobuf.Reflection;

// FIXME: Naming - is it okay to call this a "descriptor"? It doesn't describe a proto element in
// the way that other descriptors do, but it's an immutable wrapper around a proto message with some
// useful behavior.

/// <summary>
/// A resolved set of features for a file, message etc.
/// </summary>
public sealed class FeatureSetDescriptor
{
    private static readonly ConcurrentDictionary<FeatureSet, FeatureSetDescriptor> cache = new();

    // Note: this isn't the way Java has implemented defaults, but it's easier to think about for now.
    // (It also doesn't require parsing, which means it doesn't need any descriptors - removing
    // a bootstrap issue.)
    // If it proves insufficient, we can implement the more complex approach.
    private static readonly FeatureSetDescriptor edition2023Defaults = new FeatureSetDescriptor(
        new FeatureSet
        {
            EnumType = EnumType.Open,
            FieldPresence = FieldPresence.Explicit,
            JsonFormat = JsonFormat.Allow,
            MessageEncoding = MessageEncoding.LengthPrefixed,
            RepeatedFieldEncoding = RepeatedFieldEncoding.Packed,
            Utf8Validation = Utf8Validation.Verify,
        });
    private static readonly FeatureSetDescriptor proto2Defaults = new FeatureSetDescriptor(
        new FeatureSet
        {
            EnumType = EnumType.Closed,
            FieldPresence = FieldPresence.Explicit,
            JsonFormat = JsonFormat.LegacyBestEffort,
            MessageEncoding = MessageEncoding.LengthPrefixed,
            RepeatedFieldEncoding = RepeatedFieldEncoding.Expanded,
            Utf8Validation = Utf8Validation.None,
        });
    private static readonly FeatureSetDescriptor proto3Defaults = new FeatureSetDescriptor(
        new FeatureSet
        {
            EnumType = EnumType.Open,
            FieldPresence = FieldPresence.Implicit,
            JsonFormat = JsonFormat.Allow,
            MessageEncoding = MessageEncoding.LengthPrefixed,
            RepeatedFieldEncoding = RepeatedFieldEncoding.Packed,
            Utf8Validation = Utf8Validation.Verify,
        });
    // FIXME: Get the defaults for 2024.
    //private static readonly FeatureSetDescriptor edition2024Defaults = new FeatureSetDescriptor(new FeatureSet());

    private readonly FeatureSet proto;

    internal static FeatureSetDescriptor GetEditionDefaults(Edition edition) =>
        edition switch
        {
            Edition.Proto2 => proto2Defaults,
            Edition.Proto3 => proto3Defaults,
            Edition._2023 => edition2023Defaults,
            //Edition._2024 => edition2024Defaults,
            _ => throw new ArgumentOutOfRangeException($"Unsupported edition: {edition}")
        };

    /// <summary>
    /// Only relevant to fields. Indicates if a field has explicit presence.
    /// </summary>
    public FieldPresence FieldPresence => proto.FieldPresence;

    /// <summary>
    /// Only relevant to repeated fields. Indicates if and how a field is repeated.
    /// </summary>
    public RepeatedFieldEncoding RepeatedFieldEncoding => proto.RepeatedFieldEncoding;

    /// <summary>
    /// Only relevant to fields. Indicates how a message-valued field should be encoded.
    /// </summary>
    public MessageEncoding MessageEncoding => proto.MessageEncoding;

    private FeatureSetDescriptor(FeatureSet proto)
    {
        this.proto = proto;
    }

    // FIXME: Do we need this? It's the same convention as other descriptors...
    /// <summary>
    /// Returns a clone of the underlying proto.
    /// </summary>
    public FeatureSet ToProto() => proto.Clone();

    // FIXME: Check naming. "MergeFrom" suggests a mutation of the existing FeatureSetDescriptor;
    // "MergedWith" is intended to make it clear that it's returning a (potentially) different
    // object.

    /// <summary>
    /// Returns a new descriptor based on this one, with the specified overrides.
    /// Multiple calls to this method that produce equivalent feature sets will return
    /// the same instance.
    /// </summary>
    /// <param name="overrides">The proto representation of the "child" feature set to merge with this
    /// one. May be null, in which case this descriptor is returned.</param>
    /// <returns>A descriptor based on the current one, with the given set of overrides.</returns>
    public FeatureSetDescriptor MergedWith(FeatureSet overrides)
    {
        if (overrides is null)
        {
            return this;
        }

        // Note: It would be nice if we could avoid cloning unless
        // there are actual changes, but this won't happen that often;
        // it'll be temporary garbage.
        var clone = proto.Clone();
        clone.MergeFrom(overrides);
        return cache.GetOrAdd(clone, clone => new FeatureSetDescriptor(clone));
    }
}
