module google.protobuf.common;

import std.typecons : Flag;

alias bytes = ubyte[];

auto defaultValue(T)()
{
    import std.traits : isFloatingPoint;

    static if (isFloatingPoint!T)
        return cast(T) 0.0;
    else
        return T.init;
}

class ProtobufException : Exception
{
    this(string message = null, string file = __FILE__, size_t line = __LINE__,
        Throwable next = null) @safe pure nothrow
    {
        super(message, file, line, next);
    }
}

struct Proto
{
    uint tag;
    string wire;
    Flag!"packed" packed;
}

template protoByField(alias field)
{
    import std.traits : getUDAs;

    enum Proto protoByField = getUDAs!(field, Proto)[0];
}

struct Oneof
{
    string caseFieldName;
}

template isOneof(alias field)
{
    import std.traits : hasUDA;

    enum bool isOneof = hasUDA!(field, Oneof);
}

template oneofCaseFieldName(alias field)
{
    import std.traits : getUDAs;

    static assert(isOneof!field);

    enum string oneofCaseFieldName = getUDAs!(field, Oneof)[0].caseFieldName;
}

template oneofAccessorName(alias field)
{
    static string accessorName()
    {
        import std.algorithm : skipOver;

        static assert(__traits(identifier, field)[0] == '_', "Invalid oneof union member name");

        string fieldName = __traits(identifier, field);
        bool result = fieldName.skipOver('_');
        assert(result);

        return fieldName;
    }

    enum string oneofAccessorName = accessorName;
}

template oneofAccessors(alias field)
{
    static string generateAccessors()
    {
        import std.string : format;

        enum accessorName = oneofAccessorName!field;
        
        return "
            %1$s %2$s() { return %3$s == typeof(%3$s).%2$s ? _%2$s : (%1$s).init; }
            void %2$s(%1$s _) { _%2$s = _; %3$s = typeof(%3$s).%2$s; }
            ".format(typeof(field).stringof, accessorName, oneofCaseFieldName!field);
    }

    enum oneofAccessors = generateAccessors;
}

static struct Message(T)
{
    import std.meta : allSatisfy, staticMap, staticSort;
    import std.traits : getSymbolsByUDA;

    static assert(fields.length > 0, "Definition of '" ~ T.stringof ~ "' has no Proto field");
    static assert(allSatisfy!(validateField, fields));

    alias fields = staticSort!(Less, unsortedFields);
    alias protos = staticMap!(protoByField, fields);

    alias fieldNames = staticMap!(fieldName, fields);

    private alias unsortedFields = getSymbolsByUDA!(T, Proto);
    private static enum fieldName(alias field) = __traits(identifier, field);
    private static enum Less(alias field1, alias field2) = protoByField!field1.tag < protoByField!field2.tag;
}

unittest
{
    import std.meta : AliasSeq;
    import std.typecons : No;

    static class Test
    {
        @Proto(3) int foo;
        @Proto(2, "fixed") int bar;
    }

    static assert(Message!Test.fieldNames == AliasSeq!("bar", "foo"));
    static assert(Message!Test.protos == AliasSeq!(Proto(2, "fixed", No.packed), Proto(3, "", No.packed)));
}

template validateField(alias field)
{
    import std.traits : getUDAs;

    enum validateField = validateProto!(getUDAs!(field, Proto)[0], typeof(field));
}

bool validateProto(Proto proto, T)()
{
    import std.range : ElementType;
    import std.traits : isAggregateType, isArray, isAssociativeArray, isBoolean, isFloatingPoint, isIntegral;
    import std.traits : KeyType, ValueType;

    static assert(proto.tag > 0 && proto.tag < (2 << 29));

    static if (isBoolean!T)
    {
        static assert(!proto.packed);
        static assert(proto.wire == "");
    }
    else static if (is(T == int) || is(T == uint) || is(T == long) || is(T == ulong))
    {
        static assert(!proto.packed);
        static assert(proto.wire == "" || proto.wire == "fixed" || proto.wire == "zigzag");
    }
    else static if (is(T == enum) && is(T : int))
    {
        static assert(!proto.packed);
        static assert(proto.wire == "");
    }
    else static if (isFloatingPoint!T)
    {
        static assert(!proto.packed);
        static assert(proto.wire == "");
    }
    else static if (is(T == string) || is(T == bytes))
    {
        static assert(!proto.packed);
        static assert(proto.wire == "");
    }
    else static if (isArray!T)
    {
        static assert(is(ElementType!T == string) || is(ElementType!T == bytes)
            || (!isArray!(ElementType!T) && !isAssociativeArray!(ElementType!T)));
        enum elementProto = Proto(proto.tag, proto.wire);

        static assert(validateProto!(elementProto, ElementType!T));
    }
    else static if (isAssociativeArray!T)
    {
        import std.algorithm : findSplit;

        static assert(!proto.packed);
        static assert(isBoolean!(KeyType!T) || isIntegral!(KeyType!T) || is(KeyType!T == string));
        static assert(is(ValueType!T == string) || is(ValueType!T == bytes)
            || (!isArray!(ValueType!T) && !isAssociativeArray!(ValueType!T)));

        enum wires = proto.wire.findSplit(",");

        enum keyProto = Proto(1, wires[0]);
        static assert(validateProto!(keyProto, KeyType!T));

        enum valueProto = Proto(2, wires[2]);
        static assert(validateProto!(valueProto, ValueType!T));
    }
    else static if (isAggregateType!T)
    {
        static assert(!proto.packed);
        static assert(proto.wire == "");
    }
    else
    {
        static assert(0, "Invalid Proto definition for type " ~ T.stringof);
    }

    return true;
}
