package main

import "strings"

// wktKind returns a non-empty kind for well-known types we special-case.
func wktKind(fullName string) string {
	switch fullName {
	case "google.protobuf.Value":
		return "value"
	case "google.protobuf.ListValue":
		return "listvalue"
	case "google.protobuf.Struct":
		return "struct"
	case "google.protobuf.BoolValue":
		return "wrapper_bool"
	case "google.protobuf.Int32Value":
		return "wrapper_int32"
	case "google.protobuf.Int64Value":
		return "wrapper_int64"
	case "google.protobuf.UInt32Value":
		return "wrapper_uint32"
	case "google.protobuf.UInt64Value":
		return "wrapper_uint64"
	case "google.protobuf.FloatValue":
		return "wrapper_float"
	case "google.protobuf.DoubleValue":
		return "wrapper_double"
	case "google.protobuf.StringValue":
		return "wrapper_string"
	case "google.protobuf.BytesValue":
		return "wrapper_bytes"
	case "google.protobuf.FieldMask":
		return "fieldmask"
	case "google.protobuf.Empty":
		return "empty"
	case "google.protobuf.Timestamp":
		return "timestamp"
	case "google.protobuf.Duration":
		return "duration"
	case "google.protobuf.Any":
		return "any"
	}
	return ""
}

func isWKT(fullName string) bool {
	return wktKind(fullName) != ""
}

// wktFuncName returns the decode function name for a well-known type.
func wktFuncName(fullName string) string {
	// google.protobuf.Value -> decode_google_protobuf_Value
	name := strings.ReplaceAll(fullName, ".", "_")
	return "decode_" + strings.ToLower(sanitizeIdent(name))
}
