#ifndef PERL_PROTOBUF_MAP_MAP_H_
#define PERL_PROTOBUF_MAP_MAP_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/message/map.h"
#include "upb/reflection/def.h"
#include "xs/protobuf.h"

// Map wraps a upb_Map.
// It acts as a lazy map from keys to values.

SV* PerlUpb_Map_New(pTHX_ upb_Map* map, const upb_FieldDef* f, SV* arena_sv,
                    uint16_t flags);

// Returns the value for a given key.
SV* PerlUpb_Map_GetItem(pTHX_ SV* self, SV* key_sv);

// Returns true if the key exists in the map.
bool PerlUpb_Map_Exists(pTHX_ SV* self, SV* key_sv);

// Sets the value for a given key.
void PerlUpb_Map_SetItem(pTHX_ SV* self, SV* key_sv, SV* value_sv);

// Deletes an item from the map.
void PerlUpb_Map_DeleteItem(pTHX_ SV* self, SV* key_sv);

// Clears the map.
void PerlUpb_Map_Clear(pTHX_ SV* self);

// Returns the number of items in the map.
int PerlUpb_Map_Size(pTHX_ SV* self);

// Returns a standard Perl hash containing all items in the map.
SV* PerlUpb_Map_AsHash(pTHX_ SV* self);

// Frees the map wrapper.
void PerlUpb_Map_Free(pTHX_ SV* self);

// Internal helpers for iterator
const upb_FieldDef* PerlUpb_Map_GetFieldDef(pTHX_ SV* self);
upb_Map* PerlUpb_Map_GetMapPtr(pTHX_ SV* self);
SV* PerlUpb_Map_GetArenaSV(pTHX_ SV* self);

#endif  // PERL_PROTOBUF_MAP_MAP_H_
