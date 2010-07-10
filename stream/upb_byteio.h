/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * This file contains upb_bytesrc and upb_bytesink implementations for common
 * interfaces like strings, UNIX fds, and FILE*.
 *
 * Copyright (c) 2009-2010 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_BYTEIO_H
#define UPB_BYTEIO_H

#include "upb_srcsink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_stringsrc **************************************************************/

struct upb_stringsrc;
typedef struct upb_stringsrc upb_stringsrc;

// Create/free a stringsrc.
upb_stringsrc *upb_stringsrc_new();
void upb_stringsrc_free(upb_stringsrc *s);

// Resets the stringsrc to a state where it will vend the given string.  The
// stringsrc will take a reference on the string, so the caller need not ensure
// that it outlives the stringsrc.  A stringsrc can be reset multiple times.
void upb_stringsrc_reset(upb_stringsrc *s, upb_string *str);

// Returns the upb_bytesrc* for this stringsrc.  Invalidated by reset above.
upb_bytesrc *upb_stringsrc_bytesrc();


/* upb_fdsrc ******************************************************************/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
