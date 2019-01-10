/*
** upb::json::Printer
**
** Handlers that emit JSON according to a specific protobuf schema.
*/

#ifndef UPB_JSON_TYPED_PRINTER_H_
#define UPB_JSON_TYPED_PRINTER_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace json {
class Printer;
}  /* namespace json */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::json::Printer, upb_json_printer)


/* upb::json::Printer *********************************************************/

#define UPB_JSON_PRINTER_SIZE 192

#ifdef __cplusplus

/* Prints an incoming stream of data to a BytesSink in JSON format. */
class upb::json::Printer {
 public:
  static Printer* Create(Environment* env, const upb::Handlers* handlers,
                         BytesSink* output);

  /* The input to the printer. */
  Sink* input();

  static const size_t kSize = UPB_JSON_PRINTER_SIZE;
  static upb_handlercache* NewCache(bool preserve_proto_fieldnames);

 private:
  UPB_DISALLOW_POD_OPS(Printer, upb::json::Printer)
};

#endif

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_json_printer *upb_json_printer_create(upb_env *e, const upb_handlers *h,
                                          upb_bytessink *output);
upb_sink *upb_json_printer_input(upb_json_printer *p);
const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 bool preserve_fieldnames,
                                                 const void *owner);

upb_handlercache *upb_json_printer_newcache(bool preserve_proto_fieldnames);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace json {
inline Printer* Printer::Create(Environment* env, const upb::Handlers* handlers,
                                BytesSink* output) {
  return upb_json_printer_create(env, handlers, output);
}
inline Sink* Printer::input() { return upb_json_printer_input(this); }
inline upb_handlercache* Printer::NewCache(bool preserve_proto_fieldnames) {
  return upb_json_printer_newcache(preserve_proto_fieldnames);
}
}  /* namespace json */
}  /* namespace upb */

#endif

#endif  /* UPB_JSON_TYPED_PRINTER_H_ */
