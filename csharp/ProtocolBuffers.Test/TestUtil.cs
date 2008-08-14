using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Google.ProtocolBuffers {
  internal static class TestUtil {

    private static DirectoryInfo testDataDirectory;

    internal static DirectoryInfo TestDataDirectory {
      get {
        if (testDataDirectory != null) {
          return testDataDirectory;
        }

        DirectoryInfo ancestor = new DirectoryInfo(".");
        // Search each parent directory looking for "src/google/protobuf".
        while (ancestor != null) {
          string candidate = Path.Combine(ancestor.FullName, "src/google/protobuf");
          if (Directory.Exists(candidate)) {
            testDataDirectory = new DirectoryInfo(candidate);
            return testDataDirectory;
          }
          ancestor = ancestor.Parent;
        }
        // TODO(jonskeet): Come up with a better exception to throw
        throw new Exception("Unable to find directory containing test files");
      }
    }
  }
}
