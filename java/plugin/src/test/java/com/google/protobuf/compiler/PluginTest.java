package com.google.protobuf.compiler;

import com.google.protobuf.ByteString;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.compiler.CodeGenerator.GeneratorException;
import com.google.protobuf.compiler.PluginProtos.CodeGeneratorRequest;
import com.google.protobuf.compiler.PluginProtos.CodeGeneratorResponse;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import junit.framework.TestCase;

public class PluginTest extends TestCase {

  public void testParseGeneratorParameter() {
    Map<String, String> parsed;

    parsed = JavaHelpers.parseGeneratorParameter("");
    assertNotNull(parsed);
    assertTrue(parsed.isEmpty());

    parsed = JavaHelpers.parseGeneratorParameter(",,,");
    assertNotNull(parsed);
    assertTrue(parsed.isEmpty());

    // Simple value
    parsed = JavaHelpers.parseGeneratorParameter("foo");
    assertNotNull(parsed);
    assertEquals(1, parsed.size());
    assertEquals("", parsed.get("foo"));

    // Key=value pair
    parsed = JavaHelpers.parseGeneratorParameter("foo=bar");
    assertNotNull(parsed);
    assertEquals(1, parsed.size());
    assertEquals("bar", parsed.get("foo"));

    // list of key=value pairs
    parsed = JavaHelpers.parseGeneratorParameter("foo=bar,baz=quux");
    assertNotNull(parsed);
    assertEquals(2, parsed.size());
    assertEquals("bar", parsed.get("foo"));
    assertEquals("quux", parsed.get("baz"));

    // mixed list of simple values and key=value pair
    parsed = JavaHelpers.parseGeneratorParameter("foo,bar=baz,quux");
    assertNotNull(parsed);
    assertEquals(3, parsed.size());
    assertEquals("", parsed.get("foo"));
    assertEquals("baz", parsed.get("bar"));
    assertEquals("", parsed.get("quux"));

    // empty parts
    parsed = JavaHelpers.parseGeneratorParameter(",,,foo=bar,,baz=quux,,");
    assertNotNull(parsed);
    assertEquals(2, parsed.size());
    assertEquals("bar", parsed.get("foo"));
    assertEquals("quux", parsed.get("baz"));

    // duplicate keys, last value wins
    parsed = JavaHelpers.parseGeneratorParameter("foo=bar,baz,foo=quux");
    assertNotNull(parsed);
    assertEquals(2, parsed.size());
    assertEquals("quux", parsed.get("foo"));
    assertEquals("", parsed.get("baz"));
    assertFalse(parsed.containsValue("bar"));
  }

  public void testEnvironment() throws GeneratorException, InvalidProtocolBufferException {
    CodeGenerator generator = new CodeGenerator() {

      @Override
      public void generate(FileDescriptor file, String parameter,
          CodeGenerator.Context context)
          throws GeneratorException {
        context.addFile("foo", "bar");
        context.insertIntoFile("baz", "insertion point", "quux");
      }
    };

    CodeGeneratorRequest request = CodeGeneratorRequest.newBuilder()
      .addProtoFile(FileDescriptorProto.newBuilder().setName("dummy"))
      .addFileToGenerate("dummy").build();

    final InputStream stdIn = request.toByteString().newInput();
    final ByteString.Output stdOut = ByteString.newOutput();

    Plugin.run(generator, new Plugin.Environment() {

      @Override
      public OutputStream getOutputStream() {
        return stdOut;
      }

      @Override
      public InputStream getInputStream() {
        return stdIn;
      }
    });

    // Use the generator to build the expected response.
    CodeGeneratorResponse.Builder expected = CodeGeneratorResponse.newBuilder();
    generator.generate(null, null,
        new Plugin.CodeGeneratorResponseContext(
            Collections.<FileDescriptor>emptyList(), expected));

    assertEquals(expected.build(),
        CodeGeneratorResponse.parseFrom(stdOut.toByteString()));
  }

  public void testNoFileToGenerate() {
    CodeGenerator generator = new CodeGenerator() {

      @Override
      public void generate(FileDescriptor file, String parameter,
          CodeGenerator.Context context)
          throws GeneratorException {
        fail();
      }
    };

    CodeGeneratorRequest request = CodeGeneratorRequest.newBuilder().build();
    CodeGeneratorResponse.Builder response = CodeGeneratorResponse.newBuilder();

    Plugin.run(generator, request, response);

    assertEquals(0, response.getFileCount());
    assertFalse(response.hasError());
  }

  public void testDependency() {
    final Set<String> seenFiles = new HashSet<String>(2);
    CodeGenerator generator = new CodeGenerator() {

      @Override
      public void generate(FileDescriptor file, String parameter,
          CodeGenerator.Context context)
          throws GeneratorException {
        String fileName = file.getName();
        assertTrue("first".equals(fileName) || "second".equals(fileName));
        assertTrue("Generator called twice with the same file name",
            seenFiles.add(file.getName()));
      }
    };

    CodeGeneratorRequest.Builder request = CodeGeneratorRequest.newBuilder();
    request.addProtoFile(FileDescriptorProto.newBuilder().setName("first"));
    request.addProtoFile(FileDescriptorProto.newBuilder().setName("dependency"));
    request.addProtoFile(FileDescriptorProto.newBuilder().setName("second").addDependency(
        "dependency"));
    request.addFileToGenerate("first").addFileToGenerate("second");

    CodeGeneratorResponse.Builder response = CodeGeneratorResponse.newBuilder();

    Plugin.run(generator, request.build(), response);

    assertEquals(0, response.getFileCount());
    assertFalse(response.hasError());
    assertEquals(2, seenFiles.size());
  }

  public void testError() {
    CodeGenerator generator = new CodeGenerator() {

      @Override
      public void generate(FileDescriptor file, String parameter,
          CodeGenerator.Context context)
          throws GeneratorException {
        throw new GeneratorException("error");
      }
    };

    CodeGeneratorRequest request = CodeGeneratorRequest.newBuilder().addFileToGenerate(
        "foo").addProtoFile(FileDescriptorProto.newBuilder().setName("foo")).build();
    CodeGeneratorResponse.Builder response = CodeGeneratorResponse.newBuilder();

    Plugin.run(generator, request, response);

    assertEquals(0, response.getFileCount());
    assertTrue(response.hasError());
    assertEquals("error", response.getError());
  }
}
