#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Makes sure all the generated headers compile with ARC on.

#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestCycle.pbobjc.h"
#import "objectivec/Tests/UnittestDeprecated.pbobjc.h"
#import "objectivec/Tests/UnittestDeprecatedFile.pbobjc.h"
#import "objectivec/Tests/UnittestImport.pbobjc.h"
#import "objectivec/Tests/UnittestImportPublic.pbobjc.h"
#import "objectivec/Tests/UnittestMset.pbobjc.h"
#import "objectivec/Tests/UnittestObjc.pbobjc.h"
#import "objectivec/Tests/UnittestObjcOptions.pbobjc.h"
#import "objectivec/Tests/UnittestObjcStartup.pbobjc.h"
#import "objectivec/Tests/UnittestPreserveUnknownEnum.pbobjc.h"
#import "objectivec/Tests/UnittestRuntimeProto2.pbobjc.h"
#import "objectivec/Tests/UnittestRuntimeProto3.pbobjc.h"
