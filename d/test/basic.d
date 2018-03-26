
import google.protobuf;

// ------------- generated code --------------

class Foo
{
    @Proto(1) Bar bar = defaultValue!Bar;
    @Proto(2) Baz baz = defaultValue!Baz;
}

class Bar
{
    @Proto(1) string msg = defaultValue!string;
}

class Baz
{
    @Proto(1) string msg = defaultValue!string;
}

class TestMessage
{
    @Proto(1) int optionalInt32 = defaultValue!int;
    @Proto(2) long optionalInt64 = defaultValue!long;
    @Proto(3) uint optionalUint32 = defaultValue!uint;
    @Proto(4) ulong optionalUint64 = defaultValue!ulong;
    @Proto(5) bool optionalBool = defaultValue!bool;
    @Proto(6) float optionalFloat = defaultValue!float;
    @Proto(7) double optionalDouble = defaultValue!double;
    @Proto(8) string optionalString = defaultValue!string;
    @Proto(9) bytes optionalBytes = defaultValue!bytes;
    @Proto(10) TestMessage2 optionalMsg = defaultValue!TestMessage2;
    @Proto(11) TestEnum optionalEnum = defaultValue!TestEnum;

    @Proto(12) int[] repeatedInt32 = defaultValue!(int[]);
    @Proto(13) long[] repeatedInt64 = defaultValue!(long[]);
    @Proto(14) uint[] repeatedUint32 = defaultValue!(uint[]);
    @Proto(15) ulong[] repeatedUint64 = defaultValue!(ulong[]);
    @Proto(16) bool[] repeatedBool = defaultValue!(bool[]);
    @Proto(17) float[] repeatedFloat = defaultValue!(float[]);
    @Proto(18) double[] repeatedDouble = defaultValue!(double[]);
    @Proto(19) string[] repeatedString = defaultValue!(string[]);
    @Proto(20) bytes[] repeatedBytes = defaultValue!(bytes[]);
    @Proto(21) TestMessage2[] repeatedMsg = defaultValue!(TestMessage2[]);
    @Proto(22) TestEnum[] repeatedEnum = defaultValue!(TestEnum[]);
}
class TestMessage2
{
    @Proto(1) int foo = defaultValue!int;
}

class Recursive1
{
    @Proto(1) Recursive2 foo = defaultValue!Recursive2;
}
class Recursive2
{
    @Proto(1) Recursive1 foo = defaultValue!Recursive1;
}

enum TestEnum
{
    Default = 0,
    A = 1,
    B = 2,
    C = 3,
}

class MapMessage
{
    @Proto(1) int[string] mapStringInt32 = defaultValue!(int[string]);
    @Proto(2) TestMessage2[string] mapStringMsg = defaultValue!(TestMessage2[string]);
}
class MapMessageWireEquiv
{
    @Proto(1) MapMessageWireEquivEntry1[] mapStringInt32 = defaultValue!(MapMessageWireEquivEntry1[]);
    @Proto(2) MapMessageWireEquivEntry2[] mapStringMsg = defaultValue!(MapMessageWireEquivEntry2[]);
}
class MapMessageWireEquivEntry1
{
    @Proto(1) string key = defaultValue!string;
    @Proto(2) int value = defaultValue!int;
}
class MapMessageWireEquivEntry2
{
    @Proto(1) string key = defaultValue!string;
    @Proto(2) TestMessage2 value = defaultValue!TestMessage2;
}

class OneofMessage
{
    enum MyOneofCase
    {
        myOneofNotSet = 0,
        a = 1,
        b = 2,
        c = 3,
        d = 4,
    }
    MyOneofCase _myOneofCase = MyOneofCase.myOneofNotSet;
    @property MyOneofCase myOneofCase() { return _myOneofCase; }
    void clearMyOneof() { _myOneofCase = MyOneofCase.myOneofNotSet; }
    @Oneof("_myOneofCase") union
    {
        @Proto(1) string _a = defaultValue!string; mixin(oneofAccessors!_a);
        @Proto(2) int _b; mixin(oneofAccessors!_b);
        @Proto(3) TestMessage2 _c; mixin(oneofAccessors!_c);
        @Proto(4) TestEnum _d; mixin(oneofAccessors!_d);
    }
}


// ------------ test cases ---------------

