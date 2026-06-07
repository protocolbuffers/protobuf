#ifndef PERL_PROTOBUF_UTILS_H_
#define PERL_PROTOBUF_UTILS_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"

struct upb_MessageDef;
typedef struct upb_MessageDef upb_MessageDef;
struct upb_FieldDef;
typedef struct upb_FieldDef upb_FieldDef;

// String utils
const char* PerlUpb_GetStrData(pTHX_ SV* sv);
const char* PerlUpb_VerifyStrData(pTHX_ SV* sv);

// Converts a Perl class name (A::B) to a Protobuf full name (A.B).
// Caller is responsible for Safefree()ing the returned string.
char* PerlUpb_ClassNameToFullName(pTHX_ const char* class_name);

// Converts a Protobuf full name (A.B) to a Perl class name (A::B).
// Caller is responsible for Safefree()ing the returned string.
char* PerlUpb_FullNameToClassName(pTHX_ const char* full_name);

// Returns the Perl stash (HV*) for the given message definition, with caching.
HV* PerlUpb_GetMessageStash(pTHX_ const upb_MessageDef* mdef);

// Logs a message and dies with Perl context
void PerlUpb_Error_Die(pTHX_ const char* fmt, ...);

// Maps upb_DecodeStatus to a descriptive Perl croak message and dies.
#include "upb/wire/decode.h"
void PerlUpb_DecodeStatus_Die(pTHX_ upb_DecodeStatus status,
                              const char* context);

// Wraps a C pointer into a Perl object, optionally keeping another Perl object
// (the arena) alive.
SV* PerlUpb_WrapArenaBoundObject(pTHX_ const void* ptr, SV* arena_sv, HV* stash,
                                 uint16_t flags);

// Extracts the C pointer from a wrapped object, verifying the class name.
const void* PerlUpb_GetArenaBoundObject(pTHX_ SV* sv, const char* class_name);
const void* PerlUpb_GetArenaBoundObject_Silent(pTHX_ SV* sv,
                                               const char* class_name);
bool PerlUpb_IsXSBacked(pTHX_ SV* sv);

// Retrieves the arena SV associated with the wrapped object.
SV* PerlUpb_GetArenaFromObject(pTHX_ SV* sv);

// 64-bit numeric conversion
int64_t PerlUpb_SVToI64(pTHX_ SV* sv);
uint64_t PerlUpb_SVToU64(pTHX_ SV* sv);
SV* PerlUpb_I64ToSV(pTHX_ int64_t val);
SV* PerlUpb_U64ToSV(pTHX_ uint64_t val);

// Context-aware error reporting
void PerlUpb_CroakWithContext(pTHX_ const char* msg, const upb_MessageDef* mdef,
                              const upb_FieldDef* fdef);

// Batch Validation API (Field Vectors)
typedef struct {
  const upb_FieldDef** fields;
  SV** values;
  size_t count;
  size_t capacity;
} PerlUpb_FieldVector;

PerlUpb_FieldVector* PerlUpb_FieldVector_New(pTHX_ size_t capacity);
void PerlUpb_FieldVector_Add(pTHX_ PerlUpb_FieldVector* v,
                             const upb_FieldDef* f, SV* val);
void PerlUpb_FieldVector_Free(pTHX_ PerlUpb_FieldVector* v);

// CPUID & SIMD Kernels
typedef enum {
  PERL_UPB_HAS_SSE41 = 1 << 0,
  PERL_UPB_HAS_AVX2 = 1 << 1
} PerlUpb_CpuFeatures;

void PerlUpb_InitCpuFeatures(void);
uint32_t PerlUpb_GetCpuFeatures(void);

// Initialize CPU feature detection for SIMD dynamic dispatch
void PerlUpb_DetectCpuFeatures(void);

// SIMD Kernels (Internal)
bool PerlUpb_ValidateIntRange_SSE41(const int32_t* vals, size_t count,
                                    int32_t min, int32_t max);
bool PerlUpb_ValidateStrings_AVX2(const char** strings, const size_t* lens,
                                  size_t count);

// Instrumentation & Verification
void PerlUpb_VerifyBinaryDiff(pTHX_ const char* a, size_t a_len, const char* b,
                              size_t b_len, const char* name);

// Magic Management
#define PERL_UPB_MG_CACHE_DIRTY 0x01
#define PERL_UPB_MG_PROFILE_MASK 0x0E
#define PERL_UPB_MG_PROFILE_BALANCED 0x00
#define PERL_UPB_MG_PROFILE_WRITE_HEAVY 0x02
#define PERL_UPB_MG_PROFILE_READ_HEAVY 0x04
#define PERL_UPB_MG_PROFILE_ZERO_COPY 0x08
MAGIC* PerlUpb_GetMagic(pTHX_ SV* sv);

#endif  // PERL_PROTOBUF_UTILS_H_
