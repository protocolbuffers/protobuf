// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import java.lang.reflect.Field;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.util.List;

/**
 * Enumeration identifying all relevant type information for a protobuf field.
 */
@ExperimentalApi
public enum FieldType {
  DOUBLE(0, Collection.SCALAR, JavaType.DOUBLE, JavaType.VOID),
  FLOAT(1, Collection.SCALAR, JavaType.FLOAT, JavaType.VOID),
  INT64(2, Collection.SCALAR, JavaType.LONG, JavaType.VOID),
  UINT64(3, Collection.SCALAR, JavaType.LONG, JavaType.VOID),
  INT32(4, Collection.SCALAR, JavaType.INT, JavaType.VOID),
  FIXED64(5, Collection.SCALAR, JavaType.LONG, JavaType.VOID),
  FIXED32(6, Collection.SCALAR, JavaType.INT, JavaType.VOID),
  BOOL(7, Collection.SCALAR, JavaType.BOOLEAN, JavaType.VOID),
  STRING(8, Collection.SCALAR, JavaType.STRING, JavaType.VOID),
  MESSAGE(9, Collection.SCALAR, JavaType.MESSAGE, JavaType.VOID),
  BYTES(10, Collection.SCALAR, JavaType.BYTE_STRING, JavaType.VOID),
  UINT32(11, Collection.SCALAR, JavaType.INT, JavaType.VOID),
  ENUM(12, Collection.SCALAR, JavaType.ENUM, JavaType.VOID),
  SFIXED32(13, Collection.SCALAR, JavaType.INT, JavaType.VOID),
  SFIXED64(14, Collection.SCALAR, JavaType.LONG, JavaType.VOID),
  SINT32(15, Collection.SCALAR, JavaType.INT, JavaType.VOID),
  SINT64(16, Collection.SCALAR, JavaType.LONG, JavaType.VOID),
  DOUBLE_LIST(17, Collection.VECTOR, JavaType.DOUBLE, JavaType.VOID),
  FLOAT_LIST(18, Collection.VECTOR, JavaType.FLOAT, JavaType.VOID),
  INT64_LIST(19, Collection.VECTOR, JavaType.LONG, JavaType.VOID),
  UINT64_LIST(20, Collection.VECTOR, JavaType.LONG, JavaType.VOID),
  INT32_LIST(21, Collection.VECTOR, JavaType.INT, JavaType.VOID),
  FIXED64_LIST(22, Collection.VECTOR, JavaType.LONG, JavaType.VOID),
  FIXED32_LIST(23, Collection.VECTOR, JavaType.INT, JavaType.VOID),
  BOOL_LIST(24, Collection.VECTOR, JavaType.BOOLEAN, JavaType.VOID),
  STRING_LIST(25, Collection.VECTOR, JavaType.STRING, JavaType.VOID),
  MESSAGE_LIST(26, Collection.VECTOR, JavaType.MESSAGE, JavaType.VOID),
  BYTES_LIST(27, Collection.VECTOR, JavaType.BYTE_STRING, JavaType.VOID),
  UINT32_LIST(28, Collection.VECTOR, JavaType.INT, JavaType.VOID),
  ENUM_LIST(29, Collection.VECTOR, JavaType.ENUM, JavaType.VOID),
  SFIXED32_LIST(30, Collection.VECTOR, JavaType.INT, JavaType.VOID),
  SFIXED64_LIST(31, Collection.VECTOR, JavaType.LONG, JavaType.VOID),
  SINT32_LIST(32, Collection.VECTOR, JavaType.INT, JavaType.VOID),
  SINT64_LIST(33, Collection.VECTOR, JavaType.LONG, JavaType.VOID),
  DOUBLE_LIST_PACKED(34, Collection.PACKED_VECTOR, JavaType.DOUBLE, JavaType.VOID),
  FLOAT_LIST_PACKED(35, Collection.PACKED_VECTOR, JavaType.FLOAT, JavaType.VOID),
  INT64_LIST_PACKED(36, Collection.PACKED_VECTOR, JavaType.LONG, JavaType.VOID),
  UINT64_LIST_PACKED(37, Collection.PACKED_VECTOR, JavaType.LONG, JavaType.VOID),
  INT32_LIST_PACKED(38, Collection.PACKED_VECTOR, JavaType.INT, JavaType.VOID),
  FIXED64_LIST_PACKED(39, Collection.PACKED_VECTOR, JavaType.LONG, JavaType.VOID),
  FIXED32_LIST_PACKED(40, Collection.PACKED_VECTOR, JavaType.INT, JavaType.VOID),
  BOOL_LIST_PACKED(41, Collection.PACKED_VECTOR, JavaType.BOOLEAN, JavaType.VOID),
  UINT32_LIST_PACKED(42, Collection.PACKED_VECTOR, JavaType.INT, JavaType.VOID),
  ENUM_LIST_PACKED(43, Collection.PACKED_VECTOR, JavaType.ENUM, JavaType.VOID),
  SFIXED32_LIST_PACKED(44, Collection.PACKED_VECTOR, JavaType.INT, JavaType.VOID),
  SFIXED64_LIST_PACKED(45, Collection.PACKED_VECTOR, JavaType.LONG, JavaType.VOID),
  SINT32_LIST_PACKED(46, Collection.PACKED_VECTOR, JavaType.INT, JavaType.VOID),
  SINT64_LIST_PACKED(47, Collection.PACKED_VECTOR, JavaType.LONG, JavaType.VOID),
  INT32_TO_INT32_MAP(48, Collection.MAP, JavaType.INT, JavaType.INT),
  INT32_TO_INT64_MAP(49, Collection.MAP, JavaType.INT, JavaType.LONG),
  INT32_TO_UINT32_MAP(50, Collection.MAP, JavaType.INT, JavaType.INT),
  INT32_TO_UINT64_MAP(51, Collection.MAP, JavaType.INT, JavaType.LONG),
  INT32_TO_SINT32_MAP(52, Collection.MAP, JavaType.INT, JavaType.INT),
  INT32_TO_SINT64_MAP(53, Collection.MAP, JavaType.INT, JavaType.LONG),
  INT32_TO_FIXED32_MAP(54, Collection.MAP, JavaType.INT, JavaType.INT),
  INT32_TO_FIXED64_MAP(55, Collection.MAP, JavaType.INT, JavaType.LONG),
  INT32_TO_SFIXED32_MAP(56, Collection.MAP, JavaType.INT, JavaType.INT),
  INT32_TO_SFIXED64_MAP(57, Collection.MAP, JavaType.INT, JavaType.LONG),
  INT32_TO_BOOL_MAP(58, Collection.MAP, JavaType.INT, JavaType.BOOLEAN),
  INT32_TO_STRING_MAP(59, Collection.MAP, JavaType.INT, JavaType.STRING),
  INT32_TO_ENUM_MAP(60, Collection.MAP, JavaType.INT, JavaType.ENUM),
  INT32_TO_MESSAGE_MAP(61, Collection.MAP, JavaType.INT, JavaType.MESSAGE),
  INT32_TO_BYTES_MAP(62, Collection.MAP, JavaType.INT, JavaType.BYTE_STRING),
  INT32_TO_DOUBLE_MAP(63, Collection.MAP, JavaType.INT, JavaType.DOUBLE),
  INT32_TO_FLOAT_MAP(64, Collection.MAP, JavaType.INT, JavaType.FLOAT),
  INT64_TO_INT32_MAP(65, Collection.MAP, JavaType.LONG, JavaType.INT),
  INT64_TO_INT64_MAP(66, Collection.MAP, JavaType.LONG, JavaType.LONG),
  INT64_TO_UINT32_MAP(67, Collection.MAP, JavaType.LONG, JavaType.INT),
  INT64_TO_UINT64_MAP(68, Collection.MAP, JavaType.LONG, JavaType.LONG),
  INT64_TO_SINT32_MAP(69, Collection.MAP, JavaType.LONG, JavaType.INT),
  INT64_TO_SINT64_MAP(70, Collection.MAP, JavaType.LONG, JavaType.LONG),
  INT64_TO_FIXED32_MAP(71, Collection.MAP, JavaType.LONG, JavaType.INT),
  INT64_TO_FIXED64_MAP(72, Collection.MAP, JavaType.LONG, JavaType.LONG),
  INT64_TO_SFIXED32_MAP(73, Collection.MAP, JavaType.LONG, JavaType.INT),
  INT64_TO_SFIXED64_MAP(74, Collection.MAP, JavaType.LONG, JavaType.LONG),
  INT64_TO_BOOL_MAP(75, Collection.MAP, JavaType.LONG, JavaType.BOOLEAN),
  INT64_TO_STRING_MAP(76, Collection.MAP, JavaType.LONG, JavaType.STRING),
  INT64_TO_ENUM_MAP(77, Collection.MAP, JavaType.LONG, JavaType.ENUM),
  INT64_TO_MESSAGE_MAP(78, Collection.MAP, JavaType.LONG, JavaType.MESSAGE),
  INT64_TO_BYTES_MAP(79, Collection.MAP, JavaType.LONG, JavaType.BYTE_STRING),
  INT64_TO_DOUBLE_MAP(80, Collection.MAP, JavaType.LONG, JavaType.DOUBLE),
  INT64_TO_FLOAT_MAP(81, Collection.MAP, JavaType.LONG, JavaType.FLOAT),
  UINT32_TO_INT32_MAP(82, Collection.MAP, JavaType.INT, JavaType.INT),
  UINT32_TO_INT64_MAP(83, Collection.MAP, JavaType.INT, JavaType.LONG),
  UINT32_TO_UINT32_MAP(84, Collection.MAP, JavaType.INT, JavaType.INT),
  UINT32_TO_UINT64_MAP(85, Collection.MAP, JavaType.INT, JavaType.LONG),
  UINT32_TO_SINT32_MAP(86, Collection.MAP, JavaType.INT, JavaType.INT),
  UINT32_TO_SINT64_MAP(87, Collection.MAP, JavaType.INT, JavaType.LONG),
  UINT32_TO_FIXED32_MAP(88, Collection.MAP, JavaType.INT, JavaType.INT),
  UINT32_TO_FIXED64_MAP(89, Collection.MAP, JavaType.INT, JavaType.LONG),
  UINT32_TO_SFIXED32_MAP(90, Collection.MAP, JavaType.INT, JavaType.INT),
  UINT32_TO_SFIXED64_MAP(91, Collection.MAP, JavaType.INT, JavaType.LONG),
  UINT32_TO_BOOL_MAP(92, Collection.MAP, JavaType.INT, JavaType.BOOLEAN),
  UINT32_TO_STRING_MAP(93, Collection.MAP, JavaType.INT, JavaType.STRING),
  UINT32_TO_ENUM_MAP(94, Collection.MAP, JavaType.INT, JavaType.ENUM),
  UINT32_TO_MESSAGE_MAP(95, Collection.MAP, JavaType.INT, JavaType.MESSAGE),
  UINT32_TO_BYTES_MAP(96, Collection.MAP, JavaType.INT, JavaType.BYTE_STRING),
  UINT32_TO_DOUBLE_MAP(97, Collection.MAP, JavaType.INT, JavaType.DOUBLE),
  UINT32_TO_FLOAT_MAP(98, Collection.MAP, JavaType.INT, JavaType.FLOAT),
  UINT64_TO_INT32_MAP(99, Collection.MAP, JavaType.LONG, JavaType.INT),
  UINT64_TO_INT64_MAP(100, Collection.MAP, JavaType.LONG, JavaType.LONG),
  UINT64_TO_UINT32_MAP(101, Collection.MAP, JavaType.LONG, JavaType.INT),
  UINT64_TO_UINT64_MAP(102, Collection.MAP, JavaType.LONG, JavaType.LONG),
  UINT64_TO_SINT32_MAP(103, Collection.MAP, JavaType.LONG, JavaType.INT),
  UINT64_TO_SINT64_MAP(104, Collection.MAP, JavaType.LONG, JavaType.LONG),
  UINT64_TO_FIXED32_MAP(105, Collection.MAP, JavaType.LONG, JavaType.INT),
  UINT64_TO_FIXED64_MAP(106, Collection.MAP, JavaType.LONG, JavaType.LONG),
  UINT64_TO_SFIXED32_MAP(107, Collection.MAP, JavaType.LONG, JavaType.INT),
  UINT64_TO_SFIXED64_MAP(108, Collection.MAP, JavaType.LONG, JavaType.LONG),
  UINT64_TO_BOOL_MAP(109, Collection.MAP, JavaType.LONG, JavaType.BOOLEAN),
  UINT64_TO_STRING_MAP(110, Collection.MAP, JavaType.LONG, JavaType.STRING),
  UINT64_TO_ENUM_MAP(111, Collection.MAP, JavaType.LONG, JavaType.ENUM),
  UINT64_TO_MESSAGE_MAP(112, Collection.MAP, JavaType.LONG, JavaType.MESSAGE),
  UINT64_TO_BYTES_MAP(113, Collection.MAP, JavaType.LONG, JavaType.BYTE_STRING),
  UINT64_TO_DOUBLE_MAP(114, Collection.MAP, JavaType.LONG, JavaType.DOUBLE),
  UINT64_TO_FLOAT_MAP(115, Collection.MAP, JavaType.LONG, JavaType.FLOAT),
  SINT32_TO_INT32_MAP(116, Collection.MAP, JavaType.INT, JavaType.INT),
  SINT32_TO_INT64_MAP(117, Collection.MAP, JavaType.INT, JavaType.LONG),
  SINT32_TO_UINT32_MAP(118, Collection.MAP, JavaType.INT, JavaType.INT),
  SINT32_TO_UINT64_MAP(119, Collection.MAP, JavaType.INT, JavaType.LONG),
  SINT32_TO_SINT32_MAP(120, Collection.MAP, JavaType.INT, JavaType.INT),
  SINT32_TO_SINT64_MAP(121, Collection.MAP, JavaType.INT, JavaType.LONG),
  SINT32_TO_FIXED32_MAP(122, Collection.MAP, JavaType.INT, JavaType.INT),
  SINT32_TO_FIXED64_MAP(123, Collection.MAP, JavaType.INT, JavaType.LONG),
  SINT32_TO_SFIXED32_MAP(124, Collection.MAP, JavaType.INT, JavaType.INT),
  SINT32_TO_SFIXED64_MAP(125, Collection.MAP, JavaType.INT, JavaType.LONG),
  SINT32_TO_BOOL_MAP(126, Collection.MAP, JavaType.INT, JavaType.BOOLEAN),
  SINT32_TO_STRING_MAP(127, Collection.MAP, JavaType.INT, JavaType.STRING),
  SINT32_TO_ENUM_MAP(128, Collection.MAP, JavaType.INT, JavaType.ENUM),
  SINT32_TO_MESSAGE_MAP(129, Collection.MAP, JavaType.INT, JavaType.MESSAGE),
  SINT32_TO_BYTES_MAP(130, Collection.MAP, JavaType.INT, JavaType.BYTE_STRING),
  SINT32_TO_DOUBLE_MAP(131, Collection.MAP, JavaType.INT, JavaType.DOUBLE),
  SINT32_TO_FLOAT_MAP(132, Collection.MAP, JavaType.INT, JavaType.FLOAT),
  SINT64_TO_INT32_MAP(133, Collection.MAP, JavaType.LONG, JavaType.INT),
  SINT64_TO_INT64_MAP(134, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SINT64_TO_UINT32_MAP(135, Collection.MAP, JavaType.LONG, JavaType.INT),
  SINT64_TO_UINT64_MAP(136, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SINT64_TO_SINT32_MAP(137, Collection.MAP, JavaType.LONG, JavaType.INT),
  SINT64_TO_SINT64_MAP(138, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SINT64_TO_FIXED32_MAP(139, Collection.MAP, JavaType.LONG, JavaType.INT),
  SINT64_TO_FIXED64_MAP(140, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SINT64_TO_SFIXED32_MAP(141, Collection.MAP, JavaType.LONG, JavaType.INT),
  SINT64_TO_SFIXED64_MAP(142, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SINT64_TO_BOOL_MAP(143, Collection.MAP, JavaType.LONG, JavaType.BOOLEAN),
  SINT64_TO_STRING_MAP(144, Collection.MAP, JavaType.LONG, JavaType.STRING),
  SINT64_TO_ENUM_MAP(145, Collection.MAP, JavaType.LONG, JavaType.ENUM),
  SINT64_TO_MESSAGE_MAP(146, Collection.MAP, JavaType.LONG, JavaType.MESSAGE),
  SINT64_TO_BYTES_MAP(147, Collection.MAP, JavaType.LONG, JavaType.BYTE_STRING),
  SINT64_TO_DOUBLE_MAP(148, Collection.MAP, JavaType.LONG, JavaType.DOUBLE),
  SINT64_TO_FLOAT_MAP(149, Collection.MAP, JavaType.LONG, JavaType.FLOAT),
  FIXED32_TO_INT32_MAP(150, Collection.MAP, JavaType.INT, JavaType.INT),
  FIXED32_TO_INT64_MAP(151, Collection.MAP, JavaType.INT, JavaType.LONG),
  FIXED32_TO_UINT32_MAP(152, Collection.MAP, JavaType.INT, JavaType.INT),
  FIXED32_TO_UINT64_MAP(153, Collection.MAP, JavaType.INT, JavaType.LONG),
  FIXED32_TO_SINT32_MAP(154, Collection.MAP, JavaType.INT, JavaType.INT),
  FIXED32_TO_SINT64_MAP(155, Collection.MAP, JavaType.INT, JavaType.LONG),
  FIXED32_TO_FIXED32_MAP(156, Collection.MAP, JavaType.INT, JavaType.INT),
  FIXED32_TO_FIXED64_MAP(157, Collection.MAP, JavaType.INT, JavaType.LONG),
  FIXED32_TO_SFIXED32_MAP(158, Collection.MAP, JavaType.INT, JavaType.INT),
  FIXED32_TO_SFIXED64_MAP(159, Collection.MAP, JavaType.INT, JavaType.LONG),
  FIXED32_TO_BOOL_MAP(160, Collection.MAP, JavaType.INT, JavaType.BOOLEAN),
  FIXED32_TO_STRING_MAP(161, Collection.MAP, JavaType.INT, JavaType.STRING),
  FIXED32_TO_ENUM_MAP(162, Collection.MAP, JavaType.INT, JavaType.ENUM),
  FIXED32_TO_MESSAGE_MAP(163, Collection.MAP, JavaType.INT, JavaType.MESSAGE),
  FIXED32_TO_BYTES_MAP(164, Collection.MAP, JavaType.INT, JavaType.BYTE_STRING),
  FIXED32_TO_DOUBLE_MAP(165, Collection.MAP, JavaType.INT, JavaType.DOUBLE),
  FIXED32_TO_FLOAT_MAP(166, Collection.MAP, JavaType.INT, JavaType.FLOAT),
  FIXED64_TO_INT32_MAP(167, Collection.MAP, JavaType.LONG, JavaType.INT),
  FIXED64_TO_INT64_MAP(168, Collection.MAP, JavaType.LONG, JavaType.LONG),
  FIXED64_TO_UINT32_MAP(169, Collection.MAP, JavaType.LONG, JavaType.INT),
  FIXED64_TO_UINT64_MAP(170, Collection.MAP, JavaType.LONG, JavaType.LONG),
  FIXED64_TO_SINT32_MAP(171, Collection.MAP, JavaType.LONG, JavaType.INT),
  FIXED64_TO_SINT64_MAP(172, Collection.MAP, JavaType.LONG, JavaType.LONG),
  FIXED64_TO_FIXED32_MAP(173, Collection.MAP, JavaType.LONG, JavaType.INT),
  FIXED64_TO_FIXED64_MAP(174, Collection.MAP, JavaType.LONG, JavaType.LONG),
  FIXED64_TO_SFIXED32_MAP(175, Collection.MAP, JavaType.LONG, JavaType.INT),
  FIXED64_TO_SFIXED64_MAP(176, Collection.MAP, JavaType.LONG, JavaType.LONG),
  FIXED64_TO_BOOL_MAP(177, Collection.MAP, JavaType.LONG, JavaType.BOOLEAN),
  FIXED64_TO_STRING_MAP(178, Collection.MAP, JavaType.LONG, JavaType.STRING),
  FIXED64_TO_ENUM_MAP(179, Collection.MAP, JavaType.LONG, JavaType.ENUM),
  FIXED64_TO_MESSAGE_MAP(180, Collection.MAP, JavaType.LONG, JavaType.MESSAGE),
  FIXED64_TO_BYTES_MAP(181, Collection.MAP, JavaType.LONG, JavaType.BYTE_STRING),
  FIXED64_TO_DOUBLE_MAP(182, Collection.MAP, JavaType.LONG, JavaType.DOUBLE),
  FIXED64_TO_FLOAT_MAP(183, Collection.MAP, JavaType.LONG, JavaType.FLOAT),
  SFIXED32_TO_INT32_MAP(184, Collection.MAP, JavaType.INT, JavaType.INT),
  SFIXED32_TO_INT64_MAP(185, Collection.MAP, JavaType.INT, JavaType.LONG),
  SFIXED32_TO_UINT32_MAP(186, Collection.MAP, JavaType.INT, JavaType.INT),
  SFIXED32_TO_UINT64_MAP(187, Collection.MAP, JavaType.INT, JavaType.LONG),
  SFIXED32_TO_SINT32_MAP(188, Collection.MAP, JavaType.INT, JavaType.INT),
  SFIXED32_TO_SINT64_MAP(189, Collection.MAP, JavaType.INT, JavaType.LONG),
  SFIXED32_TO_FIXED32_MAP(190, Collection.MAP, JavaType.INT, JavaType.INT),
  SFIXED32_TO_FIXED64_MAP(191, Collection.MAP, JavaType.INT, JavaType.LONG),
  SFIXED32_TO_SFIXED32_MAP(192, Collection.MAP, JavaType.INT, JavaType.INT),
  SFIXED32_TO_SFIXED64_MAP(193, Collection.MAP, JavaType.INT, JavaType.LONG),
  SFIXED32_TO_BOOL_MAP(194, Collection.MAP, JavaType.INT, JavaType.BOOLEAN),
  SFIXED32_TO_STRING_MAP(195, Collection.MAP, JavaType.INT, JavaType.STRING),
  SFIXED32_TO_ENUM_MAP(196, Collection.MAP, JavaType.INT, JavaType.ENUM),
  SFIXED32_TO_MESSAGE_MAP(197, Collection.MAP, JavaType.INT, JavaType.MESSAGE),
  SFIXED32_TO_BYTES_MAP(198, Collection.MAP, JavaType.INT, JavaType.BYTE_STRING),
  SFIXED32_TO_DOUBLE_MAP(199, Collection.MAP, JavaType.INT, JavaType.DOUBLE),
  SFIXED32_TO_FLOAT_MAP(200, Collection.MAP, JavaType.INT, JavaType.FLOAT),
  SFIXED64_TO_INT32_MAP(201, Collection.MAP, JavaType.LONG, JavaType.INT),
  SFIXED64_TO_INT64_MAP(202, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SFIXED64_TO_UINT32_MAP(203, Collection.MAP, JavaType.LONG, JavaType.INT),
  SFIXED64_TO_UINT64_MAP(204, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SFIXED64_TO_SINT32_MAP(205, Collection.MAP, JavaType.LONG, JavaType.INT),
  SFIXED64_TO_SINT64_MAP(206, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SFIXED64_TO_FIXED32_MAP(207, Collection.MAP, JavaType.LONG, JavaType.INT),
  SFIXED64_TO_FIXED64_MAP(208, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SFIXED64_TO_SFIXED32_MAP(209, Collection.MAP, JavaType.LONG, JavaType.INT),
  SFIXED64_TO_SFIXED64_MAP(210, Collection.MAP, JavaType.LONG, JavaType.LONG),
  SFIXED64_TO_BOOL_MAP(211, Collection.MAP, JavaType.LONG, JavaType.BOOLEAN),
  SFIXED64_TO_STRING_MAP(212, Collection.MAP, JavaType.LONG, JavaType.STRING),
  SFIXED64_TO_ENUM_MAP(213, Collection.MAP, JavaType.LONG, JavaType.ENUM),
  SFIXED64_TO_MESSAGE_MAP(214, Collection.MAP, JavaType.LONG, JavaType.MESSAGE),
  SFIXED64_TO_BYTES_MAP(215, Collection.MAP, JavaType.LONG, JavaType.BYTE_STRING),
  SFIXED64_TO_DOUBLE_MAP(216, Collection.MAP, JavaType.LONG, JavaType.DOUBLE),
  SFIXED64_TO_FLOAT_MAP(217, Collection.MAP, JavaType.LONG, JavaType.FLOAT),
  BOOL_TO_INT32_MAP(218, Collection.MAP, JavaType.BOOLEAN, JavaType.INT),
  BOOL_TO_INT64_MAP(219, Collection.MAP, JavaType.BOOLEAN, JavaType.LONG),
  BOOL_TO_UINT32_MAP(220, Collection.MAP, JavaType.BOOLEAN, JavaType.INT),
  BOOL_TO_UINT64_MAP(221, Collection.MAP, JavaType.BOOLEAN, JavaType.LONG),
  BOOL_TO_SINT32_MAP(222, Collection.MAP, JavaType.BOOLEAN, JavaType.INT),
  BOOL_TO_SINT64_MAP(223, Collection.MAP, JavaType.BOOLEAN, JavaType.LONG),
  BOOL_TO_FIXED32_MAP(224, Collection.MAP, JavaType.BOOLEAN, JavaType.INT),
  BOOL_TO_FIXED64_MAP(225, Collection.MAP, JavaType.BOOLEAN, JavaType.LONG),
  BOOL_TO_SFIXED32_MAP(226, Collection.MAP, JavaType.BOOLEAN, JavaType.INT),
  BOOL_TO_SFIXED64_MAP(227, Collection.MAP, JavaType.BOOLEAN, JavaType.LONG),
  BOOL_TO_BOOL_MAP(228, Collection.MAP, JavaType.BOOLEAN, JavaType.BOOLEAN),
  BOOL_TO_STRING_MAP(229, Collection.MAP, JavaType.BOOLEAN, JavaType.STRING),
  BOOL_TO_ENUM_MAP(230, Collection.MAP, JavaType.BOOLEAN, JavaType.ENUM),
  BOOL_TO_MESSAGE_MAP(231, Collection.MAP, JavaType.BOOLEAN, JavaType.MESSAGE),
  BOOL_TO_BYTES_MAP(232, Collection.MAP, JavaType.BOOLEAN, JavaType.BYTE_STRING),
  BOOL_TO_DOUBLE_MAP(233, Collection.MAP, JavaType.BOOLEAN, JavaType.DOUBLE),
  BOOL_TO_FLOAT_MAP(234, Collection.MAP, JavaType.BOOLEAN, JavaType.FLOAT),
  STRING_TO_INT32_MAP(235, Collection.MAP, JavaType.STRING, JavaType.INT),
  STRING_TO_INT64_MAP(236, Collection.MAP, JavaType.STRING, JavaType.LONG),
  STRING_TO_UINT32_MAP(237, Collection.MAP, JavaType.STRING, JavaType.INT),
  STRING_TO_UINT64_MAP(238, Collection.MAP, JavaType.STRING, JavaType.LONG),
  STRING_TO_SINT32_MAP(239, Collection.MAP, JavaType.STRING, JavaType.INT),
  STRING_TO_SINT64_MAP(240, Collection.MAP, JavaType.STRING, JavaType.LONG),
  STRING_TO_FIXED32_MAP(241, Collection.MAP, JavaType.STRING, JavaType.INT),
  STRING_TO_FIXED64_MAP(242, Collection.MAP, JavaType.STRING, JavaType.LONG),
  STRING_TO_SFIXED32_MAP(243, Collection.MAP, JavaType.STRING, JavaType.INT),
  STRING_TO_SFIXED64_MAP(244, Collection.MAP, JavaType.STRING, JavaType.LONG),
  STRING_TO_BOOL_MAP(245, Collection.MAP, JavaType.STRING, JavaType.BOOLEAN),
  STRING_TO_STRING_MAP(246, Collection.MAP, JavaType.STRING, JavaType.STRING),
  STRING_TO_ENUM_MAP(247, Collection.MAP, JavaType.STRING, JavaType.ENUM),
  STRING_TO_MESSAGE_MAP(248, Collection.MAP, JavaType.STRING, JavaType.MESSAGE),
  STRING_TO_BYTES_MAP(249, Collection.MAP, JavaType.STRING, JavaType.BYTE_STRING),
  STRING_TO_DOUBLE_MAP(250, Collection.MAP, JavaType.STRING, JavaType.DOUBLE),
  STRING_TO_FLOAT_MAP(251, Collection.MAP, JavaType.STRING, JavaType.FLOAT),
  GROUP(252, Collection.SCALAR, JavaType.MESSAGE, JavaType.VOID),
  GROUP_LIST(253, Collection.VECTOR, JavaType.MESSAGE, JavaType.VOID),
  GROUP_LIST_PACKED(254, Collection.PACKED_VECTOR, JavaType.MESSAGE, JavaType.VOID);

  private final JavaType javaType1;
  private final JavaType javaType2;
  private final int id;
  private final Collection collection;
  private final Class<?> elementType1;
  private final Class<?> elementType2;

  FieldType(int id, Collection collection, JavaType javaType1, JavaType javaType2) {
    this.id = id;
    this.collection = collection;
    this.javaType1 = javaType1;
    this.javaType2 = javaType2;

    switch (collection) {
      case MAP:
        elementType1 = javaType1.getBoxedType();
        elementType2 = javaType2.getBoxedType();
        break;
      case VECTOR:
        elementType1 = javaType1.getBoxedType();
        elementType2 = null;
        break;
      case SCALAR:
      default:
        elementType1 = null;
        elementType2 = null;
        break;
    }
  }

  /**
   * A reliable unique identifier for this type.
   */
  public int id() {
    return id;
  }

  /**
   * Gets the {@link JavaType} for this field. For lists, this identifies the type of the elements
   * contained within the list.
   */
  public JavaType getJavaType() {
    return javaType1;
  }

  /**
   * Indicates whether a list field should be represented on the wire in packed form.
   */
  public boolean isPacked() {
    return Collection.PACKED_VECTOR.equals(collection);
  }

  /**
   * Indicates whether this field represents a list of values.
   */
  public boolean isList() {
    return collection.isList();
  }

  /**
   * Indicates whether or not this {@link FieldType} can be applied to the given {@link Field}.
   */
  public boolean isValidForField(Field field) {
    if (Collection.VECTOR.equals(collection)) {
      return isValidForList(field);
    } else {
      return javaType1.getType().isAssignableFrom(field.getType());
    }
  }

  private boolean isValidForList(Field field) {
    Class<?> clazz = field.getType();
    if (!javaType1.getType().isAssignableFrom(clazz)) {
      // The field isn't a List type.
      return false;
    }
    Type[] types = EMPTY_TYPES;
    Type genericType = field.getGenericType();
    if (genericType instanceof ParameterizedType) {
      types = ((ParameterizedType) field.getGenericType()).getActualTypeArguments();
    }
    Type listParameter = getListParameter(clazz, types);
    if (!(listParameter instanceof Class)) {
      // It's a wildcard, we should allow anything in the list.
      return true;
    }
    return elementType1.isAssignableFrom((Class<?>) listParameter);
  }

  /**
   * Looks up the appropriate {@link FieldType} by it's identifier.
   *
   * @return the {@link FieldType} or {@code null} if not found.
   */
  /* @Nullable */
  public static FieldType forId(byte id) {
    if (id < 0 || id >= VALUES.length) {
      return null;
    }
    return VALUES[id];
  }

  private static final FieldType[] VALUES;
  private static final Type[] EMPTY_TYPES = new Type[0];

  static {
    FieldType[] values = values();
    VALUES = new FieldType[values.length];
    for (FieldType type : values) {
      VALUES[type.id] = type;
    }
  }

  /**
   * Given a class, finds a generic super class or interface that extends {@link List}.
   *
   * @return the generic super class/interface, or {@code null} if not found.
   */
  /* @Nullable */
  private static Type getGenericSuperList(Class<?> clazz) {
    // First look at interfaces.
    Type[] genericInterfaces = clazz.getGenericInterfaces();
    for (Type genericInterface : genericInterfaces) {
      if (genericInterface instanceof ParameterizedType) {
        ParameterizedType parameterizedType = (ParameterizedType) genericInterface;
        Class<?> rawType = (Class<?>) parameterizedType.getRawType();
        if (List.class.isAssignableFrom(rawType)) {
          return genericInterface;
        }
      }
    }

    // Try the subclass
    Type type = clazz.getGenericSuperclass();
    if (type instanceof ParameterizedType) {
      ParameterizedType parameterizedType = (ParameterizedType) type;
      Class<?> rawType = (Class<?>) parameterizedType.getRawType();
      if (List.class.isAssignableFrom(rawType)) {
        return type;
      }
    }

    // No super class/interface extends List.
    return null;
  }

  /**
   * Inspects the inheritance hierarchy for the given class and finds the generic type parameter
   * for {@link List}.
   *
   * @param clazz the class to begin the search.
   * @param realTypes the array of actual type parameters for {@code clazz}. These will be used to
   * substitute generic parameters up the inheritance hierarchy. If {@code clazz} does not have any
   * generic parameters, this list should be empty.
   * @return the {@link List} parameter.
   */
  private static Type getListParameter(Class<?> clazz, Type[] realTypes) {
  top:
    while (clazz != List.class) {
      // First look at generic subclass and interfaces.
      Type genericType = getGenericSuperList(clazz);
      if (genericType instanceof ParameterizedType) {
        // Replace any generic parameters with the real values.
        ParameterizedType parameterizedType = (ParameterizedType) genericType;
        Type[] superArgs = parameterizedType.getActualTypeArguments();
        for (int i = 0; i < superArgs.length; ++i) {
          Type superArg = superArgs[i];
          if (superArg instanceof TypeVariable) {
            // Get the type variables for this class so that we can match them to the variables
            // used on the super class.
            TypeVariable<?>[] clazzParams = clazz.getTypeParameters();
            if (realTypes.length != clazzParams.length) {
              throw new RuntimeException("Type array mismatch");
            }

            // Replace the variable parameter with the real type.
            boolean foundReplacement = false;
            for (int j = 0; j < clazzParams.length; ++j) {
              if (superArg == clazzParams[j]) {
                Type realType = realTypes[j];
                superArgs[i] = realType;
                foundReplacement = true;
                break;
              }
            }
            if (!foundReplacement) {
              throw new RuntimeException("Unable to find replacement for " + superArg);
            }
          }
        }

        Class<?> parent = (Class<?>) parameterizedType.getRawType();

        realTypes = superArgs;
        clazz = parent;
        continue;
      }

      // None of the parameterized types inherit List. Just continue up the inheritance hierarchy
      // toward the List interface until we can identify the parameters.
      realTypes = EMPTY_TYPES;
      for (Class<?> iface : clazz.getInterfaces()) {
        if (List.class.isAssignableFrom(iface)) {
          clazz = iface;
          continue top;
        }
      }
      clazz = clazz.getSuperclass();
    }

    if (realTypes.length != 1) {
      throw new RuntimeException("Unable to identify parameter type for List<T>");
    }
    return realTypes[0];
  }

  enum Collection {
    SCALAR(false),
    VECTOR(true),
    PACKED_VECTOR(true),
    MAP(false);

    private final boolean isList;

    Collection(boolean isList) {
      this.isList = isList;
    }

    /**
     * @return the isList
     */
    public boolean isList() {
      return isList;
    }
  }
}
