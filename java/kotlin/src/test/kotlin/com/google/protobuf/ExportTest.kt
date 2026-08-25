package com.google.protobuf.kotlin

import kotlin.test.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

@RunWith(JUnit4::class)
class ExportTest {
  @Test
  fun testExportedProtoMessageAccess() {
    val exportedMsg = exportedMessage { value = "exported" }
    assertEquals("exported", exportedMsg.value)

    val exporterMsg = exporterMessage { value = "exporter" }
    assertEquals("exporter", exporterMsg.value)
  }
}
