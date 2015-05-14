namespace Google.ProtocolBuffers
{
    internal class TestResources
    {
        #region golden_message
        internal static byte[] golden_message
        {
            get
            {
                return System.Convert.FromBase64String(@"
CGUQZhhnIGgo0gEw1AE9awAAAEFsAAAAAAAAAE1tAAAAUW4AAAAAAAAAXQAA3kJhAAAAAAAAXEBo
AXIDMTE1egMxMTaDAYgBdYQBkgECCHaaAQIId6IBAgh4qAEDsAEGuAEJwgEDMTI0ygEDMTI1+AHJ
AfgBrQKAAsoBgAKuAogCywGIAq8CkALMAZACsAKYApoDmALiBKACnAOgAuQErQLPAAAArQIzAQAA
sQLQAAAAAAAAALECNAEAAAAAAAC9AtEAAAC9AjUBAADBAtIAAAAAAAAAwQI2AQAAAAAAAM0CAABT
Q80CAICbQ9ECAAAAAACAakDRAgAAAAAAgHNA2AIB2AIA4gIDMjE14gIDMzE16gIDMjE26gIDMzE2
8wL4AtkB9ALzAvgCvQL0AoIDAwjaAYIDAwi+AooDAwjbAYoDAwi/ApIDAwjcAZIDAwjAApgDApgD
A6ADBaADBqgDCKgDCbIDAzIyNLIDAzMyNLoDAzIyNboDAzMyNegDkQPwA5ID+AOTA4AElAOIBKoG
kASsBp0ElwEAAKEEmAEAAAAAAACtBJkBAACxBJoBAAAAAAAAvQQAgM1DwQQAAAAAAMB5QMgEANIE
AzQxNdoEAzQxNogFAZAFBJgFB6IFAzQyNKoFAzQyNQ==
");
            }
        }
        #endregion

        #region golden_packed_fields_message
        internal static byte[] golden_packed_fields_message
        {
            get
            {
                return System.Convert.FromBase64String(@"
0gUE2QS9BdoFBNoEvgXiBQTbBL8F6gUE3ATABfIFBLoJggv6BQS8CYQLggYIXwIAAMMCAACKBhBg
AgAAAAAAAMQCAAAAAAAAkgYIYQIAAMUCAACaBhBiAgAAAAAAAMYCAAAAAAAAogYIAMAYRADAMUSq
BhAAAAAAACCDQAAAAAAAQIZAsgYCAQC6BgIFBg==
");
            }
        }
        #endregion

        #region text_format_unittest_data
        internal static string text_format_unittest_data
        {
            get
            {
                return @"
optional_int32: 101
optional_int64: 102
optional_uint32: 103
optional_uint64: 104
optional_sint32: 105
optional_sint64: 106
optional_fixed32: 107
optional_fixed64: 108
optional_sfixed32: 109
optional_sfixed64: 110
optional_float: 111
optional_double: 112
optional_bool: true
optional_string: ""115""
optional_bytes: ""116""
OptionalGroup {
  a: 117
}
optional_nested_message {
  bb: 118
}
optional_foreign_message {
  c: 119
}
optional_import_message {
  d: 120
}
optional_nested_enum: BAZ
optional_foreign_enum: FOREIGN_BAZ
optional_import_enum: IMPORT_BAZ
optional_string_piece: ""124""
optional_cord: ""125""
repeated_int32: 201
repeated_int32: 301
repeated_int64: 202
repeated_int64: 302
repeated_uint32: 203
repeated_uint32: 303
repeated_uint64: 204
repeated_uint64: 304
repeated_sint32: 205
repeated_sint32: 305
repeated_sint64: 206
repeated_sint64: 306
repeated_fixed32: 207
repeated_fixed32: 307
repeated_fixed64: 208
repeated_fixed64: 308
repeated_sfixed32: 209
repeated_sfixed32: 309
repeated_sfixed64: 210
repeated_sfixed64: 310
repeated_float: 211
repeated_float: 311
repeated_double: 212
repeated_double: 312
repeated_bool: true
repeated_bool: false
repeated_string: ""215""
repeated_string: ""315""
repeated_bytes: ""216""
repeated_bytes: ""316""
RepeatedGroup {
  a: 217
}
RepeatedGroup {
  a: 317
}
repeated_nested_message {
  bb: 218
}
repeated_nested_message {
  bb: 318
}
repeated_foreign_message {
  c: 219
}
repeated_foreign_message {
  c: 319
}
repeated_import_message {
  d: 220
}
repeated_import_message {
  d: 320
}
repeated_nested_enum: BAR
repeated_nested_enum: BAZ
repeated_foreign_enum: FOREIGN_BAR
repeated_foreign_enum: FOREIGN_BAZ
repeated_import_enum: IMPORT_BAR
repeated_import_enum: IMPORT_BAZ
repeated_string_piece: ""224""
repeated_string_piece: ""324""
repeated_cord: ""225""
repeated_cord: ""325""
default_int32: 401
default_int64: 402
default_uint32: 403
default_uint64: 404
default_sint32: 405
default_sint64: 406
default_fixed32: 407
default_fixed64: 408
default_sfixed32: 409
default_sfixed64: 410
default_float: 411
default_double: 412
default_bool: false
default_string: ""415""
default_bytes: ""416""
default_nested_enum: FOO
default_foreign_enum: FOREIGN_FOO
default_import_enum: IMPORT_FOO
default_string_piece: ""424""
default_cord: ""425""

";
            }
        }
        #endregion

        #region text_format_unittest_extensions_data
        internal static string text_format_unittest_extensions_data
        {
            get
            {
                return @"
[protobuf_unittest.optional_int32_extension]: 101
[protobuf_unittest.optional_int64_extension]: 102
[protobuf_unittest.optional_uint32_extension]: 103
[protobuf_unittest.optional_uint64_extension]: 104
[protobuf_unittest.optional_sint32_extension]: 105
[protobuf_unittest.optional_sint64_extension]: 106
[protobuf_unittest.optional_fixed32_extension]: 107
[protobuf_unittest.optional_fixed64_extension]: 108
[protobuf_unittest.optional_sfixed32_extension]: 109
[protobuf_unittest.optional_sfixed64_extension]: 110
[protobuf_unittest.optional_float_extension]: 111
[protobuf_unittest.optional_double_extension]: 112
[protobuf_unittest.optional_bool_extension]: true
[protobuf_unittest.optional_string_extension]: ""115""
[protobuf_unittest.optional_bytes_extension]: ""116""
[protobuf_unittest.optionalgroup_extension] {
  a: 117
}
[protobuf_unittest.optional_nested_message_extension] {
  bb: 118
}
[protobuf_unittest.optional_foreign_message_extension] {
  c: 119
}
[protobuf_unittest.optional_import_message_extension] {
  d: 120
}
[protobuf_unittest.optional_nested_enum_extension]: BAZ
[protobuf_unittest.optional_foreign_enum_extension]: FOREIGN_BAZ
[protobuf_unittest.optional_import_enum_extension]: IMPORT_BAZ
[protobuf_unittest.optional_string_piece_extension]: ""124""
[protobuf_unittest.optional_cord_extension]: ""125""
[protobuf_unittest.repeated_int32_extension]: 201
[protobuf_unittest.repeated_int32_extension]: 301
[protobuf_unittest.repeated_int64_extension]: 202
[protobuf_unittest.repeated_int64_extension]: 302
[protobuf_unittest.repeated_uint32_extension]: 203
[protobuf_unittest.repeated_uint32_extension]: 303
[protobuf_unittest.repeated_uint64_extension]: 204
[protobuf_unittest.repeated_uint64_extension]: 304
[protobuf_unittest.repeated_sint32_extension]: 205
[protobuf_unittest.repeated_sint32_extension]: 305
[protobuf_unittest.repeated_sint64_extension]: 206
[protobuf_unittest.repeated_sint64_extension]: 306
[protobuf_unittest.repeated_fixed32_extension]: 207
[protobuf_unittest.repeated_fixed32_extension]: 307
[protobuf_unittest.repeated_fixed64_extension]: 208
[protobuf_unittest.repeated_fixed64_extension]: 308
[protobuf_unittest.repeated_sfixed32_extension]: 209
[protobuf_unittest.repeated_sfixed32_extension]: 309
[protobuf_unittest.repeated_sfixed64_extension]: 210
[protobuf_unittest.repeated_sfixed64_extension]: 310
[protobuf_unittest.repeated_float_extension]: 211
[protobuf_unittest.repeated_float_extension]: 311
[protobuf_unittest.repeated_double_extension]: 212
[protobuf_unittest.repeated_double_extension]: 312
[protobuf_unittest.repeated_bool_extension]: true
[protobuf_unittest.repeated_bool_extension]: false
[protobuf_unittest.repeated_string_extension]: ""215""
[protobuf_unittest.repeated_string_extension]: ""315""
[protobuf_unittest.repeated_bytes_extension]: ""216""
[protobuf_unittest.repeated_bytes_extension]: ""316""
[protobuf_unittest.repeatedgroup_extension] {
  a: 217
}
[protobuf_unittest.repeatedgroup_extension] {
  a: 317
}
[protobuf_unittest.repeated_nested_message_extension] {
  bb: 218
}
[protobuf_unittest.repeated_nested_message_extension] {
  bb: 318
}
[protobuf_unittest.repeated_foreign_message_extension] {
  c: 219
}
[protobuf_unittest.repeated_foreign_message_extension] {
  c: 319
}
[protobuf_unittest.repeated_import_message_extension] {
  d: 220
}
[protobuf_unittest.repeated_import_message_extension] {
  d: 320
}
[protobuf_unittest.repeated_nested_enum_extension]: BAR
[protobuf_unittest.repeated_nested_enum_extension]: BAZ
[protobuf_unittest.repeated_foreign_enum_extension]: FOREIGN_BAR
[protobuf_unittest.repeated_foreign_enum_extension]: FOREIGN_BAZ
[protobuf_unittest.repeated_import_enum_extension]: IMPORT_BAR
[protobuf_unittest.repeated_import_enum_extension]: IMPORT_BAZ
[protobuf_unittest.repeated_string_piece_extension]: ""224""
[protobuf_unittest.repeated_string_piece_extension]: ""324""
[protobuf_unittest.repeated_cord_extension]: ""225""
[protobuf_unittest.repeated_cord_extension]: ""325""
[protobuf_unittest.default_int32_extension]: 401
[protobuf_unittest.default_int64_extension]: 402
[protobuf_unittest.default_uint32_extension]: 403
[protobuf_unittest.default_uint64_extension]: 404
[protobuf_unittest.default_sint32_extension]: 405
[protobuf_unittest.default_sint64_extension]: 406
[protobuf_unittest.default_fixed32_extension]: 407
[protobuf_unittest.default_fixed64_extension]: 408
[protobuf_unittest.default_sfixed32_extension]: 409
[protobuf_unittest.default_sfixed64_extension]: 410
[protobuf_unittest.default_float_extension]: 411
[protobuf_unittest.default_double_extension]: 412
[protobuf_unittest.default_bool_extension]: false
[protobuf_unittest.default_string_extension]: ""415""
[protobuf_unittest.default_bytes_extension]: ""416""
[protobuf_unittest.default_nested_enum_extension]: FOO
[protobuf_unittest.default_foreign_enum_extension]: FOREIGN_FOO
[protobuf_unittest.default_import_enum_extension]: IMPORT_FOO
[protobuf_unittest.default_string_piece_extension]: ""424""
[protobuf_unittest.default_cord_extension]: ""425""
";
            }
        }
        #endregion
    }
}
