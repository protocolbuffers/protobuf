/*
 * This file is a data dump of a protocol buffer into a C structure.
 * It was created by the upb compiler (upbc) with the following
 * command-line:
 *
 *   /Users/haberman/code/upb/tools/upbc -i upb_file_descriptor_set -o descriptor/descriptor descriptor/descriptor.proto.pb
 *
 * This file is a dump of 'descriptor/descriptor.proto.pb'.
 * It contains exactly the same data, but in a C structure form
 * instead of a serialized protobuf.  This file contains no code,
 * only data.
 *
 * This file was auto-generated.  Do not edit. */

#include "descriptor/descriptor.h"

static char strdata[] =
  ".google.protobuf.DescriptorProto.google.protobuf.DescriptorProto.ExtensionRan"
  "ge.google.protobuf.EnumDescriptorProto.google.protobuf.EnumOptions.google.pro"
  "tobuf.EnumValueDescriptorProto.google.protobuf.EnumValueOptions.google.protob"
  "uf.FieldDescriptorProto.google.protobuf.FieldDescriptorProto.Label.google.pro"
  "tobuf.FieldDescriptorProto.Type.google.protobuf.FieldOptions.google.protobuf."
  "FieldOptions.CType.google.protobuf.FileDescriptorProto.google.protobuf.FileOp"
  "tions.google.protobuf.FileOptions.OptimizeMode.google.protobuf.MessageOptions"
  ".google.protobuf.MethodDescriptorProto.google.protobuf.MethodOptions.google.p"
  "rotobuf.ServiceDescriptorProto.google.protobuf.ServiceOptions.google.protobuf"
  ".UninterpretedOption.google.protobuf.UninterpretedOption.NamePartCODE_SIZECOR"
  "DCTypeDescriptorProtoDescriptorProtosEnumDescriptorProtoEnumOptionsEnumValueD"
  "escriptorProtoEnumValueOptionsExtensionRangeFieldDescriptorProtoFieldOptionsF"
  "ileDescriptorProtoFileDescriptorSetFileOptionsLABEL_OPTIONALLABEL_REPEATEDLAB"
  "EL_REQUIREDLabelMessageOptionsMethodDescriptorProtoMethodOptionsNamePartOptim"
  "izeModeSPEEDSTRING_PIECEServiceDescriptorProtoServiceOptionsTYPE_BOOLTYPE_BYT"
  "ESTYPE_DOUBLETYPE_ENUMTYPE_FIXED32TYPE_FIXED64TYPE_FLOATTYPE_GROUPTYPE_INT32T"
  "YPE_INT64TYPE_MESSAGETYPE_SFIXED32TYPE_SFIXED64TYPE_SINT32TYPE_SINT64TYPE_STR"
  "INGTYPE_UINT32TYPE_UINT64TypeUninterpretedOptioncom.google.protobufctypedefau"
  "lt_valuedependencydeprecateddescriptor/descriptor.protodouble_valueendenum_ty"
  "peexperimental_map_keyextendeeextensionextension_rangefalsefieldfilegoogle.pr"
  "otobufidentifier_valueinput_typeis_extensionjava_multiple_filesjava_outer_cla"
  "ssnamejava_packagelabelmessage_set_wire_formatmessage_typemethodnamename_part"
  "negative_int_valuenested_typenumberoptimize_foroptionsoutput_typepackagepacke"
  "dpositive_int_valueservicestartstring_valuetypetype_nameuninterpreted_optionv"
  "alue";

