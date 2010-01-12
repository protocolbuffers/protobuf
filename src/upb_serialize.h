/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Implements a upb_sink that writes protobuf data to the binary wire format.
 *
 * For messages that have any submessages, the serializer needs a buffer
 * containing the submessage sizes, so they can be properly written at the
 * front of each message.  Note that groups do *not* have this requirement.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_SERIALIZE_H_
#define UPB_SERIALIZE_H_

#include "upb.h"
#include "upb_sink.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t upb_get_value_size(union upb_value v, struct upb_fielddef *f);

struct upb_serializer;
typedef struct upb_serializer upb_serializer;

upb_serializer *upb_serializer_new();
void upb_serializer_free(upb_serializer *s);

void upb_serializer_reset(upb_serializer *s, uint32_t *sizes);
upb_sink *upb_serializer_sink(upb_serializer *s);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_SERIALIZE_H_ */
