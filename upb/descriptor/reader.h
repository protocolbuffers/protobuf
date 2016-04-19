/*
** upb::descriptor::Reader (upb_descreader)
**
** Provides a way of building upb::Defs from data in descriptor.proto format.
*/

#ifndef UPB_DESCRIPTOR_H
#define UPB_DESCRIPTOR_H

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace descriptor {
class Reader;
}  /* namespace descriptor */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::descriptor::Reader, upb_descreader)

#ifdef __cplusplus

/* Class that receives descriptor data according to the descriptor.proto schema
 * and use it to build upb::Defs corresponding to that schema. */
class upb::descriptor::Reader {
 public:
  /* These handlers must have come from NewHandlers() and must outlive the
   * Reader.
   *
   * TODO: generate the handlers statically (like we do with the
   * descriptor.proto defs) so that there is no need to pass this parameter (or
   * to build/memory-manage the handlers at runtime at all).  Unfortunately this
   * is a bit tricky to implement for Handlers, but necessary to simplify this
   * interface. */
  static Reader* Create(Environment* env, const Handlers* handlers);

  /* The reader's input; this is where descriptor.proto data should be sent. */
  Sink* input();

  /* Use to get the FileDefs that have been parsed. */
  size_t file_count() const;
  FileDef* file(size_t i) const;

  /* Builds and returns handlers for the reader, owned by "owner." */
  static Handlers* NewHandlers(const void* owner);

 private:
  UPB_DISALLOW_POD_OPS(Reader, upb::descriptor::Reader)
};

#endif

UPB_BEGIN_EXTERN_C

/* C API. */
upb_descreader *upb_descreader_create(upb_env *e, const upb_handlers *h);
upb_sink *upb_descreader_input(upb_descreader *r);
size_t upb_descreader_filecount(const upb_descreader *r);
upb_filedef *upb_descreader_file(const upb_descreader *r, size_t i);
const upb_handlers *upb_descreader_newhandlers(const void *owner);

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ implementation details. ************************************************/
namespace upb {
namespace descriptor {
inline Reader* Reader::Create(Environment* e, const Handlers *h) {
  return upb_descreader_create(e, h);
}
inline Sink* Reader::input() { return upb_descreader_input(this); }
inline size_t Reader::file_count() const {
  return upb_descreader_filecount(this);
}
inline FileDef* Reader::file(size_t i) const {
  return upb_descreader_file(this, i);
}
}  /* namespace descriptor */
}  /* namespace upb */
#endif

#endif  /* UPB_DESCRIPTOR_H */
