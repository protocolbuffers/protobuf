// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBUnknownField.h"

@class GPBCodedOutputStream;

@interface GPBUnknownField ()

- (void)writeToOutput:(GPBCodedOutputStream *)output;
- (size_t)serializedSize;

- (void)writeAsMessageSetExtensionToOutput:(GPBCodedOutputStream *)output;
- (size_t)serializedSizeAsMessageSetExtension;

- (void)mergeFromField:(GPBUnknownField *)other;

@end
