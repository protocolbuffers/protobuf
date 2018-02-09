<?hh

namespace Google\Protobuf\Internal {

<<__NativeData("Google\\Protobuf\\Internal\\Message")>>
class Message {
  <<__Native>>
  public function __construct(): void;
  <<__Native>>
  public function serializeToString(): string;
  <<__Native>>
  public function mergeFromString(string $data): void;
}

<<__NativeData("Google\\Protobuf\\Internal\\MapField")>>
class MapField implements \ArrayAccess, \Countable {
  <<__Native>>
  public function __construct(
      int $key_type, int $value_type, ?classname<Message> $klass = null): void;
  <<__Native>>
  public function offsetExists(mixed $index): bool;
  <<__Native>>
  public function offsetGet(mixed $index): mixed;
  <<__Native>>
  public function offsetSet(mixed $index,
                            mixed $newvalue): void;
  <<__Native>>
  public function offsetUnset(mixed $index): void;
  <<__Native>>
  public function count(): int;
  <<__Native>>
  public function append(mixed $newvalue): void;
}

<<__NativeData("Google\\Protobuf\\Internal\\RepeatedField")>>
class RepeatedField implements \ArrayAccess, \Countable {
  <<__Native>>
  public function __construct(int $type, ?classname<Message> $klass = null ): void;
  <<__Native>>
  public function offsetExists(mixed $index): bool;
  <<__Native>>
  public function offsetGet(mixed $index): mixed;
  <<__Native>>
  public function offsetSet(mixed $index,
                            mixed $newvalue): void;
  <<__Native>>
  public function offsetUnset(mixed $index): void;
  <<__Native>>
  public function count(): int;
  <<__Native>>
  public function append(mixed $newvalue): void;
}

<<__NativeData("Google\\Protobuf\\Internal\\DescriptorPool")>>
class DescriptorPool {
  <<__Native>>
  public function internalAddGeneratedFile(string $file_binary): bool;
  <<__Native>>
  public static function getGeneratedPool(): object;
}

class GPBType {
    const DOUBLE   =  1;
    const FLOAT    =  2;
    const INT64    =  3;
    const UINT64   =  4;
    const INT32    =  5;
    const FIXED64  =  6;
    const FIXED32  =  7;
    const BOOL     =  8;
    const STRING   =  9;
    const GROUP    = 10;
    const MESSAGE  = 11;
    const BYTES    = 12;
    const UINT32   = 13;
    const ENUM     = 14;
    const SFIXED32 = 15;
    const SFIXED64 = 16;
    const SINT32   = 17;
    const SINT64   = 18;
}

<<__NativeData("Google\\Protobuf\\Internal\\GPBUtil")>>
class GPBUtil {
  <<__Native>>
  public static function checkInt32(mixed $val): void;
  <<__Native>>
  public static function checkUint32(mixed $val): void;
  <<__Native>>
  public static function checkInt64(mixed $val): void;
  <<__Native>>
  public static function checkUint64(mixed $val): void;
  <<__Native>>
  public static function checkEnum(mixed $val): void;
  <<__Native>>
  public static function checkFloat(mixed $val): void;
  <<__Native>>
  public static function checkDouble(mixed $val): void;
  <<__Native>>
  public static function checkBool(mixed $val): void;
  <<__Native>>
  public static function checkString(mixed $val, bool $utf8): void;
  <<__Native>>
  public static function checkBytes(mixed $val): void;
  <<__Native>>
  public static function checkMessage(mixed $val, string $klass): void;
  <<__Native>>
  public static function checkRepeatedField(mixed $val, int $type,
      ?classname<Message> $klass = null): Variant;
  <<__Native>>
  public static function checkMapField(mixed $val, int $key_type,
      int $value_type, ?classname<Message> $klass = null): Variant;
}

}
