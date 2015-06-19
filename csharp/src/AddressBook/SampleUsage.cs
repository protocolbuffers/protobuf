using Google.Protobuf;
using System;
using System.IO;

namespace Google.ProtocolBuffers.Examples.AddressBook
{
    internal class SampleUsage
    {
        private static void Main()
        {
            byte[] bytes;
            // Create a new person
            Person person = new Person
            {
                Id = 1,
                Name = "Foo",
                Email = "foo@bar",
                Phone = { new Person.Types.PhoneNumber { Number = "555-1212" } }
            };
            using (MemoryStream stream = new MemoryStream())
            {
                // Save the person to a stream
                person.WriteTo(stream);
                bytes = stream.ToArray();
            }
            Person copy = Person.Parser.ParseFrom(bytes);

            // A more streamlined approach might look like this:
            bytes = copy.ToByteArray();
            // And read the address book back again
            AddressBook restored = AddressBook.Parser.ParseFrom(bytes);
            // The message performs a deep-comparison on equality:
            if (restored.Person.Count != 1 || !person.Equals(restored.Person[0]))
            {
                throw new ApplicationException("There is a bad person in here!");
            }
        }
    }
}