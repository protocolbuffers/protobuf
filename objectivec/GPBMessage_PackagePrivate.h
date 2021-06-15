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

#import "GPBUtilities_PackagePrivate.h"

// TODO: Remove this import. Older generated code use the OSAtomic* apis,
// so anyone that hasn't regenerated says building by having this. After
// enough time has passed, this likely can be removed as folks should have
// regenerated.
#import <libkern/OSAtomic.h>

#import "GPBBootstrap.h"

#if defined(__LP64__) && __LP64__
#define GPB_ASM_PTR " .quad "
#define GPB_ASM_PTRSIZE " 8 "
#define GPB_ASM_ONLY_LP64(x) x
#define GPB_ALIGN_SIZE " 3 "
#define GPB_DESCRIPTOR_METHOD_TYPE "@16@0:8"
#else  // __LP64__
#define GPB_ASM_PTR " .long "
#define GPB_ASM_PTRSIZE " 4 "
#define GPB_ASM_ONLY_LP64(x)
#define GPB_ALIGN_SIZE " 2 "
#define GPB_DESCRIPTOR_METHOD_TYPE "@8@0:4"
#endif  // __LP64__

#define GPB_MESSAGE_CLASS_NAME_RAW GPBMessage
#define GPB_MESSAGE_CLASS_NAME GPBStringifySymbol(GPB_MESSAGE_CLASS_NAME_RAW)

#if !__has_feature(objc_arc)
#define GPB_CLASS_FLAGS " 0x00 "
#define GPB_METACLASS_FLAGS " 0x01 "
#else
// 0x80 denotes that the class supports ARC.
#define GPB_CLASS_FLAGS " 0x80 "
#define GPB_METACLASS_FLAGS " 0x81 "
#endif

