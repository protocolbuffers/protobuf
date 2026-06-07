#ifndef PERL_PROTOBUF_UNKNOWN_FIELDS_SET_H_
#define PERL_PROTOBUF_UNKNOWN_FIELDS_SET_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

// Wraps the unknown fields of a message.
SV* PerlUpb_UnknownFieldSet_New(pTHX_ SV* message_sv);

// Returns the raw bytes of the unknown fields.
SV* PerlUpb_UnknownFieldSet_GetData(pTHX_ SV* self);

// Adds raw bytes to the unknown fields.
void PerlUpb_UnknownFieldSet_Add(pTHX_ SV* self, SV* data_sv);

// Clears the unknown fields.
void PerlUpb_UnknownFieldSet_Clear(pTHX_ SV* self);

// Selectively deletes a tag from the unknown fields.
void PerlUpb_UnknownFieldSet_DeleteTag(pTHX_ SV* self, uint32_t tag);

// Frees the wrapper.
void PerlUpb_UnknownFieldSet_Free(pTHX_ SV* sv);

#endif  // PERL_PROTOBUF_UNKNOWN_FIELDS_SET_H_
