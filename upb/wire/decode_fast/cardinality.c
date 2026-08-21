
#include "upb/wire/decode_fast/cardinality.h"

#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/eps_copy_input_stream.h"

UPB_PRESERVE_MOST
const char* upb_DecodeFast_IsDoneFallback(upb_Decoder* d, const char* ptr) {
  int overrun;
  upb_IsDoneStatus status = UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
      &d->input, ptr, &overrun);
  UPB_ASSERT(status == kUpb_IsDoneStatus_NeedFallback);
  return UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallback)(EPS(d), ptr,
                                                            overrun);
}
