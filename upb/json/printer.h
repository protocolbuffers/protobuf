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
class PrinterPtr;
}  /* namespace json */
}  /* namespace upb */
#endif

/* upb_json_printer ***********************************************************/

#define UPB_JSON_PRINTER_SIZE 192

struct upb_json_printer;
typedef struct upb_json_printer upb_json_printer;

#ifdef __cplusplus
extern "C" {
#endif

/* Native C API. */
upb_json_printer *upb_json_printer_create(upb_arena *a, const upb_handlers *h,
                                          upb_bytessink output);
upb_sink upb_json_printer_input(upb_json_printer *p);
const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 bool preserve_fieldnames,
                                                 const void *owner);

/* Lazily builds and caches handlers that will push encoded data to a bytessink.
 * Any msgdef objects used with this object must outlive it. */
upb_handlercache *upb_json_printer_newcache(bool preserve_proto_fieldnames);

#ifdef __cplusplus
}  /* extern "C" */

/* Prints an incoming stream of data to a BytesSink in JSON format. */
class upb::json::PrinterPtr {
 public:
  PrinterPtr(upb_json_printer* ptr) : ptr_(ptr) {}

  static PrinterPtr Create(Arena *arena, const upb::Handlers *handlers,
                           BytesSink output) {
    return PrinterPtr(
        upb_json_printer_create(arena->ptr(), handlers, output.sink()));
  }

  /* The input to the printer. */
  Sink input() { return upb_json_printer_input(ptr_); }

  static const size_t kSize = UPB_JSON_PRINTER_SIZE;

  static HandlerCache NewCache(bool preserve_proto_fieldnames) {
    return upb_json_printer_newcache(preserve_proto_fieldnames);
  }

 private:
  upb_json_printer* ptr_;
};

#endif  /* __cplusplus */

#endif  /* UPB_JSON_TYPED_PRINTER_H_ */
