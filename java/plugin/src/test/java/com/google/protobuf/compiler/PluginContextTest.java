package com.google.protobuf.compiler;

import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.compiler.Plugin.AbstractContext;
import com.google.protobuf.compiler.Plugin.CodeGeneratorResponseContext;
import com.google.protobuf.compiler.Plugin.StreamingContext;
import com.google.protobuf.compiler.PluginProtos.CodeGeneratorResponse;
import java.io.Writer;
import java.util.Collections;
import junit.framework.TestCase;
import org.easymock.classextension.EasyMock;
import org.easymock.classextension.IMocksControl;

public class PluginContextTest extends TestCase {

  private static final String FIRST_FILE_NAME = "file1";
  private static final String FIRST_FILE_CONTENT = "file1 content";
  private static final String SECOND_FILE_NAME = "file2";
  private static final String SECOND_FILE_CONTENT = "file2 content";
  private static final String FIRST_INSERTION_POINT = "insertion point 1";
  private static final String SECOND_INSERTION_POINT = "insertion point 2";
  
  private static final CodeGeneratorResponse.File FIRST_FILE =
    CodeGeneratorResponse.File.newBuilder().setName(FIRST_FILE_NAME)
    .setContent(FIRST_FILE_CONTENT).build();
  private static final CodeGeneratorResponse.File SECOND_FILE =
    CodeGeneratorResponse.File.newBuilder().setName(SECOND_FILE_NAME)
    .setContent(SECOND_FILE_CONTENT).build();
  private static final CodeGeneratorResponse.File FIRST_INSERT =
    CodeGeneratorResponse.File.newBuilder(FIRST_FILE)
    .setInsertionPoint(FIRST_INSERTION_POINT).build();
  private static final CodeGeneratorResponse.File SECOND_INSERT =
    CodeGeneratorResponse.File.newBuilder(SECOND_FILE)
    .setInsertionPoint(SECOND_INSERTION_POINT).build();


  private IMocksControl control;
  private AbstractContext mockContext;
  
  @Override
  protected void setUp() throws Exception {
    control = EasyMock.createStrictControl();
    mockContext = EasyMock.createMockBuilder(AbstractContext.class)
      .withConstructor(Collections.<FileDescriptor>emptyList())
      .addMockedMethod("commitFile")
      .createMock(control);
  }

  public void testAddFile() throws Exception {
    mockContext.commitFile(EasyMock.eq(FIRST_FILE));
    mockContext.commitFile(EasyMock.eq(SECOND_FILE));
    control.replay();
    
    mockContext.addFile(FIRST_FILE_NAME, FIRST_FILE_CONTENT);
    mockContext.addFile(SECOND_FILE_NAME, SECOND_FILE_CONTENT);
    
    control.verify();
  }

  public void testInsertIntoFile() throws Exception {
    mockContext.commitFile(EasyMock.eq(FIRST_INSERT));
    mockContext.commitFile(EasyMock.eq(SECOND_INSERT));
    control.replay();
    
    mockContext.insertIntoFile(FIRST_FILE_NAME, FIRST_INSERTION_POINT,
        FIRST_FILE_CONTENT);
    mockContext.insertIntoFile(SECOND_FILE_NAME, SECOND_INSERTION_POINT,
        SECOND_FILE_CONTENT);
    
    control.verify();
  }

  public void testOpen() throws Exception {
    mockContext.commitFile(EasyMock.eq(FIRST_FILE));
    control.replay();

    Writer w = mockContext.open(FIRST_FILE_NAME);
    w.append(FIRST_FILE_CONTENT);
    w.close();

    control.verify();
  }

  public void testOpenForInsert() throws Exception {
    mockContext.commitFile(EasyMock.eq(FIRST_INSERT));
    control.replay();

    Writer w = mockContext.openForInsert(FIRST_FILE_NAME,
        FIRST_INSERTION_POINT);
    w.write(FIRST_FILE_CONTENT);
    w.close();
    
    control.verify();
  }

  // Don't prevent generating duplicate files (protoc will generate an error),
  // but at least don't step on each other.
  public void testDuplicateFiles() throws Exception {
    CodeGeneratorResponse.File secondFile =
      CodeGeneratorResponse.File.newBuilder().setName(FIRST_FILE_NAME)
      .setContent(SECOND_FILE_CONTENT).build();

    mockContext.commitFile(EasyMock.eq(FIRST_FILE));
    mockContext.commitFile(EasyMock.eq(secondFile));
    control.replay();
    
    Writer w1 = mockContext.open(FIRST_FILE_NAME);
    Writer w2 = mockContext.open(FIRST_FILE_NAME);
    assertNotSame(w1, w2);
    w1.write(FIRST_FILE_CONTENT);
    w2.write(SECOND_FILE_CONTENT);
    w1.close();
    w2.close();

    control.verify();
  }
  
  public void testStreamingContext() throws Exception {
    CodeGeneratorResponse expected = CodeGeneratorResponse.newBuilder()
      .addFile(FIRST_FILE).build();
    byte[] actual = new byte[expected.getSerializedSize()];
    CodedOutputStream output = CodedOutputStream.newInstance(actual);
    
    StreamingContext context = new StreamingContext(
        Collections.<FileDescriptor>emptyList(), output);
    context.addFile(FIRST_FILE_NAME, FIRST_FILE_CONTENT);
    output.flush();

    assertEquals(expected, CodeGeneratorResponse.parseFrom(actual));
  }
  
  public void testCodeGeneratorResponseContext() {
    CodeGeneratorResponse expectedResponse =
      CodeGeneratorResponse.newBuilder().addFile(FIRST_FILE).build();
    CodeGeneratorResponse.Builder actualResponse =
      CodeGeneratorResponse.newBuilder();
    
    CodeGeneratorResponseContext context = new CodeGeneratorResponseContext(
        Collections.<FileDescriptor>emptyList(), actualResponse);
    context.addFile(FIRST_FILE_NAME, FIRST_FILE_CONTENT);
    
    assertEquals(expectedResponse, actualResponse.build());
  }
}
