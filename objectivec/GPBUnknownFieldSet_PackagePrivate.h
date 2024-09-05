// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBCodedInputStream.h"
#import "GPBCodedOutputStream.h"
#import "GPBUnknownFieldSet.h"

@interface GPBUnknownFieldSet ()

- (NSData *)data;

- (size_t)serializedSize;
- (size_t)serializedSizeAsMessageSet;

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)output;
- (void)writeAsMessageSetTo:(GPBCodedOutputStream *)output;

- (void)mergeUnknownFields:(GPBUnknownFieldSet *)other;

- (void)mergeFromCodedInputStream:(GPBCodedInputStream *)input;

- (void)mergeVarintField:(int32_t)number value:(int32_t)value;
- (void)mergeLengthDelimited:(int32_t)number value:(NSData *)value;
- (BOOL)mergeFieldFrom:(int32_t)tag input:(GPBCodedInputStream *)input;

@end
