
#include "upb/pb/varint.int.h"

/* Index is descriptor type. */
const uint8_t upb_pb_native_wire_types[] = {
  UPB_WIRE_TYPE_END_GROUP,     /* ENDGROUP */
  UPB_WIRE_TYPE_64BIT,         /* DOUBLE */
  UPB_WIRE_TYPE_32BIT,         /* FLOAT */
  UPB_WIRE_TYPE_VARINT,        /* INT64 */
  UPB_WIRE_TYPE_VARINT,        /* UINT64 */
  UPB_WIRE_TYPE_VARINT,        /* INT32 */
  UPB_WIRE_TYPE_64BIT,         /* FIXED64 */
  UPB_WIRE_TYPE_32BIT,         /* FIXED32 */
  UPB_WIRE_TYPE_VARINT,        /* BOOL */
  UPB_WIRE_TYPE_DELIMITED,     /* STRING */
  UPB_WIRE_TYPE_START_GROUP,   /* GROUP */
  UPB_WIRE_TYPE_DELIMITED,     /* MESSAGE */
  UPB_WIRE_TYPE_DELIMITED,     /* BYTES */
  UPB_WIRE_TYPE_VARINT,        /* UINT32 */
  UPB_WIRE_TYPE_VARINT,        /* ENUM */
  UPB_WIRE_TYPE_32BIT,         /* SFIXED32 */
  UPB_WIRE_TYPE_64BIT,         /* SFIXED64 */
  UPB_WIRE_TYPE_VARINT,        /* SINT32 */
  UPB_WIRE_TYPE_VARINT,        /* SINT64 */
};

/* A basic branch-based decoder, uses 32-bit values to get good performance
 * on 32-bit architectures (but performs well on 64-bits also).
 * This scheme comes from the original Google Protobuf implementation
 * (proto2). */
upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r) {
  upb_decoderet err = {NULL, 0};
  const char *p = r.p;
  uint32_t low = (uint32_t)r.val;
  uint32_t high = 0;
  uint32_t b;
  b = *(p++); low  |= (b & 0x7fU) << 14; if (!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7fU) << 21; if (!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7fU) << 28;
              high  = (b & 0x7fU) >>  4; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) <<  3; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 10; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 17; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 24; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 31; if (!(b & 0x80)) goto done;
  return err;

done:
  r.val = ((uint64_t)high << 32) | low;
  r.p = p;
  return r;
}

/* Like the previous, but uses 64-bit values. */
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r) {
  const char *p = r.p;
  uint64_t val = r.val;
  uint64_t b;
  upb_decoderet err = {NULL, 0};
  b = *(p++); val |= (b & 0x7fU) << 14; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 21; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 28; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 35; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 42; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 49; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 56; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 63; if (!(b & 0x80)) goto done;
  return err;

done:
  r.val = val;
  r.p = p;
  return r;
}
