package com.google.protobuf.utf8validation;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.ExtensionLite;
import com.google.protobuf.ExtensionRegistryLite;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.MessageLite;
import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.function.BiFunction;
import java.util.function.Function;
import java.util.function.Predicate;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class Utf8ValidationGeneratedMessageTest {
  private static final boolean isLite =
      true

          // BEGIN FULL-RUNTIME
          && false

      // END FULL-RUNTIME
      ;

  enum Validates {
    NEVER,
    ALWAYS,
    FULL_RUNTIME_ONLY
  }

  private final Case<?> testCase;

  public Utf8ValidationGeneratedMessageTest(Case<?> testCase) {
    this.testCase = testCase;
  }

  @Parameters(name = "{0}")
  public static List<Case<?>> data() throws Exception {
    List<Case<?>> params = new ArrayList<>();

    addProto2(params);
    addProto3(params);
    addEditions2023(params);
    addEditions2024(params);

    return params;
  }

  private static void addProto2(List<Case<?>> params) {
    // java_string_check_utf8: {unset, false, true}
    // x enforce_utf8: {unset, false, true}
    // x {regular, extension}
    // TODO: should be 18 once our test proto is on the allowlist for
    // enforce_utf8=false
    final int casesToAdd = 12;
    final int casesBefore = params.size();

    // java_string_check_utf8: unset
    Case.create(
        params,
        "proto2 java_string_check_utf8=unset enforce_utf8=unset",
        Utf8TestProto2.FILE_UNSET_FIELD_UNSET_FIELD_NUMBER,
        Utf8TestProto2::parseFrom,
        Utf8TestProto2::hasFileUnsetFieldUnset,
        Utf8TestProto2::getFileUnsetFieldUnset,
        Validates.NEVER);

    Case.create(
        params,
        "proto2 java_string_check_utf8=unset enforce_utf8=true",
        Utf8TestProto2.FILE_UNSET_FIELD_ENFORCED_FIELD_NUMBER,
        Utf8TestProto2::parseFrom,
        Utf8TestProto2::hasFileUnsetFieldEnforced,
        Utf8TestProto2::getFileUnsetFieldEnforced,
        Validates.NEVER);

    Case.create(
        params,
        "proto2 java_string_check_utf8=unset enforce_utf8=unset extension",
        Utf8TestProto2Proto.extFileUnsetFieldUnset,
        Utf8TestProto2::parseFrom,
        Utf8TestProto2::hasExtension,
        Utf8TestProto2::getExtension,
        Validates.NEVER);

    Case.create(
        params,
        "proto2 java_string_check_utf8=unset enforce_utf8=true extension",
        Utf8TestProto2Proto.extFileUnsetFieldEnforced,
        Utf8TestProto2::parseFrom,
        Utf8TestProto2::hasExtension,
        Utf8TestProto2::getExtension,
        Validates.NEVER);

    // java_string_check_utf8: false

    Case.create(
        params,
        "proto2 java_string_check_utf8=false enforce_utf8=unset",
        Utf8TestProto2Unchecked.FILE_UNCHECKED_FIELD_UNSET_FIELD_NUMBER,
        Utf8TestProto2Unchecked::parseFrom,
        Utf8TestProto2Unchecked::hasFileUncheckedFieldUnset,
        Utf8TestProto2Unchecked::getFileUncheckedFieldUnset,
        Validates.NEVER);

    Case.create(
        params,
        "proto2 java_string_check_utf8=false enforce_utf8=true",
        Utf8TestProto2Unchecked.FILE_UNCHECKED_FIELD_ENFORCED_FIELD_NUMBER,
        Utf8TestProto2Unchecked::parseFrom,
        Utf8TestProto2Unchecked::hasFileUncheckedFieldEnforced,
        Utf8TestProto2Unchecked::getFileUncheckedFieldEnforced,
        Validates.NEVER);

    Case.create(
        params,
        "proto2 java_string_check_utf8=false enforce_utf8=unset extension",
        Utf8TestProto2UncheckedProto.extFileUncheckedFieldUnset,
        Utf8TestProto2Unchecked::parseFrom,
        Utf8TestProto2Unchecked::hasExtension,
        Utf8TestProto2Unchecked::getExtension,
        Validates.NEVER);

    Case.create(
        params,
        "proto2 java_string_check_utf8=false enforce_utf8=true extension",
        Utf8TestProto2UncheckedProto.extFileUncheckedFieldEnforced,
        Utf8TestProto2Unchecked::parseFrom,
        Utf8TestProto2Unchecked::hasExtension,
        Utf8TestProto2Unchecked::getExtension,
        Validates.NEVER);

    // java_string_check_utf8: true

    Case.create(
        params,
        "proto2 java_string_check_utf8=true enforce_utf8=unset",
        Utf8TestProto2Checked.FILE_CHECKED_FIELD_UNSET_FIELD_NUMBER,
        Utf8TestProto2Checked::parseFrom,
        Utf8TestProto2Checked::hasFileCheckedFieldUnset,
        Utf8TestProto2Checked::getFileCheckedFieldUnset,
        Validates.ALWAYS);

    Case.create(
        params,
        "proto2 java_string_check_utf8=true enforce_utf8=true",
        Utf8TestProto2Checked.FILE_CHECKED_FIELD_ENFORCED_FIELD_NUMBER,
        Utf8TestProto2Checked::parseFrom,
        Utf8TestProto2Checked::hasFileCheckedFieldEnforced,
        Utf8TestProto2Checked::getFileCheckedFieldEnforced,
        Validates.ALWAYS);

    Case.create(
        params,
        "proto2 java_string_check_utf8=true enforce_utf8=unset extension",
        Utf8TestProto2CheckedProto.extFileCheckedFieldUnset,
        Utf8TestProto2Checked::parseFrom,
        Utf8TestProto2Checked::hasExtension,
        Utf8TestProto2Checked::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "proto2 java_string_check_utf8=true enforce_utf8=true extension",
        Utf8TestProto2CheckedProto.extFileCheckedFieldEnforced,
        Utf8TestProto2Checked::parseFrom,
        Utf8TestProto2Checked::hasExtension,
        Utf8TestProto2Checked::getExtension,
        Validates.FULL_RUNTIME_ONLY);
    if (casesBefore + casesToAdd != params.size()) {
      throw new IllegalStateException(
          String.format("Expected %d cases but got %d", casesBefore + casesToAdd, params.size()));
    }
  }

  private static void addProto3(List<Case<?>> params) {
    // java_string_check_utf8: {unset, false, true}
    // x enforce_utf8: {unset, false, true}
    // No extensions for proto3.
    // TODO: should be 9 once our test proto is on the allowlist for
    // enforce_utf8=false
    final int casesToAdd = 6;
    final int casesBefore = params.size();

    // java_string_check_utf8: unset
    Case.create(
        params,
        "proto3 java_string_check_utf8=unset enforce_utf8=unset",
        Utf8TestProto3.FILE_UNSET_FIELD_UNSET_FIELD_NUMBER,
        Utf8TestProto3::parseFrom,
        Utf8TestProto3::hasFileUnsetFieldUnset,
        Utf8TestProto3::getFileUnsetFieldUnset,
        Validates.ALWAYS);

    Case.create(
        params,
        "proto3 java_string_check_utf8=unset enforce_utf8=true",
        Utf8TestProto3.FILE_UNSET_FIELD_ENFORCED_FIELD_NUMBER,
        Utf8TestProto3::parseFrom,
        Utf8TestProto3::hasFileUnsetFieldEnforced,
        Utf8TestProto3::getFileUnsetFieldEnforced,
        Validates.ALWAYS);

    // java_string_check_utf8: false
    Case.create(
        params,
        "proto3 java_string_check_utf8=false enforce_utf8=unset",
        Utf8TestProto3Unchecked.FILE_UNCHECKED_FIELD_UNSET_FIELD_NUMBER,
        Utf8TestProto3Unchecked::parseFrom,
        Utf8TestProto3Unchecked::hasFileUncheckedFieldUnset,
        Utf8TestProto3Unchecked::getFileUncheckedFieldUnset,
        Validates.ALWAYS);

    Case.create(
        params,
        "proto3 java_string_check_utf8=false enforce_utf8=true",
        Utf8TestProto3Unchecked.FILE_UNCHECKED_FIELD_ENFORCED_FIELD_NUMBER,
        Utf8TestProto3Unchecked::parseFrom,
        Utf8TestProto3Unchecked::hasFileUncheckedFieldEnforced,
        Utf8TestProto3Unchecked::getFileUncheckedFieldEnforced,
        Validates.ALWAYS);

    // java_string_check_utf8: true
    Case.create(
        params,
        "proto3 java_string_check_utf8=true enforce_utf8=unset",
        Utf8TestProto3Checked.FILE_CHECKED_FIELD_UNSET_FIELD_NUMBER,
        Utf8TestProto3Checked::parseFrom,
        Utf8TestProto3Checked::hasFileCheckedFieldUnset,
        Utf8TestProto3Checked::getFileCheckedFieldUnset,
        Validates.ALWAYS);

    Case.create(
        params,
        "proto3 java_string_check_utf8=true enforce_utf8=true",
        Utf8TestProto3Checked.FILE_CHECKED_FIELD_ENFORCED_FIELD_NUMBER,
        Utf8TestProto3Checked::parseFrom,
        Utf8TestProto3Checked::hasFileCheckedFieldEnforced,
        Utf8TestProto3Checked::getFileCheckedFieldEnforced,
        Validates.ALWAYS);
    if (casesBefore + casesToAdd != params.size()) {
      throw new IllegalStateException(
          String.format("Expected %d cases but got %d", casesBefore + casesToAdd, params.size()));
    }
  }

  private static void addEditions2023(List<Case<?>> params) {
    // features.utf8_validation: {unset, verify, none}
    // x features.(pb.java).utf8_validation: {unset, default, verify}
    // x {regular, extension}
    final int casesToAdd = 18;
    final int casesBefore = params.size();

    // features.utf8_validation: unset
    Case.create(
        params,
        "edition2023 features.utf8_validation=unset features.(pb.java).utf8_validation=unset",
        Utf8TestEdition2023.GLOBAL_UNSET_JAVA_UNSET_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalUnsetJavaUnset,
        Utf8TestEdition2023::getGlobalUnsetJavaUnset,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2023 features.utf8_validation=unset features.(pb.java).utf8_validation=DEFAULT",
        Utf8TestEdition2023.GLOBAL_UNSET_JAVA_DEFAULT_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalUnsetJavaDefault,
        Utf8TestEdition2023::getGlobalUnsetJavaDefault,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2023 features.utf8_validation=unset features.(pb.java).utf8_validation=VERIFY",
        Utf8TestEdition2023.GLOBAL_UNSET_JAVA_VERIFY_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalUnsetJavaVerify,
        Utf8TestEdition2023::getGlobalUnsetJavaVerify,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2023 features.utf8_validation=unset features.(pb.java).utf8_validation=unset"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalUnsetJavaUnset,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "edition2023 features.utf8_validation=unset features.(pb.java).utf8_validation=DEFAULT"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalUnsetJavaDefault,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "edition2023 features.utf8_validation=unset features.(pb.java).utf8_validation=VERIFY"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalUnsetJavaVerify,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    // features.utf8_validation: verify
    Case.create(
        params,
        "edition2023 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=unset",
        Utf8TestEdition2023.GLOBAL_VERIFY_JAVA_UNSET_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalVerifyJavaUnset,
        Utf8TestEdition2023::getGlobalVerifyJavaUnset,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2023 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=DEFAULT",
        Utf8TestEdition2023.GLOBAL_VERIFY_JAVA_DEFAULT_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalVerifyJavaDefault,
        Utf8TestEdition2023::getGlobalVerifyJavaDefault,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2023 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=VERIFY",
        Utf8TestEdition2023.GLOBAL_VERIFY_JAVA_VERIFY_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalVerifyJavaVerify,
        Utf8TestEdition2023::getGlobalVerifyJavaVerify,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2023 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=unset"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalVerifyJavaUnset,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "edition2023 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=DEFAULT"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalVerifyJavaDefault,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "edition2023 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=VERIFY"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalVerifyJavaVerify,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    // features.utf8_validation: none
    Case.create(
        params,
        "edition2023 features.utf8_validation=NONE features.(pb.java).utf8_validation=unset",
        Utf8TestEdition2023.GLOBAL_NONE_JAVA_UNSET_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalNoneJavaUnset,
        Utf8TestEdition2023::getGlobalNoneJavaUnset,
        Validates.NEVER);

    Case.create(
        params,
        "edition2023 features.utf8_validation=NONE features.(pb.java).utf8_validation=DEFAULT",
        Utf8TestEdition2023.GLOBAL_NONE_JAVA_DEFAULT_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalNoneJavaDefault,
        Utf8TestEdition2023::getGlobalNoneJavaDefault,
        Validates.NEVER);

    Case.create(
        params,
        "edition2023 features.utf8_validation=NONE features.(pb.java).utf8_validation=VERIFY",
        Utf8TestEdition2023.GLOBAL_NONE_JAVA_VERIFY_FIELD_NUMBER,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasGlobalNoneJavaVerify,
        Utf8TestEdition2023::getGlobalNoneJavaVerify,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2023 features.utf8_validation=NONE features.(pb.java).utf8_validation=unset"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalNoneJavaUnset,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.NEVER);

    Case.create(
        params,
        "edition2023 features.utf8_validation=NONE features.(pb.java).utf8_validation=DEFAULT"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalNoneJavaDefault,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.NEVER);

    Case.create(
        params,
        "edition2023 features.utf8_validation=NONE features.(pb.java).utf8_validation=VERIFY"
            + " extension",
        Utf8TestEdition2023Proto.extGlobalNoneJavaVerify,
        Utf8TestEdition2023::parseFrom,
        Utf8TestEdition2023::hasExtension,
        Utf8TestEdition2023::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    if (casesBefore + casesToAdd != params.size()) {
      throw new IllegalStateException(
          String.format("Expected %d cases but got %d", casesBefore + casesToAdd, params.size()));
    }
  }

  private static void addEditions2024(List<Case<?>> params) {
    // features.utf8_validation: {unset, verify, none}
    // x features.(pb.java).utf8_validation: {unset, default, verify}
    // x {regular, extension}
    final int casesToAdd = 18;
    final int casesBefore = params.size();

    // features.utf8_validation: unset
    Case.create(
        params,
        "edition2024 features.utf8_validation=unset features.(pb.java).utf8_validation=unset",
        Utf8TestEdition2024.GLOBAL_UNSET_JAVA_UNSET_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalUnsetJavaUnset,
        Utf8TestEdition2024::getGlobalUnsetJavaUnset,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2024 features.utf8_validation=unset features.(pb.java).utf8_validation=DEFAULT",
        Utf8TestEdition2024.GLOBAL_UNSET_JAVA_DEFAULT_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalUnsetJavaDefault,
        Utf8TestEdition2024::getGlobalUnsetJavaDefault,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2024 features.utf8_validation=unset features.(pb.java).utf8_validation=VERIFY",
        Utf8TestEdition2024.GLOBAL_UNSET_JAVA_VERIFY_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalUnsetJavaVerify,
        Utf8TestEdition2024::getGlobalUnsetJavaVerify,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2024 features.utf8_validation=unset features.(pb.java).utf8_validation=unset"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalUnsetJavaUnset,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "edition2024 features.utf8_validation=unset features.(pb.java).utf8_validation=DEFAULT"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalUnsetJavaDefault,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "edition2024 features.utf8_validation=unset features.(pb.java).utf8_validation=VERIFY"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalUnsetJavaVerify,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    // features.utf8_validation: verify
    Case.create(
        params,
        "edition2024 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=unset",
        Utf8TestEdition2024.GLOBAL_VERIFY_JAVA_UNSET_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalVerifyJavaUnset,
        Utf8TestEdition2024::getGlobalVerifyJavaUnset,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2024 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=DEFAULT",
        Utf8TestEdition2024.GLOBAL_VERIFY_JAVA_DEFAULT_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalVerifyJavaDefault,
        Utf8TestEdition2024::getGlobalVerifyJavaDefault,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2024 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=VERIFY",
        Utf8TestEdition2024.GLOBAL_VERIFY_JAVA_VERIFY_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalVerifyJavaVerify,
        Utf8TestEdition2024::getGlobalVerifyJavaVerify,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2024 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=unset"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalVerifyJavaUnset,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "edition2024 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=DEFAULT"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalVerifyJavaDefault,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    Case.create(
        params,
        "edition2024 features.utf8_validation=VERIFY features.(pb.java).utf8_validation=VERIFY"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalVerifyJavaVerify,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    // features.utf8_validation: none
    Case.create(
        params,
        "edition2024 features.utf8_validation=NONE features.(pb.java).utf8_validation=unset",
        Utf8TestEdition2024.GLOBAL_NONE_JAVA_UNSET_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalNoneJavaUnset,
        Utf8TestEdition2024::getGlobalNoneJavaUnset,
        Validates.NEVER);

    Case.create(
        params,
        "edition2024 features.utf8_validation=NONE features.(pb.java).utf8_validation=DEFAULT",
        Utf8TestEdition2024.GLOBAL_NONE_JAVA_DEFAULT_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalNoneJavaDefault,
        Utf8TestEdition2024::getGlobalNoneJavaDefault,
        Validates.NEVER);

    Case.create(
        params,
        "edition2024 features.utf8_validation=NONE features.(pb.java).utf8_validation=VERIFY",
        Utf8TestEdition2024.GLOBAL_NONE_JAVA_VERIFY_FIELD_NUMBER,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasGlobalNoneJavaVerify,
        Utf8TestEdition2024::getGlobalNoneJavaVerify,
        Validates.ALWAYS);

    Case.create(
        params,
        "edition2024 features.utf8_validation=NONE features.(pb.java).utf8_validation=unset"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalNoneJavaUnset,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.NEVER);

    Case.create(
        params,
        "edition2024 features.utf8_validation=NONE features.(pb.java).utf8_validation=DEFAULT"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalNoneJavaDefault,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.NEVER);

    Case.create(
        params,
        "edition2024 features.utf8_validation=NONE features.(pb.java).utf8_validation=VERIFY"
            + " extension",
        Utf8TestEdition2024Proto.extGlobalNoneJavaVerify,
        Utf8TestEdition2024::parseFrom,
        Utf8TestEdition2024::hasExtension,
        Utf8TestEdition2024::getExtension,
        Validates.FULL_RUNTIME_ONLY);

    if (casesBefore + casesToAdd != params.size()) {
      throw new IllegalStateException(
          String.format("Expected %d cases but got %d", casesBefore + casesToAdd, params.size()));
    }
  }

  @Test
  public void testGeneratedMessageUtf8Validation() throws Exception {
    byte[] serializedPayload = buildSerialized(testCase.fieldNumber);

    if (testCase.expectedToValidate()) {
      assertThrows(InvalidProtocolBufferException.class, () -> testCase.parse(serializedPayload));
    } else {
      MessageLite msg = testCase.parse(serializedPayload);
      assertTrue(testCase.has(msg));
      assertEquals("\uFFFD\uFFFD", testCase.get(msg));
    }
  }

  private static byte[] buildSerialized(int fieldNumber) throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    CodedOutputStream codedOut = CodedOutputStream.newInstance(out);
    codedOut.writeByteArray(fieldNumber, new byte[] {(byte) 0xC0, (byte) 0x80});
    codedOut.flush();
    return out.toByteArray();
  }

  private interface MessageParser<T extends MessageLite> {
    T parse(byte[] data, ExtensionRegistryLite registry) throws InvalidProtocolBufferException;
  }

  private static class Case<T extends MessageLite> {
    private final String label;
    private final int fieldNumber;
    private final ExtensionRegistryLite registry;
    private final MessageParser<T> parser;
    private final Predicate<T> hazzer;
    private final Function<T, String> getter;
    private final Validates expected;

    static <T extends MessageLite> void create(
        List<Case<?>> params,
        String syntax,
        int fieldNumber,
        MessageParser<T> parser,
        Predicate<T> hazzer,
        Function<T, String> getter,
        Validates expected) {
      params.add(
          new Case<T>(
              syntax,
              ExtensionRegistryLite.getEmptyRegistry(),
              fieldNumber,
              parser,
              hazzer,
              getter,
              expected));
    }

    static <T extends MessageLite> void create(
        List<Case<?>> params,
        String label,
        ExtensionLite<T, String> extensionDescriptor,
        MessageParser<T> parser,
        BiFunction<? super T, ExtensionLite<T, String>, Boolean> hazzer,
        BiFunction<? super T, ExtensionLite<T, String>, String> getter,
        Validates expected) {
      ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
      registry.add(extensionDescriptor);
      params.add(
          new Case<T>(
              label,
              registry,
              extensionDescriptor.getNumber(),
              parser,
              (T msg) -> hazzer.apply(msg, extensionDescriptor),
              (T msg) -> getter.apply(msg, extensionDescriptor),
              expected));
    }

    private Case(
        String label,
        ExtensionRegistryLite registry,
        int fieldNumber,
        MessageParser<T> parser,
        Predicate<T> hazzer,
        Function<T, String> getter,
        Validates expected) {
      this.label = label;
      this.registry = registry;
      this.fieldNumber = fieldNumber;
      this.parser = parser;
      this.hazzer = hazzer;
      this.getter = getter;
      this.expected = expected;
    }

    @Override
    public String toString() {
      return String.format(
          "%s-->%s", label, expectedToValidate() ? "validates" : "doesNotValidate");
    }

    T parse(byte[] data) throws InvalidProtocolBufferException {
      return parser.parse(data, registry);
    }

    boolean expectedToValidate() {
      return expected == Validates.ALWAYS || (!isLite && expected == Validates.FULL_RUNTIME_ONLY);
    }

    /**
     * @param msg must be of type {@code T} or test will fail anyway.
     */
    @SuppressWarnings("unchecked")
    boolean has(MessageLite msg) {
      return hazzer.test((T) msg);
    }

    /**
     * @param msg must be of type {@code T} or test will fail anyway.
     */
    @SuppressWarnings("unchecked")
    String get(MessageLite msg) {
      return getter.apply((T) msg);
    }
  }
}