// This generates code equivalent to:
// ```
// @implementation _name_
// + (GPBDescriptor *)descriptor {
//      return _descriptorFunc_(self, _cmd);
// }
// @end
// ```
// We do this to avoid all of the @property metadata.
// If we use `@dynamic` the compiler generates a lot of property data including
// selectors and encoding strings. For large uses of protobufs, this can add
// up to several megabytes of unused Objective-C metadata.
// This inline class definition avoids the property data generation at the
// cost of being a little ugly. This has been tested with both 32 and 64 bits
// on intel and arm with Xcode 11.7 and Xcode 12.4.
// We make cstring_literals be local definitions by starting them with "L".
// https://ftp.gnu.org/old-gnu/Manuals/gas-2.9.1/html_chapter/as_5.html#SEC48
// This keeps our symbols tables smaller, and is what the linker expects.
// The linker in Xcode 12+ seems to be a bit more lenient about it, but the
// Xcode 11 linker requires it, and will give a cryptic "malformed method list"
// assertion if they are global.
#define GPB_MESSAGE_SUBCLASS_IMPL(name, descriptorFunc)                        \
  __asm__(                                                                     \
    ".section __TEXT, __objc_classname, cstring_literals \n"                   \
    "L_OBJC_CLASS_NAME_" GPBStringifySymbol(name) ": "                         \
    "    .asciz \"" GPBStringifySymbol(name) "\" \n"                           \
                                                                               \
    ".ifndef L_GBPDescriptorMethodName \n"                                     \
    ".section __TEXT, __objc_methname, cstring_literals \n"                    \
    "L_GBPDescriptorMethodName: .asciz \"descriptor\" \n"                      \
    ".endif \n"                                                                \
                                                                               \
    ".ifndef L_GPBDescriptorMethodType \n"                                     \
    ".section __TEXT, __objc_methtype, cstring_literals \n"                    \
    "L_GPBDescriptorMethodType: .asciz \"" GPB_DESCRIPTOR_METHOD_TYPE "\" \n"  \
    ".endif \n"                                                                \
                                                                               \
    ".section __DATA,__objc_const, regular \n"                                 \
    ".p2align" GPB_ALIGN_SIZE "\n"                                             \
    "__OBJC_$_CLASS_METHODS_" GPBStringifySymbol(name) ": \n"                  \
    ".long 3 *" GPB_ASM_PTRSIZE "\n"                                           \
    ".long 0x1 \n"                                                             \
    GPB_ASM_PTR "L_GBPDescriptorMethodName \n"                                 \
    GPB_ASM_PTR "L_GPBDescriptorMethodType \n"                                 \
    GPB_ASM_PTR "_" #descriptorFunc " \n"                                      \
                                                                               \
    ".section __DATA,__objc_const, regular \n"                                 \
    ".p2align" GPB_ALIGN_SIZE "\n"                                             \
    "__OBJC_METACLASS_RO_$_" GPBStringifySymbol(name) ": \n"                   \
    ".long" GPB_METACLASS_FLAGS "\n"                                           \
    ".long 5 *" GPB_ASM_PTRSIZE "\n"                                           \
    ".long 5 *" GPB_ASM_PTRSIZE "\n"                                           \
    GPB_ASM_ONLY_LP64(".space 4 \n")                                           \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "L_OBJC_CLASS_NAME_" GPBStringifySymbol(name) " \n"            \
    GPB_ASM_PTR "__OBJC_$_CLASS_METHODS_" GPBStringifySymbol(name) " \n"       \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "0 \n"                                                         \
                                                                               \
    ".section __DATA,__objc_const, regular \n"                                 \
    ".p2align" GPB_ALIGN_SIZE "\n"                                             \
    "__OBJC_CLASS_RO_$_" GPBStringifySymbol(name) ": \n"                       \
    ".long" GPB_CLASS_FLAGS "\n"                                               \
    ".long" GPB_ASM_PTRSIZE "\n"                                               \
    ".long" GPB_ASM_PTRSIZE "\n"                                               \
    GPB_ASM_ONLY_LP64(".long 0 \n")                                            \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "L_OBJC_CLASS_NAME_" GPBStringifySymbol(name) " \n"            \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "0 \n"                                                         \
                                                                               \
    ".globl _OBJC_METACLASS_$_" GPBStringifySymbol(name) "\n"                  \
    ".section __DATA,__objc_data, regular \n"                                  \
    ".p2align" GPB_ALIGN_SIZE "\n"                                             \
    "_OBJC_METACLASS_$_" GPBStringifySymbol(name) ": \n"                       \
    GPB_ASM_PTR "_OBJC_METACLASS_$_NSObject \n"                                \
    GPB_ASM_PTR "_OBJC_METACLASS_$_" GPB_MESSAGE_CLASS_NAME " \n"              \
    GPB_ASM_PTR "__objc_empty_cache \n"                                        \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "__OBJC_METACLASS_RO_$_" GPBStringifySymbol(name) " \n"        \
                                                                               \
    ".globl _OBJC_CLASS_$_" GPBStringifySymbol(name) "\n"                      \
    ".section __DATA,__objc_data, regular \n"                                  \
    ".p2align" GPB_ALIGN_SIZE "\n"                                             \
    "_OBJC_CLASS_$_" GPBStringifySymbol(name) ": \n"                           \
    GPB_ASM_PTR "_OBJC_METACLASS_$_" GPBStringifySymbol(name) " \n"            \
    GPB_ASM_PTR "_OBJC_CLASS_$_" GPB_MESSAGE_CLASS_NAME " \n"                  \
    GPB_ASM_PTR "__objc_empty_cache \n"                                        \
    GPB_ASM_PTR "0 \n"                                                         \
    GPB_ASM_PTR "__OBJC_CLASS_RO_$_" GPBStringifySymbol(name) " \n"            \
                                                                               \
    ".section __DATA, __objc_classlist, regular, no_dead_strip \n"             \
    ".p2align" GPB_ALIGN_SIZE "\n"                                             \
    "_OBJC_LABEL_CLASS_$" GPBStringifySymbol(name) ": \n"                      \
    GPB_ASM_PTR "_OBJC_CLASS_$_" GPBStringifySymbol(name) " \n"                \
  )

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
                extensionRegistry:(GPBExtensionRegistry *)extensionRegistry;

// Parses the next delimited message of this type from the input and merges it
// with this message.
- (void)mergeDelimitedFromCodedInputStream:(GPBCodedInputStream *)input
                         extensionRegistry:
                             (GPBExtensionRegistry *)extensionRegistry;

- (void)addUnknownMapEntry:(int32_t)fieldNum value:(NSData *)data;

@end

CF_EXTERN_C_BEGIN


// Call this before using the readOnlySemaphore_. This ensures it is created only once.
void GPBPrepareReadOnlySemaphore(GPBMessage *self);

// Returns a new instance that was automatically created by |autocreator| for
// its field |field|.
GPBMessage *GPBCreateMessageWithAutocreator(Class msgClass,
                                            GPBMessage *autocreator,
                                            GPBFieldDescriptor *field)
    __attribute__((ns_returns_retained));

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
