#ifndef PERL_PROTOBUF_EXTENSION_DICT_DICT_H_
#define PERL_PROTOBUF_EXTENSION_DICT_DICT_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

// ExtensionDict wraps the extension fields of a message.
// It acts as a lazy map from FieldDescriptor (extensions) to their values.

SV* PerlUpb_ExtensionDict_New(pTHX_ SV* message_sv);

// Returns the value for an extension field.
SV* PerlUpb_ExtensionDict_GetItem(pTHX_ SV* self, SV* field_sv);

// Sets the value for an extension field.
void PerlUpb_ExtensionDict_SetItem(pTHX_ SV* self, SV* field_sv, SV* value_sv);

// Internal helper to get the parent message SV
SV* PerlUpb_ExtensionDict_GetMessageSV(pTHX_ SV* self);

// Audit identity connection between dict and parent message
void PerlUpb_ExtensionDict_AuditIdentity(pTHX_ SV* self);

#endif  // PERL_PROTOBUF_EXTENSION_DICT_DICT_H_
