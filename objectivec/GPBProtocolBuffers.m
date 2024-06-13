// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// If you want to build protocol buffers in your own project without adding the
// project dependency, you can just add this file.

// This warning seems to treat code differently when it is #imported than when
// it is inline in the file.  GPBDictionary.m compiles cleanly in other targets,
// but when #imported here it triggers a bunch of warnings that don't make
// much sense, and don't trigger when compiled directly.  So we shut off the
// warnings here.
#pragma clang diagnostic ignored "-Wnullability-completeness"

#import "GPBArray.m"
#import "GPBCodedInputStream.m"
#import "GPBCodedOutputStream.m"
#import "GPBDescriptor.m"
#import "GPBDictionary.m"
#import "GPBExtensionInternals.m"
#import "GPBExtensionRegistry.m"
#import "GPBMessage.m"
#import "GPBRootObject.m"
#import "GPBUnknownField.m"
#import "GPBUnknownFieldSet.m"
#import "GPBUtilities.m"
#import "GPBWellKnownTypes.m"
#import "GPBWireFormat.m"

#import "GPBAny.pbobjc.m"
#import "GPBApi.pbobjc.m"
#import "GPBDuration.pbobjc.m"
#import "GPBEmpty.pbobjc.m"
#import "GPBFieldMask.pbobjc.m"
#import "GPBSourceContext.pbobjc.m"
#import "GPBStruct.pbobjc.m"
#import "GPBTimestamp.pbobjc.m"
#import "GPBType.pbobjc.m"
#import "GPBWrappers.pbobjc.m"
