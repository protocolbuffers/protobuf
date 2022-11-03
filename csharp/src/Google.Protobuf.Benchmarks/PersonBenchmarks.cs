using BenchmarkDotNet.Attributes;
using Google.Protobuf.Examples.AddressBook;
using Google.Protobuf.WellKnownTypes;

namespace Google.Protobuf.Benchmarks;

public class PersonBenchmarks
{
    private static readonly byte[] serializedEmptyPerson = new Person().ToByteArray();

    private static readonly byte[] serializedPopulatedPerson = new Person
    {
        Email = "noone@nowhere.com",
        Id = 5,
        LastUpdated = Timestamp.FromDateTime(new DateTime(2022, 11, 3, 10, 29, 0, 123, DateTimeKind.Utc)),
        Name = "Someone",
        Phones =
        {
            { new Person.Types.PhoneNumber { Number = "12345", Type = Person.Types.PhoneType.Home } },
            { new Person.Types.PhoneNumber { Number = "54321", Type = Person.Types.PhoneType.Mobile } }
        }
    }.ToByteArray();

    [Benchmark]
    public Person Constructor() => new Person();

    [Benchmark]
    public Person ParseEmpty() => Person.Parser.ParseFrom(serializedEmptyPerson);

    [Benchmark]
    public Person ParsePopulated() => Person.Parser.ParseFrom(serializedPopulatedPerson);
}
