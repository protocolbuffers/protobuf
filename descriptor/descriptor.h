/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * This file contains declarations for an array that contains the contents
 * of descriptor.proto, serialized as a protobuf.  xxd is used to create
 * the actual definition.
 */

#ifndef UPB_DESCRIPTOR_H_
#define UPB_DESCRIPTOR_H_

#include "upb_string.h"

#ifdef __cplusplus
extern "C" {
#endif

extern upb_string descriptor_str;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DESCRIPTOR_H_ */
