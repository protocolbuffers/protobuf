#ifndef UPB_TRACING_TRACING_H_
#define UPB_TRACING_TRACING_H_

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_MiniTable upb_MiniTable;
typedef struct upb_Arena upb_Arena;

#if UPB_TRACING_ENABLED
void upb_tracing_Init(void (*logMessageNewHandler)(const upb_MiniTable*,
                                                   const upb_Arena*));
void upb_tracing_LogMessageNew(const upb_MiniTable* mini_table,
                               const upb_Arena* arena);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_TRACING_TRACING_H_
