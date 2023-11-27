// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBUnknownFieldSet.h"

@class GPBCodedOutputStream;
@class GPBCodedInputStream;

@interface GPBUnknownFieldSet ()

+ (BOOL)isFieldTag:(int32_t)tag;

- (NSData *)data;

- (size_t)serializedSize;
- (size_t)serializedSizeAsMessageSet;

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)output;
- (void)writeAsMessageSetTo:(GPBCodedOutputStream *)output;

- (void)mergeUnknownFields:(GPBUnknownFieldSet *)other;

- (void)mergeFromCodedInputStream:(GPBCodedInputStream *)input;

- (void)mergeVarintField:(int32_t)number value:(int32_t)value;
- (BOOL)mergeFieldFrom:(int32_t)tag input:(GPBCodedInputStream *)input;
- (void)mergeMessageSetMessage:(int32_t)number data:(NSData *)messageData;

- (void)addUnknownMapEntry:(int32_t)fieldNum value:(NSData *)data;

@end
