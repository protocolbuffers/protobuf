// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Test that Kokoro is using the expected version of Java.
import static com.google.common.truth.Truth.assertWithMessage;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class JavaVersionTest {
  @Test
  public void testJavaVersion() throws Exception {
    String exp = System.getenv("KOKORO_JAVA_VERSION");
    if(exp == null || exp.isEmpty()) {
      System.err.println("No kokoro java version found, skipping check");
      return;
    }
    // Java 8's version is read as "1.8"
    if (exp.equals("8")) exp = "1.8";
    String version = System.getProperty("java.version");
    assertWithMessage("Expected Java " + exp + " but found Java " + version)
        .that(version.startsWith(exp))
        .isTrue();
  }
}
