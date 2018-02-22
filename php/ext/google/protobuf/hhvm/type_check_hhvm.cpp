#include "protobuf_hhvm.h"

void HHVM_METHOD(Util, checkInt32, const Variant& val);
void HHVM_METHOD(Util, checkUint32, const Variant& val);
void HHVM_METHOD(Util, checkInt64, const Variant& val);
void HHVM_METHOD(Util, checkUint64, const Variant& val);
void HHVM_METHOD(Util, checkEnum, const Variant& val, const String& klass);
void HHVM_METHOD(Util, checkFloat, const Variant& val);
void HHVM_METHOD(Util, checkDouble, const Variant& val);
void HHVM_METHOD(Util, checkBool, const Variant& val);
void HHVM_METHOD(Util, checkString, const Variant& val, bool utf8);
void HHVM_METHOD(Util, checkBytes, const Variant& val);
void HHVM_METHOD(Util, checkMessage, const Variant& val, const String& klass);
Variant HHVM_METHOD(Util, checkMapField, const Variant& val,
                    int64_t key_type, int64_t value_type, const Variant& klass);
Variant HHVM_METHOD(Util, checkRepeatedField, const Variant& val,
                    int64_t type, const Variant& klass);

const StaticString s_Util("Google\\Protobuf\\Internal\\GPBUtil");

class Util {};

void Util_init() {
  // Register methods
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkInt32, HHVM_MN(Util, checkInt32));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkUint32, HHVM_MN(Util, checkUint32));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkInt64, HHVM_MN(Util, checkInt64));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkUint64, HHVM_MN(Util, checkUint64));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkEnum, HHVM_MN(Util, checkEnum));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkFloat, HHVM_MN(Util, checkFloat));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkDouble, HHVM_MN(Util, checkDouble));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkBool, HHVM_MN(Util, checkBool));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkString, HHVM_MN(Util, checkString));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkBytes, HHVM_MN(Util, checkBytes));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkMapField, HHVM_MN(Util, checkMapField));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkRepeatedField, HHVM_MN(Util, checkRepeatedField));
  HHVM_NAMED_STATIC_ME(Google\\Protobuf\\Internal\\GPBUtil,
                       checkMessage, HHVM_MN(Util, checkMessage));

  // Register class
  Native::registerNativeDataInfo<Util>(s_Util.get()); 
}

void HHVM_METHOD(Util, checkInt32, const Variant& val) {
}

void HHVM_METHOD(Util, checkUint32, const Variant& val) {
}

void HHVM_METHOD(Util, checkInt64, const Variant& val) {
}

void HHVM_METHOD(Util, checkUint64, const Variant& val) {
}

void HHVM_METHOD(Util, checkEnum, const Variant& val, const String& klass) {
}

void HHVM_METHOD(Util, checkFloat, const Variant& val) {
}

void HHVM_METHOD(Util, checkDouble, const Variant& val) {
}

void HHVM_METHOD(Util, checkBool, const Variant& val) {
}

void HHVM_METHOD(Util, checkString, const Variant& val, bool utf8) {
}

void HHVM_METHOD(Util, checkBytes, const Variant& val) {
}

Variant HHVM_METHOD(Util, checkMapField, const Variant& val,
                    int64_t key_type, int64_t value_type,
                    const Variant& klass) {
  return val;
}

Variant HHVM_METHOD(Util, checkRepeatedField, const Variant& val,
                    int64_t type, const Variant& klass) {
  return val;
}

void HHVM_METHOD(Util, checkMessage, const Variant& val, const String& klass) {
}
