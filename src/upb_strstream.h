/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file contains upb_bytesrc and upb_bytesink implementations for
 * upb_string.
 */

#ifndef UPB_STRSTREAM_H
#define UPB_STRSTREAM_H

#include "upb_bytestream.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_stringsrc **************************************************************/

struct _upb_stringsrc {
  upb_bytesrc bytesrc;
  const char *str;
  size_t len;
};
typedef struct _upb_stringsrc upb_stringsrc;

// Create/free a stringsrc.
void upb_stringsrc_init(upb_stringsrc *s);
void upb_stringsrc_uninit(upb_stringsrc *s);

// Resets the stringsrc to a state where it will vend the given string.  The
// stringsrc will take a reference on the string, so the caller need not ensure
// that it outlives the stringsrc.  A stringsrc can be reset multiple times.
void upb_stringsrc_reset(upb_stringsrc *s, const char *str, size_t len);

// Returns the upb_bytesrc* for this stringsrc.
upb_bytesrc *upb_stringsrc_bytesrc(upb_stringsrc *s);


/* upb_stringsink *************************************************************/

struct _upb_stringsink {
  upb_bytesink bytesink;
  char *str;
  size_t len, size;
};
typedef struct _upb_stringsink upb_stringsink;

// Create/free a stringsrc.
void upb_stringsink_init(upb_stringsink *s);
void upb_stringsink_uninit(upb_stringsink *s);

// Resets the sink's string to "str", which the sink takes ownership of.
// "str" may be NULL, which will make the sink allocate a new string.
void upb_stringsink_reset(upb_stringsink *s, char *str, size_t size);

// Releases ownership of the returned string (which is "len" bytes long) and
// resets the internal string to be empty again (as if reset were called with
// NULL).
const char *upb_stringsink_release(upb_stringsink *s, size_t *len);

// Returns the upb_bytesink* for this stringsrc.  Invalidated by reset above.
upb_bytesink *upb_stringsink_bytesink();


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
