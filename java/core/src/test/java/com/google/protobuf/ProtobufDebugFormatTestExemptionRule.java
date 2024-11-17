package com.google.protobuf;

import java.util.ArrayList;
import java.util.List;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.MultipleFailureException;
import org.junit.runners.model.Statement;

/**
 * An test rule that, combined with {@link ProtobufDebugFormatTestExemption}, can make Protobuf
 * Message.toString() to always output the Protobuf text format in select libraries for a test
 * method.
 *
 * <p>See more context in the comment of {@link ProtobufDebugFormatTestExemption}.
 */
public final class ProtobufDebugFormatTestExemptionRule implements TestRule {
  @Override
  public Statement apply(Statement base, Description description) {
    if (description.getAnnotation(ProtobufDebugFormatTestExemption.class) == null) {
      return base;
    }
    return statement(base);
  }

  private Statement statement(final Statement base) {
    return new Statement() {
      @Override
      public void evaluate() throws Throwable {

        List<Throwable> errors = new ArrayList<Throwable>();
        try {
          ProtobufToStringOutput.setDisableDebugFormatForTests(true);
          base.evaluate();
        } catch (Throwable t) {
          errors.add(t);
        } finally {
          ProtobufToStringOutput.setDisableDebugFormatForTests(false);
        }
        MultipleFailureException.assertEmpty(errors);
      }
    };
  }
}
