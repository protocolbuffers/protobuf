// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/internal/message.h"

#include <stddef.h>

#include "upb/message/internal/types.h"

// Must be last.
#include "upb/port/def.inc"

// A MiniTable for an empty message, used for unlinked sub-messages that are
// built via MiniDescriptors.  Messages that use this MiniTable may possibly
// be linked later, in which case this MiniTable will be replaced with a real
// one.  This pattern is known as "dynamic tree shaking", and it introduces
// complication because sub-messages may either be the "empty" type or the
// "real" type.  A tagged bit indicates the difference.
const struct upb_MiniTable UPB_PRIVATE(_kUpb_MiniTable_Empty) = {
    .UPB_PRIVATE(subs) = NULL,
    .UPB_PRIVATE(fields) = NULL,
    .UPB_PRIVATE(size) = sizeof(struct upb_Message),
    .UPB_PRIVATE(field_count) = 0,
    .UPB_PRIVATE(ext) = kUpb_ExtMode_NonExtendable,
    .UPB_PRIVATE(dense_below) = 0,
    .UPB_PRIVATE(table_mask) = -1,
    .UPB_PRIVATE(required_count) = 0,
};

// A MiniTable for a statically tree shaken message.  Messages that use this
// MiniTable are guaranteed to remain unlinked; unlike the empty message, this
// MiniTable is never replaced, which greatly simplifies everything, because the
// type of a sub-message is always known, without consulting a tagged bit.
const struct upb_MiniTable UPB_PRIVATE(_kUpb_MiniTable_StaticallyTreeShaken) = {
    .UPB_PRIVATE(subs) = NULL,
    .UPB_PRIVATE(fields) = NULL,
    .UPB_PRIVATE(size) = sizeof(struct upb_Message),
    .UPB_PRIVATE(field_count) = 0,
    .UPB_PRIVATE(ext) = kUpb_ExtMode_NonExtendable,
    .UPB_PRIVATE(dense_below) = 0,
    .UPB_PRIVATE(table_mask) = -1,
    .UPB_PRIVATE(required_count) = 0,
};
