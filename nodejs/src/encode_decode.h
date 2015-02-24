#ifndef __PROTOBUF_NODEJS_SRC_ENCODE_DECODE_H__
#define __PROTOBUF_NODEJS_SRC_ENCODE_DECODE_H__

#include <nan.h>
#include "defs.h"
#include "util.h"

namespace protobuf_js {

NAN_METHOD(EncodeMethod);
NAN_METHOD(EncodeGlobalFunction);

NAN_METHOD(DecodeMethod);
NAN_METHOD(DecodeGlobalFunction);

NAN_METHOD(EncodeJsonMethod);
NAN_METHOD(EncodeJsonGlobalFunction);

NAN_METHOD(DecodeJsonMethod);
NAN_METHOD(DecodeJsonGlobalFunction);

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_ENCODE_DECODE_H__
