package com.google.protobuf.compiler;

import com.google.protobuf.Descriptors.FileDescriptor;
import java.io.Writer;
import java.util.List;

/**
 * Interface to be implemented by a code generator.
 * 
 * @see Plugin
 * 
 * @author t.broyer@ltgt.net Thomas Broyer
 * <br>Based on the initial work of:
 * @author kenton@google.com Kenton Varda
 */
public interface CodeGenerator {

  /**
   * Context for a given file generation.
   * <p>
   * Gives access to the full list of files to be generated (when used as a
   * protoc plugin, these are the files listed on the command-line) and methods
   * to output the generated files (or code snippets to be inserted into
   * existing files).
   * 
   * @author t.broyer@ltgt.net Thomas Broyer
   * <br>Based on the initial work of:
   * @author kenton@google.com Kenton Varda
   */
  public interface Context {
    /**
     * Returns the files for which the generator needs to generate code.
     * <p>
     * Most generators do not need to call {@code getParsedFiles()}.
     * <p>
     * The generator's {@link
     * CodeGenerator#generate(FileDescriptor, String, Context)} method will be
     * called once for each file in the list.
     * <p>
     * The list ordering is guaranteed to be stable between the calls to {@link
     * CodeGenerator#generate(FileDescriptor, String, Context)} so a generator
     * can, for instance, determine whether it is processing the first file.
     * This is useful when you want to generate a single file for a given
     * context (generating the file in every {@link
     * CodeGenerator#generate(FileDescriptor, String, Context)} call would lead
     * to a "duplicate file" error.
     */
    List<FileDescriptor> getParsedFiles();
    
    /**
     * Generates a new file with the given content.
     */
    void addFile(String filename, String content);
  
    /**
     * Inserts the given content into an existing file.
     */
    void insertIntoFile(String filename, String insertionPoint,
        String content);
  
    /**
     * Returns a {@link Writer} to generate a new file.
     * <p>
     * The filename given should be relative to the root of the source tree.
     * E.g. the C++ generator, when generating code for "foo/bar.proto", will
     * generate the files "foo/bar.pb.h" and "foo/bar.pb.cc"; note that "foo/"
     * is included in these filenames. The filename is not allowed to contain
     * "." or ".." components.
     * <p>
     * It is the caller's responsibility to commit the file by calling the
     * {@link Writer#close()} method on the returned {@link Writer}.
     */
    Writer open(String filename);
  
    /**
     * Returns a {@link Writer} to insert into an existing file.
     * <p>
     * The filename given should be relative to the root of the source tree.
     * E.g. the C++ generator, when generating code for "foo/bar.proto", will
     * generate the files "foo/bar.pb.h" and "foo/bar.pb.cc"; note that "foo/"
     * is included in these filenames. The filename is not allowed to contain
     * "." or ".." components.
     * <p>
     * It is the caller's responsibility to commit the file by calling the
     * {@link Writer#close()} method on the returned {@link Writer}.
     */
    Writer openForInsert(String filename, String insertionPoint);
  }

  /**
   * Throw by a code generator in case of error.
   *
   * @author t.broyer@ltgt.net Thomas Broyer
   * <br>Based on the initial work of:
   * @author kenton@google.com Kenton Varda
   */
  public static class GeneratorException extends Exception {
    private static final long serialVersionUID = -4317369502082361194L;

    public GeneratorException(FileDescriptor fileToGenerate, String message) {
      this(fileToGenerate.getName() + ": " + message);
    }

    public GeneratorException(FileDescriptor fileToGenerate, String message,
        Throwable cause) {
      this(fileToGenerate.getName() + ": " + message, cause);
    }

    public GeneratorException(String message) {
      super(message);
    }

    public GeneratorException(String message, Throwable cause) {
      super(message, cause);
    }
    
    public GeneratorException(Throwable cause) {
      super(cause);
    }
  }
  
  /**
   * Generates code for the given proto file, generating one or more files in
   * the given context.
   */
  void generate(FileDescriptor fileToGenerate, String parameter,
      Context context) throws GeneratorException;
}