static struct upb_string strings[] = {
  {.ptr = &strdata[0], .byte_len=32},
  {.ptr = &strdata[32], .byte_len=47},
  {.ptr = &strdata[79], .byte_len=36},
  {.ptr = &strdata[115], .byte_len=28},
  {.ptr = &strdata[143], .byte_len=41},
  {.ptr = &strdata[184], .byte_len=33},
  {.ptr = &strdata[217], .byte_len=37},
  {.ptr = &strdata[254], .byte_len=43},
  {.ptr = &strdata[297], .byte_len=42},
  {.ptr = &strdata[339], .byte_len=29},
  {.ptr = &strdata[368], .byte_len=35},
  {.ptr = &strdata[403], .byte_len=36},
  {.ptr = &strdata[439], .byte_len=28},
  {.ptr = &strdata[467], .byte_len=41},
  {.ptr = &strdata[508], .byte_len=31},
  {.ptr = &strdata[539], .byte_len=38},
  {.ptr = &strdata[577], .byte_len=30},
  {.ptr = &strdata[607], .byte_len=39},
  {.ptr = &strdata[646], .byte_len=31},
  {.ptr = &strdata[677], .byte_len=36},
  {.ptr = &strdata[713], .byte_len=45},
  {.ptr = &strdata[758], .byte_len=9},
  {.ptr = &strdata[767], .byte_len=4},
  {.ptr = &strdata[771], .byte_len=5},
  {.ptr = &strdata[776], .byte_len=15},
  {.ptr = &strdata[791], .byte_len=16},
  {.ptr = &strdata[807], .byte_len=19},
  {.ptr = &strdata[826], .byte_len=11},
  {.ptr = &strdata[837], .byte_len=24},
  {.ptr = &strdata[861], .byte_len=16},
  {.ptr = &strdata[877], .byte_len=14},
  {.ptr = &strdata[891], .byte_len=20},
  {.ptr = &strdata[911], .byte_len=12},
  {.ptr = &strdata[923], .byte_len=19},
  {.ptr = &strdata[942], .byte_len=17},
  {.ptr = &strdata[959], .byte_len=11},
  {.ptr = &strdata[970], .byte_len=14},
  {.ptr = &strdata[984], .byte_len=14},
  {.ptr = &strdata[998], .byte_len=14},
  {.ptr = &strdata[1012], .byte_len=5},
  {.ptr = &strdata[1017], .byte_len=14},
  {.ptr = &strdata[1031], .byte_len=21},
  {.ptr = &strdata[1052], .byte_len=13},
  {.ptr = &strdata[1065], .byte_len=8},
  {.ptr = &strdata[1073], .byte_len=12},
  {.ptr = &strdata[1085], .byte_len=5},
  {.ptr = &strdata[1090], .byte_len=12},
  {.ptr = &strdata[1102], .byte_len=22},
  {.ptr = &strdata[1124], .byte_len=14},
  {.ptr = &strdata[1138], .byte_len=9},
  {.ptr = &strdata[1147], .byte_len=10},
  {.ptr = &strdata[1157], .byte_len=11},
  {.ptr = &strdata[1168], .byte_len=9},
  {.ptr = &strdata[1177], .byte_len=12},
  {.ptr = &strdata[1189], .byte_len=12},
  {.ptr = &strdata[1201], .byte_len=10},
  {.ptr = &strdata[1211], .byte_len=10},
  {.ptr = &strdata[1221], .byte_len=10},
  {.ptr = &strdata[1231], .byte_len=10},
  {.ptr = &strdata[1241], .byte_len=12},
  {.ptr = &strdata[1253], .byte_len=13},
  {.ptr = &strdata[1266], .byte_len=13},
  {.ptr = &strdata[1279], .byte_len=11},
  {.ptr = &strdata[1290], .byte_len=11},
  {.ptr = &strdata[1301], .byte_len=11},
  {.ptr = &strdata[1312], .byte_len=11},
  {.ptr = &strdata[1323], .byte_len=11},
  {.ptr = &strdata[1334], .byte_len=4},
  {.ptr = &strdata[1338], .byte_len=19},
  {.ptr = &strdata[1357], .byte_len=19},
  {.ptr = &strdata[1376], .byte_len=5},
  {.ptr = &strdata[1381], .byte_len=13},
  {.ptr = &strdata[1394], .byte_len=10},
  {.ptr = &strdata[1404], .byte_len=10},
  {.ptr = &strdata[1414], .byte_len=27},
  {.ptr = &strdata[1441], .byte_len=12},
  {.ptr = &strdata[1453], .byte_len=3},
  {.ptr = &strdata[1456], .byte_len=9},
  {.ptr = &strdata[1465], .byte_len=20},
  {.ptr = &strdata[1485], .byte_len=8},
  {.ptr = &strdata[1493], .byte_len=9},
  {.ptr = &strdata[1502], .byte_len=15},
  {.ptr = &strdata[1517], .byte_len=5},
  {.ptr = &strdata[1522], .byte_len=5},
  {.ptr = &strdata[1527], .byte_len=4},
  {.ptr = &strdata[1531], .byte_len=15},
  {.ptr = &strdata[1546], .byte_len=16},
  {.ptr = &strdata[1562], .byte_len=10},
  {.ptr = &strdata[1572], .byte_len=12},
  {.ptr = &strdata[1584], .byte_len=19},
  {.ptr = &strdata[1603], .byte_len=20},
  {.ptr = &strdata[1623], .byte_len=12},
  {.ptr = &strdata[1635], .byte_len=5},
  {.ptr = &strdata[1640], .byte_len=23},
  {.ptr = &strdata[1663], .byte_len=12},
  {.ptr = &strdata[1675], .byte_len=6},
  {.ptr = &strdata[1681], .byte_len=4},
  {.ptr = &strdata[1685], .byte_len=9},
  {.ptr = &strdata[1694], .byte_len=18},
  {.ptr = &strdata[1712], .byte_len=11},
  {.ptr = &strdata[1723], .byte_len=6},
  {.ptr = &strdata[1729], .byte_len=12},
  {.ptr = &strdata[1741], .byte_len=7},
  {.ptr = &strdata[1748], .byte_len=11},
  {.ptr = &strdata[1759], .byte_len=7},
  {.ptr = &strdata[1766], .byte_len=6},
  {.ptr = &strdata[1772], .byte_len=18},
  {.ptr = &strdata[1790], .byte_len=7},
  {.ptr = &strdata[1797], .byte_len=5},
  {.ptr = &strdata[1802], .byte_len=12},
  {.ptr = &strdata[1814], .byte_len=4},
  {.ptr = &strdata[1818], .byte_len=9},
  {.ptr = &strdata[1827], .byte_len=20},
  {.ptr = &strdata[1847], .byte_len=5},
};

