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

package com.google.protobuf.kotlin.protoc

import com.google.common.truth.Truth.assertThat
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.squareup.kotlinpoet.FileSpec
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [Declarations]. */
@RunWith(JUnit4::class)
class DeclarationsTest {
  private val property = PropertySpec.builder("someProperty", INT).build()
  private val function = FunSpec.builder("someFunction").build()
  private val type = TypeSpec.objectBuilder("MyObject").build()

  private inline fun someFile(block: FileSpec.Builder.() -> Unit): String {
    return FileSpec
      .builder("com.foo.bar", "SomeFile.kt")
      .apply(block)
      .build()
      .toString()
      .trim()
  }

  @Test
  fun writeAllAtTopLevel() {
    val decls = declarations {
      addTopLevelProperty(property)
      addFunction(function)
    }
    assertThat(
      someFile {
        decls.writeAllAtTopLevel(this)
      }
    ).isEqualTo(
      """
      package com.foo.bar

      import kotlin.Int

      val someProperty: Int

      fun someFunction() {
      }
      """.trimIndent()
    )
  }

  @Test
  fun writeOnlyTopLevel() {
    val decls = declarations {
      addTopLevelProperty(property)
      addFunction(function)
    }
    assertThat(
      someFile {
        decls.writeOnlyTopLevel(this)
      }
    ).isEqualTo(
      """
      package com.foo.bar

      import kotlin.Int

      val someProperty: Int
      """.trimIndent()
    )
  }

  private fun FileSpec.Builder.writeToSomeEnclosingObject(decls: Declarations) {
    addType(
      TypeSpec.objectBuilder("SomeObject")
        .apply { decls.writeToEnclosingType(this) }
        .build()
    )
  }

  @Test
  fun writeToEnclosingType() {
    val decls = declarations {
      addTopLevelProperty(property)
      addFunction(function)
    }
    assertThat(
      someFile {
        writeToSomeEnclosingObject(decls)
      }
    ).isEqualTo(
      """
      package com.foo.bar

      object SomeObject {
        fun someFunction() {
        }
      }
      """.trimIndent()
    )
  }

  @Test
  fun hasTopLevel() {
    assertThat(
      declarations {
        addTopLevelProperty(property)
      }.hasTopLevelDeclarations
    ).isTrue()
    assertThat(
      declarations {
        addProperty(property)
      }.hasTopLevelDeclarations
    ).isFalse()
  }

  @Test
  fun hasEnclosingScopeDeclarations() {
    assertThat(
      declarations {
        addTopLevelProperty(property)
      }.hasEnclosingScopeDeclarations
    ).isFalse()
    assertThat(
      declarations {
        addProperty(property)
      }.hasEnclosingScopeDeclarations
    ).isTrue()
  }

  @Test
  fun addTopLevelProperty() {
    val decls = declarations {
      addTopLevelProperty(property)
    }
    assertThat(decls).generatesTopLevel(
      """
      import kotlin.Int

      val someProperty: Int
    """
    )
    assertThat(decls).generatesNoEnclosedMembers()
  }

  @Test
  fun addTopLevelFunction() {
    val decls = declarations {
      addTopLevelFunction(function)
    }
    assertThat(decls)
      .generatesTopLevel(
        """
        fun someFunction() {
        }
        """
      )
    assertThat(decls).generatesNoEnclosedMembers()
  }

  @Test
  fun addTopLevelType() {
    val decls = declarations {
      addTopLevelType(type)
    }
    assertThat(decls).generatesTopLevel("object MyObject")
    assertThat(decls).generatesNoEnclosedMembers()
  }
}
