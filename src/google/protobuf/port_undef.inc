// -*- c++ -*-
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// #undefs all macros defined in port_def.inc.  See comments in port_def.inc
// for more info.

#ifndef PROTOBUF_PORT_
#error "port_undef.inc must be included after port_def.inc"
#endif
#undef PROTOBUF_PORT_

// Restore macros that may have been #undef'd in port_def.inc.

#ifdef PROTOBUF_DID_UNDEF_noreturn
#pragma pop_macro("noreturn")
#undef PROTOBUF_DID_UNDEF_noreturn
#endif

#ifdef PROTOBUF_DID_UNDEF_PACKAGE
#pragma pop_macro("PACKAGE")
#undef PROTOBUF_DID_UNDEF_PACKAGE
#endif

#ifdef PROTOBUF_DID_UNDEF_PACKED
#pragma pop_macro("PACKED")
#undef PROTOBUF_DID_UNDEF_PACKED
#endif

#ifdef PROTOBUF_DID_UNDEF_DOMAIN
#pragma pop_macro("DOMAIN")
#undef PROTOBUF_DID_UNDEF_DOMAIN
#endif

#ifdef PROTOBUF_DID_UNDEF_LINUX
#pragma pop_macro("linux")
#endif

#ifdef _WIN32
#pragma pop_macro("CompareString")
#pragma pop_macro("CREATE_NEW")
#pragma pop_macro("DELETE")
#pragma pop_macro("DOUBLE_CLICK")
#pragma pop_macro("ERROR")
#pragma pop_macro("ERROR_BUSY")
#pragma pop_macro("ERROR_INSTALL_FAILED")
#pragma pop_macro("ERROR_NOT_FOUND")
#pragma pop_macro("GetClassName")
#pragma pop_macro("GetCurrentTime")
#pragma pop_macro("GetMessage")
#pragma pop_macro("GetObject")
#pragma pop_macro("IGNORE")
#pragma pop_macro("IN")
#pragma pop_macro("INPUT_KEYBOARD")
#pragma pop_macro("OUT")
#pragma pop_macro("OPTIONAL")
#pragma pop_macro("min")
#pragma pop_macro("max")
#pragma pop_macro("NEAR")
#pragma pop_macro("NO_DATA")
#pragma pop_macro("NO_ERROR")
#pragma pop_macro("REASON_UNKNOWN")
#pragma pop_macro("SERVICE_DISABLED")
#pragma pop_macro("SERVICE_STOP")
#pragma pop_macro("SEVERITY_ERROR")
#pragma pop_macro("STRICT")
#pragma pop_macro("STATUS_PENDING")
#pragma pop_macro("timezone")
#pragma pop_macro("TRUE")
#pragma pop_macro("FALSE")
#endif

#ifdef __APPLE__
#pragma pop_macro("TRUE")
#pragma pop_macro("FALSE")
#pragma pop_macro("UID_MAX")
#pragma pop_macro("GID_MAX")
#pragma pop_macro("TYPE_BOOL")
#endif  // __APPLE__

#ifdef __FreeBSD__
#pragma pop_macro("TRUE")
#pragma pop_macro("FALSE")
#pragma pop_macro("UID_MAX")
#pragma pop_macro("GID_MAX")
#endif  // __FreeBSD__

#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
#pragma pop_macro("DEBUG")
#endif // defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// Pop the warning(push) from port_def.inc
#ifdef _MSC_VER
#pragma warning(pop)
#endif
