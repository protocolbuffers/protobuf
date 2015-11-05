package com.google.protobuf.compiler;

import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.compiler.CodeGenerator.Context;
import com.google.protobuf.compiler.CodeGenerator.GeneratorException;
import com.google.protobuf.compiler.PluginProtos.CodeGeneratorRequest;
import com.google.protobuf.compiler.PluginProtos.CodeGeneratorResponse;
import com.google.protobuf.compiler.PluginProtos.CodeGeneratorResponse.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.StringWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Front-end for protoc code generator plugins written in Java.
 * <p>
 * To implement a protoc plugin in Java, simply write an implementation of
 * {@link CodeGenerator}, then create a main() method like:
 * <pre><code>
 *   public static void main(String[] args) {
 *     Plugin.run(new MyCodeGenerator());
 *   }
 * </code></pre>
 * To get protoc to use the plugin, you'll have to create a launcher script:
 * <pre><code>
 *   #!/bin/sh
 *   cd `dirname $0`
 *   exec java -jar myplugin.jar
 * </code></pre>
 * On Windows, if it lives in the same folder as the plugin's JAR, it will
 * probably look like:
 * <pre><code>
 *   {@literal @}echo off
 *   java -jar "%~dp0\myplugin.jar"
 *   exit %errorlevel%
 * </code></pre>
 * You'll then have to do one of the following:
 * <ul>
 *   <li>Place the plugin binary somewhere in the {@code PATH} and give it the
 *       name {@code protoc-gen-NAME} (replacing {@code NAME} with the name of
 *       your plugin). If you then invoke protoc with the parameter
 *       {@code --NAME_out=OUT_DIR} (again, replace {@code NAME} with your
 *       plugin's name), protoc will invoke your plugin to generate the output,
 *       which will be placed in {@code OUT_DIR}.
 *   <li>Place the plugin binary anywhere, with any name, and pass the
 *       {@code --plugin} parameter to protoc to direct it to your plugin like
 *       so:
 *       <pre>
 *   protoc --plugin=protoc-gen-NAME=path/to/myscript --NAME_out=OUT_DIR
 *       </pre>
 *       On Windows, make sure to include the {@code .bat} suffix:
 *       <pre>
 *   protoc --plugin=protoc-gen-NAME=path/to/myscript.bat --NAME_out=OUT_DIR
 *       </pre>
 * </ul>
 * 
 * @author t.broyer@ltgt.net Thomas Broyer
 * <br>Based on the initial work of:
 * @author kenton@google.com Kenton Varda
 */
public final class Plugin {
  /**
   * Thrown when something went wrong in the plugin infrastructure.
   * <p>
   * This is an unrecoverable error. You shouldn't handle it.
   */
  public static class PluginException extends RuntimeException {
    private static final long serialVersionUID = 4028115971354639383L;

    PluginException(String message) {
      super(message);
    }

    PluginException(String message, Throwable cause) {
      super(message, cause);
    }

    PluginException(Throwable cause) {
      super(cause);
    }
  }

  /**
   * Provides access to the input and output streams used to communicate with
   * protoc.
   * 
   * @see DefaultEnvironment
   */
  public static interface Environment {
    /**
     * Returns the input stream to read the protoc code generation request
     * from.
     */
    InputStream getInputStream();
  
    /**
     * Returns the output stream to write the code generation response to.
     */
    OutputStream getOutputStream();
  }

  /**
   * An {@link Environment} giving access to the "standard" input and output
   * streams.
   */
  public static class DefaultEnvironment implements Environment {

    public InputStream getInputStream() {
      return System.in;
    }

    public OutputStream getOutputStream() {
      return System.out;
    }
  }

  static abstract class AbstractContext implements CodeGenerator.Context {

    private class GeneratorResponseFileWriter extends StringWriter {

      private final CodeGeneratorResponse.File.Builder file;

      protected GeneratorResponseFileWriter(
          CodeGeneratorResponse.File.Builder file) {
        super();
        this.file = file;
      }

      @Override
      public void close() throws IOException {
        super.close();
        file.setContent(this.toString());
        commitFile(file.build());
      }
    }

    private final List<FileDescriptor> parsedFiles;

    protected AbstractContext(List<FileDescriptor> parsedFiles) {
      this.parsedFiles = parsedFiles;
    }

    public void addFile(String filename, String content) {
      CodeGeneratorResponse.File.Builder file =
        CodeGeneratorResponse.File.newBuilder();
      file.setName(filename);
      file.setContent(content);
      try {
        commitFile(file.build());
      } catch (IOException e) {
        throw new PluginException("Error writing to stdout.", e);
      }
    }

    public void insertIntoFile(String filename, String insertionPoint,
        String content) {
      CodeGeneratorResponse.File.Builder file =
        CodeGeneratorResponse.File.newBuilder();
      file.setName(filename);
      file.setInsertionPoint(insertionPoint);
      file.setContent(content);
      try {
        commitFile(file.build());
      } catch (IOException e) {
        throw new PluginException("Error writing to stdout.", e);
      }
    }

    public Writer open(String filename) {
      CodeGeneratorResponse.File.Builder file =
        CodeGeneratorResponse.File.newBuilder();
      file.setName(filename);
      return new GeneratorResponseFileWriter(file);
    }

    public Writer openForInsert(String filename, String insertionPoint) {
      CodeGeneratorResponse.File.Builder file =
        CodeGeneratorResponse.File.newBuilder();
      file.setName(filename);
      file.setInsertionPoint(insertionPoint);
      return new GeneratorResponseFileWriter(file);
    }

    public List<FileDescriptor> getParsedFiles() {
      return parsedFiles;
    }

    protected abstract void commitFile(CodeGeneratorResponse.File file)
        throws IOException;
  }
  
  /**
   * A {@link CodeGenerator.Context} that streams generated files to a
   * {@link CodedOutputStream}.
   */
  static class StreamingContext extends AbstractContext {
    
    private final CodedOutputStream output;
    
    public StreamingContext(List<FileDescriptor> parsedFiles,
        CodedOutputStream output) {
      super(parsedFiles);
      this.output = output;
    }
    
    @Override
    protected void commitFile(File file) throws IOException {
      // Protocol format guarantees that concatenated messages are parsed as
      // if they had been merged in a single message prior to being serialized.
      CodeGeneratorResponse.newBuilder().addFile(file).build().writeTo(output);
      output.flush();
    }
  }
  
  /**
   * A {@link CodeGenerator.Context} that appends generated files to a
   * {@link CodeGeneratorResponse}.
   */
  static class CodeGeneratorResponseContext extends AbstractContext {

    private final CodeGeneratorResponse.Builder response;
    
    protected CodeGeneratorResponseContext(List<FileDescriptor> parsedFiles,
        CodeGeneratorResponse.Builder response) {
      super(parsedFiles);
      this.response = response;
    }
    
    @Override
    public void commitFile(File file) {
      response.addFile(file);
    }
  }

  private Plugin() { }

  /**
   * Runs the given code generator, reading the request from {@link System#in}
   * and writing the response to {@link System#out}.
   * <p>
   * This is equivalent to {@code run(generator, new DefaultEnvironment())}.
   * 
   * @see #run(CodeGenerator, Environment)
   */
  public static void run(CodeGenerator generator) throws PluginException {
    run(generator, new DefaultEnvironment());
  }

  /**
   * Runs the given code generator in the given environment.
   */
  public static void run(CodeGenerator generator, Environment environment)
  throws PluginException {
    CodeGeneratorRequest request;
    try {
      request = CodeGeneratorRequest.parseFrom(environment.getInputStream());
    } catch (IOException e) {
      throw new PluginException("protoc sent unparseable request to plugin.",
          e);
    }

    List<FileDescriptor> filesToGenerate = parseFiles(request);
    CodedOutputStream output = CodedOutputStream.newInstance(
        environment.getOutputStream());
    try {
      generate(generator, request.getParameter(),
          new StreamingContext(filesToGenerate, output));
    } catch (GeneratorException ge) {
      try {
        CodeGeneratorResponse.newBuilder().setError(ge.getMessage()).build()
          .writeTo(output);
        output.flush();
      } catch (IOException ioe) {
        throw new PluginException("Error writing to stdout.", ioe);
      }
    }
  }

  /**
   * Runs the generator with the given request and response.
   */
  public static void run(CodeGenerator generator, CodeGeneratorRequest request,
      CodeGeneratorResponse.Builder response) throws PluginException {

    List<FileDescriptor> filesToGenerate = parseFiles(request);
    try {
      generate(generator, request.getParameter(),
          new CodeGeneratorResponseContext(filesToGenerate, response));
    } catch (GeneratorException ge) {
      response.setError(ge.getMessage());
    }
  }
  
  /**
   * Runs the generator for each parsed file in the given context.
   */
  private static void generate(CodeGenerator generator, String parameter,
      Context context) throws GeneratorException {

    for (FileDescriptor fileToGenerate : context.getParsedFiles()) {
      generator.generate(fileToGenerate, parameter, context);
    }
  }

  /**
   * Parse the request's proto files and returns the list of parsed descriptors
   * corresponding only to the files to generate (i.e. dependencies not listed
   * explicitly are not included in the returned list).
   */
  private static List<FileDescriptor> parseFiles(CodeGeneratorRequest request)
      throws PluginException {
    Map<String, FileDescriptor> filesByName =
      new HashMap<String, FileDescriptor>(request.getProtoFileCount());
    for (FileDescriptorProto protoFile : request.getProtoFileList()) {
      FileDescriptor[] dependencies =
        new FileDescriptor[protoFile.getDependencyCount()];
      for (int i = 0, l = protoFile.getDependencyCount(); i < l; i++) {
        FileDescriptor dependency = filesByName.get(
            protoFile.getDependency(i));
        if (dependency == null) {
          throw new PluginException("protoc asked plugin to generate a file "
              + "but did not provide a descriptor for a dependency (or "
              + "provided it after the file that depends on it): "
              + protoFile.getDependency(i));
        }
        dependencies[i] = dependency;
      }
      try {
        filesByName.put(protoFile.getName(), FileDescriptor.buildFrom(
            protoFile, dependencies));
      } catch (DescriptorValidationException e) {
        throw new PluginException(e);
      }
    }

    List<FileDescriptor> filesToGenerate = new ArrayList<FileDescriptor>(
        request.getFileToGenerateCount());
    for (String fileToGenerate : request.getFileToGenerateList()) {
      FileDescriptor file = filesByName.get(fileToGenerate);
      if (file == null) {
        throw new PluginException("protoc asked plugin to generate a file "
            + "but did not provide a descriptor for the file: "
            + fileToGenerate);
      }

      filesToGenerate.add(file);
    }
    
    return filesToGenerate;
  }
}
