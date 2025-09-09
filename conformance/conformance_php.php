<?php

require_once('Conformance/WireFormat.php');
require_once('Conformance/ConformanceResponse.php');
require_once('Conformance/ConformanceRequest.php');
require_once('Conformance/FailureSet.php');
require_once('Conformance/JspbEncodingConfig.php');
require_once('Conformance/TestCategory.php');
require_once('Protobuf_test_messages/Proto3/ForeignMessage.php');
require_once('Protobuf_test_messages/Proto3/ForeignEnum.php');
require_once('Protobuf_test_messages/Proto3/TestAllTypesProto3.php');
require_once('Protobuf_test_messages/Proto3/TestAllTypesProto3/AliasedEnum.php');
require_once('Protobuf_test_messages/Proto3/TestAllTypesProto3/NestedMessage.php');
require_once('Protobuf_test_messages/Proto3/TestAllTypesProto3/NestedEnum.php');
require_once('Protobuf_test_messages/Editions/Proto3/EnumOnlyProto3/PBBool.php');
require_once('Protobuf_test_messages/Editions/Proto3/EnumOnlyProto3.php');
require_once('Protobuf_test_messages/Editions/Proto3/TestAllTypesProto3/NestedMessage.php');
require_once('Protobuf_test_messages/Editions/Proto3/TestAllTypesProto3/NestedEnum.php');
require_once('Protobuf_test_messages/Editions/Proto3/TestAllTypesProto3/AliasedEnum.php');
require_once('Protobuf_test_messages/Editions/Proto3/TestAllTypesProto3.php');
require_once('Protobuf_test_messages/Editions/Proto3/NullHypothesisProto3.php');
require_once('Protobuf_test_messages/Editions/Proto3/ForeignEnum.php');
require_once('Protobuf_test_messages/Editions/Proto3/ForeignMessage.php');
require_once('GPBMetadata/Conformance.php');
require_once('GPBMetadata/TestMessagesProto3.php');
require_once('GPBMetadata/TestMessagesProto3Editions.php');

use Conformance\ConformanceRequest;
use Conformance\ConformanceResponse;
use Conformance\TestCategory;
use Conformance\WireFormat;
use Protobuf_test_messages\Proto3\TestAllTypesProto3;
use Protobuf_test_messages\Editions\Proto3\TestAllTypesProto3 as TestAllTypesProto3Editions;

if (!ini_get('date.timezone')) {
    ini_set('date.timezone', 'UTC');
}

error_reporting(0);

$test_count = 0;

function doTest($request)
{
    $response = new ConformanceResponse();

    switch ($request->getPayload()) {
        case 'protobuf_payload':
            switch ($request->getMessageType()) {
                case 'protobuf_test_messages.proto3.TestAllTypesProto3':
                    $test_message = new TestAllTypesProto3();
                    break;
                case 'protobuf_test_messages.editions.proto3.TestAllTypesProto3':
                    $test_message = new TestAllTypesProto3Editions();
                    break;
                case 'conformance.FailureSet':
                    $response->setProtobufPayload('');
                    return $response;
                case 'protobuf_test_messages.proto2.TestAllTypesProto2':
                case 'protobuf_test_messages.editions.proto2.TestAllTypesProto2':
                    $response->setSkipped('PHP doesn\'t support proto2');
                    return $response;
                case 'protobuf_test_messages.editions.TestAllTypesEdition2023':
                    $response->setSkipped('PHP doesn\'t support editions-specific features yet');
                    return $response;
                case '':
                    trigger_error(
                        'Protobuf request doesn\'t have specific payload type',
                        E_USER_ERROR
                    );
                default:
                    trigger_error(sprintf(
                        'Protobuf request doesn\'t support %s message type',
                        $request->getMessageType(),
                    ), E_USER_ERROR);
            }

            try {
                $test_message->mergeFromString($request->getProtobufPayload());
            } catch (Exception $e) {
                $response->setParseError($e->getMessage());
                return $response;
            } catch (Error $e) {
                $response->setParseError($e->getMessage());
                return $response;
            }
            break;

        case 'json_payload':
            switch ($request->getMessageType()) {
                case 'protobuf_test_messages.editions.proto3.TestAllTypesProto3':
                    $test_message = new TestAllTypesProto3Editions();
                    break;
                case 'protobuf_test_messages.proto2.TestAllTypesProto2':
                case 'protobuf_test_messages.editions.proto2.TestAllTypesProto2':
                    $response->setSkipped('PHP doesn\'t support proto2');
                    return $response;
                default:
                    $test_message = new TestAllTypesProto3();
            }

            $ignore_json_unknown =
                ($request->getTestCategory() == TestCategory::JSON_IGNORE_UNKNOWN_PARSING_TEST);
            try {
                $test_message->mergeFromJsonString(
                    $request->getJsonPayload(),
                    $ignore_json_unknown
                );
            } catch (Exception $e) {
                $response->setParseError($e->getMessage());
                return $response;
            } catch (Error $e) {
                $response->setParseError($e->getMessage());
                return $response;
            }
            break;

        case 'text_payload':
            $response->setSkipped('PHP doesn\'t support text format yet');
            return $response;
        default:
            trigger_error('Request didn\'t have payload.', E_USER_ERROR);
    }

    switch ($request->getRequestedOutputFormat()) {
        case WireFormat::TEXT_FORMAT:
            $response->setSkipped('PHP doesn\'t support text format yet');
            return $response;
        case WireFormat::UNSPECIFIED:
            trigger_error('Unspecified output format.', E_USER_ERROR);
        case WireFormat::PROTOBUF:
            $response->setProtobufPayload($test_message->serializeToString());
            break;
        case WireFormat::JSON:
            try {
                $response->setJsonPayload($test_message->serializeToJsonString());
            } catch (Exception $e) {
                $response->setSerializeError($e->getMessage());
                return $response;
            } catch (Error $e) {
                $response->setSerializeError($e->getMessage());
                return $response;
            }
    }

    return $response;
}

function doTestIO()
{
    $length_bytes = fread(STDIN, 4);
    if (strlen($length_bytes) == 0) {
        return false;   # EOF
    } elseif (strlen($length_bytes) != 4) {
        fwrite(STDERR, "I/O error\n");
        return false;
    }

    $length = unpack('V', $length_bytes)[1];
    $serialized_request = fread(STDIN, $length);
    if (strlen($serialized_request) != $length) {
        trigger_error('I/O error', E_USER_ERROR);
    }

    $request = new ConformanceRequest();
    $request->mergeFromString($serialized_request);

    $response = doTest($request);

    $serialized_response = $response->serializeToString();
    fwrite(STDOUT, pack('V', strlen($serialized_response)));
    fwrite(STDOUT, $serialized_response);

    $GLOBALS['test_count'] += 1;

    return true;
}

while(true){
    if (!doTestIO()) {
        fprintf(STDERR,
            'conformance_php: received EOF from test runner ' .
            "after %d tests, exiting\n", $test_count);
        exit;
    }
}
