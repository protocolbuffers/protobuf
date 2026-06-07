#ifndef PERL_PROTOBUF_DESCRIPTOR_H_
#define PERL_PROTOBUF_DESCRIPTOR_H_

#include "xs/descriptor/base.h"
#include "xs/descriptor/enum.h"
#include "xs/descriptor/enum_value.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/file.h"
#include "xs/descriptor/message.h"
#include "xs/descriptor/method.h"
#include "xs/descriptor/oneof.h"
#include "xs/descriptor/service.h"

// Top-level init
bool PerlUpb_InitDescriptor(pTHX_ SV* module);

#endif  // PERL_PROTOBUF_DESCRIPTOR_H_
