#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Reflection;
using NUnit.Framework;
using System;
using static Google.Protobuf.Reflection.FeatureSet.Types;

namespace Google.Protobuf.Test.Reflection;

public class FeatureSetDescriptorTest
{
    // Just selectively test a couple of hard-coded examples. This isn't meant to be exhaustive,
    // and we don't expect to add new tests for later editions unless there's a production code change.

    [Test]
    public void Proto2Defaults()
    {
        var expectedDefaults = new FeatureSet
        {
            EnumType = EnumType.Closed,
            FieldPresence = FieldPresence.Explicit,
            JsonFormat = JsonFormat.LegacyBestEffort,
            MessageEncoding = MessageEncoding.LengthPrefixed,
            RepeatedFieldEncoding = RepeatedFieldEncoding.Expanded,
            Utf8Validation = Utf8Validation.None,
        };
        var actualDefaults = FeatureSetDescriptor.GetEditionDefaults(Edition.Proto2).Proto;
        Assert.AreEqual(expectedDefaults, actualDefaults);
    }

    [Test]
    public void Proto3Defaults()
    {
        var expectedDefaults = new FeatureSet
        {
            EnumType = EnumType.Open,
            FieldPresence = FieldPresence.Implicit,
            JsonFormat = JsonFormat.Allow,
            MessageEncoding = MessageEncoding.LengthPrefixed,
            RepeatedFieldEncoding = RepeatedFieldEncoding.Packed,
            Utf8Validation = Utf8Validation.Verify,
        };
        var actualDefaults = FeatureSetDescriptor.GetEditionDefaults(Edition.Proto3).Proto;
        Assert.AreEqual(expectedDefaults, actualDefaults);
    }

    [Test]
    public void MaxSupportedEdition()
    {
        // This should be the last piece of code to be changed when updating the C# runtime to support
        // a new edition. It should only be changed when you're sure that all the features in the new
        // edition are supported. Just changing the configuration for feature set default generation
        // will *advertise* that we support the new edition, but that isn't sufficient.
        Edition maxSupportedEdition = Edition._2023;

        // These lines should not need to be changed.
        FeatureSetDescriptor.GetEditionDefaults(maxSupportedEdition);
        Edition invalidEdition = (Edition) (maxSupportedEdition + 1);
        Assert.Throws<ArgumentOutOfRangeException>(() => FeatureSetDescriptor.GetEditionDefaults(invalidEdition));
    }
}
