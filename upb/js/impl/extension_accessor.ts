/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

import * as decl from '../upb';

import {Arena} from './arena';
import {MiniTableExtension} from './mini_table';

export class ExtensionAccessor implements decl.ExtensionAccessor {
  miniTableExtension: MiniTableExtension;

  constructor(miniDescriptor: string, arena: Arena) {
    this.miniTableExtension = new MiniTableExtension(miniDescriptor, arena);
  }
}
