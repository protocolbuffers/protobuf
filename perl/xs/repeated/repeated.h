#ifndef PERL_PROTOBUF_REPEATED_REPEATED_H_
#define PERL_PROTOBUF_REPEATED_REPEATED_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/message/array.h"
#include "upb/reflection/def.h"
#include "xs/protobuf.h"

// PerlUpb_Repeated wraps a upb_Array.
SV* PerlUpb_Repeated_New(pTHX_ upb_Array* arr, const upb_FieldDef* f,
                         SV* arena_sv, uint16_t flags);

// Returns the value at a given index.
SV* PerlUpb_Repeated_GetItem(pTHX_ SV* self, int index);

// Sets the value at a given index.
void PerlUpb_Repeated_SetItem(pTHX_ SV* self, int index, SV* val_sv);

// Appends a value to the end of the array.
void PerlUpb_Repeated_Append(pTHX_ SV* self, SV* val_sv);

// Deletes items from the array (splice-like).
void PerlUpb_Repeated_Delete(pTHX_ SV* self, int index, int count);

// Returns the number of items in the array.
int PerlUpb_Repeated_Size(pTHX_ SV* self);

// Returns the associated arena SV.
SV* PerlUpb_Repeated_GetArenaSV(pTHX_ SV* self);

// Inserts a value at a given index.
void PerlUpb_Repeated_Insert(pTHX_ SV* self, int index, SV* val_sv);

// Resizes the array.
void PerlUpb_Repeated_Resize(pTHX_ SV* self, int size);

// Clears the array.
void PerlUpb_Repeated_Clear(pTHX_ SV* self);

// Returns the internal upb_Array.
upb_Array* PerlUpb_Repeated_GetArray(pTHX_ SV* self);

// Returns the field definition.
const upb_FieldDef* PerlUpb_Repeated_GetFieldDef(pTHX_ SV* self);

// Audits the integrity of the repeated field.
bool PerlUpb_Repeated_AuditIntegrity(pTHX_ SV* self);

// Frees the wrapper.
void PerlUpb_Repeated_Free(pTHX_ SV* sv);

#endif  // PERL_PROTOBUF_REPEATED_REPEATED_H_
