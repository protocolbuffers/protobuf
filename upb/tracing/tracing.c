// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#if (defined(UPB_TRACING_ENABLED) && !defined(NDEBUG))
#include "upb/tracing/tracing.h"

static void (*_upb_LogMessageNewHandler)(const upb_MiniTable*,
                                         const upb_Arena*);

void upb_tracing_Init(void (*logMessageNewHandler)(const upb_MiniTable*,
                                                   const upb_Arena*)) {
  _upb_LogMessageNewHandler = logMessageNewHandler;
}

void upb_tracing_LogMessageNew(const upb_MiniTable* mini_table,
                               const upb_Arena* arena) {
  if (_upb_LogMessageNewHandler) {
    _upb_LogMessageNewHandler(mini_table, arena);
  }
}

#endif
