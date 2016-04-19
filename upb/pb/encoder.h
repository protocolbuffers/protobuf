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
class Encoder;
}  /* namespace pb */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::pb::Encoder, upb_pb_encoder)

#define UPB_PBENCODER_MAX_NESTING 100

/* upb::pb::Encoder ***********************************************************/

/* Preallocation hint: decoder won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the decoder library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_PB_ENCODER_SIZE 768

#ifdef __cplusplus

class upb::pb::Encoder {
 public:
  /* Creates a new encoder in the given environment.  The Handlers must have
   * come from NewHandlers() below. */
  static Encoder* Create(Environment* env, const Handlers* handlers,
                         BytesSink* output);

  /* The input to the encoder. */
  Sink* input();

  /* Creates a new set of handlers for this MessageDef. */
  static reffed_ptr<const Handlers> NewHandlers(const MessageDef* msg);

  static const size_t kSize = UPB_PB_ENCODER_SIZE;

 private:
  UPB_DISALLOW_POD_OPS(Encoder, upb::pb::Encoder)
};

#endif

UPB_BEGIN_EXTERN_C

const upb_handlers *upb_pb_encoder_newhandlers(const upb_msgdef *m,
                                               const void *owner);
upb_sink *upb_pb_encoder_input(upb_pb_encoder *p);
upb_pb_encoder* upb_pb_encoder_create(upb_env* e, const upb_handlers* h,
                                      upb_bytessink* output);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace pb {
inline Encoder* Encoder::Create(Environment* env, const Handlers* handlers,
                                BytesSink* output) {
  return upb_pb_encoder_create(env, handlers, output);
}
inline Sink* Encoder::input() {
  return upb_pb_encoder_input(this);
}
inline reffed_ptr<const Handlers> Encoder::NewHandlers(
    const upb::MessageDef *md) {
  const Handlers* h = upb_pb_encoder_newhandlers(md, &h);
  return reffed_ptr<const Handlers>(h, &h);
}
}  /* namespace pb */
}  /* namespace upb */

#endif

#endif  /* UPB_ENCODER_H_ */
