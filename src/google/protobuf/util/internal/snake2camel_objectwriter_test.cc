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

#include <google/protobuf/util/internal/snake2camel_objectwriter.h>
#include <google/protobuf/util/internal/expecting_objectwriter.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace util {
namespace converter {

class Snake2CamelObjectWriterTest : public ::testing::Test {
 protected:
  Snake2CamelObjectWriterTest() : mock_(), expects_(&mock_), testing_(&mock_) {}
  virtual ~Snake2CamelObjectWriterTest() {}

  MockObjectWriter mock_;
  ExpectingObjectWriter expects_;
  Snake2CamelObjectWriter testing_;
};

TEST_F(Snake2CamelObjectWriterTest, Empty) {
  // Set expectation
  expects_.StartObject("")->EndObject();

  // Actual testing
  testing_.StartObject("")->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, UnderscoresOnly) {
  // Set expectation
  expects_.StartObject("")
      ->RenderInt32("", 1)
      ->RenderInt32("", 2)
      ->RenderInt32("", 3)
      ->RenderInt32("", 4)
      ->RenderInt32("", 5)
      ->EndObject();

  // Actual testing
  testing_.StartObject("")
      ->RenderInt32("_", 1)
      ->RenderInt32("__", 2)
      ->RenderInt32("___", 3)
      ->RenderInt32("____", 4)
      ->RenderInt32("_____", 5)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, LowercaseOnly) {
  // Set expectation
  expects_.StartObject("")
      ->RenderString("key", "value")
      ->RenderString("abracadabra", "magic")
      ->EndObject();

  // Actual testing
  testing_.StartObject("")
      ->RenderString("key", "value")
      ->RenderString("abracadabra", "magic")
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, UppercaseOnly) {
  // Set expectation
  expects_.StartObject("")
      ->RenderString("key", "VALUE")
      ->RenderString("abracadabra", "MAGIC")
      ->EndObject();

  // Actual testing
  testing_.StartObject("")
      ->RenderString("KEY", "VALUE")
      ->RenderString("ABRACADABRA", "MAGIC")
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, CamelCase) {
  // Set expectation
  expects_.StartObject("")
      ->RenderString("camelCase", "camelCase")
      ->RenderString("theQuickBrownFoxJumpsOverTheLazyDog",
                     "theQuickBrownFoxJumpsOverTheLazyDog")
      ->EndObject();

  // Actual testing
  testing_.StartObject("")
      ->RenderString("camelCase", "camelCase")
      ->RenderString("theQuickBrownFoxJumpsOverTheLazyDog",
                     "theQuickBrownFoxJumpsOverTheLazyDog")
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, FirstCapCamelCase) {
  // Sets expectation
  expects_.StartObject("camel")
      ->RenderString("camelCase", "CamelCase")
      ->RenderString("theQuickBrownFoxJumpsOverTheLazyDog",
                     "TheQuickBrownFoxJumpsOverTheLazyDog")
      ->EndObject();

  // Actual testing
  testing_.StartObject("Camel")
      ->RenderString("CamelCase", "CamelCase")
      ->RenderString("TheQuickBrownFoxJumpsOverTheLazyDog",
                     "TheQuickBrownFoxJumpsOverTheLazyDog")
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, LastCapCamelCase) {
  // Sets expectation
  expects_.StartObject("lastCapCamelCasE")->EndObject();

  // Actual testing
  testing_.StartObject("lastCapCamelCasE")->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, MixedCapCamelCase) {
  // Sets expectation
  expects_.StartObject("googleIsTheBest")
      ->RenderFloat("iLoveGOOGLE", 1.61803f)
      ->RenderFloat("goGoogleGO", 2.71828f)
      ->RenderFloat("gBikeISCool", 3.14159f)
      ->EndObject();

  // Actual testing
  testing_.StartObject("GOOGLEIsTheBest")
      ->RenderFloat("ILoveGOOGLE", 1.61803f)
      ->RenderFloat("GOGoogleGO", 2.71828f)
      ->RenderFloat("GBikeISCool", 3.14159f)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, MixedCase) {
  // Sets expectation
  expects_.StartObject("snakeCaseCamelCase")
      ->RenderBool("camelCaseSnakeCase", false)
      ->RenderBool("mixedCamelAndUnderScores", false)
      ->RenderBool("goGOOGLEGo", true)
      ->EndObject();

  // Actual testing
  testing_.StartObject("snake_case_camelCase")
      ->RenderBool("camelCase_snake_case", false)
      ->RenderBool("MixedCamel_And_UnderScores", false)
      ->RenderBool("Go_GOOGLEGo", true)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, SnakeCase) {
  // Sets expectation
  expects_.StartObject("")
      ->RenderString("snakeCase", "snake_case")
      ->RenderString("theQuickBrownFoxJumpsOverTheLazyDog",
                     "the_quick_brown_fox_jumps_over_the_lazy_dog")
      ->EndObject();

  // Actual testing
  testing_.StartObject("")
      ->RenderString("snake_case", "snake_case")
      ->RenderString("the_quick_brown_fox_jumps_over_the_lazy_dog",
                     "the_quick_brown_fox_jumps_over_the_lazy_dog")
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, FirstCapSnakeCase) {
  // Sets expectation
  expects_.StartObject("firstCapSnakeCase")
      ->RenderBool("helloWorld", true)
      ->EndObject();

  // Actual testing
  testing_.StartObject("First_Cap_Snake_Case")
      ->RenderBool("Hello_World", true)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, AllCapSnakeCase) {
  // Sets expectation
  expects_.StartObject("allCAPSNAKECASE")
      ->RenderDouble("nyseGOOGL", 600.0L)
      ->RenderDouble("aBCDE", 1.0L)
      ->RenderDouble("klMNOP", 2.0L)
      ->RenderDouble("abcIJKPQRXYZ", 3.0L)
      ->EndObject();

  // Actual testing
  testing_.StartObject("ALL_CAP_SNAKE_CASE")
      ->RenderDouble("NYSE_GOOGL", 600.0L)
      ->RenderDouble("A_B_C_D_E", 1.0L)
      ->RenderDouble("KL_MN_OP", 2.0L)
      ->RenderDouble("ABC_IJK_PQR_XYZ", 3.0L)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, RepeatedUnderScoreSnakeCase) {
  // Sets expectation
  expects_.StartObject("")
      ->RenderInt32("doubleUnderscoreSnakeCase", 2)
      ->RenderInt32("tripleUnderscoreFirstCap", 3)
      ->RenderInt32("quadrupleUNDERSCOREALLCAP", 4)
      ->EndObject();

  // Actual testing
  testing_.StartObject("")
      ->RenderInt32("double__underscore__snake__case", 2)
      ->RenderInt32("Triple___Underscore___First___Cap", 3)
      ->RenderInt32("QUADRUPLE____UNDERSCORE____ALL____CAP", 4)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, LeadingUnderscoreSnakeCase) {
  // Sets expectation
  expects_.StartObject("leadingUnderscoreSnakeCase")
      ->RenderUint32("leadingDoubleUnderscore", 2)
      ->RenderUint32("leadingTripleUnderscoreFirstCap", 3)
      ->RenderUint32("leadingQUADRUPLEUNDERSCOREALLCAP", 4)
      ->EndObject();

  // Actual testing
  testing_.StartObject("_leading_underscore_snake_case")
      ->RenderUint32("__leading_double_underscore", 2)
      ->RenderUint32("___Leading_Triple_Underscore_First_Cap", 3)
      ->RenderUint32("____LEADING_QUADRUPLE_UNDERSCORE_ALL_CAP", 4)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, TrailingUnderscoreSnakeCase) {
  // Sets expectation
  expects_.StartObject("trailingUnderscoreSnakeCase")
      ->RenderInt64("trailingDoubleUnderscore", 2L)
      ->RenderInt64("trailingTripleUnderscoreFirstCap", 3L)
      ->RenderInt64("trailingQUADRUPLEUNDERSCOREALLCAP", 4L)
      ->EndObject();

  // Actual testing
  testing_.StartObject("trailing_underscore_snake_case")
      ->RenderInt64("trailing_double_underscore__", 2L)
      ->RenderInt64("Trailing_Triple_Underscore_First_Cap___", 3L)
      ->RenderInt64("TRAILING_QUADRUPLE_UNDERSCORE_ALL_CAP____", 4L)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, EnclosingUnderscoreSnakeCase) {
  // Sets expectation
  expects_.StartObject("enclosingUnderscoreSnakeCase")
      ->RenderUint64("enclosingDoubleUnderscore", 2L)
      ->RenderUint64("enclosingTripleUnderscoreFirstCap", 3L)
      ->RenderUint64("enclosingQUADRUPLEUNDERSCOREALLCAP", 4L)
      ->EndObject();

  // Actual testing
  testing_.StartObject("_enclosing_underscore_snake_case_")
      ->RenderUint64("__enclosing_double_underscore__", 2L)
      ->RenderUint64("___Enclosing_Triple_Underscore_First_Cap___", 3L)
      ->RenderUint64("____ENCLOSING_QUADRUPLE_UNDERSCORE_ALL_CAP____", 4L)
      ->EndObject();
}

TEST_F(Snake2CamelObjectWriterTest, DisableCaseNormalizationOnlyDisablesFirst) {
  // Sets expectation
  expects_.StartObject("")
      ->RenderString("snakeCase", "snake_case")
      ->RenderString(
            "the_quick_brown_fox_jumps_over_the_lazy_dog",  // case retained
            "the_quick_brown_fox_jumps_over_the_lazy_dog")
      ->RenderBool("theSlowFox", true)  // disable case not in effect
      ->EndObject();

  // Actual testing
  testing_.StartObject("")
      ->RenderString("snake_case", "snake_case")
      ->DisableCaseNormalizationForNextKey()
      ->RenderString("the_quick_brown_fox_jumps_over_the_lazy_dog",
                     "the_quick_brown_fox_jumps_over_the_lazy_dog")
      ->RenderBool("the_slow_fox", true)
      ->EndObject();
}

}  // namespace converter
}  // namespace util
}  // namespace protobuf
}  // namespace google
