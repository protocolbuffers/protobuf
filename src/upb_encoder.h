/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Implements a upb_sink that writes protobuf data to the binary wire format.
 *
 * For messages that have any submessages, the encoder needs a buffer
 * containing the submessage sizes, so they can be properly written at the
 * front of each message.  Note that groups do *not* have this requirement.
 *
 * Copyright (c) 2009-2010 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_ENCODER_H_
#define UPB_ENCODER_H_

#include "upb.h"
#include "upb_sink.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t upb_get_encoded_tag_size(uint32_t fieldnum);
size_t upb_get_encoded_value_size(union upb_value v, struct upb_fielddef *f);

struct upb_encoder;
typedef struct upb_encoder upb_encoder;

upb_encoder *upb_encoder_new();
void upb_encoder_free(upb_encoder *s);

void upb_encoder_reset(upb_encoder *s, uint32_t *sizes);
upb_sink *upb_encoder_sink(upb_encoder *s);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_ENCODER_H_ */