/* Forward declarations of messages, and array decls. */
static google_protobuf_DescriptorProto google_protobuf_DescriptorProto_values[18];

static google_protobuf_DescriptorProto *google_protobuf_DescriptorProto_array_elems[] = {
    &google_protobuf_DescriptorProto_values[0],
    &google_protobuf_DescriptorProto_values[1],
    &google_protobuf_DescriptorProto_values[2],
    &google_protobuf_DescriptorProto_values[3],
    &google_protobuf_DescriptorProto_values[4],
    &google_protobuf_DescriptorProto_values[5],
    &google_protobuf_DescriptorProto_values[6],
    &google_protobuf_DescriptorProto_values[7],
    &google_protobuf_DescriptorProto_values[8],
    &google_protobuf_DescriptorProto_values[9],
    &google_protobuf_DescriptorProto_values[10],
    &google_protobuf_DescriptorProto_values[11],
    &google_protobuf_DescriptorProto_values[12],
    &google_protobuf_DescriptorProto_values[13],
    &google_protobuf_DescriptorProto_values[14],
    &google_protobuf_DescriptorProto_values[15],
    &google_protobuf_DescriptorProto_values[16],
    &google_protobuf_DescriptorProto_values[17],
};
static UPB_MSG_ARRAY(google_protobuf_DescriptorProto) google_protobuf_DescriptorProto_arrays[3] = {
  {.elements = &google_protobuf_DescriptorProto_array_elems[0], .len=16},
  {.elements = &google_protobuf_DescriptorProto_array_elems[16], .len=1},
  {.elements = &google_protobuf_DescriptorProto_array_elems[17], .len=1},
};
static google_protobuf_FileDescriptorProto google_protobuf_FileDescriptorProto_values[1];

static google_protobuf_FileDescriptorProto *google_protobuf_FileDescriptorProto_array_elems[] = {
    &google_protobuf_FileDescriptorProto_values[0],
};
static UPB_MSG_ARRAY(google_protobuf_FileDescriptorProto) google_protobuf_FileDescriptorProto_arrays[1] = {
  {.elements = &google_protobuf_FileDescriptorProto_array_elems[0], .len=1},
};
static google_protobuf_FileDescriptorSet google_protobuf_FileDescriptorSet_values[1];

static google_protobuf_DescriptorProto_ExtensionRange google_protobuf_DescriptorProto_ExtensionRange_values[7];

static google_protobuf_DescriptorProto_ExtensionRange *google_protobuf_DescriptorProto_ExtensionRange_array_elems[] = {
    &google_protobuf_DescriptorProto_ExtensionRange_values[0],
    &google_protobuf_DescriptorProto_ExtensionRange_values[1],
    &google_protobuf_DescriptorProto_ExtensionRange_values[2],
    &google_protobuf_DescriptorProto_ExtensionRange_values[3],
    &google_protobuf_DescriptorProto_ExtensionRange_values[4],
    &google_protobuf_DescriptorProto_ExtensionRange_values[5],
    &google_protobuf_DescriptorProto_ExtensionRange_values[6],
};
static UPB_MSG_ARRAY(google_protobuf_DescriptorProto_ExtensionRange) google_protobuf_DescriptorProto_ExtensionRange_arrays[7] = {
  {.elements = &google_protobuf_DescriptorProto_ExtensionRange_array_elems[0], .len=1},
  {.elements = &google_protobuf_DescriptorProto_ExtensionRange_array_elems[1], .len=1},
  {.elements = &google_protobuf_DescriptorProto_ExtensionRange_array_elems[2], .len=1},
  {.elements = &google_protobuf_DescriptorProto_ExtensionRange_array_elems[3], .len=1},
  {.elements = &google_protobuf_DescriptorProto_ExtensionRange_array_elems[4], .len=1},
  {.elements = &google_protobuf_DescriptorProto_ExtensionRange_array_elems[5], .len=1},
  {.elements = &google_protobuf_DescriptorProto_ExtensionRange_array_elems[6], .len=1},
};
static google_protobuf_FileOptions google_protobuf_FileOptions_values[1];

static google_protobuf_EnumDescriptorProto google_protobuf_EnumDescriptorProto_values[4];

static google_protobuf_EnumDescriptorProto *google_protobuf_EnumDescriptorProto_array_elems[] = {
    &google_protobuf_EnumDescriptorProto_values[0],
    &google_protobuf_EnumDescriptorProto_values[1],
    &google_protobuf_EnumDescriptorProto_values[2],
    &google_protobuf_EnumDescriptorProto_values[3],
};
static UPB_MSG_ARRAY(google_protobuf_EnumDescriptorProto) google_protobuf_EnumDescriptorProto_arrays[3] = {
  {.elements = &google_protobuf_EnumDescriptorProto_array_elems[0], .len=2},
  {.elements = &google_protobuf_EnumDescriptorProto_array_elems[2], .len=1},
  {.elements = &google_protobuf_EnumDescriptorProto_array_elems[3], .len=1},
};
static google_protobuf_FieldDescriptorProto google_protobuf_FieldDescriptorProto_values[63];

