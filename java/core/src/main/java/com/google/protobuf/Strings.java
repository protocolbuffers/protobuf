package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.logging.Logger;

@SuppressWarnings({"AndroidJdkLibsChecker", "Java8ApiChecker"})
final class Strings {

  private static final Logger logger = Logger.getLogger(Strings.class.getName());

  private static interface Latin1CodedInputStreamFactory {
    CodedInputStream newLatin1CodedInputStream(String[] strings);
  }

  private static byte[] getLatin1Bytes(String string) {
    final int length = string.length();
    byte[] bytes = new byte[length];
    getLatin1Bytes(string, length, bytes, 0);
    return bytes;
  }

  @SuppressWarnings("deprecation") // We actually want the deprecated behavior.
  private static void getLatin1Bytes(String string, int length, byte[] dst, int dstBegin) {
    string.getBytes(0, length, dst, dstBegin);
  }

  static final class FallbackLatin1CodedInputStreamFactory
      implements Latin1CodedInputStreamFactory {

    @Override
    public CodedInputStream newLatin1CodedInputStream(String[] strings) {
      if (strings.length == 1) {
        CodedInputStream input =
            CodedInputStream.newInstance(getLatin1Bytes(strings[0]), /* bufferIsImmutable= */ true);
        input.enableAliasing(false);
        return input;
      }
      // Cheat using the deprecated method.
      int totalLength = 0;
      for (int i = 0; i < strings.length; ++i) {
        totalLength += strings[i].length();
      }
      byte[] bytes = new byte[totalLength];
      totalLength = 0;
      for (int i = 0; i < strings.length; ++i) {
        final int length = strings[i].length();
        getLatin1Bytes(strings[i], length, bytes, totalLength);
        totalLength += length;
      }
      CodedInputStream input = CodedInputStream.newInstance(bytes, /* bufferIsImmutable= */ true);
      input.enableAliasing(false);
      return input;
    }
  }

  abstract static class UnsafeLatin1CodedInputStreamFactory
      implements Latin1CodedInputStreamFactory {

    private final byte latin1;

    private UnsafeLatin1CodedInputStreamFactory(byte latin1) {
      this.latin1 = latin1;
    }

    @Override
    public final CodedInputStream newLatin1CodedInputStream(String[] strings) {
      if (strings.length == 1) {
        CodedInputStream input;
        if (getCoder(strings[0]) != latin1) {
          input =
              CodedInputStream.newInstance(
                  getLatin1Bytes(strings[0]), /* bufferIsImmutable= */ true);
          input.enableAliasing(false);
        } else {
          input = CodedInputStream.newInstance(getValue(strings[0]), /* bufferIsImmutable= */ true);
          input.enableAliasing(true);
        }
        return input;
      }
      int totalLength = 0;
      boolean allLatin1 = true;
      for (int i = 0; i < strings.length; ++i) {
        String string = strings[i];
        totalLength += string.length();
        allLatin1 = allLatin1 && getCoder(string) == latin1;
      }
      if (allLatin1) {
        ByteBuffer[] buffers = new ByteBuffer[strings.length];
        for (int i = 0; i < strings.length; ++i) {
          buffers[i] = ByteBuffer.wrap(getValue(strings[i]));
        }
        CodedInputStream input =
            CodedInputStream.newInstance(new IterableByteBufferInputStream(Arrays.asList(buffers)));
        input.enableAliasing(true);
        return input;
      }
      byte[] bytes = new byte[totalLength];
      totalLength = 0;
      for (int i = 0; i < strings.length; ++i) {
        final int length = strings[i].length();
        getLatin1Bytes(strings[i], length, bytes, totalLength);
        totalLength += length;
      }
      CodedInputStream input = CodedInputStream.newInstance(bytes, /* bufferIsImmutable= */ true);
      input.enableAliasing(false);
      return input;
    }

    abstract byte getCoder(String string);

    abstract byte[] getValue(String string);
  }

  @SuppressWarnings("unused")
  private static final class ReflectionLatin1CodedInputStreamFactory
      extends UnsafeLatin1CodedInputStreamFactory {

    private final Field valueField;
    private final Field coderField;

    private ReflectionLatin1CodedInputStreamFactory(
        Field valueField, Field coderField, byte latin1) {
      super(latin1);
      this.valueField = checkNotNull(valueField);
      this.coderField = checkNotNull(coderField);
    }

    @Override
    byte getCoder(String string) {
      try {
        return coderField.getByte(checkNotNull(string));
      } catch (ReflectiveOperationException e) {
        throw new RuntimeException(
            "Failed to reflectively access java.lang.String." + coderField.getName(), e);
      }
    }

    @Override
    byte[] getValue(String string) {
      try {
        return (byte[]) valueField.get(checkNotNull(string));
      } catch (ReflectiveOperationException e) {
        throw new RuntimeException(
            "Failed to reflectively access java.lang.String." + valueField.getName(), e);
      }
    }
  }

