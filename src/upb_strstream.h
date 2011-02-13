/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * This file contains upb_bytesrc and upb_bytesink implementations for
 * upb_string.
 *
 * Copyright (c) 2009-2010 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_STRSTREAM_H
#define UPB_STRSTREAM_H

#include "upb_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_stringsrc **************************************************************/

struct _upb_stringsrc {
  upb_bytesrc bytesrc;
  upb_string *str;
  upb_strlen_t offset;
};
typedef struct _upb_stringsrc upb_stringsrc;

// Create/free a stringsrc.
void upb_stringsrc_init(upb_stringsrc *s);
void upb_stringsrc_uninit(upb_stringsrc *s);

// Resets the stringsrc to a state where it will vend the given string.  The
// stringsrc will take a reference on the string, so the caller need not ensure
// that it outlives the stringsrc.  A stringsrc can be reset multiple times.
void upb_stringsrc_reset(upb_stringsrc *s, upb_string *str);

// Returns the upb_bytesrc* for this stringsrc.  Invalidated by reset above.
upb_bytesrc *upb_stringsrc_bytesrc(upb_stringsrc *s);


/* upb_stringsink *************************************************************/

struct upb_stringsink;
typedef struct upb_stringsink upb_stringsink;

// Create/free a stringsrc.
upb_stringsink *upb_stringsink_new();
void upb_stringsink_free(upb_stringsink *s);

// Gets a string containing the data that has been written to this stringsink.
// The caller does *not* own any references to this string.
upb_string *upb_stringsink_getstring(upb_stringsink *s);

// Clears the internal string of accumulated data, resetting it to empty.
void upb_stringsink_reset(upb_stringsink *s);

// Returns the upb_bytesrc* for this stringsrc.  Invalidated by reset above.
upb_bytesink *upb_stringsrc_bytesink();


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
