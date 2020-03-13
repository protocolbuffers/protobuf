/**
 * @fileoverview The code size benchmark of binary kernel for accessing all
 * popular types setter and getter.
 *
 * The types are those whose usage are more than 1%:
 *
 * ('STRING__LABEL_OPTIONAL', '29.7214%')
 * ('INT32__LABEL_OPTIONAL', '17.7277%')
 * ('MESSAGE__LABEL_OPTIONAL', '15.6462%')
 * ('BOOL__LABEL_OPTIONAL', '13.0038%')
 * ('ENUM__LABEL_OPTIONAL', '11.4466%')
 * ('INT64__LABEL_OPTIONAL', '3.2198%')
 * ('DOUBLE__LABEL_OPTIONAL', '1.357%')
 * ('MESSAGE__LABEL_REPEATED', '1.2775%')
 * ('FIXED32__LABEL_REQUIRED', '1.2%')
 * ('UINT64__LABEL_OPTIONAL', '1.1771%')
 * ('STRING__LABEL_REQUIRED', '1.0785%')
 *
 */
goog.module('protobuf.benchmark.KernelCodeSizeBenchmarkPopularTypes');

const Int64 = goog.require('protobuf.Int64');
const Kernel = goog.require('protobuf.runtime.Kernel');
const TestMessage = goog.require('protobuf.testing.binary.TestMessage');
const {ensureCommonBaseLine} = goog.require('protobuf.benchmark.codeSize.codeSizeBase');

ensureCommonBaseLine();


/**
 * @return {string}
 */
function accessAllTypes() {
  const message = new TestMessage(Kernel.createEmpty());

  message.addRepeatedMessageElement(1, message, TestMessage.instanceCreator);
  message.addRepeatedMessageIterable(1, [message], TestMessage.instanceCreator);

  message.setString(1, 'abc');
  message.setInt32(1, 1);
  message.setMessage(1, message);
  message.setBool(1, true);
  message.setInt64(1, Int64.fromBits(0, 1));
  message.setDouble(1, 1.0);
  message.setRepeatedMessageElement(1, message, TestMessage.instanceCreator, 0);
  message.setRepeatedMessageIterable(1, [message]);
  message.setUint64(1, Int64.fromBits(0, 1));


  let s = '';
  s += message.getStringWithDefault(1);
  s += message.getInt32WithDefault(1);
  s += message.getMessage(1, TestMessage.instanceCreator);
  s += message.getMessageOrNull(1, TestMessage.instanceCreator);
  s += message.getBoolWithDefault(1);
  s += message.getInt64WithDefault(1);
  s += message.getDoubleWithDefault(1);
  s += message.getRepeatedMessageElement(1, TestMessage.instanceCreator, 0);
  s += message.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
  s += message.getRepeatedMessageSize(1, TestMessage.instanceCreator);
  s += message.getUint64WithDefault(1);

  s += message.serialize();

  return s;
}

goog.global['__hiddenTest'] += accessAllTypes();
