// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin.diamond

import kotlin.test.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/**
 * Regression test for Bazel Java One-Version classpath collisions in Kotlin proto generation.
 *
 * Verifies that when a test depends simultaneously on a leaf Kotlin proto library
 * (`:leaf_kt_proto`) and an exporter Kotlin proto library (`:exporter_kt_proto`, which re-exports
 * `:leaf_proto`), the Kotlin proto code generation does not duplicate class definitions with
 * differing Cyclic Redundancy Checks (CRCs) across multiple jars on the runtime classpath.
 */
@RunWith(JUnit4::class)
class DiamondDependencyTest {
  @Test
  fun testDiamondDependencyClasspath() {
    val leaf = leafMessage { value = "leaf" }
    assertEquals("leaf", leaf.value)

    val exporter = exporterMessage { value = "exporter" }
    assertEquals("exporter", exporter.value)
  }
}
