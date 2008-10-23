using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers.ProtoGen {
  /// <summary>
  /// Exception thrown to indicate that the options passed were invalid.
  /// </summary>
  public sealed class InvalidOptionsException : Exception {

    private readonly IList<string> reasons;

    /// <summary>
    /// An immutable list of reasons why the options were invalid.
    /// </summary>
    public IList<string> Reasons {
      get { return reasons; }
    }

    public InvalidOptionsException(IList<string> reasons) 
        : base(BuildMessage(reasons)) {
      this.reasons = Lists.AsReadOnly(reasons);
    }

    private static string BuildMessage(IEnumerable<string> reasons) {
      StringBuilder builder = new StringBuilder("Invalid options:");
      builder.AppendLine();
      foreach (string reason in reasons) {
        builder.Append("  ");
        builder.AppendLine(reason);
      }
      return builder.ToString();
    }
  }
}
