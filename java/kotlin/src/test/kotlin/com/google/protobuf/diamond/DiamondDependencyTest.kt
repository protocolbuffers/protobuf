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
