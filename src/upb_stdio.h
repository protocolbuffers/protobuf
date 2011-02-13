/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * This file provides upb_bytesrc and upb_bytesink implementations for
 * ANSI C stdio.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include <stdio.h>
#include "upb_stream.h"

#ifndef UPB_STDIO_H_
#define UPB_STDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

struct upb_stdio;
typedef struct upb_stdio upb_stdio;

// Creation/deletion.
upb_stdio *upb_stdio_new();
void upb_stdio_free(upb_stdio *stdio);

// Reset/initialize the object for use.  The src or sink will call
// fread()/fwrite()/etc. on the given FILE*.
void upb_stdio_reset(upb_stdio *stdio, FILE* file);

// Gets a bytesrc or bytesink for the given stdio.  The returned pointer is
// invalidated by upb_stdio_reset above.  It is perfectly valid to get both
// a bytesrc and a bytesink for the same stdio if the FILE* is open for reading
// and writing.
upb_bytesrc* upb_stdio_bytesrc(upb_stdio *stdio);
upb_bytesink* upb_stdio_bytesink(upb_stdio *stdio);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
