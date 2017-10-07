package protobuf_unittest;

option java_package = "com.google.protobuf";
option java_outer_classname = "EnumValidity";

enum E {
  default = 1; // test java keyword renaming
  FOO = 2;
  BAR = 3;
  BAZ = 4;
}

message M {
  optional E optional_e = 1;
  optional E default_e = 2 [ default = BAZ ];
  repeated E repeated_e = 3;
  repeated E packed_e = 4 [ packed = true ];
  repeated E repeated_e2 = 5;
  repeated E packed_e2 = 6 [ packed = true ];
  repeated E repeated_e3 = 7;
  repeated E packed_e3 = 8 [ packed = true ];
}

message Alt {
  optional E repeated_e2_as_optional = 5;
  repeated E packed_e2_as_non_packed = 6;
  repeated E non_packed_e3_as_packed = 7 [ packed = true ];
}
