// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBUnknownFields.h"

@interface GPBUnknownFields ()

- (nonnull NSData *)serializeAsData;
- (size_t)computeSerializedSize __attribute__((objc_direct));
- (void)writeToCodedOutputStream:(nonnull GPBCodedOutputStream *)output
    __attribute__((objc_direct));

@end