// test defaults
unittest
{
    auto m = new TestMessage;
    assert(m.optionalInt32 == 0);
    assert(m.optionalInt64 == 0);
    assert(m.optionalUint32 == 0);
    assert(m.optionalUint64 == 0);
    assert(m.optionalBool == false);
    assert(m.optionalFloat == 0.0);
    assert(m.optionalDouble == 0.0);
    assert(m.optionalString == "");
    assert(m.optionalBytes == "");
    assert(m.optionalMsg is null);
    assert(m.optionalEnum == TestEnum.Default);
}

// test setters
unittest
{
    auto m = new TestMessage;
    m.optionalInt32 = -42;
    assert(m.optionalInt32 == -42);
    m.optionalInt64 = -0x1_0000_0000;
    assert(m.optionalInt64 == -0x1_0000_0000);
    m.optionalUint32 = 0x9000_0000;
    assert(m.optionalUint32 == 0x9000_0000);
    m.optionalUint64 = 0x9000_0000_0000_0000;
    assert(m.optionalUint64 == 0x9000_0000_0000_0000);
    m.optionalBool = true;
    assert(m.optionalBool == true);
    m.optionalFloat = 0.5;
    assert(m.optionalFloat == 0.5);
    m.optionalDouble = 0.5;
    m.optionalString = "hello";
    assert(m.optionalString == "hello");
    m.optionalBytes = cast(bytes) "world";
    assert(m.optionalBytes == "world");
    m.optionalMsg = new TestMessage2;
    m.optionalMsg.foo = 42;
    assert(m.optionalMsg.foo == 42);
    m.optionalMsg = null;
    assert(m.optionalMsg is null);
}

// test parent repeated field
unittest
{
    // make sure we set the RepeatedField and can add to it
    auto m = new TestMessage;
    assert(m.repeatedString is null);
    m.repeatedString ~= "ok";
    m.repeatedString ~= "ok2";
    assert(m.repeatedString == ["ok", "ok2"]);
    m.repeatedString ~= ["ok3"];
    assert(m.repeatedString == ["ok", "ok2", "ok3"]);
}

// test map field
unittest
{
    import std.algorithm : sort;
    import std.array : array;

    auto m = new MapMessage;
    assert(m.mapStringInt32 is null);
    assert(m.mapStringMsg is null);
    m.mapStringInt32 = ["a": 1, "b": 2];
    auto a = new TestMessage2;
    a.foo = 1;
    auto b = new TestMessage2;
    b.foo = 2;
    m.mapStringMsg = ["a": a, "b": b];
    assert(m.mapStringInt32.keys.sort().array == ["a", "b"]);
    assert(m.mapStringInt32["a"] == 1);
    assert(m.mapStringMsg["b"].foo == 2);

    m.mapStringInt32["c"] = 3;
    assert(m.mapStringInt32["c"] == 3);
    auto c = new TestMessage2;
    c.foo = 3;
    m.mapStringMsg["c"] = c;
    m.mapStringMsg.remove("b");
    m.mapStringMsg.remove("c");
    assert(m.mapStringMsg.keys == ["a"]);
}

// test oneof
unittest
{
    auto d = new OneofMessage;
    assert(d.a == defaultValue!(string));
    assert(d.b == defaultValue!(int));
    assert(d.c == defaultValue!(TestMessage2));
    assert(d.d == defaultValue!(TestEnum));
    assert(d.myOneofCase == OneofMessage.MyOneofCase.myOneofNotSet);

    d.a = "foo";
    assert(d.a == "foo");
    assert(d.b == defaultValue!(int));
    assert(d.c == defaultValue!(TestMessage2));
    assert(d.d == defaultValue!(TestEnum));
    assert(d.myOneofCase == OneofMessage.MyOneofCase.a);

    d.b = 42;
    assert(d.a == defaultValue!(string));
    assert(d.b == 42);
    assert(d.c == defaultValue!(TestMessage2));
    assert(d.d == defaultValue!(TestEnum));
    assert(d.myOneofCase == OneofMessage.MyOneofCase.b);

    auto m = new TestMessage2;
    m.foo = 42;
    d.c = m;
    assert(d.a == defaultValue!(string));
    assert(d.b == defaultValue!(int));
    assert(d.c.foo == 42);
    assert(d.d == defaultValue!(TestEnum));
    assert(d.myOneofCase == OneofMessage.MyOneofCase.c);

    d.d = TestEnum.B;
    assert(d.a == defaultValue!(string));
    assert(d.b == defaultValue!(int));
    assert(d.c == defaultValue!(TestMessage2));
    assert(d.d == TestEnum.B);
    assert(d.myOneofCase == OneofMessage.MyOneofCase.d);
}
