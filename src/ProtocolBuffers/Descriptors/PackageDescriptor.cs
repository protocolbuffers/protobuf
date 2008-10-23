using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.Descriptors {
    /// <summary>
    /// Represents a package in the symbol table.  We use PackageDescriptors
    /// just as placeholders so that someone cannot define, say, a message type
    /// that has the same name as an existing package.
    /// </summary>
    internal sealed class PackageDescriptor : IDescriptor<IMessage> {

      private readonly string name;
      private readonly string fullName;
      private readonly FileDescriptor file;

      internal PackageDescriptor(string name, string fullName, FileDescriptor file) {
        this.file = file;
        this.fullName = fullName;
        this.name = name;
      }

      public IMessage Proto {
        get { return file.Proto; }
      }

      public string Name {
        get { return name; }
      }

      public string FullName {
        get { return fullName; }
      }

      public FileDescriptor File {
        get { return file; }
      }
    }
}
