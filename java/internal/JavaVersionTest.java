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
    String version = System.getProperty("java.version");
    assertWithMessage("Expected Python " + exp + " but found Python " + version)
      .that(version.startsWith(exp))
      .isTrue();
  }
}
