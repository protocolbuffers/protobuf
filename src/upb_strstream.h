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

struct _upb_stringsink {
  upb_bytesink bytesink;
  upb_string *str;
};
typedef struct _upb_stringsink upb_stringsink;

// Create/free a stringsrc.
void upb_stringsink_init(upb_stringsink *s);
void upb_stringsink_uninit(upb_stringsink *s);

// Resets the stringsink to a state where it will append to the given string.
// The string must be newly created or recycled.  The stringsink will take a
// reference on the string, so the caller need not ensure that it outlives the
// stringsink.  A stringsink can be reset multiple times.
void upb_stringsink_reset(upb_stringsink *s, upb_string *str);

// Returns the upb_bytesink* for this stringsrc.  Invalidated by reset above.
upb_bytesink *upb_stringsink_bytesink();


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
