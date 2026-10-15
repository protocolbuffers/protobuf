// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBDescriptor.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * A table of known extensions, searchable by name or field number.  When
 * parsing a protocol message that might have extensions, you must provide a
 * GPBExtensionRegistry in which you have registered any extensions that you
 * want to be able to parse. Otherwise, those extensions will just be treated
 * like unknown fields.
 **/
@protocol GPBExtensionRegistry <NSObject>

/**
 * Looks for the extension registered for the given field number on a given
 * GPBDescriptor.
 *
 * @param descriptor  The descriptor to look for a registered extension on.
 * @param fieldNumber The field number of the extension to look for.
 *
 * @return The registered GPBExtensionDescriptor or nil if none was found.
 **/
- (nullable GPBExtensionDescriptor *)extensionForDescriptor:(GPBDescriptor *)descriptor
                                                fieldNumber:(NSInteger)fieldNumber;
@end

/**
 * A concrete implementation of `GPBExtensionRegistry`.
 *
 * The *Root classes provide `+extensionRegistry` for the extensions defined
 * in a given file *and* all files it imports. You can also create a
 * GPBExtensionRegistry, and merge those registries to handle parsing
 * extensions defined from non overlapping files.
 *
 * ```
 * GPBExtensionRegistry *registry = [[MyProtoFileRoot extensionRegistry] copy];
 * [registry addExtension:[OtherMessage neededExtension]]; // Not in MyProtoFile
 * NSError *parseError;
 * MyMessage *msg = [MyMessage parseData:data extensionRegistry:registry error:&parseError];
 * ```
 **/
__attribute__((objc_subclassing_restricted))
@interface GPBExtensionRegistry : NSObject<NSCopying, GPBExtensionRegistry>

/**
 * Adds the given GPBExtensionDescriptor to this registry.
 *
 * @param extension The extension description to add.
 **/
- (void)addExtension:(GPBExtensionDescriptor *)extension;

/**
 * Adds all the extensions from another registry to this registry.
 *
 * `registry` can also be the registry returned by `+globalExtensionRegistry`
 * (or a registry it was added to). Its extensions are not copied: lookups that
 * do not find an extension added to this registry are forwarded to the global
 * registry, so adding it does not read any Mach-O section, and extensions from
 * images loaded later are found. Extensions added to this registry (with
 * `-addExtension:` or from other registries) take precedence over the global
 * registry's for the same extended class and field number. A copy of this
 * registry keeps forwarding to the global registry.
 *
 * ```
 * GPBExtensionRegistry *registry = [[GPBExtensionRegistry alloc] init];
 * [registry addExtensions:[GPBExtensionRegistry globalExtensionRegistry]];
 * [registry addExtension:MyExtension()];
 * ```
 *
 * @param registry The registry to merge into this registry. Must be a
 *                 `GPBExtensionRegistry` or the registry returned by
 *                 `+globalExtensionRegistry`; any other implementation of the
 *                 protocol raises an `NSInvalidArgumentException`. (Typed as
 *                 the protocol so the global registry can be passed without a
 *                 cast.)
 **/
- (void)addExtensions:(id<GPBExtensionRegistry>)registry;

/**
 * Returns the process-wide registry of the extensions that are linked into the
 * process and were generated with the experimental
 * `experimental_global_extension_registry` generator option (C function
 * extensions only).
 *
 * Each such extension records its description in the `__DATA,__gpb_ext_reg`
 * Mach-O section. Nothing is done when this method is called; the first
 * `-extensionForDescriptor:fieldNumber:` call reads the section of every loaded
 * image (and registers for images loaded later, such as dynamically loaded
 * frameworks), and indexes the entries by extended class and field number.
 * A `GPBExtensionDescriptor` is only created when it is looked up, and it is
 * the same instance that the generated extension function returns.
 *
 * The returned object is immutable and can be used from any thread. It is not
 * a `GPBExtensionRegistry` instance, but it can be passed to any API that takes
 * an `id<GPBExtensionRegistry>`. To add more extensions to it, pass it to
 * `-addExtensions:` of a new `GPBExtensionRegistry`.
 **/
+ (id<GPBExtensionRegistry>)globalExtensionRegistry;

@end

NS_ASSUME_NONNULL_END
