/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Handling of errno.
 */

#include "upb/upb.h"

#ifndef UPB_STDC_ERROR_H_
#define UPB_STDC_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

extern upb_errorspace upb_stdc_errorspace;
void upb_status_fromerrno(upb_status *status, int code);
bool upb_errno_is_wouldblock(int code);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_STDC_ERROR_H_ */
