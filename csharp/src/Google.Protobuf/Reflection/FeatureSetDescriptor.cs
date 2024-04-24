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

/// <summary>
/// A resolved set of features for a file, message etc.
/// </summary>
/// <remarks>
/// Only features supported by the C# runtime are exposed; currently
/// all enums in C# are open, and we never perform UTF-8 validation.
/// If either of those features are ever implemented in this runtime,
/// the feature settings will be exposed as properties in this class.
/// </remarks>
internal sealed class FeatureSetDescriptor
{
    private static readonly ConcurrentDictionary<FeatureSet, FeatureSetDescriptor> cache = new();

    // Note: this approach is deliberately chosen to circumvent bootstrapping issues.
    // This can still be tested using the binary representation.
    // TODO: Generate this code (as a partial class) from the binary representation.
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

    internal static FeatureSetDescriptor GetEditionDefaults(Edition edition) =>
        edition switch
        {
            Edition.Proto2 => proto2Defaults,
            Edition.Proto3 => proto3Defaults,
            Edition._2023 => edition2023Defaults,
            _ => throw new ArgumentOutOfRangeException($"Unsupported edition: {edition}")
        };

    // Visible for testing. The underlying feature set proto, usually derived during
    // feature resolution.
    internal FeatureSet Proto { get; }

    /// <summary>
    /// Only relevant to fields. Indicates if a field has explicit presence.
    /// </summary>
    internal FieldPresence FieldPresence => Proto.FieldPresence;

    /// <summary>
    /// Only relevant to fields. Indicates how a repeated field should be encoded.
    /// </summary>
    internal RepeatedFieldEncoding RepeatedFieldEncoding => Proto.RepeatedFieldEncoding;

    /// <summary>
    /// Only relevant to fields. Indicates how a message-valued field should be encoded.
    /// </summary>
    internal MessageEncoding MessageEncoding => Proto.MessageEncoding;

    private FeatureSetDescriptor(FeatureSet proto)
    {
        Proto = proto;
    }

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
        var clone = Proto.Clone();
        clone.MergeFrom(overrides);
        return cache.GetOrAdd(clone, clone => new FeatureSetDescriptor(clone));
    }
}
