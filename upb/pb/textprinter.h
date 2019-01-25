/*
** upb::pb::TextPrinter (upb_textprinter)
**
** Handlers for writing to protobuf text format.
*/

#ifndef UPB_TEXT_H_
#define UPB_TEXT_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace pb {
class TextPrinterPtr;
}  /* namespace pb */
}  /* namespace upb */
#endif

/* upb_textprinter ************************************************************/

struct upb_textprinter;
typedef struct upb_textprinter upb_textprinter;

#ifdef __cplusplus
extern "C" {
#endif

/* C API. */
upb_textprinter *upb_textprinter_create(upb_arena *arena, const upb_handlers *h,
                                        upb_bytessink output);
void upb_textprinter_setsingleline(upb_textprinter *p, bool single_line);
upb_sink upb_textprinter_input(upb_textprinter *p);
upb_handlercache *upb_textprinter_newcache();

#ifdef __cplusplus
}  /* extern "C" */

class upb::pb::TextPrinterPtr {
 public:
  TextPrinterPtr(upb_textprinter* ptr) : ptr_(ptr) {}

  /* The given handlers must have come from NewHandlers().  It must outlive the
   * TextPrinter. */
  static TextPrinterPtr Create(Arena *arena, upb::HandlersPtr *handlers,
                               BytesSink output) {
    return TextPrinterPtr(
        upb_textprinter_create(arena->ptr(), handlers->ptr(), output.sink()));
  }

  void SetSingleLineMode(bool single_line) {
    upb_textprinter_setsingleline(ptr_, single_line);
  }

  Sink input() { return upb_textprinter_input(ptr_); }

  /* If handler caching becomes a requirement we can add a code cache as in
   * decoder.h */
  static HandlerCache NewCache() {
    return HandlerCache(upb_textprinter_newcache());
  }

 private:
  upb_textprinter* ptr_;
};

#endif

#endif  /* UPB_TEXT_H_ */
