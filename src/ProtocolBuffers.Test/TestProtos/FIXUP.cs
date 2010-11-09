using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.TestProtos {
  public sealed partial class TestExtremeDefaultValues { 
  
#warning ToDo - These values are not currently handled by the generator...
    const double InfinityD = double.PositiveInfinity;
    const double NaND = double.NaN;
    const float InfinityF = float.PositiveInfinity;
    const float NaNF = float.NaN;
  }
}
