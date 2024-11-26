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
using System.Collections.Generic;
using System.Linq;
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
internal sealed partial class FeatureSetDescriptor
{
    private static readonly ConcurrentDictionary<FeatureSet, FeatureSetDescriptor> cache = new();

    private static readonly IReadOnlyDictionary<Edition, FeatureSetDescriptor> descriptorsByEdition = BuildEditionDefaults();

    // Note: if the debugger is set to break within this code, various type initializers will fail
    // as the debugger will try to call ToString() on messages, requiring descriptors to be accessed etc.
    // There's a possible workaround of using a hard-coded bootstrapping FeatureSetDescriptor to be returned
    // by GetEditionDefaults if descriptorsByEdition is null, but it's ugly and likely just pushes the problem
    // elsewhere. Normal debugging sessions (where the initial bootstrapping code doesn't hit any breakpoints)
    // do not cause any problems.
    private static IReadOnlyDictionary<Edition, FeatureSetDescriptor> BuildEditionDefaults()
    {
        var featureSetDefaults = FeatureSetDefaults.Parser.ParseFrom(Convert.FromBase64String(DefaultsBase64));
        var ret = new Dictionary<Edition, FeatureSetDescriptor>();

        // Note: Enum.GetValues<TEnum> isn't available until .NET 5. It's not worth making this conditional
        // based on that.
        var supportedEditions = ((Edition[]) Enum.GetValues(typeof(Edition)))
            .OrderBy(x => x)
            .Where(e => e >= featureSetDefaults.MinimumEdition && e <= featureSetDefaults.MaximumEdition);

        // We assume the embedded defaults will always contain "legacy".
        var currentDescriptor = MaybeCreateDescriptor(Edition.Legacy);
        foreach (var edition in supportedEditions)
        {
            currentDescriptor = MaybeCreateDescriptor(edition) ?? currentDescriptor;
            ret[edition] = currentDescriptor;
        }
        return ret;

        FeatureSetDescriptor MaybeCreateDescriptor(Edition edition)
        {
            var editionDefaults = featureSetDefaults.Defaults.SingleOrDefault(d => d.Edition == edition);
            if (editionDefaults is null)
            {
                return null;
            }
            var proto = new FeatureSet();
            proto.MergeFrom(editionDefaults.FixedFeatures);
            proto.MergeFrom(editionDefaults.OverridableFeatures);
            return new FeatureSetDescriptor(proto);
        }
    }

    internal static FeatureSetDescriptor GetEditionDefaults(Edition edition) =>
        descriptorsByEdition.TryGetValue(edition, out var defaults) ? defaults
        : throw new ArgumentOutOfRangeException($"Unsupported edition: {edition}");

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
