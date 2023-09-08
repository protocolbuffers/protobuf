/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Implementations of the AccessorRegistry type in our public
 * interface.
 */

import * as decl from '../upb';

import {Arena} from './arena';
import {Buffer} from './buffer';
import {EnumAccessor} from './enum_accessor';
import {ExtensionAccessor} from './extension_accessor';
import {MessageAccessorNoBits} from './message_accessor_nobits';
import {MessageAccessorWithBits} from './message_accessor_withbits';

/**
 * @define A flag for selecting whether to use the with-bits or no-bits impl.
 */
const USE_JS_BITS_FLAG = goog.define('upb.js.USE_JS_BITS', true);

/**
 * An AccessorRegistry is a factory and cache for Accessor objects.
 */
export class AccessorRegistry implements decl.AccessorRegistry {
  buffer: Buffer;
  messageAccessorCtor:
      new(registry: AccessorRegistry, miniDescriptor: string,
          arena: Arena) => decl.MessageAccessor;

  constructor(readonly instance: decl.UpbInstance, readonly arena: Arena) {
    this.buffer = new Buffer(this.arena);

    if (USE_JS_BITS_FLAG) {
      this.messageAccessorCtor = MessageAccessorWithBits;
    } else {
      this.messageAccessorCtor = MessageAccessorNoBits;
    }
  }

  newEnumAccessor(miniDescriptor: string): EnumAccessor {
    return new EnumAccessor(miniDescriptor, this.arena);
  }
  newExtensionAccessor(miniDescriptor: string): ExtensionAccessor {
    return new ExtensionAccessor(miniDescriptor, this.arena);
  }
  newMessageAccessor(miniDescriptor: string): decl.MessageAccessor {
    return new this.messageAccessorCtor(this, miniDescriptor, this.arena);
  }
}
