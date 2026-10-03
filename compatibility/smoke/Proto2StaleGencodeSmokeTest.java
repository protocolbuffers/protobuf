package smoke;

import static com.google.common.truth.Truth.assertThat;

import legacy_gencode_test.proto2.Proto2GencodeTestProto;
import legacy_gencode_test.proto2.Proto2GencodeTestProto.TestMostTypesProto2;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class Proto2StaleGencodeSmokeTest {
  @Test
  public void testProto2Extensions() throws Exception {
    com.google.protobuf.ExtensionRegistry registry = com.google.protobuf.ExtensionRegistry.newInstance();
    Proto2GencodeTestProto.registerAllExtensions(registry);

    TestMostTypesProto2.Builder b = TestMostTypesProto2.newBuilder();
    b.setExtension(Proto2GencodeTestProto.extensionInt32, 123);
    
    Proto2GencodeTestProto.ForeignMessage.Builder fb = Proto2GencodeTestProto.ForeignMessage.newBuilder();
    fb.setC(456);
    b.setExtension(Proto2GencodeTestProto.extensionMessage, fb.build());

    TestMostTypesProto2 msg = b.build();
    
    assertThat(msg.hasExtension(Proto2GencodeTestProto.extensionInt32)).isTrue();
    assertThat(msg.getExtension(Proto2GencodeTestProto.extensionInt32)).isEqualTo(123);
    assertThat(msg.hasExtension(Proto2GencodeTestProto.extensionMessage)).isTrue();
    assertThat(msg.getExtension(Proto2GencodeTestProto.extensionMessage).getC()).isEqualTo(456);

    byte[] bytes = msg.toByteArray();
    TestMostTypesProto2 parsed = TestMostTypesProto2.parseFrom(bytes, registry);
    
    assertThat(parsed.hasExtension(Proto2GencodeTestProto.extensionInt32)).isTrue();
    assertThat(parsed.getExtension(Proto2GencodeTestProto.extensionInt32)).isEqualTo(123);
    assertThat(parsed.hasExtension(Proto2GencodeTestProto.extensionMessage)).isTrue();
    assertThat(parsed.getExtension(Proto2GencodeTestProto.extensionMessage).getC()).isEqualTo(456);
  }
}
