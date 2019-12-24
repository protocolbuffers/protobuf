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

/**
 * @fileoverview Test cases for Int64-manipulation functions.
 *
 * Test suite is written using Jasmine -- see http://jasmine.github.io/
 *
 * @author cfallin@google.com (Chris Fallin)
 */

goog.require('goog.testing.asserts');
goog.require('jspb.arith.Int64');
goog.require('jspb.arith.UInt64');


describe('binaryArithTest', function() {
  /**
   * Tests comparison operations.
   */
  it('testCompare', function() {
    var a = new jspb.arith.UInt64(1234, 5678);
    var b = new jspb.arith.UInt64(1234, 5678);
    assertEquals(a.cmp(b), 0);
    assertEquals(b.cmp(a), 0);
    b.lo -= 1;
    assertEquals(a.cmp(b), 1);
    assertEquals(b.cmp(a), -1);
    b.lo += 2;
    assertEquals(a.cmp(b), -1);
    assertEquals(b.cmp(a), 1);
    b.lo = a.lo;
    b.hi = a.hi - 1;
    assertEquals(a.cmp(b), 1);
    assertEquals(b.cmp(a), -1);

    assertEquals(a.zero(), false);
    assertEquals(a.msb(), false);
    assertEquals(a.lsb(), false);
    a.hi = 0;
    a.lo = 0;
    assertEquals(a.zero(), true);
    a.hi = 0x80000000;
    assertEquals(a.zero(), false);
    assertEquals(a.msb(), true);
    a.lo = 0x00000001;
    assertEquals(a.lsb(), true);
  });


  /**
   * Tests shifts.
   */
  it('testShifts', function() {
    var a = new jspb.arith.UInt64(1, 0);
    assertEquals(a.lo, 1);
    assertEquals(a.hi, 0);
    var orig = a;
    a = a.leftShift();
    assertEquals(orig.lo, 1);  // original unmodified.
    assertEquals(orig.hi, 0);
    assertEquals(a.lo, 2);
    assertEquals(a.hi, 0);
    a = a.leftShift();
    assertEquals(a.lo, 4);
    assertEquals(a.hi, 0);
    for (var i = 0; i < 29; i++) {
      a = a.leftShift();
    }
    assertEquals(a.lo, 0x80000000);
    assertEquals(a.hi, 0);
    a = a.leftShift();
    assertEquals(a.lo, 0);
    assertEquals(a.hi, 1);
    a = a.leftShift();
    assertEquals(a.lo, 0);
    assertEquals(a.hi, 2);
    a = a.rightShift();
    a = a.rightShift();
    assertEquals(a.lo, 0x80000000);
    assertEquals(a.hi, 0);
    a = a.rightShift();
    assertEquals(a.lo, 0x40000000);
    assertEquals(a.hi, 0);
  });


  /**
   * Tests additions.
   */
  it('testAdd', function() {
    var a = new jspb.arith.UInt64(/* lo = */ 0x89abcdef,
                                         /* hi = */ 0x01234567);
    var b = new jspb.arith.UInt64(/* lo = */ 0xff52ab91,
                                         /* hi = */ 0x92fa2123);
    // Addition with carry.
    var c = a.add(b);
    assertEquals(a.lo, 0x89abcdef);  // originals unmodified.
    assertEquals(a.hi, 0x01234567);
    assertEquals(b.lo, 0xff52ab91);
    assertEquals(b.hi, 0x92fa2123);
    assertEquals(c.lo, 0x88fe7980);
    assertEquals(c.hi, 0x941d668b);

    // Simple addition without carry.
    a.lo = 2;
    a.hi = 0;
    b.lo = 3;
    b.hi = 0;
    c = a.add(b);
    assertEquals(c.lo, 5);
    assertEquals(c.hi, 0);
  });


  /**
   * Test subtractions.
   */
  it('testSub', function() {
    var kLength = 10;
    var hiValues = [0x1682ef32,
                    0x583902f7,
                    0xb62f5955,
                    0x6ea99bbf,
                    0x25a39c20,
                    0x0700a08b,
                    0x00f7304d,
                    0x91a5b5af,
                    0x89077fd2,
                    0xe09e347c];
    var loValues = [0xe1538b18,
                    0xbeacd556,
                    0x74100758,
                    0x96e3cb26,
                    0x56c37c3f,
                    0xe00b3f7d,
                    0x859f25d7,
                    0xc2ee614a,
                    0xe1d21cd7,
                    0x30aae6a4];
    for (var i = 0; i < kLength; i++) {
      for (var j = 0; j < kLength; j++) {
        var a = new jspb.arith.UInt64(loValues[i], hiValues[j]);
        var b = new jspb.arith.UInt64(loValues[j], hiValues[i]);
        var c = a.add(b).sub(b);
        assertEquals(c.hi, a.hi);
        assertEquals(c.lo, a.lo);
      }
    }
  });


  /**
   * Tests 32-by-32 multiplication.
   */
  it('testMul32x32', function() {
    var testData = [
      // a        b          low(a*b)   high(a*b)
      [0xc0abe2f8, 0x1607898a, 0x5de711b0, 0x109471b8],
      [0x915eb3cb, 0x4fb66d0e, 0xbd0d441a, 0x2d43d0bc],
      [0xfe4efe70, 0x80b48c37, 0xbcddea10, 0x7fdada0c],
      [0xe222fd4a, 0xe43d524a, 0xd5e0eb64, 0xc99d549c],
      [0xd171f469, 0xb94ebd01, 0x4be17969, 0x979bc4fa],
      [0x829cc1df, 0xe2598b38, 0xf4157dc8, 0x737c12ad],
      [0xf10c3767, 0x8382881e, 0x942b3612, 0x7bd428b8],
      [0xb0f6dd24, 0x232597e1, 0x079c98a4, 0x184bbce7],
      [0xfcdb05a7, 0x902f55bc, 0x636199a4, 0x8e69f412],
      [0x0dd0bfa9, 0x916e27b1, 0x6e2542d9, 0x07d92e65]
    ];

    for (var i = 0; i < testData.length; i++) {
      var a = testData[i][0] >>> 0;
      var b = testData[i][1] >>> 0;
      var cLow = testData[i][2] >>> 0;
      var cHigh = testData[i][3] >>> 0;
      var c = jspb.arith.UInt64.mul32x32(a, b);
      assertEquals(c.lo, cLow);
      assertEquals(c.hi, cHigh);
    }
  });


  /**
   * Tests 64-by-32 multiplication.
   */
  it('testMul', function() {
    // 64x32 bits produces 96 bits of product. The multiplication function under
    // test truncates the top 32 bits, so we compare against a 64-bit expected
    // product.
    var testData = [
      // low(a)   high(a)               low(a*b)   high(a*b)
      [0xec10955b, 0x360eb168, 0x4b7f3f5b, 0xbfcb7c59, 0x9517da5f],
      [0x42b000fc, 0x9d101642, 0x6fa1ab72, 0x2584c438, 0x6a9e6d2b],
      [0xf42d4fb4, 0xae366403, 0xa65a1000, 0x92434000, 0x1ff978df],
      [0x17e2f56b, 0x25487693, 0xf13f98c7, 0x73794e2d, 0xa96b0c6a],
      [0x492f241f, 0x76c0eb67, 0x7377ac44, 0xd4336c3c, 0xfc4b1ebe],
      [0xd6b92321, 0xe184fa48, 0xd6e76904, 0x93141584, 0xcbf44da1],
      [0x4bf007ea, 0x968c0a9e, 0xf5e4026a, 0x4fdb1ae4, 0x61b9fb7d],
      [0x10a83be7, 0x2d685ba6, 0xc9e5fb7f, 0x2ad43499, 0x3742473d],
      [0x2f261829, 0x1aca681a, 0x3d3494e3, 0x8213205b, 0x283719f8],
      [0xe4f2ce21, 0x2e74b7bd, 0xd801b38b, 0xbc17feeb, 0xc6c44e0f]
    ];

    for (var i = 0; i < testData.length; i++) {
      var a = new jspb.arith.UInt64(testData[i][0], testData[i][1]);
      var prod = a.mul(testData[i][2]);
      assertEquals(prod.lo, testData[i][3]);
      assertEquals(prod.hi, testData[i][4]);
    }
  });


  /**
   * Tests 64-div-by-32 division.
   */
  it('testDiv', function() {
    // Compute a/b, yielding quot = a/b and rem = a%b.
    var testData = [
      // --- divisors in (0, 2^32-1) to test full divisor range
      // low(a)   high(a)    b          low(quot)  high(quot) rem
      [0x712443f1, 0xe85cefcc, 0xc1a7050b, 0x332c79ad, 0x00000001, 0x92ffa882],
      [0x11912915, 0xb2699eb5, 0x30467cbe, 0xb21b4be4, 0x00000003, 0x283465dd],
      [0x0d917982, 0x201f2a6e, 0x3f35bf03, 0x8217c8e4, 0x00000000, 0x153402d6],
      [0xa072c108, 0x74020c96, 0xc60568fd, 0x95f9613e, 0x00000000, 0x3f4676c2],
      [0xd845d5d8, 0xcdd235c4, 0x20426475, 0x6154e78b, 0x00000006, 0x202fb751],
      [0xa4dbf71f, 0x9e90465e, 0xf08e022f, 0xa8be947f, 0x00000000, 0xbe43b5ce],
      [0x3dbe627f, 0xa791f4b9, 0x28a5bd89, 0x1f5dfe93, 0x00000004, 0x02bf9ed4],
      [0x5c1c53ee, 0xccf5102e, 0x198576e7, 0x07e3ae31, 0x00000008, 0x02ea8fb7],
      [0xfef1e581, 0x04714067, 0xca6540c1, 0x059e73ec, 0x00000000, 0x31658095],
      [0x1e2dd90c, 0x13dd6667, 0x8b2184c3, 0x248d1a42, 0x00000000, 0x4ca6d0c6],
      // --- divisors in (0, 2^16-1) to test larger quotient high-words
      // low(a)   high(a)    b          low(quot)  high(quot) rem
      [0x86722b47, 0x2cd57c9a, 0x00003123, 0x2ae41b7a, 0x0000e995, 0x00000f99],
      [0x1dd7884c, 0xf5e839bc, 0x00009eeb, 0x5c886242, 0x00018c21, 0x000099b6],
      [0x5c53d625, 0x899fc7e5, 0x000087d7, 0xd625007a, 0x0001035c, 0x000019af],
      [0x6932d932, 0x9d0a5488, 0x000051fb, 0x9d976143, 0x0001ea63, 0x00004981],
      [0x4d18bb85, 0x0c92fb31, 0x00001d9f, 0x03265ab4, 0x00006cac, 0x000001b9],
      [0xbe756768, 0xdea67ccb, 0x00008a03, 0x58add442, 0x00019cff, 0x000056a2],
      [0xe2466f9a, 0x2521f114, 0x0000c350, 0xa0c0860d, 0x000030ab, 0x0000a48a],
      [0xf00ddad1, 0xe2f5446a, 0x00002cfc, 0x762697a6, 0x00050b96, 0x00000b69],
      [0xa879152a, 0x0a70e0a5, 0x00007cdf, 0xb44151b3, 0x00001567, 0x0000363d],
      [0x7179a74c, 0x46083fff, 0x0000253c, 0x4d39ba6e, 0x0001e17f, 0x00000f84]
    ];

    for (var i = 0; i < testData.length; i++) {
      var a = new jspb.arith.UInt64(testData[i][0], testData[i][1]);
      var result = a.div(testData[i][2]);
      var quotient = result[0];
      var remainder = result[1];
      assertEquals(quotient.lo, testData[i][3]);
      assertEquals(quotient.hi, testData[i][4]);
      assertEquals(remainder.lo, testData[i][5]);
    }
  });


  /**
   * Tests .toString() and .fromString().
   */
  it('testStrings', function() {
    var testData = [
        [0x5e84c935, 0xcae33d0e, '14619595947299359029'],
        [0x62b3b8b8, 0x93480544, '10612738313170434232'],
        [0x319bfb13, 0xc01c4172, '13843011313344445203'],
        [0x5b8a65fb, 0xa5885b31, '11927883880638080507'],
        [0x6bdb80f1, 0xb0d1b16b, '12741159895737008369'],
        [0x4b82b442, 0x2e0d8c97, '3318463081876730946'],
        [0x780d5208, 0x7d76752c, '9040542135845999112'],
        [0x2e46800f, 0x0993778d, '690026616168284175'],
        [0xf00a7e32, 0xcd8e3931, '14811839111111540274'],
        [0x1baeccd6, 0x923048c4, '10533999535534820566'],
        [0x03669d29, 0xbff3ab72, '13831587386756603177'],
        [0x2526073e, 0x01affc81, '121593346566522686'],
        [0xc24244e0, 0xd7f40d0e, '15561076969511732448'],
        [0xc56a341e, 0xa68b66a7, '12000798502816461854'],
        [0x8738d64d, 0xbfe78604, '13828168534871037517'],
        [0x5baff03b, 0xd7572aea, '15516918227177304123'],
        [0x4a843d8a, 0x864e132b, '9677693725920476554'],
        [0x25b4e94d, 0x22b54dc6, '2500990681505655117'],
        [0x6bbe664b, 0x55a5cc0e, '6171563226690381387'],
        [0xee916c81, 0xb00aabb3, '12685140089732426881']
    ];

    for (var i = 0; i < testData.length; i++) {
      var a = new jspb.arith.UInt64(testData[i][0], testData[i][1]);
      var roundtrip = jspb.arith.UInt64.fromString(a.toString());
      assertEquals(roundtrip.lo, a.lo);
      assertEquals(roundtrip.hi, a.hi);
      assertEquals(a.toString(), testData[i][2]);
    }
  });


  /**
   * Tests signed Int64s. These are built on UInt64s, so we only need to test
   * the explicit overrides: .toString() and .fromString().
   */
  it('testSignedInt64', function() {
    var testStrings = [
        '-7847499644178593666',
        '3771946501229139523',
        '2872856549054995060',
        '-5780049594274350904',
        '3383785956695105201',
        '2973055184857072610',
        '-3879428459215627206',
        '4589812431064156631',
        '8484075557333689940',
        '1075325817098092407',
        '-4346697501012292314',
        '2488620459718316637',
        '6112655187423520672',
        '-3655278273928612104',
        '3439154019435803196',
        '1004112478843763757',
        '-6587790776614368413',
        '664320065099714586',
        '4760412909973292912',
        '-7911903989602274672'
    ];

    for (var i = 0; i < testStrings.length; i++) {
      var roundtrip =
          jspb.arith.Int64.fromString(testStrings[i]).toString();
      assertEquals(roundtrip, testStrings[i]);
    }
  });
});