static google_protobuf_FieldDescriptorProto *google_protobuf_FieldDescriptorProto_array_elems[] = {
    &google_protobuf_FieldDescriptorProto_values[0],
    &google_protobuf_FieldDescriptorProto_values[1],
    &google_protobuf_FieldDescriptorProto_values[2],
    &google_protobuf_FieldDescriptorProto_values[3],
    &google_protobuf_FieldDescriptorProto_values[4],
    &google_protobuf_FieldDescriptorProto_values[5],
    &google_protobuf_FieldDescriptorProto_values[6],
    &google_protobuf_FieldDescriptorProto_values[7],
    &google_protobuf_FieldDescriptorProto_values[8],
    &google_protobuf_FieldDescriptorProto_values[9],
    &google_protobuf_FieldDescriptorProto_values[10],
    &google_protobuf_FieldDescriptorProto_values[11],
    &google_protobuf_FieldDescriptorProto_values[12],
    &google_protobuf_FieldDescriptorProto_values[13],
    &google_protobuf_FieldDescriptorProto_values[14],
    &google_protobuf_FieldDescriptorProto_values[15],
    &google_protobuf_FieldDescriptorProto_values[16],
    &google_protobuf_FieldDescriptorProto_values[17],
    &google_protobuf_FieldDescriptorProto_values[18],
    &google_protobuf_FieldDescriptorProto_values[19],
    &google_protobuf_FieldDescriptorProto_values[20],
    &google_protobuf_FieldDescriptorProto_values[21],
    &google_protobuf_FieldDescriptorProto_values[22],
    &google_protobuf_FieldDescriptorProto_values[23],
    &google_protobuf_FieldDescriptorProto_values[24],
    &google_protobuf_FieldDescriptorProto_values[25],
    &google_protobuf_FieldDescriptorProto_values[26],
    &google_protobuf_FieldDescriptorProto_values[27],
    &google_protobuf_FieldDescriptorProto_values[28],
    &google_protobuf_FieldDescriptorProto_values[29],
    &google_protobuf_FieldDescriptorProto_values[30],
    &google_protobuf_FieldDescriptorProto_values[31],
    &google_protobuf_FieldDescriptorProto_values[32],
    &google_protobuf_FieldDescriptorProto_values[33],
    &google_protobuf_FieldDescriptorProto_values[34],
    &google_protobuf_FieldDescriptorProto_values[35],
    &google_protobuf_FieldDescriptorProto_values[36],
    &google_protobuf_FieldDescriptorProto_values[37],
    &google_protobuf_FieldDescriptorProto_values[38],
    &google_protobuf_FieldDescriptorProto_values[39],
    &google_protobuf_FieldDescriptorProto_values[40],
    &google_protobuf_FieldDescriptorProto_values[41],
    &google_protobuf_FieldDescriptorProto_values[42],
    &google_protobuf_FieldDescriptorProto_values[43],
    &google_protobuf_FieldDescriptorProto_values[44],
    &google_protobuf_FieldDescriptorProto_values[45],
    &google_protobuf_FieldDescriptorProto_values[46],
    &google_protobuf_FieldDescriptorProto_values[47],
    &google_protobuf_FieldDescriptorProto_values[48],
    &google_protobuf_FieldDescriptorProto_values[49],
    &google_protobuf_FieldDescriptorProto_values[50],
    &google_protobuf_FieldDescriptorProto_values[51],
    &google_protobuf_FieldDescriptorProto_values[52],
    &google_protobuf_FieldDescriptorProto_values[53],
    &google_protobuf_FieldDescriptorProto_values[54],
    &google_protobuf_FieldDescriptorProto_values[55],
    &google_protobuf_FieldDescriptorProto_values[56],
    &google_protobuf_FieldDescriptorProto_values[57],
    &google_protobuf_FieldDescriptorProto_values[58],
    &google_protobuf_FieldDescriptorProto_values[59],
    &google_protobuf_FieldDescriptorProto_values[60],
    &google_protobuf_FieldDescriptorProto_values[61],
    &google_protobuf_FieldDescriptorProto_values[62],
};
static UPB_MSG_ARRAY(google_protobuf_FieldDescriptorProto) google_protobuf_FieldDescriptorProto_arrays[18] = {
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[0], .len=1},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[1], .len=8},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[9], .len=7},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[16], .len=2},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[18], .len=8},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[26], .len=3},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[29], .len=3},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[32], .len=3},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[35], .len=4},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[39], .len=5},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[44], .len=2},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[46], .len=5},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[51], .len=1},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[52], .len=1},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[53], .len=1},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[54], .len=1},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[55], .len=6},
  {.elements = &google_protobuf_FieldDescriptorProto_array_elems[61], .len=2},
};
static google_protobuf_EnumValueDescriptorProto google_protobuf_EnumValueDescriptorProto_values[25];

