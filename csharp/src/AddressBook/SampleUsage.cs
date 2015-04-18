using System;
using System.IO;

namespace Google.ProtocolBuffers.Examples.AddressBook
{
    internal class SampleUsage
    {
        private static void Main()
        {
            byte[] bytes;
            //Create a builder to start building a message
            Person.Builder newContact = Person.CreateBuilder();
            //Set the primitive properties
            newContact.SetId(1)
                .SetName("Foo")
                .SetEmail("foo@bar");
            //Now add an item to a list (repeating) field
            newContact.AddPhone(
                //Create the child message inline
                Person.Types.PhoneNumber.CreateBuilder().SetNumber("555-1212").Build()
                );
            //Now build the final message:
            Person person = newContact.Build();
            //The builder is no longer valid (at least not now, scheduled for 2.4):
            newContact = null;
            using (MemoryStream stream = new MemoryStream())
            {
                //Save the person to a stream
                person.WriteTo(stream);
                bytes = stream.ToArray();
            }
            //Create another builder, merge the byte[], and build the message:
            Person copy = Person.CreateBuilder().MergeFrom(bytes).Build();

            //A more streamlined approach might look like this:
            bytes = AddressBook.CreateBuilder().AddPerson(copy).Build().ToByteArray();
            //And read the address book back again
            AddressBook restored = AddressBook.CreateBuilder().MergeFrom(bytes).Build();
            //The message performs a deep-comparison on equality:
            if (restored.PersonCount != 1 || !person.Equals(restored.PersonList[0]))
                throw new ApplicationException("There is a bad person in here!");
        }
    }
}