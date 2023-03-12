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

// This header is private to the ProtobolBuffers library and must NOT be
// included by any sources outside this library. The contents of this file are
// subject to change at any time without notice.

#import "GPBMessage.h"

// TODO: Remove this import. Older generated code use the OSAtomic* apis,
// so anyone that hasn't regenerated says building by having this. After
// enough time has passed, this likely can be removed as folks should have
// regenerated.
#import <libkern/OSAtomic.h>

#import "GPBBootstrap.h"

typedef struct GPBMessage_Storage {
  uint32_t _has_storage_[0];
} GPBMessage_Storage;

typedef struct GPBMessage_Storage *GPBMessage_StoragePtr;

@interface GPBMessage () {
 @package
  // NOTE: Because of the +allocWithZone code using NSAllocateObject(),
  // this structure should ideally always be kept pointer aligned where the
  // real storage starts is also pointer aligned. The compiler/runtime already
  // do this, but it may not be documented.

  // A pointer to the actual fields of the subclasses. The actual structure
  // pointed to by this pointer will depend on the subclass.
  // All of the actual structures will start the same as
  // GPBMessage_Storage with _has_storage__ as the first field.
  // Kept public because static functions need to access it.
  GPBMessage_StoragePtr messageStorage_;
}

// Gets an extension value without autocreating the result if not found. (i.e.
// returns nil if the extension is not set)
- (id)getExistingExtension:(GPBExtensionDescriptor *)extension;

// Parses a message of this type from the input and merges it with this
// message.
//
// Warning:  This does not verify that all required fields are present in
// the input message.
// Note:  The caller should call
// -[CodedInputStream checkLastTagWas:] after calling this to
// verify that the last tag seen was the appropriate end-group tag,
// or zero for EOF.
// NOTE: This will throw if there is an error while parsing.
- (void)mergeFromCodedInputStream:(GPBCodedInputStream *)input
                extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry;

- (void)addUnknownMapEntry:(int32_t)fieldNum value:(NSData *)data;

@end

CF_EXTERN_C_BEGIN

// Returns whether |message| autocreated this message. This is NO if the message
// was not autocreated by |message| or if it has been mutated since
// autocreation.
BOOL GPBWasMessageAutocreatedBy(GPBMessage *message, GPBMessage *parent);

// Call this when you mutate a message. It will cause the message to become
// visible to its autocreator.
void GPBBecomeVisibleToAutocreator(GPBMessage *self);

// Call this when an array/dictionary is mutated so the parent message that
// autocreated it can react.
void GPBAutocreatedArrayModified(GPBMessage *self, id array);
void GPBAutocreatedDictionaryModified(GPBMessage *self, id dictionary);

// Clear the autocreator, if any. Asserts if the autocreator still has an
// autocreated reference to this message.
void GPBClearMessageAutocreator(GPBMessage *self);

CF_EXTERN_C_END
