#ifndef __GOOGLE_PROTOBUF_PHP_FAKE_PROTOBUF_H__
#define __GOOGLE_PROTOBUF_PHP_FAKE_PROTOBUF_H__

#include "upb.h"

#ifdef HHVM
#include "port_hhvm.h"
#else  // HHVM
#include "port_php7.h"
#endif  // php7

PROTO_WRAP_OBJECT_START(InternalDescriptorPool)
  upb_symtab* symtab;
//   HashTable* pending_list;
PROTO_WRAP_OBJECT_END

void internal_descriptor_pool_init();

PROTO_METHOD(InternalDescriptorPool, internalAddGeneratedFile, bool);

#endif  // __GOOGLE_PROTOBUF_PHP_FAKE_PROTOBUF_H__
