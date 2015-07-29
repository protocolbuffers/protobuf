package protobuf_unittest;

option java_package = "com.google.protobuf";
option java_outer_classname = "NanoReferenceTypes";

message TestAllTypesNano {

  enum NestedEnum {
    FOO = 1;
    BAR = 2;
    BAZ = 3;
  }

  message NestedMessage {
    optional int32 foo = 1;
  }

  // Singular
  optional    int32 optional_int32    =  1;
  optional    int64 optional_int64    =  2;
  optional   uint32 optional_uint32   =  3;
  optional   uint64 optional_uint64   =  4;
  optional   sint32 optional_sint32   =  5;
  optional   sint64 optional_sint64   =  6;
  optional  fixed32 optional_fixed32  =  7;
  optional  fixed64 optional_fixed64  =  8;
  optional sfixed32 optional_sfixed32 =  9;
  optional sfixed64 optional_sfixed64 = 10;
  optional    float optional_float    = 11;
  optional   double optional_double   = 12;
  optional     bool optional_bool     = 13;
  optional   string optional_string   = 14;
  optional    bytes optional_bytes    = 15;

  optional group OptionalGroup = 16 {
    optional int32 a = 17;
  }

  optional NestedMessage      optional_nested_message  = 18;

  optional NestedEnum      optional_nested_enum     = 21;

  optional string optional_string_piece = 24 [ctype=STRING_PIECE];
  optional string optional_cord = 25 [ctype=CORD];

  // Repeated
  repeated    int32 repeated_int32    = 31;
  repeated    int64 repeated_int64    = 32;
  repeated   uint32 repeated_uint32   = 33;
  repeated   uint64 repeated_uint64   = 34;
  repeated   sint32 repeated_sint32   = 35;
  repeated   sint64 repeated_sint64   = 36;
  repeated  fixed32 repeated_fixed32  = 37;
  repeated  fixed64 repeated_fixed64  = 38;
  repeated sfixed32 repeated_sfixed32 = 39;
  repeated sfixed64 repeated_sfixed64 = 40;
  repeated    float repeated_float    = 41;
  repeated   double repeated_double   = 42;
  repeated     bool repeated_bool     = 43;
  repeated   string repeated_string   = 44;
  repeated    bytes repeated_bytes    = 45;

  repeated group RepeatedGroup = 46 {
    optional int32 a = 47;
  }

  repeated NestedMessage      repeated_nested_message  = 48;

  repeated NestedEnum      repeated_nested_enum  = 51;

  repeated string repeated_string_piece = 54 [ctype=STRING_PIECE];
  repeated string repeated_cord = 55 [ctype=CORD];

  // Repeated packed
  repeated    int32 repeated_packed_int32    = 87 [packed=true];
  repeated sfixed64 repeated_packed_sfixed64 = 88 [packed=true];

  repeated NestedEnum repeated_packed_nested_enum  = 89 [packed=true];

  // Singular with defaults
  optional    int32 default_int32    = 61 [default =  41    ];
  optional    int64 default_int64    = 62 [default =  42    ];
  optional   uint32 default_uint32   = 63 [default =  43    ];
  optional   uint64 default_uint64   = 64 [default =  44    ];
  optional   sint32 default_sint32   = 65 [default = -45    ];
  optional   sint64 default_sint64   = 66 [default =  46    ];
  optional  fixed32 default_fixed32  = 67 [default =  47    ];
  optional  fixed64 default_fixed64  = 68 [default =  48    ];
  optional sfixed32 default_sfixed32 = 69 [default =  49    ];
  optional sfixed64 default_sfixed64 = 70 [default = -50    ];
  optional    float default_float    = 71 [default =  51.5  ];
  optional   double default_double   = 72 [default =  52e3  ];
  optional     bool default_bool     = 73 [default = true   ];
  optional   string default_string   = 74 [default = "hello"];
  optional    bytes default_bytes    = 75 [default = "world"];


  optional  float default_float_inf      = 97  [default =  inf];
  optional  float default_float_neg_inf  = 98  [default = -inf];
  optional  float default_float_nan      = 99  [default =  nan];
  optional double default_double_inf     = 100 [default =  inf];
  optional double default_double_neg_inf = 101 [default = -inf];
  optional double default_double_nan     = 102 [default =  nan];

}

message ForeignMessageNano {
  optional int32 c = 1;
}

enum ForeignEnumNano {
  FOREIGN_NANO_FOO = 4;
  FOREIGN_NANO_BAR = 5;
  FOREIGN_NANO_BAZ = 6;
}