  private static final Latin1CodedInputStreamFactory latin1CodedInputStreamFactory;

  static {
    Latin1CodedInputStreamFactory factory;
    if (Android.assumeLiteRuntime) {
      factory = new FallbackLatin1CodedInputStreamFactory();
    } else {
      final Class<?> clazz = String.class;
      try {
        Field compactStringsField = clazz.getDeclaredField("COMPACT_STRINGS");
        compactStringsField.setAccessible(true);
        if (!Modifier.isStatic(compactStringsField.getModifiers())) {
          throw new IllegalStateException(
              "Expected "
                  + clazz.getSimpleName()
                  + "."
                  + compactStringsField.getName()
                  + " to be static");
        }
        if (compactStringsField.getType() != boolean.class) {
          throw new IllegalStateException(
              "Expected "
                  + clazz.getSimpleName()
                  + "."
                  + compactStringsField.getName()
                  + " to be "
                  + boolean.class);
        }
        if (compactStringsField.getBoolean(null)) {
          Field latin1Field = clazz.getDeclaredField("LATIN1");
          latin1Field.setAccessible(true);
          if (!Modifier.isStatic(latin1Field.getModifiers())) {
            throw new IllegalStateException(
                "Expected "
                    + clazz.getSimpleName()
                    + "."
                    + latin1Field.getName()
                    + " to be static");
          }
          if (latin1Field.getType() != byte.class) {
            throw new IllegalStateException(
                "Expected "
                    + clazz.getSimpleName()
                    + "."
                    + latin1Field.getName()
                    + " to be "
                    + byte.class);
          }
          byte latin1 = latin1Field.getByte(null);
          Field valueField = clazz.getDeclaredField("value");
          valueField.setAccessible(true);
          if (Modifier.isStatic(valueField.getModifiers())) {
            throw new IllegalStateException(
                "Expected "
                    + clazz.getSimpleName()
                    + "."
                    + valueField.getName()
                    + " to not be static");
          }
          if (valueField.getType() != byte[].class) {
            throw new IllegalStateException(
                "Expected "
                    + clazz.getSimpleName()
                    + "."
                    + valueField.getName()
                    + " to be "
                    + byte[].class);
          }
          Field coderField = clazz.getDeclaredField("coder");
          coderField.setAccessible(true);
          if (Modifier.isStatic(coderField.getModifiers())) {
            throw new IllegalStateException(
                "Expected "
                    + clazz.getSimpleName()
                    + "."
                    + coderField.getName()
                    + " to not be static");
          }
          if (coderField.getType() != byte.class) {
            throw new IllegalStateException(
                "Expected "
                    + clazz.getSimpleName()
                    + "."
                    + coderField.getName()
                    + " to be "
                    + byte.class);
          }
          factory = new ReflectionLatin1CodedInputStreamFactory(valueField, coderField, latin1);
        } else {
          factory = new FallbackLatin1CodedInputStreamFactory();
        }
      } catch (ReflectiveOperationException | RuntimeException e) {
        StringWriter stringWriter = new StringWriter();
        PrintWriter printWriter = new PrintWriter(stringWriter);
        e.printStackTrace(printWriter);
        logger.warning(
            "Failed to refectively gain access to fields in java.lang.String. Falling back to slow"
                + " path.\n"
                + stringWriter.toString());
        factory = new FallbackLatin1CodedInputStreamFactory();
      }
    }
    latin1CodedInputStreamFactory = factory;
  }

  static Latin1CodedInputStreamFactory getLatin1CodedInputStreamFactory() {
    return latin1CodedInputStreamFactory;
  }

  /**
   * Returns a {@link CodedInputStream} overtop of the string literals. This should only be used for
   * string constants emitted by protoc representing serialized {@link
   * com.google.protobuf.DescriptorProtos.FileDescriptorProto}.
   */
  static CodedInputStream newLatin1CodedInputStream(String[] strings) {
    if (strings.length == 0) {
      throw new IllegalArgumentException("Expected at least one element in the array");
    }
    return getLatin1CodedInputStreamFactory().newLatin1CodedInputStream(strings);
  }

  private Strings() {}
}
