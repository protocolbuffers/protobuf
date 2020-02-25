/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.protobuf.kotlin

import com.google.common.truth.Truth.assertThat
import com.google.protobuf.Int32Value
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests to verify an example subclass of [OneofRepresentation] actually compiles. */
@RunWith(JUnit4::class)
class OneofRepresentationTest {
  enum class MyOneofCase {
    MY_INT, MY_STRING, MY_MESSAGE, NOT_SET
  }

  abstract class MyOneof<T : Any> : OneofRepresentation<T, MyOneofCase>() {
    class MyInt(override val value: Int) : MyOneof<Int>() {
      override val case: MyOneofCase
        get() = MyOneofCase.MY_INT
    }
    class MyString(override val value: String) : MyOneof<String>() {
      override val case: MyOneofCase
        get() = MyOneofCase.MY_STRING
    }
    class MyMessage(override val value: Int32Value) : MyOneof<Int32Value>() {
      override val case: MyOneofCase
        get() = MyOneofCase.MY_MESSAGE
    }
    object NotSet : MyOneof<Unit>() {
      override val value: Unit
        get() = Unit
      override val case: MyOneofCase
        get() = MyOneofCase.NOT_SET
    }
  }

  data class MyExample(
    private val myOneofCase: MyOneofCase,
    private val myInt: Int,
    private val myString: String,
    private val myMessage: Int32Value?
  ) {
    companion object {
      /** a faux "constructor" from the oneof */
      operator fun invoke(myOneof: MyOneof<*>): MyExample = when (myOneof) {
        is MyOneof.MyInt -> MyExample(MyOneofCase.MY_INT, myOneof.value, "", null)
        is MyOneof.MyString -> MyExample(MyOneofCase.MY_STRING, 0, myOneof.value, null)
        is MyOneof.MyMessage -> MyExample(MyOneofCase.MY_MESSAGE, 0, "", myOneof.value)
        else -> MyExample(MyOneofCase.NOT_SET, 0, "", null)
      }
    }

    val myOneof: MyOneof<*>
      get() = when (myOneofCase) {
        MyOneofCase.MY_INT -> MyOneof.MyInt(myInt)
        MyOneofCase.MY_STRING -> MyOneof.MyString(myString)
        MyOneofCase.MY_MESSAGE -> MyOneof.MyMessage(myMessage!!)
        else -> MyOneof.NotSet
      }
  }

  @Test
  fun myExample() {
    val someOneof = MyOneof.MyString("apple")
    assertThat(someOneof).isEqualTo(MyOneof.MyString("apple"))
    val myExample = MyExample(MyOneofCase.MY_STRING, 0, "apple", null)
    assertThat(MyExample(someOneof)).isEqualTo(myExample)
    assertThat(myExample.myOneof).isEqualTo(someOneof)
  }
}
