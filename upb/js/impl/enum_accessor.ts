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
import {MiniTableEnum} from './mini_table';

export class EnumAccessor implements decl.EnumAccessor {
  miniTableEnum: MiniTableEnum;

  constructor(miniDescriptor: string, arena: Arena) {
    this.miniTableEnum = new MiniTableEnum(miniDescriptor, arena);
  }
}
