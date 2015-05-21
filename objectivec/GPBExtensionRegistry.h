// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import <Foundation/Foundation.h>

@class GPBDescriptor;
@class GPBExtensionField;

// A table of known extensions, searchable by name or field number.  When
// parsing a protocol message that might have extensions, you must provide an
// ExtensionRegistry in which you have registered any extensions that you want
// to be able to parse.  Otherwise, those extensions will just be treated like
// unknown fields.
//
// The *Root classes provide +extensionRegistry for the extensions defined in a
// given file *and* all files it imports.  You can also create a
// GPBExtensionRegistry, and merge those registries to handle parsing extensions
// defined from non overlapping files.
//
//    GPBExtensionRegistry *registry =
//        [[[MyProtoFileRoot extensionRegistry] copy] autorelease];
//    [registry addExtension:[OtherMessage neededExtension];  // Not in MyProtoFile
//    NSError *parseError = nil;
//    MyMessage *msg = [MyMessage parseData:data
//                        extensionRegistry:registry
//                                    error:&parseError];
//
@interface GPBExtensionRegistry : NSObject<NSCopying>

- (void)addExtension:(GPBExtensionField *)extension;
- (void)addExtensions:(GPBExtensionRegistry *)registry;

- (GPBExtensionField *)getExtension:(GPBDescriptor *)containingType
                        fieldNumber:(NSInteger)fieldNumber;

@end
