#ifndef PERL_PROTOBUF_MESSAGE_SERIALIZE_H_
#define PERL_PROTOBUF_MESSAGE_SERIALIZE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

// Parses a serialized string into a new Protobuf::Message wrapper.
SV* PerlUpb_Message_Parse(pTHX_ SV* descriptor_sv, SV* data_sv);

// Parses a serialized string and merges it into an existing Protobuf::Message
// wrapper.
void PerlUpb_Message_ParseFrom(pTHX_ SV* message_sv, SV* data_sv);

// Serializes a Protobuf::Message wrapper into a string SV.
SV* PerlUpb_Message_Serialize(pTHX_ SV* message_sv);

// Serializes a Protobuf::Message wrapper into a string SV with deterministic
// field ordering.
SV* PerlUpb_Message_Serialize_Deterministic(pTHX_ SV* message_sv);

// Serializes a Protobuf::Message wrapper into a TextFormat string SV.
SV* PerlUpb_Message_ToText(pTHX_ SV* message_sv);

// Serializes a Protobuf::Message wrapper into a JSON string SV.
SV* PerlUpb_Message_ToJson(pTHX_ SV* message_sv);

// Serializes a Protobuf::Message wrapper directly to a file handle as JSON.
void PerlUpb_Message_JsonToHandle(pTHX_ SV* message_sv, SV* fh_sv);

// Parses a JSON string SV into a new Protobuf::Message wrapper.
SV* PerlUpb_Message_FromJson(pTHX_ SV* descriptor_sv, SV* json_sv,
                             SV* options_sv);

// Serializes a Protobuf::Message wrapper directly to a file handle.
void PerlUpb_Message_ToHandle(pTHX_ SV* message_sv, SV* fh_sv,
                              bool length_prefixed);

// Parses a Protobuf::Message wrapper from a file handle.
SV* PerlUpb_Message_FromHandle(pTHX_ SV* descriptor_sv, SV* fh_sv,
                               bool length_prefixed);

#endif  // PERL_PROTOBUF_MESSAGE_SERIALIZE_H_
