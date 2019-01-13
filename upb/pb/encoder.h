/*
** upb::pb::Encoder (upb_pb_encoder)
**
** Implements a set of upb_handlers that write protobuf data to the binary wire
** format.
**
** This encoder implementation does not have any access to any out-of-band or
** precomputed lengths for submessages, so it must buffer submessages internally
** before it can emit the first byte.
*/

#ifndef UPB_ENCODER_H_
#define UPB_ENCODER_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace pb {
class EncoderPtr;
}  /* namespace pb */
}  /* namespace upb */
#endif

#define UPB_PBENCODER_MAX_NESTING 100

/* upb_pb_encoder *************************************************************/

/* Preallocation hint: decoder won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the decoder library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_PB_ENCODER_SIZE 768

struct upb_pb_encoder;
typedef struct upb_pb_encoder upb_pb_encoder;

UPB_BEGIN_EXTERN_C

upb_sink *upb_pb_encoder_input(upb_pb_encoder *p);
upb_pb_encoder* upb_pb_encoder_create(upb_env* e, const upb_handlers* h,
                                      upb_bytessink* output);

upb_handlercache *upb_pb_encoder_newcache();

UPB_END_EXTERN_C

#ifdef __cplusplus

class upb::pb::EncoderPtr {
 public:
  EncoderPtr(upb_pb_encoder* ptr) : ptr_(ptr) {}

  upb_pb_encoder* ptr() { return ptr_; }

  /* Creates a new encoder in the given environment.  The Handlers must have
   * come from NewHandlers() below. */
  static EncoderPtr Create(Environment* env, const Handlers* handlers,
                         BytesSink* output) {
    return EncoderPtr(upb_pb_encoder_create(env, handlers, output->ptr()));
  }

  /* The input to the encoder. */
  upb_sink* input() { return upb_pb_encoder_input(ptr()); }

  /* Creates a new set of handlers for this MessageDef. */
  static HandlerCache NewCache() {
    return HandlerCache(upb_pb_encoder_newcache());
  }

  static const size_t kSize = UPB_PB_ENCODER_SIZE;

 private:
  upb_pb_encoder* ptr_;
};

#endif  /* __cplusplus */

#endif  /* UPB_ENCODER_H_ */