static google_protobuf_EnumValueDescriptorProto *google_protobuf_EnumValueDescriptorProto_array_elems[] = {
    &google_protobuf_EnumValueDescriptorProto_values[0],
    &google_protobuf_EnumValueDescriptorProto_values[1],
    &google_protobuf_EnumValueDescriptorProto_values[2],
    &google_protobuf_EnumValueDescriptorProto_values[3],
    &google_protobuf_EnumValueDescriptorProto_values[4],
    &google_protobuf_EnumValueDescriptorProto_values[5],
    &google_protobuf_EnumValueDescriptorProto_values[6],
    &google_protobuf_EnumValueDescriptorProto_values[7],
    &google_protobuf_EnumValueDescriptorProto_values[8],
    &google_protobuf_EnumValueDescriptorProto_values[9],
    &google_protobuf_EnumValueDescriptorProto_values[10],
    &google_protobuf_EnumValueDescriptorProto_values[11],
    &google_protobuf_EnumValueDescriptorProto_values[12],
    &google_protobuf_EnumValueDescriptorProto_values[13],
    &google_protobuf_EnumValueDescriptorProto_values[14],
    &google_protobuf_EnumValueDescriptorProto_values[15],
    &google_protobuf_EnumValueDescriptorProto_values[16],
    &google_protobuf_EnumValueDescriptorProto_values[17],
    &google_protobuf_EnumValueDescriptorProto_values[18],
    &google_protobuf_EnumValueDescriptorProto_values[19],
    &google_protobuf_EnumValueDescriptorProto_values[20],
    &google_protobuf_EnumValueDescriptorProto_values[21],
    &google_protobuf_EnumValueDescriptorProto_values[22],
    &google_protobuf_EnumValueDescriptorProto_values[23],
    &google_protobuf_EnumValueDescriptorProto_values[24],
};
static UPB_MSG_ARRAY(google_protobuf_EnumValueDescriptorProto) google_protobuf_EnumValueDescriptorProto_arrays[4] = {
  {.elements = &google_protobuf_EnumValueDescriptorProto_array_elems[0], .len=18},
  {.elements = &google_protobuf_EnumValueDescriptorProto_array_elems[18], .len=3},
  {.elements = &google_protobuf_EnumValueDescriptorProto_array_elems[21], .len=2},
  {.elements = &google_protobuf_EnumValueDescriptorProto_array_elems[23], .len=2},
};
static google_protobuf_DescriptorProto google_protobuf_DescriptorProto_values[18] = {

  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[34],   /* "FileDescriptorSet" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[0],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[33],   /* "FileDescriptorProto" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[1],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = true,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[24],   /* "DescriptorProto" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[2],
    .nested_type = &google_protobuf_DescriptorProto_arrays[1],
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = true,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[31],   /* "FieldDescriptorProto" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[4],
    .nested_type = 0,   /* Not set. */
    .enum_type = &google_protobuf_EnumDescriptorProto_arrays[0],
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[26],   /* "EnumDescriptorProto" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[5],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[28],   /* "EnumValueDescriptorProto" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[6],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[47],   /* "ServiceDescriptorProto" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[7],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[41],   /* "MethodDescriptorProto" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[8],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = true,
    .extension_range = true,
    .extension = false,
    .options = false,
  }},
    .name = &strings[35],   /* "FileOptions" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[9],
    .nested_type = 0,   /* Not set. */
    .enum_type = &google_protobuf_EnumDescriptorProto_arrays[1],
    .extension_range = &google_protobuf_DescriptorProto_ExtensionRange_arrays[0],
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = true,
    .extension = false,
    .options = false,
  }},
    .name = &strings[40],   /* "MessageOptions" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[10],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = &google_protobuf_DescriptorProto_ExtensionRange_arrays[1],
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = true,
    .extension_range = true,
    .extension = false,
    .options = false,
  }},
    .name = &strings[32],   /* "FieldOptions" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[11],
    .nested_type = 0,   /* Not set. */
    .enum_type = &google_protobuf_EnumDescriptorProto_arrays[2],
    .extension_range = &google_protobuf_DescriptorProto_ExtensionRange_arrays[2],
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = true,
    .extension = false,
    .options = false,
  }},
    .name = &strings[27],   /* "EnumOptions" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[12],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = &google_protobuf_DescriptorProto_ExtensionRange_arrays[3],
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = true,
    .extension = false,
    .options = false,
  }},
    .name = &strings[29],   /* "EnumValueOptions" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[13],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = &google_protobuf_DescriptorProto_ExtensionRange_arrays[4],
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = true,
    .extension = false,
    .options = false,
  }},
    .name = &strings[48],   /* "ServiceOptions" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[14],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = &google_protobuf_DescriptorProto_ExtensionRange_arrays[5],
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = true,
    .extension = false,
    .options = false,
  }},
    .name = &strings[42],   /* "MethodOptions" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[15],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = &google_protobuf_DescriptorProto_ExtensionRange_arrays[6],
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = true,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[68],   /* "UninterpretedOption" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[16],
    .nested_type = &google_protobuf_DescriptorProto_arrays[2],
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[30],   /* "ExtensionRange" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[3],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .field = true,
    .nested_type = false,
    .enum_type = false,
    .extension_range = false,
    .extension = false,
    .options = false,
  }},
    .name = &strings[43],   /* "NamePart" */
    .field = &google_protobuf_FieldDescriptorProto_arrays[17],
    .nested_type = 0,   /* Not set. */
    .enum_type = 0,   /* Not set. */
    .extension_range = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
};
static google_protobuf_FileDescriptorProto google_protobuf_FileDescriptorProto_values[1] = {

  {.set_flags = {.has = {
    .name = true,
    .package = true,
    .dependency = false,
    .message_type = true,
    .enum_type = false,
    .service = false,
    .extension = false,
    .options = true,
  }},
    .name = &strings[74],   /* "descriptor/descriptor.proto" */
    .package = &strings[85],   /* "google.protobuf" */
    .dependency = 0,   /* Not set. */
    .message_type = &google_protobuf_DescriptorProto_arrays[0],
    .enum_type = 0,   /* Not set. */
    .service = 0,   /* Not set. */
    .extension = 0,   /* Not set. */
    .options = &google_protobuf_FileOptions_values[0],
  },
};
static google_protobuf_FileDescriptorSet google_protobuf_FileDescriptorSet_values[1] = {

  {.set_flags = {.has = {
    .file = true,
  }},
    .file = &google_protobuf_FileDescriptorProto_arrays[0],
  },
};
static google_protobuf_DescriptorProto_ExtensionRange google_protobuf_DescriptorProto_ExtensionRange_values[7] = {

  {.set_flags = {.has = {
    .start = true,
    .end = true,
  }},
    .start = 1000,
    .end = 536870912,
  },
  {.set_flags = {.has = {
    .start = true,
    .end = true,
  }},
    .start = 1000,
    .end = 536870912,
  },
  {.set_flags = {.has = {
    .start = true,
    .end = true,
  }},
    .start = 1000,
    .end = 536870912,
  },
  {.set_flags = {.has = {
    .start = true,
    .end = true,
  }},
    .start = 1000,
    .end = 536870912,
  },
  {.set_flags = {.has = {
    .start = true,
    .end = true,
  }},
    .start = 1000,
    .end = 536870912,
  },
  {.set_flags = {.has = {
    .start = true,
    .end = true,
  }},
    .start = 1000,
    .end = 536870912,
  },
  {.set_flags = {.has = {
    .start = true,
    .end = true,
  }},
    .start = 1000,
    .end = 536870912,
  },
};
static google_protobuf_FileOptions google_protobuf_FileOptions_values[1] = {

  {.set_flags = {.has = {
    .java_package = true,
    .java_outer_classname = true,
    .optimize_for = true,
    .java_multiple_files = false,
    .uninterpreted_option = false,
  }},
    .java_package = &strings[69],   /* "com.google.protobuf" */
    .java_outer_classname = &strings[25],   /* "DescriptorProtos" */
    .optimize_for = 1,
    .java_multiple_files = 0,   /* Not set. */
    .uninterpreted_option = 0,   /* Not set. */
  },
};
static google_protobuf_EnumDescriptorProto google_protobuf_EnumDescriptorProto_values[4] = {

  {.set_flags = {.has = {
    .name = true,
    .value = true,
    .options = false,
  }},
    .name = &strings[67],   /* "Type" */
    .value = &google_protobuf_EnumValueDescriptorProto_arrays[0],
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .value = true,
    .options = false,
  }},
    .name = &strings[39],   /* "Label" */
    .value = &google_protobuf_EnumValueDescriptorProto_arrays[1],
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .value = true,
    .options = false,
  }},
    .name = &strings[44],   /* "OptimizeMode" */
    .value = &google_protobuf_EnumValueDescriptorProto_arrays[2],
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .value = true,
    .options = false,
  }},
    .name = &strings[23],   /* "CType" */
    .value = &google_protobuf_EnumValueDescriptorProto_arrays[3],
    .options = 0,   /* Not set. */
  },
};
static google_protobuf_FieldDescriptorProto google_protobuf_FieldDescriptorProto_values[63] = {

  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[84],   /* "file" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 3,
    .type = 11,
    .type_name = &strings[11],   /* ".google.protobuf.FileDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[96],   /* "name" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[104],   /* "package" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[72],   /* "dependency" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 3,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[94],   /* "message_type" */
    .extendee = 0,   /* Not set. */
    .number = 4,
    .label = 3,
    .type = 11,
    .type_name = &strings[0],   /* ".google.protobuf.DescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[77],   /* "enum_type" */
    .extendee = 0,   /* Not set. */
    .number = 5,
    .label = 3,
    .type = 11,
    .type_name = &strings[2],   /* ".google.protobuf.EnumDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[107],   /* "service" */
    .extendee = 0,   /* Not set. */
    .number = 6,
    .label = 3,
    .type = 11,
    .type_name = &strings[17],   /* ".google.protobuf.ServiceDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[80],   /* "extension" */
    .extendee = 0,   /* Not set. */
    .number = 7,
    .label = 3,
    .type = 11,
    .type_name = &strings[6],   /* ".google.protobuf.FieldDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[102],   /* "options" */
    .extendee = 0,   /* Not set. */
    .number = 8,
    .label = 1,
    .type = 11,
    .type_name = &strings[12],   /* ".google.protobuf.FileOptions" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[96],   /* "name" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[83],   /* "field" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 3,
    .type = 11,
    .type_name = &strings[6],   /* ".google.protobuf.FieldDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[99],   /* "nested_type" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 3,
    .type = 11,
    .type_name = &strings[0],   /* ".google.protobuf.DescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[77],   /* "enum_type" */
    .extendee = 0,   /* Not set. */
    .number = 4,
    .label = 3,
    .type = 11,
    .type_name = &strings[2],   /* ".google.protobuf.EnumDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[81],   /* "extension_range" */
    .extendee = 0,   /* Not set. */
    .number = 5,
    .label = 3,
    .type = 11,
    .type_name = &strings[1],   /* ".google.protobuf.DescriptorProto.ExtensionRange" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[80],   /* "extension" */
    .extendee = 0,   /* Not set. */
    .number = 6,
    .label = 3,
    .type = 11,
    .type_name = &strings[6],   /* ".google.protobuf.FieldDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[102],   /* "options" */
    .extendee = 0,   /* Not set. */
    .number = 7,
    .label = 1,
    .type = 11,
    .type_name = &strings[14],   /* ".google.protobuf.MessageOptions" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[108],   /* "start" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 5,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[76],   /* "end" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 1,
    .type = 5,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[96],   /* "name" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[79],   /* "extendee" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[100],   /* "number" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 1,
    .type = 5,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[92],   /* "label" */
    .extendee = 0,   /* Not set. */
    .number = 4,
    .label = 1,
    .type = 14,
    .type_name = &strings[7],   /* ".google.protobuf.FieldDescriptorProto.Label" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[110],   /* "type" */
    .extendee = 0,   /* Not set. */
    .number = 5,
    .label = 1,
    .type = 14,
    .type_name = &strings[8],   /* ".google.protobuf.FieldDescriptorProto.Type" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[111],   /* "type_name" */
    .extendee = 0,   /* Not set. */
    .number = 6,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[71],   /* "default_value" */
    .extendee = 0,   /* Not set. */
    .number = 7,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[102],   /* "options" */
    .extendee = 0,   /* Not set. */
    .number = 8,
    .label = 1,
    .type = 11,
    .type_name = &strings[9],   /* ".google.protobuf.FieldOptions" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[96],   /* "name" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[113],   /* "value" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 3,
    .type = 11,
    .type_name = &strings[4],   /* ".google.protobuf.EnumValueDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[102],   /* "options" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 1,
    .type = 11,
    .type_name = &strings[3],   /* ".google.protobuf.EnumOptions" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[96],   /* "name" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[100],   /* "number" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 1,
    .type = 5,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[102],   /* "options" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 1,
    .type = 11,
    .type_name = &strings[5],   /* ".google.protobuf.EnumValueOptions" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[96],   /* "name" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[95],   /* "method" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 3,
    .type = 11,
    .type_name = &strings[15],   /* ".google.protobuf.MethodDescriptorProto" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[102],   /* "options" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 1,
    .type = 11,
    .type_name = &strings[18],   /* ".google.protobuf.ServiceOptions" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[96],   /* "name" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[87],   /* "input_type" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[103],   /* "output_type" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[102],   /* "options" */
    .extendee = 0,   /* Not set. */
    .number = 4,
    .label = 1,
    .type = 11,
    .type_name = &strings[16],   /* ".google.protobuf.MethodOptions" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[91],   /* "java_package" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[90],   /* "java_outer_classname" */
    .extendee = 0,   /* Not set. */
    .number = 8,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = true,
    .options = false,
  }},
    .name = &strings[101],   /* "optimize_for" */
    .extendee = 0,   /* Not set. */
    .number = 9,
    .label = 1,
    .type = 14,
    .type_name = &strings[13],   /* ".google.protobuf.FileOptions.OptimizeMode" */
    .default_value = &strings[45],   /* "SPEED" */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = true,
    .options = false,
  }},
    .name = &strings[89],   /* "java_multiple_files" */
    .extendee = 0,   /* Not set. */
    .number = 10,
    .label = 1,
    .type = 8,
    .type_name = 0,   /* Not set. */
    .default_value = &strings[82],   /* "false" */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[112],   /* "uninterpreted_option" */
    .extendee = 0,   /* Not set. */
    .number = 999,
    .label = 3,
    .type = 11,
    .type_name = &strings[19],   /* ".google.protobuf.UninterpretedOption" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = true,
    .options = false,
  }},
    .name = &strings[93],   /* "message_set_wire_format" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 8,
    .type_name = 0,   /* Not set. */
    .default_value = &strings[82],   /* "false" */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[112],   /* "uninterpreted_option" */
    .extendee = 0,   /* Not set. */
    .number = 999,
    .label = 3,
    .type = 11,
    .type_name = &strings[19],   /* ".google.protobuf.UninterpretedOption" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[70],   /* "ctype" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 1,
    .type = 14,
    .type_name = &strings[10],   /* ".google.protobuf.FieldOptions.CType" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[105],   /* "packed" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 1,
    .type = 8,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = true,
    .options = false,
  }},
    .name = &strings[73],   /* "deprecated" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 1,
    .type = 8,
    .type_name = 0,   /* Not set. */
    .default_value = &strings[82],   /* "false" */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[78],   /* "experimental_map_key" */
    .extendee = 0,   /* Not set. */
    .number = 9,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[112],   /* "uninterpreted_option" */
    .extendee = 0,   /* Not set. */
    .number = 999,
    .label = 3,
    .type = 11,
    .type_name = &strings[19],   /* ".google.protobuf.UninterpretedOption" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[112],   /* "uninterpreted_option" */
    .extendee = 0,   /* Not set. */
    .number = 999,
    .label = 3,
    .type = 11,
    .type_name = &strings[19],   /* ".google.protobuf.UninterpretedOption" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[112],   /* "uninterpreted_option" */
    .extendee = 0,   /* Not set. */
    .number = 999,
    .label = 3,
    .type = 11,
    .type_name = &strings[19],   /* ".google.protobuf.UninterpretedOption" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[112],   /* "uninterpreted_option" */
    .extendee = 0,   /* Not set. */
    .number = 999,
    .label = 3,
    .type = 11,
    .type_name = &strings[19],   /* ".google.protobuf.UninterpretedOption" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[112],   /* "uninterpreted_option" */
    .extendee = 0,   /* Not set. */
    .number = 999,
    .label = 3,
    .type = 11,
    .type_name = &strings[19],   /* ".google.protobuf.UninterpretedOption" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = true,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[96],   /* "name" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 3,
    .type = 11,
    .type_name = &strings[20],   /* ".google.protobuf.UninterpretedOption.NamePart" */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[86],   /* "identifier_value" */
    .extendee = 0,   /* Not set. */
    .number = 3,
    .label = 1,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[106],   /* "positive_int_value" */
    .extendee = 0,   /* Not set. */
    .number = 4,
    .label = 1,
    .type = 4,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[98],   /* "negative_int_value" */
    .extendee = 0,   /* Not set. */
    .number = 5,
    .label = 1,
    .type = 3,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[75],   /* "double_value" */
    .extendee = 0,   /* Not set. */
    .number = 6,
    .label = 1,
    .type = 1,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[109],   /* "string_value" */
    .extendee = 0,   /* Not set. */
    .number = 7,
    .label = 1,
    .type = 12,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[97],   /* "name_part" */
    .extendee = 0,   /* Not set. */
    .number = 1,
    .label = 2,
    .type = 9,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .extendee = false,
    .number = true,
    .label = true,
    .type = true,
    .type_name = false,
    .default_value = false,
    .options = false,
  }},
    .name = &strings[88],   /* "is_extension" */
    .extendee = 0,   /* Not set. */
    .number = 2,
    .label = 2,
    .type = 8,
    .type_name = 0,   /* Not set. */
    .default_value = 0,   /* Not set. */
    .options = 0,   /* Not set. */
  },
};
static google_protobuf_EnumValueDescriptorProto google_protobuf_EnumValueDescriptorProto_values[25] = {

  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[51],   /* "TYPE_DOUBLE" */
    .number = 1,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[55],   /* "TYPE_FLOAT" */
    .number = 2,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[58],   /* "TYPE_INT64" */
    .number = 3,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[66],   /* "TYPE_UINT64" */
    .number = 4,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[57],   /* "TYPE_INT32" */
    .number = 5,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[54],   /* "TYPE_FIXED64" */
    .number = 6,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[53],   /* "TYPE_FIXED32" */
    .number = 7,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[49],   /* "TYPE_BOOL" */
    .number = 8,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[64],   /* "TYPE_STRING" */
    .number = 9,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[56],   /* "TYPE_GROUP" */
    .number = 10,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[59],   /* "TYPE_MESSAGE" */
    .number = 11,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[50],   /* "TYPE_BYTES" */
    .number = 12,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[65],   /* "TYPE_UINT32" */
    .number = 13,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[52],   /* "TYPE_ENUM" */
    .number = 14,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[60],   /* "TYPE_SFIXED32" */
    .number = 15,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[61],   /* "TYPE_SFIXED64" */
    .number = 16,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[62],   /* "TYPE_SINT32" */
    .number = 17,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[63],   /* "TYPE_SINT64" */
    .number = 18,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[36],   /* "LABEL_OPTIONAL" */
    .number = 1,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[38],   /* "LABEL_REQUIRED" */
    .number = 2,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[37],   /* "LABEL_REPEATED" */
    .number = 3,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[45],   /* "SPEED" */
    .number = 1,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[21],   /* "CODE_SIZE" */
    .number = 2,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[22],   /* "CORD" */
    .number = 1,
    .options = 0,   /* Not set. */
  },
  {.set_flags = {.has = {
    .name = true,
    .number = true,
    .options = false,
  }},
    .name = &strings[46],   /* "STRING_PIECE" */
    .number = 2,
    .options = 0,   /* Not set. */
  },
};
/* The externally-visible definition. */
google_protobuf_FileDescriptorSet *upb_file_descriptor_set = &google_protobuf_FileDescriptorSet_values[0];
