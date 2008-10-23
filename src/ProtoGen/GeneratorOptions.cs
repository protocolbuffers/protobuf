using System.Collections.Generic;
using System.IO;

namespace Google.ProtocolBuffers.ProtoGen {

  /// <summary>
  /// All the configuration required for the generator - where to generate
  /// output files, the location of input files etc. While this isn't immutable
  /// in practice, the contents shouldn't be changed after being passed to
  /// the generator.
  /// </summary>
  public sealed class GeneratorOptions {

    public string OutputDirectory { get; set; }
    public IList<string> InputFiles { get; set; }

    /// <summary>
    /// Attempts to validate the options, but doesn't throw an exception if they're invalid.
    /// Instead, when this method returns false, the output variable will contain a collection
    /// of reasons for the validation failure.
    /// </summary>
    /// <param name="reasons">Variable to receive a list of reasons in case of validation failure.</param>
    /// <returns>true if the options are valid; false otherwise</returns>
    public bool TryValidate(out IList<string> reasons) {
      List<string> tmpReasons = new List<string>();

      // Output directory validation
      if (string.IsNullOrEmpty(OutputDirectory)) {
        tmpReasons.Add("No output directory specified");
      } else {
        if (!Directory.Exists(OutputDirectory)) {
          tmpReasons.Add("Specified output directory (" + OutputDirectory + " doesn't exist.");
        }
      }

      // Input file validation (just in terms of presence)
      if (InputFiles == null || InputFiles.Count == 0) {
        tmpReasons.Add("No input files specified");
      } else {
        foreach (string input in InputFiles) {
          FileInfo fi = new FileInfo(input);
          if (!fi.Exists) {
            tmpReasons.Add("Input file " + input + " doesn't exist.");
          }
        }
      }

      if (tmpReasons.Count != 0) {
        reasons = tmpReasons;
        return false;
      }

      reasons = null;
      return true;
    }

    /// <summary>
    /// Validates that all the options have been set and are valid,
    /// throwing an exception if they haven't.
    /// </summary>
    /// <exception cref="InvalidOptionsException">The options are invalid.</exception>
    public void Validate() {
      IList<string> reasons;
      if (!TryValidate(out reasons)) {
        throw new InvalidOptionsException(reasons);
      }
    }
  }
}
